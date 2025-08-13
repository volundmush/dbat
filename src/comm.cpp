/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <filesystem>
#include <fstream>
#include <thread>
#include <locale>
#include <chrono>
#include <iostream>

#include "dbat/comm.h"
#include "dbat/send.h"
#include "dbat/config.h"
#include "dbat/maputils.h"
#include "dbat/ban.h"
#include "dbat/weather.h"
#include "dbat/act.wizard.h"
#include "dbat/act.misc.h"
#include "dbat/act.other.h"
#include "dbat/dg_comm.h"
#include "dbat/handler.h"
#include "dbat/dg_scripts.h"
#include "dbat/act.item.h"
#include "dbat/interpreter.h"
#include "dbat/act.informative.h"
#include "dbat/dg_event.h"
#include "dbat/mobact.h"
#include "dbat/magic.h"
#include "dbat/genolc.h"
#include "dbat/class.h"
#include "dbat/combat.h"
#include "dbat/fight.h"
#include "dbat/local_limits.h"
#include "dbat/clan.h"
#include "dbat/mail.h"
#include "dbat/constants.h"
#include "dbat/screen.h"

#include "dbat/players.h"
#include "dbat/account.h"
#include "dbat/transformation.h"
#include "dbat/db.h"

#include "dbat/transformation.h"
#include "dbat/shop.h"

#include "dbat/saveload.h"

/* local globals */
struct descriptor_data *descriptor_list = nullptr;        /* master desc list */

struct txt_block *bufpool = nullptr;    /* pool of large output buffers */
int buf_largecount = 0;        /* # of large buffers which exist */
int buf_overflows = 0;        /* # of overflows of output */
int buf_switches = 0;        /* # of switches from small to large buf */
int circle_shutdown = 0;    /* clean shutdown */
int circle_reboot = 0;        /* reboot the game after a shutdown */
int no_specials = 0;        /* Suppress ass. of special routines */
int tics_passed = 0;        /* for extern checkpointing */
int scheck = 0;            /* for syntax checking mode */
struct timeval null_time;    /* zero-valued time structure */
int8_t reread_wizlist;        /* signal: SIGUSR1 */
int8_t emergency_unban;        /* signal: SIGUSR2 */
FILE *logfile = nullptr;        /* Where to send the log messages. */
int dg_act_check;               /* toggle for act_trigger */
uint64_t pulse = 0;        /* number of pulses since game start */
uint16_t port;
char *last_act_message = nullptr;

std::string original_cwd;

/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

void broadcast(const std::string& txt) {

}

void signal_handler(int signal) {
    // Process the signal
    switch(signal) {
        case SIGUSR1:
            circle_shutdown = 1;
            circle_reboot = 1;
            break;
        case SIGUSR2:
            circle_shutdown = 1;
            circle_reboot = 2;
            break;
        default:
            basic_mud_log("Unexpected signal: %d", signal);
    }
}


void copyover_recover_final() {
    struct descriptor_data *next_d;
    for(auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if(STATE(d) != CON_COPYOVER) continue;

		auto accID = d->obj_editval;
        auto playerID = d->id;
        room_vnum room = d->obj_type;

        d->obj_editflag = 0;
        d->obj_type = 0;

        auto accFind = accounts.find(accID);

        if(accFind == accounts.end()) {
            basic_mud_log("recoverConnection: user %d not found.", accID);
            close_socket(d);
            continue;
        }
        d->account = &accFind->second;
        d->account->descriptors.insert(d);

        auto playFind = players.find(playerID);
        if(playFind == players.end()) {
            basic_mud_log("recoverConnection: character %d not found.", playerID);
            close_socket(d);
            continue;
        }
        auto c = d->character = playFind->second.character;
        c->desc = d;

        c->setBaseStat("load_room", room);
        for(auto f : {PLR_WRITING, PLR_MAILING, PLR_CRYO}) c->player_flags.set(f, false);

    d->sendText("@rThe world comes back into focus... has something changed?@n\r\n");

        if (AFF_FLAGGED(d->character, AFF_HAYASA)) {
            d->character->setBaseStat<int>("speedboost", GET_SPEEDCALC(d->character) * 0.5);
        }
    }
}

