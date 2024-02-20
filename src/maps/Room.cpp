#include "dbat/structs.h"
#include "dbat/db.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/act.wizard.h"
#include "dbat/maputils.h"
#include "dbat/act.informative.h"

static const std::set<int> lit_sectors = {SECT_INSIDE, SECT_CITY, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};


// Check to see whether a room should normally be considered dark as a basic matter of course.
bool Room::isInsideNormallyDark() {
    if(checkFlag(FlagType::Room, ROOM_DARK)) return true;
    if(lit_sectors.contains(sector_type)) return false;
    if(checkFlag(FlagType::Room, ROOM_INDOORS)) return false;

    return false;
}

static const std::set<int> sun_down = {SUN_SET, SUN_DARK};

bool Room::isInsideDark() {

    // If the room is not normally dark, then it's definitely not dark.
    if(!isInsideNormallyDark()) return false;

    // Certain sectors, like cities, provide free light.
    if(lit_sectors.contains(sector_type)) return false;

    // Failing that, maybe the sun is up?
    if(!sun_down.contains(weather_info.sunlight)) return false;

    // welp, now it's time for the most expensive operation of all.
    for(auto u : getContents()) {
        if(u->isProvidingLight()) return false;
    }

    return true;
}

// EXITS below here.


std::map<int, Destination> Room::getDestinations(GameEntity* viewer) {
    std::map<int, Destination> out;
    for(auto &[dir, e] : getExits()) {
        auto &dest = out[e->locationType];
        dest.target = e->getDestination();
        dest.via = e;
        dest.direction = dir;
    }
    return out;
}

nlohmann::json Room::serialize() {
    auto j = GameEntity::serialize();

    if(sector_type) j["sector_type"] = sector_type;

    if(timed) j["timed"] = timed;
    if(dmg) j["dmg"] = dmg;
    if(geffect) j["geffect"] = geffect;

    for(auto p : proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }

    return j;
}

Room::Room(const nlohmann::json &j) {
    deserialize(j);
}

void Room::deserialize(const nlohmann::json &j) {
    GameEntity::deserialize(j);
    if(j.contains("sector_type")) sector_type = j["sector_type"];
    if(j.contains("timed")) timed = j["timed"];
    if(j.contains("dmg")) dmg = j["dmg"];
    if(j.contains("geffect")) geffect = j["geffect"];
}

bool Room::isEnvironment() {
    return true;
}

std::optional<room_vnum> Room::getLaunchDestination() {
    return {};
}

double Room::getEnvVar(EnvVar v) {
    switch(v) {
        case EnvVar::Gravity: {
            // Check for a gravity generator.
            for(auto c : getInventory()) if(c->gravity) return c->gravity.value();

            // what about area rules? For legacy rooms, this should be a planet or dimension etc.
            if(auto env = getEnvironment(); env) {
                return env->getEnvVar(v);
            }

            // special cases here..
            if (vn >= 64000 && vn <= 64006) {
                return 100.0;
            }
            if (vn >= 64007 && vn <= 64016) {
                return 300.0;
            }
            if (vn >= 64017 && vn <= 64030) {
                return 500.0;
            }
            if (vn >= 64031 && vn <= 64048) {
                return 1000.0;
            }
            if (vn >= 64049 && vn <= 64070) {
                return 5000.0;
            }
            if (vn >= 64071 && vn <= 64096) {
                return 10000.0;
            }
            if (vn == 64097) {
                return 1000.0;
            }

            // safe default.
            return 1.0;
        }
        default:
            return 0.0;
    }
}

bool Room::isActive() {
    return world.contains(vn);
}


int Room::getDamage() {
    return dmg;
}

int Room::setDamage(int amount) {
    auto before = dmg;
    dmg = std::clamp<int>(amount, 0, 100);
    // if(dmg != before) save();
    return dmg;
}

int Room::modDamage(int amount) {
    return setDamage(dmg + amount);
}


bool Room::isSunken() {
    return sector_type == SECT_UNDERWATER || geffect < 0;
}


static const std::set<int> inside_sectors = {SECT_INSIDE, SECT_UNDERWATER, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};

