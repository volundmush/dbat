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
#include "combat.h"
#include "act.informative.h"
#include "act.item.h"
#include "act.movement.h"
#include "class.h"
#include "comm.h"
#include "consts/attacks.h"
#include "consts/deathtype.h"
#include "consts/search.h"
#include "dg_comm.h"
#include "dg_scripts.h"
#include "fight.h"
#include "genzon.h"
#include "handler.h"
#include "mobact.h"
#include "techniques.h"

#include "extract.h"
#include "random.h"
#include "relocate.h"

#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/fightprefs.h"
#include "consts/itemdata.h"
#include "consts/materials.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "db.h"
#include "flags.h"
#include "log.h"
#include "object_api.h"
#include "object_db.h"
#include "object_impl.h"
#include "object_macros.h"
#include "object_scripts.h"
#include "event_queue_api.h"
#include "relocate.h"
#include "room_api.h"
#include "room_db.h"
#include "room_utils.h"
#include "stringutils.h"

#include "iterate.hpp"

#include <initializer_list>

/* local functions */
void damage_weapon(struct char_data *ch, struct obj_data *obj,
                   struct char_data *vict) {

  if (obj) {
    if (OBJ_FLAGGED(obj, ITEM_UNBREAKABLE))
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

  switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL)) {
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

  if (char_condition_has(ch, "curse")) {
    result += 3;
  } else if (char_condition_has(ch, "bless") && rand_number(1, 3) == 3) {
    if (result > 1) {
      result = 1;
    }
  } else if (char_condition_has(ch, "bless")) {
    result = 0;
  }

  if (GET_SKILL(ch, SKILL_HANDLING) >= axion_dice(10)) {
    act("@GYour superior handling prevents @C$p@G from being damaged.@n", TRUE,
        ch, obj, 0, TO_CHAR);
    act("@g$n's@G superior handling prevents @C$p@G from being damaged.@n",
        TRUE, ch, obj, 0, TO_ROOM);
    result = 0;
  }

  if (result > 0) {
    GET_OBJ_VAL(obj, VAL_ALL_HEALTH) -= result;
    if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) <= 0) {
      act("@RYour @C$p@R shatters on @r$N's@R body!@n", TRUE, ch, obj, vict,
          TO_CHAR);
      act("@r$n's@R @C$p@R shatters on YOUR body!@n", TRUE, ch, obj, vict,
          TO_VICT);
      act("@r$n's@R @C$p@R shatters on @r$N's@R body!@n", TRUE, ch, obj, vict,
          TO_NOTVICT);
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
      perform_remove(vict, 16);
      perform_remove(vict, 17);
    } else if (result >= 8) {
      act("@RYour @C$p@R cracks loudly from striking @r$N's@R body!@n", TRUE,
          ch, obj, vict, TO_CHAR);
      act("@r$n's@R @C$p@R cracks loudly from striking YOUR body!@n", TRUE, ch,
          obj, vict, TO_VICT);
      act("@r$n's@R @C$p@R cracks loudly from striking @r$N's@R body!@n", TRUE,
          ch, obj, vict, TO_NOTVICT);
    } else if (result >= 6) {
      act("@RYour @C$p@R chips from striking @r$N's@R body!@n", TRUE, ch, obj,
          vict, TO_CHAR);
      act("@r$n's@R @C$p@R cracks from striking YOUR body!@n", TRUE, ch, obj,
          vict, TO_VICT);
      act("@r$n's@R @C$p@R cracks from striking @r$N's@R body!@n", TRUE, ch,
          obj, vict, TO_NOTVICT);
    } else if (result >= 3) {
      act("@RYour @C$p@R loses a piece from striking @r$N's@R body!@n", TRUE,
          ch, obj, vict, TO_CHAR);
      act("@r$n's@R @C$p@R loses a piece from striking YOUR body!@n", TRUE, ch,
          obj, vict, TO_VICT);
      act("@r$n's@R @C$p@R loses a piece from striking @r$N's@R body!@n", TRUE,
          ch, obj, vict, TO_NOTVICT);
    } else if (result >= 6) {
      act("@RYour @C$p@R has a nick in it from hitting @r$N's@R body!@n", TRUE,
          ch, obj, vict, TO_CHAR);
      act("@r$n's@R @C$p@R has a nick in it from hitting YOUR body!@n", TRUE,
          ch, obj, vict, TO_VICT);
      act("@r$n's@R @C$p@R has a nick in it from hitting @r$N's@R body!@n",
          TRUE, ch, obj, vict, TO_NOTVICT);
    }
  }
}


int handle_defender(struct char_data *vict, struct char_data *ch) {

  int result = FALSE;

  if (GET_DEFENDER(vict)) {
    struct char_data *def = GET_DEFENDER(vict);
    int64_t defnum = (GET_SPEEDI(def) * 0.01) * rand_number(-10, 10);
    int64_t chnum = (GET_SPEEDI(ch) * 0.01) * rand_number(-5, 10);
    if (GET_SPEEDI(def) + defnum > GET_SPEEDI(ch) + chnum &&
        char_room_get(def) == char_room_get(vict) &&
        GET_POS(def) > POS_SITTING) {
      act("@YYou move to and manage to intercept the attack aimed at @y$N@Y!@n",
          TRUE, def, 0, vict, TO_CHAR);
      act("@y$n@Y moves to and manages to intercept the attack aimed at YOU!@n",
          TRUE, def, 0, vict, TO_VICT);
      act("@y$n@Y moves to and manages to intercept the attack aimed at "
          "@y$N@Y!@n",
          TRUE, def, 0, vict, TO_NOTVICT);
      result = TRUE;
    } else if (char_room_get(def) == char_room_get(vict) &&
               GET_POS(def) > POS_SITTING) {
      act("@YYou move to intercept the attack aimed at @y$N@Y, but just not "
          "fast enough!@n",
          TRUE, def, 0, vict, TO_CHAR);
      act("@y$n@Y moves to intercept the attack aimed at YOU, but $e wasn't "
          "fast enough!@n",
          TRUE, def, 0, vict, TO_VICT);
      act("@y$n@Y moves to intercept the attack aimed at @y$N@Y, but $e wasn't "
          "fast enough!@n",
          TRUE, def, 0, vict, TO_NOTVICT);
    }
  }

  return (result);
}

void handle_disarm(struct char_data *ch, struct char_data *vict) {

  int roll1 = rand_number(-10, 10), roll2 = rand_number(-10, 10),
      handled = FALSE;
  roll1 += GET_STR(ch) + GET_DEX(ch);
  roll2 += GET_STR(vict) + GET_DEX(vict);

  if (!IS_NPC(ch)) {
    if (PLR_FLAGGED(ch, PLR_THANDW)) {
      roll1 += 5;
    }
  }

  if (rand_number(1, 100) <= 50 && !IS_KONATSU(ch)) {
    roll1 = -500;
  } else if (rand_number(1, 100) <= 75) {
    roll1 *= 1.5;
  }

  if (IS_KONATSU(vict)) {
    roll1 *= 0.75;
  }

  if (GET_SKILL(ch, SKILL_HANDLING) >= axion_dice(10)) {
    handled = TRUE;
  }

  if (roll1 < roll2) {
    struct obj_data *obj;
    if (GET_EQ(ch, WEAR_WIELD1) && handled == FALSE) {
      obj = GET_EQ(ch, WEAR_WIELD1);
      act("@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n",
          TRUE, ch, obj, vict, TO_CHAR);
      act("@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n",
          TRUE, ch, obj, vict, TO_NOTVICT);
      act("@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n",
          TRUE, ch, obj, vict, TO_VICT);
      perform_remove(ch, 16);
      if (GET_OBJ_VNUM(obj) != 20098) {
        obj_from_char(obj);
        obj_to_room(obj, char_room_get(ch));
      }
    } else if (GET_EQ(ch, WEAR_WIELD1) && handled == TRUE) {
      obj = GET_EQ(ch, WEAR_WIELD1);
      act("@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it "
          "from slipping from your grasp!@n",
          TRUE, ch, obj, vict, TO_CHAR);
      act("@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it "
          "from slipping from $s grasp!@n",
          TRUE, ch, obj, vict, TO_NOTVICT);
      act("@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from "
          "slipping from $s grasp!@n",
          TRUE, ch, obj, vict, TO_VICT);
    } else if (GET_EQ(ch, WEAR_WIELD2) && handled == TRUE) {
      obj = GET_EQ(ch, WEAR_WIELD2);
      act("@y$N@Y almosts disarms you, but your handling of @w$p@Y saves it "
          "from slipping from your grasp!@n",
          TRUE, ch, obj, vict, TO_CHAR);
      act("@y$N@Y almost disarms @R$n@Y, but $s handling of @w$p@Y saves it "
          "from slipping from $s grasp!@n",
          TRUE, ch, obj, vict, TO_NOTVICT);
      act("@YYou almost disarm @R$n@Y, but $s handling of @w$p@Y saves it from "
          "slipping from $s grasp!@n",
          TRUE, ch, obj, vict, TO_VICT);
    } else if (GET_EQ(ch, WEAR_WIELD2)) {
      obj = GET_EQ(ch, WEAR_WIELD2);
      act("@y$N@Y manages to disarm you! The @w$p@Y falls from your grasp!@n",
          TRUE, ch, obj, vict, TO_CHAR);
      act("@y$N@Y manages to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n",
          TRUE, ch, obj, vict, TO_NOTVICT);
      act("@YYou manage to disarm @R$n@Y! The @w$p@Y falls from $s grasp!@n",
          TRUE, ch, obj, vict, TO_VICT);
      perform_remove(ch, 17);
      if (GET_OBJ_VNUM(obj) != 20098) {
        obj_from_char(obj);
        obj_to_room(obj, char_room_get(ch));
      }
    }
  }
}


static bool hot_ruby(struct obj_data *obj, void *ctx) {
  if (GET_OBJ_VNUM(obj) == 6600 && OBJ_FLAGGED(obj, ITEM_HOT)) {
    return (TRUE);
  }
  return (FALSE);
}

int check_ruby(struct char_data *ch) {

  struct obj_data *ruby =
      char_inventory_search_vnum(ch, 6600, FALSE, SEARCH_HOT);

  if (ruby) {
    act("@RYour $p@R flares up and disappears. Your fire attack has been "
        "aided!@n",
        TRUE, ch, ruby, 0, TO_CHAR);
    act("@R$n's@R $p@R flares up and disappears!@n", TRUE, ch, ruby, 0,
        TO_ROOM);
    extract_obj(ruby);
    return (1);
  } else {
    return (0);
  }
}

int64_t combo_damage(struct char_data *ch, int64_t damage, int type) {
  int64_t bonus = 0;

  if (type == 0) { /* Not a finish */
    int hits = char_condition_number_get(ch, "combo", "hits");

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
int roll_balance(struct char_data *ch) {

  int chance = 0;

  if (IS_NPC(ch)) {
    if (GET_LEVEL(ch) >= 100) {
      chance = rand_number(80, 100);
    } else if (GET_LEVEL(ch) >= 80) {
      chance = rand_number(75, 90);
    } else if (GET_LEVEL(ch) >= 70) {
      chance = rand_number(70, 80);
    } else if (GET_LEVEL(ch) >= 60) {
      chance = rand_number(65, 75);
    } else if (GET_LEVEL(ch) >= 50) {
      chance = rand_number(50, 60);
    }
  } else {
    if (GET_SKILL(ch, SKILL_BALANCE) > 50) {
      chance = GET_SKILL(ch, SKILL_BALANCE);
    }
  }

  return (chance);
}

void handle_knockdown(struct char_data *ch) {
  int chance = 0;

  if (IS_NPC(ch)) {
    if (GET_LEVEL(ch) >= 100) {
      chance = rand_number(35, 45);
    } else if (GET_LEVEL(ch) >= 90) {
      chance = rand_number(25, 35);
    } else if (GET_LEVEL(ch) >= 70) {
      chance = rand_number(15, 25);
    } else if (GET_LEVEL(ch) >= 50) {
      chance = rand_number(10, 15);
    } else if (GET_LEVEL(ch) >= 30) {
      chance = rand_number(5, 10);
    }
  } else {
    chance = GET_SKILL(ch, SKILL_BALANCE) * 0.5;
  }

  if (chance > axion_dice(0)) {
    act("@mYou are @GALMOST@m knocked off your feet, but your great balance "
        "helps you keep your footing!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n@m is @GALMOST@m knocked off $s feet, but $s great balance helps "
        "$m keep $s footing!@n",
        TRUE, ch, 0, 0, TO_ROOM);
  } else {
    act("@mYou are knocked off your feet!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n@m is knocked off $s feet!@n", TRUE, ch, 0, 0, TO_ROOM);
    char_position_set(ch, POS_SITTING);
  }
}

int boom_headshot(struct char_data *ch) {

  int skill = GET_SKILL_BASE(ch, SKILL_TWOHAND);

  if (skill >= 100 && rand_number(1, 5) >= 3)
    return (1);
  else if (skill < 100 && skill >= 75 && rand_number(1, 5) == 5)
    return (1);
  else if (skill < 75 && skill >= 50 && rand_number(1, 6) == 6)
    return (1);
  else
    return (0);
}

int64_t gun_dam(struct char_data *ch, int wlvl) {
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

  if (GET_SKILL(ch, SKILL_GUN) >= 100)
    dmg *= 2;
  else if (GET_SKILL(ch, SKILL_GUN) >= 50)
    dmg += dmg * 0.5;

  int64_t dmg_prior = 0;

  dmg_prior = (dmg * GET_DEX(ch)) * ((GET_LEVEL(ch) / 5) + 1);

  if (dmg_prior <= GET_MAX_HIT(ch) * 0.4)
    dmg = dmg_prior;
  else
    dmg = GET_MAX_HIT(ch) * 0.4;

  return (dmg);
}

void club_stamina(struct char_data *ch, struct char_data *vict, int wlvl,
                  int64_t dmg) {

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

  if (GET_SKILL(ch, SKILL_CLUB) >= 100)
    drain += 0.1;
  else if (GET_SKILL(ch, SKILL_CLUB) >= 50)
    drain += 0.05;

  drained = dmg * drain;
  decCurST(vict, drained);

  send_to_char(ch, "@D[@YVictim's @GStamina @cLoss@W: @g%s@D]@n\r\n",
               add_commas(drained));
  send_to_char(vict, "@D[@rYour @GStamina @cLoss@W: @g%s@D]@n\r\n",
               add_commas(drained));
}


void cut_limb(struct char_data *ch, struct char_data *vict, int wlvl,
              int hitspot) {

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

  if (GET_SKILL(ch, SKILL_SWORD) >= 100) {
    chance += 100;
  } else if (GET_SKILL(ch, SKILL_SWORD) >= 50) {
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
    act("@R$N's@r head is cut off in the attack!@n", TRUE, ch, 0, vict,
        TO_CHAR);
    act("@RYOUR head is cut off in the attack!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@R$N's@rhead is cut off in the attack!@n", TRUE, ch, 0, vict,
        TO_NOTVICT);

    remove_limb(vict, 0);
    die(vict, ch);
    char corp[256];
    if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
      sprintf(corp, "all.zenni corpse");
      do_get(ch, corp, 0, 0);
    }
    if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
      sprintf(corp, "all corpse");
      do_get(ch, corp, 0, 0);
    }
    return;
  } else { /* We've only succeeded in removing a limb. */
    if (!IS_NPC(vict)) {
      if (HAS_ARMS(vict) && rand_number(1, 2) == 2) {
        if (GET_LIMBCOND(vict, 2) > 0) {
          SET_LIMBCOND(vict, 2, 0);
          if (PLR_FLAGGED(vict, PLR_CLARM)) {
            REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLARM);
          }
          act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your left arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          remove_limb(vict, 2);
        } else if (GET_LIMBCOND(vict, 1) > 0) {
          SET_LIMBCOND(vict, 1, 100);
          if (PLR_FLAGGED(vict, PLR_CRARM)) {
            REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CRARM);
          }
          act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your right arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          remove_limb(vict, 1);
        }
      } else { /* It's a leg */
        if (GET_LIMBCOND(vict, 4) > 0) {
          SET_LIMBCOND(vict, 4, 100);
          if (PLR_FLAGGED(vict, PLR_CLLEG)) {
            REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLLEG);
          }
          act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your left leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          remove_limb(vict, 4);
        } else if (GET_LIMBCOND(vict, 3) > 0) {
          SET_LIMBCOND(vict, 3, 100);
          if (PLR_FLAGGED(vict, PLR_CRLEG)) {
            REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CRLEG);
          }
          act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your right leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          remove_limb(vict, 3);
        }
      }
    } else { /* It's a npc */
      if (HAS_ARMS(vict) && rand_number(1, 2) == 2) {
        if (MOB_FLAGGED(vict, MOB_LARM)) {
          REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_LARM);
          remove_limb(vict, 2);
          act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your left arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s left arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        } else if (MOB_FLAGGED(vict, MOB_RARM)) {
          REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_RARM);
          remove_limb(vict, 1);
          act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your right arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s right arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        }
      } else {
        if (MOB_FLAGGED(vict, MOB_LLEG)) {
          REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_LLEG);
          remove_limb(vict, 4);
          act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your left leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s left leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        } else if (MOB_FLAGGED(vict, MOB_RLEG)) {
          REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_RLEG);
          remove_limb(vict, 3);
          act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYOU lose your right leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@r loses $s right leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        }
      }
    }
  }
}

