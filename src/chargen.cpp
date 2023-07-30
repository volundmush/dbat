#include "dbat/chargen.h"
#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/constants.h"
#include "dbat/oasis.h"
#include "dbat/charmenu.h"
#include "dbat/players.h"
#include "dbat/dg_comm.h"

namespace net {

    int ChargenParser::roll_stats(int type, int bonus) {

        int pool = 0, base_num = bonus, max_num = bonus;
        int powerlevel = 0, ki = 1, stamina = 2;

        if (type == powerlevel) {
            base_num = ch->real_abils.str * 3;
            max_num = ch->real_abils.str * 5;
        } else if (type == ki) {
            base_num = ch->real_abils.intel * 3;
            max_num = ch->real_abils.intel * 5;
        } else if (type == stamina) {
            base_num = ch->real_abils.con * 3;
            max_num = ch->real_abils.con * 5;
        }

        pool = rand_number(base_num, max_num) + bonus;

        return (pool);
    }

    bool ChargenParser::bonus_exclusive(int type, int value, int exc) {
        if(GET_BONUS(ch, exc) > 0) {
            display_bonus_menu(type);
            sendText(fmt::format("@R{} and {} are mutually exclusive.\n\n", list_bonus[exc], list_bonus[value]));
            return false;
        }
        return true;
    }

    static std::map<int, int> exclusive = {
            {0, 1}, {2, 40}, {6, 39}, {8, 50}, {9, 43}, {10, 44}, {11, 45}, {12, 46}, {13, 47}, {14, 48},
            {15, 33}, {17, 34}, {18, 26}, {19, 29}, {20, 27}, {21, 30}, {22, 31}, {23, 32}, {24, 35}, {25, 51},
            {26, 18}, {27, 20}, {29, 19}, {30, 21}, {31, 22}, {32, 23}, {33, 15}, {34, 17}, {35, 24}, {39, 6},
            {40, 2}, {43, 9}, {44, 10}, {45, 11}, {46, 12}, {47, 13}, {48, 14}, {50, 8}, {51, 25}
    };

    int ChargenParser::opp_bonus(int value, int type) {

        switch (value) {
            case 3:
            case 16:
            case 20:
            case 27:
                if (IS_ANDROID(ch)) {
                    display_bonus_menu(type);
                    sendText(fmt::format("You can not take {} as an android!@n\r\n", list_bonus[value]));
                    return false;
                }
                break;
            case 17:
                if (IS_DEMON(ch)) {
                    display_bonus_menu(type);
                    sendText("As a demon you are already fireproof.\r\n");
                    return false;
                }
                break;
        }

        auto find = exclusive.find(value);
        if(find == exclusive.end()) return true;
        return bonus_exclusive(type, value, find->second);
    }

/* Handle CC point exchange for Bonus/negative */
    void ChargenParser::exchange_ccpoints(int value) {
        int type = 0;

        if (state == CON_BONUS) {
            type = 0;
        } else {
            type = 1;
        }


        if (GET_BONUS(ch, value) > 0 && ccpoints - list_bonus_cost[value] < 0) {
            display_bonus_menu(type);
            sendText("@RYou must unselect some bonus traits first.\r\n");
            return;
        } else if (GET_BONUS(ch, value) > 0 && ccpoints - list_bonus_cost[value] >= 0) {
            ccpoints -= list_bonus_cost[value];
            if (list_bonus_cost[value] > 0) {
                negcount -= list_bonus_cost[value];
            }
            GET_BONUS(ch, value) = 0;
            display_bonus_menu(type);
            sendText(fmt::format("@GYou cancel your selection of {}.\r\n", list_bonus[value]));
            return;
        }
        if (type == 0) {
            if (value > 25) {
                display_bonus_menu(type);
                sendText("@RYou are not in the negatives menu, enter B to switch.\r\n");
                return;
            } else if (ccpoints + list_bonus_cost[value] < 0) {
                display_bonus_menu(type);
                sendText(fmt::format("@RYou do not have enough points for {}.\r\n", list_bonus[value]));
                return;
            } else if (!opp_bonus(value, type)) {
                return;
            } else if (list_bonus_cost[value] < 0) {
                ccpoints += list_bonus_cost[value];
                GET_BONUS(ch, value) = 1;
                display_bonus_menu(type);
                sendText(fmt::format("@GYou select the bonus {}\r\n", list_bonus[value]));
                return;
            }
        } else {

            if (value < 26) {
                display_bonus_menu(type);
                sendText("@RYou are not in the bonuses menu, enter B to switch.\r\n");
                return;
            }

            int x, count = 0;

            for (x = 14; x < 52; x++) {
                if (GET_BONUS(ch, x) > 1) {
                    count += list_bonus_cost[x];
                }
            }

            if (list_bonus_cost[value] + count > 10) {
                display_bonus_menu(type);
                sendText("@RYou can not have more than +10 points from negatives.\r\n");
                return;
            } else if (!opp_bonus(value, type)) {
                return;
            } else {
                ccpoints += list_bonus_cost[value];
                negcount += list_bonus_cost[value];
                GET_BONUS(ch, value) = 2;
                display_bonus_menu(type);
                sendText(fmt::format("@GYou select the negative {}\r\n", list_bonus[value]));
                return;
            }
        }
    }

