/* ************************************************************************
 *  File: combat.c                    Part of Dragonball Advent Truth      *
 *  Usage: Combat utilities and common functions for act.offensive.c and   *
 *  act.attack.c                                                           *
 *                                                                         *
 *  All rights reserved to Iovan that are not due to anyone else.          *
 *                                                                         *
 *  This file was first written on 2011 and aside for a few instances only *
 *  contains code written by Iovan for use with the Real Dragonball Battle *
 *  System (RDBS) of the MUD Dragonball Advent Truth.                      *
 ************************************************************************ */
#include "dbat/Character.h"
#include "dbat/Object.h"
#include "dbat/Room.h"
#include "dbat/Destination.h"
#include "dbat/Descriptor.h"
#include "dbat/Zone.h"
#include "dbat/combat.h"
#include "dbat/act.movement.h"
#include "dbat/act.item.h"
#include "dbat/mobact.h"
#include "dbat/fight.h"
#include "dbat/act.attack.h"
#include "dbat/dg_comm.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/act.item.h"
#include "dbat/handler.h"
#include "dbat/constants.h"
#include "dbat/genzon.h"
#include "dbat/dg_scripts.h"
#include "dbat/class.h"
#include "dbat/techniques.h"
#include "dbat/act.informative.h"

/* local functions */
void damage_weapon(Character *ch, Object *obj, Character *vict)
{

    if (obj)
    {
        if (OBJ_FLAGGED(obj, ITEM_UNBREAKABLE))
            return;
    }
    int ranking = 0, material = 1;
    int64_t PL10 = 2000000000, PL9 = 2000000000, PL8 = 2000000000;

    PL10 = PL10 * 5;
    PL9 = PL9 * 4;
    PL8 = PL8 * 2;

    if (GET_HIT(vict) >= PL10)
    {
        ranking = 10;
    }
    else if (GET_HIT(vict) >= PL9)
    {
        ranking = 9;
    }
    else if (GET_HIT(vict) >= PL8)
    {
        ranking = 8;
    }
    else if (GET_HIT(vict) >= 2000000000)
    {
        ranking = 7;
    }
    else if (GET_HIT(vict) >= 500000000)
    {
        ranking = 6;
    }
    else if (GET_HIT(vict) >= 250000000)
    {
        ranking = 5;
    }
    else if (GET_HIT(vict) >= 100000000)
    {
        ranking = 4;
    }
    else if (GET_HIT(vict) >= 50000000)
    {
        ranking = 3;
    }
    else if (GET_HIT(vict) >= 25000000)
    {
        ranking = 2;
    }
    else if (GET_HIT(vict) >= 1000000)
    {
        ranking = 1;
    }

    switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL))
    {
    case MATERIAL_STEEL:
        material = 4;
        break;
    case MATERIAL_IRON:
    case MATERIAL_COPPER:
    case MATERIAL_BRASS:
    case MATERIAL_METAL:
        material = 2;
        break;
    case MATERIAL_SILVER:
        material = 5;
        break;
    case MATERIAL_KACHIN:
        material = 9;
        break;
    case MATERIAL_CRYSTAL:
        material = 7;
        break;
    case MATERIAL_DIAMOND:
        material = 8;
        break;
    case MATERIAL_PAPER:
    case MATERIAL_COTTON:
    case MATERIAL_SATIN:
    case MATERIAL_SILK:
    case MATERIAL_BURLAP:
    case MATERIAL_VELVET:
    case MATERIAL_HEMP:
    case MATERIAL_WAX:
        material = 0;
        break;
    default:
        break;
    }

    int result = ranking - material;

    if (AFF_FLAGGED(ch, AFF_CURSE))
    {
        result += 3;
    }
    else if (AFF_FLAGGED(ch, AFF_BLESS) && Random::get<int>(1, 3) == 3)
    {
        if (result > 1)
        {
            result = 1;
        }
    }
    else if (AFF_FLAGGED(ch, AFF_BLESS))
    {
        result = 0;
    }

    if (GET_SKILL(ch, SKILL_HANDLING) >= axion_dice(10))
    {
        act("@GYour superior handling prevents @C$p@G from being damaged.@n", true, ch, obj, nullptr, TO_CHAR);
        act("@g$n's@G superior handling prevents @C$p@G from being damaged.@n", true, ch, obj, nullptr, TO_ROOM);
        result = 0;
    }

    if (result > 0)
    {

        if (MOD_OBJ_VAL(obj, VAL_ALL_HEALTH, -result) <= 0)
        {
            act("@RYour @C$p@R shatters on @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
            act("@r$n's@R @C$p@R shatters on YOUR body!@n", true, ch, obj, vict, TO_VICT);
            act("@r$n's@R @C$p@R shatters on @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
            obj->item_flags.set(ITEM_BROKEN, true);
            perform_remove(vict, 16);
            perform_remove(vict, 17);
        }
        else if (result >= 8)
        {
            act("@RYour @C$p@R cracks loudly from striking @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
            act("@r$n's@R @C$p@R cracks loudly from striking YOUR body!@n", true, ch, obj, vict, TO_VICT);
            act("@r$n's@R @C$p@R cracks loudly from striking @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
        }
        else if (result >= 6)
        {
            act("@RYour @C$p@R chips from striking @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
            act("@r$n's@R @C$p@R cracks from striking YOUR body!@n", true, ch, obj, vict, TO_VICT);
            act("@r$n's@R @C$p@R cracks from striking @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
        }
        else if (result >= 3)
        {
            act("@RYour @C$p@R loses a piece from striking @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
            act("@r$n's@R @C$p@R loses a piece from striking YOUR body!@n", true, ch, obj, vict, TO_VICT);
            act("@r$n's@R @C$p@R loses a piece from striking @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
        }
        else if (result >= 6)
        {
            act("@RYour @C$p@R has a nick in it from hitting @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
            act("@r$n's@R @C$p@R has a nick in it from hitting YOUR body!@n", true, ch, obj, vict, TO_VICT);
            act("@r$n's@R @C$p@R has a nick in it from hitting @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
        }
    }
}

void handle_multihit(Character *ch, Character *vict)
{

    int perc = GET_DEX(ch), prob = GET_DEX(vict);

    /* Let's give the victim a bit of automatic favor due to "luck" */
    prob += Random::get<int>(1, 15);
    /* Some otherwise inclined "luck" for the attacker */
    perc += Random::get<int>(-5, 5);

    if (ch->getBaseStat<int>("throws") >= 3)
    {
        ch->setBaseStat<int>("throws", 0);
        return;
    }
    else if (ch->getBaseStat<int>("throws") == -1)
    {
        ch->setBaseStat<int>("throws", 0);
    }

    /* Racial bonuses */
    if (IS_KONATSU(ch))
    {
        perc *= 1.5;
    }
    if (ch->bio_genomes.get(Race::konatsu))
    {
        perc *= 1.4;
    }

    if (IS_NPC(ch))
    {
        perc -= perc * 0.3;
    }

    /* Weapons have less chance to multihit */
    if (LASTATK(ch) == -1)
    {
        perc *= 0.75;
    }

    int amt = 70;

    if (GET_SKILL(ch, SKILL_STYLE) >= 100)
    {
        amt -= amt * 0.1;
    }
    else if (GET_SKILL(ch, SKILL_STYLE) >= 80)
    {
        amt -= amt * 0.08;
    }
    else if (GET_SKILL(ch, SKILL_STYLE) >= 60)
    {
        amt -= amt * 0.06;
    }
    else if (GET_SKILL(ch, SKILL_STYLE) >= 40)
    {
        amt -= amt * 0.04;
    }
    else if (GET_SKILL(ch, SKILL_STYLE) >= 20)
    {
        amt -= amt * 0.02;
    }

    /* Critical Failure*/
    if (axion_dice(0) < amt)
    {
        prob += 500;
    }

    /* Success! */
    if (perc >= prob)
    {
        char buf[MAX_INPUT_LENGTH];
        act("@Y...in a lightning flash of speed you attack @y$N@Y again!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@Y...in a lightning flash of speed @y$n@Y attacks YOU again!@n", true, ch, nullptr, vict, TO_VICT);
        act("@Y...in a lightning flash of speed @y$n@Y attacks @y$N@Y again!@n", true, ch, nullptr, vict, TO_NOTVICT);
        ch->modBaseStat<int>("throws", 1);
        ch->player_flags.set(PLR_MULTIHIT, true);
        if (COMBO(ch) > -1)
        {
            switch (COMBO(ch))
            {
            case 0:
                sprintf(buf, "%s", GET_NAME(vict));
                do_punch(ch, buf, 0, 0);
                break;
            case 1:
                sprintf(buf, "%s", GET_NAME(vict));
                do_kick(ch, buf, 0, 0);
                break;
            case 2:
                sprintf(buf, "%s", GET_NAME(vict));
                do_elbow(ch, buf, 0, 0);
                break;
            case 3:
                sprintf(buf, "%s", GET_NAME(vict));
                do_knee(ch, buf, 0, 0);
                break;
            case 4:
                sprintf(buf, "%s", GET_NAME(vict));
                do_roundhouse(ch, buf, 0, 0);
                break;
            case 5:
                sprintf(buf, "%s", GET_NAME(vict));
                do_uppercut(ch, buf, 0, 0);
                break;
            case 6:
                sprintf(buf, "%s", GET_NAME(vict));
                do_slam(ch, buf, 0, 0);
                ch->modBaseStat<int>("throws", 1);
                break;
            case 8:
                sprintf(buf, "%s", GET_NAME(vict));
                do_heeldrop(ch, buf, 0, 0);
                ch->modBaseStat<int>("throws", 1);
                break;
            case 51:
                sprintf(buf, "%s", GET_NAME(vict));
                do_bash(ch, buf, 0, 0);
                ch->modBaseStat<int>("throws", 1);
                break;
            case 52:
                sprintf(buf, "%s", GET_NAME(vict));
                do_head(ch, buf, 0, 0);
                break;
            case 56:
                sprintf(buf, "%s", GET_NAME(vict));
                do_tailwhip(ch, buf, 0, 0);
                ch->modBaseStat<int>("throws", 1);
                break;
            }
        }
        else
        {
            if (LASTATK(ch) == -1)
            {
                sprintf(buf, "%s", GET_NAME(vict));
                do_attack(ch, buf, 0, 0);
            }
            else
            {
                if (Random::get<int>(1, 3) == 2 && GET_SKILL(ch, SKILL_KICK) > 0)
                {
                    sprintf(buf, "%s", GET_NAME(vict));
                    do_kick(ch, buf, 0, 0);
                }
                else
                {
                    sprintf(buf, "%s", GET_NAME(vict));
                    do_punch(ch, buf, 0, 0);
                }
            }
        }
    }
}

int handle_defender(Character *vict, Character *ch)
{

    int result = false;

    if (GET_DEFENDER(vict))
    {
        Character *def = GET_DEFENDER(vict);
        int64_t defnum = (GET_SPEEDI(def) * 0.01) * Random::get<int>(-10, 10);
        int64_t chnum = (GET_SPEEDI(ch) * 0.01) * Random::get<int>(-5, 10);
        if (GET_SPEEDI(def) + defnum > GET_SPEEDI(ch) + chnum && def->location == vict->location &&
            GET_POS(def) > POS_SITTING)
        {
            act("@YYou move to and manage to intercept the attack aimed at @y$N@Y!@n", true, def, nullptr, vict,
                TO_CHAR);
            act("@y$n@Y moves to and manages to intercept the attack aimed at YOU!@n", true, def, nullptr, vict,
                TO_VICT);
            act("@y$n@Y moves to and manages to intercept the attack aimed at @y$N@Y!@n", true, def, nullptr, vict,
                TO_NOTVICT);
            result = true;
        }
        else if (def->location == vict->location && GET_POS(def) > POS_SITTING)
        {
            act("@YYou move to intercept the attack aimed at @y$N@Y, but just not fast enough!@n", true, def, nullptr,
                vict, TO_CHAR);
            act("@y$n@Y moves to intercept the attack aimed at YOU, but $e wasn't fast enough!@n", true, def, nullptr,
                vict, TO_VICT);
            act("@y$n@Y moves to intercept the attack aimed at @y$N@Y, but $e wasn't fast enough!@n", true, def,
                nullptr, vict, TO_NOTVICT);
        }
    }

    return (result);
}

void handle_disarm(Character *ch, Character *vict)
{

    int roll1 = Random::get<int>(-10, 10), roll2 = Random::get<int>(-10, 10), handled = false;
    roll1 += GET_STR(ch) + GET_DEX(ch);
    roll2 += GET_STR(vict) + GET_DEX(vict);

    if (!IS_NPC(ch))
    {
        if (PLR_FLAGGED(ch, PLR_THANDW))
        {
            roll1 += 5;
        }
    }

    if (Random::get<int>(1, 100) <= 50 && !IS_KONATSU(ch))
    {
        roll1 = -500;
    }
    else if (Random::get<int>(1, 100) <= 75)
    {
        roll1 *= 1.5;
    }

    if (IS_KONATSU(vict))
    {
        roll1 *= 0.75;
    }

    if (GET_SKILL(ch, SKILL_HANDLING) >= axion_dice(10))
    {
        handled = true;
    }

    if (roll1 < roll2)
    {
        Object *obj;
        if (GET_EQ(ch, WEAR_WIELD1) && handled == false)
        {
            obj = GET_EQ(ch, WEAR_WIELD1);
            act("@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n", true, ch, obj, vict, TO_CHAR);
            act("@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_NOTVICT);
            act("@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_VICT);
            perform_remove(ch, 16);
            if (GET_OBJ_VNUM(obj) != 20098)
            {
                obj->clearLocation();
                obj->moveToLocation(ch);
            }
        }
        else if (GET_EQ(ch, WEAR_WIELD1) && handled == true)
        {
            obj = GET_EQ(ch, WEAR_WIELD1);
            act("@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it from slipping from your grasp!@n",
                true, ch, obj, vict, TO_CHAR);
            act("@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true,
                ch, obj, vict, TO_NOTVICT);
            act("@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true,
                ch, obj, vict, TO_VICT);
        }
        else if (GET_EQ(ch, WEAR_WIELD2) && handled == true)
        {
            obj = GET_EQ(ch, WEAR_WIELD2);
            act("@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it from slipping from your grasp!@n",
                true, ch, obj, vict, TO_CHAR);
            act("@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true,
                ch, obj, vict, TO_NOTVICT);
            act("@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true,
                ch, obj, vict, TO_VICT);
        }
        else if (GET_EQ(ch, WEAR_WIELD2))
        {
            obj = GET_EQ(ch, WEAR_WIELD2);
            act("@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n", true, ch, obj, vict, TO_CHAR);
            act("@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_NOTVICT);
            act("@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_VICT);
            perform_remove(ch, 17);
            if (GET_OBJ_VNUM(obj) != 20098)
            {
                obj->clearLocation();
                obj->moveToLocation(ch);
            }
        }
    }
}

void combine_attacks(Character *ch, Character *vict)
{

    struct follow_type *f;
    char chbuf[MAX_INPUT_LENGTH], victbuf[MAX_INPUT_LENGTH], rmbuf[MAX_INPUT_LENGTH];
    int64_t bonus = 0;
    double maxki = 0.0;
    int totalmem = 1;
    int attspd = 0, blockable = true, same = true, attsk = 0, attavg = GET_SKILL(ch, attack_skills[GET_COMBINE(ch)]);
    int burn = false, shocked = false;

    switch (GET_COMBINE(ch))
    {
    case 0: /* Kamehameha */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You cup your hands at your sides and a ball of @Benergy@W forms there. You chant @B'@CKaaaaameeeehaaaameeee@B'@W and then fire a @RKamehameha@W wave at @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W cups $s hands be $s side and chants @B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands and he quickly brings them forward and fires a @RKamehameha @Wat @rYOU@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W cups $s hands be $s side and chants @B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands and he quickly brings them forward and fires a @RKamehameha @Wat @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n@n");
        maxki = 0.15;
        attspd += 2;
        bonus += GET_MAX_MOVE(ch) * 0.02;
        break;
    case 1: /* Galik Gun */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You throw your hands forward and launch a purple beam of energy at @r$N@n while shouting @B'@mGalik Gun!@B'@W");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W throws $s arms forward and launches a purple beam at @rYOU@W while shouting @B'@mGalik Gun!@B'@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W throws $s arms forward and launches a purple beam at @r$N@W while shouting @B'@mGalik Gun!@B'@n");
        maxki = 0.15;
        attspd += 1;
        bonus += GET_MAX_MANA(ch) * 0.5;
        break;
    case 2: /* Masenko */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You raise your hands above your head with one resting atop the other and begin to pour your charged energy to that point. As soon as the energy is ready you shout @B'@RMasenko Ha!@B'@W and bringing your hands down you launch a bright reddish orange beam at @r$N@W!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises $s hands above $s head and energy quicly pools there. Suddenly $e brings $s hands down and shouts @B'@RMasenko Ha!@B'@W as a bright reddish orange beam launches toward @rYOU!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises $s hands above $s head and energy quicly pools there. Suddenly $e brings $s hands down and shouts @B'@RMasenko Ha!@B'@W as a bright reddish orange beam launches toward @r$N!@n");
        maxki = 0.15;
        attspd += 1;
        bonus += GET_MAX_MANA(ch) * 0.5;
        break;
    case 3: /* Deathbeam */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With a quick motion you point at @r$N@W and launch a lightning fast @MDeathbeam@W!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! With a quick motion $e points $s finger at @rYOU@W and launches a lightning fast @MDeathbeam@W!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! With a quick motion $e points $s finger at @r$N@W and launches a lightning fast @MDeathbeam@W!@n");
        maxki = 0.1;
        attspd += 4;
        break;
    case 4: /* Honoo */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy ready you breath out toward @r$N@W jets of incredibly hot flames in the form of a deadly @rHonoo@W!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Sudden jets of flame burst forth from $s mouth at @RYOU@W in the form of a deadly @rHonoo@W!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Sudden jets of flame burst forth from $s mouth at @R$N@W in the form of a deadly @rHonoo@W!@n");
        maxki = 0.125;
        attspd += 2;
        burn = true;
        break;
    case 5: /* Twin Slash */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy prepared you poor it into your blade and accelerate your body to incredible speeds toward @r$N! You leave two glowing green marks behind on $S body in a single instant as your @GTwin Slash@W hits!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Raising $s sword @Y$n@W accelerates toward @rYOU@W with incredible speed! Two glowing green slashes are left on YOUR body from $s successful @GTwin Slash@W!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Raising $s sword @Y$n@W accelerates toward @r$N@W with incredible speed! Two glowing green slashes are left on @R$N's@W body from the successful @GTwin Slash@W!@n");
        maxki = 0.125;
        attspd += 2;
        blockable = false;
        break;
    case 6: /* Hell Flash */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You stick one of each of your hands in your armpits and detach them. With your hands detached your point the exposed arm cannons at @r$N@W and launch a massive @RHell Flash@W at $M!");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W sticks one of each of $s hands in $s armpits and detaches them there. With the hands detached $e aims $s exposed arm cannons at @RYOU@W and launches a massive @RHell Flash@W!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W sticks one of each of $s hands in $s armpits and detaches them there. With the hands detached $e aims $s exposed arm cannons at @R$N@W and launches a massive @RHell Flash@W!@n");
        maxki = 0.2;
        attspd += 1;
        break;
    case 7: /* Psychic Blast */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy ready you look at @R$N@W as the blue light of your @CPsychic Blast@W launches from your head toward $S!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! A blue light, identifying a @CPsychic Blast@W, launches from @Y$n's@W toward @RYOUR HEAD@W!");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! A blue light, identifying a @CPsychic Blast@W, launches from @Y$n's@W toward @R$N's@W head!");
        maxki = 0.125;
        attspd += 1;
        shocked = true;
        break;
    case 8: /* Crusher Ball */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! Pooling your energy you form a large ball of red energy above an upraised palm. Slamming your other hand into it you launch it toward @r$N@W while shouting @B'@RCrusher Ball@B'@W!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises a palm above his head and red energy begins to pool there. As the energy completes the formation of a ball @Y$n@W slams $s other hand into it and launches it at @rYOU@W while shouting @B'@RCrusher Ball@B'@W!");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises a palm above his head and red energy begins to pool there. As the energy completes the formation of a ball @Y$n@W slams $s other hand into it and launches it at @r$N@W while shouting @B'@RCrusher Ball@B'@W!");
        maxki = 0.2;
        break;
    case 9: /* Water Spikes */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! Using your energy to form a ball of water between your hands you then raise the ball above your head. Several spiked of ice form from the ball of water and you hurl them at @r$N@W!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Forming a ball of water between $s palms with $s energy @Y$n@W then raises the ball of water above $s head. Suddenly several spikes of ice form from the water and $e launches them at @rYOU@W!");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Forming a ball of water between $s palms with $s energy @Y$n@W then raises the ball of water above $s head. Suddenly several spikes of ice form from the water and $e launches them at @r$N@W!");
        maxki = 0.14;
        break;
    case 10: /* Tribeam */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You form a triangle with your hands and aim the center of the triangle at @r$N@W. With the sudden shout @B'@YTribeam@B'@W you release your prepared energy at $M in the form of a beam!");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W forms a triangle with $s hands and aims the center at @rYOU@W! With the sudden shout @B'@YTribeam@B'@W a large beam of energy flashes toward @rYOU!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W forms a triangle with $s hands and aims the center at @r$N@W! With the sudden shout @B'@YTribeam@B'@W a large beam of energy flashes toward @r$N!@n");
        maxki = 0.2;
        attspd += 2;
        bonus += GET_MAX_HIT(ch) * 0.5;
        break;
    case 11: /* Starbreaker */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group, you call out to your allies to launch a group attack! You raise your right hand above your head as dark red energy begins to pool in your slightly cupped hand, while purple arcs of electricity flow up your left arm. Slamming both hands together, you shout @B'@YStarbreaker@B'@W and release your prepared energy at $M in the form of a ball!@n");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises $s right hand, pooling dark red energy in the palm. @Y$n@W slams both their hands together, shouting @B'@YStarbreaker@B'@W, a ball of energy flashes toward @rYOU!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises their right hand above their head, pooling dark red energy. @Y$n@W slams both their hands together, shouting @B'@YStarbreaker@B'@W, a ball of energy flashes toward @r$N!@n");
        maxki = 0.2;
        bonus += GET_MAX_MANA(ch) * 0.6;
        break;
    case 12: /* Seishou Enko */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You open your mouth and aim at @r$N@W. You grunt as you release your prepared energy at $M in the form of a beam!");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W opens $s mouth and aims at @rYOUW! With the sudden grunt, a large beam flashes towards @rYOU@n!");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W opens $s mouth and aims at @rYOUW! With the sudden grunt, a large beam flashes toward @r$N@n!");
        maxki = 0.125;
        attspd += 35;
        break;
    case 13: /* Renzokou Energy Dan */
        sprintf(chbuf,
                "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You slam your hands together and aim at @r$n@W. With the sudden shout @B'@YRenzoku Energy Dan@B'@W you release your prepared energy in the form of hundreds of ki blasts!");
        sprintf(victbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W slams both $s hands together and aims at @rYOUW! With the sudden shout @B'@YRenzoku Energy Dan@B'@W hundreds of ki blasts flash towards @rYOU!@n");
        sprintf(rmbuf,
                "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W slams $s hands together and aims at @r$N@W! With the sudden shout @B'@YRenzoku Energy Dan@B'@W hundreds of ki blasts flash towards @r$N!@n");
        maxki = 0.125;
        attspd += 6;
        break;
    default:
        send_to_imm("ERROR: Combine attacks failure for: %s", GET_NAME(ch));
        ch->sendText("An error has been logged. Be patient while waiting for Iovan's response.\r\n");
        return;
    }

    int64_t totki = 0;

    act(chbuf, true, ch, nullptr, vict, TO_CHAR);
    act(victbuf, true, ch, nullptr, vict, TO_VICT);
    act(rmbuf, true, ch, nullptr, vict, TO_NOTVICT);

    if (GET_CHARGE(ch) >= GET_MAX_MANA(ch) * maxki)
    {
        totki += GET_MAX_MANA(ch) * maxki;
        ch->modBaseStat<int64_t>("charge", -(GET_MAX_MANA(ch) * maxki));
    }
    else
    {
        totki += GET_CHARGE(ch);
        ch->setBaseStat<int64_t>("charge", 0);
    }

    ch->followers.for_each([&](Character* f) {
        if (!AFF_FLAGGED(f, AFF_GROUP)) return;
        if (GET_COMBINE(f) != GET_COMBINE(ch))
            {
                same = false;
            }
            if (GET_CHARGE(f) >= GET_MAX_MANA(f) * maxki)
            {
                totki += GET_MAX_MANA(f) * maxki;
                f->modBaseStat<int64_t>("charge", -(GET_MAX_MANA(f) * maxki));
            }
            else
            {
                totki += GET_CHARGE(f);
                f->setBaseStat<int64_t>("charge", 0);
            }
            totalmem += 1;
            attavg += GET_SKILL(f, attack_skills[GET_COMBINE(f)]);
            char folbuf[MAX_INPUT_LENGTH], folbuf2[MAX_INPUT_LENGTH];
            sprintf(folbuf, "@Y$n@W times and merges $s @B'@R%s@B'@W into the group attack!@n",
                    attack_names_comp[GET_COMBINE(f)]);
            sprintf(folbuf2, "@WYou time and merge your @B'@R%s@B'@W into the group attack!@n",
                    attack_names_comp[GET_COMBINE(f)]);
            act(folbuf, true, f, nullptr, nullptr, TO_ROOM);
            act(folbuf2, true, f, nullptr, nullptr, TO_CHAR);
    });

    totki += bonus;
    if (same == true)
    {
        totki += bonus;
    }
    attsk = attavg / totalmem;

    if (GET_COMBINE(ch) != 5)
    {
        if (attspd + attsk < GET_SKILL(vict, SKILL_DODGE) + (GET_CHA(ch) / 10))
        {
            act("@GYou manage to dodge nimbly through the combined attack of your enemies!@n", true, vict, nullptr,
                nullptr, TO_CHAR);
            act("@r$n@G manages to dodge nimbly through the combined attack!@n", true, vict, nullptr, nullptr, TO_ROOM);
            return;
        }
        else if (blockable == true && attspd + attsk < GET_SKILL(vict, SKILL_BLOCK) + (GET_STR(ch) / 10))
        {
            act("@GYou manage to effectivly block the combined attack of your enemies with the help of your great strength!@n",
                true, vict, nullptr, nullptr, TO_CHAR);
            act("@r$n@G manages to dodge nimbly through the combined attack!@n", true, vict, nullptr, nullptr, TO_ROOM);
            return;
        }
    }
    if (burn == true)
    {
        if (!AFF_FLAGGED(vict, AFF_BURNED) && Random::get<int>(1, 4) == 3 && !IS_DEMON(vict) &&
            !GET_BONUS(vict, BONUS_FIREPROOF))
        {
            vict->sendText("@RYou are burned by the attack!@n\r\n");
            ch->sendText("@RThey are burned by the attack!@n\r\n");
            vict->affect_flags.set(AFF_BURNED, true);
        }
        else if (GET_BONUS(vict, BONUS_FIREPROOF) || IS_DEMON(vict))
        {
            ch->sendText("@RThey appear to be fireproof!@n\r\n");
        }
        else if (GET_BONUS(vict, BONUS_FIREPRONE))
        {
            vict->sendText("@RYou are extremely flammable and are burned by the attack!@n\r\n");
            ch->sendText("@RThey are easily burned!@n\r\n");
            vict->affect_flags.set(AFF_BURNED, true);
        }
    }
    if (shocked == true)
    {
        if (!AFF_FLAGGED(vict, AFF_SHOCKED) && Random::get<int>(1, 4) == 4 && !AFF_FLAGGED(vict, AFF_SANCTUARY))
        {
            act("@MYour mind has been shocked!@n", true, vict, nullptr, nullptr, TO_CHAR);
            act("@M$n@m's mind has been shocked!@n", true, vict, nullptr, nullptr, TO_ROOM);
            vict->affect_flags.set(AFF_SHOCKED, true);
        }
    }
    hurt(0, 0, ch, vict, nullptr, totki, 1);
    if (same == true)
    {
        ch->followers.for_each([&](auto f) {
            f->sendText("@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n\r\n");
        });
        ch->sendText("@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n\r\n");
    }
}

int check_ruby(Character *ch)
{

    auto isHotRuby = [](const auto &o)
    { return o->getVnum() == 6600 && OBJ_FLAGGED(o, ITEM_HOT); };
    auto ruby = ch->searchInventory(isHotRuby);

    if (ruby)
    {
        act("@RYour $p@R flares up and disappears. Your fire attack has been aided!@n", true, ch, ruby, nullptr,
            TO_CHAR);
        act("@R$n's@R $p@R flares up and disappears!@n", true, ch, ruby, nullptr, TO_ROOM);
        extract_obj(ruby);
        return (1);
    }
    else
    {
        return (0);
    }
}

int64_t combo_damage(Character *ch, int64_t damage, int type)
{
    int64_t bonus = 0;

    if (type == 0)
    { /* Not a finish */
        int hits = COMBHITS(ch);

        if (hits >= 30)
        {
            bonus += damage * (hits * 0.15);
            bonus += damage * 12;
        }
        else if (hits >= 20)
        {
            bonus = damage * (hits * 0.1);
            bonus += damage * 10;
        }
        else if (hits >= 10)
        {
            bonus = damage * (hits * 0.1);
            bonus += damage * 5;
        }
        else if (hits >= 6)
        {
            bonus = damage * (hits * 0.1);
            bonus += damage * 1.5;
        }
        else if (hits >= 2)
        {
            bonus = damage * (hits * 0.05);
            bonus += damage * 0.2;
        }
    }
    else if (type == 1)
    {
        bonus = damage * 15;
    }

    return (bonus);
}

/* For getting into a better combat position */
int roll_balance(Character *ch)
{

    int chance = 0;

    if (IS_NPC(ch))
    {
        if (GET_LEVEL(ch) >= 100)
        {
            chance = Random::get<int>(80, 100);
        }
        else if (GET_LEVEL(ch) >= 80)
        {
            chance = Random::get<int>(75, 90);
        }
        else if (GET_LEVEL(ch) >= 70)
        {
            chance = Random::get<int>(70, 80);
        }
        else if (GET_LEVEL(ch) >= 60)
        {
            chance = Random::get<int>(65, 75);
        }
        else if (GET_LEVEL(ch) >= 50)
        {
            chance = Random::get<int>(50, 60);
        }
    }
    else
    {
        if (GET_SKILL(ch, SKILL_BALANCE) > 50)
        {
            chance = GET_SKILL(ch, SKILL_BALANCE);
        }
    }

    return (chance);
}

void handle_knockdown(Character *ch)
{
    int chance = 0;

    if (IS_NPC(ch))
    {
        if (GET_LEVEL(ch) >= 100)
        {
            chance = Random::get<int>(35, 45);
        }
        else if (GET_LEVEL(ch) >= 90)
        {
            chance = Random::get<int>(25, 35);
        }
        else if (GET_LEVEL(ch) >= 70)
        {
            chance = Random::get<int>(15, 25);
        }
        else if (GET_LEVEL(ch) >= 50)
        {
            chance = Random::get<int>(10, 15);
        }
        else if (GET_LEVEL(ch) >= 30)
        {
            chance = Random::get<int>(5, 10);
        }
    }
    else
    {
        chance = GET_SKILL(ch, SKILL_BALANCE) * 0.5;
    }

    if (chance > axion_dice(0))
    {
        act("@mYou are @GALMOST@m knocked off your feet, but your great balance helps you keep your footing!@n", true,
            ch, nullptr, nullptr, TO_CHAR);
        act("@W$n@m is @GALMOST@m knocked off $s feet, but $s great balance helps $m keep $s footing!@n", true, ch,
            nullptr, nullptr, TO_ROOM);
    }
    else
    {
        act("@mYou are knocked off your feet!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n@m is knocked off $s feet!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->setBaseStat<int>("position", POS_SITTING);
    }
}

int boom_headshot(Character *ch)
{

    int skill = GET_SKILL_BASE(ch, SKILL_TWOHAND);

    if (skill >= 100 && Random::get<int>(1, 5) >= 3)
        return (1);
    else if (skill < 100 && skill >= 75 && Random::get<int>(1, 5) == 5)
        return (1);
    else if (skill < 75 && skill >= 50 && Random::get<int>(1, 6) == 6)
        return (1);
    else
        return (0);
}

int64_t gun_dam(Character *ch, int wlvl)
{
    int64_t dmg = 100;

    switch (wlvl)
    {
    case 1:
        dmg = 50;
        break;
    case 2:
        dmg = 200;
        break;
    case 3:
        dmg = 750;
        break;
    case 4:
        dmg = 2000;
        break;
    case 5:
        dmg = 7500;
        break;
    }

    if (GET_SKILL(ch, SKILL_GUN) >= 100)
        dmg *= 2;
    else if (GET_SKILL(ch, SKILL_GUN) >= 50)
        dmg += dmg * 0.5;

    int64_t dmg_prior = 0;

    dmg_prior = (dmg * GET_DEX(ch)) * ((GET_CHA(ch) / 5) + 1);

    if (dmg_prior <= GET_MAX_HIT(ch) * 0.4)
        dmg = dmg_prior;
    else
        dmg = GET_MAX_HIT(ch) * 0.4;

    return (dmg);
}

void club_stamina(Character *ch, Character *vict, int wlvl, int64_t dmg)
{

    double drain = 0.0;
    int64_t drained = 0;

    switch (wlvl)
    {
    case 1:
        drain = 0.05;
        break;
    case 2:
        drain = 0.1;
        break;
    case 3:
        drain = 0.15;
        break;
    case 4:
        drain = 0.2;
        break;
    case 5:
        drain = 0.25;
        break;
    }

    if (GET_SKILL(ch, SKILL_CLUB) >= 100)
        drain += 0.1;
    else if (GET_SKILL(ch, SKILL_CLUB) >= 50)
        drain += 0.05;

    drained = dmg * drain;
    vict->modCurVital(CharVital::stamina, -drained);

    ch->send_to("@D[@YVictim's @GStamina @cLoss@W: @g%s@D]@n\r\n", add_commas(drained).c_str());
    vict->send_to("@D[@rYour @GStamina @cLoss@W: @g%s@D]@n\r\n", add_commas(drained).c_str());
}

int backstab(Character *ch, Character *vict, int wlvl, int64_t dmg)
{

    int chance = 0, roll_to_beat = Random::get<int>(1, 100);
    double bonus = 0.0;

    if (GET_BACKSTAB_COOL(ch) > 0)
        return (0);

    switch (wlvl)
    {
    case 1:
        chance = 10;
        bonus += 0.5;
        break;
    case 2:
        chance = 15;
        bonus += 2;
        break;
    case 3:
        chance = 20;
        bonus += 3;
        break;
    case 4:
        chance = 25;
        bonus += 4;
        break;
    case 5:
        chance = 30;
        bonus += 4;
        break;
    }

    if (GET_BONUS(ch, BONUS_POWERHIT))
    {
        bonus += 2;
    }

    if (GET_SKILL(ch, SKILL_DAGGER) >= 100)
        chance += 20;
    else if (GET_SKILL(ch, SKILL_DAGGER) >= 50)
        chance += 10;

    ch->setBaseStat<int>("backstab_cooldown", 10);

    if (chance >= roll_to_beat)
    {
        int attacker_roll =
            GET_SKILL(ch, SKILL_MOVE_SILENTLY) + GET_SKILL(ch, SKILL_SPOT) + GET_DEX(ch) + Random::get<int>(-5, 5);
        int defender_roll =
            GET_SKILL(vict, SKILL_SPOT) + GET_SKILL(vict, SKILL_LISTEN) + GET_DEX(ch) + Random::get<int>(-5, 5);

        if (attacker_roll > defender_roll)
        {
            act("@RYou manage to sneak behind @r$N@R and stab $M in the back!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@RYou feel @r$n's@R dagger thrust into your back unexpectantly!@n", true, ch, nullptr, vict, TO_VICT);
            act("@r$n@R sneaks up behind @r$N@R and stabs $M in the back!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg += dmg * bonus;
            hurt(0, 0, ch, vict, nullptr, dmg, 0);
            return (1);
        }
        else
        {
            return (0);
        }
    }
    else
    {
        return (0);
    }
}

void cut_limb(Character *ch, Character *vict, int wlvl, int hitspot)
{

    int chance = 0, decap = 0, decapitate = false;
    int roll_to_beat = Random::get<int>(1, 10000);

    if (wlvl == 1)
    {
        chance = 25;
    }
    else if (wlvl == 2)
    {
        chance = 50;
    }
    else if (wlvl == 3)
    {
        chance = 100;
        decap = 5;
    }
    else if (wlvl == 4)
    {
        chance = 200;
        decap = 10;
    }
    else if (wlvl == 5)
    {
        chance = 200;
        decap = 50;
    }

    if (GET_SKILL(ch, SKILL_SWORD) >= 100)
    {
        chance += 100;
    }
    else if (GET_SKILL(ch, SKILL_SWORD) >= 50)
    {
        chance += 50;
    }

    if (decap >= roll_to_beat && hitspot == 4)
    {
        decapitate = true;
    }
    else if (chance < roll_to_beat)
    {
        return;
    }

    if (GET_HIT(vict) <= 1)
    {
        return;
    }

    if (decapitate == true)
    {
        act("@R$N's@r head is cut off in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@RYOUR head is cut off in the attack!@n", true, ch, nullptr, vict, TO_VICT);
        act("@R$N's@rhead is cut off in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);

        remove_limb(vict, 0);
        die(vict, ch);
        if (AFF_FLAGGED(ch, AFF_GROUP))
        {
            group_gain(ch, vict);
        }
        else
        {
            solo_gain(ch, vict);
        }
        char corp[256];
        if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
        {
            sprintf(corp, "all.zenni corpse");
            do_get(ch, corp, 0, 0);
        }
        if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
        {
            sprintf(corp, "all corpse");
            do_get(ch, corp, 0, 0);
        }
        return;
    }
    else
    { /* We've only succeeded in removing a limb. */
        if (HAS_ARMS(vict) && Random::get<int>(1, 2) == 2)
        {
            if (GET_LIMBCOND(vict, 1) > 0)
            {
                GET_LIMBCOND(vict, 1) = 0;
                vict->character_flags.set(CharacterFlag::cyber_left_arm, false);
                act("@R$N@r loses $s left arm!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYOU lose your left arm!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r loses $s left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                remove_limb(vict, 2);
            }
            else if (GET_LIMBCOND(vict, 0) > 0)
            {
                GET_LIMBCOND(vict, 0) = 100;
                vict->character_flags.set(CharacterFlag::cyber_right_arm, false);
                act("@R$N@r loses $s right arm!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYOU lose your right arm!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r loses $s right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                remove_limb(vict, 1);
            }
        }
        else
        { /* It's a leg */
            if (GET_LIMBCOND(vict, 3) > 0)
            {
                GET_LIMBCOND(vict, 3) = 100;
                vict->character_flags.set(CharacterFlag::cyber_left_leg, false);
                act("@R$N@r loses $s left leg!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYOU lose your left leg!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r loses $s left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                remove_limb(vict, 4);
            }
            else if (GET_LIMBCOND(vict, 2) > 0)
            {
                GET_LIMBCOND(vict, 2) = 100;
                vict->character_flags.set(CharacterFlag::cyber_right_leg, false);
                act("@R$N@r loses $s right leg!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@RYOU lose your right leg!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r loses $s right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                remove_limb(vict, 3);
            }
        }
    }
}

int count_physical(Character *ch)
{
    int count = 0;

    if (GET_SKILL(ch, SKILL_PUNCH) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_KICK) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_KNEE) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_ELBOW) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_SLAM) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_UPPERCUT) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_TAILWHIP) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_BASH) >= 1)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_HEADBUTT) >= 1)
    {
        count += 1;
    }

    return (count);
}

int physical_mastery(Character *ch)
{

    int count = 22;

    if (GET_SKILL(ch, SKILL_PUNCH) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_KICK) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_KNEE) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_ELBOW) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_SLAM) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_UPPERCUT) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_TAILWHIP) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_BASH) >= 100)
    {
        count += 1;
    }
    if (GET_SKILL(ch, SKILL_HEADBUTT) >= 100)
    {
        count += 1;
    }

    if (count == 26)
        count += 1;
    else if (count >= 27)
        count += 2;

    return (count);
}