int count_physical(struct char_data *ch) {
  int count = 0;

  for(auto s : {SKILL_PUNCH, SKILL_KICK, SKILL_KNEE, SKILL_ELBOW, SKILL_ROUNDHOUSE, SKILL_SLAM, SKILL_UPPERCUT,
                SKILL_TAILWHIP, SKILL_BASH, SKILL_HEADBUTT, SKILL_HEELDROP}) {
    if (GET_SKILL(ch, s) >= 1) {
      count += 1;
    }
  }

  return (count);
}

int physical_mastery(struct char_data *ch) {

  int count = 22;

  for(auto s : {SKILL_PUNCH, SKILL_KICK, SKILL_KNEE, SKILL_ELBOW, SKILL_ROUNDHOUSE, SKILL_SLAM, SKILL_UPPERCUT,
                SKILL_TAILWHIP, SKILL_BASH, SKILL_HEADBUTT, SKILL_HEELDROP}) {
    if (GET_SKILL(ch, s) >= 100) {
      count += 1;
    }
  }

  if (count == 26)
    count += 1;
  else if (count >= 27)
    count += 2;

  return (count);
}

int64_t advanced_energy(struct char_data *ch, int64_t dmg) {

  if (ch == NULL) {
    return (FALSE);
  }

  double rate = 0.00;
  int count = GET_LEVEL(ch);
  int64_t add = 0;

  if (GET_BONUS(ch, BONUS_LEECH)) {
    rate = (double)(count) * 0.2;

    if (rate > 0.00) {
      add = dmg * rate;
      if (GET_CHARGE(ch) + add > GET_MAX_MANA(ch)) {
        if (GET_CHARGE(ch) < GET_MAX_MANA(ch)) {
          char_charge_set(ch, GET_MAX_MANA(ch));
          act("@MYou leech some of the energy away!@n", TRUE, ch, 0, 0,
              TO_CHAR);
          act("@m$n@M leeches some of the energy away!@n", TRUE, ch, 0, 0,
              TO_ROOM);
        } else {
          send_to_char(ch, "@MYou can't leech because there is too much "
                           "charged energy for you to handle!@n\r\n");
        }
      } else {
        char_charge_set(ch, GET_CHARGE(ch) + (int64_t)add);
        act("@MYou leech some of the energy away!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@m$n@M leeches some of the energy away!@n", TRUE, ch, 0, 0,
            TO_ROOM);
      }

    } /* End of rate check */

  } /* End of Leech Bonus */

  if (GET_BONUS(ch, BONUS_INTOLERANT)) {
    rate = (double)(count) * 0.2;

    if (rate > 0.00) {
      if (GET_CHARGE(ch) > 0 && rand_number(1, 100) <= 10) {
        act("@MThe attack causes your weak control to slip and you are shocked "
            "by your own charged energy!@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@m$n@M suffers shock from their own charged energy!@n", TRUE, ch,
            0, 0, TO_ROOM);
        decCurHealthFloored(ch, GET_CHARGE(ch) / 4, 1);
      }
      add = dmg * rate;
    }
  } /* End of Energy Intolerant bonus */

  return (add);
} /* End of advanced_energy function */

int roll_accuracy(struct char_data *ch, int skill, bool kiatt) {

  if (!IS_NPC(ch)) {
    if (GET_BONUS(ch, BONUS_ACCURATE)) {
      if (kiatt == TRUE)
        skill += skill * 0.10;
      else
        skill += skill * 0.20;
    } else if (GET_BONUS(ch, BONUS_POORDEPTH)) {
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

long double calc_critical(struct char_data *ch, int loc) {

  int roll = rand_number(1, 100);
  long double multi = 1;

  if (loc == 0) { /* Head */
    if (GET_BONUS(ch, BONUS_POWERHIT)) {
      if (roll <= 15) {
        multi = 4;
      } else if (GET_BONUS(ch, BONUS_SOFT)) {
        multi = 1;
      } else {
        multi = 2;
      }
    } else if (GET_BONUS(ch, BONUS_SOFT)) {
      multi = 1;
    } else {
      multi = 2;
    }
  } else if (loc == 1) { /* Limb */
    if (GET_BONUS(ch, BONUS_SOFT)) {
      multi = 0.25;
    } else {
      multi = 0.5;
    }
  } else { /* Body*/
    if (GET_BONUS(ch, BONUS_SOFT)) {
      multi = 0.5;
    }
  }

  return (multi);
}

int roll_hitloc(struct char_data *ch, struct char_data *vict, int skill) {

  int location = 4, critmax = 1000;
  int critical = 0;

  if (IS_NPC(ch)) {
    if (GET_LEVEL(ch) > 100)
      skill = rand_number(GET_LEVEL(ch), GET_LEVEL(ch) + 10);
    else
      skill = rand_number(GET_LEVEL(ch), 100);
  }

  if (IS_DABURA(ch) && !IS_NPC(ch)) {
    if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
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

  if (!IS_NPC(vict)) {
    if (location == 4 && GET_LIMBCOND(vict, 1) <= 0 &&
        GET_LIMBCOND(vict, 2) <= 0) { /* No arms */
      location = 5;
    }

    if (location == 5 && GET_LIMBCOND(vict, 3) <= 0 &&
        GET_LIMBCOND(vict, 4) <= 0) { /* No legs */
      location = 4;
    }

    if (location == 4 && GET_LIMBCOND(vict, 1) <= 0 &&
        GET_LIMBCOND(vict, 2) <= 0) { /* Both failed, make body */
      location = 1;
    }
  }

  if (IS_NPC(vict)) {
    if (location == 4 && !MOB_FLAGGED(vict, MOB_RARM) &&
        !MOB_FLAGGED(vict, MOB_LARM))
      location = 5;

    if (location == 5 && !MOB_FLAGGED(vict, MOB_RLEG) &&
        !MOB_FLAGGED(vict, MOB_LLEG))
      location = 4;

    if (location == 5 && !MOB_FLAGGED(vict, MOB_RARM) &&
        !MOB_FLAGGED(vict, MOB_LARM))
      location = 1;
  }

  return (location);
}

int64_t armor_calc(struct char_data *ch, int64_t dmg, int type) {
  if (IS_NPC(ch))
    return (0);

  int64_t reduce = 0;

  auto armor = GET_ARMOR(ch);

  if (armor < 1000) {
    reduce = armor * 0.5;
  } else if (armor < 2000) {
    reduce = armor * .75;
  } else if (armor < 5000) {
    reduce = armor;
  } else if (armor < 10000) {
    reduce = armor * 2;
  } else if (armor < 20000) {
    reduce = armor * 4;
  } else if (armor < 30000) {
    reduce = armor * 8;
  } else if (armor < 40000) {
    reduce = armor * 12;
  } else if (armor < 60000) {
    reduce = armor * 25;
  } else if (armor < 100000) {
    reduce = armor * 50;
  } else if (armor < 150000) {
    reduce = armor * 75;
  } else if (armor < 200000) {
    reduce = armor * 150;
  } else if (armor >= 200000) {
    reduce = armor * 250;
  }

  /* loc: 0 = Physical Bonus, 1 = Ki Bonus, 2 = Bonus To Both */
  int loc = -1;
  double bonus = 0.0;

  char_equipment_iterate(ch, [&](auto i, auto eq) {
    switch (GET_OBJ_VAL(eq, VAL_ALL_MATERIAL)) {
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
    return true;
  });

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
void hurt_limb(struct char_data *ch, struct char_data *vict, int chance,
               int area, int64_t power) {
  if (!vict || IS_NPC(vict)) {
    return;
  }

  int dmg = 0;

  if (chance > axion_dice(100)) {
    return;
  }

  if (power > (getMaxPL(vict)) * 0.5) {
    dmg = rand_number(25, 40);
  } else if (power > (getMaxPL(vict)) * 0.25) {
    dmg = rand_number(15, 24);
  } else if (power > (getMaxPL(vict)) * 0.10) {
    dmg = rand_number(8, 14);
  } else if (power > (getMaxPL(vict)) * 0.05) {
    dmg = rand_number(4, 7);
  } else {
    dmg = rand_number(1, 3);
    ;
  }

  auto armor = GET_ARMOR(vict);
  if (armor > 50000) {
    dmg -= 5;
  } else if (armor > 40000) {
    dmg -= 4;
  } else if (armor > 30000) {
    dmg -= 3;
  } else if (armor > 20000) {
    dmg -= 2;
  } else if (armor > 10000) {
    dmg -= 1;
  } else if (armor > 5000) {
    dmg -= rand_number(0, 1);
  }

  if (dmg <= 0) {
    return;
  }

  if (!is_sparring(ch)) {
    if (area == 0) { /* Arms */
      if (GET_LIMBCOND(vict, 2) - dmg <= 0) {
        act("@RYour attack @YDESTROYS @r$N's@R left arm!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack @YDESTROYS@R YOUR left arm!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack @YDESTROYS @r$N's@R left arm!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        SET_LIMBCOND(vict, 2, 0);
        if (PLR_FLAGGED(vict, PLR_THANDW)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THANDW);
        }
        if (PLR_FLAGGED(vict, PLR_CLARM)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLARM);
        }
        remove_limb(vict, 2);
      } else if (GET_LIMBCOND(vict, 2) > 0) {
        SET_LIMBCOND(vict, 2, GET_LIMBCOND(vict, 2) - (dmg));
        act("@RYour attack hurts @r$N's@R left arm!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack hurts YOUR left arm!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack hurts @r$N's@R left arm!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
      } else if (GET_LIMBCOND(vict, 1) - dmg <= 0) {
        act("@RYour attack @YDESTROYS @r$N's@R right arm!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack @YDESTROYS@R YOUR right arm!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack @YDESTROYS @r$N's@R right arm!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        SET_LIMBCOND(vict, 1, 0);
        if (PLR_FLAGGED(vict, PLR_THANDW)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THANDW);
        }
        if (PLR_FLAGGED(vict, PLR_CLARM)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CRARM);
        }
        remove_limb(vict, 2);
      } else if (GET_LIMBCOND(vict, 1) > 0) {
        SET_LIMBCOND(vict, 1, GET_LIMBCOND(vict, 1) - (dmg));
        act("@RYour attack hurts @r$N's@R right arm!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack hurts YOUR right arm!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack hurts @r$N's@R right arm!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
      }
    } else if (area == 1) { /* Legs */
      if (GET_LIMBCOND(vict, 4) - dmg <= 0) {
        act("@RYour attack @YDESTROYS @r$N's@R left leg!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack @YDESTROYS@R YOUR left leg!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack @YDESTROYS @r$N's@R left leg!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        SET_LIMBCOND(vict, 4, 0);
        if (PLR_FLAGGED(vict, PLR_THANDW)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THANDW);
        }
        if (PLR_FLAGGED(vict, PLR_CLLEG)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLLEG);
        }
        remove_limb(vict, 2);
      } else if (GET_LIMBCOND(vict, 4) > 0) {
        SET_LIMBCOND(vict, 4, GET_LIMBCOND(vict, 4) - (dmg));
        act("@RYour attack hurts @r$N's@R left leg!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack hurts YOUR left leg!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack hurts @r$N's@R left leg!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
      } else if (GET_LIMBCOND(vict, 3) - dmg <= 0) {
        act("@RYour attack @YDESTROYS @r$N's@R right leg!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack @YDESTROYS@R YOUR right leg!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack @YDESTROYS @r$N's@R right leg!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        SET_LIMBCOND(vict, 3, 0);
        if (PLR_FLAGGED(vict, PLR_THANDW)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THANDW);
        }
        if (PLR_FLAGGED(vict, PLR_CLLEG)) {
          REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLLEG);
        }
        remove_limb(vict, 2);
      } else if (GET_LIMBCOND(vict, 3) > 0) {
        SET_LIMBCOND(vict, 3, GET_LIMBCOND(vict, 3) - (dmg));
        act("@RYour attack hurts @r$N's@R right leg!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@r$n's@R attack hurts YOUR right leg!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@r$n's@R attack hurts @r$N's@R right leg!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
      }
    }
  }
}

/* For damaging equipment when hit */
void dam_eq_loc(struct char_data *vict, int area) {
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

void damage_eq(struct char_data *vict, int location) {

  if (GET_EQ(vict, location) && rand_number(1, 20) >= 19 &&
      !AFF_FLAGGED(vict, AFF_SANCTUARY)) {
    struct obj_data *eq = GET_EQ(vict, location);
    if (OBJ_FLAGGED(eq, ITEM_UNBREAKABLE)) {
      return;
    }

    int loss = rand_number(2, 5);

    if (GET_OBJ_VNUM(eq) == 20099 || GET_OBJ_VNUM(eq) == 20098)
      loss = 1;

    if (char_condition_has(vict, "curse")) {
      loss *= 3;
    } else if (char_condition_has(vict, "bless") && rand_number(1, 3) == 3) {
      loss = 1;
    } else if (char_condition_has(vict, "bless")) {
      return;
    }

    GET_OBJ_VAL(eq, VAL_ALL_HEALTH) -= loss;
    if (GET_OBJ_VAL(eq, VAL_ALL_HEALTH) <= 0) {
      GET_OBJ_VAL(eq, VAL_ALL_HEALTH) = 0;
      SET_BIT_AR(GET_OBJ_EXTRA(eq), ITEM_BROKEN);
      act("@WYour $p@W completely breaks!@n", FALSE, 0, eq, vict, TO_VICT);
      act("@C$N's@W $p@W completely breaks!@n", FALSE, 0, eq, vict, TO_NOTVICT);
      perform_remove(vict, location);
      if (!IS_NPC(vict))
        save_char(vict);
    } else if (GET_OBJ_VAL(eq, VAL_ALL_MATERIAL) == MATERIAL_LEATHER ||
               GET_OBJ_VAL(eq, VAL_ALL_MATERIAL) == MATERIAL_COTTON ||
               GET_OBJ_VAL(eq, VAL_ALL_MATERIAL) == MATERIAL_SILK) {
      act("@WYour $p@W rips a little!@n", FALSE, 0, eq, vict, TO_VICT);
      act("@C$N's@W $p@W rips a little!@n", FALSE, 0, eq, vict, TO_NOTVICT);
      if (char_condition_has(vict, "bless")) {
        send_to_char(vict, "@c...But your blessing seems to have partly mended "
                           "this damage.@n\r\n");
        act("@c...but @C$N's@c body glows blue for a moment and the damage "
            "mends a little.@n",
            TRUE, 0, 0, vict, TO_NOTVICT);
      } else if (char_condition_has(vict, "curse")) {
        send_to_char(vict, "@r...and your curse seems to have made the damage "
                           "three times worse!@n\r\n");
        act("@c...but @C$N's@c body glows red for a moment and the damage grow "
            "three times worse!@n",
            TRUE, 0, 0, vict, TO_NOTVICT);
      }
    } else {
      act("@WYour $p@W cracks a little!@n", FALSE, 0, eq, vict, TO_VICT);
      act("@C$N's@W $p@W cracks a little!@n", FALSE, 0, eq, vict, TO_NOTVICT);
      if (char_condition_has(vict, "bless")) {
        send_to_char(vict, "@c...But your blessing seems to have partly mended "
                           "this damage.@n\r\n");
        act("@c...but @C$N's@c body glows blue for a moment and the damage "
            "mends a little.@n",
            TRUE, 0, 0, vict, TO_NOTVICT);
      } else if (char_condition_has(vict, "curse")) {
        send_to_char(vict, "@r...and your curse seems to have made the damage "
                           "three times worse!@n\r\n");
        act("@c...but @C$N's@c body glows red for a moment and the damage grow "
            "three times worse!@n",
            TRUE, 0, 0, vict, TO_NOTVICT);
      }
    }
  }
}

/* This is for updating MOB android absorb */
static void update_mob_absorb_one(struct char_data *i) {
  struct char_data *vict;
  int roll = axion_dice(0) + (GET_LEVEL(i) * 0.25);

  {
    if (!IS_NPC(i))
      return;

    if (!IS_ANDROID(i))
      return;

    if (!MOB_FLAGGED(i, MOB_ABSORB))
      return;

    if (ABSORBING(i) == NULL || !ABSORBING(i))
      return;
    else if (GET_LEVEL(i) < roll)
      return;
    else if (ABSORBING(i)) {
      vict = ABSORBING(i);

      int ki = GET_MAX_MANA(vict) * 0.01;
      int stam = GET_MAX_MOVE(vict) * 0.01;
      int pl = GET_MAX_HIT(vict) * 0.01;
      int maxed = 0;

      if (roll < (GET_LEVEL(i) + 1) * 0.75) {
        ki += ki * rand_number(2, 4);
        stam += stam * rand_number(2, 4);
        pl += pl * rand_number(2, 4);
      }

      decCurHealth(vict, pl);
      incCurHealth(i, pl);

      decCurKI(vict, ki);
      incCurKI(i, ki);

      decCurST(vict, stam);
      incCurST(i, stam);

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
        act("@R$n@r absorbs the last of YOUR energy and you die...@n", TRUE, i,
            0, vict, TO_VICT);
        act("@R$n@r absorbs the last of @R$N's@r energy and $E dies...@n", TRUE,
            i, 0, vict, TO_NOTVICT);
        die(vict, i);
      } else if (maxed >= 3) {
        act("@R$n@r absorbs some of YOUR energy...but $e seems to be full now "
            "and releases YOU!@n",
            TRUE, i, 0, vict, TO_VICT);
        act("@R$n@r absorbs some of @R$N's@r energy...but $e seems to be full "
            "now and lets go.@n",
            TRUE, i, 0, vict, TO_NOTVICT);
      } else {
        act("@R$n@r absorbs some of YOUR energy!@n", TRUE, i, 0, vict, TO_VICT);
        act("@R$n@r absorbs some of @R$N's@r energy.@n", TRUE, i, 0, vict,
            TO_NOTVICT);
      }
    }
  }
}

void update_mob_absorb() {
  char_iterate_all([](struct char_data *i) {
    update_mob_absorb_one(i);
    return true;
  });
}

void huge_update() {
  /* clean up expired auction items parked in room 80 */
  obj_iterate_all([](struct obj_data *k) {
    if (GET_AUCTER(k) > 0 && GET_AUCTIME(k) + 604800 <= time(0) &&
        obj_room_vnum_get(k) == 80) {
      room_flag_set(obj_room_get(k), ROOM_HOUSE_CRASH, FALSE);
      extract_obj(k);
    }
    return true;
  });
}

/* shared deflect outcome for any homing projectile that is parried */

int handle_block(struct char_data *ch) {

  if (axion_dice(0) <= 4) { /* Critical failure */
    return (1);
  }

  if (!IS_NPC(ch)) { /* Players */
    if (!GET_SKILL(ch, SKILL_BLOCK)) {
      return (0);
    } else {
      int num = GET_SKILL(ch, SKILL_BLOCK);
      if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 3 || GET_GENOME(ch, 1) == 3)) {
        num += 10;
      }
      if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100) {
        num += 5;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 80) {
        num += 4;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60) {
        num += 3;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40) {
        num += 2;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 20) {
        num += 1;
      }
      return (num);
    }
  } else {                  /* Mobs */
    if (!IS_HUMANOID(ch)) { /* Animal/monster types */
      int top = GET_LEVEL(ch) / 4;

      if (top < 5)
        top = 6;
      return (rand_number(5, top));
    } else { /* Intelligent Skills Mobs */
      if (GET_LEVEL(ch) >= 110) {
        return (rand_number(95, 105));
      } else if (GET_LEVEL(ch) >= 100) {
        return (rand_number(85, 95));
      } else if (GET_LEVEL(ch) >= 90) {
        return (rand_number(70, 85));
      } else if (GET_LEVEL(ch) >= 75) {
        return (rand_number(50, 70));
      } else if (GET_LEVEL(ch) >= 40) {
        return (rand_number(40, 50));
      } else {
        int top = GET_LEVEL(ch);

        if (top < 15)
          top = 16;
        return (rand_number(15, top));
      }
    }
  }
}

int handle_dodge(struct char_data *ch) {

  if (axion_dice(0) <= 4) { /* Critical failure */
    return (1);
  }

  if (!IS_NPC(ch)) { /* Players */
    if (!GET_SKILL(ch, SKILL_DODGE)) {
      return (0);
    } else {
      int num = GET_SKILL(ch, SKILL_DODGE);
      if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 3 || GET_GENOME(ch, 1) == 3)) {
        num += 10;
      }
      if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100) {
        num += 5;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 80) {
        num += 4;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60) {
        num += 3;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40) {
        num += 2;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 20) {
        num += 1;
      }
      if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 100) {
        num += 3;
      } else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 75) {
        num += 2;
      } else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 50) {
        num += 1;
      }
      if (GET_SKILL_BASE(ch, SKILL_ROLL) >= 100) {
        num += 5;
      } else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 80) {
        num += 4;
      } else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 60) {
        num += 3;
      } else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 40) {
        num += 2;
      } else if (GET_SKILL_BASE(ch, SKILL_SURVIVAL) >= 20) {
        num += 1;
      }
      if (group_bonus(ch, 2) == 8) {
        num += num * 0.05;
      }
      return (num);
    }
  } else {                  /* Mobs */
    if (!IS_HUMANOID(ch)) { /* Animal/monster types */
      int top = (GET_LEVEL(ch) + 1) / 8;

      if (top < 5)
        top = 6;
      return (rand_number(5, top));
    } else { /* Intelligent Skills Mobs */
      if (GET_LEVEL(ch) >= 110) {
        return (rand_number(95, 105));
      } else if (GET_LEVEL(ch) >= 100) {
        return (rand_number(75, 95));
      } else if (GET_LEVEL(ch) >= 90) {
        return (rand_number(50, 85));
      } else if (GET_LEVEL(ch) >= 75) {
        return (rand_number(30, 70));
      } else if (GET_LEVEL(ch) >= 40) {
        return (rand_number(20, 50));
      } else {
        int top = GET_LEVEL(ch);

        if (top < 15)
          top = 16;
        return (rand_number(15, top));
      }
    }
  }
}

