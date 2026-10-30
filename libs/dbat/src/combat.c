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
#include "dbat/consts/deathtype.h"
#include "dbat/consts/attacks.h"
#include "dbat/combat.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"
#include "dbat/act.item.h"
#include "dbat/mobact.h"
#include "dbat/fight.h"
#include "dbat/act.attack.h"
#include "dbat/dg_comm.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/genzon.h"
#include "dbat/dg_scripts.h"
#include "dbat/class.h"
#include "dbat/techniques.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/extract.h"
#include "dbat/random.h"
#include "dbat/relocate.h"
#include "dbat/stringutils.h"
#include "dbat/affect.h"

/* local functions */
void damage_weapon(struct char_data *ch, struct obj_data *obj, struct char_data *vict)
{

   if (obj) {
    if (obj_extra_flagged(obj, ITEM_UNBREAKABLE))
     return;
   }
   int ranking = 0, material = 1;
   int64_t PL10 = 2000000000, PL9 = 2000000000, PL8 = 2000000000;

   PL10 = PL10 * 5;
   PL9 = PL9 * 4;
   PL8 = PL8 * 2;

   if (GET_HIT(vict) >= PL10) {
    ranking = 10;
   } else if (GET_HIT(vict) >= PL9) {
    ranking = 9;
   } else if (GET_HIT(vict) >= PL8) {
    ranking = 8;
   } else if (GET_HIT(vict) >= 2000000000) {
    ranking = 7;
   } else if (GET_HIT(vict) >= 500000000) {
    ranking = 6;
   } else if (GET_HIT(vict) >= 250000000) {
    ranking = 5;
   } else if (GET_HIT(vict) >= 100000000) {
    ranking = 4;
   } else if (GET_HIT(vict) >= 50000000) {
    ranking = 3;
   } else if (GET_HIT(vict) >= 25000000) {
    ranking = 2;
   } else if (GET_HIT(vict) >= 1000000) {
    ranking = 1;
   }

   switch (obj_get_value(obj, VAL_ALL_MATERIAL)) {
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

 if (char_aff_flagged(ch, AFF_CURSE)) {
  result += 3;
 } else if (char_aff_flagged(ch, AFF_BLESS) && rand_number(1, 3) == 3) {
  if (result > 1) {
   result = 1;
  }
 } else if (char_aff_flagged(ch, AFF_BLESS)) {
  result = 0;
 }

 if (char_get_skill(ch, SKILL_HANDLING) >= axion_dice(10)) {
  act("@GYour superior handling prevents @C$p@G from being damaged.@n", TRUE, ch, obj, 0, TO_CHAR);
  act("@g$n's@G superior handling prevents @C$p@G from being damaged.@n", TRUE, ch, obj, 0, TO_ROOM);
  result = 0;
 }

 if (result > 0) {
  obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (result));
  if (obj_get_value(obj, VAL_ALL_HEALTH) <= 0) {
   act("@RYour @C$p@R shatters on @r$N's@R body!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@r$n's@R @C$p@R shatters on YOUR body!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@r$n's@R @C$p@R shatters on @r$N's@R body!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   obj_extra_set(obj, ITEM_BROKEN, true);
   perform_remove(vict, 16);
   perform_remove(vict, 17);
  } else if (result >= 8) {
   act("@RYour @C$p@R cracks loudly from striking @r$N's@R body!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@r$n's@R @C$p@R cracks loudly from striking YOUR body!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@r$n's@R @C$p@R cracks loudly from striking @r$N's@R body!@n", TRUE, ch, obj, vict, TO_NOTVICT);
  } else if (result >= 6) {
   act("@RYour @C$p@R chips from striking @r$N's@R body!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@r$n's@R @C$p@R cracks from striking YOUR body!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@r$n's@R @C$p@R cracks from striking @r$N's@R body!@n", TRUE, ch, obj, vict, TO_NOTVICT);
  } else if (result >= 3) {
   act("@RYour @C$p@R loses a piece from striking @r$N's@R body!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@r$n's@R @C$p@R loses a piece from striking YOUR body!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@r$n's@R @C$p@R loses a piece from striking @r$N's@R body!@n", TRUE, ch, obj, vict, TO_NOTVICT);
  } else if (result >= 6) {
   act("@RYour @C$p@R has a nick in it from hitting @r$N's@R body!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@r$n's@R @C$p@R has a nick in it from hitting YOUR body!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@r$n's@R @C$p@R has a nick in it from hitting @r$N's@R body!@n", TRUE, ch, obj, vict, TO_NOTVICT);
  }
 }
}

void handle_multihit(struct char_data *ch, struct char_data *vict)
{

 int perc = char_get_aff_abils_dex(ch), prob = char_get_aff_abils_dex(vict);
 
 /* Let's give the victim a bit of automatic favor due to "luck" */
  prob += rand_number(1, 15);
 /* Some otherwise inclined "luck" for the attacker */
  perc += rand_number(-5, 5);
  
 if (ch->throws >= 3) {
  ch->throws = 0;
  return;
 } else if (ch->throws == -1) {
   ch->throws = 0;
 }

 /* Racial bonuses */
  if (char_is_konatsu(ch)) {
    perc *= 1.5;
  }
  if (char_is_bio(ch) && (char_get_genome(ch, 0) == 8 || char_get_genome(ch, 1) == 8)) {
    perc *= 1.4;
  }

  if (char_is_npc(ch)) {
   perc -= perc * 0.3;
  }
  
 /* Weapons have less chance to multihit */
  if (char_get_lastattack(ch) == -1) {
   perc *= 0.75;
  }

  int amt = 70;

  if (char_get_skill(ch, SKILL_STYLE) >= 100) {
   amt -= amt * 0.1;
  } else if (char_get_skill(ch, SKILL_STYLE) >= 80) {
   amt -= amt * 0.08;
  } else if (char_get_skill(ch, SKILL_STYLE) >= 60) {
   amt -= amt * 0.06;
  } else if (char_get_skill(ch, SKILL_STYLE) >= 40) {
   amt -= amt * 0.04;
  } else if (char_get_skill(ch, SKILL_STYLE) >= 20) {
   amt -= amt * 0.02;
  }

  /* Critical Failure*/
  if (axion_dice(0) < amt) {
   prob += 500;
  }
 
 /* Success! */
 if (perc >= prob) {
  char buf[MAX_INPUT_LENGTH];
  act("@Y...in a lightning flash of speed you attack @y$N@Y again!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@Y...in a lightning flash of speed @y$n@Y attacks YOU again!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@Y...in a lightning flash of speed @y$n@Y attacks @y$N@Y again!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  ch->throws += 1;
  char_plr_set(ch, PLR_MULTIHIT, true);
  if (char_get_combo(ch) > -1) {
   switch (char_get_combo(ch)) {
        case 0:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_punch(ch, buf, 0, 0);
	 break;
	case 1:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_kick(ch, buf, 0, 0);
	 break;
	case 2:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_elbow(ch, buf, 0, 0);
	 break;
	case 3:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_knee(ch, buf, 0, 0);
	 break;
	case 4:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_roundhouse(ch, buf, 0, 0);
	 break;
	case 5:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_uppercut(ch, buf, 0, 0);
	 break;
	case 6:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_slam(ch, buf, 0, 0);
         ch->throws += 1;
	 break;
	case 8:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_heeldrop(ch, buf, 0, 0);
         ch->throws += 1;
	 break;
	case 51:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_bash(ch, buf, 0, 0);
         ch->throws += 1;
	 break;
	case 52:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_head(ch, buf, 0, 0);
	 break;
	case 56:
	 sprintf(buf, "%s", char_get_name(vict));
	 do_tailwhip(ch, buf, 0, 0);
         ch->throws += 1;
	 break;
   }
  } else {
   if (char_get_lastattack(ch) == -1) {
    sprintf(buf, "%s", char_get_name(vict));
    do_attack(ch, buf, 0, 0);
   } else {
    if (rand_number(1, 3) == 2 && char_get_skill(ch, SKILL_KICK) > 0) {
	 sprintf(buf, "%s", char_get_name(vict));
	 do_kick(ch, buf, 0, 0);
    } else {
	 sprintf(buf, "%s", char_get_name(vict));
	 do_punch(ch, buf, 0, 0);
    }
   }
  }
 }
 
}

int handle_defender(struct char_data *vict, struct char_data *ch)
{

 int result = FALSE;

 if (char_get_defender(vict)) {
  struct char_data *def = char_get_defender(vict);
  int64_t defnum = (char_get_speedi(def) * 0.01) * rand_number(-10, 10);
  int64_t chnum = (char_get_speedi(ch) * 0.01) * rand_number(-5, 10);
  if (char_get_speedi(def) + defnum > char_get_speedi(ch) + chnum && char_get_in_room(def) == char_get_in_room(vict) && char_get_position(def) > POS_SITTING) {
   act("@YYou move to and manage to intercept the attack aimed at @y$N@Y!@n", TRUE, def, 0, vict, TO_CHAR);
   act("@y$n@Y moves to and manages to intercept the attack aimed at YOU!@n", TRUE, def, 0, vict, TO_VICT);
   act("@y$n@Y moves to and manages to intercept the attack aimed at @y$N@Y!@n", TRUE, def, 0, vict, TO_NOTVICT);
   result = TRUE;
  } else if (char_get_in_room(def) == char_get_in_room(vict) && char_get_position(def) > POS_SITTING) {
   act("@YYou move to intercept the attack aimed at @y$N@Y, but just not fast enough!@n", TRUE, def, 0, vict, TO_CHAR);
   act("@y$n@Y moves to intercept the attack aimed at YOU, but $e wasn't fast enough!@n", TRUE, def, 0, vict, TO_VICT);
   act("@y$n@Y moves to intercept the attack aimed at @y$N@Y, but $e wasn't fast enough!@n", TRUE, def, 0, vict, TO_NOTVICT);
  }
 }

 return (result);
}

void handle_disarm(struct char_data *ch, struct char_data *vict)
{

 int roll1 = rand_number(-10, 10), roll2 = rand_number(-10, 10), handled = FALSE;
  roll1 += char_get_aff_abils_str(ch) + char_get_aff_abils_dex(ch);
  roll2 += char_get_aff_abils_str(vict) + char_get_aff_abils_dex(vict);

 if (!char_is_npc(ch)) {
  if (char_plr_flagged(ch, PLR_THANDW)) {
   roll1 += 5;
  }
 }

  if (rand_number(1, 100) <= 50 && !char_is_konatsu(ch)) {
   roll1 = -500;
  } else if (rand_number(1, 100) <= 75) {
   roll1 *= 1.5;
  }

  if (char_is_konatsu(vict)) {
   roll1 *= 0.75;
  }

  if (char_get_skill(ch, SKILL_HANDLING) >= axion_dice(10)) {
   handled = TRUE;
  }

 if (roll1 < roll2) {
  struct obj_data *obj;
  if (char_get_equipment(ch, WEAR_WIELD1) && handled == FALSE) {
   obj = char_get_equipment(ch, WEAR_WIELD1);
   act("@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", TRUE, ch, obj, vict, TO_VICT); 
   perform_remove(ch, 16);
   if (obj_get_vnum(obj) != 20098) {
    obj_from_char(obj);
    obj_to_room(obj, char_get_in_room(ch));
   }
  } else if (char_get_equipment(ch, WEAR_WIELD1) && handled == TRUE) {
   obj = char_get_equipment(ch, WEAR_WIELD1);
   act("@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it from slipping from your grasp!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", TRUE, ch, obj, vict, TO_VICT);
  } else if (char_get_equipment(ch, WEAR_WIELD2) && handled == TRUE) {
   obj = char_get_equipment(ch, WEAR_WIELD2);
   act("@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it from slipping from your grasp!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", TRUE, ch, obj, vict, TO_VICT);
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
   obj = char_get_equipment(ch, WEAR_WIELD2);
   act("@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n", TRUE, ch, obj, vict, TO_CHAR);
   act("@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", TRUE, ch, obj, vict, TO_VICT);
   perform_remove(ch, 17);
   if (obj_get_vnum(obj) != 20098) {
    obj_from_char(obj);
    obj_to_room(obj, char_get_in_room(ch));
   }
  }
 }

}

void combine_attacks(struct char_data *ch, struct char_data *vict)
{

 struct follow_type *f;
 char chbuf[MAX_INPUT_LENGTH], victbuf[MAX_INPUT_LENGTH], rmbuf[MAX_INPUT_LENGTH]; 
 int64_t bonus = 0;
 double maxki = 0.0;
 int totalmem = 1;
 int attspd = 0, blockable = TRUE, same = TRUE, attsk = 0, attavg = char_get_skill(ch,  attack_skills[char_get_combine(ch)]);
 int burn = FALSE, shocked = FALSE;

 switch (char_get_combine(ch)) {
  case 0: /* Kamehameha */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You cup your hands at your sides and a ball of @Benergy@W forms there. You chant @B'@CKaaaaameeeehaaaameeee@B'@W and then fire a @RKamehameha@W wave at @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W cups $s hands be $s side and chants @B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands and he quickly brings them forward and fires a @RKamehameha @Wat @rYOU@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W cups $s hands be $s side and chants @B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands and he quickly brings them forward and fires a @RKamehameha @Wat @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n@n");
   maxki = 0.15;
   attspd += 2;
   bonus += GET_MAX_MOVE(ch) * 0.02;
   break;
  case 1: /* Galik Gun */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You throw your hands forward and launch a purple beam of energy at @r$N@n while shouting @B'@mGalik Gun!@B'@W");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W throws $s arms forward and launches a purple beam at @rYOU@W while shouting @B'@mGalik Gun!@B'@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W throws $s arms forward and launches a purple beam at @r$N@W while shouting @B'@mGalik Gun!@B'@n");
   maxki = 0.15;
   attspd += 1;
   bonus += GET_MAX_MANA(ch) * 0.5;
   break;
  case 2: /* Masenko */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You raise your hands above your head with one resting atop the other and begin to pour your charged energy to that point. As soon as the energy is ready you shout @B'@RMasenko Ha!@B'@W and bringing your hands down you launch a bright reddish orange beam at @r$N@W!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises $s hands above $s head and energy quicly pools there. Suddenly $e brings $s hands down and shouts @B'@RMasenko Ha!@B'@W as a bright reddish orange beam launches toward @rYOU!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises $s hands above $s head and energy quicly pools there. Suddenly $e brings $s hands down and shouts @B'@RMasenko Ha!@B'@W as a bright reddish orange beam launches toward @r$N!@n");
   maxki = 0.15;
   attspd += 1;
   bonus += GET_MAX_MANA(ch) * 0.5;
   break;
  case 3: /* Deathbeam */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With a quick motion you point at @r$N@W and launch a lightning fast @MDeathbeam@W!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! With a quick motion $e points $s finger at @rYOU@W and launches a lightning fast @MDeathbeam@W!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! With a quick motion $e points $s finger at @r$N@W and launches a lightning fast @MDeathbeam@W!@n");
   maxki = 0.1;
   attspd += 4;
   break;
  case 4: /* Honoo */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy ready you breath out toward @r$N@W jets of incredibly hot flames in the form of a deadly @rHonoo@W!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Sudden jets of flame burst forth from $s mouth at @RYOU@W in the form of a deadly @rHonoo@W!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Sudden jets of flame burst forth from $s mouth at @R$N@W in the form of a deadly @rHonoo@W!@n");
   maxki = 0.125;
   attspd += 2;
   burn = TRUE;
   break;
  case 5: /* Twin Slash */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy prepared you poor it into your blade and accelerate your body to incredible speeds toward @r$N! You leave two glowing green marks behind on $S body in a single instant as your @GTwin Slash@W hits!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Raising $s sword @Y$n@W accelerates toward @rYOU@W with incredible speed! Two glowing green slashes are left on YOUR body from $s successful @GTwin Slash@W!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Raising $s sword @Y$n@W accelerates toward @r$N@W with incredible speed! Two glowing green slashes are left on @R$N's@W body from the successful @GTwin Slash@W!@n");
   maxki = 0.125;
   attspd += 2;
   blockable = FALSE;
   break;
  case 6: /* Hell Flash */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You stick one of each of your hands in your armpits and detach them. With your hands detached your point the exposed arm cannons at @r$N@W and launch a massive @RHell Flash@W at $M!");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W sticks one of each of $s hands in $s armpits and detaches them there. With the hands detached $e aims $s exposed arm cannons at @RYOU@W and launches a massive @RHell Flash@W!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W sticks one of each of $s hands in $s armpits and detaches them there. With the hands detached $e aims $s exposed arm cannons at @R$N@W and launches a massive @RHell Flash@W!@n");
   maxki = 0.2;
   attspd += 1;
   break;
  case 7: /* Psychic Blast */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy ready you look at @R$N@W as the blue light of your @CPsychic Blast@W launches from your head toward $S!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! A blue light, identifying a @CPsychic Blast@W, launches from @Y$n's@W toward @RYOUR HEAD@W!");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! A blue light, identifying a @CPsychic Blast@W, launches from @Y$n's@W toward @R$N's@W head!");
   maxki = 0.125;
   attspd += 1;
   shocked = TRUE;
   break;
  case 8: /* Crusher Ball */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! Pooling your energy you form a large ball of red energy above an upraised palm. Slamming your other hand into it you launch it toward @r$N@W while shouting @B'@RCrusher Ball@B'@W!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises a palm above his head and red energy begins to pool there. As the energy completes the formation of a ball @Y$n@W slams $s other hand into it and launches it at @rYOU@W while shouting @B'@RCrusher Ball@B'@W!");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises a palm above his head and red energy begins to pool there. As the energy completes the formation of a ball @Y$n@W slams $s other hand into it and launches it at @r$N@W while shouting @B'@RCrusher Ball@B'@W!");
   maxki = 0.2;
   break;
  case 9: /* Water Spikes */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! Using your energy to form a ball of water between your hands you then raise the ball above your head. Several spiked of ice form from the ball of water and you hurl them at @r$N@W!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Forming a ball of water between $s palms with $s energy @Y$n@W then raises the ball of water above $s head. Suddenly several spikes of ice form from the water and $e launches them at @rYOU@W!");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Forming a ball of water between $s palms with $s energy @Y$n@W then raises the ball of water above $s head. Suddenly several spikes of ice form from the water and $e launches them at @r$N@W!");
   maxki = 0.14;
   break;
  case 10: /* Tribeam */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You form a triangle with your hands and aim the center of the triangle at @r$N@W. With the sudden shout @B'@YTribeam@B'@W you release your prepared energy at $M in the form of a beam!");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W forms a triangle with $s hands and aims the center at @rYOU@W! With the sudden shout @B'@YTribeam@B'@W a large beam of energy flashes toward @rYOU!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W forms a triangle with $s hands and aims the center at @r$N@W! With the sudden shout @B'@YTribeam@B'@W a large beam of energy flashes toward @r$N!@n");
   maxki = 0.2;
   attspd += 2;
   bonus += GET_MAX_HIT(ch) * 0.5;
   break;
  case 11: /* Starbreaker */
   sprintf(chbuf, "@WPositioning yourself in the center of your group, you call out to your allies to launch a group attack! You raise your right hand above your head as dark red energy begins to pool in your slightly cupped hand, while purple arcs of electricity flow up your left arm. Slamming both hands together, you shout @B'@YStarbreaker@B'@W and release your prepared energy at $M in the form of a ball!@n");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises $s right hand, pooling dark red energy in the palm. @Y$n@W slams both their hands together, shouting @B'@YStarbreaker@B'@W, a ball of energy flashes toward @rYOU!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises their right hand above their head, pooling dark red energy. @Y$n@W slams both their hands together, shouting @B'@YStarbreaker@B'@W, a ball of energy flashes toward @r$N!@n");
   maxki = 0.2;
   bonus += GET_MAX_MANA(ch) * 0.6;
   break;
  case 12: /* Seishou Enko */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You open your mouth and aim at @r$N@W. You grunt as you release your prepared energy at $M in the form of a beam!");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W opens $s mouth and aims at @rYOUW! With the sudden grunt, a large beam flashes towards @rYOU@n!");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W opens $s mouth and aims at @rYOUW! With the sudden grunt, a large beam flashes toward @r$N@n!");
   maxki = 0.125;
   attspd += 35;
   break;
  case 13: /* Renzokou Energy Dan */
   sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You slam your hands together and aim at @r$n@W. With the sudden shout @B'@YRenzoku Energy Dan@B'@W you release your prepared energy in the form of hundreds of ki blasts!");
   sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W slams both $s hands together and aims at @rYOUW! With the sudden shout @B'@YRenzoku Energy Dan@B'@W hundreds of ki blasts flash towards @rYOU!@n");
   sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W slams $s hands together and aims at @r$N@W! With the sudden shout @B'@YRenzoku Energy Dan@B'@W hundreds of ki blasts flash towards @r$N!@n");
   maxki = 0.125;
   attspd += 6;
   break;
  default:
   send_to_imm("ERROR: Combine attacks failure for: %s", char_get_name(ch));
   send_to_char(ch, "An error has been logged. Be patient while waiting for Iovan's response.\r\n");
   return;
 }

 int64_t totki = 0;

 act(chbuf, TRUE, ch, 0, vict, TO_CHAR);
 act(victbuf, TRUE, ch, 0, vict, TO_VICT);
 act(rmbuf, TRUE, ch, 0, vict, TO_NOTVICT);

 if (char_get_charge(ch) >= GET_MAX_MANA(ch) * maxki) {
  totki += GET_MAX_MANA(ch) * maxki;
  char_set_charge(ch, char_get_charge(ch) - (GET_MAX_MANA(ch) * maxki));
 } else {
  totki += char_get_charge(ch);
  char_set_charge(ch, 0);
 }

 for (f = ch->followers; f; f = f->next) {
  if (!char_aff_flagged(f->follower, AFF_GROUP)) {
   continue;
  } else {
   if (char_get_combine(f->follower) != char_get_combine(ch)) {
    same = FALSE;
   }
   if (char_get_charge(f->follower) >= GET_MAX_MANA(f->follower) * maxki) {
    totki += GET_MAX_MANA(f->follower) * maxki;
    char_set_charge(f->follower, char_get_charge(f->follower) - (GET_MAX_MANA(f->follower) * maxki));
   } else {
    totki += char_get_charge(f->follower);
    char_set_charge(f->follower, 0);
   }
   totalmem += 1;
   attavg += char_get_skill(f->follower, attack_skills[char_get_combine(f->follower)]);
   char folbuf[MAX_INPUT_LENGTH], folbuf2[MAX_INPUT_LENGTH];
   sprintf(folbuf, "@Y$n@W times and merges $s @B'@R%s@B'@W into the group attack!@n", attack_names_comp[char_get_combine(f->follower)]);
   sprintf(folbuf2, "@WYou time and merge your @B'@R%s@B'@W into the group attack!@n", attack_names_comp[char_get_combine(f->follower)]);
   act(folbuf, TRUE, f->follower, 0, 0, TO_ROOM);
   act(folbuf2, TRUE, f->follower, 0, 0, TO_CHAR);
  }
 }

 totki += bonus;
 if (same == TRUE) {
  totki += bonus;
 }
 attsk = attavg / totalmem;
 
  if (char_get_combine(ch) != 5) {
   if (attspd + attsk < char_get_skill(vict, SKILL_DODGE) + (char_get_aff_abils_cha(ch) / 10)) {
    act("@GYou manage to dodge nimbly through the combined attack of your enemies!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@r$n@G manages to dodge nimbly through the combined attack!@n", TRUE, vict, 0, 0, TO_ROOM);
    return;
   } else if (blockable == TRUE && attspd + attsk < char_get_skill(vict, SKILL_BLOCK) + (char_get_aff_abils_str(ch) / 10)) {
    act("@GYou manage to effectivly block the combined attack of your enemies with the help of your great strength!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@r$n@G manages to dodge nimbly through the combined attack!@n", TRUE, vict, 0, 0, TO_ROOM);
    return;
   }
  }
   if (burn == TRUE) {
    if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(1, 4) == 3 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
      send_to_char(vict, "@RYou are burned by the attack!@n\r\n");
      send_to_char(ch, "@RThey are burned by the attack!@n\r\n");
      char_aff_set(vict, AFF_BURNED, true);
     } else if (char_get_bonuses(vict, BONUS_FIREPROOF) || char_is_demon(vict)) {
      send_to_char(ch, "@RThey appear to be fireproof!@n\r\n");
     } else if (char_get_bonuses(vict, BONUS_FIREPRONE)) {
      send_to_char(vict, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
      send_to_char(ch, "@RThey are easily burned!@n\r\n");
      char_aff_set(vict, AFF_BURNED, true);
     }
   }
   if (shocked == TRUE) {
      if (!char_aff_flagged(vict, AFF_SHOCKED) && rand_number(1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
       act("@MYour mind has been shocked!@n", TRUE, vict, 0, 0, TO_CHAR);
       act("@M$n@m's mind has been shocked!@n", TRUE, vict, 0, 0, TO_ROOM);
       char_aff_set(vict, AFF_SHOCKED, true);
      }
   }
   hurt(0, 0, ch, vict, NULL, totki, 1);
   if (same == TRUE) {
    for (f = ch->followers; f; f = f->next) {
     send_to_char(f->follower, "@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n\r\n");
    }
    send_to_char(ch, "@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n\r\n");
   }
}

