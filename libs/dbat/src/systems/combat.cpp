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
#include "dbat/iterate.hpp"
#include "dbat/consts/deathtype.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.item.hpp"
#include "dbat/mobact.hpp"
#include "dbat/fight.hpp"
#include "dbat/act.attack.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/genzon.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/class.hpp"
#include "dbat/techniques.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/extract.hpp"
#include "dbat/random.hpp"
#include "dbat/relocate.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/affect.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/game_world.hpp"

static void damage_eq(GameWorld *g, struct char_data *vict, int location);
static int handle_combo(GameWorld *g, struct char_data *ch, struct char_data *vict);
static void handle_death_msg(GameWorld *g, struct char_data *ch, struct char_data *vict, int type);

/* local functions */
void damage_weapon(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct char_data *vict)
{

  if (obj) {
    if (obj_extra_flagged(obj, ITEM_UNBREAKABLE))
      return;
  }
  int ranking = 0, material = 1;
  int64_t PL10 = 2000000000, PL9 = 2000000000, PL8 = 2000000000;

  PL10 = PL10 * 5;
  PL9  = PL9 * 4;
  PL8  = PL8 * 2;

  if (getCurPL(g, vict) >= PL10) {
    ranking = 10;
  } else if (getCurPL(g, vict) >= PL9) {
    ranking = 9;
  } else if (getCurPL(g, vict) >= PL8) {
    ranking = 8;
  } else if (getCurPL(g, vict) >= 2000000000) {
    ranking = 7;
  } else if (getCurPL(g, vict) >= 500000000) {
    ranking = 6;
  } else if (getCurPL(g, vict) >= 250000000) {
    ranking = 5;
  } else if (getCurPL(g, vict) >= 100000000) {
    ranking = 4;
  } else if (getCurPL(g, vict) >= 50000000) {
    ranking = 3;
  } else if (getCurPL(g, vict) >= 25000000) {
    ranking = 2;
  } else if (getCurPL(g, vict) >= 1000000) {
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
  } else if (char_aff_flagged(ch, AFF_BLESS) && rand_number(g, 1, 3) == 3) {
    if (result > 1) {
      result = 1;
    }
  } else if (char_aff_flagged(ch, AFF_BLESS)) {
    result = 0;
  }

  if (char_get_skill(ch, SKILL_HANDLING) >= axion_dice(g, 10)) {
    act(g, "@GYour superior handling prevents @C$p@G from being damaged.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@g$n's@G superior handling prevents @C$p@G from being damaged.@n", true, ch, obj, nullptr, TO_ROOM);
    result = 0;
  }

  if (result > 0) {
    obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (result));
    if (obj_get_value(obj, VAL_ALL_HEALTH) <= 0) {
      act(g, "@RYour @C$p@R shatters on @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@r$n's@R @C$p@R shatters on YOUR body!@n", true, ch, obj, vict, TO_VICT);
      act(g, "@r$n's@R @C$p@R shatters on @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
      obj_extra_set(obj, ITEM_BROKEN, true);
      perform_remove(g, vict, 16);
      perform_remove(g, vict, 17);
    } else if (result >= 8) {
      act(g, "@RYour @C$p@R cracks loudly from striking @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@r$n's@R @C$p@R cracks loudly from striking YOUR body!@n", true, ch, obj, vict, TO_VICT);
      act(g, "@r$n's@R @C$p@R cracks loudly from striking @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
    } else if (result >= 6) {
      act(g, "@RYour @C$p@R chips from striking @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@r$n's@R @C$p@R cracks from striking YOUR body!@n", true, ch, obj, vict, TO_VICT);
      act(g, "@r$n's@R @C$p@R cracks from striking @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
    } else if (result >= 3) {
      act(g, "@RYour @C$p@R loses a piece from striking @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@r$n's@R @C$p@R loses a piece from striking YOUR body!@n", true, ch, obj, vict, TO_VICT);
      act(g, "@r$n's@R @C$p@R loses a piece from striking @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
    } else if (result >= 1) {
      act(g, "@RYour @C$p@R has a nick in it from hitting @r$N's@R body!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@r$n's@R @C$p@R has a nick in it from hitting YOUR body!@n", true, ch, obj, vict, TO_VICT);
      act(g, "@r$n's@R @C$p@R has a nick in it from hitting @r$N's@R body!@n", true, ch, obj, vict, TO_NOTVICT);
    }
  }
}

void handle_multihit(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  int perc = char_get_aff_abils_dex(ch), prob = char_get_aff_abils_dex(vict);

  /* Let's give the victim a bit of automatic favor due to "luck" */
  prob += rand_number(g, 1, 15);
  /* Some otherwise inclined "luck" for the attacker */
  perc += rand_number(g, -5, 5);

  if (char_get_throws(ch) >= 3) {
    char_set_throws(ch, 0);
    return;
  }
  if (char_get_throws(ch) == -1) {
    char_set_throws(ch, 0);
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
  if (axion_dice(g, 0) < amt) {
    prob += 500;
  }

  /* Success! */
  if (perc >= prob) {
    char buf[MAX_INPUT_LENGTH];
    act(g, "@Y...in a lightning flash of speed you attack @y$N@Y again!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@Y...in a lightning flash of speed @y$n@Y attacks YOU again!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@Y...in a lightning flash of speed @y$n@Y attacks @y$N@Y again!@n", true, ch, nullptr, vict, TO_NOTVICT);
    char_set_throws(ch, char_get_throws(ch) + 1);
    char_plr_set(ch, PLR_MULTIHIT, true);
    if (char_get_combo(ch) > -1) {
      switch (char_get_combo(ch)) {
        case 0:
          sprintf(buf, "%s", char_get_name(vict));
          do_punch(g, ch, buf, 0, 0);
          break;
        case 1:
          sprintf(buf, "%s", char_get_name(vict));
          do_kick(g, ch, buf, 0, 0);
          break;
        case 2:
          sprintf(buf, "%s", char_get_name(vict));
          do_elbow(g, ch, buf, 0, 0);
          break;
        case 3:
          sprintf(buf, "%s", char_get_name(vict));
          do_knee(g, ch, buf, 0, 0);
          break;
        case 4:
          sprintf(buf, "%s", char_get_name(vict));
          do_roundhouse(g, ch, buf, 0, 0);
          break;
        case 5:
          sprintf(buf, "%s", char_get_name(vict));
          do_uppercut(g, ch, buf, 0, 0);
          break;
        case 6:
          sprintf(buf, "%s", char_get_name(vict));
          do_slam(g, ch, buf, 0, 0);
          char_set_throws(ch, char_get_throws(ch) + 1);
          break;
        case 8:
          sprintf(buf, "%s", char_get_name(vict));
          do_heeldrop(g, ch, buf, 0, 0);
          char_set_throws(ch, char_get_throws(ch) + 1);
          break;
        case 51:
          sprintf(buf, "%s", char_get_name(vict));
          do_bash(g, ch, buf, 0, 0);
          char_set_throws(ch, char_get_throws(ch) + 1);
          break;
        case 52:
          sprintf(buf, "%s", char_get_name(vict));
          do_head(g, ch, buf, 0, 0);
          break;
        case 56:
          sprintf(buf, "%s", char_get_name(vict));
          do_tailwhip(g, ch, buf, 0, 0);
          char_set_throws(ch, char_get_throws(ch) + 1);
          break;
      }
    } else {
      if (char_get_lastattack(ch) == -1) {
        sprintf(buf, "%s", char_get_name(vict));
        do_attack(g, ch, buf, 0, 0);
      } else {
        /* Repeat the last physical attack command that was issued
           instead of always falling back to punch/kick. */
        sprintf(buf, "%s", char_get_name(vict));
        switch (char_get_lastattack(ch)) {
          case 0:
            do_punch(g, ch, buf, 0, 0);
            break;
          case 1:
            do_kick(g, ch, buf, 0, 0);
            break;
          case 2:
            do_elbow(g, ch, buf, 0, 0);
            break;
          case 3:
            do_knee(g, ch, buf, 0, 0);
            break;
          case 4:
            do_roundhouse(g, ch, buf, 0, 0);
            break;
          case 5:
            do_uppercut(g, ch, buf, 0, 0);
            break;
          case 6:
            do_slam(g, ch, buf, 0, 0);
            break;
          case 8:
            do_heeldrop(g, ch, buf, 0, 0);
            break;
          case 51:
            do_bash(g, ch, buf, 0, 0);
            break;
          case 52:
            do_head(g, ch, buf, 0, 0);
            break;
          case 56:
            do_tailwhip(g, ch, buf, 0, 0);
            break;
          default: /* unknown / weapon attack: fall back to punch */
            do_punch(g, ch, buf, 0, 0);
            break;
        }
      }
    }
  }
}

bool handle_defender(GameWorld *g, struct char_data *vict, struct char_data *ch)
{

  int result = false;

  if (char_get_defender(g, vict)) {
    struct char_data *def = char_get_defender(g, vict);
    int64_t defnum        = (char_get_speedi(g, def) * 0.01) * rand_number(g, -10, 10);
    int64_t chnum         = (char_get_speedi(g, ch) * 0.01) * rand_number(g, -5, 10);
    if (char_get_speedi(g, def) + defnum > char_get_speedi(g, ch) + chnum && char_get_in_room(def) == char_get_in_room(vict) && char_get_position(def) > POS_SITTING) {
      act(g, "@YYou move to and manage to intercept the attack aimed at @y$N@Y!@n", true, def, nullptr, vict, TO_CHAR);
      act(g, "@y$n@Y moves to and manages to intercept the attack aimed at YOU!@n", true, def, nullptr, vict, TO_VICT);
      act(g, "@y$n@Y moves to and manages to intercept the attack aimed at @y$N@Y!@n", true, def, nullptr, vict, TO_NOTVICT);
      result = true;
    } else if (char_get_in_room(def) == char_get_in_room(vict) && char_get_position(def) > POS_SITTING) {
      act(g, "@YYou move to intercept the attack aimed at @y$N@Y, but just not fast enough!@n", true, def, nullptr, vict, TO_CHAR);
      act(g, "@y$n@Y moves to intercept the attack aimed at YOU, but $e wasn't fast enough!@n", true, def, nullptr, vict, TO_VICT);
      act(g, "@y$n@Y moves to intercept the attack aimed at @y$N@Y, but $e wasn't fast enough!@n", true, def, nullptr, vict, TO_NOTVICT);
    }
  }

  return (result);
}

void handle_disarm(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  int roll1 = rand_number(g, -10, 10), roll2 = rand_number(g, -10, 10), handled = false;
  roll1 += char_get_aff_abils_str(ch) + char_get_aff_abils_dex(ch);
  roll2 += char_get_aff_abils_str(vict) + char_get_aff_abils_dex(vict);

  if (!char_is_npc(ch)) {
    if (char_plr_flagged(ch, PLR_THANDW)) {
      roll1 += 5;
    }
  }

  if (rand_number(g, 1, 100) <= 50 && !char_is_konatsu(ch)) {
    roll1 = -500;
  } else if (rand_number(g, 1, 100) <= 75) {
    roll1 *= 1.5;
  }

  if (char_is_konatsu(vict)) {
    roll1 *= 0.75;
  }

  if (char_get_skill(ch, SKILL_HANDLING) >= axion_dice(g, 10)) {
    handled = true;
  }

  if (roll1 < roll2) {
    struct obj_data *obj;
    if (char_get_equipment(ch, WEAR_WIELD1) && handled == false) {
      obj = char_get_equipment(ch, WEAR_WIELD1);
      act(g, "@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_NOTVICT);
      act(g, "@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_VICT);
      perform_remove(g, ch, 16);
      if (obj_get_vnum(g, obj) != 20098) {
        obj_from_char(g, obj);
        obj_to_room(g, obj, char_get_in_room(ch));
      }
    } else if (char_get_equipment(ch, WEAR_WIELD1) && handled == true) {
      obj = char_get_equipment(ch, WEAR_WIELD1);
      act(g, "@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it from slipping from your grasp!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true, ch, obj, vict, TO_NOTVICT);
      act(g, "@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true, ch, obj, vict, TO_VICT);
    } else if (char_get_equipment(ch, WEAR_WIELD2) && handled == true) {
      obj = char_get_equipment(ch, WEAR_WIELD2);
      act(g, "@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it from slipping from your grasp!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true, ch, obj, vict, TO_NOTVICT);
      act(g, "@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from slipping from $s grasp!@n", true, ch, obj, vict, TO_VICT);
    } else if (char_get_equipment(ch, WEAR_WIELD2)) {
      obj = char_get_equipment(ch, WEAR_WIELD2);
      act(g, "@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n", true, ch, obj, vict, TO_CHAR);
      act(g, "@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_NOTVICT);
      act(g, "@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n", true, ch, obj, vict, TO_VICT);
      perform_remove(g, ch, 17);
      if (obj_get_vnum(g, obj) != 20098) {
        obj_from_char(g, obj);
        obj_to_room(g, obj, char_get_in_room(ch));
      }
    }
  }
}

void combine_attacks(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  char chbuf[MAX_INPUT_LENGTH], victbuf[MAX_INPUT_LENGTH], rmbuf[MAX_INPUT_LENGTH];
  int64_t bonus = 0;
  double maxki  = 0.0;
  int totalmem  = 1;
  int attspd = 0, blockable = true, same = true, attsk = 0, attavg = char_get_skill(ch, attack_skills[char_get_combine(ch)]);
  int burn = false, shocked = false;

  switch (char_get_combine(ch)) {
    case 0: /* Kamehameha */
      sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You cup your hands at your sides and a ball of @Benergy@W forms there. You chant @B'@CKaaaaameeeehaaaameeee@B'@W and then fire a @RKamehameha@W wave at @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n");
      sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W cups $s hands be $s side and chants @B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands and he quickly brings them forward and fires a @RKamehameha @Wat @rYOU@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n");
      sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W cups $s hands be $s side and chants @B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands and he quickly brings them forward and fires a @RKamehameha @Wat @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n@n");
      maxki = 0.15;
      attspd += 2;
      bonus += getMaxST(g, ch) * 0.02;
      break;
    case 1: /* Galik Gun */
      sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You throw your hands forward and launch a purple beam of energy at @r$N@n while shouting @B'@mGalik Gun!@B'@W");
      sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W throws $s arms forward and launches a purple beam at @rYOU@W while shouting @B'@mGalik Gun!@B'@n");
      sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W throws $s arms forward and launches a purple beam at @r$N@W while shouting @B'@mGalik Gun!@B'@n");
      maxki = 0.15;
      attspd += 1;
      bonus += getMaxKI(g, ch) * 0.5;
      break;
    case 2: /* Masenko */
      sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! You raise your hands above your head with one resting atop the other and begin to pour your charged energy to that point. As soon as the energy is ready you shout @B'@RMasenko Ha!@B'@W and bringing your hands down you launch a bright reddish orange beam at @r$N@W!@n");
      sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises $s hands above $s head and energy quicly pools there. Suddenly $e brings $s hands down and shouts @B'@RMasenko Ha!@B'@W as a bright reddish orange beam launches toward @rYOU!@n");
      sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises $s hands above $s head and energy quicly pools there. Suddenly $e brings $s hands down and shouts @B'@RMasenko Ha!@B'@W as a bright reddish orange beam launches toward @r$N!@n");
      maxki = 0.15;
      attspd += 1;
      bonus += getMaxKI(g, ch) * 0.5;
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
      burn = true;
      break;
    case 5: /* Twin Slash */
      sprintf(chbuf, "@WPositioning yourself in the center of your group you call out to your allies to launch a group attack! With your energy prepared you poor it into your blade and accelerate your body to incredible speeds toward @r$N! You leave two glowing green marks behind on $S body in a single instant as your @GTwin Slash@W hits!@n");
      sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! Raising $s sword @Y$n@W accelerates toward @rYOU@W with incredible speed! Two glowing green slashes are left on YOUR body from $s successful @GTwin Slash@W!@n");
      sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! Raising $s sword @Y$n@W accelerates toward @r$N@W with incredible speed! Two glowing green slashes are left on @R$N's@W body from the successful @GTwin Slash@W!@n");
      maxki = 0.125;
      attspd += 2;
      blockable = false;
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
      shocked = true;
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
      bonus += getEffMaxPL(g, ch) * 0.5;
      break;
    case 11: /* Starbreaker */
      sprintf(chbuf, "@WPositioning yourself in the center of your group, you call out to your allies to launch a group attack! You raise your right hand above your head as dark red energy begins to pool in your slightly cupped hand, while purple arcs of electricity flow up your left arm. Slamming both hands together, you shout @B'@YStarbreaker@B'@W and release your prepared energy at $M in the form of a ball!@n");
      sprintf(victbuf, "@Y$n@W calls out to $s allies to launch a group attack against @rYOU@W! @Y$n@W raises $s right hand, pooling dark red energy in the palm. @Y$n@W slams both their hands together, shouting @B'@YStarbreaker@B'@W, a ball of energy flashes toward @rYOU!@n");
      sprintf(rmbuf, "@Y$n@W calls out to $s allies to launch a group attack against @r$N@W! @Y$n@W raises their right hand above their head, pooling dark red energy. @Y$n@W slams both their hands together, shouting @B'@YStarbreaker@B'@W, a ball of energy flashes toward @r$N!@n");
      maxki = 0.2;
      bonus += getMaxKI(g, ch) * 0.6;
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
      send_to_imm(g, "ERROR: Combine attacks failure for: %s", char_get_name(ch));
      send_to_char(g, ch, "An error has been logged. Be patient while waiting for Iovan's response.\r\n");
      return;
  }

  int64_t totki = 0;

  act(g, chbuf, true, ch, nullptr, vict, TO_CHAR);
  act(g, victbuf, true, ch, nullptr, vict, TO_VICT);
  act(g, rmbuf, true, ch, nullptr, vict, TO_NOTVICT);

  if (char_get_charge(ch) >= getMaxKI(g, ch) * maxki) {
    totki += getMaxKI(g, ch) * maxki;
    char_set_charge(ch, char_get_charge(ch) - (getMaxKI(g, ch) * maxki));
  } else {
    totki += char_get_charge(ch);
    char_set_charge(ch, 0);
  }

  for (char_data *fol : char_followers(g, ch)) {
    if (!char_aff_flagged(fol, AFF_GROUP)) {
      continue;
    }
    if (char_get_combine(fol) != char_get_combine(ch)) {
      same = false;
    }
    if (char_get_charge(fol) >= getMaxKI(g, fol) * maxki) {
      totki += getMaxKI(g, fol) * maxki;
      char_set_charge(fol, char_get_charge(fol) - (getMaxKI(g, fol) * maxki));
    } else {
      totki += char_get_charge(fol);
      char_set_charge(fol, 0);
    }
    totalmem += 1;
    attavg += char_get_skill(fol, attack_skills[char_get_combine(fol)]);
    char folbuf[MAX_INPUT_LENGTH], folbuf2[MAX_INPUT_LENGTH];
    sprintf(folbuf, "@Y$n@W times and merges $s @B'@R%s@B'@W into the group attack!@n", attack_names_comp[char_get_combine(fol)]);
    sprintf(folbuf2, "@WYou time and merge your @B'@R%s@B'@W into the group attack!@n", attack_names_comp[char_get_combine(fol)]);
    act(g, folbuf, true, fol, nullptr, nullptr, TO_ROOM);
    act(g, folbuf2, true, fol, nullptr, nullptr, TO_CHAR);
  }

  totki += bonus;
  if (same == true) {
    totki += bonus;
  }
  attsk = attavg / totalmem;

  if (char_get_combine(ch) != 5) {
    if (attspd + attsk < char_get_skill(vict, SKILL_DODGE) + (char_get_aff_abils_cha(ch) / 10)) {
      act(g, "@GYou manage to dodge nimbly through the combined attack of your enemies!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@G manages to dodge nimbly through the combined attack!@n", true, vict, nullptr, nullptr, TO_ROOM);
      return;
    }
    if (blockable == true && attspd + attsk < char_get_skill(vict, SKILL_BLOCK) + (char_get_aff_abils_str(ch) / 10)) {
      act(g, "@GYou manage to effectivly block the combined attack of your enemies with the help of your great strength!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@G manages to dodge nimbly through the combined attack!@n", true, vict, nullptr, nullptr, TO_ROOM);
      return;
    }
  }
  if (burn == true) {
    if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(g, 1, 4) == 3 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
      send_to_char(g, vict, "@RYou are burned by the attack!@n\r\n");
      send_to_char(g, ch, "@RThey are burned by the attack!@n\r\n");
      char_aff_set(vict, AFF_BURNED, true);
    } else if (char_get_bonuses(vict, BONUS_FIREPROOF) || char_is_demon(vict)) {
      send_to_char(g, ch, "@RThey appear to be fireproof!@n\r\n");
    } else if (char_get_bonuses(vict, BONUS_FIREPRONE)) {
      send_to_char(g, vict, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
      send_to_char(g, ch, "@RThey are easily burned!@n\r\n");
      char_aff_set(vict, AFF_BURNED, true);
    }
  }
  if (shocked == true) {
    if (!char_aff_flagged(vict, AFF_SHOCKED) && rand_number(g, 1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
      act(g, "@MYour mind has been shocked!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@M$n@m's mind has been shocked!@n", true, vict, nullptr, nullptr, TO_ROOM);
      char_aff_set(vict, AFF_SHOCKED, true);
    }
  }
  hurt(g, 0, 0, ch, vict, nullptr, totki, 1);
  if (same == true) {
    for (char_data *foll : char_followers(g, ch)) {
      send_to_char(g, foll, "@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n\r\n");
    }
    send_to_char(g, ch, "@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n\r\n");
  }
}

bool check_ruby(GameWorld *g, struct char_data *ch)
{

  struct obj_data *ruby = nullptr;
  int found = 0;

  for (obj_data *obj : char_inventory(g, ch)) {
    if (found == 0 && obj_get_vnum(g, obj) == 6600) {
      if (obj_extra_flagged(obj, ITEM_HOT)) {
        found = 1;
        ruby  = obj;
      }
    }
  }

  if (found > 0) {
    act(g, "@RYour $p@R flares up and disappears. Your fire attack has been aided!@n", true, ch, ruby, nullptr, TO_CHAR);
    act(g, "@R$n's@R $p@R flares up and disappears!@n", true, ch, ruby, nullptr, TO_ROOM);
    extract_obj(g, ruby);
    return true;
  }
  return false;
}

static int64_t combo_damage(struct char_data *ch, int64_t damage, int type)
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
int roll_balance(GameWorld *g, struct char_data *ch)
{

  int chance = 0;

  if (char_is_npc(ch)) {
    if (char_get_level(ch) >= 100) {
      chance = rand_number(g, 80, 100);
    } else if (char_get_level(ch) >= 80) {
      chance = rand_number(g, 75, 90);
    } else if (char_get_level(ch) >= 70) {
      chance = rand_number(g, 70, 80);
    } else if (char_get_level(ch) >= 60) {
      chance = rand_number(g, 65, 75);
    } else if (char_get_level(ch) >= 50) {
      chance = rand_number(g, 50, 60);
    }
  } else {
    if (char_get_skill(ch, SKILL_BALANCE) > 50) {
      chance = char_get_skill(ch, SKILL_BALANCE);
    }
  }

  return (chance);
}

void handle_knockdown(GameWorld *g, struct char_data *ch)
{
  int chance = 0;

  if (char_is_npc(ch)) {
    if (char_get_level(ch) >= 100) {
      chance = rand_number(g, 35, 45);
    } else if (char_get_level(ch) >= 90) {
      chance = rand_number(g, 25, 35);
    } else if (char_get_level(ch) >= 70) {
      chance = rand_number(g, 15, 25);
    } else if (char_get_level(ch) >= 50) {
      chance = rand_number(g, 10, 15);
    } else if (char_get_level(ch) >= 30) {
      chance = rand_number(g, 5, 10);
    }
  } else {
    chance = char_get_skill(ch, SKILL_BALANCE) * 0.5;
  }

  if (chance > axion_dice(g, 0)) {
    act(g, "@mYou are @GALMOST@m knocked off your feet, but your great balance helps you keep your footing!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@W$n@m is @GALMOST@m knocked off $s feet, but $s great balance helps $m keep $s footing!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "@mYou are knocked off your feet!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@W$n@m is knocked off $s feet!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_position(ch, POS_SITTING);
  }
}

bool boom_headshot(GameWorld *g, struct char_data *ch)
{

  int skill = char_get_skills_base(ch, SKILL_TWOHAND);

  if (skill >= 100 && rand_number(g, 1, 5) >= 3)
    return true;
  else if (skill < 100 && skill >= 75 && rand_number(g, 1, 5) == 5)
    return true;
  else if (skill < 75 && skill >= 50 && rand_number(g, 1, 6) == 6)
    return true;
  else
    return false;
}

int64_t gun_dam(GameWorld *g, struct char_data *ch, int wlvl)
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

  if (dmg_prior <= getEffMaxPL(g, ch) * 0.4)
    dmg = dmg_prior;
  else
    dmg = getEffMaxPL(g, ch) * 0.4;

  return (dmg);
}

void club_stamina(GameWorld *g, struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg)
{

  double drain    = 0.0;
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
  decCurST(g, vict, drained, 0);

  send_to_char(g, ch, "@D[@YVictim's @GStamina @cLoss@W: @g%s@D]@n\r\n", add_commas(drained));
  send_to_char(g, vict, "@D[@rYour @GStamina @cLoss@W: @g%s@D]@n\r\n", add_commas(drained));
}

bool backstab(GameWorld *g, struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg)
{

  int chance = 0, roll_to_beat = rand_number(g, 1, 100);
  double bonus = 0.0;

  if (char_get_backstabcool(ch) > 0)
    return false;

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
    int attacker_roll = char_get_skill(ch, SKILL_MOVE_SILENTLY) + char_get_skill(ch, SKILL_SPOT) + char_get_aff_abils_dex(ch) + rand_number(g, -5, 5);
    int defender_roll = char_get_skill(vict, SKILL_SPOT) + char_get_skill(vict, SKILL_LISTEN) + char_get_aff_abils_dex(ch) + rand_number(g, -5, 5);

    if (attacker_roll > defender_roll) {
      act(g, "@RYou manage to sneak behind @r$N@R and stab $M in the back!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RYou feel @r$n's@R dagger thrust into your back unexpectantly!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@r$n@R sneaks up behind @r$N@R and stabs $M in the back!@n", true, ch, nullptr, vict, TO_NOTVICT);
      dmg += dmg * bonus;
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
      return true;
    }
    return false;
  }
  return false;
}

void cut_limb(GameWorld *g, struct char_data *ch, struct char_data *vict, int wlvl, int hitspot)
{

  int chance = 0, decap = 0, decapitate = false;
  int roll_to_beat = rand_number(g, 1, 10000);

  if (wlvl == 1) {
    chance = 25;
  } else if (wlvl == 2) {
    chance = 50;
  } else if (wlvl == 3) {
    chance = 100;
    decap  = 5;
  } else if (wlvl == 4) {
    chance = 200;
    decap  = 10;
  } else if (wlvl == 5) {
    chance = 200;
    decap  = 50;
  }

  if (char_get_skill(ch, SKILL_SWORD) >= 100) {
    chance += 100;
  } else if (char_get_skill(ch, SKILL_SWORD) >= 50) {
    chance += 50;
  }

  if (decap >= roll_to_beat && hitspot == 4) {
    decapitate = true;
  } else if (chance < roll_to_beat) {
    return;
  }

  if (getCurPL(g, vict) <= 1) {
    return;
  }

  if (decapitate == true) {
    act(g, "@R$N's@r head is cut off in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@RYOUR head is cut off in the attack!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@R$N's@rhead is cut off in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);

    remove_limb(g, vict, 0);
    die(g, vict, ch);
    if (char_aff_flagged(ch, AFF_GROUP)) {
      group_gain(g, ch, vict);
    } else {
      solo_gain(g, ch, vict);
    }
    char corp[256];
    if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
      sprintf(corp, "all.zenni corpse");
      do_get(g, ch, corp, 0, 0);
    }
    if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
      sprintf(corp, "all corpse");
      do_get(g, ch, corp, 0, 0);
    }
    return;
  } else { /* We've only succeeded in removing a limb. */
    if (!char_is_npc(vict)) {
      if (char_has_arms(g, vict) && rand_number(g, 1, 2) == 2) {
        if (char_get_limb_condition(vict, (2) - 1) > 0) {
          char_set_limb_condition(vict, (2) - 1, 0);
          if (char_plr_flagged(vict, PLR_CLARM)) {
            char_plr_set(vict, PLR_CLARM, false);
          }
          act(g, "@R$N@r loses $s left arm!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your left arm!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
          remove_limb(g, vict, 2);
        } else if (char_get_limb_condition(vict, (1) - 1) > 0) {
          char_set_limb_condition(vict, (1) - 1, 100);
          if (char_plr_flagged(vict, PLR_CRARM)) {
            char_plr_set(vict, PLR_CRARM, false);
          }
          act(g, "@R$N@r loses $s right arm!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your right arm!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
          remove_limb(g, vict, 1);
        }
      } else { /* It's a leg */
        if (char_get_limb_condition(vict, (4) - 1) > 0) {
          char_set_limb_condition(vict, (4) - 1, 100);
          if (char_plr_flagged(vict, PLR_CLLEG)) {
            char_plr_set(vict, PLR_CLLEG, false);
          }
          act(g, "@R$N@r loses $s left leg!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your left leg!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
          remove_limb(g, vict, 4);
        } else if (char_get_limb_condition(vict, (3) - 1) > 0) {
          char_set_limb_condition(vict, (3) - 1, 100);
          if (char_plr_flagged(vict, PLR_CRLEG)) {
            char_plr_set(vict, PLR_CRLEG, false);
          }
          act(g, "@R$N@r loses $s right leg!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your right leg!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
          remove_limb(g, vict, 3);
        }
      }
    } else { /* It's a npc */
      if (char_has_arms(g, vict) && rand_number(g, 1, 2) == 2) {
        if (char_mob_flagged(vict, MOB_LARM)) {
          char_mob_set(vict, MOB_LARM, false);
          remove_limb(g, vict, 2);
          act(g, "@R$N@r loses $s left arm!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your left arm!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        } else if (char_mob_flagged(vict, MOB_RARM)) {
          char_mob_set(vict, MOB_RARM, false);
          remove_limb(g, vict, 1);
          act(g, "@R$N@r loses $s right arm!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your right arm!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        }
      } else {
        if (char_mob_flagged(vict, MOB_LLEG)) {
          char_mob_set(vict, MOB_LLEG, false);
          remove_limb(g, vict, 4);
          act(g, "@R$N@r loses $s left leg!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your left leg!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        } else if (char_mob_flagged(vict, MOB_RLEG)) {
          char_mob_set(vict, MOB_RLEG, false);
          remove_limb(g, vict, 3);
          act(g, "@R$N@r loses $s right leg!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@RYOU lose your right leg!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r loses $s right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        }
      }
    }
  }
}

static int count_physical(struct char_data *ch)
{
  int count = 0;

  if (char_get_skill(ch, SKILL_PUNCH) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_KICK) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_KNEE) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_ELBOW) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_ROUNDHOUSE) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_SLAM) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_UPPERCUT) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_TAILWHIP) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_BASH) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_HEADBUTT) >= 1) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_HEELDROP) >= 1) {
    count += 1;
  }

  return (count);
}

static int physical_mastery(struct char_data *ch)
{

  int count = 22;

  if (char_get_skill(ch, SKILL_PUNCH) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_KICK) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_KNEE) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_ELBOW) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_ROUNDHOUSE) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_SLAM) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_UPPERCUT) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_TAILWHIP) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_BASH) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_HEADBUTT) >= 100) {
    count += 1;
  }
  if (char_get_skill(ch, SKILL_HEELDROP) >= 100) {
    count += 1;
  }

  if (count == 26)
    count += 1;
  else if (count >= 27)
    count += 2;

  return (count);
}

