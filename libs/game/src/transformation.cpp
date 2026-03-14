#include <iomanip>
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/transformation.hpp"
#include "dbat/game/comm.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/weather.hpp"
#include "dbat/game/dg_comm.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/combat.hpp"
#include "dbat/game/attack.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/util/FilterWeak.hpp"

#include "dbat/game/const/Environment.hpp"

constexpr int MASTERY_THRESHOLD = 5000;
constexpr int LIMIT_THRESHOLD = 100000;
constexpr int LIMITBREAK_THRESHOLD = 5000000;

namespace trans
{
    static std::string getCustomName(Character *ch, Form form)
    {
        return "Unknown";
    }

    std::string getName(Character *ch, Form form)
    {
        switch (form)
        {
        case Form::base:
            return "Base";
        case Form::custom_1:
        case Form::custom_2:
        case Form::custom_3:
        case Form::custom_4:
        case Form::custom_5:
        case Form::custom_6:
        case Form::custom_7:
        case Form::custom_8:
        case Form::custom_9:
            return getCustomName(ch, form);

        // Saiyan'y forms.
        case Form::oozaru:
            return "Oozaru";
        case Form::golden_oozaru:
            return "@YGolden Oozaru@n";
        case Form::super_saiyan_1:
            return "@YSuper @CSaiyan @WFirst@n";
        case Form::super_saiyan_2:
            return "@YSuper @CSaiyan @WSecond@n";
        case Form::super_saiyan_3:
            return "@YSuper @CSaiyan @WThird@n";
        case Form::super_saiyan_4:
            return "@YSuper @CSaiyan @RFourth@n";
        case Form::super_saiyan_god:
            return "@YSuper @CSaiyan @RGod@n";
        case Form::super_saiyan_blue:
            return "@YSuper @CSaiyan @BBlue@n";

        // Human'y forms.
        case Form::super_human_1:
            return "@YSuper @CHuman @WFirst@n";
        case Form::super_human_2:
            return "@YSuper @CHuman @WSecond@n";
        case Form::super_human_3:
            return "@YSuper @CHuman @WThird@n";
        case Form::super_human_4:
            return "@YSuper @CHuman @WFourth@n";

        // Icer'y forms.
        case Form::icer_1:
            return "@YTransform @WFirst@n";
        case Form::icer_2:
            return "@YTransform @WSecond@n";
        case Form::icer_3:
            return "@YTransform @WThird@n";
        case Form::icer_4:
            return "@YTransform @WFourth@n";
        case Form::icer_metal:
            return "@YTransform @WMetal@n";
        case Form::icer_golden:
            return "@YTransform @YGolden@n";
        case Form::icer_black:
            return "@YTransform @XBlack@n";

        // Konatsu'y forms.
        case Form::shadow_first:
            return "@YShadow @WFirst@n";
        case Form::shadow_second:
            return "@YShadow @WSecond@n";
        case Form::shadow_third:
            return "@YShadow @WThird@n";

        // Namekian'y forms.
        case Form::super_namekian_1:
            return "@YSuper @CNamekian @WFirst@n";
        case Form::super_namekian_2:
            return "@YSuper @CNamekian @WSecond@n";
        case Form::super_namekian_3:
            return "@YSuper @CNamekian @WThird@n";
        case Form::super_namekian_4:
            return "@YSuper @CNamekian @WFourth@n";

        // Mutant Forms.
        case Form::mutate_1:
            return "@YMutate @WFirst@n";
        case Form::mutate_2:
            return "@YMutate @WSecond@n";
        case Form::mutate_3:
            return "@YMutate @WThird@n";

        // BioAndroid
        case Form::bio_mature:
            return "@YMature@n";
        case Form::bio_semi_perfect:
            return "@YSemi-Perfect@n";
        case Form::bio_perfect:
            return "@YPerfect@n";
        case Form::bio_super_perfect:
            return "@YSuper Perfect@n";

        // Android
        case Form::android_1:
            return "@YVersion 1.0@n";
        case Form::android_2:
            return "@YVersion 2.0@n";
        case Form::android_3:
            return "@YVersion 3.0@n";
        case Form::android_4:
            return "@YVersion 4.0@n";
        case Form::android_5:
            return "@YVersion 5.0@n";
        case Form::android_6:
            return "@YVersion 6.0@n";

        // Majin
        case Form::maj_affinity:
            return "@YAffinity@n";
        case Form::maj_super:
            return "@YSuper@n";
        case Form::maj_true:
            return "@YTrue@n";

        // Kai
        case Form::mystic_1:
            return "@YMystic @WFirst@n";
        case Form::mystic_2:
            return "@YMystic @WSecond@n";
        case Form::mystic_3:
            return "@YMystic @WThird@n";

        case Form::divine_halo:
            return "@YDivine @WHalo@n";

        // Tuffle
        case Form::ascend_1:
            return "@YAscend @WFirst@n";
        case Form::ascend_2:
            return "@YAscend @WSecond@n";
        case Form::ascend_3:
            return "@YAscend @WThird@n";

        // Demon
        case Form::dark_king:
            if (ch->getBaseStat<int64_t>("health") >= 600000000)
                return "@bDark @rKing@n";
            else if (ch->getBaseStat<int64_t>("health") >= 50000000)
                return "@bDark @yLord@n";
            else if (ch->getBaseStat<int64_t>("health") >= 2000000)
                return "@bDark @yCourtier@n";
            else
                return "@bDark @ySeed@n";

        // LSSJ
        case Form::ikari:
            return "@GIkari@n";
        case Form::legendary_saiyan:
            return "@GLegendary @YSuper Saiyan@n";

        // Lycanthrope
        case Form::lesser_lycanthrope:
            return "@YLesser @WLycan@n";
        case Form::lycanthrope:
            return "@WLycanthrope@n";
        case Form::alpha_lycanthrope:
            return "@YAlpha @WLycanthrope@n";

        // Unbound Alternate Forms
        case Form::potential_unleashed:
            return "@YPotential @WUnleashed@n";
        case Form::evil_aura:
            return "@YEvil @WAura@n";
        case Form::ultra_instinct:
            return "@BUltra @RInstinct@n";

        // Unbound Perm Forms
        case Form::potential_unlocked:
            return "@YPotential @WUnlocked@n";
        case Form::potential_unlocked_max:
            return "@YMax @WPotential@n";
        case Form::majinized:
            return "@YMajinized@n";
        case Form::divine_water:
            return "@YDivine @WWater@n";

        // Techniques
        case Form::kaioken:
            return "Kaioken";
        case Form::dark_metamorphosis:
            return "Dark Meta";

        case Form::tiger_stance:
            return "Tiger Stance";
        case Form::eagle_stance:
            return "Eagle Stance";
        case Form::ox_stance:
            return "Ox Stance";

        case Form::spirit_absorption:
            return "@YSpirit Absorption@n";

        // Whoops?
        default:
            return "Unknown";
        }
    }

    std::string getExtra(Character *ch, Form form)
    {
        double energy = 0;
        std::string tag = "";
        switch (form)
        {
        case Form::oozaru:
            return "@w...$e is in the form of a @rgreat ape@w!";
        case Form::golden_oozaru:
            return "@w...$e is in the form of a @rgolden great ape@w!";
        case Form::super_saiyan_1:
            return "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!";
        case Form::super_saiyan_2:
            return "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!";
        case Form::super_saiyan_3:
            return "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!";
        case Form::super_saiyan_4:
            return "@w...$e has a covering of @rdark red fur@n around $s body!";

        case Form::kaioken:
            return "@w...@r$e has a red aura around $s body!";
        case Form::dark_metamorphosis:
            return "@w...$e has a dark, @rred@w aura and menacing presence.";

        case Form::tiger_stance:
            return "@w...$e has an aggressive demeanour, ready for a fight.";
        case Form::eagle_stance:
            return "@w...$e has a calm appearance, eyes constantly alert.";
        case Form::ox_stance:
            return "@w...$e is hunkered down, yet $s presence appears larger.";

        case Form::spirit_absorption:
            energy = ch->transforms[Form::spirit_absorption].vars["energy"];
            if (energy > 1000000000)
            {
                tag = "@Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n";
            }
            else if (energy > 100000000)
            {
                tag = "@Ceclipsing@n";
            }
            else if (energy > 10000000)
            {
                tag = "@crackling@n";
            }
            else if (energy > 1000000)
            {
                tag = "bright";
            }
            else if (energy > 100000)
            {
                tag = "fading";
            }
            else
            {
                tag = "faint";
            }
            return "@w...$e has a " + tag + " aura wrapped around $s body!";

        default:
            return "@w...$e has energy crackling around $s body!";
        }
    }

    int getMaxGrade(Character *ch, Form form)
    {
        int maxGrade = 1;
        switch (form)
        {
        case Form::kaioken:
            maxGrade = GET_SKILL(ch, (int)Skill::kaioken) / 5;
            if (maxGrade > 20)
                maxGrade = 20;
            if (ch->form != Form::base)
                maxGrade /= 4;
            if (maxGrade < 1)
                maxGrade = 1;

            return maxGrade;
        case Form::dark_metamorphosis:
            return GET_SKILL(ch, (int)Skill::dark_metamorphosis) >= 100 ? 2 : 1;

        case Form::tiger_stance:
            return std::max(GET_SKILL(ch, (int)Skill::tiger_stance) / 20, 1);
        case Form::eagle_stance:
            return std::max(GET_SKILL(ch, (int)Skill::eagle_stance) / 20, 1);
        case Form::ox_stance:
            return std::max(GET_SKILL(ch, (int)Skill::ox_stance) / 20, 1);

        case Form::spirit_absorption:
            return 3;

        default:
            return 1;
        }
    }

    static std::string getCustomAbbr(Character *ch, Form form)
    {
        return "N/A";
    }

    int getMasteryTier(Character *ch, Form form)
    {
        if (ch->transforms.contains(form))
        {
            double timeSpent = ch->transforms[form].time_spent_in_form;
            int mastery = 0;
            if (ch->transforms[form].limit_broken && timeSpent > LIMITBREAK_THRESHOLD)
                mastery = 4;
            else if (timeSpent > LIMIT_THRESHOLD)
                mastery = 3;
            else if (timeSpent > MASTERY_THRESHOLD)
                mastery = 2;
            else
                mastery = 1;

            if (IS_AFFECTED(ch, AFF_LIMIT_BREAKING) && ch->transforms[form].limit_broken)
                mastery += 2;

            return mastery;
        }
        else
        {
            return 0;
        }
    }

    std::string getAbbr(Character *ch, Form form)
    {
        switch (form)
        {
        case Form::base:
            return "Base";
        case Form::custom_1:
        case Form::custom_2:
        case Form::custom_3:
        case Form::custom_4:
        case Form::custom_5:
        case Form::custom_6:
        case Form::custom_7:
        case Form::custom_8:
        case Form::custom_9:
            return getCustomAbbr(ch, form);

        // Saiyan'y forms.
        case Form::oozaru:
            return "ooz";
        case Form::golden_oozaru:
            return "gooz";
        case Form::super_saiyan_1:
            return "ssj";
        case Form::super_saiyan_2:
            return "ssj2";
        case Form::super_saiyan_3:
            return "ssj3";
        case Form::super_saiyan_4:
            return "ssj4";
        case Form::super_saiyan_god:
            return "god";
        case Form::super_saiyan_blue:
            return "blue";

        // Human'y forms.
        case Form::super_human_1:
            return "first";
        case Form::super_human_2:
            return "second";
        case Form::super_human_3:
            return "third";
        case Form::super_human_4:
            return "fourth";

        // Icer'y forms.
        case Form::icer_1:
            return "first";
        case Form::icer_2:
            return "second";
        case Form::icer_3:
            return "third";
        case Form::icer_4:
            return "fourth";
        case Form::icer_metal:
            return "metal";
        case Form::icer_golden:
            return "golden";
        case Form::icer_black:
            return "black";

        // Konatsu'y forms.
        case Form::shadow_first:
            return "first";
        case Form::shadow_second:
            return "second";
        case Form::shadow_third:
            return "third";

        // Namekian'y forms.
        case Form::super_namekian_1:
            return "first";
        case Form::super_namekian_2:
            return "second";
        case Form::super_namekian_3:
            return "third";
        case Form::super_namekian_4:
            return "fourth";

        // Mutant Forms.
        case Form::mutate_1:
            return "first";
        case Form::mutate_2:
            return "second";
        case Form::mutate_3:
            return "third";

        // BioAndroid
        case Form::bio_mature:
            return "mature";
        case Form::bio_semi_perfect:
            return "semi-perfect";
        case Form::bio_perfect:
            return "perfect";
        case Form::bio_super_perfect:
            return "super-perfect";

        // Android
        case Form::android_1:
            return "1.0";
        case Form::android_2:
            return "2.0";
        case Form::android_3:
            return "3.0";
        case Form::android_4:
            return "4.0";
        case Form::android_5:
            return "5.0";
        case Form::android_6:
            return "6.0";

        // Majin
        case Form::maj_affinity:
            return "affinity";
        case Form::maj_super:
            return "super";
        case Form::maj_true:
            return "true";

        // Kai
        case Form::mystic_1:
            return "first";
        case Form::mystic_2:
            return "second";
        case Form::mystic_3:
            return "third";

        case Form::divine_halo:
            return "halo";

        // Tuffle
        case Form::ascend_1:
            return "first";
        case Form::ascend_2:
            return "second";
        case Form::ascend_3:
            return "third";

        // Demon
        case Form::dark_king:
            return "dark";

        // LSSJ
        case Form::ikari:
            return "ikari";
        case Form::legendary_saiyan:
            return "lssj";

        // Lycanthrope
        case Form::lesser_lycanthrope:
            return "llycan";
        case Form::lycanthrope:
            return "lycan";
        case Form::alpha_lycanthrope:
            return "alycan";

        // Unbound Alternate Forms
        case Form::potential_unleashed:
            return "potential";
        case Form::evil_aura:
            return "evil";
        case Form::ultra_instinct:
            return "ui";

        case Form::potential_unlocked:
            return "punlocked";
        case Form::potential_unlocked_max:
            return "pmax";
        case Form::majinized:
            return "majinized";
        case Form::divine_water:
            return "divine_water";

        // Techniques
        case Form::kaioken:
            return "kaioken";
        case Form::dark_metamorphosis:
            return "dm";

        case Form::tiger_stance:
            return "tiger";
        case Form::eagle_stance:
            return "eagle";
        case Form::ox_stance:
            return "ox";

        case Form::spirit_absorption:
            return "spiritab";

        // Whoops?
        default:
            return "Unknown";
        }
    }