int check_ruby(struct char_data *ch)
{

 struct obj_data *obj, *next_obj = NULL, *ruby = NULL;
 int found = 0;

 for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
  if (found == 0 && obj_get_vnum(obj) == 6600) {
   if (obj_extra_flagged(obj, ITEM_HOT)) {
    found = 1;
    ruby = obj;
   }
  }
 }

 if (found > 0) {
  act("@RYour $p@R flares up and disappears. Your fire attack has been aided!@n", TRUE, ch, ruby, 0, TO_CHAR);
  act("@R$n's@R $p@R flares up and disappears!@n", TRUE, ch, ruby, 0, TO_ROOM);
  extract_obj(ruby);
  return (1);
 } else {
  return (0);
 }

}

int64_t combo_damage(struct char_data *ch, int64_t damage, int type)
{
 int64_t bonus = 0;

 if (type == 0) { /* Not a finish */
  int hits = char_get_combhits(ch);
   
  if (hits >= 30) {
   bonus += damage * (hits * 0.15);
   bonus += damage * 12;
  } else if (hits >= 20) {
   bonus = damage * (hits * 0.1);
   bonus += damage * 10;
  } else if (hits >= 10) {
   bonus = damage * (hits * 0.1);
   bonus += damage * 5;
  } else if (hits >= 6) {
   bonus = damage * (hits * 0.1);
   bonus += damage * 1.5;
  } else if (hits >= 2) {
   bonus = damage * (hits * 0.05);
   bonus += damage * 0.2;
  }
 } else if (type == 1) {
  bonus = damage * 15;
 }

 return (bonus);
}


/* For getting into a better combat position */
int roll_balance(struct char_data *ch)
{

 int chance = 0;

 if (char_is_npc(ch)) {
  if (char_get_level(ch) >= 100) {
   chance = rand_number(80, 100);
  } else if (char_get_level(ch) >= 80) {
   chance = rand_number(75, 90);
  } else if (char_get_level(ch) >= 70) {
   chance = rand_number(70, 80);
  } else if (char_get_level(ch) >= 60) {
   chance = rand_number(65, 75);
  } else if (char_get_level(ch) >= 50) {
   chance = rand_number(50, 60);
  }
 } else {
  if (char_get_skill(ch, SKILL_BALANCE) > 50) {
   chance = char_get_skill(ch, SKILL_BALANCE);
  }
 }

 return (chance);

}

void handle_knockdown(struct char_data *ch)
{
 int chance = 0;

 if (char_is_npc(ch)) {
  if (char_get_level(ch) >= 100) {
   chance = rand_number(35, 45);
  } else if (char_get_level(ch) >= 90) {
   chance = rand_number(25, 35);
  } else if (char_get_level(ch) >= 70) {
   chance = rand_number(15, 25);
  } else if (char_get_level(ch) >= 50) {
   chance = rand_number(10, 15);
  } else if (char_get_level(ch) >= 30) {
   chance = rand_number(5, 10);
  }
 } else {
  chance = char_get_skill(ch, SKILL_BALANCE) * 0.5;
 }

 if (chance > axion_dice(0)) {
  act("@mYou are @GALMOST@m knocked off your feet, but your great balance helps you keep your footing!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@W$n@m is @GALMOST@m knocked off $s feet, but $s great balance helps $m keep $s footing!@n", TRUE, ch, 0, 0, TO_ROOM);
 } else {
  act("@mYou are knocked off your feet!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@W$n@m is knocked off $s feet!@n", TRUE, ch, 0, 0, TO_ROOM);
  char_set_position(ch, POS_SITTING);
 }

}

int boom_headshot(struct char_data *ch)
{

 int skill = char_get_skills_base(ch, SKILL_TWOHAND);

 if (skill >= 100 && rand_number(1, 5) >= 3)
  return (1);
 else if (skill < 100 && skill >= 75 && rand_number(1, 5) == 5)
  return (1);
 else if (skill < 75 && skill >= 50 && rand_number(1, 6) == 6)
  return (1);
 else
  return (0);

}

 int64_t gun_dam(struct char_data *ch, int wlvl)
{
  int64_t dmg = 100;

 switch (wlvl) {
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

 if (char_get_skill(ch, SKILL_GUN) >= 100)
  dmg *= 2;
 else if (char_get_skill(ch, SKILL_GUN) >= 50)
  dmg += dmg * 0.5;

 int64_t dmg_prior = 0;

 dmg_prior = (dmg * char_get_aff_abils_dex(ch)) * ((char_get_level(ch) / 5) + 1);

 if (dmg_prior <= GET_MAX_HIT(ch) * 0.4)
  dmg = dmg_prior;
 else
  dmg = GET_MAX_HIT(ch) * 0.4;

 return (dmg);
}

void club_stamina(struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg)
{

 double drain = 0.0;
 int64_t drained = 0;

 switch (wlvl) {
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

 if (char_get_skill(ch, SKILL_CLUB) >= 100)
  drain += 0.1;
 else if (char_get_skill(ch, SKILL_CLUB) >= 50)
  drain += 0.05;

 drained = dmg * drain;
 decCurST(vict, drained, 0);

 send_to_char(ch, "@D[@YVictim's @GStamina @cLoss@W: @g%s@D]@n\r\n", add_commas(drained));
 send_to_char(vict, "@D[@rYour @GStamina @cLoss@W: @g%s@D]@n\r\n", add_commas(drained));

}

int backstab(struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg)
{

 int chance = 0, roll_to_beat = rand_number(1, 100);
 double bonus = 0.0;

 if (char_get_backstabcool(ch) > 0)
  return (0);

 switch (wlvl) {
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

 if (char_get_bonuses(ch, BONUS_POWERHIT)) {
  bonus += 2;
 }

 if (char_get_skill(ch, SKILL_DAGGER) >= 100)
  chance += 20;
 else if (char_get_skill(ch, SKILL_DAGGER) >= 50)
  chance += 10;

 char_set_backstabcool(ch, 10);

 if (chance >= roll_to_beat) {
  int attacker_roll = char_get_skill(ch, SKILL_MOVE_SILENTLY) + char_get_skill(ch, SKILL_SPOT) + char_get_aff_abils_dex(ch) + rand_number(-5, 5);
  int defender_roll = char_get_skill(vict, SKILL_SPOT) + char_get_skill(vict, SKILL_LISTEN) + char_get_aff_abils_dex(ch) + rand_number(-5, 5);

  if (attacker_roll > defender_roll) {
   act("@RYou manage to sneak behind @r$N@R and stab $M in the back!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@RYou feel @r$n's@R dagger thrust into your back unexpectantly!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R sneaks up behind @r$N@R and stabs $M in the back!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   dmg += dmg * bonus;
   hurt(0, 0, ch, vict, NULL, dmg, 0);
   return (1);
  } else {
   return (0);
  }
 } else {
  return (0);
 }
}

void cut_limb(struct char_data *ch, struct char_data *vict, int wlvl, int hitspot)
{

 int chance = 0, decap = 0, decapitate = FALSE;
 int roll_to_beat = rand_number(1, 10000);

 if (wlvl == 1) {
  chance = 25;
 } else if (wlvl == 2) {
  chance = 50;
 } else if (wlvl == 3) {
  chance = 100;
  decap = 5;
 } else if (wlvl == 4) {
  chance = 200;
  decap = 10;
 } else if (wlvl == 5) {
  chance = 200;
  decap = 50;
 }

 if (char_get_skill(ch, SKILL_SWORD) >= 100) {
  chance += 100;
 } else if (char_get_skill(ch, SKILL_SWORD) >= 50) {
  chance += 50;
 }

 if (decap >= roll_to_beat && hitspot == 4) {
  decapitate = TRUE;
 } else if (chance < roll_to_beat) {
  return;
 }
 
 if (GET_HIT(vict) <= 1) {
  return;
 }

 if (decapitate == TRUE) {
  act("@R$N's@r head is cut off in the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@RYOUR head is cut off in the attack!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@R$N's@rhead is cut off in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);

  remove_limb(vict, 0);
  die(vict, ch);
  if (char_aff_flagged(ch, AFF_GROUP)) {
   group_gain(ch, vict);
  } else {
   solo_gain(ch, vict);
  }
  char corp[256];
  if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
   sprintf(corp, "all.zenni corpse");
   do_get(ch, corp, 0, 0);
  }
  if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
   sprintf(corp, "all corpse");
   do_get(ch, corp, 0, 0);
  }
  return;
 } else { /* We've only succeeded in removing a limb. */
  if (!char_is_npc(vict)) {
    if (char_has_arms(vict) && rand_number(1, 2) == 2) {
     if (char_get_limb_condition(vict, (2)-1) > 0) {
      char_set_limb_condition(vict, (2)-1, 0);
       if (char_plr_flagged(vict, PLR_CLARM)) {
        char_plr_set(vict, PLR_CLARM, false);
       }
      act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your left arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      remove_limb(vict, 2);
     } else if (char_get_limb_condition(vict, (1)-1) > 0) {
       char_set_limb_condition(vict, (1)-1, 100);
       if (char_plr_flagged(vict, PLR_CRARM)) {
        char_plr_set(vict, PLR_CRARM, false);
       }
      act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your right arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      remove_limb(vict, 1);
     }
    } else { /* It's a leg */
     if (char_get_limb_condition(vict, (4)-1) > 0) {
       char_set_limb_condition(vict, (4)-1, 100);
       if (char_plr_flagged(vict, PLR_CLLEG)) {
        char_plr_set(vict, PLR_CLLEG, false);
       }
      act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your left leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      remove_limb(vict, 4);
     } else if (char_get_limb_condition(vict, (3)-1) > 0) {
       char_set_limb_condition(vict, (3)-1, 100);
       if (char_plr_flagged(vict, PLR_CRLEG)) {
        char_plr_set(vict, PLR_CRLEG, false);
       }
      act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your right leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      remove_limb(vict, 3);
     }
    }
  } else { /* It's a npc */
   if (char_has_arms(vict) && rand_number(1, 2) == 2) {
    if (char_mob_flagged(vict, MOB_LARM)) {
     char_mob_set(vict, MOB_LARM, false);
     remove_limb(vict, 2);
      act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your left arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    } else if (char_mob_flagged(vict, MOB_RARM)) {
     char_mob_set(vict, MOB_RARM, false);
     remove_limb(vict, 1);
      act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your right arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    }
   } else {
    if (char_mob_flagged(vict, MOB_LLEG)) {
     char_mob_set(vict, MOB_LLEG, false);
     remove_limb(vict, 4);
      act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your left leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    } else if (char_mob_flagged(vict, MOB_RLEG)) {
     char_mob_set(vict, MOB_RLEG, false);
     remove_limb(vict, 3);
      act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYOU lose your right leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    }
   }
  }
 }

}

int count_physical(struct char_data *ch)
{
 int count = 0;

 if (char_get_skill(ch, SKILL_PUNCH) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_KICK) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_KNEE) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_ELBOW) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_ROUNDHOUSE) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_SLAM) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_UPPERCUT) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_TAILWHIP) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_BASH) >= 1) {
  count += 1;
 } if (char_get_skill(ch, SKILL_HEADBUTT) >= 1) {
  count += 1;
 }

 return (count);
}

int physical_mastery(struct char_data *ch)
{

 int count = 22;

 if (char_get_skill(ch, SKILL_PUNCH) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_KICK) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_KNEE) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_ELBOW) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_ROUNDHOUSE) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_SLAM) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_UPPERCUT) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_TAILWHIP) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_BASH) >= 100) {
  count += 1;
 } if (char_get_skill(ch, SKILL_HEADBUTT) >= 100) {
  count += 1;
 }

 if (count == 26)
  count += 1;
 else if (count >= 27)
  count += 2;

 return (count);

}

