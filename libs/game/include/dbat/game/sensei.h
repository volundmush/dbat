#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/consts/senseis.h"
#include "dbat/game/races_plus.h"

#include <map>
#include <string>
// global variables

namespace dbat::sensei {
    enum sensei_id : uint8_t {
        roshi = 0,
        piccolo = 1,
        krane = 2,
        nail = 3,
        bardock = 4,
        ginyu = 5,
        frieza = 6,
        tapion = 7,
        sixteen = 8,
        dabura = 9,
        kibito = 10,
        jinto = 11,
        tsuna = 12,
        kurzak = 13,
        commoner = 28
    };

    class Sensei;
    typedef std::map<sensei_id, Sensei*> SenseiMap;
    extern SenseiMap sensei_map;


    class Sensei {
    public:
        Sensei(sensei_id sid, const std::string &name, std::string abbr, std::string style);
        sensei_id getID() const;
        const std::string &getName() const;
        const std::string &getNameLower() const;
        const std::string &getAbbr() const;
        const std::string &getStyleName() const;

        // stats stuff
        int getRPPCost(dbat::race::race_id rid) const;
        bool senseiAvailableForRace(dbat::race::race_id r_id) const;
        int getGravTolerance() const;
        bool senseiIsPcOk() const;
        IDXTYPE senseiStartRoom() const;
        IDXTYPE senseiLocationID() const;

    protected:
        sensei_id s_id;
        std::string name, lower_name, abbr, style;
    };

    void load_sensei();

    Sensei* find_sensei(const std::string& arg);
    Sensei* find_sensei_map(const std::string& arg, const SenseiMap& s_map);
    Sensei* find_sensei_map_id(const int id, const SenseiMap& s_map);
    SenseiMap valid_for_race_pc(char_data *ch);
}