MoonCheck Room::checkMoon() {
    for(auto f : {ROOM_INDOORS, ROOM_UNDERGROUND, ROOM_SPACE}) if(checkFlag(FlagType::Room, f)) return MoonCheck::NoMoon;
    if(inside_sectors.contains(sector_type)) return MoonCheck::NoMoon;
    auto plan = getPlanet();
    if(!plan) return MoonCheck::NoMoon;
    if(!plan->checkFlag(FlagType::Structure, STRUCTURE_MOON)) return MoonCheck::NoMoon;

    return MOON_TIMECHECK() ? MoonCheck::Full : MoonCheck::NotFull;

}

static const std::map<std::string, int> _dirNames = {
    {"north", NORTH},
    {"east", EAST},
    {"south", SOUTH},
    {"west", WEST},
    {"up", UP},
    {"down", DOWN},
    {"northwest", NORTHWEST},
    {"northeast", NORTHEAST},
    {"southwest", SOUTHWEST},
    {"southeast", SOUTHEAST},
    {"inside", INDIR},
    {"outside", OUTDIR}

};

const std::vector<std::string> sky_look = {
                        "sunny",
                        "cloudy",
                        "rainy",
                        "lightning"
                };

DgResults Room::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);
    char bitholder[MAX_STRING_LENGTH];

    if(auto d = _dirNames.find(lmember); d != _dirNames.end()) {
        auto exits = getExits();
        auto ex = exits[d->second];
        if(!ex) {
            return "";
        }
        if (!arg.empty()) {
            auto dest = ex->getDestination();
            if (!strcasecmp(arg.c_str(), "vnum")) {
                
                if(ex) return std::to_string(dest->getUID());
                return "";
            }
            else if (!strcasecmp(arg.c_str(), "key"))
                return fmt::format("{}", ex->key);
            else if (!strcasecmp(arg.c_str(), "bits")) {
                snprintf(bitholder, sizeof(bitholder), "%s", ex->getFlagNames(FlagType::Exit));
                return bitholder;
            }
            else if (!strcasecmp(arg.c_str(), "room")) {
                if(dest) return dest;
                return "";
            }
        } else /* no subfield - default to bits */
            {
                snprintf(bitholder, sizeof(bitholder), "%s", ex->getFlagNames(FlagType::Exit));
                return bitholder;
            }
    }

    if(lmember == "name") return getName();
    if(lmember == "sector") return sector_types[sector_type];
    if(lmember == "gravity") return fmt::format("{}", (int64_t)getEnvVar(EnvVar::Gravity));

    if(lmember == "vnum") {
        if(!arg.empty()) {
            auto v = atoll(arg.c_str());
            return vn == v ? "1":"0";
        }
        return fmt::format("{}", vn);
    }

    if(lmember == "contents") {
        if(arg.empty()) {
            if(auto inv = getInventory(); !inv.empty()) return inv.front();
            return "";
        }
        obj_vnum v = atoll(arg.c_str());
        if(auto found = findObjectVnum(v); found) return found;
        return "";
    }

    if(lmember == "people") {
        if(auto p = getPeople(); !p.empty()) return p.front();
        return "";
    }

    if(lmember == "id") return this;

    if(lmember == "weather") return !checkFlag(FlagType::Room, ROOM_INDOORS) ? sky_look[weather_info.sky] : "";

    if(lmember == "fishing") return checkFlag(FlagType::Room, ROOM_FISHING) ? "1" : "0";

    if(lmember == "roomflag") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(room_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::Room, flag) ? "1" : "0";
    }

    if(lmember == "varexists") return script->hasVar(arg) ? "1" : "0";

    if(lmember == "zonenumber") return fmt::format("{}", zone);
    if(lmember == "zonename") return zone_table[zone].name;

    if(script->hasVar(lmember)) {
        return script->getVar(lmember);
    } else {
        script_log("Trigger: %s, VNum %d. unknown room field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), lmember.c_str());
    }

    return "";
}

std::string Room::getUnitClass() {
    return "Room";
}

UnitFamily Room::getFamily() {
    return UnitFamily::Room;
}