/* Reload players after a copyover */
void copyover_recover() {
    basic_mud_log("Copyover recovery initiated");
    std::ifstream fp(COPYOVER_FILE);

    if(!fp.is_open()) {
        basic_mud_log("Copyover file not found. Exiting.\r\n");
        shutdown_game(1);
    }

    nlohmann::json j;
    fp >> j;
    fp.close();

    // erase the file.
    std::filesystem::remove(COPYOVER_FILE);

    // rebuild descriptor data...
    if(j.contains("descriptors")) {
        for(const auto &jd : j["descriptors"]) {
            auto d = new descriptor_data();
            d->obj_editval = jd["user"];
            d->id = jd["character"];
            d->connected = CON_COPYOVER;
            d->obj_type = NOWHERE;
            if(jd.contains("in_room")) d->obj_type = jd["in_room"].get<room_vnum>();
            if(jd.contains("connections")) {
                {
                    for(const auto& jc : jd["connections"]) {

                    }
                }
            }
            sessions[d->id] = d;

            d->next = descriptor_list;
            descriptor_list = d;
        }
    }
}


static void performReboot(int mode) {
    char buf[100], buf2[100];

    std::ofstream fp(COPYOVER_FILE);

    if (!fp.is_open()) {
        send_to_imm("Copyover file not writeable, aborted.\r\n");
        circle_reboot = 0;
        return;
    }

    broadcast("\t@RThe universe stops for a moment as space and time fold.@n\r\n");
    // Flush all pending output and otherwise get connections ready for a copyover.

    nlohmann::json j;



    /* For each descriptor/connection, halt them and save state. */
    for (auto &[cid, d] : sessions) {
        nlohmann::json jd;
        if(d->conns.empty()) continue;

        for(auto &[cid, c] : d->conns) {
            //jd["connections"].push_back(c->connId);
        }
        auto och = d->character;

        jd["user"] = d->account->id;
        jd["character"] = och->id;

        auto r = IN_ROOM(och);
        auto w = och->getBaseStat("was_in_room");
        if(r > 1) {
            jd["in_room"] = r;
        } else if(r <= 1 && w > 1) {
        	jd["in_room"] = w;
        }

        j["descriptors"].push_back(jd);
    }

    fp << jdumps(j) << std::endl;
    fp.close();
}

static std::vector<std::pair<std::string, double>> timings;

struct GameSystem {
    // In seconds.
    GameSystem(std::string name, double interval, std::function<void(uint64_t, double)> func) : name(std::move(name)), interval(interval), func(std::move(func)) {
        countdown = interval;
    }
    std::string name;
    double interval{0.0};
    std::function<void(uint64_t, double)> func;
    double countdown{0.0};
};

static void saveMudTimeWrapper(uint64_t heartBeat, double deltaTime) {
    save_mud_time(&time_info);
}


