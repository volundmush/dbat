/*************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <fstream>
#include <regex>
#include <thread>

#include "fmt/core.h"
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "magic_enum/magic_enum_all.hpp"

#include "dbat/db.h"
#include "dbat/send.h"
#include "dbat/feats.h"
#include "dbat/config.h"
#include "dbat/players.h"
#include "dbat/spec_assign.h"
#include "dbat/act.informative.h"
#include "dbat/act.other.h"
#include "dbat/act.social.h"
#include "dbat/assemblies.h"
#include "dbat/reset.h"
#include "dbat/class.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/interpreter.h"
#include "dbat/genolc.h"
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/handler.h"
#include "dbat/mail.h"
#include "dbat/clan.h"
#include "dbat/boards.h"
#include "dbat/constants.h"
#include "dbat/spells.h"
#include "dbat/races.h"
#include "dbat/genobj.h"
#include "dbat/account.h"
#include "dbat/maputils.h"
#include "dbat/saveload.h"


/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

bool gameIsLoading = true;
bool saveAll = false;
bool isMigrating = false;


struct config_data config_info; /* Game configuration list.    */

// The global database of entities.
NegativeKeyGuardUnorderedMap<int, std::shared_ptr<unit_data>> units;
NegativeKeyGuardMap<room_vnum, std::shared_ptr<room_data>> world;
NegativeKeyGuardUnorderedMap<int, std::shared_ptr<char_data>> uniqueCharacters;
NegativeKeyGuardUnorderedMap<int, std::shared_ptr<obj_data>> uniqueObjects;

struct char_data *affect_list = nullptr; /* global linked list of chars with affects */
struct char_data *affectv_list = nullptr; /* global linked list of chars with round-based affects */
NegativeKeyGuardMap<mob_vnum, struct index_data> mob_index;    /* index table for mobile file	 */
NegativeKeyGuardMap<mob_vnum, struct npc_proto_data> mob_proto;    /* prototypes for mobs		 */

NegativeKeyGuardMap<obj_vnum, struct index_data> obj_index;    /* index table for object file	 */
NegativeKeyGuardMap<obj_vnum, struct item_proto_data> obj_proto;    /* prototypes for objs		 */

NegativeKeyGuardMap<zone_vnum, struct zone_data> zone_table;    /* zone table			 */

NegativeKeyGuardMap<trig_vnum, struct index_data> trig_index; /* index table for triggers      */

NegativeKeyGuardMap<int64_t, player_data> players;

NegativeKeyGuardMap<int64_t, struct descriptor_data*> sessions;

std::vector<std::weak_ptr<char_data>> getAllCharacters() {
    std::vector<std::weak_ptr<char_data>> out;
    out.reserve(uniqueCharacters.size());

    for(const auto&[id, ent] : uniqueCharacters)
        out.emplace_back(ent);

    return out;
}

std::vector<std::weak_ptr<obj_data>> getAllObjects() {
    std::vector<std::weak_ptr<obj_data>> out;
    out.reserve(uniqueObjects.size());

    for(const auto&[id, ent] : uniqueObjects)
        out.emplace_back(ent);

    return out;
}

room_data* get_room(room_vnum vn) {
    if(auto it = world.find(vn); it != world.end())
        return it->second.get();
    return nullptr;
}

int dg_owner_purged;            /* For control of scripts */

void destroy_db() {
    for(auto &[id, ent] : uniqueCharacters) {
        if(ent) ent->deactivate();
    }
    uniqueCharacters.clear();
    for(auto &[id, ent] : uniqueObjects) {
        if(ent) ent->deactivate();
    }
    uniqueObjects.clear();

    for(auto &[id, ent] : world) {
        if(ent) ent->deactivate();
    }
    world.clear();

    units.clear();
    
}

int no_mail = 0;        /* mail disabled?		 */
int mini_mud = 0;        /* mini-mud mode?		 */
int no_rent_check = 0;        /* skip rent check on boot?	 */
time_t boot_time = 0;        /* time of mud boot		 */
int circle_restrict = 0;    /* level of game restriction	 */
int dballtime = 0;              /* used by dragonball load system*/
int SHENRON = false;            /* Shenron has been summoned     */
int DRAGONR = 0;                /* Room Shenron has been summoned to */
int DRAGONZ = 0;                /* Zone Shenron has been summoned to */
int WISH[2] = {0, 0};           /* Keeps track of wishes granted */
int DRAGONC = 0;                /* Keeps count of Shenron's remaining time */
struct char_data *EDRAGON = nullptr;      /* This is Shenron when he is loaded */
room_rnum r_mortal_start_room;    /* rnum of mortal start room	 */
room_rnum r_immort_start_room;    /* rnum of immort start room	 */
room_rnum r_frozen_start_room;    /* rnum of frozen start room	 */
int converting = false;

char *credits = nullptr;        /* game credits			 */
char *news = nullptr;        /* mud news			 */
char *motd = nullptr;        /* message of the day - mortals  */
char *imotd = nullptr;        /* message of the day - immorts  */
char *GREETINGS = nullptr;        /* opening credits screen	 */
char *GREETANSI = nullptr;        /* ansi opening credits screen	 */
char *help = nullptr;        /* help screen			 */
char *info = nullptr;        /* info page			 */
char *wizlist = nullptr;        /* list of higher gods		 */
char *immlist = nullptr;        /* list of peon gods		 */
char *background = nullptr;    /* background story		 */
char *handbook = nullptr;        /* handbook for new immortals	 */
char *policies = nullptr;        /* policies page		 */
char *ihelp = nullptr;        /* help screen (immortals)	 */

struct help_index_element *help_table = nullptr;    /* the help table	 */
int top_of_helpt = 0;

struct social_messg *soc_mess_list = nullptr;      /* list of socials */
int top_of_socialt = -1;                        /* number of socials */

struct time_info_data old_time_info;/* the infomation about the time    */
struct time_info_data time_info;/* the infomation about the time    */
struct time_info_data era_uptime;/* the infomation about how long the server has been up    */
struct weather_data weather_info;    /* the infomation about the weather */
std::unordered_set<zone_vnum> zone_reset_queue;

std::vector<obj_vnum> dbVnums = {20, 21, 22, 23, 24, 25, 26};

SubscriptionManager<char_data> characterSubscriptions;
SubscriptionManager<obj_data> objectSubscriptions;
SubscriptionManager<room_data> roomSubscriptions;
SubscriptionManager<trig_data> triggerSubscriptions;

/* local functions */
static void dragon_level(struct char_data *ch);

static int file_to_string(const char *name, char *buf);

static int file_to_string_alloc(const char *name, char **buf);

static int count_alias_records(FILE *fl);

static void get_one_line(FILE *fl, char *buf);

static void log_zone_error(zone_rnum zone, int cmd_no, const char *message);

static void reset_time();

void mag_assign_spells();

void create_command_list();

void sort_spells();

void load_banned();

void Read_Invalid_List();

void memorize_add(struct char_data *ch, int spellnum, int timer);

void free_feats();

void free_assemblies();

// sticking this here for a quick use of get_help...
extern int search_help(const char *argument, int level);

/* external vars */

struct help_index_element *get_help(const std::string &name, int level) {
    if (!help_table || name.empty())
        return nullptr;

    if(auto idx = search_help(name.c_str(), level); idx != NOTHING) {
        return &help_table[idx];
    }
    
    return nullptr;
}


static void dragon_level(struct char_data *ch) {
    struct descriptor_data *d;
    int level = 0, count = 0;

    for (d = descriptor_list; d; d = d->next) {
        if (IS_PLAYING(d) && GET_ADMLEVEL(d->character) < 1) {
            level += GET_LEVEL(d->character);
            count += 1;
        }
    }

    if (level > 0 && count > 0) {
        level = level / count;
    } else {
        level = rand_number(60, 110);
    }

    if (level < 50) {
        level = rand_number(40, 60);
    }

    ch->setBaseStat<int>("level", level + rand_number(5, 20));
}


/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists() {
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files() {
    char **textfiles[] = {
            &wizlist, &immlist, &news, &credits, &motd, &imotd, &help, &info,
            &policies, &handbook, &background, &GREETINGS, &GREETANSI, &ihelp, nullptr
    };
    int rf;

    for (rf = 0; textfiles[rf]; rf++)
        if (*textfiles[rf]) {
            free(*textfiles[rf]);
            *textfiles[rf] = nullptr;
        }
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot) {
    send_to_char(ch, "Not a thing anymore.\r\n");
}


static void db_load_activate_entities() {
    // activate all items which ended up "in the world".
    for(auto &[id, r] : world) {
        assign_triggers(r.get(), WLD_TRIGGER);
        r->activateScripts();
        r->activateContents();
        auto people = r->getPeople();
        for(auto c : filter_raw(people)) {
            if(IS_NPC(c)) {
                c->activate();
            }
        }
    }
}


static std::vector<std::filesystem::path> getDumpFiles() {
    std::filesystem::path dir = "dumps"; // Change to your directory
    std::vector<std::filesystem::path> directories;

    auto pattern = "dump-";
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_directory() && entry.path().filename().string().starts_with(pattern)) {
            directories.push_back(entry.path());
        }
    }

    std::sort(directories.begin(), directories.end(), std::greater<>());
    return directories;
}

