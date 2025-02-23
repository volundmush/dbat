#include "dbat/chargen.h"
#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/constants.h"
#include "dbat/charmenu.h"
#include "dbat/players.h"

namespace net {

    nlohmann::json ChargenData::serialize() {
        nlohmann::json j;
        if(!name.empty()) j["name"] = name;
        j["race"] = static_cast<int>(race);
        j["sensei"] = static_cast<int>(sensei);
        for(auto [k, v] : appearances) {
            j["appearances"].push_back(std::make_pair(static_cast<int>(k), v));
        }
        j["genome"] = genome;
        if(androidModel != -1) j["androidModel"] = androidModel;
        for(auto [k, v] : nums) {
            j["nums"].push_back(std::make_pair(static_cast<int>(k), v));
        }
        if(mimic) j["mimic"] = static_cast<int>(mimic.value());

        return j;
    }

    void ChargenData::deserialize(const nlohmann::json& j) {
        if(j.contains("name")) name = j.at("name").get<std::string>();
        if(j.contains("race")) race = static_cast<RaceID>(j.at("race").get<int>());
        if(j.contains("sensei")) sensei = static_cast<SenseiID>(j.at("sensei").get<int>());
        if(j.contains("appearances")) {
            for(auto& a : j.at("appearances")) {
                appearances[static_cast<CharAppearance>(a.at(0).get<int>())] = a.at(1).get<int>();
            }
        }
        if(j.contains("genome")) {
            for(auto& g : j.at("genome")) {
                genome.insert(g.get<int>());
            }
        }
        if(j.contains("androidModel")) androidModel = j.at("androidModel").get<int>();
        if(j.contains("nums")) {
            for(auto& n : j.at("nums")) {
                nums[static_cast<CharNum>(n.at(0).get<int>())] = n.at(1).get<int>();
            }
        }
        if(j.contains("mimic")) mimic = static_cast<RaceID>(j.at("mimic").get<int>());
    }

    std::string ChargenParser::getName() {
        return "ChargenParser";
    }

    nlohmann::json ChargenParser::serialize() {
        auto j = ConnectionParser::serialize();
        j["state"] = state;
        j["total"] = total;
        j["ccpoints"] = ccpoints;
        j["negcount"] = negcount;
        j["cg"] = cg.serialize();

        return j;
    }

    void ChargenParser::deserialize(const nlohmann::json &j) {
        if (j.contains("state")) state = static_cast<ChargenState>(j.at("state").get<int>());
        if (j.contains("total")) total = j.at("statotal").get<int>();
        if (j.contains("ccpoints")) ccpoints = j.at("ccpoints").get<int>();
        if (j.contains("stnegcount")) negcount = j.at("negcount").get<int>();

        if (j.contains("cg")) {
            cg.deserialize(j["cg"]);
        }

    }

    std::vector<RaceID> ChargenParser::valid_races() {
        auto check = [&](RaceID id) { return race::isPlayable(id); };
        return race::filterRaces(check);
    }

    void ChargenParser::display_races_sub() {
        int i = 0;
        for (const auto &r: valid_races())
            sendText(fmt::format("@C{:^15}@n{}", race::getName(r), !(++i % 2) ? "\r\n" : "   "));
    }

    void ChargenParser::display_races_mimic() {
        auto check = [&](RaceID id) { return race::isValidMimic(id); };
        auto races = race::filterRaces(check);
        int i = 0;
        for (const auto &r: races)
            sendText(fmt::format("@C{:^15}@n{}", race::getName(r), !(++i % 2) ? "\r\n" : "   "));
    }

    void ChargenParser::display_races() {
        sendText("\r\n@YRace SELECTION menu:\r\n@D---------------------------------------\r\n@n");
        display_races_sub();

        sendText("\n @BR@W) @CRandom Race Selection!\r\n@n");
        sendText("\n@WRace: @n");
    }

    std::vector<SenseiID> ChargenParser::valid_classes() {
        auto check = [&](SenseiID id) { return sensei::isPlayable(id) && sensei::isValidSenseiForRace(id, cg.race); };
        return sensei::filterSenseis(check);
    }