static std::vector<GameSystem> gameSystems = {
        GameSystem("commandWaitQueue", 0.0, commandWaitQueue),
        GameSystem("event_process", 0.0, event_process),
        GameSystem("script_trigger_check", 13.0, script_trigger_check),
        GameSystem("zone_update", 0.0, zone_update),
        GameSystem("repairRoomDamage", 600.0, repairRoomDamage),
        //GameSystem("dball_load", 1.0, dball_load),
        GameSystem("base_update", 2.0, base_update),
        GameSystem("androidAbsorbSystem", 2.0, androidAbsorbSystem),
        GameSystem("goopTimeService", 2.0, goopTimeService),
        GameSystem("fish_update", 2.0, fish_update),
        GameSystem("handle_songs", 15.0, handle_songs),
        GameSystem("wishSYS", 1.0, wishSYS),
        GameSystem("mobile_activity", 10.0, mobile_activity),
        GameSystem("shop_purge", 86400.0 * 7.0, shop_purge),
        GameSystem("check_auction", 15.0, check_auction),
        GameSystem("gamesys_oozaru", 0.0, trans::gamesys_oozaru),
        GameSystem("gamesys_transform", 0.0, trans::gamesys_transform),
        GameSystem("powerupService", 4.0, powerupService),
        GameSystem("lifeforceSystem", 4.0, lifeforceSystem),
        GameSystem("fight_stack", 4.0, fight_stack),
        GameSystem("kiChargeSystem", 4.0, kiChargeSystem),
        GameSystem("homing_update", 2.0, homing_update),
        GameSystem("huge_update", 2.0, huge_update),
        GameSystem("broken_update", 2.0, broken_update),
        GameSystem("copyover_check", 1.0, copyover_check),
        GameSystem("affect_update_violence", 5.0, affect_update_violence),
        GameSystem("advanceClock", 0.0, advanceClock),
        GameSystem("affect_update", 300.0, affect_update),
        GameSystem("characterVitalsRecovery", 0.0, characterVitalsRecovery),
        GameSystem("healTankService", 0.0, healTankService),
        GameSystem("corpseRotService", 100.0, corpseRotService),
        GameSystem("relax_update", 100.0, relax_update),
        GameSystem("poison_update", 100.0, poison_update),
        GameSystem("hunger_update", 100.0, hunger_update),
        GameSystem("point_update", 100.0, point_update),
        GameSystem("player_misc_update", 100.0, player_misc_update),
        GameSystem("kaioken_update", 100.0, kaioken_update),
        GameSystem("clan_update", 60.0, clan_update),
        GameSystem("record_usage", 5.0, record_usage),
        GameSystem("save_mud_time", 30.0, saveMudTimeWrapper),
        GameSystem("extract_pending_chars", 0.0, extract_pending_chars),
};

void heartbeat(uint64_t heart_pulse, double deltaTime) {
    static int mins_since_crashsave = 0;

    for(auto &s : gameSystems) {
        if(s.interval > 0.0) s.countdown -= deltaTime;
        if(s.countdown > 0.0) continue;

        auto start = std::chrono::high_resolution_clock::now();
        auto miniDelta = (s.interval > 0.0) ? std::abs(s.countdown + s.interval) : deltaTime;
        try {
            s.func(heart_pulse, miniDelta);
        }
        catch(const std::exception &e) {
            basic_mud_log("Exception while running GameService '%s': %s", s.name.c_str(), e.what());
            shutdown_game(1);
        }
        catch(...) {
            basic_mud_log("Unknown exception while running GameService '%s'", s.name.c_str());
            shutdown_game(1);
        }
        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back(fmt::format("heartbeat system: {}", s.name), std::chrono::duration<double>(end - start).count());

        if(s.interval > 0.0)
            s.countdown += s.interval;
    }
}

// Initialize the global function pointers to null.
distribute_output_t g_distribute_output = nullptr;
send_close_t g_send_close = nullptr;

