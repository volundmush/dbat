#include "dbat/Startup.h"
#include <vector>
#include <filesystem>

#include "dbat/RoomUtils.h"
#include "dbat/ObjectUtils.h"
#include "dbat/CharacterUtils.h"

#include "dbat/db.h"
#include "dbat/saveload.h"
#include "dbat/dg_scripts.h"
#include "dbat/filter.h"
#include "dbat/utils.h"
#include "dbat/magic.h"
#include "dbat/feats.h"
#include "dbat/maputils.h"
#include "dbat/boards.h"
#include "dbat/mail.h"
#include "dbat/act.social.h"
#include "dbat/act.informative.h"
#include "dbat/spec_assign.h"
#include "dbat/interpreter.h"

#include "dbat/DragonBall.h"

#include "dbat/act.other.h"



static std::vector<std::filesystem::path> getDumpFiles() {
    std::filesystem::path dir = "data/dumps"; // Change to your directory
    std::vector<std::filesystem::path> directories;

    auto pattern = "dump-";
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_directory() && entry.path().filename().string().starts_with(pattern)) {
            directories.push_back(entry.path());
        }
    }

    // Sorting lexicographically in descending order to get the newest first
    // Assumes the naming convention ensures that lexicographical order matches chronological order.

    std::sort(directories.begin(), directories.end(), std::greater<>());
    return directories;
}

static void db_load_activate_entities() {
    // activate all items which ended up "in the world".
    for(auto &[id, r] : Room::registry) {
        assign_triggers(r.get(), WLD_TRIGGER);
        r->activateScripts();
        auto con = r->getObjects().snapshot_weak();
        for(auto o : filter_raw(con)) {
            o->activate();
        }
        auto people = r->getPeople().snapshot_weak();
        for(auto c : filter_raw(people)) {
            if(IS_NPC(c)) {
                c->activate();
            }
        }
    }
}

void boot_db_world() {

    auto dumps = getDumpFiles();
    if (!dumps.empty()) {
        LINFO("Newest state dump: {}", dumps.front().string());
    } else {
        LINFO("No matching state dumps found.");
        return;
    }

    auto latest = dumps.front();

    basic_mud_log("Loading stat handlers...");
    init_stat_handlers();

    basic_mud_log("Loading global data...");
    load_globaldata(latest);

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

    basic_mud_log("Loading Grid Templates...");
    load_grid_templates(latest);

    basic_mud_log("Loading areas initial...");
    load_areas_initial(latest);

    basic_mud_log("Loading shops.");
    load_shops(latest);

    basic_mud_log("Loading guild masters.");
    load_guilds(latest);

    basic_mud_log("Loading exits.");
    load_exits(latest);

    basic_mud_log("Loading accounts.");
    load_accounts(latest);

    basic_mud_log("Loading players.");
    load_players(latest);

    basic_mud_log("Loading characters initial...");
    load_characters_initial(latest);

    basic_mud_log("Loading areas finish...");
    load_areas_finish(latest);

    basic_mud_log("Loading structures initial...");
    load_structures_initial(latest);

    basic_mud_log("Loading structures finish...");
    load_structures_finish(latest);

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


void mag_assign_spells();

void boot_db_spellfeats() {
    basic_mud_log("Loading spell definitions.");
    mag_assign_spells();

    basic_mud_log("Loading feats.");
    assign_feats();
}

void boot_db_help() {

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

void sort_spells();
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


void boot_db_spacemap() {
    FILE *mapfile = fopen(MAP_FILE, "r");
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
    boot_db_textfiles();
    boot_db_spellfeats();
    boot_db_world();
    boot_db_mail();
    boot_db_socials();
    boot_db_commands();
    boot_db_help();
    boot_db_specials();
    boot_db_sort();
    boot_db_boards();
    boot_db_spacemap();
    topLoad();
}