int64_t advanced_energy(struct char_data *ch, int64_t dmg)
{

 if (ch == NULL) {
  return (FALSE);
 }

 double rate = 0.00;
 int count = char_get_level(ch);
 int64_t add = 0;

 if (char_get_bonuses(ch, BONUS_LEECH)) {
  rate = (double)(count) * 0.2;
  
  if (rate > 0.00) {
   add = dmg * rate;
   if (char_get_charge(ch) + add > GET_MAX_MANA(ch)) {
    if (char_get_charge(ch) < GET_MAX_MANA(ch)) {
     char_set_charge(ch, GET_MAX_MANA(ch));
     act("@MYou leech some of the energy away!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@m$n@M leeches some of the energy away!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else {
     send_to_char(ch, "@MYou can't leech because there is too much charged energy for you to handle!@n\r\n");
    }
   } else {
     char_set_charge(ch, char_get_charge(ch) + (add));
     act("@MYou leech some of the energy away!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@m$n@M leeches some of the energy away!@n", TRUE, ch, 0, 0, TO_ROOM);
   }

  } /* End of rate check */

 } /* End of Leech Bonus */


 if (char_get_bonuses(ch, BONUS_INTOLERANT)) {
  rate = (double)(count) * 0.2;

  if (rate > 0.00) {
   if (char_get_charge(ch) > 0 && rand_number(1, 100) <= 10) {
    act("@MThe attack causes your weak control to slip and you are shocked by your own charged energy!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@m$n@M suffers shock from their own charged energy!@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurHealth(ch, char_get_charge(ch) / 4, 1);
   }
   add = dmg * rate;
  }
 } /* End of Energy Intolerant bonus */

 return (add);
} /* End of advanced_energy function */

int roll_accuracy(struct char_data *ch, int skill, bool kiatt)
{

 if (!char_is_npc(ch)) {
  if (char_get_bonuses(ch, BONUS_ACCURATE)) {
   if (kiatt == TRUE)
    skill += skill * 0.10;
   else
    skill += skill * 0.20;
  } else if (char_get_bonuses(ch, BONUS_POORDEPTH)) {
   if (kiatt == TRUE)
    skill -= skill * 0.10;
   else
    skill -= skill * 0.20;
  }
 }

 if (skill < 40) {
  skill += rand_number(3, 10);
 }

 return (skill);
}

long double calc_critical(struct char_data *ch, int loc)
{

 int roll = rand_number(1, 100);
 long double multi = 1;

 if (loc == 0) { /* Head */
  if (char_get_bonuses(ch, BONUS_POWERHIT)) {
   if (roll <= 15) {
    multi = 4;
   } else if (char_get_bonuses(ch, BONUS_SOFT)) {
    multi = 1;
   } else {
    multi = 2;
   }
  } else if (char_get_bonuses(ch, BONUS_SOFT)) {
   multi = 1;
  } else {
   multi = 2;
  }
 } else if (loc == 1) { /* Limb */
  if (char_get_bonuses(ch, BONUS_SOFT)) {
   multi = 0.25;
  } else {
   multi = 0.5;
  }
 } else { /* Body*/
  if (char_get_bonuses(ch, BONUS_SOFT)) {
   multi = 0.5;
  }
 }

 return (multi);
}

int roll_hitloc(struct char_data *ch, struct char_data *vict, int skill)
{

 int location = 4, critmax = 1000;
 int critical = 0;

 if (char_is_npc(ch)) {
  if (char_get_level(ch) > 100)
   skill = rand_number(char_get_level(ch), char_get_level(ch) + 10);
  else   
   skill = rand_number(char_get_level(ch), 100);
 }

 if (char_is_dabura(ch) && !char_is_npc(ch)) {
  if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
   critmax -= 200;
 }

 critical = rand_number(80, critmax);

 if (skill >= critical) {
  location = 2;
 } else if (skill >= rand_number(50, 750)) {
  location = 2;
 } else if (skill >= rand_number(50, 350)) {
  location = 1;
 } else if (skill >= rand_number(30, 200)) {
  location = 3;
 } else {
  location = rand_number(4, 5);
 }

 if (!char_is_npc(vict)) {
  if (location == 4 && char_get_limb_condition(vict, (1)-1) <= 0 && char_get_limb_condition(vict, (2)-1) <= 0) { /* No arms */
   location = 5;
  }

  if (location == 5 && char_get_limb_condition(vict, (3)-1) <= 0 && char_get_limb_condition(vict, (4)-1) <= 0) { /* No legs */
   location = 4;
  }
  
  if (location == 4 && char_get_limb_condition(vict, (1)-1) <= 0 && char_get_limb_condition(vict, (2)-1) <= 0) { /* Both failed, make body */
   location = 1;
  }
 }

 if (char_is_npc(vict)) {
  if (location == 4 && !char_mob_flagged(vict, MOB_RARM) && !char_mob_flagged(vict, MOB_LARM))
   location = 5;

  if (location == 5 && !char_mob_flagged(vict, MOB_RLEG) && !char_mob_flagged(vict, MOB_LLEG))
   location = 4;

  if (location == 5 && !char_mob_flagged(vict, MOB_RARM) && !char_mob_flagged(vict, MOB_LARM))
   location = 1;
 }

 return (location);

}

int64_t armor_calc(struct char_data *ch, int64_t dmg, int type)
{
 if (char_is_npc(ch))
  return (0);

 int64_t reduce = 0;

  if (char_get_armor(ch) < 1000) {
   reduce = char_get_armor(ch) * 0.5;
  } else if (char_get_armor(ch) < 2000) {
   reduce = char_get_armor(ch) * .75;
  } else if (char_get_armor(ch) < 5000) {
   reduce = char_get_armor(ch);
  } else if (char_get_armor(ch) < 10000) {
   reduce = char_get_armor(ch) * 2;
  } else if (char_get_armor(ch) < 20000) {
   reduce = char_get_armor(ch) * 4;
  } else if (char_get_armor(ch) < 30000) {
   reduce = char_get_armor(ch) * 8;
  } else if (char_get_armor(ch) < 40000) {
   reduce = char_get_armor(ch) * 12;
  } else if (char_get_armor(ch) < 60000) {
   reduce = char_get_armor(ch) * 25;
  } else if (char_get_armor(ch) < 100000) {
   reduce = char_get_armor(ch) * 50;
  } else if (char_get_armor(ch) < 150000) {
   reduce = char_get_armor(ch) * 75;
  } else if (char_get_armor(ch) < 200000) {
   reduce = char_get_armor(ch) * 150;
  } else if (char_get_armor(ch) >= 200000) {
   reduce = char_get_armor(ch) * 250;
  }

 /* loc: 0 = Physical Bonus, 1 = Ki Bonus, 2 = Bonus To Both */
 int i, loc = -1;
 double bonus = 0.0;

 for (i = 0; i < NUM_WEARS; i++) {
  if (char_get_equipment(ch, i)) {
   struct obj_data *obj = char_get_equipment(ch, i);
   switch (obj_get_value(obj, VAL_ALL_MATERIAL)) {
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

 if (loc != -1) {
  switch (type) {
   case 0:
     if (loc == 0 || loc == 2) {
      reduce += reduce * bonus;
     }
    break;
   case 1:
     if (loc == 1 || loc == 2) {
      reduce += reduce * bonus;
      reduce /= 2;
     }
    break;
  }
 }
 
 return (reduce);
}



/* For Destroying or Breaking Limbs */
void hurt_limb(struct char_data *ch, struct char_data *vict, int chance, int area, int64_t power)
{
 if (!vict || char_is_npc(vict)) {
  return;
 }

  int dmg = 0;

  if (chance > axion_dice(100)) {
   return;
  }

  if (power > (getEffMaxPL(vict)) * 0.5) {
   dmg = rand_number(25, 40);
  } else if (power > (getEffMaxPL(vict)) * 0.25) {
   dmg = rand_number(15, 24);
  } else if (power > (getEffMaxPL(vict)) * 0.10) {
   dmg = rand_number(8, 14);
  } else if (power > (getEffMaxPL(vict)) * 0.05) {
   dmg = rand_number(4, 7);
  } else {
   dmg = rand_number(1, 3);;
  }

  if (char_get_armor(vict) > 50000) {
   dmg -= 5;
  } else if (char_get_armor(vict) > 40000) {
   dmg -= 4;
  } else if (char_get_armor(vict) > 30000) {
   dmg -= 3;
  } else if (char_get_armor(vict) > 20000) {
   dmg -= 2;
  } else if (char_get_armor(vict) > 10000) {
   dmg -= 1;
  } else if (char_get_armor(vict) > 5000) {
   dmg -= rand_number(0, 1);
  } 

  if (dmg <= 0) {
   return;
  }

   if (!is_sparring(ch)) {
    if (area == 0) { /* Arms */
     if (char_get_limb_condition(vict, (2)-1) - dmg <= 0) {
      act("@RYour attack @YDESTROYS @r$N's@R left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack @YDESTROYS@R YOUR left arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack @YDESTROYS @r$N's@R left arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_limb_condition(vict, (2)-1, 0);
      if (char_plr_flagged(vict, PLR_THANDW)) {
       char_plr_set(vict, PLR_THANDW, false);
      }
      if (char_plr_flagged(vict, PLR_CLARM)) {
       char_plr_set(vict, PLR_CLARM, false);
      }
      remove_limb(vict, 2);
     } else if (char_get_limb_condition(vict, (2)-1) > 0) {
      char_set_limb_condition(vict, (2)-1, char_get_limb_condition(vict, (2)-1) - (dmg));
      act("@RYour attack hurts @r$N's@R left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack hurts YOUR left arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack hurts @r$N's@R left arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
     } else if (char_get_limb_condition(vict, (1)-1) - dmg <= 0) {
      act("@RYour attack @YDESTROYS @r$N's@R right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack @YDESTROYS@R YOUR right arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack @YDESTROYS @r$N's@R right arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_limb_condition(vict, (1)-1, 0);
      if (char_plr_flagged(vict, PLR_THANDW)) {
       char_plr_set(vict, PLR_THANDW, false);
      }
      if (char_plr_flagged(vict, PLR_CLARM)) {
       char_plr_set(vict, PLR_CRARM, false);
      }
      remove_limb(vict, 2);
     } else if (char_get_limb_condition(vict, (1)-1) > 0) {
      char_set_limb_condition(vict, (1)-1, char_get_limb_condition(vict, (1)-1) - (dmg));
      act("@RYour attack hurts @r$N's@R right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack hurts YOUR right arm!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack hurts @r$N's@R right arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
     }
    } else if (area == 1) { /* Legs */
     if (char_get_limb_condition(vict, (4)-1) - dmg <= 0) {
      act("@RYour attack @YDESTROYS @r$N's@R left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack @YDESTROYS@R YOUR left leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack @YDESTROYS @r$N's@R left leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_limb_condition(vict, (4)-1, 0);
      if (char_plr_flagged(vict, PLR_THANDW)) {
       char_plr_set(vict, PLR_THANDW, false);
      }
      if (char_plr_flagged(vict, PLR_CLLEG)) {
       char_plr_set(vict, PLR_CLLEG, false);
      }
      remove_limb(vict, 2);
     } else if (char_get_limb_condition(vict, (4)-1) > 0) {
      char_set_limb_condition(vict, (4)-1, char_get_limb_condition(vict, (4)-1) - (dmg));
      act("@RYour attack hurts @r$N's@R left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack hurts YOUR left leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack hurts @r$N's@R left leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
     } else if (char_get_limb_condition(vict, (3)-1) - dmg <= 0) {
      act("@RYour attack @YDESTROYS @r$N's@R right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack @YDESTROYS@R YOUR right leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack @YDESTROYS @r$N's@R right leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_limb_condition(vict, (3)-1, 0);
      if (char_plr_flagged(vict, PLR_THANDW)) {
       char_plr_set(vict, PLR_THANDW, false);
      }
      if (char_plr_flagged(vict, PLR_CLLEG)) {
       char_plr_set(vict, PLR_CLLEG, false);
      }
      remove_limb(vict, 2);
     } else if (char_get_limb_condition(vict, (3)-1) > 0) {
      char_set_limb_condition(vict, (3)-1, char_get_limb_condition(vict, (3)-1) - (dmg));
      act("@RYour attack hurts @r$N's@R right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@r$n's@R attack hurts YOUR right leg!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@r$n's@R attack hurts @r$N's@R right leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
     }
    }
   }
}

/* For damaging equipment when hit */
void dam_eq_loc(struct char_data *vict, int area)
{
 int location = 0, num = 0;
 /* Area is 4 possible hit locations in an attack.
    1 Arms, 2 legs, 3 head, and 4 body. */

 if (!vict || vict == NULL || GET_HIT(vict) <= 0) {
  return;
 }

 switch (area) {
  case 1:
   num = rand_number(1, 8);
   switch (num) {
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
   num = rand_number(1, 3);
   switch (num) {
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
   num = rand_number(1, 6);
   switch (num) {
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
   num = rand_number(1, 4);
   switch (num) {
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

void damage_eq(struct char_data *vict, int location)
{

  if (char_get_equipment(vict, location) && rand_number(1, 20) >= 19 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
   struct obj_data *eq = char_get_equipment(vict, location);
   if (obj_extra_flagged(eq, ITEM_UNBREAKABLE)) {
    return;
   }

   int loss = rand_number(2, 5);
   
   if (obj_get_vnum(eq) == 20099 || obj_get_vnum(eq) == 20098)
    loss = 1;

   if (char_aff_flagged(vict, AFF_CURSE)) {
    loss *= 3;
   } else if (char_aff_flagged(vict, AFF_BLESS) && rand_number(1, 3) == 3) {
    loss = 1;
   } else if (char_aff_flagged(vict, AFF_BLESS)) {
    return;
   }
 

   obj_set_value(eq, VAL_ALL_HEALTH, obj_get_value(eq, VAL_ALL_HEALTH) - (loss));
   if (obj_get_value(eq, VAL_ALL_HEALTH) <= 0) {
    obj_set_value(eq, VAL_ALL_HEALTH, 0);
    obj_extra_set(eq, ITEM_BROKEN, true);
    act("@WYour $p@W completely breaks!@n", FALSE, 0, eq, vict, TO_VICT);
    act("@C$N's@W $p@W completely breaks!@n", FALSE, 0, eq, vict, TO_NOTVICT);
    perform_remove(vict, location);
    if (!char_is_npc(vict))
     save_char(vict);
   } else if (obj_get_value(eq, VAL_ALL_MATERIAL) == MATERIAL_LEATHER || obj_get_value(eq, VAL_ALL_MATERIAL) == MATERIAL_COTTON || obj_get_value(eq, VAL_ALL_MATERIAL) == MATERIAL_SILK) {
    act("@WYour $p@W rips a little!@n", FALSE, 0, eq, vict, TO_VICT);
    act("@C$N's@W $p@W rips a little!@n", FALSE, 0, eq, vict, TO_NOTVICT);
    if (char_aff_flagged(vict, AFF_BLESS)) {
     send_to_char(vict, "@c...But your blessing seems to have partly mended this damage.@n\r\n");
     act("@c...but @C$N's@c body glows blue for a moment and the damage mends a little.@n", TRUE, 0, 0, vict, TO_NOTVICT);
    } else if (char_aff_flagged(vict, AFF_CURSE)) {
     send_to_char(vict, "@r...and your curse seems to have made the damage three times worse!@n\r\n");
     act("@c...but @C$N's@c body glows red for a moment and the damage grow three times worse!@n", TRUE, 0, 0, vict, TO_NOTVICT);
    }
   } else {
    act("@WYour $p@W cracks a little!@n", FALSE, 0, eq, vict, TO_VICT);
    act("@C$N's@W $p@W cracks a little!@n", FALSE, 0, eq, vict, TO_NOTVICT);
    if (char_aff_flagged(vict, AFF_BLESS)) {
     send_to_char(vict, "@c...But your blessing seems to have partly mended this damage.@n\r\n");
     act("@c...but @C$N's@c body glows blue for a moment and the damage mends a little.@n", TRUE, 0, 0, vict, TO_NOTVICT);
    } else if (char_aff_flagged(vict, AFF_CURSE)) {
     send_to_char(vict, "@r...and your curse seems to have made the damage three times worse!@n\r\n");
     act("@c...but @C$N's@c body glows red for a moment and the damage grow three times worse!@n", TRUE, 0, 0, vict, TO_NOTVICT);
    }
   }  
  }
}

/* This is for updating MOB android absorb */
void update_mob_absorb()
{
  int roll = 0;
  struct char_data *i, *vict;

  for (i = character_list; i; i = i->next) {
   roll = axion_dice(0) + (char_get_level(i) * 0.25);

   if (!char_is_npc(i))
    continue;

   if (!char_is_android(i))
    continue;
   
   if (!char_mob_flagged(i, MOB_ABSORB))
    continue;

   if (char_get_absorbing(i) == NULL || !char_get_absorbing(i))
    continue;
   else if (char_get_level(i) < roll)
    continue;
   else if (char_get_absorbing(i)) {
    vict = char_get_absorbing(i);

    int ki = GET_MAX_MANA(vict) * 0.01;
    int stam = GET_MAX_MOVE(vict) * 0.01;
    int pl = GET_MAX_HIT(vict) * 0.01;
    int maxed = 0;

    if (roll < (char_get_level(i) + 1) * 0.75) {
     ki += ki * rand_number(2, 4);
     stam += stam * rand_number(2, 4);
     pl += pl * rand_number(2, 4);
    }

    decCurHealth(vict, pl, 0);
    incCurHealth(i, pl, true);

    decCurKI(vict, ki, 0);
    incCurKI(i, ki, true);

    decCurST(vict, stam, 0);
    incCurST(i, stam, true);

    if (isFullHealth(i)) {
     maxed += 1;
    }
    if (isFullST(i)) {
     maxed += 1;
    }
    if (isFullKI(i)) {
     maxed += 1;
    }

    if (GET_HIT(vict) <= 0) {
     act("@R$n@r absorbs the last of YOUR energy and you die...@n", TRUE, i, 0, vict, TO_VICT);
     act("@R$n@r absorbs the last of @R$N's@r energy and $E dies...@n", TRUE, i, 0, vict, TO_NOTVICT);
     die(vict, i);
    } else if (maxed >= 3) {
     act("@R$n@r absorbs some of YOUR energy...but $e seems to be full now and releases YOU!@n", TRUE, i, 0, vict, TO_VICT);
     act("@R$n@r absorbs some of @R$N's@r energy...but $e seems to be full now and lets go.@n", TRUE, i, 0, vict, TO_NOTVICT);
    } else {
     act("@R$n@r absorbs some of YOUR energy!@n", TRUE, i, 0, vict, TO_VICT);
     act("@R$n@r absorbs some of @R$N's@r energy.@n", TRUE, i, 0, vict, TO_NOTVICT);
    }
   }
  }
}

/* This is for huge attacks that are slowly descending on a room */
void huge_update()
{
 int dge = 0, skill = 0, bonus = 1, count = 0;
 int64_t dmg = 0;
 struct obj_data *k;
 struct char_data *ch, *vict, *next_v;
 
 /* Checking the object list for any huge ki attacks */
 for (k = object_list; k; k = k->next) {
  if (obj_get_aucter(k) > 0 && obj_get_aucTime(k) + 604800 <= time(0)) {
   if (obj_get_in_room(k) && rroom_get_vnum(obj_get_in_room(k)) == 80) {
    room_vnum inroom = obj_get_in_room(k);
    rroom_set_flag(inroom, ROOM_HOUSE_CRASH, false);
    extract_obj(k);
    continue;
   }
  }  
  if (obj_get_kicharge(k) <= 0) {
   continue;
  }
  if (obj_get_vnum(k) != 82 && obj_get_vnum(k) != 83) {
   continue;
  }
  else if (obj_get_distance(k) <= 0) {
  /* Genki Dama Section */
  if (obj_get_kitype(k) == 497) {
  if (char_get_in_room(obj_get_target(k)) == obj_get_in_room(k)) {
   ch = obj_get_user(k); 
   if (rroom_get_vnum(char_get_in_room(ch)) == rroom_get_vnum(obj_get_in_room(k))) {
    bonus = 2;
   }

   act("@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on YOU! It eclipses everything above you as it crushes down into you! You struggle against it with all your might!@n", TRUE, obj_get_target(k), 0, 0, TO_CHAR);
   act("@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on @C$n@W! It completely obscures $m from view as it crushes into $s body! It appears to be facing some resistance from $m!@n", TRUE, obj_get_target(k), 0, 0, TO_ROOM);
   send_to_room(obj_get_in_room(k), "\r\n");
   if (GET_HIT(obj_get_target(k)) * bonus < obj_get_kicharge(k) * 5) {
  
    act("@WYour strength is no match for the power of the attack! It slowly grinds into you before exploding into a massive blast!@n", TRUE, obj_get_target(k), 0, 0, TO_CHAR); 
    act("@C$n@W's strength is no match for the power of the attack! It slowly grinds into $m before exploding into a massive blast!@n", TRUE, obj_get_target(k), 0, 0, TO_ROOM);
    skill = init_skill(ch, SKILL_GENKIDAMA); /* Set skill value */
    dmg = obj_get_kicharge(k) * 1.25;
    hurt(0, 0, ch, obj_get_target(k), NULL, dmg, 1);
    dmg /= 2;

    /* Hit those in the current room. */
    for (vict = world[obj_get_in_room(k)].people; vict; vict = next_v) {
     next_v = vict->next_in_room;  
  
     if (vict == ch) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
     }
     if (vict == obj_get_target(k)) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_GROUP)) {
      if (vict->master == ch) {
       continue;
      } else if (ch->master == vict) {
       continue;
      } else if (vict->master == ch->master) {
       continue;
      }
     }
     if (char_get_level(vict) <= 8 && !char_is_npc(vict)){
      continue;
     }
     if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
     }
     dge = handle_dodge(vict);
     if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
             (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_CHAR);
       act("@cYou disappear, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_VICT);
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_NOTVICT);
       char_aff_set(vict, AFF_ZANZOKEN, false);
       pcost(vict, 0, GET_MAX_HIT(vict) / 200);
       hurt(0, 0, ch, vict, NULL, 0, 1);
       continue;
     }
     else if (dge + rand_number(-10, 5) > skill) {
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@WYou manage to escape the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, 0, 1);
       improve_skill(vict, SKILL_DODGE, 0);
       continue;
     }
     else {
       count += 1;
       if (char_is_npc(vict) && count > 10) {
        if (GET_HIT(vict) < dmg) {
         double loss = 0.0;

         if (count >= 30) {
          loss = 0.80;
         } else if (count >= 20) {
          loss = 0.6;
         } else if (count >= 15) {
          loss = 0.4;
         } else if (count >= 10) {
          loss = 0.25;
         }
         char_set_exp(vict, char_get_exp(vict) - (char_get_exp(vict) * loss));
        }
       }
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@RYou are caught by the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, dmg, 1);
       continue;     
     }
    }
    rroom_set_dmg(obj_get_in_room(k), 100);
        int zone = 0;
      if ((zone = real_zone_by_thing(rroom_get_vnum(char_get_in_room(ch)))) != NOWHERE) {
       send_to_zone("A MASSIVE explosion shakes the entire area!\r\n", zone);
      }

    extract_obj(k);
    continue;
   } /* It isn't stopped! */
   else {
    act("@WYou manage to overpower the attack! You lift up into the sky slowly with it and toss it up and away out of sight!@n", TRUE, obj_get_target(k), 0, 0, TO_CHAR);
    act("@C$n@W manages to unbelievably overpower the attack! It is lifted up into the sky and tossed away dramaticly!@n", TRUE, obj_get_target(k), 0, 0, TO_ROOM); 
    hurt(0, 0, ch, obj_get_target(k), NULL, 0, 1);
    decCurST(obj_get_target(k), obj_get_kicharge(k) / 4, 0);
    extract_obj(k);
    continue;
   }
  } 
  else if (char_get_in_room(obj_get_target(k)) != obj_get_in_room(k)) {
   ch = obj_get_user(k); 
   
   send_to_room(obj_get_in_room(k), "@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on the area! It slowly burns into the ground before exploding magnificently!@n\r\n");

  
    skill = init_skill(ch, SKILL_GENKIDAMA); /* Set skill value */
    dmg = obj_get_kicharge(k);
    dmg /= 2;

    /* Hit those in the current room. */
    for (vict = world[obj_get_in_room(k)].people; vict; vict = next_v) {
     next_v = vict->next_in_room;  
  
     if (vict == ch) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict)) {
      continue;
     }
     if (char_get_level(vict) <= 8 && !char_is_npc(vict)){
      continue;
     }
     if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
     }
     dge = handle_dodge(vict);
     if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
             (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_CHAR);
       act("@cYou disappear, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_VICT);
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_NOTVICT);
       char_aff_set(vict, AFF_ZANZOKEN, false);
       pcost(vict, 0, GET_MAX_HIT(vict) / 200);
       hurt(0, 0, ch, vict, NULL, 0, 1);
       continue;
     }
     else if (dge + rand_number(-10, 5) > skill) {
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@WYou manage to escape the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, 0, 1);
       improve_skill(vict, SKILL_DODGE, 0);
       continue;
     }
     else {
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@RYou are caught by the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, dmg, 1);
       continue;     
     }
    }
    rroom_set_dmg(obj_get_in_room(k), 100);
        int zone = 0;
      if ((zone = real_zone_by_thing(rroom_get_vnum(char_get_in_room(ch)))) != NOWHERE) {
       send_to_zone("A MASSIVE explosion shakes the entire area!\r\n", zone);
      }
    extract_obj(k);
   continue;
   }/* End Genki */
   continue;
  }
  /* Genocide Section */
  if (obj_get_kitype(k) == 498) {
   if (char_get_in_room(obj_get_target(k)) == obj_get_in_room(k)) {
   ch = obj_get_user(k); 
   if (rroom_get_vnum(char_get_in_room(ch)) == rroom_get_vnum(obj_get_in_room(k))) {
    bonus = 2;
   }
   
   act("@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on YOU! It eclipses everything above you as it crushes down into you! You struggle against it with all your might!@n", TRUE, obj_get_target(k), 0, 0, TO_CHAR);
   act("@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on @C$n@W! It completely obscures $m from view as it crushes into $s body! It appears to be facing some resistance from $m!@n", TRUE, obj_get_target(k), 0, 0, TO_ROOM);
   send_to_room(obj_get_in_room(k), "\r\n");
   if (GET_HIT(obj_get_target(k)) * bonus < obj_get_kicharge(k) * 10) {
  
    act("@WYour strength is no match for the power of the attack! It slowly grinds into you before exploding into a massive blast!@n", TRUE, obj_get_target(k), 0, 0, TO_CHAR); 
    act("@C$n@W's strength is no match for the power of the attack! It slowly grinds into $m before exploding into a massive blast!@n", TRUE, obj_get_target(k), 0, 0, TO_ROOM);
    skill = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
    dmg = obj_get_kicharge(k);
    hurt(0, 0, ch, obj_get_target(k), NULL, dmg, 1);
    dmg /= 2;

    /* Hit those in the current room. */
    for (vict = world[obj_get_in_room(k)].people; vict; vict = next_v) {
     next_v = vict->next_in_room;  
  
     if (vict == ch) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
     }
     if (vict == obj_get_target(k)) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_GROUP)) {
      if (vict->master == ch) {
       continue;
      } else if (ch->master == vict) {
       continue;
      } else if (vict->master == ch->master) {
       continue;
      }
     }
     if (char_get_level(vict) <= 8 && !char_is_npc(vict)){
      continue;
     }
     if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
     }
     dge = handle_dodge(vict);
     if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
             (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_CHAR);
       act("@cYou disappear, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_VICT);
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_NOTVICT);
       char_aff_set(vict, AFF_ZANZOKEN, false);
       pcost(vict, 0, GET_MAX_HIT(vict) / 200);
       continue;
     }
     else if (dge + rand_number(-10, 5) > skill) {
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@WYou manage to escape the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, 0, 1);
       improve_skill(vict, SKILL_DODGE, 0);
       continue;
     }
     else {
       count += 1;
       if (char_is_npc(vict) && count > 10) {
        if (GET_HIT(vict) < dmg) {
         double loss = 0.0;

         if (count >= 30) {
          loss = 0.80;
         } else if (count >= 20) {
          loss = 0.6;
         } else if (count >= 15) {
          loss = 0.4;
         } else if (count >= 10) {
          loss = 0.25;
         }
         char_set_exp(vict, char_get_exp(vict) - (char_get_exp(vict) * loss));
        }
       }
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@RYou are caught by the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, dmg, 1);
       continue;     
     }
    }
    rroom_set_dmg(obj_get_in_room(k), 100);
        int zone = 0;
      if ((zone = real_zone_by_thing(rroom_get_vnum(char_get_in_room(ch)))) != NOWHERE) {
       send_to_zone("A MASSIVE explosion shakes the entire area!\r\n", zone);
      }
    extract_obj(k);
    continue;
   } /* It isn't stopped! */
   else {
    act("@WYou manage to overpower the attack! You lift up into the sky slowly with it and toss it up and away out of sight!@n", TRUE, obj_get_target(k), 0, 0, TO_CHAR);
    act("@C$n@W manages to unbelievably overpower the attack! It is lifted up into the sky and tossed away dramaticly!@n", TRUE, obj_get_target(k), 0, 0, TO_ROOM); 
    hurt(0, 0, ch, obj_get_target(k), NULL, 0, 1);
       decCurST(obj_get_target(k), obj_get_kicharge(k) / 4, 0);
    extract_obj(k);
    continue;
   }
  } 
  else if (char_get_in_room(obj_get_target(k)) != obj_get_in_room(k)) {
   ch = obj_get_user(k); 
   
   send_to_room(obj_get_in_room(k), "@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on the area! It slowly burns into the ground before exploding magnificantly!@n\r\n");

  
    skill = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
    dmg = obj_get_kicharge(k);
    dmg /= 2;

    /* Hit those in the current room. */
    for (vict = world[obj_get_in_room(k)].people; vict; vict = next_v) {
     next_v = vict->next_in_room;  
  
     if (vict == ch) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
     }
     if (char_aff_flagged(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict)) {
      continue;
     }
     if (char_get_level(vict) <= 8 && !char_is_npc(vict)){
      continue;
     }
     if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
     }
     dge = handle_dodge(vict);
     if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
             (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_CHAR);
       act("@cYou disappear, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_VICT);
       act("@C$N@c disappears, avoiding the explosion!@n", FALSE, ch, 0, vict, TO_NOTVICT);
       char_aff_set(vict, AFF_ZANZOKEN, false);
       pcost(vict, 0, GET_MAX_HIT(vict) / 200);
       continue;
     }
     else if (dge + rand_number(-10, 5) > skill) {
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@WYou manage to escape the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, 0, 1);
       improve_skill(vict, SKILL_DODGE, 0);
       continue;
     }
     else {
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@RYou are caught by the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       hurt(0, 0, ch, vict, NULL, dmg, 1);
       continue;     
     }
    }
    rroom_set_dmg(obj_get_in_room(k), 100);
        int zone = 0;
      if ((zone = real_zone_by_thing(rroom_get_vnum(char_get_in_room(ch)))) != NOWHERE) {
       send_to_zone("A MASSIVE explosion shakes the entire area!\r\n", zone);
      }
    extract_obj(k);
    continue;
   }/* End Genocide */
   continue;
  }
  else {
    extract_obj(k);
    continue;
   }
  }
   act("$p@W descends slowly towards the ground!@n", TRUE, 0, k, 0, TO_ROOM);
   obj_set_distance(k, obj_get_distance(k) - 1);
 } 

}
/* End huge ki attack update */