void runOneLoop(double deltaTime) {
    static bool sleeping = false;
    struct descriptor_data* next_d;
    // Clear profile data.

    if(sleeping && descriptor_list) {
        basic_mud_log("Waking up.");
        sleeping = false;
    }

    if(!descriptor_list) {
        if(!sleeping) {
            basic_mud_log("No connections.  Going to sleep.");
            sleeping = true;
        }
    }

    timings.clear();
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    {
        std::unordered_set<struct descriptor_data*> toLook;
        start = std::chrono::high_resolution_clock::now();
        for(auto d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            switch(STATE(d)) {
                case CON_LOGIN:
                    d->character->login();
                    break;
                case CON_COPYOVER:
                    enter_player_game(d);
                    d->connected = CON_PLAYING;
                    toLook.insert(d);
                    break;
                default:
                    break;
            }
        }
        for(auto d : toLook) d->character->lookAtLocation();
        end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("handle logins", std::chrono::duration<double>(end - start).count());
    }


    /* Process commands we just read from process_input */
    start = std::chrono::high_resolution_clock::now();
    for (auto d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if(d->character) d->character->time.played += deltaTime;
        d->handle_input();
    }
    end = std::chrono::high_resolution_clock::now();
    timings.emplace_back("handle input", std::chrono::duration<double>(end - start).count());

    start = std::chrono::high_resolution_clock::now();
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
    end = std::chrono::high_resolution_clock::now();
    timings.emplace_back("check for active descriptors", std::chrono::duration<double>(end - start).count());

    if(gameActive) {
        start = std::chrono::high_resolution_clock::now();
        heartbeat(++pulse, deltaTime);
        end = std::chrono::high_resolution_clock::now();
        //timings.emplace_back("heartbeat total", std::chrono::duration<double>(end - start).count());
    }

    /* Send queued output out to the operating system (ultimately to user). */
    {
        start = std::chrono::high_resolution_clock::now();
        for (auto d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (!d->output.empty()) {
                process_output(d);
                d->has_prompt = true;
            }
        }
        end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("process output", std::chrono::duration<double>(end - start).count());
    }

    /* Print prompts for other descriptors who had no other output */
    {
        start = std::chrono::high_resolution_clock::now();
        for (auto d = descriptor_list; d; d = d->next) {
            if (!d->has_prompt) {
                d->sendText("@n");
                process_output(d);
                d->has_prompt = true;
            }
        }
        end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("print prompts", std::chrono::duration<double>(end - start).count());
    }

    // send output to the Python side...
    g_distribute_output();

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    {
        start = std::chrono::high_resolution_clock::now();
        for (auto d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if(d->conns.empty()) {
                d->timeoutCounter += deltaTime;
                if(d->timeoutCounter > 300) {
                    STATE(d) = CON_CLOSE;
                }
            }
            if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT || STATE(d) == CON_QUITGAME)
                close_socket(d);
        }
        end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("close sockets", std::chrono::duration<double>(end - start).count());
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
    }

    tics_passed++;
}

namespace game {

    void init() {
        game::init_locale();
        game::init_database();
        game::init_zones();
        game::init_copyover();
    }

    void init_copyover() {
        if(std::filesystem::exists(COPYOVER_FILE)) {
            copyover_recover();
            copyover_recover_final();
        }
    }

    void init_locale() {
        std::locale::global(std::locale("en_US.UTF-8"));
    }

    void init_database() {
        boot_db_new();
    }

    void init_zones() {
        for (auto &[vn, z] : zone_table) {
            basic_mud_log("Resetting #%d: %s.", vn, z.name.c_str());
            z.reset();
        }
    }
}

void record_usage(uint64_t heartPulse, double deltaTime) {

}

