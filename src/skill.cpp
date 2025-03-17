#include "dbat/skill.h"

#include "boost/algorithm/string.hpp"

namespace skill {

    std::string getName(SkillID skill) {
        if(skill == SkillID::throw_object) return "throw";
        if(skill == SkillID::kamehameha) return "KameHameHa";
        auto ename = magic_enum::enum_name(skill);
        std::string name(ename);
        // replace underscores with spaces
        std::replace(name.begin(), name.end(), '_', ' ');
        // capitalize all words.
        std::vector<std::string> words;
        boost::split(words, name, boost::is_any_of(" "));
        for(auto& word: words) {
            word[0] = std::toupper(word[0]);
        }
        return boost::join(words, " ");
    }

    struct skill_affect_type {
        int location{};
        double modifier{0.0};
        int specific{-1};
        std::function<double(struct char_data *ch)> func{};
    };

    std::unordered_map<SkillID, std::vector<skill_affect_type>> skillAffects = {};

    double getModifier(char_data* ch, SkillID skill, int location, int specific) {
        double out = 0.0;
        if (auto found = skillAffects.find(skill); found != skillAffects.end()) {
            for (auto& affect: found->second) {
                if (affect.location == location) {if(specific != -1 && specific != affect.specific) continue;
                    out += affect.modifier;
                    if(affect.func) {
                        out += affect.func(ch);
                    }
                }
            }

        }

        return out;
    }

    double getModifiers(char_data* ch, int location, int specific) {
        double out = 0.0;
        for(auto &[id, data] : ch->skill) {
            if(data.level > 0)
                out += getModifier(ch, static_cast<SkillID>(id), location, specific);
        }
        return out;
    }

}