/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "comm.h"
#include "utils.h"
#include "config.h"
#include "maputils.h"
#include "ban.h"
#include "weather.h"
#include "act.wizard.h"
#include "act.misc.h"
#include "house.h"
#include "act.other.h"
#include "dg_comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "act.item.h"
#include "interpreter.h"
#include "random.h"
#include "act.informative.h"
#include "dg_event.h"
#include "mobact.h"
#include "magic.h"
#include "imc.h"
#include "objsave.h"
#include "genolc.h"
#include "class.h"
#include "combat.h"
#include "modify.h"
#include "fight.h"
#include "local_limits.h"
#include "clan.h"
#include "mail.h"
#include "constants.h"
#include "screen.h"
#include "area.h"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "telnet.h"
#include "sodium.h"
#include <thread>

/* local globals */
struct descriptor_data *descriptor_list = nullptr;        /* master desc list */
struct txt_block *bufpool = nullptr;    /* pool of large output buffers */
int buf_largecount = 0;        /* # of large buffers which exist */
int buf_overflows = 0;        /* # of overflows of output */
int buf_switches = 0;        /* # of switches from small to large buf */
int circle_shutdown = 0;    /* clean shutdown */
int circle_reboot = 0;        /* reboot the game after a shutdown */
int no_specials = 0;        /* Suppress ass. of special routines */
int max_players = 0;        /* max descriptors available */
int tics_passed = 0;        /* for extern checkpointing */
int scheck = 0;            /* for syntax checking mode */
struct timeval null_time;    /* zero-valued time structure */
int8_t reread_wizlist;        /* signal: SIGUSR1 */
int8_t emergency_unban;        /* signal: SIGUSR2 */
FILE *logfile = nullptr;        /* Where to send the log messages. */
int dg_act_check;               /* toggle for act_trigger */
unsigned long pulse = 0;        /* number of pulses since game start */
bool fCopyOver;          /* Are we booting in copyover mode? */
uint16_t port;
socklen_t mother_desc;
char *last_act_message = nullptr;

/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

void broadcast(const std::string& txt) {
    log("Broadcasting: %s", txt.c_str());
    for(auto &[cid, c] : net::connections) {
        c.sendText(txt);
    }
}

boost::asio::awaitable<void> signal_watcher() {
    while(true) {
        try {
            auto result = net::signals->async_wait(boost::asio::use_awaitable);
        } catch(...) {
            // TODO: improve this.
        }
    }
}

static boost::asio::awaitable<void> recoverTelnet(uint16_t socket, struct descriptor_data *d, const nlohmann::json& j) {
    boost::beast::tcp_stream conn(*net::io, boost::asio::ip::tcp::v4(), socket);
    auto tel = new net::telnet_data(std::move(conn));
    tel->desc = d;
    d->conn = tel;
    d->conn->deserialize(j);
    tel->state = net::ConnState::Running;
    {
        std::lock_guard<std::mutex> guard(net::connectionsMutex);
        net::connections[socket] = d->conn;
    }

    co_await d->conn->run();

    {
        std::lock_guard<std::mutex> guard(net::connectionsMutex);
        net::connections.erase(socket);
    }

    co_return;
}

static boost::asio::awaitable<void> recoverHttp(uint16_t socket, struct descriptor_data *d, const nlohmann::json& j) {
    // TODO: Implement the rest of it...

    co_return;
}

static boost::asio::awaitable<void> recoverConnection(const nlohmann::json j) {
    auto d = new descriptor_data();
    d->raw_input_queue = std::make_unique<Channel<std::string>>(*io, 200);

    auto socket = j["socket"].get<uint16_t>();
    auto protocol = j["protocol"].get<net::Protocol>();
	d->obj_was = strdup(j["user"].get<std::string>().c_str());
    d->obj_name = strdup(j["character"].get<std::string>().c_str());

    d->connected = CON_COPYOVER;

    /* Now, find the pfile */

    d->obj_editval = NOWHERE;
    if(j.contains("in_room")) d->obj_editval = j["in_room"].get<room_vnum>();

    d->next = descriptor_list;
    descriptor_list = d;

    // We are committed. what happens next must not fail.
	if(protocol == net::Protocol::Telnet) {
        co_await recoverTelnet(socket, d, j["conn"]);
    } else {
        co_await recoverHttp(socket, d, j["conn"]);
    }
    co_return;
}

void copyover_recover_final() {
    struct descriptor_data *next_d;
    for(auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if(STATE(d) != CON_COPYOVER) continue;

		std::string user = d->obj_was;
        std::string character = d->obj_name;
        room_vnum room = d->obj_editval;

        free(d->obj_was);
        d->obj_was = nullptr;
        free(d->obj_name);
        d->obj_name = nullptr;

        userLoad(d, (char*)user.c_str());
        if(!d->user) {
            log("recoverConnection: user %s not found.", user.c_str());
            close_socket(d);
            continue;
        }

        auto c = new char_data();
        c->player_specials = new player_special_data();
        d->character = c;
        auto player_i = load_char(character.c_str(), c);
        if(player_i < 0 || PLR_FLAGGED(c, PLR_DELETED)) {
            log("recoverConnection: character %s not found.", character.c_str());
            close_socket(d);
            continue;
        }
        c->desc = d;

		GET_WAS_IN(c) = room;
        GET_PFILEPOS(c) = player_i;
        REMOVE_BIT_AR(PLR_FLAGS(c), PLR_WRITING);
        REMOVE_BIT_AR(PLR_FLAGS(c), PLR_MAILING);
        REMOVE_BIT_AR(PLR_FLAGS(c), PLR_CRYO);

        write_to_output(d, "@rThe world comes back into focus... has something changed?@n\n\r");

        enter_player_game(d);
        d->connected = CON_PLAYING;
        look_at_room(IN_ROOM(d->character), d->character, 0);
        if (AFF_FLAGGED(d->character, AFF_HAYASA)) {
            GET_SPEEDBOOST(d->character) = GET_SPEEDCALC(d->character) * 0.5;
        }
    }
}

boost::asio::awaitable<void> yield_for(std::chrono::milliseconds ms) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, ms);
    timer.expires_after(ms);
    co_await timer.async_wait(boost::asio::use_awaitable);
    co_return;
}

/* Reload players after a copyover */
void copyover_recover() {
    log("Copyover recovery initiated");
    std::ifstream fp(COPYOVER_FILE);

    if(!fp.is_open()) {
        log("Copyover file not found. Exitting.\n\r");
        exit(1);
    }

    nlohmann::json j;
    fp >> j;

    // erase the file.
    unlink(COPYOVER_FILE);
    fp.close();

    if(j.contains("connections")) {
        for(const auto& c : j["connections"]) {
            co_spawn(boost::asio::make_strand(*net::io), recoverConnection(c), boost::asio::detached);
        }
    }
}

int get_max_players() {

    int max_descs = 0;
    const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
    {
        struct rlimit limit;

        /* find the limit of file descs */
        method = "rlimit";
        if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
            perror("SYSERR: calling getrlimit");
            exit(1);
        }

        /* set the current to the maximum */
        limit.rlim_cur = limit.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
            perror("SYSERR: calling setrlimit");
            exit(1);
        }
        if (limit.rlim_max == RLIM_INFINITY)
            max_descs = CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS;
        else
            max_descs = MIN(CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS, limit.rlim_max);
    }

    /* now calculate max _players_ based on max descs */
    max_descs = MIN(CONFIG_MAX_PLAYING, max_descs - NUM_RESERVED_DESCS);

    if (max_descs <= 0) {
        log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
            max_descs, method);
        exit(1);
    }
    log("   Setting player limit to %d using %s.", max_descs, method);
    return (max_descs);
}

static boost::asio::awaitable<void> performReboot(int mode) {
    struct descriptor_data *d, *d_next;
    char buf[100], buf2[100];

    std::ofstream fp(COPYOVER_FILE);

    if (!fp.is_open()) {
        send_to_imm("Copyover file not writeable, aborted.\n\r");
        circle_reboot = 0;
        co_return;
    }

    if(mode != 2) {
        save_all();
    }

    sprintf(buf, "\t\x1B[1;31m \007\007\007The universe stops for a moment as space and time fold.\x1B[0;0m\r\n");
    save_mud_time(&time_info);

    nlohmann::json j;

    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d; d = d_next) {
        auto och = d->character;
        d_next = d->next; /* We delete from the list , so need to save this */
        if (!och || STATE(d) > CON_PLAYING) {
            d->conn->sendText("\n\rSorry, we are rebooting. Come back in a few seconds.\n\r");
            close_socket(d); /* throw'em out */
        }
        d->conn->sendText(buf);
        /* save och */
        Crash_rentsave(och, 0);
        save_char(och);
    }

    // wait 200 milliseconds... that should be enough time to push out all of the data.
    auto hb = std::chrono::milliseconds(200);
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, hb);
    co_await timer.async_wait(boost::asio::use_awaitable);

    /* For each descriptor/connection, halt them and save state. */
    for (d = descriptor_list; d; d = d_next) {
        co_await d->conn->halt(mode);
        auto och = d->character;
        nlohmann::json jd;

        jd["socket"] = d->conn->socket;
        jd["protocol"] = d->conn->capabilities.protocol;
        jd["user"] = d->user;
        jd["character"] = GET_NAME(och);

        auto r = IN_ROOM(och);
        auto w = GET_WAS_IN(och);
        if(r > 1) {
            jd["in_room"] = r;
        } else if(r <= 1 && w > 1) {
        	jd["in_room"] = w;
        }

        jd["conn"] = d->conn->serialize();

        j["connections"].push_back(jd);

    }
    j["port"] = port;
    j["mother_desc"] = mother_desc;

    fp << j.dump(4, ' ', false, nlohmann::json::error_handler_t::replace) << std::endl;
    fp.close();

	co_return;

}

