#include "dbat/entity.h"
#include "dbat/constants.h"
#include "dbat/races.h"

void deserializeEntity(entt::entity ent, const nlohmann::json& j) {
    if(j.contains("Flags")) {
        auto &flags = reg.get_or_emplace<Flags>(ent);
        flags.deserialize(j["Flags"]);
    }
    
}

void serializeEntity(entt::entity ent, nlohmann::json& j) {
    nlohmann::json j;

    if(auto flags = reg.try_get<Flags>(ent); flags) {
        j["Flags"] = flags.serialize();
    }

    return j;
}

void deserializeEntityRelations(entt::entity ent, const nlohmann::json& j) {

}

void serializeEntityRelations(entt::entity ent, nlohmann::json& j) {

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

    std::string set(entt::entity ent, const std::string& key, const std::string& value) {
        if(value.empty()) {
            clear(ent, key);
            return;
        }
        auto &text = reg.get_or_emplace<Text>(ent);
        text.strings[key] = internString(value);
    }

    std::string clear(entt::entity ent, const std::string& key) {
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
        return flags::check(ent, FlagType::Admin, ADMIN_INVIS);
    }

    bool canSeeAdminInvisible(entt::entity ent) {
        return flags::check(ent, FlagType::Pref, PRF_HOLYLIGHT);
    }

    bool canSeeInvisible(entt::entity ent) {
        return flags::check(ent, FlagType::Affect, AFF_DETECT_INVIS);
    }

    bool canSeeHidden(entt::entity ent) {
        return flags::check(ent, FlagType::Affect, AFF_DETECT_HIDDEN);
    }

    bool canSeeInDark(entt::entity ent) {
        if(flags::check(ent, FlagType::Pref, PRF_HOLYLIGHT)) return true;
        // todo: add a race check here...
        return flags::check(ent, FlagType::Affect, AFF_INFRAVISION);
    }

    bool canSee(entt::entity viewer, entt::entity target) {
        if(viewer == target) return true;
        if(canSeeAdminInvisible(ent)) return true;
        if(isInvisible(target) && !canSeeInvisible(viewer)) return false;
        if(isHidden(target) && !canSeeHidden(viewer)) return false;
        return true;
    }
}

namespace find {

}

namespace contents {
    std::vector<entt::entity> get(entt::entity ent) {
        std::vector<entt::entity> out;
        if(auto contents = reg.try_get<Contents>(ent); contents) {
            for(auto &item : contents->contents) {
                if(!reg.has<Deleted>(item)) out.push_back(item);
            }
        }
        return out;
    }

    std::vector<entt::entity> get(GameEntity* ent) {
        return get(ent->ent);
    }