char *make_prompt(struct descriptor_data *d) {
    static char prompt[MAX_PROMPT_LENGTH];
    Object *chair = nullptr;
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
            Character *ch = d->character;
            if (GET_HIT(ch) << 2 < GET_MAX_HIT(ch)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "HL: %" I64T " ", GET_HIT(ch));
                if (count >= 0)
                    len += count;
            }
            if ((ch->getCurVital(CharVital::stamina)) << 2 < GET_MAX_MOVE(ch) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "STA: %" I64T " ", (ch->getCurVital(CharVital::stamina)));
                if (count >= 0)
                    len += count;
            }
            if (GET_KI(ch) << 2 < GET_MAX_MANA(ch) && len < sizeof(prompt)) {
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
            if (d->character->task != Task::nothing) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "   @D-@r%s@D-@w   ", DoingTaskName[d->character->task].c_str());
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
            auto kaioken = GET_KAIOKEN(d->character);
            if (kaioken > 0 && GET_ADMLEVEL(d->character) > 0) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", kaioken);
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (GET_SONG(d->character) > 0) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "%s - ", song_types[(int)GET_SONG(d->character)]);
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (kaioken > 0 && GET_ADMLEVEL(d->character) <= 0) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", kaioken);
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
            if (d->character->character_flags.get(CharacterFlag::sparring) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "SPARRING - ");
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_FORM) && len < sizeof(prompt)) {
                auto form = d->character->form;
                
                if(d->character->transforms[form].grade > 1)
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mForm@y: @W%s - %s@D]@n",
                        trans::getAbbr(d->character, form).c_str(), std::to_string(d->character->transforms[form].grade).c_str());
                else
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mForm@y: @W%s@D]@n",
                        trans::getAbbr(d->character, form).c_str());
                flagged = true;

                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_TECH) && len < sizeof(prompt)) {
                Form form = d->character->technique;

                if(d->character->transforms[form].grade > 1)
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mTech@y: @W%s - %s@D]@n",
                        trans::getAbbr(d->character, form).c_str(), std::to_string(d->character->transforms[form].grade).c_str());
                else
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mTech@y: @W%s@D]@n",
                        trans::getAbbr(d->character, form).c_str());
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

            {
                auto combo = COMBO(d->character);
                char *promptCombo = nullptr;
                switch(combo) {
                    case 51:
                        promptCombo = "Combo (Bash) - ";
                        break;
                    case 52:    
                        promptCombo = "Combo (Headbutt) - ";
                        break;
                    case 56:
                        promptCombo = "Combo (Tailwhip) - ";
                        break;
                    case 0:
                        promptCombo = "Combo (Punch) - ";
                        break;
                    case 1:
                        promptCombo = "Combo (Kick) - ";
                        break;
                    case 2:
                        promptCombo = "Combo (Elbow) - ";
                        break;
                    case 3:
                        promptCombo = "Combo (Knee) - ";
                        break;
                    case 4:
                        promptCombo = "Combo (Roundhouse) - ";
                        break;
                    case 5:
                        promptCombo = "Combo (Uppercut) - ";
                        break;
                    case 6:
                        promptCombo = "Combo (Slam) - ";
                        break;
                    case 8:
                        promptCombo = "Combo (Heeldrop) - ";
                        break;
                    default:
                        promptCombo = nullptr;
                        break;
                }
                if (promptCombo && len < sizeof(prompt)) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "%s", promptCombo);
                    flagged = true;
                    if (count >= 0)
                        len += count;
                }
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
                count = snprintf(prompt + len, sizeof(prompt) - len, "Sitting on: %s\r\n", chair->getShortDescription());
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if ((SITS(d->character) && GET_POS(d->character) == POS_RESTING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "Resting on: %s\r\n", chair->getShortDescription());
                flagged = true;
                if (count >= 0)
                    len += count;
            }
            if ((SITS(d->character) && GET_POS(d->character) == POS_SLEEPING) && len < sizeof(prompt) &&
                !PRF_FLAGGED(d->character, PRF_NODEC)) {
                chair = SITS(d->character);
                count = snprintf(prompt + len, sizeof(prompt) - len, "Sleeping on: %s\r\n", chair->getShortDescription());
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
                d->character->setBaseStat<int64_t>("charge", 0);
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
                    // TODO: fix this.
                    auto cvp = ch->getCurVitalMeterPercent(CharVital::health);
                    if (ch->getEffectiveStat<int64_t>("health") > ch->getBaseStat<int64_t>("health"))
                        col = "g";
                    else if (cvp > .5)
                        col = "c";
                    else if (cvp > .1)
                        col = "y";
                    else if (cvp <= .1)
                        col = "r";

                    if ((count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RHL@n@Y: @%s%s@D]@n", col,
                                          add_commas(ch->getCurVital(CharVital::health)).c_str())) > 0)
                        len += count;
                }
            } else if (PRF_FLAGGED(d->character, PRF_DISPHP)) {

                auto ch = d->character;
                auto perc = ch->getCurVitalMeterPercent(CharVital::health);
                auto col = "n";
                if (perc >= 1.0)
                    col = "g";
                else if (perc >= .70)
                    col = "c";
                else if (perc >= .51)
                    col = "Y";
                else if (perc >= .20)
                    col = "y";
                else
                    col = "r";

                if ((count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RHL@n@Y: @%s%d%s@D]@n", col, (int) perc,
                                      "@w%")) > 0)
                    len += count;
            }
            if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
                if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                    (d->character->getCurVital(CharVital::ki)) > GET_MAX_MANA(d->character) / 2) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @c%s@D]@n", add_commas(
                            (d->character->getCurVital(CharVital::ki))).c_str());
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                           (d->character->getCurVital(CharVital::ki)) > GET_MAX_MANA(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @y%s@D]@n", add_commas(
                            (d->character->getCurVital(CharVital::ki))).c_str());
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                           (d->character->getCurVital(CharVital::ki)) <= GET_MAX_MANA(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @r%s@D]@n", add_commas(
                            (d->character->getCurVital(CharVital::ki))).c_str());
                    if (count >= 0)
                        len += count;
                }
            } else if (PRF_FLAGGED(d->character, PRF_DISPKI)) {
                int64_t power = (d->character->getCurVital(CharVital::ki)), maxpower = GET_MAX_MANA(d->character);
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
                    (d->character->getCurVital(CharVital::stamina)) > GET_MAX_MOVE(d->character) / 2) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @c%s@D]@n", add_commas(
                            (d->character->getCurVital(CharVital::stamina))).c_str());
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                           (d->character->getCurVital(CharVital::stamina)) > GET_MAX_MOVE(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @y%s@D]@n", add_commas(
                            (d->character->getCurVital(CharVital::stamina))).c_str());
                    if (count >= 0)
                        len += count;
                } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                           (d->character->getCurVital(CharVital::stamina)) <= GET_MAX_MOVE(d->character) / 10) {
                    count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @r%s@D]@n", add_commas(
                            (d->character->getCurVital(CharVital::stamina))).c_str());
                    if (count >= 0)
                        len += count;
                }
            } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
                int64_t power = (d->character->getCurVital(CharVital::stamina)), maxpower = GET_MAX_MOVE(d->character);
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
            if (PRF_FLAGGED(d->character, PRF_DISTIME) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@W%2d %s@D]@n",
                                 (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
                                 time_info.hours >= 12 ? "PM" : "AM");
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_DISGOLD) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@YZen@y: @W%s@D]@n",
                                 add_commas(GET_GOLD(d->character)).c_str());
                if (count >= 0)
                    len += count;
            }
            if (PRF_FLAGGED(d->character, PRF_DISPRAC) && len < sizeof(prompt)) {
                count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mPS@y: @W%s@D]@n",
                                 add_commas(GET_PRACTICES(d->character)).c_str());
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
        snprintf(prompt, sizeof(prompt), "%s>\n", GET_NAME(d->character));
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



