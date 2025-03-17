#include <iostream>
#include <iomanip>
#include <boost/algorithm/string.hpp>

#include "dbat/transformation.h"
#include "dbat/comm.h"
#include "dbat/send.h"
#include "dbat/weather.h"
#include "dbat/genzon.h"
#include "dbat/dg_comm.h"
#include "dbat/dg_scripts.h"
#include <dbat/combat.h>
#include <dbat/attack.h>

namespace trans {
    static std::string getCustomName(struct char_data* ch, FormID form) {
        return "Unknown";
    }

    std::string getName(struct char_data* ch, FormID form) {
        switch (form) {
            case FormID::base:
                return "Base";
            case FormID::custom_1:
            case FormID::custom_2:
            case FormID::custom_3:
            case FormID::custom_4:
            case FormID::custom_5:
            case FormID::custom_6:
            case FormID::custom_7:
            case FormID::custom_8:
            case FormID::custom_9:
                return getCustomName(ch, form);

            // Saiyan'y forms.
            case FormID::oozaru:
                return "Oozaru";
            case FormID::golden_oozaru:
                return "@YGolden Oozaru@n";
            case FormID::super_saiyan_1:
                return "@YSuper @CSaiyan @WFirst@n";
            case FormID::super_saiyan_2:
                return "@YSuper @CSaiyan @WSecond@n";
            case FormID::super_saiyan_3:
                return "@YSuper @CSaiyan @WThird@n";
            case FormID::super_saiyan_4:
                return "@YSuper @CSaiyan @RFourth@n";
            case FormID::super_saiyan_god:
                return "@YSuper @CSaiyan @RGod@n";
            case FormID::super_saiyan_blue:
                return "@YSuper @CSaiyan @BBlue@n";

            // Human'y forms.
            case FormID::super_human_1:
                return "@YSuper @CHuman @WFirst@n";
            case FormID::super_human_2:
                return "@YSuper @CHuman @WSecond@n";
            case FormID::super_human_3:
                return "@YSuper @CHuman @WThird@n";
            case FormID::super_human_4:
                return "@YSuper @CHuman @WFourth@n";

            // Icer'y forms.
            case FormID::icer_1:
                return "@YTransform @WFirst@n";
            case FormID::icer_2:
                return "@YTransform @WSecond@n";
            case FormID::icer_3:
                return "@YTransform @WThird@n";
            case FormID::icer_4:
                return "@YTransform @WFourth@n";
            case FormID::icer_metal:
                return "@YTransform @WMetal@n";
            case FormID::icer_golden:
                return "@YTransform @YGolden@n";
            case FormID::icer_black:
                return "@YTransform @XBlack@n";

            // Konatsu'y forms.
            case FormID::shadow_first:
                return "@YShadow @WFirst@n";
            case FormID::shadow_second:
                return "@YShadow @WSecond@n";
            case FormID::shadow_third:
                return "@YShadow @WThird@n";

            // Namekian'y forms.
            case FormID::super_namekian_1:
                return "@YSuper @CNamekian @WFirst@n";
            case FormID::super_namekian_2:
                return "@YSuper @CNamekian @WSecond@n";
            case FormID::super_namekian_3:
                return "@YSuper @CNamekian @WThird@n";
            case FormID::super_namekian_4:
                return "@YSuper @CNamekian @WFourth@n";

            // Mutant Forms.
            case FormID::mutate_1:
                return "@YMutate @WFirst@n";
            case FormID::mutate_2:
                return "@YMutate @WSecond@n";
            case FormID::mutate_3:
                return "@YMutate @WThird@n";

            // BioAndroid
            case FormID::bio_mature:
                return "@YMature@n";
            case FormID::bio_semi_perfect:
                return "@YSemi-Perfect@n";
            case FormID::bio_perfect:
                return "@YPerfect@n";
            case FormID::bio_super_perfect:
                return "@YSuper Perfect@n";

            // Android
            case FormID::android_1:
                return "@YVersion 1.0@n";
            case FormID::android_2:
                return "@YVersion 2.0@n";
            case FormID::android_3:
                return "@YVersion 3.0@n";
            case FormID::android_4:
                return "@YVersion 4.0@n";
            case FormID::android_5:
                return "@YVersion 5.0@n";
            case FormID::android_6:
                return "@YVersion 6.0@n";

            // Majin
            case FormID::maj_affinity:
                return "@YAffinity@n";
            case FormID::maj_super:
                return "@YSuper@n";
            case FormID::maj_true:
                return "@YTrue@n";

            // Kai
            case FormID::mystic_1:
                return "@YMystic @WFirst@n";
            case FormID::mystic_2:
                return "@YMystic @WSecond@n";
            case FormID::mystic_3:
                return "@YMystic @WThird@n";

            case FormID::divine_halo:
                return "@YDivine @WHalo@n";

            // Tuffle
            case FormID::ascend_1:
                return "@YAscend @WFirst@n";
            case FormID::ascend_2:
                return "@YAscend @WSecond@n";
            case FormID::ascend_3:
                return "@YAscend @WThird@n";

            // Demon
            case FormID::dark_king:
                if(ch->getBasePL() >= 600000000)
                    return "@bDark @rKing@n";
                else if (ch->getBasePL() >= 50000000)
                    return "@bDark @yLord@n";
                else if (ch->getBasePL() >= 2000000)
                    return "@bDark @yCourtier@n";
                else
                    return "@bDark @ySeed@n";

            // LSSJ
            case FormID::ikari:
                return "@GIkari@n";
            case FormID::legendary_saiyan:
                return "@GLegendary @YSuper Saiyan@n";

            // Lycanthrope
            case FormID::lesser_lycanthrope:
                return "@YLesser @WLycan@n";
            case FormID::lycanthrope:
                return "@WLycanthrope@n";
            case FormID::alpha_lycanthrope:
                return "@YAlpha @WLycanthrope@n";


            // Unbound Alternate Forms
            case FormID::potential_unleashed:
                return "@YPotential @WUnleashed@n";
            case FormID::evil_aura:
                return "@YEvil @WAura@n";
            case FormID::ultra_instinct:
                return "@BUltra @RInstinct@n";

            //Unbound Perm Forms
            case FormID::potential_unlocked:
                return "@YPotential @WUnlocked@n";
            case FormID::potential_unlocked_max:
                return "@YMax @WPotential@n";
            case FormID::majinized:
                return "@YMajinized@n";
            case FormID::divine_water:
                return "@YDivine @WWater@n";


            // Techniques
            case FormID::kaioken:
                return "Kaioken";
            case FormID::dark_metamorphosis:
                return "Dark Meta";



            case FormID::tiger_stance:
                return "Tiger Stance";
            case FormID::eagle_stance:
                return "Eagle Stance";
            case FormID::ox_stance:
                return "Ox Stance";

            case FormID::spirit_absorption:
                return "@YSpirit Absorption@n";
                
            // Whoops?
            default: 
                return "Unknown";
        }
    }

    std::string getExtra(struct char_data* ch, FormID form) {
        double energy = 0;
        std::string tag = "";
        switch (form) {
            case FormID::oozaru:
                return "@w...$e is in the form of a @rgreat ape@w!";
            case FormID::golden_oozaru:
                return "@w...$e is in the form of a @rgolden great ape@w!";
            case FormID::super_saiyan_1:
                return "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!";
            case FormID::super_saiyan_2:
                return "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!";
            case FormID::super_saiyan_3:
                return "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!";
            case FormID::super_saiyan_4:
                return "@w...$e has a covering of @rdark red fur@n around $s body!";

            case FormID::kaioken:
                return "@w...@r$e has a red aura around $s body!";
            case FormID::dark_metamorphosis:
                return "@w...$e has a dark, @rred@w aura and menacing presence.";

            case FormID::tiger_stance:
                return "@w...$e has an aggressive demeanour, ready for a fight.";
            case FormID::eagle_stance:
                return "@w...$e has a calm appearance, eyes constantly alert.";
            case FormID::ox_stance:
                return "@w...$e is hunkered down, yet $s presence appears larger.";

            case FormID::spirit_absorption:
                energy = ch->transforms[FormID::spirit_absorption].vars[0];
                if(energy > 1000000000) {
                    tag = "@Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n";
                } else if(energy > 100000000) {
                    tag = "@Ceclipsing@n";
                } else if(energy > 10000000) {
                    tag = "@crackling@n";
                } else if(energy > 1000000) {
                    tag = "bright";
                } else if(energy > 100000) {
                    tag = "fading";
                } else {
                    tag = "faint";
                }
                return "@w...$e has a " + tag + " aura wrapped around $s body!";

            default:
                return "@w...$e has energy crackling around $s body!";
        }

    }

    int getMaxGrade(struct char_data* ch, FormID form) {
        int maxGrade = 1;
        switch (form) {
            case FormID::kaioken:
                maxGrade = GET_SKILL(ch, (int)SkillID::kaioken) / 5;
                if (maxGrade > 20)
                    maxGrade = 20;
                if(ch->form != FormID::base)
                    maxGrade /= 4;
                if (maxGrade < 1)
                    maxGrade = 1;

                return maxGrade;
            case FormID::dark_metamorphosis:
                return GET_SKILL(ch, (int)SkillID::dark_metamorphosis) >= 100 ? 2 : 1;

            case FormID::tiger_stance:
                return std::max(GET_SKILL(ch, (int)SkillID::tiger_stance) / 20, 1);
            case FormID::eagle_stance:
                return std::max(GET_SKILL(ch, (int)SkillID::eagle_stance) / 20, 1);
            case FormID::ox_stance:
                return std::max(GET_SKILL(ch, (int)SkillID::ox_stance) / 20, 1);

            case FormID::spirit_absorption:
                return 3;

            default:
                return 1;
        }
    }

    static std::string getCustomAbbr(struct char_data* ch, FormID form) {
        return "N/A";
    }

    int getMasteryTier(char_data* ch, FormID form) {
        if(ch->transforms.contains(form)) {
            double timeSpent = ch->transforms[form].timeSpentInForm;
            int mastery = 0;
            if(ch->transforms[form].limitBroken && timeSpent > LIMITBREAK_THRESHOLD)
                mastery = 4;
            else if (timeSpent > LIMIT_THRESHOLD)
                mastery = 3;
            else if (timeSpent > MASTERY_THRESHOLD)
                mastery = 2;
            else mastery = 1;

            if(IS_AFFECTED(ch, AFF_LIMIT_BREAKING) && ch->transforms[form].limitBroken)
                mastery += 2;

            return mastery;
        } else {
            return 0;
        }
    }
    
    std::string getAbbr(struct char_data* ch, FormID form) {
        switch (form) {
            case FormID::base:
                return "Base";
            case FormID::custom_1:
            case FormID::custom_2:
            case FormID::custom_3:
            case FormID::custom_4:
            case FormID::custom_5:
            case FormID::custom_6:
            case FormID::custom_7:
            case FormID::custom_8:
            case FormID::custom_9:
                return getCustomAbbr(ch, form);

            // Saiyan'y forms.
            case FormID::oozaru:
                return "ooz";
            case FormID::golden_oozaru:
                return "gooz";
            case FormID::super_saiyan_1:
                return "ssj";
            case FormID::super_saiyan_2:
                return "ssj2";
            case FormID::super_saiyan_3:
                return "ssj3";
            case FormID::super_saiyan_4:
                return "ssj4";
            case FormID::super_saiyan_god:
                return "god";
            case FormID::super_saiyan_blue:
                return "blue";

            // Human'y forms.
            case FormID::super_human_1:
                return "first";
            case FormID::super_human_2:
                return "second";
            case FormID::super_human_3:
                return "third";
            case FormID::super_human_4:
                return "fourth";

            // Icer'y forms.
            case FormID::icer_1:
                return "first";
            case FormID::icer_2:
                return "second";
            case FormID::icer_3:
                return "third";
            case FormID::icer_4:
                return "fourth";
            case FormID::icer_metal:
                return "metal";
            case FormID::icer_golden:
                return "golden";
            case FormID::icer_black:
                return "black";

            // Konatsu'y forms.
            case FormID::shadow_first:
                return "first";
            case FormID::shadow_second:
                return "second";
            case FormID::shadow_third:
                return "third";

            // Namekian'y forms.
            case FormID::super_namekian_1:
                return "first";
            case FormID::super_namekian_2:
                return "second";
            case FormID::super_namekian_3:
                return "third";
            case FormID::super_namekian_4:
                return "fourth";

            // Mutant Forms.
            case FormID::mutate_1:
                return "first";
            case FormID::mutate_2:
                return "second";
            case FormID::mutate_3:
                return "third";

            // BioAndroid
            case FormID::bio_mature:
                return "mature";
            case FormID::bio_semi_perfect:
                return "semi-perfect";
            case FormID::bio_perfect:
                return "perfect";
            case FormID::bio_super_perfect:
                return "super-perfect";

            // Android
            case FormID::android_1:
                return "1.0";
            case FormID::android_2:
                return "2.0";
            case FormID::android_3:
                return "3.0";
            case FormID::android_4:
                return "4.0";
            case FormID::android_5:
                return "5.0";
            case FormID::android_6:
                return "6.0";

            // Majin
            case FormID::maj_affinity:
                return "affinity";
            case FormID::maj_super:
                return "super";
            case FormID::maj_true:
                return "true";

            // Kai
            case FormID::mystic_1:
                return "first";
            case FormID::mystic_2:
                return "second";
            case FormID::mystic_3:
                return "third";

            case FormID::divine_halo:
                return "halo";

            // Tuffle
            case FormID::ascend_1:
                return "first";
            case FormID::ascend_2:
                return "second";
            case FormID::ascend_3:
                return "third";

            // Demon
            case FormID::dark_king:
                return "dark";

            // LSSJ
            case FormID::ikari:
                return "ikari";
            case FormID::legendary_saiyan:
                return "lssj";

            // Lycanthrope
            case FormID::lesser_lycanthrope:
                return "llycan";
            case FormID::lycanthrope:
                return "lycan";
            case FormID::alpha_lycanthrope:
                return "alycan";

            // Unbound Alternate Forms
            case FormID::potential_unleashed:
                return "potential";
            case FormID::evil_aura:
                return "evil";
            case FormID::ultra_instinct:
                return "ui";

            case FormID::potential_unlocked:
                return "punlocked";
            case FormID::potential_unlocked_max:
                return "pmax";
            case FormID::majinized:
                return "majinized";
            case FormID::divine_water:
                return "divine_water";


            // Techniques
            case FormID::kaioken:
                return "kaioken";
            case FormID::dark_metamorphosis:
                return "dm";


            case FormID::tiger_stance:
                return "tiger";
            case FormID::eagle_stance:
                return "eagle";
            case FormID::ox_stance:
                return "ox";

            case FormID::spirit_absorption:
                return "spiritab";

            // Whoops?
            default:
                return "Unknown";
        }
    }