boost::asio::awaitable<void> heartbeat(int heart_pulse, double deltaTime) {
    static int mins_since_crashsave = 0;

    event_process();

    if (!(heart_pulse % PULSE_DG_SCRIPT))
        script_trigger_check();

    if (!(heart_pulse % PULSE_ZONE))
        zone_update();

    if (!(heart_pulse % PULSE_IDLEPWD))        /* 15 seconds */
        check_idle_passwords();

    if (!(heart_pulse % (PULSE_1SEC * 60)))           /* 15 seconds */
        check_idle_menu();

    if (!(heart_pulse % (PULSE_IDLEPWD / 15))) {           /* 1 second */
        dball_load();
    }
    if (!(heart_pulse % (PULSE_2SEC))) {
        base_update();
        fish_update();
    }

    if (!(heart_pulse % (PULSE_1SEC * 15))) {
        handle_songs();
    }

    if (!(heart_pulse % (PULSE_1SEC)))
        wishSYS();

    if (!(heart_pulse % PULSE_MOBILE))
        mobile_activity();

    if (!(heart_pulse % PULSE_AUCTION))
        check_auction();

    if (!(heart_pulse % (PULSE_IDLEPWD / 15))) {
        fight_stack();
    }
    if (!(heart_pulse % ((PULSE_IDLEPWD / 15) * 2))) {
        if (rand_number(1, 2) == 2) {
            homing_update();
        }
        huge_update();
        broken_update();
        /*update_mob_absorb();*/
    }

    if (!(heart_pulse % (1 * PASSES_PER_SEC))) { /* EVERY second */
        copyover_check();
    }

    if (!(heart_pulse % PULSE_VIOLENCE)) {
        affect_update_violence();
    }

    if (!(heart_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
        weather_and_time(1);
        check_time_triggers();
        affect_update();
    }
    if (!(heart_pulse % ((SECS_PER_MUD_HOUR / 3) * PASSES_PER_SEC))) {
        point_update();
    }

    if (CONFIG_AUTO_SAVE && !(heart_pulse % PULSE_AUTOSAVE)) {    /* 1 minute */
        clan_update();
        if (++mins_since_crashsave >= CONFIG_AUTOSAVE_TIME) {
            mins_since_crashsave = 0;
            Crash_save_all();
            House_save_all();
        }
    }

    if (!(heart_pulse % PULSE_USAGE))
        record_usage();

    if (!(heart_pulse % PULSE_TIMESAVE))
        save_mud_time(&time_info);

    if (!(heart_pulse % (30 * PASSES_PER_SEC))) {
        timed_dt(nullptr);
    }

    /* Every pulse! Don't want them to stink the place up... */
    extract_pending_chars();
    co_return;
}

boost::asio::awaitable<void> runOneLoop(double deltaTime) {
    static bool sleeping = false;
    struct descriptor_data* next_d;

    while(net::pending_descriptors.ready()) {
        struct descriptor_data* d;
        if(!net::pending_descriptors.try_receive(d)) continue;
        d->next = descriptor_list;
        descriptor_list = d;
        d->character->login();
    }

    if(sleeping && descriptor_list) {
        log("Waking up.");
        sleeping = false;
    }

    if(!descriptor_list) {
        if(!sleeping) {
            log("No connections.  Going to sleep.");
            sleeping = true;
        }
        co_return;
    }

    /* Process commands we just read from process_input */
    for (auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        d->handle_input();
    }

    bool gameActive = false;
    // TODO: replace this with a smarter check...
    // to determine if the game is active, we need to check if there are any players in the game.
    // this will be the case if any descriptor has an attached character who's in a valid room.
    for(auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if(d->character && IN_ROOM(d->character) != NOWHERE) {
            gameActive = true;
            break;
        }
    }

    if(gameActive) {
        co_await heartbeat(++pulse, deltaTime);
    }

    /* Send queued output out to the operating system (ultimately to user). */
    for (auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if (!d->output.empty()) {
            process_output(d);
            d->has_prompt = true;
        }
    }

    /* Print prompts for other descriptors who had no other output */
    for (auto d = descriptor_list; d; d = d->next) {
        if (!d->has_prompt) {
            write_to_output(d, "@n");
            process_output(d);
            d->has_prompt = true;
        }
    }

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
            close_socket(d);
    }

    /* Check for any signals we may have received. */
    if (reread_wizlist) {
        reread_wizlist = false;
        mudlog(CMP, ADMLVL_IMMORT, true, "Signal received - rereading wizlists.");
        reboot_wizlists();
    }
    if (emergency_unban) {
        emergency_unban = false;
        mudlog(BRF, ADMLVL_IMMORT, true, "Received SIGUSR2 - completely unrestricting game (emergent)");
        ban_list = nullptr;
        circle_restrict = 0;
        num_invalid = 0;
    }

    tics_passed++;
	co_return;
}


/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
boost::asio::awaitable<void> game_loop() {
    auto hb = std::chrono::milliseconds(100);
    auto previousTime = boost::asio::steady_timer::clock_type::now();
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, hb);

    /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
    while (!circle_shutdown) {

        auto timeStart = boost::asio::steady_timer::clock_type::now();
        co_await timer.async_wait(boost::asio::use_awaitable);
        auto timeEnd = boost::asio::steady_timer::clock_type::now();

        auto deltaTime = timeEnd - timeStart;
        double deltaTimeInSeconds = std::chrono::duration<double>(deltaTime).count();

        try {
            SQLite::Transaction transaction(*db);
            co_await runOneLoop(deltaTimeInSeconds);
            process_dirty();
            transaction.commit();
        } catch(std::exception& e) {
            log("Exception in runOneLoop(): %s", e.what());
        }

        auto timeAfterHeartbeat = boost::asio::steady_timer::clock_type::now();
        auto elapsed = timeAfterHeartbeat - timeStart;
        auto nextWait = hb - elapsed;

        // If heartbeat takes more than 100ms, default to a very short wait
        if(nextWait.count() < 0) {
            nextWait = std::chrono::milliseconds(1);
        }

        timer.expires_from_now(nextWait);
    }

    if(circle_reboot > 0) {
        // circle_reboot at 1 is copyover, 2 is a full reboot.
        co_await performReboot(circle_reboot);
    }
	net::io->stop();
    co_return;
}

std::function<boost::asio::awaitable<void>()> gameFunc;

static void finish_copyover() {
    char buf[100], buf2[100];
    sprintf(buf, "%d", port);
    sprintf(buf2, "-C%d", mother_desc);
    chdir("..");
    execl("bin/circle", "circle", buf2, buf, (char *) nullptr);
    /* Failed - sucessful exec will not return */

    perror("do_copyover: execl");
    log("Copyover FAILED!\n\r");

    exit(1); /* too much trouble to try to recover! */
}

static boost::asio::awaitable<void> runGame() {
	// instantiate db with a shared_ptr, the filename is dbat.sqlite3
    try {
        db = std::make_shared<SQLite::Database>("dbat.sqlite3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    } catch (std::exception &e) {
        log("Exception in runGame(): %s", e.what());
        exit(1);
    }

    circle_srandom(time(nullptr));
    event_init();

    if(fCopyOver) {
        copyover_recover();
    }

    co_await boot_db();

    if (CONFIG_IMC_ENABLED) {
        imc_startup(false, -1, false); // FALSE arg, so the autoconnect setting can govern it.
    }

    {
        broadcast("Loading Space Map. ");
        FILE *mapfile = fopen("../lib/surface.map", "r");
        int rowcounter, colcounter;
        int vnum_read;
        for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
            for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
                fscanf(mapfile, "%d", &vnum_read);
                mapnums[rowcounter][colcounter] = real_room(vnum_read);
            }
        }
        fclose(mapfile);
    }

    /* Load the toplist */
    topLoad();

    /* If we made it this far, we will be able to restart without problem. */
    remove(KILLSCRIPT_FILE);

    // bring anyone who's in the middle of a copyover back into the game.
    if(fCopyOver) {
        copyover_recover_final();
    }

    // Finally, let's get the game cracking.
    if(gameFunc) co_await gameFunc();
    else co_await game_loop();

    co_return;
}