int check_def(struct char_data *vict) {
  int index = 0;
  int pry = handle_parry(vict), dge = handle_dodge(vict),
      blk = handle_block(vict);

  index = pry + dge + blk;

  if (index > 0)
    index /= 3;

  if (AFF_FLAGGED(vict, AFF_KNOCKED)) {
    index = 0;
  }
  return index;
}

void handle_defense(struct char_data *vict, int *pry, int *blk, int *dge) {

  if (!IS_NPC(vict)) {
    *pry = handle_parry(vict);

    *blk = handle_block(vict);

    *dge = handle_dodge(vict);

    if (GET_BONUS(vict, BONUS_WALL)) {
      *blk += GET_SKILL(vict, SKILL_BLOCK) * 0.20;
    }

    if (GET_BONUS(vict, BONUS_PUSHOVER)) {
      *blk -= GET_SKILL(vict, SKILL_BLOCK) * 0.20;
    }

    if (!GET_EQ(vict, WEAR_WIELD1) && !GET_EQ(vict, WEAR_WIELD2)) {
      *blk += 4;
    }

    if (*blk > 110) {
      *blk = 110;
    }

    if (GET_BONUS(vict, BONUS_EVASIVE)) {
      *dge += (GET_SKILL(vict, SKILL_DODGE) * 0.15);
    }

    if (GET_BONUS(vict, BONUS_PUNCHINGBAG)) {
      *dge -= GET_SKILL(vict, SKILL_DODGE) * 0.15;
    }

    if (*dge > 110) {
      *dge = 110;
    }

    if (*pry > 110) {
      *pry = 110;
    }
    if (PLR_FLAGGED(vict, PLR_GOOP) && rand_number(1, 100) >= 15) {
      *dge += 100;
      *blk += 100;
      *pry += 100;
    }
  } else {
    *pry = handle_parry(vict);

    *blk = handle_block(vict);

    *dge = handle_dodge(vict);
  }

  return;
}

static void damtype_unarmed_infuse(char_data *ch, int64_t *dam) {
  if (AFF_FLAGGED(ch, AFF_INFUSE)) {
    *dam += (*dam / 100) * (GET_SKILL(ch, SKILL_INFUSE) / 2);
    if (IS_JINTO(ch)) {
      if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100) {
        *dam += ((*dam * 0.01) * (GET_SKILL(ch, SKILL_INFUSE) / 2)) * 0.5;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60) {
        *dam += ((*dam * 0.01) * (GET_SKILL(ch, SKILL_INFUSE) / 2)) * 0.25;
      } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40) {
        *dam += ((*dam * 0.01) * (GET_SKILL(ch, SKILL_INFUSE) / 2)) * 0.05;
      }
    }
  }
}

static void damtype_unarmed_hasshuken(char_data *ch, int64_t *dam) {
  if (char_condition_has(ch, "hasshuken")) {
    *dam *= 2;
    if (IS_KRANE(ch)) {
      if (GET_SKILL(ch, SKILL_HASSHUKEN) >= 100) {
        *dam += *dam * 0.3;
      } else if (GET_SKILL(ch, SKILL_HASSHUKEN) >= 60) {
        *dam += *dam * 0.2;
      } else if (GET_SKILL(ch, SKILL_HASSHUKEN) >= 40) {
        *dam += *dam * 0.1;
      }
    }
  }
}

static void damtype_unarmed_hasshuken_or_infuse(char_data *ch, int64_t *dam) {
  if (char_condition_has(ch, "hasshuken")) {
    damtype_unarmed_hasshuken(ch, dam);
  } else {
    damtype_unarmed_infuse(ch, dam);
  }
}

static void damtype_unarmed_preference(char_data *ch, int64_t *dam) {
  if (GET_PREFERENCE(ch) == PREFERENCE_THROWING) {
    *dam -= *dam * 0.15;
  } else if (GET_PREFERENCE(ch) == PREFERENCE_H2H) {
    *dam += *dam * 0.20;
  }
}

static void damtype_focus(char_data *ch, int64_t *dam, int64_t focus,
                          int divby) {
  if (focus > 0) {
    *dam += focus * (*dam / divby);
  }
}