int64_t advanced_energy(Character *ch, int64_t dmg)
{

    if (ch == nullptr)
    {
        return (false);
    }

    double rate = 0.00;
    int count = GET_LEVEL(ch);
    int64_t add = 0;

    if (GET_BONUS(ch, BONUS_LEECH))
    {
        rate = (double)(count) * 0.2;

        if (rate > 0.00)
        {
            add = dmg * rate;
            if (GET_CHARGE(ch) + add > GET_MAX_MANA(ch))
            {
                if (GET_CHARGE(ch) < GET_MAX_MANA(ch))
                {
                    ch->setBaseStat<int64_t>("charge", GET_MAX_MANA(ch));
                    act("@MYou leech some of the energy away!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@m$n@M leeches some of the energy away!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
                else
                {
                    ch->sendText("@MYou can't leech because there is too much charged energy for you to handle!@n\r\n");
                }
            }
            else
            {
                ch->modBaseStat<int64_t>("charge", add);
                act("@MYou leech some of the energy away!@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@m$n@M leeches some of the energy away!@n", true, ch, nullptr, nullptr, TO_ROOM);
            }

        } /* End of rate check */

    } /* End of Leech Bonus */

    if (GET_BONUS(ch, BONUS_INTOLERANT))
    {
        rate = (double)(count) * 0.2;

        if (rate > 0.00)
        {
            if (GET_CHARGE(ch) > 0 && Random::get<int>(1, 100) <= 10)
            {
                act("@MThe attack causes your weak control to slip and you are shocked by your own charged energy!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@m$n@M suffers shock from their own charged energy!@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->modCurVital(CharVital::health, -(GET_CHARGE(ch) / 4));
            }
            add = dmg * rate;
        }
    } /* End of Energy Intolerant bonus */

    return (add);
} /* End of advanced_energy function */

int roll_accuracy(Character *ch, int skill, bool kiatt)
{
    if (!IS_NPC(ch))
    {
        if (GET_BONUS(ch, BONUS_ACCURATE))
        {
            if (kiatt == true)
                skill += skill * 0.10;
            else
                skill += skill * 0.20;
        }
        else if (GET_BONUS(ch, BONUS_POORDEPTH))
        {
            if (kiatt == true)
                skill -= skill * 0.10;
            else
                skill -= skill * 0.20;
        }
    }

    if (skill < 40)
    {
        skill += Random::get<int>(3, 10);
    }

    return (skill);
}

long double calc_critical(Character *ch, int loc)
{

    int roll = Random::get<int>(1, 100);
    long double multi = 1;

    if (loc == 0)
    { /* Head */
        if (GET_BONUS(ch, BONUS_POWERHIT))
        {
            if (roll <= 15)
            {
                multi = 4;
            }
            else if (GET_BONUS(ch, BONUS_SOFT))
            {
                multi = 1;
            }
            else
            {
                multi = 2;
            }
        }
        else if (GET_BONUS(ch, BONUS_SOFT))
        {
            multi = 1;
        }
        else
        {
            multi = 2;
        }
    }
    else if (loc == 1)
    { /* Limb */
        if (GET_BONUS(ch, BONUS_SOFT))
        {
            multi = 0.25;
        }
        else
        {
            multi = 0.5;
        }
    }
    else
    { /* Body*/
        if (GET_BONUS(ch, BONUS_SOFT))
        {
            multi = 0.5;
        }
    }

    return (multi);
}

int roll_hitloc(Character *ch, Character *vict, int skill)
{

    int location = 4, critmax = 1000;
    int critical = 0;

    if (IS_NPC(ch))
    {
        if (GET_LEVEL(ch) > 100)
            skill = Random::get<int>(GET_LEVEL(ch), GET_LEVEL(ch) + 10);
        else
            skill = Random::get<int>(GET_LEVEL(ch), 100);
    }

    if (IS_DABURA(ch) && !IS_NPC(ch))
    {
        if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
            critmax -= 200;
    }

    critical = Random::get<int>(80, critmax);

    if (skill >= critical)
    {
        location = 2;
    }
    else if (skill >= Random::get<int>(50, 750))
    {
        location = 2;
    }
    else if (skill >= Random::get<int>(50, 350))
    {
        location = 1;
    }
    else if (skill >= Random::get<int>(30, 200))
    {
        location = 3;
    }
    else
    {
        location = Random::get<int>(4, 5);
    }

    if (location == 4 && GET_LIMBCOND(vict, 0) <= 0 && GET_LIMBCOND(vict, 1) <= 0)
    { /* No arms */
        location = 5;
    }

    if (location == 5 && GET_LIMBCOND(vict, 2) <= 0 && GET_LIMBCOND(vict, 3) <= 0)
    { /* No legs */
        location = 4;
    }

    if (location == 4 && GET_LIMBCOND(vict, 0) <= 0 && GET_LIMBCOND(vict, 1) <= 0)
    { /* Both failed, make body */
        location = 1;
    }

    return (location);
}

int64_t armor_calc(Character *ch, int64_t dmg, int type)
{
    if (IS_NPC(ch))
        return (70);

    int64_t reduce = 0;

    if (GET_ARMOR(ch) < 5000)
    {
        reduce = GET_ARMOR(ch);
    }
    else if (GET_ARMOR(ch) < 10000)
    {
        reduce = GET_ARMOR(ch) * 2;
    }
    else if (GET_ARMOR(ch) < 20000)
    {
        reduce = GET_ARMOR(ch) * 4;
    }
    else if (GET_ARMOR(ch) < 30000)
    {
        reduce = GET_ARMOR(ch) * 8;
    }
    else if (GET_ARMOR(ch) < 40000)
    {
        reduce = GET_ARMOR(ch) * 12;
    }
    else if (GET_ARMOR(ch) < 60000)
    {
        reduce = GET_ARMOR(ch) * 25;
    }
    else if (GET_ARMOR(ch) < 100000)
    {
        reduce = GET_ARMOR(ch) * 50;
    }
    else if (GET_ARMOR(ch) < 150000)
    {
        reduce = GET_ARMOR(ch) * 75;
    }
    else if (GET_ARMOR(ch) < 200000)
    {
        reduce = GET_ARMOR(ch) * 150;
    }
    else if (GET_ARMOR(ch) >= 200000)
    {
        reduce = GET_ARMOR(ch) * 250;
    }

    /* loc: 0 = Physical Bonus, 1 = Ki Bonus, 2 = Bonus To Both */
    int i, loc = -1;
    double bonus = 0.0;

    for (i = 0; i < NUM_WEARS; i++)
    {
        if (GET_EQ(ch, i))
        {
            Object *obj = GET_EQ(ch, i);
            switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL))
            {
            case MATERIAL_STEEL:
                loc = 0;
                bonus = 0.05;
                break;
            case MATERIAL_IRON:
                loc = 0;
                bonus = 0.025;
                break;
            case MATERIAL_COPPER:
            case MATERIAL_BRASS:
            case MATERIAL_METAL:
                loc = 0;
                bonus = 0.01;
                break;
            case MATERIAL_SILVER:
                loc = 1;
                bonus = 0.1;
                break;
            case MATERIAL_KACHIN:
                loc = 2;
                bonus = 0.2;
                break;
            case MATERIAL_CRYSTAL:
                loc = 1;
                bonus = 0.05;
                break;
            case MATERIAL_DIAMOND:
                loc = 2;
                bonus = 0.05;
                break;
            case MATERIAL_PAPER:
            case MATERIAL_COTTON:
            case MATERIAL_SATIN:
            case MATERIAL_SILK:
            case MATERIAL_BURLAP:
            case MATERIAL_VELVET:
            case MATERIAL_HEMP:
            case MATERIAL_WAX:
                loc = 2;
                bonus = -0.05;
                break;
            default:
                break;
            }
        }
    }

    if (bonus > 0.95)
        bonus = 0.95;

    if (loc != -1)
    {
        switch (type)
        {
        case 0:
            if (loc == 0 || loc == 2)
            {
                reduce += reduce * bonus;
            }
            break;
        case 1:
            if (loc == 1 || loc == 2)
            {
                reduce += reduce * bonus;
                reduce /= 2;
            }
            break;
        }
    }

    return (reduce);
}

/* For calculating the difficulty the player has to hit with the skill. */
int chance_to_hit(Character *ch)
{

    int num = axion_dice(0);

    if (IS_NPC(ch))
        return (num);

    if (GET_COND(ch, DRUNK) > 4)
    {
        num += GET_COND(ch, DRUNK);
    }

    if (ch->location.getIsDark() && !CAN_SEE_IN_DARK(ch))
        num += 30;

    return (num);
}

/* For calculating the speed of the attacker and defender */
int handle_speed(Character *ch, Character *vict)
{

    if (ch == nullptr || vict == nullptr)
    { /* Ruh roh*/
        return (0);
    }

    if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 4)
    {
        return (15);
    }
    else if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 2)
    {
        return (10);
    }
    else if (GET_SPEEDI(ch) > GET_SPEEDI(vict))
    {
        return (5);
    }
    else if (GET_SPEEDI(ch) * 4 < GET_SPEEDI(vict))
    {
        return (-15);
    }
    else if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict))
    {
        return (-10);
    }
    else if (GET_SPEEDI(ch) < GET_SPEEDI(vict))
    {
        return (-5);
    }

    return (0);
}

