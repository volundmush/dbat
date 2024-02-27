#include "dbat/entity.h"
#include "dbat/constants.h"
#include "dbat/races.h"
#include "dbat/utils.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"

bool ObjectID::operator==(const ObjectID& other) const {
    return id == other.id && time == other.time;
}

void ObjectID::deserialize(const nlohmann::json& j) {
    if(j.contains("id")) id = j["id"];
    if(j.contains("time")) time = j["time"];
}

nlohmann::json ObjectID::serialize() {
    nlohmann::json j;
    j["id"] = id;
    j["time"] = time;
    return j;
}

ObjectID::ObjectID(const nlohmann::json& j) {
    deserialize(j);
}

entt::entity ObjectID::get() const {
    auto find = entities.find(id);
    if(find == entities.end()) return entt::null;
    if(find->second.first != time) return entt::null;
    if(!reg.valid(find->second.second)) {
        entities.erase(id);
        return entt::null;
    }
    return find->second.second;
}

ObjectID::operator bool() const {
    return get() != entt::null;
}

Destination::Destination(GameEntity* target) : Destination() {
    this->target = target->ent;
    this->locationType = 0;
}

Destination::Destination(Room* target) : Destination() {
    this->target = target->ent;
    this->locationType = 0;
}

Destination::Destination(Character* target) : Destination() {
    this->target = target->ent;
    this->locationType = 0;
}

void deserializeEntity(entt::entity ent, const nlohmann::json& j) {
    if(j.contains("Flags")) {
        auto &flags = reg.get_or_emplace<Flags>(ent);
        flags.deserialize(j["Flags"]);
    }

}

nlohmann::json serializeEntity(entt::entity ent) {
    nlohmann::json j;

    if(auto flags = reg.try_get<Flags>(ent); flags) {
        j["Flags"] = flags->serialize();
    }

    return j;
}

void deserializeEntityRelations(entt::entity ent, const nlohmann::json& j) {

}

nlohmann::json serializeEntityRelations(entt::entity ent, nlohmann::json& j) {

}

namespace send {
    void text(entt::entity ent, const std::string& txt) {

    }

    void event(entt::entity ent, const Event& event) {
        
    }

    void line(entt::entity ent, const std::string& txt) {
        if(!txt.ends_with("\r\n")) {
            text(ent, txt + "\r\n");
        } else {
            text(ent, txt);
        }
    }

    void textContents(entt::entity ent, const std::string& txt) {

    }

    void lineContents(entt::entity ent, const std::string& txt) {
        if(!txt.ends_with("\r\n")) {
            textContents(ent, txt + "\r\n");
        } else {
            textContents(ent, txt);
        }
    }
}

namespace flags {
    bool check(entt::entity ent, FlagType type, int flag) {
        if(auto flags = reg.try_get<Flags>(ent); flags) {
            if(auto found = flags->flags.find(type); found != flags->flags.end()) {
                return found->second.contains(flag);
            }
        }

        return false;
    }

    void set(entt::entity ent, FlagType type, int flag, bool value) {
        if(!value) {
            clear(ent, type, flag);
            return;
        }
        auto &flags = reg.get_or_emplace<Flags>(ent);
        flags.flags[type].insert(flag);
    }

    void clear(entt::entity ent, FlagType type, int flag) {
        if(auto flags = reg.try_get<Flags>(ent); flags) {
            if(auto found = flags->flags.find(type); found != flags->flags.end()) {
                found->second.erase(flag);
                if(found->second.empty()) {
                    flags->flags.erase(found);
                }
            }
            if(flags->flags.empty()) {
                reg.remove<Flags>(ent);
            }
        }
    }

    bool flip(entt::entity ent, FlagType type, int flag) {
        auto current = check(ent, type, flag);
        set(ent, type, flag, !current);
        return !current;
    }

    std::vector<std::string> getNames(entt::entity ent, FlagType type) {
        std::vector<std::string> out;
        if(auto flags = reg.try_get<Flags>(ent); flags) {
            if(auto found = flags->flags.find(type); found != flags->flags.end()) {
                switch(type) {
                    case FlagType::Admin:
                        for(auto i : found->second) {
                            out.push_back(admin_flag_names[i]);
                        }
                        break;
                    case FlagType::PC:
                        for(auto i : found->second) {
                            out.push_back(player_bits[i]);
                        }
                        break;
                    case FlagType::NPC:
                        for(auto i : found->second) {
                            out.push_back(action_bits[i]);
                        }
                        break;
                    case FlagType::Wear:
                        for(auto i : found->second) {
                            out.push_back(wear_bits[i]);
                        }
                        break;
                    case FlagType::Item:
                        for(auto i : found->second) {
                            out.push_back(extra_bits[i]);
                        }
                        break;
                    case FlagType::Affect:
                        for(auto i : found->second) {
                            out.push_back(affected_bits[i]);
                        }
                        break;
                    case FlagType::Pref:
                        for(auto i : found->second) {
                            out.push_back(preference_bits[i]);
                        }
                        break;
                    case FlagType::Room:
                        for(auto i : found->second) {
                            out.push_back(room_bits[i]);
                        }
                        break;
                    case FlagType::Exit:
                        for(auto i : found->second) {
                            out.push_back(exit_bits[i]);
                        }
                        break;
                }
            }
        }
        return out;
    }
}

namespace text {
    std::string get(entt::entity ent, const std::string& key, std::optional<std::string> placeholder) {
        if(auto text = reg.try_get<Text>(ent); text) {
            if(auto found = text->strings.find(key); found != text->strings.end()) {
                return found->second->get();
            }
        }
        if(placeholder) {
            return placeholder.value();
        }
        return "";
    }

    void set(entt::entity ent, const std::string& key, const std::string& value) {
        if(value.empty()) {
            clear(ent, key);
            return;
        }
        auto &text = reg.get_or_emplace<Text>(ent);
        text.strings[key] = internString(value);
    }

    void clear(entt::entity ent, const std::string& key) {
        if(auto text = reg.try_get<Text>(ent); text) {
            if(auto found = text->strings.find(key); found != text->strings.end()) {
                text->strings.erase(found);
            }
            if(text->strings.empty()) {
                reg.remove<Text>(ent);
            }
        }
    }
}

namespace vis {
    bool isInvisible(entt::entity ent) {
        return flags::check(ent, FlagType::Affect, AFF_INVISIBLE);
    }

    bool isHidden(entt::entity ent) {
        return flags::check(ent, FlagType::Affect, AFF_HIDE);
    }

    bool isAdminInvisible(entt::entity ent) {
        return false;
    }

    bool canSeeAdminInvisible(entt::entity ent) {
        return flags::check(ent, FlagType::Pref, PRF_HOLYLIGHT);
    }

    bool canSeeInvisible(entt::entity ent) {
        return flags::check(ent, FlagType::Affect, AFF_DETECT_INVIS);
    }

    bool canSeeHidden(entt::entity ent) {
        return false;
    }

    static const std::unordered_map<FlagType, std::set<int>> lightFlags = {
        {FlagType::Item, {ITEM_GLOW}},
        {FlagType::PC, {PLR_AURALIGHT}},
        {FlagType::ItemType, {ITEM_LIGHT, ITEM_CAMPFIRE}}
    };

    bool isProvidingLight(entt::entity ent) {
        for(auto &[type, flags] : lightFlags) {
            for(auto f : flags) if(flags::check(ent, type, f)) return true;
        }
        if(auto proto = reg.try_get<Proto>(ent); proto) {
            if(proto->family == EntityFamily::Object && proto->vn == 19093) return true;
        }
        for(auto &[pos, obj] : contents::getEquipment(ent)) {
            if(isProvidingLight(obj->ent)) return true;
        }

        return false;
    }

    bool canSeeInDark(entt::entity ent) {
        if(flags::check(ent, FlagType::Pref, PRF_HOLYLIGHT)) return true;
        // todo: add a race check here...
        return flags::check(ent, FlagType::Affect, AFF_INFRAVISION);
        if(auto phys = reg.try_get<Physiology>(ent); phys) {
            if(phys->race == RaceID::Animal) return true;
        }
        return false;
    }

    bool canSee(entt::entity viewer, entt::entity target) {
        if(viewer == target) return true;
        if(canSeeAdminInvisible(viewer)) return true;
        if(isInvisible(target) && !canSeeInvisible(viewer)) return false;
        if(isHidden(target) && !canSeeHidden(viewer)) return false;
        return true;
    }

    static const std::set<int> lit_sectors = {SECT_INSIDE, SECT_CITY, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};


    bool isInsideNormallyDark(entt::entity ent, entt::entity viewer) {
        if(auto room = reg.try_get<Room>(ent); room) {
            if(flags::check(ent, FlagType::Room, ROOM_DARK)) return true;
            if(lit_sectors.contains(room->sector_type)) return false;
            if(flags::check(ent, FlagType::Room, ROOM_INDOORS)) return false;
        }
        
        return false;
    }

    static const std::set<int> sun_down = {SUN_SET, SUN_DARK};

    bool isInsideDark(entt::entity ent, entt::entity viewer) {
        if(auto room = reg.try_get<Room>(ent); room) {
            // If the room is not normally dark, then it's definitely not dark.
            if(!isInsideNormallyDark(ent, viewer)) return false;

            // Certain sectors, like cities, provide free light.
            if(lit_sectors.contains(room->sector_type)) return false;

            // Failing that, maybe the sun is up?
            if(!sun_down.contains(weather_info.sunlight)) return false;

            // welp, now it's time for the most expensive operation of all.
            for(auto u : contents::get(ent)) {
                if(vis::isProvidingLight(u)) return false;
            }

            return true;
        }
        
        return false;
    
    }

}