    void ChargenParser::display_classes_sub() {
        int i = 0;
        for (const auto &s: valid_classes())
            sendText(fmt::format("@C{}@n{}", sensei::getName(s).c_str(), !(++i % 2) ? "\r\n" : "	"));
    }

    void ChargenParser::display_classes() {
        sendText("\r\n@YSensei SELECTION menu:\r\n@D--------------------------------------\r\n@n");
        display_classes_sub();

        sendText("\n @BR@W) @CRandom Sensei Selection!\r\n@n");
        sendText("\n@WSensei: @n");
    }

    void ChargenParser::display_races_help() {
        sendText("\r\n@YRace HELP menu:\r\n@G--------------------------------------------\r\n@n");
        display_races_sub();
        sendText("\n@WHelp on Race #: @n");
    }

    void ChargenParser::display_classes_help() {
        sendText("\r\n@YClass HELP menu:\r\n@G-------------------------------------------\r\n@n");
        display_classes_sub();
        sendText("\n@WHelp on Class #: @n");
    }

    void ChargenParser::start() {
        sendText("Your thoughts turn in strange directions... is this some kind of dream?\r\n");
        introTimer = 3.0;
        state = ChargenState::Intro;
    }

    static const std::vector<std::string> introLines = {
            "\r\nMany fighters say your story really starts the first time you find your limits.\r\n"
            "That only then you can really grow.\r\n",

            "\r\nThe bitter taste of dirt in your mouth, the sweltering pain of one too many"
            "blows and the darkness of unconciousness is all that greets you.\r\n"
    };

    void ChargenParser::update(double deltaTime) {
        if (state != ChargenState::Intro) return;

        introTimer = std::max<double>(0.0, introTimer - deltaTime);
        if (introTimer > 0.0) return;

        if (introLinesSent < introLines.size() - 1) {
            sendText(introLines[introLinesSent++]);
            introTimer = 3.0;
            return;
        }

        // we've sent the last line.
        changeState(ChargenState::Name);
    }
    
    static const std::unordered_set<RaceID> usesChargenRacial = {
            RaceID::Android, RaceID::BioAndroid, RaceID::Mutant
    };
    
    void ChargenParser::changeState(ChargenState newState) {
        state = newState;
        switch (state) {
            case ChargenState::Name:
                cgDisplayName();
                break;
            case ChargenState::Race:
                display_races();
                cgDisplayRace();
                break;
            case ChargenState::Sex:
                if(cg.race == RaceID::Namekian) {
                    cg.appearances[CharAppearance::Sex] = SEX_NEUTRAL;
                    changeState(ChargenState::Racial);
                    return;
                }
                cgDisplaySex();
                break;
            case ChargenState::Racial:
                if(usesChargenRacial.contains(cg.race))
                    cgDisplayRacial();
                else
                    changeState(ChargenState::Sensei);
                break;
            case ChargenState::Sensei:
                display_classes();
                cgDisplaySensei();
                break;
            case ChargenState::HairLength:
                cgDisplayHairLength();
                break;
            case ChargenState::HairColor:
                if(cg.race == RaceID::Saiyan) {
                    cg.appearances[CharAppearance::HairColor] = HAIRC_BLACK;
                    changeState(ChargenState::HairStyle);
                    return;
                }
                cgDisplayHairColor();
                break;
            case ChargenState::HairStyle:
                if(cg.appearances[CharAppearance::HairLength] == HAIRL_BALD) {
                    cg.appearances[CharAppearance::HairStyle] = HAIRS_NONE;
                    changeState(ChargenState::SkinColor);
                    return;
                }
                cgDisplayHairStyle();
                break;
            case ChargenState::SkinColor:
                if(cg.race == RaceID::Saiyan) {
                    cg.appearances[CharAppearance::SkinColor] = SKIN_WHITE;
                    changeState(ChargenState::EyeColor);
                    return;
                }
                cgDisplaySkinColor();
                break;
            case ChargenState::EyeColor:
                if(cg.race == RaceID::Saiyan) {
                    cg.appearances[CharAppearance::EyeColor] = EYE_BLACK;
                    changeState(ChargenState::DistinguishingFeature);
                    return;
                }
                cgDisplayEyeColor();
                break;
            case ChargenState::DistinguishingFeature:
                cgDisplayDistinguishingFeature();
                break;
            case ChargenState::Alignment:
                cgDisplayAlignment();
                break;
            case ChargenState::Aura:
                cgDisplayAuraColor();
                break;
            case ChargenState::Skills:
                cgDisplaySkills();
                break;
            case ChargenState::Finish:
                finish();
                break;
        }
    }