/* For Destroying or Breaking Limbs */
void hurt_limb(Character *ch, Character *vict, int chance, int area, int64_t power)
{
    if (!vict || IS_NPC(vict))
    {
        return;
    }

    int dmg = 0;

    if (chance > axion_dice(100))
    {
        return;
    }

    if (power > (vict->getEffectiveStat<int64_t>("health")) * 0.5)
    {
        dmg = Random::get<int>(25, 40);
    }
    else if (power > (vict->getEffectiveStat<int64_t>("health")) * 0.25)
    {
        dmg = Random::get<int>(15, 24);
    }
    else if (power > (vict->getEffectiveStat<int64_t>("health")) * 0.10)
    {
        dmg = Random::get<int>(8, 14);
    }
    else if (power > (vict->getEffectiveStat<int64_t>("health")) * 0.05)
    {
        dmg = Random::get<int>(4, 7);
    }
    else
    {
        dmg = Random::get<int>(1, 3);
        ;
    }

    if (GET_ARMOR(vict) > 50000)
    {
        dmg -= 5;
    }
    else if (GET_ARMOR(vict) > 40000)
    {
        dmg -= 4;
    }
    else if (GET_ARMOR(vict) > 30000)
    {
        dmg -= 3;
    }
    else if (GET_ARMOR(vict) > 20000)
    {
        dmg -= 2;
    }
    else if (GET_ARMOR(vict) > 10000)
    {
        dmg -= 1;
    }
    else if (GET_ARMOR(vict) > 5000)
    {
        dmg -= Random::get<int>(0, 1);
    }

    if (dmg <= 0)
    {
        return;
    }

    if (!ch->isSparring())
    {
        if (area == 0)
        { /* Arms */
            if (GET_LIMBCOND(vict, 1) - dmg <= 0)
            {
                act("@RYour attack @YDESTROYS @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack @YDESTROYS@R YOUR left arm!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack @YDESTROYS @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                GET_LIMBCOND(vict, 1) = 0;
                for (auto f : {PLR_THANDW})
                    vict->player_flags.set(f, false);
                remove_limb(vict, 2);
            }
            else if (GET_LIMBCOND(vict, 1) > 0)
            {
                GET_LIMBCOND(vict, 1) -= dmg;
                act("@RYour attack hurts @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack hurts YOUR left arm!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack hurts @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            }
            else if (GET_LIMBCOND(vict, 0) - dmg <= 0)
            {
                act("@RYour attack @YDESTROYS @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack @YDESTROYS@R YOUR right arm!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack @YDESTROYS @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
                GET_LIMBCOND(vict, 0) = 0;
                for (auto f : {PLR_THANDW})
                    vict->player_flags.set(f, false);
                remove_limb(vict, 2);
            }
            else if (GET_LIMBCOND(vict, 0) > 0)
            {
                GET_LIMBCOND(vict, 0) -= dmg;
                act("@RYour attack hurts @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack hurts YOUR right arm!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack hurts @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            }
        }
        else if (area == 1)
        { /* Legs */
            if (GET_LIMBCOND(vict, 3) - dmg <= 0)
            {
                act("@RYour attack @YDESTROYS @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack @YDESTROYS@R YOUR left leg!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack @YDESTROYS @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                GET_LIMBCOND(vict, 3) = 0;
                for (auto f : {PLR_THANDW})
                    vict->player_flags.set(f, false);
                remove_limb(vict, 2);
            }
            else if (GET_LIMBCOND(vict, 3) > 0)
            {
                GET_LIMBCOND(vict, 3) -= dmg;
                act("@RYour attack hurts @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack hurts YOUR left leg!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack hurts @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            }
            else if (GET_LIMBCOND(vict, 2) - dmg <= 0)
            {
                act("@RYour attack @YDESTROYS @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack @YDESTROYS@R YOUR right leg!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack @YDESTROYS @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
                GET_LIMBCOND(vict, 2) = 0;
                for (auto f : {PLR_THANDW})
                    vict->player_flags.set(f, false);
                remove_limb(vict, 2);
            }
            else if (GET_LIMBCOND(vict, 2) > 0)
            {
                GET_LIMBCOND(vict, 2) -= dmg;
                act("@RYour attack hurts @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@r$n's@R attack hurts YOUR right leg!@n", true, ch, nullptr, vict, TO_VICT);
                act("@r$n's@R attack hurts @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            }
        }
    }
}

/* For damaging equipment when hit */
void dam_eq_loc(Character *vict, int area)
{
    int location = 0, num = 0;
    /* Area is 4 possible hit locations in an attack.
    1 Arms, 2 legs, 3 head, and 4 body. */

    if (!vict || vict == nullptr || GET_HIT(vict) <= 0)
    {
        return;
    }

    switch (area)
    {
    case 1:
        num = Random::get<int>(1, 8);
        switch (num)
        {
        case 1:
            location = WEAR_FINGER_R;
            break;
        case 2:
            location = WEAR_FINGER_L;
            break;
        case 3:
            location = WEAR_ARMS;
            break;
        case 4:
            location = WEAR_WRIST_R;
            break;
        case 5:
            location = WEAR_WRIST_L;
            break;
        case 6:
        case 7:
        case 8:
            location = WEAR_HANDS;
            break;
        }
        break;
    case 2:
        num = Random::get<int>(1, 3);
        switch (num)
        {
        case 1:
            location = WEAR_LEGS;
            break;
        case 2:
            location = WEAR_FEET;
            break;
        case 3:
            location = WEAR_WAIST;
            break;
        }
        break;
    case 3:
        num = Random::get<int>(1, 6);
        switch (num)
        {
        case 1:
            location = WEAR_HEAD;
            break;
        case 2:
            location = WEAR_NECK_1;
            break;
        case 3:
            location = WEAR_NECK_2;
            break;
        case 4:
            location = WEAR_EAR_R;
            break;
        case 5:
            location = WEAR_EAR_L;
            break;
        case 6:
            location = WEAR_EYE;
            break;
        }
        break;
    case 4:
        num = Random::get<int>(1, 4);
        switch (num)
        {
        case 1:
            location = WEAR_BODY;
            break;
        case 2:
            location = WEAR_ABOUT;
            break;
        case 3:
            location = WEAR_BACKPACK;
            break;
        case 4:
            location = WEAR_SH;
            break;
        }
        break;
    default:
        location = WEAR_BODY;
        break;
    }
    damage_eq(vict, location);
}

void damage_eq(Character *vict, int location)
{

    if (GET_EQ(vict, location) && Random::get<int>(1, 20) >= 19 && !AFF_FLAGGED(vict, AFF_SANCTUARY))
    {
        Object *eq = GET_EQ(vict, location);
        if (OBJ_FLAGGED(eq, ITEM_UNBREAKABLE))
        {
            return;
        }

        int loss = Random::get<int>(2, 5);

        if (GET_OBJ_VNUM(eq) == 20099 || GET_OBJ_VNUM(eq) == 20098)
            loss = 1;

        if (AFF_FLAGGED(vict, AFF_CURSE))
        {
            loss *= 3;
        }
        else if (AFF_FLAGGED(vict, AFF_BLESS) && Random::get<int>(1, 3) == 3)
        {
            loss = 1;
        }
        else if (AFF_FLAGGED(vict, AFF_BLESS))
        {
            return;
        }

        ;
        if (MOD_OBJ_VAL(eq, VAL_ALL_HEALTH, -loss) <= 0)
        {
            SET_OBJ_VAL(eq, VAL_ALL_HEALTH, 0);
            eq->item_flags.set(ITEM_BROKEN, true);
            act("@WYour $p@W completely breaks!@n", false, nullptr, eq, vict, TO_VICT);
            act("@C$N's@W $p@W completely breaks!@n", false, nullptr, eq, vict, TO_NOTVICT);
            perform_remove(vict, location);
        }
        else if (GET_OBJ_VAL(eq, VAL_ALL_MATERIAL) == MATERIAL_LEATHER ||
                 GET_OBJ_VAL(eq, VAL_ALL_MATERIAL) == MATERIAL_COTTON ||
                 GET_OBJ_VAL(eq, VAL_ALL_MATERIAL) == MATERIAL_SILK)
        {
            act("@WYour $p@W rips a little!@n", false, nullptr, eq, vict, TO_VICT);
            act("@C$N's@W $p@W rips a little!@n", false, nullptr, eq, vict, TO_NOTVICT);
            if (AFF_FLAGGED(vict, AFF_BLESS))
            {
                vict->sendText("@c...But your blessing seems to have partly mended this damage.@n\r\n");
                act("@c...but @C$N's@c body glows blue for a moment and the damage mends a little.@n", true, nullptr,
                    nullptr, vict, TO_NOTVICT);
            }
            else if (AFF_FLAGGED(vict, AFF_CURSE))
            {
                vict->sendText("@r...and your curse seems to have made the damage three times worse!@n\r\n");
                act("@c...but @C$N's@c body glows red for a moment and the damage grow three times worse!@n", true,
                    nullptr, nullptr, vict, TO_NOTVICT);
            }
        }
        else
        {
            act("@WYour $p@W cracks a little!@n", false, nullptr, eq, vict, TO_VICT);
            act("@C$N's@W $p@W cracks a little!@n", false, nullptr, eq, vict, TO_NOTVICT);
            if (AFF_FLAGGED(vict, AFF_BLESS))
            {
                vict->sendText("@c...But your blessing seems to have partly mended this damage.@n\r\n");
                act("@c...but @C$N's@c body glows blue for a moment and the damage mends a little.@n", true, nullptr,
                    nullptr, vict, TO_NOTVICT);
            }
            else if (AFF_FLAGGED(vict, AFF_CURSE))
            {
                vict->sendText("@r...and your curse seems to have made the damage three times worse!@n\r\n");
                act("@c...but @C$N's@c body glows red for a moment and the damage grow three times worse!@n", true,
                    nullptr, nullptr, vict, TO_NOTVICT);
            }
        }
    }
}

/* This is for huge attacks that are slowly descending on a room */
void huge_update(uint64_t heartPulse, double deltaTime)
{
    int dge = 0, skill = 0, bonus = 1, count = 0;
    int64_t dmg = 0;
    Character *ch, *vict, *next_v;

    /* Checking the object list for any huge ki attacks */
    auto subs = objectSubscriptions.all("hugeKiAttacks");
    for (auto k : filter_raw(subs))
    {

        if (GET_AUCTER(k) > 0 && GET_AUCTIME(k) + 604800 <= time(nullptr))
        {
            if (IN_ROOM(k) && k->location.getVnum() == 80)
            {
                room_vnum inroom = IN_ROOM(k);
                extract_obj(k);
                continue;
            }
        }
        if (KICHARGE(k) <= 0)
        {
            continue;
        }
        if (GET_OBJ_VNUM(k) != 82 && GET_OBJ_VNUM(k) != 83)
        {
            continue;
        }
        else if (KIDIST(k) <= 0)
        {
            /* Genki Dama Section */
            if (KITYPE(k) == 497)
            {
                if (IN_ROOM(TARGET(k)) == IN_ROOM(k))
                {
                    ch = USER(k);
                    if (ch->location.getVnum() == k->location.getVnum())
                    {
                        bonus = 2;
                    }

                    act("@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on YOU! It eclipses everything above you as it crushes down into you! You struggle against it with all your might!@n",
                        true, TARGET(k), nullptr, nullptr, TO_CHAR);
                    act("@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on @C$n@W! It completely obscures $m from view as it crushes into $s body! It appears to be facing some resistance from $m!@n",
                        true, TARGET(k), nullptr, nullptr, TO_ROOM);
                    k->location.sendText("\r\n");
                    if (GET_HIT(TARGET(k)) * bonus < KICHARGE(k) * 5)
                    {

                        act("@WYour strength is no match for the power of the attack! It slowly grinds into you before exploding into a massive blast!@n",
                            true, TARGET(k), nullptr, nullptr, TO_CHAR);
                        act("@C$n@W's strength is no match for the power of the attack! It slowly grinds into $m before exploding into a massive blast!@n",
                            true, TARGET(k), nullptr, nullptr, TO_ROOM);
                        skill = init_skill(ch, SKILL_GENKIDAMA); /* Set skill value */
                        dmg = KICHARGE(k) * 1.25;
                        hurt(0, 0, ch, TARGET(k), nullptr, dmg, 1);
                        dmg /= 2;

                        /* Hit those in the current room. */
                        auto people = k->location.getPeople();
                        for (auto it : filter_raw(people))
                        {
                            vict = it;

                            if (vict == ch)
                            {
                                continue;
                            }
                            if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict))
                            {
                                continue;
                            }
                            if (vict == TARGET(k))
                            {
                                continue;
                            }
                            if (AFF_FLAGGED(vict, AFF_GROUP))
                            {
                                if (vict->master == ch)
                                {
                                    continue;
                                }
                                else if (ch->master == vict)
                                {
                                    continue;
                                }
                                else if (vict->master == ch->master)
                                {
                                    continue;
                                }
                            }
                            if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict))
                            {
                                continue;
                            }
                            if (MOB_FLAGGED(vict, MOB_NOKILL))
                            {
                                continue;
                            }
                            dge = handle_dodge(vict);
                            if (((!IS_NPC(vict) && IS_ICER(vict) && Random::get<int>(1, 30) >= 28) ||
                                 AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                                (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
                            {
                                act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
                                act("@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
                                act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict,
                                    TO_NOTVICT);
                                vict->affect_flags.set(AFF_ZANZOKEN, false);
                                pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                                hurt(0, 0, ch, vict, nullptr, 0, 1);
                                continue;
                            }
                            else if (dge + Random::get<int>(-10, 5) > skill)
                            {
                                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                                act("@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                                hurt(0, 0, ch, vict, nullptr, 0, 1);
                                improve_skill(vict, SKILL_DODGE, 0);
                                continue;
                            }
                            else
                            {
                                count += 1;
                                if (IS_NPC(vict) && count > 10)
                                {
                                    if (GET_HIT(vict) < dmg)
                                    {
                                        double loss = 0.0;

                                        if (count >= 30)
                                        {
                                            loss = 0.80;
                                        }
                                        else if (count >= 20)
                                        {
                                            loss = 0.6;
                                        }
                                        else if (count >= 15)
                                        {
                                            loss = 0.4;
                                        }
                                        else if (count >= 10)
                                        {
                                            loss = 0.25;
                                        }
                                        vict->modExperience(-(GET_EXP(vict) * loss));
                                    }
                                }
                                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                                act("@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                                continue;
                            }
                        }
                        k->location.setDamage(100);
                        if (auto zone = ch->location.getZone(); zone)
                        {
                            zone->sendText("A MASSIVE explosion shakes the entire area!\r\n");
                        }

                        extract_obj(k);
                        continue;
                    } /* It isn't stopped! */
                    else
                    {
                        act("@WYou manage to overpower the attack! You lift up into the sky slowly with it and toss it up and away out of sight!@n",
                            true, TARGET(k), nullptr, nullptr, TO_CHAR);
                        act("@C$n@W manages to unbelievably overpower the attack! It is lifted up into the sky and tossed away dramaticly!@n",
                            true, TARGET(k), nullptr, nullptr, TO_ROOM);
                        hurt(0, 0, ch, TARGET(k), nullptr, 0, 1);
                        TARGET(k)->modCurVital(CharVital::stamina, -(KICHARGE(k) / 4));
                        extract_obj(k);
                        continue;
                    }
                }
                else if (IN_ROOM(TARGET(k)) != IN_ROOM(k))
                {
                    ch = USER(k);

                    k->location.sendText("@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on the area! It slowly burns into the ground before exploding magnificently!@n\r\n");

                    skill = init_skill(ch, SKILL_GENKIDAMA); /* Set skill value */
                    dmg = KICHARGE(k);
                    dmg /= 2;

                    /* Hit those in the current room. */
                    auto people = k->location.getPeople();
                    for (auto it : filter_raw(people))
                    {
                        vict = it;

                        if (vict == ch)
                        {
                            continue;
                        }
                        if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict))
                        {
                            continue;
                        }
                        if (AFF_FLAGGED(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict))
                        {
                            continue;
                        }
                        if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict))
                        {
                            continue;
                        }
                        if (MOB_FLAGGED(vict, MOB_NOKILL))
                        {
                            continue;
                        }
                        dge = handle_dodge(vict);
                        if (((!IS_NPC(vict) && IS_ICER(vict) && Random::get<int>(1, 30) >= 28) ||
                             AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                            (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
                        {
                            act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
                            act("@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
                            act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_NOTVICT);
                            vict->affect_flags.set(AFF_ZANZOKEN, false);
                            pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                            hurt(0, 0, ch, vict, nullptr, 0, 1);
                            continue;
                        }
                        else if (dge + Random::get<int>(-10, 5) > skill)
                        {
                            act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, 0, 1);
                            improve_skill(vict, SKILL_DODGE, 0);
                            continue;
                        }
                        else
                        {
                            act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 1);
                            continue;
                        }
                    }
                    k->location.setDamage(100);
                    if (auto zone = ch->location.getZone(); zone)
                    {
                        zone->sendText("An explosion shakes the entire area!\r\n");
                    }
                    extract_obj(k);
                    continue;
                } /* End Genki */
                continue;
            }
            /* Genocide Section */
            if (KITYPE(k) == 498)
            {
                if (IN_ROOM(TARGET(k)) == IN_ROOM(k))
                {
                    ch = USER(k);
                    if (ch->location.getVnum() == k->location.getVnum())
                    {
                        bonus = 2;
                    }

                    act("@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on YOU! It eclipses everything above you as it crushes down into you! You struggle against it with all your might!@n",
                        true, TARGET(k), nullptr, nullptr, TO_CHAR);
                    act("@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on @C$n@W! It completely obscures $m from view as it crushes into $s body! It appears to be facing some resistance from $m!@n",
                        true, TARGET(k), nullptr, nullptr, TO_ROOM);
                    k->location.sendText("\r\n");
                    if (GET_HIT(TARGET(k)) * bonus < KICHARGE(k) * 10)
                    {

                        act("@WYour strength is no match for the power of the attack! It slowly grinds into you before exploding into a massive blast!@n",
                            true, TARGET(k), nullptr, nullptr, TO_CHAR);
                        act("@C$n@W's strength is no match for the power of the attack! It slowly grinds into $m before exploding into a massive blast!@n",
                            true, TARGET(k), nullptr, nullptr, TO_ROOM);
                        skill = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
                        dmg = KICHARGE(k);
                        hurt(0, 0, ch, TARGET(k), nullptr, dmg, 1);
                        dmg /= 2;

                        /* Hit those in the current room. */
                        auto people = k->location.getPeople();
                        for (auto it : filter_raw(people))
                        {
                            vict = it;

                            if (vict == ch)
                            {
                                continue;
                            }
                            if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict))
                            {
                                continue;
                            }
                            if (vict == TARGET(k))
                            {
                                continue;
                            }
                            if (AFF_FLAGGED(vict, AFF_GROUP))
                            {
                                if (vict->master == ch)
                                {
                                    continue;
                                }
                                else if (ch->master == vict)
                                {
                                    continue;
                                }
                                else if (vict->master == ch->master)
                                {
                                    continue;
                                }
                            }
                            if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict))
                            {
                                continue;
                            }
                            if (MOB_FLAGGED(vict, MOB_NOKILL))
                            {
                                continue;
                            }
                            dge = handle_dodge(vict);
                            if (((!IS_NPC(vict) && IS_ICER(vict) && Random::get<int>(1, 30) >= 28) ||
                                 AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                                (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
                            {
                                act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
                                act("@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
                                act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict,
                                    TO_NOTVICT);
                                vict->affect_flags.set(AFF_ZANZOKEN, false);
                                pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                                continue;
                            }
                            else if (dge + Random::get<int>(-10, 5) > skill)
                            {
                                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                                act("@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                                act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                                hurt(0, 0, ch, vict, nullptr, 0, 1);
                                improve_skill(vict, SKILL_DODGE, 0);
                                continue;
                            }
                            else
                            {
                                count += 1;
                                if (IS_NPC(vict) && count > 10)
                                {
                                    if (GET_HIT(vict) < dmg)
                                    {
                                        double loss = 0.0;

                                        if (count >= 30)
                                        {
                                            loss = 0.80;
                                        }
                                        else if (count >= 20)
                                        {
                                            loss = 0.6;
                                        }
                                        else if (count >= 15)
                                        {
                                            loss = 0.4;
                                        }
                                        else if (count >= 10)
                                        {
                                            loss = 0.25;
                                        }
                                        vict->modExperience(-(GET_EXP(vict) * loss));
                                    }
                                }
                                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                                act("@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                                act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                                continue;
                            }
                        }
                        k->location.setDamage(100);
                        if (auto zone = ch->location.getZone(); zone)
                        {
                            zone->sendText("An explosion shakes the entire area!\r\n");
                        }
                        extract_obj(k);
                        continue;
                    } /* It isn't stopped! */
                    else
                    {
                        act("@WYou manage to overpower the attack! You lift up into the sky slowly with it and toss it up and away out of sight!@n",
                            true, TARGET(k), nullptr, nullptr, TO_CHAR);
                        act("@C$n@W manages to unbelievably overpower the attack! It is lifted up into the sky and tossed away dramaticly!@n",
                            true, TARGET(k), nullptr, nullptr, TO_ROOM);
                        hurt(0, 0, ch, TARGET(k), nullptr, 0, 1);
                        TARGET(k)->modCurVital(CharVital::stamina, -(KICHARGE(k) / 4));
                        extract_obj(k);
                        continue;
                    }
                }
                else if (IN_ROOM(TARGET(k)) != IN_ROOM(k))
                {
                    ch = USER(k);

                    k->location.sendText("@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on the area! It slowly burns into the ground before exploding magnificantly!@n\r\n");

                    skill = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
                    dmg = KICHARGE(k);
                    dmg /= 2;

                    /* Hit those in the current room. */
                    auto people = k->location.getPeople();
                    for (auto it : filter_raw(people))
                    {
                        vict = it;

                        if (vict == ch)
                        {
                            continue;
                        }
                        if (AFF_FLAGGED(vict, AFF_SPIRIT) && !IS_NPC(vict))
                        {
                            continue;
                        }
                        if (AFF_FLAGGED(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict))
                        {
                            continue;
                        }
                        if (GET_LEVEL(vict) <= 8 && !IS_NPC(vict))
                        {
                            continue;
                        }
                        if (MOB_FLAGGED(vict, MOB_NOKILL))
                        {
                            continue;
                        }
                        dge = handle_dodge(vict);
                        if (((!IS_NPC(vict) && IS_ICER(vict) && Random::get<int>(1, 30) >= 28) ||
                             AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
                            (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
                        {
                            act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
                            act("@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
                            act("@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_NOTVICT);
                            vict->affect_flags.set(AFF_ZANZOKEN, false);
                            pcost(vict, 0, GET_MAX_HIT(vict) / 200);
                            continue;
                        }
                        else if (dge + Random::get<int>(-10, 5) > skill)
                        {
                            act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, 0, 1);
                            improve_skill(vict, SKILL_DODGE, 0);
                            continue;
                        }
                        else
                        {
                            act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            hurt(0, 0, ch, vict, nullptr, dmg, 1);
                            continue;
                        }
                    }
                    k->location.setDamage(100);
                    if (auto zone = ch->location.getZone(); zone)
                    {
                        zone->sendText("An explosion shakes the entire area!\r\n");
                    }
                    extract_obj(k);
                    continue;
                } /* End Genocide */
                continue;
            }
            else
            {
                extract_obj(k);
                continue;
            }
        }
        act("$p@W descends slowly towards the ground!@n", true, nullptr, k, nullptr, TO_ROOM);
        k->modBaseStat("distance", -1);
    }
}
/* End huge ki attack update */

/* For handling homing attacks */
void homing_update(uint64_t heartPulse, double deltaTime)
{
    auto subs = objectSubscriptions.all("homingKiAttacks");
    for (auto k : filter_raw(subs))
    {

        if (KICHARGE(k) <= 0)
        {
            continue;
        }

        if (GET_OBJ_VNUM(k) != 80 && GET_OBJ_VNUM(k) != 81 && GET_OBJ_VNUM(k) != 84)
            continue;
        if (!(TARGET(k) && USER(k)))
            continue;

        Character *ch = USER(k);
        Character *vict = TARGET(k);

        if (GET_OBJ_VNUM(k) == 80)
        { // Tsuihidan
            if (KICHARGE(k) <= 0)
            {
                k->location.send_to("%s has lost all its energy and disappears.\r\n",
                                    k->getShortDescription());
                extract_obj(k);
                continue;
            }
            if (k->location != vict->location)
            {
                act("@wThe $p@w pursues after you!@n", true, vict, k, nullptr, TO_CHAR);
                act("@wThe $p@W pursues after @C$n@w!@n", true, vict, k, nullptr, TO_ROOM);
                k->clearLocation();
                k->moveToLocation(vict);
                continue;
            }
            else
            {
                act("@RThe $p@R makes a tight turn and rockets straight for you!@n", true, vict, k, nullptr,
                    TO_CHAR);
                act("@RThe $p@R makes a tight turn and rockets straight for @r$n@n", true, vict, k, nullptr,
                    TO_ROOM);
                if (handle_parry(vict) < Random::get<int>(1, 140))
                {
                    act("@rThe $p@r slams into your body, exploding in a flash of bright light!@n", true, vict, k,
                        nullptr, TO_CHAR);
                    act("@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", true, vict,
                        k, nullptr, TO_ROOM);
                    int64_t dmg = KICHARGE(k);
                    extract_obj(k);
                    hurt(0, 0, ch, vict, nullptr, dmg, 1);
                    continue;
                }
                else if (Random::get<int>(1, 3) > 1)
                {
                    act("@wYou manage to deflect the $p@W sending it flying away and depleting some of its energy.@n",
                        true, vict, k, nullptr, TO_CHAR);
                    act("@C$n @wmanages to deflect the $p@w sending it flying away and depleting some of its energy.@n",
                        true, vict, k, nullptr, TO_ROOM);
                    auto kic = KICHARGE(k);
                    if (k->modBaseStat("kicharge", -(kic * 0.1)) <= 0)
                    {
                        k->location.send_to("%s has lost all its energy and disappears.\r\n",
                                            k->getShortDescription());
                        extract_obj(k);
                    }
                    continue;
                }
                else
                {
                    act("@wYou manage to deflect the $p@w sending it flying away into the nearby surroundings!@n",
                        true, vict, k, nullptr, TO_CHAR);
                    act("@C$n @wmanages to deflect the $p@w sending it flying away into the nearby surroundings!@n",
                        true, vict, k, nullptr, TO_ROOM);
                    if (vict->location.getDamage() < 100)
                    {
                        vict->location.modDamage(5);
                    }
                    extract_obj(k);
                    continue;
                }
            }
            continue;
        }
        else if (GET_OBJ_VNUM(k) == 81 || GET_OBJ_VNUM(k) == 84)
        { // Spirit Ball
            if (k->location != vict->location)
            {
                act("@wYou lose sight of @C$N@W and let $p@W fly away!@n", true, ch, k, vict, TO_CHAR);
                act("@wYou manage to escape @C$n's@W $p@W!@n", true, ch, k, vict, TO_VICT);
                act("@C$n@W loses sight of @c$N@W and lets $s $p@W fly away!@n", true, ch, k, vict, TO_NOTVICT);
                extract_obj(k);
                continue;
            }
            else
            {
                act("@RYou move your hand and direct $p@R after @r$N@R!@n", true, ch, k, vict, TO_CHAR);
                act("@r$n@R moves $s hand and directs $p@R after YOU!@n", true, ch, k, vict, TO_VICT);
                act("@r$n@R moves $s hand and directs $p@R after @r$N@R!@n", true, ch, k, vict, TO_NOTVICT);
                if (handle_parry(vict) < Random::get<int>(1, 140))
                {
                    if (GET_OBJ_VNUM(k) != 84)
                    {
                        act("@rThe $p@r slams into your body, exploding in a flash of bright light!@n", true, vict,
                            k, nullptr, TO_CHAR);
                        act("@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", true,
                            vict, k, nullptr, TO_ROOM);
                        int64_t dmg = KICHARGE(k);
                        extract_obj(k);
                        hurt(0, 0, ch, vict, nullptr, dmg, 1);
                    }
                    else if (GET_OBJ_VNUM(k) == 84)
                    {
                        int64_t dmg = KICHARGE(k);
                        if (dmg > GET_MAX_HIT(vict) / 5 && (!IS_MAJIN(vict) && !IS_BIO(vict)))
                        {
                            act("@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                            act("@rYou are cut in half by the attack!@n", true, ch, nullptr, vict, TO_VICT);
                            act("@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                            die(vict, ch);
                            if (AFF_FLAGGED(ch, AFF_GROUP))
                            {
                                group_gain(ch, vict);
                            }
                            else
                            {
                                solo_gain(ch, vict);
                            }
                            if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
                            {
                                do_get(ch, "all.zenni corpse", 0, 0);
                            }
                            if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
                            {
                                do_get(ch, "all corpse", 0, 0);
                            }
                        }
                        else if (dmg > GET_MAX_HIT(vict) / 5 && (IS_MAJIN(vict) || IS_BIO(vict)))
                        {
                            if (GET_SKILL(vict, SKILL_REGENERATE) > Random::get<int>(1, 101) &&
                                (vict->getCurVital(CharVital::ki)) >= GET_MAX_MANA(vict) / 40)
                            {
                                act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", true,
                                    ch, nullptr, vict, TO_CHAR);
                                act("@rYou are cut in half by the attack but regenerate a moment later!@n", true,
                                    ch, nullptr, vict, TO_VICT);
                                act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", true,
                                    ch, nullptr, vict, TO_NOTVICT);
                                vict->modCurVital(CharVital::ki, -(vict->getEffectiveStat<int64_t>("ki") / 40));
                                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                            }
                            else if (dmg > GET_MAX_HIT(vict) / 5 && (IS_MAJIN(vict) || IS_BIO(vict)))
                            {
                                if (GET_SKILL(vict, SKILL_REGENERATE) > Random::get<int>(1, 101) &&
                                    (vict->getCurVital(CharVital::ki)) >= GET_MAX_MANA(vict) / 40)
                                {
                                    act("@R$N@r is cut in half by the attack but regenerates a moment later!@n",
                                        true, ch, nullptr, vict, TO_CHAR);
                                    act("@rYou are cut in half by the attack but regenerate a moment later!@n",
                                        true, ch, nullptr, vict, TO_VICT);
                                    act("@R$N@r is cut in half by the attack but regenerates a moment later!@n",
                                        true, ch, nullptr, vict, TO_NOTVICT);
                                    vict->modCurVital(CharVital::ki, -(vict->getEffectiveStat<int64_t>("ki") / 40));
                                    hurt(0, 0, ch, vict, nullptr, dmg, 1);
                                }
                                else
                                {
                                    act("@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                                    act("@rYou are cut in half by the attack!@n", true, ch, nullptr, vict, TO_VICT);
                                    act("@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict,
                                        TO_NOTVICT);
                                    die(vict, ch);
                                    if (AFF_FLAGGED(ch, AFF_GROUP))
                                    {
                                        group_gain(ch, vict);
                                    }
                                    else
                                    {
                                        solo_gain(ch, vict);
                                    }
                                    if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
                                    {
                                        do_get(ch, "all.zenni corpse", 0, 0);
                                    }
                                    if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
                                    {
                                        do_get(ch, "all corpse", 0, 0);
                                    }
                                }
                            }
                        }
                        else
                        {
                            act("@rThe $p@r slams into your body, exploding in a flash of bright light!@n", true,
                                vict, k, nullptr, TO_CHAR);
                            act("@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n",
                                true, vict, k, nullptr, TO_ROOM);
                            hurt(0, 0, ch, vict, nullptr, dmg, 1);
                        }
                        extract_obj(k);
                    }
                    continue;
                }
                else if (Random::get<int>(1, 3) > 1)
                {
                    act("@wYou manage to deflect the $p@W sending it flying away and depleting some of its energy.@n",
                        true, vict, k, nullptr, TO_CHAR);
                    act("@C$n @wmanages to deflect the $p@w sending it flying away and depleting some of its energy.@n",
                        true, vict, k, nullptr, TO_ROOM);
                    auto kic = KICHARGE(k);
                    if (k->modBaseStat("kicharge", -kic / 10) <= 0)
                    {
                        k->location.send_to("%s has lost all its energy and disappears.\r\n", k->getShortDescription());
                        extract_obj(k);
                    }
                    continue;
                }
                else
                {
                    act("@wYou manage to deflect the $p@w sending it flying away into the nearby surroundings!@n",
                        true, vict, k, nullptr, TO_CHAR);
                    act("@C$n @wmanages to deflect the $p@w sending it flying away into the nearby surroundings!@n",
                        true, vict, k, nullptr, TO_ROOM);
                    if (vict->location.getDamage() < 100)
                    {
                        vict->location.modDamage(5);
                    }
                    extract_obj(k);
                    continue;
                }
            }
        } // Spiritball attack
    } // End for
}

/* For checking if they have enough free limbs to preform the technique. */
// type 0 is arms, type 1 is legs. 2 is both. 3 is tail.
int limb_ok(Character *ch, int type)
{
    if (IS_NPC(ch))
    {
        if (AFF_FLAGGED(ch, AFF_ENSNARED) && Random::get<int>(1, 100) <= 90)
        {
            return false;
        }
        return true;
    }
    if (GRAPPLING(ch) && GRAPTYPE(ch) != 3)
    {
        ch->sendText("You are too busy grappling!\r\n");
        return false;
    }
    if (GRAPPLED(ch) && (GRAPTYPE(ch) == 1 || GRAPTYPE(ch) == 4))
    {
        ch->sendText("You are unable to move while in this hold! Try using 'escape' to get out of it!\r\n");
        return false;
    }
    if (GET_SONG(ch) > 0)
    {
        ch->sendText("You are currently playing a song! Enter the song command in order to stop!\r\n");
        return false;
    }

    // Arms
    if (type == 0 || type == 2)
    {
        if (!HAS_ARMS(ch))
        {
            ch->sendText("You have no available arms!\r\n");
            return false;
        }
        if (AFF_FLAGGED(ch, AFF_ENSNARED) && Random::get<int>(1, 100) <= 90)
        {
            ch->sendText("You are unable to move your arms while bound by this strong silk!\r\n");
            WAIT_STATE(ch, PULSE_1SEC);
            return false;
        }
        else if (AFF_FLAGGED(ch, AFF_ENSNARED))
        {
            act("You manage to break the silk ensnaring your arms!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n manages to break the silk ensnaring $s arms!", true, ch, nullptr, nullptr, TO_ROOM);
            ch->affect_flags.set(AFF_ENSNARED, false);
        }
        if (GET_EQ(ch, WEAR_WIELD1) && GET_EQ(ch, WEAR_WIELD2))
        {
            ch->sendText("Your hands are full!\r\n");
            return false;
        }
    }

    // Legs
    if (type == 1 || type == 2)
    {
        if (!HAS_LEGS(ch))
        {
            ch->sendText("You have no working legs!\r\n");
            return false;
        }
    }

    // tail
    if (type == 3)
    {
        if (!ch->hasTail())
        {
            ch->sendText("You have no tail!\r\n");
            return false;
        }
    }

    return true;
}

int init_skill(Character *ch, int snum)
{
    int skill = 0;

    if (!IS_NPC(ch))
    {
        skill = GET_SKILL(ch, snum);

        if (PLR_FLAGGED(ch, PLR_TRANSMISSION))
        {
            skill += 4;
        }

        if (skill > 118)
            skill = 118;

        return (skill);
    }

    auto l = GET_LEVEL(ch);

    if (l <= 10)
    {
        return Random::get<int>(30, 50);
    }
    else if (l <= 20)
    {
        return Random::get<int>(45, 65);
    }
    else if (l <= 30)
    {
        return Random::get<int>(55, 70);
    }
    else if (l <= 50)
    {
        return Random::get<int>(65, 80);
    }
    else if (l <= 70)
    {
        return Random::get<int>(75, 90);
    }
    else if (l <= 80)
    {
        return Random::get<int>(85, 100);
    }
    else if (l <= 90)
    {
        return Random::get<int>(90, 100);
    }
    else if (l <= 100)
    {
        return Random::get<int>(95, 100);
    }
    else if (l <= 110)
    {
        return Random::get<int>(95, 105);
    }
    else
    {
        return Random::get<int>(100, 110);
    }
}

int handle_block(Character *ch)
{

    if (axion_dice(0) <= 4)
    { /* Critical failure */
        return (1);
    }

    if (!IS_NPC(ch))
    { /* Players */
        if (!GET_SKILL(ch, SKILL_BLOCK))
        {
            return (0);
        }
        else
        {
            int num = GET_SKILL(ch, SKILL_BLOCK);
            if (ch->mutations.get(Mutation::extreme_reflexes))
            {
                num += 10;
            }
            if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100)
            {
                num += 5;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 80)
            {
                num += 4;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60)
            {
                num += 3;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40)
            {
                num += 2;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 20)
            {
                num += 1;
            }
            return (num);
        }
    }
    else
    { /* Mobs */
        if (!IS_HUMANOID(ch))
        { /* Animal/monster types */
            int top = GET_LEVEL(ch) / 4;

            if (top < 5)
                top = 6;
            return (Random::get<int>(5, top));
        }
        else
        { /* Intelligent Skills Mobs */
            if (GET_LEVEL(ch) >= 110)
            {
                return (Random::get<int>(95, 105));
            }
            else if (GET_LEVEL(ch) >= 100)
            {
                return (Random::get<int>(85, 95));
            }
            else if (GET_LEVEL(ch) >= 90)
            {
                return (Random::get<int>(70, 85));
            }
            else if (GET_LEVEL(ch) >= 75)
            {
                return (Random::get<int>(50, 70));
            }
            else if (GET_LEVEL(ch) >= 40)
            {
                return (Random::get<int>(40, 50));
            }
            else
            {
                int top = GET_LEVEL(ch);

                if (top < 15)
                    top = 16;
                return (Random::get<int>(15, top));
            }
        }
    }
}

int handle_dodge(Character *ch)
{

    if (axion_dice(0) <= 4)
    { /* Critical failure */
        return (1);
    }

    if (!IS_NPC(ch))
    { /* Players */
        if (!GET_SKILL(ch, SKILL_DODGE))
        {
            return (0);
        }
        else
        {
            int num = GET_SKILL(ch, SKILL_DODGE);
            if (ch->mutations.get(Mutation::extreme_reflexes))
            {
                num += 10;
            }
            if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100)
            {
                num += 5;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 80)
            {
                num += 4;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60)
            {
                num += 3;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40)
            {
                num += 2;
            }
            else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 20)
            {
                num += 1;
            }
            if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 100)
            {
                num += 3;
            }
            else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 75)
            {
                num += 2;
            }
            else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 50)
            {
                num += 1;
            }
            if (GET_SKILL_BASE(ch, SKILL_ROLL) >= 100)
            {
                num += 5;
            }
            else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 80)
            {
                num += 4;
            }
            else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 60)
            {
                num += 3;
            }
            else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 40)
            {
                num += 2;
            }
            else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 20)
            {
                num += 1;
            }
            if (group_bonus(ch, 2) == 8)
            {
                num += num * 0.05;
            }
            return (num);
        }
    }
    else
    { /* Mobs */
        if (!IS_HUMANOID(ch))
        { /* Animal/monster types */
            int top = (GET_LEVEL(ch) + 1) / 8;

            if (top < 5)
                top = 6;
            return (Random::get<int>(5, top));
        }
        else
        { /* Intelligent Skills Mobs */
            if (GET_LEVEL(ch) >= 110)
            {
                return (Random::get<int>(95, 105));
            }
            else if (GET_LEVEL(ch) >= 100)
            {
                return (Random::get<int>(75, 95));
            }
            else if (GET_LEVEL(ch) >= 90)
            {
                return (Random::get<int>(50, 85));
            }
            else if (GET_LEVEL(ch) >= 75)
            {
                return (Random::get<int>(30, 70));
            }
            else if (GET_LEVEL(ch) >= 40)
            {
                return (Random::get<int>(20, 50));
            }
            else
            {
                int top = GET_LEVEL(ch);

                if (top < 15)
                    top = 16;
                return (Random::get<int>(15, top));
            }
        }
    }
}

int check_def(Character *vict)
{
    int index = 0;
    int pry = handle_parry(vict), dge = handle_dodge(vict), blk = handle_block(vict);

    index = pry + dge + blk;

    if (index > 0)
        index /= 3;

    if (AFF_FLAGGED(vict, AFF_KNOCKED))
    {
        index = 0;
    }
    return index;
}

void handle_defense(Character *vict, int *pry, int *blk, int *dge)
{

    *pry = handle_parry(vict);

    *blk = handle_block(vict);

    *dge = handle_dodge(vict);

    if (!IS_NPC(vict))
    {
        if (GET_BONUS(vict, BONUS_WALL))
        {
            *blk += GET_SKILL(vict, SKILL_BLOCK) * 0.20;
        }

        if (GET_BONUS(vict, BONUS_PUSHOVER))
        {
            *blk -= GET_SKILL(vict, SKILL_BLOCK) * 0.20;
        }

        if (!GET_EQ(vict, WEAR_WIELD1) && !GET_EQ(vict, WEAR_WIELD2))
        {
            *blk += 4;
        }

        if (*blk > 110)
        {
            *blk = 110;
        }

        if (GET_BONUS(vict, BONUS_EVASIVE))
        {
            *dge += (GET_SKILL(vict, SKILL_DODGE) * 0.15);
        }

        if (GET_BONUS(vict, BONUS_PUNCHINGBAG))
        {
            *dge -= GET_SKILL(vict, SKILL_DODGE) * 0.15;
        }

        if (*dge > 110)
        {
            *dge = 110;
        }

        if (*pry > 110)
        {
            *pry = 110;
        }
        if (PLR_FLAGGED(vict, PLR_GOOP) && Random::get<int>(1, 100) >= 15)
        {
            *dge += 100;
            *blk += 100;
            *pry += 100;
        }
    }

    return;
}

void parry_ki(double attperc, Character *ch, Character *vict, char sname[1000], int prob, int perc, int skill,
              int type)
{
    char buf[200];
    char buf2[200];
    char buf3[200];
    int foundv = false, foundo = false;
    int64_t dmg = 0;
    Object *tob, *next_obj;
    Character *tch, *next_v;
    auto people = ch->location.getPeople();
    for (auto target : filter_raw(people))
    {
        tch = target;

        if (tch == ch)
            continue;
        if (tch == vict)
            continue;
        if (!can_kill(ch, tch, nullptr, 1))
            continue;

        if (Random::get<int>(1, 101) >= 90 && foundv == false)
        {
            if (handle_parry(tch) > Random::get<int>(1, 140))
            {
                sprintf(buf, "@C$N@W deflects your %s, sending it flying away!@n", sname);
                sprintf(buf2, "@WYou deflect @C$n's@W %s sending it flying away!@n", sname);
                sprintf(buf3, "@C$N@W deflects @c$n's@W %s sending it flying away!@n", sname);
                act(buf, true, ch, nullptr, tch, TO_CHAR);
                act(buf2, true, ch, nullptr, tch, TO_VICT);
                act(buf3, true, ch, nullptr, tch, TO_NOTVICT);
                foundv = false;
            }
            else
            {
                foundv = true;
                sprintf(buf,
                        "@WYou watch as the deflected %s slams into @C$N@W, exploding with a roar of blinding light!@n",
                        sname);
                sprintf(buf2,
                        "@c$n@W watches as the deflected %s slams into you! The %s explodes with a roar of blinding light!@n",
                        sname, sname);
                sprintf(buf3,
                        "@c$n@W watches as the deflected %s slams into @C$N@W! The %s explodes with a roar of blinding light!@n",
                        sname, sname);
                act(buf, true, vict, nullptr, tch, TO_CHAR);
                act(buf2, true, vict, nullptr, tch, TO_VICT);
                act(buf3, true, vict, nullptr, tch, TO_NOTVICT);
                dmg = damtype(ch, type, skill, attperc);
                hurt(0, 0, ch, tch, nullptr, dmg, 1);
                return;
            }
        }
    }
    auto loco = ch->location.getObjects();
    for (auto tob : filter_raw(loco))
    {
        if (OBJ_FLAGGED(tob, ITEM_UNBREAKABLE))
            continue;
        if (foundo == true)
            continue;
        if (Random::get<int>(1, 101) >= 80)
        {
            foundo = true;
            sprintf(buf,
                    "@WYou watch as the deflected %s slams into @g$p@W, exploding with a roar of blinding light!@n",
                    sname);
            sprintf(buf2,
                    "@c$n@W watches as the deflected %s slams into @g$p@W, exploding with a roar of blinding light!@n",
                    sname);
            act(buf, true, vict, tob, nullptr, TO_CHAR);
            act(buf2, true, vict, tob, nullptr, TO_ROOM);
            hurt(0, 0, ch, nullptr, tob, 25, 1);
            return;
        }
    }

    if ((foundo == false || foundv == false) && !vict->location.getWhereFlag(WhereFlag::space))
    {
        sprintf(buf,
                "@WYou watch as the deflected %s slams into the ground, exploding with a roar of blinding light!@n",
                sname);
        sprintf(buf2, "@WThe deflected %s slams into the ground, exploding with a roar of blinding light!@n", sname);
        act(buf, true, vict, nullptr, nullptr, TO_CHAR);
        act(buf2, true, vict, nullptr, nullptr, TO_ROOM);
        const auto tile = vict->location.getTileType();
        const std::unordered_set<int> tiles = {SECT_INSIDE, SECT_UNDERWATER, SECT_WATER_SWIM, SECT_WATER_NOSWIM};
        if (!tiles.contains(tile))
        {
            impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
            switch (Random::get<int>(1, 8))
            {
            case 1:
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_ROOM);
                break;
            case 2:
                if (Random::get<int>(1, 4) == 4 && vict->location.getGroundEffect() == 0)
                {
                    vict->location.setGroundEffect(1);
                    act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!",
                        true, ch, nullptr, vict, TO_ROOM);
                }
                break;
            case 3:
                act("A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_CHAR);
                act("A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 4:
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 5:
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 6:
                act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
                    true, ch, nullptr, vict, TO_CHAR);
                act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
                    true, ch, nullptr, vict, TO_ROOM);
                break;
            default:
                /* we want no message for the default */
                break;
            }
        }
        if (tile == SECT_UNDERWATER)
        {
            switch (Random::get<int>(1, 3))
            {
            case 1:
                act("The water churns violently!", true, ch, nullptr, vict, TO_CHAR);
                act("The water churns violently!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 2:
                act("Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_CHAR);
                act("Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 3:
                act("The water collapses in on the hole created!", true, ch, nullptr, vict, TO_CHAR);
                act("The water collapses in on the hole create!", true, ch, nullptr, vict, TO_ROOM);
                break;
            }
        }
        if (tile == SECT_WATER_SWIM || tile == SECT_WATER_NOSWIM)
        {
            switch (Random::get<int>(1, 3))
            {
            case 1:
                act("A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 2:
                act("The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_CHAR);
                act("The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 3:
                act("A huge depression forms in the water and erupts into a wave from the impact!", true, ch,
                    nullptr, vict, TO_CHAR);
                act("A huge depression forms in the water and erupts into a wave from the impact!", true, ch,
                    nullptr, vict, TO_ROOM);
                break;
            }
        }
        if (tile == SECT_INSIDE)
        {
            impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
            switch (Random::get<int>(1, 8))
            {
            case 1:
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_ROOM);
                break;
            case 2:
                act("The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr,
                    vict, TO_CHAR);
                act("The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr,
                    vict, TO_ROOM);
                break;
            case 3:
                act("Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_CHAR);
                act("Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 4:
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 5:
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 6:
                act("The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict,
                    TO_ROOM);
                break;
            default:
                /* we want no message for the default */
                break;
            }
        }
        if (ch->location.getDamage() < 100)
        {
            ch->location.modDamage(5);
        }
        if (auto zone = ch->location.getZone(); zone)
        {
            zone->sendText("An explosion shakes the entire area!\r\n");
        }
        return;
    }
}

void dodge_ki(Character *ch, Character *vict, int type, int type2, int skill, int skill2)
{
    if (type == 0 && !vict->location.getWhereFlag(WhereFlag::space))
    {
        const auto tile = ch->location.getTileType();
        if (tile != SECT_INSIDE)
        {
            impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
            switch (Random::get<int>(1, 8))
            {
            case 1:
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_ROOM);
                break;
            case 2:
                if (Random::get<int>(1, 4) == 4 && vict->location.getGroundEffect() == 0)
                {
                    vict->location.setGroundEffect(1);
                    act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!",
                        true, ch, nullptr, vict, TO_CHAR);
                    act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!",
                        true, ch, nullptr, vict, TO_ROOM);
                }
                break;
            case 3:
                act("A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_CHAR);
                act("A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 4:
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 5:
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 6:
                act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
                    true, ch, nullptr, vict, TO_CHAR);
                act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
                    true, ch, nullptr, vict, TO_ROOM);
                break;
            default:
                /* we want no message for the default */
                break;
            }
        }
        if (tile == SECT_UNDERWATER)
        {
            switch (Random::get<int>(1, 3))
            {
            case 1:
                act("The water churns violently!", true, ch, nullptr, vict, TO_CHAR);
                act("The water churns violently!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 2:
                act("Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_CHAR);
                act("Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 3:
                act("The water collapses in on the hole created!", true, ch, nullptr, vict, TO_CHAR);
                act("The water collapses in on the hole create!", true, ch, nullptr, vict, TO_ROOM);
                break;
            }
        }
        if (tile == SECT_WATER_SWIM || tile == SECT_WATER_NOSWIM)
        {
            switch (Random::get<int>(1, 3))
            {
            case 1:
                act("A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 2:
                act("The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_CHAR);
                act("The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 3:
                act("A huge depression forms in the water and erupts into a wave from the impact!", true, ch,
                    nullptr, vict, TO_CHAR);
                act("A huge depression forms in the water and erupts into a wave from the impact!", true, ch,
                    nullptr, vict, TO_ROOM);
                break;
            }
        }
        if (tile == SECT_INSIDE)
        {
            impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
            switch (Random::get<int>(1, 8))
            {
            case 1:
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict,
                    TO_ROOM);
                break;
            case 2:
                act("The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr,
                    vict, TO_CHAR);
                act("The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr,
                    vict, TO_ROOM);
                break;
            case 3:
                act("Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_CHAR);
                act("Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 4:
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 5:
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
                act("The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
                break;
            case 6:
                act("The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict,
                    TO_ROOM);
                break;
            default:
                /* we want no message for the default */
                break;
            }
        }
        if (ch->location.getDamage() <= 95)
        {
            ch->location.modDamage(5);
        }
        if (auto zone = ch->location.getZone(); zone)
        {
            zone->sendText("An explosion shakes the entire area!\r\n");
        }
    }
    if (type == 1)
    {
        if (Random::get<int>(1, 3) != 2)
        {
            act("@RIt turns around at the last second and begins to pursue @r$N@R!@n", true, ch, nullptr, vict,
                TO_CHAR);
            act("@RIt turns around at the last second and begins to pursue YOU!@n", true, ch, nullptr, vict, TO_VICT);
            act("@RIt turns around at the last second and begins to pursue @r$N@R!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            Object *obj;
            int num = 0;

            switch (skill2)
            {
            case 461:
                num = 80;
                break;
            default:
                num = 80;
                break;
            }

            obj = read_object(num, VIRTUAL);
            obj->moveToLocation(ch);

            TARGET(obj) = vict;
            obj->setBaseStat("kicharge", damtype(ch, type2, skill, .2));
            obj->setBaseStat("kitype", skill2);
            USER(obj) = ch;
        }
        else
        {
            act("@RIt fails to follow after @r$N@R!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@RIt fails to follow after YOU!@n", true, ch, nullptr, vict, TO_VICT);
            act("@RIt fails to follow after @r$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
        }
    }
    if (type == 2 && (skill2 != 481 || IS_FRIEZA(ch)))
    {
        if (skill2 == 481)
        {
            int chance = Random::get<int>(25, 50), prob = axion_dice(0);
            if (GET_SKILL(ch, SKILL_KIENZAN) >= 100)
            {
                chance += chance * 0.8;
            }
            else if (GET_SKILL(ch, SKILL_KIENZAN) >= 60)
            {
                chance += chance * 0.5;
            }
            else if (GET_SKILL(ch, SKILL_KIENZAN) >= 40)
            {
                chance += chance * 0.25;
            }
            if (chance < prob)
            {
                return;
            }
        }
        act("@RYou turn it around and send it back after @r$N@R!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@W$n @Rturns it around and sends it back after YOU!@n", true, ch, nullptr, vict, TO_VICT);
        act("@W$n @Rturns it around and sends it back after @r$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
        Object *obj;
        int num = 0;

        switch (skill2)
        {
        case 496:
            num = 81;
            break;
        case 481:
            num = 84;
            break;
        default:
            num = 81;
            break;
        }

        obj = read_object(num, VIRTUAL);
        obj->moveToLocation(ch);

        TARGET(obj) = vict;
        obj->setBaseStat("kicharge", damtype(ch, type2, skill, .3));
        obj->setBaseStat("kitype", skill2);
        USER(obj) = ch;
    }
}

static void damtype_unarmed_infuse(Character *ch, int64_t *dam)
{
    if (AFF_FLAGGED(ch, AFF_INFUSE))
    {
        auto infuse = GET_SKILL(ch, SKILL_INFUSE);
        *dam += (*dam / 100) * (infuse / 2);
        if (IS_JINTO(ch))
        {
            auto style = GET_SKILL_BASE(ch, SKILL_STYLE);
            if (style >= 100)
            {
                *dam += ((*dam * 0.01) * (infuse / 2)) * 0.5;
            }
            else if (style >= 60)
            {
                *dam += ((*dam * 0.01) * (infuse / 2)) * 0.25;
            }
            else if (style >= 40)
            {
                *dam += ((*dam * 0.01) * (infuse / 2)) * 0.05;
            }
        }
    }
}

static void damtype_unarmed_hasshuken(Character *ch, int64_t *dam)
{
    if (AFF_FLAGGED(ch, AFF_HASS))
    {
        *dam *= 2;
        if (IS_KRANE(ch))
        {
            auto hass = GET_SKILL(ch, SKILL_HASSHUKEN);
            if (hass >= 100)
            {
                *dam += *dam * 0.3;
            }
            else if (hass >= 60)
            {
                *dam += *dam * 0.2;
            }
            else if (hass >= 40)
            {
                *dam += *dam * 0.1;
            }
        }
    }
}

static void damtype_unarmed_hasshuken_or_infuse(Character *ch, int64_t *dam)
{
    if (AFF_FLAGGED(ch, AFF_HASS))
    {
        damtype_unarmed_hasshuken(ch, dam);
    }
    else
    {
        damtype_unarmed_infuse(ch, dam);
    }
}

static void damtype_unarmed_preference(Character *ch, int64_t *dam)
{
    if (GET_PREFERENCE(ch) == PREFERENCE_THROWING)
    {
        *dam -= *dam * 0.15;
    }
    else if (GET_PREFERENCE(ch) == PREFERENCE_H2H)
    {
        *dam += *dam * 0.20;
    }
}

static void damtype_focus(Character *ch, int64_t *dam, int64_t focus, int divby)
{
    if (focus > 0)
    {
        dam += focus * (*dam / divby);
    }
}

static void damtype_unarmed(Character *ch, int skill, int64_t *dam)
{
    // General Arlian bonus.
    if (IS_ARLIAN(ch))
    {
        *dam += *dam * 0.02;
    }

    // Sixteen's Iron Hand Bonus.
    if (IS_ANDSIX(ch))
    {
        if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
            *dam += *dam * 0.1;
    }

    // Brawler bonus
    if (GET_BONUS(ch, BONUS_BRAWLER) > 0)
    {
        *dam += *dam * .2;
    }

    switch (skill)
    {
    // Punch.
    case 0:
        damtype_unarmed_hasshuken_or_infuse(ch, dam);
        // Kame Arts bonus.
        if (IS_ROSHI(ch))
        {
            if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
                *dam += *dam * 0.2;
        }
        break;
        // Kick
    case 1:
        damtype_unarmed_infuse(ch, dam);
        // Crane Arts
        if (IS_KRANE(ch))
        {
            if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
                *dam += *dam * 0.2;
        }
        break;
    case 2: // Elbow
    case 5: // Uppercut
        damtype_unarmed_hasshuken_or_infuse(ch, dam);
        break;
    case 3:
    case 4:
    case 6:
    case 8:
    case 51:
    case 52:
        damtype_unarmed_infuse(ch, dam);
    }

    damtype_unarmed_preference(ch, dam);
}

static void damtype_human_grandmaster(Character *ch, int skill, int64_t *dam)
{
    if (IS_HUMAN(ch))
    {
        switch (skill)
        {
        case 101:
            *dam = *dam * 1.1;
        case 102:
            *dam = *dam * 1.2;
        case 103:
            *dam = *dam * 1.3;
        }
    }
}

static void damtype_human_ki(Character *ch, int64_t *dam, int bon)
{
    if (IS_HUMAN(ch))
    {
        *dam += (*dam / 100) * bon;
    }
}

static void damtype_saiyan_ki(Character *ch, int64_t *dam, int bon)
{
    if (IS_SAIYAN(ch))
    {
        *dam += (*dam / 100) * bon;
    }
}

static void damtype_kai_ki(Character *ch, int64_t *dam, int bon)
{
    if (IS_KAI(ch))
    {
        *dam += (*dam / 100) * bon;
    }
}

static void damtype_icer_ki(Character *ch, int64_t *dam, int bon)
{
    if (IS_ICER(ch) || ch->bio_genomes.get(Race::icer))
    {
        *dam += (*dam / 100) * bon;
    }
}

/* Damage for player and NPC attacks  */
int64_t damtype(Character *ch, int type, int skill, double percent)
{
    int64_t dam = 0, cou1 = 0, cou2 = 0, focus = 0;

    /* Player damages based on attack */
    if (!IS_NPC(ch))
    {
        if (GET_SKILL(ch, SKILL_FOCUS))
        {
            focus = GET_SKILL(ch, SKILL_FOCUS);
        }
        if (type != -2)
        {
            ch->setBaseStat<int>("last_attack", type);
        }
        else
        {
            type = 0;
        }

        int64_t pl = ch->getPL();

        // split switch approach to compress code.
        switch (type)
        {
        // big list of h2h stuff for starters.
        case -1:
            cou1 = 1 + ((skill / 4) * ((pl / 1200) + GET_STR(ch)));
            cou2 = 1 + ((skill / 4) * ((pl / 1000) + GET_STR(ch)));
            break;
        case 0: /* Punch */
            cou1 = 15 + ((skill / 4) * ((pl / 1600) + GET_STR(ch)));
            cou2 = 15 + ((skill / 4) * ((pl / 1300) + GET_STR(ch)));
            break;
        case 1: /* Kick */
            cou1 = 40 + ((skill / 4) * ((pl / 1200) + GET_STR(ch)));
            cou2 = 40 + ((skill / 4) * ((pl / 1000) + GET_STR(ch)));
            break;
        case 2: /* Elbow */
            cou1 = 100 + ((skill / 4) * ((pl / 1300) + GET_STR(ch)));
            cou2 = 100 + ((skill / 4) * ((pl / 1050) + GET_STR(ch)));
            break;
        case 3: /* Knee */
            cou1 = 150 + ((skill / 4) * ((pl / 1100) + GET_STR(ch)));
            cou2 = 150 + ((skill / 4) * ((pl / 1000) + GET_STR(ch)));
            break;
        case 4: /* Roundhouse */
            cou1 = 500 + ((skill / 4) * ((pl / 1000) + GET_STR(ch)));
            cou2 = 500 + ((skill / 4) * ((pl / 800) + GET_STR(ch)));
            break;
        case 5: /* Uppercut */
            cou1 = 350 + ((skill / 4) * ((pl / 1100) + GET_STR(ch)));
            cou2 = 350 + ((skill / 4) * ((pl / 900) + GET_STR(ch)));
            break;
        case 6: /* Slam */
            cou1 = 8000 + ((skill / 4) * ((pl / 800) + GET_STR(ch)));
            cou2 = 8000 + ((skill / 4) * ((pl / 500) + GET_STR(ch)));
            break;
        case 8: /* Heeldrop */
            cou1 = 12500 + ((skill / 4) * ((pl / 700) + GET_STR(ch)));
            cou2 = 12500 + ((skill / 4) * ((pl / 400) + GET_STR(ch)));
            break;
        case 51: /* Bash */
            cou1 = 1000 + ((skill / 4) * ((pl / 700) + GET_STR(ch)));
            cou2 = 1000 + ((skill / 4) * ((pl / 550) + GET_STR(ch)));
            break;
        case 52: /* Headbutt */
            cou1 = 800 + ((skill / 4) * ((pl / 900) + GET_STR(ch)));
            cou2 = 800 + ((skill / 4) * ((pl / 650) + GET_STR(ch)));
            break;
        case 56: /* TAILWHIP */
            cou1 = 400 + ((skill / 4) * ((pl / 1100) + GET_STR(ch)));
            cou2 = 400 + ((skill / 4) * ((pl / 1000) + GET_STR(ch)));
            break;
        }

        bool ki = false;
        // Set initial damage value.
        switch (type)
        {
        // h2h commonalities
        case -1:
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
            dam = Random::get<int64_t>(cou1, cou2);
            dam += GET_STR(ch) * (dam * 0.005);
            break;
        case 51:
        case 52:
        case 56: /* TAILWHIP */
            dam = Random::get<int64_t>(cou1, cou2);
            dam += GET_STR(ch) * 100;
            dam += GET_STR(ch) * (dam * 0.005);
            break;
        default: // all ki abilities.
            dam = GET_MAX_MANA(ch) * percent;
            ki = true;
        }

        int mod = 0;
        // ki type move pre-processing
        switch (type)
        {
        case 11: /* Tsuihidan */
        case 12: /* Renzo */
        case 23: /* Rogafufuken */
        case 25: /* Kienzan */
            mod = 500;
            break;
        case 13: /* Kamehameha */
        case 16: /* Galik Gun */
        case 26: /* Tribeam */
        case 50: /* Seishou Enko */
            mod = 800;
            break;
        case 14: /* Masenko */
        case 30: /* Darkness Dragon Slash */
        case 44: /* Spiral Comet 1 */
        case 45: /* Spiral Comet 2 */
        case 43: /* Water Spikes */
            mod = 1000;
            break;
        case 15: /* Dodonpa */
        case 17: /* Deathbeam */
        case 19: /* Twin Slash */
            mod = 650;
            break;
        case 18: /* Eraser Cannon */
        case 33: /* Hell Spear Blast */
        case 54: /* Zen Blade */
        case 55: /* Sundering Force */
            mod = 700;
            break;
        case 20: /* Psychic Blast */
        case 27: /* Special Beam Cannon */
        case 29: /* Crusher Ball */
        case 37: /* Phoenix Slash */
            mod = 1200;
            break;
        case 21: /* Honoo */
        case 39: /* Spirit ball */
        case 47: /* Water Razor */
        case 48: /* Koteiru Bakuha */
        case 49: /* Hell Spiral */
            mod = 900;
            break;
        case 22: /* Dual Beam */
        case 24: /* Bakuhatsuha */
            mod = 600;
            break;
        case 28: /* Final Flash */
            mod = 1500;
        case 31: /* Psychic Barrage */
        case 36: /* Big Bang */
            mod = 100;
            break;
        case 32: /* Hell Flash */
        case 46: /* Star Breaker */
            mod = 1400;
            break;
        case 34: /* Kakusanha */
            mod = 1050;
            break;
        case 35: /* Scatter Shot */
        case 53: /* Star Nova */
            mod = 1600;
            break;
        case 38: /* Deathball */
            mod = 1700;
            break;
        case 40: /* Genki Dama */
        case 41: /* Genocide */
            mod = 2000;
            break;
        case 42: /* Kousengan */
            mod = 550;
            break;
        case 57: /* Light Grenade */
            mod = 1700;
            break;
        }

        if (ki)
            dam *= 1.25;

        switch (type)
        {
        case -1:
            if (!PLR_FLAGGED(ch, PLR_THANDW))
                damtype_unarmed_hasshuken_or_infuse(ch, &dam);

            if (GET_BONUS(ch, BONUS_BRAWLER) > 0)
            {
                dam += dam * .2;
            }
            if (GET_PREFERENCE(ch) == PREFERENCE_KI)
            {
                dam -= dam * 0.20;
            }
            if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON && GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.05)
            {
                dam += GET_MAX_MANA(ch) * 0.05;
                ch->modBaseStat<int64_t>("charge", -(GET_MAX_MANA(ch) * 0.05));
            }
            else if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON && GET_CHARGE(ch) > 0)
            {
                dam += GET_CHARGE(ch);
                ch->setBaseStat<int64_t>("charge", 0);
            }
            if (group_bonus(ch, 2) == 8)
            {
                dam += dam * 0.02;
            }
            break;
        case 0:  /* Punch */
        case 1:  /* Kick */
        case 2:  /* Elbow */
        case 3:  /* Knee */
        case 4:  /* Roundhouse */
        case 5:  /* Uppercut */
        case 6:  /* Slam */
        case 8:  /* Heeldrop */
        case 51: /* Bash */
        case 52: /* Headbutt */
            damtype_unarmed(ch, type, &dam);
            break;
        case 56: /* TAILWHIP */
            damtype_unarmed_infuse(ch, &dam);
            damtype_unarmed_preference(ch, &dam);
            break;

        case 7: /* Kiball */
            damtype_focus(ch, &dam, focus, 1000);
            damtype_human_ki(ch, &dam, 25);
            break;
        case 9: /* Kiblast */
            damtype_focus(ch, &dam, focus, 500);
            damtype_human_ki(ch, &dam, 25);
            break;
        case 10: /* Beam/Shog */
        case 11: /* Tsuihidan */
        case 12: /* Renzo */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_ki(ch, &dam, 25);
            break;
        case 13: /* Kamehameha */
            if (focus > 0)
            {
                dam += (dam * 0.005) * focus;
            }
            damtype_human_grandmaster(ch, skill, &dam);
            damtype_human_ki(ch, &dam, 15);
            break;
        case 14: /* Masenko */
        case 15: /* Dodonpa */
        case 16: /* Galik Gun */
        case 17: /* Deathbeam */
        case 18: /* Eraser Cannon */
        case 19: /* Twin Slash */
        case 20: /* Psychic Blast */
        case 21: /* Honoo */
        case 24: /* Bakuhatsuha */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_ki(ch, &dam, 15);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 22: /* Dual Beam */
            damtype_focus(ch, &dam, focus, 200);
            break;
        case 23: /* Rogafufuken */
            dam += (dam / 100) * GET_STR(ch);
            damtype_focus(ch, &dam, focus, 200);
            if (GET_BONUS(ch, BONUS_BRAWLER) > 0)
            {
                dam += dam * .2;
            }
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 25: /* Kienzan */
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 26: /* Tribeam */

            if (!IS_NPC(ch) && percent > 0.15)
            {
                double hitperc = (percent - 0.15) * 5;
                int64_t amount = ch->getEffectiveStat<int64_t>("health") * hitperc;
                int64_t difference = GET_HIT(ch) - amount;

                ch->modCurVital(CharVital::health, -amount);

                damtype_focus(ch, &dam, focus, 200);
            }
            else
            {
                damtype_focus(ch, &dam, focus, 200);
            }
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 27: /* Special Beam Cannon */
            dam += (dam / 100) * GET_INT(ch);
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 28: /* Final Flash */
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 29: /* Crusher Ball */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 30: /* Darkness Dragon Slash */
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 31: /* Psychic Barrage */
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 32: /* Hell Flash */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 33: /* Hell Spear Blast */
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            break;
        case 34: /* Kakusanha */
            damtype_focus(ch, &dam, focus, 200);
            damtype_icer_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 35: /* Scatter Shot */
            damtype_focus(ch, &dam, focus, 200);
            damtype_icer_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 36: /* Big Bang */
            damtype_focus(ch, &dam, focus, 200);
            damtype_icer_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 37: /* Phoenix Slash */
            damtype_focus(ch, &dam, focus, 200);
            damtype_icer_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 38: /* Deathball */
            damtype_focus(ch, &dam, focus, 200);
            damtype_icer_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 39: /* Spirit ball */
            damtype_focus(ch, &dam, focus, 200);
            damtype_icer_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 40: /* Genki Dama */
            damtype_focus(ch, &dam, focus, 200);
            damtype_kai_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 41: /* Genocide */
            damtype_focus(ch, &dam, focus, 200);
            damtype_kai_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 42: /* Kousengan */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 43: /* Water Spikes */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 44: /* Spiral Comet 1 */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 45: /* Spiral Comet 2 */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 46: /* Star Breaker */
            damtype_focus(ch, &dam, focus, 200);
            break;
        case 47: /* Water Razor */
            damtype_focus(ch, &dam, focus, 200);
            break;
        case 48: /* Koteiru Bakuha */
            damtype_focus(ch, &dam, focus, 200);
            break;
        case 49: /* Hell Spiral */
            damtype_focus(ch, &dam, focus, 200);
            switch (ch->form)
            {
            case Form::android_1:
                dam += (dam * 0.01) * 5;
                break;
            case Form::android_2:
                dam += (dam * 0.01) * 15;
                break;
            case Form::android_3:
                dam += (dam * 0.01) * 25;
                break;
            case Form::android_4:
                dam += (dam * 0.01) * 50;
                break;
            case Form::android_5:
                dam += (dam * 0.01) * 75;
                break;
            case Form::android_6:
                dam += dam;
                break;
            default:
                break;
            }
            break;
        case 50: /* Seishou Enko */
            damtype_focus(ch, &dam, focus, 200);
            break;
        case 53: /* Star Nova */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_ki(ch, &dam, 15);
            break;
        case 54: /* Zen Blade */
            damtype_focus(ch, &dam, focus, 200);
            damtype_saiyan_ki(ch, &dam, 20);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 55: /* Sundering Force */
            damtype_focus(ch, &dam, focus, 200);
            damtype_human_grandmaster(ch, skill, &dam);
            break;
        case 57: /* Light Grenade */
            damtype_focus(ch, &dam, focus, 200);
            break;
        }
    }
    else
    {
        dam = 40 + (ch->getPL() * 0.15);
        dam += (dam * 0.005) * GET_LEVEL(ch);
        if (GET_LEVEL(ch) >= 120)
        {
            dam *= 0.25;
        }
        else if (GET_LEVEL(ch) >= 110)
        {
            dam *= 0.45;
        }
        else if (GET_LEVEL(ch) >= 100)
        {
            dam *= 0.75;
        }
        else
        {
            dam *= 2;
        }
    }

    if (IS_NPC(ch))
    {

        if (type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 || type == 8 ||
            type == 51 || type == 52 || type == 56)
        {
            dam += GET_LEVEL(ch) * (dam * 0.005);
        }
        else
        {
            dam += GET_LEVEL(ch) * (dam * 0.005);
        }

        auto mob_hit = ch->getPL();
        auto max_hit = GET_MAX_HIT(ch);
        int64_t mobperc = (mob_hit * 100) / max_hit;
        if (mobperc < 98 && mobperc >= 90)
        {
            dam = dam * 0.95;
        }
        else if (mobperc < 90 && mobperc >= 80)
        {
            dam = dam * 0.90;
        }
        else if (mobperc < 80 && mobperc >= 790)
        {
            dam = dam * 0.85;
        }
        else if (mobperc < 70 && mobperc >= 50)
        {
            dam = dam * 0.80;
        }
        else if (mobperc < 50 && mobperc >= 30)
        {
            dam = dam * 0.70;
        }
        else if (mobperc <= 29)
        {
            dam = dam * 0.60;
        }

        if (GET_CLASS(ch) != Sensei::commoner)
        {
            dam += dam * 0.3;
        }
    }

    /* Start of Fury Mode for halfbreeds */
    if (PLR_FLAGGED(ch, PLR_FURY) &&
        (type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 || type == 8 ||
         type == 51 || type == 52))
    {
        dam *= 1.5;
        act("Your rage magnifies your attack power!", true, ch, nullptr, nullptr, TO_CHAR);
        act("Swirling energy flows around $n as $e releases $s rage in the attack!", true, ch, nullptr, nullptr,
            TO_ROOM);
        if (Random::get<int>(1, 10) >= 7)
        {
            ch->sendText("You feel less angry.\r\n");
            ch->affect_flags.set(PLR_FURY, false);
        }
    }
    else if (PLR_FLAGGED(ch, PLR_FURY))
    {
        dam *= 2;
        act("Your rage magnifies your attack power!", true, ch, nullptr, nullptr, TO_CHAR);
        act("Swirling energy flows around $n as $e releases $s rage in the attack!", true, ch, nullptr, nullptr,
            TO_ROOM);
        ch->affect_flags.set(PLR_FURY, false);
    }
    /* End of Fury Mode for halfbreeds */

    if ((type == -1 || type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 ||
         type == 8))
    {
        if (!IS_NPC(ch))
            dam -= dam * 0.08;
        if (!IS_NPC(ch) && dam > GET_MAX_HIT(ch) * 0.1)
            dam *= 0.6;
    }
    else
    {
        dam += (dam * 0.005) * GET_INT(ch);
        if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON)
        {
            dam -= dam * 0.25;
        }
        else if (GET_PREFERENCE(ch) == PREFERENCE_THROWING)
        {
            dam -= dam * 0.15;
        }
    }

    return dam;
}

void saiyan_gain(Character *ch, Character *vict)
{
    int weak = false;
    if (!vict)
        return;

    if (IS_NPC(ch))
        return;

    if (vict->getPL() < vict->getPL() / 10)
    {
        ch->sendText("@D[@YSaiyan @RBlood@D] @WThey are too weak to inspire your saiyan soul!@n\r\n");
        return;
    }

    if (Random::get<int>(1, 22) >= 8)
    {
        return;
    }

    std::vector<int64_t> stats;
    for (const auto stat : {CharVital::health, CharVital::ki, CharVital::stamina})
    {
        if (!ch->is_soft_cap((int)stat, 1.0))
            stats.push_back((int)stat);
    }

    if (ch->technique == Form::tiger_stance)
        stats.push_back((int)CharVital::health);
    if (ch->technique == Form::eagle_stance)
        stats.push_back((int)CharVital::ki);
    if (ch->technique == Form::ox_stance)
        stats.push_back((int)CharVital::stamina);

    auto itr = Random::get(stats);

    if (stats.empty())
    {
        ch->sendText("@D[@YSaiyan @RBlood@D] @WYou feel you have reached your current limits.@n\r\n");
        return;
    }

    double attrBonus = 0;
    double base = 0;
    switch (*itr)
    {
    case 0:
        base = ch->getBaseStat<int64_t>("health");
        attrBonus = (1 + (GET_CON(ch) / 20));
        break;
    case 1:
        base = ch->getBaseStat<int64_t>("ki");
        attrBonus = (1 + (GET_WIS(ch) / 20));
        break;
    case 2:
        base = ch->getBaseStat<int64_t>("stamina");
        attrBonus = (1 + (GET_CON(ch) / 20));
        break;
    }

    int64_t bonus = 0;
    double start_bonus = Random::get<double>(0.8, 1.2) * attrBonus * ch->getPotential();
    double soft_cap = (double)ch->calc_soft_cap();
    double diminishing_returns = (soft_cap - base) / soft_cap;
    if (diminishing_returns > 0.0)
        diminishing_returns = std::max<double>(diminishing_returns, 0.05);
    else
        diminishing_returns = 0;
    bonus = (start_bonus)*diminishing_returns;

    if (ch->bio_genomes.get(Race::saiyan))
    {
        bonus /= 2;
    }

    switch (*itr)
    {
    case 0:
        bonus *= (1 + ch->getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::health)));
        ch->gainBaseStat("health", bonus);
        ch->send_to("@D[@YSaiyan @RBlood@D] @WYou feel slightly tougher. @D[@G+%s@D]@n\r\n", add_commas(bonus).c_str());
        break;
    case 1:
        bonus *= (1 + ch->getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::ki)));
        ch->gainBaseStat("ki", bonus);
        ch->send_to("@D[@YSaiyan @RBlood@D] @WYou feel your spirit grow. @D[@G+%s@D]@n\r\n", add_commas(bonus).c_str());
        break;
    case 2:
        bonus *= (1 + ch->getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::stamina)));
        ch->gainBaseStat("stamina", bonus);
        ch->send_to("@D[@YSaiyan @RBlood@D] @WYou feel slightly more vigorous. @D[@G+%s@D]@n\r\n", add_commas(bonus).c_str());
        break;
    }
}

static void spar_helper(Character *ch, Character *vict, int type, int64_t dmg)
{
    int chance = 0, gmult, gravity, bonus = 1, pscost = 2;
    double difference = 0.0;
    int64_t gain = 0, pl = 0, ki = 0, st = 0, gaincalc = 0;
    int attrChance = 3;

    // If damage is greater than a tenth of the opponents health, there's a greater chance to proc the spar gains
    if (dmg > GET_MAX_HIT(vict) / 3)
    {
        chance = Random::get<int>(20, 100);
        attrChance = 10;
    }
    else if (dmg >= GET_MAX_HIT(vict) / 10)
    {
        chance = Random::get<int>(20, 100);
    }
    else
    {
        chance = Random::get<int>(1, 50);
    }

    // The chance is reduced if you keep farming in one area via Relaxcount
    if (GET_RELAXCOUNT(ch) >= 464)
    {
        chance = 0;
    }
    else if (GET_RELAXCOUNT(ch) >= 232)
    {
        chance -= chance * 0.5;
    }
    else if (GET_RELAXCOUNT(ch) >= 116)
    {
        chance -= chance * 0.2;
    }

    // gmult sets the gain for vital increases, based on the character's level x6
    gmult = 6;

    if (auto obj = GET_EQ(ch, WEAR_SH); obj)
    {
        // If you are using a spar booster
        if (GET_OBJ_VNUM(obj) == 1127)
        {
            gmult *= 1.5;
        }
    }

    // Bonuses by room to vital gains
    if (ch->location.getRoomFlag(ROOM_WORKOUT) || (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber)))
    {
        if (ch->location.getVnum() >= 19100 && ch->location.getVnum() <= 19199)
        {
            gmult *= 1.75;
            pscost += 2;
        }
        else
        {
            gmult *= 1.25;
            pscost += 1;
        }
        pl = Random::get<int64_t>(gmult * .8, gmult * 1.2);
        ki = Random::get<int64_t>(gmult * .8, gmult * 1.2);
        st = Random::get<int64_t>(gmult * .8, gmult * 1.2);
    }
    else
    {
        pl = Random::get<int64_t>(gmult * .4, gmult * .8);
        ki = Random::get<int64_t>(gmult * .4, gmult * .8);
        st = Random::get<int64_t>(gmult * .4, gmult * .8);
    }

    // Logic for earning xp through sparring, based on the character's level, the curve is roughly exponential
    auto chCon = GET_CON(ch);
    if (chance >= Random::get<int>(40, 75))
    {

        bool isLethal = !(ch->isSparring() && vict->isSparring());

        // Check if victim is NPC or Player, both have different logic here.
        if (vict && IS_NPC(vict))
        {
            // Work out how challenging of an npc to fight this is, if the victim is stronger we want to give more xp. This doesn't affect attributes.
            float plRatio = (float)vict->getPL() / (float)ch->getPL();
            float plGain = 1;

            // The lowest you can get from a fight is 0.5 times the xp, so long as they aren't 5x weaker than you
            if (plRatio > 0.2)
            {
                plGain = 0.5 + (plRatio / 2);
                if (plGain > 3)
                    plGain = 3;
            }
            else
            {
                ch->sendText("This foe is not strong enough for you to learn anything.\r\n");
                plGain = 0;
            }

            // Rewarded if your opponent is fighting for the kill
            float deadlyBonus = isLethal ? 1.2 : 1;

            // Average out the bonus to limit the exponential gain
            float gearGain = 1.0 + (3.0 * ch->getBaseStat("burden_ratio"));
            if (gearGain <= 0)
            {
                gearGain = 0.1;
            }

            if ((plGain * gearGain) > 1)
                gaincalc = plGain * gearGain;
            else
                gaincalc = 1;

            gaincalc *= deadlyBonus;
            type = 3;
        }
        else if (vict && !IS_NPC(vict))
        {
            // Fighting against players has randomised gains. Does not get a bonus for higher power level in spars

            // Rewarded if your opponent is fighting for the kill
            float deadlyBonus = isLethal ? 2 : 1;

            gaincalc = Random::get<int64_t>(deadlyBonus, 1.4 * deadlyBonus);
            gaincalc = gear_exp(ch, gaincalc);
        }
        if (vict)
        {
            // You are penalized for level difference in sparring? Remove this perhaps as it disincentivises oldbies teaching newbies. And it's against the lore, that happens a lot.

            if (!IS_NPC(vict) && !IS_NPC(ch))
            {
                // Logic for instructing and giving a bonus to your sparring partner, it costs practices to the 'victim'
                if (!IS_NPC(ch) && PRF_FLAGGED(vict, PRF_INSTRUCT))
                {
                    if (GET_PRACTICES(vict) > 10)
                    {
                        vict->sendText("You instruct them in proper fighting techniques and strategies.\r\n");
                        act("You take $N's instruction to heart and gain more experience.\r\n", false, ch, nullptr,
                            vict, TO_CHAR);
                        vict->modPractices(-10);
                        bonus = 2;
                    }
                }
            }
        }

        // Saiyan's and Halfies get boosted gains from sparring, Icers and Bio's with the same gene get a debuff
        if (IS_SAIYAN(ch))
        {
            gaincalc = gaincalc * 1.25;
        }
        if (IS_HALFBREED(ch))
        {
            gaincalc = gaincalc * 1.2;
        }
        if (IS_ICER(ch) || ch->bio_genomes.get(Race::icer))
        {
            gaincalc = gaincalc * 0.9;
        }
        // Room bonuses to xp gain
        if (ch->location.getRoomFlag(ROOM_WORKOUT) || (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber)))
        {
            if (ch->location.getVnum() >= 19100 && ch->location.getVnum() <= 19199)
            {
                gaincalc *= 1.5;
            }
            else
            {
                gaincalc *= 1.25;
            }
        }
        pl *= gaincalc * bonus * (GET_CON(ch) / 4) * Random::get<double>(0.8, 1.2) * ch->getPotential();
        ki *= gaincalc * bonus * (GET_WIS(ch) / 4) * Random::get<double>(0.8, 1.2) * ch->getPotential();
        st *= gaincalc * bonus * (GET_CON(ch) / 4) * Random::get<double>(0.8, 1.2) * ch->getPotential();
        if (pl > (ch->getBaseStat<int64_t>("health") / 20))
            pl = ch->getBaseStat<int64_t>("health") / 20;
        if (ki > (ch->getBaseStat<int64_t>("ki") / 20))
            ki = ch->getBaseStat<int64_t>("ki") / 20;
        if (st > (ch->getBaseStat<int64_t>("stamina") / 20))
            st = ch->getBaseStat<int64_t>("stamina") / 20;

        giveRandomVital(ch, pl, ki, st, attrChance);
    }
}

void giveRandomVital(Character *ch, int64_t pl, int64_t ki, int64_t st, int attrChance)
{
    // Handling for awarding vitals to the player
    pl *= (1 + ch->getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::health)));
    ki *= (1 + ch->getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::ki)));
    st *= (1 + ch->getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::stamina)));
    if (pl > (ch->getBaseStat<int64_t>("health") / 10))
        pl = ch->getBaseStat<int64_t>("health") / 10;
    if (ki > (ch->getBaseStat<int64_t>("ki") / 10))
        ki = ch->getBaseStat<int64_t>("ki") / 10;
    if (st > (ch->getBaseStat<int64_t>("stamina") / 10))
        st = ch->getBaseStat<int64_t>("stamina") / 10;

    std::vector<int64_t> stats;
    for (const auto stat : {CharVital::health, CharVital::ki, CharVital::stamina})
    {
        if (!ch->is_soft_cap((int)stat, 1.0))
            stats.push_back((int)stat);
    }

    if (ch->technique == Form::tiger_stance)
        stats.push_back((int)CharVital::health);
    if (ch->technique == Form::eagle_stance)
        stats.push_back((int)CharVital::ki);
    if (ch->technique == Form::ox_stance)
        stats.push_back((int)CharVital::stamina);

    if (!stats.empty())
    {

        do
        {
            auto itr = Random::get(stats);
            switch (*itr)
            {
            case 0:
                ch->send_to("@D[@Y+ @R%s @rPL@D]@n\r\n", add_commas(pl).c_str());
                ch->gainBaseStat("health", pl);
                if (axion_dice(0) <= attrChance)
                {
                    int rand = Random::get<int>(1, 2);
                    std::string val;
                    if (rand == 1)
                    {
                        val = "agility";
                        ch->sendText("@mYour body feels like it's light as a feather!@n\r\n");
                    }
                    else
                    {
                        val = "speed";
                        ch->sendText("@mThe world feels just a little slower.@n\r\n");
                    }

                    ch->modBaseStat(val, 1);
                }
                break;
            case 1:
                ch->send_to("@D[@Y+ @C%s @cKI@D]@n\r\n", add_commas(ki).c_str());
                ch->gainBaseStat("ki", ki);
                if (axion_dice(0) <= attrChance)
                {
                    int rand = Random::get<int>(1, 2);
                    std::string val;
                    if (rand == 1)
                    {
                        val = "intelligence";
                        ch->sendText("@mYou begin to notice new ways to put together your attacks.@n\r\n");
                    }
                    else
                    {
                        val = "wisdom";
                        ch->sendText("@mYou notice a couple of flaws in your opponents technique.@n\r\n");
                    }

                    ch->modBaseStat(val, 1);
                }
                break;
            case 2:
                ch->send_to("@D[@Y+ @C%s @cST@D]@n\r\n", add_commas(st).c_str());
                ch->gainBaseStat("stamina", st);
                if (axion_dice(0) <= attrChance)
                {
                    int rand = Random::get<int>(1, 2);
                    std::string val;
                    if (rand == 1)
                    {
                        val = "constitution";
                        ch->sendText("@mThe pain of your wounds feel just a little bit less important.@n\r\n");
                    }
                    else
                    {
                        val = "strength";
                        ch->sendText("@mYour hits seem to be landing just a bit harder.@n\r\n");
                    }

                    ch->modBaseStat(val, 1);
                }
                break;
            }
        } while (Random::get<int>(1, 3) == 3);
    }
    else
    {
        ch->sendText("\r\n");
    }
}

void spar_gain(Character *ch, Character *vict, int type, int64_t dmg)
{
    for (auto c : {ch, vict})
    {
        if (GET_POS(c) < POS_FIGHTING)
            return;
        for (auto a : {AFF_SLEEP, AFF_PARALYZE, AFF_STUNNED, AFF_KNOCKED, AFF_PARA, AFF_FROZEN})
            if (AFF_FLAGGED(c, a))
                return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING) != AFF_FLAGGED(vict, AFF_FLYING))
        return;

    spar_helper(ch, vict, type, dmg);
    spar_helper(vict, ch, type, dmg);
}

bool can_grav(Character *ch)
{
    if (IS_NPC(ch))
        return true;
    auto result = ch->getBaseStat("burden_ratio") <= 1.0;
    if (!result)
    {
        ch->sendText("You are too burdened to even think about it!\r\n");
    }
    return result;
}

/* If they can preform the attack or perform the attack on target. */
int can_kill(Character *ch, Character *vict, Object *obj, int num)
{
    /* Target Related */
    if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return 0;
    }

    if (IS_CARRYING_W(ch) > CAN_CARRY_W(ch))
    {
        ch->sendText("You are weighted down too much!\r\n");
        return 0;
    }

    if (ch->location.getRoomFlag(ROOM_PEACEFUL))
    {
        ch->sendText("This room just has such a peaceful, easy feeling...\r\n");
        return 0;
    }

    if (vict)
    {
        if (GET_HIT(vict) <= 0 && FIGHTING(vict))
        {
            return 0;
        }
        if (vict == ch)
        {
            ch->sendText("That's insane, don't hurt yourself. Hurt others! That's the key to life ^_^\r\n");
            return 0;
        }
        else if (vict->getBaseStat<int>("gooptime") > 0)
        {
            ch->sendText("It seems like it'll be hard to kill them right now...\r\n");
            return 0;
        }
        else if (CARRYING(ch))
        {
            ch->sendText("You are too busy protecting the person on your shoulder!\r\n");
            return 0;
        }
        else if (CARRIED_BY(vict))
        {
            ch->sendText("They are being protected by someone else!\r\n");
            return 0;
        }
        else if (AFF_FLAGGED(vict, AFF_PARALYZE))
        {
            ch->sendText("They are a statue, just leave them alone...\r\n");
            return 0;
        }
        else if (MOB_FLAGGED(vict, MOB_NOKILL))
        {
            ch->sendText("But they are not to be killed!\r\n");
            return 0;
        }
        else if (ch->getBaseStat<int>("majinizer") == vict->id)
        {
            ch->sendText("You can not harm your master!\r\n");
            return 0;
        }
        else if (GET_BONUS(ch, BONUS_COWARD) > 0 && vict->getPL() > ch->getPL() + (ch->getPL() * .5) &&
                 !FIGHTING(ch))
        {
            ch->sendText("You are too cowardly to start anything with someone so much stronger than yourself!\r\n");
            return 0;
        }
        else if (vict->getBaseStat<int>("majinizer") == ch->id)
        {
            ch->sendText("You can not harm your servant.\r\n");
            return 0;
        }
        else if ((GRAPPLING(ch) && GRAPTYPE(ch) != 3) || (GRAPPLED(ch) && (GRAPTYPE(ch) == 1 || GRAPTYPE(ch) == 4)))
        {
            ch->send_to("You are too busy grappling!%s\r\n", GRAPPLED(ch) ? " Try 'escape'!" : "");
            return 0;
        }
        else if (GRAPPLING(ch) && GRAPPLING(ch) != vict)
        {
            ch->sendText("You can't reach that far in your current position!\r\n");
            return 0;
        }
        else if (GRAPPLED(ch) && GRAPPLED(ch) != vict)
        {
            ch->sendText("You can't reach that far in your current position!\r\n");
            return 0;
        }
        else if (!IS_NPC(ch) && !IS_NPC(vict) && AFF_FLAGGED(ch, AFF_SPIRIT) &&
                 (!ch->isSparring() || !vict->isSparring()) && num != 2)
        {
            ch->sendText("You can not fight other players in AL/Hell.\r\n");
            return 0;
        }
        else if (vict->is_newbie() && !IS_NPC(ch) && !IS_NPC(vict) && (!ch->isSparring() || !vict->isSparring()))
        {
            ch->sendText("Newbie Shield Protects them!\r\n");
            return 0;
        }
        else if (ch->is_newbie() && !IS_NPC(ch) && !IS_NPC(vict) && (!ch->isSparring() || !vict->isSparring()))
        {
            ch->sendText("Newbie Shield Protects you until PL 10,000.\r\n");
            return 0;
        }
        else if (PLR_FLAGGED(vict, PLR_SPIRAL) && num != 3)
        {
            ch->sendText("Due to the nature of their current technique anything less than a Tier 4 or AOE attack will not work on them.\r\n");
            return 0;
        }
        else if (ABSORBING(ch))
        {
            ch->send_to("You are too busy absorbing %s!\r\n", GET_NAME(ABSORBING(ch)));
            return 0;
        }
        else if (ABSORBBY(ch))
        {
            ch->send_to("You are too busy being absorbed by %s!\r\n", GET_NAME(ABSORBBY(ch)));
            return 0;
        }
        else if ((std::abs(GET_ALT(vict) - GET_ALT(ch)) == 1) && (IS_NAMEK(ch) || ch->bio_genomes.get(Race::namekian)))
        {
            act("@GYou stretch your limbs toward @g$N@G in an attempt to hit $M!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@g$n@G stretches $s limbs toward @RYOU@G in an attempt to land a hit!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@g$n@G stretches $s limbs toward @g$N@G in an attempt to hit $M!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            return 1;
        }
        else if (AFF_FLAGGED(ch, AFF_FLYING) && !AFF_FLAGGED(vict, AFF_FLYING) && num == 0)
        {
            ch->sendText("You are too far above them.\r\n");
            return 0;
        }
        else if (!AFF_FLAGGED(ch, AFF_FLYING) && AFF_FLAGGED(vict, AFF_FLYING) && num == 0)
        {
            ch->sendText("They are too far above you.\r\n");
            return 0;
        }
        else if (!IS_NPC(ch) && GET_ALT(ch) > GET_ALT(vict) && !IS_NPC(vict) && num == 0)
        {
            if (GET_ALT(vict) < 0)
            {
                vict->setBaseStat<int>("altitude", GET_ALT(ch));
                return 1;
            }
            else
            {
                ch->sendText("You are too far above them.\r\n");
                return 0;
            }
        }
        else if (!IS_NPC(ch) && GET_ALT(ch) < GET_ALT(vict) && !IS_NPC(vict) && num == 0)
        {
            if (GET_ALT(vict) > 2)
            {
                vict->setBaseStat<int>("altitude", GET_ALT(ch));
                return 1;
            }
            else
            {
                ch->sendText("They are too far above you.\r\n");
                return 0;
            }
        }
        else
        {
            return 1;
        }
    }
    if (obj)
    {
        if (OBJ_FLAGGED(obj, ITEM_UNBREAKABLE) && GET_OBJ_VNUM(obj) != 87 && GET_OBJ_VNUM(obj) != 80 &&
            GET_OBJ_VNUM(obj) != 81 && GET_OBJ_VNUM(obj) != 82 && GET_OBJ_VNUM(obj) != 83)
        {
            ch->sendText("You can't hit that, it is protected by the immortals!\r\n");
            return 0;
        }
        else if (AFF_FLAGGED(ch, AFF_FLYING))
        {
            ch->sendText("You are too far above it.\r\n");
            return 0;
        }
        else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
        {
            ch->sendText("It is already broken!\r\n");
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        ch->sendText("Error: Report to imm.");
        return 0;
    }
}

/* Whether they know the skill they are trying to use */
int check_skill(Character *ch, int skill)
{
    if (!know_skill(ch, skill) && !IS_NPC(ch))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/* Whether they have enough stamina or charged ki to preform the skill */
bool check_points(Character *ch, int64_t ki, int64_t st)
{

    if (GET_PREFERENCE(ch) == PREFERENCE_H2H && GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.1)
    {
        st *= 0.5;
    }

    int fail = false;
    if (IS_NPC(ch))
    {
        if ((ch->getCurVital(CharVital::ki)) < ki)
        {
            ch->sendText("You do not have enough ki!\r\n");
            return false;
        }
        if ((ch->getCurVital(CharVital::stamina)) < st)
        {
            ch->sendText("You do not have enough stamina!\r\n");
            return false;
        }
    }

    if (GET_CHARGE(ch) < ki)
    {
        ch->sendText("You do not have enough ki charged.\r\n");
        int64_t perc = GET_MAX_MANA(ch) * 0.01;
        if (ki >= perc * 49)
        {
            ch->sendText("You need at least 50 percent charged.\r\n");
        }
        else if (ki >= perc * 44)
        {
            ch->sendText("You need at least 45 percent charged.\r\n");
        }
        else if (ki >= perc * 39)
        {
            ch->sendText("You need at least 40 percent charged.\r\n");
        }
        else if (ki >= perc * 34)
        {
            ch->sendText("You need at least 35 percent charged.\r\n");
        }
        else if (ki >= perc * 29)
        {
            ch->sendText("You need at least 30 percent charged.\r\n");
        }
        else if (ki >= perc * 24)
        {
            ch->sendText("You need at least 25 percent charged.\r\n");
        }
        else if (ki >= perc * 19)
        {
            ch->sendText("You need at least 20 percent charged.\r\n");
        }
        else if (ki >= perc * 14)
        {
            ch->sendText("You need at least 15 percent charged.\r\n");
        }
        else if (ki >= perc * 9)
        {
            ch->sendText("You need at least 10 percent charged.\r\n");
        }
        else if (ki >= perc * 4)
        {
            ch->sendText("You need at least 5 percent charged.\r\n");
        }
        else if (ki >= 1)
        {
            ch->sendText("You need at least 1 percent charged.\r\n");
        }
        fail = true;
    }

    if (IS_ICER(ch))
    {
        switch (ch->form)
        {
        case Form::icer_1:
            st *= 1.05;
            break;
        case Form::icer_2:
            st *= 1.1;
            break;
        case Form::icer_3:
            st *= 1.15;
            break;
        case Form::icer_4:
            st *= 1.20;
            break;
        default:
            break;
        }
    }

    // Below section is commented out due to not using these flags anymore.
    // TODO: Review and re-implement.

    /*
    if (IS_NONPTRANS(ch)) {
        if (PLR_FLAGGED(ch, PLR_TRANS1)) {
            st *= 0.8;
        } else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
            st *= 0.6;
        } else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
            st *= 0.4;
        } else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
            st *= 0.2;
        }
    }
     */
    auto ratio = ch->getBaseStat("burden_ratio");
    // increase the stamina costs by ratio. Ratio can be 0.0 to 1.0 or more.
    // If ratio is 0, then no change. If ratio is 1.0, then double the cost.
    st += st * ratio;

    if ((ch->getCurVital(CharVital::stamina)) < st)
    {
        ch->send_to("You do not have enough stamina.\r\n@C%s@n needed.\r\n", add_commas(st).c_str());
        fail = true;
    }

    return !fail;
}

/* Subtract the stamina or ki required */
void pcost(Character *ch, double ki, int64_t st)
{
    if (IS_NPC(ch))
    {
        if (ki)
            ch->modCurVital(CharVital::ki, -ki);
        if (st)
            ch->modCurVital(CharVital::stamina, -st);
        return;
    }

    int before = 0;
    if (ki == 0)
    {
        before = (ch->getCurVital(CharVital::stamina));
    }
    if (GET_CHARGE(ch) <= (GET_MAX_MANA(ch) * ki))
    {
        ch->setBaseStat<int64_t>("charge", 0);
    }
    if (GET_CHARGE(ch) > (GET_MAX_MANA(ch) * ki))
    {
        ch->modBaseStat<int64_t>("charge", -(GET_MAX_MANA(ch) * ki));
    }
    if (GET_CHARGE(ch) < 0)
    {
        ch->setBaseStat<int64_t>("charge", 0);
    }
    if (AFF_FLAGGED(ch, AFF_HASS))
    {
        st += st * .3;
    }
    if (!IS_NPC(ch) && GET_BONUS(ch, BONUS_HARDWORKER) > 0)
    {
        st -= st * .25;
    }
    else if (!IS_NPC(ch) && GET_BONUS(ch, BONUS_SLACKER) > 0)
    {
        st += st * .25;
    }

    if (GET_PREFERENCE(ch) == PREFERENCE_H2H && GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.1)
    {
        st -= st * 0.5;
        ch->modBaseStat<int64_t>("charge", -st);
        if (GET_CHARGE(ch) < 0)
            ch->setBaseStat<int64_t>("charge", 0);
    }

    if (IS_ICER(ch))
    {
        switch (ch->form)
        {
        case Form::icer_1:
            st *= 1.05;
            break;
        case Form::icer_2:
            st *= 1.1;
            break;
        case Form::icer_3:
            st *= 1.15;
            break;
        case Form::icer_4:
            st *= 1.20;
            break;
        default:
            break;
        }
    }

    // TODO: reimplement
    /*
    if (IS_NONPTRANS(ch)) {
        if (PLR_FLAGGED(ch, PLR_TRANS1)) {
            st -= st * 0.2;
        } else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
            st -= st * 0.4;
        } else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
            st -= st * 0.6;
        } else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
            st -= st * 0.8;
        }
    }
        */

    auto ratio = ch->getBaseStat("burden_ratio");
    st += st * ratio;
    ch->modCurVital(CharVital::stamina, -st);
}

/* Main damage function for RDBS 'Real Dragonball Battle System' */
void hurt(int limb, int chance, Character *ch, Character *vict, Object *obj, int64_t dmg, int type)
{
    int64_t index = 0;
    int64_t maindmg = dmg, beforered = dmg;
    int dead = false;

    if (vict && vict->getCurVital(CharVital::health) <= 0)
    {
        ch->sendText("They are already dead!");
        return;
    }

    /* If a character is targeted */

    if (type <= 0)
    {
        if (IS_SAIYAN(ch) && ch->character_flags.get(CharacterFlag::tail))
        {
            dmg += dmg * .15;
        }
        if (IS_NAMEK(ch) && !GET_EQ(ch, WEAR_HEAD))
        {
            dmg += dmg * .25;
        }
        if (group_bonus(ch, 2) == 4)
        {
            dmg += dmg * .1;
        }
        else if (group_bonus(ch, 2) == 12)
        {
            dmg -= dmg * .1;
        }
    }
    else
    {
        /* human racial bonus on hold */
        /*if (IS_HUMAN(ch) && !IS_NPC(ch)) {
   if (PLR_FLAGGED(ch, PLR_TRANS3)) {
    dmg += dmg * 0.45;
   }
  }*/
        dmg = dmg * .6;
        if (group_bonus(ch, 2) == 9)
        {
            dmg -= dmg * 0.1;
        }
        if (AFF_FLAGGED(ch, AFF_POTENT))
        {
            dmg += dmg * 0.3;
            ch->location.sendText("@wThere is a bright flash of @Yyellow@w light in the wake of the attack!@n\r\n");
        }
    }

    if (AFF_FLAGGED(ch, AFF_INFUSE) && !AFF_FLAGGED(ch, AFF_HASS) && type <= 0)
    {
        auto infuse_cost = ch->getMaxVitalPercent(CharVital::ki, .005);
        if (dmg > 0)
        {
            if (ch->getCurVital(CharVital::ki) - infuse_cost)
            {
                ch->modCurVital(CharVital::ki, -infuse_cost);
                ch->location.sendText("@CA swirl of ki explodes from the attack!@n\r\n");
            }
            else
            {
                act("@wYou can no longer infuse ki into your attacks!@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@c$n@w can no longer infuse ki into $s attacks!@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->affect_flags.set(AFF_INFUSE, false);
            }
        }
    }

    if (vict)
    {
        int64_t gain = IS_NPC(ch) ? vict->getExperience() : 0;

        if (vict->location.getVnum() == 17875)
        {
            return;
        }

        reveal_hiding(vict, 0);
        if (AFF_FLAGGED(vict, AFF_PARALYZE))
        {
            ch->sendText("They are a statue and can't be harmed\r\n");
            return;
        }

        if (vict->mutations.get(Mutation::rubbery_body) && type == 0)
        {
            int64_t drain = dmg * 0.1;
            dmg -= drain;
            ch->modCurVital(CharVital::stamina, -drain);
            act("@Y$N's rubbery body makes hitting it tiring!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@Y$n's stamina is sapped a bit by hitting your rubbery body!@n", true, ch, nullptr, vict, TO_VICT);
        }

        dmg *= (1.0 + ch->getAffectModifier(APPLY_COMBAT_MULT, static_cast<int>(ComStat::damage)));
        if (type == 0)
        {
            dmg *= (1.0 + ch->getAffectModifier(APPLY_DTYPE_BON, static_cast<int>(DamType::physical)));
        }
        else
        {
            dmg *= (1.0 + ch->getAffectModifier(APPLY_DTYPE_BON, static_cast<int>(DamType::ki)));
        }

        dmg *= (1.0 + vict->getAffectModifier(APPLY_COMBAT_MULT, static_cast<int>(ComStat::defense)));
        if (type == 0)
        {
            dmg *= (1.0 + vict->getAffectModifier(APPLY_DTYPE_RES, static_cast<int>(DamType::physical)));
        }
        else
        {
            dmg *= (1.0 + vict->getAffectModifier(APPLY_DTYPE_RES, static_cast<int>(DamType::ki)));
        }

        if (type > -1)
        {
            if (LASTATK(ch) != 11 && LASTATK(ch) != 39 && LASTATK(ch) != 500 && LASTATK(ch) < 1000)
            {
                if (handle_combo(ch, vict) > 0)
                {
                    if (beforered <= 1)
                    {
                        ch->setBaseStat<int>("combo", -1);
                        ch->setBaseStat<int>("combo_hits", 0);
                        ch->sendText("@RYou have cut your combo short because you missed your last hit!@n\r\n");
                    }
                    else if (COMBHITS(ch) < physical_mastery(ch))
                    {
                        dmg += combo_damage(ch, dmg, 0);
                        if (COMBHITS(ch) == 10 || COMBHITS(ch) == 20 || COMBHITS(ch) == 30)
                        {
                            int64_t gain = GET_INT(ch) * 10 * ch->getPotential();
                            if (GET_SKILL(ch, SKILL_STYLE) >= 100)
                            {
                                gain += gain * 2;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 80)
                            {
                                gain += gain * 0.4;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 60)
                            {
                                gain += gain * 0.3;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 40)
                            {
                                gain += gain * 0.2;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 20)
                            {
                                gain += gain * 0.1;
                            }
                            giveRandomVital(ch, gain, gain, gain, 2);
                        }
                    }
                    else
                    {
                        dmg += combo_damage(ch, dmg, 1);
                        if (COMBHITS(ch) == 10 || COMBHITS(ch) == 20 || COMBHITS(ch) == 30)
                        {
                            int64_t gain = GET_INT(ch) * 10 * ch->getPotential();
                            if (GET_SKILL(ch, SKILL_STYLE) >= 100)
                            {
                                gain += gain * 2;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 80)
                            {
                                gain += gain * 0.4;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 60)
                            {
                                gain += gain * 0.3;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 40)
                            {
                                gain += gain * 0.2;
                            }
                            else if (GET_SKILL(ch, SKILL_STYLE) >= 20)
                            {
                                gain += gain * 0.1;
                            }
                            giveRandomVital(ch, gain, gain, gain, 5);
                        }
                        ch->setBaseStat<int>("combo", -1);
                        ch->setBaseStat<int>("combo_hits", 0);
                    }
                }
            }
            else if (COMBHITS(ch) > 0 && LASTATK(ch) < 1000)
            {
                ch->sendText("@RYou have cut your combo short because you used the wrong attack!@n\r\n");
                ch->setBaseStat<int>("combo", -1);
                ch->setBaseStat<int>("combo_hits", 0);
            }
        }

        if (LASTATK(ch) >= 1000)
        {
            ch->modBaseStat<int>("last_attack", -1000);
        }

        if (GET_PREFERENCE(ch) == PREFERENCE_KI && GET_CHARGE(ch) > 0)
        {
            dmg -= dmg * 0.08;
        }

        if (AFF_FLAGGED(vict, AFF_SANCTUARY))
        {
            if (GET_SKILL(vict, SKILL_AQUA_BARRIER))
            {
                if (ch->location.getEnvironment(ENV_WATER) < 100.0)
                {
                    dmg = dmg * 0.85;
                }
                else
                {
                    dmg = dmg * 0.75;
                }
            }
            if (GET_BARRIER(vict) - dmg > 0)
            {
                act("@c$N's@C barrier absorbs the damage!@n", true, ch, nullptr, vict, TO_CHAR);
                char barr[MAX_INPUT_LENGTH];
                sprintf(barr, "@CYour barrier absorbs the damage! @D[@B%s@D]@n", add_commas(dmg).c_str());
                act(barr, true, ch, nullptr, vict, TO_VICT);
                act("@c$N's@C barrier absorbs the damage!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->modBaseStat<int64_t>("barrier", -dmg);
                dmg = 0;
            }
            else if (GET_BARRIER(vict) - dmg <= 0)
            {
                dmg -= GET_BARRIER(vict);
                vict->setBaseStat<int64_t>("barrier", 0);
                act("@c$N's@C barrier bursts!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@CYour barrier bursts!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N's@C barrier bursts!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->affect_flags.set(AFF_SANCTUARY, false);
            }
        }
        if (AFF_FLAGGED(vict, AFF_FIRESHIELD) && Random::get<int>(1, 200) < GET_SKILL(vict, SKILL_FIRESHIELD))
        {
            act("@c$N's@C fireshield repels the damage!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@CYour fireshield repels the damage!@n", true, ch, nullptr, vict, TO_VICT);
            act("@c$N's@C fireshield repels the damage!@n", true, ch, nullptr, vict, TO_NOTVICT);
            if (Random::get<int>(1, 3) == 3)
            {
                act("@c$N's@C fireshield disappears...@n", true, ch, nullptr, vict, TO_CHAR);
                act("@CYour fireshield disappears...@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N's@C fireshield disappears...@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->affect_flags.set(AFF_FIRESHIELD, false);
            }
            dmg = 0;
        }

        int64_t conlimit = 2000000000;
        auto vmaxhit = GET_MAX_HIT(vict);
        auto vcon = GET_CON(vict);
        if (type == 0)
        {

            if (vmaxhit < conlimit)
            {
                index += (vmaxhit / 1500) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 2)
            {
                index += (vmaxhit / 2500) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 3)
            {
                index += (vmaxhit / 3500) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 5)
            {
                index += (vmaxhit / 6000) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 10)
            {
                index += (vmaxhit / 8500) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 15)
            {
                index += (vmaxhit / 10000) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 20)
            {
                index += (vmaxhit / 12500) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 25)
            {
                index += (vmaxhit / 16000) * (vcon / 2);
            }
            else if (vmaxhit < conlimit * 30)
            {
                index += (vmaxhit / 22000) * (vcon / 2);
            }
            else if (vmaxhit > conlimit * 30)
            {
                index += (vmaxhit / 25000) * (vcon / 2);
            }
        }

        if (IS_NPC(vict) && GET_LEVEL(vict) > 0)
        {
            index /= 3;
        }
        else if (IS_NPC(vict) && GET_LEVEL(vict) < 40)
        {
            index /= 3;
        }

        index += armor_calc(vict, dmg, type);

        if (AFF_FLAGGED(vict, AFF_STONESKIN))
        {

            if (vcon < 20)
            {
                index += vcon * 250;
            }
            else if (vcon < 30)
            {
                index += vcon * 500;
            }
            else if (vcon < 50)
            {
                index += vcon * 1000;
            }
            else if (vcon < 60)
            {
                index += vcon * 2000;
            }
            else if (vcon < 70)
            {
                index += vcon * 5000;
            }
            else if (vcon < 90)
            {
                index += vcon * 10000;
            }
            else if (vcon <= 100)
            {
                index += vcon * 25000;
            }
        }

        if (AFF_FLAGGED(vict, AFF_SHELL))
        {
            dmg -= dmg * 0.25;
        }

        if (AFF_FLAGGED(vict, AFF_WITHER))
        {
            dmg += (dmg * 0.01) * 20;
        }

        if (!IS_NPC(vict) && GET_COND(vict, DRUNK) > 4)
        {
            dmg -= (dmg * 0.001) * GET_COND(vict, DRUNK);
        }

        if (AFF_FLAGGED(vict, AFF_EARMOR))
        {
            dmg -= dmg * 0.1;
        }

        if (type > 0)
        {
            advanced_energy(vict, dmg);
            dmg -= (dmg * 0.0005) * GET_WIS(vict);
        }

        if (IS_MUTANT(vict))
        {
            if (type <= 0)
            {
                dmg -= dmg * 0.3;
            }
            else if (type > 0)
            {
                dmg -= dmg * 0.25;
            }
        }

        if (GET_BONUS(vict, BONUS_THICKSKIN))
        {
            if (type <= 0)
            {
                dmg -= dmg * 0.20;
            }
            else
            {
                dmg -= dmg * 0.10;
            }
        }
        else if (GET_BONUS(vict, BONUS_THINSKIN))
        {
            if (type <= 0)
            {
                dmg += dmg * 0.20;
            }
            else
            {
                dmg += dmg * 0.10;
            }
        }

        if (PLR_FLAGGED(vict, PLR_FURY))
        {
            dmg -= dmg * 0.1;
        }

        if (IS_MAJIN(vict))
        {
            if (type <= 0)
            {
                dmg -= dmg * 0.5;
            }
        }

        if (IS_KAI(vict))
        {
            dmg += dmg * 0.15;
        }

        if (GRAPPLING(ch) == vict && GRAPTYPE(ch) == 3)
        {
            dmg += (dmg / 100) * 20;
        }

        if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_ARMOR))
        {
            int nanite = GET_SKILL(vict, SKILL_ARMOR), perc = Random::get<int>(1, 220);
            if (vict->subrace == SubRace::android_model_sense)
            {
                perc = Random::get<int>(1, 176);
            }
            if (nanite >= perc)
            {
                std::string amount = "none";
                int mult = 0;
                switch (vict->form)
                {
                case Form::base:
                    mult = 5;
                    amount = "a tiny bit";
                    break;
                case Form::android_1:
                    mult = 10;
                    amount = "a bit";
                    break;
                case Form::android_2:
                    mult = 20;
                    amount = "some";
                    break;
                case Form::android_3:
                    mult = 25;
                    amount = "a good deal";
                    break;
                case Form::android_4:
                    mult = 30;
                    amount = "a lot";
                    break;
                case Form::android_5:
                    mult = 40;
                    amount = "a great deal";
                    break;
                case Form::android_6:
                    mult = 50;
                    amount = "MOST";
                    break;
                }

                auto victMsg = fmt::format("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block {} of the damage!@n", amount);
                auto roomMsg = fmt::format("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block {} of the damage!@n", amount);
                act(victMsg.c_str(), true,
                    vict, nullptr, nullptr, TO_CHAR);
                act(roomMsg.c_str(), true,
                    vict, nullptr, nullptr, TO_ROOM);
                dmg -= (dmg * 0.01) * mult;
            }
        }

        if (!AFF_FLAGGED(vict, AFF_KNOCKED) && (GET_POS(vict) == POS_SITTING || GET_POS(vict) == POS_RESTING) &&
            GET_SKILL(vict, SKILL_ROLL) > axion_dice(0))
        {
            int64_t rollcost = (vict->getEffectiveStat<int64_t>("health") / 300) * (GET_STR(ch) / 2);
            if ((vict->getCurVital(CharVital::stamina)) >= rollcost)
            {
                act("@GYou roll to your feet in an agile fashion!@n", true, vict, nullptr, nullptr, TO_CHAR);
                act("@G$n rolls to $s feet in an agile fashion!@n", true, vict, nullptr, nullptr, TO_ROOM);
                do_stand(vict, nullptr, 0, 0);
                vict->modCurVital(CharVital::stamina, -rollcost);
            }
        }

        if (IS_NPC(vict))
        {
            hitprcnt_mtrigger(vict);
        }

        if (IS_HUMANOID(vict) && !IS_NPC(ch) && IS_NPC(vict) && (!ch->isSparring() || !vict->isSparring()))
        {
            vict->mob_specials.memory.push_back(ch->shared());
        }
        if (IS_NPC(vict) && GET_HIT(vict) > ((vict->getEffectiveStat<int64_t>("health"))) / 4)
        {
            vict->setBaseStat("lasthit", GET_IDNUM(ch));
        }
        if (AFF_FLAGGED(vict, AFF_SLEEP) && Random::get<int>(1, 2) == 2)
        {
            affect_from_char(vict, SPELL_SLEEP);
            act("@c$N@W seems to be more aware now.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@WYou are no longer so sleepy.@n", true, ch, nullptr, vict, TO_VICT);
            act("@c$N@W seems to be more aware now.@n", true, ch, nullptr, vict, TO_NOTVICT);
        }
        if (AFF_FLAGGED(vict, AFF_KNOCKED) && Random::get<int>(1, 12) >= 11)
        {
            vict->cureStatusKnockedOut(true);
            if (IS_NPC(vict) && Random::get<int>(1, 20) >= 12)
            {
                act("@W$n@W stands up.@n", false, vict, nullptr, nullptr, TO_ROOM);
                vict->setBaseStat<int>("position", POS_STANDING);
            }
        }
        if (IS_NPC(vict))
        {
            if (GET_LEVEL(vict) > 10)
            {
                if (dmg - index > 0)
                {
                    dmg -= index;
                }
                else if (dmg - index <= 0 && dmg >= 1)
                {
                    dmg = 1;
                }
            }
            else if (GET_LEVEL(vict) <= 10)
            {
                dmg = (dmg * .8);
            }
        }
        else
        {
            if (dmg >= 1)
            {
                if (dmg * 2 <= index)
                {
                    dmg = dmg * 0.025;
                }
                else if (dmg * 1.8 <= index)
                {
                    dmg = dmg * 0.05;
                }
                else if (dmg * 1.6 <= index)
                {
                    dmg = dmg * 0.1;
                }
                else if (dmg * 1.4 <= index)
                {
                    dmg = dmg * 0.2;
                }
                else if (dmg * 1.2 <= index)
                {
                    dmg = dmg * 0.3;
                }
                else if (dmg <= index)
                {
                    dmg = dmg * 0.4;
                }
                else if (dmg * 0.75 <= index)
                {
                    dmg = dmg * 0.5;
                }
                else if (dmg * 0.5 <= index)
                {
                    dmg = dmg * 0.6;
                }
                else if (dmg * 0.25 <= index)
                {
                    dmg = dmg * 0.7;
                }
                else if (dmg * 0.1 <= index)
                {
                    dmg = dmg * 0.85;
                }
                else
                {
                    dmg -= index;
                }
            }
        }
        if (dmg < 1)
        {
            dmg = 0;
        }
        if (dmg >= 50 && chance > 0)
        {
            hurt_limb(ch, vict, chance, limb, dmg);
        }

        if (IS_NPC(vict) && dmg > vict->getEffectiveStat<int64_t>("health") * .7 && GET_BONUS(ch, BONUS_SADISTIC) > 0)
        {
            gain /= 2;
        }
        else if (IS_NPC(vict) && dmg > vict->getCurVital(CharVital::health) && vict->isFullVital(CharVital::health) * .5 &&
                 GET_BONUS(ch, BONUS_SADISTIC) > 0)
        {
            gain /= 2;
        }

        if (CARRYING(vict) && dmg > (((vict->getEffectiveStat<int64_t>("health"))) * 0.01) && Random::get<int>(1, 10) >= 8)
        {
            carry_drop(vict, 2);
        }

        if (GET_POS(vict) == POS_SITTING && IS_NPC(vict) && vict->getCurVital(CharVital::health) >= ((vict->getEffectiveStat<int64_t>("health"))) * .98)
        {
            do_stand(vict, nullptr, 0, 0);
        }
        auto sup = GET_SUPPRESS(vict);
        bool suppresso = (sup > 0);
        if (ch->isSparring() && vict->isSparring() && (sup + vict->getCurVital(CharVital::health)) - dmg <= 0)
        {
            if (!IS_NPC(vict))
            {
                act("@c$N@w falls down unconscious, and you stop sparring with $M.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@C$n@w stops sparring with you as you fall unconscious.@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N@w falls down unconscious, and @C$n@w stops sparring with $M.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setCurVital(CharVital::health, 1);
                vict->setBaseStat("suppression", 0);
                if (FIGHTING(vict))
                {
                    stop_fighting(vict);
                }
                if (FIGHTING(ch))
                {
                    stop_fighting(ch);
                }
                vict->setBaseStat<int>("position", POS_SLEEPING);
                if (!IS_NPC(ch))
                {
                    vict->affect_flags.set(AFF_KNOCKED, true);
                }
            }
            else
            {
                act("@c$N@w admits defeat to you, stops sparring, and stumbles away.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@c$N@w admits defeat to $n, stops sparring, and stumbles away.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                solo_gain(ch, vict);
                int founded = 0;
                auto con = vict->getInventory();
                for (auto rew : filter_raw(con))
                {
                    rew->clearLocation();
                    rew->moveToLocation(vict);
                    founded = 1;
                }
                if (founded == 1)
                {
                    act("@c$N@w leaves a reward behind out of respect.@n", true, ch, nullptr, vict, TO_CHAR);
                }
                vict->setCurVital(CharVital::health, 0);
                extract_char(vict);
                return;
            }
        }
        else if (ch->isSparring() && (GET_SUPPRESS(vict) + GET_HIT(vict)) - dmg <= 0)
        {
            act("@c$N@w falls down unconscious, and you spare $S life.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@w spares your life as you fall unconscious.@n", true, ch, nullptr, vict, TO_VICT);
            act("@c$N@w falls down unconscious, and @C$n@w spares $S life.@n", true, ch, nullptr, vict, TO_NOTVICT);
            vict->setCurVital(CharVital::health, 1);
            if (FIGHTING(vict))
            {
                stop_fighting(vict);
            }
            if (FIGHTING(ch))
            {
                stop_fighting(ch);
            }
            vict->setBaseStat<int>("position", POS_SLEEPING);
            if (!IS_NPC(ch))
            {
                vict->affect_flags.set(AFF_KNOCKED, true);
            }
        }
        else if (ch->isSparring() && !vict->isSparring() && IS_NPC(ch))
        {
            act("@w$n@w stops sparring!@n", true, ch, nullptr, vict, TO_ROOM);
            ch->character_flags.set(CharacterFlag::sparring, false);
        }
        else if (!ch->isSparring() && vict->isSparring() && IS_NPC(vict))
        {
            act("@w$n@w stops sparring!@n", true, ch, nullptr, vict, TO_ROOM);
            vict->character_flags.set(CharacterFlag::sparring, false);
        }

        if (PLR_FLAGGED(vict, PLR_IMMORTAL) && !ch->isSparring() && vict->getCurVital(CharVital::health) - dmg <= 0)
        {
            if (IN_ARENA(vict))
            {
                send_to_all("@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n", GET_NAME(ch), GET_NAME(vict));
                ch->leaveLocation();
                ch->moveToLocation(17875);
                ch->lookAtLocation();
                vict->leaveLocation();
                vict->moveToLocation(17875);
                vict->setCurVital(CharVital::health, 1);
                vict->lookAtLocation();
                if (FIGHTING(vict))
                {
                    stop_fighting(vict);
                }
                if (FIGHTING(ch))
                {
                    stop_fighting(ch);
                }
                return;
            }
            else
            {
                act("@c$N@w disappears right before dying. $N appears to be immortal.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@CYou disappear right before death, having been saved by your immortality.@n", true, ch, nullptr,
                    vict, TO_VICT);
                act("@c$N@w disappears right before dying. $N appears to be immortal.@n.", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->modCurVitalDam(CharVital::health, 1);
                vict->modCurVitalDam(CharVital::stamina, 1);
                vict->modCurVitalDam(CharVital::ki, 1);

                if (FIGHTING(vict))
                {
                    stop_fighting(vict);
                }
                if (FIGHTING(ch))
                {
                    stop_fighting(ch);
                }
                vict->setBaseStat<int>("position", POS_SITTING);
                vict->leaveLocation();
                vict->moveToLocation(sensei::getStartRoom(vict->sensei));
            }
            return;
        }

        if (GRAPPLING(vict) && GRAPPLING(vict) != ch && type == 1)
        {
            act("@YThe attack hurts YOU as well because you are grappling with $M!@n", true, vict, nullptr,
                GRAPPLING(vict), TO_VICT);
            act("@YThe attack hurts @y$N@Y as well because $n is grappling with $m!@n", true, vict, nullptr,
                GRAPPLING(vict), TO_NOTVICT);
            maindmg = maindmg / 2;
            hurt(0, 0, ch, GRAPPLING(vict), nullptr, maindmg, 3);
        }
        if (GRAPPLED(vict) && GRAPPLED(vict) != ch && type == 1)
        {
            act("@YThe attack hurts YOU as well because you are being grappled by $M!@n", true, vict, nullptr,
                GRAPPLED(vict), TO_VICT);
            act("@YThe attack hurts @y$N@Y as well because $n is being grappled by $m!@n", true, vict, nullptr,
                GRAPPLED(vict), TO_NOTVICT);
            maindmg = maindmg / 2;
            hurt(0, 0, ch, GRAPPLED(vict), nullptr, maindmg, 3);
        }
        bool deathblow = (GET_HIT(vict) - dmg <= 0 && suppresso == false) || (suppresso == true && vict->getPL(false) - dmg <= 0);
        if (!ch->isSparring() && !PLR_FLAGGED(vict, PLR_IMMORTAL) && deathblow)
        {
            vict->modCurVitalDam(CharVital::health, 1);
            int64_t lifeloss;
            if (suppresso)
                lifeloss = dmg - vict->getPL(false);
            else
                lifeloss = dmg - GET_HIT(vict);

            if (!IS_NPC(vict) && lifeloss <= vict->getCurVital(CharVital::lifeforce))
            {
                act("@c$N@w barely clings to life!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@CYou barely cling to life!@n", true, ch, nullptr, vict, TO_VICT);
                act("@c$N@w barely clings to life!@n.", true, ch, nullptr, vict, TO_NOTVICT);
                vict->modCurVital(CharVital::lifeforce, -lifeloss);
                vict->send_to("@D[@CLifeforce@D: @R-%s@D]\n", add_commas(lifeloss).c_str());
                if ((vict->getCurVital(CharVital::lifeforce)) >= (vict->getEffectiveStat("lifeforce")) * 0.05)
                {
                    vict->sendText("@YYou recover a bit thanks to your strong life force.@n\r\n");
                    vict->modCurVital(CharVital::health, (vict->getEffectiveStat("lifeforce")) * .05);
                    vict->modCurVitalDam(CharVital::lifeforce, .05);
                }
                else
                {
                    vict->modCurVital(CharVital::health, GET_CON(vict) * 100);
                }
                vict->attemptLimitBreak();
                return;
            }
            if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
            {
                vict->setBaseStat("death_type", 0);
            }
            if (type <= 0 && (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY)))
            {
                handle_death_msg(ch, vict, 0);
            }
            else if (type > 0 && (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY)))
            {
                handle_death_msg(ch, vict, 1);
            }
            else
            {
                act("@R$N@w self destructs with a mild explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@R$N@w self destructs with a mild explosion!@n", true, ch, nullptr, vict, TO_ROOM);
            }
            if (dmg > 1)
            {
                if (type <= 0 && GET_HIT(ch) >= ((ch->getEffectiveStat<int64_t>("health")) * 0.5))
                {
                    int64_t raise = (GET_MAX_MANA(ch) * 0.005) + 1;
                    ch->modCurVital(CharVital::ki, raise);
                }
                ch->send_to("@D[@GDamage@W: @R%s@D]@n\r\n", add_commas(dmg).c_str());
                vict->send_to("@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg).c_str());
                int64_t healhp = (long double)(GET_MAX_HIT(vict)) * 0.12;
                if (ch->technique == Form::dark_metamorphosis)
                {
                    act("@RYour dark aura saps some of @r$N's@R life energy!@n", true, ch, nullptr, vict, TO_CHAR);
                    act("@r$n@R's dark aura saps some of your life energy!@n", true, ch, nullptr, vict, TO_VICT);
                    if (GET_HIT(ch) <= GET_MAX_HIT(ch))
                    {
                        ch->modCurVital(CharVital::health, healhp);
                    }
                    ch->modBaseStat<int64_t>("charge", GET_MAX_MANA(vict) * 0.12);
                }
                if (ch->mutations.get(Mutation::natural_energy))
                {
                    ch->modCurVital(CharVital::ki, dmg * .05);
                }
                if (!ch->isSparring() && IS_NPC(vict))
                {
                    if (type == 0 && Random::get<int>(1, 100) >= 97)
                    {
                        ch->sendText("@YY@yo@Yu @yg@Ya@yi@Yn@y s@Yo@ym@Ye @yb@Yo@yn@Yu@ys @Ye@yx@Yp@ye@Yr@yi@Ye@yn@Yc@ye@Y!@n\r\n");
                        gain = gain * 0.05;
                        gain += 1;
                        ch->modExperience(gain);
                    }
                    else if (type != 0 && Random::get<int>(1, 100) >= 93)
                    {
                        gain = gain * 0.05;
                        gain += 1;
                        ch->modExperience(gain);
                    }
                }
                if (AFF_FLAGGED(vict, AFF_ECHAINS))
                {
                    if (type == 0)
                    {
                        if (IS_NPC(ch))
                        {
                            assign_affect(ch, AFF_ECHAINS_DEBUFF, SKILL_RUNIC, -1, 0, 0, 0, 0, 0, -2);
                        }
                        else
                        {
                            WAIT_STATE(ch, PULSE_3SEC);
                        }
                        act("@CEthereal chains burn into existence! They quickly latch onto @RYOUR@C body and begin temporarily hampering $s actions!@n",
                            true, ch, nullptr, vict, TO_CHAR);
                        act("@CEthereal chains burn into existence! They quickly latch onto @c$n's@C body and begin temporarily hampering $s actions!@n",
                            true, ch, nullptr, vict, TO_ROOM);
                    }
                }
            }
            else if (dmg <= 1)
            {
                ch->sendText("@D[@GDamage@W: @BPitiful...@D]@n\r\n");
                vict->sendText("@D[@rDamage@W: @BPitiful...@D]@n\r\n");
            }

            vict->modCurVitalDam(CharVital::health, 1);

            if (AFF_FLAGGED(ch, AFF_GROUP))
            {
                group_gain(ch, vict);
            }
            else
            {
                solo_gain(ch, vict);
            }
            if (IS_DEMON(ch) && type == 1)
            {
                vict->affect_flags.set(AFF_ASHED, true);
            }
            die(vict, ch);
            dead = true;
        }
        else if (GET_HIT(vict) - dmg > 0 || suppresso == true)
        {
            if (suppresso == false)
            {
                vict->modCurVital(CharVital::health, -dmg);
            }
            if (FIGHTING(ch) == nullptr)
            {
                set_fighting(ch, vict);
            }
            else if (FIGHTING(ch) != vict)
            {
                set_fighting(ch, vict);
            }
            if (FIGHTING(vict) == nullptr)
            {
                set_fighting(vict, ch);
            }
            else if (FIGHTING(vict) != ch)
            {
                set_fighting(vict, ch);
            }
            if (dmg > 1 && suppresso == false)
            {
                if (type == 0 && GET_HIT(ch) >= ((ch->getEffectiveStat<int64_t>("health")) * 0.5))
                {
                    int64_t raise = (GET_MAX_MANA(ch) * 0.005) + 1;
                    ch->modCurVital(CharVital::ki, raise);
                }
                if (ch->mutations.get(Mutation::natural_energy))
                {
                    ch->modCurVital(CharVital::ki, dmg * .05);
                }
                ch->send_to("@D[@GDamage@W: @R%s@D]@n", add_commas(dmg).c_str());
                vict->send_to("@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg).c_str());
                // int64_t healhp = GET_HIT(vict) * 0.12;
                if (auto scouter = GET_EQ(ch, WEAR_EYE); scouter && scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER) && vict && !PRF_FLAGGED(ch, PRF_NODEC))
                {
                    if (IS_ANDROID(vict))
                    {
                        ch->sendText(" @D<@YProcessing@D: @c?????????????@D>@n\r\n");
                    }
                    else if (vict->getPL() >= scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER))
                    {
                        ch->sendText(" @D<@YProcessing@D: @c?????????????@D>@n\r\n");
                    }
                    else
                    {
                        ch->send_to(" @D<@YProcessing@D: @c%s - @r%s%%@D>@n\r\n", add_commas(vict->getPL()).c_str(), std::to_string((int)((1.0 - vict->getCurVitalDam(CharVital::health)) * 100)));
                    }
                }
                else
                {
                    ch->sendText("\r\n");
                }
            }
            else
            { // if (!IS_NPC(ch)) {
                if (dmg <= 1 && suppresso == false && !PRF_FLAGGED(ch, PRF_NODEC))
                {
                    ch->sendText("@D[@GDamage@W: @BPitiful...@D]@n");
                    vict->sendText("@D[@rDamage@W: @BPitiful...@D]@n\r\n");
                    if (auto scouter = GET_EQ(ch, WEAR_EYE); scouter && scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER) && vict && !PRF_FLAGGED(ch, PRF_NODEC))
                    {
                        auto vpl = vict->getPL();
                        if (IS_ANDROID(vict) || vpl > scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER))
                        {
                            ch->sendText(" @D<@YProcessing@D: @c?????????????@D>@n\r\n");
                        }
                        else
                        {
                            ch->send_to(" @D<@YProcessing@D: @c%s - @r%s%%@D>@n\r\n", add_commas(vpl).c_str(), std::to_string((int)((1.0 - vict->getCurVitalDam(CharVital::health)) * 100)));
                        }
                    }
                    else
                    {
                        ch->sendText("\r\n");
                    }
                }
                else if (dmg > 1 && suppresso == true && !PRF_FLAGGED(ch, PRF_NODEC))
                {
                    double percentageDamage = (double)dmg / (double)vict->getEffectiveStat<int64_t>("health");
                    int64_t calcdamage = GET_HIT(vict) * percentageDamage;

                    ch->send_to("@D[@GDamage@W: @R%s@D]@n", add_commas(dmg).c_str());
                    vict->send_to("@D[@rDamage@W: @R%s @c-Suppression-@D]@n\r\n", add_commas(calcdamage).c_str());
                    // int64_t healhp = GET_HIT(vict) * 0.12;
                    // Translate the damage into a percentage of max LF, remove that from the player instead

                    vict->modCurVital(CharVital::health, -calcdamage);

                    if (auto scouter = GET_EQ(ch, WEAR_EYE); scouter && scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER) && vict && !PRF_FLAGGED(ch, PRF_NODEC))
                    {
                        auto vpl = vict->getPL();
                        if (IS_ANDROID(vict) || vpl >= scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER))
                        {
                            ch->sendText(" @D<@YProcessing@D: @c?????????????@D>@n\r\n");
                        }
                        else
                        {
                            ch->send_to(" @D<@YProcessing@D: @c%s - @r%s%%@D>@n\r\n", add_commas(vpl).c_str(), std::to_string((int)((1.0 - vict->getCurVitalDam(CharVital::health)) * 100)));
                        }
                    }
                    else
                    {
                        ch->sendText("\r\n");
                    }
                }
                else if (dmg <= 1 && suppresso == true && !PRF_FLAGGED(ch, PRF_NODEC))
                {
                    ch->sendText("@D[@GDamage@W: @BPitiful...@D]@n");
                    if (auto scouter = GET_EQ(ch, WEAR_EYE); scouter && scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER) && vict && !PRF_FLAGGED(ch, PRF_NODEC))
                    {
                        auto vpl = vict->getPL();
                        if (IS_ANDROID(vict) || vpl >= scouter->getBaseStat<int64_t>(VAL_WORN_SCOUTER))
                        {
                            ch->sendText(" @D<@YProcessing@D: @c?????????????@D>@n\r\n");
                        }
                        else
                        {
                            ch->send_to(" @D<@YProcessing@D: @c%s - @r%s%%@D>@n\r\n", add_commas(vpl).c_str(), std::to_string((int)((1.0 - vict->getCurVitalDam(CharVital::health)) * 100)));
                        }
                    }
                    else
                    {
                        ch->sendText("\r\n");
                    }
                }
            }
        }
        if (GET_SKILL(ch, SKILL_FOCUS) && type == 1)
        {
            improve_skill(ch, SKILL_FOCUS, 1);
        }

        /* Increases GET_FURY for halfbreeds who get damaged. */

        if (!ch->isSparring() && IS_HALFBREED(vict) && GET_FURY(vict) < 100 && !PLR_FLAGGED(vict, PLR_FURY))
        {
            vict->sendText("@RYour fury increases a little bit!@n\r\n");
            vict->modBaseStat("fury", 1);
        }

        /* Ends GET_FURY increase for halfbreeds who got damaged */

        if (LASTATK(ch) != -1)
        {
            spar_gain(ch, vict, type, dmg);
        }

        if (dmg > 0 && !IS_NPC(ch))
        {
            if ((IS_SAIYAN(ch) || ch->bio_genomes.get(Race::saiyan)))
            {
                if (GET_POS(ch) != POS_RESTING && GET_POS(vict) != POS_RESTING)
                {
                    saiyan_gain(ch, vict);
                }
            }
        }

        if (IS_ARLIAN(vict) && dead != true && !vict->isSparring() && !ch->isSparring())
        {
            handle_evolution(vict, dmg);
        }
        if (dead == true)
        {
            char corp[256];
            if (!PLR_FLAGGED(ch, PLR_SELFD2))
            {
                if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
                {
                    sprintf(corp, "all.zenni corpse");
                    do_get(ch, corp, 0, 0);
                }
                if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
                {
                    sprintf(corp, "all corpse");
                    do_get(ch, corp, 0, 0);
                }
            }
        }
    }
    /* If an object is targeted */
    else if (obj)
    {
        switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL))
        {
        case MATERIAL_STEEL:
            dmg = dmg / 4;
            break;
        case MATERIAL_MITHRIL:
            dmg = dmg / 6;
            break;
        case MATERIAL_IRON:
            dmg = dmg / 3;
            break;
        case MATERIAL_STONE:
            dmg = dmg / 2;
            break;
        case MATERIAL_DIAMOND:
            dmg = dmg / 10;
            break;
        }
        if (dmg <= 0)
        {
            dmg = 1;
        }
        if (OBJ_FLAGGED(obj, ITEM_UNBREAKABLE))
        {
            act("$p@w seems unaffected.@n", true, ch, obj, nullptr, TO_CHAR);
            act("$p@w seems unaffected.@n", true, ch, obj, nullptr, TO_ROOM);
        }
        else if (GET_OBJ_VNUM(obj) == 79)
        {
            if (GET_OBJ_WEIGHT(obj) - dmg > 0)
            {
                if (type <= 0)
                {
                    if (AFF_FLAGGED(ch, AFF_INFUSE))
                        dmg *= 10;
                    act("$p@w cracks some.@n", true, ch, obj, nullptr, TO_CHAR);
                    act("$p@w cracks some.@n", true, ch, obj, nullptr, TO_ROOM);
                    obj->modBaseStat<weight_t>("weight", -dmg);
                    if (GET_FELLOW_WALL(obj))
                    {
                        Object *wall;
                        wall = GET_FELLOW_WALL(obj);
                        wall->modBaseStat<weight_t>("weight", -dmg);
                        act("$p@w cracks some. A humanoid shadow can be seen moving on the other side.@n", true,
                            nullptr, obj, nullptr, TO_ROOM);
                    }
                }
                else
                {
                    dmg *= 30;
                    act("$p@w melts some.@n", true, ch, obj, nullptr, TO_CHAR);
                    act("$p@w melts some.@n", true, ch, obj, nullptr, TO_ROOM);
                    obj->modBaseStat<weight_t>("weight", -dmg);
                    if (GET_FELLOW_WALL(obj))
                    {
                        Object *wall;
                        wall = GET_FELLOW_WALL(obj);
                        wall->modBaseStat<weight_t>("weight", -dmg);
                        act("$p@w melts some.@n", true, ch, obj, nullptr, TO_ROOM);
                    }
                }
            }
            else
            {
                if (type <= 0)
                {
                    act("$p@w breaks completely apart and then melts away.@n", true, ch, obj, nullptr, TO_CHAR);
                    act("$p@w breaks completely apart and then melts away.@n", true, ch, obj, nullptr, TO_ROOM);
                    extract_obj(obj);
                }
                else
                {
                    act("$p@w is blown away into snow and water!@n", true, ch, obj, nullptr, TO_CHAR);
                    act("$p@w is blown away into snow and water!@n", true, ch, obj, nullptr, TO_ROOM);
                    extract_obj(obj);
                }
            }
        }
        else if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) - dmg > 0)
        {
            act("$p@w cracks some.@n", true, ch, obj, nullptr, TO_CHAR);
            act("$p@w cracks some.@n", true, ch, obj, nullptr, TO_ROOM);
            MOD_OBJ_VAL(obj, VAL_ALL_HEALTH, -dmg);
        }
        else
        {
            if (type <= 0)
            {
                act("$p@w shatters apart!@n", true, ch, obj, nullptr, TO_CHAR);
                act("$p@w shatters apart!@n", true, ch, obj, nullptr, TO_ROOM);
                SET_OBJ_VAL(obj, VAL_ALL_HEALTH, 0);
                obj->item_flags.set(ITEM_BROKEN, true);
                if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON && GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)
                {
                    SET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL, 0);
                }
            }
            else if (type != 0)
            {
                act("$p@w is disintegrated!@n", true, ch, obj, nullptr, TO_CHAR);
                act("$p@w is disintegrated!@n", true, ch, obj, nullptr, TO_ROOM);
                extract_obj(obj);
            }
        }
    }
    else
    {
        basic_mud_log("Log: Error with hurt.\n");
    }
}