/* For handling homing attacks */
void homing_update()
{
 struct obj_data *k;
 struct obj_data *next_k;

 for (k = object_list; k; k = next_k) {

  next_k = k->next;

  if (obj_get_kicharge(k) <= 0) {
   continue;
  }

  if (obj_get_vnum(k) != 80 && obj_get_vnum(k) != 81 && obj_get_vnum(k) != 84) {
   continue;
  } else if (obj_get_target(k) && obj_get_user(k)) {
   struct char_data *ch = obj_get_user(k);
   struct char_data *vict = obj_get_target(k);

   if (obj_get_vnum(k) == 80) { // Tsuihidan
    if (obj_get_in_room(k) != char_get_in_room(vict)) {
     act("@wThe $p@w pursues after you!@n", TRUE, vict, k, 0, TO_CHAR);
     act("@wThe $p@W pursues after @C$n@w!@n", TRUE, vict, k, 0, TO_ROOM);
     obj_from_room(k);
     obj_to_room(k, char_get_in_room(vict));
     continue;
    } else {
     act("@RThe $p@R makes a tight turn and rockets straight for you!@n", TRUE, vict, k, 0, TO_CHAR);
     act("@RThe $p@R makes a tight turn and rockets straight for @r$n@n", TRUE, vict, k, 0, TO_ROOM);
     if (handle_parry(vict) < rand_number(1, 140)) {
      act("@rThe $p@r slams into your body, exploding in a flash of bright light!@n", TRUE, vict, k, 0, TO_CHAR);
      act("@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", TRUE, vict, k, 0, TO_ROOM);
      int64_t dmg = obj_get_kicharge(k);
      extract_obj(k);
      hurt(0, 0, ch, vict, NULL, dmg, 1);
      continue;
     } else if (rand_number(1, 3) > 1) {
      act("@wYou manage to deflect the $p@W sending it flying away and depleting some of its energy.@n", TRUE, vict, k, 0, TO_CHAR);
      act("@C$n @wmanages to deflect the $p@w sending it flying away and depleting some of its energy.@n", TRUE, vict, k, 0, TO_ROOM);
      obj_set_kicharge(k, obj_get_kicharge(k) - (obj_get_kicharge(k) / 10));
      if (obj_get_kicharge(k) <= 0) {
       send_to_room(obj_get_in_room(k), "%s has lost all its energy and disappears.\r\n", k->short_description);
       extract_obj(k);
      }
      continue;
     } else {
      act("@wYou manage to deflect the $p@w sending it flying away into the nearby surroundings!@n", TRUE, vict, k, 0, TO_CHAR);
      act("@C$n @wmanages to deflect the $p@w sending it flying away into the nearby surroundings!@n", TRUE, vict, k, 0, TO_ROOM);
      if (rroom_get_dmg(char_get_in_room(vict)) <= 95) {
       rroom_set_dmg(char_get_in_room(vict), rroom_get_dmg(char_get_in_room(vict)) + (5));
      }
      extract_obj(k);
      continue;
     }
    }
    continue;
   } else if (obj_get_vnum(k) == 81 || obj_get_vnum(k) == 84) { // Spirit Ball
    if (obj_get_in_room(k) != char_get_in_room(vict)) {
     act("@wYou lose sight of @C$N@W and let $p@W fly away!@n", TRUE, ch, k, vict, TO_CHAR);
     act("@wYou manage to escape @C$n's@W $p@W!@n", TRUE, ch, k, vict, TO_VICT);
     act("@C$n@W loses sight of @c$N@W and lets $s $p@W fly away!@n", TRUE, ch, k, vict, TO_NOTVICT);
     extract_obj(k);
     continue;
    } else {
     act("@RYou move your hand and direct $p@R after @r$N@R!@n", TRUE, ch, k, vict, TO_CHAR);
     act("@r$n@R moves $s hand and directs $p@R after YOU!@n", TRUE, ch, k, vict, TO_VICT);
     act("@r$n@R moves $s hand and directs $p@R after @r$N@R!@n", TRUE, ch, k, vict, TO_NOTVICT);
     if (handle_parry(vict) < rand_number(1, 140)) {
      if (obj_get_vnum(k) != 84) {
       act("@rThe $p@r slams into your body, exploding in a flash of bright light!@n", TRUE, vict, k, 0, TO_CHAR);
       act("@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", TRUE, vict, k, 0, TO_ROOM);
       int64_t dmg = obj_get_kicharge(k);
       extract_obj(k);
       hurt(0, 0, ch, vict, NULL, dmg, 1);
      } else if (obj_get_vnum(k) == 84) {
       int64_t dmg = obj_get_kicharge(k);
      if (dmg > GET_MAX_HIT(vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
       act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@rYou are cut in half by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       die(vict, ch);
       if (char_aff_flagged(ch, AFF_GROUP)) {
        group_gain(ch, vict);
       }
       else {
        solo_gain(ch, vict);
       }
       if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
        do_get(ch, "all.zenni corpse", 0, 0);
       }
       if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
        do_get(ch, "all corpse", 0, 0);
       }
      } else if (dmg > GET_MAX_HIT(vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
       if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(1, 101) && (getCurKI(vict)) >= GET_MAX_MANA(vict) / 40) {
        act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou are cut in half by the attack but regenerate a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        decCurKI(vict, getMaxKI(vict) / 40, 0);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
       } else if (dmg > GET_MAX_HIT(vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
       if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(1, 101) && (getCurKI(vict)) >= GET_MAX_MANA(vict) / 40) {
        act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou are cut in half by the attack but regenerate a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           decCurKI(vict, getMaxKI(vict) / 40, 0);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
       }
       else {
       act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@rYou are cut in half by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
       act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
       die(vict, ch);
       if (char_aff_flagged(ch, AFF_GROUP)) {
        group_gain(ch, vict);
       }
       else {
        solo_gain(ch, vict);
       }
       if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
        do_get(ch, "all.zenni corpse", 0, 0);
       }
       if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
        do_get(ch, "all corpse", 0, 0);
       }
       }
      }
     } else {
       act("@rThe $p@r slams into your body, exploding in a flash of bright light!@n", TRUE, vict, k, 0, TO_CHAR);
       act("@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", TRUE, vict, k, 0, TO_ROOM);
       hurt(0, 0, ch, vict, NULL, dmg, 1);
     }
       extract_obj(k);
      }
       continue;
     } else if (rand_number(1, 3) > 1) {
      act("@wYou manage to deflect the $p@W sending it flying away and depleting some of its energy.@n", TRUE, vict, k, 0, TO_CHAR);
      act("@C$n @wmanages to deflect the $p@w sending it flying away and depleting some of its energy.@n", TRUE, vict, k, 0, TO_ROOM);
      obj_set_kicharge(k, obj_get_kicharge(k) - (obj_get_kicharge(k) / 10));
      if (obj_get_kicharge(k) <= 0) {
       send_to_room(obj_get_in_room(k), "%s has lost all its energy and disappears.\r\n", k->short_description);
       extract_obj(k);
      }
      continue;
     } else {
      act("@wYou manage to deflect the $p@w sending it flying away into the nearby surroundings!@n", TRUE, vict, k, 0, TO_CHAR);
      act("@C$n @wmanages to deflect the $p@w sending it flying away into the nearby surroundings!@n", TRUE, vict, k, 0, TO_ROOM);
      if (rroom_get_dmg(char_get_in_room(vict)) <= 95) {
       rroom_set_dmg(char_get_in_room(vict), rroom_get_dmg(char_get_in_room(vict)) + (5));
      }
      extract_obj(k);
      continue;
     }
    }
   } // Spiritball attack
  } // pursue the target.
 } // End for
}



int handle_block(struct char_data *ch)
{

 if (axion_dice(0) <= 4) { /* Critical failure */
  return (1);
 }

 if (!char_is_npc(ch)) { /* Players */
  if (!char_get_skill(ch, SKILL_BLOCK)) {
   return (0);
  } else {
   int num = char_get_skill(ch, SKILL_BLOCK);
   if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 3 || char_get_genome(ch, 1) == 3)) {
    num += 10;
   }
   if (char_get_skills_base(ch, SKILL_STYLE) >= 100) {
    num += 5;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 80) {
    num += 4;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 60) {
    num += 3;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 40) {
    num += 2;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 20) {
    num += 1;
   }
   return (num);
  }
 } else { /* Mobs */
  if (!char_is_humanoid(ch)) { /* Animal/monster types */
    int top = char_get_level(ch) / 4;

     if (top < 5)
      top = 6;
   return (rand_number(5, top));
  } else { /* Intelligent Skills Mobs */
   if (char_get_level(ch) >= 110) {
    return (rand_number(95, 105));
   } else if (char_get_level(ch) >= 100) {
    return (rand_number(85, 95));
   } else if (char_get_level(ch) >= 90) {
    return (rand_number(70, 85));
   } else if (char_get_level(ch) >= 75) {
    return (rand_number(50, 70));
   } else if (char_get_level(ch) >= 40) {
    return (rand_number(40, 50));
   } else {
    int top = char_get_level(ch);

     if (top < 15)
      top = 16;
    return (rand_number(15, top));
   }
  }
 }
}

int handle_dodge(struct char_data *ch)
{

 if (axion_dice(0) <= 4) { /* Critical failure */
  return (1);
 }

 if (!char_is_npc(ch)) { /* Players */
  if (!char_get_skill(ch, SKILL_DODGE)) {
   return (0);
  } else {
   int num = char_get_skill(ch, SKILL_DODGE);
   if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 3 || char_get_genome(ch, 1) == 3)) {
    num += 10;
   }
   if (char_get_skills_base(ch, SKILL_STYLE) >= 100) {
    num += 5;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 80) {
    num += 4;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 60) {
    num += 3;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 40) {
    num += 2;
   } else if (char_get_skills_base(ch, SKILL_STYLE) >= 20) {
    num += 1;
   }
   if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 100) {
    num += 3;
   } else if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 75) {
    num += 2;
   } else if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 50) {
    num += 1;
   }
   if (char_get_skills_base(ch, SKILL_ROLL) >= 100) {
    num += 5;
   } else if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 80) {
    num += 4;
   } else if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 60) {
    num += 3;
   } else if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 40) {
    num += 2;
   } else if (char_get_skills_base(ch, SKILL_SURVIVAL) >= 20) {
    num += 1;
   }
   if (group_bonus(ch, 2) == 8) {
    num += num * 0.05;
   }
  return (num);
  }
 } else { /* Mobs */
  if (!char_is_humanoid(ch)) { /* Animal/monster types */
    int top = (char_get_level(ch) + 1) / 8;

     if (top < 5)
      top = 6;
   return (rand_number(5, top));
  } else { /* Intelligent Skills Mobs */
   if (char_get_level(ch) >= 110) {
    return (rand_number(95, 105));
   } else if (char_get_level(ch) >= 100) {
    return (rand_number(75, 95));
   } else if (char_get_level(ch) >= 90) {
    return (rand_number(50, 85));
   } else if (char_get_level(ch) >= 75) {
    return (rand_number(30, 70));
   } else if (char_get_level(ch) >= 40) {
    return (rand_number(20, 50));
   } else {
    int top = char_get_level(ch);

     if (top < 15)
      top = 16;
    return (rand_number(15, top));
   }
  }
 }
}

int check_def(struct char_data *vict) {
  int index = 0;
  int pry = handle_parry(vict), dge = handle_dodge(vict), blk = handle_block(vict);

  index = pry + dge + blk;

  if (index > 0)
   index /= 3;

  if (char_aff_flagged(vict, AFF_KNOCKED)) {
   index = 0;
  }
  return index;
}

void handle_defense(struct char_data *vict, int *pry, int *blk, int *dge)
{

  if (!char_is_npc(vict)) {
   *pry = handle_parry(vict);

   *blk = handle_block(vict);
  
   *dge = handle_dodge(vict);
 
   if (char_get_bonuses(vict, BONUS_WALL)) {
    *blk += char_get_skill(vict, SKILL_BLOCK) * 0.20;
   }

   if (char_get_bonuses(vict, BONUS_PUSHOVER)) {
    *blk -= char_get_skill(vict, SKILL_BLOCK) * 0.20;
   }

   if (!char_get_equipment(vict, WEAR_WIELD1) && !char_get_equipment(vict, WEAR_WIELD2)) {
    *blk += 4;
   }

   if (*blk > 110) {
    *blk = 110;
   }

   if (char_get_bonuses(vict, BONUS_EVASIVE)) {
    *dge += (char_get_skill(vict, SKILL_DODGE) * 0.15);
   }

   if (char_get_bonuses(vict, BONUS_PUNCHINGBAG)) {
    *dge -= char_get_skill(vict, SKILL_DODGE) * 0.15;
   }
  
   if (*dge > 110) {
    *dge = 110;
   }

   if (*pry > 110) {
    *pry = 110;
   }
   if (char_plr_flagged(vict, PLR_GOOP) && rand_number(1, 100) >= 15) {
    *dge += 100;
    *blk += 100;
    *pry += 100;
   }
  }
  else {
   *pry = handle_parry(vict);

   *blk = handle_block(vict);

   *dge = handle_dodge(vict);
  }
 
  return;
}

void parry_ki(double attperc, struct char_data *ch, struct char_data *vict, char sname[1000], int prob, int perc, int skill, int type) {
     char buf[200];
     char buf2[200];
     char buf3[200];
     int foundv = FALSE, foundo = FALSE;
     int64_t dmg = 0;
     struct obj_data *tob, *next_obj;
     struct char_data *tch, *next_v;

     for (tch = world[char_get_in_room(ch)].people; tch; tch = next_v) {
      next_v = tch->next_in_room;

      if (tch == ch)
        continue;
      if (tch == vict)
        continue;
      if (!can_kill(ch, tch, NULL, 1))
        continue;

      if (rand_number(1, 101) >= 90 && foundv == FALSE) {
       if (handle_parry(tch) > rand_number(1, 140)) {
        sprintf(buf, "@C$N@W deflects your %s, sending it flying away!@n", sname);
        sprintf(buf2, "@WYou deflect @C$n's@W %s sending it flying away!@n", sname);
        sprintf(buf3, "@C$N@W deflects @c$n's@W %s sending it flying away!@n", sname);
        act(buf, TRUE, ch, 0, tch, TO_CHAR);
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
        act(buf3, TRUE, ch, 0, tch, TO_NOTVICT);
        foundv = FALSE;
       }
       else {
        foundv = TRUE;
        sprintf(buf, "@WYou watch as the deflected %s slams into @C$N@W, exploding with a roar of blinding light!@n", sname);
        sprintf(buf2, "@c$n@W watches as the deflected %s slams into you! The %s explodes with a roar of blinding light!@n", sname, sname);
        sprintf(buf3, "@c$n@W watches as the deflected %s slams into @C$N@W! The %s explodes with a roar of blinding light!@n", sname, sname);
        act(buf, TRUE, vict, 0, tch, TO_CHAR);
        act(buf2, TRUE, vict, 0, tch, TO_VICT);
        act(buf3, TRUE, vict, 0, tch, TO_NOTVICT);
        dmg = damtype(ch, type, skill, attperc);
        hurt(0, 0, ch, tch, NULL, dmg, 1);
        return;
       }
      }
     }

      for (tob = world[char_get_in_room(ch)].contents; tob; tob = next_obj) {
       next_obj = tob->next_content;
       if (obj_extra_flagged(tob, ITEM_UNBREAKABLE))
         continue;
       if (foundo == TRUE)
         continue;
       if (rand_number(1, 101) >= 80) {
        foundo = TRUE;
        sprintf(buf, "@WYou watch as the deflected %s slams into @g$p@W, exploding with a roar of blinding light!@n", sname);
        sprintf(buf2, "@c$n@W watches as the deflected %s slams into @g$p@W, exploding with a roar of blinding light!@n", sname);
        act(buf, TRUE, vict, tob, 0, TO_CHAR);
        act(buf2, TRUE, vict, tob, 0, TO_ROOM);
        hurt(0, 0, ch, NULL, tob, 25, 1);
        return;
       }
      }

      if ((foundo == FALSE || foundv == FALSE) && !rroom_flagged(char_get_in_room(vict), ROOM_SPACE)) {
      sprintf(buf, "@WYou watch as the deflected %s slams into the ground, exploding with a roar of blinding light!@n", sname);
      sprintf(buf2, "@WThe deflected %s slams into the ground, exploding with a roar of blinding light!@n", sname);
      act(buf, TRUE, vict, 0, 0, TO_CHAR);
      act(buf2, TRUE, vict, 0, 0, TO_ROOM);
      if (rroom_sect(char_get_in_room(vict)) != SECT_INSIDE && rroom_sect(char_get_in_room(vict)) != SECT_UNDERWATER && rroom_sect(char_get_in_room(vict)) != SECT_WATER_SWIM && rroom_sect(char_get_in_room(vict)) != SECT_WATER_NOSWIM && rroom_sect(char_get_in_room(vict)) != SECT_UNDERWATER && rroom_sect(char_get_in_room(vict)) != SECT_WATER_SWIM && rroom_sect(char_get_in_room(vict)) != SECT_WATER_NOSWIM) {
       impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
       switch (rand_number(1, 8)) {
        case 1:
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_CHAR);
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 2:
         if (rand_number(1, 4) == 4 && rroom_get_geffect(char_get_in_room(vict)) == 0) {
          rroom_set_geffect(char_get_in_room(vict), 1);
          act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", TRUE, ch, 0, vict, TO_CHAR);
          act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", TRUE, ch, 0, vict, TO_ROOM);
         }
        break;
        case 3:
         act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, vict, TO_CHAR);
         act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 4:
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 5:
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 6:
         act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.", TRUE, ch, 0, vict, TO_CHAR);
         act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.", TRUE, ch, 0, vict, TO_ROOM);
        break;
        default:
         /* we want no message for the default */
        break;
       }
      }
      if (rroom_sect(char_get_in_room(vict)) == SECT_UNDERWATER) {
       switch (rand_number(1, 3)) {
        case 1:
         act("The water churns violently!", TRUE, ch, 0, vict, TO_CHAR);
         act("The water churns violently!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 2:
         act("Large bubbles rise from the movement!", TRUE, ch, 0, vict, TO_CHAR);
         act("Large bubbles rise from the movement!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 3:
         act("The water collapses in on the hole created!", TRUE, ch, 0, vict, TO_CHAR);
         act("The water collapses in on the hole create!", TRUE, ch, 0, vict, TO_ROOM);
         break;
       }
      }
      if (rroom_sect(char_get_in_room(vict)) == SECT_WATER_SWIM || rroom_sect(char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
       switch (rand_number(1, 3)) {
        case 1:
         act("A huge column of water erupts from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("A huge column of water erupts from the impact!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 2:
         act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, vict, TO_CHAR);
         act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 3:
         act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        }
      }
      if (rroom_sect(char_get_in_room(vict)) == SECT_INSIDE) {
         impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
       switch (rand_number(1, 8)) {
        case 1:
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_CHAR);
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 2:
         act("The structure of the surrounding room cracks and quakes from the blast!", TRUE, ch, 0, vict, TO_CHAR);
         act("The structure of the surrounding room cracks and quakes from the blast!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 3:
         act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, vict, TO_CHAR);
         act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 4:
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 5:
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 6:
         act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, vict, TO_CHAR);
         act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        default:
         /* we want no message for the default */
        break;
       }
      }
      if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
       rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
      }
        int zone = 0;
      if ((zone = real_zone_by_thing(rroom_get_vnum(char_get_in_room(ch)))) != NOWHERE) {
       send_to_zone("An explosion shakes the entire area!\r\n", zone);
      }
      return;
     }
}

