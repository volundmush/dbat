#include "dbat/game/races.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"


#include "dbat/game/utils.hpp"
#include "dbat/game/interpreter.hpp"
//#include "dbat/game/spells.hpp"
//#include "dbat/game/comm.hpp"
#include "dbat/game/class.hpp"
//#include "dbat/game/fight.hpp"
#include "dbat/game/weather.hpp"

#include "dbat/game/const/AdminLevel.hpp"
#include "dbat/game/const/Wield.hpp"
#include "dbat/game/const/Environment.hpp"

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
    "\n"};

#define Y true
#define N false

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
struct
{
    int height[NUM_SEX]; /* cm */
    int heightdie;       /* 2d(heightdie) added to height */
    int weight[NUM_SEX]; /* kg */
    int weightfac;       /* added height * weightfac/100 added to weight */
} hw_info[NUM_RACES] = {
    /* RACE_HUMAN      */ {{141, 147, 135}, 26, {46, 54, 39}, 89},
    /* RACE_SAIYAN     */
    {{140, 147, 135}, 26, {46, 54, 39}, 89},
    /* RACE_ICER       */
    {{100, 111, 95}, 10, {17, 18, 16}, 18},
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
    {{40, 50, 45}, 16, {16, 24, 9}, 8},
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
    {{141, 147, 135}, 26, {46, 54, 39}, 89}};

void set_height_and_weight_by_race(Character *ch)
{
    if (!IS_NPC(ch))
    {
        return;
    }

    auto race = (int)GET_RACE(ch);
    auto sex = static_cast<int>(GET_SEX(ch));

    auto mod = dice(2, hw_info[race].heightdie);
    ch->setBaseStat("height", hw_info[race].height[sex] + mod);
    mod *= hw_info[race].weightfac;
    mod /= 100;
    ch->setBaseStat("weight", hw_info[race].weight[sex] + mod);
}

int invalid_race(Character *ch, Object *obj)
{
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
        return false;

    if (obj->only_race.count() && !obj->only_race.get(ch->race))
        return true;

    if (obj->not_race.get(ch->race))
        return true;

    return false;
}

int get_size(Character *ch)
{
    return ch->getSize();
}

const int size_bonus_table[NUM_SIZES] = {
    /* XTINY */ 8,
    /* TINY */ 4,
    /* XSMALL */ 2,
    /* SMALL */ 1,
    /* MEDIUM */ 0,
    /* LARGE */ -1,
    /* HUGE */ -2,
    /* GIGANTIC */ -4,
    /* COLOSSAL */ -8};

int get_size_bonus(int sz)
{
    if (sz < 0 || sz >= NUM_SIZES)
        sz = SIZE_MEDIUM;
    return size_bonus_table[sz];
}