/* This handles the length of time between attacks and other actions *
 * players AND non-players will have to endure. Allowing for a more   *
 * balanced and interesting attack cooldown system. - Iovan 2/25/2011 */
void handle_cooldown(Character *ch, int cooldown)
{

    /* Let's clear any cooldown they may accidently have so it doesn't stack *
     * This is only for NPCs as player cooldown is handled through the stock *
     * descriptor_list command interpreter. This also initializes cooldown   */

    if (!IS_NPC(ch))
    {
        if (PLR_FLAGGED(ch, PLR_MULTIHIT))
        {
            ch->player_flags.set(PLR_MULTIHIT, false);
            return;
        }
    }

    reveal_hiding(ch, 0);
    int waitCalc = 10, base = cooldown;
    int64_t cspd = GET_SPEEDI(ch);

    if (cspd > 10000000)
    { /* WTF Fast */
        waitCalc -= 9;
    }
    else if (cspd > 5000000)
    {
        waitCalc -= 8;
    }
    else if (cspd > 2500000)
    {
        waitCalc -= 7;
    }
    else if (cspd > 1000000)
    {
        waitCalc -= 6;
    }
    else if (cspd > 500000)
    {
        waitCalc -= 5;
    }
    else if (cspd > 100000)
    {
        waitCalc -= 4;
    }
    else if (cspd > 50000)
    {
        waitCalc -= 3;
    }
    else if (cspd > 25000)
    {
        waitCalc -= 2;
    }
    else if (cspd > 50)
    {
        waitCalc -= 1;
    }

    base *= 10;

    if (base >= 100)
    {
        base = 30;
    }
    else if (base >= 70)
    {
        base = 20;
    }
    else if (base >= 30)
    {
        base = 10;
    }

    /* Alright now let's determine the cooldown based on the wait and cooldown assigned *
     * by the attack which called handle_cooldown.                                      */
    cooldown *= waitCalc;
    cooldown += base;
    if (cooldown <= 0)
    { /* Can't have this. */
        cooldown = 10;
    }
    if (cooldown >= 120)
    {
        WAIT_STATE(ch, PULSE_CD12);
    }
    else if (cooldown >= 110)
    {
        WAIT_STATE(ch, PULSE_CD11);
    }
    else if (cooldown >= 100)
    {
        WAIT_STATE(ch, PULSE_CD10);
    }
    else if (cooldown >= 90)
    {
        WAIT_STATE(ch, PULSE_CD9);
    }
    else if (cooldown >= 80)
    {
        WAIT_STATE(ch, PULSE_CD8);
    }
    else if (cooldown >= 70)
    {
        WAIT_STATE(ch, PULSE_CD7);
    }
    else if (cooldown >= 60)
    {
        WAIT_STATE(ch, PULSE_CD6);
    }
    else if (cooldown >= 50)
    {
        WAIT_STATE(ch, PULSE_CD5);
    }
    else if (cooldown >= 40)
    {
        WAIT_STATE(ch, PULSE_CD4);
    }
    else if (cooldown >= 30)
    {
        WAIT_STATE(ch, PULSE_CD3);
    }
    else if (cooldown >= 20)
    {
        WAIT_STATE(ch, PULSE_CD2);
    }
    else
    {
        WAIT_STATE(ch, PULSE_CD1);
    }
}