void descriptor_data::sendText(const std::string& txt) {
    output += txt;
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
    sendText(GREETANSI);
    sendText("\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
    send_to("@D                 ---(@CPeak Logon Count Today@W: @w%4d@D)---@n\r\n", PCOUNT);
    send_to("@D                 ---(@CHighest Logon Count   @W: @w%4d@D)---@n\r\n", HIGHPCOUNT);
    send_to("@D                 ---(@CTotal Era %d Characters@W: @w%4s@D)---@n\r\n", CURRENT_ERA,
        add_commas(players.size()).c_str());
    sendText("\r\n@cEnter your desired username or the username you have already made.\n@CEnter Username:@n\r\n");
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
    } else {
        auto o = out.substr(2, out.length() - 2);
    }

    /* Handle snooping: prepend "% " and send to snooper. */
    if (t->snoop_by)
    t->snoop_by->send_to("\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n%s\n^^^^^^^^^^^^^[Snoop]^^^^^^^^^^^^^\n",
            t->output.c_str());

    t->processed_output += out;
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

void close_socket(struct descriptor_data *d) {
    struct descriptor_data *temp;

    REMOVE_FROM_LIST(d, descriptor_list, next, temp);

    /* Forget snooping */
    if (d->snooping)
        d->snooping->snoop_by = nullptr;

    if (d->snoop_by) {
    d->snoop_by->sendText("Your victim is no longer among us.\r\n");
        d->snoop_by->snooping = nullptr;
    }

    auto c = d->character;

    /* JE 2/22/95 -- part of my unending quest to make switch stable */
    if (d->original && d->original->desc)
        d->original->desc = nullptr;

    if (d->obj_name)
        free(d->obj_name);
    if (d->obj_short)
        free(d->obj_short);
    if (d->obj_long)
        free(d->obj_long);

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

    if(d->account) {
        d->account->descriptors.erase(d);
        d->account = nullptr;
    }

    if(c) {
        c->desc = nullptr;
        // Notify the server that this socket will be closed.
        // This will send a None event to all attached streams
        // and close them gracefully.
        g_send_close(c->id);
    }

    sessions.erase(d->id);
    delete d;
    if(c && c->active) extract_char(c);
}


/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */


int arena_watch(Character *ch) {

    struct descriptor_data *d;
    int found = false, room = NOWHERE;

    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING)
            continue;

        if (IN_ARENA(d->character)) {
            if (ARENA_IDNUM(ch) == GET_IDNUM(d->character)) {
                found = true;
                room = d->character->location.getVnum();
            }
        }
    }

    if (found == false) {
        ch->pref_flags.set(PRF_ARENAWATCH, false);
        ch->setBaseStat<room_vnum>("arena_watch", -1);
        return (NOWHERE);
    } else {
        return (room);
    }
}