void boot_db_world() {

    auto dumps = getDumpFiles();
    if (!dumps.empty()) {
        std::cout << "Newest state dump: " << dumps.front() << '\n';
    } else {
        std::cout << "No matching state dumps found.\n";
        return;
    }

    auto latest = dumps.front();

    basic_mud_log("Loading stat handlers...");
    init_stat_handlers();

    basic_mud_log("Loading Zones...");
    load_zones(latest);

    basic_mud_log("Loading DgScripts and generating index.");
    load_dgscript_prototypes(latest);

    basic_mud_log("Loading mobs and generating index.");
    load_npc_prototypes(latest);

    basic_mud_log("Loading objs and generating index.");
    load_item_prototypes(latest);

    basic_mud_log("Loading rooms.");
    load_rooms(latest);

    basic_mud_log("Loading shops.");
    load_shops(latest);

    basic_mud_log("Loading guild masters.");
    load_guilds(latest);

    basic_mud_log("Loading exits.");
    load_exits(latest);

    basic_mud_log("Loading global data...");
    load_globaldata(latest);

    basic_mud_log("Loading accounts.");
    load_accounts(latest);

    basic_mud_log("Loading players.");
    load_players(latest);

    basic_mud_log("Loading characters initial...");
    load_characters_initial(latest);

    basic_mud_log("Loading items initial...");
    load_items_initial(latest);

    // Now that all of the game entities have been spawned, we can finish loading
    // relations between them.

    basic_mud_log("Loading characters finish...");
    load_characters_finish(latest);

    basic_mud_log("Loading items finish...");
    load_items_finish(latest);

    basic_mud_log("Loading dgscript instances...");
    load_dgscripts(latest);

    basic_mud_log("Running activation of entities...");
    db_load_activate_entities();

    basic_mud_log("Checking start rooms.");
    check_start_rooms();

    basic_mud_log("Loading disabled commands list...");
    load_disabled();

    basic_mud_log("Loading help entries.");
    load_help(latest);

    basic_mud_log("Loading assemblies.");
    load_assemblies(latest);

    boot_db_shadow();
}

void boot_db_shadow() {
    if (SELFISHMETER >= 10) {
        basic_mud_log("Loading Shadow Dragons.");
        load_shadow_dragons();
    }
}



void free_extra_descriptions(struct extra_descr_data *edesc) {
    struct extra_descr_data *enext;

    for (; edesc; edesc = enext) {
        enext = edesc->next;

        free(edesc->keyword);
        free(edesc->description);
        free(edesc);
    }
}


/* Free the world, in a memory allocation sense. */

/* You can define this to anything you want; 1 would work but it would
   be very inefficient. I would recommend that it actually be close to
   your total number of in-game objects if not double or triple it just
   to minimize collisions. The only O(n) [n=NUM_OBJ_UNIQUE_POOLS]
   operation is initialization of the hash table, all other operations
   that have to traverse are O(n) [n=num elements in pool], so more
   pools are better.
     - Elie Rosenblum Dec. 12 2003 */
constexpr int NUM_OBJ_UNIQUE_POOLS = 5000;

void boot_db_textfiles() {
    basic_mud_log("Reading news, credits, help, ihelp, bground, info & motds.");
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
    file_to_string_alloc(IHELP_PAGE_FILE, &ihelp);
    file_to_string_alloc(GREETINGS_FILE, &GREETINGS);
    file_to_string_alloc(GREETANSI_FILE, &GREETANSI);
}

void boot_db_time() {
    basic_mud_log("Resetting the game time:");
    reset_time();
}

void boot_db_spellfeats() {
    basic_mud_log("Loading spell definitions.");
    mag_assign_spells();

    basic_mud_log("Loading feats.");
    assign_feats();
}

void boot_db_help() {
    basic_mud_log("Setting up context sensitive help system for OLC");
    boot_context_help();
}

void boot_db_mail() {
    basic_mud_log("Booting mail system.");
    if (!scan_file()) {
        basic_mud_log("    Mail boot failed -- Mail system disabled");
    }
}

void boot_db_socials() {
    basic_mud_log("Loading social messages.");
    boot_social_messages();
}

void boot_db_clans() {
    basic_mud_log("Loading Clans.");
    clanBoot();
}

void boot_db_commands() {
    basic_mud_log("Building command list.");
    create_command_list(); /* aedit patch -- M. Scott */
}

void boot_db_specials() {
    basic_mud_log("Assigning function pointers:");
    basic_mud_log("   Mobiles.");
    assign_mobiles();
    basic_mud_log("   Shopkeepers.");
    assign_the_shopkeepers();
    basic_mud_log("   Objects.");
    assign_objects();
    basic_mud_log("   Rooms.");
    assign_rooms();
    basic_mud_log("   Guildmasters.");
    assign_the_guilds();
}


void boot_db_sort() {
    basic_mud_log("Sorting command list and spells.");
    sort_commands();
    sort_spells();
    sort_feats();
}

void boot_db_boards() {
    basic_mud_log("Booting boards system.");
    init_boards();
}

void boot_db_banned() {
    basic_mud_log("Reading banned site and invalid-name list.");
    load_banned();
    Read_Invalid_List();
}


void boot_db_spacemap() {
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


/* body of the booting system */


void boot_db_new() {
    boot_db_time();
    boot_db_textfiles();
    boot_db_spellfeats();
    boot_db_world();
    boot_db_mail();
    boot_db_socials();
    boot_db_clans();
    boot_db_commands();
    boot_db_help();
    boot_db_specials();
    boot_db_sort();
    boot_db_boards();
    boot_db_banned();
    boot_db_spacemap();
    topLoad();
}


/* save the auction file */
void auc_save() {
    FILE *fl;

    if ((fl = fopen(AUCTION_FILE, "w")) == nullptr)
        basic_mud_log("SYSERR: Can't write to '%s' auction file.", AUCTION_FILE);
    else {
        auto con = get_room(80)->getObjects();
        for (auto obj : filter_raw(con)) {
            fprintf(fl, "%d %s %ld %ld %d %d %ld\n", obj->id, GET_AUCTERN(obj), GET_AUCTER(obj),
                        GET_CURBID(obj), GET_STARTBID(obj), GET_BID(obj), GET_AUCTIME(obj));
        }
        fprintf(fl, "~END~\n");
        fclose(fl);
    }
}

/* load from auction file */
void auc_load(struct obj_data *obj) {
    char line[500], filler[50];
    int64_t oID;
    time_t timer;
    int aID, bID, cost, startc;
    FILE *fl;

    if ((fl = fopen(AUCTION_FILE, "r")) == nullptr)
        basic_mud_log("SYSERR: Can't read from '%s' auction file.", AUCTION_FILE);
    else {
        while (!feof(fl)) {
            get_line(fl, line);
            sscanf(line, "%" I64T " %s %d %d %d %d %ld\n", &oID, filler, &aID, &bID, &startc, &cost, &timer);
            if (obj->id == oID) {
                GET_AUCTERN(obj) = strdup(filler);
                GET_AUCTER(obj) = aID;
                GET_CURBID(obj) = bID;
                GET_STARTBID(obj) = startc;
                GET_BID(obj) = cost;
                GET_AUCTIME(obj) = timer;
            }
        }
        fclose(fl);
    }
}

time_t old_beginning_of_time;

/* reset the time in the game from file */
static void reset_time() {
    time_t beginning_of_time = 0;
    FILE *bgtime;

    if ((bgtime = fopen(TIME_FILE, "r")) == nullptr)
        basic_mud_log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
    else {
        fscanf(bgtime, "%ld\n", &beginning_of_time);
        fscanf(bgtime, "%ld\n", &NEWSUPDATE);
        fscanf(bgtime, "%ld\n", &BOARDNEWMORT);
        fscanf(bgtime, "%ld\n", &BOARDNEWDUO);
        fscanf(bgtime, "%ld\n", &BOARDNEWCOD);
        fscanf(bgtime, "%ld\n", &BOARDNEWBUI);
        fscanf(bgtime, "%ld\n", &BOARDNEWIMM);
        fscanf(bgtime, "%ld\n", &INTERESTTIME);
        fscanf(bgtime, "%ld\n", &LASTINTEREST);
        fscanf(bgtime, "%d\n", &HIGHPCOUNT);
        fscanf(bgtime, "%ld\n", &PCOUNTDATE);
        fscanf(bgtime, "%d\n", &WISHTIME);
        fscanf(bgtime, "%d\n", &PCOUNT);
        fscanf(bgtime, "%ld\n", &LASTPAYOUT);
        fscanf(bgtime, "%d\n", &LASTPAYTYPE);
        fscanf(bgtime, "%d\n", &LASTNEWS);
        fscanf(bgtime, "%d\n", &dballtime);
        fscanf(bgtime, "%d\n", &SELFISHMETER);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON1);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON2);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON3);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON4);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON5);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON6);
        fscanf(bgtime, "%d\n", &SHADOW_DRAGON7);
        fscanf(bgtime, "%d\n", &ERAPLAYERS);
        fclose(bgtime);
    }

    if (dballtime == 0)
        dballtime = 604800;

    if (beginning_of_time == 0)
        beginning_of_time = 650336715;
    old_beginning_of_time = beginning_of_time;

}


/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when) {
    FILE *bgtime;

    if ((bgtime = fopen(TIME_FILE, "w")) == nullptr)
        basic_mud_log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
    else {
        fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
        fprintf(bgtime, "%ld\n", NEWSUPDATE);
        fprintf(bgtime, "%ld\n", BOARDNEWMORT);
        fprintf(bgtime, "%ld\n", BOARDNEWDUO);
        fprintf(bgtime, "%ld\n", BOARDNEWCOD);
        fprintf(bgtime, "%ld\n", BOARDNEWBUI);
        fprintf(bgtime, "%ld\n", BOARDNEWIMM);
        fprintf(bgtime, "%ld\n", INTERESTTIME);
        fprintf(bgtime, "%ld\n", LASTINTEREST);
        fprintf(bgtime, "%d\n", HIGHPCOUNT);
        fprintf(bgtime, "%ld\n", PCOUNTDATE);
        fprintf(bgtime, "%d\n", WISHTIME);
        fprintf(bgtime, "%d\n", PCOUNT);
        fprintf(bgtime, "%ld\n", LASTPAYOUT);
        fprintf(bgtime, "%d\n", LASTPAYTYPE);
        fprintf(bgtime, "%d\n", LASTNEWS);
        fprintf(bgtime, "%d\n", dballtime);
        fprintf(bgtime, "%d\n", SELFISHMETER);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON1);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON2);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON3);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON4);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON5);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON6);
        fprintf(bgtime, "%d\n", SHADOW_DRAGON7);
        fprintf(bgtime, "%d\n", ERAPLAYERS);
        fclose(bgtime);
    }
}