/* Init sockets, run game, and cleanup sockets */
void init_game(uint16_t cmport) {
    /* We don't want to restart if we crash before we get up. */
    touch(KILLSCRIPT_FILE);

    if (sodium_init() == -1) {
        log("Could not initialize libsodium!");
        exit(1);
    }

    if(!net::io) net::io = std::make_unique<boost::asio::io_context>();
    if(!net::pending_descriptors) net::pending_descriptors = std::make_unique<net::Channel<descriptor_data*>>(*net::io, 200);

    if(!gameFunc) {
        if (!fCopyOver) { /* If copyover mother_desc is already set up */
            log("Opening mother connection.");
            // open up net::acceptor on 0.0.0.0:<cmport>...
            net::acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*net::io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), cmport));
            mother_desc = net::acceptor->native_handle();
        } else {
            // a socket has already been created and needs to be re-used.
            // use the one from mother_desc...
            net::acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*net::io, boost::asio::ip::tcp::v4(), mother_desc);
        }

        log("Signal trapping.");
        net::signals = std::make_unique<boost::asio::signal_set>(*net::io);
        for(auto s : {SIGUSR1, SIGUSR2, SIGVTALRM, SIGHUP, SIGCHLD, SIGINT, SIGTERM, SIGPIPE, SIGALRM})
            net::signals->add(s);

        //boost::asio::co_spawn(boost::asio::make_strand(*net::io), signal_watcher(), boost::asio::detached);

        boost::asio::co_spawn(boost::asio::make_strand(*net::io), net::runAcceptor(), boost::asio::detached);
    }

    boost::asio::co_spawn(boost::asio::make_strand(*net::io), runGame(), boost::asio::detached);

    log("Entering game loop.");
    std::vector<std::thread> threads;
    if(!gameFunc) {
        auto max_threads = std::thread::hardware_concurrency() - 1;
        if(max_threads > 0) {
            for (int i = 0; i < max_threads; ++i) {
                threads.emplace_back([&] () {
                    net::io->run();
                });
            }
        }
    }

    net::io->run();

    for(auto &t : threads) {
        t.join();
    }
    threads.clear();

    // Release the executor and acceptor.
    // ASIO maintains its own socket polling fd and killing the executor is the
    // only way to release it.
    net::io.reset();

    if(circle_reboot == 1) {
        // release the mother_desc so that it doesn't get autoclosed...
        mother_desc = net::acceptor->release();
        finish_copyover();
        // The above should never return if called thanks to execl()...
    }

    net::acceptor.reset();

    Crash_save_all();

    log("Closing all sockets.");
    while (descriptor_list)
        close_socket(descriptor_list);

    close(mother_desc);

    if (CONFIG_IMC_ENABLED) {
        imc_shutdown(false);
    }

    if (circle_reboot != 2)
        save_all();

    log("Saving current MUD time.");
    save_mud_time(&time_info);

    save_areas();


    if (circle_reboot) {
        log("Rebooting.");
        exit(52);            /* what's so great about HHGTTG, anyhow? */
    }
    log("Normal termination of game.");
}

/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */



void record_usage() {
    int sockets_connected = 0, sockets_playing = 0;
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next) {
        sockets_connected++;
        if (IS_PLAYING(d))
            sockets_playing++;
    }

    log("nusage: %-3d sockets connected, %-3d sockets playing",
        sockets_connected, sockets_playing);
}


/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d) {
    char off_string[] =
            {
                    (char) 255,
                    (char) 251,
                    (char) 1,
                    (char) 0,
            };

    write_to_output(d, "%s", off_string);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d) {
    char on_string[] =
            {
                    (char) 255,
                    (char) 252,
                    (char) 1,
                    (char) 0
            };

    write_to_output(d, "%s", on_string);
}


