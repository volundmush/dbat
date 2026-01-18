#include "serde/Startup.h"
#include "serde/saveload.h"
#include <vector>
#include <filesystem>

#include "dbat/RoomUtils.h"
#include "dbat/ObjectUtils.h"
#include "dbat/CharacterUtils.h"

#include "dbat/db.h"
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

void boot_db_shadow() {
    if (SELFISHMETER >= 10) {
        LINFO("Loading Shadow Dragons.");
        load_shadow_dragons();
    }
}

void boot_db_world() {

    auto assetDumps = getDumpFiles("data/dumps/assets", "assets-");
    if (!assetDumps.empty()) {
        LINFO("Newest assets dump: {}", assetDumps.front().string());
    } else {
        LINFO("No matching assets dumps found.");
        return;
    }

    auto assetLatest = assetDumps.front();

    auto userDumps = getDumpFiles("data/dumps/user", "user-");
    if (!userDumps.empty()) {
        LINFO("Newest users dump: {}", userDumps.front().string());
    } else {
        LINFO("No matching user dumps found.");
        return;
    }

    auto userLatest = userDumps.front();

    LINFO("Loading stat handlers...");
    init_stat_handlers();

    LINFO("Loading global data...");
    load_globaldata(assetLatest);

    LINFO("Loading Zones...");
    load_zones(assetLatest);

    LINFO("Loading DgScripts and generating index.");
    load_dgscript_prototypes(assetLatest);

    LINFO("Loading mobs and generating index.");
    load_npc_prototypes(assetLatest);

    LINFO("Loading objs and generating index.");
    load_item_prototypes(assetLatest);

    LINFO("Loading rooms.");
    load_rooms(assetLatest);

    LINFO("Loading Grid Templates...");
    load_grid_templates(assetLatest);

    LINFO("Loading areas initial...");
    load_areas_initial(userLatest);

    LINFO("Loading shops.");
    load_shops(assetLatest);

    LINFO("Loading guild masters.");
    load_guilds(assetLatest);

    LINFO("Loading exits.");
    load_exits(assetLatest);

    LINFO("Loading help entries.");
    load_help(assetLatest);
    
    LINFO("Loading assemblies.");
    load_assemblies(assetLatest);

    LINFO("Loading areas finish...");
    load_areas_finish(assetLatest);

    // Now loading user data...
    LINFO("Loading accounts.");
    load_accounts(userLatest);

    LINFO("Loading players.");
    load_players(userLatest);

    LINFO("Loading characters initial...");
    load_characters_initial(userLatest);

    LINFO("Loading structures initial...");
    load_structures_initial(userLatest);

    LINFO("Loading structures finish...");
    load_structures_finish(userLatest);

    // Now that all of the game entities have been spawned, we can finish loading
    // relations between them.

    LINFO("Running activation of entities...");
    db_load_activate_entities();

    LINFO("Checking start rooms.");
    check_start_rooms();

    boot_db_shadow();
}

void boot_db_textfiles() {
    LINFO("Reading news, credits, help, ihelp, bground, info & motds.");
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
    LINFO("Loading spell definitions.");
    mag_assign_spells();

    LINFO("Loading feats.");
    assign_feats();
}

void boot_db_help() {

}

void boot_db_mail() {
    LINFO("Booting mail system.");
    if (!scan_file()) {
        LINFO("    Mail boot failed -- Mail system disabled");
    }
}

void boot_db_socials() {
    LINFO("Loading social messages.");
    boot_social_messages();
}

void boot_db_commands() {
    LINFO("Building command list.");
    create_command_list(); /* aedit patch -- M. Scott */
}

void boot_db_specials() {
    LINFO("Assigning function pointers:");
    LINFO("   Mobiles.");
    assign_mobiles();
    LINFO("   Shopkeepers.");
    assign_the_shopkeepers();
    LINFO("   Objects.");
    assign_objects();
    LINFO("   Rooms.");
    assign_rooms();
    LINFO("   Guildmasters.");
    assign_the_guilds();
}

void sort_spells();
void boot_db_sort() {
    LINFO("Sorting command list and spells.");
    sort_commands();
    sort_spells();
    sort_feats();
}

void boot_db_boards() {
    LINFO("Booting boards system.");
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

namespace dbat::init {

    void init() {
        init_locale();
        init_database();
        init_zones();
    }

    void init_locale() {
        std::locale::global(std::locale("en_US.UTF-8"));
    }

    void init_database() {
        boot_db_new();
    }

    void init_zones() {
        for (auto &[vn, z] : zone_table) {
            LINFO("Resetting #%d: %s.", vn, z->name.c_str());
            z->reset();
        }
    }
}