static void damtype_unarmed(char_data *ch, int skill, int64_t *dam) {
  // General Arlian bonus.
  if (IS_ARLIAN(ch)) {
    *dam += *dam * 0.2;
  }

  // Sixteen's Iron Hand Bonus.
  if (IS_ANDSIX(ch)) {
    if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
      *dam += *dam * 0.1;
  }

  // Brawler bonus
  if (GET_BONUS(ch, BONUS_BRAWLER) > 0) {
    *dam += *dam * .2;
  }

  switch (skill) {
  // Punch.
  case 0:
    damtype_unarmed_hasshuken_or_infuse(ch, dam);
    // Kame Arts bonus.
    if (IS_ROSHI(ch)) {
      if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
        *dam += *dam * 0.2;
    }
    break;
  // Kick
  case 1:
    damtype_unarmed_infuse(ch, dam);
    // Crane Arts
    if (IS_KRANE(ch)) {
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

static void damtype_human_grandmaster(char_data *ch, int skill, int64_t *dam) {
  if (IS_HUMAN(ch)) {
    switch (skill) {
    case 101:
      *dam = *dam * 1.1;
      break;
    case 102:
      *dam = *dam * 1.2;
      break;
    case 103:
      *dam = *dam * 1.3;
      break;
    }
  }
}

static void damtype_human_ki(char_data *ch, int64_t *dam, int bon) {
  if (IS_HUMAN(ch)) {
    *dam += (*dam / 100) * bon;
  }
}

static void damtype_saiyan_ki(char_data *ch, int64_t *dam, int bon) {
  if (IS_SAIYAN(ch)) {
    *dam += (*dam / 100) * bon;
  }
}

static void damtype_kai_ki(char_data *ch, int64_t *dam, int bon) {
  if (IS_KAI(ch)) {
    *dam += (*dam / 100) * bon;
  }
}

static void damtype_icer_ki(char_data *ch, int64_t *dam, int bon) {
  if (IS_ICER(ch) ||
      (IS_BIO(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4))) {
    *dam += (*dam / 100) * bon;
  }
}

/* Damage for player and NPC attacks  */
int64_t damtype(struct char_data *ch, int type, int skill, double percent) {
  int64_t dam = 0, cou1 = 0, cou2 = 0, focus = 0;

  /* Player damages based on attack */
  if (!IS_NPC(ch)) {
    if (GET_SKILL(ch, SKILL_FOCUS)) {
      focus = GET_SKILL(ch, SKILL_FOCUS);
    }
    if (type != -2) {
      LASTATK(ch) = type;
    } else {
      type = 0;
    }

    // split switch approach to compress code.
    switch (type) {
    // big list of h2h stuff for starters.
    case -1:
      cou1 = 1 + ((skill / 4) * ((GET_HIT(ch) / 1200) + GET_STR(ch)));
      cou2 = 1 + ((skill / 4) * ((GET_HIT(ch) / 1000) + GET_STR(ch)));
      break;
    case 0: /* Punch */
      cou1 = 15 + ((skill / 4) * ((GET_HIT(ch) / 1600) + GET_STR(ch)));
      cou2 = 15 + ((skill / 4) * ((GET_HIT(ch) / 1300) + GET_STR(ch)));
      break;
    case 1: /* Kick */
      cou1 = 40 + ((skill / 4) * ((GET_HIT(ch) / 1200) + GET_STR(ch)));
      cou2 = 40 + ((skill / 4) * ((GET_HIT(ch) / 1000) + GET_STR(ch)));
      break;
    case 2: /* Elbow */
      cou1 = 100 + ((skill / 4) * ((GET_HIT(ch) / 1300) + GET_STR(ch)));
      cou2 = 100 + ((skill / 4) * ((GET_HIT(ch) / 1050) + GET_STR(ch)));
      break;
    case 3: /* Knee */
      cou1 = 150 + ((skill / 4) * ((GET_HIT(ch) / 1100) + GET_STR(ch)));
      cou2 = 150 + ((skill / 4) * ((GET_HIT(ch) / 1000) + GET_STR(ch)));
      break;
    case 4: /* Roundhouse */
      cou1 = 500 + ((skill / 4) * ((GET_HIT(ch) / 1000) + GET_STR(ch)));
      cou2 = 500 + ((skill / 4) * ((GET_HIT(ch) / 800) + GET_STR(ch)));
      break;
    case 5: /* Uppercut */
      cou1 = 350 + ((skill / 4) * ((GET_HIT(ch) / 1100) + GET_STR(ch)));
      cou2 = 350 + ((skill / 4) * ((GET_HIT(ch) / 900) + GET_STR(ch)));
      break;
    case 6: /* Slam */
      cou1 = 8000 + ((skill / 4) * ((GET_HIT(ch) / 800) + GET_STR(ch)));
      cou2 = 8000 + ((skill / 4) * ((GET_HIT(ch) / 500) + GET_STR(ch)));
      break;
    case 8: /* Heeldrop */
      cou1 = 12500 + ((skill / 4) * ((GET_HIT(ch) / 700) + GET_STR(ch)));
      cou2 = 12500 + ((skill / 4) * ((GET_HIT(ch) / 400) + GET_STR(ch)));
      break;
    case 51: /* Bash */
      cou1 = 1000 + ((skill / 4) * ((GET_HIT(ch) / 700) + GET_STR(ch)));
      cou2 = 1000 + ((skill / 4) * ((GET_HIT(ch) / 550) + GET_STR(ch)));
      break;
    case 52: /* Headbutt */
      cou1 = 800 + ((skill / 4) * ((GET_HIT(ch) / 900) + GET_STR(ch)));
      cou2 = 800 + ((skill / 4) * ((GET_HIT(ch) / 650) + GET_STR(ch)));
      break;
    case 56: /* TAILWHIP */
      cou1 = 400 + ((skill / 4) * ((GET_HIT(ch) / 1100) + GET_STR(ch)));
      cou2 = 400 + ((skill / 4) * ((GET_HIT(ch) / 1000) + GET_STR(ch)));
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
      dam = large_rand(cou1, cou2);
      dam += GET_STR(ch) * (dam * 0.005);
      break;
    case 51:
    case 52:
    case 56: /* TAILWHIP */
      dam = large_rand(cou1, cou2);
      dam += GET_LEVEL(ch) * 100;
      dam += GET_STR(ch) * (dam * 0.005);
      break;
    default: // all ki abilities.
      dam = GET_MAX_MANA(ch) * percent;
      ki = true;
    }

    // ki type move pre-processing
    switch (type) {
    case 11: /* Tsuihidan */
    case 12: /* Renzo */
    case 23: /* Rogafufuken */
    case 25: /* Kienzan */
      dam += GET_LEVEL(ch) * 500;
      break;
    case 13: /* Kamehameha */
    case 16: /* Galik Gun */
    case 26: /* Tribeam */
    case 50: /* Seishou Enko */
      dam += GET_LEVEL(ch) * 800;
      break;
    case 14: /* Masenko */
    case 30: /* Darkness Dragon Slash */
    case 44: /* Spiral Comet 1 */
    case 45: /* Spiral Comet 2 */
    case 43: /* Water Spikes */
      dam += GET_LEVEL(ch) * 1000;
      break;
    case 15: /* Dodonpa */
    case 17: /* Deathbeam */
    case 19: /* Twin Slash */
      dam += GET_LEVEL(ch) * 650;
      break;
    case 18: /* Eraser Cannon */
    case 33: /* Hell Spear Blast */
    case 54: /* Zen Blade */
    case 55: /* Sundering Force */
      dam += GET_LEVEL(ch) * 700;
      break;
    case 20: /* Psychic Blast */
    case 27: /* Special Beam Cannon */
    case 29: /* Crusher Ball */
    case 37: /* Phoenix Slash */
      dam += GET_LEVEL(ch) * 1200;
      break;
    case 21: /* Honoo */
    case 39: /* Spirit ball */
    case 47: /* Water Razor */
    case 48: /* Koteiru Bakuha */
    case 49: /* Hell Spiral */
      dam += GET_LEVEL(ch) * 900;
      break;
    case 22: /* Dual Beam */
    case 24: /* Bakuhatsuha */
      dam += GET_LEVEL(ch) * 600;
      break;
    case 28: /* Final Flash */
      dam += GET_LEVEL(ch) * 1500;
    case 31: /* Psychic Barrage */
    case 36: /* Big Bang */
      dam += GET_LEVEL(ch) * 1100;
      break;
    case 32: /* Hell Flash */
    case 46: /* Star Breaker */
      dam += GET_LEVEL(ch) * 1400;
      break;
    case 34: /* Kakusanha */
      dam += GET_LEVEL(ch) * 1050;
      break;
    case 35: /* Scatter Shot */
    case 53: /* Star Nova */
      dam += GET_LEVEL(ch) * 1600;
      break;
    case 38: /* Deathball */
      dam += GET_LEVEL(ch) * 1700;
      break;
    case 40: /* Genki Dama */
    case 41: /* Genocide */
      dam += GET_LEVEL(ch) * 2000;
      break;
    case 42: /* Kousengan */
      dam += GET_LEVEL(ch) * 550;
      break;
    case 57: /* Light Grenade */
      dam += GET_LEVEL(ch) * 1700;
      break;
    }

    if (ki)
      dam *= 1.25;

    switch (type) {
    case -1:
      if (!PLR_FLAGGED(ch, PLR_THANDW))
        damtype_unarmed_hasshuken_or_infuse(ch, &dam);

      if (GET_BONUS(ch, BONUS_BRAWLER) > 0) {
        dam += dam * .2;
      }
      if (GET_PREFERENCE(ch) == PREFERENCE_KI) {
        dam -= dam * 0.20;
      }
      if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON &&
          GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.05) {
        dam += GET_MAX_MANA(ch) * 0.05;
        char_charge_set(ch, GET_CHARGE(ch) - (int64_t)(GET_MAX_MANA(ch) * 0.05));
      } else if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON &&
                 GET_CHARGE(ch) > 0) {
        dam += GET_CHARGE(ch);
        char_charge_set(ch, 0);
      }
      if (group_bonus(ch, 2) == 8) {
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
      dam += (dam / 100) * GET_STR(ch);
      damtype_focus(ch, &dam, focus, 200);
      if (GET_BONUS(ch, BONUS_BRAWLER) > 0) {
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

      if (!IS_NPC(ch) && percent > 0.15) {
        double hitperc = (percent - 0.15) * 5;
        int64_t amount = getMaxPL(ch) * hitperc;
        int64_t difference = GET_HIT(ch) - amount;

        decCurHealthFloored(ch, amount, 1);

        damtype_focus(ch, &dam, focus, 200);
      } else {
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
      if (!IS_NPC(ch)) {
        if (PLR_FLAGGED(ch, PLR_TRANS6)) {
          dam += dam;
        } else if (PLR_FLAGGED(ch, PLR_TRANS5)) {
          dam += (dam * 0.01) * 75;
        } else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
          dam += (dam * 0.01) * 50;
        } else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
          dam += (dam * 0.01) * 25;
        } else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
          dam += (dam * 0.01) * 15;
        } else if (PLR_FLAGGED(ch, PLR_TRANS1)) {
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
    dam = (GET_HIT(ch) * 0.05) + (GET_MAX_HIT(ch) * 0.025);
    dam += (dam * 0.005) * GET_STR(ch);
    if (GET_LEVEL(ch) >= 120) {
      dam *= 0.25;
    } else if (GET_LEVEL(ch) >= 110) {
      dam *= 0.45;
    } else if (GET_LEVEL(ch) >= 100) {
      dam *= 0.75;
    }
  }

  if (IS_NPC(ch)) {

    if (type == 0 || type == 1 || type == 2 || type == 3 || type == 4 ||
        type == 5 || type == 6 || type == 8 || type == 51 || type == 52 ||
        type == 56) {
      dam += GET_STR(ch) * (dam * 0.005);
    } else {
      dam += GET_INT(ch) * (dam * 0.005);
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

    if (GET_CLASS(ch) != CLASS_NPC_COMMONER) {
      dam += dam * 0.3;
    }
  }

  if (GET_KAIOKEN(ch) > 0) {
    dam += (dam / 200) * GET_KAIOKEN(ch);
  }

  /* Start of Fury Mode for halfbreeds */
  if (char_condition_has(ch, "halfbreed_fury") &&
      (type == 0 || type == 1 || type == 2 || type == 3 || type == 4 ||
       type == 5 || type == 6 || type == 8 || type == 51 || type == 52)) {
    dam *= 1.5;
    act("Your rage magnifies your attack power!", TRUE, ch, 0, 0, TO_CHAR);
    act("Swirling energy flows around $n as $e releases $s rage in the attack!",
        TRUE, ch, 0, 0, TO_ROOM);
    if (rand_number(1, 10) >= 7) {
      send_to_char(ch, "You feel less angry.\r\n");
      char_condition_remove(ch, "halfbreed_fury", "spent");
    }
  } else if (char_condition_has(ch, "halfbreed_fury")) {
    dam *= 2;
    act("Your rage magnifies your attack power!", TRUE, ch, 0, 0, TO_CHAR);
    act("Swirling energy flows around $n as $e releases $s rage in the attack!",
        TRUE, ch, 0, 0, TO_ROOM);
    char_condition_remove(ch, "halfbreed_fury", "spent");
  }
  /* End of Fury Mode for halfbreeds */

  if ((type == -1 || type == 0 || type == 1 || type == 2 || type == 3 ||
       type == 4 || type == 5 || type == 6 || type == 8)) {
    if (!IS_NPC(ch))
      dam -= dam * 0.08;
    if (!IS_NPC(ch) && dam > GET_MAX_HIT(ch) * 0.1)
      dam *= 0.6;
  } else {
    dam += (dam * 0.005) * GET_INT(ch);
    if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON) {
      dam -= dam * 0.25;
    } else if (GET_PREFERENCE(ch) == PREFERENCE_THROWING) {
      dam -= dam * 0.15;
    }
  }

  return (dam);
}

void saiyan_gain(struct char_data *ch, struct char_data *vict) {
  int gain = rand_number(GET_LEVEL(ch) * 6, GET_LEVEL(ch) * 8);
  int weak = FALSE;

  if (!vict)
    return;

  if (IS_NPC(ch))
    return;

  if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) / 10) {
    weak = TRUE;
  }

  if (GET_LEVEL(ch) > 99) {
    gain += rand_number(GET_LEVEL(ch) * 300, GET_LEVEL(ch) * 500);
  } else if (GET_LEVEL(ch) > 80) {
    gain += rand_number(GET_LEVEL(ch) * 150, GET_LEVEL(ch) * 200);
  } else if (GET_LEVEL(ch) > 60) {
    gain += rand_number(GET_LEVEL(ch) * 80, GET_LEVEL(ch) * 100);
  } else if (GET_LEVEL(ch) > 50) {
    gain += rand_number(GET_LEVEL(ch) * 20, GET_LEVEL(ch) * 25);
  } else if (GET_LEVEL(ch) > 40) {
    gain += rand_number(GET_LEVEL(ch) * 8, GET_LEVEL(ch) * 10);
  } else if (GET_LEVEL(ch) > 30) {
    gain += rand_number(GET_LEVEL(ch) * 5, GET_LEVEL(ch) * 8);
  } else {
  }

  if (IS_BIO(ch) && (GET_GENOME(ch, 0) == 2 || GET_GENOME(ch, 1) == 2)) {
    gain /= 2;
  }
  if (rand_number(1, 22) >= 18 &&
      (GET_LEVEL(ch) == 100 ||
       level_exp(ch, GET_LEVEL(ch) + 1) - (GET_EXP(ch)) > 0)) {
    if (weak) {
      send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WThey are too weak to inspire "
                       "your saiyan soul!@n\r\n");
    } else {
      int stats[3] = {0, 1, 2};
      int available[3];
      int avail_count = 0;

      for (int i = 0; i < 3; i++) {
        if (!is_soft_cap_mult(ch, stats[i], 1.5)) {
          available[avail_count++] = stats[i];
        }
      }

      if (avail_count == 0) {
        send_to_char(ch, "@D[@YSaiyan @RBlood@D] @WYou feel you have reached "
                         "your current limits.@n\r\n");
        return;
      }

      int choice = available[rand_number(0, avail_count - 1)];

      switch (choice) {
      case 0:
        gainBasePL(ch, gain);
        send_to_char(ch,
                     "@D[@YSaiyan @RBlood@D] @WYou feel slightly stronger. "
                     "@D[@G+%s@D]@n\r\n",
                     add_commas(gain));
        break;
      case 1:
        gainBaseKI(ch, gain);
        send_to_char(ch,
                     "@D[@YSaiyan @RBlood@D] @WYou feel your spirit grow. "
                     "@D[@G+%s@D]@n\r\n",
                     add_commas(gain));
        break;
      case 2:
        gainBaseST(ch, gain);
        send_to_char(ch,
                     "@D[@YSaiyan @RBlood@D] @WYou feel slightly more "
                     "vigorous. @D[@G+%s@D]@n\r\n",
                     add_commas(gain));
        break;
      }
    }
  }
}

void spar_gain(struct char_data *ch, struct char_data *vict, int type,
               int64_t dmg) {
  int chance = 0;
  int64_t vitalgain = 0, gain = 0, pl = 0, ki = 0, st = 0, gaincalc = 0;

  if (ch == NULL)
    return;
  if (vict == NULL)
    return;
  if (IS_NPC(ch))
    return;

  if (dmg > GET_MAX_HIT(vict) / 10) {
    chance = rand_number(20, 100);
  } else if (dmg <= GET_MAX_HIT(vict) / 10) {
    chance = rand_number(1, 75);
  }

  // Work out xp gained
  int64_t victavg = (getMaxPL(vict) + getMaxKI(vict) + getMaxST(vict)) / 3;
  int victimStrength = get_digits(victavg);

  if (chance >= rand_number(60, 75)) {
    int64_t num = 0, maxnum = 1000000;
    if (victimStrength >= 13) {
      num += GET_LEVEL(ch) * 7000;
    } else if (victimStrength >= 12) {
      num += GET_LEVEL(ch) * 6500;
    } else if (victimStrength >= 11) {
      num += GET_LEVEL(ch) * 6000;
    } else if (victimStrength >= 10) {
      num += GET_LEVEL(ch) * 5500;
    } else if (victimStrength >= 9) {
      num += GET_LEVEL(ch) * 5000;
    } else if (victimStrength >= 8) {
      num += GET_LEVEL(ch) * 4500;
    } else if (victimStrength >= 7) {
      num += GET_LEVEL(ch) * 4000;
    } else if (victimStrength >= 6) {
      num += GET_LEVEL(ch) * 3000;
    } else if (victimStrength >= 5) {
      num += GET_LEVEL(ch) * 1500;
    } else if (victimStrength >= 4) {
      num += GET_LEVEL(ch) * 500;
    } else if (victimStrength >= 3) {
      num += GET_LEVEL(ch) * 200;
    } else if (victimStrength >= 2) {
      num += GET_LEVEL(ch) * 100;
    } else {
      num += GET_LEVEL(ch) * 50;
    }

    if (num > maxnum) {
      num = maxnum;
    }

    gaincalc = large_rand(num * 0.7, num * 1.2);

    if (vict != NULL && (is_sparring(ch) && is_sparring(vict)))
      gaincalc *= 0.8;

    if (IS_SAIYAN(ch)) {
      gaincalc = gaincalc + (gaincalc * .50);
    }
    if (IS_HALFBREED(ch)) {
      gaincalc = gaincalc + (gaincalc * .40);
    }
    if (IS_ICER(ch) ||
        (IS_BIO(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4))) {
      gaincalc = gaincalc - (gaincalc * .20);
    }
    if (room_flagged(char_room_get(ch), ROOM_WORKOUT) ||
        (room_flagged(char_room_get(ch), ROOM_HBTC))) {
      if (char_room_vnum_get(ch) >= 19100 && char_room_vnum_get(ch) <= 19199) {
        gaincalc *= 1.5;
      } else {
        gaincalc *= 1.25;
      }
    }

    gain_exp(ch, gaincalc);
    send_to_char(ch, "@D[@Y+ @G%s @mExp@D]@n ", add_commas(gain));

    // Work out Vitals gained based on damage dealt
    if (GET_LEVEL(ch) >= 100) {
      vitalgain = dmg / 10000;

      if (GET_EQ(ch, WEAR_SH)) {
        struct obj_data *obj = GET_EQ(ch, WEAR_SH);
        if (GET_OBJ_VNUM(obj) == 1127) {
          vitalgain *= 4;
        }
      }

      if ((char_room_get(ch) &&
           room_flagged(char_room_get(ch), ROOM_WORKOUT)) ||
          ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_HBTC)))) {
        if (char_room_vnum_get(ch) >= 19100 &&
            char_room_vnum_get(ch) <= 19199) {
          vitalgain *= 1.75;
        } else {
          vitalgain *= 1.25;
        }
        pl = large_rand(vitalgain * .8, vitalgain * 1.2);
        ki = large_rand(vitalgain * .8, vitalgain * 1.2);
      } else {
        pl = large_rand(vitalgain * .4, vitalgain * .8);
        ki = large_rand(vitalgain * .4, vitalgain * .8);
      }
      if (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) < 0 &&
          GET_LEVEL(ch) < 100) {
        pl = 0;
      }
      if (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) < 0 &&
          GET_LEVEL(ch) < 100) {
        ki = 0;
      }

      if (type == 0 && rand_number(1, 5) >= 4) {
        send_to_char(ch, "@D[@Y+ @R%s @rPL@D]@n ",
                     pl > 0 ? add_commas(pl) : "SOFT-CAP");
        gainBasePL(ch, pl);
      } else if (type == 1 && rand_number(1, 5) >= 4) {
        send_to_char(ch, "@D[@Y+ @C%s @cKi@D]@n ",
                     ki > 0 ? add_commas(ki) : "SOFT-CAP");
        gainBaseKI(ch, ki);
      }
    }
    send_to_char(ch, "\r\n");
  }
}