bitvector_t asciiflag_conv(char *flag) {
    bitvector_t flags = 0;
    int is_num = true;
    char *p;

    for (p = flag; *p; p++) {
        if (islower(*p))
            flags |= 1 << (*p - 'a');
        else if (isupper(*p))
            flags |= 1 << (26 + (*p - 'A'));

        if (!(isdigit(*p) || (*p == '-')))
            is_num = false;
    }

    if (is_num)
        flags = atol(flag);

    return (flags);
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms() {
    if ((r_mortal_start_room = real_room(CONFIG_MORTAL_START)) == NOWHERE) {
        basic_mud_log("SYSERR:  Mortal start room does not exist.  Change mortal_start_room in lib/etc/config.");
        exit(1);
    }
    if ((r_immort_start_room = real_room(CONFIG_IMMORTAL_START)) == NOWHERE) {
        if (!mini_mud)
            basic_mud_log("SYSERR:  Warning: Immort start room does not exist.  Change immort_start_room in /lib/etc/config.");
        r_immort_start_room = r_mortal_start_room;
    }
    if ((r_frozen_start_room = real_room(CONFIG_FROZEN_START)) == NOWHERE) {
        if (!mini_mud)
            basic_mud_log("SYSERR:  Warning: Frozen start room does not exist.  Change frozen_start_room in /lib/etc/config.");
        r_frozen_start_room = r_mortal_start_room;
    }
}


void free_help(struct help_index_element *cmhelp) {
    if (cmhelp->keywords)
        free(cmhelp->keywords);
    if (cmhelp->entry && !cmhelp->duplicate)
        free(cmhelp->entry);

    free(cmhelp);
}

void free_help_table() {
    if (help_table) {
        int hp;
        for (hp = 0; hp < top_of_helpt; hp++) {
            free_help(&help_table[hp]);
        }
        free(help_table);
        help_table = nullptr;
    }
    top_of_helpt = 0;
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/

int vnum_mobile(char *searchname, struct char_data *ch) {
    int found = 0;

    for (auto &[vn, m] : mob_proto)
        if (isname(searchname, m.name))
            send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                         ++found, vn, m.short_description,
                         !m.proto_script.empty() ? m.scriptString().c_str() : "");

    return (found);
}


int vnum_object(char *searchname, struct char_data *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (isname(searchname, o.second.name))
            send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                         ++found, o.first, o.second.short_description,
                         !o.second.proto_script.empty() ? o.second.scriptString().c_str() : "");

    return (found);
}


int vnum_material(char *searchname, struct char_data *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (isname(searchname, material_names[o.second.getBaseStat<int>(VAL_ALL_MATERIAL)])) {
            send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                         ++found, o.first, o.second.short_description,
                         !o.second.proto_script.empty() ? o.second.scriptString().c_str() : "");
        }

    return (found);
}


int vnum_weapontype(char *searchname, struct char_data *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (o.second.type_flag == ItemType::weapon) {
            if (isname(searchname, weapon_type[o.second.getBaseStat<int>(VAL_WEAPON_SKILL)])) {
                send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                             ++found, o.first, o.second.short_description,
                             !o.second.proto_script.empty() ? o.second.scriptString().c_str() : "");
            }
        }

    return (found);
}