/* This handles whether parry is turned on */
int handle_parry(Character *ch)
{

    if (axion_dice(0) <= 4)
    { /* Critical failure */
        return (1);
    }

    if (IS_NPC(ch))
    {

        /*  Non-humanoids are only rarely capable of parrying against weak players. Never
         * against strong players. Humanoids get progressively better at parry the higher
         * level they are.
         **/
        if (!IS_HUMANOID(ch))
        {
            return (Random::get<int>(0, 5));
        }
        else
        {
            if (GET_LEVEL(ch) >= 110)
            {
                return (Random::get<int>(90, 105));
            }
            else if (GET_LEVEL(ch) >= 100)
            {
                return (Random::get<int>(85, 95));
            }
            else if (GET_LEVEL(ch) >= 80)
            {
                return (Random::get<int>(70, 85));
            }
            else if (GET_LEVEL(ch) >= 40)
            {
                return (Random::get<int>(50, 70));
            }
            else
            {
                int top = GET_LEVEL(ch);

                if (top < 15)
                    top = 16;
                return (Random::get<int>(15, top));
            }
        }
    }

    if (PRF_FLAGGED(ch, PRF_NOPARRY))
    {
        return (-2);
    }
    else
    {
        int num = GET_SKILL(ch, SKILL_PARRY);
        if (ch->mutations.get(Mutation::extreme_reflexes))
        {
            num += 10;
        }
        if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100)
        {
            num += 5;
        }
        else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 80)
        {
            num += 4;
        }
        else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60)
        {
            num += 3;
        }
        else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40)
        {
            num += 2;
        }
        else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 20)
        {
            num += 1;
        }
        return (num);
    }
}

