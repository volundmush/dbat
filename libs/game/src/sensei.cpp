#include "dbat/game/sensei.h"
#include "dbat/db/characters.h"

#include <utility>
#include <string>
#include <algorithm>

namespace dbat::sensei {

    Sensei::Sensei(sensei_id sid, const std::string &name, std::string abbr, std::string style) {
        this->s_id=sid;
        this->abbr = std::move(abbr);
        this->name = name;
        this->lower_name = name;
        this->style = std::move(style);
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    }

    sensei_id Sensei::getID() const {
        return s_id;
    }

    const std::string& Sensei::getAbbr() const {
        return abbr;
    }

    const std::string& Sensei::getName() const {
        return name;
    }

    const std::string& Sensei::getNameLower() const {
        return lower_name;
    }

    const std::string& Sensei::getStyleName() const {
        return style;
    }

    bool Sensei::senseiAvailableForRace(race::race_id r_id) const {
        switch(s_id) {
            case sixteen:
                return r_id == race::android;
            case dabura:
                return r_id == race::demon;
            case tsuna:
                return r_id == race::kanassan;
            case kurzak:
                return r_id == race::arlian;
            case jinto:
                return r_id == race::hoshijin;
            default:
                return r_id != race::android;
        }
    }

    IDXTYPE Sensei::senseiLocationID() const {
        switch(s_id) {
            case roshi:
                return 1131;
            case kibito:
                return 12098;
            case nail:
                return 11683;
            case bardock:
                return 2267;
            case krane:
                return 13012;
            case tapion:
                return 8233;
            case piccolo:
                return 1662;
            case sixteen:
                return 1714;
            case dabura:
                return 6487;
            case frieza:
                return 4283;
            case ginyu:
                return 4290;
            case jinto:
                return 3499;
            case kurzak:
                return 16100;
            case tsuna:
                return 15009;
            case commoner:
                return 300;
        }
    }

    IDXTYPE Sensei::senseiStartRoom() const {
        switch(s_id) {
            case roshi:
                return 1130;
            case kibito:
                return 12098;
            case nail:
                return 11683;
            case bardock:
                return 2268;
            case krane:
                return 13009;
            case tapion:
                return 8231;
            case piccolo:
                return 1659;
            case sixteen:
                return 1713;
            case dabura:
                return 6486;
            case frieza:
                return 4282;
            case ginyu:
                return 4289;
            case jinto:
                return 3499;
            case kurzak:
                return 16100;
            case tsuna:
                return 15009;
            case commoner:
                return 300;
        }
    }

    int Sensei::getGravTolerance() const {
        switch(s_id) {
            case bardock:
                return 10;
            default:
                return 0;
        }
    }

    bool Sensei::senseiIsPcOk() const {
        switch(s_id) {
            case commoner:
                return false;
            default:
                return true;
        }
    }

    int Sensei::getRPPCost(race::race_id rid) const {
        switch(s_id) {
            case kibito:
                if(rid != race::kai) {
                    return 10;
                } else {
                    return 0;
                }
            default:
                return 0;
        }
    }

    SenseiMap sensei_map;

    void load_sensei() {
        sensei_map[roshi] = new Sensei(roshi, "Roshi", "Ro", "Kame Arts");
        sensei_map[piccolo] = new Sensei(piccolo, "Piccolo", "Pi", "Demon Taijutsu");
        sensei_map[krane] = new Sensei(krane, "Krane", "Kr", "Crane Arts");
        sensei_map[nail] = new Sensei(nail, "Nail", "Na", "Tranquil Palm");
        sensei_map[bardock] = new Sensei(bardock, "Bardock", "Ba", "Brutal Beast");
        sensei_map[ginyu] = new Sensei(ginyu, "Ginyu", "Gi", "Flaunted Style");
        sensei_map[frieza] = new Sensei(frieza, "Frieza", "Fr", "Frozen Fist");
        sensei_map[tapion] = new Sensei(tapion, "Tapion", "Ta", "Shadow Grappling");
        sensei_map[sixteen] = new Sensei(sixteen, "Android 16", "16", "Iron Hand");
        sensei_map[dabura] = new Sensei(dabura, "Dabura", "Da", "Devil Dance");
        sensei_map[kibito] = new Sensei(kibito, "Kibito", "Ki", "Gentle Fist");
        sensei_map[jinto] = new Sensei(jinto, "Jinto", "Ji", "Star's Radiance");
        sensei_map[tsuna] = new Sensei(tsuna, "Tsuna", "Ts", "Sacred Tsunami");
        sensei_map[kurzak] = new Sensei(kurzak, "Kurzak", "Kurzak", "Adaptive Taijutsu");

        sensei_map[commoner] = new Sensei(commoner, "Commoner", "--", "Like a Bum");
    }

    Sensei* find_sensei(const std::string& arg) {
        return find_sensei_map(arg, sensei_map);
    }

    Sensei* find_sensei_map(const std::string& arg, const SenseiMap& s_map) {
        std::string lower(arg);
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for(const auto& s : s_map) {
            if(s.second->getNameLower() == lower) {
                return s.second;
            }
        }
        return nullptr;
    }

    Sensei* find_sensei_map_id(const int id, const SenseiMap& s_map) {
        for(const auto& s : s_map) {
            if(s.first == id) {
                return s.second;
            }
        }
        return nullptr;
    }

    SenseiMap valid_for_race_pc(char_data *ch) {
        SenseiMap out;
        for(const auto& s : sensei_map) {
            if(s.second->senseiIsPcOk() && s.second->senseiAvailableForRace(get_race(ch->race)->getID())) {
                out[s.first] = s.second;
            }
        }
        return out;
    }
}
