#include "dbat/game/HasInteractive.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/Structure.hpp"
#include "dbat/game/players.hpp"

bool Character::isVisibleTo(Character* viewer) {
    if(viewer == this) return true; // you can always see yourself.

    auto viewerAdminLevel = viewer->getBaseStat<int>("admin_level");
    auto myInvisLevel = getBaseStat<int>("invis_level");

    if(myInvisLevel > viewerAdminLevel) return false;

    if(viewer->player_flags.get(PRF_HOLYLIGHT)) return true;

    if(affect_flags.get(AFF_HIDE) && viewerAdminLevel <= 0) return false;
    if(affect_flags.get(AFF_INVISIBLE) && !viewer->affect_flags.get(AFF_DETECT_INVIS)) return false;

    return true;
}

bool Object::isVisibleTo(Character *viewer) {
    if(item_flags.get(ITEM_INVISIBLE) && !viewer->affect_flags.get(AFF_DETECT_INVIS))
        return false;

    if(auto wornby = getWornBy()) {
        // You can always see your own equipment.
        if(wornby == viewer) return true;
        // but if someone else is wearing it then you also need to be able to see them!
        else return wornby->isVisibleTo(viewer);
    }

    if(auto carriedby = getCarriedBy()) {
        if(carriedby == viewer) return true;
        else return carriedby->isVisibleTo(viewer);
    }

    if(auto containedby = getContainer()) {
        return containedby->isVisibleTo(viewer);
    }

    return true;
}

bool Structure::isVisibleTo(Character *viewer) {
    return true;
}

std::string Character::getDisplayName(struct Character* viewer, bool capitalizeArticle) {
    if(!viewer->canSee(this)) {
        return "Someone";
    }

    if(player) {
        if(IS_NPC(viewer) || getBaseStat<int>("admin_level") > 0) {
            // NPCs just don't use the dub system at all, and admin characters
            // reveal their names to everyone who can see them.
            return name;
        } else {
            if(viewer->pref_flags.get(PRF_HOLYLIGHT)) {
                // admins see real names.
                return name;
            } else {
                if(!player_flags.get(PLR_DISGUISED)) {
                    // players see dub names if they have them.
                    auto &p = players.at(viewer->id);
                    if(auto find = p->dub_names.find(id); find != p->dub_names.end()) {
                        return find->second;
                    }
                }
            }
            // But we don't have a dub name. We'll have to fall through to apparent sex and race...
            std::string displayName = capitalizeArticle ? "A " : "a ";

            auto appRace = getApparentRace(viewer);
            auto race = race::getName(appRace);
            auto appSex = getApparentSex(viewer);
            std::string sex = "questionably-gendered";
            switch(appSex) {
                case Sex::male:
                    sex = "male";
                    break;
                case Sex::female:
                    sex = "female";
                    break;
                default:
                    break;
            }

            return fmt::format("{} {} {}", capitalizeArticle ? "A" : "a", sex, race);
        }
    } else {
        std::string shortDesc = getShortDescription();
        if(boost::istarts_with(shortDesc, "a ") || boost::istarts_with(shortDesc, "an ") || boost::istarts_with(shortDesc, "the ")) {
            if(capitalizeArticle) {
                shortDesc[0] = toupper(shortDesc[0]);
            } else {
                shortDesc[0] = tolower(shortDesc[0]);
            }
            return shortDesc;
        }
        return shortDesc;
    }
}

std::vector<std::string> Character::getInteractivityKeywords(struct Character* viewer) {
    std::vector<std::string> keywords;
    if(viewer == this) {
        keywords.push_back("self");
        keywords.push_back("me");
    }

    auto appRace = getApparentRace(viewer);
    keywords.push_back(race::getName(appRace));

    auto appSex = getApparentSex(viewer);
    switch(appSex) {
        case Sex::male:
            keywords.push_back("male");
            break;
        case Sex::female:
            keywords.push_back("female");
            break;
        default:
            break;
    }

    if(player) {
        if(IS_NPC(viewer) || getBaseStat<int>("admin_level") > 0) {
            // NPCs just don't use the dub system at all, and admin characters
            // reveal their names to everyone who can see them.
            keywords.push_back(name);
        } else {
            if(viewer->pref_flags.get(PRF_HOLYLIGHT)) {
                // admins see real names.
                keywords.push_back(name);
            } else {
                // players MIGHT have a dub name...
                // But if not, they will have to settle for the race added earlier.
                auto &p = players.at(viewer->id);
                if(auto find = p->dub_names.find(id); find != p->dub_names.end()) {
                    keywords.push_back(find->second);
                }
            }
        }
    } else {
        // this is actually really simple...
        std::vector<std::string> toks;
        boost::split(toks, name, boost::is_space(), boost::token_compress_on);
        for(const auto& t : toks) {
            if(!t.empty())
                keywords.emplace_back(std::move(t));
        }
    }
    return keywords;
}



std::vector<std::string> Object::getInteractivityKeywords(Character* viewer) {
    std::vector<std::string> keywords;
    boost::split(keywords, name, boost::is_space(), boost::token_compress_on);
    return keywords;
}

std::string Object::getDisplayName(Character* viewer, bool capitalizeArticle) {
    std::string shortDesc = getShortDescription();
    if(boost::istarts_with(shortDesc, "a ") || boost::istarts_with(shortDesc, "an ") || boost::istarts_with(shortDesc, "the ")) {
        if(capitalizeArticle) {
            shortDesc[0] = toupper(shortDesc[0]);
        } else {
            shortDesc[0] = tolower(shortDesc[0]);
        }
        return shortDesc;
    }
    return shortDesc;
}

std::vector<std::string> Structure::getInteractivityKeywords(Character* viewer) {
    std::vector<std::string> keywords;
    boost::split(keywords, name, boost::is_space(), boost::token_compress_on);
    return keywords;
}

std::string Structure::getDisplayName(Character* viewer, bool capitalizeArticle) {
    std::string shortDesc = getShortDescription();
    if(boost::istarts_with(shortDesc, "a ") || boost::istarts_with(shortDesc, "an ") || boost::istarts_with(shortDesc, "the ")) {
        if(capitalizeArticle) {
            shortDesc[0] = toupper(shortDesc[0]);
        } else {
            shortDesc[0] = tolower(shortDesc[0]);
        }
        return shortDesc;
    }
    return shortDesc;
}
