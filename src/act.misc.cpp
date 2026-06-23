/* ************************************************************************
 *  File: act.misc.c                                    Part of DBAT       *
 *  Usage: Miscellaneous player-level commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 *                                   ---                                   *
 *  This is an original file created by me for Dragonball Advent Truth     *
 *  original credits maintained where relevant for act.other.c as this is  *
 *  practically an act.other.c part two - Iovan 3/20/2011                  *
 ************************************************************************ */
#include "act.misc.h"
#include "consts/applies.h"
#include "consts/attacks.h"
#include "consts/fightprefs.h"
#include "consts/fish.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/races.h"
#include "consts/recipes.h"
#include "consts/roomflags.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "consts/sizes.h"
#include "consts/skills.h"
#include "consts/songs.h"


#include "extract.h"
#include "interpreter.h"
#include "iterate.hpp"
#include "random.h"
#include "relocate.h"
#include "search.h"

#include "db.h"

#include "act.informative.h"
#include "act.movement.h"
#include "act.wizard.h"
#include "dg_comm.h"
#include "object_utils.h"
#include "room_utils.h"

#include "comm.h"
#include "spells.h"

#include "character_api.h"
#include "character_db.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "combat.h"
#include "consts/applies.h"
#include "consts/constates.h"
#include "consts/mobflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "event_queue_api.h"
#include "fight.h"
#include "flags.h"
#include "handler.h"
#include "log.h"
#include "obj_edit.h"
#include "object_macros.h"
#include "races.h"
#include "races_plus.h"
#include "room_api.h"
#include "room_db.h"
#include "stringutils.h"
#include "util_macros.h"

#include "search.hpp"
#include "iterate.hpp"

#include <cstdlib>
#include <vector>

/* local functions  */
static void generate_multiform(struct char_data *ch, int count);
static int valid_recipe(struct char_data *ch, int recipe, int type);
static int has_pole(struct char_data *ch);
static void catch_fish(struct char_data *ch, int quality);
static void ev_fish_tick(int ctx_type, int64_t ctx_a, int64_t ctx_b);
static int valid_silk(struct obj_data *obj);

/* do_spiritcontrol moved to lua/characters/commands/misc/spiritcontrol.lua */
/* do_tailhide moved to lua/characters/commands/misc/tailhide.lua */
/* do_nogrow moved to lua/characters/commands/misc/nogrow.lua */

ACMD(do_restring) {

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int pay = 0;

  one_argument(argument, arg);

  if (char_room_vnum_get(ch) >= 178 && char_room_vnum_get(ch) <= 184) {
    pay = 5000;
    if (GET_GOLD(ch) < pay) {
      send_to_char(ch, "You need at least 5,000 zenni to initiate an equipment "
                       "restring.\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
      send_to_char(
          ch,
          "You don't have a that equipment to restring in your inventory.\r\n");
      send_to_char(ch, "Syntax: restring (obj name)\r\n");
      return;
    } else if (OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
      send_to_char(ch, "You can not restring a custom piece. Why? Because you "
                       "already restrung it you dummy.\r\n");
      return;
    } else {
      STATE(ch->desc) = CON_POBJ;
      char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH],
          thelong[MAX_INPUT_LENGTH];

      *thename = '\0';
      *theshort = '\0';
      *thelong = '\0';

      sprintf(thename, "%s", obj->name);
      sprintf(theshort, "%s", obj->short_description);
      sprintf(thelong, "%s", obj->description);

      ch->desc->obj_name = strdup(thename);
      ch->desc->obj_was = strdup(theshort);
      ch->desc->obj_short = strdup(theshort);
      ch->desc->obj_long = strdup(thelong);
      ch->desc->obj_point = obj;
      ch->desc->obj_type = 1;
      ch->desc->obj_weapon = 0;
      disp_restring_menu(ch->desc);
      ch->desc->obj_editflag = EDIT_RESTRING;
      ch->desc->obj_editval = EDIT_RESTRING_MAIN;
      return;
    }
  }
}

ACMD(do_multiform) {

  if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_MULTIFORM)) {
    send_to_char(ch, "You do not know how to perform that technique.\r\n");
    return;
  }

  std::vector<struct char_data *> multiforms;

  auto room = char_room_get(ch);
  room_people_iterate(room, [&](auto tch) {
    if (tch == ch || !IS_NPC(tch)) {
      return true;
    }
    if (GET_MOB_VNUM(tch) == 25 && GET_ORIGINAL(tch) == ch) {
      multiforms.push_back(tch);
    }
    return true;
  });

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!strcasecmp(arg, "merge")) {
    if (multiforms.empty()) {
      send_to_char(ch, "You have no multiforms present to merge with!\r\n");
      return;
    }
    for (auto tch : multiforms) {
      if (tch == ch || !IS_NPC(tch)) {
        continue;
      }
      if (GET_MOB_VNUM(tch) == 25 && GET_ORIGINAL(tch) == ch) {
        extract_char(tch);
      }
    }
    char_condition_remove(ch, "multiform_original", "command");
    return;
  }

  if (!strcasecmp(arg, "split")) {
    int64_t cost = (GET_MAX_MANA(ch) * 0.005) + (GET_MAX_MOVE(ch) * 0.005) + 2;
    int penalty = 0;

    if (FIGHTING(ch)) {
      penalty = rand_number(8, 15);
    }

    int roll = axion_dice(penalty);

    cost *= (GET_SKILL(ch, SKILL_MULTIFORM) * 0.2);

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to split!\r\n");
      return;
    }
    if ((getCurST(ch)) < cost) {
      send_to_char(ch, "You do not have enough stamina to split!\r\n");
      return;
    }
    improve_skill(ch, SKILL_MULTIFORM, 1);

    if (GET_SKILL(ch, SKILL_MULTIFORM) < roll) {
      act("@YYou focus your ki into your body while concentrating on the image "
          "of your body splitting into two. @yYou lose your concentration and "
          "fail to split though...@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@y$n@Y seems to concentrate really hard for a moment, before "
          "relaxing.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurST(ch, cost);
      decCurKI(ch, cost);
      return;
    }
    act("@YYou focus your ki into your body while concentrating on the image "
        "of your body splitting into two. Another you splits out of your "
        "body!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@YSuddenly @y$n@Y seems to concentrates really and after a brief "
        "moment splits into two copies of $mself!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    generate_multiform(ch, 1);
    return;
  } else {
    send_to_char(ch, "Huh? Try help multiform\r\n");
    return;
  }
}

static void generate_multiform(struct char_data *ch, int count) {
  char blamo[MAX_INPUT_LENGTH];
  sprintf(blamo, "p.%s", GET_NAME(ch));
  struct mob_proto_data *proto = mob_proto_by_id(25);
  if (!proto) {
    send_to_imm("Multiform Clone prototype doesn't exist!");
    return;
  }

  if (char_condition_has(ch, "multiform_original")) {
    auto clone_count = char_condition_number_get(ch, "multiform_original", "clones");
    clone_count += count;
    char_condition_number_set(ch, "multiform_original", "clones", clone_count);
  } else {
    char_condition_apply_with_number(ch, "multiform_original", "skill",
                                     "multiform", "clones", count);
  }

  char clone_name[MAX_INPUT_LENGTH];
  snprintf(clone_name, sizeof(clone_name), "%s's Clone", ch->name);

  char clone_sdesc[MAX_INPUT_LENGTH];
  snprintf(clone_sdesc, sizeof(clone_sdesc), "%s's @CClone@n", ch->name);
  char clone_ldesc[MAX_INPUT_LENGTH];
  snprintf(clone_ldesc, sizeof(clone_ldesc),
           "%s's @CClone@w is standing here.@n\n", ch->name);

  for (int i = 0; i < count; i++) {
    char_data *clone = nullptr;
    clone = read_mobile(25, VIRTUAL);

    clone->name = strdup(clone_name);
    clone->short_descr = strdup(clone_sdesc);
    clone->long_descr = strdup(clone_ldesc);
    if (ch->description)
      clone->description = strdup(ch->description);
    clone->race = ch->race;
    clone->chclass = ch->chclass;

    // Not sure if these are actually used...
    char_stat_set(clone, "alignment", char_stat_get(ch, "alignment"));

    // Make the physical appearance match!
    clone->sex = ch->sex;
    clone->hairl = ch->hairl;
    clone->hairs = ch->hairs;
    clone->hairc = ch->hairc;
    clone->skin = ch->skin;
    clone->eye = ch->eye;
    clone->distfea = ch->distfea;
    clone->aura = ch->aura;

    for(const char* stat :   {"weight", "height", "level", "powerlevel", "ki", "stamina"}) {
      char_stat_set(clone, stat, char_stat_get(ch, stat));
    }

    clone->time = ch->time;

    ch->transclass = ch->transclass;

    // Copying these values, but it shouldn't matter because clones no longer
    // work this way.

    // Bioandroid Genome copy...
    clone->genome[0] = ch->genome[0];
    clone->genome[1] = ch->genome[1];

    char_multiform_clone_set(clone, ch);
    char_condition_apply_with_number(clone, "multiform", "skill", "multiform",
                                     "original_id", char_id_get(ch));
    char_to_room(clone, char_room_get(ch));
    add_follower(clone, ch);
  }
}

void handle_multi_merge(struct char_data *form) {
  struct char_data *ch = GET_ORIGINAL(form);

  if (ch == NULL)
    return;

  send_to_char(ch, "@YYou merge with one of your forms!@n\r\n");
  act("@y$n@Y merges with one of his multiforms!@n\r\n", TRUE, ch, 0, 0,
      TO_ROOM);
  
  auto count = char_condition_number_get(ch, "multiform_original", "clones");
  count--;
  char_condition_number_set(ch, "multiform_original", "clones", count);

  extract_char(form);
}

/* song system moved to lua/characters/conditions/mystic_melody.lua */
void handle_songs() {}






/* do_song moved to lua/characters/commands/misc/song.lua */
ACMD(do_song) { (void)ch; (void)argument; (void)cmd; (void)subcmd; }

/* do_preference moved to lua/characters/commands/misc/preference.lua */

ACMD(do_moondust) {
  int64_t cost = GET_MAX_MOVE(ch) * 0.02, heal = 0;

  /* Can they do the technique? */

  if (!IS_ARLIAN(ch) || GET_SEX(ch) != SEX_FEMALE) {
    send_to_char(ch, "You are not an arlian female.\r\n");
    return;
  }

  if (!char_condition_has(ch, "group")) {
    send_to_char(ch, "You need to be in a group to use this skill!\r\n");
    return;
  }

  cost += (getMaxLF(ch)) * 0.02;
  heal = cost * 3;

  if (GET_HIT(ch) >= (getMaxPL(ch)) * 0.8) {
    cost = cost * 0.5;
  }

  if ((getCurST(ch)) < cost) {
    send_to_char(
        ch, "You do not have enough stamina to perform this technique.\r\n");
    return;
  }

  int chance = axion_dice(0);

  if (chance > GET_WIS(ch) + rand_number(1, 10)) {
    act("@GYou spread your wings and begin to concentrate. Your wings begin to "
        "glow a soft sea green color. As you prepare to release a cloud of "
        "your charged wing dust you lose focus and the power you had begun to "
        "charge into your wings dissipates.@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n@G spreads $s wings and seems to concentrate for a moment. "
        "Suddenly $s wings begin to glow a soft sea green color. This soft "
        "glow grows brighter for a second before fading completely.@n",
        TRUE, ch, 0, 0, TO_ROOM);
    decCurST(ch, cost);
    WAIT_STATE(ch, PULSE_1SEC);
    return;
  }
  incCurHealth(ch, heal);
  decCurST(ch, cost);
  WAIT_STATE(ch, PULSE_1SEC);

  act("@GYou spread your wings and begin to concentrate. Your wings begin to "
      "glow a soft sea green color. As your wings grow brighter you focus your "
      "charged bio energy in a shockwave the unleashes a cloud of glowing "
      "green dust. You breath in the dust and feel it rejuvinate your body's "
      "cells!@n",
      TRUE, ch, 0, 0, TO_CHAR);
  act("@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly "
      "$s wings begin to glow a soft sea green color. This soft glow grows "
      "brighter and as $e flexes $s wings to their full extent a shockwave of "
      "energy explodes outward. Carried on this shockwave is a cloud of "
      "glowing dust! You notice some of the dust being breathed in by $s!@n",
      TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "@RHeal@Y: @C%s@n\r\n", add_commas(heal));

  struct char_data *vict = NULL, *next_v = NULL;

  room_people_iterate(char_room_get(ch), [&](auto vict) {
    if (vict == ch) {
      return true;
    }
    if (char_condition_has(vict, "group")) {
      if (MASTER(ch) == MASTER(vict) || MASTER(vict) == ch ||
          MASTER(ch) == vict) {
        incCurHealth(vict, heal);
        act("@CYou breathe in the dust and are healed by it somewhat!@n", TRUE,
            vict, 0, 0, TO_CHAR);
        act("@c$n@C breathes in the dust and is healed somewhat!@n", TRUE, vict,
            0, 0, TO_ROOM);
        send_to_char(vict, "@RHeal@Y: @C%s@n\r\n", add_commas(heal));
      }
    }
    return true;
  });
}

/* do_shell moved to lua/characters/commands/misc/shell.lua */