void Room::assignTriggers() {

    // remove all duplicates from i->proto_script but do not change its order otherwise.
    std::set<trig_vnum> existVnums;
    std::set<trig_vnum> valid;
    for(auto t : proto_script) valid.insert(t);
    
    for(auto t : script->dgScripts) existVnums.insert(t->parent->vn);
    bool added = false;
    bool removed = false;

    // remove any dgScript instances in i->script->dgScripts that aren't in i->proto_script
    std::list<std::shared_ptr<trig_data>> validScripts;
    for(auto t : script->dgScripts) {
        if(valid.contains(t->parent->vn)) {
            validScripts.push_back(t);
        }
        else {
            removed = true;
        }
    }
    if(removed) script->dgScripts = validScripts;

    for(auto p : proto_script) {
        // only add if they don't already have one...
        if(!existVnums.contains(p)) {
            script->addTrigger(read_trigger(p), -1);
            added = true;
            existVnums.insert(p);
        }
    }

    if(added || removed) {
        // we need to sort i->script->dgScripts by the order of i->proto_script
        std::list<std::shared_ptr<trig_data>> sorted;
        for(auto p : proto_script) {
            for(auto t : script->dgScripts) {
                if(t->parent->vn == p) {
                    sorted.push_back(t);
                    break;
                }
            }
        }
        script->dgScripts = sorted;
    }

}