/* This handles whether a step of the combo was preformed. */
int handle_combo(Character *ch, Character *vict)
{
    int success = false, pass = false;

    if (IS_NPC(ch))
        return 0;

    switch (LASTATK(ch))
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 51:
    case 52:
    case 56:
        pass = true;
        break;
    default:
        if (COMBO(ch) != -1)
        {
            ch->sendText("@RYou have cut your combo short with the wrong attack!@n\r\n");
        }
        ch->setBaseStat<int>("combo", -1);
        ch->setBaseStat<int>("combo_hits", 0);
        pass = false;
        break;
    }

    if (pass == false)
        return 0;

    if (count_physical(ch) < 3)
        return 0;

    int chance;
    int speedPercentage = ((double)(GET_SPEEDI(ch)) / (double)GET_SPEEDI(vict)) * (double)100;
    if (speedPercentage < 1)
    {
        chance = 1;
    }
    else if (speedPercentage > 100)
    {
        chance = 100;
    }
    else
    {
        chance = speedPercentage;
    }
    chance = 100 - chance;
    if (chance < 1)
    {
        chance = 1;
    }
    chance += 25;
    if (LASTATK(ch) == 0 || LASTATK(ch) == 1)
    {
        chance -= 10;
    }
    if (LASTATK(ch) == 2 || LASTATK(ch) == 3)
    {
        chance -= 5;
    }

    if (COMBO(ch) <= -1 && Random::get<int>(1, 100) > chance)
    {
        while (success == false)
        {
            switch (Random::get<int>(1, 24))
            {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                if (GET_SKILL(ch, SKILL_PUNCH) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try a@R punch @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 0);
                    success = true;
                }
                break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
                if (GET_SKILL(ch, SKILL_KICK) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try a@R kick @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 1);
                    success = true;
                }
                break;
            case 11:
            case 12:
            case 13:
            case 14:
                if (GET_SKILL(ch, SKILL_ELBOW) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try an@R elbow @Gnext!@n\r\n");
                    ch->setBaseStat("combo", 2);
                    success = true;
                }
                break;
            case 15:
            case 16:
            case 17:
                if (GET_SKILL(ch, SKILL_KNEE) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try a@R knee @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 3);
                    success = true;
                }
                break;
            case 18:
            case 19:
                if (GET_SKILL(ch, SKILL_ROUNDHOUSE) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try a@R roundhouse @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 4);
                    success = true;
                }
                break;
            case 20:
            case 21:
                if (GET_SKILL(ch, SKILL_UPPERCUT) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try an@R uppercut @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 5);
                    success = true;
                }
                break;
            case 22:
                if (GET_SKILL(ch, SKILL_HEELDROP) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try a@R heeldrop @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 8);
                    success = true;
                }
                break;
            case 24:
                if (GET_SKILL(ch, SKILL_SLAM) > 0)
                {
                    ch->sendText("@GYou have a chance for a COMBO! Try a@R slam @Gnext!@n\r\n");
                    ch->setBaseStat<int>("combo", 6);
                    success = true;
                }
                break;
            }
        }
        return 0;
    }
    else if (LASTATK(ch) == COMBO(ch) && COMBHITS(ch) < physical_mastery(ch))
    {
        ch->modBaseStat<int>("combo_hits", 1);
        while (success == false)
        {
            if (COMBHITS(ch) >= 20)
            { /* We're kicking ass! */
                switch (Random::get<int>(1, 34))
                {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    if (GET_SKILL(ch, SKILL_ELBOW) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 2);
                        success = true;
                    }
                    break;
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 16:
                    if (GET_SKILL(ch, SKILL_KNEE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 3);
                        success = true;
                    }
                    break;
                case 17:
                case 18:
                case 19:
                case 20:
                case 21:
                    if (GET_SKILL(ch, SKILL_UPPERCUT) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 5);
                        success = true;
                    }
                    break;
                case 22:
                case 23:
                case 24:
                case 25:
                case 26:
                    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 4);
                        success = true;
                    }
                    break;
                case 27:
                case 28:
                case 29:
                    if (GET_SKILL(ch, SKILL_BASH) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 51);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 56);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 52);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_SLAM) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 6);
                        success = true;
                    }
                    break;
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                    if (GET_SKILL(ch, SKILL_BASH) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 51);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 56);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 52);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEELDROP) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 8);
                        success = true;
                    }
                    break;
                } /* Switch End */
            }
            else if (COMBHITS(ch) >= 15)
            { /* We're doing good! */
                switch (Random::get<int>(1, 36))
                {
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
                    if (GET_SKILL(ch, SKILL_ELBOW) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 2);
                        success = true;
                    }
                    break;
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
                    if (GET_SKILL(ch, SKILL_KNEE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 3);
                        success = true;
                    }
                    break;
                case 21:
                case 22:
                case 23:
                    if (GET_SKILL(ch, SKILL_PUNCH) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 0);
                        success = true;
                    }
                    break;
                case 25:
                case 26:
                case 27:
                    if (GET_SKILL(ch, SKILL_KICK) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 1);
                        success = true;
                    }
                    break;
                case 29:
                case 30:
                    if (GET_SKILL(ch, SKILL_UPPERCUT) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 5);
                        success = true;
                    }
                    break;
                case 31:
                case 32:
                case 33:
                case 34:
                    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 4);
                        success = true;
                    }
                    break;
                case 35:
                    if (GET_SKILL(ch, SKILL_BASH) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 51);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 56);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 52);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_SLAM) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 6);
                        success = true;
                    }
                    break;
                case 36:
                    if (GET_SKILL(ch, SKILL_BASH) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 51);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 56);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 52);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEELDROP) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 8);
                        success = true;
                    }
                    break;
                }
            }
            else if (COMBHITS(ch) >= 10)
            { /* We're on a roll */
                switch (Random::get<int>(1, 34))
                {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    if (GET_SKILL(ch, SKILL_ELBOW) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 2);
                        success = true;
                    }
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                    if (GET_SKILL(ch, SKILL_KNEE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 3);
                        success = true;
                    }
                    break;
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 16:
                case 17:
                case 18:
                    if (GET_SKILL(ch, SKILL_PUNCH) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 0);
                        success = true;
                    }
                    break;
                case 19:
                case 20:
                case 21:
                case 22:
                case 23:
                case 24:
                case 25:
                case 26:
                    if (GET_SKILL(ch, SKILL_KICK) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 1);
                        success = true;
                    }
                    break;
                case 27:
                case 28:
                case 29:
                    if (GET_SKILL(ch, SKILL_UPPERCUT) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 5);
                        success = true;
                    }
                    break;
                case 30:
                case 31:
                    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 4);
                        success = true;
                    }
                    break;
                case 32:
                case 33:
                    if (GET_SKILL(ch, SKILL_BASH) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 51);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 56);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 52);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_SLAM) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 6);
                        success = true;
                    }
                    break;
                case 34:
                    if (GET_SKILL(ch, SKILL_BASH) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 51);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 56);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && Random::get<int>(1, 2) == 2)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 52);
                        success = true;
                    }
                    else if (GET_SKILL(ch, SKILL_HEELDROP) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 8);
                        success = true;
                    }
                    break;
                }
            }
            else if (COMBHITS(ch) >= 5)
            { /* We're staring off well */
                switch (Random::get<int>(1, 30))
                {
                case 1:
                case 2:
                case 3:
                case 4:
                    if (GET_SKILL(ch, SKILL_ELBOW) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 2);
                        success = true;
                    }
                    break;
                case 5:
                case 6:
                case 7:
                case 8:
                    if (GET_SKILL(ch, SKILL_KNEE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 3);
                        success = true;
                    }
                    break;
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
                    if (GET_SKILL(ch, SKILL_PUNCH) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 0);
                        success = true;
                    }
                    break;
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
                    if (GET_SKILL(ch, SKILL_KICK) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 1);
                        success = true;
                    }
                    break;
                case 29:
                    if (GET_SKILL(ch, SKILL_UPPERCUT) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 5);
                        success = true;
                    }
                    break;
                case 30:
                    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 4);
                        success = true;
                    }
                    break;
                }
            }
            else
            { /* We just the combo not long ago */
                switch (Random::get<int>(1, 30))
                {
                case 1:
                case 2:
                case 3:
                    if (GET_SKILL(ch, SKILL_ELBOW) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 2);
                        success = true;
                    }
                    break;
                case 4:
                case 5:
                case 6:
                    if (GET_SKILL(ch, SKILL_KNEE) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 3);
                        success = true;
                    }
                    break;
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
                    if (GET_SKILL(ch, SKILL_PUNCH) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 0);
                        success = true;
                    }
                    break;
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
                    if (GET_SKILL(ch, SKILL_KICK) > 0)
                    {
                        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", COMBHITS(ch));
                        ch->setBaseStat("combo", 1);
                        success = true;
                    }
                    break;
                }
            } /* End continued hits section */
        }
        return COMBHITS(ch);
    }
    else if (LASTATK(ch) == COMBO(ch) && COMBHITS(ch) >= physical_mastery(ch))
    {
        ch->modBaseStat<int>("combo_hits", 1);
        ch->send_to("@D(@GC-c-combo Bonus @gx%d@G!@D)@C Combo FINISHED for massive damage@G!@n\r\n", COMBHITS(ch));
    }
    else if (COMBO(ch) != LASTATK(ch) && COMBO(ch) > -1)
    {
        ch->sendText("@GCombo failed!@n\r\n");
        ch->setBaseStat<int>("combo", -1);
        ch->setBaseStat<int>("combo_hits", 0);
        return 0;
    }
    else
    {
        ch->setBaseStat<int>("combo", -1);
        ch->setBaseStat<int>("combo_hits", 0);
        return 0;
    }
    return 0;
}