    static const char *bonus[] = {
            "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost             @D[@G-2pts @D]", /* Bonus 0 */
            "Prodigy     - +25% Experience Gained Until Level 80                  @D[@G-5pts @D]", /* Bonus 1 */
            "Quick Study - Character auto-trains skills faster                    @D[@G-3pts @D]", /* Bonus 2 */
            "Die Hard    - Life Force's PL regen doubled, but cost is the same    @D[@G-6pts @D]", /* Bonus 3 */
            "Brawler     - Physical attacks do 20% more damage                    @D[@G-4pts @D]", /* Bonus 4 */
            "Destroyer   - Damaged Rooms act as regen rooms for you               @D[@G-3pts @D]", /* Bonus 5 */
            "Hard Worker - Physical rewards better + activity drains less stamina @D[@G-3pts @D]", /* Bonus 6 */
            "Healer      - Heal/First-aid/Vigor/Repair restore +10%               @D[@G-3pts @D]", /* Bonus 7 */
            "Loyal       - +20% Experience When Grouped As Follower               @D[@G-2pts @D]", /* Bonus 8 */
            "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%     @D[@G-5pts @D]", /* Bonus 9 */
            "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75% @D[@G-5pts @D]", /* Bonus 10 */
            "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%       @D[@G-5pts @D]", /* Bonus 11 */
            "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%      @D[@G-4pts @D]", /* Bonus 12 */
            "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%        @D[@G-6pts @D]", /* Bonus 13 */
            "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%       @D[@G-5pts @D]", /* Bonus 14 */
            "Thick Skin  - -20% Physical and -10% ki dmg received                 @D[@G-5pts @D]", /* Bonus 15 */
            "Recipe Int. - Food cooked by you lasts longer/heals better           @D[@G-2pts @D]", /* Bonus 16 */
            "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn         @D[@G-4pts @D]", /* Bonus 17 */
            "Powerhitter - 15% critical hits will be x4 instead of x2             @D[@G-4pts @D]", /* Bonus 18 */
            "Healthy     - 40% chance to recover from ill effects when sleeping   @D[@G-3pts @D]", /* Bonus  19 */
            "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis    @D[@G-2pts @D]", /* Bonus  20 */
            "Evasive     - +15% to dodge rolls                                    @D[@G-3pts @D]", /* Bonus  21 */
            "The Wall    - +20% chance to block                                   @D[@G-3pts @D]", /* Bonus  22 */
            "Accurate    - +20% chance to hit physical, +10% to hit with ki       @D[@G-4pts @D]", /* Bonus  23 */
            "Energy Leech- -2% ki damage received for every 5 character levels,   @D[@G-5pts @D]\n                  @cas long as you can take that ki to your charge pool.@D        ", /* Bonus  24*/
            "Good Memory - +2 Skill Slots initially, +1 every 20 levels after     @D[@G-6pts @D]", /* Bonus 25 */
            "Soft Touch  - Half damage for all hit locations                      @D[@G+5pts @D]", /* Neg 26 */
            "Late Sleeper- Can only wake automatically. 33% every hour if maxed   @D[@G+5pts @D]", /* Neg 27 */
            "Impulse Shop- +25% shop costs                                        @D[@G+3pts @D]", /* Neg 28 */
            "Sickly      - Suffer from harmful effects longer                     @D[@G+5pts @D]", /* Neg 29 */
            "Punching Bag- -15% to dodge rolls                                    @D[@G+3pts @D]", /* Neg 30 */
            "Pushover    - -20% block chance                                      @D[@G+3pts @D]", /* Neg 31 */
            "Poor D. Perc- -20% chance to hit with physical, -10% with ki         @D[@G+4pts @D]", /* Neg 32 */
            "Thin Skin   - +20% physical and +10% ki damage received              @D[@G+4pts @D]", /* Neg 33 */
            "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned            @D[@G+5pts @D]", /* Neg 34 */
            "Energy Int. - +2% ki damage received for every 5 character levels,   @D[@G+6pts @D]\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
            "Coward      - Can't Attack Enemy With 150% Your Powerlevel           @D[@G+6pts @D]", /* Neg 36 */
            "Arrogant    - Cannot Suppress                                        @D[@G+1pt  @D]", /* Neg 37 */
            "Unfocused   - Charge concentration randomly breaks                   @D[@G+3pts @D]", /* Neg 38 */
            "Slacker     - Physical activity drains more stamina                  @D[@G+3pts @D]", /* Neg 39 */
            "Slow Learner- Character auto-trains skills slower                    @D[@G+3pts @D]", /* Neg 40 */
            "Masochistic - Defense Skills Cap At 75                               @D[@G+5pts @D]", /* Neg 41 */
            "Mute        - Can't use IC speech related commands                   @D[@G+4pts @D]", /* Neg 42 */
            "Wimp        - Strength is capped at 45                               @D[@G+6pts @D]", /* Neg 43 */
            "Dull        - Intelligence is capped at 45                           @D[@G+6pts @D]", /* Neg 44 */
            "Foolish     - Wisdom is capped at 45                                 @D[@G+6pts @D]", /* Neg 45 */
            "Clumsy      - Agility is capped at 45                                @D[@G+3pts @D]", /* Neg 46 */
            "Slow        - Speed is capped at 45                                  @D[@G+6pts @D]", /* Neg 47 */
            "Frail       - Constitution capped at 45                              @D[@G+4pts @D]", /* Neg 48 */
            "Sadistic    - Half Experience Gained For Quick Kills                 @D[@G+3pts @D]", /* Neg 49 */
            "Loner       - Can't Group, +5% Train gains, +10% to physical gains   @D[@G+2pts @D]", /* Neg 50 */
            "Bad Memory  - -5 Skill Slots                                         @D[@G+6pts @D]"  /* Neg 51 */
    };

/* This is the bonus/negatives menu for Character Creation */
    void ChargenParser::display_bonus_menu(int type) {

        int BonusCount = 26, NegCount = 52, x = 0, y = 0;

        if (type == 0) {
            sendText("\r\n@YBonus Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
            for (x = 0; x < BonusCount; x++) {
                sendText(fmt::format("@C{:-2}@D)@c {} <@g{}@D>\n", x + 1, bonus[x], GET_BONUS(ch, x) > 0 ? "X" : " "));
            }
            sendText("\n");
        }

        if (type == 1) {
            y = BonusCount;
            sendText("@YNegative Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
            while (y < NegCount) {
                sendText(fmt::format("@R{:-2}@D)@r {} <@g{}@D>\n", y - 14, bonus[y], GET_BONUS(ch, y) > 0 ? "X" : " "));
                y += 1;
            }
        }

        if (type == 0) {
            sendText("\n@CN@D)@c Show Negatives@n\n");
        } else {
            sendText("\n@CB@D)@c Show Bonuses@n\n");
        }
        sendText("@CX@D)@c Exit Traits Section and complete your character@n\n");
        sendText(fmt::format("@D---------------------------------------\n[@WCurrent Points Pool@W: @y{}@D] [@WPTS From Neg@W: @y{}@D]@w\n",
                  ccpoints, negcount));
    }

/* Return -1 if not an acceptable menu option *
 * Return 31 if selection is X                 *
 * Return other value if Bonus/Negative        */

    int parse_bonuses(const std::string &arg) {
        int value = -1, ident = -1;

        if(arg.empty()) return value;

        switch (toupper(arg[0])) {
            case 'B':
                value = 53;
                break;
            case 'N':
                value = 54;
                break;
            case 'X':
                value = 55;
                break;
        }

        if (value < 52) {
            ident = atoi(arg.c_str());
        }

        switch (ident) {
            /* Valid Selections */
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
            case 38:
            case 39:
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
            case 48:
            case 49:
            case 50:
            case 51:
                value = ident - 1;
                break;

                /* Invalid Selections */
            default:
                value = -1;
                break;
        }

        return (value);
    }


    ChargenParser::ChargenParser(std::shared_ptr<Connection>& co) : ConnectionParser(co) {
        ch = new char_data();
    }

    void ChargenParser::start() {
        parse("");
    }

    ChargenParser::~ChargenParser() {
        // Completing chargen should set our state to -1.
        // That will prevent the character from being freed.
        // Because in all other cases, we don't want this char_data to be laying around if
        // chargen is canceled somehow.
        if(state != -1 && ch) {
            free_char(ch);
        }
    }


    race::RaceMap ChargenParser::valid_races() {
        return race::valid_for_sex_pc(GET_SEX(ch));
    }

    void ChargenParser::display_races_sub() {
        auto v_races = valid_races();
        int i = 0;
        for (const auto &r: v_races)
            sendText(fmt::format("@C{:^15}@D[@R{} RPP@D]@n{}", r.second->getName(), r.second->getRPPCost(),
                      !(++i % 2) ? "\r\n" : "   "));
    }

    void ChargenParser::display_races() {
        sendText("\r\n@YRace SELECTION menu:\r\n@D---------------------------------------\r\n@n");
        display_races_sub();

        sendText("\n @BR@W) @CRandom Race Selection!\r\n@n");
        sendText("\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
        sendText("\n@WRace: @n");
    }

    sensei::SenseiMap ChargenParser::valid_classes() {
        return sensei::valid_for_race_pc(ch);
    }

    void ChargenParser::display_classes_sub() {
        auto v_classes = valid_classes();
        int i = 0;
        for (const auto &s: v_classes)
            sendText(fmt::format("@C{}@n{}", s.second->getName().c_str(), !(++i % 2) ? "\r\n" : "	"));
    }

    void ChargenParser::display_classes() {
        sendText("\r\n@YSensei SELECTION menu:\r\n@D--------------------------------------\r\n@n");
        display_classes_sub();

        sendText("\n @BR@W) @CRandom Sensei Selection!\r\n@n");
        sendText("\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
        sendText("\n@WSensei: @n");
    }

    void ChargenParser::display_races_help() {
        sendText("\r\n@YRace HELP menu:\r\n@G--------------------------------------------\r\n@n");
        display_races_sub();

        sendText("\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
        sendText("\n@WHelp on Race #: @n");
    }

    void ChargenParser::display_classes_help() {
        sendText("\r\n@YClass HELP menu:\r\n@G-------------------------------------------\r\n@n");
        display_classes_sub();

        sendText("\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
        sendText("\n@WHelp on Class #: @n");
    }

    void ChargenParser::parse(const std::string &arg) {
        race::Race *chosen_race;
        race::RaceMap v_races;
        sensei::Sensei *chosen_sensei;
        sensei::SenseiMap v_sensei;
        bool penalty = false, moveon = false;
        int roll = rand_number(1, 6);
        int value;

        struct char_data *found;

        switch(state) {
            case CON_GET_NAME:
                if(arg.empty()) {
                    sendText("\r\nPick a good name for this character.\r\nName: ");
                    return;
                } else {
                    maybeName = boost::trim_copy(arg);
                    if(findPlayer(maybeName)) {
                        sendText("\r\nUnfortunately that name's already taken.");
                        sendText("\r\nPick a good name for this character.\r\nName: ");
                        return;
                    }
                    auto result = validate_pc_name(maybeName);
                    if(!result.first) {
                        sendText("\r\n" + result.second.value() + "\r\n");
                        maybeName.clear();
                        sendText("\r\nPick a good name for this character.\r\nName: ");
                        return;
                    }
                    maybeName = result.second.value();
                    sendText(fmt::format("\r\n{}, huh? Are you sure?\r\n(Y/N): ", maybeName));
                    state = CON_NAME_CNFRM;
                    break;
                }
            case CON_NAME_CNFRM:
                if(boost::iequals(arg, "Y")) {
                    ch->name = strdup(maybeName.c_str());
                    state = CON_QRACE;
                    display_races();
                    break;
                } else if(boost::iequals(arg, "N")) {
                    maybeName.clear();
                    state = CON_GET_NAME;
                    sendText("\r\nOkay, let's try that again then.");
                    sendText("\r\nPick a good name for this character.\r\nName: ");
                    return;
                } else {
                    sendText(fmt::format("\r\n{}, huh? Are you sure?\r\n(Y/N): ", maybeName));
                    return;
                }
                break;
            case CON_QRACE:
            case CON_RACE_HELP:
                switch (arg.size()) {
                    case 0:
                        sendText("\r\nThat's not a race.\r\nRace: ");
                        return;
                    case 1:
                        switch(arg[0]) {
                            case 't':
                            case 'T':
                                switch (state) {
                                    case CON_QRACE:
                                        display_races_help();
                                        state = CON_RACE_HELP;
                                        break;
                                    case CON_RACE_HELP:
                                        display_races();
                                        state = CON_QRACE;
                                        break;
                                }
                                return;
                            default:
                                sendText("\r\nThat's not a race.\r\nRace: ");
                                return;
                        }
                        break;
                    default:
                        v_races = valid_races();
                        chosen_race = race::find_race_map(arg, v_races);
                        if (!chosen_race) {
                            sendText("\r\nThat's not a race.\r\nRace: ");
                            return;
                        }

                        switch (state) {
                            case CON_QRACE:
                                if (chosen_race->getRPPCost()) {
                                    sendText(fmt::format("\r\n{} RPP will be paid upon your first level up.\r\n",
                                              chosen_race->getRPPCost()));
                                }
                                ch->race = chosen_race;
                                break;
                            case CON_RACE_HELP:
                                show_help(conn, chosen_race->getName().c_str());
                                chosen_sensei = nullptr;
                                return;
                        }
                }

                if (IS_HALFBREED(ch)) {
                    sendText("@YWhat race do you prefer to by identified with?\r\n");
                    sendText("@cThis controls how others first view you and whether you start with\na tail and how fast it regrows when missing.\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Human\n@B2@W)@C Saiyan\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_RACIAL;
                } else if (IS_ANDROID(ch)) {
                    sendText("@YWhat do you want to be identified as at first glance?\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Android\n@B2@W)@C Human\n@B3@W)@C Robotic Humanoid\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_RACIAL;
                } else if (IS_NAMEK(ch)) {
                    GET_SEX(ch) = SEX_NEUTRAL;
                    state = CON_QSEX;
                } else {
                    sendText("\r\n@wWhat is your sex @W(@BM@W/@MF@W/@GN@W)@w?@n");
                    state = CON_QSEX;
                }
                break;

            case CON_RACIAL:
                switch (arg[0]) {
                    case '1':
                        ch->racial_pref = 1;
                        break;
                    case '2':
                        ch->racial_pref = 2;
                        break;
                    case '3':
                        if (IS_HALFBREED(ch)) {
                            sendText("That is not an acceptable option.\r\n");
                            return;
                        } else {
                            ch->racial_pref = 3;
                        }
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@wWhat is your sex @W(@BM@W/@MF@W/@GN@W)@w?@n");
                state = CON_QSEX;
                break;

            case CON_HAIRL:                /* query hair length */
                if (IS_HUMAN(ch) || IS_SAIYAN(ch) || IS_KONATSU(ch) ||
                    IS_MUTANT(ch) || IS_ANDROID(ch) || IS_KAI(ch) ||
                    IS_HALFBREED(ch) || IS_TRUFFLE(ch) ||
                    (IS_HOSHIJIN(ch) && IS_FEMALE(ch))) {
                    switch(arg[0]) {
                        case '1':
                            ch->hairl = HAIRL_BALD;
                            ch->hairc = HAIRC_NONE;
                            ch->hairs = HAIRS_NONE;
                            break;
                        case '2':
                            ch->hairl = HAIRL_SHORT;
                            break;
                        case '3':
                            ch->hairl = HAIRL_MEDIUM;
                            break;
                        case '4':
                            ch->hairl = HAIRL_LONG;
                            break;
                        case '5':
                            ch->hairl = HAIRL_RLONG;
                            break;
                        default:
                            sendText("That is not an acceptable option.\r\n");
                            return;
                    }
                    if (ch->hairl == HAIRL_BALD) {
                        sendText("@YSkin color SELECTION menu:\r\n");
                        sendText("@D---------------------------------------@n\r\n");
                        sendText("@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
                        sendText("@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
                        sendText("@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
                        sendText("@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
                        sendText("@w\r\nMake a selection:@n\r\n");
                        state = CON_SKIN;
                    } else {
                        sendText("@YHair color SELECTION menu:\r\n");
                        sendText("@D---------------------------------------@n\r\n");
                        sendText("@B1@W)@C Black  @B2@W)@C Brown  @B3@W)@C Blonde\r\n");
                        sendText("@B4@W)@C Grey   @B5@W)@C Red    @B6@W)@C Orange@n\r\n");
                        sendText("@B7@W)@C Green  @B8@W)@C Blue   @B9@W)@C Pink\r\n");
                        sendText("@BA@W)@C Purple @BB@W)@C Silver @BC@W)@C Crimson@n\r\n");
                        sendText("@BD@W)@C White@n\r\n");
                        sendText("@w\r\nMake a selection:@n\r\n");
                        state = CON_HAIRC;
                    }
                } else {
                    if (IS_DEMON(ch) || IS_ICER(ch)) {
                        switch(arg[0]) {
                            case '1':
                                ch->hairl = HAIRL_BALD;
                                break;
                            case '2':
                                ch->hairl = HAIRL_SHORT;
                                break;
                            case '3':
                                ch->hairl = HAIRL_MEDIUM;
                                break;
                            case '4':
                                ch->hairl = HAIRL_LONG;
                                break;
                            case '5':
                                ch->hairl = HAIRL_RLONG;
                                break;
                            default:
                                sendText("That is not an acceptable option.\r\n");
                                return;
                        }
                        ch->hairc = HAIRC_NONE;
                        ch->hairs = HAIRS_NONE;
                        sendText("@YSkin color SELECTION menu:\r\n");
                        sendText("@D---------------------------------------@n\r\n");
                        sendText("@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
                        sendText("@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
                        sendText("@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
                        sendText("@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
                        sendText("@w\r\nMake a selection:@n\r\n");
                        state = CON_SKIN;
                    }
                    if (IS_MAJIN(ch) || IS_NAMEK(ch) || IS_ARLIAN(ch)) {
                        switch(arg[0]) {
                            case '1':
                                ch->hairl = HAIRL_BALD;
                                break;
                            case '2':
                                ch->hairl = HAIRL_SHORT;
                                break;
                            case '3':
                                ch->hairl = HAIRL_MEDIUM;
                                break;
                            case '4':
                                ch->hairl = HAIRL_LONG;
                                break;
                            case '5':
                                ch->hairl = HAIRL_RLONG;
                                break;
                            default:
                                sendText("That is not an acceptable option.\r\n");
                                return;
                        }
                        if (IS_ARLIAN(ch) && IS_FEMALE(ch)) {
                            sendText("@YWing color SELECTION menu:\r\n");
                            sendText("@D---------------------------------------@n\r\n");
                            sendText("@B1@W)@C Black  @B2@W)@C Brown  @B3@W)@C Blonde\r\n");
                            sendText("@B4@W)@C Grey   @B5@W)@C Red    @B6@W)@C Orange@n\r\n");
                            sendText("@B7@W)@C Green  @B8@W)@C Blue   @B9@W)@C Pink\r\n");
                            sendText("@BA@W)@C Purple @BB@W)@C Silver @BC@W)@C Crimson@n\r\n");
                            sendText("@BD@W)@C White@n\r\n");
                            sendText("@w\r\nMake a selection:@n\r\n");
                            state = CON_HAIRC;
                        } else {
                            ch->hairc = HAIRC_NONE;
                            ch->hairs = HAIRS_NONE;
                            sendText("@YSkin color SELECTION menu:\r\n");
                            sendText("@D---------------------------------------@n\r\n");
                            sendText("@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
                            sendText("@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
                            sendText("@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
                            sendText("@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
                            sendText("@w\r\nMake a selection:@n\r\n");
                            state = CON_SKIN;
                        }
                    } else {
                        ch->hairc = HAIRC_NONE;
                        ch->hairs = HAIRS_NONE;
                        state = CON_SKIN;
                    }
                }
                break;

            case CON_HAIRC:     /* query hair color */
                switch(arg[0]) {
                    case '1':
                        ch->hairc = HAIRC_BLACK;
                        break;
                    case '2':
                        ch->hairc = HAIRC_BROWN;
                        break;
                    case '3':
                        ch->hairc = HAIRC_BLONDE;
                        break;
                    case '4':
                        ch->hairc = HAIRC_GREY;
                        break;
                    case '5':
                        ch->hairc = HAIRC_RED;
                        break;
                    case '6':
                        ch->hairc = HAIRC_ORANGE;
                        break;
                    case '7':
                        ch->hairc = HAIRC_GREEN;
                        break;
                    case '8':
                        ch->hairc = HAIRC_BLUE;
                        break;
                    case '9':
                        ch->hairc = HAIRC_PINK;
                        break;
                    case 'A':
                        ch->hairc = HAIRC_PURPLE;
                        break;
                    case 'a':
                        ch->hairc = HAIRC_PURPLE;
                        break;
                    case 'B':
                        ch->hairc = HAIRC_SILVER;
                        break;
                    case 'b':
                        ch->hairc = HAIRC_SILVER;
                        break;
                    case 'C':
                        ch->hairc = HAIRC_CRIMSON;
                        break;
                    case 'c':
                        ch->hairc = HAIRC_CRIMSON;
                        break;
                    case 'D':
                        ch->hairc = HAIRC_WHITE;
                        break;
                    case 'd':
                        ch->hairc = HAIRC_WHITE;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                if (IS_ARLIAN(ch)) {
                    ch->hairs = HAIRS_NONE;
                    sendText("@YSkin color SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
                    sendText("@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
                    sendText("@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
                    sendText("@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_SKIN;
                } else {
                    sendText("@YHair style SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Plain     @B2@W)@C Mohawk    @B3@W)@C Spiky\r\n");
                    sendText("@B4@W)@C Curly     @B5@W)@C Uneven    @B6@W)@C Ponytail@n\r\n");
                    sendText("@B7@W)@C Afro      @B8@W)@C Fade      @B9@W)@C Crew Cut\r\n");
                    sendText("@BA@W)@C Feathered @BB@W)@C Dread Locks@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_HAIRS;
                }
                break;

            case CON_HAIRS:
                switch(arg[0]) {
                    case '1':
                        ch->hairs = HAIRS_PLAIN;
                        break;
                    case '2':
                        ch->hairs = HAIRS_MOHAWK;
                        break;
                    case '3':
                        ch->hairs = HAIRS_SPIKY;
                        break;
                    case '4':
                        ch->hairs = HAIRS_CURLY;
                        break;
                    case '5':
                        ch->hairs = HAIRS_UNEVEN;
                        break;
                    case '6':
                        ch->hairs = HAIRS_PONYTAIL;
                        break;
                    case '7':
                        ch->hairs = HAIRS_AFRO;
                        break;
                    case '8':
                        ch->hairs = HAIRS_FADE;
                        break;
                    case '9':
                        ch->hairs = HAIRS_CREW;
                        break;
                    case 'A':
                        ch->hairs = HAIRS_FEATHERED;
                        break;
                    case 'a':
                        ch->hairs = HAIRS_FEATHERED;
                        break;
                    case 'B':
                        ch->hairs = HAIRS_DRED;
                        break;
                    case 'b':
                        ch->hairs = HAIRS_DRED;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("@YSkin color SELECTION menu:\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
                sendText("@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
                sendText("@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
                sendText("@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_SKIN;
                break;

            case CON_SKIN:
                switch(arg[0]) {
                    case '1':
                        ch->skin = SKIN_WHITE;
                        break;
                    case '2':
                        ch->skin = SKIN_BLACK;
                        break;
                    case '3':
                        ch->skin = SKIN_GREEN;
                        break;
                    case '4':
                        ch->skin = SKIN_ORANGE;
                        break;
                    case '5':
                        ch->skin = SKIN_YELLOW;
                        break;
                    case '6':
                        ch->skin = SKIN_RED;
                        break;
                    case '7':
                        ch->skin = SKIN_GREY;
                        break;
                    case '8':
                        ch->skin = SKIN_BLUE;
                        break;
                    case '9':
                        ch->skin = SKIN_AQUA;
                        break;
                    case 'A':
                        ch->skin = SKIN_PINK;
                        break;
                    case 'a':
                        ch->skin = SKIN_PINK;
                        break;
                    case 'B':
                        ch->skin = SKIN_PURPLE;
                        break;
                    case 'b':
                        ch->skin = SKIN_PURPLE;
                        break;
                    case 'C':
                        ch->skin = SKIN_TAN;
                        break;
                    case 'c':
                        ch->skin = SKIN_TAN;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("@YEye color SELECTION menu:\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C Blue  @B2@W)@C Black  @B3@W)@C Green\r\n");
                sendText("@B4@W)@C Brown @B5@W)@C Red    @B6@W)@C Aqua@n\r\n");
                sendText("@B7@W)@C Pink  @B8@W)@C Purple @B9@W)@C Crimson\r\n");
                sendText("@BA@W)@C Gold  @BB@W)@C Amber  @BC@W)@C Emerald@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_EYE;
                break;

            case CON_EYE:

                switch(arg[0]) {
                    case '1':
                        ch->eye = EYE_BLUE;
                        break;
                    case '2':
                        ch->eye = EYE_BLACK;
                        break;
                    case '3':
                        ch->eye = EYE_GREEN;
                        break;
                    case '4':
                        ch->eye = EYE_BROWN;
                        break;
                    case '5':
                        ch->eye = EYE_RED;
                        break;
                    case '6':
                        ch->eye = EYE_AQUA;
                        break;
                    case '7':
                        ch->eye = EYE_PINK;
                        break;
                    case '8':
                        ch->eye = EYE_PURPLE;
                        break;
                    case '9':
                        ch->eye = EYE_CRIMSON;
                        break;
                    case 'A':
                        ch->eye = EYE_GOLD;
                        break;
                    case 'a':
                        ch->eye = EYE_GOLD;
                        break;
                    case 'B':
                        ch->eye = EYE_AMBER;
                        break;
                    case 'b':
                        ch->eye = EYE_AMBER;
                        break;
                    case 'C':
                        ch->eye = EYE_EMERALD;
                        break;
                    case 'c':
                        ch->eye = EYE_EMERALD;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                        break;
                }
                sendText("@YWhat do you want to be your most distinguishing feature:\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C My Eyes@n\r\n");
                if (IS_MAJIN(ch)) {
                    sendText("@B2@W)@C My Forelock@n\r\n");
                } else if (IS_NAMEK(ch) || IS_ARLIAN(ch)) {
                    sendText("@B2@W)@C My Antennae@n\r\n");
                } else if (IS_ICER(ch) || IS_DEMON(ch)) {
                    sendText("@B2@W)@C My Horns@n\r\n");
                } else if (GET_HAIRL(ch) == HAIRL_BALD) {
                    sendText("@B2@W)@C My Baldness@n\r\n");
                } else {
                    sendText("@B2@W)@C My Hair@n\r\n");
                }
                sendText("@B3@W)@C My Skin@n\r\n");
                sendText("@B4@W)@C My Height@n\r\n");
                sendText("@B5@W)@C My Weight@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");

                state = CON_DISTFEA;
                break;

            case CON_DISTFEA:
                switch(arg[0]) {
                    case '1':
                        ch->distfea = 0;
                        break;
                    case '2':
                        ch->distfea = 1;
                        break;
                    case '3':
                        ch->distfea = 2;
                        break;
                    case '4':
                        ch->distfea = 3;
                        break;
                    case '5':
                        ch->distfea = 4;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                        break;
                }
                sendText("@YWhat Height/Weight Range do you prefer:\r\n");
                sendText("@D---------------------------------------@n\r\n");
                if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                    sendText("@B1@W)@C 100-120cm, 25-30kg@n\r\n");
                    sendText("@B2@W)@C 120-140cm, 30-35kg@n\r\n");
                    sendText("@B3@W)@C 140-160cm, 35-45kg@n\r\n");
                    sendText("@B4@W)@C 160-180cm, 45-60kg@n\r\n");
                    sendText("@B5@W)@C 180-200cm, 60-80kg@n\r\n");
                    sendText("@B6@W)@C 200-220cm, 80-100kg@n\r\n");
                } else if (IS_ICER(ch)) {
                    sendText("@B1@W)@C 100-120cm, 25-30kg@n\r\n");
                    sendText("@B2@W)@C 120-140cm, 30-35kg@n\r\n");
                    sendText("@B3@W)@C 140-160cm, 35-45kg@n\r\n");
                } else {
                    sendText("@B1@W)@C 20-35cm, 5-8kg@n\r\n");
                    sendText("@B2@W)@C 35-40cm, 8-10kg@n\r\n");
                    sendText("@B3@W)@C 40-50cm, 10-12kg@n\r\n");
                    sendText("@B4@W)@C 50-60cm, 12-15kg@n\r\n");
                    sendText("@B5@W)@C 60-70cm, 15-18kg@n\r\n");
                }
                sendText("\n@WMake a selection:@n ");
                state = CON_HW;
                break;

            case CON_HW:
                switch(arg[0]) {
                    case '1':
                        if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                            ch->height = rand_number(100, 120);
                            ch->weight = rand_number(25, 30);
                        } else if (IS_ICER(ch)) {
                            ch->height = rand_number(100, 120);
                            ch->weight = rand_number(25, 30);
                        } else {
                            ch->height = rand_number(20, 35);
                            ch->weight = rand_number(5, 8);
                        }
                        break;
                    case '2':
                        if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                            ch->height = rand_number(120, 140);
                            ch->weight = rand_number(30, 35);
                        } else if (IS_ICER(ch)) {
                            ch->height = rand_number(120, 140);
                            ch->weight = rand_number(30, 35);
                        } else {
                            ch->height = rand_number(35, 40);
                            ch->weight = rand_number(8, 10);
                        }
                        break;
                    case '3':
                        if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                            ch->height = rand_number(140, 160);
                            ch->weight = rand_number(35, 45);
                        } else if (IS_ICER(ch)) {
                            ch->height = rand_number(140, 160);
                            ch->weight = rand_number(35, 45);
                        } else {
                            ch->height = rand_number(40, 50);
                            ch->weight = rand_number(10, 12);
                        }
                        break;
                    case '4':
                        if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                            ch->height = rand_number(160, 180);
                            ch->weight = rand_number(45, 60);
                        } else if (IS_ICER(ch)) {
                            sendText("That is not an acceptable option.\r\n");
                            return;
                        } else {
                            ch->height = rand_number(50, 60);
                            ch->weight = rand_number(12, 15);
                        }
                        break;
                    case '5':
                        if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                            ch->height = rand_number(180, 200);
                            ch->weight = rand_number(60, 80);
                        } else if (IS_ICER(ch)) {
                            sendText("That is not an acceptable option.\r\n");
                            return;
                        } else {
                            ch->height = rand_number(60, 70);
                            ch->weight = rand_number(15, 18);
                        }
                        break;
                    case '6':
                        if (!IS_TRUFFLE(ch) && !IS_ICER(ch)) {
                            ch->height = rand_number(200, 220);
                            ch->weight = rand_number(80, 100);
                        } else {
                            sendText("That is not an acceptable option.\r\n");
                            return;
                        }
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                        break;
                }
                sendText("@YAura color SELECTION menu:\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C White  @B2@W)@C Blue@n\r\n");
                sendText("@B3@W)@C Red    @B4@W)@C Green@n\r\n");
                sendText("@B5@W)@C Pink   @B6@W)@C Purple@n\r\n");
                sendText("@B7@W)@C Yellow @B8@W)@C Black@n\r\n");
                sendText("@B9@W)@C Orange@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_AURA;
                break;

            case CON_AURA:
                switch(arg[0]) {
                    case '1':
                        ch->aura = 0;
                        break;
                    case '2':
                        ch->aura = 1;
                        break;
                    case '3':
                        ch->aura = 2;
                        break;
                    case '4':
                        ch->aura = 3;
                        break;
                    case '5':
                        ch->aura = 4;
                        break;
                    case '6':
                        ch->aura = 5;
                        break;
                    case '7':
                        ch->aura = 6;
                        break;
                    case '8':
                        ch->aura = 7;
                        break;
                    case '9':
                        ch->aura = 8;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                        break;
                }

                display_classes();
                state = CON_QCLASS;
                break;

            case CON_Q1:
                ch->basepl = rand_number(30, 50);
                ch->basest = rand_number(30, 50);
                ch->baseki = rand_number(30, 50);
                if (IS_SAIYAN(ch)) {
                    ch->real_abils.str = rand_number(12, 18);
                    ch->real_abils.con = rand_number(12, 18);
                    ch->real_abils.wis = rand_number(8, 16);
                    ch->real_abils.intel = rand_number(8, 14);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 16);
                } else if (IS_HALFBREED(ch)) {
                    ch->real_abils.str = rand_number(10, 18);
                    ch->real_abils.con = rand_number(10, 18);
                    ch->real_abils.wis = rand_number(8, 18);
                    ch->real_abils.intel = rand_number(8, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_HUMAN(ch)) {
                    ch->real_abils.str = rand_number(8, 18);
                    ch->real_abils.con = rand_number(8, 18);
                    ch->real_abils.wis = rand_number(10, 18);
                    ch->real_abils.intel = rand_number(12, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_HOSHIJIN(ch)) {
                    ch->real_abils.str = rand_number(10, 18);
                    ch->real_abils.con = rand_number(9, 18);
                    ch->real_abils.wis = rand_number(9, 18);
                    ch->real_abils.intel = rand_number(9, 18);
                    ch->real_abils.cha = rand_number(10, 18);
                    ch->real_abils.dex = rand_number(9, 18);
                } else if (IS_NAMEK(ch)) {
                    ch->real_abils.str = rand_number(9, 18);
                    ch->real_abils.con = rand_number(9, 18);
                    ch->real_abils.wis = rand_number(12, 18);
                    ch->real_abils.intel = rand_number(8, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_ARLIAN(ch)) {
                    ch->real_abils.str = rand_number(15, 20);
                    ch->real_abils.con = rand_number(15, 20);
                    ch->real_abils.wis = rand_number(8, 16);
                    ch->real_abils.intel = rand_number(8, 16);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_ANDROID(ch)) {
                    ch->real_abils.str = rand_number(12, 18);
                    ch->real_abils.con = rand_number(8, 18);
                    ch->real_abils.wis = rand_number(8, 16);
                    ch->real_abils.intel = rand_number(8, 16);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_BIO(ch)) {
                    ch->real_abils.str = rand_number(14, 18);
                    ch->real_abils.con = rand_number(8, 18);
                    ch->real_abils.wis = rand_number(8, 18);
                    ch->real_abils.intel = rand_number(8, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 14);
                } else if (IS_MAJIN(ch)) {
                    ch->real_abils.str = rand_number(11, 18);
                    ch->real_abils.con = rand_number(14, 18);
                    ch->real_abils.wis = rand_number(8, 14);
                    ch->real_abils.intel = rand_number(8, 14);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 17);
                } else if (IS_TRUFFLE(ch)) {
                    ch->real_abils.str = rand_number(8, 14);
                    ch->real_abils.con = rand_number(8, 14);
                    ch->real_abils.wis = rand_number(8, 18);
                    ch->real_abils.intel = rand_number(14, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_KAI(ch)) {
                    ch->real_abils.str = rand_number(9, 18);
                    ch->real_abils.con = rand_number(8, 18);
                    ch->real_abils.wis = rand_number(14, 18);
                    ch->real_abils.intel = rand_number(10, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_ICER(ch)) {
                    ch->real_abils.str = rand_number(10, 18);
                    ch->real_abils.con = rand_number(12, 18);
                    ch->real_abils.wis = rand_number(8, 18);
                    ch->real_abils.intel = rand_number(8, 18);
                    ch->real_abils.cha = rand_number(8, 15);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_MUTANT(ch)) {
                    ch->real_abils.str = rand_number(9, 18);
                    ch->real_abils.con = rand_number(9, 18);
                    ch->real_abils.wis = rand_number(9, 18);
                    ch->real_abils.intel = rand_number(9, 18);
                    ch->real_abils.cha = rand_number(9, 18);
                    ch->real_abils.dex = rand_number(9, 18);
                } else if (IS_KANASSAN(ch)) {
                    ch->real_abils.str = rand_number(8, 16);
                    ch->real_abils.con = rand_number(8, 16);
                    ch->real_abils.wis = rand_number(12, 18);
                    ch->real_abils.intel = rand_number(12, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_DEMON(ch)) {
                    ch->real_abils.str = rand_number(11, 18);
                    ch->real_abils.con = rand_number(8, 18);
                    ch->real_abils.wis = rand_number(10, 18);
                    ch->real_abils.intel = rand_number(10, 18);
                    ch->real_abils.cha = rand_number(8, 18);
                    ch->real_abils.dex = rand_number(8, 18);
                } else if (IS_KONATSU(ch)) {
                    ch->real_abils.str = rand_number(10, 14);
                    ch->real_abils.con = rand_number(10, 14);
                    ch->real_abils.wis = rand_number(10, 16);
                    ch->real_abils.intel = rand_number(10, 14);
                    ch->real_abils.cha = rand_number(12, 18);
                    ch->real_abils.dex = rand_number(14, 18);
                }

                switch(arg[0]) {
                    case '1':
                        ch->basepl += roll_stats(5, 25);
                        ch->basest += roll_stats(8, 50);
                        ch->baseki += roll_stats(6, 50);
                        break;
                    case '2':
                        ch->basepl += roll_stats(5, 55);
                        ch->basest += roll_stats(8, 40);
                        ch->baseki += roll_stats(6, 40);
                        break;
                    case '3':
                        ch->basepl += roll_stats(5, 125);
                        ch->basest += roll_stats(8, 50);
                        ch->baseki += roll_stats(6, 40);
                        break;
                    case '4':
                        ch->basepl += roll_stats(5, 65);
                        ch->basest += roll_stats(8, 65);
                        ch->baseki += roll_stats(6, 65);
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_SKILLP);
                        break;
                    case '5':
                        ch->basepl += roll_stats(5, 75);
                        ch->basest += roll_stats(8, 100);
                        ch->baseki += roll_stats(6, 75);
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G2@W out of @g10@W)\r\n");
                sendText("@YAnswer the following question:\r\n");
                sendText("@wYou are faced with the strongest opponent you have ever\r\nfaced in your life. You both have beat each other to the\r\nlimits of both your strengths. A situation has presented \r\nan opportunity to win the fight, what do you do?\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C Kill my opponent in a very brutal fashion!@n\r\n");
                sendText("@B2@W)@C Disable my opponent, but spare their life.@n\r\n");
                sendText("@B3@W)@C Kill my opponent, I have no other choice.@n\r\n");
                sendText("@B4@W)@C Try to evade my opponent till I can get away.@n\r\n");
                sendText("@B5@W)@C Take their head clean off and bathe in their blood!@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q2;
                break;

            case CON_Q2:
                switch(arg[0]) {
                    case '1':
                        ch->alignment += -200;
                        break;
                    case '2':
                        ch->alignment += 100;
                        break;
                    case '3':
                        ch->alignment += 10;
                        break;
                    case '4':
                        ch->alignment += 0;
                        break;
                    case '5':
                        ch->alignment += -400;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G3@W out of @g10@W)\r\n");
                sendText("@YAnswer the following question:\r\n");
                sendText("@wYou are one day offered a means to gain incredible strength\r\nby some extraordinary means. The only problem is it requires\r\nthe lives of innocents to obtain. What do you do?\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C Hell yeah I take the opportunity to get stronger!@n\r\n");
                sendText("@B2@W)@C I refuse to gain unnatural strength.@n\r\n");
                sendText("@B3@W)@C I refuse to gain strength at the cost of the innocent.@n\r\n");
                sendText("@B4@W)@C I kill that many innocents before breakfast anyway...@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q3;
                break;
            case CON_Q3:
                switch(arg[0]) {
                    case '1':
                        ch->alignment += -100;
                        ch->basepl += 100;
                        ch->basest += 80;
                        ch->baseki += 10;
                        break;
                    case '2':
                        ch->alignment += 10;
                        ch->basepl += 25;
                        ch->basest += 25;
                        ch->baseki += 25;
                        break;
                    case '3':
                        ch->alignment += 50;
                        ch->basepl += 20;
                        ch->basest += 20;
                        ch->baseki += 20;
                        break;
                    case '4':
                        ch->alignment += -200;
                        ch->basepl += 100;
                        ch->basest += 100;
                        ch->baseki += 100;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G4@W out of @g10@W)\r\n");
                sendText("@YAnswer the following question:\r\n");
                sendText("@wOne day you are offered a way to make a lot of money, but in order\r\nto do so you will need to stop training for a whole month to\r\nhandle business. What do you do?\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C I take the opportunity, with more money I can train better later.@n\r\n");
                sendText("@B2@W)@C I refuse to waste my time. What I need is some nice hard training.@n\r\n");
                sendText("@B3@W)@C Hmm. With more money I can live better, certainly that is worth the time.@n\r\n");
                sendText("@B4@W)@C I choose to earn a little money while still training instead.@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q4;
                break;

            case CON_Q4:
                switch(arg[0]) {
                    case '1':
                        ch->gold += 1000;
                        ch->basepl -= rand_number(10, 30);
                        ch->basest -= rand_number(10, 30);
                        ch->baseki -= rand_number(10, 30);
                        break;
                    case '2':
                        ch->gold = 0;
                        ch->basepl += rand_number(50, 165);
                        ch->basest += rand_number(50, 165);
                        ch->baseki += rand_number(50, 165);
                        break;
                    case '3':
                        ch->gold = 2500;
                        ch->basepl -= rand_number(15, 25);
                        ch->basest -= rand_number(15, 25);
                        ch->baseki -= rand_number(15, 25);
                        break;
                    case '4':
                        ch->gold = 150;
                        ch->basepl += rand_number(25, 80);
                        ch->basest += rand_number(25, 80);
                        ch->baseki += rand_number(25, 80);
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G5@W out of @g10@W)\r\n");
                sendText("@YAnswer the following question:\r\n");
                sendText("@wYou are introduced to a new way of training one day, what do you do?\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C I prefer my way, it has worked so far.@n\r\n");
                sendText("@B2@W)@C I am open to new possibilites, sure.@n\r\n");
                sendText("@B3@W)@C I will at least try it, for a little while.@n\r\n");
                sendText("@B4@W)@C No way is superior to eating spinach everyday...@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q5;
                break;

            case CON_Q5:
                switch(arg[0]) {
                    case '1':
                        ch->basepl += rand_number(0, 40);
                        break;
                    case '2':
                        ch->basepl += rand_number(-30, 80);
                        break;
                    case '3':
                        ch->basepl += rand_number(-25, 60);
                        break;
                    case '4':
                        ch->basepl += rand_number(0, 50);
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G6@W out of @g10@W)\r\n");
                sendText("\r\n@YAnswer the following question:\r\n");
                sendText("@wYou have an enemy before you, what is your prefered method to attack him?@n\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C I prefer to defend rather than attack.@n\r\n");
                sendText("@B2@W)@C I prefer to throw a strong punch at the enemy's throat.@n\r\n");
                sendText("@B3@W)@C I prefer to send a devestating kick at the enemy's neck.@n\r\n");
                sendText("@B4@W)@C I prefer to smash them with a two-handed slam!@n\r\n");
                sendText("@B5@W)@C I prefer to throw one of my many energy attacks!@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q6;
                break;


            case CON_Q6:
                switch(arg[0]) {
                    case '1':
                        ch->basepl += rand_number(0, 15);
                        ch->basest += rand_number(0, 15);
                        ch->choice = 1;
                        break;
                    case '2':
                        ch->basepl += rand_number(0, 30);
                        ch->choice = 2;
                        break;
                    case '3':
                        ch->basepl += rand_number(0, 30);
                        ch->choice = 3;
                        break;
                    case '4':
                        ch->basepl += rand_number(0, 30);
                        ch->choice = 4;
                        break;
                    case '5':
                        ch->baseki += rand_number(0, 50);
                        ch->choice = 5;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G7@W out of @g10@W)\r\n");
                sendText("\r\n@YAnswer the following question:\r\n");
                sendText("@wYou are camped out one night in a field, the sky is clear and the\r\nstars visible. Looking at them, what thought crosses your mind?@n\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C One day, I am going to conquer every one of those.@n\r\n");
                sendText("@B2@W)@C I really wish I had brought that special someone along as this is a great night for romance@n\r\n");
                sendText("@B3@W)@C Those stars hold greater meaning to you than just planets, they dictate\r\nyour life or guide you and those arround you in one form or another.@n\r\n");
                sendText("@B4@W)@C You'd very much like to travel into space to get to one of these stars to study all that it holds.@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q7;
                break;

            case CON_Q7:
                switch(arg[0]) {
                    case '1':
                        ch->alignment += -10;
                        ch->real_abils.str += 1;
                        break;
                    case '2':
                        ch->alignment += +10;
                        ch->real_abils.cha += 1;
                        break;
                    case '3':
                        ch->real_abils.wis += 1;
                        break;
                    case '4':
                        ch->real_abils.intel += 1;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G8@W out of @g10@W)\r\n");
                sendText("\r\n@YAnswer the following question:\r\n");
                sendText("@wOne day, you are on the way home. You happen to walk past two\r\nof your best friends about to lock horns. Do you@n\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C You try to talk them out of it. With a silver tongue, you can stop the fight before it has began.@n\r\n");
                sendText("@B2@W)@C It's clear that they are quite chuffed about something, you could stop the fight now but they may just try again later. It's best to investigate and find out the source of the problem.@n\r\n");
                sendText("@B3@W)@C Pick the one you like the most and help them to win.@n\r\n");
                sendText("@B4@W)@C You try to sneak past them, reckoning no matter what you do there its a no win situation, so long as they don't see you.@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q8;
                break;

            case CON_Q8:
                switch(arg[0]) {
                    case '1':
                        ch->alignment += +10;
                        ch->real_abils.cha += 1;
                        break;
                    case '2':
                        ch->alignment += +20;
                        ch->real_abils.wis += 1;
                        break;
                    case '3':
                        ch->alignment += -10;
                        ch->real_abils.str += 1;
                        break;
                    case '4':
                        ch->alignment += -20;
                        ch->real_abils.dex += 1;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G9@W out of @g10@W)\r\n");
                sendText("\r\n@YAnswer the following question:\r\n");
                sendText("@wAs a kid you were confronted with this. On the way from the bakery,\r\na group of kids corner you in an alley and the leader demands that\r\nyou surrender your jam donut to him. Do you...@n\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C Throw the donut up in the air, hoping that the leader will pay enough attention to it so that you can get at least one good shot in.@n\r\n");
                sendText("@B2@W)@C Surrender the donut for now but come back later with your friends and gain your revenge.@n\r\n");
                sendText("@B3@W)@C Give him the donut, then return to the baker. You tell him a sob story and convince him to give you a replacement donut.@n\r\n");
                sendText("@B4@W)@C It's better just to do as he says and leave it to that. After all its only a donut.@n\r\n");
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_Q9;
                break;

            case CON_Q9:
                switch(arg[0]) {
                    case '1':
                        ch->real_abils.str += 1;
                        break;
                    case '2':
                        ch->alignment += -30;
                        ch->real_abils.wis += 1;
                        break;
                    case '3':
                        ch->alignment += -10;
                        ch->real_abils.cha += 1;
                        break;
                    case '4':
                        ch->alignment += -5;
                        ch->real_abils.intel += 1;
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }
                sendText("\r\n@WQuestion (@G10@W out of @g10@W)\r\n");
                sendText("\r\n@YAnswer the following question:\r\n");
                sendText("@wWhat do you wish your starting age to be?@n\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@B1@W)@C  8@n    @B2@W)@C 10@n\r\n");
                sendText("@B3@W)@C 12@n    @B4@W)@C 14@n\r\n");
                sendText("@B5@W)@C 16@n    @B6@W)@C 18@n\r\n");
                sendText("@B7@W)@C 20@n    @B8@W)@C 22@n\r\n");
                sendText("@B9@W)@C 24@n    @BA@W)@C 26@n\r\n");
                sendText("@BB@W)@C 28@n    @BC@W)@C 30@n\r\n");
                sendText("@BD@W)@C 40@n    @BE@W)@C 50@n\r\n");
                sendText("@BF@W)@C 60@n    @BG@W)@C 65@n\r\n");
                if (IS_KAI(ch) || IS_DEMON(ch) || IS_MAJIN(ch)) {
                    sendText("@BH@W)@C 500@n   @BI@W)@C 800@n\r\n");
                } else if (IS_NAMEK(ch)) {
                    sendText("@BH@W)@C 250@n   @BI@W)@C 400@n\r\n");
                } else {
                    sendText("@BH@W)@C 70@n    @BI@W)@C 75@n\r\n");
                }
                sendText("@w\r\nMake a selection:@n\r\n");
                state = CON_QX;
                break;

            case CON_QX:
                switch(arg[0]) {
                    case '1':
                        ch->time.birth = time(nullptr) - (8 * SECS_PER_MUD_YEAR);
                        break;
                    case '2':
                        ch->time.birth = time(nullptr) - (10 * SECS_PER_MUD_YEAR);
                        break;
                    case '3':
                        ch->time.birth = time(nullptr) - (12 * SECS_PER_MUD_YEAR);
                        break;
                    case '4':
                        ch->time.birth = time(nullptr) - (14 * SECS_PER_MUD_YEAR);
                        break;
                    case '5':
                        ch->time.birth = time(nullptr) - (16 * SECS_PER_MUD_YEAR);
                        break;
                    case '6':
                        ch->time.birth = time(nullptr) - (18 * SECS_PER_MUD_YEAR);
                        break;
                    case '7':
                        ch->time.birth = time(nullptr) - (20 * SECS_PER_MUD_YEAR);
                        break;
                    case '8':
                        ch->time.birth = time(nullptr) - (22 * SECS_PER_MUD_YEAR);
                        break;
                    case '9':
                        ch->time.birth = time(nullptr) - (24 * SECS_PER_MUD_YEAR);
                        break;
                    case 'A':
                    case 'a':
                        ch->time.birth = time(nullptr) - (26 * SECS_PER_MUD_YEAR);
                        break;
                    case 'B':
                    case 'b':
                        ch->time.birth = time(nullptr) - (28 * SECS_PER_MUD_YEAR);
                        break;
                    case 'C':
                    case 'c':
                        ch->time.birth = time(nullptr) - (30 * SECS_PER_MUD_YEAR);
                        break;
                    case 'D':
                    case 'd':
                        ch->time.birth = time(nullptr) - (40 * SECS_PER_MUD_YEAR);
                        break;
                    case 'E':
                    case 'e':
                        ch->time.birth = time(nullptr) - (50 * SECS_PER_MUD_YEAR);
                        break;
                    case 'F':
                    case 'f':
                        ch->time.birth = time(nullptr) - (60 * SECS_PER_MUD_YEAR);
                        break;
                    case 'G':
                    case 'g':
                        ch->time.birth = time(nullptr) - (65 * SECS_PER_MUD_YEAR);
                        break;
                    case 'H':
                    case 'h':
                        if (IS_KAI(ch) || IS_DEMON(ch) || IS_MAJIN(ch)) {
                            ch->time.birth = time(nullptr) - (500 * SECS_PER_MUD_YEAR);
                        } else if (IS_NAMEK(ch)) {
                            ch->time.birth = time(nullptr) - (250 * SECS_PER_MUD_YEAR);
                        } else {
                            ch->time.birth = time(nullptr) - (70 * SECS_PER_MUD_YEAR);
                        }
                        break;
                    case 'I':
                    case 'i':
                        if (IS_KAI(ch) || IS_DEMON(ch) || IS_MAJIN(ch)) {
                            ch->time.birth = time(nullptr) - (800 * SECS_PER_MUD_YEAR);
                        } else if (IS_NAMEK(ch)) {
                            ch->time.birth = time(nullptr) - (400 * SECS_PER_MUD_YEAR);
                        } else {
                            ch->time.birth = time(nullptr) - (75 * SECS_PER_MUD_YEAR);
                        }
                        break;
                    default:
                        sendText("That is not an acceptable option.\r\n");
                        return;
                }

                if (!IS_HOSHIJIN(ch)) {
                    ccpoints = 5;
                } else if (IS_BIO(ch)) {
                    ccpoints = 3;
                } else {
                    ccpoints = 10;
                }
                sendText("@C             Alignment Menu@n\r\n");
                sendText("@D---------------------------------------@n\r\n");
                sendText(fmt::format("@cCurrent Alignment@D: {}{}@n\r\n",
                          GET_ALIGNMENT(ch) < -50 ? "@R" : (GET_ALIGNMENT(ch) > 50 ? "@C" : "@G"),
                          disp_align(ch)));
                sendText("@YThis is the alignment your character has based on your choices.\r\n");
                sendText("Choose to keep this alignment with no penalty, or choose new\r\n");
                sendText("alignment and suffer a -5%s PL and -1 stat (random) penalty.\r\n@n");
                sendText("@D---------------------------------------@n\r\n");
                sendText("@BK@W) @wKeep This Alignment@n\r\n");
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

                state = CON_ALIGN;
                break;

            case CON_ALIGN:
                sendText("Choose: \r\n");
                switch(toupper(arg[0])) {
                    case 'K':
                        moveon = true;
                        break;
                    case '1':
                        GET_ALIGNMENT(ch) = 1000;
                        penalty = true;
                        break;
                    case '2':
                        GET_ALIGNMENT(ch) = 799;
                        penalty = true;
                        break;
                    case '3':
                        GET_ALIGNMENT(ch) = 599;
                        penalty = true;
                        break;
                    case '4':
                        GET_ALIGNMENT(ch) = 299;
                        penalty = true;
                        break;
                    case '5':
                        GET_ALIGNMENT(ch) = 0;
                        penalty = true;
                        break;
                    case '6':
                        GET_ALIGNMENT(ch) = -299;
                        penalty = true;
                        break;
                    case '7':
                        GET_ALIGNMENT(ch) = -599;
                        penalty = true;
                        break;
                    case '8':
                        GET_ALIGNMENT(ch) = -799;
                        penalty = true;
                        break;
                    case '9':
                        GET_ALIGNMENT(ch) = -1000;
                        penalty = true;
                        break;
                    default:
                        sendText("That is not an acceptable option! Choose again...\r\n");
                        return;
                }
                if (moveon == true) {
                    sendText("@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you will receive 200 PS in exchange.@n\r\n");
                    sendText("keep or forget: \r\n");
                    state = CON_SKILLS;
                } else if (penalty == true) {
                    ch->loseBasePLPercent(.2);

                    switch (roll) {
                        case 1:
                            ch->real_abils.str -= 1;
                        case 2:
                            ch->real_abils.con -= 1;
                        case 3:
                            ch->real_abils.wis -= 1;
                        case 4:
                            ch->real_abils.intel -= 1;
                        case 5:
                            ch->real_abils.cha -= 1;
                        case 6:
                            ch->real_abils.dex -= 1;
                            break;
                    }
                    sendText("@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you get 200 PS in exchange.@n\r\n");
                    sendText("keep or forget: \r\n");
                    state = CON_SKILLS;
                } else {
                    return;
                }
                break;

            case CON_QSEX:        /* query sex of new user         */
                if (!IS_NAMEK(ch)) {
                    switch(arg[0]) {
                        case 'm':
                        case 'M':
                            ch->sex = SEX_MALE;
                            break;
                        case 'f':
                        case 'F':
                            ch->sex = SEX_FEMALE;
                            break;
                        case 'n':
                        case 'N':
                            ch->sex = SEX_NEUTRAL;
                            break;
                        default:
                            sendText("That is not a sex..\r\n"
                                      "What IS your sex? ");
                            return;
                    }
                }
                if (IS_HUMAN(ch) || IS_SAIYAN(ch) || IS_KONATSU(ch) ||
                    IS_MUTANT(ch) || IS_ANDROID(ch) || IS_KAI(ch) ||
                    IS_HALFBREED(ch) || IS_TRUFFLE(ch) ||
                    (IS_HOSHIJIN(ch) && IS_FEMALE(ch))) {
                    sendText("@YHair Length SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Bald  @B2@W)@C Short  @B3@W)@C Medium\r\n");
                    sendText("@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_HAIRL;
                } else if (IS_DEMON(ch) || IS_ICER(ch)) {
                    sendText("@YHorn Length SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C None  @B2@W)@C Short  @B3@W)@C Medium\r\n");
                    sendText("@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_HAIRL;
                } else if (IS_MAJIN(ch)) {
                    sendText("@YForelock Length SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Tiny  @B2@W)@C Short  @B3@W)@C Medium\r\n");
                    sendText("@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_HAIRL;
                } else if (IS_NAMEK(ch) || IS_ARLIAN(ch)) {
                    sendText("@YAntenae Length SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Tiny  @B2@W)@C Short  @B3@W)@C Medium\r\n");
                    sendText("@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_HAIRL;
                } else {
                    sendText("@YSkin color SELECTION menu:\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
                    sendText("@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
                    sendText("@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
                    sendText("@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_SKIN;
                }
                break;

            case CON_SKILLS:
                if(arg.empty()) {
                    sendText("keep or forget: \r\n");
                    return;
                } else if (!strcasecmp(arg.c_str(), "keep")) {
                    if (!IS_BIO(ch) && !IS_MUTANT(ch)) {
                        display_bonus_menu(0);
                        sendText("@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
                        sendText("@wChoose: ");
                        state = CON_BONUS;
                    } else if (IS_MUTANT(ch)) {
                        sendText("\n@RSelect a mutation. A second will be chosen automatically..\n");
                        sendText("@D--------------------------------------------------------@n\n");
                        sendText("@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n");
                        sendText("@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n");
                        sendText("@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
                        sendText("@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
                        sendText("@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
                        sendText("@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
                        sendText("@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
                        sendText("@B 8@W) @CRubbery Body        @c-10%s of physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n");
                        sendText("@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
                        sendText("@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n");
                        sendText("@wChoose: ");
                        ch->genome[0] = 0;
                        ch->genome[1] = 0;
                        state = CON_GENOME;
                    } else {
                        sendText("\n@RSelect two genomes to be your primary DNA strains.\n");
                        sendText("@D--------------------------------------------------------@n\n");
                        sendText("@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
                        sendText("@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
                        sendText("@B3@W) @CNamek   @c- @CNo food needed@n\n");
                        sendText("@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n");
                        sendText("@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
                        sendText("@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
                        sendText("@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
                        sendText("@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n");
                        sendText("@wChoose: ");
                        ch->genome[0] = 0;
                        ch->genome[1] = 0;
                        state = CON_GENOME;
                    }
                } else if (!strcasecmp(arg.c_str(), "forget")) {
                    if (!IS_BIO(ch) && !IS_MUTANT(ch)) {
                        GET_PRACTICES(ch) += 200;
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_FORGET);
                        display_bonus_menu(0);
                        sendText("@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
                        sendText("@wChoose: ");
                        state = CON_BONUS;
                    } else if (IS_MUTANT(ch)) {
                        GET_PRACTICES(ch) += 200;
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_FORGET);
                        sendText("\n@RSelect a mutation. A second will be chosen automatically..\n");
                        sendText("@D--------------------------------------------------------@n\n");
                        sendText("@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n");
                        sendText("@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n");
                        sendText("@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
                        sendText("@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
                        sendText("@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
                        sendText("@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
                        sendText("@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
                        sendText("@B 8@W) @CRubbery Body        @c-10%s less physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n");
                        sendText("@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
                        sendText("@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n");
                        sendText("@wChoose: ");
                        ch->genome[0] = 0;
                        ch->genome[1] = 0;
                        state = CON_GENOME;
                    } else {
                        GET_PRACTICES(ch) += 200;
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_FORGET);
                        sendText("\n@RSelect two genomes to be your primary DNA strains.\n");
                        sendText("@D--------------------------------------------------------@n\n");
                        sendText("@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
                        sendText("@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
                        sendText("@B3@W) @CNamek   @c- @CNo food needed@n\n");
                        sendText("@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n");
                        sendText("@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
                        sendText("@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
                        sendText("@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
                        sendText("@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n");
                        sendText("@wChoose: ");
                        ch->genome[0] = 0;
                        ch->genome[1] = 0;
                        state = CON_GENOME;
                    }
                } else {
                    sendText("keep or forget: \r\n");
                    return;
                }
                break;

            case CON_GENOME:
                if (IS_MUTANT(ch)) {
                    const char *display_genome[11] = {"Unselected", /* 0 */
                                                      "Extreme Speed", /* 1 */
                                                      "Increased Cell Regen", /* 2 */
                                                      "Extreme Reflexes", /* 3 */
                                                      "Infravision", /* 4 */
                                                      "Natural Camo", /* 5 */
                                                      "Limb Regen", /* 6 */
                                                      "Poisonous", /* 7 */
                                                      "Rubbery Body", /* 8 */
                                                      "Innate Telepathy", /* 9 */
                                                      "Natural Energy" /* 10 */

                    };

                    sendText("\n@RSelect a mutation. A second will be chosen automatically..\n");
                    sendText("@D--------------------------------------------------------@n\n");
                    sendText("@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n");
                    sendText("@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n");
                    sendText("@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
                    sendText("@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
                    sendText("@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
                    sendText("@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
                    sendText("@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
                    sendText("@B 8@W) @CRubbery Body        @c-10%s less physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n");
                    sendText("@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
                    sendText("@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n");
                    sendText("\n@wChoose: ");

                    if (ch->genome[0] > 0 && ch->genome[1] <= 0) {
                        int num = rand_number(1, 8);
                        if (num == ch->genome[0]) {
                            while (num == ch->genome[0]) {
                                num = rand_number(1, 8);
                            }
                        }
                        if (num == 3) {
                            ch->real_abils.dex += 10;
                        }
                        sendText(fmt::format("@CRolling second mutation... Your second mutation is @D[@Y{}@D]@n\r\n",
                                  display_genome[num]));
                        ch->genome[1] = num;
                        return;
                    } else if (ch->genome[1] > 0) {
                        display_bonus_menu(0);
                        sendText("@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
                        sendText("\n@wChoose: ");
                        state = CON_BONUS;
                    } else if(arg.empty()) {
                        sendText("That is not an acceptable choice!\r\n");
                        sendText("\n@wChoose: ");
                        return;
                    } else {
                        int choice = atoi(arg.c_str());
                        if (choice < 1 || choice > 10) {
                            sendText("That is not an acceptable choice!\r\n");
                            return;
                        } else {
                            sendText(fmt::format("@CYou have chosen the mutation @D[@Y{}@D]@n\r\n", display_genome[choice]));
                            ch->genome[0] = choice;
                            if (choice == 3) {
                                ch->real_abils.dex += 10;
                            } else if (choice == 9) {
                                SET_SKILL(ch, SKILL_TELEPATHY, 50);
                            }
                            return;
                        }
                    }
                } else if (ch->genome[1] > 0) {
                    display_bonus_menu(0);
                    sendText("@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
                    sendText("@wChoose: ");
                    state = CON_BONUS;
                } else if(arg.empty()) {
                    const char *display_genome[9] = {"Unselected", /* 0 */
                                                     "Human", /* 1 */
                                                     "Saiyan", /* 2 */
                                                     "Namek", /* 3 */
                                                     "Icer", /* 4 */
                                                     "Truffle", /* 5 */
                                                     "Arlian", /* 6 */
                                                     "Kai", /* 7 */
                                                     "Konatsu" /* 8 */

                    };
                    sendText("@RSelect two genomes to be your primary DNA strains.\n");
                    sendText("@D--------------------------------------------------------@n\n");
                    sendText("@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
                    sendText("@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
                    sendText("@B3@W) @CNamek   @c- @CNo food needed@n\n");
                    sendText("@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n");
                    sendText("@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
                    sendText("@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
                    sendText("@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
                    sendText("@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n");
                    sendText(fmt::format("@D----[@gGenome 1@W: @G{}@D]----[@gGenome 2@W: @G{}@D]----",
                              display_genome[ch->genome[0]], display_genome[ch->genome[1]]));

                    sendText("\n@wChoose: ");
                    return;

                } else {
                    int select = atoi(arg.c_str());
                    if (select > 8 || select < 1) {
                        sendText("@RSelect two genomes to be your primary DNA strains.\n");
                        sendText("@D--------------------------------------------------------@n\n");
                        sendText("@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
                        sendText("@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
                        sendText("@B3@W) @CNamek   @c- @CNo food needed@n\n");
                        sendText("@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n");
                        sendText("@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
                        sendText("@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
                        sendText("@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
                        sendText("@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n");
                        sendText("@RThat is not an acceptable selection. @WTry again:@n\n");
                        return;
                    } else {
                        if (ch->genome[0] == 0) {
                            ch->genome[0] = select;
                            if (select == 7) {
                                SET_SKILL(ch, SKILL_TELEPATHY, 30);
                                SET_SKILL(ch, SKILL_FOCUS, 30);
                            }
                        } else if (ch->genome[0] > 0 && ch->genome[0] == select) {
                            sendText("You can't choose the same thing for both genomes!\r\n");
                            sendText("\n@wChoose: ");
                            return;
                        } else if (ch->genome[1] == 0) {
                            ch->genome[1] = select;
                            if (select == 7) {
                                SET_SKILL(ch, SKILL_TELEPATHY, 30);
                                SET_SKILL(ch, SKILL_FOCUS, 30);
                            }
                        }
                        const char *display_genome[9] = {"Unselected", /* 0 */
                                                         "Human", /* 1 */
                                                         "Saiyan", /* 2 */
                                                         "Namek", /* 3 */
                                                         "Icer", /* 4 */
                                                         "Truffle", /* 5 */
                                                         "Arlian", /* 6 */
                                                         "Kai", /* 7 */
                                                         "Konatsu" /* 8 */

                        };
                        sendText("@RSelect two genomes to be your primary DNA strains.\n");
                        sendText("@D--------------------------------------------------------@n\n");
                        sendText("@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
                        sendText("@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
                        sendText("@B3@W) @CNamek   @c- @CNo food needed@n\n");
                        sendText("@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n");
                        sendText("@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
                        sendText("@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
                        sendText("@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
                        sendText("@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n");
                        sendText(fmt::format("@D----[@gGenome 1@W: @G%s@D]----[@gGenome 2@W: @G%s@D]----",
                                  display_genome[ch->genome[0]], display_genome[ch->genome[1]]));
                        return;
                    }
                }
                break;

            case CON_BONUS:
                if(arg.empty()) {
                    display_bonus_menu(0);
                    sendText("@wChoose: ");
                    return;
                } else if (!strcasecmp(arg.c_str(), "b") || !strcasecmp(arg.c_str(), "B")) {
                    display_bonus_menu(0);
                    sendText("@RYou are already in that menu.\r\n");
                    sendText("@wChoose: ");
                    return;
                } else if (!strcasecmp(arg.c_str(), "N") || !strcasecmp(arg.c_str(), "n")) {
                    display_bonus_menu(1);
                    sendText("@wChoose: ");
                    state = CON_NEGATIVE;
                } else if (!strcasecmp(arg.c_str(), "x") || !strcasecmp(arg.c_str(), "X")) {
                    negcount = 0;
                    sendText("\r\n@wTo check the bonuses/negatives you have in game use the status command");
                    if (ccpoints > 0) {
                        sendText("\r\n@GYour left over points were spent on Practice Sessions@w");
                        GET_PRACTICES(ch) += (100 * ccpoints);
                    }
                    finish();
                    return;
                } else if ((value = parse_bonuses(arg)) != 1337) {
                    if (value == -1) {
                        display_bonus_menu(0);
                        sendText("@RThat is not an option.\r\n");
                        sendText("@wChoose: ");
                        return;
                    } else {
                        exchange_ccpoints(value);
                        sendText("@wChoose: ");
                        return;
                    }
                } else {
                    display_bonus_menu(0);
                    sendText("@wChoose: ");
                    return;
                }
                break;

            case CON_NEGATIVE:
                if(arg.empty()) {
                    display_bonus_menu(1);
                    sendText("@wChoose: ");
                    return;
                } else if (!strcasecmp(arg.c_str(), "n") || !strcasecmp(arg.c_str(), "N")) {
                    display_bonus_menu(1);
                    sendText("@RYou are already in that menu.\r\n");
                    sendText("@wChoose: ");
                    return;
                } else if (!strcasecmp(arg.c_str(), "b") || !strcasecmp(arg.c_str(), "B")) {
                    display_bonus_menu(0);
                    sendText("@wChoose: ");
                    state = CON_BONUS;
                } else if (!strcasecmp(arg.c_str(), "x") || !strcasecmp(arg.c_str(), "X")) {
                    negcount = 0;
                    if (ch->basepl <= 0) {
                        ch->basepl = 90;
                    }
                    if (ch->baseki <= 0) {
                        ch->baseki = 90;
                    }
                    if (ch->basest <= 0) {
                        ch->basest = 90;
                    }
                    sendText("\r\n@wTo check the bonuses/negatives you have in game use the status command");
                    if (ccpoints > 0) {
                        sendText("\r\n@GYour left over points were spent on Practice Sessions@w");
                        GET_PRACTICES(ch) += (100 * ccpoints);
                    }
                    finish();
                } else if ((value = parse_bonuses(arg)) != 1337) {
                    if (value == -1) {
                        display_bonus_menu(1);
                        sendText("@RThat is not an option.\r\n");
                        sendText("@wChoose: ");
                        return;
                    } else {
                        value += 15;
                        exchange_ccpoints(value);
                        sendText("@wChoose: ");
                        return;
                    }
                } else {
                    display_bonus_menu(1);
                    sendText("@wChoose: ");
                    return;
                }
                break;

            case CON_QCLASS:
            case CON_CLASS_HELP:
                switch (arg.size()) {
                    case 1:
                        switch(arg[0]) {
                            case 't':
                            case 'T':
                                switch (state) {
                                    case CON_CLASS_HELP:
                                        state = CON_QCLASS;
                                        display_classes();
                                        return;
                                    case CON_QCLASS:
                                        display_classes_help();
                                        state = CON_CLASS_HELP;
                                        return;
                                }
                                break;
                            default:
                                sendText("\r\nThat's not a sensei.\r\nSensei: ");
                                return;
                        }
                        break;
                    default:
                        chosen_sensei = sensei::find_sensei(arg);
                        if (!chosen_sensei) {
                            sendText("\r\nThat's not a sensei.\r\nSensei: ");
                            return;
                        }

                        switch (state) {
                            case CON_CLASS_HELP:
                                show_help(conn, chosen_sensei->getName().c_str());
                                chosen_sensei = nullptr;
                                return;
                            case CON_QCLASS:
                                if (chosen_sensei->getID() == sensei::kibito && !IS_KAI(ch) &&
                                    conn->account->rpp < 10 && ch->rbank < 10) {
                                    sendText("\r\nIt costs 10 RPP to select Kibito unless you are a Kai.\r\nSensei: ");
                                    return;
                                } else {
                                    ch->chclass = chosen_sensei;
                                    if (chosen_sensei->getID() == sensei::kibito && !IS_KAI(ch)) {
                                        if (conn->account->rpp >= 10)
                                            conn->account->rpp -= 10;
                                        else
                                            conn->account->rpp -= 10;

                                        sendText("\r\n10 RPP deducted from your bank since you are not a kai.\n");
                                    }
                                }
                                break;
                        }
                }

                if (IS_ANDROID(ch)) {
                    sendText("\r\n@YChoose your model type.\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Absorbtion Model@n\r\n");
                    sendText("@B2@W)@C Repair Model@n\r\n");
                    sendText("@B3@W)@C Sense, Powersense Model@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_ANDROID;
                } else {
                    sendText("\r\n@RAnswer The following questions carefully, they construct your alignment\r\nand affect your stats.\r\n\r\n");
                    sendText("@WQuestion (@G1@W out of @g10@W)\r\n");
                    sendText("@YAnswer the following question:\r\n");
                    sendText("@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
                    sendText("@D---------------------------------------@n\r\n");
                    sendText("@B1@W)@C Ask someone with more experience what to do.@n\r\n");
                    sendText("@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
                    sendText("@B3@W)@C Search for something magical to increase my strength.@n\r\n");
                    sendText("@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
                    sendText("@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
                    sendText("@w\r\nMake a selection:@n\r\n");
                    state = CON_Q1;
                }
                break;

            case CON_ANDROID:
                switch(arg[0]) {
                    case '1':
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_ABSORB);
                        sendText("\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your liking.\r\n\r\n");
                        sendText("\r\n@WQuestion (@G1@W out of @g10@W)");
                        sendText("@YAnswer the following question:\r\n");
                        sendText("@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
                        sendText("@D---------------------------------------@n\r\n");
                        sendText("@B1@W)@C Ask someone with more experience what to do.@n\r\n");
                        sendText("@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
                        sendText("@B3@W)@C Search for something magical to increase my strength.@n\r\n");
                        sendText("@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
                        sendText("@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
                        sendText("@w\r\nMake a selection:@n\r\n");
                        state = CON_Q1;
                        break;
                    case '2':
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_REPAIR);
                        sendText("\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your linking.\r\n\r\n");
                        sendText("@YAnswer the following question:\r\n");
                        sendText("@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
                        sendText("@D---------------------------------------@n\r\n");
                        sendText("@B1@W)@C Ask someone with more experience what to do.@n\r\n");
                        sendText("@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
                        sendText("@B3@W)@C Search for something magical to increase my strength.@n\r\n");
                        sendText("@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
                        sendText("@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
                        sendText("@w\r\nMake a selection:@n\r\n");
                        state = CON_Q1;
                        break;
                    case '3':
                        SET_BIT_AR(PLR_FLAGS(ch), PLR_SENSEM);
                        sendText("\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer or your stats contrary to your liking.\r\n\r\n");
                        sendText("@YAnswer the following question:\r\n");
                        sendText("@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
                        sendText("@D---------------------------------------@n\r\n");
                        sendText("@B1@W)@C Ask someone with more experience what to do.@n\r\n");
                        sendText("@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
                        sendText("@B3@W)@C Search for something magical to increase my strength.@n\r\n");
                        sendText("@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
                        sendText("@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
                        sendText("@w\r\nMake a selection:@n\r\n");
                        state = CON_Q1;
                        break;
                    default:
                        sendText("@wThat is not a correct selection, try again.@n\r\n");
                        return;
                }
                break;
        }
    }

    void ChargenParser::finish() {
        // CREATE PLAYER ENTRY
        ch->id = nextCharID();
        ch->generation = time(nullptr);
        check_unique_id(ch);
        add_unique_id(ch);
        auto &p = players[ch->id];
        p.name = ch->name;
        p.id = ch->id;
        p.account = conn->account;
        p.character = ch;
        init_char(ch);
        save_char(ch);
        // set state to -1 to prevent accidental freeing of ch...
        state = -1;
        send_to_imm("New Character '%s' created by Account: %s", ch->name, p.account->name.c_str());
        conn->setParser(new CharacterMenu(conn, ch));
    }
}