int wield_type(int chsize, Object *weap)
{
    if (GET_OBJ_TYPE(weap) != ITEM_WEAPON)
    {
        return OBJ_FLAGGED(weap, ITEM_2H) ? WIELD_TWOHAND : WIELD_ONEHAND;
    }
    else if (chsize > GET_OBJ_SIZE(weap))
    {
        return WIELD_LIGHT;
    }
    else if (chsize == GET_OBJ_SIZE(weap))
    {
        return WIELD_ONEHAND;
    }
    else if (chsize == GET_OBJ_SIZE(weap) - 1)
    {
        return WIELD_TWOHAND;
    }
    else if (chsize < GET_OBJ_SIZE(weap) - 1)
    {
        return WIELD_NONE; /* It's just too big for you! */
    }
    else
    {
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
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

void racial_body_parts(Character *ch)
{

    auto parts = race_bodyparts[(int)GET_RACE(ch)];

    for (auto i = 1; i < NUM_WEARS; i++)
    {
        ch->bodyparts.set(i, parts[i]);
    }
}

namespace race
{

    static std::unordered_map<Race, int> race_sizes = {
        {Race::tuffle, SIZE_SMALL},
        {Race::animal, SIZE_FINE},
        {Race::saiba, SIZE_LARGE},
        {Race::ogre, SIZE_LARGE},
        {Race::spirit, SIZE_TINY}};

    int getSize(Race id)
    {
        if (const auto found = race_sizes.find(id); found != race_sizes.end())
            return found->second;
        return SIZE_MEDIUM;
    }

    static std::unordered_set<Race> playable = {Race::human, Race::saiyan, Race::icer, Race::konatsu, Race::namekian,
                                                Race::mutant, Race::kanassan, Race::halfbreed, Race::bio_android, Race::android, Race::demon, Race::majin,
                                                Race::kai, Race::tuffle};

    bool isPlayable(Race id)
    {
        return playable.contains(id);
    }

    bool isValidGenome(Race id)
    {
        switch (id)
        {
        case Race::human:
        case Race::saiyan:
        case Race::namekian:
        case Race::icer:
        case Race::tuffle:
        case Race::arlian:
        case Race::kai:
        case Race::konatsu:
            return true;
        default:
            return false;
        }
    }

    std::unordered_set<Sex> getValidSexes(Race id)
    {
        switch (id)
        {
        case Race::namekian:
            return {SEX_NEUTRAL};
        default:
            return {SEX_NEUTRAL, SEX_MALE, SEX_FEMALE};
        }
    }

    static std::map<Race, std::string> race_names = {
        {Race::human, "Human"},
        {Race::saiyan, "Saiyan"},
        {Race::icer, "Icer"},
        {Race::konatsu, "Konatsu"},
        {Race::namekian, "Namekian"},
        {Race::mutant, "Mutant"},
        {Race::kanassan, "Kanassan"},
        {Race::halfbreed, "Half-Breed"},
        {Race::bio_android, "BioAndroid"},
        {Race::android, "Android"},
        {Race::demon, "Demon"},
        {Race::majin, "Majin"},
        {Race::kai, "Kai"},
        {Race::tuffle, "Tuffle"},
        {Race::hoshijin, "Hoshijin"},
        {Race::animal, "Animal"},
        {Race::saiba, "Saiba"},
        {Race::serpent, "Serpent"},
        {Race::ogre, "Ogre"},
        {Race::yardratian, "Yardratian"},
        {Race::arlian, "Arlian"},
        {Race::dragon, "Dragon"},
        {Race::mechanical, "Mechanical"},
        {Race::spirit, "Spirit"}};

    std::string getName(Race id)
    {
        if (const auto found = race_names.find(id); found != race_names.end())
            return found->second;
        return "Unknown";
    }

    static std::map<Race, std::string> race_abbr = {
        {Race::human, "Hum"},
        {Race::saiyan, "Sai"},
        {Race::icer, "Ice"},
        {Race::konatsu, "kon"},
        {Race::namekian, "Nam"},
        {Race::mutant, "Mut"},
        {Race::kanassan, "Kan"},
        {Race::halfbreed, "H-B"},
        {Race::bio_android, "Bio"},
        {Race::android, "And"},
        {Race::demon, "Dem"},
        {Race::majin, "Maj"},
        {Race::kai, "Kai"},
        {Race::tuffle, "Tru"},
        {Race::hoshijin, "Hos"},
        {Race::animal, "Ict"},
        {Race::saiba, "Sab"},
        {Race::serpent, "Ser"},
        {Race::ogre, "Ogr"},
        {Race::yardratian, "Yar"},
        {Race::arlian, "Arl"},
        {Race::dragon, "Drg"},
        {Race::mechanical, "Mec"},
        {Race::spirit, "Spi"}};

    std::string getAbbr(Race id)
    {
        if (const auto found = race_abbr.find(id); found != race_abbr.end())
            return found->second;
        return "N/A";
    }

    bool isPeople(Race id)
    {
        switch (id)
        {
        case Race::animal:
        case Race::saiba:
        case Race::mechanical:
        case Race::spirit:
            return false;
        default:
            return true;
        }
    }

    bool hasTail(Race id)
    {
        switch (id)
        {
        case Race::icer:
        case Race::bio_android:
        case Race::saiyan:
        case Race::halfbreed:
            return true;
        default:
            return false;
        }
    }

    bool isValidMimic(Race id)
    {
        switch (id)
        {
        case Race::icer:
        case Race::namekian:
        case Race::demon:
        case Race::majin:
        case Race::hoshijin:
        case Race::arlian:
            return false;
        default:
            return isPlayable(id);
        }
    }

    int64_t getSoftCap(Race id, int level)
    {
        switch (id)
        {
        case Race::kanassan:
        case Race::demon:
            return 500 * pow(1.1390, level);
        default:
            return 500 * pow(1.1357, level);
        }
    }

    bool isSenseable(Race id)
    {
        return id != Race::android;
    }

    static std::unordered_map<Race, std::vector<character_affect_type>> race_affects = {
        {Race::human, {
                          {APPLY_CVIT_REGEN_MULT, -0.5, ~0},
                      }},
        {Race::namekian, {
                             {APPLY_CVIT_REGEN_MULT, 0.5, ~0},
                         }},
        {Race::mutant, {
                           {APPLY_CVIT_REGEN_MULT, -0.1, ~0},
                       }},
        {Race::arlian, {
                           {APPLY_CVIT_REGEN_MULT, -0.7, ~0, [](auto ch)
                            { return (IS_FEMALE(ch) && OUTSIDE(ch)) ? 4.0 : 0.0; }},
                       }},
        {Race::kanassan, {
                             {APPLY_CVIT_REGEN_MULT, 0.0, ~0, [](auto ch)
                              {
                                  double out = 0.0;
                                  if (weather_info.sky == SKY_RAINING && OUTSIDE(ch))
                                      out += 0.1;
                                  if (ch->location.getEnvironment(ENV_WATER) >= 100.0)
                                      out += 16.0;
                                  return out;
                              }},
                         }},
        {Race::android, {
                            {APPLY_CVIT_REGEN_MULT, 0.0, ~0, [](auto ch)
                             { return ch->model == AndroidModel::Absorb ? -0.66 : 0.0; }},
                        }},
        {Race::saiyan, {
                           {APPLY_CSTAT_GAIN_MULT, 0.3, static_cast<int>(CharStat::experience)},
                           //{APPLY_PHYS_DAM_PERC, 0.0, 0, [](Character *ch) {return PLR_FLAGGED(ch, PLR_TAIL) ? 0.15 : 0;}},
                           //{APPLY_DAM_ATK_TIER, 0.2, 3},
                           //{APPLY_SKILL_SLOTS, -1},
                           //{APPLY_TRANS_ST_UPKEEP, 0.0, 0, [](Character *ch) {return ch->getCurLFPercent() > 0.7 ? -0.25 : 0.0}}
                       }},

        {Race::halfbreed, {
                              {APPLY_CSTAT_GAIN_MULT, 0.2, static_cast<int>(CharStat::experience)},
                              //{APPLY_SKILL_SLOTS, 1},
                              //{APPLY_ATTR_TRAIN_COST, -0.25, (int)CharTrain::Intelligence},
                              //{APPLY_ATTR_TRAIN_COST, -0.25, (int)CharTrain::Strength},
                              //{APPLY_PS_GAIN_MULT, -0.4}
                          }},

        {Race::icer, {
                         {APPLY_CSTAT_GAIN_MULT, -0.1, static_cast<int>(CharStat::experience)},

                     }},

        {Race::kai, {
                        {APPLY_CSTAT_GAIN_MULT, -0.1, static_cast<int>(CharStat::experience)},

                    }}};

    double getModifier(Character *ch, int location, int specific)
    {
        double out = 0.0;
        if (auto found = race_affects.find(ch->race); found != race_affects.end())
        {

            for (auto &affect : found->second)
            {
                if (affect.match(location, specific))
                {
                    out += affect.modifier;
                    if (affect.func)
                    {
                        out += affect.func(ch);
                    }
                }
            }
        }

        return out;
    }

    std::vector<Race> filterRaces(std::function<bool(Race)> func)
    {
        return dbat::util::getEnumList<Race>(func);
    };

    std::string defaultAppearance(Character *ch, Appearance type)
    {
        switch (type)
        {
        case Appearance::hair_style:
            switch (ch->race)
            {
            case Race::saiyan:
                return "spiky";
            case Race::icer:
            case Race::namekian:
            case Race::majin:
            case Race::arlian:
            case Race::bio_android:
            case Race::kanassan:
                return "bald";
            case Race::hoshijin:
                // someone decided that male hoshijin are always bald.
                // ... it wasn't me.
                if (ch->sex == Sex::male)
                    return "bald";
            default:
                return "plain";
            }
        case Appearance::hair_color:
            if (ch->getAppearance(Appearance::hair_style) == "bald")
                return "none";
            switch (ch->race)
            {
            case Race::kai:
                return "white";
            default:
                return "black";
            }
        case Appearance::skin_color:
            switch (ch->race)
            {
            case Race::kai: // basing this on Shin, Kibito, and Chronoa.
                return "mauve";
            case Race::majin:
                return "pink";
            case Race::namekian:
            case Race::yardratian:
                return "green";
            case Race::bio_android:
                return "mottled";
            case Race::kanassan:
                return "cyan";
            default: // I have no desire to start a flame war about what the 'normal'
                     // skin color of human or equivalent races is. Do not read anything into
                     // this besides needing a simple default.
                return "fair";
            }
        case Appearance::eye_color:
            return "black";
        case Appearance::aura_color:
            return "clear";
        case Appearance::build:
            return "average";
        case Appearance::posture:
            return "at ease";
        case Appearance::features:
            // features aren't used for body parts that can be lost, hidden, or optional.
            // like tails. they're for weird things like majin forelock or namekian antennae.
            switch (ch->race)
            {
            case Race::namekian:
                return "antennae";
            case Race::majin:
                return "forelock";
            case Race::arlian:
                if (ch->sex == Sex::female)
                    return "wings";
            default:
                return "nothing";
            }
        case Appearance::seeming:
            return boost::to_lower_copy(getName(ch->race));
        default:
            return "unknown";
        }
    }

}