std::string Room::renderExits1(GameEntity *viewer) {
    std::string result;

    int door, door_found = 0, has_light = false, i;
    char dlist1[500];
    char dlist2[500];
    char dlist3[500];
    char dlist4[500];
    char dlist5[500];
    char dlist6[500];
    char dlist7[500];
    char dlist8[500];
    char dlist9[500];
    char dlist10[500];
    char dlist11[500];
    char dlist12[500];

    *dlist1 = '\0';
    *dlist2 = '\0';
    *dlist3 = '\0';
    *dlist4 = '\0';
    *dlist5 = '\0';
    *dlist6 = '\0';
    *dlist7 = '\0';
    *dlist8 = '\0';
    *dlist9 = '\0';
    *dlist10 = '\0';
    *dlist11 = '\0';
    *dlist12 = '\0';
    auto exit_mode = exitlevel(viewer);

    if (exit_mode == EXIT_OFF) {
        result += fmt::sprintf("@D------------------------------------------------------------------------@n\r\n");
    }
    int space = false;
    if (sector_type == SECT_SPACE && getUID() >= 20000) {
        space = true;
    }
    if (exit_mode == EXIT_NORMAL && space == false && viewer->getRoom() == this) {
        /* Compass and Auto-map - Iovan 9-11-10 */
        result += fmt::sprintf("@D------------------------------------------------------------------------@n\r\n");
        result += fmt::sprintf("@w      Compass           Auto-Map            Map Key\r\n");
        result += fmt::sprintf("@R     ---------         ----------   -----------------------------\r\n");
        result += generateMap(viewer, 0);
        result += fmt::sprintf("@D------------------------------------------------------------------------@n\r\n");
    }
    if (exit_mode == EXIT_NORMAL && space == true) {
        /* printmap */
        result += fmt::sprintf("@D------------------------------[@CRadar@D]---------------------------------@n\r\n");
        result += printMap(viewer, 1, -1);
        result += fmt::sprintf("     @D[@wTurn autoexit complete on for directions instead of radar@D]@n\r\n");
        result += fmt::sprintf("@D------------------------------------------------------------------------@n\r\n");
    }
    if (exit_mode == EXIT_COMPLETE || (exit_mode == EXIT_NORMAL && space == false && viewer->getRoom() != this)) {
        result += fmt::sprintf("@D----------------------------[@gObvious Exits@D]-----------------------------@n\r\n");
        
        bool admVision = viewer->checkFlag(FlagType::Admin, ADM_SEESECRET);

        std::map<int, char*> dlists = {
                {0, dlist2},
                {1, dlist4},
                {2, dlist6},
                {3, dlist8},
                {4, dlist9},
                {5, dlist10},
                {6, dlist1},
                {7, dlist3},
                {8, dlist5},
                {9, dlist7},
                {10, dlist11},
                {11, dlist12}
        };

        auto exits = getExits();
        auto has_light = viewer->isProvidingLight();
        auto nightvision = viewer->canSeeInDark();

        for (auto &[door, d] : exits) {
            auto dest = d->getDestination();
            if(!dest) continue;
            auto dl = dlists[door];

            auto al = d->getAlias();

            if (admVision) {
                /* Immortals see everything */
                door_found++;
                char blam[9];
                sprintf(blam, "%s", dirs[door]);
                *blam = toupper(*blam);

                auto dirname = dirs[door];
                auto rdirname = dirs[rev_dir[door]];


                sprintf(dl, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, dest->getUID(), dest->getDisplayName(viewer).c_str());
                if (d->checkFlag(FlagType::Exit, EX_ISDOOR) || d->checkFlag(FlagType::Exit, EX_SECRET)) {
                    /* This exit has a door - tell all about it */
                    char argh[100];
                    sprintf(argh, "%s ",
                            strcasecmp(fname(al.c_str()), "undefined") ? fname(
                                    al.c_str()) : "opening");
                    sprintf(dl + strlen(dl), "                    The %s%s %s %s %s%s.\r\n",
                            d->checkFlag(FlagType::Exit, EX_SECRET) ?
                            "@rsecret@w " : "",
                            (al.c_str() && strcasecmp(fname(al.c_str()), "undefined")) ?
                            fname(al.c_str()) : "opening",
                            strstr(argh, "s ") != nullptr ? "are" : "is",
                            d->checkFlag(FlagType::Exit, EX_CLOSED) ?
                            "closed" : "open",
                            d->checkFlag(FlagType::Exit, EX_LOCKED) ?
                            "and locked" : "and unlocked",
                            d->checkFlag(FlagType::Exit, EX_PICKPROOF) ?
                            " (pickproof)" : "");
                }
            }
            else { /* This is what mortal characters see */
                if (!d->checkFlag(FlagType::Exit, EX_CLOSED)) {
                    /* And the door is open */
                    door_found++;
                    char blam[9];
                    sprintf(blam, "%s", dirs[door]);
                    *blam = toupper(*blam);

                    sprintf(dl, "@c%-9s @D-@w %s\r\n", blam,
                            IS_DARK(dest->getUID()) && !nightvision && !has_light
                            ? "@bToo dark to tell.@w" : dest->getDisplayName(viewer).c_str());

                } else if (CONFIG_DISP_CLOSED_DOORS && !d->checkFlag(FlagType::Exit, EX_SECRET)) {
                    /* But we tell them the door is closed */
                    door_found++;
                    char blam[9];
                    sprintf(blam, "%s", dirs[door]);
                    *blam = toupper(*blam);
                    if (door == 6) {

                    }
                    sprintf(dl, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam,
                            (al.c_str()) ? fname(al.c_str())
                                         : "opening");
                }
            }
        }

        if (!door_found)
            result += fmt::sprintf(" None.\r\n");
        if (strstr(dlist1, "Northwest")) {
            result += fmt::sprintf("%s", dlist1);
            *dlist1 = '\0';
        }
        if (strstr(dlist2, "North")) {
            result += fmt::sprintf("%s", dlist2);
            *dlist2 = '\0';
        }
        if (strstr(dlist3, "Northeast")) {
            result += fmt::sprintf("%s", dlist3);
            *dlist3 = '\0';
        }
        if (strstr(dlist4, "East")) {
            result += fmt::sprintf("%s", dlist4);
            *dlist4 = '\0';
        }
        if (strstr(dlist5, "Southeast")) {
            result += fmt::sprintf("%s", dlist5);
            *dlist5 = '\0';
        }
        if (strstr(dlist6, "South")) {
            result += fmt::sprintf("%s", dlist6);
            *dlist6 = '\0';
        }
        if (strstr(dlist7, "Southwest")) {
            result += fmt::sprintf("%s", dlist7);
            *dlist7 = '\0';
        }
        if (strstr(dlist8, "West")) {
            result += fmt::sprintf("%s", dlist8);
            *dlist8 = '\0';
        }
        if (strstr(dlist9, "Up")) {
            result += fmt::sprintf("%s", dlist9);
            *dlist9 = '\0';
        }
        if (strstr(dlist10, "Down")) {
            result += fmt::sprintf("%s", dlist10);
            *dlist10 = '\0';
        }
        if (strstr(dlist11, "Inside")) {
            result += fmt::sprintf("%s", dlist11);
            *dlist11 = '\0';
        }
        if (strstr(dlist12, "Outside")) {
            result += fmt::sprintf("%s", dlist12);
            *dlist12 = '\0';
        }
        
        result += fmt::sprintf("@D------------------------------------------------------------------------@n\r\n");
    }
    return result;
}


