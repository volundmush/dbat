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
        Skills,
        Alignment,
    };

    struct ChargenData {
        nlohmann::json serialize();
        void deserialize(const nlohmann::json& j);

        std::string name;
        RaceID race{RaceID::Spirit};
        SenseiID sensei{SenseiID::Commoner};
        std::unordered_map<CharAppearance, appearance_t> appearances;
        int genome[2]{};                /* Bio racial bonus, Genome */
        std::bitset<NUM_PLR_FLAGS> playerFlags{}; /* act flag for NPC's; player flag for PC's */
        int androidModel{-1};
        std::unordered_map<CharNum, num_t> nums;
        std::optional<RaceID> mimic;
    };

    class ChargenParser : public ConnectionParser {
    public:
        using ConnectionParser::ConnectionParser;
        explicit ChargenParser(const std::shared_ptr<Connection>& co);
        void parse(const std::string &txt) override;
        void start() override;
        std::string getName() override;
        bool canCopyover() override {return true;};
        nlohmann::json serialize() override;
        void deserialize(const nlohmann::json& j) override;
        void update(double deltaTime) override;

    protected:
        ChargenData ch{};
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

        // CON_GET_NAME
        void cgDisplayName();
        ChargenState cgHandleName(const std::string& arg);

        // CON_QRACE
        void cgDisplayRace();
        ChargenState cgHandleRace(const std::string& arg);

        // CON_QSEX
        void cgDisplaySex();
        ChargenState cgHandleSex(const std::string& arg);

        // CON_RACIAL
        void cgDisplayRacial();
        ChargenState cgHandleRacial(const std::string& arg);

        // CON_CLASS
        void cgDisplaySensei();
        ChargenState cgHandleSensei(const std::string& arg);

        // CON_QHAIRL
        void cgDisplayHairLength();
        ChargenState cgHandleHairLength(const std::string& arg);

        // CON_HAIRC
        void cgDisplayHairColor();
        ChargenState cgHandleHairColor(const std::string& arg);

        // CON_HAIRS
        void cgDisplayHairStyle();
        ChargenState cgHandleHairStyle(const std::string& arg);

        // CON_SKIN
        void cgDisplaySkinColor();
        ChargenState cgHandleSkinColor(const std::string& arg);

        // CON_EYE
        void cgDisplayEyeColor();
        ChargenState cgHandleEyeColor(const std::string& arg);

        // CON_DISTFEA
        void cgDisplayDistinguishingFeature();
        ChargenState cgHandleDistinguishingFeature(const std::string &arg);

        // CON_AURA
        void cgDisplayAuraColor();
        ChargenState cgHandleAuraColor(const std::string& arg);

        // CON_HEIGHT
        void cgDisplayHeight();
        ChargenState cgHandleHeight(const std::string& arg);

        // CON_WEIGHT
        void cgDisplayWeight();
        ChargenState cgHandleWeight(const std::string& arg);

    };

}