    std::vector<GameEntity*> getContents(entt::entity ent) {
        std::vector<GameEntity> out;
        for(auto e : get(ent)) {
            auto &i = reg.get<Info>(e);
            if(i.family == EntityType::Character) {
                out.push_back(reg.try_get<Character>(e));
            } else if(i.family == EntityType::Object) {
                out.push_back(reg.try_get.get<Object>(e));
            } else if(i.family == EntityType::Room) {
                out.push_back(reg.try_get.get<Room>(e));
            } else if(i.family == EntityType::Exit) {
                out.push_back(reg.try_get.get<Exit>(e));
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
        return getRooms(ent->ent);
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

    static void updateCoordinates(entt::entity ent, entt::entity mover, std::optional<Coordinates> previous) {
        if(previous) {
            handleRemoveFromCoordinates(mover, previous.value());
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

    static void handleRemove(entt::entity ent, entt::enity mover) {
        if(auto con = reg.try_get<Contents>(ent); con) {
            std::erase_if(con->contents, [u](auto c) {return c == u;});
            if(con->contents.empty()) reg.remove<Contents>(ent);
        }
    }

    void removeFrom(entt::entity ent) {
        if(auto loc = reg.try_get<Location>(ent); !loc) {
            basic_mud_log("Attempted to remove unit '%d: %s' from location, but location was not found.", info::uid(ent), text::get(ent, "name", "nameless").c_str());
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
                basic_mud_log("Attempted to add unit '%d: %s' to location, but location was already found.", info::uid(ent), text::get(ent, "name", "nameless").c_str());
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
        return reg.has<PlayerCharacter>(ent);
    }

    bool isNPC(entt::entity ent) {
        return reg.has<NonPlayerCharacter>(ent);
    }
}


namespace render {

    std::pair<RaceID, int> getApparentRaceSex(entt::entity ent, entt::entity viewer) {
        if(flags::check(viewer, FlagType::Pref, PRF_HOLYLIGHT)) {
            if(auto phys = reg.try_get<Physiology>(ent); phys) {
                return {phys->race, phys->sex};
            }
        } else {
            if(auto mim = reg.try_get<Mimic>(ent); mim) {
                if(auto phys = reg.try_get<Physiology>(mim->mimic); phys) {
                    return {mim->race, mim->sex};
                }
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

    std::string apparentRaceName(entt::entity ent, entt::entity viewer, bool capitalized) {
        const auto [race, sex] = getApparentRaceSex(ent, viewer);

        return capitalized : race::getName(appRace) : to_lower(race::getName(appRace));
    }

    // base character keywords is race name and, optionally, sex.
    static std::vector<std::string> _characterBaseKeywords(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;
        const auto [race, sex] = getApparentRaceSex(ent, viewer);
        auto raceName = to_lower(race::getName(race));
        out.emplace_back(appRaceName);
        // male is 1, female is 2... neutral is 0.
        if(sex) {
            out.emplace_back(genders[sex]);
        }
        return out;
    }

    static std::vector<std::string> _characterKeywords(entt::entity ent, entt::entity viewer) {
        auto out = _characterBaseKeywords(ent, viewer);

        if(check::isPC(ent)) {
            auto name = split(getDisplayName(ent, viewer), ' ');
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
            case EntityType::Room:
            case EntityType::Exit: {
                    auto name = getDisplayName(ent, viewer);
                    return split(name, ' ');
                }
                break;
            case EntityType::Character: {
                return _characterKeywords(ent, viewer);
                }
                break;
            case EntityType::Object: {
                return split(getText(ent, "name", "nameless"), ' ');
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

    std::string roomLine(entt::entity ent, entt::entity viewer) {

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

    std::string displayName(entt::entity ent, entt::entity viewer) {

    }


    static std::vector<std::string> _characterStatusLines(entt::entity ent, entt::entity viewer) {
        auto &[race, sex] = getApparentRaceSex(ent, viewer);

        auto heshe = heShe(sex);
        auto himher = himHer(sex);
        auto hisher = hisHer(sex);
        
        std::vector<std::string> messages;

        auto &ch = reg.get<Character>(ent);

        auto icur = ch.getCurPL();
        auto imax = ch.getEffMaxPL();

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
        if (ch.listenroom > 0) {
            messages.emplace_back(fmt::format("@w...{} is spying on everything to the @c{}@w.", heshe, dirs[ch.eavesdir]));
        }

        auto aura = aura_types[ch.get(CharAppearance::Aura)];

        // Maybe this should go to the main line and not a status modifier?
        if (flags::check(ent, FlagType::PC, PLR_FISHING)) {
            messages.emplace_back(fmt::format("@w...{} is @Cfishing@w.@n", heshe));
        }
        if (flags::check(ent, FlagType::PC, PLR_AURALIGHT)) {
            // TODO: implement aura-based-on-form and modifiers.
            messages.emplace_back(fmt::format("...is surrounded by a bright {} aura.", aura));
        }

        auto is_oozaru = (ch.form == FormID::Oozaru || ch.form == FormID::GoldenOozaru);
        auto is_transformed = ch.form != FormID::Base;

        auto saiyan_blooded = (race == RaceID::Saiyan || race == RaceID::Halfbreed);

        if(flags::check(ent, FlagType::Affect, AFF_FLYING)) {
            if (ch.altitude == 1)
                messages.emplace_back(fmt::format("...{} is in the air!", heshe));
            if (ch.altitude == 2)
                messages.emplace_back(fmt::format("...{} is high in the air!", heshe));
        }

        if(flags::check(ent, FlagType::Affect, AFF_SANCTUARY)) {
            if (GET_SKILL(this, SKILL_AQUA_BARRIER))
                messages.emplace_back(fmt::format("...{} has a @Gbarrier@w of @cwater@w and @Cki@w around {} body!", heshe, hisher));
            else
                messages.emplace_back(fmt::format("...{} has a barrier around {} body!", heshe, hisher));
        }
        
        auto charge = ch.charge;

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

        if (ch.kaioken > 0)
            messages.emplace_back(fmt::format("@w...@r{} has a red aura around {} body!", heshe, hisher));
        if (!isNPC(ent) && flags::check(ent, FlagType::PC, PLR_SPIRAL))
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
        if (form != FormID::Oozaru && !charge && is_transformed && (saiyan_blooded))
            messages.emplace_back(fmt::format("@w...{} has energy crackling around {} body!", heshe, hisher));
        if (form == FormID::Oozaru && charge && saiyan_blooded)
            messages.emplace_back(fmt::format("@w...{} is in the form of a @rgreat ape@w!", heshe));
        
        if (form == FormID::Oozaru && !charge && saiyan_blooded)
            messages.emplace_back(fmt::format("@w...{} has energy crackling around {} @rgreat ape@w body!", heshe, hisher));
        if (ch.feature) {
            messages.emplace_back(fmt::format("@C{}@n", ch.feature));
        }


        return messages;
    }


    std::vector<std::string> statusLines(entt::entity ent, entt::entity viewer) {
        std::vector<std::string> out;

    }


}