std::string Room::renderExits2(GameEntity* viewer) {
    int door, slen = 0;
    std::string result;
    result += fmt::sprintf("\nExits: ");

    for (auto &[door, d] : getExits()) {

        auto dest = d->getDestination();
        if(!dest) continue;
        if (d->checkFlag(FlagType::Exit, EX_CLOSED))
            continue;

        result += fmt::sprintf("%s ", abbr_dirs[door]);
        slen++;
    }

    result += fmt::sprintf("%s\r\n", slen ? "" : "None!");
    return result;
}

std::string Room::generateMap(GameEntity *viewer, int num) {
    std::string result;
    int door, i;
    char map[9][10] = {{'-'},
                       {'-'}};
    char buf2[MAX_INPUT_LENGTH];

    if (num == 1) {
        /* Map Key */
        result += fmt::sprintf("@W               @D-[@CArea Map@D]-\r\n");
        result += fmt::sprintf("@D-------------------------------------------@w\r\n");
        result += fmt::sprintf("@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
        result += fmt::sprintf("@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
        result += fmt::sprintf("@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
        result += fmt::sprintf("@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
        result += fmt::sprintf("@WLastly @RX@W = You.\r\n");
        result += fmt::sprintf("@D-------------------------------------------\r\n");
        result += fmt::sprintf("@D                  @CNorth@w\r\n");
        result += fmt::sprintf("@D                    @c^@w\r\n");
        result += fmt::sprintf("@D             @CWest @c< O > @CEast@w\r\n");
        result += fmt::sprintf("@D                    @cv@w\r\n");
        result += fmt::sprintf("@D                  @CSouth@w\r\n");
        result += fmt::sprintf("@D                ---------@w\r\n");
    }

    /* blank the map */
    for (i = 0; i < 9; i++) {
        strcpy(map[i], "         ");
    }
    
    /* print out exits */
    map_draw_room(map, 4, 4, this, viewer);
    auto exits = getExits();
    for (auto &[door, d] : exits) {
        if(d->checkFlag(FlagType::Exit, EX_CLOSED)) continue;
        auto dest = d->getDestination();
        if(!dest) continue;

        switch (door) {
            case NORTH:
                map_draw_room(map, 4, 3, dest, viewer);
                break;
            case EAST:
                map_draw_room(map, 5, 4, dest, viewer);
                break;
            case SOUTH:
                map_draw_room(map, 4, 5, dest, viewer);
                break;
            case WEST:
                map_draw_room(map, 3, 4, dest, viewer);
                break;
            case NORTHEAST:
                map_draw_room(map, 5, 3, dest, viewer);
                break;
            case NORTHWEST:
                map_draw_room(map, 3, 3, dest, viewer);
                break;
            case SOUTHEAST:
                map_draw_room(map, 5, 5, dest, viewer);
                break;
            case SOUTHWEST:
                map_draw_room(map, 3, 5, dest, viewer);
                break;
        }
    }

    /* make it obvious what room they are in */
    map[4][4] = 'x';

    /* print out the map */
    int key = 0;
    *buf2 = '\0';
    for (i = 2; i < 9; i++) {
        if (i > 6) {
            continue;
        }
        if (num == 1) {
            sprintf(buf2, "@w                %s\r\n", map[i]);
        } else {
            if (i == 2) {
                sprintf(buf2, "@w       @w|%s@w|           %s",
                        (exits[0] && !EXIT_FLAGGED(exits[0], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[0],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rN " : " @CN ")
                                                                                          : "   ", map[i]);
            }
            if (i == 3) {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                        (exits[6] && !EXIT_FLAGGED(exits[6], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[6],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rNW" : " @CNW")
                                                                                          : "   ",
                        (exits[4] && !EXIT_FLAGGED(exits[4], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[4],
                                                                                                     EX_CLOSED)
                                                                                             ? " @yU " : " @YU ")
                                                                                          : "   ",
                        (exits[7] && !EXIT_FLAGGED(exits[7], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[7],
                                                                                                     EX_SECRET)
                                                                                             ? "@rNE " : "@CNE ")
                                                                                          : "   ", map[i]);
            }
            if (i == 4) {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                        (exits[3] && !EXIT_FLAGGED(exits[3], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[3],
                                                                                                     EX_CLOSED)
                                                                                             ? "  @rW" : "  @CW")
                                                                                          : "   ",
                        (exits[10] && !EXIT_FLAGGED(exits[10], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                            exits[10],
                                                                                                       EX_CLOSED)
                                                                                               ? " @rI " : " @mI ")
                                                                                            : ((exits[11] &&
                                                                                                !EXIT_FLAGGED(
                                                                                                        exits[11],
                                                                                                        EX_SECRET))
                                                                                               ? (EXIT_FLAGGED(
                                                                                                          exits[11],
                                                                                                          EX_CLOSED)
                                                                                                  ? "@rOUT" : "@mOUT")
                                                                                               : "@r{ }"),
                        (exits[1] && !EXIT_FLAGGED(exits[1], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[1],
                                                                                                     EX_CLOSED)
                                                                                             ? "@rE  " : "@CE  ")
                                                                                          : "   ", map[i]);
            }
            if (i == 5) {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                        (exits[9] && !EXIT_FLAGGED(exits[9], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[9],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rSW" : " @CSW")
                                                                                          : "   ",
                        (exits[5] && !EXIT_FLAGGED(exits[5], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[5],
                                                                                                     EX_CLOSED)
                                                                                             ? " @yD " : " @YD ")
                                                                                          : "   ",
                        (exits[8] && !EXIT_FLAGGED(exits[8], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[8],
                                                                                                     EX_SECRET)
                                                                                             ? "@rSE " : "@CSE ")
                                                                                          : "   ", map[i]);
            }
            if (i == 6) {
                sprintf(buf2, "@w       @w|%s@w|           %s",
                        (exits[2] && !EXIT_FLAGGED(exits[2], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[2],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rS " : " @CS ")
                                                                                          : "   ", map[i]);
            }
        }
        search_replace(buf2, "x", "@RX");
        search_replace(buf2, "&", "@m$");
        search_replace(buf2, "*", "@m#");
        search_replace(buf2, "+", "@c~");
        search_replace(buf2, "s", "@CS");
        search_replace(buf2, "i", "@wI");
        search_replace(buf2, "(", "@WC");
        search_replace(buf2, "^", "@DM");
        search_replace(buf2, "h", "@yH");
        search_replace(buf2, "`", "@BW");
        search_replace(buf2, "=", "@bU");
        search_replace(buf2, "p", "@GP");
        search_replace(buf2, "f", "@gF");
        search_replace(buf2, "!", "@YD");
        search_replace(buf2, "-", "@w:");
        /* ------- Do Lava Rooms ------- */
        search_replace(buf2, "1", "@4@YC@n");
        search_replace(buf2, "2", "@4@YP@n");
        search_replace(buf2, "3", "@4@YH@n");
        search_replace(buf2, "7", "@4@YD@n");
        search_replace(buf2, "5", "@4@YM@n");
        search_replace(buf2, "6", "@4@YF@n");
        /* ------- Do Closed Rooms------- */
        search_replace(buf2, "8", "@1 @n");

        if (num != 1) {
            if (key == 0) {
                result += fmt::sprintf("%s    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n", buf2);
            }
            if (key == 1) {
                result += fmt::sprintf("%s    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n", buf2);
            }
            if (key == 2) {
                result += fmt::sprintf("%s    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n", buf2);
            }
            if (key == 3) {
                result += fmt::sprintf("%s    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n", buf2);
            }
            if (key == 4) {
                result += fmt::sprintf("%s    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n", buf2);
            }
            key += 1;
        } else {
            result += fmt::sprintf(buf2);
        }
    }
    if (num == 1) {
        result += fmt::sprintf("@D                ---------@w\r\n");
    }
    return result;
}