void dodge_ki(struct char_data *ch, struct char_data *vict, int type, int type2, int skill, int skill2) {
    if (type == 0 && !rroom_flagged(char_get_in_room(vict), ROOM_SPACE)) {
      if (rroom_sect(char_get_in_room(ch)) != SECT_INSIDE) {
       impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
       switch (rand_number(1, 8)) {
        case 1:
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_CHAR);
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 2:
         if (rand_number(1, 4) == 4 && rroom_get_geffect(char_get_in_room(vict)) == 0) {
         rroom_set_geffect(char_get_in_room(vict), 1);
         act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", TRUE, ch, 0, vict, TO_CHAR);
         act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", TRUE, ch, 0, vict, TO_ROOM);
         }
        break;
        case 3:
         act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, vict, TO_CHAR);
         act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 4:
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 5:
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 6:
         act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.", TRUE, ch, 0, vict, TO_CHAR);
         act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.", TRUE, ch, 0, vict, TO_ROOM);
        break;
        default:
         /* we want no message for the default */
        break;
       }
      }
      if (rroom_sect(char_get_in_room(vict)) == SECT_UNDERWATER) {
       switch (rand_number(1, 3)) {
        case 1:
         act("The water churns violently!", TRUE, ch, 0, vict, TO_CHAR);
         act("The water churns violently!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 2:
         act("Large bubbles rise from the movement!", TRUE, ch, 0, vict, TO_CHAR);
         act("Large bubbles rise from the movement!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 3:
         act("The water collapses in on the hole created!", TRUE, ch, 0, vict, TO_CHAR);
         act("The water collapses in on the hole create!", TRUE, ch, 0, vict, TO_ROOM);
         break;
       }
      }
      if (rroom_sect(char_get_in_room(vict)) == SECT_WATER_SWIM || rroom_sect(char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
       switch (rand_number(1, 3)) {
        case 1:
         act("A huge column of water erupts from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("A huge column of water erupts from the impact!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 2:
         act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, vict, TO_CHAR);
         act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        case 3:
         act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, vict, TO_ROOM);
         break;
        }
      }
      if (rroom_sect(char_get_in_room(ch)) == SECT_INSIDE) {
         impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
       switch (rand_number(1, 8)) {
        case 1:
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_CHAR);
         act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 2:
         act("The structure of the surrounding room cracks and quakes from the blast!", TRUE, ch, 0, vict, TO_CHAR);
         act("The structure of the surrounding room cracks and quakes from the blast!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 3:
         act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, vict, TO_CHAR);
         act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 4:
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 5:
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_CHAR);
         act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        case 6:
         act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, vict, TO_CHAR);
         act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, vict, TO_ROOM);
        break;
        default:
         /* we want no message for the default */
        break;
       }
      }
     if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
       rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
     }
        int zone = 0;
      if ((zone = real_zone_by_thing(rroom_get_vnum(char_get_in_room(ch)))) != NOWHERE) {
       send_to_zone("An explosion shakes the entire area!\r\n", zone);
      }
  }
  if (type == 1) {
   if (rand_number(1, 3) != 2) {
    act("@RIt turns around at the last second and begins to pursue @r$N@R!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@RIt turns around at the last second and begins to pursue YOU!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@RIt turns around at the last second and begins to pursue @r$N@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    struct obj_data *obj;
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
    obj_to_room(obj, char_get_in_room(ch));     
    
    obj_set_target(obj, vict);
    obj_set_kicharge(obj, damtype(ch, type2, skill, .2));
    obj_set_kitype(obj, skill2);
    obj_set_user(obj, ch);
   } else {
    act("@RIt fails to follow after @r$N@R!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@RIt fails to follow after YOU!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@RIt fails to follow after @r$N@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   }
  }
  if (type == 2 && (skill2 != 481 || char_is_frieza(ch))) {
    if (skill2 == 481) {
     int chance = rand_number(25, 50), prob = axion_dice(0);
     if (char_get_skill(ch, SKILL_KIENZAN) >= 100) {
      chance += chance * 0.8;
     } else if (char_get_skill(ch, SKILL_KIENZAN) >= 60) {
      chance += chance * 0.5;
     } else if (char_get_skill(ch, SKILL_KIENZAN) >= 40) {
      chance += chance * 0.25;
     }
     if (chance < prob) {
      return;
     }
    }
    act("@RYou turn it around and send it back after @r$N@R!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@W$n @Rturns it around and sends it back after YOU!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@W$n @Rturns it around and sends it back after @r$N@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    struct obj_data *obj;
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
    obj_to_room(obj, char_get_in_room(ch));

    obj_set_target(obj, vict);
    obj_set_kicharge(obj, damtype(ch, type2, skill, .3));
    obj_set_kitype(obj, skill2);
    obj_set_user(obj, ch);
  }
}

static void damtype_unarmed_infuse(char_data *ch, int64_t *dam) {
    if (char_aff_flagged(ch, AFF_INFUSE)) {
        *dam += (*dam / 100) * (char_get_skill(ch, SKILL_INFUSE) / 2);
        if (char_is_jinto(ch)) {
            if (char_get_skills_base(ch, SKILL_STYLE) >= 100) {
                *dam += ((*dam * 0.01) * (char_get_skill(ch, SKILL_INFUSE) / 2)) * 0.5;
            } else if (char_get_skills_base(ch, SKILL_STYLE) >= 60) {
                *dam += ((*dam * 0.01) * (char_get_skill(ch, SKILL_INFUSE) / 2)) * 0.25;
            } else if (char_get_skills_base(ch, SKILL_STYLE) >= 40) {
                *dam += ((*dam * 0.01) * (char_get_skill(ch, SKILL_INFUSE) / 2)) * 0.05;
            }
        }
    }
}

static void damtype_unarmed_hasshuken(char_data *ch, int64_t *dam) {
    if (char_aff_flagged(ch, AFF_HASS)) {
        *dam *= 2;
        if (char_is_krane(ch)) {
            if (char_get_skill(ch, SKILL_HASSHUKEN) >= 100) {
                *dam += *dam * 0.3;
            } else if (char_get_skill(ch, SKILL_HASSHUKEN) >= 60) {
                *dam += *dam * 0.2;
            } else if (char_get_skill(ch, SKILL_HASSHUKEN) >= 40) {
                *dam += *dam * 0.1;
            }
        }
    }
}

static void damtype_unarmed_hasshuken_or_infuse(char_data *ch, int64_t *dam) {
    if(char_aff_flagged(ch, AFF_HASS)) {
        damtype_unarmed_hasshuken(ch, dam);
    } else {
        damtype_unarmed_infuse(ch, dam);
    }
}

static void damtype_unarmed_preference(char_data *ch, int64_t *dam) {
    if (char_get_preference(ch) == PREFERENCE_THROWING) {
        *dam -= *dam * 0.15;
    } else if (char_get_preference(ch) == PREFERENCE_H2H) {
        *dam += *dam * 0.20;
    }
}

static void damtype_focus(char_data *ch, int64_t *dam, int64_t focus, int divby) {
    if (focus > 0) {
        *dam += focus * (*dam / divby);
    }
}

static void damtype_unarmed(char_data *ch, int skill, int64_t *dam) {
    // General Arlian bonus.
    if (char_is_arlian(ch)) {
        *dam += *dam * 0.02;
    }

    // Sixteen's Iron Hand Bonus.
    if (char_is_andsix(ch)) {
        if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
            *dam += *dam * 0.1;
    }

    // Brawler bonus
    if (char_get_bonuses(ch, BONUS_BRAWLER) > 0) {
        *dam += *dam * .2;
    }

    switch(skill) {
        // Punch.
        case 0:
            damtype_unarmed_hasshuken_or_infuse(ch, dam);
            // Kame Arts bonus.
            if (char_is_roshi(ch)) {
                if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
                    *dam += *dam * 0.2;
            }
            break;
        // Kick
        case 1:
            damtype_unarmed_infuse(ch, dam);
            // Crane Arts
            if (char_is_krane(ch)) {
                if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
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

static void damtype_human_grandmaster(char_data *ch, int skill, int64_t *dam) {
    if(char_is_human(ch)) {
        switch(skill) {
            case 101:
                *dam = *dam * 1.1;
            case 102:
                *dam = *dam * 1.2;
            case 103:
                *dam = *dam * 1.3;
        }
    }
}

static void damtype_human_ki(char_data *ch, int64_t *dam, int bon) {
    if (char_is_human(ch)) {
        *dam += (*dam / 100) * bon;
    }
}

static void damtype_saiyan_ki(char_data *ch, int64_t *dam, int bon) {
    if (char_is_saiyan(ch)) {
        *dam += (*dam / 100) * bon;
    }
}

static void damtype_kai_ki(char_data *ch, int64_t *dam, int bon) {
    if (char_is_kai(ch)) {
        *dam += (*dam / 100) * bon;
    }
}

static void damtype_icer_ki(char_data *ch, int64_t *dam, int bon) {
    if (char_is_icer(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4))) {
        *dam += (*dam / 100) * bon;
    }
}

/* Damage for player and NPC attacks  */
int64_t damtype(struct char_data *ch, int type, int skill, double percent)
{
 int64_t dam = 0, cou1 = 0, cou2 = 0, focus = 0;

 /* Player damages based on attack */
 if (!char_is_npc(ch)) {
  if (char_get_skill(ch, SKILL_FOCUS)) {
   focus = char_get_skill(ch, SKILL_FOCUS);
  }
  if (type != -2) {
   char_set_lastattack(ch, type);
  } else {
   type = 0;
  }

  // split switch approach to compress code.
  switch(type) {
      // big list of h2h stuff for starters.
      case -1:
          cou1 = 1 + ((skill / 4) * ((GET_HIT(ch) / 1200) + char_get_aff_abils_str(ch)));
          cou2 = 1 + ((skill / 4) * ((GET_HIT(ch) / 1000) + char_get_aff_abils_str(ch)));
          break;
      case 0: /* Punch */
          cou1 = 15 + ((skill / 4) * ((GET_HIT(ch) / 1600) + char_get_aff_abils_str(ch)));
          cou2 = 15 + ((skill / 4) * ((GET_HIT(ch) / 1300) + char_get_aff_abils_str(ch)));
          break;
      case 1: /* Kick */
          cou1 = 40 + ((skill / 4) * ((GET_HIT(ch) / 1200) + char_get_aff_abils_str(ch)));
          cou2 = 40 + ((skill / 4) * ((GET_HIT(ch) / 1000) + char_get_aff_abils_str(ch)));
          break;
      case 2: /* Elbow */
          cou1 = 100 + ((skill / 4) * ((GET_HIT(ch) / 1300) + char_get_aff_abils_str(ch)));
          cou2 = 100 + ((skill / 4) * ((GET_HIT(ch) / 1050) + char_get_aff_abils_str(ch)));
          break;
      case 3: /* Knee */
          cou1 = 150 + ((skill / 4) * ((GET_HIT(ch) / 1100) + char_get_aff_abils_str(ch)));
          cou2 = 150 + ((skill / 4) * ((GET_HIT(ch) / 1000) + char_get_aff_abils_str(ch)));
          break;
      case 4: /* Roundhouse */
          cou1 = 500 + ((skill / 4) * ((GET_HIT(ch) / 1000) + char_get_aff_abils_str(ch)));
          cou2 = 500 + ((skill / 4) * ((GET_HIT(ch) / 800) + char_get_aff_abils_str(ch)));
          break;
      case 5: /* Uppercut */
          cou1 = 350 + ((skill / 4) * ((GET_HIT(ch) / 1100) + char_get_aff_abils_str(ch)));
          cou2 = 350 + ((skill / 4) * ((GET_HIT(ch) / 900) + char_get_aff_abils_str(ch)));
          break;
      case 6: /* Slam */
          cou1 = 8000 + ((skill / 4) * ((GET_HIT(ch) / 800) + char_get_aff_abils_str(ch)));
          cou2 = 8000 + ((skill / 4) * ((GET_HIT(ch) / 500) + char_get_aff_abils_str(ch)));
          break;
      case 8: /* Heeldrop */
          cou1 = 12500 + ((skill / 4) * ((GET_HIT(ch) / 700) + char_get_aff_abils_str(ch)));
          cou2 = 12500 + ((skill / 4) * ((GET_HIT(ch) / 400) + char_get_aff_abils_str(ch)));
          break;
      case 51: /* Bash */
          cou1 = 1000 + ((skill / 4) * ((GET_HIT(ch) / 700) + char_get_aff_abils_str(ch)));
          cou2 = 1000 + ((skill / 4) * ((GET_HIT(ch) / 550) + char_get_aff_abils_str(ch)));
          break;
      case 52: /* Headbutt */
          cou1 = 800 + ((skill / 4) * ((GET_HIT(ch) / 900) + char_get_aff_abils_str(ch)));
          cou2 = 800 + ((skill / 4) * ((GET_HIT(ch) / 650) + char_get_aff_abils_str(ch)));
          break;
      case 56: /* TAILWHIP */
          cou1 = 400 + ((skill / 4) * ((GET_HIT(ch) / 1100) + char_get_aff_abils_str(ch)));
          cou2 = 400 + ((skill / 4) * ((GET_HIT(ch) / 1000) + char_get_aff_abils_str(ch)));
          break;
  }

  bool ki = false;
  // Set initial damage value.
  switch(type) {
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
          dam = large_rand(cou1, cou2);
          dam += char_get_aff_abils_str(ch) * (dam * 0.005);
          break;
      case 51:
      case 52:
      case 56: /* TAILWHIP */
          dam = large_rand(cou1, cou2);
          dam += char_get_level(ch) * 100;
          dam += char_get_aff_abils_str(ch) * (dam * 0.005);
          break;
      default: // all ki abilities.
          dam = GET_MAX_MANA(ch) * percent;
          ki = true;
  }

  // ki type move pre-processing
  switch(type) {
      case 11: /* Tsuihidan */
      case 12: /* Renzo */
      case 23: /* Rogafufuken */
      case 25: /* Kienzan */
          dam += char_get_level(ch) * 500;
          break;
      case 13: /* Kamehameha */
      case 16: /* Galik Gun */
      case 26: /* Tribeam */
      case 50: /* Seishou Enko */
          dam += char_get_level(ch) * 800;
          break;
      case 14: /* Masenko */
      case 30: /* Darkness Dragon Slash */
      case 44: /* Spiral Comet 1 */
      case 45: /* Spiral Comet 2 */
      case 43: /* Water Spikes */
          dam += char_get_level(ch) * 1000;
          break;
      case 15: /* Dodonpa */
      case 17: /* Deathbeam */
      case 19: /* Twin Slash */
          dam += char_get_level(ch) * 650;
          break;
      case 18: /* Eraser Cannon */
      case 33: /* Hell Spear Blast */
      case 54: /* Zen Blade */
      case 55: /* Sundering Force */
          dam += char_get_level(ch) * 700;
          break;
      case 20: /* Psychic Blast */
      case 27: /* Special Beam Cannon */
      case 29: /* Crusher Ball */
      case 37: /* Phoenix Slash */
          dam += char_get_level(ch) * 1200;
          break;
      case 21: /* Honoo */
      case 39: /* Spirit ball */
      case 47: /* Water Razor */
      case 48: /* Koteiru Bakuha */
      case 49: /* Hell Spiral */
          dam += char_get_level(ch) * 900;
          break;
      case 22: /* Dual Beam */
      case 24: /* Bakuhatsuha */
          dam += char_get_level(ch) * 600;
          break;
      case 28: /* Final Flash */
          dam += char_get_level(ch) * 1500;
      case 31: /* Psychic Barrage */
      case 36: /* Big Bang */
          dam += char_get_level(ch) * 1100;
          break;
      case 32: /* Hell Flash */
      case 46: /* Star Breaker */
          dam += char_get_level(ch) * 1400;
          break;
      case 34: /* Kakusanha */
          dam += char_get_level(ch) * 1050;
          break;
      case 35: /* Scatter Shot */
      case 53: /* Star Nova */
          dam += char_get_level(ch) * 1600;
          break;
      case 38: /* Deathball */
          dam += char_get_level(ch) * 1700;
          break;
      case 40: /* Genki Dama */
      case 41: /* Genocide */
          dam += char_get_level(ch) * 2000;
          break;
      case 42: /* Kousengan */
          dam += char_get_level(ch) * 550;
          break;
      case 57: /* Light Grenade */
          dam += char_get_level(ch) * 1700;
          break;
  }

  if(ki) dam *= 1.25;


  switch (type) {
   case -1:
    if(!char_plr_flagged(ch, PLR_THANDW))
        damtype_unarmed_hasshuken_or_infuse(ch, &dam);

    if (char_get_bonuses(ch, BONUS_BRAWLER) > 0) {
     dam += dam * .2;
    }
    if (char_get_preference(ch) == PREFERENCE_KI) {
     dam -= dam * 0.20;
    }
    if (char_get_preference(ch) == PREFERENCE_WEAPON && char_get_charge(ch) >= GET_MAX_MANA(ch) * 0.05) {
     dam += GET_MAX_MANA(ch) * 0.05;
     char_set_charge(ch, char_get_charge(ch) - (GET_MAX_MANA(ch) * 0.05));
    } else if (char_get_preference(ch) == PREFERENCE_WEAPON && char_get_charge(ch) > 0) {
     dam += char_get_charge(ch);
     char_set_charge(ch, char_get_charge(ch) - (0));
    }
    if (group_bonus(ch, 2) == 8) {
     dam += dam * 0.02;
    }
   break;
   case 0: /* Punch */
   case 1: /* Kick */
   case 2: /* Elbow */
   case 3: /* Knee */
   case 4: /* Roundhouse */
   case 5: /* Uppercut */
   case 6: /* Slam */
   case 8: /* Heeldrop */
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
    if (focus > 0) {
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
    dam += (dam / 100) * char_get_aff_abils_str(ch);
    damtype_focus(ch, &dam, focus, 200);
    if (char_get_bonuses(ch, BONUS_BRAWLER) > 0) {
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

    if (!char_is_npc(ch) && percent > 0.15) {
     double hitperc = (percent - 0.15) * 5;
      int64_t amount = getEffMaxPL(ch) * hitperc;
     int64_t difference = GET_HIT(ch) - amount;

     decCurHealth(ch, amount, 1);

     damtype_focus(ch, &dam, focus, 200);
   } else {
        damtype_focus(ch, &dam, focus, 200);
   }
    damtype_saiyan_ki(ch, &dam, 20);
   damtype_human_grandmaster(ch, skill, &dam);
   break;
   case 27: /* Special Beam Cannon */
    dam += (dam / 100) * char_get_aff_abils_intel(ch);
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
    if (!char_is_npc(ch)) {
     if (char_plr_flagged(ch, PLR_TRANS6)) {
      dam += dam;
     } else if (char_plr_flagged(ch, PLR_TRANS5)) {
      dam += (dam * 0.01) * 75;
     } else if (char_plr_flagged(ch, PLR_TRANS4)) {
      dam += (dam * 0.01) * 50;
     } else if (char_plr_flagged(ch, PLR_TRANS3)) {
      dam += (dam * 0.01) * 25;
     } else if (char_plr_flagged(ch, PLR_TRANS2)) {
      dam += (dam * 0.01) * 15;
     } else if (char_plr_flagged(ch, PLR_TRANS1)) {
      dam += (dam * 0.01) * 5;
     }
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
 else {
   dam = (GET_HIT(ch) * 0.05) + (GET_MAX_HIT(ch) * 0.025);
   dam += (dam * 0.005) * char_get_aff_abils_str(ch);
   if (char_get_level(ch) >= 120) {
    dam *= 0.25;
   } else if (char_get_level(ch) >= 110) {
    dam *= 0.45;
   } else if (char_get_level(ch) >= 100) {
    dam *= 0.75;
   }
  }
  
  if (char_is_npc(ch)) {

   if (type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 || type == 8 || type == 51 || type == 52 || type == 56) {
    dam += char_get_aff_abils_str(ch) * (dam * 0.005);
   } else {
    dam += char_get_aff_abils_intel(ch) * (dam * 0.005);
   }

   int64_t mob_hit = GET_HIT(ch);
   int64_t max_hit = GET_MAX_HIT(ch);
   int64_t mobperc = (mob_hit * 100) / max_hit;
   if (mobperc < 98 && mobperc >= 90) {
    dam = dam * 0.95;
   } else if (mobperc < 90 && mobperc >= 80) {
    dam = dam * 0.90;
   } else if (mobperc < 80 && mobperc >= 790) {
    dam = dam * 0.85;
   } else if (mobperc < 70 && mobperc >= 50) {
    dam = dam * 0.80;
   } else if (mobperc < 50 && mobperc >= 30) {
    dam = dam * 0.70;
   } else if (mobperc <= 29) {
    dam = dam * 0.60;
   }

   if (char_get_class(ch) != CLASS_NPC_COMMONER) {
    dam += dam * 0.3;
   }
  }

  
  if (char_get_kaioken(ch) > 0) {
   dam += (dam / 200) * char_get_kaioken(ch);
  }
  
  /* Start of Fury Mode for halfbreeds */
   if (char_plr_flagged(ch, PLR_FURY) && (type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 || type == 8 || type == 51 || type == 52)) {
    dam *= 1.5;
    act("Your rage magnifies your attack power!", TRUE, ch, 0, 0, TO_CHAR);
    act("Swirling energy flows around $n as $e releases $s rage in the attack!", TRUE, ch, 0, 0, TO_ROOM);
    if (rand_number(1, 10) >= 7) {
     send_to_char(ch, "You feel less angry.\r\n");
     char_plr_set(ch, PLR_FURY, false);
    }
   }
   else if (char_plr_flagged(ch, PLR_FURY)) {
    dam *= 2;
    act("Your rage magnifies your attack power!", TRUE, ch, 0, 0, TO_CHAR);
    act("Swirling energy flows around $n as $e releases $s rage in the attack!", TRUE, ch, 0, 0, TO_ROOM);
    char_plr_set(ch, PLR_FURY, false);
   }
  /* End of Fury Mode for halfbreeds */

  if ((type == -1 || type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 || type == 8)) {
   if (!char_is_npc(ch))
    dam -= dam * 0.08;
   if (!char_is_npc(ch) && dam > GET_MAX_HIT(ch) * 0.1)
    dam *= 0.6;
  } else {
   dam += (dam * 0.005) * char_get_aff_abils_intel(ch);
   if (char_get_preference(ch) == PREFERENCE_WEAPON) {
    dam -= dam * 0.25;
   } else if (char_get_preference(ch) == PREFERENCE_THROWING) {
    dam -= dam * 0.15;
   }
  }

  return (dam);
}

void saiyan_gain(struct char_data *ch, struct char_data *vict)
{
 int gain = rand_number(char_get_level(ch) * 6, char_get_level(ch) * 8);
 int weak = FALSE;

  if (!vict)
   return;

  if (char_is_npc(ch))
   return;
  
  if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) / 10) {
   weak = TRUE;
  }

  if (char_get_level(ch) > 99) {
   gain += rand_number(char_get_level(ch) * 300, char_get_level(ch) * 500);
  }
  else if (char_get_level(ch) > 80) {
   gain += rand_number(char_get_level(ch) * 150, char_get_level(ch) * 200);
  }
  else if (char_get_level(ch) > 60) {
   gain += rand_number(char_get_level(ch) * 80, char_get_level(ch) * 100);
  }
  else if (char_get_level(ch) > 50) {
   gain += rand_number(char_get_level(ch) * 20, char_get_level(ch) * 25);
  }
  else if (char_get_level(ch) > 40) {
   gain += rand_number(char_get_level(ch) * 8, char_get_level(ch) * 10);
  }
  else if (char_get_level(ch) > 30) {
   gain += rand_number(char_get_level(ch) * 5, char_get_level(ch) * 8);
  }
  else {
  }

  if (char_is_bio(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2)) {
   gain /= 2;
  }
 if (rand_number(1, 22) >= 18 && (char_get_level(ch) == 100 || level_exp(ch, char_get_level(ch) + 1) - (char_get_exp(ch)) > 0)) {
  if (weak) {
     send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WThey are too weak to inspire your saiyan soul!@n\r\n");
   } else {
    int stats[3] = {0, 1, 2};
    int available[3];
    int avail_count = 0;

    for (int i = 0; i < 3; i++) {
        if (!is_soft_cap(ch, stats[i], 1.5)) {
            available[avail_count++] = stats[i];
        }
    }

    if (avail_count == 0) {
        send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WYou feel you have reached your current limits.@n\r\n");
        return;
    }

    int choice = available[rand_number(0, avail_count - 1)];

    switch (choice) {
     case 0:
         gainBasePL(ch, gain, false);
       send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WYou feel slightly stronger. @D[@G+%s@D]@n\r\n", add_commas(gain));
      break;
     case 1:
         gainBaseKI(ch, gain, false);
       send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WYou feel your spirit grow. @D[@G+%s@D]@n\r\n", add_commas(gain));
      break;
     case 2:
         gainBaseST(ch, gain, false);
       send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WYou feel slightly more vigorous. @D[@G+%s@D]@n\r\n", add_commas(gain));
      break;
    }
   }
 }

}

void spar_gain(struct char_data *ch, struct char_data *vict, int type, int64_t dmg)
{
 int chance = 0, gmult, gravity, bonus = 1, pscost = 2, difference = 0;
 int64_t gain = 0, pl = 0, ki = 0, st = 0, gaincalc = 0;

 if (ch != NULL && !char_is_npc(ch)) {
  if (dmg > GET_MAX_HIT(vict) / 10) {
   chance = rand_number(20, 100);
  }
  else if (dmg <= GET_MAX_HIT(vict) / 10) {
   chance = rand_number(1, 75);
  }

  if (char_get_relax_count(ch) >= 464) {
   chance = 0;
  } else if (char_get_relax_count(ch) >= 232) {
   chance -= chance * 0.5;
  } else if (char_get_relax_count(ch) >= 116) {
   chance -= chance * 0.2;
  } 

  gravity = rroom_get_gravity(char_get_in_room(ch));
  gmult = char_get_level(ch) * ((gravity / 5) + 6);
 
  if (char_get_equipment(ch, WEAR_SH)) {
   struct obj_data *obj = char_get_equipment(ch, WEAR_SH);
   if (obj_get_vnum(obj) == 1127) {
    gmult *= 4;
   }
  }

 
  
   if (rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT) || (rroom_flagged(char_get_in_room(ch), ROOM_HBTC))) {
    if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
     gmult *= 1.75;
     pscost += 2;
    } else {
     gmult *= 1.25;
     pscost += 1;
    }
    pl = large_rand(gmult * .8, gmult * 1.2);
    ki = large_rand(gmult * .8, gmult * 1.2);
   } else {
    pl = large_rand(gmult * .4, gmult * .8);
    ki = large_rand(gmult * .4, gmult * .8);
   }
  if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) < 0 && char_get_level(ch) < 100) {
   pl = 0;
  }
  if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) < 0 && char_get_level(ch) < 100) {
   ki = 0;
  }

  if (chance >= rand_number(60, 75)) {
   int64_t num = 0,  maxnum = 500000;
   if (char_get_level(ch) >= 70) {
    num += char_get_level(ch) * 10000;
   } else if (char_get_level(ch) >= 60) {
    num += char_get_level(ch) * 6000;
   } else if (char_get_level(ch) >= 50) {
    num += char_get_level(ch) * 5000;
   } else if (char_get_level(ch) >= 45) {
    num += char_get_level(ch) * 2500;
   } else if (char_get_level(ch) >= 40) {
    num += char_get_level(ch) * 2200;
   } else if (char_get_level(ch) >= 35) {
    num += char_get_level(ch) * 1500;
   } else if (char_get_level(ch) >= 30) {
    num += char_get_level(ch) * 1200;
   } else if (char_get_level(ch) >= 25) {
    num += char_get_level(ch) * 550;
   } else if (char_get_level(ch) >= 20) {
    num += char_get_level(ch) * 400;
   } else if (char_get_level(ch) >= 15) {
    num += char_get_level(ch) * 250;
   } else if (char_get_level(ch) >= 10) {
    num += char_get_level(ch) * 100;
   } else if (char_get_level(ch) >= 5) {
    num += char_get_level(ch) * 50;
   } else {
    num += char_get_level(ch) * 30;
   }
   if (num > maxnum) {
    num = maxnum;
   }
   if (vict != NULL && char_is_npc(vict)) {
    num = num * 0.7;
    gaincalc = num * 1.5;
    type = 3;
   } else if (vict != NULL && !char_is_npc(vict)) {
    gaincalc = large_rand(num * .7, num);
    if (char_get_level(ch) > char_get_level(vict))
     difference = char_get_level(ch) - char_get_level(vict);
    else if (char_get_level(ch) < char_get_level(vict))
     difference = char_get_level(vict) - char_get_level(ch);
   } else {
    gaincalc = 0;
   }
   if (vict != NULL) {
    if (difference >= 51) {
     send_to_char(ch, "The difference in your levels is too great for you to gain anything.\r\n");
     return;
    } else if (difference >= 40) {
     gaincalc = gaincalc * 0.05;
    } else if (difference >= 30) {
     gaincalc = gaincalc * 0.10;
    } else if (difference >= 20) {
     gaincalc = gaincalc * 0.25;
    } else if (difference >= 10) {
     gaincalc = gaincalc * 0.50;
    }
    if (!char_is_npc(vict)) {
     if (char_prf_flagged(vict, PRF_INSTRUCT)) {
      if (GET_PRACTICES(vict, char_get_class(vict)) > 10) {
       send_to_char(vict, "You instruct them in proper fighting techniques and strategies.\r\n");
       act("You take $N's instruction to heart and gain more experience.\r\n", FALSE, ch, 0, vict, TO_CHAR);
       GET_PRACTICES(vict, char_get_class(vict)) -= 10;
       bonus = 2;
      }
     }
    }
   }
   
   if (char_is_saiyan(ch)) {
    gaincalc = gaincalc + (gaincalc * .50);
   }
   if (char_is_halfbreed(ch)) {
    gaincalc = gaincalc + (gaincalc * .40);
   }
   if (char_is_icer(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4))) {
    gaincalc = gaincalc - (gaincalc * .20);
   }
   if (rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT) || (rroom_flagged(char_get_in_room(ch), ROOM_HBTC))) {
    if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
     gaincalc *= 1.5;
    } else {
     gaincalc *= 1.25;
    }
   }
   gain = gear_exp(ch, gaincalc);
   if (GET_PRACTICES(ch, char_get_class(ch)) >= pscost) {
    GET_PRACTICES(ch, char_get_class(ch)) -= pscost;
    gain = gain * bonus;
    gain_exp(ch, gain);
    send_to_char(ch, "@D[@Y+ @G%s @mExp@D]@n ", add_commas(gain));
    if (type == 0 && rand_number(1, 5) >= 4) {
     send_to_char(ch, "@D[@Y+ @R%s @rPL@D]@n ", pl > 0 ? add_commas(pl) : "SOFT-CAP");
     gainBasePL(ch, pl, false);
    }
    else if (type == 1 && rand_number(1, 5) >= 4) {
     send_to_char(ch, "@D[@Y+ @C%s @cKi@D]@n ", ki > 0 ? add_commas(ki) : "SOFT-CAP");
     gainBaseKI(ch, ki, false);
    }
    send_to_char(ch, "@D[@R- @M%d @mPS@D]@n ", pscost);
    send_to_char(ch, "\r\n");
   } else {
    send_to_char(ch, "@RYou need at least %d Practice Sessions in order to gain while sparring here.@n\r\n", pscost);
   }
  }
 }

 if (vict != NULL && !char_is_npc(vict) && !char_is_npc(ch)) {
  if (dmg > GET_MAX_HIT(vict) / 4) {
   chance = rand_number(1, 100);
  }
  else if (dmg <= GET_MAX_HIT(vict) / 4) {
   chance = rand_number(1, 70);
  }

  if (char_get_relax_count(vict) >= 464) {
   chance = 0;
  } else if (char_get_relax_count(vict) >= 232) {
   chance -= chance * 0.5;
  } else if (char_get_relax_count(vict) >= 116) {
   chance -= chance * 0.2;
  }

  gravity = rroom_get_gravity(char_get_in_room(ch));
  gmult = char_get_level(vict) * ((gravity / 5) + 6);
   if (rroom_flagged(char_get_in_room(vict), ROOM_WORKOUT) || (rroom_flagged(char_get_in_room(ch), ROOM_HBTC))) {
    if (rroom_get_vnum(char_get_in_room(vict)) >= 19100 && rroom_get_vnum(char_get_in_room(vict)) <= 19199) {
     gmult *= 1.75;
    } else {
     gmult *= 1.25;
    }
     st = large_rand(gmult * .8, gmult * 1.2);
   } else {
    st = large_rand(gmult * .4, gmult * .8);
   }

  if (level_exp(vict, char_get_level(vict) + 1) - char_get_exp(vict) < 0 && char_get_level(vict) < 100) {
   st = 0;
  }

  if (chance >= rand_number(60, 75)) {
   int64_t num = 0, maxnum = 500000;

   if (char_get_level(vict) >= 70) {
    num += char_get_level(vict) * 10000;
   } else if (char_get_level(vict) >= 60) {
    num += char_get_level(vict) * 6000;
   } else if (char_get_level(vict) >= 50) {
    num += char_get_level(vict) * 5000;
   } else if (char_get_level(vict) >= 45) {
    num += char_get_level(vict) * 2500;
   } else if (char_get_level(vict) >= 40) {
    num += char_get_level(vict) * 2200;
   } else if (char_get_level(vict) >= 35) {
    num += char_get_level(vict) * 1500;
   } else if (char_get_level(vict) >= 30) {
    num += char_get_level(vict) * 1200;
   } else if (char_get_level(vict) >= 25) {
    num += char_get_level(vict) * 550;
   } else if (char_get_level(vict) >= 20) {
    num += char_get_level(vict) * 400;
   } else if (char_get_level(vict) >= 15) {
    num += char_get_level(vict) * 250;
   } else if (char_get_level(vict) >= 10) {
    num += char_get_level(vict) * 100;
   } else if (char_get_level(vict) >= 5) {
    num += char_get_level(vict) * 50;
   } else {
    num += char_get_level(vict) * 30;
   }
   if (num > maxnum) {
    num = maxnum;
   }
   gain = large_rand(num * .7, num);

    if (difference > 50) {
     send_to_char(ch, "The difference in your levels is too great for you to gain anything.\r\n");
     return;
    } else if (difference >= 40) {
     gain = gain * 0.05;
    } else if (difference >= 30) {
     gain = gain * 0.10;
    } else if (difference >= 20) {
     gain = gain * 0.25;
    } else if (difference >= 10) {
     gain = gain * 0.50;
    }

   if (char_is_saiyan(vict) || char_is_halfbreed(vict)) {
    gain = gain + (gain * .30);
   }
   if (char_is_icer(vict)) {
    gain = gain - (gain * .10);
   }
   if (rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT) || (rroom_flagged(char_get_in_room(ch), ROOM_HBTC))) {
    if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
     gain *= 1.5;
    } else {
     gain *= 1.25;
    }
   }
   if (GET_PRACTICES(vict, char_get_class(vict)) >= pscost) {
    GET_PRACTICES(vict, char_get_class(vict)) -= pscost;
    send_to_char(vict, "@D[@Y+ @G%s @mExp@D]@n ", add_commas(gain));
    gain = gear_exp(vict, gain);
    gain_exp(vict, gain);
    if (rand_number(1, 5) >= 4) {
     send_to_char(vict, "@D[@Y+ @G%s @gSt@D]@n ", st > 0 ? add_commas(st) : "SOFT-CAP");
     gainBaseST(vict, st, false);
    }
    send_to_char(vict, "@D[@R- @M%d @mPS@D]@n ", pscost);
    send_to_char(vict, "\r\n");
   } else {
    send_to_char(vict, "@RYou need at least %d Practice Sessions in order to gain while sparring here.@n\r\n", pscost);    
   }
  }
 }
}