static int64_t advanced_energy(GameWorld *g, struct char_data *ch, int64_t dmg)
{

  if (ch == nullptr) {
    return (false);
  }

  double rate = 0.00;
  int count   = char_get_level(ch);
  int64_t add = 0;

  if (char_get_bonuses(ch, BONUS_LEECH)) {
    rate = (double)(count) * 0.2;

    if (rate > 0.00) {
      add = dmg * rate;
      if (char_get_charge(ch) + add > getMaxKI(g, ch)) {
        if (char_get_charge(ch) < getMaxKI(g, ch)) {
          char_set_charge(ch, getMaxKI(g, ch));
          act(g, "@MYou leech some of the energy away!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@m$n@M leeches some of the energy away!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else {
          send_to_char(g, ch, "@MYou can't leech because there is too much charged energy for you to handle!@n\r\n");
        }
      } else {
        char_set_charge(ch, char_get_charge(ch) + (add));
        act(g, "@MYou leech some of the energy away!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@m$n@M leeches some of the energy away!@n", true, ch, nullptr, nullptr, TO_ROOM);
      }

    } /* End of rate check */

  } /* End of Leech Bonus */

  if (char_get_bonuses(ch, BONUS_INTOLERANT)) {
    rate = (double)(count) * 0.2;

    if (rate > 0.00) {
      if (char_get_charge(ch) > 0 && rand_number(g, 1, 100) <= 10) {
        act(g, "@MThe attack causes your weak control to slip and you are shocked by your own charged energy!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@m$n@M suffers shock from their own charged energy!@n", true, ch, nullptr, nullptr, TO_ROOM);
        decCurHealth(g, ch, char_get_charge(ch) / 4, 1);
      }
      add = dmg * rate;
    }
  } /* End of Energy Intolerant bonus */

  return (add);
} /* End of advanced_energy function */

int roll_accuracy(GameWorld *g, struct char_data *ch, int skill, bool kiatt)
{

  if (!char_is_npc(ch)) {
    if (char_get_bonuses(ch, BONUS_ACCURATE)) {
      if (kiatt == true)
        skill += skill * 0.10;
      else
        skill += skill * 0.20;
    } else if (char_get_bonuses(ch, BONUS_POORDEPTH)) {
      if (kiatt == true)
        skill -= skill * 0.10;
      else
        skill -= skill * 0.20;
    }
  }

  if (skill < 40) {
    skill += rand_number(g, 3, 10);
  }

  return (skill);
}

long double calc_critical(GameWorld *g, struct char_data *ch, int loc)
{

  int roll          = rand_number(g, 1, 100);
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

int roll_hitloc(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill)
{

  int location = 4, critmax = 1000;
  int critical = 0;

  if (char_is_npc(ch)) {
    if (char_get_level(ch) > 100)
      skill = rand_number(g, char_get_level(ch), char_get_level(ch) + 10);
    else
      skill = rand_number(g, char_get_level(ch), 100);
  }

  if (char_is_dabura(ch) && !char_is_npc(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      critmax -= 200;
  }

  critical = rand_number(g, 80, critmax);

  if (skill >= critical) {
    location = 2;
  } else if (skill >= rand_number(g, 50, 750)) {
    location = 2;
  } else if (skill >= rand_number(g, 50, 350)) {
    location = 1;
  } else if (skill >= rand_number(g, 30, 200)) {
    location = 3;
  } else {
    location = rand_number(g, 4, 5);
  }

  if (!char_is_npc(vict)) {
    if (location == 4 && char_get_limb_condition(vict, (1) - 1) <= 0 && char_get_limb_condition(vict, (2) - 1) <= 0) { /* No arms */
      location = 5;
    }

    if (location == 5 && char_get_limb_condition(vict, (3) - 1) <= 0 && char_get_limb_condition(vict, (4) - 1) <= 0) { /* No legs */
      location = 4;
    }

    if (location == 4 && char_get_limb_condition(vict, (1) - 1) <= 0 && char_get_limb_condition(vict, (2) - 1) <= 0) { /* Both failed, make body */
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

static int64_t armor_calc(struct char_data *ch, int64_t dmg, int type)
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
          loc   = 0;
          bonus = 0.05;
          break;
        case MATERIAL_IRON:
          loc   = 0;
          bonus = 0.025;
          break;
        case MATERIAL_COPPER:
        case MATERIAL_BRASS:
        case MATERIAL_METAL:
          loc   = 0;
          bonus = 0.01;
          break;
        case MATERIAL_SILVER:
          loc   = 1;
          bonus = 0.1;
          break;
        case MATERIAL_KACHIN:
          loc   = 2;
          bonus = 0.2;
          break;
        case MATERIAL_CRYSTAL:
          loc   = 1;
          bonus = 0.05;
          break;
        case MATERIAL_DIAMOND:
          loc   = 2;
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
          loc   = 2;
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
static void hurt_limb(GameWorld *g, struct char_data *ch, struct char_data *vict, int chance, int area, int64_t power)
{
  if (!vict || char_is_npc(vict)) {
    return;
  }

  int dmg = 0;

  if (chance > axion_dice(g, 100)) {
    return;
  }

  if (power > (getEffMaxPL(g, vict)) * 0.5) {
    dmg = rand_number(g, 25, 40);
  } else if (power > (getEffMaxPL(g, vict)) * 0.25) {
    dmg = rand_number(g, 15, 24);
  } else if (power > (getEffMaxPL(g, vict)) * 0.10) {
    dmg = rand_number(g, 8, 14);
  } else if (power > (getEffMaxPL(g, vict)) * 0.05) {
    dmg = rand_number(g, 4, 7);
  } else {
    dmg = rand_number(g, 1, 3);
    ;
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
    dmg -= rand_number(g, 0, 1);
  }

  if (dmg <= 0) {
    return;
  }

  if (!is_sparring(ch)) {
    if (area == 0) { /* Arms */
      if (char_get_limb_condition(vict, (2) - 1) - dmg <= 0) {
        act(g, "@RYour attack @YDESTROYS @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack @YDESTROYS@R YOUR left arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack @YDESTROYS @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_set_limb_condition(vict, (2) - 1, 0);
        if (char_plr_flagged(vict, PLR_THANDW)) {
          char_plr_set(vict, PLR_THANDW, false);
        }
        if (char_plr_flagged(vict, PLR_CLARM)) {
          char_plr_set(vict, PLR_CLARM, false);
        }
        remove_limb(g, vict, 2);
      } else if (char_get_limb_condition(vict, (2) - 1) > 0) {
        char_set_limb_condition(vict, (2) - 1, char_get_limb_condition(vict, (2) - 1) - (dmg));
        act(g, "@RYour attack hurts @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack hurts YOUR left arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack hurts @r$N's@R left arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
      } else if (char_get_limb_condition(vict, (1) - 1) - dmg <= 0) {
        act(g, "@RYour attack @YDESTROYS @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack @YDESTROYS@R YOUR right arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack @YDESTROYS @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_set_limb_condition(vict, (1) - 1, 0);
        if (char_plr_flagged(vict, PLR_THANDW)) {
          char_plr_set(vict, PLR_THANDW, false);
        }
        if (char_plr_flagged(vict, PLR_CLARM)) {
          char_plr_set(vict, PLR_CRARM, false);
        }
        remove_limb(g, vict, 2);
      } else if (char_get_limb_condition(vict, (1) - 1) > 0) {
        char_set_limb_condition(vict, (1) - 1, char_get_limb_condition(vict, (1) - 1) - (dmg));
        act(g, "@RYour attack hurts @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack hurts YOUR right arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack hurts @r$N's@R right arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
      }
    } else if (area == 1) { /* Legs */
      if (char_get_limb_condition(vict, (4) - 1) - dmg <= 0) {
        act(g, "@RYour attack @YDESTROYS @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack @YDESTROYS@R YOUR left leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack @YDESTROYS @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_set_limb_condition(vict, (4) - 1, 0);
        if (char_plr_flagged(vict, PLR_THANDW)) {
          char_plr_set(vict, PLR_THANDW, false);
        }
        if (char_plr_flagged(vict, PLR_CLLEG)) {
          char_plr_set(vict, PLR_CLLEG, false);
        }
        remove_limb(g, vict, 2);
      } else if (char_get_limb_condition(vict, (4) - 1) > 0) {
        char_set_limb_condition(vict, (4) - 1, char_get_limb_condition(vict, (4) - 1) - (dmg));
        act(g, "@RYour attack hurts @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack hurts YOUR left leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack hurts @r$N's@R left leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
      } else if (char_get_limb_condition(vict, (3) - 1) - dmg <= 0) {
        act(g, "@RYour attack @YDESTROYS @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack @YDESTROYS@R YOUR right leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack @YDESTROYS @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_set_limb_condition(vict, (3) - 1, 0);
        if (char_plr_flagged(vict, PLR_THANDW)) {
          char_plr_set(vict, PLR_THANDW, false);
        }
        if (char_plr_flagged(vict, PLR_CLLEG)) {
          char_plr_set(vict, PLR_CLLEG, false);
        }
        remove_limb(g, vict, 2);
      } else if (char_get_limb_condition(vict, (3) - 1) > 0) {
        char_set_limb_condition(vict, (3) - 1, char_get_limb_condition(vict, (3) - 1) - (dmg));
        act(g, "@RYour attack hurts @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@r$n's@R attack hurts YOUR right leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@r$n's@R attack hurts @r$N's@R right leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
      }
    }
  }
}

/* For damaging equipment when hit */
void dam_eq_loc(GameWorld *g, struct char_data *vict, int area)
{
  int location = 0, num = 0;
  /* Area is 4 possible hit locations in an attack.
    1 Arms, 2 legs, 3 head, and 4 body. */

  if (!vict || getCurPL(g, vict) <= 0) {
    return;
  }

  switch (area) {
    case 1:
      num = rand_number(g, 1, 8);
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
      num = rand_number(g, 1, 3);
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
      num = rand_number(g, 1, 6);
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
      num = rand_number(g, 1, 4);
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
  damage_eq(g, vict, location);
}

static void damage_eq(GameWorld *g, struct char_data *vict, int location)
{

  if (char_get_equipment(vict, location) && rand_number(g, 1, 20) >= 19 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
    struct obj_data *eq = char_get_equipment(vict, location);
    if (obj_extra_flagged(eq, ITEM_UNBREAKABLE)) {
      return;
    }

    int loss = rand_number(g, 2, 5);

    if (obj_get_vnum(g, eq) == 20099 || obj_get_vnum(g, eq) == 20098)
      loss = 1;

    if (char_aff_flagged(vict, AFF_CURSE)) {
      loss *= 3;
    } else if (char_aff_flagged(vict, AFF_BLESS) && rand_number(g, 1, 3) == 3) {
      loss = 1;
    } else if (char_aff_flagged(vict, AFF_BLESS)) {
      return;
    }

    obj_set_value(eq, VAL_ALL_HEALTH, obj_get_value(eq, VAL_ALL_HEALTH) - (loss));
    if (obj_get_value(eq, VAL_ALL_HEALTH) <= 0) {
      obj_set_value(eq, VAL_ALL_HEALTH, 0);
      obj_extra_set(eq, ITEM_BROKEN, true);
      act(g, "@WYour $p@W completely breaks!@n", false, nullptr, eq, vict, TO_VICT);
      act(g, "@C$N's@W $p@W completely breaks!@n", false, nullptr, eq, vict, TO_NOTVICT);
      perform_remove(g, vict, location);
      if (!char_is_npc(vict))
        save_char(g, vict);
    } else if (obj_get_value(eq, VAL_ALL_MATERIAL) == MATERIAL_LEATHER || obj_get_value(eq, VAL_ALL_MATERIAL) == MATERIAL_COTTON || obj_get_value(eq, VAL_ALL_MATERIAL) == MATERIAL_SILK) {
      act(g, "@WYour $p@W rips a little!@n", false, nullptr, eq, vict, TO_VICT);
      act(g, "@C$N's@W $p@W rips a little!@n", false, nullptr, eq, vict, TO_NOTVICT);
      if (char_aff_flagged(vict, AFF_BLESS)) {
        send_to_char(g, vict, "@c...But your blessing seems to have partly mended this damage.@n\r\n");
        act(g, "@c...but @C$N's@c body glows blue for a moment and the damage mends a little.@n", true, nullptr, nullptr, vict, TO_NOTVICT);
      } else if (char_aff_flagged(vict, AFF_CURSE)) {
        send_to_char(g, vict, "@r...and your curse seems to have made the damage three times worse!@n\r\n");
        act(g, "@c...but @C$N's@c body glows red for a moment and the damage grow three times worse!@n", true, nullptr, nullptr, vict, TO_NOTVICT);
      }
    } else {
      act(g, "@WYour $p@W cracks a little!@n", false, nullptr, eq, vict, TO_VICT);
      act(g, "@C$N's@W $p@W cracks a little!@n", false, nullptr, eq, vict, TO_NOTVICT);
      if (char_aff_flagged(vict, AFF_BLESS)) {
        send_to_char(g, vict, "@c...But your blessing seems to have partly mended this damage.@n\r\n");
        act(g, "@c...but @C$N's@c body glows blue for a moment and the damage mends a little.@n", true, nullptr, nullptr, vict, TO_NOTVICT);
      } else if (char_aff_flagged(vict, AFF_CURSE)) {
        send_to_char(g, vict, "@r...and your curse seems to have made the damage three times worse!@n\r\n");
        act(g, "@c...but @C$N's@c body glows red for a moment and the damage grow three times worse!@n", true, nullptr, nullptr, vict, TO_NOTVICT);
      }
    }
  }
}

/* This is for updating MOB android absorb */
/* This is for huge attacks that are slowly descending on a room */
void huge_update(GameWorld *g)
{
  int dge = 0, skill = 0, bonus = 1, count = 0;
  int64_t dmg = 0;
  struct char_data *ch;

  /* Checking the object list for any huge ki attacks */
  for (obj_data *k : each_obj(g)) { /* k may be extracted below */
    if (obj_get_aucter(k) > 0 && obj_get_aucTime(k) + 604800 <= game_now(g)) {
      if (obj_get_in_room(k) && obj_get_in_room(k) == 80) {
        room_vnum inroom = obj_get_in_room(k);
        room_set_flag(g, inroom, ROOM_HOUSE_CRASH, false);
        extract_obj(g, k);
        continue;
      }
    }
    if (obj_get_kicharge(k) <= 0) {
      continue;
    }
    if (obj_get_vnum(g, k) != 82 && obj_get_vnum(g, k) != 83) {
      continue;
    }
    if (obj_get_distance(k) <= 0) {
      /* Genki Dama Section */
      if (obj_get_kitype(k) == 497) {
        if (char_get_in_room(obj_get_target(g, k)) == obj_get_in_room(k)) {
          ch = obj_get_user(g, k);
          if (char_get_in_room(ch) == obj_get_in_room(k)) {
            bonus = 2;
          }

          act(g, "@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on YOU! It eclipses everything above you as it crushes down into you! You struggle against it with all your might!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_CHAR);
          act(g, "@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on @C$n@W! It completely obscures $m from view as it crushes into $s body! It appears to be facing some resistance from $m!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_ROOM);
          send_to_room(g, obj_get_in_room(k), "\r\n");
          if (getCurPL(g, obj_get_target(g, k)) * bonus < obj_get_kicharge(k) * 5) {

            act(g, "@WYour strength is no match for the power of the attack! It slowly grinds into you before exploding into a massive blast!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_CHAR);
            act(g, "@C$n@W's strength is no match for the power of the attack! It slowly grinds into $m before exploding into a massive blast!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_ROOM);
            skill = init_skill(g, ch, SKILL_GENKIDAMA); /* Set skill value */
            dmg   = obj_get_kicharge(k) * 1.25;
            hurt(g, 0, 0, ch, obj_get_target(g, k), nullptr, dmg, 1);
            dmg /= 2;

            /* Hit those in the current room. */
            for (char_data *vict : room_people(g, obj_get_in_room(k))) {

              if (vict == ch) {
                continue;
              }
              if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
                continue;
              }
              if (vict == obj_get_target(g, k)) {
                continue;
              }
              if (char_aff_flagged(vict, AFF_GROUP)) {
                if (char_get_master(g, vict) == ch) {
                  continue;
                }
                if (char_get_master(g, ch) == vict) {
                  continue;
                }
                if (char_get_master(g, vict) == char_get_master(g, ch)) {
                  continue;
                }
              }
              if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
                continue;
              }
              if (char_mob_flagged(vict, MOB_NOKILL)) {
                continue;
              }
              dge = handle_dodge(g, vict);
              if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
                  (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
                act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
                act(g, "@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
                act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_NOTVICT);
                char_aff_set(vict, AFF_ZANZOKEN, false);
                pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
                hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
                continue;
              }
              if (dge + rand_number(g, -10, 5) > skill) {
                act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
                improve_skill(g, vict, SKILL_DODGE, 0);
                continue;
              }
              count += 1;
              if (char_is_npc(vict) && count > 10) {
                if (getCurPL(g, vict) < dmg) {
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
              act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
              act(g, "@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
              act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
              hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
              continue;
            }
            room_set_dmg(g, obj_get_in_room(k), 100);
            struct zone_data *zone = nullptr;
            if ((zone = find_zone_by_thing(g, char_get_in_room(ch))) != nullptr) {
              send_to_zone(g, "A MASSIVE explosion shakes the entire area!\r\n", zone);
            }

            extract_obj(g, k);
            continue;
          } /* It isn't stopped! */
          else {
            act(g, "@WYou manage to overpower the attack! You lift up into the sky slowly with it and toss it up and away out of sight!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_CHAR);
            act(g, "@C$n@W manages to unbelievably overpower the attack! It is lifted up into the sky and tossed away dramaticly!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_ROOM);
            hurt(g, 0, 0, ch, obj_get_target(g, k), nullptr, 0, 1);
            decCurST(g, obj_get_target(g, k), obj_get_kicharge(k) / 4, 0);
            extract_obj(g, k);
            continue;
          }
        } else if (char_get_in_room(obj_get_target(g, k)) != obj_get_in_room(k)) {
          ch = obj_get_user(g, k);

          send_to_room(g, obj_get_in_room(k), "@WThe large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W descends on the area! It slowly burns into the ground before exploding magnificently!@n\r\n");

          skill = init_skill(g, ch, SKILL_GENKIDAMA); /* Set skill value */
          dmg   = obj_get_kicharge(k);
          dmg /= 2;

          /* Hit those in the current room. */
          for (char_data *vict : room_people(g, obj_get_in_room(k))) {

            if (vict == ch) {
              continue;
            }
            if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
              continue;
            }
            if (char_aff_flagged(vict, AFF_GROUP) && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict)) {
              continue;
            }
            if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
              continue;
            }
            if (char_mob_flagged(vict, MOB_NOKILL)) {
              continue;
            }
            dge = handle_dodge(g, vict);
            if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
                (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
              act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
              act(g, "@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
              act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_NOTVICT);
              char_aff_set(vict, AFF_ZANZOKEN, false);
              pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
              hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
              continue;
            }
            if (dge + rand_number(g, -10, 5) > skill) {
              act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
              act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
              act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
              hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
              improve_skill(g, vict, SKILL_DODGE, 0);
              continue;
            }
            act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
            continue;
          }
          room_set_dmg(g, obj_get_in_room(k), 100);
          struct zone_data *zone = nullptr;
          if ((zone = find_zone_by_thing(g, char_get_in_room(ch))) != nullptr) {
            send_to_zone(g, "A MASSIVE explosion shakes the entire area!\r\n", zone);
          }
          extract_obj(g, k);
          continue;
        } /* End Genki */
        continue;
      }
      /* Genocide Section */
      if (obj_get_kitype(k) == 498) {
        if (char_get_in_room(obj_get_target(g, k)) == obj_get_in_room(k)) {
          ch = obj_get_user(g, k);
          if (char_get_in_room(ch) == obj_get_in_room(k)) {
            bonus = 2;
          }

          act(g, "@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on YOU! It eclipses everything above you as it crushes down into you! You struggle against it with all your might!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_CHAR);
          act(g, "@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on @C$n@W! It completely obscures $m from view as it crushes into $s body! It appears to be facing some resistance from $m!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_ROOM);
          send_to_room(g, obj_get_in_room(k), "\r\n");
          if (getCurPL(g, obj_get_target(g, k)) * bonus < obj_get_kicharge(k) * 10) {

            act(g, "@WYour strength is no match for the power of the attack! It slowly grinds into you before exploding into a massive blast!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_CHAR);
            act(g, "@C$n@W's strength is no match for the power of the attack! It slowly grinds into $m before exploding into a massive blast!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_ROOM);
            skill = init_skill(g, ch, SKILL_GENOCIDE); /* Set skill value */
            dmg   = obj_get_kicharge(k);
            hurt(g, 0, 0, ch, obj_get_target(g, k), nullptr, dmg, 1);
            dmg /= 2;

            /* Hit those in the current room. */
            for (char_data *vict : room_people(g, obj_get_in_room(k))) {

              if (vict == ch) {
                continue;
              }
              if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
                continue;
              }
              if (vict == obj_get_target(g, k)) {
                continue;
              }
              if (char_aff_flagged(vict, AFF_GROUP)) {
                if (char_get_master(g, vict) == ch) {
                  continue;
                }
                if (char_get_master(g, ch) == vict) {
                  continue;
                }
                if (char_get_master(g, vict) == char_get_master(g, ch)) {
                  continue;
                }
              }
              if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
                continue;
              }
              if (char_mob_flagged(vict, MOB_NOKILL)) {
                continue;
              }
              dge = handle_dodge(g, vict);
              if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
                  (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
                act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
                act(g, "@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
                act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_NOTVICT);
                char_aff_set(vict, AFF_ZANZOKEN, false);
                pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
                continue;
              }
              if (dge + rand_number(g, -10, 5) > skill) {
                act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
                act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
                act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
                hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
                improve_skill(g, vict, SKILL_DODGE, 0);
                continue;
              }
              count += 1;
              if (char_is_npc(vict) && count > 10) {
                if (getCurPL(g, vict) < dmg) {
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
              act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
              act(g, "@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
              act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
              hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
              continue;
            }
            room_set_dmg(g, obj_get_in_room(k), 100);
            struct zone_data *zone = nullptr;
            if ((zone = find_zone_by_thing(g, char_get_in_room(ch))) != nullptr) {
              send_to_zone(g, "A MASSIVE explosion shakes the entire area!\r\n", zone);
            }
            extract_obj(g, k);
            continue;
          } /* It isn't stopped! */
          else {
            act(g, "@WYou manage to overpower the attack! You lift up into the sky slowly with it and toss it up and away out of sight!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_CHAR);
            act(g, "@C$n@W manages to unbelievably overpower the attack! It is lifted up into the sky and tossed away dramaticly!@n", true, obj_get_target(g, k), nullptr, nullptr, TO_ROOM);
            hurt(g, 0, 0, ch, obj_get_target(g, k), nullptr, 0, 1);
            decCurST(g, obj_get_target(g, k), obj_get_kicharge(k) / 4, 0);
            extract_obj(g, k);
            continue;
          }
        } else if (char_get_in_room(obj_get_target(g, k)) != obj_get_in_room(k)) {
          ch = obj_get_user(g, k);

          send_to_room(g, obj_get_in_room(k), "@WThe large @mG@Me@wn@mo@Mc@wi@md@Me@W descends on the area! It slowly burns into the ground before exploding magnificantly!@n\r\n");

          skill = init_skill(g, ch, SKILL_GENOCIDE); /* Set skill value */
          dmg   = obj_get_kicharge(k);
          dmg /= 2;

          /* Hit those in the current room. */
          for (char_data *vict : room_people(g, obj_get_in_room(k))) {

            if (vict == ch) {
              continue;
            }
            if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
              continue;
            }
            if (char_aff_flagged(vict, AFF_GROUP) && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict)) {
              continue;
            }
            if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
              continue;
            }
            if (char_mob_flagged(vict, MOB_NOKILL)) {
              continue;
            }
            dge = handle_dodge(g, vict);
            if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
                (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
              act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_CHAR);
              act(g, "@cYou disappear, avoiding the explosion!@n", false, ch, nullptr, vict, TO_VICT);
              act(g, "@C$N@c disappears, avoiding the explosion!@n", false, ch, nullptr, vict, TO_NOTVICT);
              char_aff_set(vict, AFF_ZANZOKEN, false);
              pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
              continue;
            }
            if (dge + rand_number(g, -10, 5) > skill) {
              act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
              act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
              act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
              hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
              improve_skill(g, vict, SKILL_DODGE, 0);
              continue;
            }
            act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
            continue;
          }
          room_set_dmg(g, obj_get_in_room(k), 100);
          struct zone_data *zone = nullptr;
          if ((zone = find_zone_by_thing(g, char_get_in_room(ch))) != nullptr) {
            send_to_zone(g, "A MASSIVE explosion shakes the entire area!\r\n", zone);
          }
          extract_obj(g, k);
          continue;
        } /* End Genocide */
        continue;
      }
      extract_obj(g, k);
      continue;
    }
    act(g, "$p@W descends slowly towards the ground!@n", true, nullptr, k, nullptr, TO_ROOM);
    obj_set_distance(k, obj_get_distance(k) - 1);
  }
}
/* End huge ki attack update */

/* For handling homing attacks */
void homing_update(GameWorld *g)
{

  for (obj_data *k : each_obj(g)) {

    if (obj_get_kicharge(k) <= 0) {
      continue;
    }

    if (obj_get_vnum(g, k) != 80 && obj_get_vnum(g, k) != 81 && obj_get_vnum(g, k) != 84) {
      continue;
    }
    if (obj_get_target(g, k) && obj_get_user(g, k)) {
      struct char_data *ch   = obj_get_user(g, k);
      struct char_data *vict = obj_get_target(g, k);

      if (obj_get_vnum(g, k) == 80) { // Tsuihidan
        if (obj_get_in_room(k) == NOWHERE) /* someone has picked it up, or it's inside something */
          continue;
        if (obj_get_in_room(k) != char_get_in_room(vict)) {
          act(g, "@wThe $p@w pursues after you!@n", true, vict, k, nullptr, TO_CHAR);
          act(g, "@wThe $p@W pursues after @C$n@w!@n", true, vict, k, nullptr, TO_ROOM);
          obj_from_room(g, k);
          obj_to_room(g, k, char_get_in_room(vict));
          continue;
        }
        act(g, "@RThe $p@R makes a tight turn and rockets straight for you!@n", true, vict, k, nullptr, TO_CHAR);
        act(g, "@RThe $p@R makes a tight turn and rockets straight for @r$n@n", true, vict, k, nullptr, TO_ROOM);
        if (handle_parry(g, vict) < rand_number(g, 1, 140)) {
          act(g, "@rThe $p@r slams into your body, exploding in a flash of bright light!@n", true, vict, k, nullptr, TO_CHAR);
          act(g, "@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", true, vict, k, nullptr, TO_ROOM);
          int64_t dmg = obj_get_kicharge(k);
          extract_obj(g, k);
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          continue;
        }
        if (rand_number(g, 1, 3) > 1) {
          act(g, "@wYou manage to deflect the $p@W sending it flying away and depleting some of its energy.@n", true, vict, k, nullptr, TO_CHAR);
          act(g, "@C$n @wmanages to deflect the $p@w sending it flying away and depleting some of its energy.@n", true, vict, k, nullptr, TO_ROOM);
          obj_set_kicharge(k, obj_get_kicharge(k) - (obj_get_kicharge(k) / 10));
          if (obj_get_kicharge(k) <= 0) {
            send_to_room(g, obj_get_in_room(k), "%s has lost all its energy and disappears.\r\n", obj_get_short_description(k));
            extract_obj(g, k);
          }
          continue;
        }
        act(g, "@wYou manage to deflect the $p@w sending it flying away into the nearby surroundings!@n", true, vict, k, nullptr, TO_CHAR);
        act(g, "@C$n @wmanages to deflect the $p@w sending it flying away into the nearby surroundings!@n", true, vict, k, nullptr, TO_ROOM);
        if (room_get_dmg(g, char_get_in_room(vict)) <= 95) {
          room_set_dmg(g, char_get_in_room(vict), room_get_dmg(g, char_get_in_room(vict)) + (5));
        }
        extract_obj(g, k);
        continue;
        continue;
      } else if (obj_get_vnum(g, k) == 81 || obj_get_vnum(g, k) == 84) { // Spirit Ball
        if (obj_get_in_room(k) != char_get_in_room(vict)) {
          act(g, "@wYou lose sight of @C$N@W and let $p@W fly away!@n", true, ch, k, vict, TO_CHAR);
          act(g, "@wYou manage to escape @C$n's@W $p@W!@n", true, ch, k, vict, TO_VICT);
          act(g, "@C$n@W loses sight of @c$N@W and lets $s $p@W fly away!@n", true, ch, k, vict, TO_NOTVICT);
          extract_obj(g, k);
          continue;
        }
        act(g, "@RYou move your hand and direct $p@R after @r$N@R!@n", true, ch, k, vict, TO_CHAR);
        act(g, "@r$n@R moves $s hand and directs $p@R after YOU!@n", true, ch, k, vict, TO_VICT);
        act(g, "@r$n@R moves $s hand and directs $p@R after @r$N@R!@n", true, ch, k, vict, TO_NOTVICT);
        if (handle_parry(g, vict) < rand_number(g, 1, 140)) {
          if (obj_get_vnum(g, k) != 84) {
            act(g, "@rThe $p@r slams into your body, exploding in a flash of bright light!@n", true, vict, k, nullptr, TO_CHAR);
            act(g, "@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", true, vict, k, nullptr, TO_ROOM);
            int64_t dmg = obj_get_kicharge(k);
            extract_obj(g, k);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          } else if (obj_get_vnum(g, k) == 84) {
            int64_t dmg = obj_get_kicharge(k);
            if (dmg > getEffMaxPL(g, vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
              act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
              act(g, "@rYou are cut in half by the attack!@n", true, ch, nullptr, vict, TO_VICT);
              act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
              die(g, vict, ch);
              if (char_aff_flagged(ch, AFF_GROUP)) {
                group_gain(g, ch, vict);
              } else {
                solo_gain(g, ch, vict);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
                do_get(g, ch, "all.zenni corpse", 0, 0);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
                do_get(g, ch, "all corpse", 0, 0);
              }
            } else if (dmg > getEffMaxPL(g, vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
              if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(g, 1, 101) && (getCurKI(g, vict)) >= getMaxKI(g, vict) / 40) {
                act(g, "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
                act(g, "@rYou are cut in half by the attack but regenerate a moment later!@n", true, ch, nullptr, vict, TO_VICT);
                act(g, "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
                decCurKI(g, vict, getMaxKI(g, vict) / 40, 0);
                hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
              } else if (dmg > getEffMaxPL(g, vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
                if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(g, 1, 101) && (getCurKI(g, vict)) >= getMaxKI(g, vict) / 40) {
                  act(g, "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
                  act(g, "@rYou are cut in half by the attack but regenerate a moment later!@n", true, ch, nullptr, vict, TO_VICT);
                  act(g, "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
                  decCurKI(g, vict, getMaxKI(g, vict) / 40, 0);
                  hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
                } else {
                  act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
                  act(g, "@rYou are cut in half by the attack!@n", true, ch, nullptr, vict, TO_VICT);
                  act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
                  die(g, vict, ch);
                  if (char_aff_flagged(ch, AFF_GROUP)) {
                    group_gain(g, ch, vict);
                  } else {
                    solo_gain(g, ch, vict);
                  }
                  if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
                    do_get(g, ch, "all.zenni corpse", 0, 0);
                  }
                  if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
                    do_get(g, ch, "all corpse", 0, 0);
                  }
                }
              }
            } else {
              act(g, "@rThe $p@r slams into your body, exploding in a flash of bright light!@n", true, vict, k, nullptr, TO_CHAR);
              act(g, "@rThe $p@r slams into @R$n's@r body, exploding in a flash of bright light!@n", true, vict, k, nullptr, TO_ROOM);
              hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
            }
            extract_obj(g, k);
          }
          continue;
        }
        if (rand_number(g, 1, 3) > 1) {
          act(g, "@wYou manage to deflect the $p@W sending it flying away and depleting some of its energy.@n", true, vict, k, nullptr, TO_CHAR);
          act(g, "@C$n @wmanages to deflect the $p@w sending it flying away and depleting some of its energy.@n", true, vict, k, nullptr, TO_ROOM);
          obj_set_kicharge(k, obj_get_kicharge(k) - (obj_get_kicharge(k) / 10));
          if (obj_get_kicharge(k) <= 0) {
            send_to_room(g, obj_get_in_room(k), "%s has lost all its energy and disappears.\r\n", obj_get_short_description(k));
            extract_obj(g, k);
          }
          continue;
        }
        act(g, "@wYou manage to deflect the $p@w sending it flying away into the nearby surroundings!@n", true, vict, k, nullptr, TO_CHAR);
        act(g, "@C$n @wmanages to deflect the $p@w sending it flying away into the nearby surroundings!@n", true, vict, k, nullptr, TO_ROOM);
        if (room_get_dmg(g, char_get_in_room(vict)) <= 95) {
          room_set_dmg(g, char_get_in_room(vict), room_get_dmg(g, char_get_in_room(vict)) + (5));
        }
        extract_obj(g, k);
        continue;
      } // Spiritball attack
    } // pursue the target.
  } // End for
}

static int handle_block(GameWorld *g, struct char_data *ch)
{

  if (axion_dice(g, 0) <= 4) { /* Critical failure */
    return (1);
  }

  if (!char_is_npc(ch)) { /* Players */
    if (!char_get_skill(ch, SKILL_BLOCK)) {
      return (0);
    }
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
  } else {                       /* Mobs */
    if (!char_is_humanoid(ch)) { /* Animal/monster types */
      int top = char_get_level(ch) / 4;

      if (top < 5)
        top = 6;
      return (rand_number(g, 5, top));
    } else { /* Intelligent Skills Mobs */
      if (char_get_level(ch) >= 110) {
        return (rand_number(g, 95, 105));
      }
      if (char_get_level(ch) >= 100) {
        return (rand_number(g, 85, 95));
      }
      if (char_get_level(ch) >= 90) {
        return (rand_number(g, 70, 85));
      }
      if (char_get_level(ch) >= 75) {
        return (rand_number(g, 50, 70));
      }
      if (char_get_level(ch) >= 40) {
        return (rand_number(g, 40, 50));
      }
      int top = char_get_level(ch);

      if (top < 15)
        top = 16;
      return (rand_number(g, 15, top));
    }
  }
}

int handle_dodge(GameWorld *g, struct char_data *ch)
{

  if (axion_dice(g, 0) <= 4) { /* Critical failure */
    return (1);
  }

  if (!char_is_npc(ch)) { /* Players */
    if (!char_get_skill(ch, SKILL_DODGE)) {
      return (0);
    }
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
    if (group_bonus(g, ch, 2) == 8) {
      num += num * 0.05;
    }
    return (num);
  } else {                       /* Mobs */
    if (!char_is_humanoid(ch)) { /* Animal/monster types */
      int top = (char_get_level(ch) + 1) / 8;

      if (top < 5)
        top = 6;
      return (rand_number(g, 5, top));
    } else { /* Intelligent Skills Mobs */
      if (char_get_level(ch) >= 110) {
        return (rand_number(g, 95, 105));
      }
      if (char_get_level(ch) >= 100) {
        return (rand_number(g, 75, 95));
      }
      if (char_get_level(ch) >= 90) {
        return (rand_number(g, 50, 85));
      }
      if (char_get_level(ch) >= 75) {
        return (rand_number(g, 30, 70));
      }
      if (char_get_level(ch) >= 40) {
        return (rand_number(g, 20, 50));
      }
      int top = char_get_level(ch);

      if (top < 15)
        top = 16;
      return (rand_number(g, 15, top));
    }
  }
}

int check_def(GameWorld *g, struct char_data *vict)
{
  int index = 0;
  int pry = handle_parry(g, vict), dge = handle_dodge(g, vict), blk = handle_block(g, vict);

  index = pry + dge + blk;

  if (index > 0)
    index /= 3;

  if (char_aff_flagged(vict, AFF_KNOCKED)) {
    index = 0;
  }
  return index;
}

void handle_defense(GameWorld *g, struct char_data *vict, int *pry, int *blk, int *dge)
{

  if (!char_is_npc(vict)) {
    *pry = handle_parry(g, vict);

    *blk = handle_block(g, vict);

    *dge = handle_dodge(g, vict);

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
    if (char_plr_flagged(vict, PLR_GOOP) && rand_number(g, 1, 100) >= 15) {
      *dge += 100;
      *blk += 100;
      *pry += 100;
    }
  } else {
    *pry = handle_parry(g, vict);

    *blk = handle_block(g, vict);

    *dge = handle_dodge(g, vict);
  }

  return;
}

void parry_ki(GameWorld *g, double attperc, struct char_data *ch, struct char_data *vict, const char *sname, int prob, int perc, int skill, int type)
{
  char buf[200];
  char buf2[200];
  char buf3[200];
  int foundv = false, foundo = false;
  int64_t dmg = 0;

  for (char_data *tch : room_people(g, char_get_in_room(ch))) {

    if (tch == ch)
      continue;
    if (tch == vict)
      continue;
    if (!can_kill(g, ch, tch, nullptr, 1))
      continue;

    if (rand_number(g, 1, 101) >= 90 && foundv == false) {
      if (handle_parry(g, tch) > rand_number(g, 1, 140)) {
        sprintf(buf, "@C$N@W deflects your %s, sending it flying away!@n", sname);
        sprintf(buf2, "@WYou deflect @C$n's@W %s sending it flying away!@n", sname);
        sprintf(buf3, "@C$N@W deflects @c$n's@W %s sending it flying away!@n", sname);
        act(g, buf, true, ch, nullptr, tch, TO_CHAR);
        act(g, buf2, true, ch, nullptr, tch, TO_VICT);
        act(g, buf3, true, ch, nullptr, tch, TO_NOTVICT);
        foundv = false;
      } else {
        foundv = true;
        sprintf(buf, "@WYou watch as the deflected %s slams into @C$N@W, exploding with a roar of blinding light!@n", sname);
        sprintf(buf2, "@c$n@W watches as the deflected %s slams into you! The %s explodes with a roar of blinding light!@n", sname, sname);
        sprintf(buf3, "@c$n@W watches as the deflected %s slams into @C$N@W! The %s explodes with a roar of blinding light!@n", sname, sname);
        act(g, buf, true, vict, nullptr, tch, TO_CHAR);
        act(g, buf2, true, vict, nullptr, tch, TO_VICT);
        act(g, buf3, true, vict, nullptr, tch, TO_NOTVICT);
        dmg = damtype(g, ch, type, skill, attperc);
        hurt(g, 0, 0, ch, tch, nullptr, dmg, 1);
        return;
      }
    }
  }

  for (obj_data *tob : room_contents(g, char_get_in_room(ch))) {
    if (obj_extra_flagged(tob, ITEM_UNBREAKABLE))
      continue;
    if (foundo == true)
      continue;
    if (rand_number(g, 1, 101) >= 80) {
      foundo = true;
      sprintf(buf, "@WYou watch as the deflected %s slams into @g$p@W, exploding with a roar of blinding light!@n", sname);
      sprintf(buf2, "@c$n@W watches as the deflected %s slams into @g$p@W, exploding with a roar of blinding light!@n", sname);
      act(g, buf, true, vict, tob, nullptr, TO_CHAR);
      act(g, buf2, true, vict, tob, nullptr, TO_ROOM);
      hurt(g, 0, 0, ch, nullptr, tob, 25, 1);
      return;
    }
  }

  if ((foundo == false || foundv == false) && !room_flagged(g, char_get_in_room(vict), ROOM_SPACE)) {
    sprintf(buf, "@WYou watch as the deflected %s slams into the ground, exploding with a roar of blinding light!@n", sname);
    sprintf(buf2, "@WThe deflected %s slams into the ground, exploding with a roar of blinding light!@n", sname);
    act(g, buf, true, vict, nullptr, nullptr, TO_CHAR);
    act(g, buf2, true, vict, nullptr, nullptr, TO_ROOM);
    if (room_sect(g, char_get_in_room(vict)) != SECT_INSIDE && room_sect(g, char_get_in_room(vict)) != SECT_UNDERWATER && room_sect(g, char_get_in_room(vict)) != SECT_WATER_SWIM && room_sect(g, char_get_in_room(vict)) != SECT_WATER_NOSWIM && room_sect(g, char_get_in_room(vict)) != SECT_UNDERWATER && room_sect(g, char_get_in_room(vict)) != SECT_WATER_SWIM && room_sect(g, char_get_in_room(vict)) != SECT_WATER_NOSWIM) {
      impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
      switch (rand_number(g, 1, 8)) {
        case 1:
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          if (rand_number(g, 1, 4) == 4 && room_get_geffect(g, char_get_in_room(vict)) == 0) {
            room_set_geffect(g, char_get_in_room(vict), 1);
            act(g, "Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", true, ch, nullptr, vict, TO_CHAR);
            act(g, "Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", true, ch, nullptr, vict, TO_ROOM);
          }
          break;
        case 3:
          act(g, "A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 4:
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 5:
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 6:
          act(g, "The explosion continues to burn spreading out and devouring some more of the ground before dying out.", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The explosion continues to burn spreading out and devouring some more of the ground before dying out.", true, ch, nullptr, vict, TO_ROOM);
          break;
        default:
          /* we want no message for the default */
          break;
      }
    }
    if (room_sect(g, char_get_in_room(vict)) == SECT_UNDERWATER) {
      switch (rand_number(g, 1, 3)) {
        case 1:
          act(g, "The water churns violently!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The water churns violently!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          act(g, "Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 3:
          act(g, "The water collapses in on the hole created!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The water collapses in on the hole create!", true, ch, nullptr, vict, TO_ROOM);
          break;
      }
    }
    if (room_sect(g, char_get_in_room(vict)) == SECT_WATER_SWIM || room_sect(g, char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
      switch (rand_number(g, 1, 3)) {
        case 1:
          act(g, "A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          act(g, "The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 3:
          act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
      }
    }
    if (room_sect(g, char_get_in_room(vict)) == SECT_INSIDE) {
      impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
      switch (rand_number(g, 1, 8)) {
        case 1:
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          act(g, "The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 3:
          act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 4:
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 5:
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 6:
          act(g, "The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict, TO_ROOM);
          break;
        default:
          /* we want no message for the default */
          break;
      }
    }
    if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
      room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
    }
    struct zone_data *zone = nullptr;
    if ((zone = find_zone_by_thing(g, char_get_in_room(ch))) != nullptr) {
      send_to_zone(g, "An explosion shakes the entire area!\r\n", zone);
    }
    return;
  }
}

void dodge_ki(GameWorld *g, struct char_data *ch, struct char_data *vict, int type, int type2, int skill, int skill2)
{
  if (type == 0 && !room_flagged(g, char_get_in_room(vict), ROOM_SPACE)) {
    if (room_sect(g, char_get_in_room(ch)) != SECT_INSIDE) {
      impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
      switch (rand_number(g, 1, 8)) {
        case 1:
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          if (rand_number(g, 1, 4) == 4 && room_get_geffect(g, char_get_in_room(vict)) == 0) {
            room_set_geffect(g, char_get_in_room(vict), 1);
            act(g, "Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", true, ch, nullptr, vict, TO_CHAR);
            act(g, "Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", true, ch, nullptr, vict, TO_ROOM);
          }
          break;
        case 3:
          act(g, "A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "A cloud of dust envelopes the entire area!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 4:
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 5:
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 6:
          act(g, "The explosion continues to burn spreading out and devouring some more of the ground before dying out.", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The explosion continues to burn spreading out and devouring some more of the ground before dying out.", true, ch, nullptr, vict, TO_ROOM);
          break;
        default:
          /* we want no message for the default */
          break;
      }
    }
    if (room_sect(g, char_get_in_room(vict)) == SECT_UNDERWATER) {
      switch (rand_number(g, 1, 3)) {
        case 1:
          act(g, "The water churns violently!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The water churns violently!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          act(g, "Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Large bubbles rise from the movement!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 3:
          act(g, "The water collapses in on the hole created!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The water collapses in on the hole create!", true, ch, nullptr, vict, TO_ROOM);
          break;
      }
    }
    if (room_sect(g, char_get_in_room(vict)) == SECT_WATER_SWIM || room_sect(g, char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
      switch (rand_number(g, 1, 3)) {
        case 1:
          act(g, "A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "A huge column of water erupts from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          act(g, "The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The impact briefly causes a swirling vortex of water!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 3:
          act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
      }
    }
    if (room_sect(g, char_get_in_room(ch)) == SECT_INSIDE) {
      impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
      switch (rand_number(g, 1, 8)) {
        case 1:
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 2:
          act(g, "The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 3:
          act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 4:
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 5:
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, vict, TO_ROOM);
          break;
        case 6:
          act(g, "The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict, TO_CHAR);
          act(g, "The walls of the surrounding room crack in the same instant!", true, ch, nullptr, vict, TO_ROOM);
          break;
        default:
          /* we want no message for the default */
          break;
      }
    }
    if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
      room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
    }
    struct zone_data *zone = nullptr;
    if ((zone = find_zone_by_thing(g, char_get_in_room(ch))) != nullptr) {
      send_to_zone(g, "An explosion shakes the entire area!\r\n", zone);
    }
  }
  if (type == 1) {
    if (rand_number(g, 1, 3) != 2) {
      act(g, "@RIt turns around at the last second and begins to pursue @r$N@R!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RIt turns around at the last second and begins to pursue YOU!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@RIt turns around at the last second and begins to pursue @r$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
      struct obj_data *obj;
      obj_vnum num = 0;

      switch (skill2) {
        case 461:
          num = 80;
          break;
        default:
          num = 80;
          break;
      }

      obj = read_object(g, num);
      if (!obj) /* a builder deleted it */
        return;
      obj_to_room(g, obj, char_get_in_room(ch));

      obj_set_target(obj, vict);
      obj_set_kicharge(obj, damtype(g, ch, type2, skill, .2));
      obj_set_kitype(obj, skill2);
      obj_set_user(obj, ch);
    } else {
      act(g, "@RIt fails to follow after @r$N@R!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RIt fails to follow after YOU!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@RIt fails to follow after @r$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
    }
  }
  if (type == 2 && (skill2 != 481 || char_is_frieza(ch))) {
    if (skill2 == 481) {
      int chance = rand_number(g, 25, 50), prob = axion_dice(g, 0);
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
    act(g, "@RYou turn it around and send it back after @r$N@R!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@W$n @Rturns it around and sends it back after YOU!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@W$n @Rturns it around and sends it back after @r$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
    struct obj_data *obj;
    obj_vnum num = 0;

    switch (skill2) {
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

    obj = read_object(g, num);
    if (!obj) /* a builder deleted it */
      return;
    obj_to_room(g, obj, char_get_in_room(ch));

    obj_set_target(obj, vict);
    obj_set_kicharge(obj, damtype(g, ch, type2, skill, .3));
    obj_set_kitype(obj, skill2);
    obj_set_user(obj, ch);
  }
}

static void damtype_unarmed_infuse(char_data *ch, int64_t *dam)
{
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

static void damtype_unarmed_hasshuken(char_data *ch, int64_t *dam)
{
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

static void damtype_unarmed_hasshuken_or_infuse(char_data *ch, int64_t *dam)
{
  if (char_aff_flagged(ch, AFF_HASS)) {
    damtype_unarmed_hasshuken(ch, dam);
  } else {
    damtype_unarmed_infuse(ch, dam);
  }
}

static void damtype_unarmed_preference(char_data *ch, int64_t *dam)
{
  if (char_get_preference(ch) == PREFERENCE_THROWING) {
    *dam -= *dam * 0.15;
  } else if (char_get_preference(ch) == PREFERENCE_H2H) {
    *dam += *dam * 0.20;
  }
}

static void damtype_focus(char_data *ch, int64_t *dam, int64_t focus, int divby)
{
  if (focus > 0) {
    *dam += focus * (*dam / divby);
  }
}

static void damtype_unarmed(char_data *ch, int skill, int64_t *dam)
{
  // General Arlian bonus.
  if (char_is_arlian(ch)) {
    *dam += *dam * 0.2;
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

  switch (skill) {
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

static void damtype_human_grandmaster(char_data *ch, int skill, int64_t *dam)
{
  if (char_is_human(ch)) {
    switch (skill) {
      case 101:
        *dam = *dam * 1.1;
        break;
      case 102:
        *dam = *dam * 1.2;
        break;
      case 103:
        *dam = *dam * 1.3;
    }
  }
}

static void damtype_human_ki(char_data *ch, int64_t *dam, int bon)
{
  if (char_is_human(ch)) {
    *dam += (*dam / 100) * bon;
  }
}

static void damtype_saiyan_ki(char_data *ch, int64_t *dam, int bon)
{
  if (char_is_saiyan(ch)) {
    *dam += (*dam / 100) * bon;
  }
}

static void damtype_kai_ki(char_data *ch, int64_t *dam, int bon)
{
  if (char_is_kai(ch)) {
    *dam += (*dam / 100) * bon;
  }
}

static void damtype_icer_ki(char_data *ch, int64_t *dam, int bon)
{
  if (char_is_icer(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4))) {
    *dam += (*dam / 100) * bon;
  }
}

/* Damage for player and NPC attacks  */
int64_t damtype(GameWorld *g, struct char_data *ch, int type, int skill, double percent)
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
    switch (type) {
      // big list of h2h stuff for starters.
      case -1:
        cou1 = 1 + ((skill / 4) * ((getCurPL(g, ch) / 1200) + char_get_aff_abils_str(ch)));
        cou2 = 1 + ((skill / 4) * ((getCurPL(g, ch) / 1000) + char_get_aff_abils_str(ch)));
        break;
      case 0: /* Punch */
        cou1 = 15 + ((skill / 4) * ((getCurPL(g, ch) / 1600) + char_get_aff_abils_str(ch)));
        cou2 = 15 + ((skill / 4) * ((getCurPL(g, ch) / 1300) + char_get_aff_abils_str(ch)));
        break;
      case 1: /* Kick */
        cou1 = 40 + ((skill / 4) * ((getCurPL(g, ch) / 1200) + char_get_aff_abils_str(ch)));
        cou2 = 40 + ((skill / 4) * ((getCurPL(g, ch) / 1000) + char_get_aff_abils_str(ch)));
        break;
      case 2: /* Elbow */
        cou1 = 100 + ((skill / 4) * ((getCurPL(g, ch) / 1300) + char_get_aff_abils_str(ch)));
        cou2 = 100 + ((skill / 4) * ((getCurPL(g, ch) / 1050) + char_get_aff_abils_str(ch)));
        break;
      case 3: /* Knee */
        cou1 = 150 + ((skill / 4) * ((getCurPL(g, ch) / 1100) + char_get_aff_abils_str(ch)));
        cou2 = 150 + ((skill / 4) * ((getCurPL(g, ch) / 1000) + char_get_aff_abils_str(ch)));
        break;
      case 4: /* Roundhouse */
        cou1 = 500 + ((skill / 4) * ((getCurPL(g, ch) / 1000) + char_get_aff_abils_str(ch)));
        cou2 = 500 + ((skill / 4) * ((getCurPL(g, ch) / 800) + char_get_aff_abils_str(ch)));
        break;
      case 5: /* Uppercut */
        cou1 = 350 + ((skill / 4) * ((getCurPL(g, ch) / 1100) + char_get_aff_abils_str(ch)));
        cou2 = 350 + ((skill / 4) * ((getCurPL(g, ch) / 900) + char_get_aff_abils_str(ch)));
        break;
      case 6: /* Slam */
        cou1 = 8000 + ((skill / 4) * ((getCurPL(g, ch) / 800) + char_get_aff_abils_str(ch)));
        cou2 = 8000 + ((skill / 4) * ((getCurPL(g, ch) / 500) + char_get_aff_abils_str(ch)));
        break;
      case 8: /* Heeldrop */
        cou1 = 12500 + ((skill / 4) * ((getCurPL(g, ch) / 700) + char_get_aff_abils_str(ch)));
        cou2 = 12500 + ((skill / 4) * ((getCurPL(g, ch) / 400) + char_get_aff_abils_str(ch)));
        break;
      case 51: /* Bash */
        cou1 = 1000 + ((skill / 4) * ((getCurPL(g, ch) / 700) + char_get_aff_abils_str(ch)));
        cou2 = 1000 + ((skill / 4) * ((getCurPL(g, ch) / 550) + char_get_aff_abils_str(ch)));
        break;
      case 52: /* Headbutt */
        cou1 = 800 + ((skill / 4) * ((getCurPL(g, ch) / 900) + char_get_aff_abils_str(ch)));
        cou2 = 800 + ((skill / 4) * ((getCurPL(g, ch) / 650) + char_get_aff_abils_str(ch)));
        break;
      case 56: /* TAILWHIP */
        cou1 = 400 + ((skill / 4) * ((getCurPL(g, ch) / 1100) + char_get_aff_abils_str(ch)));
        cou2 = 400 + ((skill / 4) * ((getCurPL(g, ch) / 1000) + char_get_aff_abils_str(ch)));
        break;
    }

    bool ki = false;
    // Set initial damage value.
    switch (type) {
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
        dam = large_rand(g, cou1, cou2);
        dam += char_get_aff_abils_str(ch) * (dam * 0.005);
        break;
      case 51:
      case 52:
      case 56: /* TAILWHIP */
        dam = large_rand(g, cou1, cou2);
        dam += char_get_level(ch) * 100;
        dam += char_get_aff_abils_str(ch) * (dam * 0.005);
        break;
      default: // all ki abilities.
        dam = getMaxKI(g, ch) * percent;
        ki  = true;
    }

    // ki type move pre-processing
    switch (type) {
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
        break;
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

    if (ki) dam *= 1.25;

    switch (type) {
      case -1:
        if (!char_plr_flagged(ch, PLR_THANDW))
          damtype_unarmed_hasshuken_or_infuse(ch, &dam);

        if (char_get_bonuses(ch, BONUS_BRAWLER) > 0) {
          dam += dam * .2;
        }
        if (char_get_preference(ch) == PREFERENCE_KI) {
          dam -= dam * 0.20;
        }
        if (char_get_preference(ch) == PREFERENCE_WEAPON && char_get_charge(ch) >= getMaxKI(g, ch) * 0.05) {
          dam += getMaxKI(g, ch) * 0.05;
          char_set_charge(ch, char_get_charge(ch) - (getMaxKI(g, ch) * 0.05));
        } else if (char_get_preference(ch) == PREFERENCE_WEAPON && char_get_charge(ch) > 0) {
          dam += char_get_charge(ch);
          char_set_charge(ch, 0);
        }
        if (group_bonus(g, ch, 2) == 8) {
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
          int64_t amount = getEffMaxPL(g, ch) * hitperc;

          decCurHealth(g, ch, amount, 1);

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
  } else {
    dam = (getCurPL(g, ch) * 0.05) + (getEffMaxPL(g, ch) * 0.025);
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

    int64_t mob_hit = getCurPL(g, ch);
    int64_t max_hit = getEffMaxPL(g, ch);
    int64_t mobperc = (mob_hit * 100) / max_hit;
    if (mobperc < 98 && mobperc >= 90) {
      dam = dam * 0.95;
    } else if (mobperc < 90 && mobperc >= 80) {
      dam = dam * 0.90;
    } else if (mobperc < 80 && mobperc >= 70) {
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
    act(g, "Your rage magnifies your attack power!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "Swirling energy flows around $n as $e releases $s rage in the attack!", true, ch, nullptr, nullptr, TO_ROOM);
    if (rand_number(g, 1, 10) >= 7) {
      send_to_char(g, ch, "You feel less angry.\r\n");
      char_plr_set(ch, PLR_FURY, false);
    }
  } else if (char_plr_flagged(ch, PLR_FURY)) {
    dam *= 2;
    act(g, "Your rage magnifies your attack power!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "Swirling energy flows around $n as $e releases $s rage in the attack!", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_FURY, false);
  }
  /* End of Fury Mode for halfbreeds */

  if ((type == -1 || type == 0 || type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6 || type == 8)) {
    if (!char_is_npc(ch))
      dam -= dam * 0.08;
    if (!char_is_npc(ch) && dam > getEffMaxPL(g, ch) * 0.1)
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

static void saiyan_gain(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  int gain = rand_number(g, char_get_level(ch) * 6, char_get_level(ch) * 8);
  int weak = false;

  if (!vict)
    return;

  if (char_is_npc(ch))
    return;

  if (getEffMaxPL(g, vict) < getEffMaxPL(g, ch) / 10) {
    weak = true;
  }

  if (char_get_level(ch) > 99) {
    gain += rand_number(g, char_get_level(ch) * 300, char_get_level(ch) * 500);
  } else if (char_get_level(ch) > 80) {
    gain += rand_number(g, char_get_level(ch) * 150, char_get_level(ch) * 200);
  } else if (char_get_level(ch) > 60) {
    gain += rand_number(g, char_get_level(ch) * 80, char_get_level(ch) * 100);
  } else if (char_get_level(ch) > 50) {
    gain += rand_number(g, char_get_level(ch) * 20, char_get_level(ch) * 25);
  } else if (char_get_level(ch) > 40) {
    gain += rand_number(g, char_get_level(ch) * 8, char_get_level(ch) * 10);
  } else if (char_get_level(ch) > 30) {
    gain += rand_number(g, char_get_level(ch) * 5, char_get_level(ch) * 8);
  } else {
  }

  if (char_is_bio(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2)) {
    gain /= 2;
  }
  if (rand_number(g, 1, 22) >= 18 && (char_get_level(ch) == 100 || level_exp(ch, char_get_level(ch) + 1) - (char_get_exp(ch)) > 0)) {
    if (weak) {
      send_to_char(g, ch, "@D[@YSaiyan @RBlood@D] @WThey are too weak to inspire your saiyan soul!@n\r\n");
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
        send_to_char(g, ch, "@D[@YSaiyan @RBlood@D] @WYou feel you have reached your current limits.@n\r\n");
        return;
      }

      int choice = available[rand_number(g, 0, avail_count - 1)];

      switch (choice) {
        case 0:
          gainBasePL(ch, gain, false);
          send_to_char(g, ch, "@D[@YSaiyan @RBlood@D] @WYou feel slightly stronger. @D[@G+%s@D]@n\r\n", add_commas(gain));
          break;
        case 1:
          gainBaseKI(ch, gain, false);
          send_to_char(g, ch, "@D[@YSaiyan @RBlood@D] @WYou feel your spirit grow. @D[@G+%s@D]@n\r\n", add_commas(gain));
          break;
        case 2:
          gainBaseST(ch, gain, false);
          send_to_char(g, ch, "@D[@YSaiyan @RBlood@D] @WYou feel slightly more vigorous. @D[@G+%s@D]@n\r\n", add_commas(gain));
          break;
      }
    }
  }
}

static void spar_gain(GameWorld *g, struct char_data *ch, struct char_data *vict, int type, int64_t dmg)
{
  int chance = 0, gmult, gravity, bonus = 1, pscost = 2, difference = 0;
  int64_t gain = 0, pl = 0, ki = 0, st = 0, gaincalc = 0;

  if (ch != nullptr && !char_is_npc(ch)) {
    if (dmg > getEffMaxPL(g, vict) / 10) {
      chance = rand_number(g, 20, 100);
    } else if (dmg <= getEffMaxPL(g, vict) / 10) {
      chance = rand_number(g, 1, 75);
    }

    if (char_get_relax_count(ch) >= 464) {
      chance = 0;
    } else if (char_get_relax_count(ch) >= 232) {
      chance -= chance * 0.5;
    } else if (char_get_relax_count(ch) >= 116) {
      chance -= chance * 0.2;
    }

    gravity = room_get_gravity(g, char_get_in_room(ch));
    gmult   = char_get_level(ch) * ((gravity / 5) + 6);

    if (char_get_equipment(ch, WEAR_SH)) {
      struct obj_data *obj = char_get_equipment(ch, WEAR_SH);
      if (obj_get_vnum(g, obj) == 1127) {
        gmult *= 4;
      }
    }

    if (room_flagged(g, char_get_in_room(ch), ROOM_WORKOUT) || (room_flagged(g, char_get_in_room(ch), ROOM_HBTC))) {
      if (char_get_in_room(ch) >= 19100 && char_get_in_room(ch) <= 19199) {
        gmult *= 1.75;
        pscost += 2;
      } else {
        gmult *= 1.25;
        pscost += 1;
      }
      pl = large_rand(g, gmult * .8, gmult * 1.2);
      ki = large_rand(g, gmult * .8, gmult * 1.2);
    } else {
      pl = large_rand(g, gmult * .4, gmult * .8);
      ki = large_rand(g, gmult * .4, gmult * .8);
    }
    if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) < 0 && char_get_level(ch) < 100) {
      pl = 0;
    }
    if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) < 0 && char_get_level(ch) < 100) {
      ki = 0;
    }

    if (chance >= rand_number(g, 60, 75)) {
      int64_t num = 0, maxnum = 500000;
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
      if (vict != nullptr && char_is_npc(vict)) {
        num      = num * 0.7;
        gaincalc = num * 1.5;
        type     = 3;
      } else if (vict != nullptr && !char_is_npc(vict)) {
        gaincalc = large_rand(g, num * .7, num);
        if (char_get_level(ch) > char_get_level(vict))
          difference = char_get_level(ch) - char_get_level(vict);
        else if (char_get_level(ch) < char_get_level(vict))
          difference = char_get_level(vict) - char_get_level(ch);
      } else {
        gaincalc = 0;
      }
      if (vict != nullptr) {
        if (difference >= 51) {
          send_to_char(g, ch, "The difference in your levels is too great for you to gain anything.\r\n");
          return;
        }
        if (difference >= 40) {
          gaincalc = gaincalc * 0.05;
        } else if (difference >= 30) {
          gaincalc = gaincalc * 0.10;
        } else if (difference >= 20) {
          gaincalc = gaincalc * 0.25;
        } else if (difference >= 10) {
          gaincalc = gaincalc * 0.50;
        }
        if (!char_is_npc(vict)) {
          if (char_prf_flagged(g, vict, PRF_INSTRUCT)) {
            if (char_get_practice_points(g, vict) > 10) {
              send_to_char(g, vict, "You instruct them in proper fighting techniques and strategies.\r\n");
              act(g, "You take $N's instruction to heart and gain more experience.\r\n", false, ch, nullptr, vict, TO_CHAR);
              char_set_practice_points(g, vict, char_get_practice_points(g, vict) - (10));
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
      if (room_flagged(g, char_get_in_room(ch), ROOM_WORKOUT) || (room_flagged(g, char_get_in_room(ch), ROOM_HBTC))) {
        if (char_get_in_room(ch) >= 19100 && char_get_in_room(ch) <= 19199) {
          gaincalc *= 1.5;
        } else {
          gaincalc *= 1.25;
        }
      }
      gain = gear_exp(g, ch, gaincalc);
      if (char_get_practice_points(g, ch) >= pscost) {
        char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (pscost));
        gain = gain * bonus;
        gain_exp(g, ch, gain);
        send_to_char(g, ch, "@D[@Y+ @G%s @mExp@D]@n ", add_commas(gain));
        if (type == 0 && rand_number(g, 1, 5) >= 4) {
          send_to_char(g, ch, "@D[@Y+ @R%s @rPL@D]@n ", pl > 0 ? add_commas(pl) : "SOFT-CAP");
          gainBasePL(ch, pl, false);
        } else if (type == 1 && rand_number(g, 1, 5) >= 4) {
          send_to_char(g, ch, "@D[@Y+ @C%s @cKi@D]@n ", ki > 0 ? add_commas(ki) : "SOFT-CAP");
          gainBaseKI(ch, ki, false);
        }
        send_to_char(g, ch, "@D[@R- @M%d @mPS@D]@n ", pscost);
        send_to_char(g, ch, "\r\n");
      } else {
        send_to_char(g, ch, "@RYou need at least %d Practice Sessions in order to gain while sparring here.@n\r\n", pscost);
      }
    }
  }

  if (vict != nullptr && !char_is_npc(vict) && !char_is_npc(ch)) {
    if (dmg > getEffMaxPL(g, vict) / 4) {
      chance = rand_number(g, 1, 100);
    } else if (dmg <= getEffMaxPL(g, vict) / 4) {
      chance = rand_number(g, 1, 70);
    }

    if (char_get_relax_count(vict) >= 464) {
      chance = 0;
    } else if (char_get_relax_count(vict) >= 232) {
      chance -= chance * 0.5;
    } else if (char_get_relax_count(vict) >= 116) {
      chance -= chance * 0.2;
    }

    gravity = room_get_gravity(g, char_get_in_room(ch));
    gmult   = char_get_level(vict) * ((gravity / 5) + 6);
    if (room_flagged(g, char_get_in_room(vict), ROOM_WORKOUT) || (room_flagged(g, char_get_in_room(ch), ROOM_HBTC))) {
      if (char_get_in_room(vict) >= 19100 && char_get_in_room(vict) <= 19199) {
        gmult *= 1.75;
      } else {
        gmult *= 1.25;
      }
      st = large_rand(g, gmult * .8, gmult * 1.2);
    } else {
      st = large_rand(g, gmult * .4, gmult * .8);
    }

    if (level_exp(vict, char_get_level(vict) + 1) - char_get_exp(vict) < 0 && char_get_level(vict) < 100) {
      st = 0;
    }

    if (chance >= rand_number(g, 60, 75)) {
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
      gain = large_rand(g, num * .7, num);

      if (difference > 50) {
        send_to_char(g, ch, "The difference in your levels is too great for you to gain anything.\r\n");
        return;
      }
      if (difference >= 40) {
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
      if (room_flagged(g, char_get_in_room(ch), ROOM_WORKOUT) || (room_flagged(g, char_get_in_room(ch), ROOM_HBTC))) {
        if (char_get_in_room(ch) >= 19100 && char_get_in_room(ch) <= 19199) {
          gain *= 1.5;
        } else {
          gain *= 1.25;
        }
      }
      if (char_get_practice_points(g, vict) >= pscost) {
        char_set_practice_points(g, vict, char_get_practice_points(g, vict) - (pscost));
        send_to_char(g, vict, "@D[@Y+ @G%s @mExp@D]@n ", add_commas(gain));
        gain = gear_exp(g, vict, gain);
        gain_exp(g, vict, gain);
        if (rand_number(g, 1, 5) >= 4) {
          send_to_char(g, vict, "@D[@Y+ @G%s @gSt@D]@n ", st > 0 ? add_commas(st) : "SOFT-CAP");
          gainBaseST(vict, st, false);
        }
        send_to_char(g, vict, "@D[@R- @M%d @mPS@D]@n ", pscost);
        send_to_char(g, vict, "\r\n");
      } else {
        send_to_char(g, vict, "@RYou need at least %d Practice Sessions in order to gain while sparring here.@n\r\n", pscost);
      }
    }
  }
}

/* Main damage function for RDBS 'Real Dragonball Battle System' */
void hurt(GameWorld *g, int limb, int chance, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int64_t dmg, int type)
{
  int64_t index   = 0;
  int64_t maindmg = dmg, beforered = dmg;
  int dead = false;

  /* If a character is trageted */

  if (type <= 0) {
    if (char_is_saiyan(ch) && char_plr_flagged(ch, PLR_STAIL)) {
      dmg += dmg * .15;
    }
    if (char_is_namek(ch) && !char_get_equipment(ch, WEAR_HEAD)) {
      dmg += dmg * .25;
    }
    if (group_bonus(g, ch, 2) == 4) {
      dmg += dmg * .1;
    } else if (group_bonus(g, ch, 2) == 12) {
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
    if (group_bonus(g, ch, 2) == 9) {
      dmg -= dmg * 0.1;
    }
    if (char_aff_flagged(ch, AFF_POTENT)) {
      dmg += dmg * 0.3;
      send_to_room(g, char_get_in_room(ch), "@wThere is a bright flash of @Yyellow@w light in the wake of the attack!@n\r\n");
    }
  }

  if (char_aff_flagged(ch, AFF_INFUSE) && !char_aff_flagged(ch, AFF_HASS) && type <= 0) {
    int64_t infuse_cost = getPercentOfMaxKI(g, ch, .005);
    if (dmg > 0) {
      if (getCurKI(g, ch) - infuse_cost) {
        decCurKI(g, ch, infuse_cost, 0);
        send_to_room(g, char_get_in_room(ch), "@CA swirl of ki explodes from the attack!@n\r\n");
      } else {
        act(g, "@wYou can no longer infuse ki into your attacks!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@c$n@w can no longer infuse ki into $s attacks!@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_aff_set(ch, AFF_INFUSE, false);
      }
    }
  }

  if (vict) {

    if (char_get_in_room(vict) == 17875) {
      return;
    }

    reveal_hiding(g, vict, 0);
    if (char_aff_flagged(vict, AFF_PARALYZE)) {
      send_to_char(g, ch, "They are a statue and can't be harmed\r\n");
      return;
    }

    if (char_get_kaioken(ch) > 0) {
      dmg += (dmg / 100) * (char_get_kaioken(ch) * 2);
    }

    if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 8 || char_get_genome(vict, 1) == 8) && type == 0) {
      int64_t drain = dmg * 0.1;
      dmg -= drain;
      decCurST(g, ch, drain, 1);
      act(g, "@Y$N's rubbery body makes hitting it tiring!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@Y$n's stamina is sapped a bit by hitting your rubbery body!@n", true, ch, nullptr, vict, TO_VICT);
    }

    if (!char_is_npc(ch)) {
      if (char_plr_flagged(ch, PLR_OOZARU)) {
        dmg += dmg * 0.30;
      }
    }
    if (!char_is_npc(vict)) {
      if (char_plr_flagged(vict, PLR_OOZARU)) {
        dmg -= dmg * 0.30;
      }
    }

    if (type > -1) {
      if (char_get_lastattack(ch) != 11 && char_get_lastattack(ch) != 39 && char_get_lastattack(ch) != 500 && char_get_lastattack(ch) < 1000) {
        if (handle_combo(g, ch, vict) > 0) {
          if (beforered <= 1) {
            char_set_combo(ch, -1);
            char_set_combhits(ch, 0);
            send_to_char(g, ch, "@RYou have cut your combo short because you missed your last hit!@n\r\n");
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
              gain_exp(g, ch, gain);
              send_to_char(g, ch, "@D[@mExp@W: @G%s@D]@n\r\n", add_commas(gain));
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
              gain_exp(g, ch, gain);
              send_to_char(g, ch, "@D[@mExp@W: @G%s@D]@n\r\n", add_commas(gain));
            }
            char_set_combo(ch, -1);
            char_set_combhits(ch, 0);
          }
        }
      } else if (char_get_combhits(ch) > 0 && char_get_lastattack(ch) < 1000) {
        send_to_char(g, ch, "@RYou have cut your combo short because you used the wrong attack!@n\r\n");
        char_set_combo(ch, -1);
        char_set_combhits(ch, 0);
      }
    }

    if (char_get_lastattack(ch) >= 1000) {
      char_set_lastattack(ch, char_get_lastattack(ch) - (1000));
    }

    if (char_get_preference(vict) == PREFERENCE_KI && char_get_charge(vict) > 0) {
      dmg -= dmg * 0.08;
    }

    if (char_aff_flagged(vict, AFF_SANCTUARY)) {
      if (char_get_skill(vict, SKILL_AQUA_BARRIER)) {
        if (!room_sunken(g, char_get_in_room(ch))) {
          dmg = dmg * 0.85;
        } else {
          dmg = dmg * 0.75;
        }
      }
      if (char_get_barrier(vict) - dmg > 0) {
        act(g, "@c$N's@C barrier absorbs the damage!@n", true, ch, nullptr, vict, TO_CHAR);
        char barr[MAX_INPUT_LENGTH];
        sprintf(barr, "@CYour barrier absorbs the damage! @D[@B%s@D]@n", add_commas(dmg));
        act(g, barr, true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N's@C barrier absorbs the damage!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_set_barrier(vict, char_get_barrier(vict) - (dmg));
        dmg = 0;
      } else if (char_get_barrier(vict) - dmg <= 0) {
        dmg -= char_get_barrier(vict);
        char_set_barrier(vict, 0);
        act(g, "@c$N's@C barrier bursts!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@CYour barrier bursts!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N's@C barrier bursts!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_SANCTUARY, false);
      }
    }
    if (char_aff_flagged(vict, AFF_FIRESHIELD) && rand_number(g, 1, 200) < char_get_skill(vict, SKILL_FIRESHIELD)) {
      act(g, "@c$N's@C fireshield repels the damage!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@CYour fireshield repels the damage!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$N's@C fireshield repels the damage!@n", true, ch, nullptr, vict, TO_NOTVICT);
      if (rand_number(g, 1, 3) == 3) {
        act(g, "@c$N's@C fireshield disappears...@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@CYour fireshield disappears...@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N's@C fireshield disappears...@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_FIRESHIELD, false);
      }
      dmg = 0;
    }

    int64_t conlimit = 2000000000;

    if (type == 0) {
      int64_t vpl = getEffMaxPL(g, vict);
      int vcon    = char_get_aff_abils_con(vict);

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
      } else if (char_get_level(vict) < 30) {
        index += char_get_level(vict) * 500;
      } else if (char_get_level(vict) < 50) {
        index += char_get_level(vict) * 1000;
      } else if (char_get_level(vict) < 60) {
        index += char_get_level(vict) * 2000;
      } else if (char_get_level(vict) < 70) {
        index += char_get_level(vict) * 5000;
      } else if (char_get_level(vict) < 90) {
        index += char_get_level(vict) * 10000;
      } else if (char_get_level(vict) <= 100) {
        index += char_get_level(vict) * 25000;
      }
    }

    if (char_aff_flagged(vict, AFF_SHELL)) {
      dmg = dmg * 0.75;
    }

    if (is_affected(vict, AFF_WITHER)) {
      dmg += (dmg * 0.01) * 20;
    }

    if (!char_is_npc(vict) && char_get_conditions(g, vict, DRUNK) > 4) {
      dmg -= (dmg * 0.001) * char_get_conditions(g, vict, DRUNK);
    }

    if (char_aff_flagged(vict, AFF_EARMOR)) {
      dmg -= dmg * 0.1;
    }

    if (type > 0) {
      advanced_energy(g, vict, dmg);
      dmg -= (dmg * 0.0005) * char_get_aff_abils_wis(vict);
    }

    // Leech bonus trait reduces damage by 2% per 5 levels, up to 40% at level 100
    if (char_get_bonuses(vict, BONUS_LEECH)) {
      if (type > 0) {
        dmg -= dmg * (((char_get_level(vict) / 5) * 0.02));
      }
    }
    // Fireproof bonus trait reduces damage by 10% from ki attacks
    if (char_get_bonuses(vict, BONUS_FIREPROOF)) {
      if (type > 0) {
        dmg -= dmg * 0.1;
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

    if (char_is_mutant(vict)) {
      if (type <= 0) {
        dmg -= dmg * 0.3;
      } else {
        dmg -= dmg * 0.25;
      }
    }

    if (char_is_majin(vict)) {
      if (type <= 0) {
        dmg -= dmg * 0.5;
      }
    }

    if (char_is_kai(vict)) {
      dmg += dmg * 0.15;
    }

    if (char_get_grappling(g, ch) == vict && char_get_grap(ch) == 3) {
      dmg += (dmg / 100) * 20;
    }

    if (!char_is_npc(vict) && char_get_skill(vict, SKILL_ARMOR)) {
      int nanite = char_get_skill(vict, SKILL_ARMOR), perc = rand_number(g, 1, 220);
      if (char_plr_flagged(vict, PLR_SENSEM)) {
        perc = rand_number(g, 1, 176);
      }
      if (nanite >= perc) {
        if (char_plr_flagged(vict, PLR_TRANS6)) {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block MOST of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block MOST of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 50;
        } else if (char_plr_flagged(vict, PLR_TRANS5)) {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 40;
        } else if (char_plr_flagged(vict, PLR_TRANS4)) {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a lot of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a lot of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 30;
        } else if (char_plr_flagged(vict, PLR_TRANS3)) {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a good deal of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a good deal of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 25;
        } else if (char_plr_flagged(vict, PLR_TRANS2)) {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block some of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 20;
        } else if (char_plr_flagged(vict, PLR_TRANS1)) {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a bit of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a bit of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 10;
        } else {
          act(g, "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a tiny bit of the damage!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block a tiny bit of the damage!@n", true, vict, nullptr, nullptr, TO_ROOM);
          dmg -= (dmg * 0.01) * 5;
        }
      }
    }

    if (!char_aff_flagged(vict, AFF_KNOCKED) && (char_get_position(vict) == POS_SITTING || char_get_position(vict) == POS_RESTING) && char_get_skill(vict, SKILL_ROLL) > axion_dice(g, 0)) {
      int64_t rollcost = getMaxHealth(g, vict) / 300 * (char_get_aff_abils_str(ch) / 2);
      if ((getCurST(g, vict)) >= rollcost) {
        act(g, "@GYou roll to your feet in an agile fashion!@n", true, vict, nullptr, nullptr, TO_CHAR);
        act(g, "@G$n rolls to $s feet in an agile fashion!@n", true, vict, nullptr, nullptr, TO_ROOM);
        do_stand(g, vict, "", 0, 0);
        decCurST(g, vict, rollcost, 0);
      }
    }

    if (char_is_npc(vict)) {
      hitprcnt_mtrigger(g, vict);
    }

    if (char_is_humanoid(vict) && !char_is_npc(ch) && char_is_npc(vict) && (!is_sparring(ch) || !is_sparring(vict))) {
      remember(g, vict, ch);
    }
    if (char_is_npc(vict) && getCurPL(g, vict) > ((getEffMaxPL(g, vict))) / 4) {
      char_set_lasthit(vict, char_get_idnum(ch));
    }
    if (char_aff_flagged(vict, AFF_SLEEP) && rand_number(g, 1, 2) == 2) {
      affect_from_char(g, vict, SPELL_SLEEP);
      act(g, "@c$N@W seems to be more aware now.@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@WYou are no longer so sleepy.@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$N@W seems to be more aware now.@n", true, ch, nullptr, vict, TO_NOTVICT);
    }
    if (char_aff_flagged(vict, AFF_KNOCKED) && rand_number(g, 1, 12) >= 11) {
      cureStatusKnockedOut(g, vict, true);
      if (char_is_npc(vict) && rand_number(g, 1, 20) >= 12) {
        act(g, "@W$n@W stands up.@n", false, vict, nullptr, nullptr, TO_ROOM);
        char_set_position(vict, POS_STANDING);
      }
    }
    if (char_is_npc(ch)) {
      if (char_get_level(ch) > 10) {
        if (dmg - index > 0) {
          dmg -= index;
        } else if (dmg - index <= 0 && dmg >= 1) {
          dmg = 1;
        }
      } else if (char_get_level(ch) <= 10) {
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
      hurt_limb(g, ch, vict, chance, limb, dmg);
    }
    if (char_is_npc(vict) && dmg > getMaxHealth(g, vict) * .7 && char_get_bonuses(ch, BONUS_SADISTIC) > 0) {
      char_set_exp(vict, char_get_exp(vict) / (2));
    } else if (char_is_npc(vict) && dmg > getCurHealth(g, vict) && isFullHealth(vict) && char_get_bonuses(ch, BONUS_SADISTIC) > 0) {
      char_set_exp(vict, char_get_exp(vict) / (2));
    }

    if (char_get_carrying(g, vict) && dmg > (((getEffMaxPL(g, vict))) * 0.01) && rand_number(g, 1, 10) >= 8) {
      carry_drop(g, vict, 2);
    }

    if (char_get_position(vict) == POS_SITTING && char_is_npc(vict) && getCurHealth(g, vict) >= ((getEffMaxPL(g, vict))) * .98) {
      do_stand(g, vict, "", 0, 0);
    }
    int suppresso = false;
    if (is_sparring(ch) && is_sparring(vict) && (char_get_suppressed(vict) + getCurHealth(g, vict)) - dmg <= 0) {
      if (!char_is_npc(vict)) {
        act(g, "@c$N@w falls down unconscious, and you stop sparring with $M.@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@w stops sparring with you as you fall unconscious.@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N@w falls down unconscious, and @C$n@w stops sparring with $M.@n", true, ch, nullptr, vict, TO_NOTVICT);
        setCurHealth(vict, 1);
        if (char_get_suppressed(vict) > 0) {
          char_set_suppressed(vict, 0);
          char_set_suppression(vict, 0);
        }
        if (char_get_fighting(g, vict)) {
          stop_fighting(g, vict);
        }
        if (char_get_fighting(g, ch)) {
          stop_fighting(g, ch);
        }
        char_set_position(vict, POS_SLEEPING);
        if (!char_is_npc(ch)) {
          char_aff_set(vict, AFF_KNOCKED, true);
        }
      } else {
        act(g, "@c$N@w admits defeat to you, stops sparring, and stumbles away.@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$N@w admits defeat to $n, stops sparring, and stumbles away.@n", true, ch, nullptr, vict, TO_NOTVICT);
        solo_gain(g, ch, vict);
        int founded = 0;
        for (obj_data *rew : char_inventory(g, vict)) {
          if (rew) {
            obj_from_char(g, rew);
            obj_to_room(g, rew, char_get_in_room(vict));
            founded = 1;
          }
        }
        if (founded == 1) {
          act(g, "@c$N@w leaves a reward behind out of respect.@n", true, ch, nullptr, vict, TO_CHAR);
        }
        setCurHealth(vict, 0);
        extract_char(g, vict);
        return;
      }
    } else if (is_sparring(ch) && (char_get_suppressed(vict) + getCurPL(g, vict)) - dmg <= 0) {
      act(g, "@c$N@w falls down unconscious, and you spare $S life.@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@w spares your life as you fall unconscious.@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$N@w falls down unconscious, and @C$n@w spares $S life.@n", true, ch, nullptr, vict, TO_NOTVICT);
      setCurHealth(vict, 1);
      if (char_get_fighting(g, vict)) {
        stop_fighting(g, vict);
      }
      if (char_get_fighting(g, ch)) {
        stop_fighting(g, ch);
      }
      char_set_position(vict, POS_SLEEPING);
      if (!char_is_npc(ch)) {
        char_aff_set(vict, AFF_KNOCKED, true);
      }
    } else if (is_sparring(ch) && !is_sparring(vict) && char_is_npc(ch)) {
      act(g, "@w$n@w stops sparring!@n", true, ch, nullptr, vict, TO_ROOM);
      char_mob_set(ch, MOB_SPAR, false);
    } else if (!is_sparring(ch) && is_sparring(vict) && char_is_npc(vict)) {
      act(g, "@w$n@w stops sparring!@n", true, ch, nullptr, vict, TO_ROOM);
      char_mob_set(vict, MOB_SPAR, false);
    }
    if (char_get_suppressed(vict) > 0 && char_get_suppression(vict) > 0) {
      if (char_get_suppressed(vict) > dmg) {
        char_set_suppressed(vict, char_get_suppressed(vict) - (dmg));
        suppresso = true;
      } else if (char_get_suppressed(vict) <= dmg) {
        send_to_char(g, vict, "@GYou no longer have any reserve powerlevel suppressed.@n\r\n");
        dmg -= char_get_suppressed(vict);
        char_set_suppressed(vict, 0);
        char_set_suppression(vict, 0);
      }
    }

    if (char_plr_flagged(vict, PLR_IMMORTAL) && !is_sparring(ch) && getCurHealth(g, vict) - dmg <= 0) {
      if (char_in_arena(g, vict)) {
        send_to_all(g, "@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n", char_get_name(ch), char_get_name(vict));
        char_from_room(g, ch);
        char_to_room(g, ch, 17875);
        look_at_room(g, char_get_in_room(ch), ch, 0);
        char_from_room(g, vict);
        char_to_room(g, vict, 17875);
        setCurHealth(vict, 1);
        look_at_room(g, char_get_in_room(vict), vict, 0);
        if (char_get_fighting(g, vict)) {
          stop_fighting(g, vict);
        }
        if (char_get_fighting(g, ch)) {
          stop_fighting(g, ch);
        }
        return;
      }
      act(g, "@c$N@w disappears right before dying. $N appears to be immortal.@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@CYou disappear right before death, having been saved by your immortality.@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$N@w disappears right before dying. $N appears to be immortal.@n.", true, ch, nullptr, vict, TO_NOTVICT);
      decCurHealthPercent(g, vict, 1, 1);
      decCurSTPercent(g, vict, 1, 1);
      decCurKIPercent(g, vict, 1, 1);

      if (char_get_fighting(g, vict)) {
        stop_fighting(g, vict);
      }
      if (char_get_fighting(g, ch)) {
        stop_fighting(g, ch);
      }
      char_set_position(vict, POS_SITTING);
      char_from_room(g, vict);
      char_to_room(g, vict, sensei_start_room(char_get_class(vict)));
      return;
    }

    if (char_get_grappling(g, vict) && char_get_grappling(g, vict) != ch && type == 1) {
      act(g, "@YThe attack hurts YOU as well because you are grappling with $M!@n", true, vict, nullptr, char_get_grappling(g, vict), TO_VICT);
      act(g, "@YThe attack hurts @y$N@Y as well because $n is grappling with $m!@n", true, vict, nullptr, char_get_grappling(g, vict), TO_NOTVICT);
      maindmg = maindmg / 2;
      hurt(g, 0, 0, ch, char_get_grappling(g, vict), nullptr, maindmg, 3);
    }
    if (char_get_grappled(g, vict) && char_get_grappled(g, vict) != ch && type == 1) {
      act(g, "@YThe attack hurts YOU as well because you are being grappled by $M!@n", true, vict, nullptr, char_get_grappled(g, vict), TO_VICT);
      act(g, "@YThe attack hurts @y$N@Y as well because $n is being grappled by $m!@n", true, vict, nullptr, char_get_grappled(g, vict), TO_NOTVICT);
      maindmg = maindmg / 2;
      hurt(g, 0, 0, ch, char_get_grappled(g, vict), nullptr, maindmg, 3);
    }
    if (!is_sparring(ch) && !char_plr_flagged(vict, PLR_IMMORTAL) && getCurPL(g, vict) - dmg <= 0) {

      if (getCurPL(g, vict) - dmg <= 0 && suppresso == false) {
        decCurHealthPercent(g, vict, 1, 0);
        if (!char_is_npc(vict) && char_get_lifeperc(vict) > 0 && (getCurLF(g, vict)) - (dmg - getCurPL(g, vict)) >= 0) {
          act(g, "@c$N@w barely clings to life!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@CYou barely cling to life!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@c$N@w barely clings to life!@n.", true, ch, nullptr, vict, TO_NOTVICT);
          int64_t lifeloss = dmg - getCurPL(g, vict);
          decCurLF(g, vict, lifeloss, 0);
          send_to_char(g, vict, "@D[@CLifeforce@D: @R-%s@D]\n", add_commas(lifeloss));
          if ((getCurLF(g, vict)) >= (getMaxLF(g, vict)) * 0.05) {
            send_to_char(g, vict, "@YYou recover a bit thanks to your strong life force.@n\r\n");
            incCurHealth(g, vict, (getMaxLF(g, vict)) * .05, true);
            decCurLFPercent(g, vict, .05, 0);
          } else {
            incCurHealth(g, vict, char_get_level(vict) * 100, true);
          }
          return;
        }
        if (char_get_death_type(vict) != DTYPE_HEAD) {
          char_set_death_type(vict, 0);
        }
        if (type <= 0 && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          handle_death_msg(g, ch, vict, 0);
        } else if (type > 0 && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          handle_death_msg(g, ch, vict, 1);
        } else {
          act(g, "@R$N@w self destructs with a mild explosion!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@R$N@w self destructs with a mild explosion!@n", true, ch, nullptr, vict, TO_ROOM);
        }
        if (dmg > 1) {
          if (type <= 0 && getCurPL(g, ch) >= getEffMaxPL(g, ch) * 0.5) {
            int64_t raise = (getMaxKI(g, ch) * 0.005) + 1;
            incCurKI(g, ch, raise, true);
          }
          send_to_char(g, ch, "@D[@GDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
          send_to_char(g, vict, "@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
          int64_t healhp = (long double)(getEffMaxPL(g, vict)) * 0.12;
          if (char_aff_flagged(ch, AFF_METAMORPH) && getCurPL(g, ch) <= getEffMaxPL(g, ch)) {
            act(g, "@RYour dark aura saps some of @r$N's@R life energy!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@r$n@R's dark aura saps some of your life energy!@n", true, ch, nullptr, vict, TO_VICT);
            incCurHealth(g, ch, healhp, true);
          }
          if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 10 || char_get_genome(ch, 1) == 10)) {
            incCurKI(g, ch, dmg * .05, true);
          }
          if (!is_sparring(ch) && char_is_npc(vict)) {
            if (type == 0 && rand_number(g, 1, 100) >= 97) {
              send_to_char(g, ch, "@YY@yo@Yu @yg@Ya@yi@Yn@y s@Yo@ym@Ye @yb@Yo@yn@Yu@ys @Ye@yx@Yp@ye@Yr@yi@Ye@yn@Yc@ye@Y!@n\r\n");
              int64_t gain = char_get_exp(vict) * 0.05;
              gain += 1;
              gain_exp(g, ch, gain);
            } else if (type != 0 && rand_number(g, 1, 100) >= 93) {
              int64_t gain = char_get_exp(vict) * 0.05;
              gain += 1;
              gain_exp(g, ch, gain);
            }
          }
          if (is_affected(vict, AFF_ECHAINS) && type == 0) {
            act(g, "@CEthereal chains burn into existence! They quickly latch onto @RYOUR@C body and begin temporarily hampering $s actions!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@CEthereal chains burn into existence! They quickly latch onto @c$n's@C body and begin temporarily hampering $s actions!@n", true, ch, nullptr, vict, TO_ROOM);
            if (char_is_npc(ch))
              assign_affect(g, vict, AFF_ECHAINS, 0, -1, 0, 0, 0, 0, 0, -2);
            else
              char_set_wait(ch, PULSE_3SEC);
          }
        } else if (dmg <= 1) {
          send_to_char(g, ch, "@D[@GDamage@W: @BPitiful...@D]@n\r\n");
          send_to_char(g, vict, "@D[@rDamage@W: @BPitiful...@D]@n\r\n");
        }

        decCurHealthPercent(g, vict, 1, 0);

        if (char_aff_flagged(ch, AFF_GROUP)) {
          group_gain(g, ch, vict);
        } else {
          solo_gain(g, ch, vict);
        }
        if (char_is_demon(ch) && type == 1) {
          char_aff_set(vict, AFF_ASHED, true);
        }
        die(g, vict, ch);
        dead = true;
      }
    } else if (getCurPL(g, vict) - dmg > 0 || suppresso == true) {
      if (suppresso == false) {
        decCurHealth(g, vict, dmg, 0);
      }
      if (char_get_fighting(g, ch) == nullptr)
        set_fighting(g, ch, vict);
      if (char_get_fighting(g, vict) == nullptr)
        set_fighting(g, vict, ch);
      if (dmg > 1 && suppresso == false) {
        if (type == 0 && getCurPL(g, ch) >= getEffMaxPL(g, ch) * 0.5) {
          int64_t raise = (getMaxKI(g, ch) * 0.005) + 1;
          incCurKI(g, ch, raise, true);
        }
        if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 10 || char_get_genome(ch, 1) == 10)) {
          incCurKI(g, ch, dmg * .05, true);
        }
        send_to_char(g, ch, "@D[@GDamage@W: @R%s@D]@n", add_commas(dmg));
        send_to_char(g, vict, "@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
        //int64_t healhp = GET_HIT(vict) * 0.12;
        if (char_get_equipment(ch, WEAR_EYE) && vict && !char_prf_flagged(g, ch, PRF_NODEC)) {
          if (char_is_android(vict)) {
            send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
          } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && getCurPL(g, vict) >= 150000) {
            send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
          } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && getCurPL(g, vict) >= 5000000) {
            send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
          } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && getCurPL(g, vict) >= 15000000) {
            send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
          } else {
            send_to_char(g, ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(getCurPL(g, vict)));
          }
        } else {
          send_to_char(g, ch, "\r\n");
        }
      } else if (!char_is_npc(ch)) {
        if (dmg <= 1 && suppresso == false && !char_prf_flagged(g, ch, PRF_NODEC)) {
          send_to_char(g, ch, "@D[@GDamage@W: @BPitiful...@D]@n");
          send_to_char(g, vict, "@D[@rDamage@W: @BPitiful...@D]@n\r\n");
          if (char_get_equipment(ch, WEAR_EYE) && vict && !char_prf_flagged(g, ch, PRF_NODEC)) {
            if (char_is_android(vict)) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && getCurPL(g, vict) >= 150000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && getCurPL(g, vict) >= 5000000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && getCurPL(g, vict) >= 15000000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else {
              send_to_char(g, ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(getCurPL(g, vict)));
            }
          } else {
            send_to_char(g, ch, "\r\n");
          }
        } else if (dmg > 1 && suppresso == true && !char_prf_flagged(g, ch, PRF_NODEC)) {
          send_to_char(g, ch, "@D[@GDamage@W: @R%s@D]@n", add_commas(dmg));
          send_to_char(g, vict, "@D[@rDamage@W: @R%s @c-Suppression-@D]@n\r\n", add_commas(dmg));
          send_to_char(g, vict, "@D[Suppression@W: @G%s@D]@n\r\n", add_commas(char_get_suppressed(vict)));
          //int64_t healhp = GET_HIT(vict) * 0.12;
          if (char_get_equipment(ch, WEAR_EYE) && vict && !char_prf_flagged(g, ch, PRF_NODEC)) {
            if (char_is_android(vict)) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && getCurPL(g, vict) >= 150000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && getCurPL(g, vict) >= 5000000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && getCurPL(g, vict) >= 15000000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else {
              send_to_char(g, ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(getCurPL(g, vict)));
            }
          } else {
            send_to_char(g, ch, "\r\n");
          }
        } else if (dmg <= 1 && suppresso == true && !char_prf_flagged(g, ch, PRF_NODEC)) {
          send_to_char(g, ch, "@D[@GDamage@W: @BPitiful...@D]@n");
          send_to_char(g, vict, "@D[@rDamage@W: @BPitiful... @c-Suppression-@D]@n\r\n");
          send_to_char(g, vict, "@D[Suppression@W: @G%s@D]@n\r\n", add_commas(char_get_suppressed(vict)));
          if (char_get_equipment(ch, WEAR_EYE) && vict) {
            if (char_is_android(vict) && !char_prf_flagged(g, ch, PRF_NODEC)) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_BSCOUTER) && getCurPL(g, vict) >= 150000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_MSCOUTER) && getCurPL(g, vict) >= 5000000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else if (obj_extra_flagged(char_get_equipment(ch, WEAR_EYE), ITEM_ASCOUTER) && getCurPL(g, vict) >= 15000000) {
              send_to_char(g, ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
            } else {
              send_to_char(g, ch, " @D<@YProcessing@D: @c%s@D>@n\r\n", add_commas(getCurPL(g, vict)));
            }
          } else {
            send_to_char(g, ch, "\r\n");
          }
        }
      }
    }
    if (char_get_skill(ch, SKILL_FOCUS) && type == 1) {
      improve_skill(g, ch, SKILL_FOCUS, 1);
    }

    /* Increases GET_FURY for halfbreeds who get damaged. */

    if (!is_sparring(ch) && char_is_halfbreed(vict) && char_get_fury(vict) < 100 && !char_plr_flagged(vict, PLR_FURY)) {
      send_to_char(g, vict, "@RYour fury increases a little bit!@n\r\n");
      char_set_fury(vict, char_get_fury(vict) + (1));
    }

    /* Ends GET_FURY increase for halfbreeds who got damaged */

    if (is_sparring(ch) && is_sparring(vict) && char_get_lastattack(ch) != -1) {
      spar_gain(g, ch, vict, type, dmg);
    }
    if ((char_is_saiyan(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2))) && !char_is_npc(ch) && ((is_sparring(ch) && is_sparring(vict)) || (!is_sparring(ch) && !is_sparring(vict)))) {
      if (char_get_position(ch) != POS_RESTING && char_get_position(vict) != POS_RESTING && dmg > 1) {
        saiyan_gain(g, ch, vict);
      }
    }
    if (char_is_arlian(vict) && dead != true && !is_sparring(vict) && !is_sparring(ch)) {
      handle_evolution(g, vict, dmg);
    }
    if (dead == true) {
      char corp[256];
      if (!char_plr_flagged(ch, PLR_SELFD2)) {
        if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
          sprintf(corp, "all.zenni corpse");
          do_get(g, ch, corp, 0, 0);
        }
        if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
          sprintf(corp, "all corpse");
          do_get(g, ch, corp, 0, 0);
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
      act(g, "$p@w seems unaffected.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "$p@w seems unaffected.@n", true, ch, obj, nullptr, TO_ROOM);
    } else if (obj_get_vnum(g, obj) == 79) {
      if (obj_get_weight(obj) - dmg > 0) {
        if (type <= 0) {
          if (char_aff_flagged(ch, AFF_INFUSE))
            dmg *= 10;
          act(g, "$p@w cracks some.@n", true, ch, obj, nullptr, TO_CHAR);
          act(g, "$p@w cracks some.@n", true, ch, obj, nullptr, TO_ROOM);
          obj_set_weight(obj, obj_get_weight(obj) - (dmg));
          if (obj_get_fellow_wall(g, obj)) {
            struct obj_data *wall;
            wall = obj_get_fellow_wall(g, obj);
            obj_set_weight(wall, obj_get_weight(wall) - (dmg));
            act(g, "$p@w cracks some. A humanoid shadow can be seen moving on the other side.@n", true, nullptr, obj, nullptr, TO_ROOM);
          }
        } else {
          dmg *= 30;
          act(g, "$p@w melts some.@n", true, ch, obj, nullptr, TO_CHAR);
          act(g, "$p@w melts some.@n", true, ch, obj, nullptr, TO_ROOM);
          obj_set_weight(obj, obj_get_weight(obj) - (dmg));
          if (obj_get_fellow_wall(g, obj)) {
            struct obj_data *wall;
            wall = obj_get_fellow_wall(g, obj);
            obj_set_weight(wall, obj_get_weight(wall) - (dmg));
            act(g, "$p@w melts some.@n", true, ch, obj, nullptr, TO_ROOM);
          }
        }
      } else {
        if (type <= 0) {
          act(g, "$p@w breaks completely apart and then melts away.@n", true, ch, obj, nullptr, TO_CHAR);
          act(g, "$p@w breaks completely apart and then melts away.@n", true, ch, obj, nullptr, TO_ROOM);
          extract_obj(g, obj);
        } else {
          act(g, "$p@w is blown away into snow and water!@n", true, ch, obj, nullptr, TO_CHAR);
          act(g, "$p@w is blown away into snow and water!@n", true, ch, obj, nullptr, TO_ROOM);
          extract_obj(g, obj);
        }
      }
    } else if (obj_get_value(obj, VAL_ALL_HEALTH) - dmg > 0) {
      act(g, "$p@w cracks some.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "$p@w cracks some.@n", true, ch, obj, nullptr, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (dmg));
    } else {
      if (type <= 0) {
        act(g, "$p@w shatters apart!@n", true, ch, obj, nullptr, TO_CHAR);
        act(g, "$p@w shatters apart!@n", true, ch, obj, nullptr, TO_ROOM);
        obj_set_value(obj, VAL_ALL_HEALTH, 0);
        obj_extra_set(obj, ITEM_BROKEN, true);
        if (obj_get_type_flag(obj) == ITEM_DRINKCON && obj_get_type_flag(obj) == ITEM_FOUNTAIN) {
          obj_set_value(obj, VAL_DRINKCON_HOWFULL, 0);
        }
      } else if (type != 0) {
        act(g, "$p@w is disintegrated!@n", true, ch, obj, nullptr, TO_CHAR);
        act(g, "$p@w is disintegrated!@n", true, ch, obj, nullptr, TO_ROOM);
        extract_obj(g, obj);
      }
    }
  } else {
    mud_log("Log: Error with hurt.\n");
  }
}

/* This handles the length of time between attacks and other actions *
* players AND non-players will have to endure. Allowing for a more   *
* balanced and interesting attack cooldown system. - Iovan 2/25/2011 */
void handle_cooldown(GameWorld *g, struct char_data *ch, int cooldown)
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

  reveal_hiding(g, ch, 0);
  int waitCalc = 10, base = cooldown;
  int64_t cspd = 0;

  /* Ok calculating speed. */
  cspd = char_get_speedi(g, ch);

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
      char_set_wait(ch, PULSE_CD12);
    } else if (cooldown >= 110) {
      char_set_wait(ch, PULSE_CD11);
    } else if (cooldown >= 100) {
      char_set_wait(ch, PULSE_CD10);
    } else if (cooldown >= 90) {
      char_set_wait(ch, PULSE_CD9);
    } else if (cooldown >= 80) {
      char_set_wait(ch, PULSE_CD8);
    } else if (cooldown >= 70) {
      char_set_wait(ch, PULSE_CD7);
    } else if (cooldown >= 60) {
      char_set_wait(ch, PULSE_CD6);
    } else if (cooldown >= 50) {
      char_set_wait(ch, PULSE_CD5);
    } else if (cooldown >= 40) {
      char_set_wait(ch, PULSE_CD4);
    } else if (cooldown >= 30) {
      char_set_wait(ch, PULSE_CD3);
    } else if (cooldown >= 20) {
      char_set_wait(ch, PULSE_CD2);
    } else {
      char_set_wait(ch, PULSE_CD1);
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
int handle_parry(GameWorld *g, struct char_data *ch)
{

  if (axion_dice(g, 0) <= 4) { /* Critical failure */
    return (1);
  }

  if (char_is_npc(ch)) {

    /*  Non-humanoids are only rarely capable of parrying against weak players. Never
   * against strong players. Humanoids get progressively better at parry the higher
   * level they are.
   **/
    if (!char_is_humanoid(ch)) {
      return (rand_number(g, 0, 5));
    }
    if (char_get_level(ch) >= 110) {
      return (rand_number(g, 90, 105));
    }
    if (char_get_level(ch) >= 100) {
      return (rand_number(g, 85, 95));
    }
    if (char_get_level(ch) >= 80) {
      return (rand_number(g, 70, 85));
    }
    if (char_get_level(ch) >= 40) {
      return (rand_number(g, 50, 70));
    }
    int top = char_get_level(ch);

    if (top < 15)
      top = 16;
    return (rand_number(g, 15, top));
  }

  if (char_prf_flagged(g, ch, PRF_NOPARRY)) {
    return (-2);
  }
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

/* This handles whether a step of the combo was preformed. */
static int handle_combo(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  int success = false, pass = false;

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
      pass = true;
      break;
    default:
      if (char_get_combo(ch) != -1) {
        send_to_char(g, ch, "@RYou have cut your combo short with the wrong attack!@n\r\n");
      }
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pass = false;
      break;
  }

  if (pass == false)
    return 0;

  if (count_physical(ch) < 3)
    return 0;

  int chance;
  int64_t chspeed         = char_get_speedi(g, ch);
  int64_t victspeed       = char_get_speedi(g, vict);
  int64_t speedPercentage = ((double)(chspeed) / (double)(victspeed)) * 100.0;
  if (speedPercentage < 1) {
    chance = 1;
  } else if (speedPercentage > 100) {
    chance = 100;
  } else {
    chance = speedPercentage;
  }
  chance = 100 - chance;
  if (chance < 1) {
    chance = 1;
  }
  chance += 25;
  if (char_get_lastattack(ch) == 0 || char_get_lastattack(ch) == 1) {
    chance -= 10;
  }
  if (char_get_lastattack(ch) == 2 || char_get_lastattack(ch) == 3) {
    chance -= 5;
  }

  if (char_get_combo(ch) <= -1 && rand_number(g, 1, 100) > chance) {
    while (success == false) {
      switch (rand_number(g, 1, 24)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
          if (char_get_skill(ch, SKILL_PUNCH) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try a@R punch @Gnext!@n\r\n");
            char_set_combo(ch, 0);
            success = true;
          }
          break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
          if (char_get_skill(ch, SKILL_KICK) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try a@R kick @Gnext!@n\r\n");
            char_set_combo(ch, 1);
            success = true;
          }
          break;
        case 11:
        case 12:
        case 13:
        case 14:
          if (char_get_skill(ch, SKILL_ELBOW) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try an@R elbow @Gnext!@n\r\n");
            char_set_combo(ch, 2);
            success = true;
          }
          break;
        case 15:
        case 16:
        case 17:
          if (char_get_skill(ch, SKILL_KNEE) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try a@R knee @Gnext!@n\r\n");
            char_set_combo(ch, 3);
            success = true;
          }
          break;
        case 18:
        case 19:
          if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try a@R roundhouse @Gnext!@n\r\n");
            char_set_combo(ch, 4);
            success = true;
          }
          break;
        case 20:
        case 21:
          if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try an@R uppercut @Gnext!@n\r\n");
            char_set_combo(ch, 5);
            success = true;
          }
          break;
        case 22:
          if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try a@R heeldrop @Gnext!@n\r\n");
            char_set_combo(ch, 8);
            success = true;
          }
          break;
        case 24:
          if (char_get_skill(ch, SKILL_SLAM) > 0) {
            send_to_char(g, ch, "@GYou have a chance for a COMBO! Try a@R slam @Gnext!@n\r\n");
            char_set_combo(ch, 6);
            success = true;
          }
          break;
      }
    }
    return 0;
  }
  if (char_get_lastattack(ch) == char_get_combo(ch) && char_get_combhits(ch) < physical_mastery(ch)) {
    char_set_combhits(ch, char_get_combhits(ch) + (1));
    while (success == false) {
      if (char_get_combhits(ch) >= 20) { /* We're kicking ass! */
        switch (rand_number(g, 1, 34)) {
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
            if (char_get_skill(ch, SKILL_ELBOW) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 2);
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
            if (char_get_skill(ch, SKILL_KNEE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 3);
              success = true;
            }
            break;
          case 17:
          case 18:
          case 19:
          case 20:
          case 21:
            if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 5);
              success = true;
            }
            break;
          case 22:
          case 23:
          case 24:
          case 25:
          case 26:
            if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 4);
              success = true;
            }
            break;
          case 27:
          case 28:
          case 29:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 8);
              success = true;
            } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 6);
              success = true;
            }
            break;
          case 30:
          case 31:
          case 32:
          case 33:
          case 34:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 8);
              success = true;
            } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 6);
              success = true;
            }
            break;
        } /* Switch End */
      } else if (char_get_combhits(ch) >= 15) { /* We're doing good! */
        switch (rand_number(g, 1, 36)) {
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
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 2);
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
            if (char_get_skill(ch, SKILL_KNEE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 3);
              success = true;
            }
            break;
          case 21:
          case 22:
          case 23:
            if (char_get_skill(ch, SKILL_PUNCH) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 0);
              success = true;
            }
            break;
          case 25:
          case 26:
          case 27:
            if (char_get_skill(ch, SKILL_KICK) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 1);
              success = true;
            }
            break;
          case 29:
          case 30:
            if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 5);
              success = true;
            }
            break;
          case 31:
          case 32:
          case 33:
          case 34:
            if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 4);
              success = true;
            }
            break;
          case 35:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 6);
              success = true;
            }
            break;
          case 36:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 8);
              success = true;
            }
            break;
        }
      } else if (char_get_combhits(ch) >= 10) { /* We're on a roll */
        switch (rand_number(g, 1, 34)) {
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
            if (char_get_skill(ch, SKILL_ELBOW) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 2);
              success = true;
            }
            break;
          case 6:
          case 7:
          case 8:
          case 9:
          case 10:
            if (char_get_skill(ch, SKILL_KNEE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 3);
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
            if (char_get_skill(ch, SKILL_PUNCH) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 0);
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
            if (char_get_skill(ch, SKILL_KICK) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 1);
              success = true;
            }
            break;
          case 27:
          case 28:
          case 29:
            if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 5);
              success = true;
            }
            break;
          case 30:
          case 31:
            if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 4);
              success = true;
            }
            break;
          case 32:
          case 33:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_SLAM) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 6);
              success = true;
            }
            break;
          case 34:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEELDROP) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 8);
              success = true;
            }
            break;
        }
      } else if (char_get_combhits(ch) >= 5) { /* We're staring off well */
        switch (rand_number(g, 1, 30)) {
          case 1:
          case 2:
          case 3:
          case 4:
            if (char_get_skill(ch, SKILL_ELBOW) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 2);
              success = true;
            }
            break;
          case 5:
          case 6:
          case 7:
          case 8:
            if (char_get_skill(ch, SKILL_KNEE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 3);
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
            if (char_get_skill(ch, SKILL_PUNCH) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 0);
              success = true;
            }
            break;
          case 19:
          case 20:
          case 21:
          case 22:
            if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            }
            break;
          case 23:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            }
            break;
          case 24:
          case 25:
            if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            }
            break;
          case 26:
            if (char_get_skill(ch, SKILL_HEELDROP) > 0 && rand_number(g, 1, 3) == 3) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 8);
              success = true;
            }
            break;
          case 27:
            if (char_get_skill(ch, SKILL_SLAM) > 0 && rand_number(g, 1, 3) == 3) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 6);
              success = true;
            }
            break;
          case 28:
            if (char_get_skill(ch, SKILL_KICK) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 1);
              success = true;
            }
            break;
          case 29:
            if (char_get_skill(ch, SKILL_UPPERCUT) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 5);
              success = true;
            }
            break;
          case 30:
            if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 4);
              success = true;
            }
            break;
        }
      } else { /* We just the combo not long ago */
        switch (rand_number(g, 1, 30)) {
          case 1:
          case 2:
          case 3:
            if (char_get_skill(ch, SKILL_ELBOW) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R elbow@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 2);
              success = true;
            }
            break;
          case 4:
          case 5:
          case 6:
            if (char_get_skill(ch, SKILL_KNEE) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rknee@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 3);
              success = true;
            }
            break;
          case 7:
          case 8:
          case 9:
          case 10:
            if (char_get_skill(ch, SKILL_UPPERCUT) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try an@R uppercut@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 5);
              success = true;
            }
            break;
          case 11:
          case 12:
          case 13:
          case 14:
            if (char_get_skill(ch, SKILL_ROUNDHOUSE) > 0 && rand_number(g, 1, 2) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 4);
              success = true;
            }
            break;
          case 15:
          case 16:
          case 17:
          case 18:
            if (char_get_skill(ch, SKILL_PUNCH) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rpunch@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 0);
              success = true;
            }
            break;
          case 19:
          case 20:
          case 21:
          case 22:
            if (char_get_skill(ch, SKILL_BASH) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try bash@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 51);
              success = true;
            } else if (char_get_skill(ch, SKILL_TAILWHIP) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 56);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEADBUTT) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 52);
              success = true;
            } else if (char_get_skill(ch, SKILL_HEELDROP) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 8);
              success = true;
            } else if (char_get_skill(ch, SKILL_SLAM) > 0 && rand_number(g, 1, 3) == 2) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rslam@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 6);
              success = true;
            }
            break;
          case 23:
          case 24:
          case 25:
          case 26:
          case 27:
          case 28:
          case 29:
          case 30:
            if (char_get_skill(ch, SKILL_KICK) > 0) {
              send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Next try a @Rkick@G!@n\r\n", char_get_combhits(ch));
              char_set_combo(ch, 1);
              success = true;
            }
            break;
        }
      } /* End continued hits section */
    }
    return char_get_combhits(ch);
  }
  if (char_get_lastattack(ch) == char_get_combo(ch) && char_get_combhits(ch) >= physical_mastery(ch)) {
    char_set_combhits(ch, char_get_combhits(ch) + (1));
    send_to_char(g, ch, "@D(@GC-c-combo Bonus @gx%d@G!@D)@C Combo FINISHED for massive damage@G!@n\r\n", char_get_combhits(ch));
  } else if (char_get_combo(ch) != char_get_lastattack(ch) && char_get_combo(ch) > -1) {
    send_to_char(g, ch, "@GCombo failed! Try harder next time!@n\r\n");
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

void handle_spiral(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill, int first)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2;
  int64_t dmg;
  double amount = 0.0;

  if (first == false) {
    amount = 0.05;
  } else {
    amount = 0.5;
  }

  if (vict == nullptr && char_get_fighting(g, ch)) {
    vict = char_get_fighting(g, ch);
  } else if (vict == nullptr) {
    act(g, "@WHaving lost your target you slow down until your vortex disappears, and end your attack.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W slows down until $s vortex disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_SPIRAL, false);
    return;
  }

  if (char_get_charge(ch) <= 0) {
    act(g, "@WHaving no more charged ki you slow down until your vortex disappears, and end your attack.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W slows down until $s vortex disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_SPIRAL, false);
    return;
  }

  if (vict) {
    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = skill;
    perc = axion_dice(g, 0);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    if (avo > 0 && avo < 70) {
      prob -= avo;
    } else if (avo >= 70) {
      prob -= 69;
    }
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Spiral Comet Blast")) {
      pcost(g, ch, amount, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W moves quickly and blocks your Spiral Comet blast!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W Spiral Comet blast!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W Spiral Comet blast!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, amount, 0);
          dmg = damtype(g, ch, 10, skill, .05);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Spiral Comet blast, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Spiral Comet blast, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Spiral Comet blast, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(g, ch, vict, 0, 45, skill, SKILL_SPIRAL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
          }

          pcost(g, ch, amount, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Spiral Comet blast at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, amount, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Spiral Comet blast at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, amount, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    if (first == true) {
      dmg = damtype(g, ch, 44, skill, .5);
    } else {
      dmg = damtype(g, ch, 45, skill, .01);
    }
    switch (rand_number(g, 1, 5)) {
      case 1:
        act(g, "@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S chest and explodes!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR chest and explodes!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S chest and explodes!@n", true, ch, nullptr, vict, TO_NOTVICT);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 2: /* Critical */
        act(g, "@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S head and explodes!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR head and explodes!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S head and explodes!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= 2;
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        break;
      case 3:
        act(g, "@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S body and explodes!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR body and explodes!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S body and explodes!@n", true, ch, nullptr, vict, TO_NOTVICT);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 4: /* Weak */
        act(g, "@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S arm and explodes!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR arm and explodes!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S arm and explodes!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg /= 2;
        dam_eq_loc(g, vict, 1);
        hurt(g, 0, 190, ch, vict, nullptr, dmg, 1);
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S leg and explodes!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR leg and explodes!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S leg and explodes!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg /= 2;
        dam_eq_loc(g, vict, 2);
        hurt(g, 1, 190, ch, vict, nullptr, dmg, 1);
        break;
    }
    pcost(g, ch, amount, 0);
    return;
  }
  return;
}

static void handle_death_msg(GameWorld *g, struct char_data *ch, struct char_data *vict, int type)
{
  if (type == 0) {
    if (!room_sunken(g, char_get_in_room(vict)) && room_sect(g, char_get_in_room(vict)) != SECT_WATER_SWIM && room_sect(g, char_get_in_room(vict)) != SECT_WATER_NOSWIM && !room_flagged(g, char_get_in_room(vict), ROOM_SPACE) && room_sect(g, char_get_in_room(vict)) != SECT_FLYING) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r coughs up blood before falling to the ground dead.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cough up blood before falling to the ground dead.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r coughs up blood before falling down dead.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 2:
          act(g, "@R$N@r crumples to the ground dead.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou crumple to the ground dead.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r crumples to the ground dead.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 3:
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 4:
          act(g, "@R$N@r writhes on the ground screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou writhe on the ground screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r writhes on the ground screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
        case 5:
          act(g, "@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou hit the ground dead with such force that blood flies into the air briefly!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
      }
    } else if (room_sect(g, char_get_in_room(vict)) == SECT_WATER_SWIM || room_sect(g, char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r coughs up blood and dies before falling down to the water. A large splash accompanies $S body hitting the water!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cough up blood and die before falling down to the water. A large splash accompanies your body hitting the water!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r coughs up blood and dies before falling down to the water. A large splash accompanies $S body hitting the water!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 2:
          act(g, "@R$N@r crumples down to the water, with the signs of life leaving $S eyes as $E floats in the water.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou crumple down to the water and die.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r crumples down to the water, with the signs of life leaving $S eyes as $E floats in the water.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 3:
          act(g, "@R$N@r cries out $S last breath before dying and leaving a floating corpse behind.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r cries out $S last breath before dying and leaving a floating corpse behind.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 4:
          act(g, "@R$N@r writhes in the water screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou writhe in the water screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r writhes in the water screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
        case 5:
          act(g, "@R$N@r hits the water dead with such force that blood mixed with water flies into the air briefly!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou hit the water dead with such force that blood mixed with water flies into the air briefly!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r hits the water dead with such force that blood mixed with water flies into the air briefly!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
      }
    } else if (room_flagged(g, char_get_in_room(vict), ROOM_SPACE)) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r coughs up blood and dies. The blood freezes and floats freely through space...@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cough up blood and die. The blood freezes and floats freely through space...@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r coughs up blood and dies. The blood freezes and floats freely through space...@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 2:
          act(g, "@R$N@r dies and leaves $S corpse floating freely in space.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou die and leave your corpse floating freely in space.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r dies and leaves $S corpse floating freely in space.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 3:
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 4:
          act(g, "@R$N@r writhes in space trying to scream in pain before $e finally dies!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou writhe in space trying to scream in pain before you finally die!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r writhes in space trying to scream in pain before $e finally dies!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
        case 5:
          act(g, "@R$N@r dies suddenly leaving behind a badly damaged corpse floating in space!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou die suddenly leaving behind a badly damaged corpse floating in space!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r dies suddenly leaving behind a badly damaged corpse floating in space!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
      }
    } else if (room_sect(g, char_get_in_room(vict)) == SECT_FLYING) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r coughs up blood before $s corpse starts to fall to the ground far below.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou coughs up blood before your corpse starts to fall to the ground far below.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r coughs up blood before $s corpse starts to fall to the ground far below.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 2:
          act(g, "@R$N@r dies and $S corpse begins to fall to the ground below.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou die and your corpse begins to fall to the ground below.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r dies and $S corpse begins to fall to the ground below.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 3:
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 4:
          act(g, "@R$N@r writhes in midair screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou writhe in midair screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r writhes in midair screaming in pain before finally dying!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
        case 5:
          act(g, "@R$N@r snaps back and dies with such force that blood flies into the air briefly!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou snap back and die with such force that blood flies into the air briefly!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r hits the ground dead with such force that blood flies into the air briefly!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
      }
    } else {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r coughs up blood before $s corpse starts to float limply in the water.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou coughs up blood before your corpse starts to float limply in the water.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r coughs up blood before $s corpse starts to float limply in the water.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 2:
          act(g, "@R$N@r dies and $S corpse begins to float limply in the water.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou die and your corpse begins to float limply in the water.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r dies and $S corpse begins to float limply in the water.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 3:
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou cry out your last breath before dying.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r cries out $S last breath before dying.@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 4:
          act(g, "@R$N@r writhes and thrases in the water trying to scream before finally dying!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou writhe and thrash in the water trying to scream before finally dying!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r writhes and thrashes in the water trying to scream before finally dying!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
        case 5:
          act(g, "@R$N@r snaps back and dies with such force that blood floods out of $S body into the water!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou snap back and die with such force that blood floods out of your body into the water!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r hits the ground dead with such force that blood floods out of $S body into the water!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_death_type(vict) != DTYPE_HEAD) {
            char_set_death_type(vict, DTYPE_PULP);
          }
          break;
      }
    }
  } else {
    if (!room_sunken(g, char_get_in_room(vict)) && room_sect(g, char_get_in_room(vict)) != SECT_WATER_SWIM && room_sect(g, char_get_in_room(vict)) != SECT_WATER_NOSWIM && !room_flagged(g, char_get_in_room(vict), ROOM_SPACE) && room_sect(g, char_get_in_room(vict)) != SECT_FLYING) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 2:
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HALF);
          break;
        case 3:
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 4:
          act(g, "@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 5:
          act(g, "@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rWhat's left of your body slams into the ground as you die!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
      }
    } else if (room_sect(g, char_get_in_room(vict)) == SECT_WATER_SWIM || room_sect(g, char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r explodes and chunks of $M shower to the ground.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 2:
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HALF);
          break;
        case 3:
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 4:
          act(g, "@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r falls down as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 5:
          act(g, "@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rWhat's left of your body slams into the ground as you die!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rWhat's left of @R$N@r's body slams into the ground as $E dies!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
      }
    } else if (room_flagged(g, char_get_in_room(vict), ROOM_SPACE)) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r explodes and chunks of $M shower out into every direction of space.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r explodes and chunks of $M shower out into every direction of space.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 2:
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HALF);
          break;
        case 3:
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 4:
          act(g, "@R$N@r floats away as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body floats away as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r floats away as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 5:
          act(g, "@rWhat's left of @R$N@r's body floats away through space!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rWhat's left of your body floats away through space!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rWhat's left of @R$N@r's body floats away through space!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
      }
    } else if (room_sect(g, char_get_in_room(vict)) == SECT_FLYING) {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r explodes and chunks of $M shower towards the ground far below.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r explodes and chunks of $M shower toward the ground far below.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 2:
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HALF);
          break;
        case 3:
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 4:
          act(g, "@R$N@r falls down toward the ground as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body falls down toward the ground as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r falls down toward the ground as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 5:
          act(g, "@rWhat's left of @R$N@r's body falls toward the ground as $E dies!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rWhat's left of yor body falls toward the ground as you die!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rWhat's left of @R$N@r's body falls toward the ground as $E dies!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
      }
    } else {
      switch (rand_number(g, 1, 5)) {
        case 1:
          act(g, "@R$N@r explodes and chunks of $M float freely through the water.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou explode leaving only chunks behind.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r explodes and chunks of $M float freely through the water.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 2:
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rThe bottom half of your body is all that remains as you die.@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rThe bottom half of @R$N@r is all that remains as $E dies.@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HALF);
          break;
        case 3:
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body completely disintegrates in the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r is completely disintegrated in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_VAPOR);
          break;
        case 4:
          act(g, "@R$N@r falls back as a smoldering corpse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour body falls back as a smoldering corpse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r falls back as a smoldering corpse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
        case 5:
          act(g, "@rWhat's left of @R$N@r's body floats limply as $E dies!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rWhat's left of yor body floats limply as you die!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@rWhat's left of @R$N@r's body floats limply as $E dies!@n", true, ch, nullptr, vict, TO_NOTVICT);
          break;
      }
    }
  }
}