    static std::unordered_map<FormID, std::vector<FormID>> transform_exclusive = {
        {FormID::super_saiyan_2, {FormID::legendary_saiyan, FormID::ikari}},
        {FormID::ikari, {FormID::super_saiyan_2}},
        {FormID::legendary_saiyan, {FormID::super_saiyan_2, FormID::golden_oozaru}},

        {FormID::divine_halo, {FormID::mystic_3}},
        {FormID::mystic_3, {FormID::divine_halo}},

        {FormID::dark_metamorphosis, {FormID::kaioken}},
        {FormID::kaioken, {FormID::dark_metamorphosis}},
    };

    static std::unordered_map<FormID, std::function<bool(struct char_data *ch)>> trans_requirements = {
        {FormID::spirit_absorption, [](struct char_data *ch) {
            auto loc = get_obj_in_room(ch->getRoom(), "Spirit Bomb");
            return loc == nullptr ? false : true;
            }},
        {FormID::super_saiyan_4, [](struct char_data *ch) {
            return ch->hasTail();
            }},
    };

    static std::unordered_map<FormID, std::function<bool(struct char_data *ch)>> trans_unlocks = {
        // Saiyan
        {FormID::super_saiyan_1, [](struct char_data *ch) {
            return ((ch->race == RaceID::saiyan || ch->race == RaceID::halfbreed));
            }},
        {FormID::super_saiyan_2, [](struct char_data *ch) {
            return ((ch->race == RaceID::saiyan || ch->race == RaceID::halfbreed) && !ch->transforms.contains(FormID::ikari) 
                && !ch->transforms.contains(FormID::legendary_saiyan) && getMasteryTier(ch, FormID::super_saiyan_1) >=2);
            }},
        {FormID::super_saiyan_3, [](struct char_data *ch) {
            return ((ch->race == RaceID::saiyan || ch->race == RaceID::halfbreed) && getMasteryTier(ch, FormID::super_saiyan_2) >=2);
            }},
        {FormID::super_saiyan_4, [](struct char_data *ch) {
            return ((ch->race == RaceID::saiyan || ch->race == RaceID::halfbreed) && ch->hasTail() && getMasteryTier(ch, FormID::golden_oozaru) >=3 && getMasteryTier(ch, FormID::super_saiyan_3) >=3);
            }},

        // Legendary Saiyan
        {FormID::ikari, [](struct char_data *ch) {
            return (ch->race == RaceID::saiyan && !ch->transforms.contains(FormID::super_saiyan_2) && getMasteryTier(ch, FormID::oozaru) >=3);
            }},
        {FormID::legendary_saiyan, [](struct char_data *ch) {
            return (ch->race == RaceID::saiyan && !ch->transforms.contains(FormID::super_saiyan_2) && ch->transforms.contains(FormID::super_saiyan_1) 
                && getMasteryTier(ch, FormID::super_saiyan_1) >=4 && getMasteryTier(ch, FormID::ikari) >=3);
            }},

        // Human
        {FormID::super_human_1, [](struct char_data *ch) {
            return (ch->race == RaceID::human);
            }},
        {FormID::super_human_2, [](struct char_data *ch) {
            return (ch->race == RaceID::human && getMasteryTier(ch, FormID::super_human_1) >=2);
            }},
        {FormID::super_human_3, [](struct char_data *ch) {
            return (ch->race == RaceID::human && getMasteryTier(ch, FormID::super_human_2) >=2);
            }},
        {FormID::super_human_4, [](struct char_data *ch) {
            return (ch->race == RaceID::human && getMasteryTier(ch, FormID::super_human_3) >=3);
            }},

        // Icer
        {FormID::icer_1, [](struct char_data *ch) {
            return (ch->race == RaceID::icer);
            }},
        {FormID::icer_2, [](struct char_data *ch) {
            return (ch->race == RaceID::icer && getMasteryTier(ch, FormID::icer_1) >=2);
            }},
        {FormID::icer_3, [](struct char_data *ch) {
            return (ch->race == RaceID::icer && getMasteryTier(ch, FormID::icer_2) >=2);
            }},
        {FormID::icer_4, [](struct char_data *ch) {
            return (ch->race == RaceID::icer && getMasteryTier(ch, FormID::icer_3) >=3);
            }},

        // Namekian
        {FormID::super_namekian_1, [](struct char_data *ch) {
            return (ch->race == RaceID::namekian);
            }},
        {FormID::super_namekian_2, [](struct char_data *ch) {
            return (ch->race == RaceID::namekian && getMasteryTier(ch, FormID::super_namekian_1) >=2);
            }},
        {FormID::super_namekian_3, [](struct char_data *ch) {
            return (ch->race == RaceID::namekian && getMasteryTier(ch, FormID::super_namekian_2) >=2);
            }},
        {FormID::super_namekian_4, [](struct char_data *ch) {
            return (ch->race == RaceID::namekian && getMasteryTier(ch, FormID::super_namekian_3) >=3);
            }},

        // Konatsu
        {FormID::shadow_first, [](struct char_data *ch) {
            return (ch->race == RaceID::konatsu);
            }},
        {FormID::shadow_second, [](struct char_data *ch) {
            return (ch->race == RaceID::konatsu && getMasteryTier(ch, FormID::shadow_first) >=2);
            }},
        {FormID::shadow_third, [](struct char_data *ch) {
            return (ch->race == RaceID::konatsu && getMasteryTier(ch, FormID::shadow_second) >=3);
            }},

        // Mutant
        {FormID::mutate_1, [](struct char_data *ch) {
            return (ch->race == RaceID::mutant);
            }},
        {FormID::mutate_2, [](struct char_data *ch) {
            return (ch->race == RaceID::mutant && getMasteryTier(ch, FormID::mutate_1) >=2);
            }},
        {FormID::mutate_3, [](struct char_data *ch) {
            return (ch->race == RaceID::mutant && getMasteryTier(ch, FormID::mutate_2) >=3);
            }},

        // BioAndroid
        {FormID::bio_mature, [](struct char_data *ch) {
            return (ch->race == RaceID::bio_android);
            }},
        {FormID::bio_semi_perfect, [](struct char_data *ch) {
            return (ch->race == RaceID::bio_android && getMasteryTier(ch, FormID::bio_mature) >=2);
            }},
        {FormID::bio_perfect, [](struct char_data *ch) {
            return (ch->race == RaceID::bio_android && getMasteryTier(ch, FormID::bio_semi_perfect) >=2);
            }},
        {FormID::bio_super_perfect, [](struct char_data *ch) {
            return (ch->race == RaceID::bio_android && getMasteryTier(ch, FormID::bio_perfect) >=3);
            }},

        // Android
        {FormID::android_1, [](struct char_data *ch) {
            return (ch->race == RaceID::android);
            }},
        {FormID::android_2, [](struct char_data *ch) {
            return (ch->race == RaceID::android && getMasteryTier(ch, FormID::android_1) >=2);
            }},
        {FormID::android_3, [](struct char_data *ch) {
            return (ch->race == RaceID::android && getMasteryTier(ch, FormID::android_2) >=2);
            }},
        {FormID::android_4, [](struct char_data *ch) {
            return (ch->race == RaceID::android && getMasteryTier(ch, FormID::android_3) >=2);
            }},
        {FormID::android_5, [](struct char_data *ch) {
            return (ch->race == RaceID::android && getMasteryTier(ch, FormID::android_4) >=2);
            }},
        {FormID::android_6, [](struct char_data *ch) {
            return (ch->race == RaceID::android && getMasteryTier(ch, FormID::android_5) >=3);
            }},

        // Majin
        {FormID::maj_affinity, [](struct char_data *ch) {
            return (ch->race == RaceID::majin);
            }},
        {FormID::maj_super, [](struct char_data *ch) {
            return (ch->race == RaceID::majin && getMasteryTier(ch, FormID::maj_affinity) >=2);
            }},
        {FormID::maj_true, [](struct char_data *ch) {
            return (ch->race == RaceID::majin && getMasteryTier(ch, FormID::maj_super) >=3);
            }},

        // Kai
        {FormID::mystic_1, [](struct char_data *ch) {
            return (ch->race == RaceID::kai);
            }},
        {FormID::mystic_2, [](struct char_data *ch) {
            return (ch->race == RaceID::kai && getMasteryTier(ch, FormID::mystic_1) >=2);
            }},
        {FormID::mystic_3, [](struct char_data *ch) {
            return (ch->race == RaceID::kai && getMasteryTier(ch, FormID::mystic_2) >=3);
            }},

        {FormID::divine_halo, [](struct char_data *ch) {
            return (ch->race == RaceID::kai && getMasteryTier(ch, FormID::mystic_2) >=3);
            }},

        // Tuffle
        {FormID::ascend_1, [](struct char_data *ch) {
            return (ch->race == RaceID::tuffle);
            }},
        {FormID::ascend_2, [](struct char_data *ch) {
            return (ch->race == RaceID::tuffle && getMasteryTier(ch, FormID::ascend_1) >=2);
            }},
        {FormID::ascend_3, [](struct char_data *ch) {
            return (ch->race == RaceID::tuffle && getMasteryTier(ch, FormID::ascend_2) >=3);
            }},

        // Demon
        {FormID::dark_king, [](struct char_data *ch) {
            return (ch->race == RaceID::demon);
            }},

        // Lycanthrope
        {FormID::lycanthrope, [](struct char_data *ch) {
            return (ch->race == RaceID::konatsu && ch->getLocationEnvironment(ENV_MOONLIGHT));
            }},
        {FormID::alpha_lycanthrope, [](struct char_data *ch) {
            return (ch->getLocationEnvironment(ENV_MOONLIGHT) && getMasteryTier(ch, FormID::lycanthrope) >=4);
            }},

        // Techniques
        {FormID::kaioken, [](struct char_data *ch) {
            return GET_SKILL(ch, (int) SkillID::kaioken) > 0 && !ch->transforms.contains(FormID::dark_metamorphosis);
            }},

        {FormID::dark_metamorphosis, [](struct char_data *ch) {
            return GET_SKILL(ch, (int) SkillID::dark_metamorphosis) > 0 && !ch->transforms.contains(FormID::kaioken);
            }},


        {FormID::tiger_stance, [](struct char_data *ch) {
            return GET_SKILL(ch, (int) SkillID::tiger_stance) > 0;
            }},

        {FormID::eagle_stance, [](struct char_data *ch) {
            return GET_SKILL(ch, (int) SkillID::eagle_stance) > 0;
            }},
        
        {FormID::ox_stance, [](struct char_data *ch) {
            return GET_SKILL(ch, (int) SkillID::ox_stance) > 0;
            }},

        {FormID::spirit_absorption, [](struct char_data *ch) {
            auto loc = get_obj_in_room(ch->getRoom(), "Spirit Bomb");
            return loc == nullptr ? false : true;
            }},

    };

    using trans_affect_type = character_affect_type;

    static double getModifierHelper(char_data* ch, FormID form, int location, int specific);