/* Main damage function for RDBS 'Real Dragonball Battle System' */
void hurt(int limb, int chance, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int64_t dmg, int type)
{
 int64_t index = 0;
 int64_t maindmg = dmg, beforered = dmg;
 int dead = FALSE;

 /* If a character is trageted */

 if (type <= 0) {
  if (char_is_saiyan(ch) && char_plr_flagged(ch, PLR_STAIL)) {
   dmg += dmg * .15;
  }
  if (char_is_namek(ch) && !char_get_equipment(ch, WEAR_HEAD)) {
   dmg += dmg * .25;
  }
  if (group_bonus(ch, 2) == 4) {
   dmg += dmg * .1;
  } else if (group_bonus(ch, 2) == 12) {
   dmg -= dmg * .1;
  }
 } else {
  /* human racial bonus on hold */
  /*if (IS_HUMAN(ch) && !IS_NPC(ch)) {
   if (PLR_FLAGGED(ch, PLR_TRANS3)) {
    dmg += dmg * 0.45;
   }
  }*/
  dmg = dmg * .6;
  if (group_bonus(ch, 2) == 9) {
   dmg -= dmg * 0.1;
  }
  if (char_aff_flagged(ch, AFF_POTENT)) {
   dmg += dmg * 0.3;
   send_to_room(char_get_in_room(ch), "@wThere is a bright flash of @Yyellow@w light in the wake of the attack!@n\r\n");
  }
 }

 if (char_aff_flagged(ch, AFF_INFUSE) && !char_aff_flagged(ch, AFF_HASS) && type <= 0) {
     int64_t infuse_cost = getPercentOfMaxKI(ch, .005);
  if (dmg > 0) {
      if (getCurKI(ch) - infuse_cost) {
          decCurKI(ch, infuse_cost, 0);
          send_to_room(char_get_in_room(ch), "@CA swirl of ki explodes from the attack!@n\r\n");
      } else {
          act("@wYou can no longer infuse ki into your attacks!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@c$n@w can no longer infuse ki into $s attacks!@n", TRUE, ch, 0, 0, TO_ROOM);
          char_aff_set(ch, AFF_INFUSE, false);
      }
  }
 }

 if (vict) {

   if (rroom_get_vnum(char_get_in_room(vict)) == 17875) {
    return;
   }

   reveal_hiding(vict, 0);
   if (char_aff_flagged(vict, AFF_PARALYZE)) {
    send_to_char(ch, "They are a statue and can't be harmed\r\n");
    return;
   }

  if (char_get_kaioken(ch) > 0) {
   dmg += (dmg / 100) * (char_get_kaioken(ch) * 2);
  }

  if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 8 || char_get_genome(vict, 1) == 8) && type == 0) {
   int64_t drain = dmg * 0.1;
   dmg -= drain;
   decCurST(ch, drain, 1);
   act("@Y$N's rubbery body makes hitting it tiring!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@Y$n's stamina is sapped a bit by hitting your rubbery body!@n", TRUE, ch, 0, vict, TO_VICT);
  }

  
  if (!char_is_npc(ch) ) {
   if (char_plr_flagged(ch, PLR_OOZARU)) {
    dmg += dmg * 0.30;
   }
  } if (!char_is_npc(vict)) {
   if (char_plr_flagged(vict, PLR_OOZARU)) {
    dmg -= dmg * 0.30;
   }
  }


 if(type > -1) {
  if (char_get_lastattack(ch) != 11 && char_get_lastattack(ch) != 39 && char_get_lastattack(ch) != 500 && char_get_lastattack(ch) < 1000) {
   if (handle_combo(ch, vict) > 0) {
    if (beforered <= 1) {
     char_set_combo(ch, -1);
     char_set_combhits(ch, 0);
     send_to_char(ch, "@RYou have cut your combo short because you missed your last hit!@n\r\n");
    } else if (char_get_combhits(ch) < physical_mastery(ch)) {
     dmg += combo_damage(ch, dmg, 0);
     if ((char_get_combhits(ch) == 10 || char_get_combhits(ch) == 20 || char_get_combhits(ch) == 30) && (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 || char_get_level(ch) == 100)) {
      int64_t gain = char_get_level(ch) * 1000;
      if (char_get_skill(ch, SKILL_STYLE) >= 100) {
       gain += gain * 2;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 80) {
       gain += gain * 0.4;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 60) {
       gain += gain * 0.3;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 40) {
       gain += gain * 0.2;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 20) {
       gain += gain * 0.1;
      }
      gain_exp(ch, gain);
      send_to_char(ch, "@D[@mExp@W: @G%s@D]@n\r\n", add_commas(gain));
     }
    } else {
     dmg += combo_damage(ch, dmg, 1);
     if ((char_get_combhits(ch) == 10 || char_get_combhits(ch) == 20 || char_get_combhits(ch) == 30) && (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 || char_get_level(ch) == 100)) {
      int64_t gain = char_get_level(ch) * 1000;
      if (char_get_skill(ch, SKILL_STYLE) >= 100) {
       gain += gain * 2;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 80) {
       gain += gain * 0.4;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 60) {
       gain += gain * 0.3;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 40) {
       gain += gain * 0.2;
      } else if (char_get_skill(ch, SKILL_STYLE) >= 20) {
       gain += gain * 0.1;
      }
      gain_exp(ch, gain);
      send_to_char(ch, "@D[@mExp@W: @G%s@D]@n\r\n", add_commas(gain));
     }
     char_set_combo(ch, -1);
     char_set_combhits(ch, 0);
    }
   }
  } else if (char_get_combhits(ch) > 0 && char_get_lastattack(ch) < 1000) {
     send_to_char(ch, "@RYou have cut your combo short because you used the wrong attack!@n\r\n");
     char_set_combo(ch, -1);
     char_set_combhits(ch, 0);
  }
 }

  if (char_get_lastattack(ch) >= 1000) {
   char_set_lastattack(ch, char_get_lastattack(ch) - (1000));
  }

  if (char_get_preference(ch) == PREFERENCE_KI && char_get_charge(ch) > 0) {
   dmg -= dmg * 0.08;
  }

  if (char_aff_flagged(vict, AFF_SANCTUARY)) {
   if (char_get_skill(vict, SKILL_AQUA_BARRIER)) {
    if (!rroom_sunken(char_get_in_room(ch))) {
     dmg = dmg * 0.85;
    } else {
     dmg = dmg * 0.75;
    }
   }
   if (char_get_barrier(vict) - dmg > 0) {
    act("@c$N's@C barrier absorbs the damage!@n", TRUE, ch, 0, vict, TO_CHAR);
    char barr[MAX_INPUT_LENGTH];
    sprintf(barr, "@CYour barrier absorbs the damage! @D[@B%s@D]@n", add_commas(dmg));
    act(barr, TRUE, ch, 0, vict, TO_VICT);
    act("@c$N's@C barrier absorbs the damage!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    char_set_barrier(vict, char_get_barrier(vict) - (dmg));
    dmg = 0;
   }
   else if (char_get_barrier(vict) - dmg <= 0) {
    dmg -= char_get_barrier(vict);
    char_set_barrier(vict, 0);
    act("@c$N's@C barrier bursts!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@CYour barrier bursts!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@c$N's@C barrier bursts!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_SANCTUARY, false);
   }
  }
  if (char_aff_flagged(vict, AFF_FIRESHIELD) && rand_number(1, 200) < char_get_skill(vict, SKILL_FIRESHIELD)) {
    act("@c$N's@C fireshield repels the damage!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@CYour fireshield repels the damage!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@c$N's@C fireshield repels the damage!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    if (rand_number(1, 3) == 3) {
     act("@c$N's@C fireshield disappears...@n", TRUE, ch, 0, vict, TO_CHAR);
     act("@CYour fireshield disappears...@n", TRUE, ch, 0, vict, TO_VICT);
     act("@c$N's@C fireshield disappears...@n", TRUE, ch, 0, vict, TO_NOTVICT);
     char_aff_set(vict, AFF_FIRESHIELD, false);
    }
    dmg = 0;
  }

  int64_t conlimit = 2000000000;
  
  if (type == 0) {
    int64_t vpl = GET_MAX_HIT(vict);
    int vcon = char_get_aff_abils_con(vict);

   if (vpl < conlimit) {
    index += (vpl / 1500) * (vcon / 2);
   } else if (vpl < conlimit * 2) {
    index += (vpl / 2500) * (vcon / 2);
   } else if (vpl < conlimit * 3) {
    index += (vpl / 3500) * (vcon / 2);
   } else if (vpl < conlimit * 5) {
    index += (vpl / 6000) * (vcon / 2);
   } else if (vpl < conlimit * 10) {
    index += (vpl / 8500) * (vcon / 2);
   } else if (vpl < conlimit * 15) {
    index += (vpl / 10000) * (vcon / 2);
   } else if (vpl < conlimit * 20) {
    index += (vpl / 12500) * (vcon / 2);
   } else if (vpl < conlimit * 25) {
    index += (vpl / 16000) * (vcon / 2);
   } else if (vpl < conlimit * 30) {
    index += (vpl / 22000) * (vcon / 2);
   } else if (vpl > conlimit * 30) {
    index += (vpl / 25000) * (vcon / 2);
   }
  }

  if (char_is_npc(vict) && char_get_level(vict) > 0) {
   index /= 3;
  } else if (char_is_npc(vict) && char_get_level(vict) < 40) {
   index /= 3;
  }

  index += armor_calc(vict, dmg, type);

  if (char_aff_flagged(vict, AFF_STONESKIN)) {
   if (char_get_level(vict) < 20) {
    index += char_get_level(vict) * 250;
   }
   else if (char_get_level(vict) < 30) {
    index += char_get_level(vict) * 500;
   }
   else if (char_get_level(vict) < 50) {
    index += char_get_level(vict) * 1000;
   }
   else if (char_get_level(vict) < 60) {
    index += char_get_level(vict) * 2000;
   }
   else if (char_get_level(vict) < 70) {
    index += char_get_level(vict) * 5000;
   }
   else if (char_get_level(vict) < 90) {
    index += char_get_level(vict) * 10000;
   }
   else if (char_get_level(vict) <= 100) {
    index += char_get_level(vict) * 25000;
   }
  }

  if (char_aff_flagged(vict, AFF_SHELL)) {
   dmg = dmg * 0.75;
  }

  if (is_affected(vict, AFF_WITHER)) {
   dmg += (dmg * 0.01) * 20;
  }

  if (!char_is_npc(vict) && GET_COND(vict, DRUNK) > 4) {
   dmg -= (dmg * 0.001) * GET_COND(vict, DRUNK);
  }

  if (char_aff_flagged(vict, AFF_EARMOR)) {
   dmg -= dmg * 0.1;
  }

  if (type > 0) {
   advanced_energy(vict, dmg);
   dmg -= (dmg * 0.0005) * char_get_aff_abils_wis(vict);
  }

  if (char_is_mutant(vict)) {  
   if (type <= 0) {
    dmg -= dmg * 0.3;
   }
   else if (type > 0) {
    dmg -= dmg * 0.25;
   }
  }

  if (char_get_bonuses(vict, BONUS_THICKSKIN)) {
   if (type <= 0) {
    dmg -= dmg * 0.20;
   } else {
    dmg -= dmg * 0.10;
   }
  } else if (char_get_bonuses(vict, BONUS_THINSKIN)) {
   if (type <= 0) {
    dmg += dmg * 0.20;
   } else {
    dmg += dmg * 0.10;
   }
  }

  if (char_plr_flagged(vict, PLR_FURY)) {
    dmg -= dmg * 0.1;
  }

  if (char_is_majin(vict)) {
   if (type <= 0) {
    dmg -= dmg * 0.5;
   }
  }

  if (char_is_kai(vict)) {
   dmg += dmg * 0.15;
  }

 if (char_get_grappling(ch) == vict && char_get_grap(ch) == 3) {
  dmg += (dmg / 100) * 20;
 }

 if (!char_is_npc(vict) && char_get_skill(vict, SKILL_ARMOR)) {
  int nanite = char_get_skill(vict, SKILL_ARMOR), perc = rand_number(1, 220);
  if (char_plr_flagged(vict, PLR_SENSEM)) {
   perc = rand_number(1, 176);
  }
  if (nanite >= perc) {
   if (char_plr_flagged(vict, PLR_TRANS6)) {
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block MOST of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block MOST of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 50;
   } else if (char_plr_flagged(vict, PLR_TRANS5)){
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 40;
   } else if (char_plr_flagged(vict, PLR_TRANS4)){
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a lot of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a lot of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 30;
   } else if (char_plr_flagged(vict, PLR_TRANS3)){
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a good deal of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a good deal of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 25;
   } else if (char_plr_flagged(vict, PLR_TRANS2)){
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 20;
   } else if (char_plr_flagged(vict, PLR_TRANS1)){
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a bit of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a bit of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 10;
   } else {
    act("@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a tiny bit of the damage!@n", TRUE, vict, 0, 0, TO_CHAR);
    act("@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a tiny bit of the damage!@n", TRUE, vict, 0, 0, TO_ROOM);
    dmg -= (dmg * 0.01) * 5;
   }
  }
 }

 if (!char_aff_flagged(vict, AFF_KNOCKED) && (char_get_position(vict) == POS_SITTING || char_get_position(vict) == POS_RESTING) && char_get_skill(vict, SKILL_ROLL) > axion_dice(0)) {
   int64_t rollcost = getMaxHealth(vict) / 300 * (char_get_aff_abils_str(ch) / 2);
  if ((getCurST(vict)) >= rollcost) {
   act("@GYou roll to your feet in an agile fashion!@n", TRUE, vict, 0, 0, TO_CHAR);
   act("@G$n rolls to $s feet in an agile fashion!@n", TRUE, vict, 0, 0, TO_ROOM);
   do_stand(vict, 0, 0, 0);
   decCurST(vict, rollcost, 0);
  }
 }

 if (char_is_npc(vict)) {
  hitprcnt_mtrigger(vict);
 }

 if (char_is_humanoid(vict) && !char_is_npc(ch) && char_is_npc(vict) && (!is_sparring(ch) || !is_sparring(vict))) {
  remember(vict, ch);
 }
  if (char_is_npc(vict) && GET_HIT(vict) > ((getEffMaxPL(vict))) / 4) {
    char_set_lasthit(vict, char_get_idnum(ch));
  }
  if (char_aff_flagged(vict, AFF_SLEEP) && rand_number(1, 2) == 2) {
    affect_from_char(vict, SPELL_SLEEP);
    act("@c$N@W seems to be more aware now.@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@WYou are no longer so sleepy.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@c$N@W seems to be more aware now.@n", TRUE, ch, 0, vict, TO_NOTVICT);
  }
  if (char_aff_flagged(vict, AFF_KNOCKED) && rand_number(1, 12) >= 11) {
      cureStatusKnockedOut(vict, true);
      if (char_is_npc(vict) && rand_number(1, 20) >= 12) {
          act("@W$n@W stands up.@n", FALSE, vict, 0, 0, TO_ROOM);
          char_set_position(vict, POS_STANDING);
      }
  }
  if (char_is_npc(ch)) {
   if (char_get_level(ch) > 10) {
    if (dmg - index > 0) {
     dmg -= index;
    }
    else if (dmg - index <= 0 && dmg >= 1) {
     dmg = 1;
    }
   }
   else if (char_get_level(ch) <= 10) {
    dmg = (dmg * .8);
   }
  }
  if (!char_is_npc(ch)) {
    if (dmg >= 1) {
     if ((dmg + (dmg * 0.5)) - index <= 0) {
      dmg = 1;
     } else if ((dmg + (dmg * 0.4)) - index <= 0) {
      dmg = dmg * 0.04;
     } else if ((dmg + (dmg * 0.3)) - index <= 0) {
      dmg = dmg * 0.08;
     } else if ((dmg + (dmg * 0.2)) - index <= 0) {
      dmg = dmg * 0.12;
     } else if ((dmg + (dmg * 0.1)) - index <= 0) {
      dmg = dmg * 0.16;
     } else if (dmg - index <= 0) {
      dmg = dmg * 0.2;
     } else if (dmg - index > dmg * 0.25) {
      dmg -= index;
     } else {
      dmg = dmg * 0.25;
     }
    }
  }
  if (dmg < 1) {
   dmg = 0;
  }
  if (dmg >= 50 && chance > 0) {
   hurt_limb(ch, vict, chance, limb, dmg);
  }
   if (char_is_npc(vict) && dmg > getMaxHealth(vict) * .7 && char_get_bonuses(ch, BONUS_SADISTIC) > 0) {
   char_set_exp(vict, char_get_exp(vict) / (2));
  } else if (char_is_npc(vict) && dmg > getCurHealth(vict) && isFullHealth(vict) * .5 && char_get_bonuses(ch, BONUS_SADISTIC) > 0) {
   char_set_exp(vict, char_get_exp(vict) / (2));
  }
 

  if (CARRYING(vict) && dmg > (((getEffMaxPL(vict))) * 0.01) && rand_number(1, 10) >= 8) {
   carry_drop(vict, 2);
  }

  if (char_get_position(vict) == POS_SITTING && char_is_npc(vict) && getCurHealth(vict) >= ((getEffMaxPL(vict))) * .98) {
    do_stand(vict, 0, 0, 0);
  }
 int suppresso = FALSE;
  if (is_sparring(ch) && is_sparring(vict) && (char_get_suppressed(vict) + getCurHealth(vict)) - dmg <= 0) {
    if (!char_is_npc(vict)) {
     act("@c$N@w falls down unconscious, and you stop sparring with $M.@n", TRUE, ch, 0, vict, TO_CHAR);
     act("@C$n@w stops sparring with you as you fall unconscious.@n", TRUE, ch, 0, vict, TO_VICT);
     act("@c$N@w falls down unconscious, and @C$n@w stops sparring with $M.@n", TRUE, ch, 0, vict, TO_NOTVICT);
     setCurHealth(vict, 1);
     if (char_get_suppressed(vict) > 0) {
      char_set_suppressed(vict, 0);
      char_set_suppression(vict, 0);
     }
     if (char_get_fighting(vict)) {
      stop_fighting(vict);
     }
     if (char_get_fighting(ch)) {
      stop_fighting(ch);
     }
     char_set_position(vict, POS_SLEEPING);
     if (!char_is_npc(ch)) {
      char_aff_set(vict, AFF_KNOCKED, true);
     }
    }
    else {
     act("@c$N@w admits defeat to you, stops sparring, and stumbles away.@n", TRUE, ch, 0, vict, TO_CHAR);
     act("@c$N@w admits defeat to $n, stops sparring, and stumbles away.@n", TRUE, ch, 0, vict, TO_NOTVICT);
     solo_gain(ch, vict);
     struct obj_data *rew, *next_rew;
     int founded = 0;
     for (rew = vict->carrying; rew; rew = next_rew) {
      next_rew = rew->next_content;
      if (rew) {
       obj_from_char(rew);
       obj_to_room(rew, char_get_in_room(vict));
       founded = 1;
      }
     }
     if (founded == 1) {
      act("@c$N@w leaves a reward behind out of respect.@n", TRUE, ch, 0, vict, TO_CHAR);
     }
        setCurHealth(vict, 0);
     extract_char(vict);
     return;
   }
  }
  else if (is_sparring(ch) && (char_get_suppressed(vict) + GET_HIT(vict)) - dmg <= 0) {
   act("@c$N@w falls down unconscious, and you spare $S life.@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@w spares your life as you fall unconscious.@n", TRUE, ch, 0, vict, TO_VICT);
   act("@c$N@w falls down unconscious, and @C$n@w spares $S life.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      setCurHealth(vict, 1);
   if (char_get_fighting(vict)) {
    stop_fighting(vict);
   }
   if (char_get_fighting(ch)) {
    stop_fighting(ch);
   }
   char_set_position(vict, POS_SLEEPING);
   if (!char_is_npc(ch)) {
    char_aff_set(vict, AFF_KNOCKED, true);
   }
  }
  else if (is_sparring(ch) && !is_sparring(vict) && char_is_npc(ch)) {
   act("@w$n@w stops sparring!@n", TRUE, ch, 0, vict, TO_ROOM);
   char_mob_set(ch, MOB_SPAR, false);
  }
  else if (!is_sparring(ch) && is_sparring(vict) && char_is_npc(vict)) {
   act("@w$n@w stops sparring!@n", TRUE, ch, 0, vict, TO_ROOM);
   char_mob_set(vict, MOB_SPAR, false);
  }
  if (char_get_suppressed(vict) > 0 && char_get_suppression(vict) > 0) {
    if (char_get_suppressed(vict) > dmg) {
     char_set_suppressed(vict, char_get_suppressed(vict) - (dmg));
     suppresso = TRUE;
    }
    else if (char_get_suppressed(vict) <= dmg) {
     send_to_char(vict, "@GYou no longer have any reserve powerlevel suppressed.@n\r\n");
     dmg -= char_get_suppressed(vict);
     char_set_suppressed(vict, 0);
     char_set_suppression(vict, 0);
    }
   }


  if (char_plr_flagged(vict, PLR_IMMORTAL) && !is_sparring(ch) && getCurHealth(vict) - dmg <= 0) {
   if (char_in_arena(vict)) {
       send_to_all("@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n", char_get_name(ch), char_get_name(vict));
       char_from_room(ch);
       char_to_room(ch, real_room(17875));
       look_at_room(char_get_in_room(ch), ch, 0);
       char_from_room(vict);
       char_to_room(vict, real_room(17875));
       setCurHealth(vict, 1);
       look_at_room(char_get_in_room(vict), vict, 0);
       if (char_get_fighting(vict)) {
        stop_fighting(vict);
       }
       if (char_get_fighting(ch)) {
        stop_fighting(ch);
       }
     return;
   }
   else {
   act("@c$N@w disappears right before dying. $N appears to be immortal.@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@CYou disappear right before death, having been saved by your immortality.@n", TRUE, ch, 0, vict, TO_VICT);
   act("@c$N@w disappears right before dying. $N appears to be immortal.@n.", TRUE, ch, 0, vict, TO_NOTVICT);
   decCurHealthPercent(vict, 1, 1);
   decCurSTPercent(vict, 1, 1);
   decCurKIPercent(vict, 1, 1);

   if (char_get_fighting(vict)) {
    stop_fighting(vict);
   }
   if (char_get_fighting(ch)) {
    stop_fighting(ch);
   }
   char_set_position(vict, POS_SITTING);
   char_from_room(vict);
   char_to_room(vict, real_room(sensei_start_room(vict->chclass)));
   }
   return;
  }

  if (char_get_grappling(vict) && char_get_grappling(vict) != ch && type == 1) {
   act("@YThe attack hurts YOU as well because you are grappling with $M!@n", TRUE, vict, 0, char_get_grappling(vict), TO_VICT);
   act("@YThe attack hurts @y$N@Y as well because $n is grappling with $m!@n", TRUE, vict, 0, char_get_grappling(vict), TO_NOTVICT);
   maindmg = maindmg / 2;
   hurt(0, 0, ch, char_get_grappling(vict), NULL, maindmg, 3);
  }
  if (char_get_grappled(vict) && char_get_grappled(vict) != ch && type == 1) {
   act("@YThe attack hurts YOU as well because you are being grappled by $M!@n", TRUE, vict, 0, char_get_grappled(vict), TO_VICT);
   act("@YThe attack hurts @y$N@Y as well because $n is being grappled by $m!@n", TRUE, vict, 0, char_get_grappled(vict), TO_NOTVICT);
   maindmg = maindmg / 2;
   hurt(0, 0, ch, char_get_grappled(vict), NULL, maindmg, 3);
  }
  if (!is_sparring(ch) && !char_plr_flagged(vict, PLR_IMMORTAL) && GET_HIT(vict) - dmg <= 0) {
  
    if (GET_HIT(vict) - dmg <= 0 && suppresso == FALSE) {
   decCurHealthPercent(vict, 1, 0);
    if (!char_is_npc(vict) && vict->lifeperc > 0 && (getCurLF(vict)) - (dmg - GET_HIT(vict)) >= 0) {
        act("@c$N@w barely clings to life!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@CYou barely cling to life!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N@w barely clings to life!@n.", TRUE, ch, 0, vict, TO_NOTVICT);
        int64_t lifeloss = dmg - GET_HIT(vict);
        decCurLF(vict, lifeloss, 0);
        send_to_char(vict, "@D[@CLifeforce@D: @R-%s@D]\n", add_commas(lifeloss));
      if ((getCurLF(vict)) >= (getMaxLF(vict)) * 0.05) {
        send_to_char(vict, "@YYou recover a bit thanks to your strong life force.@n\r\n");
        incCurHealth(vict, (getMaxLF(vict)) * .05, true);
        decCurLFPercent(vict, .05, 0);
       } else {
          incCurHealth(vict, char_get_level(vict) * 100, true);
       }
     return;
    }
   if (char_get_death_type(vict) != DTYPE_HEAD) {
    char_set_death_type(vict, 0);
   }
   if (type <= 0 && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
    handle_death_msg(ch, vict, 0);
   } else if (type > 0 && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
    handle_death_msg(ch, vict, 1);
   } else {
      act("@R$N@w self destructs with a mild explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@R$N@w self destructs with a mild explosion!@n", TRUE, ch, 0, vict, TO_ROOM);
   }
   if (dmg > 1) {
     if (type <= 0 && GET_HIT(ch) >= getEffMaxPL(ch) * 0.5) {
     int64_t raise = (GET_MAX_MANA(ch) * 0.005) + 1;
     incCurKI(ch, raise, true);
    }
    send_to_char(ch, "@D[@GDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
    send_to_char(vict, "@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
    int64_t healhp = (long double)(GET_MAX_HIT(vict)) * 0.12;
    if (char_aff_flagged(ch, AFF_METAMORPH) && GET_HIT(ch) <= GET_MAX_HIT(ch)) {
     act("@RYour dark aura saps some of @r$N's@R life energy!@n", TRUE, ch, 0, vict, TO_CHAR);
     act("@r$n@R's dark aura saps some of your life energy!@n", TRUE, ch, 0, vict, TO_VICT);
     incCurHealth(ch, healhp, true);
    }
    if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 10 || char_get_genome(ch, 1) == 10)) {
        incCurKI(ch, dmg * .05, true);
    }
    if (!is_sparring(ch) && char_is_npc(vict)) {
     if (type == 0 && rand_number(1, 100) >= 97) {
      send_to_char(ch, "@YY@yo@Yu @yg@Ya@yi@Yn@y s@Yo@ym@Ye @yb@Yo@yn@Yu@ys @Ye@yx@Yp@ye@Yr@yi@Ye@yn@Yc@ye@Y!@n\r\n");
      int64_t gain = char_get_exp(vict) * 0.05;
      gain += 1;
      gain_exp(ch, gain);
     } else if (type != 0 && rand_number(1, 100) >= 93) {
      int64_t gain = char_get_exp(vict) * 0.05;
      gain += 1;
      gain_exp(ch, gain);
     }
    }
    if (is_affected(vict, AFF_ECHAINS) && type == 0) {
      act("@CEthereal chains burn into existence! They quickly latch onto @RYOUR@C body and begin temporarily hampering $s actions!@n", TRUE, ch, 0, vict, TO_CHAR);
       act("@CEthereal chains burn into existence! They quickly latch onto @c$n's@C body and begin temporarily hampering $s actions!@n", TRUE, ch, 0, vict, TO_ROOM);
     if (char_is_npc(ch))
      assign_affect(vict, AFF_ECHAINS, 0, -1, 0, 0, 0, 0, 0, -2);
     else 
      WAIT_STATE(ch, PULSE_3SEC);
    }
   } else if (dmg <= 1) {
    send_to_char(ch, "@D[@GDamage@W: @BPitiful...@D]@n\r\n");
    send_to_char(vict, "@D[@rDamage@W: @BPitiful...@D]@n\r\n");
   }

   decCurHealthPercent(vict, 1, 0);

   if (char_aff_flagged(ch, AFF_GROUP)) {
    group_gain(ch, vict);
   } else {
    solo_gain(ch, vict);
   }
    if (char_is_demon(ch) && type == 1) {
     char_aff_set(vict, AFF_ASHED, true);
    }   
   die(vict, ch);
   dead = TRUE;
   }
  } else if (GET_HIT(vict) - dmg > 0 || suppresso == TRUE){
   if (suppresso == FALSE) {
   decCurHealth(vict, dmg, 0);
   }
   if (char_get_fighting(ch) == NULL) {
    set_fighting(ch, vict);
   } else if (char_get_fighting(ch) != vict) {
    set_fighting(ch, vict);
   }
   if (char_get_fighting(vict) == NULL) {
    set_fighting(vict, ch);
   } else if (char_get_fighting(vict) != ch) {
    set_fighting(vict, ch);
   }
   if (dmg > 1 && suppresso == FALSE) {
     if (type == 0 && GET_HIT(ch) >= getEffMaxPL(ch) * 0.5) {
     int64_t raise = (GET_MAX_MANA(ch) * 0.005) + 1;
     incCurKI(ch, raise, true);
    }
    if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 10 || char_get_genome(ch, 1) == 10)) {
        incCurKI(ch, dmg * .05, true);
    }
    send_to_char(ch, "@D[@GDamage@W: @R%s@D]@n", add_commas(dmg));
    send_to_char(vict, "@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
    //int64_t healhp = GET_HIT(vict) * 0.12;
    if (char_get_equipment(ch, WEAR_EYE) && vict && !char_prf_flagged(ch, PRF_NODEC)) {
     if (char_is_android(vict)) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && GET_HIT(vict) >= 150000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && GET_HIT(vict) >= 5000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && GET_HIT(vict) >= 15000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else {
      send_to_char(ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(GET_HIT(vict)));
     }
    } else {
     send_to_char(ch, "\r\n");
    }
   } else if (!char_is_npc(ch)) {
    if (dmg <= 1 && suppresso == FALSE && !char_prf_flagged(ch, PRF_NODEC)) {
    send_to_char(ch, "@D[@GDamage@W: @BPitiful...@D]@n");
    send_to_char(vict, "@D[@rDamage@W: @BPitiful...@D]@n\r\n");
    if (char_get_equipment(ch, WEAR_EYE) && vict && !char_prf_flagged(ch, PRF_NODEC)) {
     if (char_is_android(vict)) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && GET_HIT(vict) >= 150000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && GET_HIT(vict) >= 5000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && GET_HIT(vict) >= 15000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else {
      send_to_char(ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(GET_HIT(vict)));
     }
    } else {
     send_to_char(ch, "\r\n");
    }
   } else if (dmg > 1 && suppresso == TRUE && !char_prf_flagged(ch, PRF_NODEC)) {
    send_to_char(ch, "@D[@GDamage@W: @R%s@D]@n", add_commas(dmg));
    send_to_char(vict, "@D[@rDamage@W: @R%s @c-Suppression-@D]@n\r\n", add_commas(dmg));
    send_to_char(vict, "@D[Suppression@W: @G%s@D]@n\r\n", add_commas(char_get_suppressed(vict)));
    //int64_t healhp = GET_HIT(vict) * 0.12;
    if (char_get_equipment(ch, WEAR_EYE) && vict && !char_prf_flagged(ch, PRF_NODEC)) {
     if (char_is_android(vict)) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && GET_HIT(vict) >= 150000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && GET_HIT(vict) >= 5000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && GET_HIT(vict) >= 15000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else {
      send_to_char(ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(GET_HIT(vict)));
     }
    } else {
     send_to_char(ch, "\r\n");
    }
   } else if (dmg <= 1 && suppresso == TRUE && !char_prf_flagged(ch, PRF_NODEC)) {
    send_to_char(ch, "@D[@GDamage@W: @BPitiful...@D]@n");
    send_to_char(vict, "@D[@rDamage@W: @BPitiful... @c-Suppression-@D]@n\r\n");
    send_to_char(vict, "@D[Suppression@W: @G%s@D]@n\r\n", add_commas(char_get_suppressed(vict)));
    if (char_get_equipment(ch, WEAR_EYE) && vict) {
     if (char_is_android(vict) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && GET_HIT(vict) >= 150000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && GET_HIT(vict) >= 5000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && GET_HIT(vict) >= 15000000) {
      send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
     } else {
      send_to_char(ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(GET_HIT(vict)));
     }
    } else {
     send_to_char(ch, "\r\n");
    }
   }
   }
  }
   if (char_get_skill(ch, SKILL_FOCUS) && type == 1) {
    improve_skill(ch, SKILL_FOCUS, 1);
   }

   /* Increases GET_FURY for halfbreeds who get damaged. */

   if (!is_sparring(ch) && char_is_halfbreed(vict) && char_get_fury(vict) < 100 && !char_plr_flagged(vict, PLR_FURY)) {
    send_to_char(vict, "@RYour fury increases a little bit!@n\r\n");
    char_set_fury(vict, char_get_fury(vict) + (1));
   }

   /* Ends GET_FURY increase for halfbreeds who got damaged */

   if (is_sparring(ch) && is_sparring(vict) && char_get_lastattack(ch) != -1) {
    spar_gain(ch, vict, type, dmg);
   }
   if ((char_is_saiyan(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2))) && !char_is_npc(ch) && ((is_sparring(ch) && is_sparring(vict)) || (!is_sparring(ch) && !is_sparring(vict)))) {
    if (char_get_position(ch) != POS_RESTING && char_get_position(vict) != POS_RESTING && dmg > 1) {
     saiyan_gain(ch, vict);
    }
   }
   if (char_is_arlian(vict) && dead != TRUE && !is_sparring(vict) && !is_sparring(ch)) {
    handle_evolution(vict, dmg);
   }
   if (dead == TRUE) {
    char corp[256];
    if (!char_plr_flagged(ch, PLR_SELFD2)) {
     if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
      sprintf(corp, "all.zenni corpse");
      do_get(ch, corp, 0, 0);
     }
     if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
      sprintf(corp, "all corpse");
      do_get(ch, corp, 0, 0);
     }
    }
   }
 }
 /* If an object is targeted */
 else if (obj) {
   switch (obj_get_value(obj, VAL_ALL_MATERIAL)) {
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
   if (dmg <= 0) {
    dmg = 1;
   }
   if (obj_extra_flagged(obj, ITEM_UNBREAKABLE)) {
    act("$p@w seems unaffected.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("$p@w seems unaffected.@n", TRUE, ch, obj, 0, TO_ROOM);
   } else if (obj_get_vnum(obj) == 79) {
     if (obj_get_weight(obj) - dmg > 0) {
      if (type <= 0) {
       if (char_aff_flagged(ch, AFF_INFUSE))
        dmg *= 10;
       act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_CHAR);
       act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_ROOM);
       obj_set_weight(obj, obj_get_weight(obj) - (dmg));
       if (obj_get_fellow_wall(obj)) {
        struct obj_data *wall;
        wall = obj_get_fellow_wall(obj);
        obj_set_weight(wall, obj_get_weight(wall) - (dmg));
        act("$p@w cracks some. A humanoid shadow can be seen moving on the other side.@n", TRUE, 0, obj, 0, TO_ROOM);
       }
      } else {
       dmg *= 30;
       act("$p@w melts some.@n", TRUE, ch, obj, 0, TO_CHAR);
       act("$p@w melts some.@n", TRUE, ch, obj, 0, TO_ROOM);
       obj_set_weight(obj, obj_get_weight(obj) - (dmg));
       if (obj_get_fellow_wall(obj)) {
        struct obj_data *wall;
        wall = obj_get_fellow_wall(obj);
        obj_set_weight(wall, obj_get_weight(wall) - (dmg));
        act("$p@w melts some.@n", TRUE, ch, obj, 0, TO_ROOM);
       }
      }
     } else {
      if (type <= 0) {
       act("$p@w breaks completely apart and then melts away.@n", TRUE, ch, obj, 0, TO_CHAR);
       act("$p@w breaks completely apart and then melts away.@n", TRUE, ch, obj, 0, TO_ROOM);
       extract_obj(obj);
      } else {
       act("$p@w is blown away into snow and water!@n", TRUE, ch, obj, 0, TO_CHAR);
       act("$p@w is blown away into snow and water!@n", TRUE, ch, obj, 0, TO_ROOM);
       extract_obj(obj);
      }
     }
   } else if (obj_get_value(obj, VAL_ALL_HEALTH) - dmg > 0) {
    act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_ROOM);
    obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (dmg));
   } else {
    if (type <= 0) {
     act("$p@w shatters apart!@n", TRUE, ch, obj, 0, TO_CHAR);
     act("$p@w shatters apart!@n", TRUE, ch, obj, 0, TO_ROOM);
     obj_set_value(obj, VAL_ALL_HEALTH, 0);
     obj_extra_set(obj, ITEM_BROKEN, true);
     if (obj_get_type_flag(obj) == ITEM_DRINKCON && obj_get_type_flag(obj) == ITEM_FOUNTAIN) {
      obj_set_value(obj, VAL_DRINKCON_HOWFULL, 0);
     }
    }
    else if (type != 0) {
     act("$p@w is disintegrated!@n", TRUE, ch, obj, 0, TO_CHAR);
     act("$p@w is disintegrated!@n", TRUE, ch, obj, 0, TO_ROOM);
       extract_obj(obj);
    }
   }
 }
 else {
  log("Log: Error with hurt.\n");
 }
}