char *make_prompt(struct descriptor_data *d) {
    static char prompt[MAX_PROMPT_LENGTH];
    struct obj_data *chair = nullptr;
    int flagged = false;

    /* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h) */

    if (d->str) {
        if (STATE(d) == CON_EXDESC) {
            strcpy(prompt, "Enter Description(/h for editor help)> ");
        } else if (PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING)) {
            strcpy(prompt, "Enter Message(/h for editor help)> ");
        } else if (PLR_FLAGGED(d->character, PLR_MAILING)) {
            strcpy(prompt, "Enter Mail Message(/h for editor help)> ");
        } else {
            strcpy(prompt, "Enter Message> ");
        }
    } else if (STATE(d) == CON_PLAYING && !IS_NPC(d->character)) {
        int count;
        size_t len = 0;

        *prompt = '\0';

        if (GET_INVIS_LEV(d->character) && len < sizeof(prompt)) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "i%d ", GET_INVIS_LEV(d->character));
            if (count >= 0)
                len += count;
        }
        /* show only when below 25% */
        if (PRF_FLAGGED(d->character, PRF_DISPAUTO) && GET_LEVEL(d->character) >= 500 && len < sizeof(prompt)) {
            struct char_data *ch = d->character;
            if (GET_HIT(ch) << 2 < GET_MAX_HIT(ch)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "PL: %" I64T " ", GET_HIT(ch));
                if (count >= 0)
                    len += count;
            }
            if ((ch->getCurST()) << 2 < GET_MAX_MOVE(ch) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "STA: %" I64T " ", (ch->getCurST()));
                if (count >= 0)
                    len += count;
            }
            if (GET_KI(ch) << 2 < GET_MAX_KI(ch) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "KI: %" I64T " ", GET_KI(ch));
                if (count >= 0)
                    len += count;
            }
        } else { /* not auto prompt */
            if (len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@w");
                if (count >= 0)
                    len += count;
            }
            if (PLR_FLAGGED(d->character, PLR_SELFD) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RSELF-D@r: @w%s@D]@n",
                                 PLR_FLAGGED(d->character, PLR_SELFD2) ? "READY" : "PREP");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (IS_HALFBREED(d->character) && !PLR_FLAGGED(d->character, PLR_FURY) &&
                PRF_FLAGGED(d->character, PRF_FURY)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mFury@W: @r%d@D]@w", GET_FURY(d->character));
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (IS_HALFBREED(d->character) && PLR_FLAGGED(d->character, PLR_FURY) &&
                PRF_FLAGGED(d->character, PRF_FURY)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mFury@W: @rENGAGED@D]@w");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (has_mail(GET_IDNUM(d->character)) && !PRF_FLAGGED(d->character, PRF_NMWARN) &&
                (GET_ADMLEVEL(d->character) > 0) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "CHECK MAIL - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (GET_KAIOKEN(d->character) > 0 && GET_ADMLEVEL(d->character) > 0) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", GET_KAIOKEN(d->character));
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (GET_SONG(d->character) > 0) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "%s - ", song_types[GET_SONG(d->character)]);
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (GET_KAIOKEN(d->character) > 0 && GET_ADMLEVEL(d->character) <= 0) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", GET_KAIOKEN(d->character));
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (has_mail(GET_IDNUM(d->character)) && (GET_ADMLEVEL(d->character) <= 0) &&
                !PRF_FLAGGED(d->character, PRF_NMWARN) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "CHECK MAIL - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (d->snooping && d->snooping->character != nullptr && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Snooping: (%s) - ",
                                 GET_NAME(d->snooping->character));
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (DRAGGING(d->character) && DRAGGING(d->character) != nullptr && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Dragging: (%s) - ",
                                 GET_NAME(DRAGGING(d->character)));
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_BUILDWALK) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "BUILDWALKING - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (AFF_FLAGGED(d->character, AFF_FLYING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "FLYING - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (AFF_FLAGGED(d->character, AFF_HIDE) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "HIDING - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (PLR_FLAGGED(d->character, PLR_SPAR) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "SPARRING - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (PLR_FLAGGED(d->character, PLR_NOSHOUT) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "MUTED - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 51 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Bash) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 52 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Headbutt) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 56 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Tailwhip) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 0 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Punch) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 1 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Kick) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 2 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Elbow) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 3 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Knee) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 4 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Roundhouse) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 5 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Uppercut) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 6 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Slam) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (COMBO(d->character) == 8 && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Heeldrop) - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_AFK) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "AFK - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (PLR_FLAGGED(d->character, PLR_FISHING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "FISHING -");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (flagged == true && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@n\n");
                if (count >= 0)
                    len += count;
            }
            if ((SITS(d->character) && PLR_FLAGGED(d->character, PLR_HEALT)) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "@c<@CFloating inside a healing tank@c>@n\r\n");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if ((SITS(d->character) && GET_POS(d->character) == POS_SITTING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "Sitting on: %s\r\n", chair->short_description);
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if ((SITS(d->character) && GET_POS(d->character) == POS_RESTING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "Resting on: %s\r\n", chair->short_description);
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if ((SITS(d->character) && GET_POS(d->character) == POS_SLEEPING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "Sleeping on: %s\r\n", chair->short_description);
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (AFF_FLAGGED(d->character, AFF_POSITION) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "(Best Position)\r\n");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (GET_CHARGE(d->character) < GET_MAX_MANA(d->character) * .01 && GET_CHARGE(d->character) > 0) {
                GET_CHARGE(d->character) = 0;
            }
            if (GET_CHARGE(d->character) > 0) {
                int64_t charge = GET_CHARGE(d->character);
                if (!PRF_FLAGGED(d->character, PRF_NODEC) && !PRF_FLAGGED(d->character, PRF_DISPERC)) {
                    if (charge >= GET_MAX_MANA(d->character)) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==@D<@RMAX@D>@G===@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .95) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=========-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .9) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=========@g-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .85) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G========-@g-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .80) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G========@g--@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .75) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=======-@g--@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .70) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=======@g---@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .65) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G======-@g---@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .60) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G======@g----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .55) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=====-@g----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .50) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=====@g-----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .45) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G====-@g-----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .40) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G====@g------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .35) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G===-@g------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .30) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G===@g-------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .25) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==-@g-------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .20) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==@g--------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .15) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=-@g--------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .10) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=@g---------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge >= GET_MAX_MANA(d->character) * .05) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G-@g---------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (charge < GET_MAX_MANA(d->character) * .05) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@g----------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@g----------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    }
                }
                if (PRF_FLAGGED(d->character, PRF_DISPERC) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
                    if (GET_CHARGE(d->character) > 0) {
                        int64_t perc = (GET_CHARGE(d->character) * 100) / GET_MAX_MANA(d->character);
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@BCharge@Y: @C%" I64T "%s@D]@n\n",
                                         perc, "%");
                        if (count >= 0)
                            len += count;
                    }
                }
                if (PRF_FLAGGED(d->character, PRF_NODEC)) {
                    if (charge > 0) {
                        int64_t perc = (charge * 100) / GET_MAX_MANA(d->character);
                        count = snprintf(prompt + len, sizeof(prompt) - len, "Ki is charged to %" I64T " percent.\n",
                                         perc);
                        if (count >= 0)
                            len += count;
                    }
                }
            }
            if (AFF_FLAGGED(d->character, AFF_FIRESHIELD)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D(@rF@RI@YR@rE@RS@YH@rI@RE@YL@rD@D)@n\n");
                if (count >= 0)
                    len += count;
            }
            if (AFF_FLAGGED(d->character, AFF_SANCTUARY)) {
                if (PRF_FLAGGED(d->character, PRF_DISPERC) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
                    if (GET_BARRIER(d->character) > 0) {
                        int64_t perc = (GET_BARRIER(d->character) * 100) / GET_MAX_MANA(d->character);
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GBarrier@Y: @B%" I64T "%s@D]@n\n",
                                         perc, "%");
                        if (count >= 0)
                            len += count;
                    }
                }

                if (!PRF_FLAGGED(d->character, PRF_NODEC) && !PRF_FLAGGED(d->character, PRF_DISPERC)) {
                    if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .75) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==MAX==@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .70) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=======@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .65) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C======-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .60) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C======@c-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .55) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=====-@c-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .50) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=====@c--@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .45) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C====-@c--@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .40) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C====@c---@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .35) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C===-@c---@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .30) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C===@c----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .25) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==-@c----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .20) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==@c-----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .15) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=-@c-----@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .10) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=@c------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .05) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C-@c------@D]@n\n");
                        if (count >= 0)
                            len += count;
                    } else if (GET_BARRIER(d->character) < GET_MAX_MANA(d->character) * .05) {
                        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C--Low-@D]@n\n");
                        if (count >= 0)
                            len += count;
                    }
                }
                if (PRF_FLAGGED(d->character, PRF_NODEC)) {
                    if (GET_BARRIER(d->character) > 0) {
                        int64_t perc = (GET_BARRIER(d->character) * 100) / GET_MAX_MANA(d->character);
                        count = snprintf(prompt + len, sizeof(prompt) - len,
                                         "A barrier charged to %" I64T " percent surrounds you.@n\n", perc);
                        if (count >= 0)
                            len += count;
                    }
                }
            }
            if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
                if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt)) {
                    auto col = "n";
                    auto ch = d->character;
                    if (ch->getMaxPL() > ch->getMaxPLTrans())
                        col = "g";
                    else if (ch->isWeightedPL())
                        col = "m";
                    else if (ch->getCurHealthPercent() > .5)
                        col = "c";
                    else if (ch->getCurHealthPercent() > .1)
                        col = "y";
                    else if (ch->getCurHealthPercent() <= .1)
                        col = "r";

                    if ((count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @%s%s@D]@n", col,
                                          add_commas(ch->getCurPL()))) > 0)
                        len += count;
                }
            } else if (PRF_FLAGGED(d->character, PRF_DISPHP)) {

                auto ch = d->character;
                auto perc = ((double) ch->getCurHealth() / (double) ch->getMaxPLTrans()) * 100;
                auto col = "n";
                if (perc > 100)
                    col = "g";
                else if (perc >= 70)
                    col = "c";
                else if (perc >= 51)
                    col = "Y";
                else if (perc >= 20)
                    col = "y";
                else
                    col = "r";

                if ((count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @%s%d%s@D]@n", col, (int) perc,
                                      "@w%")) > 0)
                    len += count;
            }
            if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
                if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                    (d->character->getCurKI()) > GET_MAX_MANA(d->character) / 2) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @c%s@D]@n", add_commas(
                            (d->character->getCurKI())));
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                           (d->character->getCurKI()) > GET_MAX_MANA(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @y%s@D]@n", add_commas(
                            (d->character->getCurKI())));
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                           (d->character->getCurKI()) <= GET_MAX_MANA(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @r%s@D]@n", add_commas(
                            (d->character->getCurKI())));
                    if (count >= 0)
                        len += count;
                }
            } else if (PRF_FLAGGED(d->character, PRF_DISPKI)) {
                int64_t power = (d->character->getCurKI()), maxpower = GET_MAX_MANA(d->character);
                int perc = 0;
                if (power <= 0) {
                    power = 1;
                }
                if (maxpower <= 0) {
                    maxpower = 1;
                }
                perc = (power * 100) / maxpower;
                if (perc > 100) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @G%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else if (perc >= 70) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @c%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else if (perc >= 51) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @Y%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else if (perc >= 20) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @y%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @r%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                }
            }
            if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
                if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                    (d->character->getCurST()) > GET_MAX_MOVE(d->character) / 2) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @c%s@D]@n", add_commas(
                            (d->character->getCurST())));
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                           (d->character->getCurST()) > GET_MAX_MOVE(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @y%s@D]@n", add_commas(
                            (d->character->getCurST())));
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                           (d->character->getCurST()) <= GET_MAX_MOVE(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @r%s@D]@n", add_commas(
                            (d->character->getCurST())));
                    if (count >= 0)
                        len += count;
                }
            } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
                int64_t power = (d->character->getCurST()), maxpower = GET_MAX_MOVE(d->character);
                int perc = 0;
                if (power <= 0) {
                    power = 1;
                }
                if (maxpower <= 0) {
                    maxpower = 1;
                }
                perc = (power * 100) / maxpower;
                if (perc > 100) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @G%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else if (perc >= 70) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @c%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else if (perc >= 51) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @Y%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else if (perc >= 20) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @y%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                } else {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @r%d%s@D]@n", perc, "@w%");
                    if (count >= 0)
                        len += count;
                }
            }
            if (PRF_FLAGGED(d->character, PRF_DISPTNL) && len < sizeof(prompt) && GET_LEVEL(d->character) < 100) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@yTNL@Y: @W%s@D]@n", add_commas(
                        level_exp(d->character, GET_LEVEL(d->character) + 1) - GET_EXP(d->character)));
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_DISTIME) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@W%2d %s@D]@n",
                                 (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
                                 time_info.hours >= 12 ? "PM" : "AM");
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_DISGOLD) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@YZen@y: @W%s@D]@n",
                                 add_commas(GET_GOLD(d->character)));
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_DISPRAC) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mPS@y: @W%s@D]@n",
                                 add_commas(GET_PRACTICES(d->character, GET_CLASS(d->character))));
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_DISHUTH) && len < sizeof(prompt)) {
                int hun = GET_COND(d->character, HUNGER), thir = GET_COND(d->character, THIRST);
                count = snprintf(prompt + len, sizeof(prompt) - len, "\n@D[@mHung@y:");
                if (count >= 0)
                    len += count;
                if (hun >= 48) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WFull@D]@n");
                } else if (hun >= 40) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WAlmost Full@D]@n");
                } else if (hun >= 30) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WNeed Snack@D]@n");
                } else if (hun >= 20) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WHungry@D]@n");
                } else if (hun >= 20) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WVery Hungry@D]@n");
                } else if (hun >= 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WAlmost Starving@D]@n");
                } else if (hun >= 5) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WNear Starving@D]@n");
                } else if (hun >= 0) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WStarving@D]@n");
                } else {
                    count = snprintf(prompt + len, sizeof(prompt) - len, " @WN/A@D]@n");
                }
                if (count >= 0)
                    len += count;
                if (thir >= 48) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WQuenched@D]@n");
                } else if (thir >= 40) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNeed Sip@D]@n");
                } else if (thir >= 30) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNeed Drink@D]@n");
                } else if (thir >= 20) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WThirsty@D]@n");
                } else if (thir >= 20) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WVery Thirsty@D]@n");
                } else if (thir >= 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WAlmost Dehydrated@D]@n");
                } else if (thir >= 5) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNear Dehydration@D]@n");
                } else if (thir >= 0) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WDehydrated@D]@n");
                } else {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WN/A@D]@n");
                }
                if (count >= 0)
                    len += count;
            }
            if (len < sizeof(prompt) && has_group(d->character) && !PRF_FLAGGED(d->character, PRF_GHEALTH)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "\n%s", report_party_health(d->character));
                if (d->character->temp_prompt)
                    free(d->character->temp_prompt);
                if (count >= 0)
                    len += count;
            }
            if (len < sizeof(prompt))
                count = snprintf(prompt + len, sizeof(prompt) - len, "\n"); /* strncat: OK */
        }
        if (len < sizeof(prompt) && len < 5)
            strncat(prompt, ">\n", sizeof(prompt) - len - 1);    /* strncat: OK */

    } else if (STATE(d) == CON_PLAYING && IS_NPC(d->character))
        snprintf(prompt, sizeof(prompt), "%s>\n", CAP(GET_NAME(d->character)));
    else
        *prompt = '\0';

    return (prompt);
}


