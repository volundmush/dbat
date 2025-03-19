#include <utility>

#include <boost/algorithm/string.hpp>

#include "dbat/races.h"
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/weather.h"

const char *race_abbrevs[NUM_RACES + 1] = {
        "Hum",
        "Sai",
        "Ice",
        "Kon",
        "Nam",
        "Mut",
        "Kan",
        "H-B",
        "Bio",
        "And",
        "Dem",
        "Maj",
        "Kai",
        "Tru",
        "Hos",
        "Ict",
        "Sab",
        "Ser",
        "Trl",
        "Dra",
        "Arl",
        "Mnd",
        "Mec",
        "Spi",
        "\n"
};

#define Y   true
#define N   false


const char *race_display[NUM_RACES] = {
        "@B1@W) @cHuman\r\n",
        "@B2@W) @cSaiyan\r\n",
        "@B3@W) @cIcer\r\n",
        "@B4@W) @cKonatsu\r\n",
        "@B5@W) @cNamekian\r\n",
        "@B6@W) @cMutant\r\n",
        "@B7@W) @cKanassan\r\n",
        "@B8@W) @cHalf Breed\r\n",
        "@B9@W) @cBio-Android\r\n",
        "@B10@W) @cAndroid\r\n",
        "@B11@W) @cDemon\r\n",
        "@B12@W) @cMajin\r\n",
        "@B13@W) @cKai\r\n",
        "@B14@W) @cTuffle\r\n",
        "@B15@W) @cHoshijin\r\n",
        "@B16@W) @YArlian\r\n",
        "@B17@W) @GAnimal\r\n",
        "@B18@W) @MSaiba\r\n",
        "@B19@W) @BSerpent\r\n",
        "@B20@W) @ROgre\r\n",
        "@B21@W) @CYardratian\r\n",
        "@B22@W) @GLizardfolk\r\n",
        "@B23@W) @GMechanical\r\n",
        "@B24@W) @MSpirit\r\n",
};