    // CON_GET_NAME
    void ChargenParser::cgDisplayName() {
        sendText("\r\nCan you remember your name?\r\n");
    }

    ChargenState ChargenParser::cgHandleName(const std::string &arg) {
        auto maybe = arg;
        boost::trim(maybe);

        if (cg.name.empty()) {
            if (arg.empty()) {
                cgDisplayName();
                return state;
            }

            if (findPlayer(maybe)) {
                sendText("\r\nNah... that doesn't seem right. Maybe it's\r\nsomeone else's name?\r\n");
                cgDisplayName();
                return state;
            }
            auto result = validate_pc_name(maybe);
            if (!result.first) {
                sendText("\r\n" + result.second.value() + "\r\n");
                cgDisplayName();
                return state;
            }
            cg.name = maybe;
            sendText(fmt::format("\r\n{}, huh? Let's hear it again to be sure.\r\n", cg.name));
            return state;
        }
        if (!boost::iequals(maybe, cg.name)) {
            cg.name.clear();
            parse(maybe);
            return state;
        }
        return ChargenState::Race;
    }

    void ChargenParser::cgDisplayRace() {
        sendText("\r\nCan you remember what race you are?\r\n"
                 "(@Whelp <race>@n to display useful info)\r\n");
    }

    ChargenState ChargenParser::cgHandleRace(const std::string &arg) {
        auto check = [&](RaceID id) { return race::isPlayable(id); };

        std::vector<std::string> args;
        boost::split(args, arg, boost::is_space());

        if(args.empty()) {
            cgDisplayRace();
            return state;
        }

        std::string toCheck = args[0];
        bool checkHelp = false;

        if (boost::iequals(toCheck, "help")) {
            if(args.size() < 2) {
                sendText("\r\nHelp with which race?\r\n");
                return state;
            }
            toCheck = args[1];
            checkHelp = true;
        }

        auto chosen_race = race::findRace(toCheck, check);
        if(!chosen_race) {
            sendText("\r\nUnfortunately that's not a race.\r\n");
            display_races();
            cgDisplayRace();
            return state;
        }

        auto race = chosen_race.value();

        if(checkHelp) {
            show_help(conn, race::getName(*chosen_race).c_str());
            return state;
        }

        cg.race = race;

        return ChargenState::Sex;
    }

    void ChargenParser::cgDisplaySex() {
        sendText("\r\nWhat is your sex? (M / F / N)\r\n");
    }

    ChargenState ChargenParser::cgHandleSex(const std::string &arg) {
        if(arg.empty()) {
            cgDisplaySex();
            return state;
        }

        switch(toupper(arg[0])) {
            case 'M':
                cg.appearances[CharAppearance::Sex] = SEX_MALE;
                break;
            case 'F':
                cg.appearances[CharAppearance::Sex] = SEX_FEMALE;
                break;
            case 'N':
                cg.appearances[CharAppearance::Sex] = SEX_NEUTRAL;
                break;
            default:
                sendText("That is not an option!\r\n");
                return state;
        }
        
        return ChargenState::Racial;
    }