/*
 * NOTE: 'txt' must be at most MAX_INPUT_LENGTH big.
 */
void write_to_q(const char *txt, struct txt_q *queue, int aliased) {
    struct txt_block *newt;

    CREATE(newt, struct txt_block, 1);
    newt->text = strdup(txt);
    newt->aliased = aliased;

    /* queue empty? */
    if (!queue->head) {
        newt->next = nullptr;
        queue->head = queue->tail = newt;
    } else {
        queue->tail->next = newt;
        queue->tail = newt;
        newt->next = nullptr;
    }
}



/* Add a new string to a player's output queue. For outside use. */
size_t write_to_output(struct descriptor_data *t, const char *txt, ...) {
    va_list args;
    size_t left;

    va_start(args, txt);
    left = vwrite_to_output(t, txt, args);
    va_end(args);

    return left;
}

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

/* Color replacement arrays. Orig. Renx -- 011100, now modified */
char *ANSI[] = {
        "@",
        AA_NORMAL,
        AA_NORMAL ANSISEPSTR AF_BLACK,
        AA_NORMAL ANSISEPSTR AF_BLUE,
        AA_NORMAL ANSISEPSTR AF_GREEN,
        AA_NORMAL ANSISEPSTR AF_CYAN,
        AA_NORMAL ANSISEPSTR AF_RED,
        AA_NORMAL ANSISEPSTR AF_MAGENTA,
        AA_NORMAL ANSISEPSTR AF_YELLOW,
        AA_NORMAL ANSISEPSTR AF_WHITE,
        AA_BOLD ANSISEPSTR AF_BLACK,
        AA_BOLD ANSISEPSTR AF_BLUE,
        AA_BOLD ANSISEPSTR AF_GREEN,
        AA_BOLD ANSISEPSTR AF_CYAN,
        AA_BOLD ANSISEPSTR AF_RED,
        AA_BOLD ANSISEPSTR AF_MAGENTA,
        AA_BOLD ANSISEPSTR AF_YELLOW,
        AA_BOLD ANSISEPSTR AF_WHITE,
        AB_BLACK,
        AB_BLUE,
        AB_GREEN,
        AB_CYAN,
        AB_RED,
        AB_MAGENTA,
        AB_YELLOW,
        AB_WHITE,
        AA_BLINK,
        AA_UNDERLINE,
        AA_BOLD,
        AA_REVERSE,
        "!"
};

const char CCODE[] = "@ndbgcrmywDBGCRMYW01234567luoex!";
/*
  Codes are:      @n - normal
  @d - black      @D - gray           @0 - background black
  @b - blue       @B - bright blue    @1 - background blue
  @g - green      @G - bright green   @2 - background green
  @c - cyan       @C - bright cyan    @3 - background cyan
  @r - red        @R - bright red     @4 - background red
  @m - magneta    @M - bright magneta @5 - background magneta
  @y - yellow     @Y - bright yellow  @6 - background yellow
  @w - white      @W - bright white   @7 - background white
  @x - random
Extra codes:      @l - blink          @o - bold
  @u - underline  @e - reverse video  @@ - single @

  @[num] - user color choice num, [] are required
*/
const char RANDOM_COLORS[] = "bgcrmywBGCRMWY";

#define NEW_STRING_LENGTH (size_t)(dest_char-save_pos)

size_t proc_colors(char *txt, size_t maxlen, int parse, char **choices) {
    char *dest_char, *source_char, *color_char, *save_pos, *replacement = nullptr;
    int i, temp_color;
    size_t wanted;

    if (!txt || !strchr(txt, '@')) /* skip out if no color codes     */
        return strlen(txt);

    source_char = txt;
    CREATE(dest_char, char, maxlen);
    save_pos = dest_char;
    for (; *source_char && (NEW_STRING_LENGTH < maxlen);) {
        /* no color code - just copy */
        if (*source_char != '@') {
            *dest_char++ = *source_char++;
            continue;
        }

        /* if we get here we have a color code */

        source_char++; /* source_char now points to the code */

        /* look for a random color code picks a random number between 1 and 14 */
        if (*source_char == 'x') {
            temp_color = (rand() % 14);
            *source_char = RANDOM_COLORS[temp_color];
        }

        if (*source_char == '\0') { /* string was terminated with color code - just put it in */
            *dest_char++ = '@';
            /* source_char will now point to '\0' in the for() check */
            continue;
        }

        if (!parse) { /* not parsing, just skip the code, unless it's @@ */
            if (*source_char == '@') {
                *dest_char++ = '@';
            }
            if (*source_char == '[') { /* Multi-character code */
                source_char++;
                while (*source_char && isdigit(*source_char))
                    source_char++;
                if (!*source_char)
                    source_char--;
            }
            source_char++; /* skip to next (non-colorcode) char */
            continue;
        }

        /* parse the color code */
        if (*source_char == '[') { /* User configurable color */
            source_char++;
            if (*source_char) {
                i = atoi(source_char);
                if (i < 0 || i >= NUM_COLOR)
                    i = COLOR_NORMAL;
                replacement = default_color_choices[i];
                if (choices && choices[i])
                    replacement = choices[i];
                while (*source_char && isdigit(*source_char))
                    source_char++;
                if (!*source_char)
                    source_char--;
            }
        } else if (*source_char == 'n') {
            replacement = default_color_choices[COLOR_NORMAL];
            if (choices && choices[COLOR_NORMAL])
                replacement = choices[COLOR_NORMAL];
        } else {
            for (i = 0; CCODE[i] != '!'; i++) { /* do we find it ? */
                if ((*source_char) == CCODE[i]) {           /* if so :*/
                    replacement = ANSI[i];
                    break;
                }
            }
        }
        if (replacement) {
            if (NEW_STRING_LENGTH + strlen(replacement) + strlen(ANSISTART) + 1 <
                maxlen) { /* only substitute if there's room for the whole code */
                if (isdigit(replacement[0]))
                    for (color_char = ANSISTART; *color_char;)
                        *dest_char++ = *color_char++;
                for (color_char = replacement; *color_char;)
                    *dest_char++ = *color_char++;
                if (isdigit(replacement[0]))
                    *dest_char++ = ANSIEND;
            }
            replacement = nullptr;
        }
        /* If we couldn't find any correct color code, or we found it and
         * substituted above, let's just process the next character.
         * - Welcor
         */
        source_char++;

    } /* for loop */

    /* make sure output is nullptr - terminated */
    *dest_char = '\0';

    wanted = strlen(source_char); /* see if we wanted more space */
    strncpy(txt, save_pos, maxlen - 1);
    free(save_pos); /* plug memory leak */

    return NEW_STRING_LENGTH + wanted;
}

#undef NEW_STRING_LENGTH

/* Add a new string to a player's output queue. */
// It used to return how many bytes couldn't be written.
// That's no longer a problem so it always returns 0.
size_t vwrite_to_output(struct descriptor_data *t, const char *format, va_list args) {
    // Use a temporary buffer to get the required size
    char temp;
    va_list args_copy;
    va_copy(args_copy, args);  // Make a copy of va_list, since vsnprintf will alter it
    int required_size = std::vsnprintf(&temp, 1, format, args_copy) + 1; // +1 for null-terminator
    va_end(args_copy);

    if (required_size <= 0) {
        // Handle error here (print message, throw exception, etc)
    }

    // Create a vector of the required size
    std::vector<char> buffer(required_size);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);

    // Append to the output
    t->output += std::string(buffer.begin(), buffer.end() - 1); // -1 to ignore the null-terminator

    return required_size - 1; // Return the size of the appended string (-1 to exclude the null-terminator)
}