    static std::unordered_map<FormID, std::vector<trans_affect_type>> trans_affects = {

        // Saiyan'y forms...
        {
            FormID::oozaru, {
                {APPLY_COMBAT_MULT,  0.0, static_cast<int>(ComStat::block),                    [](struct char_data *ch) {return 0.3 + (0.1 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_COMBAT_MULT,    0.0, static_cast<int>(ComStat::dodge),                    [](struct char_data *ch) {return -0.7 * 1.0 + (0.05 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_CDIM_MULT,      0.0, static_cast<int>(CharDim::height),                        [](struct char_data *ch) {return 9.0 + (0.5 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](struct char_data *ch) {return 49.0 + (4.00 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::defense), [](struct char_data *ch) {return -0.1 - (0.05 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](struct char_data *ch) {return 0.2 + (0.05 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_CVIT_MULT, 0.0, ~0, [](struct char_data *ch) {return 0.9 + (0.05 * getMasteryTier(ch, FormID::oozaru));}},
                {APPLY_CVIT_BASE, 0.0, ~0, [](struct char_data *ch) {return 9000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::oozaru));}},
            }
        },
        {
            FormID::golden_oozaru, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {
                    if(ch->transforms.contains(FormID::super_saiyan_4)) return getModifierHelper(ch,
                                                                                               FormID::super_saiyan_4,
                                                                                               APPLY_CVIT_BASE,
                                                                                               ~0);
                    if(ch->transforms.contains(FormID::super_saiyan_3)) return getModifierHelper(ch,
                                                                                               FormID::super_saiyan_3,
                                                                                               APPLY_CVIT_BASE,
                                                                                               ~0);
                    if(ch->transforms.contains(FormID::super_saiyan_2)) return getModifierHelper(ch,
                                                                                               FormID::super_saiyan_2,
                                                                                               APPLY_CVIT_BASE,
                                                                                               ~0);
                    return getModifierHelper(ch, FormID::super_saiyan_1, APPLY_CVIT_BASE, ~0);
                }},
                {APPLY_CVIT_MULT,     0.0, ~0,                    [](struct char_data *ch) {
                    if(ch->transforms.contains(FormID::super_saiyan_4)) return getModifierHelper(ch,
                                                                                               FormID::super_saiyan_4,
                                                                                               APPLY_CVIT_MULT,
                                                                                               ~0);
                    if(ch->transforms.contains(FormID::super_saiyan_3)) return getModifierHelper(ch,
                                                                                               FormID::super_saiyan_3,
                                                                                               APPLY_CVIT_MULT,
                                                                                               ~0);
                    if(ch->transforms.contains(FormID::super_saiyan_2)) return getModifierHelper(ch,
                                                                                               FormID::super_saiyan_2,
                                                                                               APPLY_CVIT_MULT,
                                                                                               ~0);
                    return getModifierHelper(ch, FormID::super_saiyan_1, APPLY_CVIT_MULT, ~0);
                }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::block), [](struct char_data *ch) {return 0.3 + (0.1 * getMasteryTier(ch, FormID::golden_oozaru));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](struct char_data *ch) {return -0.7 + (0.05 * getMasteryTier(ch, FormID::golden_oozaru));}},
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](struct char_data *ch) {return 9.0 + (0.5 * getMasteryTier(ch, FormID::golden_oozaru));}},
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](struct char_data *ch) {return 49.0 + (4.00 * getMasteryTier(ch, FormID::golden_oozaru));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::defense), [](struct char_data *ch) {return -0.1 - (0.05 * getMasteryTier(ch, FormID::golden_oozaru));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](struct char_data *ch) {return 0.2 + (0.05 * getMasteryTier(ch, FormID::golden_oozaru));}},
            }
        },

        {
            FormID::super_saiyan_1, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.9 + (0.1 * getMasteryTier(ch, FormID::super_saiyan_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 700000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::super_saiyan_1));}},
            }
        },

        {
            FormID::super_saiyan_2, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.5 + (0.15 * getMasteryTier(ch, FormID::super_saiyan_2));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 15000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::super_saiyan_2));}},
            }
        },

        {
            FormID::super_saiyan_3, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.4 + (0.25 * getMasteryTier(ch, FormID::super_saiyan_3));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 70000000 * 1.0 + (0.25 * getMasteryTier(ch, FormID::super_saiyan_3));}},
            }
        },

        {
            FormID::super_saiyan_4, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 3.8 + (0.35 * getMasteryTier(ch, FormID::super_saiyan_4));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 17000000 * 1.0 + (0.35 * getMasteryTier(ch, FormID::super_saiyan_4));}},
            }
        },

        // Human'y forms.
        {
            FormID::super_human_1, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.8 + (0.1 * getMasteryTier(ch, FormID::super_human_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 950000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::super_human_1));}},
            }
        },

        {
            FormID::super_human_2, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.6 + (0.2 * getMasteryTier(ch, FormID::super_human_2));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 10000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::super_human_2));}},
            }
        },

        {
            FormID::super_human_3, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.5 + (0.3 * getMasteryTier(ch, FormID::super_human_3));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 40000000 * 1.0 + (0.3 * getMasteryTier(ch, FormID::super_human_3));}},
            }
        },

        {
            FormID::super_human_4, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 3.1 + (0.4 * getMasteryTier(ch, FormID::super_human_4));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 200000000 * 1.0 + (0.4 * getMasteryTier(ch, FormID::super_human_4));}},
            }
        },

        // Icer'y Forms.
        {
            FormID::icer_1, {
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height),                    [](struct char_data *ch) {return 2.0 + (0.2 * getMasteryTier(ch, FormID::icer_1));}},
                {APPLY_CDIM_MULT,   0.0, static_cast<int>(CharDim::weight),                    [](struct char_data *ch) {return 3.0 + (0.3 * getMasteryTier(ch, FormID::icer_1));}},
                {APPLY_CVIT_MULT,        0.0, ~0,                        [](struct char_data *ch) {return 1.0 + (0.05 * getMasteryTier(ch, FormID::icer_1));}},
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 400000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::icer_1));}},


            }
        },
        {
            FormID::icer_2, {
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height),                    [](struct char_data *ch) {return 2.0 + (0.2 * getMasteryTier(ch, FormID::icer_2));}},
                {APPLY_CDIM_MULT,   0.0, static_cast<int>(CharDim::weight),                    [](struct char_data *ch) {return 3.0 + (0.3 * getMasteryTier(ch, FormID::icer_2));}},
                {APPLY_CVIT_MULT,        0.0, ~0,                        [](struct char_data *ch) {return 2.0 + (0.05 * getMasteryTier(ch, FormID::icer_2));}},
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 7000000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::icer_2));}},
            }
        },

        {
            FormID::icer_3, {
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height),                    [](struct char_data *ch) {return 0.5 + (0.25 * getMasteryTier(ch, FormID::icer_3));}},
                {APPLY_CDIM_MULT,   0.0, static_cast<int>(CharDim::weight),                    [](struct char_data *ch) {return 1.0 + (0.1 * getMasteryTier(ch, FormID::icer_3));}},
                {APPLY_CVIT_MULT,        0.0, ~0,                        [](struct char_data *ch) {return 3.0 + (0.075 * getMasteryTier(ch, FormID::icer_3));}},
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 45000000 * 1.0 + (0.075 * getMasteryTier(ch, FormID::icer_3));}},
            }
        },

        {
            FormID::icer_4, {
                {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height),                    [](struct char_data *ch) {return 1.0 + (0.1 * getMasteryTier(ch, FormID::icer_4));}},
                {APPLY_CDIM_MULT,   0.0, static_cast<int>(CharDim::weight),                    [](struct char_data *ch) {return 2.0 + (0.2 * getMasteryTier(ch, FormID::icer_4));}},
                {APPLY_CVIT_MULT,        0.0, ~0,                        [](struct char_data *ch) {return 4.0 + (0.1 * getMasteryTier(ch, FormID::icer_4));}},
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 200000000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::icer_4));}},
            }
        },

        // Namekian Forms.
        {
            FormID::super_namekian_1, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.0 + (0.05 * getMasteryTier(ch, FormID::super_namekian_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 190000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::super_namekian_1));}},
            }
        },
        {
            FormID::super_namekian_2, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.9 + (0.1 * getMasteryTier(ch, FormID::super_namekian_2));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 3500000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::super_namekian_2));}},
            }
        },
        {
            FormID::super_namekian_3, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.95 + (0.15 * getMasteryTier(ch, FormID::super_namekian_3));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 60000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::super_namekian_3));}},
            }
        },
        {
            FormID::super_namekian_4, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 3.3 + (0.2 * getMasteryTier(ch, FormID::super_namekian_4));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 210000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::super_namekian_4));}},
            }
        },

        // Konatsu Forms.
        {
            FormID::shadow_first, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.0 + (0.05 * getMasteryTier(ch, FormID::shadow_first));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 70000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::shadow_first));}},
            }
        },
        {
            FormID::shadow_second, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.8 + (0.15 * getMasteryTier(ch, FormID::shadow_second));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 7000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::shadow_second));}},
            }
        },
        {
            FormID::shadow_third, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.5 + (0.3 * getMasteryTier(ch, FormID::shadow_third));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 60000000 * 1.0 + (0.3 * getMasteryTier(ch, FormID::shadow_third));}},
            }
        },

        // Mutant Forms.
        {
            FormID::mutate_1, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.9 + (0.05 * getMasteryTier(ch, FormID::mutate_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 90000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::mutate_1));}},
            }
        },
        {
            FormID::mutate_2, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.7 + (0.15 * getMasteryTier(ch, FormID::mutate_2));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 8000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::mutate_2));}},
            }
        },
        {
            FormID::mutate_3, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.4 + (0.3 * getMasteryTier(ch, FormID::mutate_3));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 70000000 * 1.0 + (0.3 * getMasteryTier(ch, FormID::mutate_3));}},
            }
        },
        // BioAndroid forms.
        {
            FormID::bio_mature, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.6 + (0.05 * getMasteryTier(ch, FormID::bio_mature));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 900000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::bio_mature));}},
            }
        },
        {
            FormID::bio_semi_perfect, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.6 + (0.05 * getMasteryTier(ch, FormID::bio_semi_perfect));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 7500000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::bio_semi_perfect));}},
            }
        },
        {
            FormID::bio_perfect, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.6 + (0.05 * getMasteryTier(ch, FormID::bio_super_perfect));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 60000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::bio_perfect));}},
            }
        },
        {
            FormID::bio_super_perfect, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.6 + (0.2 * getMasteryTier(ch, FormID::bio_super_perfect));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 350000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::bio_super_perfect));}},
            }
        },

        // Android Forms
        {
            FormID::android_1, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 11000000 * 1.0 + (0.05 * getMasteryTier(ch, FormID::android_1));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {return 0.001 + (0.05 * getMasteryTier(ch, FormID::android_1));}},
                {APPLY_CATTR_BASE,      2,   ~0},
            }
        },
        {
            FormID::android_2, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 45000000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::android_2));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {return 0.001 + (0.05 * getMasteryTier(ch, FormID::android_2));}},
                {APPLY_CATTR_BASE,      2,   ~0},
            }
        },
        {
            FormID::android_3, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 300000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::android_3));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {return 0.001 + (0.05 * getMasteryTier(ch, FormID::android_3));}},
                {APPLY_CATTR_BASE,      2,   ~0},
            }
        },
        {
            FormID::android_4, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 2000000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::android_4));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {return 0.001 + (0.05 * getMasteryTier(ch, FormID::android_4));}},
                {APPLY_CATTR_BASE,      2,   ~0},
            }
        },
        {
            FormID::android_5, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 4000000000 * 1.0 + (0.25 * getMasteryTier(ch, FormID::android_5));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {return 0.001 + (0.05 * getMasteryTier(ch, FormID::android_5));}},
                {APPLY_CATTR_BASE,      2,   ~0},
            }
        },
        {
            FormID::android_6, {
                {APPLY_CVIT_BASE,   0.0, ~0, [](struct char_data *ch) {return 7000000000 * 1.0 + (0.3 * getMasteryTier(ch, FormID::android_6));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {return 0.001 + (0.05 * getMasteryTier(ch, FormID::android_6));}},
                {APPLY_CATTR_BASE,      2,   ~0},
            }
        },

        // Majin Forms
        {
            FormID::maj_affinity, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.8 + (0.05 * getMasteryTier(ch, FormID::mystic_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 1150000 * 1.0 + (0.10 * getMasteryTier(ch, FormID::mystic_1));}},
            }
        },
        {
            FormID::maj_super, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.8 + (0.05 * getMasteryTier(ch, FormID::maj_super));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 12000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::maj_super));}},
            }
        },
        {
            FormID::maj_true, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.8 + (0.05 * getMasteryTier(ch, FormID::maj_true));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 300000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::maj_true));}},
            }
        },

        // Kai Forms
        {
            FormID::mystic_1, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.8 + (0.10 * getMasteryTier(ch, FormID::mystic_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 1000000 * 1.0 + (0.10 * getMasteryTier(ch, FormID::mystic_1));}},
            }
        },
        {
            FormID::mystic_2, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 2.7 + (0.15 * getMasteryTier(ch, FormID::mystic_2));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 100000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::mystic_2));}},
            }
        },
        {
            FormID::mystic_3, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 3.6 + (0.2 * getMasteryTier(ch, FormID::mystic_3));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 220000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::mystic_3));}},
            }
        },

        {
            FormID::divine_halo, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 3.6 + (0.2 * getMasteryTier(ch, FormID::divine_halo));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 220000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::divine_halo));}},
                {APPLY_SKILL,            0.0, (int) SkillID::divine_halo, [](struct char_data *ch) {return 20 * getMasteryTier(ch, FormID::divine_halo);}},
            }
        },

        // Tuffle Forms
        {
            FormID::ascend_1, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.9 + (0.1 * getMasteryTier(ch, FormID::ascend_1));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 1200000 * 1.0 + (0.10 * getMasteryTier(ch, FormID::ascend_1));}},
            }
        },
        {
            FormID::ascend_2, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.8 + (0.075 * getMasteryTier(ch, FormID::ascend_2));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 70000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::ascend_2));}},
            }
        },
        {
            FormID::ascend_3, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.7 + (0.05 * getMasteryTier(ch, FormID::ascend_3));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 250000000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::ascend_3));}},
            }
        },
        // Demon Form
        {
            FormID::dark_king, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {
                    double base;
                    auto bpl = ch->getBasePL();
                    if(bpl < 2000000)
                        base = 1.0;
                    else if(bpl < 50000000)
                        base = 1.5;
                    else if(bpl < 600000000)
                        base = 2.0;
                    else
                        base = 3.0;

                    return base + ((base/10.0) * getMasteryTier(ch, FormID::dark_king));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](struct char_data *ch) {
                    double base;
                    if(ch->getCurLF() > 0) {
                        auto bpl = ch->getBasePL();

                        if(bpl < 2000000) {
                            base = 0.4;
                        }
                        else if(bpl < 50000000) {
                            base = 0.6;
                        }
                        else if(bpl < 600000000) {
                            base = 0.8;
                        }
                        else {
                            base = 1.0;
                        }

                        send_to_char(ch, "@MYour tainted lifeforce infuses your attack with a sickly purple hue!@n");

                        base = base * ch->decCurLFPercent(0.05);
                    }

                    return base + ((base/10) * getMasteryTier(ch, FormID::dark_king));
                }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::defense), [](struct char_data *ch) {
                    double base = 0.0;
                    if(ch->getCurLF() > 0) {
                        int chance = 30;
                        auto bpl = ch->getBasePL();

                        if(bpl < 2000000) {
                            chance = 30;
                            base = 0.2;
                        }
                        else if(bpl < 85000000) {
                            chance = 50;
                            base = 0.3;
                        }
                        else if(bpl < 1225000000) {
                            chance = 60;
                            base = 0.5;
                        }
                        else {
                            chance = 80;
                            base = 0.6;
                        }

                        if(axion_dice(0) <= chance && GET_BARRIER(ch) <= 0) {
                            ch->barrier = ch->getCurLF() / 5;
                            send_to_char(ch, "@MThe mantle of your tainted life flares out, creating a barrier.@n");
                        }
                        else
                            send_to_char(ch, "@MYour tainted lifeforce saps the strength of your opponents attack!@n");

                        ch->decCurLFPercent(0.05);
                    } else {
                        send_to_char(ch, "@MYou feel far too exhausted to strengthen yourself!@n");
                    }

                    return -base - ((base/10) * getMasteryTier(ch, FormID::dark_king));
                    }},

            }
        },

        // LSSJ
        {
            FormID::ikari, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {
                    return 1.0 + (0.15 * getMasteryTier(ch, FormID::ikari) + 0.15 * getMasteryTier(ch, FormID::oozaru));
                    }},
                {APPLY_CVIT_BASE,        0.0, ~0,                    [](struct char_data *ch) {
                    return 700000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::ikari) + 0.15 * getMasteryTier(ch, FormID::oozaru));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](struct char_data *ch) {
                    return -0.3 - (0.04 * getMasteryTier(ch, FormID::ikari) + 0.04 * getMasteryTier(ch, FormID::oozaru));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](struct char_data *ch) {
                    return -0.3 - (0.04 * getMasteryTier(ch, FormID::ikari) + 0.04 * getMasteryTier(ch, FormID::oozaru));
                    }},
                {APPLY_CATTR_BASE, 0.0, ~0, [](struct char_data *ch) {
                    return 2 * 1.0 + (0.15 * getMasteryTier(ch, FormID::ikari) + 0.15 * getMasteryTier(ch, FormID::oozaru));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](struct char_data *ch) {
                    return 0.1 * 1.0 + (0.15 * getMasteryTier(ch, FormID::ikari) + 0.15 * getMasteryTier(ch, FormID::oozaru));
                    }},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {
                    return 0.02 * 1.0 + (0.15 * getMasteryTier(ch, FormID::ikari) + 0.15 * getMasteryTier(ch, FormID::oozaru));
                    }},
            }
        },
        {
            FormID::legendary_saiyan, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {
                    return 4.0 + (0.1 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_CVIT_BASE,        0.0, ~0,                    [](struct char_data *ch) {
                    return 100000000 * 1.0 + (0.15 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki),         [](struct char_data *ch) {
                    return 1 * 1.0 + (0.15 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](struct char_data *ch) {
                    return -0.4 - (0.8 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](struct char_data *ch) {
                    return -0.4 - (0.8 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::block), [](struct char_data *ch) {
                    return 0.2 + (0.1 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_CATTR_BASE, 0.0, ~0, [](struct char_data *ch) {
                    return 5 * 1.0 + (0.15 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](struct char_data *ch) {
                    return 0.3 * 1.0 + (0.15 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](struct char_data *ch) {
                    return 0.1 * 1.0 + (0.15 * getMasteryTier(ch, FormID::legendary_saiyan));
                    }},
            }
        },

        // Lycan
        {
            FormID::lesser_lycanthrope, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.5 + (0.1 * getMasteryTier(ch, FormID::lesser_lycanthrope));}},
                {APPLY_DTYPE_BON,   0.0, static_cast<int>(DamType::physical),            [](struct char_data *ch) {return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lesser_lycanthrope));}},
                {APPLY_COMBAT_MULT,      0.0, static_cast<int>(ComStat::parry),                    [](struct char_data *ch) {return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lesser_lycanthrope));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {return 0.1 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lesser_lycanthrope));}},
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {return 0.3 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lesser_lycanthrope));}},
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lesser_lycanthrope));}},
            }
        },
        {
            FormID::lycanthrope, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.0 + (0.1 * getMasteryTier(ch, FormID::lycanthrope));}},
                {APPLY_DTYPE_BON,   0.0, static_cast<int>(DamType::physical),            [](struct char_data *ch) {return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lycanthrope));}},
                {APPLY_COMBAT_MULT,      0.0, static_cast<int>(ComStat::parry),                    [](struct char_data *ch) {return 0.3 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lycanthrope));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lycanthrope));}},
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {return 0.5 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lycanthrope));}},
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {return 0.25 * 1.0 + (0.10 * getMasteryTier(ch, FormID::lycanthrope));}},
            }
        },
        {
            FormID::alpha_lycanthrope, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 1.4 + (0.1 * getMasteryTier(ch, FormID::alpha_lycanthrope));}},
                {APPLY_DTYPE_BON,   0.0, static_cast<int>(DamType::physical),            [](struct char_data *ch) {return 0.7 * 1.0 + (0.10 * getMasteryTier(ch, FormID::alpha_lycanthrope));}},
                {APPLY_COMBAT_MULT,      0.0, static_cast<int>(ComStat::parry),                    [](struct char_data *ch) {return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, FormID::alpha_lycanthrope));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, FormID::alpha_lycanthrope));}},
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {return 0.8 * 1.0 + (0.10 * getMasteryTier(ch, FormID::alpha_lycanthrope));}},
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::ki), [](struct char_data *ch) {return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, FormID::alpha_lycanthrope));}},
            }
        },

        // Unbound Forms
        {
            FormID::potential_unleashed, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {
                    auto cl = ch->lifetimeGrowth;
                    if(cl < 50) return 1.0 + (0.1 * getMasteryTier(ch, FormID::potential_unleashed));
                    if(cl < 100) return 2.0 + (0.1 * getMasteryTier(ch, FormID::potential_unleashed));
                    if(cl < 150) return 3.0 + (0.15 * getMasteryTier(ch, FormID::potential_unleashed));
                    if(cl < 200) return 4.0 + (0.15 * getMasteryTier(ch, FormID::potential_unleashed));
                    if(cl < 250) return 4.5 + (0.2 * getMasteryTier(ch, FormID::potential_unleashed));
                    if(cl >= 250) return 5.0 + (0.2 * getMasteryTier(ch, FormID::potential_unleashed));
                    return 1.0 + (0.1 * getMasteryTier(ch, FormID::potential_unleashed));
                }},
            }
        },
        {
            FormID::evil_aura, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) { return 2.0 + (0.2 * getMasteryTier(ch, FormID::evil_aura));}},
                {APPLY_CVIT_MULT,     0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {
	                double healthBoost = ch->getCurVitalDam(CharVital::powerlevel) * 5.0;
	                return healthBoost + (0.2 * getMasteryTier(ch, FormID::evil_aura));
                }},
                {APPLY_CVIT_BASE,        0.0, ~0,                        [](struct char_data *ch) { return 1300000 * 1.0 + (0.2 * getMasteryTier(ch, FormID::evil_aura));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](struct char_data *ch) {
                    double healthBoost = 0.1;
                    auto chealth = 1.0 - ch->getCurVitalDam(CharVital::powerlevel);
                    if(chealth < 0.75) {
                        healthBoost = (1 - chealth) * (4 + (0.2 * getMasteryTier(ch, FormID::evil_aura)));

                        if(chealth < 0.25)
                            send_to_char(ch, "@RYou feel your agony infuse the attack.\r\n@n");
                        else if(chealth < 0.50)
                            send_to_char(ch, "@RYou feel your rage infuse the attack.\r\n@n");
                        else
                            send_to_char(ch, "@RYou feel your anger infuse the attack.\r\n@n");
                    }
	                return healthBoost;
                }},
            }
        },
        {
            FormID::ultra_instinct, {
                {APPLY_COMBAT_MULT,    0.0, static_cast<int>(ComStat::accuracy),                    [](struct char_data *ch) {return 1 + (0.1 * getMasteryTier(ch, FormID::ultra_instinct));}},
                {APPLY_CVIT_MULT,     0.0, static_cast<int>(CharVital::ki),         [](struct char_data *ch) {return 2 + (0.4 * getMasteryTier(ch, FormID::ultra_instinct));}},
                {APPLY_COMBAT_MULT,      0.0, static_cast<int>(ComStat::damage),                        [](struct char_data *ch) {return -0.7 + (0.2 * getMasteryTier(ch, FormID::ultra_instinct));}},
                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::perfect_dodge), [](struct char_data *ch) {return -0.4 - (0.05 * getMasteryTier(ch, FormID::ultra_instinct));}},
                {APPLY_SKILL, 0.0, (int) SkillID::instinctual_combat, [](struct char_data *ch) {return 20 * getMasteryTier(ch, FormID::ultra_instinct);}},
            }
        },

        {
            FormID::potential_unlocked, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.4 + (0.05 * getMasteryTier(ch, FormID::potential_unlocked));}},
                {APPLY_CATTR_BASE,    2,   ~0},
            }
        },
        {
            FormID::potential_unlocked_max, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.4 + (0.05 * getMasteryTier(ch, FormID::potential_unlocked_max));}},
                {APPLY_CATTR_BASE,    4,   ~0},
            }
        },
        {
            FormID::majinized, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.6 + (0.02 * getMasteryTier(ch, FormID::majinized));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 50000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::majinized));}},
            }
        },
        {
            FormID::divine_water, {
                {APPLY_CVIT_MULT,   0.0, ~0,                    [](struct char_data *ch) {return 0.2 + (0.05 * getMasteryTier(ch, FormID::divine_water));}},
                {APPLY_CVIT_BASE,     0.0, ~0,                    [](struct char_data *ch) {return 15000 * 1.0 + (0.1 * getMasteryTier(ch, FormID::divine_water));}},
                {APPLY_CATTR_BASE,       2,   ~0},
            }
        },


        // Techniques
        {
            FormID::kaioken, {
                {APPLY_CVIT_MULT,   0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {return 0.1 * (ch->transforms[FormID::kaioken].grade);}},
                {APPLY_COMBAT_MULT,     0.0, static_cast<int>(ComStat::damage),            [](struct char_data *ch) {
                    double mult = (1 + (0.05 * getMasteryTier(ch, FormID::kaioken)));
                    if (axion_dice(0) <= (GET_SKILL(ch, (int)SkillID::kaioken) * mult)) {
                        send_to_char(ch, "You push yourself to the limit as you attack!.\r\n");
                        act("$n's red aura ripples with power as they strike!", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)SkillID::kaioken, 0);
                        return (0.06 * ch->transforms[FormID::kaioken].grade) * mult;
                    }
                    return 0.03 * (ch->transforms[FormID::kaioken].grade);}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {return -0.05 * (ch->transforms[FormID::kaioken].grade) + (0.01 * getMasteryTier(ch, FormID::kaioken));}},
                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki),         [](struct char_data *ch) {return -0.05 * (ch->transforms[FormID::kaioken].grade) + (0.01 * getMasteryTier(ch, FormID::kaioken));}},
            }
        },
        {
            FormID::dark_metamorphosis, {
                {APPLY_CVIT_MULT,   0.0, static_cast<int>(CharVital::powerlevel), [](struct char_data *ch) {return 0.6 * (ch->transforms[FormID::dark_metamorphosis].grade) + (0.05 * getMasteryTier(ch, FormID::dark_metamorphosis));}},
                {APPLY_DTYPE_BON,   0.0, static_cast<int>(DamType::ki),                    [](struct char_data *ch) {
                    double mult = (1 + (0.05 * getMasteryTier(ch, FormID::dark_metamorphosis)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)SkillID::dark_metamorphosis) * mult)) {
                        send_to_char(ch, "Your darkness burns into your very ki!.\r\n");
                        act("$n's ki ripples with a horrifying darkness!", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)SkillID::dark_metamorphosis, 0);
                        return (0.05 * ch->transforms[FormID::dark_metamorphosis].grade) * mult;
                    }

                    return 0.0;}},
            }
        },

        {
            FormID::tiger_stance, {
                {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {
                    double mult = (1 + (0.05 * getMasteryTier(ch, FormID::tiger_stance)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)SkillID::tiger_stance) * mult)) {
                        send_to_char(ch, "Primal strength courses through you.\r\n");
                        act("$n lurches forwards with ferocious might.", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)SkillID::tiger_stance, 0);
                        return (0.05 * ch->transforms[FormID::tiger_stance].grade) * mult;
                    }

                    return 0.0;}},
            }
        },
        {
            FormID::eagle_stance, {
                {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {
                    double mult = (1 + (0.05 * getMasteryTier(ch, FormID::eagle_stance)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)SkillID::eagle_stance) * mult)) {
                        send_to_char(ch, "Your mind and ki align, power flaring.\r\n");
                        act("$n's movements slow, their ki redoubling in strength.", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)SkillID::eagle_stance, 0);
                        return (0.05 * ch->transforms[FormID::eagle_stance].grade) * mult;
                    }

                    return 0.0;}},
            }
        },
        {
            FormID::ox_stance, {
                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](struct char_data *ch) {
                    double mult = (1 + (0.05 * getMasteryTier(ch, FormID::ox_stance)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)SkillID::ox_stance) * mult)) {
                        send_to_char(ch, "Hah, that didn't hurt half as much!\r\n");
                        act("$n's hulking form barely seems grazed by the attack!", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)SkillID::ox_stance, 0);
                        return (0.05 * ch->transforms[FormID::ox_stance].grade) * mult;
                    }

                    return 0.0;}},
            }
        },
        {
            FormID::spirit_absorption, {
                {APPLY_COMBAT_BASE, 0.0, static_cast<int>(ComStat::armor), [](struct char_data *ch) {
                    return sqrt(ch->transforms[FormID::spirit_absorption].vars[0]) * 10;}},
                {APPLY_CVIT_BASE,        0.0, ~0,                    [](struct char_data *ch) {
                    return (ch->transforms[FormID::spirit_absorption].vars[0] / 5) * 1 + (0.1 * ch->transforms[FormID::spirit_absorption].grade);}},
            }
        },
        {
            FormID::death_phase, {
                                    {APPLY_CVIT_REGEN_MULT, -0.5, ~0},
                },
        },
        {
            FormID::birth_phase, {
                                        {APPLY_CVIT_REGEN_MULT, 1.0, ~0},
                                        {APPLY_CVIT_MULT, 1.0, ~0, [](auto ch) {
                                            return (IN_ROOM(ch) != NOWHERE && ETHER_STREAM(ch)) ? 0.5 : 0.0;
                                        }},
                                        {APPLY_CATTR_BASE, 5.0, static_cast<int>(CharAttribute::speed)},
                },
        },
        {
            FormID::life_phase, {
                                       {APPLY_CVIT_REGEN_MULT, 2.0, ~0},
                                       {APPLY_CVIT_MULT, 2.0, ~0, [](auto ch) {
                                           return (IN_ROOM(ch) != NOWHERE && ETHER_STREAM(ch)) ? 0.5 : 0.0;
                                       }},
                                       {APPLY_CATTR_BASE, 8.0, static_cast<int>(CharAttribute::speed)},
                },
        }
    };

    static double getModifierHelper(char_data* ch, FormID form, int location, int specific) {
        if (form == FormID::base) return 0.0;
        double out = 0.0;
        if (auto found = trans_affects.find(form); found != trans_affects.end()) {
            for (auto& affect: found->second) {
                if (affect.match(location, specific)) {
                    out += affect.modifier;
                    if(affect.func) {
                        out += affect.func(ch);
                    }
                }
            }
        }

        return out;
    }

    double getModifier(char_data *ch, int location, int specific) {
        double modifier = 0;
        if(!ch->permForms.empty()) {
            for(auto form : ch->permForms) {
                modifier += getModifierHelper(ch, form, location, specific);
            }
        }
        modifier += getModifierHelper(ch, ch->form, location, specific);
        modifier += getModifierHelper(ch, ch->technique, location, specific);

        if(IS_HOSHIJIN(ch)) {
            switch(GET_PHASE(ch)) {
                case 0: // Death phase
                    modifier += getModifierHelper(ch, FormID::death_phase, location, specific);
                    break;
                case 1: // Birth Phase
                    modifier += getModifierHelper(ch, FormID::birth_phase, location, specific);
                    break;
                case 2: // Life phase
                    modifier += getModifierHelper(ch, FormID::life_phase, location, specific);
                    break;
                default:
                    // oops?
                    break;
            }
        }

        return modifier;
    }

    static std::unordered_map<FormID, double> trans_drain = {
        // Saiyan forms.
        {FormID::super_saiyan_1, .06},
        {FormID::super_saiyan_2, .08},
        {FormID::super_saiyan_3, .2},
        {FormID::super_saiyan_4, .15},

        // Lssj
        {FormID::ikari, .2},
        {FormID::legendary_saiyan, .4},

        // Human Forms
        {FormID::super_human_1, .06},
        {FormID::super_human_2, .08},
        {FormID::super_human_3, .12},
        {FormID::super_human_4, .14},

        // icer Forms
        {FormID::icer_1, .06},
        {FormID::icer_2, .08},
        {FormID::icer_3, .12},
        {FormID::icer_4, .14},

        // Konatsu Forms
        {FormID::shadow_first, .06},
        {FormID::shadow_second, .08},
        {FormID::shadow_third, .1},

        // Namekian Forms
        {FormID::super_namekian_1, .06},
        {FormID::super_namekian_2, .07},
        {FormID::super_namekian_3, .08},
        {FormID::super_namekian_4, .1},

        // Mutant Forms
        {FormID::mutate_1, .05},
        {FormID::mutate_2, .07},
        {FormID::mutate_3, .1},

        // Bio, Android, Majin, have no drain.

        // Kai
        {FormID::mystic_1, .05},
        {FormID::mystic_2, .07},
        {FormID::mystic_3, .1},

        {FormID::divine_halo, .1},

        // Demon
        {FormID::dark_king, .1},

        // Lycan
        {FormID::lesser_lycanthrope, .1},

        // Unbound Alternate Forms
        {FormID::potential_unleashed, .1},
        {FormID::evil_aura, .08},
        {FormID::ultra_instinct, .08},

        // Unbound Permanant Forms


        // Techniques
        {FormID::kaioken, .05},
        {FormID::dark_metamorphosis, .1},

        {FormID::tiger_stance, .02},
        {FormID::eagle_stance, .02},
        {FormID::ox_stance, .02},

        {FormID::spirit_absorption, .15},
    };

    double getStaminaDrain(char_data* ch, FormID form, bool upkeep) {
        if (ch->form == FormID::base) return 0.0;

        double drain = 0.0;

        if (auto found = trans_drain.find(form); found != trans_drain.end()) {
            drain = found->second;
        }

        if(form == FormID::super_saiyan_1 && PLR_FLAGGED(ch, PLR_FPSSJ)) drain *= 0.5;

        if (ch->getRoomFlag(ROOM_RHELL) || ch->getRoomFlag(ROOM_AL)) 
            drain *= 0.75;

        if(ch->form != FormID::base && ch->technique != FormID::base)
            drain *= 2;

        if(upkeep) {
            drain *= 0.01;
            if(ch->race == RaceID::icer) drain = 0.0;
        }
        return drain * ch->transforms[form].grade * (1.0 + ch->getAffectModifier(APPLY_TRANS_UPKEEP_CVIT, static_cast<int>(CharVital::stamina)));
    }

    static const std::unordered_set<FormID> moonForms = {
        FormID::oozaru,
        FormID::golden_oozaru,
        FormID::lycanthrope,
        FormID::alpha_lycanthrope,
    };

    void gamesys_transform(uint64_t heartPulse, double deltaTime) {
        // TODO: replace this with a subscription for anyone who's not in base form.
        auto subs = characterSubscriptions.all("transforms");
        for(auto ch : filter_raw(subs)) {

            // check transform logic...
            auto form = ch->form;
            auto technique = ch->technique;
            auto &data = ch->transforms[form];
            double timeBefore = data.timeSpentInForm;
            data.timeSpentInForm += deltaTime;
            double timeAfter = data.timeSpentInForm;

            auto &techdata = ch->transforms[technique];
            double techTimeBefore = data.timeSpentInForm;
            techdata.timeSpentInForm += deltaTime;
            double techTimeAfter = data.timeSpentInForm;

            if(moonForms.contains(ch->form)) {
                if(auto moonlight = ch->getLocationEnvironment(ENV_MOONLIGHT); moonlight >= 100.0) {
                    // Top off the blutz.
                    data.blutz = 60.0 * 30;
                }
                data.blutz -= deltaTime;
                if(data.blutz <= 0 || !ch->playerFlags.test(PLR_TAIL)) {
                    data.blutz = 0.0;
                    oozaru_revert(ch);
                }
            }

            double kigain = getModifierHelper(ch, form, APPLY_CVIT_REGEN_MULT,
                                              static_cast<int>(CharVital::ki));
            double plgain = getModifierHelper(ch, form, APPLY_CVIT_REGEN_MULT,
                                              -static_cast<int>(CharVital::powerlevel));
            ch->incCurHealthPercent(plgain);
            ch->incCurKIPercent(kigain);

            // Notify at thresholds
            if(form != FormID::base) {
                if(timeBefore < MASTERY_THRESHOLD && timeAfter >= MASTERY_THRESHOLD)
                    send_to_char(ch, "@mSomething settles in your core, you feel more comfortable using @n" + getName(ch, form) + "\r\n");

                if(timeBefore < LIMIT_THRESHOLD && timeAfter >= LIMIT_THRESHOLD)
                    send_to_char(ch, "@mYou feel power overwhelming emanate from your core, you instinctively know you've hit the limit of @n" + getName(ch, form) + "\r\n");

                if(timeBefore < LIMITBREAK_THRESHOLD && timeAfter >= LIMITBREAK_THRESHOLD && data.limitBroken == true)
                    send_to_char(ch, "@mThere's a snap as a tide of power rushes throughout your veins,@n " + getName(ch, form) + " @mhas evolved.@n\r\n");
            }
            if(technique != FormID::base) {
                if(techTimeBefore < MASTERY_THRESHOLD && techTimeAfter >= MASTERY_THRESHOLD)
                    send_to_char(ch, "@mSomething settles in your core, you feel more comfortable using @n" + getName(ch, technique) + "\r\n");

                if(techTimeBefore < LIMIT_THRESHOLD && techTimeAfter >= LIMIT_THRESHOLD)
                    send_to_char(ch, "@mYou feel power overwhelming emanate from your core, you instinctively know you've hit the limit of @n" + getName(ch, technique) + "\r\n");

                if(techTimeBefore < LIMITBREAK_THRESHOLD && techTimeAfter >= LIMITBREAK_THRESHOLD && techdata.limitBroken == true)
                    send_to_char(ch, "@mThere's a snap as a tide of power rushes throughout your veins,@n " + getName(ch, technique) + " @mhas evolved.@n\r\n");
            }


            // Check stamina drain.
            if (auto drain = (getStaminaDrain(ch, ch->form, true) + getStaminaDrain(ch, ch->technique, true)) * deltaTime; drain > 0) {
                if(ch->decCurSTPercent(drain) == 0) {
                    act("@mExhausted of stamina, your body forcibly reverts from its form.@n\r\n", true, ch, nullptr,
                        nullptr, TO_CHAR);
                    act("@C$n @wbreathing heavily, reverts from $s form, returning to normal.@n\r\n", true, ch, nullptr,
                        nullptr, TO_ROOM);
                    revert(ch);

                }
            }
        }

    }

    struct trans_echo {
        std::string self;
        std::string room;
    };

    static std::unordered_map<FormID, trans_echo> trans_echoes = {
            {
                    FormID::super_saiyan_1,     {
                                                     "@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n",
                                                     "@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n"
                                             }
            },

            {
                    FormID::super_saiyan_2,    {
                                                     "@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n",
                                                     "@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n"
                                             }
            },

            {
                    FormID::super_saiyan_3,    {
                                                     "@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n",
                                                     "@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n"
                                             }
            },

            {
                    FormID::super_saiyan_4,    {
                                                     "@WHaving absorbed enough blutz waves, your body begins to transform! Red fur grows over certain parts of your skin as your hair grows longer and unkempt. A red outline forms around your eyes while the irises of those very same eyes change to an amber color. Energy crackles about your body violently as you achieve the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n",
                                                     "@WHaving absorbed enough blutz waves, @C$n@W's body begins to transform! Red fur grows over certain parts of $s skin as $s hair grows longer and unkempt. A red outline forms around $s eyes while the irises of those very same eyes change to an amber color. Energy crackles about $s body violently as $e achieves the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n"
                                             }
            },

            {
                FormID::ikari,    {
                                                    "@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n",
                                                    "@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n"
                                            }
            },

            {
                FormID::legendary_saiyan,    {
                                                    "@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n",
                                                    "@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n"
                                            }
            },




            // Human Forms
            {
                    FormID::super_human_1,      {
                                                     "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!",
                                                     "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!"
                                             }
            },

            {
                    FormID::super_human_2,     {
                                                     "@WSuddenly a bright white aura bursts into existance around your body, you feel the intensity of your hidden potential boil until it can't be contained any longer! Waves of ki shoot out from your aura streaking outwards in many directions. A roar that shakes everything in the surrounding area sounds right as your energy reaches its potential and you achieve @CSuper @cHuman @GSecond@W!",
                                                     "@C$n@W is suddenly covered with a bright white aura as $e grits $s teeth, apparently struggling with the power boiling to the surface! Waves of ki shoot out from $s aura, streaking in several directions as a mighty roar shakes everything in the surrounding area. As $s aura calms $e smiles, having achieved @CSuper @cHuman @GSecond@W!"
                                             }
            },

            {
                    FormID::super_human_3,     {
                                                     "@WYou clench both of your fists as the bright white aura around your body is absorbed back into your flesh. As it is absorbed, your muscles triple in size and electricity crackles across your flesh. You grin as you feel the power of @CSuper @cHuman @GThird@W!",
                                                     "@C$n@W clenches both of $s fists as the bright white aura around $s body is absorbed back into $s flesh. As it is absorbed, $s muscles triple in size and bright electricity crackles across $s flesh. $e smiles as $e achieves the power of @CSuper @cHuman @GThird@W!"
                                             }
            },

            {
                    FormID::super_human_4,     {
                                                     "@WYou grit your teeth and clench your fists as a sudden surge of power begins to tear through your body! Your muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over you, flashes of multicolored light flaring up in rising stars around your new form, a corona of glory! You feel your ultimate potential realized as you ascend to @CSuper @cHuman @GFourth@W!@n",
                                                     "@C$n@W grits $s teeth and clenches $s fists as a sudden surge of power begins to tear through $s body! $n@W's muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over $m, flashes of multicolored light flare up in rising stars around $s new form, a corona of glory! $n@W smiles as his ultimate potential is realized as $e ascends to @CSuper @cHuman @GFourth@W!@n"
                                             }
            },

            // Icer Forms
            {
                    FormID::icer_1,       {
                                                     "@WYou yell with pain as your body begins to grow and power surges within! Your legs expand outward to triple their previous length. Soon after your arms, chest, and head follow. Your horns grow longer and curve upwards while lastly your tail expands. You are left confidently standing, having completed your @GFirst @cTransformation@W.@n",
                                                     "@C$n@W yells with pain as $s body begins to grow and power surges outward! $s legs expand outward to triple their previous length. Soon after $s arms, chest, and head follow. $s horns grow longer and curve upwards while lastly $s tail expands. $e is left confidently standing, having completed $s @GFirst @cTransformation@W.@n"
                                             }
            },

            {
                    FormID::icer_2,      {
                                                     "@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n",
                                                     "@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n"
                                             }
            },

            {
                    FormID::icer_3,       {
                                                     "@WA blinding light surrounds your body while your rising power begins to rip up the ground beneath you! Your skin and torso shell begin to crack as your new body struggles to free its self. Huge chunks of debris lift free of the ground as your power begins to rise to unbelievable heights. Suddenly your old skin and torso shell burst off from your body, leaving a sleek form glowing where they had been. Everything comes crashing down as your power evens out, leaving you with your @GThird @cTransformation @Wcompleted!@n",
                                                     "@WA blinding light surrounds @C$n@W's body while $s rising power begins to rip up the ground beneath $m! $s skin and torso shell begin to crack as $s new body struggles to free its self. Huge chunks of debris lift free of the ground as $s power begins to rise to unbelievable heights. Suddenly $s old skin and torso shell burst off from $s body, leaving a sleek form glowing where they had been. Everything comes crashing down as @C$n@W's power evens out, leaving $m with $s @GThird @cTransformation @Wcompleted!@n"
                                             }
            },

            {
                    FormID::icer_4,      {
                                                     "@WA feeling of complete power courses through your viens as your body begins to change radically! You triple in height while a hard shell forms over your entire torso. Hard bones grow out from your head forming four ridges that jut outward. A hard covering grows up over your mouth and nose completing the transformation! A dark crimson aura flames around your body as you realize your @GFourth @cTransformation@W!@n",
                                                     "@C$n@W's body begins to change radically! $e triples in height while a hard shell forms over $s entire torso. Hard bones grow out from $s head forming four ridges that jut outward. A hard covering grows up over $s mouth and nose completing the transformation! A dark crimson aura flames around @C$n@W's body as $e realizes $s @GFourth @cTransformation@W!@n"
                                             }
            },


            // Konatsu Forms
            {
                    FormID::shadow_first,     {
                                                     "@WA dark shadowy aura with flecks of white energy begins to burn around your body! Strength and agility can be felt rising up within as your form becomes blurred and ethereal looking. You smile as you realize your @GFirst @DShadow @BForm@W!@n",
                                                     "@WA dark shadowy aura with flecks of white energy begins to burn around @C$n@W's body! $s form becomes blurred and ethereal-looking as $s muscles become strong and lithe. $e smiles as $e achieves $s @GFirst @DShadow @BForm@W!@n"
                                             }
            },

            {
                    FormID::shadow_second,    {
                                                     "@WThe shadowy aura surrounding your body burns larger than ever as dark bolts of purple electricity crackles across your skin. Your eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into your aura in an instant as you achieve your @GSecond @DShadow @BForm@W!@n",
                                                     "@WThe shadowy aura surrounding @C$n@W's body burns larger than ever as dark bolts of purple electricity crackles across $s skin. $s eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into $s aura in an instant as $e achieves $s @GSecond @DShadow @BForm@W!@n"
                                             }
            },

            {
                    FormID::shadow_third,     {
                                                     "@WThe shadowy aura around you explodes outward as your power begins to rise!  You're overcome with a sudden realization, that the shadows are an extension of yourself, that light isn't needed for your shadows to bloom.  With this newfound wisdom comes ability and power!  The color in your aura drains as the shadows slide inward and cling to your body like a second, solid black skin!  Shockwaves roll off of you in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath you, slowly spreading the shadows to the whole area, projecting onto any surface nearby!  Purple and black electricity crackle in your solid white aura, and you grin as you realize your @GThird @DShadow @BForm@W!@n",
                                                     "@WThe shadowy aura around $n explodes outward as $s power begins to rise!  Realization dawns on $s face, followed shortly by confidence! The color in $s aura drains as the shadows slide inward to cling to $s body like a second, solid black skin! Shockwaves roll off of $n in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath them, slowly spreading the shadows to the whole area, projecting onto any surface nearby! Purple and black electricity crackle in $s solid white aura, and he grins as $e realizes $s @GThird @DShadow @BForm@W!@n"
                                             }
            },

            // Namekian Forms
            {
                    FormID::super_namekian_1,   {
                                                     "@WYou crouch down and clench your fists as your muscles begin to bulge! Sweat pours down your body as the ground beneath your feet cracks and warps under the pressure of your rising ki! With a sudden burst that sends debris flying you realize a new plateau in your power, having achieved @CSuper @gNamek @GFirst@W!@n",
                                                     "@C$n @Wcrouches down and clenches $s fists as $s muscles begin to bulge! Sweat pours down $s body as the ground beneath $s feet cracks and warps under the pressure of  $s rising ki! With a sudden burst that sends debris flying $e seems to realize a new plateau in $s power, having achieved @CSuper @gNamek @GFirst@W!@n"
                                             }
            },
            {
                    FormID::super_namekian_2,  {
                                                     "@WYou gasp in shock as a power within your body that you had not been aware of begins to surge to the surface! Your muscles grow larger as energy crackles between your antennae intensely! A shockwave of energy explodes outward as you achieve a new plateau in power, @CSuper @gNamek @GSecond@W!@n",
                                                     "@C$n @Wgasps in shock as a power within $s body begins to surge out! $s muscles grow larger as energy crackles between $s antennae intensely! A shockwave of energy explodes outward as $e achieves a new plateau in power, @CSuper @gNamek @GSecond@W!@n"
                                             }
            },
            {
                    FormID::super_namekian_3,  {
                                                     "@WA fierce clear aura bursts up around your body as you struggle to control a growing power within! Energy leaks off of your aura at an astounding rate filling the air around you with small orbs of ki. As your power begins to level off the ambient ki hovering around you is absorbed inward in a sudden shock that leaves your skin glowing! You have achieved a rare power, @CSuper @gNamek @GThird@W!@n",
                                                     "@WA fierce clear aura bursts up around @C$n@W's body as $e struggles to control $s own power! Energy leaks off of $s aura at an astounding rate filling the air around $m with small orbs of ki. As $s power begins to level off the ambient ki hovering around $m is absorbed inward in a sudden shock that leaves $s skin glowing! $e has achieved a rare power, @CSuper @gNamek @GThird@W!@n"
                                             }
            },
            {
                    FormID::super_namekian_4,  {
                                                     "@WAn inner calm fills your mind as your power surges higher than ever before. Complete clarity puts everything once questioned into perspective. While this inner calm is filling your mind, an outer storm of energy erupts around your body! The storm of energy boils and crackles while growing larger. You have achieved @CSuper @gNamek @GFourth@W, a mystery of the ages.@n",
                                                     "@C$n@W smiles calmly as a look of complete understand fills $s eyes. While $e remains perfectly calm and detached a massivly powerful storm of energy erupts from his body. This storm of energy shimmers with the colors of the rainbow and boils and crackles with awesome power! $s smile disappears as he realizes a mysterious power of the ages, @CSuper @gNamek @GFourth@W!@n"
                                             }
            },

            {
                    FormID::mutate_1,     {
                                                     "@WYour flesh grows tougher as power surges up from within. Your fingernails grow longer, sharper, and more claw-like. Lastly your muscles double in size as you achieve your @GFirst @mMutation@W!@n",
                                                     "@C$n@W flesh grows tougher as power surges up around $m. $s fingernails grow longer, sharper, and more claw-like. Lastly $s muscles double in size as $e achieves $s @GFirst @mMutation@W!@n"
                                             }
            },

            {
                    FormID::mutate_2,    {
                                                     "@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n",
                                                     "@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n"
                                             }
            },

            {
                    FormID::mutate_3,     {
                                                     "@WA dark cyan aura bursts up around your body as the ground begins to crack beneath you! You scream out in pain as your power begins to explode! Two large spikes grow out from your shoulder blades as you reach your @GThird @mMutation!@n",
                                                     "@WA dark cyan aura bursts up around @C$n@W's body as the ground begins to crack beneath $m and $e screams out in pain as $s power begins to explode! Two large spikes grow out from $s shoulder blades as $e reaches $s @GThird @mMutation!@n"
                                             }
            },

            {
                    FormID::bio_mature,       {
                                                     "@gYou bend over as @rpain@g wracks your body! Your limbs begin to grow out, becoming more defined and muscular. As your limbs finish growing outward you feel a painful sensation coming from your back as a long tail with a spike grows out of your back! As the pain subsides you stand up straight and a current of power shatters part of the ground beneath you. You have @rmatured@g beyond your @Gl@ga@Dr@gv@Ga@ge stage!@n",
                                                     "@W$n @gbends over as a @rpainful@g look covers $s face! $s limbs begin to grow out, becoming more defined and muscular. As $s limbs finish growing outward $e screams as a long tail with a spike grows rips out of $s back! As $e calms $e stands up straight and a current of power shatters part of the ground beneath $m. $e has @rmatured@g beyond $s @Gl@ga@Dr@gv@Ga@ge stage!@n"
                                             }
            },

            {
                    FormID::bio_semi_perfect,  {
                                                     "@WYour exoskeleton begins to glow spectacularly while the shape of your body begins to change. Your tail shrinks slightly. Your hands, feet, and facial features become more refined. While your body colors change slightly. The crests on your head change, standing up straighter on either side of your head as well. As you finish transforming a wave of power floods your being. You have achieved your @gSemi@D-@GPerfect @BForm@W!@n",
                                                     "@C$n@W's exoskeleton begins to glow spectacularly while the shape of $s body begins to change. $s tail shrinks slightly. $s hands, feet, and facial features become more refined. While $s body colors change slightly. The crests on $s head change, standing up straighter on either side of $s head as well. As $e finishes transforming a wave of power rushes out from $m. $e has achieved $s @gSemi@D-@GPerfect @BForm@W!@n"
                                             }
            },

            {
                    FormID::bio_perfect,      {
                                                     "@WYour whole body is engulfed in blinding light as your exoskeleton begins to change shape! Your hands, feet, and facial features become more refined and humanoid. While your colors change, becoming more subdued and neutral. A bright golden aura bursts up around your body as you achieve your @GPerfect @BForm@W!@n",
                                                     "@C$n@W whole body is engulfed in blinding light as $s exoskeleton begins to change shape! $s hands, feet, and facial features become more refined and humanoid. While $s colors change, becoming more subdued and neutral. A bright golden aura bursts up around $s body as $e achieves $s @GPerfect @BForm@W!@n"
                                             }
            },

            {
                    FormID::bio_super_perfect, {
                                                     "@WA rush of power explodes from your perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout your aura intensely while your muscles grow slightly larger but incredibly dense. You smile as you realize that you have taken your perfect form beyond imagination. You are now @CSuper @GPerfect@W!@n",
                                                     "@WA rush of power explodes from @C$n@W's perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout $s aura intensely while $s muscles grow slightly larger but incredibly dense. $e smiles as $e has taken $s perfect form beyond imagination. $e is now @CSuper @GPerfect@W!@n"
                                             }
            },

            {
                    FormID::android_1,       {
                                                     "@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                                                     "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
                                             }
            },

            {
                    FormID::android_2,       {
                                                     "@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                                                     "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
                                             }
            },

            {
                    FormID::android_3,       {
                                                     "@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                                                     "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
                                             }
            },

            {
                    FormID::android_4,       {
                                                     "@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                                                     "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
                                             }
            },

            {
                    FormID::android_5,       {
                                                     "@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                                                     "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
                                             }
            },

            {
                    FormID::android_6,       {
                                                     "@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
                                                     "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
                                             }
            },

            {
                    FormID::maj_affinity,     {
                                                     "@WA dark pink aura bursts up around your body as images of good and evil fill your mind! You feel the power within your body growing intensely, reflecting your personal alignment as your body changes!@n",
                                                     "@WA dark pink aura bursts up around @C$n@W's body as images of good and evil fill $s mind! $e feels the power within $s body growing intensely, reflecting $s personal alignment as $s body changes!@n"
                                             }
            },

            {
                    FormID::maj_super,        {
                                                     "@WAn intense pink aura surrounds your body as it begins to change, taking on the characteristics of those you have ingested! Explosions of pink energy burst into existence all around you as your power soars to sights unseen!@n",
                                                     "@WAn intense pink aura surrounds @C$n@W's body as it begins to change, taking on the characteristics of those $e has ingested! Explosions of pink energy burst into existence all around $m as $s power soars to sights unseen!@n"
                                             }
            },

            {
                    FormID::maj_true,         {
                                                     "@WRipples of intense pink energy rush upwards around your body as it begins to morph into its truest form! The ground beneath your feet forms into a crater from the very pressure of your rising ki! Earthquakes shudder throughout the area as your finish morphing!@n",
                                                     "@WRipples of intense pink energy rush upwards around @C$n@W's body as it begins to morph into its truest form! The ground beneath $s feet forms into a crater from the very pressure of $s rising ki! Earthquakes shudder throughout the area as $e finishes morphing!@n"
                                             }
            },

            {
                    FormID::mystic_1,     {
                                                     "@WThoughts begin to flow through your mind of events throughout your life. The progression leads up to more recent events and finally to this very moment. All of it's significance overwhelms you momentarily and your motivation and drive increase. As your attention is drawn back to your surroundings, you feel as though your thinking, senses, and reflexes have sharpened dramatically.  At the core of your being, a greater depth of power can be felt.@n",
                                                     "@W$n@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings. Though it's not apparent why they were so distracted, something definitely seems different about $m.@n"
                                             }
            },

            {
                    FormID::mystic_2,    {
                                                     "@WYou feel a sudden rush of emotion, escalating almost to a loss of control as your thoughts race. Your heart begins to beat fast as memories mix with the raw emotion. A faint blue glow begins to surround you. As your emotions level off, you feel a deeper understanding of the universe as you know it. You visibly calm back down to an almost steely eyed resolve as you assess your surroundings. The blue aura wicks around you for a few moments and then dissipates. Thought it's full impact is not yet clear to you, you are left feeling as though both your power and inner strength have turned into nearly bottomless wells.@n",
                                                     "@W$n@W's appears to be hit by some sudden pangs of agony, their face contorted in pain.  After a moment a faint blue aura appears around them, glowing brighter as time passes. You can feel something in the pit of your stomach, letting you know that something very significant is changing around you. Before long $n@W's aura fades, leaving a very determined looking person in your presence.@n"
                                             }
            },

            {
                    FormID::mystic_3,     {
                                                     "@WYour minds' eye becomes overwhelmed by secrets unimaginable. The threads of the very universe become visible in your heightened state of awareness. Reaching out, a single thread vibrates, producing a @Rred @Wcolor -- yours. Your fingertips brush against it and your senses become clouded by a vast expanse of white color and noise. As your vision and hearing return, you understand the threads tying every living being together. Your awareness has expanded beyond comprehension!@n",
                                                     "@C$n@W's eyes grow wide, mouth agape. $s body begins to shiver uncontrollably! $s arms reaches out cautiously before falling back down to $s side. $s face relaxes visibly, features returning to a normal state. $s irises remain larger than before, a slight smile softening $s gaze.@n"
                                             }
            },

            {
                    FormID::ascend_1,     {
                                                     "@WYour mind accelerates working through the mysteries of the universe while at the same time your body begins to change! Innate nano-technology within your body begins to activate, forming flexible metal plating across parts of your skin!@n",
                                                     "@C$n@W begins to write complicated calculations in the air as though $e were possessed while at the same time $s body begins to change! Innate nano-technology within $s body begins to activate, forming flexible metal plating across parts of $s skin!@n"
                                             }
            },

            {
                    FormID::ascend_2,    {
                                                     "@WComplete understanding of every physical thing floods your mind as the nano-technology within you continues to change your body! Your eyes change; becoming glassy, hard, and glowing. Your muscles merge with a nano-fiber strengthening them at the molecular level! Finally your very bones become plated in nano-metals that have yet to be invented naturally!@n",
                                                     "@C$n@.s nano-technology continues to change $s body! $s eyes change; becoming glassy, hard, and glowing. $s muscles merge with a nano-fiber strengthening them at the molecular level! Finally $s very bones become plated in nano-metals that have yet to be invented naturally!@n"
                                             }
            },

            {
                    FormID::ascend_3,     {
                                                     "@WYou have reached the final stage of enlightenment and the nano-technology thriving inside you begin to initiate the changes! Your neural pathways become refined, your reflexes honed, your auditory and ocular senses sharpening far beyond normal levels! Your gravitational awareness improves, increasing sensitivity and accuracy in your equilibrum!@n",
                                                     "@C$n begins to mumble quietly, slowly at first and gradually picking up speed. A glint is seen from $s eyes and $s arms reach outwards briefly as $e appears to catch his balance. $s arms drop back to $s sides as balance is regained, a vicious smile on $s face.@n"
                                             }
            },

            {       FormID::oozaru,          {
                                                     "@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Hair sprouts  all over your body and your teeth become sharp as your body takes on the Oozaru form!@n",
                                                     "@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant ape!@n"
                                             }},
            {       FormID::golden_oozaru,          {
                                                     "@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Golden hair sprouts all over your body, your teeth sharpen, as your body takes on the Golden Oozaru form!@n",
                                                     "@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Golden hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant golden ape!@n"
                                             }},

            {
                    FormID::dark_king,     {
                                                     "@WYou kindle the dark seed within you, rooted through your body, and in a mere moment its roots pulse with power untold. Wrapping them within yourself, bound eternal, your body begins to shift as your combined strength becomes manifest.@n",
                                                     "@C$n strains for a second as their eyes shine a fierce red, soon their body contorts, unleashing an ominous aura from within themself.@n"
                                             }
            },

            {
                    FormID::potential_unleashed,     {
                                                     "@WThere is a well of potential within you, beyond simple forms. Reaching deep you tug at it, pulling what was once simple potential and flaring it to life in a burst of simple power.@n",
                                                     "@C$n takes a moment for themselves, focusing inwards. A moment later a white glow of pure aura explodes outwards, energy unbound.@n"
                                             }
            },

            {
                    FormID::evil_aura,       {
                                                     "@WMorality is just a crutch for the weak. You know this better than anyone, for discarding it gleans a look at true power. You indulge. And outwards ruptures your Ki, tainted and drenched in evil beyond parallel.@n",
                                                     "@C$n gives a sickeningly twisted smile as something behind their eyes relents. Forward from them rushes forwards a surge of black ki, their evil manifest, with nothing left to hold it back.@n"
                                             }
            },
            {
                    FormID::ultra_instinct,   {
                                                     "@WThe tempo of the fight soon gives way to a single rhythm, every blow falling in perfect harmony, you can see it all as your emotions drain away into a perfect and sharp focus.@n",
                                                     "@C$n starts to slow their movements as emotion fades away from them, each move growing precise, calculated with perfect accuracy.@n"
                                             }
            },

            // Techniques
            {
                    FormID::kaioken,        {
                                                     "@rA blazing red aura bursts up around your body, flashing intensely as the pressure of your aura magnifies!@n",
                                                     "@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely as the pressure of $e aura magnifies!@n"
                                             }
            },


    };

    void handleEchoTransform(struct char_data* ch, FormID form) {
        if (auto found = trans_echoes.find(form); found != trans_echoes.end()) {
            auto& echo = found->second;
            act(echo.self.c_str(), true, ch, nullptr, nullptr, TO_CHAR);
            act(echo.room.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
        } else {
            auto name = getName(ch, form);
            auto self = fmt::format("@wYou transform to {}.@n", name);
            auto room = fmt::format("@w$n@w transforms into {}.@n", name);

            act(self.c_str(), true, ch, nullptr, nullptr, TO_CHAR);
            act(room.c_str(), true, ch, nullptr, nullptr, TO_ROOM);

        }
    }

    void handleEchoRevert(struct char_data* ch, FormID form) {
        if (form == FormID::base) return;

        auto name = getName(ch, form);
        auto self = fmt::format("@wYou revert from {}.@n", name);
        auto room = fmt::format("@w$n@w reverts from {}.@n", name);

        act(self.c_str(), true, ch, nullptr, nullptr, TO_CHAR);
        act(room.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
    }

    static const std::vector<FormID> forms = {
        // Universal
        FormID::base,
        FormID::custom_1,
        FormID::custom_2,
        FormID::custom_3,
        FormID::custom_4,
        FormID::custom_5,
        FormID::custom_6,
        FormID::custom_7,
        FormID::custom_8,
        FormID::custom_9,

        // Saiyan'y forms.
        FormID::oozaru,
        FormID::golden_oozaru,
        FormID::super_saiyan_1,
        FormID::super_saiyan_2,
        FormID::super_saiyan_3,
        FormID::super_saiyan_4,
        FormID::super_saiyan_god,
        FormID::super_saiyan_blue,

        // Lssj
        FormID::ikari,
        FormID::legendary_saiyan,

        // Human'y Forms
        FormID::super_human_1,
        FormID::super_human_2,
        FormID::super_human_3,
        FormID::super_human_4,

        // Icer'y Forms
        FormID::icer_1,
        FormID::icer_2,
        FormID::icer_3,
        FormID::icer_4,
        FormID::icer_metal,
        FormID::icer_golden,
        FormID::icer_black,

        // Konatsu
        FormID::shadow_first,
        FormID::shadow_second,
        FormID::shadow_third,

        // Namekian
        FormID::super_namekian_1,
        FormID::super_namekian_2,
        FormID::super_namekian_3,
        FormID::super_namekian_4,

        // Mutant
        FormID::mutate_1,
        FormID::mutate_2,
        FormID::mutate_3,

        // BioAndroid
        FormID::bio_mature,
        FormID::bio_semi_perfect,
        FormID::bio_perfect,
        FormID::bio_super_perfect,

        // Android
        FormID::android_1,
        FormID::android_2,
        FormID::android_3,
        FormID::android_4,
        FormID::android_5,
        FormID::android_6,

        // Majin
        FormID::maj_affinity,
        FormID::maj_super,
        FormID::maj_true,

        // Kai
        FormID::mystic_1,
        FormID::mystic_2,
        FormID::mystic_3,

        FormID::divine_halo,

        // Tuffle
        FormID::ascend_1,
        FormID::ascend_2,
        FormID::ascend_3,

        // Demon
        FormID::dark_king,

        // Lycanthrope
        FormID::lesser_lycanthrope,
        FormID::lycanthrope,
        FormID::alpha_lycanthrope,

        // Unbound Alternate Forms
        FormID::potential_unleashed,
        FormID::evil_aura,
        FormID::ultra_instinct,

        // Unbound Perm Forms
        FormID::potential_unlocked,
        FormID::potential_unlocked_max,
        FormID::majinized,
        FormID::divine_water,

        // Techniques
        FormID::kaioken,
        FormID::dark_metamorphosis,

        FormID::tiger_stance,
        FormID::eagle_stance,
        FormID::ox_stance,

        FormID::spirit_absorption,
    };


    std::optional<FormID> findForm(char_data* ch, const std::string& form) {
        for (auto formFound : forms) {
            if (form == getAbbr(ch, formFound)) {
                return formFound;
                break;
            }
        }

        //Failure state
        return {};
    }

    void initTransforms(char_data* ch) {
        auto transforms = ch->transforms;

        for (auto unlockForms : trans_unlocks) {
            if (!transforms.contains(unlockForms.first)) {
                if(unlockForms.second(ch))
                    ch->addTransform(unlockForms.first);
            }
        }
    }

    bool requirementsMet(char_data* ch, FormID form) {
        if(trans_requirements.contains(form)) {
            auto formReq = trans_requirements.find(form);
            return formReq->second(ch);
        }

        return true;
    }

    void removeExclusives(char_data* ch, FormID form) {
        if(!transform_exclusive.contains(form) || !ch->transforms[form].unlocked)
            return;

        auto formList = transform_exclusive.find(form);
        
        for (auto& checkForm : formList->second ) {
            if (ch->transforms.contains(checkForm))
                ch->transforms.erase(checkForm);
        }
    }

    std::unordered_set<FormID> getFormsFor(char_data* ch) {
        initTransforms(ch);
        auto forms = ch->transforms;
        std::unordered_set<FormID> pforms;
        
        for (auto form : forms) {
            if(form.first == FormID::base || form.first == FormID::oozaru || form.first == FormID::golden_oozaru)
                form.second.visible = false;
            if(form.second.visible && requirementsMet(ch, form.first)) {
                pforms.insert(form.first);
            }
        }
        return pforms;
    }

    void displayForms(char_data* ch) {
        auto forms = getFormsFor(ch);
        if (forms.empty()) {
            send_to_char(ch, "You have no forms. Bummer.\r\n");
            return;
        }

        send_to_char(ch, "              @YForms@n\r\n");
        
        auto ik = ch->internalGrowth;
        std::vector<std::string> form_names;
        std::vector<FormID> locked_forms;
        bool bannerDisplayed = false;
        for (auto form: forms) {
            bool unlocked = ch->transforms[form].unlocked;
            bool permActive = ch->permForms.contains(form);
            auto name = getName(ch, form);
            if(getMasteryTier(ch, form) > 3)
                name = "@RLIMITBREAK@n " + name;
            else if(getMasteryTier(ch, form) > 2)
                name = "@RLIMIT@n " + name;
            else if (getMasteryTier(ch, form) > 1)
                name = "@BMASTERED@n " + name;
            if(!permActive) {
                if (unlocked) {
                    if (!bannerDisplayed) {
                        send_to_char(ch, "@b-------------------Unlocked--------------------@n\r\n");
                        bannerDisplayed = true;
                    }
                    send_to_char(ch, "@W%s@n\r\n", name);
                } else {
                    //send_to_char(ch, "@W%s@n @R-@G %s Growth Req\r\n", name,
                    //       (ik >= (req * 0.75) && !unlocked) ? add_commas(req) : "??????????");
                    locked_forms.push_back(form);
                }

                form_names.push_back(getAbbr(ch, form));
            }
        }

        if(!locked_forms.empty()) {
            send_to_char(ch, "@b--------------------Locked---------------------@n\r\n");
            for (auto form: locked_forms) {
                auto name = getName(ch, form);
                auto req = getRequiredPL(ch, form);
                std::string type = "alt";
                if(getFormType(ch, form) == 1)
                    type = "perm";
                else if(getFormType(ch, form) == 2)
                    type = "tech";

                send_to_char(ch, "@W(%s) %s@n @R-@G %s Growth Req\r\n", type.c_str(), name,
                       (ik >= (req * 0.75)) ? add_commas(req) : "??????????");
            }
        }

        if(!ch->permForms.empty()) {
            send_to_char(ch, "@b-------------------Perm Forms-------------------@n\r\n");
            for(auto form : ch->permForms) {
                auto name = getName(ch, form);
                if(getMasteryTier(ch, form) > 3)
                    name = "@RLIMITBREAK@n " + name;
                else if(getMasteryTier(ch, form) > 2)
                    name = "@RLIMIT@n " + name;
                else if (getMasteryTier(ch, form) > 1)
                    name = "@BMASTERED@n " + name;
                send_to_char(ch, "@W%s@n\r\n", name);
            }
        }

        send_to_char(ch, "@b------------------------------------------------@n\r\n");
        if (!form_names.empty()) {
            auto names = boost::join(form_names, ", ");
            send_to_char(ch, "Available Forms: @W%s@n\r\n", names.c_str());
        }

        const double epsilon = 0.05;  // A small tolerance value for floating point comparison

        if (ch->transBonus <= -0.3 + epsilon) {
            send_to_char(ch, "\r\n@WYou have @wGREAT@W transformation BPL Requirements.@n\r\n");
        } else if (ch->transBonus <= -0.2 + epsilon) {
            send_to_char(ch, "\r\n@MYou have @mabove average@M transformation BPL Requirements.@n\r\n");
        } else if (ch->transBonus <= -0.1 + epsilon) {
            send_to_char(ch, "\r\n@BYou have @bslightly above average@B transformation BPL Requirements.@n\r\n");
        } else if (ch->transBonus < 0.1 - epsilon) {
            send_to_char(ch, "\r\n@GYou have @gaverage@G transformation BPL Requirements.@n\r\n");
        } else if (ch->transBonus < 0.2 - epsilon) {
            send_to_char(ch, "\r\n@YYou have @yslightly below average@Y transformation BPL Requirements.@n\r\n");
        } else if (ch->transBonus < 0.3 - epsilon) {
            send_to_char(ch, "\r\n@CYou have @cbelow average@C transformation BPL Requirements.@n\r\n");
        } else {
            send_to_char(ch, "\r\n@RYou have @rterrible@R transformation BPL Requirements.@n\r\n");
        }
        send_to_char(ch, "@b------------------------------------------------@n\r\n");
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << ik;
        std::string growthString = ss.str();
        send_to_char(ch, "\r\n@BGrowth: %s@n\r\n", growthString);
    }


    static const std::unordered_map<FormID, std::pair<int64_t, int>> trans_pl = {
        // Human
        {FormID::super_human_1, {40, 0}},
        {FormID::super_human_2, {80, 0}},
        {FormID::super_human_3, {90, 0}},
        {FormID::super_human_4, {140, 0}},

        // Saiyan/Halfbreed.
        {FormID::super_saiyan_1, {30, 0}},
        {FormID::super_saiyan_2, {60, 0}},
        {FormID::super_saiyan_3, {90, 0}},
        {FormID::super_saiyan_4, {180, 0}},

        // Lssj
        {FormID::ikari, {120, 0}},
        {FormID::legendary_saiyan, {220, 0}},

        // Namek Forms
        {FormID::super_namekian_1, {50, 0}},
        {FormID::super_namekian_2, {50, 0}},
        {FormID::super_namekian_3, {70, 0}},
        {FormID::super_namekian_4, {130, 0}},

        // Icer Forms
        {FormID::icer_1, {25, 0}},
        {FormID::icer_2, {50, 0}},
        {FormID::icer_3, {120, 0}},
        {FormID::icer_4, {160, 0}},

        // Majin Forms
        {FormID::maj_affinity, {40, 1}},
        {FormID::maj_super, {80, 1}},
        {FormID::maj_true, {120, 1}},

        // Tuffle Forms
        {FormID::ascend_1, {50, 1}},
        {FormID::ascend_2, {70, 1}},
        {FormID::ascend_3, {120, 1}},

        // Mutant Forms
        {FormID::mutate_1, {30, 0}},
        {FormID::mutate_2, {75, 0}},
        {FormID::mutate_3, {140, 0}},


        // Kai Forms
        {FormID::mystic_1, {50, 0}},
        {FormID::mystic_2, {80, 0}},
        {FormID::mystic_3, {150, 0}},

        {FormID::divine_halo, {250, 0}},

        // Konatsu Forms

        {FormID::shadow_first, {35, 0}},
        {FormID::shadow_second, {75, 0}},
        {FormID::shadow_third, {125, 0}},

        // Android Forms
        {FormID::android_1, {10, 1}},
        {FormID::android_2, {20, 1}},
        {FormID::android_3, {30, 1}},
        {FormID::android_4, {40, 1}},
        {FormID::android_5, {50, 1}},
        {FormID::android_6, {60, 1}},

        // Bio Forms
        {FormID::bio_mature, {30, 1}},
        {FormID::bio_semi_perfect, {60, 1}},
        {FormID::bio_perfect, {80, 1}},
        {FormID::bio_super_perfect, {120, 1}},

        // Demon Forms
        {FormID::dark_king, {180, 0}},

        // Lycan Forms
        {FormID::lycanthrope, {60, 0}},
        {FormID::alpha_lycanthrope, {120, 0}},

        // Unbound Alternate Forms
        {FormID::potential_unleashed, {120, 0}},
        {FormID::ultra_instinct, {200, 0}},

        // Unbound Perm Forms
        {FormID::potential_unlocked, {40, 1}},
        {FormID::potential_unlocked_max, {90, 1}},
        {FormID::majinized, {0, 1}},
        {FormID::divine_water, {25, 1}},

        // Techniques
        {FormID::kaioken, {0, 2}},
        {FormID::dark_metamorphosis, {0, 2}},

        {FormID::tiger_stance, {0, 2}},
        {FormID::eagle_stance, {0, 2}},
        {FormID::ox_stance, {0, 2}},

        {FormID::evil_aura, {90, 2}},
        {FormID::spirit_absorption, {0, 2}},

    };

    int64_t getRequiredPL(struct char_data* ch, FormID trans) {
        if (auto req = trans_pl.find(trans); req != trans_pl.end()) {
            return req->second.first;
        }
        return 0;
    }

    int getFormType(struct char_data* ch, FormID trans) {
        if (auto req = trans_pl.find(trans); req != trans_pl.end()) {
            return req->second.second;
        }
        return 0;
    }

    std::optional<FormID> findFormFor(struct char_data* ch, const std::string& arg) {
        for (auto form: getFormsFor(ch)) {
            if (boost::iequals(arg, getAbbr(ch, form))) return form;
        }

        return {};
    }

    bool unlock(struct char_data *ch, FormID form) {
        initTransforms(ch);
        auto &data = ch->transforms[form];

        if(data.unlocked == false) {
            if(ch->internalGrowth >= getRequiredPL(ch, form))
                ch->internalGrowth -= getRequiredPL(ch, form);
            else 
                return false;
            data.unlocked = true;
            removeExclusives(ch, form);
            if(form == FormID::lycanthrope || form == FormID::alpha_lycanthrope)
                data.visible = false;
        }
        return data.unlocked;
    }

    void gamesys_oozaru(uint64_t heartPulse, double deltaTime) {

    };

    static std::unordered_map<FormID, std::function<void(struct char_data *ch)>> trans_on_transform = {
        {FormID::spirit_absorption, [](struct char_data *ch) {
            auto loc = get_obj_in_room(ch->getRoom(), "Spirit Bomb");
            if (loc == nullptr)
                revert(ch);
            else {
                act("@WYou launch yourself into the large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W! It eclipses everything in your sight as you desperately try to hold onto the coursing energy!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@W$n jumps directly into the @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W as it descends! It completely obscures $n from view before starting to be absorbed by them!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                hurt(0, 0, loc->user, ch, nullptr, loc->kicharge * 1.25, 1);
                ch->transforms[FormID::spirit_absorption].vars[0] = loc->kicharge * 0.9 + (0.1 * getMasteryTier(ch, FormID::spirit_absorption));
                ch->transforms[FormID::spirit_absorption].vars[1] = loc->kicharge * 0.9 + (0.1 * getMasteryTier(ch, FormID::spirit_absorption));
                send_to_char(ch, "@W[@cSpirit Force: @C%s@W]@n\r\n", add_commas((int64_t)ch->transforms[FormID::spirit_absorption].vars[0]).c_str());
                extract_obj(loc);
                act("@WThe @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W completely vanishes as it's absorbed, imploding inwards!@n",
                    true, nullptr, nullptr, nullptr, TO_ROOM);
            }

            return loc == nullptr ? false : true;
            }},
        {FormID::lycanthrope, [](struct char_data *ch) {
            if(!ch->transforms.contains(FormID::lesser_lycanthrope)) 
                ch->addTransform(FormID::lesser_lycanthrope);
            
            if(!ch->transforms[FormID::lesser_lycanthrope].unlocked)
                ch->transforms[FormID::lesser_lycanthrope].unlocked = true;

            }},
    };

    static std::unordered_map<FormID, std::function<void(struct char_data *ch, atk::Attack& outgoing)>> trans_on_attack = {
        {FormID::spirit_absorption, [](struct char_data *ch, atk::Attack& outgoing) {
            send_to_char(ch, "Spirit Force infuses your %s, cloaking it in an @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n hue.\r\n", outgoing.getName());
            act("@W$n's @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n aura infuses the attack with cataclysmic energy.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
            
            double empower = ch->transforms[FormID::spirit_absorption].vars[1] / 50;
            empower *= ch->transforms[FormID::spirit_absorption].grade;
            if(outgoing.isKiAttack())
                empower *= 4;
            if(empower > ch->transforms[FormID::spirit_absorption].vars[0])
                empower = ch->transforms[FormID::spirit_absorption].vars[0];
            ch->transforms[FormID::spirit_absorption].vars[0] -= empower;
            outgoing.calcDamage += empower * 2;

            if(ch->transforms[FormID::spirit_absorption].vars[0] <= 0)
                revert(ch);
            send_to_char(ch, "@W[@cSpirit Force: @C%s@W]@n\r\n", add_commas((int64_t)ch->transforms[FormID::spirit_absorption].vars[0]).c_str());
            }},
    };

    static std::unordered_map<FormID, std::function<void(struct char_data *ch, atk::Attack& incoming)>> trans_on_attacked = {
        {FormID::spirit_absorption, [](struct char_data *ch, atk::Attack& incoming) {
            send_to_char(ch, "Your @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n aura burns away at your opponent's %s.\r\n", incoming.getName());
            act("@W$n's @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n aura burns away some of the incoming attack.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
            
            double disempower = ch->transforms[FormID::spirit_absorption].vars[1] / 50;
            disempower *= ch->transforms[FormID::spirit_absorption].grade;
            if(disempower > ch->transforms[FormID::spirit_absorption].vars[0])
                disempower = ch->transforms[FormID::spirit_absorption].vars[0];
            ch->transforms[FormID::spirit_absorption].vars[0] -= disempower;
            incoming.calcDamage -= disempower * 2;

            if(ch->transforms[FormID::spirit_absorption].vars[0] <= 0)
                revert(ch);
            send_to_char(ch, "@W[@cSpirit Force: @C%s@W]@n\r\n", add_commas((int64_t)ch->transforms[FormID::spirit_absorption].vars[0]).c_str());
            }},
    };

    void onAttack(char_data *ch, atk::Attack& outgoing, FormID form) {
        if(trans_on_attack.contains(form)) {
            auto on_trans_attk = trans_on_attack.at(form);
            on_trans_attk(ch, outgoing);
        }
    }

    void onAttacked(char_data *ch, atk::Attack& incoming, FormID form) {
        if(trans_on_attacked.contains(form)) {
            auto on_trans_attkd = trans_on_attacked.at(form);
            on_trans_attkd(ch, incoming);
        }
    }

    void onTransform(char_data *ch, FormID form) {
        ch->removeLimitBreak();
        reveal_hiding(ch, 0);
        handleEchoTransform(ch, form);
        for (double var : ch->transforms[form].vars)
                var = 0.0;
        
        if(trans_on_transform.contains(form)) {
            auto on_trans = trans_on_transform.at(form);
            on_trans(ch);
        }
    }

    void onRevert(char_data *ch, FormID form) {
        handleEchoRevert(ch, ch->form);
        ch->removeLimitBreak();
        for (double var : ch->transforms[form].vars)
                var = 0.0;
    }

    void revert(char_data *ch) {
        int64_t beforeKi = ch->getMaxKI();
        if(ch->form != FormID::base) {
            onRevert(ch, ch->form);
            ch->form = FormID::base;
            if(ch->technique == FormID::base) characterSubscriptions.unsubscribe("transforms", ch);
        }
        if(ch->technique != FormID::base) {
            onRevert(ch, ch->technique);
            ch->technique = FormID::base;
            if(ch->form == FormID::base) characterSubscriptions.unsubscribe("transforms", ch);
        }
        int64_t afterKi = ch->getMaxKI();

        if (beforeKi > afterKi && GET_BARRIER(ch) > 0) {
            int64_t barrier = GET_BARRIER(ch);
            float ratio = (float) afterKi / (float) beforeKi;
            GET_BARRIER(ch) = barrier * ratio;

            send_to_char(ch, "Your barrier shimmers as it loses some energy with your transformation.\r\n");
        }
    }

    void transform(char_data *ch, FormID form, int grade) {
        int64_t beforeKi = ch->getMaxKI();
        if (grade > getMaxGrade(ch, form)) {
            grade = getMaxGrade(ch, form);
            send_to_char(ch, "The max grade of this form is %s!\r\nSetting to max.\r\n", std::to_string(grade));
        }
        if (grade < 1)
            grade = 1;

        int formtype = getFormType(ch, form);

        if (grade > getMaxGrade(ch, form)) {
            grade = getMaxGrade(ch, form);
            send_to_char(ch, "The max grade of this form is %s!\r\nSetting to max.\r\n", std::to_string(grade));
        }
        if (grade < 1)
        grade = 1;
        if(formtype == 0)
            ch->form = form;
        else if (formtype == 1)
            ch->permForms.insert(form);
        else if (formtype == 2)
            ch->technique = form;
        ch->transforms[form].grade = grade;
        
        characterSubscriptions.subscribe("transforms", ch);
        onTransform(ch, form);

        int64_t afterKi = ch->getMaxKI();

        if (beforeKi > afterKi && GET_BARRIER(ch) > 0) {
            int64_t barrier = GET_BARRIER(ch);
            float ratio = (float) afterKi / (float) beforeKi;
            GET_BARRIER(ch) = barrier * ratio;

            send_to_char(ch, "Your barrier shimmers as it loses some energy with your transformation.\r\n");
        }

        // Announce noisy transformations in the zone.
        int zone = 0;
        if (race::isSenseable(ch->race)) {
            if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
                send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
            };
        }
        


        char buf3[MAX_INPUT_LENGTH];
        send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(ch->getPL()).c_str());
        send_to_scouter(buf3, ch, 1, 0);
    }

    

}


trans_data::~trans_data() {
    if(description) free(description);
}
