#pragma once

#include "structs.h"

// global variables
extern const struct guild_info_type guild_info[6];

// functions
extern void racial_body_parts(struct char_data *ch);

extern void set_height_and_weight_by_race(struct char_data *ch);

extern int invalid_race(struct char_data *ch, struct obj_data *obj);

// C++ conversion

namespace dbat::race {

    enum race_id : uint8_t {
        human = 0,
        saiyan = 1,
        icer = 2,
        konatsu = 3,
        namekian = 4,
        mutant = 5,
        kanassan = 6,
        halfbreed = 7,
        bio = 8,
        android = 9,
        demon = 10,
        majin = 11,
        kai = 12,
        truffle = 13,
        hoshijin = 14,
        animal = 15,
        saiba = 16,
        serpent = 17,
        ogre = 18,
        yardratian = 19,
        arlian = 20,
        dragon = 21,
        mechanical = 22,
        spirit = 23
    };

    struct transform_bonus {
        int64_t bonus;
        long double mult, drain;
        int flag;
    };

    extern transform_bonus base_form;
    extern transform_bonus oozaru;

    class Race;

    typedef std::map<race_id, Race *> RaceMap;
    extern RaceMap race_map;

    enum SoftCapType : uint8_t {
        Fixed = 0,
        Variable = 1
    };

    class Race {
    public:
        Race(race_id rid, const std::string &name, std::string abbr, int size, bool pc);

        race_id getID() const;

        const std::string &getName() const;

        const std::string &getNameLower() const;

        const std::string &getAbbr() const;

        const std::string &getDisguised() const;

        int getSize() const;

        bool isPcOk() const;


        // stats stuff
        bool raceCanBeSensed() const;

        bool isValidSex(int sex_id) const;

        bool raceCanBeMimiced() const;

        int getRPPCost() const;

        int getRPPRefund() const;

        bool raceIsPeople() const;

        // limts stuff

        bool raceHasTail() const;

        bool hasTail(char_data *ch) const;

        void loseTail(char_data *ch) const;

        void gainTail(char_data *ch, bool announce = true) const;

        // softcap stuff
        const std::map<int, int64_t> &getSoftMap(const char_data *ch) const;

        SoftCapType getSoftType(const char_data *ch) const;

        // transform stuff
        void displayTransReq(char_data *ch) const;

        bool checkCanTransform(char_data *ch) const;

        bool checkTransUnlock(char_data *ch, int tier) const;

        int getMaxTransformTier(char_data *ch) const;

        int getCurrentTransTier(const char_data *ch) const;

        transform_bonus getCurForm(const char_data *ch) const;

        const std::unordered_map<std::string, int> &getTierMap(char_data *ch) const;

        const std::map<int, transform_bonus> &getTransMap(const char_data *ch) const;

        int flagToTier(int flag) const;

        std::optional<transform_bonus> findForm(char_data *ch, const std::string &arg) const;

        bool raceCanTransform() const;

        bool raceCanRevert() const;

        void displayForms(char_data *ch) const;

        bool raceHasNoisyTransformations() const;

        void echoTransform(char_data *ch, int tier) const;

        void echoRevert(char_data *ch, int tier) const;

    protected:
        race_id r_id;
        std::string name, lower_name, race_abbr, disg;
        int race_size;
        bool pc_check;


    };

    RaceMap valid_for_sex(int sex);

    RaceMap valid_for_sex_pc(int sex);

    Race *find_race(const std::string &arg);

    Race *find_pc_race(const std::string &arg);

    Race *find_race_map(const std::string &arg, const RaceMap &r_map);

    void load_races();

    Race *find_race_map_id(const int id, const RaceMap &r_map);

}