const char *ACTNULL = "<nullptr>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == nullptr) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, Character *ch, Object *obj, const void *vict_obj, Character *to) {
    const char *i = nullptr;
    char lbuf[MAX_STRING_LENGTH], *buf, *j;
    bool uppercasenext = false;
    Character *dg_victim = nullptr;
    Object *dg_target = nullptr;
    char *dg_arg = nullptr;

    buf = lbuf;

    for (;;) {
        if (*orig == '$') {
            switch (*(++orig)) {
                case 'n':
                    i = PERS(ch, to);
                    break;
                case 'N':
                    CHECK_NULL(vict_obj, PERS((Character *) vict_obj, to));
                    dg_victim = (Character *) vict_obj;
                    break;
                case 'm':
                    i = HMHR(ch);
                    break;
                case 'M':
                    CHECK_NULL(vict_obj, HMHR((Character *) vict_obj));
                    dg_victim = (Character *) vict_obj;
                    break;
                case 's':
                    i = HSHR(ch);
                    break;
                case 'S':
                    CHECK_NULL(vict_obj, HSHR((Character *) vict_obj));
                    dg_victim = (Character *) vict_obj;
                    break;
                case 'e':
                    i = HSSH(ch);
                    break;
                case 'E':
                    CHECK_NULL(vict_obj, HSSH((Character *) vict_obj));
                    dg_victim = (Character *) vict_obj;
                    break;
                case 'o':
                    CHECK_NULL(obj, OBJN(obj, to));
                    break;
                case 'O':
                    CHECK_NULL(vict_obj, OBJN((Object *) vict_obj, to));
                    dg_target = (Object *) vict_obj;
                    break;
                case 'p':
                    CHECK_NULL(obj, OBJS(obj, to));
                    break;
                case 'P':
                    CHECK_NULL(vict_obj, OBJS((Object *) vict_obj, to));
                    dg_target = (Object *) vict_obj;
                    break;
                case 'a':
                    CHECK_NULL(obj, SANA(obj));
                    break;
                case 'A':
                    CHECK_NULL(vict_obj, SANA((Object *) vict_obj));
                    dg_target = (Object *) vict_obj;
                    break;
                case 'T':
                    CHECK_NULL(vict_obj, (char *) vict_obj);
                    dg_arg = (char *) vict_obj;
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
    to->desc->send_to("%s", CAP(lbuf));

    if ((IS_NPC(to) && dg_act_check) && (to != ch))
        act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);

    if (last_act_message)
        free(last_act_message);
    last_act_message = strdup(lbuf);

}

char *act(const char *str, int hide_invisible, Character *ch,
          Object *obj, const void *vict_obj, int type) {
    std::vector<std::weak_ptr<Character>> to;
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
        if (ch->mutations.get(Mutation::natural_camouflage)) {
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
        auto to = (Character *) vict_obj;
        if (to != nullptr && SENDOK(to) &&
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
                !i->character->location.getRoomFlag(ROOM_SOUNDPROOF)) {

                snprintf(buf, sizeof(buf), "@y%s@n", str);
                perform_act(buf, ch, obj, vict_obj, i->character);
                char buf2[MAX_STRING_LENGTH];
                snprintf(buf2, sizeof(buf2), "%s\r\n", buf);
                add_history(i->character, buf2, HIST_GOSSIP);
            }
        }
        return last_act_message;
    }

    /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

    if (ch && IN_ROOM(ch) != NOWHERE)
        to = ch->location.getPeople();
    else if (obj && IN_ROOM(obj) != NOWHERE)
        to = obj->location.getPeople();
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
                        if (arena_watch(d->character) == ch->location.getVnum()) {
                            char buf3[2000];
                            *buf3 = '\0';
                            sprintf(buf3, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", str);
                            perform_act(buf3, ch, obj, vict_obj, d->character);
                        }
                    }
                }
            }
            if (auto eaves = GET_EAVESDROP(d->character); eaves > 0) {
                int roll = rand_number(1, 101);
                if (!resskill || (roll_skill(d->character, resskill) >= dcval)) {
                    if (ch != nullptr && eaves == ch->location.getVnum() &&
                        GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
                        char buf3[1000];
                        *buf3 = '\0';
                        sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
                        perform_act(buf3, ch, obj, vict_obj, d->character);
                    } else if (obj != nullptr && eaves == obj->location.getVnum() &&
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
    for (auto target : filter_raw(to)) {
        if (!SENDOK(target) || (target == ch))
            continue;
        if (hide_invisible && ch && !CAN_SEE(target, ch))
            continue;
        if (type != TO_ROOM && target == vict_obj)
            continue;
        if (resskill && roll_skill(target, resskill) < dcval)
            continue;
        perform_act(str, ch, obj, vict_obj, target);
    }
    return last_act_message;
}


void descriptor_data::handle_input() {
    // Now we need to process the raw_input_queue, watching for special characters and also aliases.
    // Commands are processed first-come-first served...
    for(auto &command : raw_input_queue) {
        if (snoop_by)
            snoop_by->send_to("%% %s\r\n", command.c_str());

        if(command == "--") {
            // this is a special command that clears out the processed input_queue.
            input_queue.clear();
            character->wait_input_queue.clear();
            sendText("All queued commands cancelled.\r\n");
            if (character->task != Task::nothing) {
                character->setTask(Task::nothing);
                sendText("You stop focussing on your task.\r\n");
            }
            characterSubscriptions.unsubscribe("commandWaitQueue", character);
        } else {
            perform_alias(this, (char*)command.c_str());
        }
    }
    raw_input_queue.clear();

    if(input_queue.empty())
        return;

    auto command = input_queue.front();
    input_queue.pop_front();

    has_prompt = false;

    auto comm = (char*)command.c_str();

    if (str)        /* Writing boards, mail, etc. */
        string_add(this, comm);
    else if(std_str) {
        std_string_add(this, comm);
    }
    else if (STATE(this) != CON_PLAYING) /* In menus, etc. */
        nanny(this, comm);
    else {            /* else: we're playing normally. */
        try {
            command_interpreter(character, comm); /* Send it to interpreter */
        }
        catch(const std::exception & err) {
            basic_mud_log("Exception when running Command Interpreter for %s: %s", GET_NAME(character), err.what());
            basic_mud_log("Command was: %s", comm);
            shutdown_game(EXIT_FAILURE);
        }
        catch(...) {
            basic_mud_log("Unknown exception when running Command Interpreter for %s", GET_NAME(character));
            basic_mud_log("Command was: %s", comm);
            shutdown_game(EXIT_FAILURE);
        }
    }

}

void shutdown_game(int exitCode) {
    basic_mud_log("Process exiting with exit code %d", exitCode);
    std::exit(exitCode);
}

void descriptor_data::onConnectionClosed(int64_t connId) {
    conns.erase(connId);
    if(conns.empty()) {
        handleLostLastConnection(true);
    }
}

void descriptor_data::onConnectionLost(int64_t connId) {
    conns.erase(connId);
    if(conns.empty()) {
        handleLostLastConnection(false);
    }
}

void descriptor_data::handleLostLastConnection(bool graceful) {
    // At the moment, it doesn't really matter if the disconnect was graceful or not.
    // If they didn't use 'quit', then we have a problem.
    // We need to set the timeout for this character...
    if(character) {
        act("$n has lost $s link.", true, character, nullptr, nullptr, TO_ROOM);
    }
}