namespace find {
    Object* object(entt::entity ent, const std::function<bool(Object*)> &func, bool working) {
        for(auto obj : contents::getInventory(ent)) {
            if(func(obj)) {
                if(working && !obj->isWorking()) continue;
                return obj;
            }
        }
        if(reg.all_of<Character>(ent)) {
            for(auto &[pos, obj] : contents::getEquipment(ent)) {
                if(func(obj)) {
                    if(working && !obj->isWorking()) continue;
                    return obj;
                }
            }
        }

        return nullptr;
    }

    std::set<Object*> gatherObjects(entt::entity ent, const std::function<bool(Object*)> &func, bool working) {
        std::set<Object*> out;
        for(auto obj : contents::getInventory(ent)) {
            if(func(obj)) {
                if(working && !obj->isWorking()) continue;
                out.insert(obj);
            }
            auto contents = obj->gatherObjects(func, working);
            out.insert(contents.begin(), contents.end());
        }

        if(reg.all_of<Character>(ent)) {
            for(auto &[pos, obj] : contents::getEquipment(ent)) {
                if(func(obj)) {
                    if(working && !obj->isWorking()) continue;
                    out.insert(obj);
                }
                auto contents = obj->gatherObjects(func, working);
                out.insert(contents.begin(), contents.end());
            }
        }
        return out;
    }

    entt::entity holderType(entt::entity ent, int type) {
        entt::entity holder = entt::null;
        if(auto loc = reg.try_get<Location>(ent); loc) {
            holder = loc->location;
        }
        while(holder != entt::null) {
            if(flags::check(holder, FlagType::ItemType, type)) return holder;
            if(auto loc = reg.try_get<Location>(holder); loc) {
                holder = loc->location;
            } else {
                holder = entt::null;
            }
        }
        return entt::null;
    }

}

int64_t getUID(entt::entity ent) {
        if(auto info = reg.try_get<Info>(ent); info) {
            return info->uid;
        }
        return -1;
    }

EntityFamily getFamily(entt::entity ent) {
    if(auto info = reg.try_get<Info>(ent); info) {
        return info->family;
    }
    return EntityFamily::Object;
}

double getEnvVar(entt::entity env, EnvVar v) {
    return 0.0;
}

namespace contents {
    std::vector<entt::entity> get(entt::entity ent) {
        std::vector<entt::entity> out;
        if(auto contents = reg.try_get<Contents>(ent); contents) {
            for(auto &item : contents->contents) {
                if(!reg.all_of<Deleted>(item)) out.push_back(item);
            }
        }
        return out;
    }

    std::vector<entt::entity> get(GameEntity* ent) {
        return get(ent->ent);
    }

    std::vector<GameEntity*> getContents(entt::entity ent) {
        std::vector<GameEntity*> out;
        for(auto e : get(ent)) {
            auto &i = reg.get<Info>(e);
            if(i.family == EntityFamily::Character) {
                auto c = reg.try_get<Character>(e);
                out.push_back(c);
            } else if(i.family == EntityFamily::Object) {
                auto o = reg.try_get<Object>(e);
                out.push_back(o);
            } else if(i.family == EntityFamily::Room) {
                auto r = reg.try_get<Room>(e);
                out.push_back(r);
            } else if(i.family == EntityFamily::Exit) {
                auto ex = reg.try_get<Exit>(e);
                out.push_back(ex);
            }
        }
        return out;
    }

    std::vector<GameEntity*> getContents(GameEntity* ent) {
        return getContents(ent->ent);
    }

    std::vector<Room*> getRooms(entt::entity ent) {
        return get<Room>(ent);
    }

    std::vector<Room*> getRooms(GameEntity* ent) {
        return getRooms(ent->ent);
    }

    std::vector<Character*> getPeople(entt::entity ent) {
        return get<Character>(ent);
    }

    std::vector<Character*> getPeople(GameEntity* ent) {
        return getPeople(ent->ent);
    }

    std::vector<Object*> getInventory(entt::entity ent) {
        std::vector<Object*> out;
        for(auto e : get(ent)) {
            if(auto o = reg.try_get<Object>(e); o) {
                auto &loc = reg.get<Location>(e);
                if(loc.locationType == 0) out.push_back(o);
            }
        }
        return out;
    }

    std::vector<Object*> getInventory(GameEntity* ent) {
        return getInventory(ent->ent);
    }

    std::map<int, Object*> getEquipment(entt::entity ent) {
        std::map<int, Object*> out;
        for(auto e : get(ent)) {
            if(auto o = reg.try_get<Object>(e); o) {
                auto &loc = reg.get<Location>(e);
                if(loc.locationType > 0) out[loc.locationType] = o;
            }
        }
        return out;
    }

    std::map<int, Object*> getEquipment(GameEntity* ent) {
        return getEquipment(ent->ent);
    }

    std::map<int, Exit*> getExits(entt::entity ent) {
        std::map<int, Exit*> out;
        for(auto e : get(ent)) {
            if(auto o = reg.try_get<Exit>(e); o) {
                auto &loc = reg.get<Location>(e);
                out[loc.locationType] = o;
            }
        }
        return out;
    }

    std::map<int, Exit*> getExits(GameEntity* ent) {
        return getExits(ent->ent);
    }

    static void handleRemoveFromCoordinates(entt::entity ent, entt::entity mover, const Coordinates& c) {
        auto &con = reg.get_or_emplace<CoordinateContents>(ent);
        if(auto found = con.coordinateContents.find(c); found != con.coordinateContents.end()) {
            auto &c = found->second;
            // erase mover from c...
            std::erase_if(c, [mover](auto& p) { return p == mover; });
            if(c.empty())
                con.coordinateContents.erase(found);
            if(con.coordinateContents.empty())
                reg.remove<CoordinateContents>(ent);
        }
    }

    static void updateCoordinates(entt::entity ent, entt::entity mover, std::optional<Coordinates> previous) {
        if(previous) {
            handleRemoveFromCoordinates(ent, mover, previous.value());
        }
        auto &con = reg.get_or_emplace<CoordinateContents>(ent);
        auto &loc = reg.get<Location>(mover);
        con.coordinateContents[loc.coords].push_back(mover);
    }

    static void handleAdd(entt::entity ent, entt::entity mover) {
        auto &con = reg.get_or_emplace<Contents>(ent);
        con.contents.push_back(mover);
        updateCoordinates(ent, mover);
    }

    

    static void handleRemove(entt::entity ent, entt::entity mover) {
        if(auto con = reg.try_get<Contents>(ent); con) {
            std::erase_if(con->contents, [mover](auto c) {return c == mover;});
            if(con->contents.empty()) reg.remove<Contents>(ent);
        }
    }

    void removeFrom(entt::entity ent) {
        if(auto loc = reg.try_get<Location>(ent); !loc) {
            basic_mud_log("Attempted to remove unit '%d: %s' from location, but location was not found.", getUID(ent), text::get(ent, "name", "nameless").c_str());
            return;
        } else {
            handleRemove(loc->location, ent);
            handleRemoveFromCoordinates(loc->location, ent, loc->coords);
            reg.remove<Location>(ent);
        }
    }

    void addTo(entt::entity ent, const Destination& dest) {
        if(auto location = reg.try_get<Location>(ent); location) {
            if(location->location != dest.target) {
                basic_mud_log("Attempted to add unit '%d: %s' to location, but location was already found.", getUID(ent), text::get(ent, "name", "nameless").c_str());
                return;
            }
            location->locationType = dest.locationType;
            auto prevCoords = location->coords;
            location->coords = dest.coords;
            updateCoordinates(location->location, ent, prevCoords);
        } else {
            auto &loc = reg.get_or_emplace<Location>(ent);
            loc.location = dest.target;
            loc.locationType = dest.locationType;
            loc.coords = dest.coords;
            handleAdd(dest.target, ent);
        }
    }

}

namespace check {
    bool isPC(entt::entity ent) {
        return reg.all_of<PlayerCharacter>(ent);
    }

    bool isNPC(entt::entity ent) {
        return reg.all_of<NonPlayerCharacter>(ent);
    }
}

static const struct {
        int percent;
        std::string text;
    } character_diagnosis[] = {
            {100, "@wis in @Gexcellent@w condition."},
            {90,  "@whas a few @Rscratches@w."},
            {80,  "@whas some small @Rwounds@w and @Rbruises@w."},
            {70,  "@whas quite a few @Rwounds@w."},
            {60,  "@whas some big @rnasty wounds@w and @Rscratches@w."},
            {50,  "@wlooks pretty @rhurt@w."},
            {40,  "@wis mainly @rinjured@w."},
            {30,  "@wis a @rmess@w of @rinjuries@w."},
            {20,  "@wis @rstruggling@w to @msurvive@w."},
            {10,  "@wis in @mawful condition@w."},
            {0,   "@Ris barely alive.@w"},
            {-1,  "@ris nearly dead.@w"},
    };

static const struct {
        int percent;
        std::string text;
    } object_diagnosis[] = {
            {100, "is in excellent condition."},
            {90,  "has a few scuffs."},
            {75,  "has some small scuffs and scratches."},
            {50,  "has quite a few scratches."},
            {30,  "has some big nasty scrapes and scratches."},
            {15,  "looks pretty damaged."},
            {0,   "is in awful condition."},
            {-1,  "is in need of repair."},
    };

static const char *weapon_disp[6] = {
        "Sword",
        "Dagger",
        "Spear",
        "Club",
        "Gun",
        "Brawling"
};

