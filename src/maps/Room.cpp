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
bool Room::isInsideNormallyDark(GameEntity* viewer) {
    if(checkFlag(FlagType::Room, ROOM_DARK)) return true;
    if(lit_sectors.contains(sector_type)) return false;
    if(checkFlag(FlagType::Room, ROOM_INDOORS)) return false;

    return false;
}

static const std::set<int> sun_down = {SUN_SET, SUN_DARK};

bool Room::isInsideDark(GameEntity* viewer) {

    // If the room is not normally dark, then it's definitely not dark.
    if(!isInsideNormallyDark(viewer)) return false;

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
    if(!plan->checkFlag(FlagType::Structure, STRUCTURE_HASMOON)) return MoonCheck::NoMoon;

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
                            dest->isInsideDark(viewer) && !nightvision && !has_light
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

std::string Room::renderLocationFor(GameEntity *viewer) {
    std::string result;
    
    auto sunk = isSunken();
    auto nodec = viewer->checkFlag(FlagType::Pref, PRF_NODEC);
    double grav = getEnvVar(EnvVar::Gravity);

    if (viewer->checkFlag(FlagType::Pref, PRF_ROOMFLAGS)) {
        char buf[MAX_STRING_LENGTH];
        char buf2[MAX_STRING_LENGTH];
        char buf3[MAX_STRING_LENGTH];

        sprintf(buf, "%s", join(getFlagNames(FlagType::Room), " ").c_str());
        sprinttype(sector_type, sector_types, buf2, sizeof(buf2));
        if (!nodec) {
            result += "\r\n@wO----------------------------------------------------------------------O@n\r\n";
        }

        result += fmt::sprintf("@wLocation: @G%-70s@w\r\n", getDisplayName(viewer));
        if (!script->dgScripts.empty()) {
            result += fmt::sprintf("@D[@GTriggers");
            for (auto t : script->dgScripts)
                result += fmt::sprintf(" %d", GET_TRIG_VNUM(t));
            result += fmt::sprintf("@D] ");
        }
        if(auto parent = getLocation(); parent) {
            std::vector<std::string> ancestors;
            while(parent) {
                ancestors.emplace_back(fmt::format("{}@n {}@n", parent->renderListPrefixFor(viewer), parent->getDisplayName(viewer)));
                parent = parent->getLocation();
                if(parent == this) break;
            }
            // Reverse areas.
            std::reverse(ancestors.begin(), ancestors.end());
            auto joined = join(ancestors, " -> ");
            result += fmt::sprintf("@wArea: @D[@n %s @D]@n\r\n", joined.c_str());
        }
        
        auto g = fmt::format("{}", grav);
        sprintf(buf3, "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2,
                getUID(), g.c_str());
        result += fmt::sprintf("@wFlags: %-70s@w\r\n", buf3);
        if (!nodec) {
            result += fmt::sprintf("@wO----------------------------------------------------------------------O@n\r\n");
        }
    } else {
        if (!nodec) {
            result += fmt::sprintf("@wO----------------------------------------------------------------------O@n\r\n");
        }
        result += fmt::sprintf("@wLocation: %-70s@n\r\n", getDisplayName(viewer));
        if(auto planet = getPlanet(); planet) {
            result += fmt::sprintf("@wPlanet: @G%s@n\r\n", planet->getDisplayName(viewer));
        } else {
            if (checkFlag(FlagType::Room, ROOM_NEO)) {
                result += fmt::sprintf("@wPlanet: @WNeo Nirvana@n\r\n");
            } else if (checkFlag(FlagType::Room, ROOM_AL)) {
                result += fmt::sprintf("@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
            } else if (checkFlag(FlagType::Room, ROOM_HELL)) {
                result += fmt::sprintf("@wDimension: @RPunishment Hell@n\r\n");
            } else if (checkFlag(FlagType::Room, ROOM_RHELL)) {
                result += fmt::sprintf("@wDimension: @RH@re@Dl@Rl@n\r\n");
            }
        }

        if(grav <= 1.0) {
            result += fmt::sprintf("@wGravity: @WNormal@n\r\n");
        } else {
            auto g = fmt::format("{}", grav);
            result += fmt::sprintf("@wGravity: @W%sx@n\r\n", g.c_str());
        }
        if (checkFlag(FlagType::Room, ROOM_REGEN)) {
            result += fmt::sprintf("@CA feeling of calm and relaxation fills this room.@n\r\n");
        }
        if (checkFlag(FlagType::Room, ROOM_AURA)) {
            result += fmt::sprintf("@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
        }
        if (checkFlag(FlagType::Room, ROOM_HBTC)) {
            result += fmt::sprintf("@rThis room feels like it opperates in a different time frame.@n\r\n");
        }
        if (!nodec) {
            result += fmt::sprintf("@wO----------------------------------------------------------------------O@n\r\n");
        }
    }
    
    auto dmg = getDamage();

    if ((!viewer->checkFlag(FlagType::Pref, PRF_BRIEF)) || checkFlag(FlagType::Room, ROOM_DEATH)) {
        if (dmg <= 99) {
            result += fmt::sprintf("@w%s@n",getLookDesc());
        }
        if (dmg == 100 &&
            (sector_type == SECT_WATER_SWIM || sunk || sector_type == SECT_FLYING ||
             sector_type == SECT_SHOP || sector_type == SECT_IMPORTANT)) {
            result += fmt::sprintf("@w%s@n", getLookDesc());
        }
        if (sector_type == SECT_INSIDE && dmg > 0) {
            result += fmt::sprintf("\r\n");
            if (dmg <= 2) {
                result += fmt::sprintf("@wA small hole with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 4) {
                result += fmt::sprintf("@wA couple small holes with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 6) {
                result += fmt::sprintf("@wA few small holes with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 10) {
                result += fmt::sprintf(
                             "@wThere are several small holes with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 20) {
                result += fmt::sprintf("@wMany holes fill the floor of this area, many of which have burn marks.@n");
            } else if (dmg <= 30) {
                result += fmt::sprintf("@wThe floor is severely damaged with many large holes.@n");
            } else if (dmg <= 50) {
                result += fmt::sprintf(
                             "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
            } else if (dmg <= 75) {
                result += fmt::sprintf("@wThis entire area is falling apart, it has been damaged so badly.@n");
            } else if (dmg <= 99) {
                result += fmt::sprintf(
                             "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
            } else if (dmg >= 100) {
                result += fmt::sprintf(
                             "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up holes, and overflowing onto what is left of the\r\nfloor. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
            }
            result += fmt::sprintf("\r\n");
        } else if (
                (sector_type == SECT_CITY || sector_type == SECT_FIELD || sector_type == SECT_HILLS ||
                 sector_type == SECT_IMPORTANT) && dmg > 0) {
            result += fmt::sprintf("\r\n");
            if (dmg <= 2) {
                result += fmt::sprintf("@wA small hole with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 4) {
                result += fmt::sprintf(
                             "@wA couple small craters with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 6) {
                result += fmt::sprintf("@wA few small craters with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 10) {
                result += fmt::sprintf(
                             "@wThere are several small craters with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 20) {
                result += fmt::sprintf("@wMany craters fill the ground of this area, many of which have burn marks.@n");
            } else if (dmg <= 30) {
                result += fmt::sprintf("@wThe ground is severely damaged with many large craters.@n");
            } else if (dmg <= 50) {
                result += fmt::sprintf(
                             "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
            } else if (dmg <= 75) {
                result += fmt::sprintf("@wThis entire area is falling apart, it has been damaged so badly.@n");
            } else if (dmg <= 99) {
                result += fmt::sprintf(
                             "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
            } else if (dmg >= 100) {
                result += fmt::sprintf(
                             "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up craters, and overflowing onto what is left of the\r\nground. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
            }
            result += fmt::sprintf("\r\n");
        } else if (sector_type == SECT_FOREST && dmg > 0) {
            result += fmt::sprintf("\r\n");
            if (dmg <= 2) {
                result += fmt::sprintf("@wA small tree sits in a little crater here.@n");
            } else if (dmg <= 4) {
                result += fmt::sprintf("@wTrees have been uprooted by craters in the ground.@n");
            } else if (dmg <= 6) {
                result += fmt::sprintf(
                             "@wSeveral trees have been reduced to chunks of debris and are\r\nlaying in a few craters here. @n");
            } else if (dmg <= 10) {
                result += fmt::sprintf("@wA large patch of trees have been destroyed and are laying in craters here.@n");
            } else if (dmg <= 20) {
                result += fmt::sprintf("@wSeveral craters have merged into one large crater in one part of this forest.@n");
            } else if (dmg <= 30) {
                result += fmt::sprintf(
                             "@wThe open sky can easily be seen through a hole of trees destroyed\r\nand resting at the bottom of several craters here.@n");
            } else if (dmg <= 50) {
                result += fmt::sprintf(
                             "@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
            } else if (dmg <= 75) {
                result += fmt::sprintf(
                             "@wVery few trees are left standing in this area, replaced instead by large craters.@n");
            } else if (dmg <= 99) {
                result += fmt::sprintf(
                             "@wSingle solitary trees can be found still standing here or there in the area.\r\nThe rest have been almost completely obliterated in recent conflicts.@n");
            } else if (dmg >= 100) {
                result += fmt::sprintf(
                             "@w  One massive crater fills this area. This desolate crater leaves no\r\nevidence of what used to be found in the area. Smoke slowly wafts into\r\nthe sky from the central point of the crater, creating an oppressive\r\natmosphere.@n");
            }
            result += fmt::sprintf("\r\n");
        } else if (sector_type == SECT_MOUNTAIN && dmg > 0) {
            result += fmt::sprintf("\r\n");
            
            if (dmg <= 2) {
                result += fmt::sprintf("@wA small crater has been burned into the side of this mountain.@n");
            } else if (dmg <= 4) {
                result += fmt::sprintf("@wA couple craters have been burned into the side of this mountain.@n");
            } else if (dmg <= 6) {
                result += fmt::sprintf(
                             "@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
            } else if (dmg <= 10) {
                result += fmt::sprintf("@wSeveral bad craters can be seen in the side of the mountain here.@n");
            } else if (dmg <= 20) {
                result += fmt::sprintf(
                             "@wLarge boulders have rolled down the mountain side and collected in many nearby craters.@n");
            } else if (dmg <= 30) {
                result += fmt::sprintf("@wMany craters are covering the mountainside here.@n");
            } else if (dmg <= 50) {
                result += fmt::sprintf(
                             "@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
            } else if (dmg <= 75) {
                result += fmt::sprintf("@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
            } else if (dmg <= 99) {
                result += fmt::sprintf(
                             "@wThe mountain side here has completely collapsed, shedding dangerous rubble down to its base.@n");
            } else if (dmg >= 100) {
                result += fmt::sprintf(
                             "@w  Half the mountain has been blown away, leaving a scarred and jagged\r\nrock in its place. Billowing smoke wafts up from several parts of the\r\nmountain, filling the nearby skies and blotting out the sun.@n");
            }
            result += fmt::sprintf("\r\n");
        }
        if (geffect >= 1 && geffect <= 5) {
            result += fmt::sprintf("@rLava@w is pooling in someplaces here...@n\r\n");
        }
        if (geffect >= 6) {
            result += fmt::sprintf("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        if (geffect < 0) {
            result += fmt::sprintf("@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
        }
    }

    if(auto exstr = nodec ? renderExits2(viewer) : renderExits1(viewer); !exstr.empty()) result += exstr;

    std::vector<std::string> contentLines;
    for(auto c : getContents()) {
        if(c == viewer) continue;
        if(c->getFamily() == UnitFamily::Exit) continue;
        if(!viewer->canSee(c)) continue;
        auto line = c->renderRoomListingFor(viewer);
        trim(line);
        contentLines.emplace_back(line);
    }
    if(!contentLines.empty()) {
        auto joined = join(contentLines, "\r\n");
        result += joined;
    }

    return result;
}

bool Room::checkPostEnter(GameEntity* mover, const Location& loc, const Destination& dest) {
    if(auto ch = dynamic_cast<BaseCharacter*>(mover); ch) {
        entry_memory_mtrigger(ch);
    if (!greet_mtrigger(ch, dest.direction)) {
        ch->removeFromLocation();
        ch->addToLocation(loc);
        ch->lookAtLocation();
    } else greet_memory_mtrigger(ch);

    }

    return 1;
}

bool Room::checkCanLeave(GameEntity* mover, const Destination& dest, bool need_specials_check) {
    auto was_in = mover->getLocationInfo();
    char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
    auto direction = dest.direction;

    if(auto ch = dynamic_cast<BaseCharacter*>(mover); ch) {
        if (need_specials_check && special(ch, direction + 1, throwaway))
            return false;
        if (!leave_mtrigger(ch, direction) || ch->getLocationInfo() != was_in) /* prevent teleport crashes */
            return false;
        if (!leave_wtrigger(this, ch, direction) || ch->getLocationInfo() != was_in) /* prevent teleport crashes */
            return false;
        if (!leave_otrigger(this, ch, direction) || ch->getLocationInfo() != was_in) /* prevent teleport crashes */
            return false;
    }

    return true;

}

bool Room::checkCanReachDestination(GameEntity* mover, const Destination& dest) {
    
    if(auto ch = dynamic_cast<BaseCharacter*>(mover); ch) {
        // First check for zone restrictions.
        if (!IS_NPC(ch) && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
            (GET_LEVEL(ch) < ZONE_MINLVL(zone)) && (ZONE_MINLVL(zone) > 0)) {
            ch->sendf("Sorry, you are too low a level to enter this zone.\r\n");
            return false;
        }

        if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && (GET_LEVEL(ch) > ZONE_MAXLVL(zone)) &&
            (ZONE_MAXLVL(zone) > 0)) {
            ch->sendf("Sorry, you are too high a level to enter this zone.\r\n");
            return false;
        }

        if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && ZONE_FLAGGED(zone, ZONE_CLOSED)) {
            ch->sendf("This zone is currently closed to mortals.\r\n");
            return false;
        }

        if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GRGOD)
            && ZONE_FLAGGED(zone, ZONE_NOIMMORT)) {
            ch->sendf("This zone is closed to all.\r\n");
            return false;
        }

        if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GOD) &&
            !can_edit_zone(ch, zone) && ZONE_FLAGGED(zone, ZONE_QUEST)) {
            ch->sendf("This is a Quest zone.\r\n");
            return false;
        }

        // Next, we'll handle terrain checks.


        /*
        int willfall = false;
    if ((r->sector_type == SECT_FLYING) || (dest->sector_type == SECT_FLYING)) {
        if (!has_flight(ch)) {
            if (dir != 4) {
                willfall = true;
            } else {
                ch->sendf("You need to fly to go there!\r\n");
                return (0);
            }
        }
    }

    if (((r->sector_type == SECT_WATER_NOSWIM) || (dest->sector_type == SECT_WATER_NOSWIM)) &&
        IS_HUMANOID(ch)) {
        if (IS_KANASSAN(ch) && !has_flight(ch)) {
            act("@CYou swim swiftly.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@c$n@C swims swiftly.@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (IS_ICER(ch) && !has_flight(ch)) {
            act("@CYou swim swiftly.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@c$n@C swims swiftly.@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (!IS_KANASSAN(ch) && !IS_ICER(ch) && !has_flight(ch)) {
            if (!check_swim(ch)) {
                return (0);
            } else {
                act("@CYou swim through the cold water.@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@c$n@C swim through the cold water.@n", true, ch, nullptr, nullptr, TO_ROOM);
                WAIT_STATE(ch, PULSE_1SEC);
            }
        }
    }

    if (r->checkFlag(FlagType::Room, ROOM_SPACE)) {
        if (!IS_ANDROID(ch)) {
            if (!check_swim(ch)) {
                return (0);
            }
        }
    }

    if (dest->geffect == 6 && !IS_HUMANOID(ch) && IS_NPC(ch)) {
        return (0);
    }

    if (IS_NPC(ch) && dest->checkFlag(FlagType::Room, ROOM_NOMOB) && !ch->master) {
        return (0);
    }

    if (r->isSunken() || dest->isSunken()) {
        if (!has_o2(ch) &&
            ((group_bonus(ch, 2) != 10 && (ch->getCurKI()) < GET_MAX_MANA(ch) / 200) || (group_bonus(ch, 2) == 10 &&
                                                                                         (ch->getCurKI()) <
                                                                                         GET_MAX_MANA(ch) / 800))) {
            if (ch->decCurHealthPercent(0.05) > 0) {
                ch->sendf("@RYou struggle to breath!@n\r\n");
            }
            else {
                ch->sendf("@rYou drown!@n\r\n");
                die(ch, nullptr);
                return (0);
            }
        }
        if (!has_o2(ch) &&
            ((group_bonus(ch, 2) != 10 && (ch->getCurKI()) >= GET_MAX_MANA(ch) / 200) || (group_bonus(ch, 2) == 10 &&
                                                                                          (ch->getCurKI()) >=
                                                                                          GET_MAX_MANA(ch) / 800))) {
            ch->sendf("@CYou hold your breath!@n\r\n");
            if (group_bonus(ch, 2) == 10) {
                ch->decCurKI(ch->getMaxKI() / 800);
            } else {
                ch->decCurKI(ch->getMaxKI() / 200);
            }
        }
    }

    if(!IS_NPC(ch)) {
        auto gravity = ch->myEnvVar(EnvVar::Gravity);
        need_movement = (gravity * gravity) * ch->getBurdenRatio();
    }

    if (GET_LEVEL(ch) <= 1) {
        need_movement = 0;
    }

    if (AFF_FLAGGED(ch, AFF_HIDE))
        need_movement *= ((roll_skill(ch, SKILL_HIDE) > 15) ? 2 : 4);

    if (AFF_FLAGGED(ch, AFF_SNEAK))
        need_movement *= ((roll_skill(ch, SKILL_MOVE_SILENTLY) > 15) ? 1.2 : 2);

    int flight_cost = 0;

    if (AFF_FLAGGED(ch, AFF_FLYING) && !IS_ANDROID(ch)) {
        if (!GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS)) {
            flight_cost = GET_MAX_MANA(ch) / 100;
        } else if (GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS)) {
            flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_CONCENTRATION) * 2);
        } else if (!GET_SKILL(ch, SKILL_CONCENTRATION) && GET_SKILL(ch, SKILL_FOCUS)) {
            flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_FOCUS) * 3);
        } else {
            flight_cost =
                    GET_MAX_MANA(ch) / ((GET_SKILL(ch, SKILL_CONCENTRATION) * 2) + (GET_SKILL(ch, SKILL_FOCUS) * 3));
        }
    }

    if (AFF_FLAGGED(ch, AFF_FLYING) && ((ch->getCurKI()) < flight_cost) && !IS_ANDROID(ch)) {
        ch->decCurKI(flight_cost);
        act("@WYou crash to the ground, too tired to fly anymore!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n@W crashes to the ground!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->clearFlag(FlagType::Affect,AFF_FLYING);
    } else if (AFF_FLAGGED(ch, AFF_FLYING) && !IS_ANDROID(ch)) {
        ch->decCurKI(flight_cost);
    }

    if ((ch->getCurST()) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch)) {
        if (need_specials_check && ch->master) {
            ch->sendf("You are too exhausted to follow.\r\n");
        } else {
            ch->sendf("You are too exhausted.\r\n");
        }

        return (0);
    }

    if (e->dcskill != 0) {
        if (e->dcmove > roll_skill(ch, e->dcskill)) {
            ch->sendf("Your skill in %s isn't enough to move that way!\r\n",
                         spell_info[e->dcskill].name);

            if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_FLYING))
                ch->decCurST(need_movement);
            return (0);
        } else {
            ch->sendf("Your skill in %s aids in your movement.\r\n", spell_info[e->dcskill].name);
        }
    }

    if (dest->checkFlag(FlagType::Room, ROOM_TUNNEL) && (num_pc_in_room(dest) >= CONFIG_TUNNEL_SIZE)) {
        if (CONFIG_TUNNEL_SIZE > 1)
            ch->sendf("There isn't enough room for you to go there!\r\n");
        else
            ch->sendf("There isn't enough room there for more than one person!\r\n");
        return (0);
    }

    if (dest->checkFlag(FlagType::Room, ROOM_GODROOM) &&
        GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
        ch->sendf("You aren't godly enough to use that room!\r\n");
        return (0);
    }
    */

    }
    
    return true;

}