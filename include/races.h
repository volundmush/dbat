//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_RACES_H
#define CIRCLE_RACES_H

#include "structs.h"


// global variables
extern const int race_ok_gender[NUM_SEX][NUM_RACES];
extern const char *d_race_types[NUM_RACES+1];
extern const char *race_names[NUM_RACES+1];
extern const char *pc_race_types[NUM_RACES+1];
extern const struct guild_info_type guild_info[6];
extern const int race_def_sizetable[NUM_RACES + 1];
extern const char *race_abbrevs[NUM_RACES+1];

// functions
void racial_body_parts(struct char_data *ch);
void racial_ability_modifiers(struct char_data *ch);
void set_height_and_weight_by_race(struct char_data *ch);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int parse_race(struct char_data *ch, int arg);

// C++ conversion



namespace dbat { namespace race {

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

        class Race;
        class RaceHandler;
        extern std::map<race_id, Race*> race_map;

        class Race {
        public:
            Race(race_id rid, std::string name, std::string abbr, int size, bool pc);
            race_id getID() const;
            const std::string& getName() const;
            const std::string& getNameLower() const;
            const std::string& getAbbr() const;
            int getSize() const;
            bool isPcOk() const;

            void displayTransReq(char_data *ch) const;
            // virtual functions
            virtual bool isValidSex(int sex_id) const;
            virtual bool checkCanTransform(char_data *ch) const;
            virtual int getMaxTransformTier() const;
            virtual bool checkCanTransformTier(char_data *ch, int tier) const;
            virtual bool raceCanTransform() const;
            virtual bool raceCanRevert() const;
            virtual void parse_transform(char_data *ch, char *arg) const;
            virtual void handle_transform(char_data *ch, int64_t add, double mult, double drain) const;
            virtual void revert_transform(char_data *ch) const;
            virtual void display_forms(char_data *ch) const;

        protected:
            race_id r_id;
            std::string name, lower_name, race_abbr, disg;
            int race_size;
            bool pc_check;
        };

        class Human : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Saiyan : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
            bool checkCanTransform(char_data *ch) const override;
        };

        class Icer : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };
        class Konatsu : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Namekian : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Mutant : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Kanassan : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Halfbreed : public Saiyan {
        public:
            using Saiyan::Saiyan;
            void display_forms(char_data *ch) const override;
        };

        class BioAndroid : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Android : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Demon : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };


        class Majin : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Kai : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Truffle : public Race {
        public:
            using Race::Race;
            void display_forms(char_data *ch) const override;
        };

        class Hoshijin : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Animal : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Saiba : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Serpent : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Ogre : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Yardratian : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Arlian : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Dragon : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Mechanical : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };

        class Spirit : public Race {
        public:
            using Race::Race;
            bool raceCanTransform() const;
        };



    Race* find_race(const std::string& arg);
    Race* find_pc_race(const std::string& arg);
    void load_races();



} }





#endif //CIRCLE_RACES_H