int vnum_armortype(char *searchname, struct char_data *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (o.second.type_flag == ItemType::armor) {
            if (isname(searchname, armor_type[o.second.getBaseStat<int>(VAL_ARMOR_SKILL)])) {
                send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                             ++found, o.first, o.second.short_description,
                             !o.second.proto_script.empty() ? o.second.scriptString().c_str() : "");
            }
        }

    return (found);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
    auto proto = mob_proto.find(nr);

    if(proto == mob_proto.end()) {
        basic_mud_log("WARNING: Mobile vnum %d does not exist in database.", nr);
        return (nullptr);
    }
    auto sh = std::make_shared<char_data>();
    auto mob = sh.get();

    *mob = proto->second;
    mob->id = getNextUnitID();
    mob->generation = time(nullptr);
    uniqueCharacters.emplace(mob->id, sh);
    units.emplace(mob->id, sh);

    mob->activate();

    if (!(IS_HOSHIJIN(mob) && GET_SEX(mob) == SEX_MALE)) {
        //setNumsTo[CharAppearance::hair_length] = rand_number(0, 4);
        //setNumsTo[CharAppearance::hair_color] = rand_number(1, 13);
        //setNumsTo[CharAppearance::hair_style] = rand_number(1, 11);
    }

    //setNumsTo[CharAppearance::eye_color] = rand_number(0, 11);

    if (!IS_HUMAN(mob) && !IS_SAIYAN(mob) && !IS_HALFBREED(mob) && !IS_NAMEK(mob)) {
        //setNumsTo[CharAppearance::skin_color] = rand_number(0, 11);
    }
    if (IS_NAMEK(mob)) {
        //setNumsTo[CharAppearance::skin_color] = 2;
    }
    if (IS_HUMAN(mob) || IS_SAIYAN(mob) || IS_HALFBREED(mob)) {
        /*
        if (rand_number(1, 5) <= 2) {
            setNumsTo[CharAppearance::skin_color] = rand_number(0, 1);
        } else if (rand_number(1, 5) <= 4) {
            setNumsTo[CharAppearance::skin_color] = rand_number(4, 5);
        } else if (rand_number(1, 5) <= 5) {
            setNumsTo[CharAppearance::skin_color] = rand_number(9, 10);
        }
            */
    }
    if (IS_SAIYAN(mob)) {
        //setNumsTo[CharAppearance::hair_color] = HAIRC_BLACK;
        //setNumsTo[CharAppearance::eye_color] = 1;
    }

    if (GET_MOB_VNUM(mob) >= 81 && GET_MOB_VNUM(mob) <= 87) {
        dragon_level(mob);
    }

    int64_t mult = 0;

    switch (GET_LEVEL(mob)) {
        case 1:
            mult = rand_number(50, 80);
            break;
        case 2:
            mult = rand_number(90, 120);
            break;
        case 3:
            mult = rand_number(100, 140);
            break;
        case 4:
            mult = rand_number(120, 180);
            break;
        case 5:
            mult = rand_number(200, 250);
            break;
        case 6:
            mult = rand_number(240, 300);
            break;
        case 7:
            mult = rand_number(280, 350);
            break;
        case 8:
            mult = rand_number(320, 400);
            break;
        case 9:
            mult = rand_number(380, 480);
            break;
        case 10:
            mult = rand_number(500, 600);
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            mult = rand_number(1200, 1600);
            break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
            mult = rand_number(2400, 3000);
            break;
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            mult = rand_number(5500, 8000);
            break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
            mult = rand_number(10000, 14000);
            break;
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
            mult = rand_number(16000, 20000);
            break;
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
            mult = rand_number(22000, 30000);
            break;
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
            mult = rand_number(50000, 70000);
            break;
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
            mult = rand_number(95000, 140000);
            break;
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
            mult = rand_number(180000, 250000);
            break;
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
            mult = rand_number(400000, 480000);
            break;
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
            mult = rand_number(700000, 900000);
            break;
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
            mult = rand_number(1400000, 1600000);
            break;
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
            mult = rand_number(2200000, 2500000);
            break;
        case 76:
        case 77:
        case 78:
        case 79:
        case 80:
            mult = rand_number(3000000, 3500000);
            break;
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
            mult = rand_number(4250000, 4750000);
            break;
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
            mult = rand_number(6500000, 8500000);
            break;
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
            mult = rand_number(15000000, 18000000);
            break;
        case 96:
        case 97:
        case 98:
        case 99:
        case 100:
            mult = rand_number(22000000, 30000000);
            break;
        case 101:
            mult = rand_number(32000000, 40000000);
            break;
        case 102:
            mult = rand_number(42000000, 55000000);
            break;
        case 103:
            mult = rand_number(80000000, 95000000);
            break;
        case 104:
            mult = rand_number(150000000, 200000000);
            break;
        case 105:
            mult = rand_number(220000000, 250000000);
            break;
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
            mult = rand_number(500000000, 750000000);
            break;
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 116:
        case 117:
        case 118:
        case 119:
        case 120:
            mult = rand_number(800000000, 900000000);
            break;
        default:
            if (GET_LEVEL(mob) >= 150) {
                mult = rand_number(1500000000, 2000000000);
            } else {
                mult = rand_number(1250000000, 1500000000);
            }
            break;
    }

    mob->setBaseStat("last_played", time(nullptr));
    bool autoset = mob->getBaseStat<int64_t>("health") <= 1;
    if(autoset) {
        for(auto c : {"health", "ki", "stamina"}) {
            vital_t base = GET_LEVEL(mob) * mult;
            if (GET_LEVEL(mob) > 140) {
                base *= 8;
            } else if (GET_LEVEL(mob) > 130) {
                base *= 6;
            } else if (GET_LEVEL(mob) > 120) {
                base *= 3;
            } else if (GET_LEVEL(mob) > 110) {
                base *= 2;
            }
            mob->setBaseStat(c, base);
        }
    }

    if (GET_MOB_VNUM(mob) == 2245) {
        for(auto c : {"health", "ki", "stamina"}) {
            mob->setBaseStat(c, rand_number(1, 4));
        }
    }

    int base = 0;
    switch (GET_LEVEL(mob)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            base = rand_number(80, 120);
            break;
        case 6:
            base = rand_number(200, 280);
            break;
        case 7:
            base = rand_number(250, 350);
            break;
        case 8:
            base = rand_number(275, 375);
            break;
        case 9:
            base = rand_number(300, 400);
            break;
        case 10:
            base = rand_number(325, 450);
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            base = rand_number(500, 700);
            break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
            base = rand_number(700, 1000);
            break;
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            base = rand_number(1000, 1200);
            break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
            base = rand_number(1200, 1400);
            break;
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
            base = rand_number(1400, 1600);
            break;
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
            base = rand_number(1600, 1800);
            break;
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
            base = rand_number(1800, 2000);
            break;
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
            base = rand_number(2000, 2200);
            break;
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
            base = rand_number(2200, 2500);
            break;
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
            base = rand_number(2500, 2800);
            break;
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
            base = rand_number(2800, 3000);
            break;
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
            base = rand_number(3000, 3200);
            break;
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
            base = rand_number(3200, 3500);
            break;
        case 76:
        case 77:
        case 78:
        case 79:
            base = rand_number(3500, 3800);
            break;
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
            base = rand_number(4000, 4500);
            break;
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
            base = rand_number(4500, 5500);
            break;
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
            base = rand_number(5500, 7000);
            break;
        case 96:
        case 97:
        case 98:
        case 99:
            base = rand_number(8000, 10000);
            break;
        case 100:
            base = rand_number(10000, 15000);
            break;
        case 101:
            base = rand_number(15000, 25000);
            break;
        case 102:
            base = rand_number(35000, 40000);
            break;
        case 103:
            base = rand_number(40000, 50000);
            break;
        case 104:
            base = rand_number(60000, 80000);
            break;
        case 105:
            base = rand_number(80000, 100000);
            break;
        default:
            base = rand_number(130000, 180000);
            break;
    }

    auto money = GET_GOLD(mob);
    if (money <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
        if (GET_LEVEL(mob) < 4) {
            money = GET_LEVEL(mob) * rand_number(1, 2);
        } else if (GET_LEVEL(mob) < 10) {
            money = (GET_LEVEL(mob) * rand_number(1, 2)) - 1;
        } else if (GET_LEVEL(mob) < 20) {
            money = (GET_LEVEL(mob) * rand_number(1, 3)) - 2;
        } else if (GET_LEVEL(mob) < 30) {
            money = (GET_LEVEL(mob) * rand_number(1, 3)) - 4;
        } else if (GET_LEVEL(mob) < 40) {
            money = (GET_LEVEL(mob) * rand_number(1, 3)) - 6;
        } else if (GET_LEVEL(mob) < 50) {
            money = (GET_LEVEL(mob) * rand_number(2, 3)) - 25;
        } else if (GET_LEVEL(mob) < 60) {
            money = (GET_LEVEL(mob) * rand_number(2, 3)) - 40;
        } else if (GET_LEVEL(mob) < 70) {
            money = (GET_LEVEL(mob) * rand_number(2, 3)) - 50;
        } else if (GET_LEVEL(mob) < 80) {
            money = (GET_LEVEL(mob) * rand_number(2, 4)) - 60;
        } else if (GET_LEVEL(mob) < 90) {
            money = (GET_LEVEL(mob) * rand_number(2, 4)) - 70;
        } else {
            money = (GET_LEVEL(mob) * rand_number(3, 4)) - 85;
        }
        if (!IS_HUMANOID(mob)) {
            money = GET_GOLD(mob) * 0.5;
            if (GET_GOLD(mob) <= 0)
                money = 1;
        }
        mob->setBaseStat("money_carried", money);
    }

    
    if (GET_EXP(mob) <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
        int64_t mexp = GET_LEVEL(mob) * base;
        mexp = mexp * .9;
        mexp += GET_LEVEL(mob) / 2;
        mexp += GET_LEVEL(mob) / 3;
        if (IS_DRAGON(mob)) {
            mexp *= 1.4;
        } else if (IS_ANDROID(mob)) {
            mexp *= 1.25;
        } else if (IS_SAIYAN(mob)) {
            mexp *= 1.1;
        } else if (IS_BIO(mob)) {
            mexp *= 1.2;
        } else if (IS_MAJIN(mob)) {
            mexp *= 1.25;
        } else if (IS_DEMON(mob)) {
            mexp *= 1.1;
        }
        if (GET_CLASS(mob) == Sensei::commoner && IS_HUMANOID(mob) && !IS_DRAGON(mob)) {
            if (!IS_ANDROID(mob) && !IS_SAIYAN(mob) && !IS_BIO(mob) && !IS_MAJIN(mob)) {
                mexp *= 0.75;
            }
        }

        if (GET_LEVEL(mob) > 90) {
            mexp = mexp * .7;
        } else if (GET_LEVEL(mob) > 80) {
            mexp = mexp * .75;
        } else if (GET_LEVEL(mob) > 70) {
            mexp = mexp * .8;
        } else if (GET_LEVEL(mob) > 60) {
            mexp = mexp * .85;
        } else if (GET_LEVEL(mob) > 40) {
            mexp = mexp * .9;
        } else if (GET_LEVEL(mob) > 30) {
            mexp = mexp * .95;
        }

        if (GET_EXP(mob) > 20000000) {
            mexp = 20000000;
        }
        mob->setExperience(mexp);
    }

    mob->setAge(birth_age(mob));
    mob->time.created = mob->time.logon = time(nullptr); /* why not */
    mob->time.played = 0.0;
    mob->time.logon = time(nullptr);
    mob->setBaseStat("position", mob->mob_specials.default_pos);
    for(const auto& i : {0, 1, 2, 3}) mob->limb_condition[i] = 100;

    assign_triggers(mob, MOB_TRIGGER);
    racial_body_parts(mob);

    if (GET_MOB_VNUM(mob) >= 800 && GET_MOB_VNUM(mob) <= 805) {
        number_of_assassins += 1;
    }

    return mob;
}

char *sprintuniques(int low, int high) {
    return strdup("Temporarily disabled.");
}


/* create an object, and add it to the object list */
struct obj_data *create_obj() {
    auto sh = std::make_shared<obj_data>();
    sh->id = getNextUnitID();
    sh->generation = time(nullptr);
    uniqueObjects.emplace(sh->id, sh);
    units.emplace(sh->id, sh);
    sh->activate();
    assign_triggers(sh.get(), OBJ_TRIGGER);
    return sh.get();
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
    auto i = nr;
    int j;

    auto proto = obj_proto.find(i);

    if (proto == obj_proto.end()) {
        basic_mud_log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
        return (nullptr);
    }
    auto sh = std::make_shared<obj_data>();
    auto obj = sh.get();
    // the operator= will copy the prototype data into the new object.
    *obj = proto->second;

    OBJ_LOADROOM(obj) = NOWHERE;
    obj->id = getNextUnitID();
    obj->generation = time(nullptr);
    uniqueObjects.emplace(obj->id, sh);
    units.emplace(obj->id, sh);

    
    if (nr == 65) {
        SET_OBJ_VAL(obj, VAL_BED_HTANK_CHARGE, 20);
    }
    if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
        if (GET_OBJ_VAL(obj, VAL_FOOD_MAXFOODVAL) == 0) {
            SET_OBJ_VAL(obj, VAL_FOOD_MAXFOODVAL, GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL));
        }
        FOOB(obj) = GET_OBJ_VAL(obj, VAL_FOOD_MAXFOODVAL);
    }

    obj->activate();

    return (obj);
}


constexpr int ZO_DEAD = 999;

static std::deque<zone_vnum> zonesToUpdate;

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(uint64_t heartPulse, double deltaTime) {

    for (auto &[vn, z] : zone_table) {
        z.age += deltaTime;
        auto secs = (z.lifespan * 60.0);
        if(z.age < secs) continue;

        bool doReset = false;
        switch(z.reset_mode) {
            case 0:
                // Never reset.
            break;
            case 1:
                // reset only if zone is empty.
                if(z.playersInZone.empty()) doReset = true;
            break;
            case 2:
                // Always reset.
                doReset = true;
            break;
            default:
                // This shouldn't happen.
                    break;
        }
        if(doReset) {
            zonesToUpdate.emplace_back(vn);

            z.age -= secs;

            break;
        }
    }

    // Stagger zone updates so they don't all happen in the exact same heartbeat.
    while(!zonesToUpdate.empty()) {
        auto vn = zonesToUpdate.front();
        auto& z = zone_table.at(vn);
        reset_zone(vn);
        mudlog(CMP, ADMLVL_GOD, false, "Auto zone reset: %s (Zone %d)",
               z.name, vn);
        zonesToUpdate.pop_front();
        break;
    }

}

#define ZCMD2 zone_table.at(zone).cmd[cmd_no]
static void log_zone_error(zone_rnum zone, int cmd_no, const char *message) {
    mudlog(NRM, ADMLVL_GOD, true, "SYSERR: zone file: %s", message);
    mudlog(NRM, ADMLVL_GOD, true, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
           ZCMD2.command, zone_table.at(zone).number, ZCMD2.line);
}

#define ZONE_ERROR(message) \
    { log_zone_error(zone, cmd_no, message); last_cmd = 0; }