ACMD(do_liquefy) {

  if (!IS_MAJIN(ch)) {
    send_to_char(ch, "You are not capable of liquefying yourself right now. "
                     "Try finding a giant blender maybe?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
    act("@MSuddenly large chunks of goo start to hover up slowly. These very "
        "same chunks quickly begin to fly into each other, piling on as the "
        "ball of goo grows. Suddenly @m$n@M emerges as the ball of goo takes "
        "$s shape!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    act("@MYou begin to pull the liquid chunks of your body together. Those "
        "chunks hover upward and merge into each other until a large ball of "
        "goo is formed. Slowly your body emerges as the pieces of your body "
        "take on their old form!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_LIQUEFIED);
    WAIT_STATE(ch, PULSE_3SEC);
    WAIT_STATE(ch, PULSE_3SEC);
    WAIT_STATE(ch, PULSE_3SEC);
    WAIT_STATE(ch, PULSE_3SEC);
    WAIT_STATE(ch, PULSE_3SEC);
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch,
                 "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
    return;
  }

  if ((getCurKI(ch)) < (GET_MAX_MANA(ch) * 0.002) + 150) {
    send_to_char(
        ch,
        "You do not have enough ki to manage this level of body control!\r\n");
    return;
  }

  if (!strcasecmp(arg, "hide")) {
    if (GRAPPLED(ch)) {
      struct char_data *other = GRAPPLED(ch);
      char_grappling_set(other, NULL, 0);
      char_grappled_set(ch, NULL, 0);
    }
    if (GRAPPLING(ch)) {
      struct char_data *other = GRAPPLING(ch);
      char_grappling_set(ch, NULL, 0);
      char_grappled_set(other, NULL, 0);
    }
    if (DRAGGING(ch)) {
      char_being_dragged_set(DRAGGING(ch), NULL);
      char_dragging_set(ch, NULL);
    }
    if (axion_dice(0) > GET_LEVEL(ch)) {
      act("@MYour body starts to become loose and sag, but you lose focus and "
          "it reverts to its original shape!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag, but $e seems to "
          "return normal a moment later.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (GET_MAX_MANA(ch) * .002) + 150);

      return;
    } else {
      act("@MYour body starts to become loose and sag. It continues to droop "
          "down until it begins to run down like a river of goo flowing from "
          "where your body was.@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag. Much of $s body "
          "begins to pour down and scatter around as pools of goo.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (GET_MAX_MANA(ch) * .002) + 150);
      SET_BIT_AR(AFF_FLAGS(ch), AFF_LIQUEFIED);
      return;
    }
  } else if (!strcasecmp(arg, "explode")) {
    struct char_data *vict;
    if (GRAPPLED(ch)) {
      struct char_data *other = GRAPPLED(ch);
      char_grappling_set(other, NULL, 0);
      char_grappled_set(ch, NULL, 0);
    }
    if (GRAPPLING(ch)) {
      struct char_data *other = GRAPPLING(ch);
      char_grappling_set(ch, NULL, 0);
      char_grappled_set(other, NULL, 0);
    }
    if (DRAGGING(ch)) {
      char_being_dragged_set(DRAGGING(ch), NULL);
      char_dragging_set(ch, NULL);
    }
    if (!*arg2) {
      send_to_char(
          ch, "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
      return;
    } else if ((getCurKI(ch)) < (GET_MAX_MANA(ch) * 0.10) + 150) {
      send_to_char(ch, "You do not have enough ki for that action!@n\r\n");
      return;
    } else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "That target isn't here.\r\n");
      return;
    } else if (!can_kill(ch, vict, NULL, 1)) {
      send_to_char(ch, "You can't kill them!\r\n");
      return;
    } else if (axion_dice(0) > GET_LEVEL(ch)) {
      act("@MYour body starts to become loose and sag, but you lose focus and "
          "it reverts to its original shape!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag, but $e seems to "
          "return normal a moment later.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (GET_MAX_MANA(ch) * .002) + 150);
      WAIT_STATE(ch, PULSE_3SEC);
      return;
    } else if (GET_SPEEDI(ch) < GET_SPEEDI(vict)) {
      act("@MYour body rapidly turns to liquid and flies for @R$N's@M open "
          "mouth! However $E easily dodges and avoids your attempt!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open "
          "mouth! However you are faster and managed to dodge the attempt.@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open "
          "mouth! However $E easily dodges and avoids @m$n's@M attempt!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, (GET_MAX_MANA(ch) * .002) + 150);
      if (!FIGHTING(ch)) {
        set_fighting(ch, vict);
      }
      if (!FIGHTING(vict)) {
        set_fighting(vict, ch);
      }
      WAIT_STATE(ch, PULSE_3SEC);
      WAIT_STATE(ch, PULSE_3SEC);
      return;
    } else if (GET_HIT(ch) < GET_HIT(vict) * 2) {
      act("@MYour body rapidly turns to liquid and flies for @R$N's@M open "
          "mouth! However as you force yourself in through $S mouth $E "
          "successfully resists and forces your back out!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open "
          "mouth! However you think quickly and force $m out before $e has a "
          "chance to get fully into your body!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open "
          "mouth! However as $e forces $mself in through @R$N's@M mouth $E "
          "manages to resist and force @m$n@M back out!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, (GET_MAX_MANA(ch) * .002) + 150);
      int64_t dmg = GET_MAX_HIT(ch) * 0.08;
      hurt(0, 0, ch, vict, NULL, dmg, 0);
      WAIT_STATE(ch, PULSE_3SEC);
      return;
    } else {
      act("@MYour body rapidly turns to liquid and flies for @R$N's@M open "
          "mouth! As you fill $S body you expand outward until $s body "
          "explodes into a gory mess!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open "
          "mouth! As $e fills your body it begins to expand until it is unable "
          "to take the strain any longer and explodes!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open "
          "mouth! As $e forces $mself in through @R$N's@M mouth $S body begins "
          "to expand until it can't take the strain any longer and explodes!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, (GET_MAX_MANA(ch) * .002) + 150);
      die(vict, ch);
      SET_BIT_AR(AFF_FLAGS(ch), AFF_LIQUEFIED);
      WAIT_STATE(ch, PULSE_3SEC);
      WAIT_STATE(ch, PULSE_3SEC);
      WAIT_STATE(ch, PULSE_3SEC);
      handle_cooldown(ch, 9);
      return;
    }
  } else {
    send_to_char(ch,
                 "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
    return;
  }
}

/* do_lifeforce moved to lua/characters/pcommands/info/lifeforce.lua */
/* do_defend moved to lua/characters/commands/misc/defend.lua */

/* fishing system moved to lua/characters/conditions/fishing.lua
   and lua/characters/commands/misc/fish.lua */
static int has_pole(struct char_data *ch) { (void)ch; return FALSE; }
static void catch_fish(struct char_data *ch, int quality) { (void)ch; (void)quality; }
static bool handle_fishing(struct char_data *ch) { (void)ch; return false; }
static void ev_fish_tick(int ctx_type, int64_t ctx_a, int64_t ctx_b) { (void)ctx_type; (void)ctx_a; (void)ctx_b; }
ACMD(do_fish) { (void)ch; (void)argument; (void)cmd; (void)subcmd; }

/* do_extract moved to lua/characters/commands/misc/extract.lua */
/* do_runic moved to lua/characters/commands/misc/runic.lua */

ACMD(do_scry) {

  if (strcasecmp(CAP(GET_NAME(ch)), "Galeos")) {
    send_to_char(ch, "You do not know how to perform that technique.\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: scry (target)\r\n");
    return;
  }

  struct char_data *vict;

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Who are you using Oracle Scry on?\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(ch, "You can't do that to yourself!\r\n");
    return;
  }

  if (IS_NPC(vict)) {
    send_to_char(ch, "No using this on mobs!\r\n");
    return;
  }

  int cost = 2000;

  if (GET_PRACTICES(ch, GET_CLASS(ch)) < cost) {
    send_to_char(ch, "You do not have enough PS to Oracle Scry!\r\n");
    return;
  } else {
    reveal_hiding(ch, 0);
    act("@GYou focus your mind and begin to allow the flood of images and "
        "energy to roar through your mind. You then allow those thoughts to "
        "make their way into the mind of @c$N@G. You can hardly comprehend the "
        "vastness of the information flooding in, yet still glimpse bits and "
        "pieces of your own destiny.@n",
        TRUE, ch, 0, vict, TO_CHAR);
    act("@GYou see @C$n@G begin to focus, and then without warning, your mind "
        "is flooded painfully with images, energy and information. The data "
        "streams in a mad torrent through your psyche, and just when you think "
        "snapping is possible, the voice of @C$n@G comes to you and eases and "
        "guides you. You see images of potential futures, information not yet "
        "known, knowledge yet undiscovered. Though you could not fully  grasp "
        "what is to come, you feel more prepared at facing the unknown.@n",
        TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@W appears to be performing some sort of ritual or something with "
        "@c$N@W.@n",
        TRUE, ch, 0, vict, TO_NOTVICT);
    int64_t boost = GET_INT(ch) * 0.5;

    gainBasePL(vict, (getBasePL(vict) * .01) * boost);
    gainBaseKI(vict, (getBaseKI(vict) * .01) * boost);
    gainBaseST(vict, (getBaseST(vict) * .01) * boost);

    send_to_char(
        vict,
        "Your Powerlevel, Ki, and Stamina have improved drastically! On top of "
        "that your Intelligence and Wisdom have improved permanantly!\r\n");
    char_stat_mod(vict, "intelligence", 2);
    char_stat_mod(vict, "wisdom", 2);
    char_stat_mod(ch, "practices", -2000);
    if (GET_LEVEL(ch) < 100) {
      send_to_char(ch, "@D[@mPractice Sessions@D:@R -2000@D]@n\r\n");
      if (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
        char_stat_mod(ch, "experience",
                      level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch));
        send_to_char(ch, "The remaining experience needed for your next level "
                         "up has been gained!@n\r\n");
      } else {
        send_to_char(ch, "Due to already having enough experience to level up "
                         "you gain no expereince.\r\n");
      }
    } else {
      gainBaseAllPercentTransformed(ch, .025, true);
      send_to_char(ch, "Your Powerlevel, Ki, and Stamina have improved!\r\n");
    }
  }
}

void ash_burn(struct char_data *ch) {

  if (ch && char_room_get(ch) != NULL) {
    room_contents_iterate(char_room_get(ch), [&](auto obj) {
      if (GET_OBJ_VNUM(obj) == 1306) {
        if (axion_dice(0) > GET_CON(ch)) {
          if (!IS_ANDROID(ch) && !IS_DEMON(ch) && !IS_ICER(ch)) {
            reveal_hiding(ch, 0);
            decCurST(ch, ((GET_MAX_MOVE(ch) * 0.005) + 20) * GET_OBJ_COST(obj));
            act("@RYou choke on the the burning hot "
                "@Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n",
                TRUE, ch, 0, 0, TO_CHAR);
            act("@r$n@R chokes on the burning hot "
                "@Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n",
                TRUE, ch, 0, 0, TO_ROOM);
          }
          if (!IS_ANDROID(ch) && !IS_DEMON(ch) && !IS_NPC(ch)) {
            if (!PLR_FLAGGED(ch, PLR_EYEC) && !AFF_FLAGGED(ch, AFF_BLIND)) {
              reveal_hiding(ch, 0);
              act("@DYour eyes sting from the hot ash! You can't see!@n", TRUE,
                  ch, 0, 0, TO_CHAR);
              act("@r$n@D eyes appear to have been hurt by the ash!@n", TRUE,
                  ch, 0, 0, TO_ROOM);
              int duration = 1;
              char_condition_apply_with_duration(ch, "ash_blinded", "skill", "ash_burn", duration * SECS_PER_MUD_HOUR);

            }
          }
        }
      }
      return true;
    });
  }
}

ACMD(do_ashcloud) {

  if (!IS_DEMON(ch)) {
    send_to_char(ch, "You are not trained in the use of ash and fire!\r\n");
    return;
  }
  int level = 1;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: ashcloud (1 | 2 | 3)\r\n");
    return;
  }

  struct obj_data *ash = NULL;
  int there = FALSE;

  char_inventory_iterate(ch, [&](auto obj) {
    if (GET_OBJ_VNUM(obj) == 1305) {
      ash = obj;
    }
    return true;
  });

  struct room_data *room = char_room_get(ch);

  room_contents_iterate(room, [&](auto obj) {
    if (GET_OBJ_VNUM(obj) == 1306) {
      there = TRUE;
    }
    return true;
  });

  if (there == TRUE) {
    send_to_char(ch, "You can not pile more ash into the air without causing "
                     "it to clump together and settle.\r\n");
    return;
  }

  if (!ash) {
    send_to_char(ch, "You do not have any ash!\r\n");
    return;
  }

  level = atoi(arg);

  int64_t mult = 5;
  double initial = 0.0;

  switch (level) {
  case 1:
    mult = 20;
    initial = 0.25;
    break;
  case 2:
    mult = 10;
    initial = 0.10;
    break;
  case 3:
    mult = 5;
    initial = 0.05;
    break;
  default:
    send_to_char(ch, "Syntax: ashcloud (1 | 2 | 3)\r\n");
    return;
  }

  int64_t cost = (GET_MAX_MANA(ch) * initial) + (GET_INT(ch) * mult);

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki!\r\n");
    return;
  } else if (room_is_sunken(char_room_get(ch))) {
    send_to_char(
        ch, "You can not create an ashcloud here, because it is too wet.\r\n");
    return;
  } else if (room_sector_type_get(char_room_get(ch)) == SECT_SPACE) {
    send_to_char(ch, "You can not create an ashcloud in space.\r\n");
    return;
  } else if (GET_INT(ch) < axion_dice(-10)) {
    reveal_hiding(ch, 0);
    act("@RYou take a handful of ashes, and when you go to blow flames at it "
        "you lose focus. The ashes are blown from your hands by your huge gust "
        "of breath.@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@r$n@R takes a handful of ashes from $s belongings and blows it out "
        "of $s hands with a strong gust of air. @YStrange.@n",
        TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(ash);
    decCurKI(ch, cost);
    return;
  } else {
    struct obj_data *ashcloud;
    reveal_hiding(ch, 0);
    if (level == 3) {
      decCurKI(ch, cost);
      act("@RYou take a handful of ashes and you create a fierce heat within "
          "your lungs. With the heat ready you breathe ki infused flames at "
          "the pile of ashes! The flames and ashes mix and fill the "
          "surrounding area with a hot burning ash!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R takes a handful of ashes and $e breathes ki infused flames "
          "at the pile of ashes! The flames and ashes mix and fill the "
          "surrounding area with a hot burning ash!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      send_to_room(
          room,
          "@WThe ashes ripple with an intense aftershock of power.@n\r\n");
      ashcloud = read_object(1306, VIRTUAL);
      obj_to_room(ashcloud, char_room_get(ch));
      extract_obj(ash);
      GET_OBJ_TIMER(ashcloud) = 4;
      GET_OBJ_COST(ashcloud) = 3;
      ash_burn(ch);
    } else if (level == 2) {
      decCurKI(ch, cost);
      act("@RYou take a handful of ashes and you create a fierce heat within "
          "your lungs. With the heat ready you breathe ki infused flames at "
          "the pile of ashes! The flames and ashes mix and fill the "
          "surrounding area with a hot burning ash!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R takes a handful of ashes and $e breathes ki infused flames "
          "at the pile of ashes! The flames and ashes mix and fill the "
          "surrounding area with a hot burning ash!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      send_to_room(
          room, "@WThe ashes ripple with a strong aftershock of power.@n\r\n");
      ashcloud = read_object(1306, VIRTUAL);
      obj_to_room(ashcloud, char_room_get(ch));
      GET_OBJ_TIMER(ashcloud) = 2;
      GET_OBJ_COST(ashcloud) = 2;
      extract_obj(ash);
      ash_burn(ch);
    } else {
      decCurKI(ch, cost);
      act("@RYou take a handful of ashes and you create a fierce heat within "
          "your lungs. With the heat ready you breathe ki infused flames at "
          "the pile of ashes! The flames and ashes mix and fill the "
          "surrounding area with a hot burning ash!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R takes a handful of ashes and $e breathes ki infused flames "
          "at the pile of ashes! The flames and ashes mix and fill the "
          "surrounding area with a hot burning ash!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      ashcloud = read_object(1306, VIRTUAL);
      obj_to_room(ashcloud, char_room_get(ch));
      extract_obj(ash);
      GET_OBJ_TIMER(ashcloud) = 1;
      GET_OBJ_COST(ashcloud) = 1;
      ash_burn(ch);
    }
  }
}

ACMD(do_resize) {
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  two_arguments(argument, arg, arg2);

  if (!GET_SKILL(ch, SKILL_BUILD)) {
    send_to_char(ch, "You do not have the skill to resize equipment!\r\n");
    return;
  } else if (GET_SKILL(ch, SKILL_BUILD) < 80) {
    send_to_char(ch, "Your build skill must be at least level 80 before you "
                     "can resize equipment.\r\n");
    return;
  } else {
    if (!*arg || !*arg2) {
      send_to_char(ch, "Syntax: resize (obj) (small | medium)\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You don't have that object!\r\n");
      return;
    } else {
      if (!wearable_obj(obj)) {
        send_to_char(
            ch, "That is not equipment! You can only resize equipment.\r\n");
        return;
      } else {
        if ((getCurST(ch)) < GET_OBJ_WEIGHT(obj) + (GET_MAX_MOVE(ch) / 40)) {
          send_to_char(ch, "You do not have enough stamina to resize this "
                           "object at this time.\r\n");
          return;
        } else if (!strcasecmp(arg2, "small")) {
          if (GET_OBJ_SIZE(obj) == SIZE_SMALL) {
            send_to_char(ch, "The equipment is already small sized.\r\n");
            return;
          } else {
            act("@WYou carefully adjust the size of @c$p@W.@n", TRUE, ch, obj,
                0, TO_CHAR);
            act("@C$n@W carefully adjusts the size of @c$p@W.@n", TRUE, ch, obj,
                0, TO_ROOM);
            GET_OBJ_SIZE(obj) = SIZE_SMALL;
            decCurST(ch, GET_OBJ_WEIGHT(obj) + (GET_MAX_MOVE(ch) / 40));
          }
        } else if (!strcasecmp(arg2, "medium")) {
          if (GET_OBJ_SIZE(obj) == SIZE_MEDIUM) {
            send_to_char(ch, "The equipment is already medium sized.\r\n");
            return;
          } else {
            act("@WYou carefully adjust the size of @c$p@W.@n", TRUE, ch, obj,
                0, TO_CHAR);
            act("@C$n@W carefully adjusts the size of @c$p@W.@n", TRUE, ch, obj,
                0, TO_ROOM);
            GET_OBJ_SIZE(obj) = SIZE_MEDIUM;
            decCurST(ch, GET_OBJ_WEIGHT(obj) + (GET_MAX_MOVE(ch) / 40));
          }
        } else {
          send_to_char(ch, "Syntax: resize (obj) (small | medium)\r\n");
        }
      }
    }
  }
}

/* do_healglow moved to lua/characters/commands/misc/healglow.lua */

/* do_metamorph moved to lua/characters/commands/misc/metamorph.lua */

ACMD(do_shimmer) {

  int skill = 0, perc = 0, location = 0;
  int64_t cost = 0;
  struct char_data *tar = NULL;

  char arg[MAX_INPUT_LENGTH] = "";

  one_argument(argument, arg);

  if (!IS_NPC(ch)) {
    if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
      ARENA_IDNUM(ch) = -1;
      send_to_char(ch, "You stop watching the arena action.\r\n");
    }
  }
  if (strcasecmp(GET_NAME(ch), "Anubis")) {
    send_to_char(ch, "You do not even know how to perform that skill!\r\n");
    return;
  } else if (PLR_FLAGGED(ch, PLR_PILOTING)) {
    send_to_char(ch, "You are busy piloting a ship!\r\n");
    return;
  } else if (PLR_FLAGGED(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return;
  } else if (char_room_vnum_get(ch) >= 19800 &&
             char_room_vnum_get(ch) <= 19899) {
    send_to_char(ch, "@rYou are in a pocket dimension!@n\r\n");
    return;
  } else if (!*arg) {
    send_to_char(ch, "Who or where do you want to shimmer to? [target | "
                     "planet-(planet name) | afterlife]\r\n");
    send_to_char(ch,
                 "Example: shimmer goku\nExample 2: shimmer planet-earth\r\n");
    return;
  }

  cost = GET_MAX_MANA(ch) / 40;

  if ((getCurKI(ch)) - cost < 0) {
    send_to_char(ch, "You do not have enough ki to instantaneously move.\r\n");
    return;
  }

  perc = axion_dice(0);
  skill = 100;

  if (!strcasecmp(arg, "planet-earth")) {
    location = 300;
  } else if (!strcasecmp(arg, "planet-namek")) {
    location = 10222;
  } else if (!strcasecmp(arg, "planet-frigid")) {
    location = 4017;
  } else if (!strcasecmp(arg, "planet-vegeta")) {
    location = 2200;
  } else if (!strcasecmp(arg, "planet-konack")) {
    location = 8006;
  } else if (!strcasecmp(arg, "planet-aether")) {
    location = 12024;
  } else if (!strcasecmp(arg, "afterlife")) {
    location = 6000;
  } else if (!(tar = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "@RThat target doesn't exist.@n\r\n");
    send_to_char(ch, "Who or where do you want to shimmer to? [target | "
                     "planet-(planet name) | afterlife]\r\n");
    send_to_char(ch,
                 "Example: shimmer goku\nExample 2: shimmer planet-earth\r\n");
    return;
  }

  if (skill < perc || (FIGHTING(ch) && rand_number(1, 2) <= 1)) {
    if (tar != NULL) {
      if (tar != ch) {
        send_to_char(ch, "You prepare to move instantly but mess up the "
                         "process and waste some of your ki!\r\n");
        decCurKI(ch, cost);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
      } else {
        send_to_char(
            ch, "Moving to yourself would be kinda impossible wouldn't it? If "
                "not that then it would at least be pointless.\r\n");
        return;
      }
    } else {
      send_to_char(ch, "You prepare to move instantly but mess up the process "
                       "and waste some of your ki!\r\n");
      decCurKI(ch, cost);
      WAIT_STATE(ch, PULSE_2SEC);
      return;
    }
  }

  reveal_hiding(ch, 0);
  WAIT_STATE(ch, PULSE_2SEC);
  if (tar != NULL) {
    if (tar == ch) {
      send_to_char(ch,
                   "Moving to yourself would be kinda impossible wouldn't it? "
                   "If not that then it would at least be pointless.\r\n");
      return;
    } else if (GRAPPLING(ch) && GRAPPLING(ch) == tar) {
      send_to_char(ch, "You are already in the same room with them and are "
                       "grappling with them!\r\n");
      return;
    } else if (GET_ADMLEVEL(tar) > 0 && GET_ADMLEVEL(ch) < 1) {
      send_to_char(ch, "That immortal prevents you from reaching them.\r\n");
      return;
    } else if (room_flagged(char_room_get(tar), ROOM_NOINSTANT)) {
      send_to_char(ch, "You can not go there as it is a protected area!\r\n");
      return;
    } else if (GRAPPLING(ch) && AFF_FLAGGED(GRAPPLING(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    } else if (DRAGGING(ch) && AFF_FLAGGED(DRAGGING(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    } else if (GRAPPLED(ch) && AFF_FLAGGED(GRAPPLED(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    }

    decCurKI(ch, cost);
    act("@wYour body begins to fade away almost appearing ghost like, before a "
        "ripple passes through your image and your are gone in an instant!@n",
        TRUE, ch, 0, tar, TO_CHAR);
    act("@w$n@w appears in an instant out of nowhere right next to you!@n",
        TRUE, ch, 0, tar, TO_VICT);
    act("@w$n@w body begins to fade away almost appearing ghost like, before a "
        "ripple passes through $s image and $e is gone in an instant!@n",
        TRUE, ch, 0, tar, TO_NOTVICT);
    SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANSMISSION);
    handle_teleport(ch, tar, 0);
  } else {
    decCurKI(ch, cost);
    act("@wYour body begins to fade away almost appearing ghost like, before a "
        "ripple passes through your image and your are gone in an instant!@n",
        TRUE, ch, 0, tar, TO_CHAR);
    act("@w$n@w body begins to fade away almost appearing ghost like, before a "
        "ripple passes through $s image and $e is gone in an instant!@n",
        TRUE, ch, 0, tar, TO_NOTVICT);
    handle_teleport(ch, NULL, location);
  }
}

ACMD(do_channel) {

  if (!IS_DEMON(ch) || GET_SKILL_BASE(ch, SKILL_STYLE) < 40) {
    send_to_char(ch, "You are not a Demon!\r\n");
    return;
  }

  if (GET_SKILL_BASE(ch, SKILL_STYLE) < 40) {
    send_to_char(ch,
                 "This requires a fighting style at level 40 or more!!\r\n");
    return;
  }

  int64_t cost = GET_MAX_MANA(ch) * 0.15;

  int chance = axion_dice(0), skill = GET_SKILL(ch, SKILL_STYLE);

  if (cost > (getCurKI(ch))) {
    send_to_char(ch, "You do not have enough ki to channel with!\r\n");
    return;
  }

  struct obj_data *ruby = NULL;
  int found = FALSE;

  char_inventory_iterate(ch, [&](auto obj) {
    if (found == FALSE && GET_OBJ_VNUM(obj) == 6600) {
      if (!OBJ_FLAGGED(obj, ITEM_HOT)) {
        found = TRUE;
        ruby = obj;
      }
    }
    return true;
  });

  if (found == FALSE) {
    send_to_char(ch, "You do not have any uncharged blood rubies.\r\n");
    return;
  }

  if (room_geffect_get(char_room_get(ch)) <= 0) {
    send_to_char(ch, "There is no lava here!\r\n");
    return;
  }

  if (ruby) {
    if (skill < chance) {
      act("@RAs you move your ki through the lava you begin to draw heat away "
          "from it into the ruby. You screw up the rate of heating though and "
          "cause the ruby to crumble to dust!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@RAs $n@R moves $s ki through the lava $e begins to draw heat away "
          "from it into a blood ruby. However $e screws up the rate of heating "
          "and causes the ruby to crumble to dust!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(ruby);
    } else {
      act("@RAs you move your ki through the lava you begin to draw heat away "
          "from it into the ruby. You do so at an even rate and end up with a "
          "glowing red hot blood ruby!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@RAs $n@R moves $s ki through the lava $e begins to draw heat away "
          "from it into a blood ruby. The ruby glows red hot as $e finishes "
          "the process of channeling the heat!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      room_geffect_set(char_room_get(ch), 0);
      SET_BIT_AR(GET_OBJ_EXTRA(ruby), ITEM_HOT);
    }
    decCurKI(ch, cost);
    WAIT_STATE(ch, PULSE_1SEC);
  }
}

ACMD(do_hydromancy) {
}

void rpp_feature(struct char_data *ch, const char *arg) {
  int cost = 0, change = FALSE;

  if (!*arg) {
    send_to_char(
        ch,
        "Syntax: rpp 13 (description)\nExample: rpp 13 a large red scar on his "
        "face\nDisplayed to others: He has a large red scar on his face.\r\n");
    return;
  }

  if (strlen(arg) > 60) {
    send_to_char(ch, "Please limit it to 60 characters.\r\n");
    return;
  }

  if (!GET_FEATURE(ch)) {
    cost = 2;
  } else {
    cost = 2;
    change = TRUE;
  }

  if (cost > GET_RP(ch)) {
    send_to_char(ch, "You do not have enough RPP for that!\r\n");
    return;
  } else {
    char sex[128], buf8[MAX_INPUT_LENGTH];
    sprintf(sex, "%s",
            GET_SEX(ch) == SEX_FEMALE ? "She"
            : GET_SEX(ch) == SEX_MALE ? "He"
                                      : "It");

    GET_RP(ch) -= cost;
    sprintf(buf8, "...%s has %s.", sex, arg);
    send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
    send_to_char(ch,
                 "You now have the following line underneath you when someone "
                 "sees you in a room as:\n@C%s@n\r\n",
                 buf8);
    GET_FEATURE(ch) = strdup(buf8);
    if (change == TRUE) {
      send_to_imm("%s has altered their extra description. Make sure the "
                  "reason is legit! If it is then reimb them 2 RPP.\r\n",
                  GET_USER(ch));
      send_to_char(ch, "The immortals have been notified about this change. It "
                       "had better have been for a good reason.\r\n");
    }
    mud_log("%s RPP Feature: '%s' Check for rule compliance.", GET_USER(ch), buf8);
    return;
  }
}

ACMD(do_instill) {

  if (IS_NPC(ch))
    return;

  struct obj_data *obj, *token;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(token = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, inv_for_char(ch)))) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!OBJ_FLAGGED(token, ITEM_TOKEN)) {
    send_to_char(ch, "That is not a token.\r\n");
    return;
  }

  if (OBJ_FLAGGED(token, ITEM_FORGED)) {
    send_to_char(ch, "That token is a forgery!\r\n");
    return;
  }

  if (!wearable_obj(obj)) {
    send_to_char(ch, "You can only instill tokens into equipment.\r\n");
    return;
  }

  if (!OBJ_FLAGGED(obj, ITEM_SLOT1) && !OBJ_FLAGGED(obj, ITEM_SLOT2)) {
    send_to_char(ch, "That piece of equipment does not have any slots.\r\n");
    return;
  }

  if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "That piece of equipment has already had its token slots "
                     "filled. This can not be reversed.");
    return;
  } else { /* It has at least one open slot. */
    int stat = 0, raise = 0;
    stat = token->affected[0].location;

    if (obj->affected[0].location != 0 && obj->affected[1].location != 0 &&
        obj->affected[2].location != 0 && obj->affected[3].location != 0 &&
        obj->affected[4].location != 0 && obj->affected[5].location != 0) {
      if (obj->affected[0].location != stat &&
          obj->affected[1].location != stat &&
          obj->affected[2].location != stat &&
          obj->affected[3].location != stat &&
          obj->affected[4].location != stat &&
          obj->affected[5].location != stat) {
        send_to_char(
            ch, "This already has as many different stats as it can hold.\r\n");
        return;
      }
    }

    act("@GYou instill the token into @g$p@G. It glows @ggreen@G for a moment "
        "before returning to normal. The token disappears with the glow.@n",
        TRUE, ch, obj, 0, TO_CHAR);
    act("@g$n@G instills a token into @g$p@G. It glows @ggreen@G for a moment "
        "before returning to normal. The token disappears with the glow.@n",
        TRUE, ch, obj, 0, TO_ROOM);
    raise = token->affected[0].modifier;
    extract_obj(token);

    if (OBJ_FLAGGED(obj, ITEM_SLOT1))
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOTS_FILLED);
    else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOT_ONE))
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOT_ONE);
    else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && OBJ_FLAGGED(obj, ITEM_SLOT_ONE))
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOTS_FILLED);

    /* Check it's slots for the appropriate stat and add to it if possible */
    if (obj->affected[0].location == stat) {
      obj->affected[0].modifier += raise;
    } else if (obj->affected[1].location == stat) {
      obj->affected[1].modifier += raise;
    } else if (obj->affected[2].location == stat) {
      obj->affected[2].modifier += raise;
    } else if (obj->affected[3].location == stat) {
      obj->affected[3].modifier += raise;
    } else if (obj->affected[4].location == stat) {
      obj->affected[4].modifier += raise;
    } else if (obj->affected[5].location == stat) {
      obj->affected[5].modifier += raise;
    } else if (obj->affected[0].location ==
               0) { /* It's empty, put it here regardless */
      obj->affected[0].location = stat;
      obj->affected[0].modifier = raise;
    } else if (obj->affected[1].location ==
               0) { /* It's empty, put it here regardless */
      obj->affected[1].location = stat;
      obj->affected[1].modifier = raise;
    } else if (obj->affected[2].location ==
               0) { /* It's empty, put it here regardless */
      obj->affected[2].location = stat;
      obj->affected[2].modifier = raise;
    } else if (obj->affected[3].location ==
               0) { /* It's empty, put it here regardless */
      obj->affected[3].location = stat;
      obj->affected[3].modifier = raise;
    } else if (obj->affected[4].location ==
               0) { /* It's empty, put it here regardless */
      obj->affected[4].location = stat;
      obj->affected[4].modifier = raise;
    } else if (obj->affected[5].location ==
               0) { /* It's empty, put it here regardless */
      obj->affected[5].location = stat;
      obj->affected[5].modifier = raise;
    }
  }
}

/* do_hayasa moved to lua/characters/commands/misc/hayasa.lua */

/* This is the mortal dig command. */
ACMD(do_bury) {

  if (!HAS_ARMS(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
    send_to_char(ch, "You are busy grappling with someone!\r\n");
    return;
  }

  if (ABSORBING(ch) || ABSORBBY(ch)) {
    send_to_char(ch, "You are busy struggling with someone!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }

  int sect = room_sector_type_get(char_room_get(ch));

  if (sect != SECT_FIELD && sect != SECT_HILLS && sect != SECT_FOREST &&
      sect != SECT_DESERT && sect != SECT_MOUNTAIN) {
    send_to_char(
        ch,
        "You are not in a room with enough available dirt or sand to dig.\r\n");
    return;
  }

  struct obj_data *obj = NULL, *fobj = NULL;

  room_contents_iterate(char_room_get(ch), [&](auto buried) {
    if (OBJ_FLAGGED(buried, ITEM_BURIED)) {
      fobj = buried;
    }
    return true;
  });

  if (!strcasecmp(arg, "bury")) {
    if (!*arg2) {
      send_to_char(ch, "Bury what?\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You don't have that object to bury.\r\n");
      return;
    } else if (fobj != NULL) {
      send_to_char(ch, "There is already something buried near here.\r\n");
      return;
    } else {
      if (sect != SECT_DESERT) {
        act("@yYou start digging in a spot of soft dirt. Once you have an "
            "appropriately sized hole you drop @G$p@y in and then cover it.@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@y starts digging in a spot of soft dirt. Once $e has an "
            "appropriately sized hole $e drops @G$p@y in and then covers it.@n",
            TRUE, ch, obj, 0, TO_ROOM);
      } else {
        act("@YYou start digging in a spot of soft sand. Once you have an "
            "appropriately sized hole you drop @G$p@Y in and then cover it.@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@Y starts digging in a spot of soft sand. Once $e has an "
            "appropriately sized hole $e drops @G$p@Y in and then covers it.@n",
            TRUE, ch, obj, 0, TO_ROOM);
      }
      obj_from_char(obj);
      obj_to_room(obj, char_room_get(ch));
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BURIED);
    }
  } else if (!strcasecmp(arg, "uncover")) {
    if (fobj == NULL) {
      send_to_char(ch, "There is nothing buried here.\r\n");
      return;
    } else {
      if (sect != SECT_DESERT) {
        act("@yYou slowly dig and reveal @G$p@y buried in the dirt! You pull "
            "it out and set it on the ground before covering the hole back "
            "up.@n",
            TRUE, ch, fobj, 0, TO_CHAR);
        act("@C$n@y starts digging and shortly reveals @G$p@y buried in the "
            "dirt! Quickly $e pulls it out and sets it on the ground before "
            "covering the hole back up.@n",
            TRUE, ch, fobj, 0, TO_ROOM);
      } else {
        act("@YYou slowly dig and reveal @G$p@Y buried in the sand! You pull "
            "it out and set it on the ground before covering the hole back "
            "up.@n",
            TRUE, ch, fobj, 0, TO_CHAR);
        act("@C$n@Y starts digging and shortly reveals @G$p@Y buried in the "
            "sand! Quickly $e pulls it out and sets it on the ground before "
            "covering the hole back up.@n",
            TRUE, ch, fobj, 0, TO_ROOM);
      }
      REMOVE_BIT_AR(GET_OBJ_EXTRA(fobj), ITEM_BURIED);
    }
  } else {
    send_to_char(ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }
}

ACMD(do_arena) {

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (IN_ARENA(ch)) {
    send_to_char(ch, "You are too busy competing to be a spectator.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch,
                 "Syntax: arena (fighter number of participant)\r\n        "
                 "arena look\r\n        arena scan\r\n        arena stop\r\n");
    return;
  } else if (!strcasecmp(arg, "stop")) {
    send_to_char(ch, "You stop viewing what's going on in the arena.\r\n");
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
    ARENA_IDNUM(ch) = -1;
    return;
  } else if (char_room_vnum_get(ch) != 17875) {
    send_to_char(ch,
                 "You are not close enough to the arena floor to see it.\r\n");
    return;
  } else if (!strcasecmp(arg, "look")) {
    if (!PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
      send_to_char(ch, "You are not even watching anyone in the arena.\r\n");
      return;
    } else if (arena_watch(ch) != NOWHERE) {
      look_at_room(room_by_id(arena_watch(ch)), ch, 0);
    }
  } else if (!strcasecmp(arg, "scan")) {
    if (char_room_vnum_get(ch) == 17875) {
      int found = FALSE;
      struct descriptor_data *d;

      send_to_char(ch, "@D---@CFighters in the arena@D---@n\r\n");
      for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING)
          continue;

        if (IN_ARENA(d->character)) {
          char buf[MAX_INPUT_LENGTH];
          sprintf(buf, "@YFighter Number@D: @w%d, $N.@n",
                  GET_IDNUM(d->character));
          act(buf, TRUE, ch, 0, d->character, TO_CHAR);
          found = TRUE;
        }
      }

      if (found == FALSE) {
        send_to_char(ch, "@wNone.@n\r\n");
      }
    } else {
      send_to_char(ch, "You are not close enough to see what fighters are in "
                       "the arena.\r\n");
      return;
    }
  } else {
    int num = atoi(arg);

    if (num < 0) {
      send_to_char(ch, "That is not a valid fighter number\r\n");
      return;
    } else {
      struct descriptor_data *d;
      int found = FALSE;

      for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING)
          continue;

        if (GET_IDNUM(d->character) == num) {
          if (IN_ARENA(d->character)) {
            found = TRUE;
          }
        }
      }

      if (found == TRUE) {
        act("@wYou start watching the action surrounding that particular "
            "fighter in the arena.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w starts watching the action in the arena.@n", TRUE, ch, 0, 0,
            TO_ROOM);
        SET_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
        ARENA_IDNUM(ch) = num;
      } else {
        send_to_char(
            ch, "A fighter with such a number was not found in the arena.\r\n");
        return;
      }
    } /* Secondary else end */
  } /* Main Else end */
} /* End of Arena Function */

ACMD(do_ensnare) {

  if (!know_skill(ch, SKILL_ENSNARE)) {
    return;
  }

  struct obj_data *obj = NULL;
  int found = FALSE;

  char_inventory_iterate(ch, [&](auto weave) {
    if (found == FALSE && valid_silk(weave) &&
        !OBJ_FLAGGED(weave, ITEM_FORGED)) {
      found = TRUE;
      obj = weave;
    }
    return true;
  });

  if (found == FALSE) {
    send_to_char(
        ch,
        "You do not have a bundle of silk to ensnare an opponent with!\r\n");
    return;
  } else {
    int prob = GET_SKILL(ch, SKILL_ENSNARE), perc = axion_dice(0);
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;

    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(ch, "Syntax: ensnare (target)\r\n");
      return;
    }

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Who are you trying to target with ensnare?\r\n");
      return;
    } else if (AFF_FLAGGED(vict, AFF_ENSNARED)) {
      send_to_char(ch, "They are already ensnared!\r\n");
      return;
    } else if (!HAS_ARMS(vict)) {
      send_to_char(ch, "They don't have arms to ensnare!\r\n");
      return;
    } else if (prob <= perc) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand you "
          "swing the strand at @c$N@W! Unfortunately you miss and lose the "
          "bundle...@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at YOU! Fortunately $e misses and loses the "
          "bundle...@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at @c$N@W! Fortunately $e misses and loses the "
          "bundle...@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      WAIT_STATE(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
    } else if (char_condition_has(vict, "zanzoken") &&
               !char_condition_has(ch, "zanzoken")) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand you "
          "swing the strand at @c$N@W! Unfortunately @c$N@W zanzokens away "
          "avoiding it and you lose the bundle...@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at YOU! Fortunately you zanzoken away avoiding it "
          "and @C$n@W loses the bundle...@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at @c$N@W! Fortunately @c$N@W zanzokens away "
          "avoiding it and @C$n@W loses the bundle...@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      WAIT_STATE(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
      char_condition_remove(vict, "zanzoken", "zanzoken_over");
    } else if (char_condition_has(vict, "zanzoken") &&
               char_condition_has(ch, "zanzoken")) {
      if (GET_SPEEDI(ch) + rand_number(1, 100) <
          GET_SPEEDI(vict) + rand_number(1, 100)) {
        act("@WYou unwind your bundle of silk and grab a loose end of it. "
            "Splitting that end to reveal the sticky innards of the strand you "
            "swing the strand at @c$N@W! You both zanzoken! Unfortunately "
            "@c$N@W manages to avoid it and you lose the bundle...@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
            "Splitting that end to reveal the sticky innards of the strand $e "
            "swings the strand at YOU! You both zanzoken! Fortunately you "
            "manage to avoid it and @C$n@W loses the bundle...@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
            "Splitting that end to reveal the sticky innards of the strand $e "
            "swings the strand at @c$N@W! They both zanzoken! Fortunately "
            "@c$N@W manages to avoid it and @C$n@W loses the bundle...@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        extract_obj(obj);
        WAIT_STATE(ch, PULSE_3SEC);
        improve_skill(ch, SKILL_ENSNARE, 0);
        char_condition_remove(vict, "zanzoken", "zanzoken_over");
        char_condition_remove(ch, "zanzoken", "zanzoken_over");
      } else {
        act("@WYou unwind your bundle of silk and grab a loose end of it. "
            "Splitting that end to reveal the sticky innards of the strand you "
            "swing the strand at @c$N@W! Fortunately you manage to hit $M! You "
            "both zanzoken! Quickly you spin around $M and ensnare $S arms "
            "with the silk!@n",
            TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
            "Splitting that end to reveal the sticky innards of the strand $e "
            "swings the strand at YOU! Unfortunately $e manages to hit YOU! "
            "You both zanzoken! Quickly $e spins around you and ensnares your "
            "arms with the silk!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
            "Splitting that end to reveal the sticky innards of the strand $e "
            "swings the strand at @c$N@W! Unfortunately $e manages to hit $M! "
            "They both zanzoken! Quickly $e spins around @c$N@W and ensnares "
            "$S arms with the silk!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        extract_obj(obj);
        char_condition_apply(vict, "ensnared", "skill", "ensnare");
        WAIT_STATE(ch, PULSE_3SEC);
        improve_skill(ch, SKILL_ENSNARE, 0);
        char_condition_remove(vict, "zanzoken", "zanzoken_over");
        char_condition_remove(ch, "zanzoken", "zanzoken_over");
      }
    } else if (char_condition_has(ch, "zanzoken") &&
               !char_condition_has(vict, "zanzoken")) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand you "
          "swing the strand at @c$N@W! Fortunately you manage to hit $M! "
          "Quickly you zanzoken and spin around $M and ensnare $S arms with "
          "the silk!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at YOU! Unfortunately $e manages to hit YOU! "
          "Quickly $e zanzokens and spins around you and ensnares your arms "
          "with the silk!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at @c$N@W! Unfortunately $e manages to hit $M! "
          "Quickly $e zanzokens and spins around @c$N@W and ensnares $S arms "
          "with the silk!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_condition_apply(vict, "ensnared", "skill", "ensnare");
      WAIT_STATE(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
      char_condition_remove(ch, "zanzoken", "zanzoken_over");
    } else if (GET_SPEEDI(ch) + rand_number(1, 100) <
               GET_SPEEDI(vict) + rand_number(1, 100)) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand you "
          "swing the strand at @c$N@W! Unfortunately @c$N@W manages to avoid "
          "it and you lose the bundle...@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at YOU! Fortunately you manage to avoid it and "
          "@C$n@W loses the bundle...@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at @c$N@W! Fortunately @c$N@W manages to avoid it "
          "and @C$n@W loses the bundle...@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      WAIT_STATE(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
    } else {
      act("@WYou unwind your bundle of silk and grab a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand you "
          "swing the strand at @c$N@W! Fortunately you manage to hit $M! "
          "Quickly you spin around $M and ensnare $S arms with the silk!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at YOU! Unfortunately $e manages to hit YOU! "
          "Quickly $e spins around you and ensnares your arms with the silk!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. "
          "Splitting that end to reveal the sticky innards of the strand $e "
          "swings the strand at @c$N@W! Unfortunately $e manages to hit $M! "
          "Quickly $e spins around @c$N@W and ensnares $S arms with the "
          "silk!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_condition_apply(vict, "ensnared", "skill", "ensnare");
      WAIT_STATE(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
    }
  } /* Main else function */
}

/* This determines of an object is a suitable bundle of silk or not */
static int valid_silk(struct obj_data *obj) {
  int value = 0;

  switch (GET_OBJ_VNUM(obj)) {
  case 16700:
  case 16701:
  case 16702:
  case 16703:
  case 16704:
  case 16708:
    value = 1;
    break;
  }

  return (value);
}

ACMD(do_silk) {

  if (!know_skill(ch, SKILL_SILK)) {
    return;
  }

  struct obj_data *obj = NULL, *weaved = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);
  int prob = GET_SKILL(ch, SKILL_SILK), perc = rand_number(1, 120);

  if (!*arg) {
    send_to_char(ch, "Syntax: silk (weave | bundle)\r\n");
    return;
  }

  if (!strcasecmp(arg, "weave")) {

    if (!*arg2) {
      send_to_char(ch, "Syntax: silk weave (head | wrist | belt)\r\n");
      return;
    }

    int found = FALSE, armor = 500, str = 0, intel = 0, olevel = 0;
    double price = 1;

    char_inventory_iterate(ch, [&](auto weave) {
      if (found == FALSE && valid_silk(weave) &&
          !OBJ_FLAGGED(weave, ITEM_FORGED)) {
        found = TRUE;
        obj = weave;
      }
      return true;
    });

    if (found == FALSE) {
      send_to_char(ch, "You do not have an acceptable bundle of silk in your "
                       "inventory!\r\n");
      return;
    } else {
      if (!strcasecmp(arg2, "head")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up "
              "ruining the entire bundle instead.@n",
              TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but "
              "ends up ruining the entire bundle instead.@n",
              TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          WAIT_STATE(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16705, VIRTUAL);
          obj_to_room(weaved, char_room_get(ch));
          if (GET_OBJ_VNUM(obj) == 16708) {
            armor *= 20;
            str = 4;
            intel = 4;
            price = 4;
            olevel = 80;
          } else if (GET_OBJ_VNUM(obj) == 16700) {
            armor *= 15;
            str = 3;
            intel = 3;
            price = 4;
            olevel = 75;
          } else if (GET_OBJ_VNUM(obj) == 16701) {
            armor *= 10;
            str = 3;
            intel = 3;
            price = 3;
            olevel = 50;
          } else if (GET_OBJ_VNUM(obj) == 16702) {
            armor *= 6;
            str = 2;
            intel = 2;
            price = 2;
            olevel = 25;
          } else if (GET_OBJ_VNUM(obj) == 16703) {
            armor *= 4;
            str = 1;
            intel = 1;
            price = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          GET_OBJ_COST(weaved) *= price;
          GET_OBJ_VAL(weaved, 0) = olevel;
          obj_level_set(weaved, olevel);
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n",
              TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to "
              "create $p@W!@n",
              TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, "headsash", 0, 0);
          extract_obj(obj);
          WAIT_STATE(ch, PULSE_4SEC);
        }
      } else if (!strcasecmp(arg2, "wrist")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up "
              "ruining the entire bundle instead.@n",
              TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but "
              "ends up ruining the entire bundle instead.@n",
              TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          WAIT_STATE(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16706, VIRTUAL);
          obj_to_room(weaved, char_room_get(ch));
          if (GET_OBJ_VNUM(obj) == 16708) {
            armor *= 20;
            str = 4;
            intel = 4;
            price = 4;
            olevel = 80;
          } else if (GET_OBJ_VNUM(obj) == 16700) {
            armor *= 15;
            str = 3;
            intel = 3;
            price = 4;
            olevel = 75;
          } else if (GET_OBJ_VNUM(obj) == 16701) {
            armor *= 10;
            str = 3;
            intel = 3;
            price = 3;
            olevel = 50;
          } else if (GET_OBJ_VNUM(obj) == 16702) {
            armor *= 6;
            str = 2;
            intel = 2;
            price = 2;
            olevel = 25;
          } else if (GET_OBJ_VNUM(obj) == 16703) {
            armor *= 4;
            str = 1;
            intel = 1;
            price = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          GET_OBJ_COST(weaved) *= price;
          GET_OBJ_VAL(weaved, 0) = olevel;
          obj_level_set(weaved, olevel);
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n",
              TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to "
              "create $p@W!@n",
              TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, "wristband", 0, 0);
          extract_obj(obj);
          WAIT_STATE(ch, PULSE_4SEC);
        }
      } else if (!strcasecmp(arg2, "belt")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up "
              "ruining the entire bundle instead.@n",
              TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but "
              "ends up ruining the entire bundle instead.@n",
              TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          WAIT_STATE(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16707, VIRTUAL);
          obj_to_room(weaved, char_room_get(ch));
          if (GET_OBJ_VNUM(obj) == 16708) {
            armor *= 20;
            str = 4;
            intel = 4;
            price = 4;
            olevel = 80;
          } else if (GET_OBJ_VNUM(obj) == 16700) {
            armor *= 15;
            str = 3;
            intel = 3;
            price = 4;
            olevel = 75;
          } else if (GET_OBJ_VNUM(obj) == 16701) {
            armor *= 10;
            str = 3;
            intel = 3;
            price = 3;
            olevel = 50;
          } else if (GET_OBJ_VNUM(obj) == 16702) {
            armor *= 6;
            str = 2;
            intel = 2;
            price = 2;
            olevel = 25;
          } else if (GET_OBJ_VNUM(obj) == 16703) {
            armor *= 4;
            str = 1;
            intel = 1;
            price = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          GET_OBJ_COST(weaved) *= price;
          GET_OBJ_VAL(weaved, 0) = olevel;
          obj_level_set(weaved, olevel);
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n",
              TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to "
              "create $p@W!@n",
              TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, "belt", 0, 0);
          extract_obj(obj);
          WAIT_STATE(ch, PULSE_4SEC);
        }
      } else {
        send_to_char(ch, "Syntax: silk weave (head | wrist | belt)");
        return;
      }
      return;
    } ////

  } else if (!strcasecmp(arg, "bundle")) {
    int64_t cost = ((GET_MAX_MANA(ch) * 0.01) * (prob * 0.20)) +
                   (GET_INT(ch) * GET_LEVEL(ch));

    if ((getCurKI(ch)) < cost) {
      send_to_char(
          ch, "You do not have enough ki to weave any bundles of silk.\r\n");
      return;
    } else {
      WAIT_STATE(ch, PULSE_3SEC);
      int super = FALSE, superoll = rand_number(1, 100);
      if (IS_KURZAK(ch)) {
        if (GET_SKILL(ch, SKILL_SILK) >= 100) {
          if (8 > superoll) {
            super = TRUE;
          }
        } else if (GET_SKILL(ch, SKILL_SILK) >= 60) {
          if (6 > superoll) {
            super = TRUE;
          }
        } else if (GET_SKILL(ch, SKILL_SILK) >= 40) {
          if (3 > superoll) {
            super = TRUE;
          }
        }
      }
      if (super == TRUE) {
        obj = read_object(16708, VIRTUAL);
        obj_to_room(obj, char_room_get(ch));
        act("@YYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You gently weave the silk and in no time "
            "at all you have a $p@Y piled at your feet!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        send_to_char(ch, "@YIt's SUPER grand!@n\r\n");
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "golden colored silk from $s mouth. Gently $e weaves the silk and "
            "in no time at all $e has a $p@W piled at $s feet!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
      } else if (prob > perc && prob >= 100) { /* Second Best Quality */
        obj = read_object(16700, VIRTUAL);
        obj_to_room(obj, char_room_get(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You gently weave the silk and in no time "
            "at all you have a $p@W piled at your feet!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "golden colored silk from $s mouth. Gently $e weaves the silk and "
            "in no time at all $e has a $p@W piled at $s feet!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
      } else if (prob > perc && prob >= 90) { /* Great Quality */
        obj = read_object(16701, VIRTUAL);
        obj_to_room(obj, char_room_get(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You gently weave the silk and in no time "
            "at all you have a $p@W piled at your feet!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "golden colored silk from $s mouth. Gently $e weaves the silk and "
            "in no time at all $e has a $p@W piled at $s feet!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
      } else if (prob > perc && prob >= 80) { /* Good Quality */
        obj = read_object(16702, VIRTUAL);
        obj_to_room(obj, char_room_get(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You gently weave the silk and in no time "
            "at all you have a $p@W piled at your feet!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "golden colored silk from $s mouth. Gently $e weaves the silk and "
            "in no time at all $e has a $p@W piled at $s feet!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
      } else if (prob > perc && prob >= 50) { /* Decent Quality */
        obj = read_object(16703, VIRTUAL);
        obj_to_room(obj, char_room_get(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You gently weave the silk and in no time "
            "at all you have a $p@W piled at your feet!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "golden colored silk from $s mouth. Gently $e weaves the silk and "
            "in no time at all $e has a $p@W piled at $s feet!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
      } else if (prob > perc) { /* Bad Quality */
        obj = read_object(16704, VIRTUAL);
        obj_to_room(obj, char_room_get(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You gently weave the silk and in no time "
            "at all you have a $p@W piled at your feet!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "golden colored silk from $s mouth. Gently $e weaves the silk and "
            "in no time at all $e has a $p@W piled at $s feet!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
      } else {
        act("@WYou concentrate your ki into your silk sacs and begin to spit "
            "silk out of your mouth. You end up making a poorly formed puddle "
            "of goo...@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a "
            "poorly formed puddle of goo...@n",
            TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost);
        improve_skill(ch, SKILL_SILK, 1);
      }
    }
  } else {
    send_to_char(ch, "Syntax: silk (weave | bundle)\r\n");
    return;
  }
}

/* do_adrenaline moved to lua/characters/commands/misc/adrenaline.lua */

/* This handles displaying the rpp item store to a player. */
void disp_rpp_store(struct char_data *ch) {

  send_to_char(ch, "@m                        RPP Item Store@n\n");
  send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                   "~~~~~~~~~~~~~@n\n");
  send_to_char(ch, "@GItem Name                      @gRPP Cost        "
                   "@cChoice Number   @yMin Lvl@n\n");
  send_to_char(ch, "@WStardust Equipment Set         @D[@Y20@D]            @D[ "
                   "@C1@D]            @w50@n\n");
  send_to_char(ch, "@WPlatinum Masamune (Sword Skill)@D[@Y 5@D]            @D[ "
                   "@C2@D]            @w40@n\n");
  send_to_char(ch, "@WObsidian Dirk (Dagger Skill)   @D[@Y 5@D]            @D[ "
                   "@C3@D]            @w40@n\n");
  send_to_char(ch, "@WEmerald Javelin (Spear Skill)  @D[@Y 5@D]            @D[ "
                   "@C4@D]            @w40@n\n");
  send_to_char(ch, "@WIvory Cane (Club Skill)        @D[@Y 5@D]            @D[ "
                   "@C5@D]            @w40@n\n");
  send_to_char(ch, "@WHyper X65 Cannon (Gun Skill)   @D[@Y 5@D]            @D[ "
                   "@C6@D]            @w40@n\n");
  send_to_char(ch, "@WJagged Rock (Brawl skill)      @D[@Y 5@D]            @D[ "
                   "@C7@D]            @w40@n\n");
  send_to_char(
      ch,
      "@WKachin Mountain                @D[@Y 8@D]            @D[ @C8@D]@n\n");
  send_to_char(
      ch,
      "@WSpar Booster                   @D[@Y15@D]            @D[ @C9@D]@n\n");
  send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                   "~~~~~~~~~~~~~@n\n");

  send_to_char(ch, "@wSyntax: rpp 12 (choice number)@n\r\n");
}

/* This handles buying an item from the rpp item store. */
void handle_rpp_store(struct char_data *ch, int choice) {
  struct obj_data *obj;
  int objnum = 0, cost = 0;

  switch (choice) { /* Find the cost of their selection. */
  case 1:
    cost = 20;
    break;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
    cost = 5;
    break;
  case 8:
    cost = 8;
    break;
  case 9:
    cost = 15;
    break;
  default:
    send_to_char(ch, "That is not a selection option!\r\n");
    return;
  }

  if (GET_RP(ch) < cost) { /* They can't afford it. */
    send_to_char(ch, "You do not have enough RPP to afford that option.\r\n");
    return;
  } else { /* They can afford it. */
    switch (choice) {
    case 1:
      if (IS_CARRYING_W(ch) + 26 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 13 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else if (GET_LEVEL(ch) < 50) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        for (objnum = 1110; objnum < 1120; objnum++) {
          if (objnum <= 1116) {
            obj = read_object(objnum, VIRTUAL);
            obj_to_char(obj, ch);
            GET_OBJ_SIZE(obj) = get_size(ch);
            obj = NULL;
          } else {
            obj = read_object(objnum, VIRTUAL);
            obj_to_char(obj, ch);
            GET_OBJ_SIZE(obj) = get_size(ch);
            obj = NULL;
            obj = read_object(objnum, VIRTUAL);
            obj_to_char(obj, ch);
            GET_OBJ_SIZE(obj) = get_size(ch);
          }
        }
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 2:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else if (GET_LEVEL(ch) < 40) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        obj = read_object(1120, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(
            ch,
            "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n",
            cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 3:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else if (GET_LEVEL(ch) < 40) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        obj = read_object(1121, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 4:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else if (GET_LEVEL(ch) < 40) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        obj = read_object(1122, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 5:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(
            ch,
            "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n",
            cost);
      } else if (GET_LEVEL(ch) < 40) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        obj = read_object(1123, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 6:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else if (GET_LEVEL(ch) < 40) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        obj = read_object(1124, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 7:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else if (GET_LEVEL(ch) < 40) {
        send_to_char(ch, "You are below the minimum level to equip it.\r\n");
      } else {
        obj = read_object(1125, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 8:
      if (IS_CARRYING_W(ch) + 10000000 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else {
        obj = read_object(1126, VIRTUAL);
        GET_OBJ_WEIGHT(obj) = 10000000;
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    case 9:
      if (IS_CARRYING_W(ch) + 2 > CAN_CARRY_W(ch)) {
        send_to_char(ch,
                     "You can not carry that much weight at this moment.\r\n");
      } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        send_to_char(ch, "You have too many items on you to carry anymore at "
                         "this moment.\r\n");
      } else {
        obj = read_object(1127, VIRTUAL);
        obj_to_char(obj, ch);
        GET_OBJ_SIZE(obj) = get_size(ch);
        GET_RP(ch) -= cost;
        ch->desc->rpp = GET_RP(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n",
                     cost);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), cost);
      }
      break;
    } /* End switch */
  }
}

static int valid_recipe(struct char_data *ch, int recipe, int type) {
  /* Plant Variables */
  int tomato = -1, cucumber = -1, onion = -1, greenbean = -1, garlic = -1,
      redpep = -1;
  int potato = -1, carrot = -1, brownmush = -1, lettuce = -1;
  /* Meat Variables  */
  int normmeat = -1, goodmeat = -1, normfish = -1, goodfish = -1,
      greatfish = -1, bestfish = -1;
  /* Store */
  int rice = -1, flour = -1, appleplum = -1, fberry = -1, carambola = -1;

  struct obj_data *obj2, *next_obj;
  int pass = FALSE;

  /* Determine ingredients needed for recipe */
  switch (recipe) {
  case RECIPE_TOMATO_SOUP:
    tomato = 2;
    break;
  case RECIPE_STEAK:
    normmeat = 1;
    break;
  case RECIPE_POTATO_SOUP:
    potato = 2;
    break;
  case RECIPE_VEGETABLE_SOUP:
    potato = 1;
    tomato = 1;
    carrot = 1;
    greenbean = 1;
    onion = 1;
    break;
  case RECIPE_MEAT_STEW:
    normmeat = 1;
    potato = 1;
    tomato = 1;
    garlic = 1;
    break;
  case RECIPE_ROAST:
    normmeat = 1;
    potato = 2;
    garlic = 1;
    onion = 1;
    greenbean = 3;
    break;
  case RECIPE_CHILI_SOUP:
    normmeat = 1;
    redpep = 4;
    tomato = 2;
    break;
  case RECIPE_GRILLED_NORMFISH:
    normfish = 1;
    break;
  case RECIPE_GRILLED_GOODFISH:
    goodfish = 1;
    break;
  case RECIPE_GRILLED_GREATFISH:
    greatfish = 1;
    break;
  case RECIPE_GRILLED_BESTFISH:
    bestfish = 1;
    break;
  case RECIPE_COOKED_RICE:
    rice = 1;
    break;
  case RECIPE_SUSHI:
    rice = 1;
    normfish = 1;
    break;
  case RECIPE_BREAD:
    flour = 1;
    break;
  case RECIPE_SALAD:
    tomato = 1;
    cucumber = 1;
    carrot = 1;
    lettuce = 1;
    break;
  case RECIPE_APPLEPLUM:
    flour = 1;
    appleplum = 1;
    break;
  case RECIPE_FBERRY_MUFFIN:
    flour = 1;
    fberry = 1;
    break;
  case RECIPE_CARAMBOLA_BREAD:
    flour = 1;
    carambola = 1;
    break;
  }

  if (type == 0) {
    /* Check for ingredients in inventory */
    char_inventory_iterate(ch, [&](auto obj2) {
      switch (GET_OBJ_VNUM(obj2)) {
      case RCP_TOMATO:
        if (tomato > 0) {
          tomato -= 1;
        }
        break;
      case RCP_NORMAL_MEAT:
        if (normmeat > 0) {
          normmeat -= 1;
        }
        break;
      case RCP_POTATO:
        if (potato > 0) {
          potato -= 1;
        }
        break;
      case RCP_ONION:
        if (onion > 0) {
          onion -= 1;
        }
        break;
      case RCP_CUCUMBER:
        if (cucumber > 0) {
          cucumber -= 1;
        }
        break;
      case RCP_CHILIPEPPER:
        if (redpep > 0) {
          redpep -= 1;
        }
        break;
      case RCP_CARROT:
        if (carrot > 0) {
          carrot -= 1;
        }
        break;
      case RCP_GREENBEAN:
        if (greenbean > 0) {
          greenbean -= 1;
        }
        break;
      case RCP_BLACKBASS:
      case RCP_FLOUNDER:
      case RCP_NARRI:
      case RCP_GRAVELREBOI:
        if (normfish > 0) {
          normfish -= 1;
        }
        break;
      case RCP_SILVERTROUT:
      case RCP_SILVEREEL:
      case RCP_VALBISH:
      case RCP_VOOSPIKE:
        if (goodfish > 0) {
          goodfish -= 1;
        }
        break;
      case RCP_STRIPEDBASS:
      case RCP_COBIA:
      case RCP_GUSBLAT:
      case RCP_SHADOWFISH:
        if (greatfish > 0) {
          greatfish -= 1;
        }
        break;
      case RCP_BLUECATFISH:
      case RCP_TAMBOR:
      case RCP_REPEEIL:
      case RCP_SHADEEEL:
        if (bestfish > 0) {
          bestfish -= 1;
        }
        break;
      case RCP_BROWNMUSH:
        if (brownmush > 0) {
          brownmush -= 1;
        }
        break;
      case RCP_GARLIC:
        if (garlic > 0) {
          garlic -= 1;
        }
        break;
      case RCP_RICE:
        if (rice > 0) {
          rice -= 1;
        }
        break;
      case RCP_FLOUR:
        if (flour > 0) {
          flour -= 1;
        }
        break;
      case RCP_LETTUCE:
        if (lettuce > 0) {
          lettuce -= 1;
        }
        break;
      case RCP_APPLEPLUM:
        if (appleplum > 0) {
          appleplum -= 1;
        }
        break;
      case RCP_FROZENBERRY:
        if (fberry > 0) {
          fberry -= 1;
        }
        break;
      case RCP_CARAMBOLA:
        if (carambola > 0) {
          carambola -= 1;
        }
        break;
      }
      return true;
    });
  } else { /* We know the ingredients are there, remove and exit. */
    char_inventory_iterate(ch, [&](auto obj2) {
      switch (GET_OBJ_VNUM(obj2)) {
      case RCP_TOMATO:
        if (tomato > 0) {
          tomato -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_NORMAL_MEAT:
        if (normmeat > 0) {
          normmeat -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_POTATO:
        if (potato > 0) {
          potato -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_ONION:
        if (onion > 0) {
          onion -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_CUCUMBER:
        if (cucumber > 0) {
          cucumber -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_CHILIPEPPER:
        if (redpep > 0) {
          redpep -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_CARROT:
        if (carrot > 0) {
          carrot -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_GREENBEAN:
        if (greenbean > 0) {
          greenbean -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_BLACKBASS:
      case RCP_FLOUNDER:
      case RCP_NARRI:
      case RCP_GRAVELREBOI:
        if (normfish > 0) {
          normfish -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_SILVERTROUT:
      case RCP_SILVEREEL:
      case RCP_VALBISH:
      case RCP_VOOSPIKE:
        if (goodfish > 0) {
          goodfish -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_STRIPEDBASS:
      case RCP_COBIA:
      case RCP_GUSBLAT:
      case RCP_SHADOWFISH:
        if (greatfish > 0) {
          greatfish -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_BLUECATFISH:
      case RCP_TAMBOR:
      case RCP_REPEEIL:
      case RCP_SHADEEEL:
        if (bestfish > 0) {
          bestfish -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_BROWNMUSH:
        if (brownmush > 0) {
          brownmush -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_GARLIC:
        if (garlic > 0) {
          garlic -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_RICE:
        if (rice > 0) {
          rice -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_FLOUR:
        if (flour > 0) {
          flour -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_LETTUCE:
        if (lettuce > 0) {
          lettuce -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_APPLEPLUM:
        if (appleplum > 0) {
          appleplum -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_FROZENBERRY:
        if (fberry > 0) {
          fberry -= 1;
          extract_obj(obj2);
        }
        break;
      case RCP_CARAMBOLA:
        if (carambola > 0) {
          carambola -= 1;
          extract_obj(obj2);
        }
        break;
      }
      return true;
    });
    return (TRUE);
    /* We'll exit here after removing the ingredients, for safety */
  }

  /* Make sure all ingredients were accounted for and pass if so */
  switch (recipe) {
  case RECIPE_TOMATO_SOUP:
    if (tomato == 0) {
      pass = TRUE;
    }
    break;
  case RECIPE_STEAK:
    if (normmeat == 0) {
      pass = TRUE;
    }
    break;
  case RECIPE_POTATO_SOUP:
    if (potato == 0) {
      pass = TRUE;
    }
    break;
  case RECIPE_VEGETABLE_SOUP:
    if (potato == 0 && tomato == 0 && carrot == 0 && greenbean == 0 &&
        onion == 0) {
      pass = TRUE;
    }
    break;
  case RECIPE_MEAT_STEW:
    if (normmeat == 0 && potato == 0 && tomato == 0 && garlic == 0)
      pass = TRUE;
    break;
  case RECIPE_ROAST:
    if (normmeat == 0 && potato == 0 && garlic == 0 && onion == 0 &&
        greenbean == 0)
      pass = TRUE;
    break;
  case RECIPE_CHILI_SOUP:
    if (normmeat == 0 && redpep == 0 && tomato == 0)
      pass = TRUE;
    break;
  case RECIPE_GRILLED_NORMFISH:
    if (normfish == 0)
      pass = TRUE;
    break;
  case RECIPE_GRILLED_GOODFISH:
    if (goodfish == 0)
      pass = TRUE;
    break;
  case RECIPE_GRILLED_GREATFISH:
    if (greatfish == 0)
      pass = TRUE;
    break;
  case RECIPE_GRILLED_BESTFISH:
    if (bestfish == 0)
      pass = TRUE;
    break;
  case RECIPE_COOKED_RICE:
    if (rice == 0)
      pass = TRUE;
    break;
  case RECIPE_SUSHI:
    if (rice == 0 && normfish == 0)
      pass = TRUE;
    break;
  case RECIPE_BREAD:
    if (flour == 0)
      pass = TRUE;
    break;
  case RECIPE_SALAD:
    if (tomato == 0 && cucumber == 0 && carrot == 0 && lettuce == 0)
      pass = TRUE;
    break;
  case RECIPE_APPLEPLUM:
    if (flour == 0 && appleplum == 0)
      pass = TRUE;
    break;
  case RECIPE_FBERRY_MUFFIN:
    if (flour == 0 && fberry == 0)
      pass = TRUE;
    break;
  case RECIPE_CARAMBOLA_BREAD:
    if (flour == 0 && carambola == 0)
      pass = TRUE;
    break;
  }

  if (pass == FALSE) {
    if (tomato > 0) {
      send_to_char(ch, "@WYou need @m%d@W tomato%s for this recipe.@n\r\n",
                   tomato, tomato > 1 ? "es" : "");
    }
    if (potato > 0) {
      send_to_char(ch, "@WYou need @m%d@W potato%s for this recipe.@n\r\n",
                   potato, potato > 1 ? "es" : "");
    }
    if (onion > 0) {
      send_to_char(ch, "@WYou need @m%d@W onion%s for this recipe.@n\r\n",
                   onion, onion > 1 ? "s" : "");
    }
    if (appleplum > 0) {
      send_to_char(ch, "@WYou need @m%d@W appleplum%s for this recipe.@n\r\n",
                   appleplum, appleplum > 1 ? "s" : "");
    }
    if (fberry > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W frozen berry%s for this recipe.@n\r\n",
                   fberry, fberry > 1 ? "s" : "");
    }
    if (carambola > 0) {
      send_to_char(ch, "@WYou need @m%d@W carambola%s for this recipe.@n\r\n",
                   carambola, carambola > 1 ? "s" : "");
    }
    if (lettuce > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W head%s of lettuce for this recipe.@n\r\n",
                   lettuce, lettuce > 1 ? "s" : "");
    }
    if (flour > 0) {
      send_to_char(
          ch, "@WYou need @m%d@W cup%s of white flour for this recipe.@n\r\n",
          flour, flour > 1 ? "s" : "");
    }
    if (rice > 0) {
      send_to_char(
          ch, "@WYou need @m%d@W cup%s of white rice for this recipe.@n\r\n",
          rice, rice > 1 ? "s" : "");
    }
    if (garlic > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W garlic clove%s for this recipe.@n\r\n",
                   garlic, garlic > 1 ? "s" : "");
    }
    if (carrot > 0) {
      send_to_char(ch, "@WYou need @m%d@W carrot%s for this recipe.@n\r\n",
                   carrot, carrot > 1 ? "s" : "");
    }
    if (cucumber > 0) {
      send_to_char(ch, "@WYou need @m%d@W cucumber%s for this recipe.@n\r\n",
                   cucumber, cucumber > 1 ? "s" : "");
    }
    if (greenbean > 0) {
      send_to_char(ch, "@WYou need @m%d@W green bean%s for this recipe.@n\r\n",
                   greenbean, greenbean > 1 ? "s" : "");
    }
    if (normmeat > 0) {
      send_to_char(
          ch, "@WYou need @m%d@W normal raw steak%s for this recipe.@n\r\n",
          normmeat, normmeat > 1 ? "s" : "");
    }
    if (goodmeat > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W good raw steak%s for this recipe.@n\r\n",
                   goodmeat, goodmeat > 1 ? "s" : "");
    }
    if (redpep > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W chili pepper%s for this recipe.@n\r\n",
                   redpep, redpep > 1 ? "s" : "");
    }
    if (normfish > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W black bass, flounder, narri, or gravel "
                   "reboi for this recipe.@n\r\n",
                   normfish);
    }
    if (goodfish > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W silver trout, silver eel, valbish, or "
                   "voos pike for this recipe.@n\r\n",
                   goodfish);
    }
    if (greatfish > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W striped bass, cobia, gusblat, or "
                   "shadowfish for this recipe.@n\r\n",
                   greatfish);
    }
    if (bestfish > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W blue catfish, tambor, repeeil, or "
                   "shadeeel for this recipe.@n\r\n",
                   bestfish);
    }
    if (brownmush > 0) {
      send_to_char(ch,
                   "@WYou need @m%d@W brown mushroom%s for this recipe.@n\r\n",
                   brownmush, brownmush > 1 ? "s" : "");
    }
    return (FALSE);
  } else {
    return (TRUE);
  }
}

static int campfire_cook(int recipe) {

  switch (recipe) {
  case RECIPE_STEAK:
  case RECIPE_GRILLED_NORMFISH:
  case RECIPE_GRILLED_GOODFISH:
  case RECIPE_GRILLED_GREATFISH:
  case RECIPE_GRILLED_BESTFISH:
  case RECIPE_ROAST:
    return (TRUE);
    break;
  }

  return (FALSE);
}

ACMD(do_cook) {
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (IS_NPC(ch))
    return;

  struct room_data *room = char_room_get(ch);

  int cook_elem = cook_element(room);

  if (!cook_elem) {
    send_to_char(ch,
                 "You need a campfire or Flambus Stove nearby to cook.\r\n");
    return;
  }

  if (!GET_SKILL(ch, SKILL_COOKING)) {
    send_to_char(ch, "You don't even know the basics!\r\n");
    return;
  }

  int skill = GET_SKILL(ch, SKILL_COOKING), prob = axion_dice(0);

  if (!*arg) {
    send_to_char(
        ch, "@D---------------------@RCooking@D---------------------@n\r\n");
    send_to_char(
        ch,
        "@Y 1@B) @CCooked Steak		@Y17@B) @CCarambola Bread@n\r\n");
    send_to_char(ch, "@Y 2@B) @CTomato Soup		@n\r\n");
    send_to_char(ch, "@Y 3@B) @CPotato Soup		@n\r\n");
    send_to_char(ch, "@Y 4@B) @CVegetable Soup		@n\r\n");
    send_to_char(ch, "@Y 5@B) @CMeat Stew			@n\r\n");
    send_to_char(ch, "@Y 6@B) @CChili Soup		@n\r\n");
    send_to_char(ch, "@Y 7@B) @CGrilled Fish		@n\r\n");
    send_to_char(ch, "@Y 8@B) @CGood Grilled Fish		@n\r\n");
    send_to_char(ch, "@Y 9@B) @CGreat Grilled Fish	@n\r\n");
    send_to_char(ch, "@Y10@B) @CMagnificent Grilled Fish	@n\r\n");
    send_to_char(ch, "@Y11@B) @CCooked White Rice		@n\r\n");
    send_to_char(ch, "@Y12@B) @CSushi			@n\r\n");
    send_to_char(ch, "@Y13@B) @CWhite Bread		@n\r\n");
    send_to_char(ch, "@Y14@B) @CBasic Salad		@n\r\n");
    send_to_char(ch, "@Y15@B) @CAppleplum Chasan		@n\r\n");
    send_to_char(ch, "@Y16@B) @CFrozen Berry Muffin	@n\r\n");
    send_to_char(ch, "@wSyntax: cook (recipe number)@n\r\n");
    return;
  } else {
    int num = atoi(arg), pass = FALSE;
    struct obj_data *meal = NULL;

    int recipe = -1;
    switch (num) {
    case 1:
      recipe = RECIPE_STEAK;
      prob += 8;
      break;
    case 2:
      recipe = RECIPE_TOMATO_SOUP;
      break;
    case 3:
      recipe = RECIPE_POTATO_SOUP;
      break;
    case 4:
      recipe = RECIPE_VEGETABLE_SOUP;
      break;
    case 5:
      recipe = RECIPE_MEAT_STEW;
      break;
    case 6:
      recipe = RECIPE_CHILI_SOUP;
      break;
    case 7:
      recipe = RECIPE_GRILLED_NORMFISH;
      prob += 6;
      break;
    case 8:
      recipe = RECIPE_GRILLED_GOODFISH;
      prob += 10;
      break;
    case 9:
      recipe = RECIPE_GRILLED_GREATFISH;
      prob += 12;
      break;
    case 10:
      recipe = RECIPE_GRILLED_BESTFISH;
      prob += 16;
      break;
    case 11:
      recipe = RECIPE_COOKED_RICE;
      break;
    case 12:
      recipe = RECIPE_SUSHI;
      break;
    case 13:
      recipe = RECIPE_BREAD;
      break;
    case 14:
      recipe = RECIPE_SALAD;
      break;
    case 15:
      recipe = RECIPE_APPLEPLUM;
      break;
    case 16:
      recipe = RECIPE_FBERRY_MUFFIN;
      break;
    case 17:
      recipe = RECIPE_CARAMBOLA_BREAD;
      break;
    }

    if (recipe == -1) {
      send_to_char(ch, "That is not a valid dish!\r\n");
      return;
    }

    if (!valid_recipe(ch, recipe, 0)) {
      return;
    } else if (cook_elem == 1 && !campfire_cook(recipe)) {
      send_to_char(ch, "You can not cook that dish over a campfire.\r\n");
      return;
    } else {
      valid_recipe(ch, recipe, 1);
      pass = TRUE;
    }
    if (pass == TRUE) {
      if (skill < prob) {
        act("@wYou screw up the preparation of the recipe and end up wasting "
            "the ingredients!@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w starts to prepare some food, but ends up ruining the "
            "ingredients instead!@n",
            TRUE, ch, 0, 0, TO_ROOM);
        improve_skill(ch, SKILL_COOKING, 0);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
      } /* End failed to cook it right */

      int psbonus = 0, expbonus = 0;

      switch (num) {
      case 1:
        meal = read_object(MEAL_STEAK, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 1;
        expbonus = 5;
        break;
      case 2:
        meal = read_object(MEAL_TOMATO_SOUP, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 2;
        expbonus = 15;
        break;
      case 3:
        meal = read_object(MEAL_POTATO_SOUP, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 1;
        expbonus = 20;
        break;
      case 4:
        meal = read_object(MEAL_VEGETABLE_SOUP, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 3;
        expbonus = 45;
        break;
      case 5:
        meal = read_object(MEAL_MEAT_STEW, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 2;
        expbonus = 50;
        break;
      case 6:
        meal = read_object(MEAL_CHILI_SOUP, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 0;
        expbonus = 100;
        break;
      case 7:
        meal = read_object(MEAL_NORM_FISH, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 2;
        expbonus = 12;
        break;
      case 8:
        meal = read_object(MEAL_GOOD_FISH, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 3;
        expbonus = 40;
        break;
      case 9:
        meal = read_object(MEAL_GREAT_FISH, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 5;
        expbonus = 80;
        break;
      case 10:
        meal = read_object(MEAL_BEST_FISH, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 7;
        expbonus = 125;
        break;
      case 11:
        meal = read_object(MEAL_COOKED_RICE, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 1;
        expbonus = 8;
        break;
      case 12:
        meal = read_object(MEAL_SUSHI, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 2;
        expbonus = 20;
        break;
      case 13:
        meal = read_object(MEAL_BREAD, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 1;
        expbonus = 8;
        break;
      case 14:
        meal = read_object(MEAL_SALAD, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 5;
        expbonus = 8;
        break;
      case 15:
        meal = read_object(MEAL_APPLEPLUM, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 1;
        expbonus = 9;
        break;
      case 16:
        meal = read_object(MEAL_FBERRY_MUFFIN, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 3;
        expbonus = 12;
        break;
      case 17:
        meal = read_object(MEAL_CARAMBOLA_BREAD, VIRTUAL);
        obj_to_char(meal, ch);
        psbonus = 1;
        expbonus = 9;
        break;
      default:
        send_to_char(ch, "That is not a valid dish!\r\n");
        return;
      }
      if (GET_BONUS(ch, BONUS_RECIPE)) {
        psbonus += 1;
        expbonus += 3;
      }
      act("@wYou carefully prepare the ingredients and then start cooking "
          "them. After a while of patience  and skillful care you successfully "
          "make @D'@C$p@D'@w!@n",
          TRUE, ch, meal, 0, TO_CHAR);
      act("@C$n@w carefully prepares some ingredients and starts cooking them. "
          "After a while of patience and skillful care $e succeeds in making "
          "@D'@C$p@D'@w!@n",
          TRUE, ch, meal, 0, TO_ROOM);
      improve_skill(ch, SKILL_COOKING, 0);

      if (psbonus > 0) {
        if (skill * 0.10 > 0)
          psbonus = (skill * 0.10) * psbonus;
      }
      if (expbonus > 0) {
        expbonus = skill * expbonus;
      }

      GET_OBJ_VAL(meal, VAL_FOOD_PSBONUS) = psbonus;
      GET_OBJ_VAL(meal, VAL_FOOD_EXPBONUS) = expbonus;

      WAIT_STATE(ch, PULSE_2SEC);
    } /* End has ingredients */
  }
}

/* This allows a player to cover their body in a shield of fire. */
ACMD(do_fireshield) {

  if (!know_skill(ch, SKILL_FIRESHIELD)) {
    return;
  }

  if (AFF_FLAGGED(ch, AFF_FIRESHIELD)) {
    send_to_char(ch, "You are already covered in a fireshield!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
    send_to_char(ch, "You are covered in a barrier!\r\n");
    return;
  }

  if (room_is_sunken(char_room_get(ch))) {
    send_to_char(ch, "There is way too much water here!\r\n");
    return;
  }

  int64_t cost = GET_MAX_MANA(ch) * 0.03;

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki!\r\n");
    return;
  }

  int skill = init_skill(ch, SKILL_FIRESHIELD), prob = axion_dice(0);

  if (skill <= prob) {
    act("@WYou hold your hands up in front of you on either side and try to "
        "summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover your body. Yet you "
        "screw up and the technique fails!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@W holds $s hands up in front of $m on either side and tries to "
        "summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover $s body. Yet $e seems "
        "to screw up and the technique fails!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_FIRESHIELD, 0);
    decCurKI(ch, cost);
    return;
  } else {
    act("@WYou hold your hands up in front of you on either side and try to "
        "summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover your body. The ki you "
        "have gathered pours out of your body and creates intense black "
        "@rf@Rl@Ya@rm@Re@Ys@W that cover your entire body in a protective "
        "layer!",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@W holds $s hands up in front of $m on either side and tries to "
        "summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover $s body. The ki $e has "
        "gathered pours out of $s body and creates intense black "
        "@rf@Rl@Ya@rm@Re@Ys@W that cover $s entire body in a protective layer!",
        TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_FIRESHIELD, 0);
    decCurKI(ch, cost);
    char_condition_apply(ch, "fireshield", "skill", "fireshield");
    return;
  }
}

/* This allows a player to warp from one ocean/sea to another. */
ACMD(do_warppool) {

  if (IS_NPC(ch))
    return;

  if (!know_skill(ch, SKILL_WARP)) {
    return;
  }

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return;
  }

  if (ABSORBING(ch) || ABSORBBY(ch)) {
    send_to_char(ch, "You are struggling with someone!\r\n");
    return;
  }

  if (SITS(ch)) {
    send_to_char(ch, "You should get up first.\r\n");
    return;
  }

  int perc = GET_SKILL(ch, SKILL_WARP);
  int prob = axion_dice(0), cost = GET_MAX_MANA(ch) / 20, pass = FALSE;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What planet are you wanting to warp to?\n[ earth | "
                     "frigid | kanassa | namek | aether ]\r\n");
    return;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }

  struct room_data *room = char_room_get(ch);

  if (char_room_vnum_get(ch) >= 4600 && char_room_vnum_get(ch) < 4700) {
    pass = TRUE;
  } else if (char_room_vnum_get(ch) >= 795 && char_room_vnum_get(ch) < 1099) {
    pass = TRUE;
  } else if (char_room_vnum_get(ch) >= 15100 &&
             char_room_vnum_get(ch) < 15299) {
    pass = TRUE;
  } else if (char_room_vnum_get(ch) >= 13155 &&
             char_room_vnum_get(ch) < 13199) {
    pass = TRUE;
  } else if (room_flagged(room, ROOM_NAMEK) &&
             room_sector_type_get(room) == SECT_WATER_NOSWIM) {
    pass = TRUE;
  } else if (char_room_vnum_get(ch) >= 12103 &&
             char_room_vnum_get(ch) < 12289) {
    pass = TRUE;
  }

  if (pass == FALSE) {
    send_to_char(
        ch, "You must be on or in a sea or ocean for warp pool to work.\r\n");
    return;
  }

  if (!strcasecmp("earth", arg) && room_flagged(room, ROOM_EARTH)) {
    send_to_char(ch, "You are already on Earth!\r\n");
    return;
  } else if (!strcasecmp("frigid", arg) && room_flagged(room, ROOM_FRIGID)) {
    send_to_char(ch, "You are already on Frigid!\r\n");
    return;
  } else if (!strcasecmp("kanassa", arg) && room_flagged(room, ROOM_KANASSA)) {
    send_to_char(ch, "You are already on Kanasssa!\r\n");
    return;
  } else if (!strcasecmp("namek", arg) && room_flagged(room, ROOM_NAMEK)) {
    send_to_char(ch, "You are already on Namek!\r\n");
    return;
  } else if (!strcasecmp("aether", arg) && room_flagged(room, ROOM_AETHER)) {
    send_to_char(ch, "You are already on Aether!\r\n");
    return;
  } else if (!strcasecmp("earth", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. You "
          "lose your concentration and the ritual fails!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly a puzzled look comes across @c$n's @Cface and the water "
          "returns to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. As "
          "you complete the ritual you connect the water you disturbed with "
          "the water you envisioned and warp between the two points!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly @c$n@C vanishes into this water! A moment later the waters "
          "return to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, room_by_id(850));
      act("@CSuddenly a large whirlpool of flashing water begins to form "
          "nearby. After a few seconds @c$n@C pops out of the center of the "
          "pool! The water then return to normal a moment laterr...@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
    }
  } else if (!strcasecmp("frigid", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. You "
          "lose your concentration and the ritual fails!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly a puzzled look comes across @c$n's @Cface and the water "
          "returns to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. As "
          "you complete the ritual you connect the water you disturbed with "
          "the water you envisioned and warp between the two points!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly @c$n@C vanishes into this water! A moment later the waters "
          "return to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, room_by_id(4609));
      act("@CSuddenly a large whirlpool of flashing water begins to form "
          "nearby. After a few seconds @c$n@C pops out of the center of the "
          "pool! The water then return to normal a moment laterr...@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
    }
  } else if (!strcasecmp("namek", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. You "
          "lose your concentration and the ritual fails!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly a puzzled look comes across @c$n's @Cface and the water "
          "returns to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. As "
          "you complete the ritual you connect the water you disturbed with "
          "the water you envisioned and warp between the two points!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly @c$n@C vanishes into this water! A moment later the waters "
          "return to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, room_by_id(10904));
      act("@CSuddenly a large whirlpool of flashing water begins to form "
          "nearby. After a few seconds @c$n@C pops out of the center of the "
          "pool! The water then return to normal a moment laterr...@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
    }
  } else if (!strcasecmp("kanassa", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. You "
          "lose your concentration and the ritual fails!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly a puzzled look comes across @c$n's @Cface and the water "
          "returns to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. As "
          "you complete the ritual you connect the water you disturbed with "
          "the water you envisioned and warp between the two points!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly @c$n@C vanishes into this water! A moment later the waters "
          "return to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, room_by_id(15100));
      act("@CSuddenly a large whirlpool of flashing water begins to form "
          "nearby. After a few seconds @c$n@C pops out of the center of the "
          "pool! The water then return to normal a moment laterr...@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
    }
  } else if (!strcasecmp("aether", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. You "
          "lose your concentration and the ritual fails!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly a puzzled look comes across @c$n's @Cface and the water "
          "returns to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. "
          "At the same time you release ki into the water and focus your mind "
          "on sensing out the distant body of water you wish to travel to. As "
          "you complete the ritual you connect the water you disturbed with "
          "the water you envisioned and warp between the two points!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with "
          "it. The water that is being swirled begins to glow @wbright@B "
          "blue@C and has a distinct separation from the rest of the waters. "
          "Suddenly @c$n@C vanishes into this water! A moment later the waters "
          "return to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, room_by_id(12252));
      act("@CSuddenly a large whirlpool of flashing water begins to form "
          "nearby. After a few seconds @c$n@C pops out of the center of the "
          "pool! The water then return to normal a moment laterr...@n",
          TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost);
    }
  } else {
    send_to_char(
        ch,
        "That is not an acceptable choice. It must be a planet with a large "
        "body of water.\n[ earth | frigid | kanassa | namek | aether ]\r\n");
    return;
  }
}

/* This allows a player to block off the exit of a room */
ACMD(do_obstruct) {

  if (IS_NPC(ch))
    return;

  if (!know_skill(ch, SKILL_HYOGA_KABE)) {
    return;
  }

  struct room_data *room = char_room_get(ch);
  int sect = room_sector_type_get(room);

  if (room_flagged(room, ROOM_PEACEFUL)) {
    send_to_char(ch, "You can not use this in such a peaceful area.\r\n");
    return;
  }

  if (sect == SECT_SPACE || room_flagged(room, ROOM_SPACE)) {
    send_to_char(ch, "You can not wall off the vastness of space.\r\n");
    return;
  }

  if (sect == SECT_FLYING) {
    send_to_char(ch, "You can not create gravity defying glacial walls.\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH];
  int skill = GET_SKILL(ch, SKILL_HYOGA_KABE);
  int prob = axion_dice(0), cost = ((GET_MAX_MANA(ch) / skill) * 2.5);

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What direction are you wanting to block off?\n[ N | E | "
                     "S | W | NE | NW | SE | SW | U | D | I | O ]\r\n");
    return;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }

  int dir = -1, dir2 = -1;

  if (!strcasecmp("n", arg) || !strcasecmp("N", arg)) {
    dir = 0;
    dir2 = 2;
  } else if (!strcasecmp("e", arg) || !strcasecmp("E", arg)) {
    dir = 1;
    dir2 = 3;
  } else if (!strcasecmp("s", arg) || !strcasecmp("S", arg)) {
    dir = 2;
    dir2 = 0;
  } else if (!strcasecmp("w", arg) || !strcasecmp("W", arg)) {
    dir = 3;
    dir2 = 1;
  } else if (!strcasecmp("u", arg) || !strcasecmp("U", arg)) {
    dir = 4;
    dir2 = 5;
  } else if (!strcasecmp("d", arg) || !strcasecmp("D", arg)) {
    dir = 5;
    dir2 = 4;
  } else if (!strcasecmp("i", arg) || !strcasecmp("I", arg)) {
    dir = 10;
    dir2 = 11;
  } else if (!strcasecmp("o", arg) || !strcasecmp("O", arg)) {
    dir = 11;
    dir2 = 10;
  } else if (!strcasecmp("nw", arg) || !strcasecmp("NW", arg)) {
    dir = 6;
    dir2 = 8;
  } else if (!strcasecmp("ne", arg) || !strcasecmp("NE", arg)) {
    dir = 7;
    dir2 = 9;
  } else if (!strcasecmp("se", arg) || !strcasecmp("SE", arg)) {
    dir = 8;
    dir2 = 6;
  } else if (!strcasecmp("sw", arg) || !strcasecmp("SW", arg)) {
    dir = 9;
    dir2 = 7;
  } else {
    send_to_char(ch, "That is not an acceptable direction.\n[ N | E | S | W | "
                     "NE | NW | SE | SW | U | D | I | O ]\r\n");
    return;
  }

  if (!EXIT(ch, dir)) {
    send_to_char(ch, "That direction does not exist here.\r\n");
    return;
  } else if (skill < prob) {
    act("@CYou channel your ki and start to create a wall of water, but lose "
        "your concentration and the water promptly disappears.@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C channels $s ki and starts to create a wall of water, but loses "
        "$s concentration and the water promptly disappears.@n",
        TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, cost);
    improve_skill(ch, SKILL_HYOGA_KABE, 0);
    return;
  } else {
    auto ex = EXIT(ch, dir);
    int newroom = exit_to_room_vnum_get(ex);
    struct room_data *nrm = exit_dest_get(ex);

    if (room_flagged(nrm, ROOM_PEACEFUL)) {
      send_to_char(ch, "You can not block off a peaceful area.\r\n");
      return;
    }

    bool found_wall = false;
    room_contents_iterate(nrm, [&](auto obj) {
      if (GET_OBJ_VNUM(obj) == 79) {
        if (GET_OBJ_COST(obj) == dir2) {
          if (skill < prob) {
            act("@CYou place your hands on the glacial wall and concentrate. "
                "You fail to undo the composition of the wall!@n",
                TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C places $s hands on the glacial wall and concentrates. "
                "Nothing happens...@n",
                TRUE, ch, 0, 0, TO_ROOM);
            decCurKI(ch, cost / 2);
          } else {
            act("@CYou place your hands on the glacial wall and concentrate. "
                "You unfreeze the wall and evaporate the water effortlessly.@n",
                TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C places $s hands on the glacial wall and concentrates. "
                "Suddenly the wall melts and then evaporates!@n",
                TRUE, ch, 0, 0, TO_ROOM);
            decCurKI(ch, cost / 2);
            extract_obj(obj);
          }
          found_wall = true;
          return false;
        }
      }
      return true;
    });
    if (found_wall) return;

    struct obj_data *obj2, *obj3;

    obj2 = read_object(79, VIRTUAL);
    obj_to_room(obj2, nrm);
    obj3 = read_object(79, VIRTUAL);
    obj_to_room(obj3, char_room_get(ch));

    int64_t strength = (((GET_INT(ch) * skill) * GET_WIS(ch)) * 20) +
                       (GET_MAX_MANA(ch) * 0.001);

    if (strength > GET_MAX_HIT(ch) * 20) {
      strength = GET_MAX_HIT(ch) + (strength / 20);
    } else if (strength > GET_MAX_HIT(ch) * 15) {
      strength = GET_MAX_HIT(ch) + (strength / 15);
    } else if (strength > GET_MAX_HIT(ch) * 10) {
      strength = GET_MAX_HIT(ch) + (strength / 10);
    } else if (strength > GET_MAX_HIT(ch) * 5) {
      strength = GET_MAX_HIT(ch) + (strength / 5);
    } else if (strength > GET_MAX_HIT(ch) * 2) {
      strength = GET_MAX_HIT(ch) + (strength / 2);
    }

    GET_OBJ_COST(obj2) = dir2;
    GET_OBJ_WEIGHT(obj2) = strength;
    GET_OBJ_COST(obj3) = dir;
    GET_OBJ_WEIGHT(obj3) = strength;
    GET_FELLOW_WALL(obj2) = obj3;
    GET_FELLOW_WALL(obj3) = obj2;
    act("@CYou concentrate and channel your ki. A wall of water starts to form "
        "in such a way to block off the direction of your choice. As the wall "
        "becomes complete it freezes solid by your will!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C concentrates and channels $s ki. A wall of water starts to "
        "form in such a way to block off one of the directions of this area. "
        "As the wall becomes complete it freezes solid by @c$n's@C will!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    send_to_room(nrm,
                 "@cA wall of water forms slowly upward blocking off the %s "
                 "direction. This wall of water then freezes instantly once it "
                 "stops growing.@n\r\n",
                 dirs[dir2]);
    improve_skill(ch, SKILL_HYOGA_KABE, 0);
    decCurKI(ch, cost);
    return;
  }
}

/* do_dimizu moved to lua/characters/commands/misc/dimizu.lua */

/* Allows a player to place a "beacon" on a room they want to return to if
 * they revive from death. */
ACMD(do_beacon) {

  if (IS_NPC(ch))
    return;

  if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
    send_to_char(ch, "You are dead. You can not stake out a room to return to "
                     "upon revival.\r\n");
    return;
  } else if (char_room_vnum_get(ch) >= 0 && char_room_vnum_get(ch) <= 14) {
    send_to_char(
        ch, "You can not stake out an immortal room to be revived in.\r\n");
    return;
  } else {
    send_to_char(ch, "You stake out the room you are in and will return to it "
                     "if you die and are revived.\r\n");
    GET_DROOM(ch) = char_room_vnum_get(ch);
    return;
  }
}

/* Feed senzu to someone you are grouped with. Why? TEAMWORK! */
ACMD(do_feed) {

  if (IS_NPC(ch))
    return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *obj;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Feed a senzu to whom?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That target isn't here.\r\n");
    return;
  }

  if (IS_ANDROID(vict)) {
    send_to_char(ch, "They are unaffected by senzu beans.\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, inv_for_char(ch)))) {
    send_to_char(ch, "You need to give them a senzu.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_POTION) {
    send_to_char(ch, "You can only feed senzu beans.\r\n");
    return;
  }

  if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
    send_to_char(ch, "They can't swallow that, it is fake!\r\n");
    return;
  }

  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, "They can't swallow that, it is broken!\r\n");
    return;
  }

  if (FIGHTING(vict)) {
    send_to_char(ch, "They are a bit busy at the moment!\r\n");
    return;
  }

  if (MASTER(vict) != ch && MASTER(ch) != vict && MASTER(ch) != MASTER(vict)) {
    send_to_char(ch, "You need to be grouped with them first.\r\n");
    return;
  }

  if (!char_condition_has(vict, "group") || !char_condition_has(ch, "group")) {
    send_to_char(ch, "You need to be grouped with them first.\r\n");
    return;
  }

  act("@WYou take $p@W and pop it into @C$N@W's mouth!@n", TRUE, ch, obj, vict,
      TO_CHAR);
  act("@C$n@W takes $p@W and pops it into YOUR mouth!@n", TRUE, ch, obj, vict,
      TO_VICT);
  act("@C$n@W takes $p@W and pops it into @c$N@W's mouth!@n", TRUE, ch, obj,
      vict, TO_NOTVICT);
  mag_objectmagic(vict, obj, "");
}

/* This allows players to decapitate a corpse for a sick trophy. */
ACMD(do_spoil) {

  if (IS_NPC(ch))
    return;

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int type = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What corpse do you want to decapitate?\r\n");
    return;
  }

  if (!(obj =
             get_obj_in_list_vis(ch, arg, NULL, inv_for_room(char_room_get(ch))))) {
    send_to_char(ch, "No corpse around here by that name.\r\n");
    return;
  }

  if (GET_OBJ_VAL(obj, VAL_CORPSE_HEAD) == 0) {
    send_to_char(ch, "That corpse is already missing its head.\r\n");
    return;
  }

  if (GET_EQ(ch, WEAR_WIELD1)) {
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
        TYPE_SLASH - TYPE_HIT) {
      type = 1;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
               TYPE_PIERCE - TYPE_HIT) {
      type = 1;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
               TYPE_STAB - TYPE_HIT) {
      type = 1;
    }
  } else if (GET_EQ(ch, WEAR_WIELD2)) {
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
        TYPE_SLASH - TYPE_HIT) {
      type = 2;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_PIERCE - TYPE_HIT) {
      type = 2;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_STAB - TYPE_HIT) {
      type = 2;
    }
  }

  if (type == 0) {
    act("@C$n@W reaches down and @rtears@W the head off of @R$p@W!@n", TRUE, ch,
        obj, 0, TO_ROOM);
    act("@WYou reach down and @rtear@W the head off of @R$p@W!@n", TRUE, ch,
        obj, 0, TO_CHAR);
  } else if (type == 1) {
    act("@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", TRUE, ch,
        obj, 0, TO_ROOM);
    act("@WYou reach down and @rcut@W the head off of @R$p@W!@n", TRUE, ch, obj,
        0, TO_CHAR);
  } else if (type == 2) {
    act("@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", TRUE, ch,
        obj, 0, TO_ROOM);
    act("@WYou reach down and @rcut@W the head off of @R$p@W!@n", TRUE, ch, obj,
        0, TO_CHAR);
  }

  GET_OBJ_VAL(obj, VAL_CORPSE_HEAD) = 0;

  struct obj_data *body_part;
  char part[1000];
  char buf[1000];
  char buf2[1000];
  char buf3[1000];

  *part = '\0';
  *buf = '\0';
  *buf2 = '\0';
  *buf3 = '\0';

  body_part = create_obj();
  body_part->proto_id = NOTHING;
  IN_ROOM(body_part) = NOWHERE;
  snprintf(part, sizeof(part), "%s", obj->name);
  search_replace(part, "headless", "");
  search_replace(part, "corpse", "");
  search_replace(part, "half", "");
  search_replace(part, "burnt", "");
  search_replace(part, "chunks", "");
  search_replace(part, "beaten", "");
  search_replace(part, "bloody", "");
  trim(part);
  snprintf(buf, sizeof(buf), "bloody head %s", part);
  snprintf(buf2, sizeof(buf2), "@wThe bloody head of %s@w is lying here@n",
           part);
  snprintf(buf3, sizeof(buf3), "@wThe bloody head of %s@w@n", part);

  body_part->name = strdup(buf);
  body_part->description = strdup(buf2);
  body_part->short_description = strdup(buf3);

  GET_OBJ_TYPE(body_part) = ITEM_OTHER;
  SET_BIT_AR(GET_OBJ_WEAR(body_part), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(body_part), ITEM_UNIQUE_SAVE);
  GET_OBJ_VAL(body_part, 4) = 1;
  GET_OBJ_VAL(body_part, 5) = 1;
  GET_OBJ_WEIGHT(body_part) = rand_number(4, 10);
  obj_to_room(body_part, char_room_get(ch));
  obj_from_room(body_part);
  obj_to_char(body_part, ch);
}