/* This handles the length of time between attacks and other actions *
* players AND non-players will have to endure. Allowing for a more   *
* balanced and interesting attack cooldown system. - Iovan 2/25/2011 */
void handle_cooldown(struct char_data *ch, int cooldown)
{

  /* Let's clear any cooldown they may accidently have so it doesn't stack *
   * This is only for NPCs as player cooldown is handled through the stock *
   * descriptor_list command interpreter. This also initializes cooldown   */

  if (!char_is_npc(ch)) {
   if (char_plr_flagged(ch, PLR_MULTIHIT)) {
    char_plr_set(ch, PLR_MULTIHIT, false);
    return;
   }
  }

  if (char_is_npc(ch)) {
   char_set_cooldown(ch, 0);
  }

  reveal_hiding(ch, 0);
  int  waitCalc = 10, base = cooldown;
  int64_t cspd = 0;

  /* Ok calculating speed. */
  cspd = char_get_speedi(ch);
  
  if (cspd > 10000000) { /* WTF Fast */
   waitCalc -= 9;
  } else if (cspd > 5000000) {
   waitCalc -= 8;
  } else if (cspd > 2500000) {
   waitCalc -= 7;
  } else if (cspd > 1000000) {
   waitCalc -= 6;
  } else if (cspd > 500000) {
   waitCalc -= 5;
  } else if (cspd > 100000) {
   waitCalc -= 4;
  } else if (cspd > 50000) {
   waitCalc -= 3;
  } else if (cspd > 25000) {
   waitCalc -= 2;
  } else if (cspd > 50) {
   waitCalc -= 1;
  }

  base *= 10;
 
  if (base >= 100) {
   base = 30;
  } else if (base >= 70) {
   base = 20;
  } else if (base >= 30) {
   base = 10;
  }

  /* Alright now let's determine the cooldown based on the wait and cooldown assigned *
   * by the attack which called handle_cooldown.                                      */
  if (!char_is_npc(ch)) {
   cooldown *= waitCalc;
   cooldown += base;
   if (cooldown <= 0) { /* Can't have this. */
    cooldown = 10;
   }
   if (cooldown >= 120) {
    WAIT_STATE(ch, PULSE_CD12);
   } else if (cooldown >= 110) {
    WAIT_STATE(ch, PULSE_CD11);
   } else if (cooldown >= 100) {
    WAIT_STATE(ch, PULSE_CD10);
   } else if (cooldown >= 90) {
    WAIT_STATE(ch, PULSE_CD9);
   } else if (cooldown >= 80) {
    WAIT_STATE(ch, PULSE_CD8);
   } else if (cooldown >= 70) {
    WAIT_STATE(ch, PULSE_CD7);
   } else if (cooldown >= 60) {
    WAIT_STATE(ch, PULSE_CD6);
   } else if (cooldown >= 50) {
    WAIT_STATE(ch, PULSE_CD5);
   } else if (cooldown >= 40) {
    WAIT_STATE(ch, PULSE_CD4);
   } else if (cooldown >= 30) {
    WAIT_STATE(ch, PULSE_CD3);
   } else if (cooldown >= 20) {
    WAIT_STATE(ch, PULSE_CD2);
   } else {
    WAIT_STATE(ch, PULSE_CD1);
   }
    
  } else { /* We handle NPCs differently. */
   cooldown *= waitCalc;
   cooldown += base;
   if (cooldown >= 120) {
    char_set_cooldown(ch, 12);
   } else if (cooldown >= 110) {
    char_set_cooldown(ch, 11);
   } else if (cooldown >= 100) {
    char_set_cooldown(ch, 10);
   } else if (cooldown >= 90) {
    char_set_cooldown(ch, 9);
   } else if (cooldown >= 80) {
    char_set_cooldown(ch, 8);
   } else if (cooldown >= 70) {
    char_set_cooldown(ch, 7);
   } else if (cooldown >= 60) {
    char_set_cooldown(ch, 6);
   } else if (cooldown >= 50) {
    char_set_cooldown(ch, 5);
   } else if (cooldown >= 40) {
    char_set_cooldown(ch, 4);
   } else if (cooldown >= 30) {
    char_set_cooldown(ch, 3);
   } else if (cooldown >= 20) {
    char_set_cooldown(ch, 2);
   } else {
    char_set_cooldown(ch, 1);
   }
  }
}