/*Get the amount of digits within a number*/
int get_digits(int64_t n) {
  int count = 1;

  while (n <= -10 || n >= 10) {
    n /= 10;
    count++;
  }
  return count;
}

/* Main damage function for RDBS 'Real Dragonball Battle System' */
void hurt(int limb, int chance, struct char_data *ch, struct char_data *vict,
          struct obj_data *obj, int64_t dmg, int type) {
  int64_t index = 0;
  int64_t maindmg = dmg, beforered = dmg;
  int dead = FALSE;

  if (type <= 0) {
    if (IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_STAIL))
      dmg += dmg * .15;
    if (IS_NAMEK(ch) && !GET_EQ(ch, WEAR_HEAD))
      dmg += dmg * .25;
    if (group_bonus(ch, 2) == 4)
      dmg += dmg * .1;
    else if (group_bonus(ch, 2) == 12)
      dmg -= dmg * .1;
  } else {
    /* human racial bonus on hold */
    /*if (IS_HUMAN(ch) && !IS_NPC(ch)) {
     if (PLR_FLAGGED(ch, PLR_TRANS3)) {
      dmg += dmg * 0.45;
     }
    }*/
    dmg = dmg * .6;
    if (group_bonus(ch, 2) == 9)
      dmg -= dmg * 0.1;
    if (char_condition_has(ch, "rune_purisaz")) {
      dmg += dmg * 0.3;
      send_to_room(char_room_get(ch), "@wThere is a bright flash of @Yyellow@w "
                                      "light in the wake of the attack!@n\r\n");
    }
  }

  if (AFF_FLAGGED(ch, AFF_INFUSE) && !char_condition_has(ch, "hasshuken") && type <= 0) {
    int64_t infuse_cost = getPercentOfMaxKI(ch, .005);
    if (dmg > 0) {
      if (getCurKI(ch) - infuse_cost) {
        decCurKI(ch, infuse_cost);
        send_to_room(char_room_get(ch),
                     "@CA swirl of ki explodes from the attack!@n\r\n");
      } else {
        act("@wYou can no longer infuse ki into your attacks!@n", TRUE, ch, 0,
            0, TO_CHAR);
        act("@c$n@w can no longer infuse ki into $s attacks!@n", TRUE, ch, 0, 0,
            TO_ROOM);
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INFUSE);
      }
    }
  }

  if (vict) {

    if (char_room_vnum_get(vict) == 17875)
      return;

    reveal_hiding(vict, 0);
    if (AFF_FLAGGED(vict, AFF_PARALYZE)) {
      send_to_char(ch, "They are a statue and can't be harmed\r\n");
      return;
    }

    if (GET_KAIOKEN(ch) > 0)
      dmg += (dmg / 100) * (GET_KAIOKEN(ch) * 2);

    if (IS_MUTANT(vict) &&
        (GET_GENOME(vict, 0) == 8 || GET_GENOME(vict, 1) == 8) && type == 0) {
      int64_t drain = dmg * 0.1;
      dmg -= drain;
      decCurSTFloored(ch, drain, 1);
      act("@Y$N's rubbery body makes hitting it tiring!@n", TRUE, ch, 0, vict,
          TO_CHAR);
      act("@Y$n's stamina is sapped a bit by hitting your rubbery body!@n",
          TRUE, ch, 0, vict, TO_VICT);
    }

    if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_OOZARU))
      dmg += dmg * 0.30;
    if (!IS_NPC(vict) && PLR_FLAGGED(vict, PLR_OOZARU))
      dmg -= dmg * 0.30;

    if (type > -1) {
      if (LASTATK(ch) != 11 && LASTATK(ch) != 39 && LASTATK(ch) != 500 &&
          LASTATK(ch) < 1000) {
        if (handle_combo(ch, vict) > 0) {
          auto style_gain = [&](int64_t hits) {
            if ((hits == 10 || hits == 20 || hits == 30) &&
                (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 ||
                 GET_LEVEL(ch) == 100)) {
              int64_t gain = GET_LEVEL(ch) * 1000;
              auto sk_style = GET_SKILL(ch, SKILL_STYLE);
              if (sk_style >= 100)      gain += gain * 2;
              else if (sk_style >= 80)  gain += gain * 0.4;
              else if (sk_style >= 60)  gain += gain * 0.3;
              else if (sk_style >= 40)  gain += gain * 0.2;
              else if (sk_style >= 20)  gain += gain * 0.1;
              gain_exp(ch, gain);
              send_to_char(ch, "@D[@mExp@W: @G%s@D]@n\r\n", add_commas(gain));
            }
          };
          if (beforered <= 1) {
            char_condition_remove(ch, "combo", "end_combo");
            send_to_char(ch, "@RYou have cut your combo short because you "
                             "missed your last hit!@n\r\n");
          } else if (auto hits = char_condition_number_get(ch, "combo", "hits"); hits < physical_mastery(ch)) {
            dmg += combo_damage(ch, dmg, 0);
            style_gain(hits);
          } else {
            dmg += combo_damage(ch, dmg, 1);
            style_gain(char_condition_number_get(ch, "combo", "hits"));
            char_condition_remove(ch, "combo", "end_combo");
          }
        }
      } else if (auto hits = char_condition_number_get(ch, "combo", "hits"); hits > 0 && LASTATK(ch) < 1000) {
        send_to_char(ch, "@RYou have cut your combo short because you used the "
                         "wrong attack!@n\r\n");
        char_condition_remove(ch, "combo", "end_combo");
      }
    }

    if (LASTATK(ch) >= 1000)
      LASTATK(ch) -= 1000;

    if (GET_PREFERENCE(vict) == PREFERENCE_KI && GET_CHARGE(vict) > 0)
      dmg -= dmg * 0.08;

    if (AFF_FLAGGED(vict, AFF_SANCTUARY)) {
      if (GET_SKILL(vict, SKILL_AQUA_BARRIER)) {
        if (!room_is_sunken(char_room_get(ch))) {
          dmg = dmg * 0.85;
        } else {
          dmg = dmg * 0.75;
        }
      }
      if (GET_BARRIER(vict) - dmg > 0) {
        act("@c$N's@C barrier absorbs the damage!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        char barr[MAX_INPUT_LENGTH];
        sprintf(barr, "@CYour barrier absorbs the damage! @D[@B%s@D]@n",
                add_commas(dmg));
        act(barr, TRUE, ch, 0, vict, TO_VICT);
        act("@c$N's@C barrier absorbs the damage!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        char_barrier_set(vict, GET_BARRIER(vict) - (int64_t)dmg);
        dmg = 0;
      } else if (GET_BARRIER(vict) - dmg <= 0) {
        dmg -= GET_BARRIER(vict);
        char_barrier_set(vict, 0);
        act("@c$N's@C barrier bursts!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@CYour barrier bursts!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N's@C barrier bursts!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        char_condition_remove(vict, "barrier", "burst");
      }
    }
    if (AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
        rand_number(1, 200) < GET_SKILL(vict, SKILL_FIRESHIELD)) {
      act("@c$N's@C fireshield repels the damage!@n", TRUE, ch, 0, vict,
          TO_CHAR);
      act("@CYour fireshield repels the damage!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@c$N's@C fireshield repels the damage!@n", TRUE, ch, 0, vict,
          TO_NOTVICT);
      if (rand_number(1, 3) == 3) {
        act("@c$N's@C fireshield disappears...@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@CYour fireshield disappears...@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N's@C fireshield disappears...@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        char_condition_remove(vict, "fireshield", "blocked");
      }
      dmg = 0;
    }

    if (type == 0) {
      int64_t vpl = GET_MAX_HIT(vict);
      int vcon = GET_CON(vict);
      int64_t conlimit = 2000000000;
      static const struct { int64_t mult; int divisor; } con_tiers[] = {
        { 1, 1500}, { 2, 2500}, { 3, 3500}, { 5, 6000},
        {10, 8500}, {15,10000}, {20,12500}, {25,16000}, {30,22000},
      };
      bool con_set = false;
      for (auto &ct : con_tiers) {
        if (vpl < conlimit * ct.mult) {
          index += (vpl / ct.divisor) * (vcon / 2);
          con_set = true;
          break;
        }
      }
      if (!con_set && vpl > conlimit * 30)
        index += (vpl / 25000) * (vcon / 2);
    }

    if (IS_NPC(vict))
      index /= 3;

    index += armor_calc(vict, dmg, type);

    if (char_condition_has(vict, "stoneskin")) {
      static const struct { int limit; int mult; } stone_tiers[] = {
        {20, 250}, {30, 500}, {50, 1000}, {60, 2000},
        {70, 5000}, {90, 10000}, {101, 25000},
      };
      for (auto &st : stone_tiers) {
        if (GET_LEVEL(vict) < st.limit) {
          index += GET_LEVEL(vict) * st.mult;
          break;
        }
      }
    }

    if (AFF_FLAGGED(vict, AFF_SHELL))
      dmg = dmg * 0.75;
    if (char_condition_has(vict, "wither"))
      dmg += (dmg * 0.01) * 20;
    if (!IS_NPC(vict) && char_stat_get(vict, "drunk") > 4)
      dmg -= (dmg * 0.001) * char_stat_get(vict, "drunk");
    if (char_condition_has(vict, "ethereal_armor"))
      dmg -= dmg * 0.1;
    if (type > 0) {
      advanced_energy(vict, dmg);
      dmg -= (dmg * 0.0005) * GET_WIS(vict);
    }
    if (GET_BONUS(vict, BONUS_LEECH) && type > 0)
      dmg -= dmg * (((GET_LEVEL(vict) / 5) * 0.02));
    if (GET_BONUS(vict, BONUS_FIREPROOF) && type > 0)
      dmg -= dmg * 0.1;

    if (GET_BONUS(vict, BONUS_THICKSKIN)) {
      if (type <= 0)
        dmg -= dmg * 0.20;
      else
        dmg -= dmg * 0.10;
    } else if (GET_BONUS(vict, BONUS_THINSKIN)) {
      if (type <= 0)
        dmg += dmg * 0.20;
      else
        dmg += dmg * 0.10;
    }

    if (char_condition_has(vict, "halfbreed_fury"))
      dmg -= dmg * 0.1;
    if (IS_MUTANT(vict)) {
      if (type <= 0)
        dmg -= dmg * 0.3;
      else
        dmg -= dmg * 0.25;
    }
    if (IS_MAJIN(vict) && type <= 0)
      dmg -= dmg * 0.5;
    if (IS_KAI(vict))
      dmg += dmg * 0.15;
    if (GRAPPLING(ch) == vict && GRAPTYPE(ch) == 3)
      dmg += (dmg / 100) * 20;

    if (GET_CLAN(vict) != NULL &&
        !strcasecmp(GET_CLAN(vict), "Heavenly Kaios")) {
      if ((getCurKI(vict)) >= GET_MAX_MANA(vict) / 2) {
        dmg -= (dmg / 100) * 20;
        act("@wYou are covered in a pristine @Cglow@w.@n", TRUE, vict, 0, 0,
            TO_CHAR);
        act("@w$n is covered in a pristine @Cglow@w!@n", TRUE, vict, 0, 0,
            TO_ROOM);
      }
    }

    if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_ARMOR)) {
      int nanite = GET_SKILL(vict, SKILL_ARMOR);
      int perc = PLR_FLAGGED(vict, PLR_SENSEM) ? rand_number(1, 176) : rand_number(1, 220);
      if (nanite >= perc) {
        static const struct { int flag; int pct; const char *word; } armor_tiers[] = {
          {PLR_TRANS6, 50, "MOST"},
          {PLR_TRANS5, 40, "some"},
          {PLR_TRANS4, 30, "a lot"},
          {PLR_TRANS3, 25, "a good deal"},
          {PLR_TRANS2, 20, "some"},
          {PLR_TRANS1, 10, "a bit"},
          {0,           5, "a tiny bit"},
        };
        char buf_ch[256], buf_room[256];
        for (auto &at : armor_tiers) {
          if (at.flag == 0 || PLR_FLAGGED(vict, at.flag)) {
            snprintf(buf_ch, sizeof(buf_ch),
                     "@WYour @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block "
                     "%s of the damage!@n", at.word);
            snprintf(buf_room, sizeof(buf_room),
                     "@W$n's @gn@Ga@Wn@wite @Da@Wr@wm@Do@wr@W reacts in time to block "
                     "%s of the damage!@n", at.word);
            act(buf_ch, TRUE, vict, 0, 0, TO_CHAR);
            act(buf_room, TRUE, vict, 0, 0, TO_ROOM);
            dmg -= (dmg * 0.01) * at.pct;
            break;
          }
        }
      }
    }

    if (!AFF_FLAGGED(vict, AFF_KNOCKED) &&
        (GET_POS(vict) == POS_SITTING || GET_POS(vict) == POS_RESTING) &&
        GET_SKILL(vict, SKILL_ROLL) > axion_dice(0)) {
      int64_t rollcost = getMaxHealth(vict) / 300 * (GET_STR(ch) / 2);
      if ((getCurST(vict)) >= rollcost) {
        act("@GYou roll to your feet in an agile fashion!@n", TRUE, vict, 0, 0,
            TO_CHAR);
        act("@G$n rolls to $s feet in an agile fashion!@n", TRUE, vict, 0, 0,
            TO_ROOM);
        char_cmd_execute(vict, "stand", NULL);
        decCurST(vict, rollcost);
      }
    }

    if (IS_NPC(vict))
      hitprcnt_mtrigger(vict);

    if (IS_HUMANOID(vict) && !IS_NPC(ch) && IS_NPC(vict) &&
        (!is_sparring(ch) || !is_sparring(vict)))
      remember(vict, ch);
    if (IS_NPC(vict) && GET_HIT(vict) > ((getMaxPL(vict))) / 4)
      LASTHIT(vict) = GET_IDNUM(ch);
    if (is_affected(vict, AFF_SLEEP) && rand_number(1, 2) == 2) {
      char_condition_remove_tag(vict, "sleep_aff", "struck");
      act("@c$N@W seems to be more aware now.@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@WYou are no longer so sleepy.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@c$N@W seems to be more aware now.@n", TRUE, ch, 0, vict,
          TO_NOTVICT);
    }
    if (AFF_FLAGGED(vict, AFF_KNOCKED) && rand_number(1, 12) >= 11) {
      cureStatusKnockedOutAnnounced(vict, true);
      if (IS_NPC(vict) && rand_number(1, 20) >= 12) {
        act("@W$n@W stands up.@n", FALSE, vict, 0, 0, TO_ROOM);
        char_position_set(vict, POS_STANDING);
      }
    }

    if (IS_NPC(ch)) {
      if (GET_LEVEL(ch) > 10) {
        if (dmg - index > 0)
          dmg -= index;
        else if (dmg - index <= 0 && dmg >= 1)
          dmg = 1;
      } else {
        dmg = (dmg * .8);
      }
    } else {
      if (dmg >= 1) {
        if ((dmg + (dmg * 0.5)) - index <= 0)
          dmg = 1;
        else if ((dmg + (dmg * 0.4)) - index <= 0)
          dmg = dmg * 0.04;
        else if ((dmg + (dmg * 0.3)) - index <= 0)
          dmg = dmg * 0.08;
        else if ((dmg + (dmg * 0.2)) - index <= 0)
          dmg = dmg * 0.12;
        else if ((dmg + (dmg * 0.1)) - index <= 0)
          dmg = dmg * 0.16;
        else if (dmg - index <= 0)
          dmg = dmg * 0.2;
        else if (dmg - index > dmg * 0.25)
          dmg -= index;
        else
          dmg = dmg * 0.25;
      }
    }
    if (dmg < 1)
      dmg = 0;
    if (dmg >= 50 && chance > 0)
      hurt_limb(ch, vict, chance, limb, dmg);
    if (IS_NPC(vict) && dmg > getMaxHealth(vict) * .7 &&
        GET_BONUS(ch, BONUS_SADISTIC) > 0) {
      char_stat_set(vict, "experience", GET_EXP(vict) / 2);
    } else if (IS_NPC(vict) && dmg > getCurHealth(vict) &&
               isFullHealth(vict) * .5 && GET_BONUS(ch, BONUS_SADISTIC) > 0) {
      char_stat_set(vict, "experience", GET_EXP(vict) / 2);
    }

    if (CARRYING(vict) && dmg > (((getMaxPL(vict))) * 0.01) &&
        rand_number(1, 10) >= 8)
      carry_drop(vict, 2);

    if (GET_POS(vict) == POS_SITTING && IS_NPC(vict) &&
        getCurHealth(vict) >= ((getMaxPL(vict))) * .98)
      char_cmd_execute(vict, "stand", NULL);

    if (is_sparring(ch) && is_sparring(vict)) {
      if (!IS_NPC(vict)) {
        act("@c$N@w falls down unconscious, and you stop sparring with $M.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@w stops sparring with you as you fall unconscious.@n", TRUE,
            ch, 0, vict, TO_VICT);
        act("@c$N@w falls down unconscious, and @C$n@w stops sparring with "
            "$M.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        setCurHealth(vict, 1);
        if (FIGHTING(vict))
          stop_fighting(vict);
        if (FIGHTING(ch))
          stop_fighting(ch);
        char_position_set(vict, POS_SLEEPING);
        if (!IS_NPC(ch))
          char_condition_apply(vict, "knocked_out", "combat", "sparring");
      } else {
        act("@c$N@w admits defeat to you, stops sparring, and stumbles away.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@c$N@w admits defeat to $n, stops sparring, and stumbles away.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        int founded = 0;
        char_inventory_iterate(vict, [&](auto rew) {
          if (rew) {
            obj_from_char(rew);
            obj_to_room(rew, char_room_get(vict));
            founded = 1;
          }
          return true;
        });
        if (founded == 1)
          act("@c$N@w leaves a reward behind out of respect.@n", TRUE, ch, 0,
              vict, TO_CHAR);
        setCurHealth(vict, 0);
        extract_char(vict);
        return;
      }
    } else if (is_sparring(ch)) {
      act("@c$N@w falls down unconscious, and you spare $S life.@n", TRUE, ch,
          0, vict, TO_CHAR);
      act("@C$n@w spares your life as you fall unconscious.@n", TRUE, ch, 0,
          vict, TO_VICT);
      act("@c$N@w falls down unconscious, and @C$n@w spares $S life.@n", TRUE,
          ch, 0, vict, TO_NOTVICT);
      setCurHealth(vict, 1);
      if (FIGHTING(vict))
        stop_fighting(vict);
      if (FIGHTING(ch))
        stop_fighting(ch);
      char_position_set(vict, POS_SLEEPING);
      if (!IS_NPC(ch))
        char_condition_apply(vict, "knocked_out", "combat", "sparring");
    } else if (!is_sparring(ch) && is_sparring(vict) && IS_NPC(vict)) {
      act("@w$n@w stops sparring!@n", TRUE, ch, 0, vict, TO_ROOM);
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_SPAR);
    }

    if (PLR_FLAGGED(vict, PLR_IMMORTAL) && !is_sparring(ch) &&
        getCurHealth(vict) - dmg <= 0) {
      if (IN_ARENA(vict)) {
        send_to_all("@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n",
                    GET_NAME(ch), GET_NAME(vict));
        char_from_room(ch);
        char_to_room(ch, room_by_id(17875));
        look_at_room(char_room_get(ch), ch, 0);
        char_from_room(vict);
        char_to_room(vict, room_by_id(17875));
        setCurHealth(vict, 1);
        look_at_room(char_room_get(vict), vict, 0);
        if (FIGHTING(vict))
          stop_fighting(vict);
        if (FIGHTING(ch))
          stop_fighting(ch);
        return;
      } else {
        act("@c$N@w disappears right before dying. $N appears to be "
            "immortal.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@CYou disappear right before death, having been saved by your "
            "immortality.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@c$N@w disappears right before dying. $N appears to be "
            "immortal.@n.",
            TRUE, ch, 0, vict, TO_NOTVICT);
        decCurHealthPercentFloored(vict, 1, 1);
        decCurSTPercentFloored(vict, 1, 1);
        decCurKIPercentFloored(vict, 1, 1);
        if (FIGHTING(vict))
          stop_fighting(vict);
        if (FIGHTING(ch))
          stop_fighting(ch);
        char_position_set(vict, POS_SITTING);
        char_from_room(vict);
        char_to_room(vict, room_by_id(sensei_start_room(vict->chclass)));
      }
      return;
    }

    if (GRAPPLING(vict) && GRAPPLING(vict) != ch && type == 1) {
      act("@YThe attack hurts YOU as well because you are grappling with $M!@n",
          TRUE, vict, 0, GRAPPLING(vict), TO_VICT);
      act("@YThe attack hurts @y$N@Y as well because $n is grappling with "
          "$m!@n",
          TRUE, vict, 0, GRAPPLING(vict), TO_NOTVICT);
      maindmg = maindmg / 2;
      hurt(0, 0, ch, GRAPPLING(vict), NULL, maindmg, 3);
    }
    if (GRAPPLED(vict) && GRAPPLED(vict) != ch && type == 1) {
      act("@YThe attack hurts YOU as well because you are being grappled by "
          "$M!@n",
          TRUE, vict, 0, GRAPPLED(vict), TO_VICT);
      act("@YThe attack hurts @y$N@Y as well because $n is being grappled by "
          "$m!@n",
          TRUE, vict, 0, GRAPPLED(vict), TO_NOTVICT);
      maindmg = maindmg / 2;
      hurt(0, 0, ch, GRAPPLED(vict), NULL, maindmg, 3);
    }

    auto show_scouter = [&]() {
      auto eye = GET_EQ(ch, WEAR_EYE);
      if (eye && !PRF_FLAGGED(ch, PRF_NODEC)) {
        if (IS_ANDROID(vict) ||
            (OBJ_FLAGGED(eye, ITEM_BSCOUTER) && GET_HIT(vict) >= 150000) ||
            (OBJ_FLAGGED(eye, ITEM_MSCOUTER) && GET_HIT(vict) >= 5000000) ||
            (OBJ_FLAGGED(eye, ITEM_ASCOUTER) && GET_HIT(vict) >= 15000000))
          send_to_char(ch, " @D<@YProcessing@D: @c?????????????@D>@n\r\n");
        else
          send_to_char(ch, " @D<@YProcessing@D: @c%s@D>@n\r\n",
                       add_commas(GET_HIT(vict)));
      } else {
        send_to_char(ch, "\r\n");
      }
    };

    if (!is_sparring(ch) && !PLR_FLAGGED(vict, PLR_IMMORTAL) &&
        GET_HIT(vict) - dmg <= 0) {
      decCurHealthPercentFloored(vict, 1, 0);
      if (!IS_NPC(vict) && char_stat_get(vict, "life_percent") > 0 &&
          (getCurLF(vict)) - (dmg - GET_HIT(vict)) >= 0) {
        act("@c$N@w barely clings to life!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@CYou barely cling to life!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N@w barely clings to life!@n.", TRUE, ch, 0, vict,
            TO_NOTVICT);
        int64_t lifeloss = dmg - GET_HIT(vict);
        decCurLF(vict, lifeloss);
        send_to_char(vict, "@D[@CLifeforce@D: @R-%s@D]\n",
                     add_commas(lifeloss));
        if ((getCurLF(vict)) >= (getMaxLF(vict)) * 0.05) {
          send_to_char(
              vict,
              "@YYou recover a bit thanks to your strong life force.@n\r\n");
          incCurHealth(vict, (getMaxLF(vict)) * .05);
          decCurLFPercent(vict, .05);
        } else {
          incCurHealth(vict, GET_LEVEL(vict) * 100);
        }
        return;
      }
      if (GET_DEATH_TYPE(vict) != DTYPE_HEAD)
        GET_DEATH_TYPE(vict) = 0;
      if (type <= 0 && (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY)))
        handle_death_msg(ch, vict, 0);
      else if (type > 0 && (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY)))
        handle_death_msg(ch, vict, 1);
      else {
        act("@R$N@w self destructs with a mild explosion!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@R$N@w self destructs with a mild explosion!@n", TRUE, ch, 0,
            vict, TO_ROOM);
      }
      if (dmg > 1) {
        if (type <= 0 && GET_HIT(ch) >= getMaxPL(ch) * 0.5) {
          int64_t raise = (GET_MAX_MANA(ch) * 0.005) + 1;
          incCurKI(ch, raise);
        }
        send_to_char(ch, "@D[@GDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
        send_to_char(vict, "@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
        int64_t healhp = (long double)(GET_MAX_HIT(vict)) * 0.12;
        if (char_condition_has(ch, "dark_metamorphosis") &&
            GET_HIT(ch) <= GET_MAX_HIT(ch)) {
          act("@RYour dark aura saps some of @r$N's@R life energy!@n", TRUE,
              ch, 0, vict, TO_CHAR);
          act("@r$n@R's dark aura saps some of your life energy!@n", TRUE, ch,
              0, vict, TO_VICT);
          incCurHealth(ch, healhp);
        }
        if (IS_MUTANT(ch) &&
            (GET_GENOME(ch, 0) == 10 || GET_GENOME(ch, 1) == 10))
          incCurKI(ch, dmg * .05);
        if (!is_sparring(ch) && IS_NPC(vict)) {
          if (type == 0 && rand_number(1, 100) >= 97) {
            send_to_char(
                ch, "@YY@yo@Yu @yg@Ya@yi@Yn@y s@Yo@ym@Ye @yb@Yo@yn@Yu@ys "
                    "@Ye@yx@Yp@ye@Yr@yi@Ye@yn@Yc@ye@Y!@n\r\n");
            int64_t gain = GET_EXP(vict) * 0.05;
            gain += 1;
            gain_exp(ch, gain);
          } else if (type != 0 && rand_number(1, 100) >= 93) {
            int64_t gain = GET_EXP(vict) * 0.05;
            gain += 1;
            gain_exp(ch, gain);
          }
        }
        if (char_condition_has(vict, "rune_oagaz") && type == 0) {
          act("@CEthereal chains burn into existence! They quickly latch "
              "onto @RYOUR@C body and begin temporarily hampering $s "
              "actions!@n",
              TRUE, ch, 0, vict, TO_CHAR);
          act("@CEthereal chains burn into existence! They quickly latch "
              "onto @c$n's@C body and begin temporarily hampering $s "
              "actions!@n",
              TRUE, ch, 0, vict, TO_ROOM);
          char_condition_apply_with_duration(vict, "ethereal_chains", "skill", "ethereal_chains", 60);
        }
      } else {
        send_to_char(ch, "@D[@GDamage@W: @BPitiful...@D]@n\r\n");
        send_to_char(vict, "@D[@rDamage@W: @BPitiful...@D]@n\r\n");
      }
      decCurHealthPercentFloored(vict, 1, 0);
      if (IS_DEMON(ch) && type == 1)
        SET_BIT_AR(AFF_FLAGS(vict), AFF_ASHED);
      die(vict, ch);
      dead = TRUE;
    } else if (GET_HIT(vict) - dmg > 0) {
      decCurHealth(vict, dmg);
      if (FIGHTING(ch) == NULL)
        set_fighting(ch, vict);
      else if (FIGHTING(ch) != vict)
        set_fighting(ch, vict);
      if (FIGHTING(vict) == NULL)
        set_fighting(vict, ch);
      else if (FIGHTING(vict) != ch)
        set_fighting(vict, ch);
      if (dmg > 1) {
        if (type == 0 && GET_HIT(ch) >= getMaxPL(ch) * 0.5) {
          int64_t raise = (GET_MAX_MANA(ch) * 0.005) + 1;
          incCurKI(ch, raise);
        }
        if (IS_MUTANT(ch) &&
            (GET_GENOME(ch, 0) == 10 || GET_GENOME(ch, 1) == 10))
          incCurKI(ch, dmg * .05);
        send_to_char(ch, "@D[@GDamage@W: @R%s@D]@n", add_commas(dmg));
        send_to_char(vict, "@D[@rDamage@W: @R%s@D]@n\r\n", add_commas(dmg));
        show_scouter();
      } else if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
        send_to_char(ch, "@D[@GDamage@W: @BPitiful...@D]@n");
        send_to_char(vict, "@D[@rDamage@W: @BPitiful...@D]@n\r\n");
        show_scouter();
      }
    }
    if (GET_SKILL(ch, SKILL_FOCUS) && type == 1)
      improve_skill(ch, SKILL_FOCUS, 1);

    if (dead != TRUE) {
      /* Increases GET_FURY for halfbreeds who get damaged. */
      if (!is_sparring(ch) && IS_HALFBREED(vict) && GET_FURY(vict) < 100 &&
          !char_condition_has(vict, "halfbreed_fury")) {
        send_to_char(vict, "@RYour fury increases a little bit!@n\r\n");
        char_stat_mod(vict, "fury", 1);
      }

      if (GET_ALT(ch) == GET_ALT(vict) && LASTATK(ch) != -1) {
        spar_gain(ch, vict, type, dmg);
        spar_gain(vict, ch, type, dmg);
      }
      if ((IS_SAIYAN(ch) ||
           (IS_BIO(ch) &&
            (GET_GENOME(ch, 0) == 2 || GET_GENOME(ch, 1) == 2))) &&
          !IS_NPC(ch) &&
          ((is_sparring(ch) && is_sparring(vict)) ||
           (!is_sparring(ch) && !is_sparring(vict)))) {
        if (GET_POS(ch) != POS_RESTING && GET_POS(vict) != POS_RESTING &&
            dmg > 1) {
          saiyan_gain(ch, vict);
        }
      }
    }
    if (IS_ARLIAN(vict) && dead != TRUE && !is_sparring(vict) &&
        !is_sparring(ch))
      handle_evolution(vict, dmg);
    if (dead == TRUE) {
      char corp[256];
      if (!PLR_FLAGGED(ch, PLR_SELFD2)) {
        if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
          sprintf(corp, "all.zenni corpse");
          do_get(ch, corp, 0, 0);
        }
        if (!IS_NPC(ch) && (ch != vict) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
          sprintf(corp, "all corpse");
          do_get(ch, corp, 0, 0);
        }
      }
    }
  }
  /* If an object is targeted */
  else if (obj) {
    switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL)) {
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
    if (OBJ_FLAGGED(obj, ITEM_UNBREAKABLE)) {
      act("$p@w seems unaffected.@n", TRUE, ch, obj, 0, TO_CHAR);
      act("$p@w seems unaffected.@n", TRUE, ch, obj, 0, TO_ROOM);
    } else if (GET_OBJ_VNUM(obj) == 79) {
      if (GET_OBJ_WEIGHT(obj) - dmg > 0) {
        if (type <= 0) {
          if (AFF_FLAGGED(ch, AFF_INFUSE))
            dmg *= 10;
          act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_ROOM);
          GET_OBJ_WEIGHT(obj) -= dmg;
          if (GET_FELLOW_WALL(obj)) {
            struct obj_data *wall;
            wall = GET_FELLOW_WALL(obj);
            GET_OBJ_WEIGHT(wall) -= dmg;
            act("$p@w cracks some. A humanoid shadow can be seen moving on the "
                "other side.@n",
                TRUE, 0, obj, 0, TO_ROOM);
          }
        } else {
          dmg *= 30;
          act("$p@w melts some.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("$p@w melts some.@n", TRUE, ch, obj, 0, TO_ROOM);
          GET_OBJ_WEIGHT(obj) -= dmg;
          if (GET_FELLOW_WALL(obj)) {
            struct obj_data *wall;
            wall = GET_FELLOW_WALL(obj);
            GET_OBJ_WEIGHT(wall) -= dmg;
            act("$p@w melts some.@n", TRUE, ch, obj, 0, TO_ROOM);
          }
        }
      } else {
        if (type <= 0) {
          act("$p@w breaks completely apart and then melts away.@n", TRUE, ch,
              obj, 0, TO_CHAR);
          act("$p@w breaks completely apart and then melts away.@n", TRUE, ch,
              obj, 0, TO_ROOM);
          extract_obj(obj);
        } else {
          act("$p@w is blown away into snow and water!@n", TRUE, ch, obj, 0,
              TO_CHAR);
          act("$p@w is blown away into snow and water!@n", TRUE, ch, obj, 0,
              TO_ROOM);
          extract_obj(obj);
        }
      }
    } else if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) - dmg > 0) {
      act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_CHAR);
      act("$p@w cracks some.@n", TRUE, ch, obj, 0, TO_ROOM);
      GET_OBJ_VAL(obj, VAL_ALL_HEALTH) -= dmg;
    } else {
      if (type <= 0) {
        act("$p@w shatters apart!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("$p@w shatters apart!@n", TRUE, ch, obj, 0, TO_ROOM);
        GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = 0;
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
        if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON &&
            GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) {
          GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) = 0;
        }
      } else if (type != 0) {
        act("$p@w is disintegrated!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("$p@w is disintegrated!@n", TRUE, ch, obj, 0, TO_ROOM);
        extract_obj(obj);
      }
    }
  } else {
    mud_log("Log: Error with hurt.\n");
  }
}