namespace render {

    std::string appearance(entt::entity ent, entt::entity viewer) {
        std::string result;

        auto ch = reg.try_get<Character>(ent);
        auto obj = reg.try_get<Object>(ent);
        auto proto = reg.try_get<Proto>(ent);

        // getLookDesc() may be anything, for PCs. for NPCs, it's a fixed thing.
        if (auto ld = text::get(ent, "look_description"); !ld.empty()) {
            result += fmt::sprintf("%s\r\n", ld);
        }

        if(ch) {
            if(IS_HUMANOID(ch)) {
                if (GET_LIMBCOND(ch, 0) >= 50 && !PLR_FLAGGED(ch, PLR_CRARM)) {
                    result += fmt::sprintf("            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 0),
                                "%", "%");
                } else if (GET_LIMBCOND(ch, 0) > 0 && !PLR_FLAGGED(ch, PLR_CRARM)) {
                    result += fmt::sprintf("            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                                    GET_LIMBCOND(ch, 0), "%", "%");
                } else if (GET_LIMBCOND(ch, 0) > 0 && PLR_FLAGGED(ch, PLR_CRARM)) {
                    result += fmt::sprintf("            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                                    GET_LIMBCOND(ch, 0), "%", "%");
                } else if (GET_LIMBCOND(ch, 0) <= 0) {
                    result += fmt::sprintf("            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
                }
                if (GET_LIMBCOND(ch, 1) >= 50 && !PLR_FLAGGED(ch, PLR_CLARM)) {
                    result += fmt::sprintf("            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 1),
                                    "%", "%");
                } else if (GET_LIMBCOND(ch, 1) > 0 && !PLR_FLAGGED(ch, PLR_CLARM)) {
                    result += fmt::sprintf("            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                                    GET_LIMBCOND(ch, 1), "%", "%");
                } else if (GET_LIMBCOND(ch, 1) > 0 && PLR_FLAGGED(ch, PLR_CLARM)) {
                    result += fmt::sprintf("            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                                    GET_LIMBCOND(ch, 1), "%", "%");
                } else if (GET_LIMBCOND(ch, 1) <= 0) {
                    result += fmt::sprintf("            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
                }
                if (GET_LIMBCOND(ch, 2) >= 50 && !PLR_FLAGGED(ch, PLR_CLARM)) {
                    result += fmt::sprintf("            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 2),
                                    "%", "%");
                } else if (GET_LIMBCOND(ch, 2) > 0 && !PLR_FLAGGED(ch, PLR_CRLEG)) {
                    result += fmt::sprintf("            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                                    GET_LIMBCOND(ch, 2), "%", "%");
                } else if (GET_LIMBCOND(ch, 2) > 0 && PLR_FLAGGED(ch, PLR_CRLEG)) {
                    result += fmt::sprintf("            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                                    GET_LIMBCOND(ch, 2), "%", "%");
                } else if (GET_LIMBCOND(ch, 2) <= 0) {
                    result += fmt::sprintf("            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
                }
                if (GET_LIMBCOND(ch, 3) >= 50 && !PLR_FLAGGED(ch, PLR_CLLEG)) {
                    result += fmt::sprintf("            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 3),
                                    "%", "%");
                } else if (GET_LIMBCOND(ch, 3) > 0 && !PLR_FLAGGED(ch, PLR_CLLEG)) {
                    result += fmt::sprintf("            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                                    GET_LIMBCOND(ch, 3), "%", "%");
                } else if (GET_LIMBCOND(ch, 3) > 0 && PLR_FLAGGED(ch, PLR_CLLEG)) {
                    result += fmt::sprintf("            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                                    GET_LIMBCOND(ch, 3), "%", "%");
                } else if (GET_LIMBCOND(ch, 3) <= 0) {
                    result += fmt::sprintf("            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
                }
                if (PLR_FLAGGED(ch, PLR_HEAD)) {
                    result += fmt::sprintf("            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
                }
                if (!PLR_FLAGGED(ch, PLR_HEAD)) {
                    result += fmt::sprintf("            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
                }
                if (race::hasTail(ch->race) && !PLR_FLAGGED(ch, PLR_TAILHIDE)) {
                    if(PLR_FLAGGED(ch, PLR_TAIL))
                        result += fmt::sprintf("            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
                    else
                        result += fmt::sprintf("            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
                }
                result += fmt::sprintf("\r\n");
            }

            result += fmt::sprintf("\r\n         @D----------------------------------------@n\r\n");
            //result += trans_check(ch, viewer);
            result += fmt::sprintf("         @D----------------------------------------@n\r\n");
            result += fmt::sprintf("\r\n");

            auto [race, sex] = getApparentRaceSex(ent, viewer);

            auto heshe = heShe(sex);
            auto raceName = race::getName(race);

            result += fmt::format("{} appears to be a {} {}, ", heshe, AN(raceName.c_str()), raceName);

            auto w = ch->getWeight();
            int h = ch->getHeight();
            auto wString = fmt::format("{}kg", w);
            result += fmt::sprintf("is %s sized, about %dcm tall,\r\nabout %s heavy,", size_names[get_size(ch)],
                            h, wString.c_str());
            
            result += diagnostics(ent, viewer);

            if(!flags::check(viewer, FlagType::Pref, PRF_NOEQSEE))
                if(auto eq = equipment(ent, viewer); !eq.empty()) {
                    result += fmt::sprintf("\r\n");    /* act() does capitalization. */
                    result += fmt::sprintf("They are using:\r\n");
                    result += eq;
                }
        }

        if(obj) {

            if(flags::check(ent, FlagType::ItemType, ITEM_CORPSE)) {
                int mention = false;
                result += fmt::sprintf("This corpse has ");

                if (GET_OBJ_VAL(obj, VAL_CORPSE_HEAD) == 0) {
                    result += fmt::sprintf("no head,");
                    mention = true;
                }

                if (GET_OBJ_VAL(obj, VAL_CORPSE_RARM) == 0) {
                    result += fmt::sprintf("no right arm, ");
                    mention = true;
                } else if (GET_OBJ_VAL(obj, VAL_CORPSE_RARM) == 2) {
                    result += fmt::sprintf("a broken right arm, ");
                    mention = true;
                }

                if (GET_OBJ_VAL(obj, VAL_CORPSE_LARM) == 0) {
                    result += fmt::sprintf("no left arm, ");
                    mention = true;
                } else if (GET_OBJ_VAL(obj, VAL_CORPSE_LARM) == 2) {
                    result += fmt::sprintf("a broken left arm, ");
                    mention = true;
                }

                if (GET_OBJ_VAL(obj, VAL_CORPSE_RLEG) == 0) {
                    result += fmt::sprintf("no right leg, ");
                    mention = true;
                } else if (GET_OBJ_VAL(obj, VAL_CORPSE_RLEG) == 2) {
                    result += fmt::sprintf("a broken right leg, ");
                    mention = true;
                }

                if (GET_OBJ_VAL(obj, VAL_CORPSE_LLEG) == 0) {
                    result += fmt::sprintf("no left leg, ");
                    mention = true;
                } else if (GET_OBJ_VAL(obj, VAL_CORPSE_LLEG) == 2) {
                    result += fmt::sprintf("a broken left leg, ");
                    mention = true;
                }

                if (mention == false) {
                    result += fmt::sprintf("nothing missing from it but life.");
                } else {
                    result += fmt::sprintf("and is dead.");
                }

                result += fmt::sprintf("\r\n");
            }

            if(flags::check(ent, FlagType::ItemType, ITEM_DRINKCON)) {
                result += fmt::sprintf("It looks like a drink container.\r\n");
            }

            if(flags::check(ent, FlagType::ItemType, ITEM_FOOD) || flags::check(ent, FlagType::ItemType, ITEM_YUM)) {
                auto foob = FOOB(obj);
                auto foodval = GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL);
                if (foob >= 4) {
                    if (foodval < foob / 4) {
                        result += fmt::sprintf("Condition of the food: Almost gone.\r\n");
                    } else if (foodval < foob / 2) {
                        result += fmt::sprintf("Condition of the food: Half Eaten.");
                    } else if (foodval < foob) {
                        result += fmt::sprintf("Condition of the food: Partially Eaten.");
                    } else if (foodval == foob) {
                        result += fmt::sprintf("Condition of the food: Whole.");
                    }
                } else if (foob > 0) {
                    if (foodval < foob) {
                        result += fmt::sprintf("Condition of the food: Almost gone.");
                    } else if (foodval == foob) {
                        result += fmt::sprintf("Condition of the food: Whole.");
                    }
                } else {
                    result += fmt::sprintf("Condition of the food: Insignificant.");
                }
            }

            if(proto && proto->vn == 25) {
                if (auto grav = obj->emitEnvVar(EnvVar::Gravity); grav) {
                    result += fmt::sprintf("@wA gravity generator, set to {}x gravity, is built here.", grav.value());
                } else {
                    result += fmt::sprintf("@wA gravity generator, currently on standby, is built here.");
                }
            }

            // Glacial wall
            if(proto && proto->vn == 79) {
                auto curWeight = GET_OBJ_WEIGHT(obj);

                result += fmt::sprintf(
                                    "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction",
                                    add_commas(curWeight).c_str(), dirs[obj->cost]);
            }

            if(flags::check(ent, FlagType::ItemType, ITEM_WEAPON)) {
                int num = 0;
                auto damtype = GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE);
                if (damtype == TYPE_PIERCE - TYPE_HIT) {
                    num = 1;
                } else if (damtype == TYPE_SLASH - TYPE_HIT) {
                    num = 0;
                } else if (damtype == TYPE_CRUSH - TYPE_HIT) {
                    num = 3;
                } else if (damtype == TYPE_STAB - TYPE_HIT) {
                    num = 2;
                } else if (damtype == TYPE_BLAST - TYPE_HIT) {
                    num = 4;
                } else {
                    num = 5;
                }
                result += fmt::sprintf("The weapon type of %s@n is '%s'.\r\n", GET_OBJ_SHORT(obj), weapon_disp[num]);
                //result += fmt::sprintf("You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), this)]);

            }

            if(flags::check(ent, FlagType::ItemType, ITEM_PLANT)) {
                int water = GET_OBJ_VAL(obj, VAL_WATERLEVEL);
                auto sd = obj->getShortDesc();
                if (water >= 0) {
                    
                    switch (GET_OBJ_VAL(obj, VAL_MATURITY)) {
                        case 0:
                            result += fmt::sprintf("@wA @G%s@y seed@w has been planted here. @D(@C%d Water Hours@D)@n\r\n", sd, water);
                            break;
                        case 1:
                            result += fmt::sprintf("@wA very young @G%s@w has sprouted from a planter here. @D(@C%d Water Hours@D)@n\r\n", sd, water);
                            break;
                        case 2:
                            result += fmt::sprintf("@wA half grown @G%s@w is in a planter here. @D(@C%d Water Hours@D)@n\r\n", sd, water);
                            break;
                        case 3:
                            result += fmt::sprintf("@wA mature @G%s@w is growing in a planter here. @D(@C%d Water Hours@D)@n\r\n", sd, water);
                            break;
                        case 4:
                            result += fmt::sprintf("@wA mature @G%s@w is flowering in a planter here. @D(@C%d Water Hours@D)@n\r\n", sd, water);
                            break;
                        case 5:
                            result += fmt::sprintf("@wA mature @G%s@w that is close to harvestable is here. @D(@C%d Water Hours@D)@n\r\n", sd, water);
                            break;
                        case 6:
                            result += fmt::sprintf("@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d Water Hours@D)@n\r\n",sd, water);
                            break;
                        default:
                            break;
                    }
                } else {
                    if (water > -4) {
                        result += fmt::sprintf("@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n", sd);
                    } else if (water > -10) {
                        result += fmt::sprintf("@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n", sd);
                    } else if (water <= -10) {
                        result += fmt::sprintf("@yA @G%s@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n",
                                    sd);
                    }
                }
            }


            result += diagnostics(ent, viewer);
            result += fmt::sprintf("It appears to be made of %s, and weighs %s", material_names[GET_OBJ_MATERIAL(obj)],
                    add_commas(GET_OBJ_WEIGHT(obj)).c_str());
        }

        

        return result;
    }

    std::string diagnostics(entt::entity ent, entt::entity viewer) {
        std::string result;

        if(auto ch = reg.try_get<Character>(ent); ch) {
            int percent, ar_index;

            int64_t hit = GET_HIT(ch), max = (ch->getEffMaxPL());

            int64_t total = max;

            if (hit == total) {
                percent = 100;
            } else if (hit < total && hit >= (total * .9)) {
                percent = 90;
            } else if (hit < total && hit >= (total * .8)) {
                percent = 80;
            } else if (hit < total && hit >= (total * .7)) {
                percent = 70;
            } else if (hit < total && hit >= (total * .6)) {
                percent = 60;
            } else if (hit < total && hit >= (total * .5)) {
                percent = 50;
            } else if (hit < total && hit >= (total * .4)) {
                percent = 40;
            } else if (hit < total && hit >= (total * .3)) {
                percent = 30;
            } else if (hit < total && hit >= (total * .2)) {
                percent = 20;
            } else if (hit < total && hit >= (total * .1)) {
                percent = 10;
            } else if (hit < total * .1) {
                percent = 0;
            } else {
                percent = -1;        /* How could MAX_HIT be < 1?? */
            }

            for (ar_index = 0; character_diagnosis[ar_index].percent >= 0; ar_index++)
                if (percent >= character_diagnosis[ar_index].percent)
                    break;

            result += fmt::sprintf("%s\r\n", character_diagnosis[ar_index].text);
        }

        if(auto obj = reg.try_get<Object>(ent); obj) {
            int percent, ar_index;
            std::string objs = vis::canSee(viewer, ent) ? text::get(ent, "short_description") : "something";

            if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
                percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) / GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
            else
                percent = 0;               /* How could MAX_HIT be < 1?? */

            for (ar_index = 0; object_diagnosis[ar_index].percent >= 0; ar_index++)
                if (percent >= object_diagnosis[ar_index].percent)
                    break;

            result += fmt::sprintf("\r\n%c%s %s\r\n", UPPER(objs[0]), objs.substr(1), object_diagnosis[ar_index].text);
        }

        return result;

    }

    std::pair<RaceID, int> getApparentRaceSex(entt::entity ent, entt::entity viewer) {
        if(flags::check(viewer, FlagType::Pref, PRF_HOLYLIGHT)) {
            if(auto phys = reg.try_get<Physiology>(ent); phys) {
                return {phys->race, phys->sex};
            }
        } else {
            if(auto mim = reg.try_get<Mimic>(ent); mim) {
                return {mim->race, mim->sex};
            }
            if(auto phys = reg.try_get<Physiology>(ent); phys) {
                return {phys->race, phys->sex};
            }
        }
        return {RaceID::Spirit, SEX_NEUTRAL};
    }

    std::string hisHer(int sex, bool capitalized) {
        switch(sex) {
            case SEX_NEUTRAL:
                return capitalized ? "Its" : "its";
                break;
            case SEX_MALE:
                return capitalized ? "His" : "his";
                break;
            case SEX_FEMALE:
                return capitalized ? "Her" : "her";
                break;
        }
        return capitalized ? "Its" : "its";
    }

    std::string heShe(int sex, bool capitalized) {
        switch(sex) {
            case SEX_NEUTRAL:
                return capitalized ? "It" : "it";
                break;
            case SEX_MALE:
                return capitalized ? "He" : "he";
                break;
            case SEX_FEMALE:
                return capitalized ? "She" : "she";
                break;
        }
        return capitalized ? "It" : "it";
    }

    std::string himHer(int sex, bool capitalized) {
        switch(sex) {
            case SEX_NEUTRAL:
                return capitalized ? "It" : "it";
                break;
            case SEX_MALE:
                return capitalized ? "Him" : "him";
                break;
            case SEX_FEMALE:
                return capitalized ? "Her" : "her";
                break;
        }
        return capitalized ? "It" : "it";
    }

    std::string hisHers(int sex, bool capitalized) {
        switch(sex) {
            case SEX_NEUTRAL:
                return capitalized ? "It" : "it";
                break;
            case SEX_MALE:
                return capitalized ? "His" : "his";
                break;
            case SEX_FEMALE:
                return capitalized ? "Hers" : "hers";
                break;
        }
        return capitalized ? "It" : "it";
    }

    std::string maFe(int sex, bool capitalized) {
        switch(sex) {
            case SEX_NEUTRAL:
                return capitalized ? "Questionably Gendered" : "questionably gendered";
                break;
            case SEX_MALE:
                return capitalized ? "Male" : "male";
                break;
            case SEX_FEMALE:
                return capitalized ? "Female" : "female";
                break;
        }
        return capitalized ? "Questionably Gendered" : "questionably gendered";
    }

    static std::string _pcRenderName(entt::entity ent, entt::entity viewer) {
        if(ent == viewer || vis::canSeeAdminInvisible(ent)) {
            return text::get(ent, "name", "nameless");
        }
        auto &pc = reg.get<PlayerCharacter>(viewer);
        auto &info = reg.get<Info>(ent);
        auto found = pc.dubNames.find(info.uid);
        if(flags::check(ent, FlagType::PC, PLR_DISGUISED) || reg.all_of<Mimic>(ent)) found = pc.dubNames.end();
        if(found != pc.dubNames.end()) {
            return found->second;
        }
        auto [rc, sex] = getApparentRaceSex(ent, viewer);
        auto rname = race::getName(rc);
        to_lower(rname);
        auto an = AN(rname.c_str());
        if(sex) {
            return fmt::format("{} {} {}",an,genders[sex], rname);
        } else {
            return fmt::format("{} {}",an,rname);
        }  
    }

    std::string displayName(entt::entity ent, entt::entity viewer) {
        auto &info = reg.get<Info>(ent);
        switch(info.family) {
            case EntityFamily::Room:
                return text::get(ent, "name", "nameless");
            case EntityFamily::Exit: {
                auto &loc = reg.get<Location>(ent);
                return dirs[loc.locationType];
            }
            case EntityFamily::Character: {
                if(check::isPC(ent) && check::isPC(viewer)) {
                    return _pcRenderName(ent, viewer);
                }
                return text::get(ent, "short_desscription", text::get(ent, "name", "nameless"));
            }
            case EntityFamily::Object: {
                return text::get(ent, "short_desscription", text::get(ent, "name", "nameless"));
            }
        }
    }

    std::string apparentRaceName(entt::entity ent, entt::entity viewer, bool capitalized) {
        const auto [race, sex] = getApparentRaceSex(ent, viewer);

        auto name = race::getName(race);
        if(!capitalized) {
            to_lower(name);
        }

        return name;
    }

    // base character keywords is race name and, optionally, sex.
    static std::vector<std::string> _characterBaseKeywords(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;
        const auto [race, sex] = getApparentRaceSex(ent, viewer);
        auto rname = race::getName(race);
        to_lower(rname);
        out.emplace_back(rname);
        // male is 1, female is 2... neutral is 0.
        if(sex) {
            out.emplace_back(genders[sex]);
        }
        return out;
    }

    static std::vector<std::string> _characterKeywords(entt::entity ent, entt::entity viewer) {
        auto out = _characterBaseKeywords(ent, viewer);

        if(check::isPC(ent)) {
            auto name = split(displayName(ent, viewer), ' ');
            out.insert(out.end(), name.begin(), name.end());
        }

        else {
            auto name = split(text::get(ent, "name", "nameless"), ' ');
            out.insert(out.end(), name.begin(), name.end());
        }

        return out;
    }

    std::vector<std::string> keywords(entt::entity ent, entt::entity viewer) {
        auto &info = reg.get<Info>(ent);
        auto proto = reg.try_get<Proto>(ent);

        switch(info.family) {
            case EntityFamily::Room:
            case EntityFamily::Exit: {
                    auto name = displayName(ent, viewer);
                    return split(name, ' ');
                }
                break;
            case EntityFamily::Character: {
                return _characterKeywords(ent, viewer);
                }
                break;
            case EntityFamily::Object: {
                return split(text::get(ent, "name", "nameless"), ' ');
                }
                break;
        }
    }


    std::string listPrefix(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;

        auto &info = reg.get<Info>(ent);
        out.push_back(fmt::format("@W[#{}]", info.uid));

        if(auto proto = reg.try_get<Proto>(ent); proto) {
            out.push_back(fmt::format("@G[VN{}]", proto->vn));
        }

        // TODO: Scripts IDs.

        return out.empty() ? "" : join(out, "@n  ") + "@n";
    }

    static const std::vector<std::string> positions = {
        " is dead",
        " is mortally wounded",
        " is lying here, incapacitated",
        " is lying here, stunned",
        " is sleeping here",
        " is resting here",
        " is sitting here",
        "!FIGHTING!",
        " is standing here"
    };

    static std::string _characterRoomLine(entt::entity ent, entt::entity viewer) {
        std::string result;

        auto shd = displayName(ent, viewer);
        auto ch = reg.try_get<Character>(ent);

        // should an exclamation be used at the end of the line instead of a period?
        bool exclamation = false;

        std::vector<std::string> commaSeparated;
        auto spar = is_sparring(ch);
        if (flags::check(ent, FlagType::Affect, AFF_INVISIBLE)) {
            commaSeparated.emplace_back("is invisible");
        }
        if(flags::check(ent, FlagType::Affect, AFF_ETHEREAL)) {
            commaSeparated.emplace_back("has a halo");
        }
        if(flags::check(ent, FlagType::Affect, AFF_HIDE)) {
            commaSeparated.emplace_back("is hiding");
        }
        if(flags::check(ent, FlagType::PC, PLR_WRITING)) {
            commaSeparated.emplace_back("is writing");
        }
        if (flags::check(ent, FlagType::PC, PRF_BUILDWALK)) {
            commaSeparated.emplace_back("is buildwalking");
        }
        if(auto fight = FIGHTING(ch); fight) {
            exclamation = true;
            if(spar) {
                if(fight->ent == viewer) {
                    commaSeparated.emplace_back("is sparring with YOU");
                } else {
                    commaSeparated.emplace_back(fmt::sprintf("is sparring with %s", displayName(fight->ent, viewer)));
                }
            } else {
                if(fight->ent == viewer) {
                    commaSeparated.emplace_back("is fighting YOU");
                } else {
                    commaSeparated.emplace_back(fmt::sprintf("is fighting %s", displayName(fight->ent, viewer)));
                }
            }
        }
        if(auto grap = GRAPPLED(ch); grap) {
            exclamation = true;
            if(grap->ent == viewer) {
                commaSeparated.emplace_back("is being grappled with by YOU");
            } else {
                commaSeparated.emplace_back(fmt::sprintf("is being grappled with by %s", displayName(grap->ent, viewer)));
            }
        }
        if(auto abs = ABSORBBY(ch); abs) {
            exclamation = true;
            if(abs->ent == viewer) {
                commaSeparated.emplace_back("is being absorbed from by YOU");
            } else {
                commaSeparated.emplace_back(fmt::sprintf("is being absorbed from by %s", displayName(abs->ent, viewer)));
            }
        }
        if(auto carry = CARRYING(ch); carry) {
            if(carry->ent == viewer) {
                commaSeparated.emplace_back("is carrying YOU");
            } else {
                commaSeparated.emplace_back(fmt::sprintf("is carrying %s", displayName(carry->ent, viewer)));
            }
        }
        if(auto carryby = CARRIED_BY(ch); carryby) {
            if(carryby->ent == viewer) {
                commaSeparated.emplace_back("is being carried by YOU");
            } else {
                commaSeparated.emplace_back(fmt::sprintf("is being carried by %s", displayName(carryby->ent, viewer)));
            }
        }
        if (auto chair = SITS(ch); chair) {
            if (flags::check(ent, FlagType::PC, PLR_HEALT)) {
                commaSeparated.emplace_back("is floating inside a healing tank");
            } else if (flags::check(ent, FlagType::PC, PLR_PILOTING)) {
                commaSeparated.emplace_back("is sitting in the pilot's chair");
            } else {
                commaSeparated.emplace_back(fmt::format("{} on {}", positions[(int) GET_POS(ch)], displayName(chair->ent, viewer)));
            }
        }

        // If there are commaSeparated sections, we want result to open up as:
        // <name>, who <value, value, value>, and <last value>
        // But there might only be one value, or maybe none at all.
        if(commaSeparated.empty()) {
            if(check::isNPC(ent)) {
                result = text::get(ent, "room_description");
            } else {
                // TODO: Add a custom posture for Player characters?
                result = fmt::sprintf("@w%s is standing here.", shd);
            }
        } else if(commaSeparated.size() == 1) {
            result += fmt::sprintf("@w%s, who %s%s", shd, commaSeparated[0], exclamation ? "!" : ".");
        } else {
            for(size_t i = 0; i < commaSeparated.size(); ++i) {
                if(i == commaSeparated.size() - 1) {
                    result += fmt::sprintf("and %s%s", commaSeparated[i], exclamation ? "!" : ".");
                } else {
                    result += fmt::sprintf("%s, ", commaSeparated[i]);
                }
            }
        }

        // Lastly, we will append any suffix states, if needed.
        // There shouldn't be many of these.
        // TODO: Add a new Linkdead detection, the old one was dumb.
        std::vector<std::string> states;

        if (flags::check(viewer, FlagType::Affect, AFF_DETECT_ALIGN)) {
            if (IS_EVIL(ch))
                states.emplace_back("(@rRed@[3] Aura)@n");
            else if (IS_GOOD(ch))
                states.emplace_back("(@bBlue@[3] Aura)@n");
        }
        if (flags::check(ent, FlagType::Pref, PRF_AFK))
            states.emplace_back("@D(@RAFK@D)@n");
        else if (ch->timer > 3)
            states.emplace_back("@D(@RIDLE@D)@n");

        if(!states.empty()) {
            result += " ";
            result += join(states, " ");
        }

        return result;
    }


    std::string roomLine(entt::entity ent, entt::entity viewer) {
        std::string result;
        if(auto obj = reg.try_get<Object>(ent); obj) {
            if (GET_OBJ_POSTTYPE(obj) > 0) {
                result += fmt::sprintf("%s@w, has been posted here.@n", obj->getShortDesc());
            } else {
                result += fmt::sprintf("%s@n", obj->getRoomDesc());
            }
            auto proto = reg.try_get<Proto>(ent);

            if (obj->type_flag == ITEM_VEHICLE) {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && proto && proto->vn > 19199)
                    result += fmt::sprintf("\r\n@c...its outer hatch is open@n");
                else if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && proto && proto->vn <= 19199)
                    result += fmt::sprintf("\r\n@c...its door is open@n");
            }
            if (obj->type_flag == ITEM_CONTAINER && !IS_CORPSE(obj)) {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
                    result += fmt::sprintf(". @D[@G-open-@D]@n");
                else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
                    result += fmt::sprintf(". @D[@rclosed@D]@n");
            }
            if (obj->type_flag == ITEM_HATCH) {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED))
                    result += fmt::sprintf(", it is open");
                else
                    result += fmt::sprintf(", it is closed");
                if (OBJVAL_FLAGGED(obj, CONT_LOCKED))
                    result += fmt::sprintf(" and locked@n");
                else
                    result += fmt::sprintf("@n");
            }
            if (obj->type_flag == ITEM_FOOD) {
                if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
                    result += fmt::sprintf(", and it has been ate on@n");
                }
            }
        }

        return result;
    }

    std::string contentLine(entt::entity ent, entt::entity viewer) {

    }


    std::vector<std::string> fullRoomBlock(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;
        if(flags::check(ent, FlagType::Pref, PRF_ROOMFLAGS)) {
            out.push_back(listPrefix(ent, viewer));
        }
        out.push_back(roomLine(ent, viewer));
        auto res = join(out, "@n ");
        out.clear();
        out.push_back(res);
        auto status = statusLines(ent, viewer);
        out.insert(out.end(), status.begin(), status.end());
        return out;
    }

    std::vector<std::string> fullContentBlock(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;
        if(flags::check(ent, FlagType::Pref, PRF_ROOMFLAGS)) {
            out.push_back(listPrefix(ent, viewer));
        }
        out.push_back(contentLine(ent, viewer));
        auto res = join(out, "@n ");
        out.clear();
        out.push_back(res);
        auto status = statusLines(ent, viewer);
        out.insert(out.end(), status.begin(), status.end());
        return out;
    }

    std::string inventory(entt::entity ent, entt::entity viewer) {

    }

    std::string equipment(entt::entity ent, entt::entity viewer, bool showEmpty) {

    }


    static std::vector<std::string> _characterStatusLines(entt::entity ent, entt::entity viewer) {
        const auto [race, sex] = getApparentRaceSex(ent, viewer);

        auto heshe = heShe(sex);
        auto himher = himHer(sex);
        auto hisher = hisHer(sex);
        
        std::vector<std::string> messages;

        auto ch = reg.try_get<Character>(ent);

        auto icur = ch->getCurPL();
        auto imax = ch->getEffMaxPL();

        if (icur >= (imax) * .9 && icur != (imax))
            messages.emplace_back(fmt::format("@R...Some slight wounds on {} body.@w", hisher));
        else if (icur >= (imax) * .8 && icur < (imax) * .9)
            messages.emplace_back(fmt::format("@R...A few wounds on {} body.@w", hisher));
        else if (icur >= (imax) * .7 && icur < (imax) * .8)
            messages.emplace_back(fmt::format("@R...Many wounds on {} body.@w", hisher));
        else if (icur >= (imax) * .6 && icur < (imax) * .7)
            messages.emplace_back(fmt::format("@R...Quite a few wounds on {} body.@w", hisher));
        else if (icur >= (imax) * .5 && icur < (imax) * .6)
            messages.emplace_back(fmt::format("@R...Horrible wounds on {} body.@w", hisher));
        else if (icur >= (imax) * .4 && icur < (imax) * .5)
            messages.emplace_back(fmt::format("@R...Blood is seeping from the wounds on {} body.@w", hisher));
        else if (icur >= (imax) * .3 && icur < (imax) * .4)
            messages.emplace_back(fmt::format("@R...{} body is in terrible shape.@w", hisher));
        else if (icur >= (imax) * .2 && icur < (imax) * .3)
            messages.emplace_back(fmt::format("@R...{} absolutely covered in wounds.@w", heshe));
        else if (icur >= (imax) * .1 && icur < (imax) * .2)
            messages.emplace_back(fmt::format("@R...Is on {} last leg.@w", hisher));
        else if (icur < (imax) * .1)
            messages.emplace_back("@R...Should be DEAD soon.@w");


            // current actions.
        if (ch->listenroom > 0) {
            messages.emplace_back(fmt::format("@w...{} is spying on everything to the @c{}@w.", heshe, dirs[ch->eavesdir]));
        }

        auto aura = aura_types[ch->get(CharAppearance::Aura)];

        // Maybe this should go to the main line and not a status modifier?
        if (flags::check(ent, FlagType::PC, PLR_FISHING)) {
            messages.emplace_back(fmt::format("@w...{} is @Cfishing@w.@n", heshe));
        }
        if (flags::check(ent, FlagType::PC, PLR_AURALIGHT)) {
            // TODO: implement aura-based-on-form and modifiers.
            messages.emplace_back(fmt::format("...is surrounded by a bright {} aura.", aura));
        }

        auto is_oozaru = (ch->form == FormID::Oozaru || ch->form == FormID::GoldenOozaru);
        auto is_transformed = ch->form != FormID::Base;

        auto saiyan_blooded = (race == RaceID::Saiyan || race == RaceID::Halfbreed);

        if(flags::check(ent, FlagType::Affect, AFF_FLYING)) {
            if (ch->altitude == 1)
                messages.emplace_back(fmt::format("...{} is in the air!", heshe));
            if (ch->altitude == 2)
                messages.emplace_back(fmt::format("...{} is high in the air!", heshe));
        }

        if(flags::check(ent, FlagType::Affect, AFF_SANCTUARY)) {
            if (GET_SKILL(ch, SKILL_AQUA_BARRIER))
                messages.emplace_back(fmt::format("...{} has a @Gbarrier@w of @cwater@w and @Cki@w around {} body!", heshe, hisher));
            else
                messages.emplace_back(fmt::format("...{} has a barrier around {} body!", heshe, hisher));
        }
        
        auto charge = ch->charge;

        if (flags::check(ent, FlagType::Affect, AFF_FIRESHIELD))
            messages.emplace_back(fmt::format("...{} has @rf@Rl@Ya@rm@Re@Ys@w around {} body!", heshe, hisher));
        if (flags::check(ent, FlagType::PC, PLR_SPIRAL))
            messages.emplace_back(fmt::format("...{} is spinning in a vortex!", heshe));
        if (charge)
            messages.emplace_back(fmt::format("...{} has a bright {} aura around {} body!", heshe, aura, hisher));
        if (flags::check(ent, FlagType::Affect, AFF_METAMORPH))
            messages.emplace_back(fmt::format("@w...{} has a dark, @rred@w aura and menacing presence.", heshe));
        if (flags::check(ent, FlagType::Affect, AFF_HAYASA))
            messages.emplace_back(fmt::format("@w...{} has a soft @cblue@w glow around {} body!", heshe, hisher));
        if (flags::check(ent, FlagType::Affect, AFF_BLIND))
            messages.emplace_back(fmt::format("...{} is groping around blindly!", heshe));
        if (flags::check(ent, FlagType::Affect, AFF_KYODAIKA))
            messages.emplace_back(fmt::format("@w...{} has expanded {} body size@w!", heshe, hisher));

        // is this even USED?
        //if (affected_by_spell(this, SPELL_FAERIE_FIRE))
         //   messages.emplace_back(fmt::format("@m...{} @mis outlined with purple fire!", heshe));

        if (flags::check(ent, FlagType::Affect, AFF_HEALGLOW))
            messages.emplace_back(fmt::format("@w...{} has a serene @Cblue@Y glow@w around {} body.", heshe, hisher));
        if (flags::check(ent, FlagType::Affect, AFF_EARMOR))
            messages.emplace_back(fmt::format("@w...{} has ghostly @Ggreen@w ethereal armor around {} body.", heshe, hisher));

        if (ch->kaioken > 0)
            messages.emplace_back(fmt::format("@w...@r{} has a red aura around {} body!", heshe, hisher));
        if (!check::isNPC(ent) && flags::check(ent, FlagType::PC, PLR_SPIRAL))
            messages.emplace_back(fmt::format("@w...{} is spinning in a vortex!", heshe));
        if (is_transformed && race != RaceID::Android && !saiyan_blooded)
            messages.emplace_back(fmt::format("@w...{} has energy crackling around {} body!", heshe, hisher));
        if (charge && !saiyan_blooded) {
            messages.emplace_back(fmt::format("@w...{} has a bright {} aura around {} body!", heshe, aura, hisher));
        }
        if (!is_oozaru && charge && is_transformed && saiyan_blooded)
            messages.emplace_back(fmt::format("@w...{} has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around {} body!", heshe, hisher));
        if (!is_oozaru && charge && !is_transformed && saiyan_blooded) {
            messages.emplace_back(fmt::format("@w...{} has a @Ybright@w {} aura around {} body!", heshe, aura, hisher));
        }
        if (ch->form != FormID::Oozaru && !charge && is_transformed && (saiyan_blooded))
            messages.emplace_back(fmt::format("@w...{} has energy crackling around {} body!", heshe, hisher));
        if (ch->form == FormID::Oozaru && charge && saiyan_blooded)
            messages.emplace_back(fmt::format("@w...{} is in the form of a @rgreat ape@w!", heshe));
        
        if (ch->form == FormID::Oozaru && !charge && saiyan_blooded)
            messages.emplace_back(fmt::format("@w...{} has energy crackling around {} @rgreat ape@w body!", heshe, hisher));
        if (ch->feature) {
            messages.emplace_back(fmt::format("@C{}@n", ch->feature));
        }


        return messages;
    }

    std::string modifiers(entt::entity ent, entt::entity viewer) {
        std::string result;

        if(auto obj = reg.try_get<Object>(ent); obj) {
            if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
                result += fmt::sprintf(" (invisible)");
            }
            if (OBJ_FLAGGED(obj, ITEM_BLESS) && flags::check(viewer, FlagType::Affect, AFF_DETECT_ALIGN)) {
                result += fmt::sprintf(" ..It glows blue!");
            }
            if (OBJ_FLAGGED(obj, ITEM_MAGIC) && flags::check(viewer, FlagType::Affect, AFF_DETECT_MAGIC)) {
                result += fmt::sprintf(" ..It glows yellow!");
            }
            if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
                result += fmt::sprintf(" @D(@GGlowing@D)@n");
            }
            if (OBJ_FLAGGED(obj, ITEM_HOT)) {
                result += fmt::sprintf(" @D(@RHOT@D)@n");
            }
            if (OBJ_FLAGGED(obj, ITEM_HUM)) {
                result += fmt::sprintf(" @D(@RHumming@D)@n");
            }
            if (OBJ_FLAGGED(obj, ITEM_SLOT2)) {
                if (OBJ_FLAGGED(obj, ITEM_SLOT_ONE) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
                    result += fmt::sprintf(" @D[@m1/2 Tokens@D]@n");
                else if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
                    result += fmt::sprintf(" @D[@m2/2 Tokens@D]@n");
                else
                    result += fmt::sprintf(" @D[@m0/2 Tokens@D]@n");
            }
            if (OBJ_FLAGGED(obj, ITEM_SLOT1)) {
                if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
                    result += fmt::sprintf(" @D[@m1/1 Tokens@D]@n");
                else
                    result += fmt::sprintf(" @D[@m0/1 Tokens@D]@n");
            }
            if (KICHARGE(obj) > 0) {
                int num = (KIDIST(obj) * 20) + rand_number(1, 5);
                result += fmt::sprintf(" %d meters away", num);
            }
            if (OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
                result += fmt::sprintf(" @D(@YCUSTOM@D)@n");
            }
            if (OBJ_FLAGGED(obj, ITEM_RESTRING)) {
                //result += fmt::sprintf(" @D(@R%s@D)@n", GET_ADMLEVEL(ch) > 0 ? !obj->getName().empty() : "*");
            }
            if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
                if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL ||
                    GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL ||
                    GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
                    result += fmt::sprintf(", and appears to be twisted and broken.");
                } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
                    result += fmt::sprintf(", and is broken into hundreds of splinters.");
                } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
                    result += fmt::sprintf(", and is shattered on the ground.");
                } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
                    result += fmt::sprintf(", and is a pile of rubble.");
                } else {
                    result += fmt::sprintf(", and is broken.");
                }
            } else {
                if (GET_OBJ_TYPE(obj) != ITEM_BOARD) {
                    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
                        result += fmt::sprintf(".");
                    }
                    if (auto obj2 = GET_OBJ_POSTED(obj); obj2 && GET_OBJ_POSTTYPE(obj) <= 0) {
                        auto dvnum = flags::check(viewer, FlagType::Pref, PRF_ROOMFLAGS) ? fmt::sprintf("@D[@G%d@D]@w ", GET_OBJ_VNUM(obj2)) : "";
                        result += fmt::sprintf("\n...%s%s has been posted to it.", dvnum, obj2->getShortDesc());
                    }
                }
            }
        }

        return result;
    }

    std::vector<std::string> statusLines(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;

    }

    std::string inside(entt::entity ent, entt::entity viewer) {
        std::string result;

        if(auto room = reg.try_get<Room>(ent); room) {
            auto sunk = room->isSunken();
            auto nodec = flags::check(viewer, FlagType::Pref, PRF_NODEC);
            double grav = getEnvVar(ent, EnvVar::Gravity);

            if (flags::check(viewer, FlagType::Pref, PRF_ROOMFLAGS)) {
                char buf[MAX_STRING_LENGTH];
                char buf2[MAX_STRING_LENGTH];
                char buf3[MAX_STRING_LENGTH];

                sprintf(buf, "%s", join(flags::getNames(ent, FlagType::Room), " ").c_str());
                sprinttype(room->sector_type, sector_types, buf2, sizeof(buf2));
                if (!nodec) {
                    result += "\r\n@wO----------------------------------------------------------------------O@n\r\n";
                }

                result += fmt::sprintf("@wLocation: @G%-70s@w\r\n", displayName(ent, viewer));
                if (!room->script->dgScripts.empty()) {
                    result += fmt::sprintf("@D[@GTriggers");
                    for (auto t : room->script->dgScripts)
                        result += fmt::sprintf(" %d", GET_TRIG_VNUM(t));
                    result += fmt::sprintf("@D] ");
                }
                if(auto parent = reg.try_get<Location>(ent); parent) {
                    std::vector<std::string> ancestors;
                    while(parent) {
                        ancestors.emplace_back(fmt::format("{}@n {}@n", listPrefix(parent->location, viewer), displayName(parent->location, viewer)));
                        parent = reg.try_get<Location>(parent->location);
                        if(parent && parent->location == ent) break;
                    }
                    // Reverse areas.
                    std::reverse(ancestors.begin(), ancestors.end());
                    auto joined = join(ancestors, " -> ");
                    result += fmt::sprintf("@wArea: @D[@n %s @D]@n\r\n", joined.c_str());
                }
                
                auto g = fmt::format("{}", grav);
                sprintf(buf3, "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2,
                        getUID(ent), g.c_str());
                result += fmt::sprintf("@wFlags: %-70s@w\r\n", buf3);
                if (!nodec) {
                    result += fmt::sprintf("@wO----------------------------------------------------------------------O@n\r\n");
                }
            } else {
                if (!nodec) {
                    result += fmt::sprintf("@wO----------------------------------------------------------------------O@n\r\n");
                }
                result += fmt::sprintf("@wLocation: %-70s@n\r\n", displayName(ent, viewer));
                if(auto planet = find::holderType(ent, ITEM_PLANET); planet != entt::null) {
                    result += fmt::sprintf("@wPlanet: @G%s@n\r\n", render::displayName(planet, viewer));
                } else {
                    if (flags::check(ent, FlagType::Room, ROOM_NEO)) {
                        result += fmt::sprintf("@wPlanet: @WNeo Nirvana@n\r\n");
                    } else if (flags::check(ent, FlagType::Room, ROOM_AL)) {
                        result += fmt::sprintf("@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
                    } else if (flags::check(ent, FlagType::Room, ROOM_HELL)) {
                        result += fmt::sprintf("@wDimension: @RPunishment Hell@n\r\n");
                    } else if (flags::check(ent, FlagType::Room, ROOM_RHELL)) {
                        result += fmt::sprintf("@wDimension: @RH@re@Dl@Rl@n\r\n");
                    }
                }

                if(grav <= 1.0) {
                    result += fmt::sprintf("@wGravity: @WNormal@n\r\n");
                } else {
                    auto g = fmt::format("{}", grav);
                    result += fmt::sprintf("@wGravity: @W%sx@n\r\n", g.c_str());
                }
                if (flags::check(ent, FlagType::Room, ROOM_REGEN)) {
                    result += fmt::sprintf("@CA feeling of calm and relaxation fills this room.@n\r\n");
                }
                if (flags::check(ent, FlagType::Room, ROOM_AURA)) {
                    result += fmt::sprintf("@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
                }
                if (flags::check(ent, FlagType::Room, ROOM_HBTC)) {
                    result += fmt::sprintf("@rThis room feels like it opperates in a different time frame.@n\r\n");
                }
                if (!nodec) {
                    result += fmt::sprintf("@wO----------------------------------------------------------------------O@n\r\n");
                }
            }
            
            auto dmg = room->dmg;

            if ((!flags::check(viewer, FlagType::Pref, PRF_BRIEF)) || flags::check(ent, FlagType::Room, ROOM_DEATH)) {
                if (dmg <= 99) {
                    result += fmt::sprintf("@w%s@n", text::get(ent, "look_description"));
                }
                if (dmg == 100 &&
                    (room->sector_type == SECT_WATER_SWIM || sunk || room->sector_type == SECT_FLYING ||
                    room->sector_type == SECT_SHOP || room->sector_type == SECT_IMPORTANT)) {
                    result += fmt::sprintf("@w%s@n", text::get(ent, "look_description"));
                }
                if (room->sector_type == SECT_INSIDE && dmg > 0) {
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
                        (room->sector_type == SECT_CITY || room->sector_type == SECT_FIELD || room->sector_type == SECT_HILLS ||
                        room->sector_type == SECT_IMPORTANT) && dmg > 0) {
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
                } else if (room->sector_type == SECT_FOREST && dmg > 0) {
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
                } else if (room->sector_type == SECT_MOUNTAIN && dmg > 0) {
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
                if (room->geffect >= 1 && room->geffect <= 5) {
                    result += fmt::sprintf("@rLava@w is pooling in someplaces here...@n\r\n");
                }
                if (room->geffect >= 6) {
                    result += fmt::sprintf("@RLava@r covers pretty much the entire area!@n\r\n");
                }
                if (room->geffect < 0) {
                    result += fmt::sprintf("@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
                }
            }

            if(auto exstr = nodec ? room->renderExits2(viewer) : room->renderExits1(viewer); !exstr.empty()) result += exstr;

            std::vector<std::string> contentLines;
            for(auto c : contents::get(ent)) {
                if(c == viewer) continue;
                if(getFamily(c) == EntityFamily::Exit) continue;
                if(!vis::canSee(viewer, c)) continue;
                auto line = fullRoomBlock(c, viewer);
                if(!line.empty()) contentLines.insert(contentLines.end(), line.begin(), line.end());
            }
            if(!contentLines.empty()) {
                result += join(contentLines, "@n\r\n") + "@n";
            }
        }


        return result;
    }


}


namespace movement {
    std::map<int, Destination> getDestinations(entt::entity ent, entt::entity viewer) {
        return {};
    }

    std::optional<Destination> getDestination(entt::entity ent, entt::entity viewer, int direction) {
        auto destinations = getDestinations(ent, viewer);
        if(auto found = destinations.find(direction); found != destinations.end()) {
            return found->second;
        }
        return std::nullopt;
    }

    bool moveInDirection(entt::entity ent, int direction, bool need_specials_check) {
        auto ch = reg.try_get<Character>(ent);
        
        if(ch) {
            if (GRAPPLING(ch) || GRAPPLED(ch)) {
                ch->sendf("You are grappling with someone!\r\n");
                return false;
            }

            if (ABSORBING(ch) || ABSORBBY(ch)) {
                ch->sendf("You are struggling with someone!\r\n");
                return false;
            }

            if (!AFF_FLAGGED(ch, AFF_SNEAK) || (AFF_FLAGGED(ch, AFF_SNEAK) && GET_SKILL(ch, SKILL_MOVE_SILENTLY) < axion_dice(0))) {
                reveal_hiding(ch, 0);
            }
        }

        auto loc = reg.try_get<Location>(ent);
        if(!loc || loc->location == entt::null) {
            send::printf(ent, "You are nowhere!\r\n");
            return false;
        }
        auto destinations = getDestinations(loc->location, ent);
        auto found = destinations.find(direction);
        if(found == destinations.end()) {
            send::printf(ent, "Alas, you cannot go that way...\r\n");
            return false;
        }

        auto &dest = found->second;
        if(dest.via != entt::null) {
            auto &ex = dest.via;
            if (flags::check(ex, FlagType::Exit, EX_CLOSED)) {
                if(flags::check(ex, FlagType::Exit, EX_SECRET)) {
                    send::printf(ent, "Alas, you cannot go that way...\r\n");
                }
                
                if (auto alias = text::get(ex, "alias"); !alias.empty())
                    send::printf(ent, "The %s seems to be closed.\r\n", alias);
                else
                    send::printf(ent, "It seems to be closed.\r\n");
                return false;
            }
        }

        for (auto wall : contents::getInventory(loc->location)) {
            if (GET_OBJ_VNUM(wall) == 79) {
                if (GET_OBJ_COST(wall) == direction) {
                    send::printf(ent, "That direction has a glacial wall blocking it.\r\n");
                    return false;
                }
            }
        }

        if(ch && !ch->followers)
            return doSimpleMove(ent, direction, need_specials_check);

        auto was_in = reg.get<Location>(ent);

        if (!doSimpleMove(ent, direction, need_specials_check))
            return false;

        if(ch) {
            auto zanz = flags::check(ent, FlagType::Affect, AFF_ZANZOKEN);

            for (auto k = ch->followers; k; k = k->next) {
                auto follower = k->follower;
                auto folLoc = reg.get<Location>(follower->ent);
                if(folLoc != was_in) continue;
                if(GET_POS(follower) < POS_STANDING) continue;

                if ((!zanz || (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(follower, AFF_GROUP)))) {
                    act("You follow $N.\r\n", false, follower, nullptr, ch, TO_CHAR);
                    follower->moveInDirection(direction, 1);
                } else if ((zanz && AFF_FLAGGED(follower, AFF_ZANZOKEN)) && (!AFF_FLAGGED(ch, AFF_GROUP) || !AFF_FLAGGED(follower, AFF_GROUP))) {
                    act("$N tries to zanzoken and escape, but your zanzoken matches $S!\r\n", false, follower, nullptr, ch, TO_CHAR);
                    act("$N tries to zanzoken and escape, but $n's zanzoken matches $S!\r\n", false, follower, nullptr, ch, TO_NOTVICT);
                    act("You zanzoken to try and escape, but $n's zanzoken matches yours!\r\n", false, follower, nullptr, ch, TO_VICT);
                    for(auto c : {ch, follower}) c->clearFlag(FlagType::Affect,AFF_ZANZOKEN);
                    follower->moveInDirection(direction, 1);
                } else if ((zanz && !AFF_FLAGGED(k->follower, AFF_ZANZOKEN))) {
                    act("You try to follow $N, but $E disappears in a flash of movement!\r\n", false, k->follower, nullptr, ch, TO_CHAR);
                    act("$n tries to follow $N, but $E disappears in a flash of movement!\r\n", false, k->follower, nullptr, ch, TO_NOTVICT);
                    act("$n tries to follow you, but you manage to zanzoken away!\r\n", false, k->follower, nullptr, ch, TO_VICT);
                    ch->clearFlag(FlagType::Affect,AFF_ZANZOKEN);
                }
            }
        }
        
        return true;
    }

    bool doSimpleMove(entt::entity ent, int direction, bool need_specials_check) {
        char buf2[MAX_STRING_LENGTH];
        char buf3[MAX_STRING_LENGTH];
        auto was_in = reg.get<Location>(ent);
        int need_movement;

        auto destMaybe = movement::getDestination(was_in.location, ent, direction);
        if(!destMaybe) {
            send::printf(ent, "Alas, you cannot go that way.\r\n");
            return false;
        }
        auto &dest = destMaybe.value();

        auto ch = reg.try_get<Character>(ent);

        if(ch) {
            auto master = ch->master;
            if (AFF_FLAGGED(ch, AFF_CHARM) && master && was_in == reg.get<Location>(master->ent)) {
                send::printf(ent, "The thought of leaving your master makes you weep.\r\n");
                act("$n bursts into tears.", false, ch, nullptr, nullptr, TO_ROOM);
                return false;
            }
        }
        

        if(!checkCanLeave(was_in.location, ent, dest, need_specials_check)) return false;

        if(!checkCanReachDestination(dest.target, ent, dest)) return false;

        // We now know we can go into the room.

        /*
        if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_FLYING)) {
            ch->decCurST(need_movement);
        }
        */

       if(ch) {
            if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch)) {
                sprintf(buf2, "$n sneaks %s.", dirs[dest.direction]);
                if (GET_SKILL(ch, SKILL_MOVE_SILENTLY)) {
                    improve_skill(ch, SKILL_MOVE_SILENTLY, 0);
                } else if (slot_count(ch) + 1 > GET_SLOTS(ch)) {
                    ch->sendf("@RYour skill slots are full. You can not learn Move Silently.\r\n");
                    ch->clearFlag(FlagType::Affect,AFF_SNEAK);
                } else {
                    ch->sendf("@GYou learn the very basics of moving silently.@n\r\n");
                    SET_SKILL(ch, SKILL_MOVE_SILENTLY, rand_number(5, 10));
                    act(buf2, true, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
                    if (GET_DEX(ch) < rand_number(1, 30)) {
                        WAIT_STATE(ch, PULSE_1SEC);
                    }
                }
            }

            if (!AFF_FLAGGED(ch, AFF_SNEAK) && !AFF_FLAGGED(ch, AFF_FLYING)) {
                sprintf(buf2, "$n leaves %s.", dirs[dest.direction]);
                act(buf2, true, ch, nullptr, nullptr, TO_ROOM);
            }
            if (!AFF_FLAGGED(ch, AFF_SNEAK) && AFF_FLAGGED(ch, AFF_FLYING)) {
                sprintf(buf2, "$n flies %s.", dirs[dest.direction]);
                act(buf2, true, ch, nullptr, nullptr, TO_ROOM);
            }


            if (auto drag = DRAGGING(ch); drag) {
                act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, drag, TO_ROOM);
            }
            if (auto carry = CARRYING(ch); carry) {
                act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, carry, TO_ROOM);
            }
            ch->setFlag(FlagType::Affect, AFF_PURSUIT);
       }

        

        if(dest.target != was_in.location) {
            contents::removeFrom(ent);
        }
        contents::addTo(ent, dest);

        if(!checkPostEnter(dest.target, ent, was_in, dest))
            return false;

        return true;
    }

    std::vector<std::pair<std::string, Destination>> getLandingSpotsFor(entt::entity ent, entt::entity mover) {
        std::vector<std::pair<std::string, Destination>> ret;

        auto st = reg.try_get<Structure>(ent);
        if(!st) return ret;

        switch(st->type) {
            case StructureType::Rooms:
            // this one entails a recursive search through all contained Rooms and Regions.
            for(auto e : contents::get(ent)) {
                if(auto r = reg.try_get<Room>(e)) {
                    if(r->checkFlag(FlagType::Room, ROOM_LANDING)) {
                        ret.push_back({r->getName(), Destination{r}});
                    }
                } else if(auto s = reg.try_get<Structure>(e); s && s->type == StructureType::Rooms) {
                    auto res = getLandingSpotsFor(e, mover);
                    ret.insert(ret.end(), res.begin(), res.end());
                }
            }
            break;
            case StructureType::Grid3D: {
                if(auto tiles = reg.try_get<Grid3D>(ent); tiles) {
                    for(auto& [k, v] : tiles->tiles) {
                        if(v.flags.contains(ROOM_LANDING)) {
                            Destination dest;
                            dest.target = ent;
                            dest.coords = k;
                            ret.push_back({withPlaceholder(v.name, render::displayName(ent, mover)), dest});
                        }
                    }
                }
            }
            break;
            case StructureType::Space3D:
            break;
        }
        return ret;
    }

    std::optional<Destination> getLaunchDestinationFor(entt::entity ent, entt::entity mover) {
        auto loc = reg.try_get<Location>(ent);
        if(loc && loc->location != entt::null) return Destination(*loc);
        return {};
    }

    bool checkCanReachDestination(entt::entity ent, entt::entity mover, const Destination& dest) {
        if(auto ch = reg.try_get<Character>(mover); ch) {

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

    bool checkPostEnter(entt::entity ent, entt::entity mover, const Location& loc, const Destination& dest) {
        if(auto ch = reg.try_get<Character>(mover); ch) {
            entry_memory_mtrigger(ch);
        if (!greet_mtrigger(ch, dest.direction)) {
            ch->removeFromLocation();
            contents::addTo(mover, Destination(loc));
            ch->lookAtLocation();
        } else greet_memory_mtrigger(ch);

        }

        return 1;
    }

    bool checkCanLeave(entt::entity ent, entt::entity mover, const Destination& dest, bool need_specials_check) {
        auto was_in = reg.get<Location>(mover);
        char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
        auto direction = dest.direction;
        auto room = reg.try_get<Room>(ent);

        if(auto ch = reg.try_get<Character>(mover); ch) {
            if (need_specials_check && special(ch, direction + 1, throwaway))
                return false;
            if (!leave_mtrigger(ch, direction) || reg.get<Location>(ch->ent) != was_in) /* prevent teleport crashes */
                return false;
            if (room && !leave_wtrigger(room, ch, direction) || reg.get<Location>(ch->ent) != was_in) /* prevent teleport crashes */
                return false;
            if (room && !leave_otrigger(room, ch, direction) || reg.get<Location>(ch->ent) != was_in) /* prevent teleport crashes */
                return false;
            }

        return true;
    }

}