    static std::unordered_map<Form, std::vector<Form>> transform_exclusive = {
        {Form::super_saiyan_2, {Form::legendary_saiyan, Form::ikari}},
        {Form::ikari, {Form::super_saiyan_2}},
        {Form::legendary_saiyan, {Form::super_saiyan_2, Form::golden_oozaru}},

        {Form::divine_halo, {Form::mystic_3}},
        {Form::mystic_3, {Form::divine_halo}},

        {Form::dark_metamorphosis, {Form::kaioken}},
        {Form::kaioken, {Form::dark_metamorphosis}},
    };

    static std::unordered_map<Form, std::function<bool(Character *ch)>> trans_requirements = {
        {Form::spirit_absorption, [](Character *ch)
         {
             auto loc = ch->location.searchObjects([](Object *obj)
                                                   { return isname("Spirit Bomb", obj->getName()); });
             return loc == nullptr ? false : true;
         }},
        {Form::super_saiyan_4, [](Character *ch)
         {
             return ch->hasTail();
         }},
    };

    static std::unordered_map<Form, std::function<bool(Character *ch)>> trans_unlocks = {
        // Saiyan
        {Form::super_saiyan_1, [](Character *ch)
         {
             return ((ch->race == Race::saiyan || ch->race == Race::halfbreed));
         }},
        {Form::super_saiyan_2, [](Character *ch)
         {
             return ((ch->race == Race::saiyan || ch->race == Race::halfbreed) && !ch->transforms.contains(Form::ikari) && !ch->transforms.contains(Form::legendary_saiyan) && getMasteryTier(ch, Form::super_saiyan_1) >= 2);
         }},
        {Form::super_saiyan_3, [](Character *ch)
         {
             return ((ch->race == Race::saiyan || ch->race == Race::halfbreed) && getMasteryTier(ch, Form::super_saiyan_2) >= 2);
         }},
        {Form::super_saiyan_4, [](Character *ch)
         {
             return ((ch->race == Race::saiyan || ch->race == Race::halfbreed) && ch->hasTail() && getMasteryTier(ch, Form::golden_oozaru) >= 3 && getMasteryTier(ch, Form::super_saiyan_3) >= 3);
         }},

        // Legendary Saiyan
        {Form::ikari, [](Character *ch)
         {
             return (ch->race == Race::saiyan && !ch->transforms.contains(Form::super_saiyan_2) && getMasteryTier(ch, Form::oozaru) >= 3);
         }},
        {Form::legendary_saiyan, [](Character *ch)
         {
             return (ch->race == Race::saiyan && !ch->transforms.contains(Form::super_saiyan_2) && ch->transforms.contains(Form::super_saiyan_1) && getMasteryTier(ch, Form::super_saiyan_1) >= 4 && getMasteryTier(ch, Form::ikari) >= 3);
         }},

        // Human
        {Form::super_human_1, [](Character *ch)
         {
             return (ch->race == Race::human);
         }},
        {Form::super_human_2, [](Character *ch)
         {
             return (ch->race == Race::human && getMasteryTier(ch, Form::super_human_1) >= 2);
         }},
        {Form::super_human_3, [](Character *ch)
         {
             return (ch->race == Race::human && getMasteryTier(ch, Form::super_human_2) >= 2);
         }},
        {Form::super_human_4, [](Character *ch)
         {
             return (ch->race == Race::human && getMasteryTier(ch, Form::super_human_3) >= 3);
         }},

        // Icer
        {Form::icer_1, [](Character *ch)
         {
             return (ch->race == Race::icer);
         }},
        {Form::icer_2, [](Character *ch)
         {
             return (ch->race == Race::icer && getMasteryTier(ch, Form::icer_1) >= 2);
         }},
        {Form::icer_3, [](Character *ch)
         {
             return (ch->race == Race::icer && getMasteryTier(ch, Form::icer_2) >= 2);
         }},
        {Form::icer_4, [](Character *ch)
         {
             return (ch->race == Race::icer && getMasteryTier(ch, Form::icer_3) >= 3);
         }},

        // Namekian
        {Form::super_namekian_1, [](Character *ch)
         {
             return (ch->race == Race::namekian);
         }},
        {Form::super_namekian_2, [](Character *ch)
         {
             return (ch->race == Race::namekian && getMasteryTier(ch, Form::super_namekian_1) >= 2);
         }},
        {Form::super_namekian_3, [](Character *ch)
         {
             return (ch->race == Race::namekian && getMasteryTier(ch, Form::super_namekian_2) >= 2);
         }},
        {Form::super_namekian_4, [](Character *ch)
         {
             return (ch->race == Race::namekian && getMasteryTier(ch, Form::super_namekian_3) >= 3);
         }},

        // Konatsu
        {Form::shadow_first, [](Character *ch)
         {
             return (ch->race == Race::konatsu);
         }},
        {Form::shadow_second, [](Character *ch)
         {
             return (ch->race == Race::konatsu && getMasteryTier(ch, Form::shadow_first) >= 2);
         }},
        {Form::shadow_third, [](Character *ch)
         {
             return (ch->race == Race::konatsu && getMasteryTier(ch, Form::shadow_second) >= 3);
         }},

        // Mutant
        {Form::mutate_1, [](Character *ch)
         {
             return (ch->race == Race::mutant);
         }},
        {Form::mutate_2, [](Character *ch)
         {
             return (ch->race == Race::mutant && getMasteryTier(ch, Form::mutate_1) >= 2);
         }},
        {Form::mutate_3, [](Character *ch)
         {
             return (ch->race == Race::mutant && getMasteryTier(ch, Form::mutate_2) >= 3);
         }},

        // BioAndroid
        {Form::bio_mature, [](Character *ch)
         {
             return (ch->race == Race::bio_android);
         }},
        {Form::bio_semi_perfect, [](Character *ch)
         {
             return (ch->race == Race::bio_android && getMasteryTier(ch, Form::bio_mature) >= 2);
         }},
        {Form::bio_perfect, [](Character *ch)
         {
             return (ch->race == Race::bio_android && getMasteryTier(ch, Form::bio_semi_perfect) >= 2);
         }},
        {Form::bio_super_perfect, [](Character *ch)
         {
             return (ch->race == Race::bio_android && getMasteryTier(ch, Form::bio_perfect) >= 3);
         }},

        // Android
        {Form::android_1, [](Character *ch)
         {
             return (ch->race == Race::android);
         }},
        {Form::android_2, [](Character *ch)
         {
             return (ch->race == Race::android && getMasteryTier(ch, Form::android_1) >= 2);
         }},
        {Form::android_3, [](Character *ch)
         {
             return (ch->race == Race::android && getMasteryTier(ch, Form::android_2) >= 2);
         }},
        {Form::android_4, [](Character *ch)
         {
             return (ch->race == Race::android && getMasteryTier(ch, Form::android_3) >= 2);
         }},
        {Form::android_5, [](Character *ch)
         {
             return (ch->race == Race::android && getMasteryTier(ch, Form::android_4) >= 2);
         }},
        {Form::android_6, [](Character *ch)
         {
             return (ch->race == Race::android && getMasteryTier(ch, Form::android_5) >= 3);
         }},

        // Majin
        {Form::maj_affinity, [](Character *ch)
         {
             return (ch->race == Race::majin);
         }},
        {Form::maj_super, [](Character *ch)
         {
             return (ch->race == Race::majin && getMasteryTier(ch, Form::maj_affinity) >= 2);
         }},
        {Form::maj_true, [](Character *ch)
         {
             return (ch->race == Race::majin && getMasteryTier(ch, Form::maj_super) >= 3);
         }},

        // Kai
        {Form::mystic_1, [](Character *ch)
         {
             return (ch->race == Race::kai);
         }},
        {Form::mystic_2, [](Character *ch)
         {
             return (ch->race == Race::kai && getMasteryTier(ch, Form::mystic_1) >= 2);
         }},
        {Form::mystic_3, [](Character *ch)
         {
             return (ch->race == Race::kai && getMasteryTier(ch, Form::mystic_2) >= 3);
         }},

        {Form::divine_halo, [](Character *ch)
         {
             return (ch->race == Race::kai && getMasteryTier(ch, Form::mystic_2) >= 3);
         }},

        // Tuffle
        {Form::ascend_1, [](Character *ch)
         {
             return (ch->race == Race::tuffle);
         }},
        {Form::ascend_2, [](Character *ch)
         {
             return (ch->race == Race::tuffle && getMasteryTier(ch, Form::ascend_1) >= 2);
         }},
        {Form::ascend_3, [](Character *ch)
         {
             return (ch->race == Race::tuffle && getMasteryTier(ch, Form::ascend_2) >= 3);
         }},

        // Demon
        {Form::dark_king, [](Character *ch)
         {
             return (ch->race == Race::demon);
         }},

        // Lycanthrope
        {Form::lycanthrope, [](Character *ch)
         {
             return (ch->race == Race::konatsu && ch->location.getEnvironment(ENV_MOONLIGHT));
         }},
        {Form::alpha_lycanthrope, [](Character *ch)
         {
             return (ch->location.getEnvironment(ENV_MOONLIGHT) && getMasteryTier(ch, Form::lycanthrope) >= 4);
         }},

        // Techniques
        {Form::kaioken, [](Character *ch)
         {
             return GET_SKILL(ch, (int)Skill::kaioken) > 0 && !ch->transforms.contains(Form::dark_metamorphosis);
         }},

        {Form::dark_metamorphosis, [](Character *ch)
         {
             return GET_SKILL(ch, (int)Skill::dark_metamorphosis) > 0 && !ch->transforms.contains(Form::kaioken);
         }},

        {Form::tiger_stance, [](Character *ch)
         {
             return GET_SKILL(ch, (int)Skill::tiger_stance) > 0;
         }},

        {Form::eagle_stance, [](Character *ch)
         {
             return GET_SKILL(ch, (int)Skill::eagle_stance) > 0;
         }},

        {Form::ox_stance, [](Character *ch)
         {
             return GET_SKILL(ch, (int)Skill::ox_stance) > 0;
         }},

        {Form::spirit_absorption, [](Character *ch)
         {
             auto loc = ch->location.searchObjects([](Object *obj)
                                                   { return isname("Spirit Bomb", obj->getName()); });
             return loc == nullptr ? false : true;
         }},

    };

    using trans_affect_type = character_affect_type;

    static double getModifierHelper(Character *ch, Form form, int location, int specific);

    static std::unordered_map<Form, std::vector<trans_affect_type>> trans_affects = {

        // Saiyan'y forms...
        {
            Form::oozaru, {
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::block), [](Character *ch)
                               { return 0.3 + (0.1 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](Character *ch)
                               { return -0.7 * 1.0 + (0.05 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](Character *ch)
                               { return 9.0 + (0.5 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](Character *ch)
                               { return 49.0 + (4.00 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::defense), [](Character *ch)
                               { return -0.1 - (0.05 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                               { return 0.2 + (0.05 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                               { return 0.9 + (0.05 * getMasteryTier(ch, Form::oozaru)); }},
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 9000 * 1.0 + (0.05 * getMasteryTier(ch, Form::oozaru)); }},
                          }},
        {Form::golden_oozaru, {
                                  {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                   {
                                       if (ch->transforms.contains(Form::super_saiyan_4))
                                           return getModifierHelper(ch,
                                                                    Form::super_saiyan_4,
                                                                    APPLY_CVIT_BASE,
                                                                    ~0);
                                       if (ch->transforms.contains(Form::super_saiyan_3))
                                           return getModifierHelper(ch,
                                                                    Form::super_saiyan_3,
                                                                    APPLY_CVIT_BASE,
                                                                    ~0);
                                       if (ch->transforms.contains(Form::super_saiyan_2))
                                           return getModifierHelper(ch,
                                                                    Form::super_saiyan_2,
                                                                    APPLY_CVIT_BASE,
                                                                    ~0);
                                       return getModifierHelper(ch, Form::super_saiyan_1, APPLY_CVIT_BASE, ~0);
                                   }},
                                  {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                   {
                                       if (ch->transforms.contains(Form::super_saiyan_4))
                                           return getModifierHelper(ch,
                                                                    Form::super_saiyan_4,
                                                                    APPLY_CVIT_MULT,
                                                                    ~0);
                                       if (ch->transforms.contains(Form::super_saiyan_3))
                                           return getModifierHelper(ch,
                                                                    Form::super_saiyan_3,
                                                                    APPLY_CVIT_MULT,
                                                                    ~0);
                                       if (ch->transforms.contains(Form::super_saiyan_2))
                                           return getModifierHelper(ch,
                                                                    Form::super_saiyan_2,
                                                                    APPLY_CVIT_MULT,
                                                                    ~0);
                                       return getModifierHelper(ch, Form::super_saiyan_1, APPLY_CVIT_MULT, ~0);
                                   }},
                                  {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::block), [](Character *ch)
                                   { return 0.3 + (0.1 * getMasteryTier(ch, Form::golden_oozaru)); }},
                                  {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](Character *ch)
                                   { return -0.7 + (0.05 * getMasteryTier(ch, Form::golden_oozaru)); }},
                                  {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](Character *ch)
                                   { return 9.0 + (0.5 * getMasteryTier(ch, Form::golden_oozaru)); }},
                                  {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](Character *ch)
                                   { return 49.0 + (4.00 * getMasteryTier(ch, Form::golden_oozaru)); }},
                                  {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::defense), [](Character *ch)
                                   { return -0.1 - (0.05 * getMasteryTier(ch, Form::golden_oozaru)); }},
                                  {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                                   { return 0.2 + (0.05 * getMasteryTier(ch, Form::golden_oozaru)); }},
                              }},