void free_bufpool() {
    struct txt_block *tmp;

    while (bufpool) {
        tmp = bufpool->next;
        if (bufpool->text)
            free(bufpool->text);
        free(bufpool);
        bufpool = tmp;
    }
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


void descriptor_data::start() {
    write_to_output(this, GREETANSI);
    write_to_output(this, "\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
    write_to_output(this, "@D                 ---(@CPeak Logon Count Today@W: @w%4d@D)---@n\r\n", PCOUNT);
    write_to_output(this, "@D                 ---(@CHighest Logon Count   @W: @w%4d@D)---@n\r\n", HIGHPCOUNT);
    write_to_output(this, "@D                 ---(@CTotal Era %d Characters@W: @w%4s@D)---@n\r\n", CURRENT_ERA,
                    add_commas(ERAPLAYERS));
    write_to_output(this,
                    "\r\n@cEnter your desired username or the username you have already made.\n@CEnter Username:@n\r\n");
    this->pass = strdup("Empty");
    this->tmp1 = strdup("Empty");
    this->tmp2 = strdup("Empty");
    this->tmp3 = strdup("Empty");
    this->tmp4 = strdup("Empty");
    this->tmp5 = strdup("Empty");
}



/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 *
 * 32 int8_tGARBAGE_SPACE in MAX_SOCK_BUF used for:
 *	 2 bytes: prepended \r\n
 *	14 bytes: overflow message
 *	 2 bytes: extra \r\n for non-comapct
 *      14 bytes: unused
 */
int process_output(struct descriptor_data *t) {
    /* we may need this \r\n for later -- see below */
    std::string out = "\r\n" + t->output, realout;
    char i[MAX_SOCK_BUF], *osb = i + 2;
    int result;

    /* add the extra CRLF if the person isn't in compact mode */
    if (STATE(t) == CON_PLAYING && t->character && !IS_NPC(t->character) && !PRF_FLAGGED(t->character, PRF_COMPACT))
        out.append("\r\n");    /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

    /* add a prompt */

    out.append(make_prompt(t));    /* strcpy: OK (i:MAX_SOCK_BUF reserves space) */

    /*
     * now, send the output.  If this is an 'interruption', use the prepended
     * CRLF, otherwise send the straight output sans CRLF.
     */
    if (t->has_prompt) {
        t->has_prompt = false;
        t->conn->sendText(out);
    } else {
        t->conn->sendText(out.substr(2, out.length() - 2));
    }

    /* Handle snooping: prepend "% " and send to snooper. */
    if (t->snoop_by)
        write_to_output(t->snoop_by, "\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n%s\n^^^^^^^^^^^^^[Snoop]^^^^^^^^^^^^^\n",
                        t->output.c_str());

    t->output.clear();

    return (result);
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

/* perform_socket_write for all Non-Windows platforms */



/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 * >=0  If all is well and good.
 *  -1  If an error was encountered, so that the player should be cut off.
 */
int write_to_descriptor(socklen_t desc, const char *txt) {
    ssize_t bytes_written;
    size_t total = strlen(txt), write_total = 0;

    while (total > 0) {
        bytes_written = perform_socket_write(desc, txt, total);

        if (bytes_written < 0) {
            /* Fatal error.  Disconnect the player. */
            perror("SYSERR: Write to socket");
            return (-1);
        } else if (bytes_written == 0) {
            /* Temporary failure -- socket buffer full. */
            return (write_total);
        } else {
            txt += bytes_written;
            total -= bytes_written;
            write_total += bytes_written;
        }
    }

    return (write_total);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socklen_t desc, char *read_point, size_t space_left) {
    ssize_t ret;

    ret = read(desc, read_point, space_left);

    /* Read was successful. */
    if (ret > 0)
        return (ret);

    /* read() returned 0, meaning we got an EOF. */
    if (ret == 0) {
        log("WARNING: EOF on socket read (connection broken by peer)");
        return (-1);
    }

    /*
     * read returned a value < 0: there was an error
     */

    if (errno == EINTR)
        return (0);

    if (errno == EAGAIN)
        return (0);

    if (errno == ECONNRESET)
        return (-1);
    /*
     * We don't know what happened, cut them off. This qualifies for
     * a SYSERR because we have no idea what happened at this point.
     */
    perror("SYSERR: perform_socket_read: about to lose connection");
    return (-1);
}


/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst) {
    char newsub[MAX_INPUT_LENGTH + 5];

    char *first, *second, *strpos;

    /*
     * first is the position of the beginning of the first string (the one
     * to be replaced
     */
    first = subst + 1;

    /* now find the second '^' */
    if (!(second = strchr(first, '^'))) {
        write_to_output(t, "Invalid substitution.\r\n");
        return (1);
    }
    /* terminate "first" at the position of the '^' and make 'second' point
     * to the beginning of the second string */
    *(second++) = '\0';

    /* now, see if the contents of the first string appear in the original */
    if (!(strpos = strstr(orig, first))) {
        write_to_output(t, "Invalid substitution.\r\n");
        return (1);
    }
    /* now, we construct the new string for output. */

    /* first, everything in the original, up to the string to be replaced */
    strncpy(newsub, orig, strpos - orig);    /* strncpy: OK (newsub:MAX_INPUT_LENGTH+5 > orig:MAX_INPUT_LENGTH) */
    newsub[strpos - orig] = '\0';

    /* now, the replacement string */
    strncat(newsub, second, MAX_INPUT_LENGTH - strlen(newsub) - 1);    /* strncpy: OK */

    /* now, if there's anything left in the original after the string to
     * replaced, copy that too. */
    if (((strpos - orig) + strlen(first)) < strlen(orig))
        strncat(newsub, strpos + strlen(first), MAX_INPUT_LENGTH - strlen(newsub) - 1);    /* strncpy: OK */

    /* terminate the string in case of an overflow from strncat */
    newsub[MAX_INPUT_LENGTH - 1] = '\0';
    strcpy(subst, newsub);    /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */

    return (0);
}

void free_user(struct descriptor_data *d) {
    if (d->account == nullptr) {
        send_to_imm("ERROR: free_user called but no user to free!");
        return;
    }
    d->account = nullptr;
}

void close_socket(struct descriptor_data *d) {
    struct descriptor_data *temp;

    REMOVE_FROM_LIST(d, descriptor_list, next, temp);

    /* Forget snooping */
    if (d->snooping)
        d->snooping->snoop_by = nullptr;

    if (d->snoop_by) {
        write_to_output(d->snoop_by, "Your victim is no longer among us.\r\n");
        d->snoop_by->snooping = nullptr;
    }

    if (d->character) {
        /* If we're switched, this resets the mobile taken. */
        d->character->desc = nullptr;

        /* Plug memory leak, from Eric Green. */
        if (!IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
            if (*(d->str))
                free(*(d->str));
            free(d->str);
            d->str = nullptr;
        } else if (d->backstr && !IS_NPC(d->character) && !PLR_FLAGGED(d->character, PLR_WRITING)) {
            free(d->backstr);      /* editing description ... not olc */
            d->backstr = nullptr;
        }
        if (IS_PLAYING(d) || STATE(d) == CON_DISCONNECT) {
            struct char_data *link_challenged = d->original ? d->original : d->character;

            /* We are guaranteed to have a person. */
            act("$n has lost $s link.", true, link_challenged, nullptr, nullptr, TO_ROOM);
            save_char(link_challenged);
            mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(link_challenged)), true, "Closing link to: %s.",
                   GET_NAME(link_challenged));
        } else {
            free_char(d->character);
        }
    } else
        mudlog(CMP, ADMLVL_IMMORT, true, "Losing descriptor without char.");

    /* JE 2/22/95 -- part of my unending quest to make switch stable */
    if (d->original && d->original->desc)
        d->original->desc = nullptr;

    if (d->obj_name)
        free(d->obj_name);
    if (d->obj_short)
        free(d->obj_short);
    if (d->obj_long)
        free(d->obj_long);

    free_user(d);

    /*. Kill any OLC stuff .*/
    switch (d->connected) {
        case CON_OEDIT:
        case CON_IEDIT:
        case CON_REDIT:
        case CON_ZEDIT:
        case CON_MEDIT:
        case CON_SEDIT:
        case CON_TEDIT:
        case CON_AEDIT:
        case CON_TRIGEDIT:
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            break;
    }
	if(d->conn) {
        d->conn->halt(0);
    }
    delete d;
}

void check_idle_passwords() {
    struct descriptor_data *d, *next_d;

    for (d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_EMAIL && STATE(d) != CON_NEWPASSWD)
            continue;
        if (!d->idle_tics) {
            d->idle_tics++;
            continue;
        } else {
            echo_on(d);
            write_to_output(d, "\r\nTimed out... goodbye.\r\n");
            STATE(d) = CON_CLOSE;
        }
    }
}

void check_idle_menu() {
    struct descriptor_data *d, *next_d;

    for (d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if (STATE(d) != CON_MENU && STATE(d) != CON_GET_USER && STATE(d) != CON_UMENU)
            continue;
        if (!d->idle_tics) {
            d->idle_tics++;
            write_to_output(d, "\r\nYou are about to be disconnected due to inactivity in 60 seconds.\r\n");
            continue;
        } else {
            echo_on(d);
            write_to_output(d, "\r\nTimed out... goodbye.\r\n");
            STATE(d) = CON_CLOSE;
        }
    }
}


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

void reread_wizlists(int sig) {
    reread_wizlist = true;
}


void unrestrict_game(int sig) {
    emergency_unban = true;
}

/* clean up our zombie kids to avoid defunct processes */
void reap(int sig) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);

    signal(SIGCHLD, reap);
}

/* Dying anyway... */
void checkpointing(int sig) {
#ifndef MEMORY_DEBUG
    if (!tics_passed) {
        log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
        abort();
    } else
        tics_passed = 0;
#endif
}