    void ChargenParser::cgDisplayRacial() {
        switch(cg.race) {
            case RaceID::Android:
                if(cg.androidModel == -1) {
                    sendText("\r\n@YChoose your model type.\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Absorbtion Model@n\r\n");
                    sendText("@B2@W)@C Repair Model@n\r\n");
                    sendText("@B3@W)@C Sense, Powersense Model@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");

                } else {
                    sendText("@YWhat do you want to be identified as at first glance?\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    display_races_mimic();
                    sendText("\r\nor @Wnone@n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                }
                break;
            case RaceID::BioAndroid:
                sendText("\r\n@RSelect two genomes to be your primary DNA strains.\r\n");
                sendText("@D--------------------------------------------------------@n\r\n");
                sendText("@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\r\n");
                sendText("@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\r\n");
                sendText("@B3@W) @CNamek   @c- @CStretchy arms that allow greater reach@n\r\n");
                sendText("@B4@W) @CIcer    @c- @C+20%% damage for Tier 4 attacks@n\r\n");
                sendText("@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\r\n");
                sendText("@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\r\n");
                sendText("@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
                sendText("@B8@w) @CKonatsu @c- @C40%% higher chance to multihit on physical attacks.\r\n");
                sendText("@wChoose: ");
                break;
            case RaceID::Mutant:
                sendText("\r\n@RSelect two mutations.\r\n");
                sendText("@D--------------------------------------------------------@n\r\n");
                sendText("@B 1@W) @CExtreme Speed       @c-+30%% to Speed Index @C@n\r\n");
                sendText("@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%% instead of 5%%@C@n\r\n");
                sendText(
                        "@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\r\n");
                sendText("@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\r\n");
                sendText("@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\r\n");
                sendText("@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\r\n");
                sendText("@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\r\n");
                sendText(
                        "@B 8@W) @CRubbery Body        @c-10%% of physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\r\n");
                sendText("@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\r\n");
                sendText(
                        "@B10@w) @CNatural Energy      @c-Get 5%% of your ki damage refunded back into your current ki total.@n\r\n");
                sendText("@wChoose: ");
                break;
        }
    }

    ChargenState ChargenParser::cgHandleRacial(const std::string& arg) {
        if(arg.empty()) {
            sendText("That's not an option!\r\n");
            cgDisplayRacial();
            return state;
        }
        
        switch(cg.race) {
            case RaceID::Android: {
                
            if(cg.androidModel == -1) {
                switch(arg[0]) {
                case '1':
                    cg.androidModel = PLR_ABSORB;
                    break;
                case '2':
                    cg.androidModel = PLR_REPAIR;
                    break;
                case '3':
                    cg.androidModel = PLR_SENSEM;
                    break;
                default:
                    sendText("That is not an acceptable option.\r\n");
                    cgDisplayRacial();
                    return state;
                }
                break;
            }
            if (boost::iequals(arg, "none")) {
                    cg.mimic.reset();
                    break;
                }
                auto check = [&](RaceID id) { return race::isPlayable(id); };
                auto chosen_race = race::findRace(arg, check);
                if(!chosen_race) {
                    sendText("That is not an acceptable choice!\r\n");
                    cgDisplayRacial();
                    return state;
                }
                cg.mimic = chosen_race.value();
            
                break;
            }
            case RaceID::BioAndroid:
            case RaceID::Mutant: {
                auto choice = atoi(arg.c_str());
                int choiceMax = (cg.race == RaceID::Mutant) ? 10 : 8;
                if(choice < 1 || choice > choiceMax) {
                    sendText("That is not an acceptable option.\r\n");
                    cgDisplayRacial();
                    return state;
                }
                if(cg.genome.contains(choice)) {
                    sendText("Deselected that one.\r\n");
                    cg.genome.erase(choice);
                    cgDisplayRacial();
                    return state;
                } else {
                    cg.genome.insert(choice);
                    if(cg.genome.size() == 1) {
                        sendText("First genome selected. Now pick your second.\r\n");
                        cgDisplayRacial();
                        return state;
                    }
                    sendText("Second genome selected.\r\n");
                }
            }
            break;
        }

        return ChargenState::Sensei;
    }

    void ChargenParser::cgDisplaySensei() {
        sendText("\r\nCan you remember your sensei's name?\r\n"
                 "(@Whelp <sensei>@n to display useful info)\r\n");
    }

    ChargenState ChargenParser::cgHandleSensei(const std::string& arg) {
        auto check = [&](SenseiID id) { return sensei::isPlayable(id); };

        std::vector<std::string> args;
        boost::split(args, arg, boost::is_space());

        if(args.empty()) {
            cgDisplaySensei();
            return state;
        }

        std::string toCheck = args[0];
        bool checkHelp = false;

        if (boost::iequals(toCheck, "help")) {
            if(args.size() < 2) {
                sendText("\r\nHelp with which sensei?\r\n");
                return state;
            }
            toCheck = args[1];
            checkHelp = true;
        }

        auto chosen_sensei = sensei::findSensei(toCheck, check);
        if(!chosen_sensei) {
            sendText("\r\nUnfortunately that's not a sensei.\r\n");
            display_classes();
            cgDisplaySensei();
            return state;
        }

        auto sensei = chosen_sensei.value();

        if(checkHelp) {
            show_help(conn, sensei::getName(*chosen_sensei).c_str());
            return state;
        }

        cg.sensei = sensei;

        return ChargenState::HairLength;

    }

    static const std::unordered_set<RaceID> normalHair = {
            RaceID::Human, RaceID::Saiyan, RaceID::Konatsu, RaceID::Mutant,
            RaceID::Android, RaceID::Kai, RaceID::Halfbreed, RaceID::Tuffle,
            RaceID::Hoshijin
    };

    static bool usesNormalHair(RaceID r, int sex) {
        if(!normalHair.contains(r)) return false;
        if(r == RaceID::Hoshijin && sex == SEX_MALE) return false;
        return true;
    }

    void ChargenParser::cgDisplayHairLength() {
        char *buf1, *buf2 = "None";
        if(cg.race == RaceID::Icer || cg.race == RaceID::Demon) {
            buf1 = "Horn";
        } else if(cg.race == RaceID::Majin) {
            buf1 = "Forelock";
        } else if(cg.race == RaceID::Namekian || cg.race == RaceID::Arlian) {
            buf1 = "Antennae";
        } else {
            buf1 = "Hair";
            buf2 = "Bald";
        }
        sendText(fmt::format("@Y{} Length SELECTION menu:\r\n", buf1));
        sendText("@D---------------------------------------@n\r\n");
        sendText(fmt::format("@B1@W)@C {}  @B2@W)@C Short  @B3@W)@C Medium\r\n", buf2));
        sendText("@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleHairLength(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto hairTo = atoi(arg.c_str());
        if(hairTo < 1 || hairTo > 5) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::HairLength] = hairTo;

        return ChargenState::HairColor;
    }


    void ChargenParser::cgDisplayHairColor() {
        char *buf = "Hair";
        if(cg.race == RaceID::Arlian && cg.appearances[CharAppearance::Sex] == SEX_FEMALE) {
            buf = "Wing";
        }
        sendText(fmt::format("@Y{} color SELECTION menu:\r\n", buf));
        sendText("@D---------------------------------------@n\r\n");
        sendText(" @B1@W)@C Black   @B2@W)@C Brown   @B3@W)@C Blonde\r\n");
        sendText(" @B4@W)@C Grey    @B5@W)@C Red     @B6@W)@C Orange@n\r\n");
        sendText(" @B7@W)@C Green   @B8@W)@C Blue    @B9@W)@C Pink\r\n");
        sendText("@B10@W)@C Purple @B11@W)@C Silver @B12@W)@C Crimson@n\r\n");
        sendText("@B13@W)@C White@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleHairColor(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto hairTo = atoi(arg.c_str());
        if(hairTo < 1 || hairTo > 13) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::HairColor] = hairTo;

        return ChargenState::HairStyle;
    }

    void ChargenParser::cgDisplayHairStyle() {
        sendText("@YHair style SELECTION menu:\r\n");
        sendText("@D---------------------------------------@n\r\n");
        sendText("@B1@W)@C Plain     @B2@W)@C Mohawk    @B3@W)@C Spiky\r\n");
        sendText("@B4@W)@C Curly     @B5@W)@C Uneven    @B6@W)@C Ponytail@n\r\n");
        sendText("@B7@W)@C Afro      @B8@W)@C Fade      @B9@W)@C Crew Cut\r\n");
        sendText("@BA@W)@C Feathered @BB@W)@C Dread Locks@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleHairStyle(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto hairTo = atoi(arg.c_str());
        if(hairTo < 1 || hairTo > 11) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::HairStyle] = hairTo;

        return ChargenState::SkinColor;
    }


    void ChargenParser::cgDisplaySkinColor() {
        sendText("@YSkin color SELECTION menu:\r\n");
        sendText("@D---------------------------------------@n\r\n");
        sendText(" @B1@W)@C White   @B2@W)@C Black   @B3@W)@C Green\r\n");
        sendText(" @B4@W)@C Orange  @B5@W)@C Yellow  @B6@W)@C Red@n\r\n");
        sendText(" @B7@W)@C Grey    @B8@W)@C Blue    @B9@W)@C Aqua\r\n");
        sendText("@B10@W)@C Pink   @B11@W)@C Purple @B12@W)@C Tan@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleSkinColor(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto skinTo = atoi(arg.c_str());
        if(skinTo < 1 || skinTo > 12) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::SkinColor] = skinTo;

        return ChargenState::EyeColor;
    }

    void ChargenParser::cgDisplayEyeColor() {
        sendText("@YEye color SELECTION menu:\r\n");
        sendText("@D---------------------------------------@n\r\n");
        sendText("@B1@W)@C Blue  @B2@W)@C Black  @B3@W)@C Green\r\n");
        sendText("@B4@W)@C Brown @B5@W)@C Red    @B6@W)@C Aqua@n\r\n");
        sendText("@B7@W)@C Pink  @B8@W)@C Purple @B9@W)@C Crimson\r\n");
        sendText("@BA@W)@C Gold  @BB@W)@C Amber  @BC@W)@C Emerald@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleEyeColor(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto eyeTo = atoi(arg.c_str());
        if(eyeTo < 1 || eyeTo > 12) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::EyeColor] = eyeTo;

        return ChargenState::DistinguishingFeature;
    }

    void ChargenParser::cgDisplayDistinguishingFeature() {
        sendText("@YWhat do you want to be your most distinguishing feature:\r\n");
        sendText("@D---------------------------------------@n\r\n");
        sendText("@B1@W)@C My Eyes@n\r\n");
        char *buf = "Hair";
        switch(cg.race) {
            case RaceID::Majin:
                buf = "Forelock";
                break;
            case RaceID::Namekian:
            case RaceID::Arlian:
                buf = "Antennae";
                break;
            case RaceID::Icer:
            case RaceID::Demon:
                buf = "Horns";
                break;
        }
        sendText(fmt::format("@B2@W)@C My {}@n\r\n", buf));
        sendText("@B3@W)@C My Skin@n\r\n");
        sendText("@B4@W)@C My Height@n\r\n");
        sendText("@B5@W)@C My Weight@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleDistinguishingFeature(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto dist = atoi(arg.c_str());
        if(dist < 1 || dist > 5) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::DistinguishingFeature] = dist;

        return ChargenState::Aura;
    }

    void ChargenParser::cgDisplayAuraColor() {
        sendText("@YAura color SELECTION menu:\r\n");
        sendText("@D---------------------------------------@n\r\n");
        sendText("@B1@W)@C White  @B2@W)@C Blue@n\r\n");
        sendText("@B3@W)@C Red    @B4@W)@C Green@n\r\n");
        sendText("@B5@W)@C Pink   @B6@W)@C Purple@n\r\n");
        sendText("@B7@W)@C Yellow @B8@W)@C Black@n\r\n");
        sendText("@B9@W)@C Orange@n\r\n");
        sendText("@w\r\nMake a selection:@n\r\n");
    }

    ChargenState ChargenParser::cgHandleAuraColor(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto auraTo = atoi(arg.c_str());
        if(auraTo < 1 || auraTo > 9) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        cg.appearances[CharAppearance::Aura] = auraTo;

        return ChargenState::Alignment;
    }

    void ChargenParser::cgDisplayAlignment() {
        sendText("@C             Alignment Menu@n\r\n");
        sendText("@D---------------------------------------@n\r\n");
        sendText("@B1@W) @wSaintly@n\r\n");
        sendText("@B2@W) @wValiant@n\r\n");
        sendText("@B3@W) @wHero@n\r\n");
        sendText("@B4@W) @wDo-gooder@n\r\n");
        sendText("@B5@W) @wNeutral\r\n");
        sendText("@B6@W) @wCrook@n\r\n");
        sendText("@B7@W) @wVillain@n\r\n");
        sendText("@B8@W) @wTerrible@n\r\n");
        sendText("@B9@W) @wHorribly Evil@n\r\n");
        sendText("Choose: \r\n");
    }

    ChargenState ChargenParser::cgHandleAlignment(const std::string& arg) {
        if(arg.empty()) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        auto alignTo = atoi(arg.c_str());
        if(alignTo < 1 || alignTo > 9) {
            sendText("That is not an acceptable option.\r\n");
            return state;
        }
        //cg.alignment = alignTo;

        return ChargenState::Finish;
    }

    void ChargenParser::cgDisplaySkills() {

    }

    ChargenState ChargenParser::cgHandleSkills(const std::string& arg) {
        return ChargenState::Finish;
    }


    void ChargenParser::parse(const std::string &arg) {
        // Do nothing while the intro is playing.
        if (state == ChargenState::Intro) return;

        std::optional<SenseiID> chosen_sensei;
        bool penalty = false, moveon = false;
        int roll = rand_number(1, 6);
        int value;

        struct char_data *found;

        ChargenState resultState = state;

        switch (state) {
            case ChargenState::Name:
                resultState = cgHandleName(arg);
                break;
            case ChargenState::Race:
                resultState = cgHandleRace(arg);
                break;
            case ChargenState::Racial:
                resultState = cgHandleRacial(arg);
                break;
            case ChargenState::Sex:
                resultState = cgHandleSex(arg);
                break;
            case ChargenState::Sensei:
                resultState = cgHandleSensei(arg);
                break;
            case ChargenState::HairLength:
                resultState = cgHandleHairLength(arg);
                break;
            case ChargenState::HairColor:
                resultState = cgHandleHairColor(arg);
                break;
            case ChargenState::HairStyle:
                resultState = cgHandleHairStyle(arg);
                break;
            case ChargenState::SkinColor:
                resultState = cgHandleSkinColor(arg);
                break;
            case ChargenState::EyeColor:
                resultState = cgHandleEyeColor(arg);
                break;
            case ChargenState::DistinguishingFeature:
                resultState = cgHandleDistinguishingFeature(arg);
                break;
            case ChargenState::Alignment:
                resultState = cgHandleAlignment(arg);
                break;
            case ChargenState::Skills:
                resultState = cgHandleSkills(arg);
                break;
            case ChargenState::Aura:
                resultState = cgHandleAuraColor(arg);
                break;
        }

            if(resultState != state) {
                changeState(resultState);
            }
                
    }

    void ChargenParser::finish() {
        // CREATE PLAYER ENTRY
        auto ch = std::make_shared<char_data>();
        ch->name = strdup(cg.name.c_str());
        ch->id = nextID();
        ch->generation = time(nullptr);
        ch->pref.set(PRF_COLOR);
        auto &p = players[ch->id];
        p.name = cg.name;
        p.id = ch->id;
        p.account = conn->account;
        conn->account->characters.emplace_back(ch->id);
        p.character = ch.get();
        units.emplace(ch->id, ch);
        uniqueCharacters.emplace(ch->id, ch);

        ch->chclass = cg.sensei;
        ch->race = cg.race;
        if(cg.androidModel != -1) ch->playerFlags.set(cg.androidModel);
        if(cg.mimic) ch->mimic = cg.mimic;
        ch->appearances = cg.appearances;
        ch->nums = cg.nums;
        ch->dims = cg.dims;
        if(cg.race == RaceID::BioAndroid || cg.race == RaceID::Mutant) {
            ch->genome = cg.genome;
        }

        init_char(ch.get());
        // set state to -1 to prevent accidental freeing of cg...
        send_to_imm("New Character '%s' created by Account: %s", cg.name, p.account->name.c_str());
        conn->setParser(new CharacterMenu(conn, ch.get()));
    }
}