/* This handles whether parry is turned on */
int handle_parry(struct char_data *ch)
{

 if (axion_dice(0) <= 4) { /* Critical failure */
  return (1);
 }

 if (char_is_npc(ch)) { 

  /*  Non-humanoids are only rarely capable of parrying against weak players. Never
   * against strong players. Humanoids get progressively better at parry the higher
   * level they are.
   **/
  if (!char_is_humanoid(ch)) {
   return (rand_number(0, 5));
  } else {
   if (char_get_level(ch) >= 110) {
    return (rand_number(90, 105));
   } else if (char_get_level(ch) >= 100) {
    return (rand_number(85, 95));
   } else if (char_get_level(ch) >= 80) {
    return (rand_number(70, 85));
   } else if (char_get_level(ch) >= 40) {
    return (rand_number(50, 70));
   } else {
    int top = char_get_level(ch);

     if (top < 15)
      top = 16;
    return (rand_number(15, top));
   }
  } 
 }

 if (char_prf_flagged(ch, PRF_NOPARRY)) {
  return (-2);
 } else {
  int num = char_get_skill(ch, SKILL_PARRY);
  if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 3 || char_get_genome(ch, 1) == 3)) {
   num += 10;
  }
  if (char_get_skills_base(ch, SKILL_STYLE) >= 100) {
   num += 5;
  } else if (char_get_skills_base(ch, SKILL_STYLE) >= 80) {
   num += 4;
  } else if (char_get_skills_base(ch, SKILL_STYLE) >= 60) {
   num += 3;
  } else if (char_get_skills_base(ch, SKILL_STYLE) >= 40) {
   num += 2;
  } else if (char_get_skills_base(ch, SKILL_STYLE) >= 20) {
   num += 1;
  }
  return (num);
 }

}

/* This handles whether a step of the combo was preformed. */
int handle_combo(struct char_data *ch, struct char_data *vict)
{
 int success = FALSE, pass = FALSE;

 if (char_is_npc(ch))
  return 0;

 switch (char_get_lastattack(ch)) {
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
   pass = TRUE;
  break;
  default:
    if (char_get_combo(ch) != -1) {
     send_to_char(ch, "@RYou have cut your combo short with the wrong attack!@n\r\n");
    }
    char_set_combo(ch, -1);
    char_set_combhits(ch, 0);
    pass = FALSE;
  break;
 }

 if (pass == FALSE)
  return 0;

 if (count_physical(ch) < 3)
  return 0;

    int chance;
    int64_t chspeed = char_get_speedi(ch);
    int64_t victspeed = char_get_speedi(vict);
    int64_t speedPercentage = ((double)(chspeed) / (double)(victspeed)) * 100.0;
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
    if (char_get_lastattack(ch) == 0 || char_get_lastattack(ch) == 1) {
        chance -= 10;
    }
    if (char_get_lastattack(ch) == 2 || char_get_lastattack(ch) == 3) {
        chance -= 5;
    }

    if (char_get_combo(ch) <= -1 && rand_number(1, 100) > chance) {
 while (success == FALSE) {
  switch(rand_number(1, 24)) {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
    if (char_get_skill(ch, SKILL_PUNCH) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try a@R punch @Gnext!@n\r\n");
     char_set_combo(ch, 0);
     success = TRUE;
    }
    break;
   case 6:
   case 7:
   case 8:
   case 9:
   case 10:
    if (char_get_skill(ch, SKILL_KICK) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try a@R kick @Gnext!@n\r\n");
     char_set_combo(ch, 1);
     success = TRUE;
    }
    break;
   case 11:
   case 12:
   case 13:
   case 14:
    if (char_get_skill(ch, SKILL_ELBOW) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try an@R elbow @Gnext!@n\r\n");
     char_set_combo(ch, 2);
     success = TRUE;
    }
    break;
   case 15:
   case 16:
   case 17:
    if (char_get_skill(ch, SKILL_KNEE) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try a@R knee @Gnext!@n\r\n");
     char_set_combo(ch, 3);
     success = TRUE;
    }
    break;
   case 18:
   case 19:
    if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try a@R roundhouse @Gnext!@n\r\n");
     char_set_combo(ch, 4);
     success = TRUE;
    }
    break;
   case 20:
   case 21:
    if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try an@R uppercut @Gnext!@n\r\n");
     char_set_combo(ch, 5);
     success = TRUE;
    }
    break;
   case 22:
    if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try a@R heeldrop @Gnext!@n\r\n");
     char_set_combo(ch, 8);
     success = TRUE;
    }
    break;
   case 24:
    if (char_get_skill(ch, SKILL_SLAM) > 0) {
     send_to_char(ch, "@GYou have a chance for a COMBO! Try a@R slam @Gnext!@n\r\n");
     char_set_combo(ch, 6);
     success = TRUE;
    }
    break;
  } 
  }
  return 0;
 } else if (char_get_lastattack(ch) == char_get_combo(ch) && char_get_combhits(ch) < physical_mastery(ch)) {
  char_set_combhits(ch, char_get_combhits(ch) + (1));
  while (success == FALSE) {
  if (char_get_combhits(ch) >= 20) { /* We're kicking ass! */
   switch (rand_number(1, 34)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
     if (char_get_skill(ch, SKILL_ELBOW) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 2);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_KNEE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 3);
      success = TRUE;
     }     
     break;
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
     if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 5);
      success = TRUE;
     }
     break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
     if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 4);
      success = TRUE;
     }     
     break;
    case 27:
    case 28:
    case 29:
     if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 51);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 56);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
     send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
     char_set_combo(ch, 52);
     success = TRUE;
     } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 6);
      success = TRUE;
     }
     break;
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
     if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 51);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 56);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
     send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
     char_set_combo(ch, 52);
     success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 8);
      success = TRUE;
     }
     break;
   } /* Switch End */
  } else if (char_get_combhits(ch) >= 15) { /* We're doing good! */
   switch (rand_number(1, 36)) {
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
     if (char_get_skill(ch, SKILL_ELBOW) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 2);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_KNEE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 3);
      success = TRUE;
     }     
     break;
    case 21:
    case 22:
    case 23:
     if (char_get_skill(ch, SKILL_PUNCH) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 0);
      success = TRUE;
     }
     break;
    case 25:
    case 26:
    case 27:
     if (char_get_skill(ch, SKILL_KICK) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 1);
      success = TRUE;
     }
     break;
    case 29:
    case 30:
     if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 5);
      success = TRUE;
     }
     break;
    case 31:
    case 32:
    case 33:
    case 34:
     if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 4);
      success = TRUE;
     }
     break;
    case 35:
     if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 51);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 56);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
     send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
     char_set_combo(ch, 52);
     success = TRUE;
     } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 6);
      success = TRUE;
     }
     break;
    case 36:
     if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 51);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 56);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
     send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
     char_set_combo(ch, 52);
     success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 8);
      success = TRUE;
     }
     break;
   }
  } else if (char_get_combhits(ch) >= 10) { /* We're on a roll */
   switch (rand_number(1, 34)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
     if (char_get_skill(ch, SKILL_ELBOW) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 2);
      success = TRUE;
     }
     break;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
     if (char_get_skill(ch, SKILL_KNEE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 3);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_PUNCH) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 0);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_KICK) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 1);
      success = TRUE;
     }
     break;
    case 27:
    case 28:
    case 29:
     if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 5);
      success = TRUE;
     }
     break;
    case 30:
    case 31:
     if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 4);
      success = TRUE;
     }
     break;
    case 32:
    case 33:
     if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 51);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 56);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
     send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
     char_set_combo(ch, 52);
     success = TRUE;
     } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 6);
      success = TRUE;
     }
     break;
    case 34:
     if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 51);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 56);
      success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
     send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
     char_set_combo(ch, 52);
     success = TRUE;
     } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 8);
      success = TRUE;
     }
     break;
   }
  } else if (char_get_combhits(ch) >= 5) { /* We're staring off well */
   switch (rand_number(1, 30)) {
    case 1:
    case 2:
    case 3:
    case 4:
     if (char_get_skill(ch, SKILL_ELBOW) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 2);
      success = TRUE;
     }
     break;
    case 5:
    case 6:
    case 7:
    case 8:
     if (char_get_skill(ch, SKILL_KNEE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 3);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_PUNCH) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 0);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_KICK) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 1);
      success = TRUE;
     }
     break;
    case 29:
     if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 5);
      success = TRUE;
     }
     break;
    case 30:
     if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 4);
      success = TRUE;
     }
     break;
   }
  } else { /* We just the combo not long ago */
   switch (rand_number(1, 30)) {
    case 1:
    case 2:
    case 3:
     if (char_get_skill(ch, SKILL_ELBOW) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 2);
      success = TRUE;
     }
     break;
    case 4:
    case 5:
    case 6:
     if (char_get_skill(ch, SKILL_KNEE) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 3);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_PUNCH) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 0);
      success = TRUE;
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
     if (char_get_skill(ch, SKILL_KICK) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
      char_set_combo(ch, 1);
      success = TRUE;
     }
     break;
    }
   } /* End continued hits section */
  }
  return char_get_combhits(ch);
 } else if (char_get_lastattack(ch) == char_get_combo(ch) && char_get_combhits(ch) >= physical_mastery(ch)) {
   char_set_combhits(ch, char_get_combhits(ch) + (1));
   send_to_char(ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Combo FINISHED for massive damage@G!@n\r\n", char_get_combhits(ch));
 } else if (char_get_combo(ch) != char_get_lastattack(ch) && char_get_combo(ch) > -1) {
   send_to_char(ch, "@GCombo failed! Try harder next time!@n\r\n");
   char_set_combo(ch, -1);
   char_set_combhits(ch, 0);
   return 0;
 } else {
  char_set_combo(ch, -1);
  char_set_combhits(ch, 0);
  return 0;
 }
 return 0;
}

void handle_spiral(struct char_data *ch, struct char_data *vict, int skill, int first)
{
 int prob, perc, avo, index, pry = 2, dge = 2, blk = 2;
 int64_t dmg;
 double amount = 0.0;

 if (first == FALSE) {
  amount = 0.05;
 } else {
  amount = 0.5;
 }

 if (vict == NULL && char_get_fighting(ch)) {
  vict = char_get_fighting(ch);
 } else if (vict == NULL) {
  act("@WHaving lost your target you slow down until your vortex disappears, and end your attack.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W slows down until $s vortex disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_SPIRAL, false);
  return;
 }

 if (char_get_charge(ch) <= 0) {
  act("@WHaving no more charged ki you slow down until your vortex disappears, and end your attack.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W slows down until $s vortex disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_SPIRAL, false);
  return;
 }

 if (vict) {
  index = check_def(vict); /* Check parry/block/dodge of vict */

  prob = skill;
  perc = axion_dice(0);

  index -= handle_speed(ch, vict);

  avo = index / 4;

  handle_defense(vict, &pry, &blk, &dge);

  if (avo > 0 && avo < 70) {
   prob -= avo;
  }
  else if (avo >= 70) {
   prob -= 69;
  }
  tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

  if(!tech_handle_zanzoken(ch, vict, "Spiral Comet Blast")) {
      pcost(ch, amount, 0);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
  }

  if (prob < perc) {
   if ((getCurST(vict)) > 0) {
    if (blk > rand_number(1, 130)) {
     act("@C$N@W moves quickly and blocks your Spiral Comet blast!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@WYou move quickly and block @C$n's@W Spiral Comet blast!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@W moves quickly and blocks @c$n's@W Spiral Comet blast!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     pcost(ch, amount, 0);
     dmg = damtype(ch, 10, skill, .05);
     dmg /= 4;
     hurt(0, 0, ch, vict, NULL, dmg, 1);
     return;
    }
    else if (dge > rand_number(1, 130)) {
     act("@C$N@W manages to dodge your Spiral Comet blast, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@WYou dodge @C$n's@W Spiral Comet blast, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@W manages to dodge @c$n's@W Spiral Comet blast, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

         dodge_ki(ch, vict, 0, 45, skill, SKILL_SPIRAL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

     if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
       rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
     }

     pcost(ch, amount, 0);
     hurt(0, 0, ch, vict, NULL, 0, 1);
     return;
    }
    else {
     act("@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@C$n@W fires a Spiral Comet blast at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
     act("@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
     pcost(ch, amount, 0);
     hurt(0, 0, ch, vict, NULL, 0, 1);
     return;
    }
   }
   else {
     act("@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@C$n@W fires a Spiral Comet blast at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     pcost(ch, amount, 0);
   }
     hurt(0, 0, ch, vict, NULL, 0, 1);
   return;
  }
  else {
   if (first == TRUE) {
    dmg = damtype(ch, 44, skill, .5);
   } else {
    dmg = damtype(ch, 45, skill, .01);
   }
   switch(rand_number(1, 5)) {
    case 1:
      act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S chest and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR chest and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S chest and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      hurt(0, 0, ch, vict, NULL, dmg, 1);
      dam_eq_loc(vict, 4);
     break;
    case 2: /* Critical */
      act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S head and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR head and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S head and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      dmg *= 2;
      hurt(0, 0, ch, vict, NULL, dmg, 1);
      dam_eq_loc(vict, 3);
     break;
    case 3:
      act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S body and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR body and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S body and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      hurt(0, 0, ch, vict, NULL, dmg, 1);
      dam_eq_loc(vict, 4);
     break;
    case 4: /* Weak */
      act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S arm and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR arm and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S arm and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      dmg /= 2;
      dam_eq_loc(vict, 1);
      hurt(0, 190, ch, vict, NULL, dmg, 1);
     break;
    case 5: /* Weak 2 */
      act("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S leg and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR leg and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S leg and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      dmg /= 2;
      dam_eq_loc(vict, 2);
      hurt(1, 190, ch, vict, NULL, dmg, 1);
     break;
   }
     pcost(ch, amount, 0);
    return;
  }
 }
 else {
  return;
 }
}

void handle_death_msg(struct char_data *ch, struct char_data *vict, int type)
{
  if (type == 0) {
   if (!rroom_sunken(char_get_in_room(vict)) && rroom_sect(char_get_in_room(vict)) != SECT_WATER_SWIM && rroom_sect(char_get_in_room(vict)) != SECT_WATER_NOSWIM && !rroom_flagged(char_get_in_room(vict), ROOM_SPACE) && rroom_sect(char_get_in_room(vict)) != SECT_FLYING) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r coughs up blood before falling to the ground dead.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cough up blood before falling to the ground dead.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r coughs up blood before falling down dead.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 2:
      act("@R$N@r crumples to the ground dead.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou crumple to the ground dead.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r crumples to the ground dead.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 3:
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 4:
      act("@R$N@r writhes on the ground screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou writhe on the ground screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r writhes on the ground screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
     case 5:
      act("@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou hit the ground dead with such force that blood flies into the air briefly!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
    }
   } else if (rroom_sect(char_get_in_room(vict)) == SECT_WATER_SWIM || rroom_sect(char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r coughs up blood and dies before falling down to the water. A large splash accompanies $S body hitting the water!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cough up blood and die before falling down to the water. A large splash accompanies your body hitting the water!@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r coughs up blood and dies before falling down to the water. A large splash accompanies $S body hitting the water!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 2:
      act("@R$N@r crumples down to the water, with the signs of life leaving $S eyes as $E floats in the water.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou crumple down to the water and die.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r crumples down to the water, with the signs of life leaving $S eyes as $E floats in the water.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 3:
      act("@R$N@r cries out $S last breath before dying and leaving a floating corpse behind.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r cries out $S last breath before dying and leaving a floating corpse behind.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 4:
      act("@R$N@r writhes in the water screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou writhe in the water screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r writhes in the water screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
     case 5:
      act("@R$N@r hits the water dead with such force that blood mixed with water flies into the air briefly!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou hit the water dead with such force that blood mixed with water flies into the air briefly!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r hits the water dead with such force that blood mixed with water flies into the air briefly!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
    }
   } else if (rroom_flagged(char_get_in_room(vict), ROOM_SPACE)) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r coughs up blood and dies. The blood freezes and floats freely through space...@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cough up blood and die. The blood freezes and floats freely through space...@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r coughs up blood and dies. The blood freezes and floats freely through space...@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 2:
      act("@R$N@r dies and leaves $S corpse floating freely in space.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou die and leave your corpse floating freely in space.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r dies and leaves $S corpse floating freely in space.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 3:
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 4:
      act("@R$N@r writhes in space trying to scream in pain before $e finally dies!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou writhe in space trying to scream in pain before you finally die!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r writhes in space trying to scream in pain before $e finally dies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
     case 5:
      act("@R$N@r dies suddenly leaving behind a badly damaged corpse floating in space!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou die suddenly leaving behind a badly damaged corpse floating in space!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r dies suddenly leaving behind a badly damaged corpse floating in space!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
    }
   } else if (rroom_sect(char_get_in_room(vict)) == SECT_FLYING) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r coughs up blood before $s corpse starts to fall to the ground far below.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou coughs up blood before your corpse starts to fall to the ground far below.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r coughs up blood before $s corpse starts to fall to the ground far below.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 2:
      act("@R$N@r dies and $S corpse begins to fall to the ground below.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou die and your corpse begins to fall to the ground below.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r dies and $S corpse begins to fall to the ground below.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 3:
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 4:
      act("@R$N@r writhes in midair screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou writhe in midair screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r writhes in midair screaming in pain before finally dying!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
     case 5:
      act("@R$N@r snaps back and dies with such force that blood flies into the air briefly!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou snap back and die with such force that blood flies into the air briefly!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
    }
   } else {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r coughs up blood before $s corpse starts to float limply in the water.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou coughs up blood before your corpse starts to float limply in the water.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r coughs up blood before $s corpse starts to float limply in the water.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 2:
      act("@R$N@r dies and $S corpse begins to float limply in the water.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou die and your corpse begins to float limply in the water.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r dies and $S corpse begins to float limply in the water.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 3:
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 4:
      act("@R$N@r writhes and thrases in the water trying to scream before finally dying!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou writhe and thrash in the water trying to scream before finally dying!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r writhes and thrashes in the water trying to scream before finally dying!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
     case 5:
      act("@R$N@r snaps back and dies with such force that blood floods out of $S body into the water!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou snap back and die with such force that blood floods out of your body into the water!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r hits the ground dead with such force that blood floods out of $S body into the water!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      if (char_get_death_type(vict) != DTYPE_HEAD) {
       char_set_death_type(vict, DTYPE_PULP);
      }
      break;
    }
   }
 } else {
   if (!rroom_sunken(char_get_in_room(vict)) && rroom_sect(char_get_in_room(vict)) != SECT_WATER_SWIM && rroom_sect(char_get_in_room(vict)) != SECT_WATER_NOSWIM && !rroom_flagged(char_get_in_room(vict), ROOM_SPACE) && rroom_sect(char_get_in_room(vict)) != SECT_FLYING) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 2:
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rThe bottom half of your body is all that remains as you die.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_HALF);
      break;
     case 3:
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body completely disintegrates in the attack!@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 4:
      act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body falls down as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 5:
      act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rWhat's left of your body slams into the ground as you die!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
    }
   } else if (rroom_sect(char_get_in_room(vict)) == SECT_WATER_SWIM || rroom_sect(char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 2:
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rThe bottom half of your body is all that remains as you die.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_HALF);
      break;
     case 3:
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body completely disintegrates in the attack!@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 4:
      act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body falls down as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 5:
      act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rWhat's left of your body slams into the ground as you die!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
    }
   } else if (rroom_flagged(char_get_in_room(vict), ROOM_SPACE)) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r explodes and chunks of $M shower out into every direction of space.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r explodes and chunks of $M shower out into every direction of space.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 2:
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rThe bottom half of your body is all that remains as you die.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_HALF);
      break;
     case 3:
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body completely disintegrates in the attack!@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 4:
      act("@R$N@r floats away as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body floats away as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r floats away as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 5:
      act("@rWhat's left of @R$N@r's body floats away through space!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rWhat's left of your body floats away through space!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@rWhat's left of @R$N@r's body floats away through space!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
    }
   } else if (rroom_sect(char_get_in_room(vict)) == SECT_FLYING) {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r explodes and chunks of $M shower towards the ground far below.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r explodes and chunks of $M shower toward the ground far below.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 2:
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rThe bottom half of your body is all that remains as you die.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_HALF);
      break;
     case 3:
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body completely disintegrates in the attack!@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 4:
      act("@R$N@r falls down toward the ground as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body falls down toward the ground as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r falls down toward the ground as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 5:
      act("@rWhat's left of @R$N@r's body falls toward the ground as $E dies!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rWhat's left of yor body falls toward the ground as you die!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@rWhat's left of @R$N@r's body falls toward the ground as $E dies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
    }
   } else {
    switch (rand_number(1, 5)) {
     case 1:
      act("@R$N@r explodes and chunks of $M float freely through the water.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r explodes and chunks of $M float freely through the water.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 2:
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rThe bottom half of your body is all that remains as you die.@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_HALF);
      break;
     case 3:
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body completely disintegrates in the attack!@n", TRUE, ch, 0, vict, TO_VICT); 
      act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_death_type(vict, DTYPE_VAPOR);
      break;
     case 4:
      act("@R$N@r falls back as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rYour body falls back as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@R$N@r falls back as a smoldering corpse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
     case 5:
      act("@rWhat's left of @R$N@r's body floats limply as $E dies!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@rWhat's left of yor body floats limply as you die!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@rWhat's left of @R$N@r's body floats limply as $E dies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      break;
    }
   }
 }
}