/* Dying anyway... */
void hupsig(int sig) {
    log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
    exit(1);            /* perhaps something more elegant should
				 * substituted */
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */


void signal_handle(const boost::system::error_code& error,
                   int signal_number) {
    if(error) {
        // TODO: an error occured...
        return;
    }

    switch(signal_number) {
        case SIGUSR1:
            reread_wizlists(signal_number);
            break;
        case SIGUSR2:
            unrestrict_game(signal_number);
            break;
        case SIGVTALRM:
            checkpointing(signal_number);
            break;
        case SIGHUP:
        case SIGINT:
        case SIGTERM:
            hupsig(signal_number);
            break;
    }
}




/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

size_t send_to_char(struct char_data *ch, const char *messg, ...) {
    if (ch->desc && messg && *messg) {
        size_t left;
        va_list args;

        va_start(args, messg);
        left = vwrite_to_output(ch->desc, messg, args);
        va_end(args);
        return left;
    }
    return 0;
}

int arena_watch(struct char_data *ch) {

    struct descriptor_data *d;
    int found = false, room = NOWHERE;

    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING)
            continue;

        if (IN_ARENA(d->character)) {
            if (ARENA_IDNUM(ch) == GET_IDNUM(d->character)) {
                found = true;
                room = GET_ROOM_VNUM(IN_ROOM(d->character));
            }
        }
    }

    if (found == false) {
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
        ARENA_IDNUM(ch) = -1;
        return (NOWHERE);
    } else {
        return (room);
    }
}


void send_to_all(const char *messg, ...) {
    struct descriptor_data *i;
    va_list args;

    if (messg == nullptr)
        return;

    for (i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING)
            continue;

        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
    }
}


void send_to_outdoor(const char *messg, ...) {
    struct descriptor_data *i;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next) {
        va_list args;

        if (STATE(i) != CON_PLAYING || i->character == nullptr)
            continue;
        if (!AWAKE(i->character) || !OUTSIDE(i->character))
            continue;

        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
    }
}

void send_to_moon(const char *messg, ...) {
    struct descriptor_data *i;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next) {
        va_list args;

        if (STATE(i) != CON_PLAYING || i->character == nullptr)
            continue;
        if (!AWAKE(i->character) || !HAS_MOON(i->character))
            continue;

        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
    }
}

void send_to_planet(int type, int planet, const char *messg, ...) {
    struct descriptor_data *i;

    if (!messg || !*messg)
        return;

    for (i = descriptor_list; i; i = i->next) {
        va_list args;

        if (STATE(i) != CON_PLAYING || i->character == nullptr)
            continue;
        if (!AWAKE(i->character) || !ROOM_FLAGGED(IN_ROOM(i->character), planet))
            continue;
        else {
            if (type == 0) {
                va_start(args, messg);
                vwrite_to_output(i, messg, args);
                va_end(args);
            } else if (OUTSIDE(i->character) && GET_SKILL(i->character, SKILL_SPOT) >= axion_dice(-5)) {
                va_start(args, messg);
                vwrite_to_output(i, messg, args);
                va_end(args);
            }
        }
    }
}


void send_to_room(room_rnum room, const char *messg, ...) {
    struct char_data *i;
    va_list args;

    if (messg == nullptr)
        return;

    for (i = world[room].people; i; i = i->next_in_room) {
        if (!i->desc)
            continue;

        va_start(args, messg);
        vwrite_to_output(i->desc, messg, args);
        va_end(args);
    }

    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING)
            continue;

        if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
            if (arena_watch(d->character) == room) {
                char buf[2000];
                *buf = '\0';
                sprintf(buf, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", messg);
                va_start(args, messg);
                vwrite_to_output(d, buf, args);
                va_end(args);
            }
        }
        if (GET_EAVESDROP(d->character) > 0) {
            int roll = rand_number(1, 101);
            if (GET_EAVESDROP(d->character) == room && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
                char buf[1000];
                *buf = '\0';
                sprintf(buf, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", messg);
                va_start(args, messg);
                vwrite_to_output(d, buf, args);
                va_end(args);
            }
        }

    }

}


const char *ACTNULL = "<nullptr>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == nullptr) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void
perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to) {
    const char *i = nullptr;
    char lbuf[MAX_STRING_LENGTH], *buf, *j;
    bool uppercasenext = false;
    const struct char_data *dg_victim = nullptr;
    const struct obj_data *dg_target = nullptr;
    const char *dg_arg = nullptr;

    buf = lbuf;

    for (;;) {
        if (*orig == '$') {
            switch (*(++orig)) {
                case 'n':
                    i = PERS(ch, to);
                    break;
                case 'N':
                    CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
                    dg_victim = (const struct char_data *) vict_obj;
                    break;
                case 'm':
                    i = HMHR(ch);
                    break;
                case 'M':
                    CHECK_NULL(vict_obj, HMHR((const struct char_data *) vict_obj));
                    dg_victim = (const struct char_data *) vict_obj;
                    break;
                case 's':
                    i = HSHR(ch);
                    break;
                case 'S':
                    CHECK_NULL(vict_obj, HSHR((const struct char_data *) vict_obj));
                    dg_victim = (const struct char_data *) vict_obj;
                    break;
                case 'e':
                    i = HSSH(ch);
                    break;
                case 'E':
                    CHECK_NULL(vict_obj, HSSH((const struct char_data *) vict_obj));
                    dg_victim = (const struct char_data *) vict_obj;
                    break;
                case 'o':
                    CHECK_NULL(obj, OBJN(obj, to));
                    break;
                case 'O':
                    CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj, to));
                    dg_target = (const struct obj_data *) vict_obj;
                    break;
                case 'p':
                    CHECK_NULL(obj, OBJS(obj, to));
                    break;
                case 'P':
                    CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
                    dg_target = (const struct obj_data *) vict_obj;
                    break;
                case 'a':
                    CHECK_NULL(obj, SANA(obj));
                    break;
                case 'A':
                    CHECK_NULL(vict_obj, SANA((const struct obj_data *) vict_obj));
                    dg_target = (const struct obj_data *) vict_obj;
                    break;
                case 'T':
                    CHECK_NULL(vict_obj, (const char *) vict_obj);
                    dg_arg = (const char *) vict_obj;
                    break;
                case 't':
                    CHECK_NULL(obj, (char *) obj);
                    break;
                case 'F':
                    CHECK_NULL(vict_obj, fname((const char *) vict_obj));
                    break;
                    /* uppercase previous word */
                case 'u':
                    for (j = buf; j > lbuf && !isspace((int) *(j - 1)); j--);
                    if (j != buf)
                        *j = UPPER(*j);
                    i = "";
                    break;
                    /* uppercase next word */
                case 'U':
                    uppercasenext = true;
                    i = "";
                    break;
                case '$':
                    i = "$";
                    break;
                default:
                    return;
                    break;
            }
            while ((*buf = *(i++))) {
                if (uppercasenext && !isspace((int) *buf)) {
                    *buf = UPPER(*buf);
                    uppercasenext = false;
                }
                buf++;
            }
            orig++;
        } else if (!(*(buf++) = *(orig++))) {
            break;
        } else if (uppercasenext && !isspace((int) *(buf - 1))) {
            *(buf - 1) = UPPER(*(buf - 1));
            uppercasenext = false;
        }
    }

    *(--buf) = '\r';
    *(++buf) = '\n';
    *(++buf) = '\0';

    if (to->desc)
        write_to_output(to->desc, "%s", CAP(lbuf));

    if ((IS_NPC(to) && dg_act_check) && (to != ch))
        act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);

    if (last_act_message)
        free(last_act_message);
    last_act_message = strdup(lbuf);

}