static void do_reset_cmds(zone_data &z) {
    int cmd_no = 0, last_cmd = 0;
    struct char_data *mob = nullptr;
    struct obj_data *obj, *obj_to;
    room_vnum rvnum;
    room_rnum rrnum;
    struct char_data *tmob = nullptr; /* for trigger assignment */
    struct obj_data *tobj = nullptr;  /* for trigger assignment */
    int mob_load = false;             /* ### */
    int obj_load = false;             /* ### */
    auto oproto = obj_proto.find(-1);
    auto mproto = mob_proto.find(-1);
    struct room_data* room = nullptr;
    auto zone = z.number;

    for (auto &c : z.cmd)
    {
        cmd_no = c.line;
        if (c.command == 'S')
            break;

        if (c.if_flag && !last_cmd && !mob_load && !obj_load)
            continue;

        if (!c.if_flag)
        { /* ### */
            mob_load = false;
            obj_load = false;
        }

        /*  This is the list of actual zone commands.  If any new
         *  zone commands are added to the game, be certain to update
         *  the list of commands in load_zone() so that the counting
         *  will still be correct. - ae.
         */
        try
        {
            switch (c.command)
            {
            case '*': /* ignore command */
                last_cmd = 0;
                break;

            case 'M': /* read a mobile */
                room = get_room(c.arg3);
                if (mob_proto.contains(c.arg1) && (characterSubscriptions.count(fmt::format("vnum_{}", c.arg1)) < c.arg2) && room &&
                    (rand_number(1, 100) >= c.arg5))
                {
                    int room_max = 0;
                    struct char_data *i;

                    /* First find out how many mobs of VNUM are in the mud with this rooms */
                    /* VNUM as a load point for max from room checks. */
                    /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

                    if (c.arg4 > 0)
                    {
                        auto chars = characterSubscriptions.all(fmt::format("vnum_{}", c.arg1));
                        for (auto i : filter_raw(chars))
                        {
                            if (MOB_LOADROOM(i) == c.arg3)
                            {
                                if (++room_max >= c.arg4)
                                {
                                    // no need to keep counting more at this point...
                                    break;
                                }
                            }
                            if (room_max >= c.arg4)
                            {
                                break;
                            }
                        }
                        /* Break out if room_max has been met, ignore room_max if zero */
                        if (room_max >= c.arg4)
                        {
                            last_cmd = 0;
                            break;
                        }
                    }

                    mob = read_mobile(c.arg1, REAL);
                    /*  Set the mobs loadroom for room_max checks. */
                    char_to_room(mob, c.arg3);
                    mob->setBaseStat("hometown", c.arg3);

                    load_mtrigger(mob);
                    tmob = mob;
                    last_cmd = 1;
                    mob_load = true;
                }
                else
                    last_cmd = 0;
                tobj = nullptr;
                break;

            case 'O': /* read an object */
                room = get_room(c.arg3);
                oproto = obj_proto.find(c.arg1);
                if (oproto != obj_proto.end())
                {
                    if (oproto->second.type_flag == ItemType::hatch || oproto->second.type_flag == ItemType::control || oproto->second.type_flag == ItemType::window || oproto->second.type_flag == ItemType::vehicle)
                    {
                        c.arg2 = 1;
                        c.arg4 = 1;
                    }
                }

                if (obj_proto.contains(c.arg1) && objectSubscriptions.count(fmt::format("vnum_{}", c.arg1)) < c.arg2 &&
                    room && (rand_number(1, 100) >= c.arg5))
                {
                    int room_max = 0;

                    /* First find out how many obj of VNUM are in the mud with this rooms */
                    /* VNUM as a load point for max from room checks. */
                    /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

                    if (c.arg4 > 0)
                    {   
                        auto objects = objectSubscriptions.all(fmt::format("vnum_{}", c.arg1));
                        for (auto k : filter_raw(objects))
                        {
                            if (OBJ_LOADROOM(k) == c.arg3 && (IN_ROOM(k) == c.arg3))
                            {
                                if (++room_max >= c.arg4)
                                {
                                    // no need to keep counting more at this point...
                                    break;
                                }
                            }
                            if (room_max >= c.arg4)
                            {
                                /* Get rid of it if room_max has been met. */
                                break;
                            }
                        }
                        if (room_max >= c.arg4)
                        {
                            /* Get rid of it if room_max has been met. */
                            last_cmd = 0;
                            break;
                        }
                    }

                    obj = read_object(c.arg1, REAL);
                    obj_to_room(obj, c.arg3);
                    /* Set the loadroom for room_max checks */
                    OBJ_LOADROOM(obj) = c.arg3;

                    last_cmd = 1;
                    load_otrigger(obj);
                    tobj = obj;
                    obj_load = true;
                }
                else
                    last_cmd = 0;
                tmob = nullptr;
                break;

            case 'P': /* object to object */
                if (obj_proto.contains(c.arg1) && (objectSubscriptions.count(fmt::format("vnum_{}", c.arg1)) < c.arg2) &&
                    obj_load && (rand_number(1, 100) >= c.arg5))
                {

                    if (!(obj_to = get_obj_num(c.arg3)))
                    {
                        ZONE_ERROR("target obj not found, command disabled");
                        c.command = '*';
                        break;
                    }
                    obj = read_object(c.arg1, REAL);
                    obj_to_obj(obj, obj_to);
                    last_cmd = 1;
                    load_otrigger(obj);
                    tobj = obj;
                }
                else
                    last_cmd = 0;
                tmob = nullptr;
                break;

            case 'G': /* obj_to_char */
                if (!mob)
                {
                    ZONE_ERROR("attempt to give obj to non-existent mob, command disabled");
                    c.command = '*';
                    break;
                }
                if (obj_proto.contains(c.arg1) && (objectSubscriptions.count(fmt::format("vnum_{}", c.arg1)) < c.arg2) &&
                    mob_load && (rand_number(1, 100) >= c.arg5))
                {
                    obj = read_object(c.arg1, REAL);
                    obj_to_char(obj, mob);
                    if (GET_MOB_SPEC(mob) != shop_keeper)
                    {
                        randomize_eq(obj);
                    }
                    last_cmd = 1;
                    load_otrigger(obj);
                    tobj = obj;
                }
                else
                    last_cmd = 0;
                tmob = nullptr;
                break;

            case 'E': /* object to equipment list */
                if (!mob)
                {
                    ZONE_ERROR("trying to equip non-existant mob, command disabled");
                    c.command = '*';
                    break;
                }
                if (obj_proto.contains(c.arg1) && (objectSubscriptions.count(fmt::format("vnum_{}", c.arg1)) < c.arg2) &&
                    mob_load && (rand_number(1, 100) >= c.arg5))
                {
                    if (c.arg3 < 0 || c.arg3 >= NUM_WEARS)
                    {
                        ZONE_ERROR("invalid equipment pos number");
                    }
                    else
                    {
                        obj = read_object(c.arg1, REAL);
                        obj->room = mob->room;
                        load_otrigger(obj);
                        if (wear_otrigger(obj, mob, c.arg3))
                        {
                            obj->room = nullptr;
                            equip_char(mob, obj, c.arg3);
                        }
                        else
                            obj_to_char(obj, mob);
                        tobj = obj;
                        last_cmd = 1;
                    }
                }
                else
                    last_cmd = 0;
                tmob = nullptr;
                break;

            case 'R': /* rem obj from room */
                if (obj = get_room(c.arg1)->findObjectVnum(c.arg2))
                    extract_obj(obj);
                last_cmd = 1;
                tmob = nullptr;
                tobj = nullptr;
                break;

            case 'D': /* set state of door */
                room = get_room(c.arg1);
                if (!room || c.arg2 < 0 || c.arg2 >= NUM_OF_DIRS ||
                    (room->dir_option[c.arg2] == nullptr))
                {
                    ZONE_ERROR("room or door does not exist, command disabled");
                    c.command = '*';
                }
                else
                    switch (c.arg3)
                    {
                    case 0:
                        REMOVE_BIT(room->dir_option[c.arg2]->exit_info,
                                   EX_LOCKED);
                        REMOVE_BIT(room->dir_option[c.arg2]->exit_info,
                                   EX_CLOSED);
                        break;
                    case 1:
                        SET_BIT(room->dir_option[c.arg2]->exit_info,
                                EX_CLOSED);
                        REMOVE_BIT(room->dir_option[c.arg2]->exit_info,
                                   EX_LOCKED);
                        break;
                    case 2:
                        SET_BIT(room->dir_option[c.arg2]->exit_info,
                                EX_LOCKED);
                        SET_BIT(room->dir_option[c.arg2]->exit_info,
                                EX_CLOSED);
                        break;
                    }
                last_cmd = 1;
                tmob = nullptr;
                tobj = nullptr;
                break;

            case 'T': /* trigger command */
                if (c.arg1 == MOB_TRIGGER && tmob)
                {
                    add_trigger(SCRIPT(tmob), read_trigger(c.arg2), -1);
                    last_cmd = 1;
                }
                else if (c.arg1 == OBJ_TRIGGER && tobj)
                {
                    add_trigger(SCRIPT(tobj), read_trigger(c.arg2), -1);
                    last_cmd = 1;
                }
                else if (c.arg1 == WLD_TRIGGER)
                {
                    room = get_room(c.arg3);
                    if (!room)
                    {
                        ZONE_ERROR("Invalid room number in trigger assignment");
                    }
                    add_trigger(room, read_trigger(c.arg2), -1);
                    last_cmd = 1;
                }

                break;

            case 'V':
                if (c.arg1 == MOB_TRIGGER && tmob)
                {
                    if (!SCRIPT(tmob))
                    {
                        ZONE_ERROR("Attempt to give a variable to scriptless mobile");
                    }
                    else
                        tmob->script_variables[c.sarg1] = c.sarg2;
                    last_cmd = 1;
                }
                else if (c.arg1 == OBJ_TRIGGER && tobj)
                {
                    if (!SCRIPT(tobj))
                    {
                        ZONE_ERROR("Attempt to give variable to scriptless object");
                    }
                    else
                        tobj->script_variables[c.sarg1] = c.sarg2;
                    last_cmd = 1;
                }
                else if (c.arg1 == WLD_TRIGGER)
                {
                    if (!get_room(c.arg3))
                    {
                        ZONE_ERROR("Invalid room number in variable assignment");
                    }
                    else
                    {
                        get_room(c.arg3)->script_variables[c.sarg1] = c.sarg2;
                        last_cmd = 1;
                    }
                }
                break;

            default:
                ZONE_ERROR("unknown cmd in reset table; cmd disabled");
                c.command = '*';
                break;
            }
        }
        catch (const std::exception &e)
        {
            basic_mud_log("Exception thrown in reset_zone '%d' line %d", zone, c.line);
            shutdown_game(1);
        }
    }
}