/* This handles the length of time between attacks and other actions *
 * players AND non-players will have to endure. Allowing for a more   *
 * balanced and interesting attack cooldown system. - Iovan 2/25/2011 */
void handle_cooldown(struct char_data *ch, int cooldown) {

  /* Let's clear any cooldown they may accidently have so it doesn't stack *
   * This is only for NPCs as player cooldown is handled through the stock *
   * descriptor_list command interpreter. This also initializes cooldown   */

  if (!IS_NPC(ch)) {
    if (PLR_FLAGGED(ch, PLR_MULTIHIT)) {
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_MULTIHIT);
      return;
    }
  }

  if (IS_NPC(ch)) {
    MOB_COOLDOWN(ch) = 0;
  }

  reveal_hiding(ch, 0);
  int waitCalc = 10, base = cooldown;
  int64_t cspd = 0;

  /* Ok calculating speed. */
  cspd = GET_SPEEDI(ch);

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

  /* Alright now let's determine the cooldown based on the wait and cooldown
   * assigned * by the attack which called handle_cooldown. */
  if (!IS_NPC(ch)) {
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
      MOB_COOLDOWN(ch) = 12;
    } else if (cooldown >= 110) {
      MOB_COOLDOWN(ch) = 11;
    } else if (cooldown >= 100) {
      MOB_COOLDOWN(ch) = 10;
    } else if (cooldown >= 90) {
      MOB_COOLDOWN(ch) = 9;
    } else if (cooldown >= 80) {
      MOB_COOLDOWN(ch) = 8;
    } else if (cooldown >= 70) {
      MOB_COOLDOWN(ch) = 7;
    } else if (cooldown >= 60) {
      MOB_COOLDOWN(ch) = 6;
    } else if (cooldown >= 50) {
      MOB_COOLDOWN(ch) = 5;
    } else if (cooldown >= 40) {
      MOB_COOLDOWN(ch) = 4;
    } else if (cooldown >= 30) {
      MOB_COOLDOWN(ch) = 3;
    } else if (cooldown >= 20) {
      MOB_COOLDOWN(ch) = 2;
    } else {
      MOB_COOLDOWN(ch) = 1;
    }
  }
}