        {Form::super_saiyan_1, {
                                   {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                    { return 0.9 + (0.1 * getMasteryTier(ch, Form::super_saiyan_1)); }},
                                   {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                    { return 700000 * 1.0 + (0.1 * getMasteryTier(ch, Form::super_saiyan_1)); }},
                               }},

        {Form::super_saiyan_2, {
                                   {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                    { return 1.5 + (0.15 * getMasteryTier(ch, Form::super_saiyan_2)); }},
                                   {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                    { return 15000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::super_saiyan_2)); }},
                               }},

        {Form::super_saiyan_3, {
                                   {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                    { return 2.4 + (0.25 * getMasteryTier(ch, Form::super_saiyan_3)); }},
                                   {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                    { return 70000000 * 1.0 + (0.25 * getMasteryTier(ch, Form::super_saiyan_3)); }},
                               }},

        {Form::super_saiyan_4, {
                                   {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                    { return 3.8 + (0.35 * getMasteryTier(ch, Form::super_saiyan_4)); }},
                                   {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                    { return 17000000 * 1.0 + (0.35 * getMasteryTier(ch, Form::super_saiyan_4)); }},
                               }},

        // Human'y forms.
        {Form::super_human_1, {
                                  {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                   { return 0.8 + (0.1 * getMasteryTier(ch, Form::super_human_1)); }},
                                  {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                   { return 950000 * 1.0 + (0.1 * getMasteryTier(ch, Form::super_human_1)); }},
                              }},

        {Form::super_human_2, {
                                  {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                   { return 1.6 + (0.2 * getMasteryTier(ch, Form::super_human_2)); }},
                                  {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                   { return 10000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::super_human_2)); }},
                              }},

        {Form::super_human_3, {
                                  {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                   { return 2.5 + (0.3 * getMasteryTier(ch, Form::super_human_3)); }},
                                  {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                   { return 40000000 * 1.0 + (0.3 * getMasteryTier(ch, Form::super_human_3)); }},
                              }},

        {Form::super_human_4, {
                                  {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                   { return 3.1 + (0.4 * getMasteryTier(ch, Form::super_human_4)); }},
                                  {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                   { return 200000000 * 1.0 + (0.4 * getMasteryTier(ch, Form::super_human_4)); }},
                              }},

        // Icer'y Forms.
        {Form::icer_1, {
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](Character *ch)
                            { return 2.0 + (0.2 * getMasteryTier(ch, Form::icer_1)); }},
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](Character *ch)
                            { return 3.0 + (0.3 * getMasteryTier(ch, Form::icer_1)); }},
                           {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                            { return 1.0 + (0.05 * getMasteryTier(ch, Form::icer_1)); }},
                           {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                            { return 400000 * 1.0 + (0.05 * getMasteryTier(ch, Form::icer_1)); }},

                       }},
        {Form::icer_2, {
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](Character *ch)
                            { return 2.0 + (0.2 * getMasteryTier(ch, Form::icer_2)); }},
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](Character *ch)
                            { return 3.0 + (0.3 * getMasteryTier(ch, Form::icer_2)); }},
                           {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                            { return 2.0 + (0.05 * getMasteryTier(ch, Form::icer_2)); }},
                           {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                            { return 7000000 * 1.0 + (0.05 * getMasteryTier(ch, Form::icer_2)); }},
                       }},

        {Form::icer_3, {
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](Character *ch)
                            { return 0.5 + (0.25 * getMasteryTier(ch, Form::icer_3)); }},
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](Character *ch)
                            { return 1.0 + (0.1 * getMasteryTier(ch, Form::icer_3)); }},
                           {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                            { return 3.0 + (0.075 * getMasteryTier(ch, Form::icer_3)); }},
                           {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                            { return 45000000 * 1.0 + (0.075 * getMasteryTier(ch, Form::icer_3)); }},
                       }},

        {Form::icer_4, {
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::height), [](Character *ch)
                            { return 1.0 + (0.1 * getMasteryTier(ch, Form::icer_4)); }},
                           {APPLY_CDIM_MULT, 0.0, static_cast<int>(CharDim::weight), [](Character *ch)
                            { return 2.0 + (0.2 * getMasteryTier(ch, Form::icer_4)); }},
                           {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                            { return 4.0 + (0.1 * getMasteryTier(ch, Form::icer_4)); }},
                           {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                            { return 200000000 * 1.0 + (0.1 * getMasteryTier(ch, Form::icer_4)); }},
                       }},

        // Namekian Forms.
        {Form::super_namekian_1, {
                                     {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                      { return 1.0 + (0.05 * getMasteryTier(ch, Form::super_namekian_1)); }},
                                     {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                      { return 190000 * 1.0 + (0.05 * getMasteryTier(ch, Form::super_namekian_1)); }},
                                 }},
        {Form::super_namekian_2, {
                                     {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                      { return 1.9 + (0.1 * getMasteryTier(ch, Form::super_namekian_2)); }},
                                     {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                      { return 3500000 * 1.0 + (0.1 * getMasteryTier(ch, Form::super_namekian_2)); }},
                                 }},
        {Form::super_namekian_3, {
                                     {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                      { return 2.95 + (0.15 * getMasteryTier(ch, Form::super_namekian_3)); }},
                                     {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                      { return 60000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::super_namekian_3)); }},
                                 }},
        {Form::super_namekian_4, {
                                     {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                      { return 3.3 + (0.2 * getMasteryTier(ch, Form::super_namekian_4)); }},
                                     {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                      { return 210000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::super_namekian_4)); }},
                                 }},

        // Konatsu Forms.
        {Form::shadow_first, {
                                 {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                  { return 1.0 + (0.05 * getMasteryTier(ch, Form::shadow_first)); }},
                                 {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                  { return 70000 * 1.0 + (0.05 * getMasteryTier(ch, Form::shadow_first)); }},
                             }},
        {Form::shadow_second, {
                                  {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                   { return 1.8 + (0.15 * getMasteryTier(ch, Form::shadow_second)); }},
                                  {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                   { return 7000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::shadow_second)); }},
                              }},
        {Form::shadow_third, {
                                 {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                  { return 2.5 + (0.3 * getMasteryTier(ch, Form::shadow_third)); }},
                                 {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                  { return 60000000 * 1.0 + (0.3 * getMasteryTier(ch, Form::shadow_third)); }},
                             }},

        // Mutant Forms.
        {Form::mutate_1, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 0.9 + (0.05 * getMasteryTier(ch, Form::mutate_1)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 90000 * 1.0 + (0.05 * getMasteryTier(ch, Form::mutate_1)); }},
                         }},
        {Form::mutate_2, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 1.7 + (0.15 * getMasteryTier(ch, Form::mutate_2)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 8000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::mutate_2)); }},
                         }},
        {Form::mutate_3, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 2.4 + (0.3 * getMasteryTier(ch, Form::mutate_3)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 70000000 * 1.0 + (0.3 * getMasteryTier(ch, Form::mutate_3)); }},
                         }},
        // BioAndroid forms.
        {Form::bio_mature, {
                               {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                { return 0.6 + (0.05 * getMasteryTier(ch, Form::bio_mature)); }},
                               {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                { return 900000 * 1.0 + (0.1 * getMasteryTier(ch, Form::bio_mature)); }},
                           }},
        {Form::bio_semi_perfect, {
                                     {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                      { return 0.6 + (0.05 * getMasteryTier(ch, Form::bio_semi_perfect)); }},
                                     {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                      { return 7500000 * 1.0 + (0.15 * getMasteryTier(ch, Form::bio_semi_perfect)); }},
                                 }},
        {Form::bio_perfect, {
                                {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                 { return 0.6 + (0.05 * getMasteryTier(ch, Form::bio_super_perfect)); }},
                                {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                 { return 60000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::bio_perfect)); }},
                            }},
        {Form::bio_super_perfect, {
                                      {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                       { return 2.6 + (0.2 * getMasteryTier(ch, Form::bio_super_perfect)); }},
                                      {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                       { return 350000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::bio_super_perfect)); }},
                                  }},

        // Android Forms
        {Form::android_1, {
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 11000000 * 1.0 + (0.05 * getMasteryTier(ch, Form::android_1)); }},
                              {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                               { return 0.001 + (0.05 * getMasteryTier(ch, Form::android_1)); }},
                              {APPLY_CATTR_BASE, 2, ~0},
                          }},
        {Form::android_2, {
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 45000000 * 1.0 + (0.1 * getMasteryTier(ch, Form::android_2)); }},
                              {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                               { return 0.001 + (0.05 * getMasteryTier(ch, Form::android_2)); }},
                              {APPLY_CATTR_BASE, 2, ~0},
                          }},
        {Form::android_3, {
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 300000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::android_3)); }},
                              {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                               { return 0.001 + (0.05 * getMasteryTier(ch, Form::android_3)); }},
                              {APPLY_CATTR_BASE, 2, ~0},
                          }},
        {Form::android_4, {
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 2000000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::android_4)); }},
                              {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                               { return 0.001 + (0.05 * getMasteryTier(ch, Form::android_4)); }},
                              {APPLY_CATTR_BASE, 2, ~0},
                          }},
        {Form::android_5, {
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 4000000000 * 1.0 + (0.25 * getMasteryTier(ch, Form::android_5)); }},
                              {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                               { return 0.001 + (0.05 * getMasteryTier(ch, Form::android_5)); }},
                              {APPLY_CATTR_BASE, 2, ~0},
                          }},
        {Form::android_6, {
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 7000000000 * 1.0 + (0.3 * getMasteryTier(ch, Form::android_6)); }},
                              {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                               { return 0.001 + (0.05 * getMasteryTier(ch, Form::android_6)); }},
                              {APPLY_CATTR_BASE, 2, ~0},
                          }},

        // Majin Forms
        {Form::maj_affinity, {
                                 {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                  { return 0.8 + (0.05 * getMasteryTier(ch, Form::mystic_1)); }},
                                 {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                  { return 1150000 * 1.0 + (0.10 * getMasteryTier(ch, Form::mystic_1)); }},
                             }},
        {Form::maj_super, {
                              {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                               { return 0.8 + (0.05 * getMasteryTier(ch, Form::maj_super)); }},
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 12000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::maj_super)); }},
                          }},
        {Form::maj_true, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 0.8 + (0.05 * getMasteryTier(ch, Form::maj_true)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 300000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::maj_true)); }},
                         }},

        // Kai Forms
        {Form::mystic_1, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 1.8 + (0.10 * getMasteryTier(ch, Form::mystic_1)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 1000000 * 1.0 + (0.10 * getMasteryTier(ch, Form::mystic_1)); }},
                         }},
        {Form::mystic_2, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 2.7 + (0.15 * getMasteryTier(ch, Form::mystic_2)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 100000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::mystic_2)); }},
                         }},
        {Form::mystic_3, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 3.6 + (0.2 * getMasteryTier(ch, Form::mystic_3)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 220000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::mystic_3)); }},
                         }},

        {Form::divine_halo, {
                                {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                 { return 3.6 + (0.2 * getMasteryTier(ch, Form::divine_halo)); }},
                                {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                 { return 220000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::divine_halo)); }},
                                {APPLY_SKILL, 0.0, (int)Skill::divine_halo, [](Character *ch)
                                 { return 20 * getMasteryTier(ch, Form::divine_halo); }},
                            }},

        // Tuffle Forms
        {Form::ascend_1, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 0.9 + (0.1 * getMasteryTier(ch, Form::ascend_1)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 1200000 * 1.0 + (0.10 * getMasteryTier(ch, Form::ascend_1)); }},
                         }},
        {Form::ascend_2, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 0.8 + (0.075 * getMasteryTier(ch, Form::ascend_2)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 70000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::ascend_2)); }},
                         }},
        {Form::ascend_3, {
                             {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                              { return 0.7 + (0.05 * getMasteryTier(ch, Form::ascend_3)); }},
                             {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                              { return 250000000 * 1.0 + (0.2 * getMasteryTier(ch, Form::ascend_3)); }},
                         }},
        // Demon Form
        {Form::dark_king, {
                              {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                               {
                                   double base;
                                   auto bpl = ch->getBaseStat<int64_t>("health");
                                   if (bpl < 2000000)
                                       base = 1.0;
                                   else if (bpl < 50000000)
                                       base = 1.5;
                                   else if (bpl < 600000000)
                                       base = 2.0;
                                   else
                                       base = 3.0;

                                   return base + ((base / 10.0) * getMasteryTier(ch, Form::dark_king));
                               }},
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                               {
                                   double base;
                                   if (ch->getCurVital(CharVital::lifeforce) > 0)
                                   {
                                       auto bpl = ch->getBaseStat<int64_t>("health");

                                       if (bpl < 2000000)
                                       {
                                           base = 0.4;
                                       }
                                       else if (bpl < 50000000)
                                       {
                                           base = 0.6;
                                       }
                                       else if (bpl < 600000000)
                                       {
                                           base = 0.8;
                                       }
                                       else
                                       {
                                           base = 1.0;
                                       }

                                       ch->sendText("@MYour tainted lifeforce infuses your attack with a sickly purple hue!@n");

                                       base = base * ch->modCurVitalDam(CharVital::lifeforce, 0.05);
                                   }

                                   return base + ((base / 10) * getMasteryTier(ch, Form::dark_king));
                               }},
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::defense), [](Character *ch)
                               {
                                   double base = 0.0;
                                   if (ch->getCurVital(CharVital::lifeforce) > 0)
                                   {
                                       int chance = 30;
                                       auto bpl = ch->getBaseStat<int64_t>("health");

                                       if (bpl < 2000000)
                                       {
                                           chance = 30;
                                           base = 0.2;
                                       }
                                       else if (bpl < 85000000)
                                       {
                                           chance = 50;
                                           base = 0.3;
                                       }
                                       else if (bpl < 1225000000)
                                       {
                                           chance = 60;
                                           base = 0.5;
                                       }
                                       else
                                       {
                                           chance = 80;
                                           base = 0.6;
                                       }

                                       if (axion_dice(0) <= chance && GET_BARRIER(ch) <= 0)
                                       {
                                           ch->setBaseStat("barrier", ch->getCurVital(CharVital::lifeforce) / 5);
                                           ch->sendText("@MThe mantle of your tainted life flares out, creating a barrier.@n");
                                       }
                                       else
                                           ch->sendText("@MYour tainted lifeforce saps the strength of your opponents attack!@n");

                                       ch->modCurVitalDam(CharVital::lifeforce, 0.05);
                                   }
                                   else
                                   {
                                       ch->sendText("@MYou feel far too exhausted to strengthen yourself!@n");
                                   }

                                   return -base - ((base / 10) * getMasteryTier(ch, Form::dark_king));
                               }},

                          }},

        // LSSJ
        {Form::ikari, {
                          {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                           {
                               return 1.0 + (0.15 * getMasteryTier(ch, Form::ikari) + 0.15 * getMasteryTier(ch, Form::oozaru));
                           }},
                          {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                           {
                               return 700000 * 1.0 + (0.15 * getMasteryTier(ch, Form::ikari) + 0.15 * getMasteryTier(ch, Form::oozaru));
                           }},
                          {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](Character *ch)
                           {
                               return -0.3 - (0.04 * getMasteryTier(ch, Form::ikari) + 0.04 * getMasteryTier(ch, Form::oozaru));
                           }},
                          {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](Character *ch)
                           {
                               return -0.3 - (0.04 * getMasteryTier(ch, Form::ikari) + 0.04 * getMasteryTier(ch, Form::oozaru));
                           }},
                          {APPLY_CATTR_BASE, 0.0, ~0, [](Character *ch)
                           {
                               return 2 * 1.0 + (0.15 * getMasteryTier(ch, Form::ikari) + 0.15 * getMasteryTier(ch, Form::oozaru));
                           }},
                          {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                           {
                               return 0.1 * 1.0 + (0.15 * getMasteryTier(ch, Form::ikari) + 0.15 * getMasteryTier(ch, Form::oozaru));
                           }},
                          {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                           {
                               return 0.02 * 1.0 + (0.15 * getMasteryTier(ch, Form::ikari) + 0.15 * getMasteryTier(ch, Form::oozaru));
                           }},
                      }},
        {Form::legendary_saiyan, {
                                     {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                      {
                                          return 4.0 + (0.1 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                      {
                                          return 100000000 * 1.0 + (0.15 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                                      {
                                          return 1 * 1.0 + (0.15 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](Character *ch)
                                      {
                                          return -0.4 - (0.8 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::dodge), [](Character *ch)
                                      {
                                          return -0.4 - (0.8 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::block), [](Character *ch)
                                      {
                                          return 0.2 + (0.1 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_CATTR_BASE, 0.0, ~0, [](Character *ch)
                                      {
                                          return 5 * 1.0 + (0.15 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                                      {
                                          return 0.3 * 1.0 + (0.15 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                     {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                                      {
                                          return 0.1 * 1.0 + (0.15 * getMasteryTier(ch, Form::legendary_saiyan));
                                      }},
                                 }},

        // Lycan
        {Form::lesser_lycanthrope, {
                                       {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                        { return 0.5 + (0.1 * getMasteryTier(ch, Form::lesser_lycanthrope)); }},
                                       {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                        { return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, Form::lesser_lycanthrope)); }},
                                       {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](Character *ch)
                                        { return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, Form::lesser_lycanthrope)); }},
                                       {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                                        { return 0.1 * 1.0 + (0.10 * getMasteryTier(ch, Form::lesser_lycanthrope)); }},
                                       {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                        { return 0.3 * 1.0 + (0.10 * getMasteryTier(ch, Form::lesser_lycanthrope)); }},
                                       {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                        { return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, Form::lesser_lycanthrope)); }},
                                   }},
        {Form::lycanthrope, {
                                {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                 { return 1.0 + (0.1 * getMasteryTier(ch, Form::lycanthrope)); }},
                                {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                 { return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, Form::lycanthrope)); }},
                                {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](Character *ch)
                                 { return 0.3 * 1.0 + (0.10 * getMasteryTier(ch, Form::lycanthrope)); }},
                                {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                                 { return 0.2 * 1.0 + (0.10 * getMasteryTier(ch, Form::lycanthrope)); }},
                                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                 { return 0.5 * 1.0 + (0.10 * getMasteryTier(ch, Form::lycanthrope)); }},
                                {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                 { return 0.25 * 1.0 + (0.10 * getMasteryTier(ch, Form::lycanthrope)); }},
                            }},
        {Form::alpha_lycanthrope, {
                                      {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                       { return 1.4 + (0.1 * getMasteryTier(ch, Form::alpha_lycanthrope)); }},
                                      {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                       { return 0.7 * 1.0 + (0.10 * getMasteryTier(ch, Form::alpha_lycanthrope)); }},
                                      {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::parry), [](Character *ch)
                                       { return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, Form::alpha_lycanthrope)); }},
                                      {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                                       { return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, Form::alpha_lycanthrope)); }},
                                      {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                       { return 0.8 * 1.0 + (0.10 * getMasteryTier(ch, Form::alpha_lycanthrope)); }},
                                      {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::ki), [](Character *ch)
                                       { return 0.4 * 1.0 + (0.10 * getMasteryTier(ch, Form::alpha_lycanthrope)); }},
                                  }},

        // Unbound Forms
        {Form::potential_unleashed, {
                                        {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                         {
                                             auto cl = ch->getBaseStat("lifetimeGrowth");
                                             if (cl < 50)
                                                 return 1.0 + (0.1 * getMasteryTier(ch, Form::potential_unleashed));
                                             if (cl < 100)
                                                 return 2.0 + (0.1 * getMasteryTier(ch, Form::potential_unleashed));
                                             if (cl < 150)
                                                 return 3.0 + (0.15 * getMasteryTier(ch, Form::potential_unleashed));
                                             if (cl < 200)
                                                 return 4.0 + (0.15 * getMasteryTier(ch, Form::potential_unleashed));
                                             if (cl < 250)
                                                 return 4.5 + (0.2 * getMasteryTier(ch, Form::potential_unleashed));
                                             if (cl >= 250)
                                                 return 5.0 + (0.2 * getMasteryTier(ch, Form::potential_unleashed));
                                             return 1.0 + (0.1 * getMasteryTier(ch, Form::potential_unleashed));
                                         }},
                                    }},
        {Form::evil_aura, {
                              {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                               { return 2.0 + (0.2 * getMasteryTier(ch, Form::evil_aura)); }},
                              {APPLY_CVIT_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                               {
                                   double healthBoost = ch->getCurVitalDam(CharVital::health) * 5.0;
                                   return healthBoost + (0.2 * getMasteryTier(ch, Form::evil_aura));
                               }},
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 1300000 * 1.0 + (0.2 * getMasteryTier(ch, Form::evil_aura)); }},
                              {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                               {
                                   double healthBoost = 0.1;
                                   auto chealth = 1.0 - ch->getCurVitalDam(CharVital::health);
                                   if (chealth < 0.75)
                                   {
                                       healthBoost = (1 - chealth) * (4 + (0.2 * getMasteryTier(ch, Form::evil_aura)));

                                       if (chealth < 0.25)
                                           ch->sendText("@RYou feel your agony infuse the attack.\r\n@n");
                                       else if (chealth < 0.50)
                                           ch->sendText("@RYou feel your rage infuse the attack.\r\n@n");
                                       else
                                           ch->sendText("@RYou feel your anger infuse the attack.\r\n@n");
                                   }
                                   return healthBoost;
                               }},
                          }},
        {Form::ultra_instinct, {
                                   {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::accuracy), [](Character *ch)
                                    { return 1 + (0.1 * getMasteryTier(ch, Form::ultra_instinct)); }},
                                   {APPLY_CVIT_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                                    { return 2 + (0.4 * getMasteryTier(ch, Form::ultra_instinct)); }},
                                   {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                                    { return -0.7 + (0.2 * getMasteryTier(ch, Form::ultra_instinct)); }},
                                   {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::perfect_dodge), [](Character *ch)
                                    { return -0.4 - (0.05 * getMasteryTier(ch, Form::ultra_instinct)); }},
                                   {APPLY_SKILL, 0.0, (int)Skill::instinctual_combat, [](Character *ch)
                                    { return 20 * getMasteryTier(ch, Form::ultra_instinct); }},
                               }},

        {Form::potential_unlocked, {
                                       {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                        { return 0.4 + (0.05 * getMasteryTier(ch, Form::potential_unlocked)); }},
                                       {APPLY_CATTR_BASE, 2, ~0},
                                   }},
        {Form::potential_unlocked_max, {
                                           {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                            { return 0.4 + (0.05 * getMasteryTier(ch, Form::potential_unlocked_max)); }},
                                           {APPLY_CATTR_BASE, 4, ~0},
                                       }},
        {Form::majinized, {
                              {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                               { return 0.6 + (0.02 * getMasteryTier(ch, Form::majinized)); }},
                              {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                               { return 50000 * 1.0 + (0.1 * getMasteryTier(ch, Form::majinized)); }},
                          }},
        {Form::divine_water, {
                                 {APPLY_CVIT_MULT, 0.0, ~0, [](Character *ch)
                                  { return 0.2 + (0.05 * getMasteryTier(ch, Form::divine_water)); }},
                                 {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                  { return 15000 * 1.0 + (0.1 * getMasteryTier(ch, Form::divine_water)); }},
                                 {APPLY_CATTR_BASE, 2, ~0},
                             }},

        // Techniques
        {Form::kaioken, {
                            {APPLY_CVIT_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                             { return 0.1 * (ch->transforms[Form::kaioken].grade); }},
                            {APPLY_COMBAT_MULT, 0.0, static_cast<int>(ComStat::damage), [](Character *ch)
                             {
                    double mult = (1 + (0.05 * getMasteryTier(ch, Form::kaioken)));
                    if (axion_dice(0) <= (GET_SKILL(ch, (int)Skill::kaioken) * mult)) {
                                                ch->sendText("You push yourself to the limit as you attack!.\r\n");
                        act("$n's red aura ripples with power as they strike!", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)Skill::kaioken, 0);
                        return (0.06 * ch->transforms[Form::kaioken].grade) * mult;
                    }
                    return 0.03 * (ch->transforms[Form::kaioken].grade); }},
                            {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                             { return -0.05 * (ch->transforms[Form::kaioken].grade) + (0.01 * getMasteryTier(ch, Form::kaioken)); }},
                            {APPLY_CVIT_REGEN_MULT, 0.0, static_cast<int>(CharVital::ki), [](Character *ch)
                             { return -0.05 * (ch->transforms[Form::kaioken].grade) + (0.01 * getMasteryTier(ch, Form::kaioken)); }},
                        }},
        {Form::dark_metamorphosis, {
                                       {APPLY_CVIT_MULT, 0.0, static_cast<int>(CharVital::health), [](Character *ch)
                                        { return 0.6 * (ch->transforms[Form::dark_metamorphosis].grade) + (0.05 * getMasteryTier(ch, Form::dark_metamorphosis)); }},
                                       {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::ki), [](Character *ch)
                                        {
                    double mult = (1 + (0.05 * getMasteryTier(ch, Form::dark_metamorphosis)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)Skill::dark_metamorphosis) * mult)) {
                                                ch->sendText("Your darkness burns into your very ki!.\r\n");
                        act("$n's ki ripples with a horrifying darkness!", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)Skill::dark_metamorphosis, 0);
                        return (0.05 * ch->transforms[Form::dark_metamorphosis].grade) * mult;
                    }

                    return 0.0; }},
                                   }},

        {Form::tiger_stance, {
                                 {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                  {
                    double mult = (1 + (0.05 * getMasteryTier(ch, Form::tiger_stance)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)Skill::tiger_stance) * mult)) {
                                                ch->sendText("Primal strength courses through you.\r\n");
                        act("$n lurches forwards with ferocious might.", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)Skill::tiger_stance, 0);
                        return (0.05 * ch->transforms[Form::tiger_stance].grade) * mult;
                    }

                    return 0.0; }},
                             }},
        {Form::eagle_stance, {
                                 {APPLY_DTYPE_BON, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                                  {
                    double mult = (1 + (0.05 * getMasteryTier(ch, Form::eagle_stance)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)Skill::eagle_stance) * mult)) {
                                                ch->sendText("Your mind and ki align, power flaring.\r\n");
                        act("$n's movements slow, their ki redoubling in strength.", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)Skill::eagle_stance, 0);
                        return (0.05 * ch->transforms[Form::eagle_stance].grade) * mult;
                    }

                    return 0.0; }},
                             }},
        {Form::ox_stance, {
                              {APPLY_DTYPE_RES, 0.0, static_cast<int>(DamType::physical), [](Character *ch)
                               {
                    double mult = (1 + (0.05 * getMasteryTier(ch, Form::ox_stance)));

                    if (axion_dice(0) <= (GET_SKILL(ch, (int)Skill::ox_stance) * mult)) {
                                                ch->sendText("Hah, that didn't hurt half as much!\r\n");
                        act("$n's hulking form barely seems grazed by the attack!", true, ch, nullptr, nullptr, TO_ROOM);
                        improve_skill(ch, (int)Skill::ox_stance, 0);
                        return (0.05 * ch->transforms[Form::ox_stance].grade) * mult;
                    }

                    return 0.0; }},
                          }},
        {Form::spirit_absorption, {
                                      {APPLY_COMBAT_BASE, 0.0, static_cast<int>(ComStat::armor), [](Character *ch)
                                       { return sqrt(ch->transforms[Form::spirit_absorption].vars["energy"]) * 10; }},
                                      {APPLY_CVIT_BASE, 0.0, ~0, [](Character *ch)
                                       { return (ch->transforms[Form::spirit_absorption].vars["energy"] / 5) * 1 + (0.1 * ch->transforms[Form::spirit_absorption].grade); }},
                                  }},
        {
            Form::death_phase,
            {
                {APPLY_CVIT_REGEN_MULT, -0.5, ~0},
            },
        },
        {
            Form::birth_phase,
            {
                {APPLY_CVIT_REGEN_MULT, 1.0, ~0},
                {APPLY_CVIT_MULT, 1.0, ~0, [](auto ch)
                 {
                     return (IN_ROOM(ch) != NOWHERE && ETHER_STREAM(ch)) ? 0.5 : 0.0;
                 }},
                {APPLY_CATTR_BASE, 5.0, static_cast<int>(CharAttribute::speed)},
            },
        },
        {
            Form::life_phase,
            {
                {APPLY_CVIT_REGEN_MULT, 2.0, ~0},
                {APPLY_CVIT_MULT, 2.0, ~0, [](auto ch)
                 {
                     return (IN_ROOM(ch) != NOWHERE && ETHER_STREAM(ch)) ? 0.5 : 0.0;
                 }},
                {APPLY_CATTR_BASE, 8.0, static_cast<int>(CharAttribute::speed)},
            },
        }};

    static double getModifierHelper(Character *ch, Form form, int location, int specific)
    {
        if (form == Form::base)
            return 0.0;
        double out = 0.0;
        if (auto found = trans_affects.find(form); found != trans_affects.end())
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

    double getModifier(Character *ch, int location, int specific)
    {
        double modifier = 0;
        if (!ch->permForms.empty())
        {
            for (auto form : ch->permForms)
            {
                modifier += getModifierHelper(ch, form, location, specific);
            }
        }
        modifier += getModifierHelper(ch, ch->form, location, specific);
        modifier += getModifierHelper(ch, ch->technique, location, specific);

        if (IS_HOSHIJIN(ch))
        {
            switch (GET_PHASE(ch))
            {
            case 0: // Death phase
                modifier += getModifierHelper(ch, Form::death_phase, location, specific);
                break;
            case 1: // Birth Phase
                modifier += getModifierHelper(ch, Form::birth_phase, location, specific);
                break;
            case 2: // Life phase
                modifier += getModifierHelper(ch, Form::life_phase, location, specific);
                break;
            default:
                // oops?
                break;
            }
        }

        return modifier;
    }

    static std::unordered_map<Form, double> trans_drain = {
        // Saiyan forms.
        {Form::super_saiyan_1, .06},
        {Form::super_saiyan_2, .08},
        {Form::super_saiyan_3, .2},
        {Form::super_saiyan_4, .15},

        // Lssj
        {Form::ikari, .2},
        {Form::legendary_saiyan, .4},

        // Human Forms
        {Form::super_human_1, .06},
        {Form::super_human_2, .08},
        {Form::super_human_3, .12},
        {Form::super_human_4, .14},

        // icer Forms
        {Form::icer_1, .06},
        {Form::icer_2, .08},
        {Form::icer_3, .12},
        {Form::icer_4, .14},

        // Konatsu Forms
        {Form::shadow_first, .06},
        {Form::shadow_second, .08},
        {Form::shadow_third, .1},

        // Namekian Forms
        {Form::super_namekian_1, .06},
        {Form::super_namekian_2, .07},
        {Form::super_namekian_3, .08},
        {Form::super_namekian_4, .1},

        // Mutant Forms
        {Form::mutate_1, .05},
        {Form::mutate_2, .07},
        {Form::mutate_3, .1},

        // Bio, Android, Majin, have no drain.

        // Kai
        {Form::mystic_1, .05},
        {Form::mystic_2, .07},
        {Form::mystic_3, .1},

        {Form::divine_halo, .1},

        // Demon
        {Form::dark_king, .1},

        // Lycan
        {Form::lesser_lycanthrope, .1},

        // Unbound Alternate Forms
        {Form::potential_unleashed, .1},
        {Form::evil_aura, .08},
        {Form::ultra_instinct, .08},

        // Unbound Permanant Forms

        // Techniques
        {Form::kaioken, .05},
        {Form::dark_metamorphosis, .1},

        {Form::tiger_stance, .02},
        {Form::eagle_stance, .02},
        {Form::ox_stance, .02},

        {Form::spirit_absorption, .15},
    };

    double getStaminaDrain(Character *ch, Form form, bool upkeep)
    {
        if (ch->form == Form::base)
            return 0.0;

        double drain = 0.0;

        if (auto found = trans_drain.find(form); found != trans_drain.end())
        {
            drain = found->second;
        }

        // TODO: fix this. FPSSJ check.
        if (form == Form::super_saiyan_1 && false)
            drain *= 0.5;

        if (ch->location.getWhereFlag(WhereFlag::afterlife_hell) || ch->location.getWhereFlag(WhereFlag::afterlife))
            drain *= 0.75;

        if (ch->form != Form::base && ch->technique != Form::base)
            drain *= 2;

        if (upkeep)
        {
            drain *= 0.01;
            if (ch->race == Race::icer)
                drain = 0.0;
        }
        return drain * ch->transforms[form].grade * (1.0 + ch->getAffectModifier(APPLY_TRANS_UPKEEP_CVIT, static_cast<int>(CharVital::stamina)));
    }

    static const std::unordered_set<Form> moonForms = {
        Form::oozaru,
        Form::golden_oozaru,
        Form::lycanthrope,
        Form::alpha_lycanthrope,
    };

    void gamesys_transform(uint64_t heartPulse, double deltaTime)
    {
        // TODO: replace this with a subscription for anyone who's not in base form.
        auto subs = characterSubscriptions.all("transforms");
        for (auto ch : dbat::util::filter_raw(subs))
        {

            // check transform logic...
            auto form = ch->form;
            auto technique = ch->technique;
            auto &data = ch->transforms[form];
            double timeBefore = data.time_spent_in_form;
            data.time_spent_in_form += deltaTime;
            double timeAfter = data.time_spent_in_form;

            auto &techdata = ch->transforms[technique];
            double techTimeBefore = data.time_spent_in_form;
            techdata.time_spent_in_form += deltaTime;
            double techTimeAfter = data.time_spent_in_form;

            if (moonForms.contains(ch->form))
            {
                if (auto moonlight = ch->location.getEnvironment(ENV_MOONLIGHT); moonlight >= 100.0)
                {
                    // Top off the blutz.
                    data.vars["blutz"] = 60.0 * 30;
                }
                data.vars["blutz"] -= deltaTime;
                if (data.vars["blutz"] <= 0 || !ch->character_flags.get(CharacterFlag::tail))
                {
                    data.vars["blutz"] = 0.0;
                    oozaru_revert(ch);
                }
            }

            double kigain = getModifierHelper(ch, form, APPLY_CVIT_REGEN_MULT,
                                              static_cast<int>(CharVital::ki));
            double plgain = getModifierHelper(ch, form, APPLY_CVIT_REGEN_MULT,
                                              -static_cast<int>(CharVital::health));
            ch->modCurVitalDam(CharVital::health, -plgain);
            ch->modCurVitalDam(CharVital::ki, -kigain);

            // Notify at thresholds
            if (form != Form::base)
            {
                if (timeBefore < MASTERY_THRESHOLD && timeAfter >= MASTERY_THRESHOLD)
                    ch->sendText("@mSomething settles in your core, you feel more comfortable using @n" + getName(ch, form) + "\r\n");

                if (timeBefore < LIMIT_THRESHOLD && timeAfter >= LIMIT_THRESHOLD)
                    ch->sendText("@mYou feel power overwhelming emanate from your core, you instinctively know you've hit the limit of @n" + getName(ch, form) + "\r\n");

                if (timeBefore < LIMITBREAK_THRESHOLD && timeAfter >= LIMITBREAK_THRESHOLD && data.limit_broken == true)
                    ch->sendText("@mThere's a snap as a tide of power rushes throughout your veins,@n " + getName(ch, form) + " @mhas evolved.@n\r\n");
            }
            if (technique != Form::base)
            {
                if (techTimeBefore < MASTERY_THRESHOLD && techTimeAfter >= MASTERY_THRESHOLD)
                    ch->sendText("@mSomething settles in your core, you feel more comfortable using @n" + getName(ch, technique) + "\r\n");

                if (techTimeBefore < LIMIT_THRESHOLD && techTimeAfter >= LIMIT_THRESHOLD)
                    ch->sendText("@mYou feel power overwhelming emanate from your core, you instinctively know you've hit the limit of @n" + getName(ch, technique) + "\r\n");

                if (techTimeBefore < LIMITBREAK_THRESHOLD && techTimeAfter >= LIMITBREAK_THRESHOLD && techdata.limit_broken == true)
                    ch->sendText("@mThere's a snap as a tide of power rushes throughout your veins,@n " + getName(ch, technique) + " @mhas evolved.@n\r\n");
            }

            // Check stamina drain.
            if (auto drain = (getStaminaDrain(ch, ch->form, true) + getStaminaDrain(ch, ch->technique, true)) * deltaTime; drain > 0)
            {
                if (ch->modCurVitalDam(CharVital::stamina, drain) == 0)
                {
                    act("@mExhausted of stamina, your body forcibly reverts from its form.@n\r\n", true, ch, nullptr,
                        nullptr, TO_CHAR);
                    act("@C$n @wbreathing heavily, reverts from $s form, returning to normal.@n\r\n", true, ch, nullptr,
                        nullptr, TO_ROOM);
                    revert(ch);
                }
            }
        }
    }

    struct trans_echo
    {
        std::string self;
        std::string room;
    };

    static std::unordered_map<Form, trans_echo> trans_echoes = {
        {Form::super_saiyan_1, {"@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n", "@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n"}},

        {Form::super_saiyan_2, {"@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n", "@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n"}},

        {Form::super_saiyan_3, {"@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n", "@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n"}},

        {Form::super_saiyan_4, {"@WHaving absorbed enough blutz waves, your body begins to transform! Red fur grows over certain parts of your skin as your hair grows longer and unkempt. A red outline forms around your eyes while the irises of those very same eyes change to an amber color. Energy crackles about your body violently as you achieve the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n", "@WHaving absorbed enough blutz waves, @C$n@W's body begins to transform! Red fur grows over certain parts of $s skin as $s hair grows longer and unkempt. A red outline forms around $s eyes while the irises of those very same eyes change to an amber color. Energy crackles about $s body violently as $e achieves the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n"}},

        {Form::ikari, {"@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n", "@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n"}},

        {Form::legendary_saiyan, {"@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n", "@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n"}},

        // Human Forms
        {
            Form::super_human_1, {"@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!", "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!"}},

        {Form::super_human_2, {"@WSuddenly a bright white aura bursts into existance around your body, you feel the intensity of your hidden potential boil until it can't be contained any longer! Waves of ki shoot out from your aura streaking outwards in many directions. A roar that shakes everything in the surrounding area sounds right as your energy reaches its potential and you achieve @CSuper @cHuman @GSecond@W!", "@C$n@W is suddenly covered with a bright white aura as $e grits $s teeth, apparently struggling with the power boiling to the surface! Waves of ki shoot out from $s aura, streaking in several directions as a mighty roar shakes everything in the surrounding area. As $s aura calms $e smiles, having achieved @CSuper @cHuman @GSecond@W!"}},

        {Form::super_human_3, {"@WYou clench both of your fists as the bright white aura around your body is absorbed back into your flesh. As it is absorbed, your muscles triple in size and electricity crackles across your flesh. You grin as you feel the power of @CSuper @cHuman @GThird@W!", "@C$n@W clenches both of $s fists as the bright white aura around $s body is absorbed back into $s flesh. As it is absorbed, $s muscles triple in size and bright electricity crackles across $s flesh. $e smiles as $e achieves the power of @CSuper @cHuman @GThird@W!"}},

        {Form::super_human_4, {"@WYou grit your teeth and clench your fists as a sudden surge of power begins to tear through your body! Your muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over you, flashes of multicolored light flaring up in rising stars around your new form, a corona of glory! You feel your ultimate potential realized as you ascend to @CSuper @cHuman @GFourth@W!@n", "@C$n@W grits $s teeth and clenches $s fists as a sudden surge of power begins to tear through $s body! $n@W's muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over $m, flashes of multicolored light flare up in rising stars around $s new form, a corona of glory! $n@W smiles as his ultimate potential is realized as $e ascends to @CSuper @cHuman @GFourth@W!@n"}},

        // Icer Forms
        {
            Form::icer_1, {"@WYou yell with pain as your body begins to grow and power surges within! Your legs expand outward to triple their previous length. Soon after your arms, chest, and head follow. Your horns grow longer and curve upwards while lastly your tail expands. You are left confidently standing, having completed your @GFirst @cTransformation@W.@n", "@C$n@W yells with pain as $s body begins to grow and power surges outward! $s legs expand outward to triple their previous length. Soon after $s arms, chest, and head follow. $s horns grow longer and curve upwards while lastly $s tail expands. $e is left confidently standing, having completed $s @GFirst @cTransformation@W.@n"}},

        {Form::icer_2, {"@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n", "@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n"}},

        {Form::icer_3, {"@WA blinding light surrounds your body while your rising power begins to rip up the ground beneath you! Your skin and torso shell begin to crack as your new body struggles to free its self. Huge chunks of debris lift free of the ground as your power begins to rise to unbelievable heights. Suddenly your old skin and torso shell burst off from your body, leaving a sleek form glowing where they had been. Everything comes crashing down as your power evens out, leaving you with your @GThird @cTransformation @Wcompleted!@n", "@WA blinding light surrounds @C$n@W's body while $s rising power begins to rip up the ground beneath $m! $s skin and torso shell begin to crack as $s new body struggles to free its self. Huge chunks of debris lift free of the ground as $s power begins to rise to unbelievable heights. Suddenly $s old skin and torso shell burst off from $s body, leaving a sleek form glowing where they had been. Everything comes crashing down as @C$n@W's power evens out, leaving $m with $s @GThird @cTransformation @Wcompleted!@n"}},

        {Form::icer_4, {"@WA feeling of complete power courses through your viens as your body begins to change radically! You triple in height while a hard shell forms over your entire torso. Hard bones grow out from your head forming four ridges that jut outward. A hard covering grows up over your mouth and nose completing the transformation! A dark crimson aura flames around your body as you realize your @GFourth @cTransformation@W!@n", "@C$n@W's body begins to change radically! $e triples in height while a hard shell forms over $s entire torso. Hard bones grow out from $s head forming four ridges that jut outward. A hard covering grows up over $s mouth and nose completing the transformation! A dark crimson aura flames around @C$n@W's body as $e realizes $s @GFourth @cTransformation@W!@n"}},

        // Konatsu Forms
        {
            Form::shadow_first, {"@WA dark shadowy aura with flecks of white energy begins to burn around your body! Strength and agility can be felt rising up within as your form becomes blurred and ethereal looking. You smile as you realize your @GFirst @DShadow @BForm@W!@n", "@WA dark shadowy aura with flecks of white energy begins to burn around @C$n@W's body! $s form becomes blurred and ethereal-looking as $s muscles become strong and lithe. $e smiles as $e achieves $s @GFirst @DShadow @BForm@W!@n"}},

        {Form::shadow_second, {"@WThe shadowy aura surrounding your body burns larger than ever as dark bolts of purple electricity crackles across your skin. Your eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into your aura in an instant as you achieve your @GSecond @DShadow @BForm@W!@n", "@WThe shadowy aura surrounding @C$n@W's body burns larger than ever as dark bolts of purple electricity crackles across $s skin. $s eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into $s aura in an instant as $e achieves $s @GSecond @DShadow @BForm@W!@n"}},

        {Form::shadow_third, {"@WThe shadowy aura around you explodes outward as your power begins to rise!  You're overcome with a sudden realization, that the shadows are an extension of yourself, that light isn't needed for your shadows to bloom.  With this newfound wisdom comes ability and power!  The color in your aura drains as the shadows slide inward and cling to your body like a second, solid black skin!  Shockwaves roll off of you in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath you, slowly spreading the shadows to the whole area, projecting onto any surface nearby!  Purple and black electricity crackle in your solid white aura, and you grin as you realize your @GThird @DShadow @BForm@W!@n", "@WThe shadowy aura around $n explodes outward as $s power begins to rise!  Realization dawns on $s face, followed shortly by confidence! The color in $s aura drains as the shadows slide inward to cling to $s body like a second, solid black skin! Shockwaves roll off of $n in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath them, slowly spreading the shadows to the whole area, projecting onto any surface nearby! Purple and black electricity crackle in $s solid white aura, and he grins as $e realizes $s @GThird @DShadow @BForm@W!@n"}},

        // Namekian Forms
        {
            Form::super_namekian_1, {"@WYou crouch down and clench your fists as your muscles begin to bulge! Sweat pours down your body as the ground beneath your feet cracks and warps under the pressure of your rising ki! With a sudden burst that sends debris flying you realize a new plateau in your power, having achieved @CSuper @gNamek @GFirst@W!@n", "@C$n @Wcrouches down and clenches $s fists as $s muscles begin to bulge! Sweat pours down $s body as the ground beneath $s feet cracks and warps under the pressure of  $s rising ki! With a sudden burst that sends debris flying $e seems to realize a new plateau in $s power, having achieved @CSuper @gNamek @GFirst@W!@n"}},
        {Form::super_namekian_2, {"@WYou gasp in shock as a power within your body that you had not been aware of begins to surge to the surface! Your muscles grow larger as energy crackles between your antennae intensely! A shockwave of energy explodes outward as you achieve a new plateau in power, @CSuper @gNamek @GSecond@W!@n", "@C$n @Wgasps in shock as a power within $s body begins to surge out! $s muscles grow larger as energy crackles between $s antennae intensely! A shockwave of energy explodes outward as $e achieves a new plateau in power, @CSuper @gNamek @GSecond@W!@n"}},
        {Form::super_namekian_3, {"@WA fierce clear aura bursts up around your body as you struggle to control a growing power within! Energy leaks off of your aura at an astounding rate filling the air around you with small orbs of ki. As your power begins to level off the ambient ki hovering around you is absorbed inward in a sudden shock that leaves your skin glowing! You have achieved a rare power, @CSuper @gNamek @GThird@W!@n", "@WA fierce clear aura bursts up around @C$n@W's body as $e struggles to control $s own power! Energy leaks off of $s aura at an astounding rate filling the air around $m with small orbs of ki. As $s power begins to level off the ambient ki hovering around $m is absorbed inward in a sudden shock that leaves $s skin glowing! $e has achieved a rare power, @CSuper @gNamek @GThird@W!@n"}},
        {Form::super_namekian_4, {"@WAn inner calm fills your mind as your power surges higher than ever before. Complete clarity puts everything once questioned into perspective. While this inner calm is filling your mind, an outer storm of energy erupts around your body! The storm of energy boils and crackles while growing larger. You have achieved @CSuper @gNamek @GFourth@W, a mystery of the ages.@n", "@C$n@W smiles calmly as a look of complete understand fills $s eyes. While $e remains perfectly calm and detached a massivly powerful storm of energy erupts from his body. This storm of energy shimmers with the colors of the rainbow and boils and crackles with awesome power! $s smile disappears as he realizes a mysterious power of the ages, @CSuper @gNamek @GFourth@W!@n"}},

        {Form::mutate_1, {"@WYour flesh grows tougher as power surges up from within. Your fingernails grow longer, sharper, and more claw-like. Lastly your muscles double in size as you achieve your @GFirst @mMutation@W!@n", "@C$n@W flesh grows tougher as power surges up around $m. $s fingernails grow longer, sharper, and more claw-like. Lastly $s muscles double in size as $e achieves $s @GFirst @mMutation@W!@n"}},

        {Form::mutate_2, {"@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n", "@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n"}},

        {Form::mutate_3, {"@WA dark cyan aura bursts up around your body as the ground begins to crack beneath you! You scream out in pain as your power begins to explode! Two large spikes grow out from your shoulder blades as you reach your @GThird @mMutation!@n", "@WA dark cyan aura bursts up around @C$n@W's body as the ground begins to crack beneath $m and $e screams out in pain as $s power begins to explode! Two large spikes grow out from $s shoulder blades as $e reaches $s @GThird @mMutation!@n"}},

        {Form::bio_mature, {"@gYou bend over as @rpain@g wracks your body! Your limbs begin to grow out, becoming more defined and muscular. As your limbs finish growing outward you feel a painful sensation coming from your back as a long tail with a spike grows out of your back! As the pain subsides you stand up straight and a current of power shatters part of the ground beneath you. You have @rmatured@g beyond your @Gl@ga@Dr@gv@Ga@ge stage!@n", "@W$n @gbends over as a @rpainful@g look covers $s face! $s limbs begin to grow out, becoming more defined and muscular. As $s limbs finish growing outward $e screams as a long tail with a spike grows rips out of $s back! As $e calms $e stands up straight and a current of power shatters part of the ground beneath $m. $e has @rmatured@g beyond $s @Gl@ga@Dr@gv@Ga@ge stage!@n"}},

        {Form::bio_semi_perfect, {"@WYour exoskeleton begins to glow spectacularly while the shape of your body begins to change. Your tail shrinks slightly. Your hands, feet, and facial features become more refined. While your body colors change slightly. The crests on your head change, standing up straighter on either side of your head as well. As you finish transforming a wave of power floods your being. You have achieved your @gSemi@D-@GPerfect @BForm@W!@n", "@C$n@W's exoskeleton begins to glow spectacularly while the shape of $s body begins to change. $s tail shrinks slightly. $s hands, feet, and facial features become more refined. While $s body colors change slightly. The crests on $s head change, standing up straighter on either side of $s head as well. As $e finishes transforming a wave of power rushes out from $m. $e has achieved $s @gSemi@D-@GPerfect @BForm@W!@n"}},

        {Form::bio_perfect, {"@WYour whole body is engulfed in blinding light as your exoskeleton begins to change shape! Your hands, feet, and facial features become more refined and humanoid. While your colors change, becoming more subdued and neutral. A bright golden aura bursts up around your body as you achieve your @GPerfect @BForm@W!@n", "@C$n@W whole body is engulfed in blinding light as $s exoskeleton begins to change shape! $s hands, feet, and facial features become more refined and humanoid. While $s colors change, becoming more subdued and neutral. A bright golden aura bursts up around $s body as $e achieves $s @GPerfect @BForm@W!@n"}},

        {Form::bio_super_perfect, {"@WA rush of power explodes from your perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout your aura intensely while your muscles grow slightly larger but incredibly dense. You smile as you realize that you have taken your perfect form beyond imagination. You are now @CSuper @GPerfect@W!@n", "@WA rush of power explodes from @C$n@W's perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout $s aura intensely while $s muscles grow slightly larger but incredibly dense. $e smiles as $e has taken $s perfect form beyond imagination. $e is now @CSuper @GPerfect@W!@n"}},

        {Form::android_1, {"@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"}},

        {Form::android_2, {"@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"}},

        {Form::android_3, {"@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"}},

        {Form::android_4, {"@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"}},

        {Form::android_5, {"@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"}},

        {Form::android_6, {"@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", "@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"}},

        {Form::maj_affinity, {"@WA dark pink aura bursts up around your body as images of good and evil fill your mind! You feel the power within your body growing intensely, reflecting your personal alignment as your body changes!@n", "@WA dark pink aura bursts up around @C$n@W's body as images of good and evil fill $s mind! $e feels the power within $s body growing intensely, reflecting $s personal alignment as $s body changes!@n"}},

        {Form::maj_super, {"@WAn intense pink aura surrounds your body as it begins to change, taking on the characteristics of those you have ingested! Explosions of pink energy burst into existence all around you as your power soars to sights unseen!@n", "@WAn intense pink aura surrounds @C$n@W's body as it begins to change, taking on the characteristics of those $e has ingested! Explosions of pink energy burst into existence all around $m as $s power soars to sights unseen!@n"}},

        {Form::maj_true, {"@WRipples of intense pink energy rush upwards around your body as it begins to morph into its truest form! The ground beneath your feet forms into a crater from the very pressure of your rising ki! Earthquakes shudder throughout the area as your finish morphing!@n", "@WRipples of intense pink energy rush upwards around @C$n@W's body as it begins to morph into its truest form! The ground beneath $s feet forms into a crater from the very pressure of $s rising ki! Earthquakes shudder throughout the area as $e finishes morphing!@n"}},

        {Form::mystic_1, {"@WThoughts begin to flow through your mind of events throughout your life. The progression leads up to more recent events and finally to this very moment. All of it's significance overwhelms you momentarily and your motivation and drive increase. As your attention is drawn back to your surroundings, you feel as though your thinking, senses, and reflexes have sharpened dramatically.  At the core of your being, a greater depth of power can be felt.@n", "@W$n@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings. Though it's not apparent why they were so distracted, something definitely seems different about $m.@n"}},

        {Form::mystic_2, {"@WYou feel a sudden rush of emotion, escalating almost to a loss of control as your thoughts race. Your heart begins to beat fast as memories mix with the raw emotion. A faint blue glow begins to surround you. As your emotions level off, you feel a deeper understanding of the universe as you know it. You visibly calm back down to an almost steely eyed resolve as you assess your surroundings. The blue aura wicks around you for a few moments and then dissipates. Thought it's full impact is not yet clear to you, you are left feeling as though both your power and inner strength have turned into nearly bottomless wells.@n", "@W$n@W's appears to be hit by some sudden pangs of agony, their face contorted in pain.  After a moment a faint blue aura appears around them, glowing brighter as time passes. You can feel something in the pit of your stomach, letting you know that something very significant is changing around you. Before long $n@W's aura fades, leaving a very determined looking person in your presence.@n"}},

        {Form::mystic_3, {"@WYour minds' eye becomes overwhelmed by secrets unimaginable. The threads of the very universe become visible in your heightened state of awareness. Reaching out, a single thread vibrates, producing a @Rred @Wcolor -- yours. Your fingertips brush against it and your senses become clouded by a vast expanse of white color and noise. As your vision and hearing return, you understand the threads tying every living being together. Your awareness has expanded beyond comprehension!@n", "@C$n@W's eyes grow wide, mouth agape. $s body begins to shiver uncontrollably! $s arms reaches out cautiously before falling back down to $s side. $s face relaxes visibly, features returning to a normal state. $s irises remain larger than before, a slight smile softening $s gaze.@n"}},

        {Form::ascend_1, {"@WYour mind accelerates working through the mysteries of the universe while at the same time your body begins to change! Innate nano-technology within your body begins to activate, forming flexible metal plating across parts of your skin!@n", "@C$n@W begins to write complicated calculations in the air as though $e were possessed while at the same time $s body begins to change! Innate nano-technology within $s body begins to activate, forming flexible metal plating across parts of $s skin!@n"}},

        {Form::ascend_2, {"@WComplete understanding of every physical thing floods your mind as the nano-technology within you continues to change your body! Your eyes change; becoming glassy, hard, and glowing. Your muscles merge with a nano-fiber strengthening them at the molecular level! Finally your very bones become plated in nano-metals that have yet to be invented naturally!@n", "@C$n@.s nano-technology continues to change $s body! $s eyes change; becoming glassy, hard, and glowing. $s muscles merge with a nano-fiber strengthening them at the molecular level! Finally $s very bones become plated in nano-metals that have yet to be invented naturally!@n"}},

        {Form::ascend_3, {"@WYou have reached the final stage of enlightenment and the nano-technology thriving inside you begin to initiate the changes! Your neural pathways become refined, your reflexes honed, your auditory and ocular senses sharpening far beyond normal levels! Your gravitational awareness improves, increasing sensitivity and accuracy in your equilibrum!@n", "@C$n begins to mumble quietly, slowly at first and gradually picking up speed. A glint is seen from $s eyes and $s arms reach outwards briefly as $e appears to catch his balance. $s arms drop back to $s sides as balance is regained, a vicious smile on $s face.@n"}},

        {Form::oozaru, {"@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Hair sprouts  all over your body and your teeth become sharp as your body takes on the Oozaru form!@n", "@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant ape!@n"}},
        {Form::golden_oozaru, {"@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Golden hair sprouts all over your body, your teeth sharpen, as your body takes on the Golden Oozaru form!@n", "@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Golden hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant golden ape!@n"}},

        {Form::dark_king, {"@WYou kindle the dark seed within you, rooted through your body, and in a mere moment its roots pulse with power untold. Wrapping them within yourself, bound eternal, your body begins to shift as your combined strength becomes manifest.@n", "@C$n strains for a second as their eyes shine a fierce red, soon their body contorts, unleashing an ominous aura from within themself.@n"}},

        {Form::potential_unleashed, {"@WThere is a well of potential within you, beyond simple forms. Reaching deep you tug at it, pulling what was once simple potential and flaring it to life in a burst of simple power.@n", "@C$n takes a moment for themselves, focusing inwards. A moment later a white glow of pure aura explodes outwards, energy unbound.@n"}},

        {Form::evil_aura, {"@WMorality is just a crutch for the weak. You know this better than anyone, for discarding it gleans a look at true power. You indulge. And outwards ruptures your Ki, tainted and drenched in evil beyond parallel.@n", "@C$n gives a sickeningly twisted smile as something behind their eyes relents. Forward from them rushes forwards a surge of black ki, their evil manifest, with nothing left to hold it back.@n"}},
        {Form::ultra_instinct, {"@WThe tempo of the fight soon gives way to a single rhythm, every blow falling in perfect harmony, you can see it all as your emotions drain away into a perfect and sharp focus.@n", "@C$n starts to slow their movements as emotion fades away from them, each move growing precise, calculated with perfect accuracy.@n"}},

        // Techniques
        {
            Form::kaioken, {"@rA blazing red aura bursts up around your body, flashing intensely as the pressure of your aura magnifies!@n", "@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely as the pressure of $e aura magnifies!@n"}},

    };

    void handleEchoTransform(Character *ch, Form form)
    {
        if (auto found = trans_echoes.find(form); found != trans_echoes.end())
        {
            auto &echo = found->second;
            act(echo.self.c_str(), true, ch, nullptr, nullptr, TO_CHAR);
            act(echo.room.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
        }
        else
        {
            auto name = getName(ch, form);
            auto self = fmt::format("@wYou transform to {}.@n", name);
            auto room = fmt::format("@w$n@w transforms into {}.@n", name);

            act(self.c_str(), true, ch, nullptr, nullptr, TO_CHAR);
            act(room.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
        }
    }

    void handleEchoRevert(Character *ch, Form form)
    {
        if (form == Form::base)
            return;

        auto name = getName(ch, form);
        auto self = fmt::format("@wYou revert from {}.@n", name);
        auto room = fmt::format("@w$n@w reverts from {}.@n", name);

        act(self.c_str(), true, ch, nullptr, nullptr, TO_CHAR);
        act(room.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
    }

    static const std::vector<Form> forms = {
        // Universal
        Form::base,
        Form::custom_1,
        Form::custom_2,
        Form::custom_3,
        Form::custom_4,
        Form::custom_5,
        Form::custom_6,
        Form::custom_7,
        Form::custom_8,
        Form::custom_9,

        // Saiyan'y forms.
        Form::oozaru,
        Form::golden_oozaru,
        Form::super_saiyan_1,
        Form::super_saiyan_2,
        Form::super_saiyan_3,
        Form::super_saiyan_4,
        Form::super_saiyan_god,
        Form::super_saiyan_blue,

        // Lssj
        Form::ikari,
        Form::legendary_saiyan,

        // Human'y Forms
        Form::super_human_1,
        Form::super_human_2,
        Form::super_human_3,
        Form::super_human_4,

        // Icer'y Forms
        Form::icer_1,
        Form::icer_2,
        Form::icer_3,
        Form::icer_4,
        Form::icer_metal,
        Form::icer_golden,
        Form::icer_black,

        // Konatsu
        Form::shadow_first,
        Form::shadow_second,
        Form::shadow_third,

        // Namekian
        Form::super_namekian_1,
        Form::super_namekian_2,
        Form::super_namekian_3,
        Form::super_namekian_4,

        // Mutant
        Form::mutate_1,
        Form::mutate_2,
        Form::mutate_3,

        // BioAndroid
        Form::bio_mature,
        Form::bio_semi_perfect,
        Form::bio_perfect,
        Form::bio_super_perfect,

        // Android
        Form::android_1,
        Form::android_2,
        Form::android_3,
        Form::android_4,
        Form::android_5,
        Form::android_6,

        // Majin
        Form::maj_affinity,
        Form::maj_super,
        Form::maj_true,

        // Kai
        Form::mystic_1,
        Form::mystic_2,
        Form::mystic_3,

        Form::divine_halo,

        // Tuffle
        Form::ascend_1,
        Form::ascend_2,
        Form::ascend_3,

        // Demon
        Form::dark_king,

        // Lycanthrope
        Form::lesser_lycanthrope,
        Form::lycanthrope,
        Form::alpha_lycanthrope,

        // Unbound Alternate Forms
        Form::potential_unleashed,
        Form::evil_aura,
        Form::ultra_instinct,

        // Unbound Perm Forms
        Form::potential_unlocked,
        Form::potential_unlocked_max,
        Form::majinized,
        Form::divine_water,

        // Techniques
        Form::kaioken,
        Form::dark_metamorphosis,

        Form::tiger_stance,
        Form::eagle_stance,
        Form::ox_stance,

        Form::spirit_absorption,
    };

    std::optional<Form> findForm(Character *ch, const std::string &form)
    {
        for (auto formFound : forms)
        {
            if (form == getAbbr(ch, formFound))
            {
                return formFound;
                break;
            }
        }

        // Failure state
        return {};
    }

    void initTransforms(Character *ch)
    {
        auto transforms = ch->transforms;

        for (auto unlockForms : trans_unlocks)
        {
            if (!transforms.contains(unlockForms.first))
            {
                if (unlockForms.second(ch))
                    ch->addTransform(unlockForms.first);
            }
        }
    }

    bool requirementsMet(Character *ch, Form form)
    {
        if (trans_requirements.contains(form))
        {
            auto formReq = trans_requirements.find(form);
            return formReq->second(ch);
        }

        return true;
    }

    void removeExclusives(Character *ch, Form form)
    {
        if (!transform_exclusive.contains(form) || !ch->transforms[form].unlocked)
            return;

        auto formList = transform_exclusive.find(form);

        for (auto &checkForm : formList->second)
        {
            if (ch->transforms.contains(checkForm))
                ch->transforms.erase(checkForm);
        }
    }

    std::unordered_set<Form> getFormsFor(Character *ch)
    {
        initTransforms(ch);
        auto forms = ch->transforms;
        std::unordered_set<Form> pforms;

        for (auto form : forms)
        {
            if (form.first == Form::base || form.first == Form::oozaru || form.first == Form::golden_oozaru)
                form.second.visible = false;
            if (form.second.visible && requirementsMet(ch, form.first))
            {
                pforms.insert(form.first);
            }
        }
        return pforms;
    }

    void displayForms(Character *ch)
    {
        auto forms = getFormsFor(ch);
        if (forms.empty())
        {
            ch->sendText("You have no forms. Bummer.\r\n");
            return;
        }

        ch->sendText("              @YForms@n\r\n");

        auto ik = ch->getBaseStat("internalGrowth");
        std::vector<std::string> form_names;
        std::vector<Form> locked_forms;
        bool bannerDisplayed = false;
        for (auto form : forms)
        {
            bool unlocked = ch->transforms[form].unlocked;
            bool permActive = ch->permForms.contains(form);
            auto name = getName(ch, form);
            if (getMasteryTier(ch, form) > 3)
                name = "@RLIMITBREAK@n " + name;
            else if (getMasteryTier(ch, form) > 2)
                name = "@RLIMIT@n " + name;
            else if (getMasteryTier(ch, form) > 1)
                name = "@BMASTERED@n " + name;
            if (!permActive)
            {
                if (unlocked)
                {
                    if (!bannerDisplayed)
                    {
                        ch->sendText("@b-------------------Unlocked--------------------@n\r\n");
                        bannerDisplayed = true;
                    }
                    ch->send_to("@W%s@n\r\n", name);
                }
                else
                {
                    //                    ch->send_to("@W%s@n @R-@G %s Growth Req\r\n", name, //       (ik >= (req * 0.75) && !unlocked) ? add_commas(req) : "??????????");
                    locked_forms.push_back(form);
                }

                form_names.push_back(getAbbr(ch, form));
            }
        }

        if (!locked_forms.empty())
        {
            ch->sendText("@b--------------------Locked---------------------@n\r\n");
            for (auto form : locked_forms)
            {
                auto name = getName(ch, form);
                auto req = getRequiredPL(ch, form);
                std::string type = "alt";
                if (getFormType(ch, form) == 1)
                    type = "perm";
                else if (getFormType(ch, form) == 2)
                    type = "tech";

                ch->send_to("@W(%s) %s@n @R-@G %s Growth Req\r\n", type.c_str(), name, (ik >= (req * 0.75)) ? add_commas(req) : "??????????");
            }
        }

        if (!ch->permForms.empty())
        {
            ch->sendText("@b-------------------Perm Forms-------------------@n\r\n");
            for (auto form : ch->permForms)
            {
                auto name = getName(ch, form);
                if (getMasteryTier(ch, form) > 3)
                    name = "@RLIMITBREAK@n " + name;
                else if (getMasteryTier(ch, form) > 2)
                    name = "@RLIMIT@n " + name;
                else if (getMasteryTier(ch, form) > 1)
                    name = "@BMASTERED@n " + name;
                ch->send_to("@W%s@n\r\n", name);
            }
        }

        ch->sendText("@b------------------------------------------------@n\r\n");
        if (!form_names.empty())
        {
            auto names = boost::join(form_names, ", ");
            ch->send_to("Available Forms: @W%s@n\r\n", names.c_str());
        }

        const double epsilon = 0.05; // A small tolerance value for floating point comparison

        auto tbonus = ch->getBaseStat("transBonus");

        if (tbonus <= -0.3 + epsilon)
        {
            ch->sendText("\r\n@WYou have @wGREAT@W transformation BPL Requirements.@n\r\n");
        }
        else if (tbonus <= -0.2 + epsilon)
        {
            ch->sendText("\r\n@MYou have @mabove average@M transformation BPL Requirements.@n\r\n");
        }
        else if (tbonus <= -0.1 + epsilon)
        {
            ch->sendText("\r\n@BYou have @bslightly above average@B transformation BPL Requirements.@n\r\n");
        }
        else if (tbonus < 0.1 - epsilon)
        {
            ch->sendText("\r\n@GYou have @gaverage@G transformation BPL Requirements.@n\r\n");
        }
        else if (tbonus < 0.2 - epsilon)
        {
            ch->sendText("\r\n@YYou have @yslightly below average@Y transformation BPL Requirements.@n\r\n");
        }
        else if (tbonus < 0.3 - epsilon)
        {
            ch->sendText("\r\n@CYou have @cbelow average@C transformation BPL Requirements.@n\r\n");
        }
        else
        {
            ch->sendText("\r\n@RYou have @rterrible@R transformation BPL Requirements.@n\r\n");
        }
        ch->sendText("@b------------------------------------------------@n\r\n");
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << ik;
        std::string growthString = ss.str();
        ch->send_to("\r\n@BGrowth: %s@n\r\n", growthString);
    }

    static const std::unordered_map<Form, std::pair<int64_t, int>> trans_pl = {
        // Human
        {Form::super_human_1, {40, 0}},
        {Form::super_human_2, {80, 0}},
        {Form::super_human_3, {90, 0}},
        {Form::super_human_4, {140, 0}},

        // Saiyan/Halfbreed.
        {Form::super_saiyan_1, {30, 0}},
        {Form::super_saiyan_2, {60, 0}},
        {Form::super_saiyan_3, {90, 0}},
        {Form::super_saiyan_4, {180, 0}},

        // Lssj
        {Form::ikari, {120, 0}},
        {Form::legendary_saiyan, {220, 0}},

        // Namek Forms
        {Form::super_namekian_1, {50, 0}},
        {Form::super_namekian_2, {50, 0}},
        {Form::super_namekian_3, {70, 0}},
        {Form::super_namekian_4, {130, 0}},

        // Icer Forms
        {Form::icer_1, {25, 0}},
        {Form::icer_2, {50, 0}},
        {Form::icer_3, {120, 0}},
        {Form::icer_4, {160, 0}},

        // Majin Forms
        {Form::maj_affinity, {40, 1}},
        {Form::maj_super, {80, 1}},
        {Form::maj_true, {120, 1}},

        // Tuffle Forms
        {Form::ascend_1, {50, 1}},
        {Form::ascend_2, {70, 1}},
        {Form::ascend_3, {120, 1}},

        // Mutant Forms
        {Form::mutate_1, {30, 0}},
        {Form::mutate_2, {75, 0}},
        {Form::mutate_3, {140, 0}},

        // Kai Forms
        {Form::mystic_1, {50, 0}},
        {Form::mystic_2, {80, 0}},
        {Form::mystic_3, {150, 0}},

        {Form::divine_halo, {250, 0}},

        // Konatsu Forms

        {Form::shadow_first, {35, 0}},
        {Form::shadow_second, {75, 0}},
        {Form::shadow_third, {125, 0}},

        // Android Forms
        {Form::android_1, {10, 1}},
        {Form::android_2, {20, 1}},
        {Form::android_3, {30, 1}},
        {Form::android_4, {40, 1}},
        {Form::android_5, {50, 1}},
        {Form::android_6, {60, 1}},

        // Bio Forms
        {Form::bio_mature, {30, 1}},
        {Form::bio_semi_perfect, {60, 1}},
        {Form::bio_perfect, {80, 1}},
        {Form::bio_super_perfect, {120, 1}},

        // Demon Forms
        {Form::dark_king, {180, 0}},

        // Lycan Forms
        {Form::lycanthrope, {60, 0}},
        {Form::alpha_lycanthrope, {120, 0}},

        // Unbound Alternate Forms
        {Form::potential_unleashed, {120, 0}},
        {Form::ultra_instinct, {200, 0}},

        // Unbound Perm Forms
        {Form::potential_unlocked, {40, 1}},
        {Form::potential_unlocked_max, {90, 1}},
        {Form::majinized, {0, 1}},
        {Form::divine_water, {25, 1}},

        // Techniques
        {Form::kaioken, {0, 2}},
        {Form::dark_metamorphosis, {0, 2}},

        {Form::tiger_stance, {0, 2}},
        {Form::eagle_stance, {0, 2}},
        {Form::ox_stance, {0, 2}},

        {Form::evil_aura, {90, 2}},
        {Form::spirit_absorption, {0, 2}},

    };

    int64_t getRequiredPL(Character *ch, Form trans)
    {
        if (auto req = trans_pl.find(trans); req != trans_pl.end())
        {
            return req->second.first;
        }
        return 0;
    }

    int getFormType(Character *ch, Form trans)
    {
        if (auto req = trans_pl.find(trans); req != trans_pl.end())
        {
            return req->second.second;
        }
        return 0;
    }

    std::optional<Form> findFormFor(Character *ch, const std::string &arg)
    {
        for (auto form : getFormsFor(ch))
        {
            if (boost::iequals(arg, getAbbr(ch, form)))
                return form;
        }

        return {};
    }

    bool unlock(Character *ch, Form form)
    {
        initTransforms(ch);
        auto &data = ch->transforms[form];

        if (data.unlocked == false)
        {
            if (ch->getBaseStat("internalGrowth") >= getRequiredPL(ch, form))
                ch->modBaseStat("internalGrowth", -getRequiredPL(ch, form));
            else
                return false;
            data.unlocked = true;
            removeExclusives(ch, form);
            if (form == Form::lycanthrope || form == Form::alpha_lycanthrope)
                data.visible = false;
        }
        return data.unlocked;
    }

    void gamesys_oozaru(uint64_t heartPulse, double deltaTime) {

    };

    static std::unordered_map<Form, std::function<void(Character *ch)>> trans_on_transform = {
        {Form::spirit_absorption, [](Character *ch)
         {
             auto loc = ch->location.searchObjects([](Object *obj)
                                                   { return isname("Spirit Bomb", obj->getName()); });
             if (loc == nullptr)
                 revert(ch);
             else
             {
                 act("@WYou launch yourself into the large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W! It eclipses everything in your sight as you desperately try to hold onto the coursing energy!@n",
                     true, ch, nullptr, nullptr, TO_CHAR);
                 act("@W$n jumps directly into the @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W as it descends! It completely obscures $n from view before starting to be absorbed by them!@n",
                     true, ch, nullptr, nullptr, TO_ROOM);
                 hurt(0, 0, loc->user, ch, nullptr, loc->getBaseStat("kicharge") * 1.25, 1);
                 ch->transforms[Form::spirit_absorption].vars["energy"] = loc->getBaseStat("kicharge") * 0.9 + (0.1 * getMasteryTier(ch, Form::spirit_absorption));
                 ch->transforms[Form::spirit_absorption].vars["absorbed"] = loc->getBaseStat("kicharge") * 0.9 + (0.1 * getMasteryTier(ch, Form::spirit_absorption));
                 ch->send_to("@W[@cSpirit Force: @C%s@W]@n\r\n", add_commas((int64_t)ch->transforms[Form::spirit_absorption].vars["energy"]).c_str());
                 extract_obj(loc);
                 act("@WThe @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W completely vanishes as it's absorbed, imploding inwards!@n",
                     true, nullptr, nullptr, nullptr, TO_ROOM);
             }

             return loc == nullptr ? false : true;
         }},
        {Form::lycanthrope, [](Character *ch)
         {
             if (!ch->transforms.contains(Form::lesser_lycanthrope))
                 ch->addTransform(Form::lesser_lycanthrope);

             if (!ch->transforms[Form::lesser_lycanthrope].unlocked)
                 ch->transforms[Form::lesser_lycanthrope].unlocked = true;
         }},
    };

    static std::unordered_map<Form, std::function<void(Character *ch, atk::Attack &outgoing)>> trans_on_attack = {
        {Form::spirit_absorption, [](Character *ch, atk::Attack &outgoing)
         {
             ch->send_to("Spirit Force infuses your %s, cloaking it in an @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n hue.\r\n", outgoing.getName());
             act("@W$n's @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n aura infuses the attack with cataclysmic energy.@n",
                 true, ch, nullptr, nullptr, TO_ROOM);

             double empower = ch->transforms[Form::spirit_absorption].vars["absorbed"] / 50;
             empower *= ch->transforms[Form::spirit_absorption].grade;
             if (outgoing.isKiAttack())
                 empower *= 4;
             if (empower > ch->transforms[Form::spirit_absorption].vars["energy"])
                 empower = ch->transforms[Form::spirit_absorption].vars["energy"];
             ch->transforms[Form::spirit_absorption].vars["energy"] -= empower;
             outgoing.calcDamage += empower * 2;

             if (ch->transforms[Form::spirit_absorption].vars["energy"] <= 0)
                 revert(ch);
             ch->send_to("@W[@cSpirit Force: @C%s@W]@n\r\n", add_commas((int64_t)ch->transforms[Form::spirit_absorption].vars["energy"]).c_str());
         }},
    };

    static std::unordered_map<Form, std::function<void(Character *ch, atk::Attack &incoming)>> trans_on_attacked = {
        {Form::spirit_absorption, [](Character *ch, atk::Attack &incoming)
         {
             ch->send_to("Your @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n aura burns away at your opponent's %s.\r\n", incoming.getName());
             act("@W$n's @Ci@cn@Cc@ca@Cn@cd@Ce@cs@Cc@ce@Cn@ct@n aura burns away some of the incoming attack.@n",
                 true, ch, nullptr, nullptr, TO_ROOM);

             double disempower = ch->transforms[Form::spirit_absorption].vars["absorbed"] / 50;
             disempower *= ch->transforms[Form::spirit_absorption].grade;
             if (disempower > ch->transforms[Form::spirit_absorption].vars["energy"])
                 disempower = ch->transforms[Form::spirit_absorption].vars["energy"];
             ch->transforms[Form::spirit_absorption].vars["energy"] -= disempower;
             incoming.calcDamage -= disempower * 2;

             if (ch->transforms[Form::spirit_absorption].vars["energy"] <= 0)
                 revert(ch);
             ch->send_to("@W[@cSpirit Force: @C%s@W]@n\r\n", add_commas((int64_t)ch->transforms[Form::spirit_absorption].vars["energy"]).c_str());
         }},
    };

    void onAttack(Character *ch, atk::Attack &outgoing, Form form)
    {
        if (trans_on_attack.contains(form))
        {
            auto &on_trans_attk = trans_on_attack.at(form);
            on_trans_attk(ch, outgoing);
        }
    }

    void onAttacked(Character *ch, atk::Attack &incoming, Form form)
    {
        if (trans_on_attacked.contains(form))
        {
            auto &on_trans_attkd = trans_on_attacked.at(form);
            on_trans_attkd(ch, incoming);
        }
    }

    void onTransform(Character *ch, Form form)
    {
        ch->removeLimitBreak();
        reveal_hiding(ch, 0);
        handleEchoTransform(ch, form);

        if (trans_on_transform.contains(form))
        {
            auto &on_trans = trans_on_transform.at(form);
            on_trans(ch);
        }
    }

    void onRevert(Character *ch, Form form)
    {
        handleEchoRevert(ch, ch->form);
        ch->removeLimitBreak();
    }

    void revert(Character *ch)
    {
        int64_t beforeKi = ch->getEffectiveStat<int64_t>("ki");
        if (ch->form != Form::base)
        {
            onRevert(ch, ch->form);
            ch->form = Form::base;
            if (ch->technique == Form::base)
                characterSubscriptions.unsubscribe("transforms", ch);
        }
        if (ch->technique != Form::base)
        {
            onRevert(ch, ch->technique);
            ch->technique = Form::base;
            if (ch->form == Form::base)
                characterSubscriptions.unsubscribe("transforms", ch);
        }
        int64_t afterKi = ch->getEffectiveStat<int64_t>("ki");

        if (beforeKi > afterKi && GET_BARRIER(ch) > 0)
        {
            int64_t barrier = GET_BARRIER(ch);
            float ratio = (float)afterKi / (float)beforeKi;
            ch->setBaseStat<int64_t>("barrier", barrier * ratio);

            ch->sendText("Your barrier shimmers as it loses some energy with your transformation.\r\n");
        }
    }

    void transform(Character *ch, Form form, int grade)
    {
        int64_t beforeKi = ch->getEffectiveStat<int64_t>("ki");
        if (grade > getMaxGrade(ch, form))
        {
            grade = getMaxGrade(ch, form);
            ch->send_to("The max grade of this form is %s!\r\nSetting to max.\r\n", std::to_string(grade));
        }
        if (grade < 1)
            grade = 1;

        int formtype = getFormType(ch, form);

        if (grade > getMaxGrade(ch, form))
        {
            grade = getMaxGrade(ch, form);
            ch->send_to("The max grade of this form is %s!\r\nSetting to max.\r\n", std::to_string(grade));
        }
        if (grade < 1)
            grade = 1;
        if (formtype == 0)
            ch->form = form;
        else if (formtype == 1)
            ch->permForms.insert(form);
        else if (formtype == 2)
            ch->technique = form;
        ch->transforms[form].grade = grade;

        characterSubscriptions.subscribe("transforms", ch);
        onTransform(ch, form);

        int64_t afterKi = ch->getEffectiveStat<int64_t>("ki");

        if (beforeKi > afterKi && GET_BARRIER(ch) > 0)
        {
            int64_t barrier = GET_BARRIER(ch);
            float ratio = (float)afterKi / (float)beforeKi;
            ch->setBaseStat<int64_t>("barrier", barrier * ratio);

            ch->sendText("Your barrier shimmers as it loses some energy with your transformation.\r\n");
        }

        // Announce noisy transformations in the zone.
        if (race::isSenseable(ch->race))
        {
            if (auto zone = ch->location.getZone(); zone)
            {
                zone->sendText("An explosion of power ripples through the surrounding area!\r\n");
            }
        }

        char buf3[MAX_INPUT_LENGTH];
        if(auto z = ch->location.getZone()->getRoot())
            z->sendToSense(ch, "powering up dramatically");
        sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(ch->getPL()).c_str());
        send_to_scouter(buf3, ch, 1, 0);
    }

    std::optional<std::string> getAppearance(Character *ch, Form form, Appearance type)
    {
        // first we'll check for a specific override
        if (auto found = ch->transforms.find(form); found != ch->transforms.end())
        {
            auto &data = found->second;
            if (auto appfound = data.appearances.find(type); appfound != data.appearances.end())
            {
                return appfound->second;
            }
        }

        switch (type)
        {
        case Appearance::hair_color:
            switch (form)
            {
            case Form::oozaru:
                return "brown";
            case Form::golden_oozaru:
            case Form::super_saiyan_1:
            case Form::super_saiyan_2:
            case Form::super_saiyan_3:
                return "golden";
            case Form::super_saiyan_4:
                return "black";
            case Form::super_saiyan_god:
                return "red";
            case Form::super_saiyan_blue:
                return "blue";
            case Form::legendary_saiyan:
                return "greenish-golden";
            default:
                return {};
            }
        case Appearance::aura_color:
            switch (form)
            {
            case Form::golden_oozaru:
            case Form::super_saiyan_1:
            case Form::super_saiyan_2:
            case Form::super_saiyan_3:
            case Form::super_saiyan_4:
                return "golden";
            case Form::super_saiyan_god:
                return "red";
            case Form::super_saiyan_blue:
                return "blue";
            case Form::legendary_saiyan:
                return "greenish-golden";
            case Form::ultra_instinct:
                return "silver";
            default:
                return {};
            }
        case Appearance::eye_color:
            switch (form)
            {
            case Form::super_saiyan_1:
            case Form::super_saiyan_2:
            case Form::super_saiyan_3:
                return "green";
            case Form::super_saiyan_god:
                return "red";
            case Form::super_saiyan_blue:
                return "blue";
            default:
                return {};
            }
            default:
                return {};
        }

        return {};
    }

}