static void do_reset_rooms(zone_data &z) {
    for (auto &rvnum : z.rooms)
        if(auto room = get_room(rvnum); room)
            reset_wtrigger(room);
}

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
    auto& z = zone_table.at(zone);
    z.age = 0;

    if (!pre_reset(z.number))
    {
        do_reset_cmds(z);
        do_reset_rooms(z);
    }

    // TODO: Split this off into subscriptions.
    for (auto &rrnum : z.rooms)
    {
        auto r = get_room(rrnum);
        if(!r) continue;

        if (r->room_flags.get(ROOM_AURA) && rand_number(1, 5) >= 4)
        {
            send_to_room(r, "The aura of regeneration covering the surrounding area disappears.\r\n");
            r->room_flags.set(ROOM_AURA, false);
        }

        if (r->sector_type == SectorType::lava)
        {
            r->ground_effect = 5;
        }

        if (r->ground_effect < -1)
        {
            send_to_room(r, "The area loses some of the water flooding it.\r\n");
            r->ground_effect += 1;
        }
        else if (r->ground_effect == -1)
        {
            send_to_room(r, "The area loses the last of the water flooding it in one large rush.\r\n");
            r->ground_effect = 0;
        }

        if (r->ground_effect >= 1 && rand_number(1, 4) == 4 && !r->getEnvironment(ENV_WATER) >= 100.0 && r->sector_type != SectorType::lava)
        {
            send_to_room(r, "The lava has cooled and become solid rock.\r\n");
            r->ground_effect = 0;
        }
        else if (r->ground_effect >= 1 && rand_number(1, 2) == 2 && r->getEnvironment(ENV_WATER) >= 100.0 &&
                 r->sector_type != SectorType::lava)
        {
            send_to_room(r, "The water has cooled the lava and it has become solid rock.\r\n");
            r->ground_effect = 0;
        }
    }
    post_reset(z.number);
}

void repairRoomDamage(uint64_t heartPulse, double deltaTime) {
    auto subs = roomSubscriptions.all("repairRoomDamage");
    for(auto room : filter_raw(subs)) {

        if(auto dmg = room->getDamage(); dmg > 0) {
            int toRepair = 0;
            if(dmg >= 100) toRepair = rand_number(5, 10);
            else if(dmg >= 10) toRepair = rand_number(1, 10);
            else if(dmg > 1) toRepair = rand_number(1, dmg);
            else toRepair = 1;
            room->modDamage(-toRepair);
            send_to_room(room, "The area gets rebuilt a little.\r\n");
        }
    }
}


/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr) {
    return zone_table.at(zone_nr).playersInZone.empty();
}


/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error) {
    char buf[MAX_STRING_LENGTH], tmp[520];
    char *point;
    int done = 0, length = 0, templength;

    *buf = *tmp = '\0';

    do {
        if (!fgets(tmp, 512, fl)) {
            basic_mud_log("SYSERR: fread_string: format error at string (pos %ld): %s at or near %s",
                ftell(fl), feof(fl) ? "EOF" : ferror(fl) ? "read error" : "unknown error", error);
            exit(1);
        }
        /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
        /* now only removes trailing ~'s -- Welcor */
        for (point = tmp; *point && *point != '\r' && *point != '\n'; point++);
        if (point > tmp && point[-1] == '~') {
            *(--point) = '\0';
            done = 1;
        } else {
            *point = '\r';
            *(++point) = '\n';
            *(++point) = '\0';
        }

        templength = point - tmp;

        if (length + templength >= MAX_STRING_LENGTH) {
            basic_mud_log("SYSERR: fread_string: string too large (db.c)");
            basic_mud_log("%s", error);
            exit(1);
        } else {
            strcat(buf + length, tmp);    /* strcat: OK (size checked above) */
            length += templength;
        }
    } while (!done);

    /* allocate space for the new string and copy it */
    return (strlen(buf) ? strdup(buf) : nullptr);
}

/* Called to free all allocated follow_type structs - Update by Jamie Nelson */
void free_followers(struct follow_type *k) {
    if (!k)
        return;

    if (k->next)
        free_followers(k->next);

    k->follower = nullptr;
    free(k);
}

/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
static int file_to_string_alloc(const char *name, char **buf) {
    int temppage;
    char temp[MAX_STRING_LENGTH];

    /* Lets not free() what used to be there unless we succeeded. */
    if (file_to_string(name, temp) < 0)
        return (-1);

    if (*buf)
        free(*buf);

    *buf = strdup(temp);
    return (0);
}


/* read contents of a text file, and place in buf */
static int file_to_string(const char *name, char *buf) {
    try {
        std::ifstream f(name);
        std::string out;
        // read all of f to out.
        std::getline(f, out, '\0');
        boost::trim_right(out);
        strcpy(buf, out.c_str());
        return 0;
    } catch (std::exception &e) {
        return -1;
    }
}


/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch) {
    int i;

    ch->followers = nullptr;
    ch->master = nullptr;
    FIGHTING(ch) = nullptr;
    ch->setBaseStat("position", POS_STANDING);
    ch->mob_specials.default_pos = POS_STANDING;
    ch->time.logon = time(nullptr);

}


/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(struct char_data *ch) {
    int i;

    ch->setBaseStat("money_carried", 1500);
    GET_CLAN(ch) = strdup("None.");
    ch->setBaseStat("practices", 600);


    /* If this is our first player make him LVL_IMPL. */
    if (players.size() == 0) {
        admin_set(ch, ADMLVL_IMPL);
    }

    /*ch->time.birth = time(0) - birth_age(ch);*/
    ch->time.logon = ch->time.created = time(nullptr);
    ch->time.played = 0.0;

    ch->setBaseStat("hometown", 1);

    set_height_and_weight_by_race(ch);

    for (i = 1; i < SKILL_TABLE_SIZE; i++) {
        if (GET_ADMLEVEL(ch) >= ADMLVL_IMPL)
            SET_SKILL(ch, i, 100);
    }

    for (i = 0; i < 3; i++)
        ch->limbs[i] = 100;

    for (i = 0; i < 3; i++)
        GET_COND(ch, i) = (GET_ADMLEVEL(ch) == ADMLVL_IMPL ? -1 : 24);

    ch->setBaseStat("load_room", NOWHERE);

    do_start(ch);
}

/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum) {
    return world.count(vnum) ? vnum : NOWHERE;
}


/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum) {
    return mob_proto.count(vnum) ? vnum : NOBODY;
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum) {
    return obj_proto.count(vnum) ? vnum : NOTHING;
}

/* returns the real number of the room with given virtual number */
zone_rnum real_zone(zone_vnum vnum) {
    return zone_table.count(vnum) ? vnum : NOWHERE;
}


/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */


/* This procedure removes the '\r\n' from a string so that it may be
   saved to a file.  Use it only on buffers, not on the orginal
   strings. */

void strip_string(char *buffer) {
    char *ptr, *str;

    ptr = buffer;
    str = ptr;

    while ((*str = *ptr)) {
        str++;
        ptr++;
        if (*ptr == '\r')
            ptr++;
    }
}

/* External variables from config.c */