/* This handles whether parry is turned on */
int handle_parry(struct char_data *ch) {

  if (axion_dice(0) <= 4) { /* Critical failure */
    return (1);
  }

  if (IS_NPC(ch)) {

    /*  Non-humanoids are only rarely capable of parrying against weak players.
     * Never against strong players. Humanoids get progressively better at parry
     * the higher level they are.
     **/
    if (!IS_HUMANOID(ch)) {
      return (rand_number(0, 5));
    } else {
      if (GET_LEVEL(ch) >= 110) {
        return (rand_number(90, 105));
      } else if (GET_LEVEL(ch) >= 100) {
        return (rand_number(85, 95));
      } else if (GET_LEVEL(ch) >= 80) {
        return (rand_number(70, 85));
      } else if (GET_LEVEL(ch) >= 40) {
        return (rand_number(50, 70));
      } else {
        int top = GET_LEVEL(ch);

        if (top < 15)
          top = 16;
        return (rand_number(15, top));
      }
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOPARRY)) {
    return (-2);
  } else {
    int num = GET_SKILL(ch, SKILL_PARRY);
    if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 3 || GET_GENOME(ch, 1) == 3)) {
      num += 10;
    }
    if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100) {
      num += 5;
    } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 80) {
      num += 4;
    } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 60) {
      num += 3;
    } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 40) {
      num += 2;
    } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 20) {
      num += 1;
    }
    return (num);
  }
}

