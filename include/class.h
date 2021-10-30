//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_CLASS_H
#define CIRCLE_CLASS_H

#include "structs.h"
#include "races.h"

// global variables

extern const char *config_sect[NUM_CONFIG_SECTIONS+1];
extern const int class_hit_die_size[NUM_CLASSES];

// functions
void do_start(struct char_data *ch);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(struct char_data *ch, int level);
int load_levels();
void cedit_creation(struct char_data *ch);
void advance_level(struct char_data *ch, int whichclass);
int8_t ability_mod_value(int abil);
int8_t dex_mod_capped(const struct char_data *ch);
char *class_desc_str(struct char_data *ch, int howlong, int wantthe);
int total_skill_levels(struct char_data *ch, int skill);
int highest_skill_value(int level, int type);
int calc_penalty_exp(struct char_data *ch, int gain);
time_t birth_age(struct char_data *ch);
time_t max_age(struct char_data *ch);

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
        commoner = 255
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
        int getRPPCost(race::race_id rid) const;
        bool senseiAvailableForRace(race::race_id r_id) const;
        int getGravTolerance() const;
        bool senseiIsPcOk() const;
        IDXTYPE senseiStartRoom() const;

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

#endif //CIRCLE_CLASS_H