void load_default_config() {
    /****************************************************************************/
    /** This function is called only once, at boot-time.                       **/
    /** - We assume config_info is empty                          -- Welcor    **/
    /****************************************************************************/
    /****************************************************************************/
    /** Game play options.                                                     **/
    /****************************************************************************/
    CONFIG_PK_ALLOWED = pk_allowed;
    CONFIG_PT_ALLOWED = pt_allowed;
    CONFIG_LEVEL_CAN_SHOUT = level_can_shout;
    CONFIG_HOLLER_MOVE_COST = holler_move_cost;
    CONFIG_TUNNEL_SIZE = tunnel_size;
    CONFIG_MAX_EXP_GAIN = max_exp_gain;
    CONFIG_MAX_EXP_LOSS = max_exp_loss;
    CONFIG_MAX_NPC_CORPSE_TIME = max_npc_corpse_time;
    CONFIG_MAX_PC_CORPSE_TIME = max_pc_corpse_time;
    CONFIG_IDLE_VOID = idle_void;
    CONFIG_IDLE_RENT_TIME = idle_rent_time;
    CONFIG_IDLE_MAX_LEVEL = idle_max_level;
    CONFIG_DTS_ARE_DUMPS = dts_are_dumps;
    CONFIG_LOAD_INVENTORY = load_into_inventory;
    CONFIG_OK = strdup(OK);
    CONFIG_NOPERSON = strdup(NOPERSON);
    CONFIG_NOEFFECT = strdup(NOEFFECT);
    CONFIG_TRACK_T_DOORS = track_through_doors;
    CONFIG_LEVEL_CAP = level_cap;
    CONFIG_STACK_MOBS = show_mob_stacking;
    CONFIG_STACK_OBJS = show_obj_stacking;
    CONFIG_MOB_FIGHTING = mob_fighting;
    CONFIG_DISP_CLOSED_DOORS = disp_closed_doors;
    CONFIG_REROLL_PLAYER_CREATION = reroll_status;
    CONFIG_INITIAL_POINTS_POOL = initial_points;
    CONFIG_ENABLE_COMPRESSION = enable_compression;
    CONFIG_ENABLE_LANGUAGES = enable_languages;
    CONFIG_ALL_ITEMS_UNIQUE = all_items_unique;
    CONFIG_EXP_MULTIPLIER = exp_multiplier;
    /****************************************************************************/
    /** Rent / crashsave options.                                              **/
    /****************************************************************************/
    CONFIG_FREE_RENT = free_rent;
    CONFIG_MAX_OBJ_SAVE = max_obj_save;
    CONFIG_MIN_RENT_COST = min_rent_cost;
    CONFIG_AUTO_SAVE = auto_save;
    CONFIG_AUTOSAVE_TIME = autosave_time;
    CONFIG_CRASH_TIMEOUT = crash_file_timeout;
    CONFIG_RENT_TIMEOUT = rent_file_timeout;

    /****************************************************************************/
    /** Room numbers.                                                          **/
    /****************************************************************************/
    CONFIG_MORTAL_START = mortal_start_room;
    CONFIG_IMMORTAL_START = immort_start_room;
    CONFIG_FROZEN_START = frozen_start_room;
    CONFIG_DON_ROOM_1 = donation_room_1;
    CONFIG_DON_ROOM_2 = donation_room_2;
    CONFIG_DON_ROOM_3 = donation_room_3;

    /****************************************************************************/
    /** Game operation options.                                                **/
    /****************************************************************************/
    CONFIG_DFLT_PORT = DFLT_PORT;

    if (DFLT_IP)
        CONFIG_DFLT_IP = strdup(DFLT_IP);
    else
        CONFIG_DFLT_IP = nullptr;

    CONFIG_DFLT_DIR = strdup(DFLT_DIR);

    if (LOGNAME)
        CONFIG_LOGNAME = strdup(LOGNAME);
    else
        CONFIG_LOGNAME = nullptr;

    CONFIG_MAX_PLAYING = max_playing;
    CONFIG_MAX_FILESIZE = max_filesize;
    CONFIG_MAX_BAD_PWS = max_bad_pws;
    CONFIG_SITEOK_ALL = siteok_everyone;
    CONFIG_NS_IS_SLOW = nameserver_is_slow;
    CONFIG_NEW_SOCIALS = use_new_socials;
    CONFIG_OLC_SAVE = auto_save_olc;
    CONFIG_MENU = strdup(MENU);
    CONFIG_WELC_MESSG = strdup(WELC_MESSG);
    CONFIG_START_MESSG = strdup(START_MESSG);
    CONFIG_IMC_ENABLED = imc_is_enabled;
    CONFIG_EXP_MULTIPLIER = 1.0;

    /****************************************************************************/
    /** Autowiz options.                                                       **/
    /****************************************************************************/
    CONFIG_USE_AUTOWIZ = use_autowiz;
    CONFIG_MIN_WIZLIST_LEV = min_wizlist_lev;

    /****************************************************************************/
    /** Character advancement options.                                         **/
    /****************************************************************************/
    CONFIG_ALLOW_MULTICLASS = allow_multiclass;
    CONFIG_ALLOW_PRESTIGE = allow_prestige;

    /****************************************************************************/
    /** ticks menu                                                             **/
    /****************************************************************************/
    CONFIG_PULSE_VIOLENCE = pulse_violence;
    CONFIG_PULSE_MOBILE = pulse_mobile;
    CONFIG_PULSE_ZONE = pulse_zone;
    CONFIG_PULSE_CURRENT = pulse_current;
    CONFIG_PULSE_SANITY = pulse_sanity;
    CONFIG_PULSE_IDLEPWD = pulse_idlepwd;
    CONFIG_PULSE_AUTOSAVE = pulse_autosave;
    CONFIG_PULSE_USAGE = pulse_usage;
    CONFIG_PULSE_TIMESAVE = pulse_timesave;

    /****************************************************************************/
    /** Character Creation Method                                              **/
    /****************************************************************************/
    CONFIG_CREATION_METHOD = method;
}

