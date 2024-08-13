#pragma once
#include "net.h"
#include "structs.h"
#include "races.h"
#include "class.h"

namespace net {

    enum class ChargenState : uint8_t {
        Intro = 0,
        Name,
        Race,
        Racial,
        Sex,
        Sensei,
        HairLength,
        HairColor,
        HairStyle,
        SkinColor,
        EyeColor,
        DistinguishingFeature,
        Height,
        Weight,
        Age,
        Aura,
        Skills,
        Alignment,
        Finish
    };

    struct ChargenData {
        nlohmann::json serialize();
        void deserialize(const nlohmann::json& j);

        std::string name;
        RaceID race{RaceID::Spirit};
        SenseiID sensei{SenseiID::Commoner};
        std::unordered_map<CharAppearance, appearance_t> appearances;
        std::unordered_set<int> genome{};                /* Bio racial bonus, Genome */

        int androidModel{-1};
        std::unordered_map<CharNum, num_t> nums;
        std::unordered_map<CharDim, dim_t> dims;
        std::optional<RaceID> mimic;
        int age;
    };

    class ChargenParser : public ConnectionParser {
    public:
        using ConnectionParser::ConnectionParser;
        void parse(const std::string &txt) override;
        void start() override;
        std::string getName() override;
        bool canCopyover() override {return true;};
        nlohmann::json serialize() override;
        void deserialize(const nlohmann::json& j) override;
        void update(double deltaTime) override;

    protected:
        ChargenData cg{};
        double introTimer{0.0};
        size_t introLinesSent{};
        ChargenState state{ChargenState::Intro};
        int total{};
        int ccpoints{};
        int negcount{};
        std::string maybeName;
        int roll_stats(int type, int bonus);
        void display_bonus_menu(int type);
        std::vector<RaceID> valid_races();
        void display_races();
        void display_races_sub();
        void display_races_mimic();
        std::vector<SenseiID> valid_classes();
        void display_classes_sub();
        void display_classes();
        void display_races_help();
        void display_classes_help();
        void exchange_ccpoints(int value);
        int opp_bonus(int value, int type);
        bool bonus_exclusive(int type, int value, int exc);
        void finish();

        // cg handlers.
        void changeState(ChargenState newState);

        void cgDisplayName();
        ChargenState cgHandleName(const std::string& arg);

        void cgDisplayRace();
        ChargenState cgHandleRace(const std::string& arg);

        void cgDisplaySex();
        ChargenState cgHandleSex(const std::string& arg);

        void cgDisplayRacial();
        ChargenState cgHandleRacial(const std::string& arg);

        void cgDisplaySensei();
        ChargenState cgHandleSensei(const std::string& arg);

        void cgDisplayHairLength();
        ChargenState cgHandleHairLength(const std::string& arg);

        void cgDisplayHairColor();
        ChargenState cgHandleHairColor(const std::string& arg);

        void cgDisplayHairStyle();
        ChargenState cgHandleHairStyle(const std::string& arg);

        void cgDisplaySkinColor();
        ChargenState cgHandleSkinColor(const std::string& arg);

        void cgDisplayEyeColor();
        ChargenState cgHandleEyeColor(const std::string& arg);

        void cgDisplayDistinguishingFeature();
        ChargenState cgHandleDistinguishingFeature(const std::string &arg);

        void cgDisplayAuraColor();
        ChargenState cgHandleAuraColor(const std::string& arg);

        void cgDisplayHeight();
        ChargenState cgHandleHeight(const std::string& arg);

        void cgDisplayWeight();
        ChargenState cgHandleWeight(const std::string& arg);

        void cgDisplayAge();
        ChargenState cgHandleAge(const std::string& arg);

        void cgDisplaySkills();
        ChargenState cgHandleSkills(const std::string& arg);

        void cgDisplayAlignment();
        ChargenState cgHandleAlignment(const std::string& arg);
    };

}