void handle_spiral(Character *ch, Character *vict, int skill, int first)
{
    int prob, perc, avo, index, pry = 2, dge = 2, blk = 2;
    int64_t dmg;
    double amount = 0.0;

    if (first == false)
    {
        amount = 0.05;
    }
    else
    {
        amount = 0.5;
    }

    if (vict == nullptr && FIGHTING(ch))
    {
        vict = FIGHTING(ch);
    }
    else if (vict == nullptr)
    {
        act("@WHaving lost your target you slow down until your vortex disappears, and end your attack.@n", true, ch,
            nullptr, nullptr, TO_CHAR);
        act("@C$n@W slows down until $s vortex disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->player_flags.set(PLR_SPIRAL, false);
        return;
    }

    if (GET_CHARGE(ch) <= 0)
    {
        act("@WHaving no more charged ki you slow down until your vortex disappears, and end your attack.@n", true, ch,
            nullptr, nullptr, TO_CHAR);
        act("@C$n@W slows down until $s vortex disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->player_flags.set(PLR_SPIRAL, false);
        return;
    }

    if (vict)
    {
        index = check_def(vict); /* Check parry/block/dodge of vict */

        prob = skill;
        perc = axion_dice(0);

        index -= handle_speed(ch, vict);

        avo = index / 4;

        handle_defense(vict, &pry, &blk, &dge);

        if (avo > 0 && avo < 70)
        {
            prob -= avo;
        }
        else if (avo >= 70)
        {
            prob -= 69;
        }
        tech_handle_posmodifier(vict, pry, blk, dge, prob);

        if (!tech_handle_zanzoken(ch, vict, "Spiral Comet Blast"))
        {
            pcost(ch, amount, 0);
            pcost(vict, 0, GET_MAX_HIT(vict) / 200);
            return;
        }

        if (prob < perc)
        {
            if ((vict->getCurVital(CharVital::stamina)) > 0)
            {
                if (blk > Random::get<int>(1, 130))
                {
                    act("@C$N@W moves quickly and blocks your Spiral Comet blast!@n", false, ch, nullptr, vict,
                        TO_CHAR);
                    act("@WYou move quickly and block @C$n's@W Spiral Comet blast!@n", false, ch, nullptr, vict,
                        TO_VICT);
                    act("@C$N@W moves quickly and blocks @c$n's@W Spiral Comet blast!@n", false, ch, nullptr, vict,
                        TO_NOTVICT);
                    pcost(ch, amount, 0);
                    dmg = damtype(ch, 10, skill, .05);
                    dmg /= 4;
                    hurt(0, 0, ch, vict, nullptr, dmg, 1);
                    return;
                }
                else if (dge > Random::get<int>(1, 130))
                {
                    act("@C$N@W manages to dodge your Spiral Comet blast, letting it slam into the surroundings!@n",
                        false, ch, nullptr, vict, TO_CHAR);
                    act("@WYou dodge @C$n's@W Spiral Comet blast, letting it slam into the surroundings!@n", false, ch,
                        nullptr, vict, TO_VICT);
                    act("@C$N@W manages to dodge @c$n's@W Spiral Comet blast, letting it slam into the surroundings!@n",
                        false, ch, nullptr, vict, TO_NOTVICT);
                    vict->location.sendText("@wA bright explosion erupts from the impact!\r\n");

                    dodge_ki(ch, vict, 0, 45, skill, SKILL_SPIRAL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

                    ch->location.modDamage(5);

                    pcost(ch, amount, 0);
                    hurt(0, 0, ch, vict, nullptr, 0, 1);
                    return;
                }
                else
                {
                    act("@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n",
                        false, ch, nullptr, vict, TO_CHAR);
                    act("@C$n@W fires a Spiral Comet blast at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
                    act("@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict,
                        TO_NOTVICT);
                    pcost(ch, amount, 0);
                    hurt(0, 0, ch, vict, nullptr, 0, 1);
                    return;
                }
            }
            else
            {
                act("@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n",
                    false, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W fires a Spiral Comet blast at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
                act("@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict,
                    TO_NOTVICT);
                pcost(ch, amount, 0);
            }
            hurt(0, 0, ch, vict, nullptr, 0, 1);
            return;
        }
        else
        {
            if (first == true)
            {
                dmg = damtype(ch, 44, skill, .5);
            }
            else
            {
                dmg = damtype(ch, 45, skill, .01);
            }
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S chest and explodes!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR chest and explodes!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S chest and explodes!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                dam_eq_loc(vict, 4);
                break;
            case 2: /* Critical */
                act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S head and explodes!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR head and explodes!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S head and explodes!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                dmg *= 2;
                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                dam_eq_loc(vict, 3);
                break;
            case 3:
                act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S body and explodes!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR body and explodes!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S body and explodes!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                hurt(0, 0, ch, vict, nullptr, dmg, 1);
                dam_eq_loc(vict, 4);
                break;
            case 4: /* Weak */
                act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S arm and explodes!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR arm and explodes!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S arm and explodes!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                dmg /= 2;
                dam_eq_loc(vict, 1);
                hurt(0, 190, ch, vict, nullptr, dmg, 1);
                break;
            case 5: /* Weak 2 */
                act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S leg and explodes!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR leg and explodes!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S leg and explodes!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                dmg /= 2;
                dam_eq_loc(vict, 2);
                hurt(1, 190, ch, vict, nullptr, dmg, 1);
                break;
            }
            pcost(ch, amount, 0);
            return;
        }
    }
    else
    {
        return;
    }
}

void handle_death_msg(Character *ch, Character *vict, int type)
{
    const auto tile = vict->location.getTileType();
    if (type == 0)
    {
        if (vict->location.getEnvironment(ENV_WATER) < 100.0 && tile != SECT_WATER_SWIM &&
            tile != SECT_WATER_NOSWIM && !vict->location.getWhereFlag(WhereFlag::space) &&
            tile != SECT_FLYING)
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r coughs up blood before falling to the ground dead.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYou cough up blood before falling to the ground dead.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r coughs up blood before falling down dead.@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 2:
                act("@R$N@r crumples to the ground dead.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou crumple to the ground dead.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r crumples to the ground dead.@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 3:
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 4:
                act("@R$N@r writhes on the ground screaming in pain before finally dying!@n", true, ch, nullptr,
                    vict, TO_CHAR);
                act("@rYou writhe on the ground screaming in pain before finally dying!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r writhes on the ground screaming in pain before finally dying!@n", true, ch, nullptr,
                    vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            case 5:
                act("@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", true,
                    ch, nullptr, vict, TO_CHAR);
                act("@rYou hit the ground dead with such force that blood flies into the air briefly!@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", true,
                    ch, nullptr, vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            }
        }
        else if (tile == SECT_WATER_SWIM || tile == SECT_WATER_NOSWIM)
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r coughs up blood and dies before falling down to the water. A large splash accompanies $S body hitting the water!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@rYou cough up blood and die before falling down to the water. A large splash accompanies your body hitting the water!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r coughs up blood and dies before falling down to the water. A large splash accompanies $S body hitting the water!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 2:
                act("@R$N@r crumples down to the water, with the signs of life leaving $S eyes as $E floats in the water.@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@rYou crumple down to the water and die.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r crumples down to the water, with the signs of life leaving $S eyes as $E floats in the water.@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 3:
                act("@R$N@r cries out $S last breath before dying and leaving a floating corpse behind.@n", true,
                    ch, nullptr, vict, TO_CHAR);
                act("@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r cries out $S last breath before dying and leaving a floating corpse behind.@n", true,
                    ch, nullptr, vict, TO_NOTVICT);
                break;
            case 4:
                act("@R$N@r writhes in the water screaming in pain before finally dying!@n", true, ch, nullptr,
                    vict, TO_CHAR);
                act("@rYou writhe in the water screaming in pain before finally dying!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r writhes in the water screaming in pain before finally dying!@n", true, ch, nullptr,
                    vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            case 5:
                act("@R$N@r hits the water dead with such force that blood mixed with water flies into the air briefly!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@rYou hit the water dead with such force that blood mixed with water flies into the air briefly!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r hits the water dead with such force that blood mixed with water flies into the air briefly!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            }
        }
        else if (vict->location.getWhereFlag(WhereFlag::space))
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r coughs up blood and dies. The blood freezes and floats freely through space...@n", true,
                    ch, nullptr, vict, TO_CHAR);
                act("@rYou cough up blood and die. The blood freezes and floats freely through space...@n", true,
                    ch, nullptr, vict, TO_VICT);
                act("@R$N@r coughs up blood and dies. The blood freezes and floats freely through space...@n", true,
                    ch, nullptr, vict, TO_NOTVICT);
                break;
            case 2:
                act("@R$N@r dies and leaves $S corpse floating freely in space.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYou die and leave your corpse floating freely in space.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r dies and leaves $S corpse floating freely in space.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            case 3:
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 4:
                act("@R$N@r writhes in space trying to scream in pain before $e finally dies!@n", true, ch, nullptr,
                    vict, TO_CHAR);
                act("@rYou writhe in space trying to scream in pain before you finally die!@n", true, ch, nullptr,
                    vict, TO_VICT);
                act("@R$N@r writhes in space trying to scream in pain before $e finally dies!@n", true, ch, nullptr,
                    vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            case 5:
                act("@R$N@r dies suddenly leaving behind a badly damaged corpse floating in space!@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@rYou die suddenly leaving behind a badly damaged corpse floating in space!@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$N@r dies suddenly leaving behind a badly damaged corpse floating in space!@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            }
        }
        else if (tile == SECT_FLYING)
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r coughs up blood before $s corpse starts to fall to the ground far below.@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@rYou coughs up blood before your corpse starts to fall to the ground far below.@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$N@r coughs up blood before $s corpse starts to fall to the ground far below.@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                break;
            case 2:
                act("@R$N@r dies and $S corpse begins to fall to the ground below.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYou die and your corpse begins to fall to the ground below.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r dies and $S corpse begins to fall to the ground below.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            case 3:
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 4:
                act("@R$N@r writhes in midair screaming in pain before finally dying!@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYou writhe in midair screaming in pain before finally dying!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r writhes in midair screaming in pain before finally dying!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            case 5:
                act("@R$N@r snaps back and dies with such force that blood flies into the air briefly!@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@rYou snap back and die with such force that blood flies into the air briefly!@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", true,
                    ch, nullptr, vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            }
        }
        else
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r coughs up blood before $s corpse starts to float limply in the water.@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@rYou coughs up blood before your corpse starts to float limply in the water.@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$N@r coughs up blood before $s corpse starts to float limply in the water.@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                break;
            case 2:
                act("@R$N@r dies and $S corpse begins to float limply in the water.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYou die and your corpse begins to float limply in the water.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r dies and $S corpse begins to float limply in the water.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            case 3:
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 4:
                act("@R$N@r writhes and thrases in the water trying to scream before finally dying!@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@rYou writhe and thrash in the water trying to scream before finally dying!@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$N@r writhes and thrashes in the water trying to scream before finally dying!@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            case 5:
                act("@R$N@r snaps back and dies with such force that blood floods out of $S body into the water!@n",
                    true, ch, nullptr, vict, TO_CHAR);
                act("@rYou snap back and die with such force that blood floods out of your body into the water!@n",
                    true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r hits the ground dead with such force that blood floods out of $S body into the water!@n",
                    true, ch, nullptr, vict, TO_NOTVICT);
                if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
                {
                    vict->setBaseStat("death_type", DTYPE_PULP);
                }
                break;
            }
        }
    }
    else
    {
        if (vict->location.getEnvironment(ENV_WATER) < 100.0 && tile != SECT_WATER_SWIM &&
            tile != SECT_WATER_NOSWIM && !vict->location.getWhereFlag(WhereFlag::space) &&
            tile != SECT_FLYING)
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 2:
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_HALF);
                break;
            case 3:
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 4:
                act("@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 5:
                act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rWhat's left of your body slams into the ground as you die!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            }
        }
        else if (tile == SECT_WATER_SWIM || tile == SECT_WATER_NOSWIM)
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 2:
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_HALF);
                break;
            case 3:
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 4:
                act("@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 5:
                act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rWhat's left of your body slams into the ground as you die!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            }
        }
        else if (vict->location.getWhereFlag(WhereFlag::space))
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r explodes and chunks of $M shower out into every direction of space.@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r explodes and chunks of $M shower out into every direction of space.@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 2:
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_HALF);
                break;
            case 3:
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 4:
                act("@R$N@r floats away as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body floats away as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r floats away as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 5:
                act("@rWhat's left of @R$N@r's body floats away through space!@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rWhat's left of your body floats away through space!@n", true, ch, nullptr, vict, TO_VICT);
                act("@rWhat's left of @R$N@r's body floats away through space!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            }
        }
        else if (tile == SECT_FLYING)
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r explodes and chunks of $M shower towards the ground far below.@n", true, ch, nullptr,
                    vict, TO_CHAR);
                act("@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r explodes and chunks of $M shower toward the ground far below.@n", true, ch, nullptr,
                    vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 2:
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_HALF);
                break;
            case 3:
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 4:
                act("@R$N@r falls down toward the ground as a smoldering corpse!@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYour body falls down toward the ground as a smoldering corpse!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@R$N@r falls down toward the ground as a smoldering corpse!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            case 5:
                act("@rWhat's left of @R$N@r's body falls toward the ground as $E dies!@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rWhat's left of yor body falls toward the ground as you die!@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rWhat's left of @R$N@r's body falls toward the ground as $E dies!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            }
        }
        else
        {
            switch (Random::get<int>(1, 5))
            {
            case 1:
                act("@R$N@r explodes and chunks of $M float freely through the water.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r explodes and chunks of $M float freely through the water.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 2:
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                act("@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict,
                    TO_VICT);
                act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_HALF);
                break;
            case 3:
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                vict->setBaseStat("death_type", DTYPE_VAPOR);
                break;
            case 4:
                act("@R$N@r falls back as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYour body falls back as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r falls back as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
                break;
            case 5:
                act("@rWhat's left of @R$N@r's body floats limply as $E dies!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rWhat's left of yor body floats limply as you die!@n", true, ch, nullptr, vict, TO_VICT);
                act("@rWhat's left of @R$N@r's body floats limply as $E dies!@n", true, ch, nullptr, vict,
                    TO_NOTVICT);
                break;
            }
        }
    }
}