/* Converted into metric units: cm and kg; SRD has english units. */
struct {
    int height[NUM_SEX];    /* cm */
    int heightdie;    /* 2d(heightdie) added to height */
    int weight[NUM_SEX];    /* kg */
    int weightfac;    /* added height * weightfac/100 added to weight */
} hw_info[NUM_RACES] = {
/* RACE_HUMAN      */ {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SAIYAN     */
                      {{140, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ICER       */
                      {{100, 111, 95},  10, {17, 18, 16}, 18},
/* RACE_KONATSU    */
                      {{121, 124, 109}, 20, {52, 59, 45}, 125},
/* RACE_NAMEK      */
                      {{137, 140, 135}, 20, {40, 45, 36}, 89},
/* RACE_MUTANT     */
                      {{141, 150, 140}, 10, {46, 54, 39}, 89},
/* RACE_KANASSAN   */
                      {{135, 135, 135}, 15, {37, 39, 36}, 63},
/* RACE_HALFBREED  */
                      {{141, 147, 135}, 30, {59, 68, 50}, 125},
/* RACE_BIO        */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ANDROID    */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_DEMON      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MAJIN      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KAI        */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TRUFFLE    */
                      {{40,  50,  45},  16, {16, 24, 9},  8},
/* RACE_GOBLIN     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ANIMAL     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ORC        */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SNAKE      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TROLL      */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MINOTAUR   */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KOBOLD     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_LIZARDFOLK */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_WARHOST    */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_FAERIE     */
                      {{141, 147, 135}, 26, {46, 54, 39}, 89}
};


void set_height_and_weight_by_race(struct char_data *ch) {
    int race, sex, mod;

    if (!IS_NPC(ch)) {
        return;
    }

    race = (int)GET_RACE(ch);
    sex = GET_SEX(ch);
    if (sex < SEX_NEUTRAL || sex >= NUM_SEX) {
        basic_mud_log("Invalid gender in set_height_and_weight_by_race: %d", sex);
        sex = SEX_NEUTRAL;
    }

    mod = dice(2, hw_info[race].heightdie);
    ch->setHeight( hw_info[race].height[sex] + mod);
    mod *= hw_info[race].weightfac;
    mod /= 100;
    ch->set(CharDim::weight, hw_info[race].weight[sex] + mod);
}


int invalid_race(struct char_data *ch, struct obj_data *obj) {
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
        return false;

    if(!obj->only_race.empty() && !obj->only_race.contains(ch->race))
        return true;

    if(obj->not_race.contains(ch->race))
        return true;

    return false;
}


int get_size(struct char_data *ch) {
    return ch->getSize();
}


const int size_bonus_table[NUM_SIZES] = {
/* XTINY */    8,
/* TINY */    4,
/* XSMALL */    2,
/* SMALL */    1,
/* MEDIUM */    0,
/* LARGE */    -1,
/* HUGE */    -2,
/* GIGANTIC */    -4,
/* COLOSSAL */    -8
};


int get_size_bonus(int sz) {
    if (sz < 0 || sz >= NUM_SIZES)
        sz = SIZE_MEDIUM;
    return size_bonus_table[sz];
}


int wield_type(int chsize, struct obj_data *weap) {
    if (GET_OBJ_TYPE(weap) != ITEM_WEAPON) {
        return OBJ_FLAGGED(weap, ITEM_2H) ? WIELD_TWOHAND : WIELD_ONEHAND;
    } else if (chsize > GET_OBJ_SIZE(weap)) {
        return WIELD_LIGHT;
    } else if (chsize == GET_OBJ_SIZE(weap)) {
        return WIELD_ONEHAND;
    } else if (chsize == GET_OBJ_SIZE(weap) - 1) {
        return WIELD_TWOHAND;
    } else if (chsize < GET_OBJ_SIZE(weap) - 1) {
        return WIELD_NONE; /* It's just too big for you! */
    } else {
        basic_mud_log("unknown size vector in wield_type: chsize=%d, weapsize=%d", chsize, GET_OBJ_SIZE(weap));
        return WIELD_NONE;
    }
}

int race_bodyparts[NUM_RACES][NUM_WEARS] = {
        /* 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22 */
        /* U, F, F, N, N, B, H, L, F, H, A, U, A, W, W, W, W, W, B, E, E, W, M */
/* RACE_HUMAN       */ {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_SAIYAN      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_ICER        */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_KONATSU     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_NAMEK       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_MUTANT      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_KANASSAN    */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_HALFBREED   */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_BIO         */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_ANDROID     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_DEMON       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_MAJIN       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_KAI         */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_TRUFFLE     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_HOSHIJIN    */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_ANIMAL      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_ORC         */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_SNAKE       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_TROLL       */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_MINOTAUR    */
                       {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_ARLIAN      */
                       {0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_LIZARDFOLK  */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
/* RACE_WARHOST     */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
/* RACE_FAERIE      */
                       {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

void racial_body_parts(struct char_data *ch) {

    auto parts = race_bodyparts[(int)GET_RACE(ch)];

    for (auto i = 1; i < NUM_WEARS; i++) {
        ch->bodyparts.set(i, parts[i]);
    }
}

namespace race {

    static std::unordered_map<RaceID, int> race_sizes = {
        {RaceID::tuffle, SIZE_SMALL},
        {RaceID::animal, SIZE_FINE},
        {RaceID::saiba, SIZE_LARGE},
        {RaceID::ogre, SIZE_LARGE},
        {RaceID::spirit, SIZE_TINY}
    };

    int getSize(RaceID id) {
        if(const auto found = race_sizes.find(id); found != race_sizes.end()) return found->second;
        return SIZE_MEDIUM;
    }

    static std::unordered_set<RaceID> playable = {RaceID::human, RaceID::saiyan, RaceID::icer, RaceID::konatsu, RaceID::namekian,
    RaceID::mutant, RaceID::kanassan, RaceID::halfbreed, RaceID::bio_android, RaceID::android, RaceID::demon, RaceID::majin,
    RaceID::kai, RaceID::tuffle};

    bool isPlayable(RaceID id) {
        return playable.contains(id);
    }

    static std::vector<RaceID> all_races = {
        RaceID::human, RaceID::saiyan, RaceID::icer, RaceID::konatsu, RaceID::namekian, RaceID::mutant,
        RaceID::kanassan, RaceID::halfbreed, RaceID::bio_android, RaceID::android, RaceID::demon, RaceID::majin,
        RaceID::kai, RaceID::tuffle, RaceID::hoshijin, RaceID::animal, RaceID::saiba, RaceID::serpent, RaceID::ogre,
        RaceID::yardratian, RaceID::arlian, RaceID::dragon, RaceID::mechanical, RaceID::spirit
    };

    bool exists(RaceID id) {
        auto find = std::find(all_races.begin(), all_races.end(), id);
        return find != all_races.end();
    }


    std::unordered_set<int> getValidSexes(RaceID id) {
        switch(id) {
            case RaceID::namekian:
                return {SEX_NEUTRAL};
            default:
                return {SEX_NEUTRAL, SEX_MALE, SEX_FEMALE};
        }
    }

    static std::map<RaceID, std::string> race_names = {
        {RaceID::human, "Human"},
        {RaceID::saiyan, "Saiyan"},
        {RaceID::icer, "Icer"},
        {RaceID::konatsu, "Konatsu"},
        {RaceID::namekian, "Namekian"},
        {RaceID::mutant, "Mutant"},
        {RaceID::kanassan, "Kanassan"},
        {RaceID::halfbreed, "Halfbreed"},
        {RaceID::bio_android, "BioAndroid"},
        {RaceID::android, "Android"},
        {RaceID::demon, "Demon"},
        {RaceID::majin, "Majin"},
        {RaceID::kai, "Kai"},
        {RaceID::tuffle, "Tuffle"},
        {RaceID::hoshijin, "Hoshijin"},
        {RaceID::animal, "Animal"},
        {RaceID::saiba, "Saiba"},
        {RaceID::serpent, "Serpent"},
        {RaceID::ogre, "Ogre"},
        {RaceID::yardratian, "Yardratian"},
        {RaceID::arlian, "Arlian"},
        {RaceID::dragon, "Dragon"},
        {RaceID::mechanical, "Mechanical"},
        {RaceID::spirit, "Spirit"}
    };

    std::string getName(RaceID id) {
        if(const auto found = race_names.find(id); found != race_names.end()) return found->second;
        return "Unknown";
    }

    static std::map<RaceID, std::string> race_abbr = {
        {RaceID::human, "Hum"},
        {RaceID::saiyan, "Sai"},
        {RaceID::icer, "Ice"},
        {RaceID::konatsu, "kon"},
        {RaceID::namekian, "Nam"},
        {RaceID::mutant, "Mut"},
        {RaceID::kanassan, "Kan"},
        {RaceID::halfbreed, "H-B"},
        {RaceID::bio_android, "Bio"},
        {RaceID::android, "And"},
        {RaceID::demon, "Dem"},
        {RaceID::majin, "Maj"},
        {RaceID::kai, "Kai"},
        {RaceID::tuffle, "Tru"},
        {RaceID::hoshijin, "Hos"},
        {RaceID::animal, "Ict"},
        {RaceID::saiba, "Sab"},
        {RaceID::serpent, "Ser"},
        {RaceID::ogre, "Ogr"},
        {RaceID::yardratian, "Yar"},
        {RaceID::arlian, "Arl"},
        {RaceID::dragon, "Drg"},
        {RaceID::mechanical, "Mec"},
        {RaceID::spirit, "Spi"}
    };

    std::string getAbbr(RaceID id) {
        if(const auto found = race_abbr.find(id); found != race_abbr.end()) return found->second;
        return "N/A";
    }

    bool isPeople(RaceID id) {
        switch (id) {
            case RaceID::animal:
            case RaceID::saiba:
            case RaceID::mechanical:
            case RaceID::spirit:
                return false;
            default:
                return true;
        }
    }

    bool hasTail(RaceID id) {
        switch (id) {
            case RaceID::icer:
            case RaceID::bio_android:
            case RaceID::saiyan:
            case RaceID::halfbreed:
                return true;
            default:
                return false;
        }
    }

    bool isValidMimic(RaceID id) {
        switch (id) {
            case RaceID::icer:
            case RaceID::namekian:
            case RaceID::demon:
            case RaceID::majin:
            case RaceID::hoshijin:
            case RaceID::arlian:
                return false;
            default:
                return isPlayable(id);
        }
    }

    int64_t getSoftCap(RaceID id, int level) {
        switch(id) {
            case RaceID::kanassan:
            case RaceID::demon:
                return 500 * pow(1.1390, level);
            default:
                return 500 * pow(1.1357, level);
        }
    }

    bool isSenseable(RaceID id) {
        return id != RaceID::android;
    }

    static std::unordered_map<RaceID, std::vector<character_affect_type>> race_affects = {
            {RaceID::human, {
                                    {APPLY_CVIT_REGEN_MULT, -0.5, ~0},
            }},
            {RaceID::namekian, {
                                    {APPLY_CVIT_REGEN_MULT, 0.5, ~0},
                            }},
            {RaceID::mutant, {
                                       {APPLY_CVIT_REGEN_MULT,  -0.1, ~0},
                               }},
            {RaceID::arlian, {
                                     {APPLY_CVIT_REGEN_MULT,  -0.7, ~0, [](auto ch) {return (IS_FEMALE(ch) && OUTSIDE(ch)) ? 4.0 : 0.0;}},
                             }},
            {RaceID::kanassan, {
                                     {APPLY_CVIT_REGEN_MULT,  0.0, ~0, [](auto ch) {
                                         double out = 0.0;
                                         if(weather_info.sky == SKY_RAINING && OUTSIDE(ch)) out += 0.1;
                                         if(ch->getLocationEnvironment(ENV_WATER) >= 100.0) out += 16.0;
                                         return out;
                                     }},
                             }},
            {RaceID::android, {
                                     {APPLY_CVIT_REGEN_MULT,  0.0, ~0, [](auto ch) {return ch->character_flags.get(CharacterFlag::android_model_absorb) ? -0.66 : 0.0;}},
                             }},
            {RaceID::saiyan, {
                                     {APPLY_CSTAT_GAIN_MULT, 0.3,  static_cast<int>(CharStat::experience)},
                                     //{APPLY_PHYS_DAM_PERC, 0.0, 0, [](struct char_data *ch) {return PLR_FLAGGED(ch, PLR_TAIL) ? 0.15 : 0;}},
                                     //{APPLY_DAM_ATK_TIER, 0.2, 3},
                                     //{APPLY_SKILL_SLOTS, -1},
                                     //{APPLY_TRANS_ST_UPKEEP, 0.0, 0, [](struct char_data *ch) {return ch->getCurLFPercent() > 0.7 ? -0.25 : 0.0}}
                             }},

            {RaceID::halfbreed, {
                                     {APPLY_CSTAT_GAIN_MULT, 0.2,  static_cast<int>(CharStat::experience)},
                                     //{APPLY_SKILL_SLOTS, 1},
                                     //{APPLY_ATTR_TRAIN_COST, -0.25, (int)CharTrain::Intelligence},
                                     //{APPLY_ATTR_TRAIN_COST, -0.25, (int)CharTrain::Strength},
                                     //{APPLY_PS_GAIN_MULT, -0.4}
                             }},

            {RaceID::icer, {
                                     {APPLY_CSTAT_GAIN_MULT, -0.1, static_cast<int>(CharStat::experience)},

                             }},

            {RaceID::kai, {
                                     {APPLY_CSTAT_GAIN_MULT, -0.1, static_cast<int>(CharStat::experience)},

                             }}
    };

    double getModifier(char_data *ch, int location, int specific) {
        double out = 0.0;
        if(auto found = race_affects.find(ch->race); found != race_affects.end()) {

            for(auto& affect : found->second) {
                if(affect.match(location, specific)) {
                    out += affect.modifier;
                    if(affect.func) {
                        out += affect.func(ch);
                    }
                }
            }
        }

        return out;
    }

    std::vector<RaceID> filterRaces(std::function<bool(RaceID)> func) {
        std::vector<RaceID> out;
        std::copy_if(all_races.begin(), all_races.end(), std::back_inserter(out), func);

        return out;
    };

    std::optional<RaceID> findRace(const std::string& arg, std::function<bool(RaceID)> func) {
        for(auto r : all_races) {
            if(!func(r)) continue;
            if(boost::iequals(arg, getName(r))) return r;
        }
        return {};

    }

}