void load_config() {
    FILE *fl;
    char line[MAX_STRING_LENGTH];
    char tag[MAX_INPUT_LENGTH];
    int num;
    float fum;
    char buf[MAX_INPUT_LENGTH];

    load_default_config();

    snprintf(buf, sizeof(buf), "%s/%s", "lib", "etc/config");
    if (!(fl = fopen(buf, "r"))) {
        snprintf(buf, sizeof(buf), "Game Config File: %s", "etc/config");
        perror(buf);
        return;
    }

    /****************************************************************************/
    /** Load the game configuration file.                                      **/
    /****************************************************************************/
    while (get_line(fl, line)) {
        split_argument(line, tag);
        num = atoi(line);
        fum = atof(line);

        switch (LOWER(*tag)) {
            case 'a':
                if (!strcasecmp(tag, "auto_save"))
                    CONFIG_AUTO_SAVE = num;
                else if (!strcasecmp(tag, "autosave_time"))
                    CONFIG_AUTOSAVE_TIME = num;
                else if (!strcasecmp(tag, "auto_save_olc"))
                    CONFIG_OLC_SAVE = num;
                else if (!strcasecmp(tag, "allow_multiclass"))
                    CONFIG_ALLOW_MULTICLASS = num;
                else if (!strcasecmp(tag, "allow_prestige"))
                    CONFIG_ALLOW_PRESTIGE = num;
                else if (!strcasecmp(tag, "auto_level"))
                    basic_mud_log("ignoring obsolete config option auto_level");
                else if (!strcasecmp(tag, "all_items_unique"))
                    CONFIG_ALL_ITEMS_UNIQUE = num;
                break;

            case 'c':
                if (!strcasecmp(tag, "crash_file_timeout"))
                    CONFIG_CRASH_TIMEOUT = num;
                else if (!strcasecmp(tag, "compression")) {
                    CONFIG_ENABLE_COMPRESSION = num;
                }
                break;

            case 'd':
                if (!strcasecmp(tag, "disp_closed_doors"))
                    CONFIG_DISP_CLOSED_DOORS = num;
                else if (!strcasecmp(tag, "dts_are_dumps"))
                    CONFIG_DTS_ARE_DUMPS = num;
                else if (!strcasecmp(tag, "donation_room_1"))
                    if (num == -1)
                        CONFIG_DON_ROOM_1 = NOWHERE;
                    else
                        CONFIG_DON_ROOM_1 = num;
                else if (!strcasecmp(tag, "donation_room_2"))
                    if (num == -1)
                        CONFIG_DON_ROOM_2 = NOWHERE;
                    else
                        CONFIG_DON_ROOM_2 = num;
                else if (!strcasecmp(tag, "donation_room_3"))
                    if (num == -1)
                        CONFIG_DON_ROOM_3 = NOWHERE;
                    else
                        CONFIG_DON_ROOM_3 = num;
                else if (!strcasecmp(tag, "dflt_dir")) {
                    if (CONFIG_DFLT_DIR)
                        free(CONFIG_DFLT_DIR);
                    if (line && *line)
                        CONFIG_DFLT_DIR = strdup(line);
                    else
                        CONFIG_DFLT_DIR = strdup(DFLT_DIR);
                } else if (!strcasecmp(tag, "dflt_ip")) {
                    if (CONFIG_DFLT_IP)
                        free(CONFIG_DFLT_IP);
                    if (line && *line)
                        CONFIG_DFLT_IP = strdup(line);
                    else
                        CONFIG_DFLT_IP = nullptr;
                } else if (!strcasecmp(tag, "dflt_port"))
                    CONFIG_DFLT_PORT = num;
                break;

            case 'e':
                if (!strcasecmp(tag, "enable_languages"))
                    CONFIG_ENABLE_LANGUAGES = num;
                else if (!strcasecmp(tag, "exp_multiplier"))
                    CONFIG_EXP_MULTIPLIER = fum;
                break;

            case 'f':
                if (!strcasecmp(tag, "free_rent"))
                    CONFIG_FREE_RENT = num;
                else if (!strcasecmp(tag, "frozen_start_room"))
                    CONFIG_FROZEN_START = num;
                break;

            case 'h':
                if (!strcasecmp(tag, "holler_move_cost"))
                    CONFIG_HOLLER_MOVE_COST = num;
                break;

            case 'i':
                if (!strcasecmp(tag, "idle_void"))
                    CONFIG_IDLE_VOID = num;
                else if (!strcasecmp(tag, "idle_rent_time"))
                    CONFIG_IDLE_RENT_TIME = num;
                else if (!strcasecmp(tag, "idle_max_level")) {
                    if (num >= CONFIG_LEVEL_CAP)
                        num += 1 - CONFIG_LEVEL_CAP;
                    CONFIG_IDLE_MAX_LEVEL = num;
                } else if (!strcasecmp(tag, "immort_level_ok"))
                    basic_mud_log("Ignoring immort_level_ok obsolete config");
                else if (!strcasecmp(tag, "immort_start_room"))
                    CONFIG_IMMORTAL_START = num;
                else if (!strcasecmp(tag, "imc_enabled"))
                    CONFIG_IMC_ENABLED = num;
                else if (!strcasecmp(tag, "initial_points"))
                    CONFIG_INITIAL_POINTS_POOL = num;
                break;

            case 'l':
                if (!strcasecmp(tag, "level_can_shout"))
                    CONFIG_LEVEL_CAN_SHOUT = num;
                else if (!strcasecmp(tag, "level_cap"))
                    CONFIG_LEVEL_CAP = num;
                else if (!strcasecmp(tag, "load_into_inventory"))
                    CONFIG_LOAD_INVENTORY = num;
                else if (!strcasecmp(tag, "logname")) {
                    if (CONFIG_LOGNAME)
                        free(CONFIG_LOGNAME);
                    if (line && *line)
                        CONFIG_LOGNAME = strdup(line);
                    else
                        CONFIG_LOGNAME = nullptr;
                }
                break;

            case 'm':
                if (!strcasecmp(tag, "max_bad_pws"))
                    CONFIG_MAX_BAD_PWS = num;
                else if (!strcasecmp(tag, "max_exp_gain"))
                    CONFIG_MAX_EXP_GAIN = num;
                else if (!strcasecmp(tag, "max_exp_loss"))
                    CONFIG_MAX_EXP_LOSS = num;
                else if (!strcasecmp(tag, "max_filesize"))
                    CONFIG_MAX_FILESIZE = num;
                else if (!strcasecmp(tag, "max_npc_corpse_time"))
                    CONFIG_MAX_NPC_CORPSE_TIME = num;
                else if (!strcasecmp(tag, "max_obj_save"))
                    CONFIG_MAX_OBJ_SAVE = num;
                else if (!strcasecmp(tag, "max_pc_corpse_time"))
                    CONFIG_MAX_PC_CORPSE_TIME = num;
                else if (!strcasecmp(tag, "max_playing"))
                    CONFIG_MAX_PLAYING = num;
                else if (!strcasecmp(tag, "menu")) {
                    if (CONFIG_MENU)
                        free(CONFIG_MENU);
                    strncpy(buf, "Reading menu in load_config()", sizeof(buf));
                    CONFIG_MENU = fread_string(fl, buf);
                } else if (!strcasecmp(tag, "min_rent_cost"))
                    CONFIG_MIN_RENT_COST = num;
                else if (!strcasecmp(tag, "min_wizlist_lev")) {
                    if (num >= CONFIG_LEVEL_CAP)
                        num += 1 - CONFIG_LEVEL_CAP;
                    CONFIG_MIN_WIZLIST_LEV = num;
                } else if (!strcasecmp(tag, "mob_fighting"))
                    CONFIG_MOB_FIGHTING = num;
                else if (!strcasecmp(tag, "mortal_start_room"))
                    CONFIG_MORTAL_START = num;
                else if (!strcasecmp(tag, "method"))
                    CONFIG_CREATION_METHOD = num;
                break;

            case 'n':
                if (!strcasecmp(tag, "nameserver_is_slow"))
                    CONFIG_NS_IS_SLOW = num;
                else if (!strcasecmp(tag, "noperson")) {
                    char tmp[READ_SIZE];
                    if (CONFIG_NOPERSON)
                        free(CONFIG_NOPERSON);
                    snprintf(tmp, sizeof(tmp), "%s\r\n", line);
                    CONFIG_NOPERSON = strdup(tmp);
                } else if (!strcasecmp(tag, "noeffect")) {
                    char tmp[READ_SIZE];
                    if (CONFIG_NOEFFECT)
                        free(CONFIG_NOEFFECT);
                    snprintf(tmp, sizeof(tmp), "%s\r\n", line);
                    CONFIG_NOEFFECT = strdup(tmp);
                }
                break;

            case 'o':
                if (!strcasecmp(tag, "ok")) {
                    char tmp[READ_SIZE];
                    if (CONFIG_OK)
                        free(CONFIG_OK);
                    snprintf(tmp, sizeof(tmp), "%s\r\n", line);
                    CONFIG_OK = strdup(tmp);
                }
                break;

            case 'p':
                if (!strcasecmp(tag, "pk_allowed"))
                    CONFIG_PK_ALLOWED = num;
                else if (!strcasecmp(tag, "pt_allowed"))
                    CONFIG_PT_ALLOWED = num;
                else if (!strcasecmp(tag, "pulse_viol"))
                    CONFIG_PULSE_VIOLENCE = num;
                else if (!strcasecmp(tag, "pulse_mobile"))
                    CONFIG_PULSE_MOBILE = num;
                else if (!strcasecmp(tag, "pulse_current"))
                    CONFIG_PULSE_CURRENT = num;
                else if (!strcasecmp(tag, "pulse_zone"))
                    CONFIG_PULSE_ZONE = num;
                else if (!strcasecmp(tag, "pulse_autosave"))
                    CONFIG_PULSE_AUTOSAVE = num;
                else if (!strcasecmp(tag, "pulse_usage"))
                    CONFIG_PULSE_USAGE = num;
                else if (!strcasecmp(tag, "pulse_sanity"))
                    CONFIG_PULSE_SANITY = num;
                else if (!strcasecmp(tag, "pulse_timesave"))
                    CONFIG_PULSE_TIMESAVE = num;
                else if (!strcasecmp(tag, "pulse_idlepwd"))
                    CONFIG_PULSE_IDLEPWD = num;
                break;

            case 'r':
                if (!strcasecmp(tag, "rent_file_timeout"))
                    CONFIG_RENT_TIMEOUT = num;
                else if (!strcasecmp(tag, "reroll_stats"))
                    CONFIG_REROLL_PLAYER_CREATION = num;
                break;

            case 's':
                if (!strcasecmp(tag, "siteok_everyone"))
                    CONFIG_SITEOK_ALL = num;
                else if (!strcasecmp(tag, "start_messg")) {
                    strncpy(buf, "Reading start message in load_config()", sizeof(buf));
                    if (CONFIG_START_MESSG)
                        free(CONFIG_START_MESSG);
                    CONFIG_START_MESSG = fread_string(fl, buf);
                } else if (!strcasecmp(tag, "stack_mobs"))
                    CONFIG_STACK_MOBS = num;
                else if (!strcasecmp(tag, "stack_objs"))
                    CONFIG_STACK_OBJS = num;
                break;

            case 't':
                if (!strcasecmp(tag, "tunnel_size"))
                    CONFIG_TUNNEL_SIZE = num;
                else if (!strcasecmp(tag, "track_through_doors"))
                    CONFIG_TRACK_T_DOORS = num;
                break;

            case 'u':
                if (!strcasecmp(tag, "use_autowiz"))
                    CONFIG_USE_AUTOWIZ = num;
                else if (!strcasecmp(tag, "use_new_socials"))
                    CONFIG_NEW_SOCIALS = num;
                break;

            case 'w':
                if (!strcasecmp(tag, "welc_messg")) {
                    strncpy(buf, "Reading welcome message in load_config()", sizeof(buf));
                    if (CONFIG_WELC_MESSG)
                        free(CONFIG_WELC_MESSG);
                    CONFIG_WELC_MESSG = fread_string(fl, buf);
                }
                break;

            default:
                break;
        }
    }

    fclose(fl);
}


int getNextUnitID() {
    static int id = 0;
    while(units.contains(id)) id++;
    return id;
}

int getNextAccountID() {
    static int id = 0;
    while(accounts.contains(id)) id++;
    return id;
}

// ^#(?<id>\d+)(?::(?<generation>\d+)?)?
static std::regex uid_regex(R"(^#(\d+)(?::(\d+)?)?(!)?)", std::regex::icase);

bool isUID(const std::string& uid) {
    return std::regex_match(uid, uid_regex);
}

std::shared_ptr<unit_data> resolveUID(const std::string& uid) {
    // First we need to check if it matches or not.
    std::smatch match;
    std::optional<time_t> generation = std::nullopt;

    if(!std::regex_search(uid, match, uid_regex)) {
        return nullptr;
    }

    int64_t id = std::stoll(match[1].str()); // First capture group
    if(match[2].matched) { // Second capture group
        generation = std::stoll(match[2].str());
    }

    bool active = match[3].matched; // Third capture group
    
    if(auto find = units.find(id); find != units.end()) {
        if(generation && find->second->generation != *generation) return nullptr;
        if(active && !find->second->isActive()) return nullptr;
        return find->second;
    }

    return nullptr;
}

int create_join_session(int account_id, int character_id, int64_t connection_id, const std::string& ip) {
    auto acc_found = accounts.find(account_id);
    if(acc_found == accounts.end()) return -1;
    auto &acc = acc_found->second;
    auto ch_found = uniqueCharacters.find(character_id);
    if(ch_found == uniqueCharacters.end()) return -1;
    auto &ch = ch_found->second;
    ch->player_flags.set(PLR_NOTDEADYET, false);
    auto exist_sess = sessions.find(character_id);
    if(exist_sess != sessions.end()) {
        // a session already exists. we'll be joining it.
        auto &sess = exist_sess->second;
        if(sess->conns.empty()) {
            // the character is currently active, but link dead.
            sess->timeoutCounter = 0.0;
            send_to_char(ch.get(), "You have reconnected to %s from %s.\r\n", ch->getName(), ip);
        } else {
            send_to_char(ch.get(), "Another connection is now linked to %s, from %s.\r\n", ch->getName(), ip);
        }
        sess->conns.emplace(connection_id, ip);
        acc.descriptors.insert(sess);
        return sess->conns.size();
    } else {
        // no session exists. We'll have to create one.
        if(acc.admin_level < 1) {
            // non-admins can only have one character active at once.
            // Scan acc.descriptors for any with a character that isn't
            // this character_id.
            for(auto desc : acc.descriptors) {
                if(desc->id != character_id) {
                    return -2;
                }
            }
        }

        auto desc = new descriptor_data();
        STATE(desc) = CON_LOGIN;
        desc->character = ch.get();
        ch->desc = desc;
        desc->account = &acc;
        desc->id = character_id;
        desc->conns.emplace(connection_id, ip);
        acc.descriptors.insert(desc);
        sessions.emplace(character_id, desc);
        desc->next = descriptor_list;
        descriptor_list = desc;
        send_to_char(ch.get(), "You have connected to %s from %s.\r\n", ch->getName(), ip);
        return 1;
    }
}