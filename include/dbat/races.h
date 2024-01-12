#pragma once

#include "structs.h"

// global variables
extern const struct guild_info_type guild_info[6];

// functions
extern void racial_body_parts(struct char_data *ch);

extern void set_height_and_weight_by_race(struct char_data *ch);

extern int invalid_race(struct char_data *ch, struct obj_data *obj);

// C++ conversion

namespace race {

    struct transform_bonus {
        int64_t bonus;
        long double mult, drain;
        int flag;
    };

    extern transform_bonus base_form;
    extern transform_bonus oozaru;

    class Race;

    typedef std::map<RaceID, Race *> RaceMap;
    extern RaceMap race_map;

    enum SoftCapType : uint8_t {
        Fixed = 0,
        Variable = 1
    };

    extern std::string getName(RaceID id);
    extern std::string getAbbr(RaceID id);
    extern bool isPlayable(RaceID id);
    extern std::vector<RaceID> getAll();
    extern std::vector<RaceID> getPlayable();
    extern std::set<int> getValidSexes(RaceID id);
    extern bool isValidMimic(RaceID id);
    extern bool isPeople(RaceID id);
    extern bool hasTail(RaceID id);
    extern int getRPPCost(RaceID id);
    extern int getRPPRefund(RaceID id);
    extern int64_t getSoftCap(RaceID id, int level);
    extern bool isSenseable(RaceID id);
    extern int getMaxTransformTier(RaceID id);

    class Race {
    public:
        Race(RaceID rid, const std::string &name, std::string abbr, int size, bool pc);
        RaceID getID() const;

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

        // limits stuff

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

        void echoTransform(char_data *ch, int tier) const;

        void echoRevert(char_data *ch, int tier) const;

    protected:
        RaceID r_id;
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