/* This handles whether a step of the combo was preformed. */
int handle_combo(struct char_data *ch, struct char_data *vict) {
  if (IS_NPC(ch))
    return 0;

  switch (LASTATK(ch)) {
  case 0: case 1: case 2: case 3: case 4:
  case 5: case 6: case 8: case 51: case 52: case 56:
    break;
  default:
    if (char_condition_has(ch, "combo"))
      send_to_char(ch, "@RYou have cut your combo short with the wrong attack!@n\r\n");
    char_condition_remove(ch, "combo", "end_combo");
    return 0;
  }

  if (count_physical(ch) < 3)
    return 0;

  int64_t chspeed = GET_SPEEDI(ch);
  int64_t victspeed = GET_SPEEDI(vict);
  int64_t speedPercentage = ((double)(chspeed) / (double)(victspeed)) * 100.0;
  int chance;
  if (speedPercentage < 1)
    chance = 1;
  else if (speedPercentage > 100)
    chance = 100;
  else
    chance = (int)speedPercentage;
  chance = 100 - chance;
  if (chance < 1)
    chance = 1;
  chance += 25;
  if (LASTATK(ch) == 0 || LASTATK(ch) == 1)
    chance -= 10;
  if (LASTATK(ch) == 2 || LASTATK(ch) == 3)
    chance -= 5;

  if (!char_condition_has(ch, "combo") && rand_number(1, 100) > chance) {
    struct combo_init_entry { int skill; int state; const char *msg; int lo; int hi; };
    static const combo_init_entry init_picks[] = {
      {SKILL_PUNCH,      0, "@GYou have a chance for a COMBO! Try a@R punch @Gnext!@n\r\n",           1,  5},
      {SKILL_KICK,       1, "@GYou have a chance for a COMBO! Try a@R kick @Gnext!@n\r\n",            6, 10},
      {SKILL_ELBOW,      2, "@GYou have a chance for a COMBO! Try an@R elbow @Gnext!@n\r\n",         11, 14},
      {SKILL_KNEE,       3, "@GYou have a chance for a COMBO! Try a@R knee @Gnext!@n\r\n",           15, 17},
      {SKILL_ROUNDHOUSE, 4, "@GYou have a chance for a COMBO! Try a@R roundhouse @Gnext!@n\r\n",     18, 19},
      {SKILL_UPPERCUT,   5, "@GYou have a chance for a COMBO! Try an@R uppercut @Gnext!@n\r\n",      20, 21},
      {SKILL_HEELDROP,   8, "@GYou have a chance for a COMBO! Try a@R heeldrop @Gnext!@n\r\n",       22, 22},
      {SKILL_SLAM,       6, "@GYou have a chance for a COMBO! Try a@R slam @Gnext!@n\r\n",           24, 24},
    };
    int new_combo = -1;
    bool found = false;
    while (!found) {
      int r = rand_number(1, 24);
      for (const auto &e : init_picks) {
        if (r >= e.lo && r <= e.hi && GET_SKILL(ch, e.skill) > 0) {
          send_to_char(ch, "%s", e.msg);
          new_combo = e.state;
          found = true;
          break;
        }
      }
    }
    char_condition_add(ch, "combo", "start_combo", "new_combo");
    char_condition_number_set(ch, "combo", "state", new_combo);
    return 0;
  }

  if (!char_condition_has(ch, "combo"))
    return 0;

  auto state = char_condition_number_get(ch, "combo", "state");
  auto hits = char_condition_number_get(ch, "combo", "hits");

  if (LASTATK(ch) != state) {
    send_to_char(ch, "@GCombo failed! Try harder next time!@n\r\n");
    char_condition_remove(ch, "combo", "end_combo");
    return 0;
  }

  if (hits >= physical_mastery(ch)) {
    char_condition_number_set(ch, "combo", "hits", hits + 1);
    send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Combo FINISHED for "
                     "massive damage@G!@n\r\n", hits);
    return 0;
  }

  hits += 1;
  char_condition_number_set(ch, "combo", "hits", hits);

  auto try_pick = [&](int skill, int next_state, const char *name) -> bool {
    if (GET_SKILL(ch, skill) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try %s@G!@n\r\n",
                   hits, name);
      char_condition_number_set(ch, "combo", "state", next_state);
      return true;
    }
    return false;
  };

  auto try_bash_chain = [&](bool with_heeldrop, bool with_slam) -> bool {
    if (GET_SKILL(ch, SKILL_BASH) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try bash@G!@n\r\n", hits);
      char_condition_number_set(ch, "combo", "state", 51);
      return true;
    }
    if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", hits);
      char_condition_number_set(ch, "combo", "state", 56);
      return true;
    }
    if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 2) == 2) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", hits);
      char_condition_number_set(ch, "combo", "state", 52);
      return true;
    }
    if (with_heeldrop && GET_SKILL(ch, SKILL_HEELDROP) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", hits);
      char_condition_number_set(ch, "combo", "state", 8);
      return true;
    }
    if (with_slam && GET_SKILL(ch, SKILL_SLAM) > 0) {
      send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rslam@G!@n\r\n", hits);
      char_condition_number_set(ch, "combo", "state", 6);
      return true;
    }
    return false;
  };

  int success = FALSE;
  while (success == FALSE) {
    if (hits >= 20) {
      switch (rand_number(1, 34)) {
      case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
        success = try_pick(SKILL_ELBOW, 2, "an@R elbow"); break;
      case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16:
        success = try_pick(SKILL_KNEE, 3, "a @Rknee"); break;
      case 17: case 18: case 19: case 20: case 21:
        success = try_pick(SKILL_UPPERCUT, 5, "an@R uppercut"); break;
      case 22: case 23: case 24: case 25: case 26:
        success = try_pick(SKILL_ROUNDHOUSE, 4, "a @Rroundhouse"); break;
      case 27: case 28: case 29:
        success = try_bash_chain(true, true); break;
      case 30: case 31: case 32: case 33: case 34:
        success = try_bash_chain(true, false); break;
      }
    } else if (hits >= 15) {
      switch (rand_number(1, 36)) {
      case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
        success = try_pick(SKILL_ELBOW, 2, "an@R elbow"); break;
      case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20:
        success = try_pick(SKILL_KNEE, 3, "a @Rknee"); break;
      case 21: case 22: case 23:
        success = try_pick(SKILL_PUNCH, 0, "a @Rpunch"); break;
      case 25: case 26: case 27:
        success = try_pick(SKILL_KICK, 1, "a @Rkick"); break;
      case 29: case 30:
        success = try_pick(SKILL_UPPERCUT, 5, "an@R uppercut"); break;
      case 31: case 32: case 33: case 34:
        success = try_pick(SKILL_ROUNDHOUSE, 4, "a @Rroundhouse"); break;
      case 35:
        success = try_bash_chain(false, true); break;
      case 36:
        success = try_bash_chain(true, false); break;
      }
    } else if (hits >= 10) {
      switch (rand_number(1, 34)) {
      case 1: case 2: case 3: case 4: case 5:
        success = try_pick(SKILL_ELBOW, 2, "an@R elbow"); break;
      case 6: case 7: case 8: case 9: case 10:
        success = try_pick(SKILL_KNEE, 3, "a @Rknee"); break;
      case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18:
        success = try_pick(SKILL_PUNCH, 0, "a @Rpunch"); break;
      case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26:
        success = try_pick(SKILL_KICK, 1, "a @Rkick"); break;
      case 27: case 28: case 29:
        success = try_pick(SKILL_UPPERCUT, 5, "an@R uppercut"); break;
      case 30: case 31:
        success = try_pick(SKILL_ROUNDHOUSE, 4, "a @Rroundhouse"); break;
      case 32: case 33:
        success = try_bash_chain(false, true); break;
      case 34:
        success = try_bash_chain(true, false); break;
      }
    } else if (hits >= 5) {
      switch (rand_number(1, 30)) {
      case 1: case 2: case 3: case 4:
        success = try_pick(SKILL_ELBOW, 2, "an@R elbow"); break;
      case 5: case 6: case 7: case 8:
        success = try_pick(SKILL_KNEE, 3, "a @Rknee"); break;
      case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18:
        success = try_pick(SKILL_PUNCH, 0, "a @Rpunch"); break;
      case 19: case 20: case 21: case 22:
        if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 56);
          success = TRUE;
        }
        break;
      case 23:
        if (GET_SKILL(ch, SKILL_BASH) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try bash@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 51);
          success = TRUE;
        }
        break;
      case 24: case 25:
        if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 52);
          success = TRUE;
        }
        break;
      case 26:
        if (GET_SKILL(ch, SKILL_HEELDROP) > 0 && rand_number(1, 3) == 3) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 8);
          success = TRUE;
        }
        break;
      case 27:
        if (GET_SKILL(ch, SKILL_SLAM) > 0 && rand_number(1, 3) == 3) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rslam@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 6);
          success = TRUE;
        }
        break;
      case 28:
        success = try_pick(SKILL_KICK, 1, "a @Rkick"); break;
      case 29:
        success = try_pick(SKILL_UPPERCUT, 5, "an@R uppercut"); break;
      case 30:
        success = try_pick(SKILL_ROUNDHOUSE, 4, "a @Rroundhouse"); break;
      }
    } else {
      switch (rand_number(1, 30)) {
      case 1: case 2: case 3:
        success = try_pick(SKILL_ELBOW, 2, "an@R elbow"); break;
      case 4: case 5: case 6:
        success = try_pick(SKILL_KNEE, 3, "a @Rknee"); break;
      case 7: case 8: case 9: case 10:
        if (GET_SKILL(ch, SKILL_UPPERCUT) > 0 && rand_number(1, 2) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try an@R uppercut@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 5);
          success = TRUE;
        }
        break;
      case 11: case 12: case 13: case 14:
        if (GET_SKILL(ch, SKILL_ROUNDHOUSE) > 0 && rand_number(1, 2) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rroundhouse@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 4);
          success = TRUE;
        }
        break;
      case 15: case 16: case 17: case 18:
        success = try_pick(SKILL_PUNCH, 0, "a @Rpunch"); break;
      case 19: case 20: case 21: case 22:
        if (GET_SKILL(ch, SKILL_BASH) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try bash@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 51);
          success = TRUE;
        } else if (GET_SKILL(ch, SKILL_TAILWHIP) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rtailwhip@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 56);
          success = TRUE;
        } else if (GET_SKILL(ch, SKILL_HEADBUTT) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rheadbutt@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 52);
          success = TRUE;
        } else if (GET_SKILL(ch, SKILL_HEELDROP) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rheeldrop@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 8);
          success = TRUE;
        } else if (GET_SKILL(ch, SKILL_SLAM) > 0 && rand_number(1, 3) == 2) {
          send_to_char(ch, "@D(@GC-c-combo Bonus @gx%ld@G!@D)@C Next try a @Rslam@G!@n\r\n", hits);
          char_condition_number_set(ch, "combo", "state", 6);
          success = TRUE;
        }
        break;
      case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30:
        success = try_pick(SKILL_KICK, 1, "a @Rkick"); break;
      }
    }
  }
  return hits;
}

void handle_death_msg(struct char_data *ch, struct char_data *vict, int type) {
  struct room_data *vroom = char_room_get(vict);
  int vsect = room_sector_type_get(vroom);
  bool vsunken = room_is_sunken(vroom);
  if (type == 0) {
    if (!vsunken && vsect != SECT_WATER_SWIM && vsect != SECT_WATER_NOSWIM &&
        !room_flagged(vroom, ROOM_SPACE) && vsect != SECT_FLYING) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r coughs up blood before falling to the ground dead.@n", TRUE,
            ch, 0, vict, TO_CHAR);
        act("@rYou cough up blood before falling to the ground dead.@n", TRUE,
            ch, 0, vict, TO_VICT);
        act("@R$N@r coughs up blood before falling down dead.@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        break;
      case 2:
        act("@R$N@r crumples to the ground dead.@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@rYou crumple to the ground dead.@n", TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r crumples to the ground dead.@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        break;
      case 3:
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        break;
      case 4:
        act("@R$N@r writhes on the ground screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou writhe on the ground screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r writhes on the ground screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      case 5:
        act("@R$N@r hits the ground dead with such force that blood flies into "
            "the air briefly!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou hit the ground dead with such force that blood flies into "
            "the air briefly!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r hits the ground dead with such force that blood flies into "
            "the air briefly!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      }
    } else if (vsect == SECT_WATER_SWIM || vsect == SECT_WATER_NOSWIM) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r coughs up blood and dies before falling down to the water. "
            "A large splash accompanies $S body hitting the water!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou cough up blood and die before falling down to the water. A "
            "large splash accompanies your body hitting the water!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r coughs up blood and dies before falling down to the water. "
            "A large splash accompanies $S body hitting the water!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 2:
        act("@R$N@r crumples down to the water, with the signs of life leaving "
            "$S eyes as $E floats in the water.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou crumple down to the water and die.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r crumples down to the water, with the signs of life leaving "
            "$S eyes as $E floats in the water.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 3:
        act("@R$N@r cries out $S last breath before dying and leaving a "
            "floating corpse behind.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r cries out $S last breath before dying and leaving a "
            "floating corpse behind.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 4:
        act("@R$N@r writhes in the water screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou writhe in the water screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r writhes in the water screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      case 5:
        act("@R$N@r hits the water dead with such force that blood mixed with "
            "water flies into the air briefly!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou hit the water dead with such force that blood mixed with "
            "water flies into the air briefly!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r hits the water dead with such force that blood mixed with "
            "water flies into the air briefly!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      }
    } else if (room_flagged(vroom, ROOM_SPACE)) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r coughs up blood and dies. The blood freezes and floats "
            "freely through space...@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou cough up blood and die. The blood freezes and floats freely "
            "through space...@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r coughs up blood and dies. The blood freezes and floats "
            "freely through space...@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 2:
        act("@R$N@r dies and leaves $S corpse floating freely in space.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou die and leave your corpse floating freely in space.@n", TRUE,
            ch, 0, vict, TO_VICT);
        act("@R$N@r dies and leaves $S corpse floating freely in space.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 3:
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        break;
      case 4:
        act("@R$N@r writhes in space trying to scream in pain before $e "
            "finally dies!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou writhe in space trying to scream in pain before you finally "
            "die!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r writhes in space trying to scream in pain before $e "
            "finally dies!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      case 5:
        act("@R$N@r dies suddenly leaving behind a badly damaged corpse "
            "floating in space!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou die suddenly leaving behind a badly damaged corpse floating "
            "in space!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r dies suddenly leaving behind a badly damaged corpse "
            "floating in space!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      }
    } else if (vsect == SECT_FLYING) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r coughs up blood before $s corpse starts to fall to the "
            "ground far below.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou coughs up blood before your corpse starts to fall to the "
            "ground far below.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r coughs up blood before $s corpse starts to fall to the "
            "ground far below.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 2:
        act("@R$N@r dies and $S corpse begins to fall to the ground below.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou die and your corpse begins to fall to the ground below.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r dies and $S corpse begins to fall to the ground below.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 3:
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        break;
      case 4:
        act("@R$N@r writhes in midair screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou writhe in midair screaming in pain before finally dying!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r writhes in midair screaming in pain before finally "
            "dying!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      case 5:
        act("@R$N@r snaps back and dies with such force that blood flies into "
            "the air briefly!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou snap back and die with such force that blood flies into the "
            "air briefly!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r hits the ground dead with such force that blood flies into "
            "the air briefly!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      }
    } else {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r coughs up blood before $s corpse starts to float limply in "
            "the water.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou coughs up blood before your corpse starts to float limply "
            "in the water.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r coughs up blood before $s corpse starts to float limply in "
            "the water.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 2:
        act("@R$N@r dies and $S corpse begins to float limply in the water.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou die and your corpse begins to float limply in the water.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r dies and $S corpse begins to float limply in the water.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 3:
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYou cry out your last breath before dying.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r cries out $S last breath before dying.@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        break;
      case 4:
        act("@R$N@r writhes and thrases in the water trying to scream before "
            "finally dying!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou writhe and thrash in the water trying to scream before "
            "finally dying!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r writhes and thrashes in the water trying to scream before "
            "finally dying!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      case 5:
        act("@R$N@r snaps back and dies with such force that blood floods out "
            "of $S body into the water!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou snap back and die with such force that blood floods out of "
            "your body into the water!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r hits the ground dead with such force that blood floods out "
            "of $S body into the water!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_DEATH_TYPE(vict) != DTYPE_HEAD) {
          GET_DEATH_TYPE(vict) = DTYPE_PULP;
        }
        break;
      }
    }
  } else {
    if (!vsunken && vsect != SECT_WATER_SWIM && vsect != SECT_WATER_NOSWIM &&
        !room_flagged(vroom, ROOM_SPACE) && vsect != SECT_FLYING) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE,
            ch, 0, vict, TO_CHAR);
        act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE,
            ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 2:
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rThe bottom half of your body is all that remains as you die.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_HALF;
        break;
      case 3:
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYour body completely disintegrates in the attack!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 4:
        act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@rYour body falls down as a smoldering corpse!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        break;
      case 5:
        act("@rWhat's left of @R$N@r's body slams into the ground as $E "
            "dies!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rWhat's left of your body slams into the ground as you die!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rWhat's left of @R$N@r's body slams into the ground as $E "
            "dies!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      }
    } else if (vsect == SECT_WATER_SWIM || vsect == SECT_WATER_NOSWIM) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE,
            ch, 0, vict, TO_CHAR);
        act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r explodes and chunks of $M shower to the ground.@n", TRUE,
            ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 2:
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rThe bottom half of your body is all that remains as you die.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_HALF;
        break;
      case 3:
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYour body completely disintegrates in the attack!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 4:
        act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@rYour body falls down as a smoldering corpse!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@R$N@r falls down as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        break;
      case 5:
        act("@rWhat's left of @R$N@r's body slams into the ground as $E "
            "dies!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rWhat's left of your body slams into the ground as you die!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rWhat's left of @R$N@r's body slams into the ground as $E "
            "dies!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      }
    } else if (room_flagged(vroom, ROOM_SPACE)) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r explodes and chunks of $M shower out into every direction "
            "of space.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r explodes and chunks of $M shower out into every direction "
            "of space.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 2:
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rThe bottom half of your body is all that remains as you die.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_HALF;
        break;
      case 3:
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYour body completely disintegrates in the attack!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 4:
        act("@R$N@r floats away as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@rYour body floats away as a smoldering corpse!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@R$N@r floats away as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        break;
      case 5:
        act("@rWhat's left of @R$N@r's body floats away through space!@n", TRUE,
            ch, 0, vict, TO_CHAR);
        act("@rWhat's left of your body floats away through space!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@rWhat's left of @R$N@r's body floats away through space!@n", TRUE,
            ch, 0, vict, TO_NOTVICT);
        break;
      }
    } else if (vsect == SECT_FLYING) {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r explodes and chunks of $M shower towards the ground far "
            "below.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r explodes and chunks of $M shower toward the ground far "
            "below.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 2:
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rThe bottom half of your body is all that remains as you die.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_HALF;
        break;
      case 3:
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYour body completely disintegrates in the attack!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 4:
        act("@R$N@r falls down toward the ground as a smoldering corpse!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYour body falls down toward the ground as a smoldering "
            "corpse!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r falls down toward the ground as a smoldering corpse!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      case 5:
        act("@rWhat's left of @R$N@r's body falls toward the ground as $E "
            "dies!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rWhat's left of yor body falls toward the ground as you die!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rWhat's left of @R$N@r's body falls toward the ground as $E "
            "dies!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        break;
      }
    } else {
      switch (rand_number(1, 5)) {
      case 1:
        act("@R$N@r explodes and chunks of $M float freely through the "
            "water.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rYou explode leaving only chunks behind.@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@R$N@r explodes and chunks of $M float freely through the "
            "water.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 2:
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@rThe bottom half of your body is all that remains as you die.@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@rThe bottom half of @R$N@r is all that remains as $E dies.@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_HALF;
        break;
      case 3:
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@rYour body completely disintegrates in the attack!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@R$N@r is completely disintegrated in the attack!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        GET_DEATH_TYPE(vict) = DTYPE_VAPOR;
        break;
      case 4:
        act("@R$N@r falls back as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@rYour body falls back as a smoldering corpse!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@R$N@r falls back as a smoldering corpse!@n", TRUE, ch, 0, vict,
            TO_NOTVICT);
        break;
      case 5:
        act("@rWhat's left of @R$N@r's body floats limply as $E dies!@n", TRUE,
            ch, 0, vict, TO_CHAR);
        act("@rWhat's left of yor body floats limply as you die!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@rWhat's left of @R$N@r's body floats limply as $E dies!@n", TRUE,
            ch, 0, vict, TO_NOTVICT);
        break;
      }
    }
  }
}