char *act(const char *str, int hide_invisible, struct char_data *ch,
          struct obj_data *obj, const void *vict_obj, int type) {
    struct char_data *to;
    int to_sleeping, res_sneak, res_hide, dcval = 0, resskill = 0;

    if (!str || !*str)
        return nullptr;

    /* Warning: the following TO_SLEEP code is a hack. I wanted to be able to tell
     * act to deliver a message regardless of sleep without adding an additional
     * argument.  TO_SLEEP is 128 (a single bit high up).  It's ONLY legal to
     * combine TO_SLEEP with one other TO_x command.  It's not legal to combine
     * TO_x's with each other otherwise. TO_SLEEP only works because its value
     * "happens to be" a single bit; do not change it to something else.  In
     * short, it is a hack.  The same applies to TO_*RESIST.  */

    /* check if TO_SLEEP is there, and remove it if it is. */
    if ((to_sleeping = (type & TO_SLEEP)))
        type &= ~TO_SLEEP;

    if ((res_sneak = (type & TO_SNEAKRESIST)))
        type &= ~TO_SNEAKRESIST;

    if ((res_hide = (type & TO_HIDERESIST)))
        type &= ~TO_HIDERESIST;

    if (res_sneak && AFF_FLAGGED(ch, AFF_SNEAK)) {
        dcval = roll_skill(ch, SKILL_MOVE_SILENTLY); /* How difficult to counter? */
        if (GET_SKILL(ch, SKILL_BALANCE))
            dcval += GET_SKILL(ch, SKILL_BALANCE) / 10;
        if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 5 || GET_GENOME(ch, 1) == 5)) {
            dcval += 10;
        }
        resskill = SKILL_SPOT;             /* Skill used to resist      */
    } else if (res_hide && AFF_FLAGGED(ch, AFF_HIDE)) {
        dcval = roll_skill(ch, SKILL_HIDE);
        if (GET_SKILL(ch, SKILL_BALANCE))
            dcval += GET_SKILL(ch, SKILL_BALANCE) / 10;
        resskill = SKILL_SPOT;
    }

    /* this is a hack as well - DG_NO_TRIG is 256 -- Welcor */
    /* If the bit is set, unset dg_act_check, thus the ! below */
    if (!(dg_act_check = !IS_SET(type, DG_NO_TRIG)))
        REMOVE_BIT(type, DG_NO_TRIG);

    if (type == TO_CHAR) {
        if (ch && SENDOK(ch) && (!resskill || (roll_skill(ch, resskill) >= dcval))) {
            perform_act(str, ch, obj, vict_obj, ch);
            return last_act_message;
        }
        return nullptr;
    }

    if (type == TO_VICT) {
        if ((to = (struct char_data *) vict_obj) != nullptr && SENDOK(to) &&
            (!resskill || (roll_skill(to, resskill) >= dcval))) {
            perform_act(str, ch, obj, vict_obj, to);
            return last_act_message;
        }
        return nullptr;
    }

    if (type == TO_GMOTE) {
        struct descriptor_data *i;
        char buf[MAX_STRING_LENGTH];
        for (i = descriptor_list; i; i = i->next) {
            if (!i->connected && i->character &&
                !PRF_FLAGGED(i->character, PRF_NOGOSS) &&
                !PLR_FLAGGED(i->character, PLR_WRITING) &&
                !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) {

                sprintf(buf, "@y%s@n", str);
                perform_act(buf, ch, obj, vict_obj, i->character);
                char buf2[MAX_STRING_LENGTH];
                sprintf(buf2, "%s\r\n", buf);
                add_history(i->character, buf2, HIST_GOSSIP);
            }
        }
        return last_act_message;
    }

    /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

    if (ch && IN_ROOM(ch) != NOWHERE)
        to = world[IN_ROOM(ch)].people;
    else if (obj && IN_ROOM(obj) != NOWHERE)
        to = world[IN_ROOM(obj)].people;
    else {
        return nullptr;
    }

    if ((type & TO_ROOM)) {
        struct descriptor_data *d;

        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING)
                continue;

            if (ch != nullptr) {
                if (IN_ARENA(ch)) {
                    if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
                        if (arena_watch(d->character) == GET_ROOM_VNUM(IN_ROOM(ch))) {
                            char buf3[2000];
                            *buf3 = '\0';
                            sprintf(buf3, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", str);
                            perform_act(buf3, ch, obj, vict_obj, d->character);
                        }
                    }
                }
            }
            if (GET_EAVESDROP(d->character) > 0) {
                int roll = rand_number(1, 101);
                if (!resskill || (roll_skill(d->character, resskill) >= dcval)) {
                    if (ch != nullptr && GET_EAVESDROP(d->character) == GET_ROOM_VNUM(IN_ROOM(ch)) &&
                        GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
                        char buf3[1000];
                        *buf3 = '\0';
                        sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
                        perform_act(buf3, ch, obj, vict_obj, d->character);
                    } else if (obj != nullptr && GET_EAVESDROP(d->character) == GET_ROOM_VNUM(IN_ROOM(obj)) &&
                               GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
                        char buf3[1000];
                        *buf3 = '\0';
                        sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
                        perform_act(buf3, ch, obj, vict_obj, d->character);
                    }
                }
            }
        }
    }

    for (; to; to = to->next_in_room) {
        if (!SENDOK(to) || (to == ch))
            continue;
        if (hide_invisible && ch && !CAN_SEE(to, ch))
            continue;
        if (type != TO_ROOM && to == vict_obj)
            continue;
        if (resskill && roll_skill(to, resskill) < dcval)
            continue;
        perform_act(str, ch, obj, vict_obj, to);
    }
    return last_act_message;
}


/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd) {
    FILE *s_fp;

    s_fp = stderr;

    if (filename == nullptr || *filename == '\0') {
        /* No filename, set us up with the descriptor we just opened. */
        logfile = s_fp;
        puts("Using file descriptor for logging.");
        return;
    }

    /* We honor the default filename first. */
    if (open_logfile(filename, s_fp))
        return;

    /* Well, that failed but we want it logged to a file so try a default. */
    if (open_logfile("log/syslog", s_fp))
        return;

    /* Ok, one last shot at a file. */
    if (open_logfile("syslog", s_fp))
        return;

    /* Erp, that didn't work either, just die. */
    puts("SYSERR: Couldn't open anything to log to, giving up.");
    exit(1);
}

int open_logfile(const char *filename, FILE *stderr_fp) {
    if (stderr_fp)    /* freopen() the descriptor. */
        logfile = freopen(filename, "w", stderr_fp);
    else
        logfile = fopen(filename, "w");

    if (logfile) {
        printf("Using log file '%s'%s.\n",
               filename, stderr_fp ? " with redirection" : "");
        return (true);
    }

    printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
    return (false);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */


void show_help(struct descriptor_data *t, const char *entry) {
    int chk, bot, top, mid, minlen;
    char buf[MAX_STRING_LENGTH];

    if (!help_table) return;

    bot = 0;
    top = top_of_helpt;
    minlen = strlen(entry);

    for (;;) {
        mid = (bot + top) / 2;

        if (bot > top) {
            return;
        } else if (!(chk = strncasecmp(entry, help_table[mid].keywords, minlen))) {
            while ((mid > 0) &&
                   (!(chk = strncasecmp(entry, help_table[mid - 1].keywords, minlen))))
                mid--;
            write_to_output(t, "\r\n");
            snprintf(buf, sizeof(buf), "%s\r\n[ PRESS RETURN TO CONTINUE ]",
                     help_table[mid].entry);
            write_to_output(t, buf);
            return;
        } else {
            if (chk > 0) bot = mid + 1;
            else top = mid - 1;
        }
    }
}

/* Thx to Jamie Nelson of 4D for this contribution */
void send_to_range(room_vnum start, room_vnum finish, const char *messg, ...) {
    struct char_data *i;
    va_list args;
    int j;

    if (start > finish) {
        log("send_to_range passed start room value greater then finish.");
        return;
    }
    if (messg == nullptr)
        return;

    for (auto &r : world) {
        if (r.first >= start && r.first <= finish) {
            for (i = r.second.people; i; i = i->next_in_room) {
                if (!i->desc)
                    continue;

                va_start(args, messg);
                vwrite_to_output(i->desc, messg, args);
                va_end(args);
            }
        }
    }
}

ssize_t perform_socket_write(socklen_t desc, const char *txt, size_t length) {
    ssize_t result = 0;

    result = write(desc, txt, length);

    if (result > 0) {
        /* Write was successful. */
        return (result);
    }

    if (result == 0) {
        /* This should never happen! */
        log("SYSERR: Huh??  write() returned 0???  Please report this!");
        return (-1);
    }

    /*
     * result < 0, so an error was encountered - is it transient?
     * Unfortunately, different systems use different constants to
     * indicate this.
     */

    if (errno == EAGAIN)
        return (0);

    /* Looks like the error was fatal.  Too bad. */
    return (-1);
}

void descriptor_data::handle_input() {
    // Now we need to process the raw_input_queue, watching for special characters and also aliases.
    // Commands are processed first-come-first served...
    while(raw_input_queue->ready()) {
        std::string command;
        if(!raw_input_queue->try_receive(command)) continue;

        if (snoop_by)
            write_to_output(snoop_by, "%% %s\r\n", command.c_str());

        if(command == "--") {
            // this is a special command that clears out the processed input_queue.
            input_queue.clear();
            write_to_output(this, "All queued commands cancelled.\r\n");
        } else {
            perform_alias(this, (char*)command.c_str());
        }

    }

    if (character) {
        GET_WAIT_STATE(character) -= (GET_WAIT_STATE(character) > 0);

        if (GET_WAIT_STATE(character)) {
            return;
        }
    }

    if(input_queue.empty()) return;
    auto command = input_queue.front();
    input_queue.pop_front();

    if (character) {
        /* Reset the idle timer & pull char back from void if necessary */
        character->timer = 0;
        if (STATE(this) == CON_PLAYING && GET_WAS_IN(character) != NOWHERE) {
            if (IN_ROOM(character) != NOWHERE)
                char_from_room(character);
            char_to_room(character, GET_WAS_IN(character));
            GET_WAS_IN(character) = NOWHERE;
            act("$n has returned.", true, character, nullptr, nullptr, TO_ROOM);
        }
        GET_WAIT_STATE(character) = 1;
    }
    has_prompt = false;

    auto comm = (char*)command.c_str();

    if (str)        /* Writing boards, mail, etc. */
        string_add(this, comm);
    else if (STATE(this) != CON_PLAYING) /* In menus, etc. */
        nanny(this, comm);
    else {            /* else: we're playing normally. */
        command_interpreter(character, comm); /* Send it to interpreter */
    }

}