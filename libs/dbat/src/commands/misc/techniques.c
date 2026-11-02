/* ************************************************************************
*  File: misc/techniques.c                                    Part of DBAT       *
*  Usage: Race and class technique commands                               *
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
#include "dbat/act.misc.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/fish.h"
#include "dbat/consts/recipes.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"

#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/descriptor_utils.h"

#include "dbat/random.h"
#include "dbat/affect.h"
#include "dbat/search.h"
#include "dbat/stringutils.h"
#include "dbat/extract.h"
#include "dbat/relocate.h"
#include "dbat/interpreter.h"

#include "dbat/db.h"

#include "dbat/broadcast.h"
#include "dbat/act.wizard.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"

#include "dbat/spells.h"
#include "dbat/comm.h"

#include "dbat/handler.h"
#include "dbat/combat.h"
#include "dbat/obj_edit.h"
#include "dbat/fight.h"
#include "dbat/class.h"
#include "dbat/act.item.h"
#include "misc.h"

/* local functions */
static void generate_multiform(struct char_data *ch, int count);

ACMD(do_spiritcontrol)
{

  if (!char_get_skill(ch, SKILL_SPIRITCONTROL)) {
    send_to_char(ch, "You do not know how to perform that technique.\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_SPIRITCONTROL)) {
    send_to_char(ch, "You have already concentrated and have full control of your spirit.\r\n");
    return;
  }
  int64_t cost = getMaxKI(ch) * 0.2;
  if ((getCurST(ch)) < cost) {
    send_to_char(ch, "You need at least 20%s of your max ki in stamina to prepare this skill.\r\n", "%");
    return;
  }
  decCurST(ch, cost, 0);
  act("@YYou concentrate and quantify every last bit of your spiritual and mental energies. You have full control of them and can bring them forth in an instant.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@y$n@Y seems to concentrate hard for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);
  int duration = rand_number(2, 4);
  assign_affect(ch, AFF_SPIRITCONTROL, SKILL_SPIRITCONTROL, duration, 0, 0, 0, 0, 0, 0);
}

ACMD(do_tailhide)
{

  if (char_is_npc(ch))
    return;

  if (!(char_is_saiyan(ch)) && !(char_is_halfbreed(ch))) {
    send_to_char(ch, "You have no need to hide your tail!\r\n");
    return;
  }

  if (!(char_plr_flagged(ch, PLR_TAILHIDE))) {
    char_plr_set(ch, PLR_TAILHIDE, true);
    act("You tuck your tail away, hiding it from view.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tucks $s tail away, hiding it from view.", FALSE, ch, 0, 0, TO_ROOM);
  } else {
    char_plr_set(ch, PLR_TAILHIDE, false);
    act("You have decided to display your tail for all to see!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has decided to display $s tail for all to see!", FALSE, ch, 0, 0, TO_ROOM);
  }
}

ACMD(do_nogrow)
{

  if (char_is_npc(ch))
    return;

  if (!(char_is_saiyan(ch)) && !(char_is_halfbreed(ch))) {
    send_to_char(ch, "What do you mean?\r\n");
  }
  if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && !(char_plr_flagged(ch, PLR_NOGROW))) {
    char_plr_set(ch, PLR_NOGROW, true);
    send_to_char(ch, "You have decided to halt your tail growth!\r\n");
  } else if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && char_plr_flagged(ch, PLR_NOGROW)) {
    char_plr_set(ch, PLR_NOGROW, false);
    send_to_char(ch, "You have decided to regrow your tail!\r\n");
  }
}

ACMD(do_multiform)
{

  if (!char_is_npc(ch) && !char_get_skill(ch, SKILL_MULTIFORM)) {
    send_to_char(ch, "You do not know how to perform that technique.\r\n");
    return;
  }

  struct char_data *tch = NULL, *next_v = NULL;
  int count = 0;

  for (tch = world[char_get_in_room(ch)].people; tch; tch = next_v) {
    next_v = tch->next_in_room;
    if (tch == ch || !char_is_npc(tch)) {
      continue;
    }
    if (char_get_mob_vnum(tch) == 25 && char_get_original(tch) == ch) {
      count++;
    }
  }

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!strcasecmp(arg, "merge")) {
    if (count == 0) {
      send_to_char(ch, "You have no multiforms present to merge with!\r\n");
      return;
    }
    for (tch = world[char_get_in_room(ch)].people; tch; tch = next_v) {
      next_v = tch->next_in_room;
      if (tch == ch || !char_is_npc(tch)) {
        continue;
      }
      if (char_get_mob_vnum(tch) == 25 && char_get_original(tch) == ch) {
        extract_char(tch);
      }
    }
    return;
  }

  if (!strcasecmp(arg, "split")) {
    int64_t cost = (getMaxKI(ch) * 0.005) + (getMaxST(ch) * 0.005) + 2;
    int penalty  = 0;

    if (char_get_fighting(ch)) {
      penalty = rand_number(8, 15);
    }

    int roll = axion_dice(penalty);

    cost *= (char_get_skill(ch, SKILL_MULTIFORM) * 0.2);

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to split!\r\n");
      return;
    }
    if ((getCurST(ch)) < cost) {
      send_to_char(ch, "You do not have enough stamina to split!\r\n");
      return;
    }
    improve_skill(ch, SKILL_MULTIFORM, 1);

    if (char_get_skill(ch, SKILL_MULTIFORM) < roll) {
      act("@YYou focus your ki into your body while concentrating on the image of your body splitting into two. @yYou lose your concentration and fail to split though...@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@y$n@Y seems to concentrate really hard for a moment, before relaxing.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurST(ch, cost, 0);
      decCurKI(ch, cost, 0);
      return;
    }
    act("@YYou focus your ki into your body while concentrating on the image of your body splitting into two. Another you splits out of your body!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@YSuddenly @y$n@Y seems to concentrates really and after a brief moment splits into two copies of $mself!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    generate_multiform(ch, 1);
    return;
  }
  send_to_char(ch, "Huh? Try help multiform\r\n");
  return;
}

static void generate_multiform(struct char_data *ch, int count)
{
  char blamo[MAX_INPUT_LENGTH];
  snprintf(blamo, sizeof(blamo), "p.%s", char_get_name(ch));

  mob_rnum r_num;
  if ((r_num = real_mobile(25)) == NOBODY) {
    send_to_imm("Multiform Clone doesn't exist!");
    return;
  }

  char clone_name[MAX_INPUT_LENGTH];
  snprintf(clone_name, sizeof(clone_name), "%s's Clone", ch->name);

  char clone_sdesc[MAX_INPUT_LENGTH];
  snprintf(clone_sdesc, sizeof(clone_sdesc), "%s's @CClone@n", ch->name);
  char clone_ldesc[MAX_INPUT_LENGTH];
  snprintf(clone_ldesc, sizeof(clone_ldesc), "%s's @CClone@w is standing here.@n\n", ch->name);

  for (int i = 0; i < count; i++) {
    char_data *clone = nullptr;
    clone            = read_mobile(r_num, REAL);

    clone->name        = strdup(clone_name);
    clone->short_descr = strdup(clone_sdesc);
    clone->long_descr  = strdup(clone_ldesc);
    if (ch->description)
      clone->description = strdup(ch->description);
    clone->race    = ch->race;
    clone->chclass = ch->chclass;

    // Not sure if these are actually used...
    clone->alignment = ch->alignment;

    // Make the physical appearance match!
    clone->sex     = ch->sex;
    clone->hairl   = ch->hairl;
    clone->hairs   = ch->hairs;
    clone->hairc   = ch->hairc;
    clone->skin    = ch->skin;
    clone->eye     = ch->eye;
    clone->distfea = ch->distfea;
    clone->aura    = ch->aura;

    clone->weight = ch->weight;
    clone->height = ch->height;
    clone->size   = ch->size;
    clone->level  = ch->level;
    clone->time   = ch->time;

    clone->tail_growth = ch->tail_growth;
    ch->transclass     = ch->transclass;

    // Copying these values, but it shouldn't matter because clones no longer work this way.
    clone->basepl = ch->basepl;
    clone->baseki = ch->baseki;
    clone->basest = ch->basest;

    // Bioandroid Genome copy...
    clone->genome[0] = ch->genome[0];
    clone->genome[1] = ch->genome[1];

    // Limb copy...
    for (int l = 0; l < 3; l++) {
      clone->limbs[l]          = ch->limbs[l];
      clone->limb_condition[l] = ch->limb_condition[l];
    }

    char_set_class_level(clone, char_get_class_level(ch));
    char_set_clones(ch, char_get_clones(ch) + (1));

    char_set_original(clone, ch);
    char_to_room(clone, char_get_in_room(ch));
    add_follower(clone, ch);
  }
}

void handle_multi_merge(struct char_data *form)
{
  struct char_data *ch = char_get_original(form);

  if (ch == NULL)
    return;

  send_to_char(ch, "@YYou merge with one of your forms!@n\r\n");
  act("@y$n@Y merges with one of his multiforms!@n\r\n", TRUE, ch, 0, 0, TO_ROOM);

  ch->clones = MAX(0, ch->clones - 1);
  extract_char(form);
}

ACMD(do_moondust)
{
  int64_t cost = getMaxST(ch) * 0.02, heal = 0;

  /* Can they do the technique? */

  if (!char_is_arlian(ch) || char_get_sex(ch) != SEX_FEMALE) {
    send_to_char(ch, "You are not an arlian female.\r\n");
    return;
  }

  if (!char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(ch, "You need to be in a group to use this skill!\r\n");
    return;
  }

  cost += (getMaxLF(ch)) * 0.02;
  heal = cost * 3;

  if (getCurPL(ch) >= (getEffMaxPL(ch)) * 0.8) {
    cost = cost * 0.5;
  }

  if ((getCurST(ch)) < cost) {
    send_to_char(ch, "You do not have enough stamina to perform this technique.\r\n");
    return;
  }

  int chance = axion_dice(0);

  if (chance > char_get_aff_abils_wis(ch) + rand_number(1, 10)) {
    act("@GYou spread your wings and begin to concentrate. Your wings begin to glow a soft sea green color. As you prepare to release a cloud of your charged wing dust you lose focus and the power you had begun to charge into your wings dissipates.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly $s wings begin to glow a soft sea green color. This soft glow grows brighter for a second before fading completely.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurST(ch, cost, 0);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  incCurHealth(ch, heal, true);
  decCurST(ch, cost, 0);
  char_set_wait(ch, PULSE_1SEC);

  act("@GYou spread your wings and begin to concentrate. Your wings begin to glow a soft sea green color. As your wings grow brighter you focus your charged bio energy in a shockwave the unleashes a cloud of glowing green dust. You breath in the dust and feel it rejuvinate your body's cells!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly $s wings begin to glow a soft sea green color. This soft glow grows brighter and as $e flexes $s wings to their full extent a shockwave of energy explodes outward. Carried on this shockwave is a cloud of glowing dust! You notice some of the dust being breathed in by $s!@n", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "@RHeal@Y: @C%s@n\r\n", add_commas(heal));

  struct char_data *vict = NULL, *next_v = NULL;

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_GROUP)) {
      if (ch->master == vict->master || vict->master == ch || ch->master == vict) {
        incCurHealth(vict, heal, true);
        act("@CYou breathe in the dust and are healed by it somewhat!@n", TRUE, vict, 0, 0, TO_CHAR);
        act("@c$n@C breathes in the dust and is healed somewhat!@n", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char(vict, "@RHeal@Y: @C%s@n\r\n", add_commas(heal));
      }
    }
  }
}

ACMD(do_shell)
{

  if (!char_is_arlian(ch)) {
    send_to_char(ch, "You are not capable of doing that!\r\n");
    return;
  }

  if (char_get_sex(ch) == SEX_FEMALE) {
    send_to_char(ch, "Sorry, you can't do that.\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SHELL)) {
    act("@mYou quickly absorb the armor carapace covering your body back inside.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n's@m armored carapce retreats back to its original size.@n", TRUE, ch, 0, 0, TO_ROOM);
    char_aff_set(ch, AFF_SHELL, false);
    return;
  }

  if ((getCurST(ch)) < getMaxST(ch) * 0.2) {
    send_to_char(ch, "You do not have enough stamina to grow your armored carapace.@n\r\n");
    return;
  }
  if (axion_dice(0) > char_get_aff_abils_con(ch) + rand_number(1, 10)) {
    act("@mYou crouch down and begin to focus on your body's carapace cells encouraging them to multiply! However your control is lacking and you ultimately fail to grow your armor very much.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n@m crouches down and seems to strain for a moment before giving up and resuming $s normal stance.@n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  act("@mYou crouch down and begin to focus on your body's carapace cells, encouraging them to multiply! Very quickly millions of new carapace cells have been born and your armored carapace extends over all parts of your body!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@M$n@m crouches down and after a few moments of straining $s body's carapace armor starts to grow thicker and extends to cover all parts of $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
  decCurSTPercent(ch, .2, 0);
  char_aff_set(ch, AFF_SHELL, true);
  return;
}

ACMD(do_liquefy)
{

  if (!char_is_majin(ch)) {
    send_to_char(ch, "You are not capable of liquefying yourself right now. Try finding a giant blender maybe?\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_LIQUEFIED)) {
    act("@MSuddenly large chunks of goo start to hover up slowly. These very same chunks quickly begin to fly into each other, piling on as the ball of goo grows. Suddenly @m$n@M emerges as the ball of goo takes $s shape!@n", TRUE, ch, 0, 0, TO_ROOM);
    act("@MYou begin to pull the liquid chunks of your body together. Those chunks hover upward and merge into each other until a large ball of goo is formed. Slowly your body emerges as the pieces of your body take on their old form!@n", TRUE, ch, 0, 0, TO_CHAR);
    char_aff_set(ch, AFF_LIQUEFIED, false);
    char_set_wait(ch, PULSE_3SEC);
    char_set_wait(ch, PULSE_3SEC);
    char_set_wait(ch, PULSE_3SEC);
    char_set_wait(ch, PULSE_3SEC);
    char_set_wait(ch, PULSE_3SEC);
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
    return;
  }

  if ((getCurKI(ch)) < (getMaxKI(ch) * 0.002) + 150) {
    send_to_char(ch, "You do not have enough ki to manage this level of body control!\r\n");
    return;
  }

  if (!strcasecmp(arg, "hide")) {
    if (char_get_grappled(ch)) {
      char_set_grappling(char_get_grappled(ch), NULL);
      char_set_grappled(ch, NULL);
    }
    if (char_get_grappling(ch)) {
      char_set_grappled(char_get_grappling(ch), NULL);
      char_set_grappling(ch, NULL);
    }
    if (char_get_drag(ch)) {
      char_set_dragged(char_get_drag(ch), NULL);
      char_set_drag(ch, NULL);
    }
    if (axion_dice(0) > char_get_level(ch)) {
      act("@MYour body starts to become loose and sag, but you lose focus and it reverts to its original shape!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag, but $e seems to return normal a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);

      return;
    }
    act("@MYour body starts to become loose and sag. It continues to droop down until it begins to run down like a river of goo flowing from where your body was.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@m$n@M's body starts to become loose and sag. Much of $s body begins to pour down and scatter around as pools of goo.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
    char_aff_set(ch, AFF_LIQUEFIED, true);
    return;
  }
  if (!strcasecmp(arg, "explode")) {
    struct char_data *vict;
    if (char_get_grappled(ch)) {
      char_set_grappling(char_get_grappled(ch), NULL);
      char_set_grappled(ch, NULL);
    }
    if (char_get_grappling(ch)) {
      char_set_grappled(char_get_grappling(ch), NULL);
      char_set_grappling(ch, NULL);
    }
    if (char_get_drag(ch)) {
      char_set_dragged(char_get_drag(ch), NULL);
      char_set_drag(ch, NULL);
    }
    if (!*arg2) {
      send_to_char(ch, "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
      return;
    }
    if ((getCurKI(ch)) < (getMaxKI(ch) * 0.10) + 150) {
      send_to_char(ch, "You do not have enough ki for that action!@n\r\n");
      return;
    }
    if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "That target isn't here.\r\n");
      return;
    }
    if (!can_kill(ch, vict, NULL, 1)) {
      send_to_char(ch, "You can't kill them!\r\n");
      return;
    }
    if (axion_dice(0) > char_get_level(ch)) {
      act("@MYour body starts to become loose and sag, but you lose focus and it reverts to its original shape!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag, but $e seems to return normal a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (char_get_speedi(ch) < char_get_speedi(vict)) {
      act("@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However $E easily dodges and avoids your attempt!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you are faster and managed to dodge the attempt.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However $E easily dodges and avoids @m$n's@M attempt!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
      if (!char_get_fighting(ch)) {
        set_fighting(ch, vict);
      }
      if (!char_get_fighting(vict)) {
        set_fighting(vict, ch);
      }
      char_set_wait(ch, PULSE_3SEC);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (getCurPL(ch) < getCurPL(vict) * 2) {
      act("@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However as you force yourself in through $S mouth $E successfully resists and forces your back out!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you think quickly and force $m out before $e has a chance to get fully into your body!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However as $e forces $mself in through @R$N's@M mouth $E manages to resist and force @m$n@M back out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
      int64_t dmg = getEffMaxPL(ch) * 0.08;
      hurt(0, 0, ch, vict, NULL, dmg, 0);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    act("@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! As you fill $S body you expand outward until $s body explodes into a gory mess!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! As $e fills your body it begins to expand until it is unable to take the strain any longer and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! As $e forces $mself in through @R$N's@M mouth $S body begins to expand until it can't take the strain any longer and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
    if (char_aff_flagged(ch, AFF_GROUP)) {
      group_gain(ch, vict);
    } else {
      solo_gain(ch, vict);
    }
    die(vict, ch);
    char_aff_set(ch, AFF_LIQUEFIED, true);
    char_set_wait(ch, PULSE_3SEC);
    char_set_wait(ch, PULSE_3SEC);
    char_set_wait(ch, PULSE_3SEC);
    handle_cooldown(ch, 9);
    return;
  }
  send_to_char(ch, "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
  return;
}

ACMD(do_lifeforce)
{

  char arg[MAX_INPUT_LENGTH];
  int setting = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: life (0 - 99)\n0 is off.\r\n");
    return;
  }

  setting = atoi(arg);

  if (setting > 99) {
    send_to_char(ch, "Syntax: life (1 - 99)\n%s isn't an acceptable percent.\r\n", add_commas(setting));
    return;
  }
  if (setting <= 0) {
    send_to_char(ch, "Your will just isn't in the fight, huh?\nYou will not use up life force to maintain your PL period.\r\n");
    char_set_lifeperc(ch, 0);
    return;
  }
  send_to_char(ch, "Your life force will automatically kick in at %d%s of your optimal PL.\r\n", setting, "%");
  char_set_lifeperc(ch, setting);
  return;
}

ACMD(do_defend)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg && char_get_defending(ch) == NULL) {
    send_to_char(ch, "Defend who?\r\n");
    return;
  }
  if (!*arg && char_get_defending(ch)) {
    act("@YYou stop defending @y$N@Y.@n", TRUE, ch, 0, char_get_defending(ch), TO_CHAR);
    act("@y$n@Y stops defending you.@n", TRUE, ch, 0, char_get_defending(ch), TO_VICT);
    act("@y$n@Y stops defending @y$N@Y.@n", TRUE, ch, 0, char_get_defending(ch), TO_NOTVICT);
    char_set_defender(char_get_defending(ch), NULL);
    char_set_defending(ch, NULL);
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "You can't seem to find that person.\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "Well hopefully you are smart enough to defend yourself.\r\n");
    return;
  }
  act("@YYou start defending @y$N@Y.@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@y$n@Y starts defending you.@n", TRUE, ch, 0, vict, TO_VICT);
  act("@y$n@Y starts defending @y$N@Y.@n", TRUE, ch, 0, vict, TO_NOTVICT);
  char_set_defender(vict, ch);
  char_set_defending(ch, vict);
  return;
}

ACMD(do_scry)
{

  if (strcasecmp(CAP(char_get_name(ch)), "Galeos")) {
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

  if (char_is_npc(vict)) {
    send_to_char(ch, "No using this on mobs!\r\n");
    return;
  }

  int cost = 2000;

  if (char_get_practice_points(ch) < cost) {
    send_to_char(ch, "You do not have enough PS to Oracle Scry!\r\n");
    return;
  }
  reveal_hiding(ch, 0);
  act("@GYou focus your mind and begin to allow the flood of images and energy to roar through your mind. You then allow those thoughts to make their way into the mind of @c$N@G. You can hardly comprehend the vastness of the information flooding in, yet still glimpse bits and pieces of your own destiny.@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@GYou see @C$n@G begin to focus, and then without warning, your mind is flooded painfully with images, energy and information. The data streams in a mad torrent through your psyche, and just when you think snapping is possible, the voice of @C$n@G comes to you and eases and guides you. You see images of potential futures, information not yet known, knowledge yet undiscovered. Though you could not fully  grasp what is to come, you feel more prepared at facing the unknown.@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W appears to be performing some sort of ritual or something with @c$N@W.@n", TRUE, ch, 0, vict, TO_NOTVICT);
  int64_t boost = char_get_aff_abils_intel(ch) * 0.5;

  gainBasePL(vict, (getBasePL(vict) * .01) * boost, false);
  gainBaseKI(vict, (getBaseKI(vict) * .01) * boost, false);
  gainBaseST(vict, (getBaseST(vict) * .01) * boost, false);

  send_to_char(vict, "Your Powerlevel, Ki, and Stamina have improved drastically! On top of that your Intelligence and Wisdom have improved permanantly!\r\n");
  vict->real_abils.intel += 2;
  vict->real_abils.wis += 2;
  char_set_practice_points(ch, char_get_practice_points(ch) - (2000));
  if (char_get_level(ch) < 100) {
    send_to_char(ch, "@D[@mPractice Sessions@D:@R -2000@D]@n\r\n");
    if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0) {
      char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch)));
      send_to_char(ch, "The remaining experience needed for your next level up has been gained!@n\r\n");
    } else {
      send_to_char(ch, "Due to already having enough experience to level up you gain no expereince.\r\n");
    }
  } else {
    gainBaseAllPercent(ch, .025, true);
    send_to_char(ch, "Your Powerlevel, Ki, and Stamina have improved!\r\n");
  }
}

void ash_burn(struct char_data *ch)
{
  struct obj_data *obj, *next_obj;

  if (ch && char_get_in_room(ch) != NOWHERE) {
    for (obj = world[char_get_in_room(ch)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (obj_get_vnum(obj) == 1306) {
        if (axion_dice(0) > char_get_aff_abils_con(ch)) {
          if (!char_is_android(ch) && !char_is_demon(ch) && !char_is_icer(ch)) {
            reveal_hiding(ch, 0);
            decCurST(ch, ((getMaxST(ch) * 0.005) + 20) * obj_get_cost(obj), 0);
            act("@RYou choke on the the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@r$n@R chokes on the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n", TRUE, ch, 0, 0, TO_ROOM);
          }
          if (!char_is_android(ch) && !char_is_demon(ch) && !char_is_npc(ch)) {
            if (!char_plr_flagged(ch, PLR_EYEC) && !char_aff_flagged(ch, AFF_BLIND)) {
              reveal_hiding(ch, 0);
              act("@DYour eyes sting from the hot ash! You can't see!@n", TRUE, ch, 0, 0, TO_CHAR);
              act("@r$n@D eyes appear to have been hurt by the ash!@n", TRUE, ch, 0, 0, TO_ROOM);
              int duration = 1;
              assign_affect(ch, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
            }
          }
        }
      }
    }
  }
}

ACMD(do_ashcloud)
{

  if (!char_is_demon(ch)) {
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

  struct obj_data *ash = NULL, *obj, *next_obj;
  int there            = FALSE;

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj_get_vnum(obj) == 1305) {
      ash = obj;
    }
  }

  for (obj = world[char_get_in_room(ch)].contents; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj_get_vnum(obj) == 1306) {
      there = TRUE;
    }
  }

  if (there == TRUE) {
    send_to_char(ch, "You can not pile more ash into the air without causing it to clump together and settle.\r\n");
    return;
  }

  if (!ash) {
    send_to_char(ch, "You do not have any ash!\r\n");
    return;
  }

  level = atoi(arg);

  int64_t mult   = 5;
  double initial = 0.0;

  switch (level) {
    case 1:
      mult    = 20;
      initial = 0.25;
      break;
    case 2:
      mult    = 10;
      initial = 0.10;
      break;
    case 3:
      mult    = 5;
      initial = 0.05;
      break;
    default:
      send_to_char(ch, "Syntax: ashcloud (1 | 2 | 3)\r\n");
      return;
  }

  int64_t cost = (getMaxKI(ch) * initial) + (char_get_aff_abils_intel(ch) * mult);

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki!\r\n");
    return;
  }
  if (rroom_sunken(char_get_in_room(ch))) {
    send_to_char(ch, "You can not create an ashcloud here, because it is too wet.\r\n");
    return;
  }
  if (rroom_sect(char_get_in_room(ch)) == SECT_SPACE) {
    send_to_char(ch, "You can not create an ashcloud in space.\r\n");
    return;
  }
  if (char_get_aff_abils_intel(ch) < axion_dice(-10)) {
    reveal_hiding(ch, 0);
    act("@RYou take a handful of ashes, and when you go to blow flames at it you lose focus. The ashes are blown from your hands by your huge gust of breath.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@r$n@R takes a handful of ashes from $s belongings and blows it out of $s hands with a strong gust of air. @YStrange.@n", TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(ash);
    decCurKI(ch, cost, 0);
    return;
  }
  struct obj_data *ashcloud;
  reveal_hiding(ch, 0);
  if (level == 3) {
    decCurKI(ch, cost, 0);
    act("@RYou take a handful of ashes and you create a fierce heat within your lungs. With the heat ready you breathe ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@r$n@R takes a handful of ashes and $e breathes ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_room(char_get_in_room(ch), "@WThe ashes ripple with an intense aftershock of power.@n\r\n");
    ashcloud = read_object(1306, VIRTUAL);
    obj_to_room(ashcloud, char_get_in_room(ch));
    extract_obj(ash);
    obj_set_timer(ashcloud, 4);
    obj_set_cost(ashcloud, 3);
    ash_burn(ch);
  } else if (level == 2) {
    decCurKI(ch, cost, 0);
    act("@RYou take a handful of ashes and you create a fierce heat within your lungs. With the heat ready you breathe ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@r$n@R takes a handful of ashes and $e breathes ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_room(char_get_in_room(ch), "@WThe ashes ripple with a strong aftershock of power.@n\r\n");
    ashcloud = read_object(1306, VIRTUAL);
    obj_to_room(ashcloud, char_get_in_room(ch));
    obj_set_timer(ashcloud, 2);
    obj_set_cost(ashcloud, 2);
    extract_obj(ash);
    ash_burn(ch);
  } else {
    decCurKI(ch, cost, 0);
    act("@RYou take a handful of ashes and you create a fierce heat within your lungs. With the heat ready you breathe ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@r$n@R takes a handful of ashes and $e breathes ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", TRUE, ch, 0, 0, TO_ROOM);
    ashcloud = read_object(1306, VIRTUAL);
    obj_to_room(ashcloud, char_get_in_room(ch));
    extract_obj(ash);
    obj_set_timer(ashcloud, 1);
    obj_set_cost(ashcloud, 1);
    ash_burn(ch);
  }
}

ACMD(do_healglow)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!char_get_skill(ch, SKILL_HEALGLOW)) {
    send_to_char(ch, "You do not know how to perform that technique.\r\n");
    return;
  }

  if (!*arg) {
    vict = ch;
  } else if (char_get_skill(ch, SKILL_HEALGLOW) < 100) {
    send_to_char(ch, "You can not target anyone except yourself unless you are a master of this technique.\nSyntax: healingglow\r\n");
    return;
  } else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Nobody around by that name.\r\n");
    return;
  }

  if (char_aff_flagged(vict, AFF_HEALGLOW) && vict == ch) {
    send_to_char(ch, "You already have a healing glow surrounding your body.\r\n");
    return;
  }
  if (char_aff_flagged(vict, AFF_HEALGLOW)) {
    send_to_char(ch, "They already have a healing glow surrounding their body.\r\n");
    return;
  }

  if (char_get_fighting(vict) && vict == ch) {
    send_to_char(ch, "You are too busy fighting!@n\r\n");
    return;
  }
  if (char_get_fighting(vict)) {
    send_to_char(ch, "They are too busy fighting!@n\r\n");
    return;
  }

  int64_t cost = getMaxKI(ch) * 0.5;

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki. It requires at least 50%s of your ki in cost.\r\n", "%");
    return;
  }
  if (vict == ch) {
    act("@CPlacing your hands on your body you begin to focus your energies. Slowly a strong blue glow glistens and shines across your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C places $s hands on $s body. Slowly a strong blue glow glistens and shines across $s skin!@n", TRUE, ch, 0, 0, TO_ROOM);
    char_aff_set(vict, AFF_HEALGLOW, true);
    int duration = (char_get_skill(ch, SKILL_HEALGLOW) * 0.1);
    if (duration <= 0)
      duration = 1;
    assign_affect(ch, AFF_HEALGLOW, SKILL_HEALGLOW, duration, 0, 0, 0, 0, 0, 0);
    decCurKI(ch, cost, 0);
  } else {
    act("@CPlacing your hands on @c$N's@C body you begin to focus your energies. Slowly a strong blue glow glistens and shines across $S skin!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@c$n@C places $s hands on YOUR body. Slowly a strong blue glow glistens and shines across your skin!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@c$n@C places $s hands on @c$N's@C body. Slowly a strong blue glow glistens and shines across $S skin!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    int duration = (char_get_skill(ch, SKILL_HEALGLOW) * 0.1);
    duration += rand_number(-2, 1);
    if (duration <= 0)
      duration = 1;
    assign_affect(ch, AFF_HEALGLOW, SKILL_HEALGLOW, duration, 0, 0, 0, 0, 0, 0);
    decCurKI(ch, cost, 0);
  }
}

/* Demon's lame skill */
ACMD(do_metamorph)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(ch, SKILL_METAMORPH)) {
    return;
  }

  if (char_get_alignment(ch) >= 51) {
    send_to_char(ch, "Your heart is too pure to use that technique!\r\n");
    return;
  }

  int64_t cost = (getMaxKI(ch) * 0.16);

  if (char_aff_flagged(ch, AFF_METAMORPH)) {
    send_to_char(ch, "You are already surrounded by a dark aura!\r\n");
    return;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki. You need %s.\r\n", add_commas(cost));
    return;
  }

  int chance = axion_dice(0), perc = (char_get_aff_abils_wis(ch) * 2);

  if (perc < 100 && perc > 60)
    perc += 100 - perc;
  else if (perc < 100)
    perc += 10;

  decCurKI(ch, cost / 2, 0);
  if (perc < chance) {
    act("@WYou focus your energies and prepare your @RDark Metamorphisis@W but screw up your focus!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@WA dark @Rred@W glow starts to surround @C$n@W, but it fades quickly.@n", TRUE, ch, 0, 0, TO_ROOM);

    return;
  }
  act("'@RDark@W...' An explosion of sanguine aura erupts over the surface of your body, your eyes darkening to a bleeding crimson. The flaring glow emanating from your body pronounces the shadows cast, a darkening umbrage that threatens a malicious promise. Fists clench tightly, muscles bulking as you hiss; You complete the transition, relaxing visibly, '...@RMetamorphosis@W'@n", TRUE, ch, 0, 0, TO_CHAR);
  act("'@RDark@W...' An explosion of sanguine aura erupts over the surface of @C$n@W's body, $s eyes darkening to a bleeding crimson. The flaring glow emanating from $s body pronounces the shadows cast, a darkening umbrage that threatens a malicious promise. Fists clench tightly, muscles bulking as $e hisses; $e completes the transition, relaxing visibly, '...@RMetamorphosis@W'@n", TRUE, ch, 0, 0, TO_ROOM);

  int duration = char_get_aff_abils_intel(ch) / 12;
  assign_affect(ch, AFF_METAMORPH, SKILL_METAMORPH, duration, 0, 0, 0, 0, 0, 0);
  incCurHealthPercent(ch, .6, true);
  return;
}

ACMD(do_shimmer)
{

  int skill = 0, perc = 0, location = 0;
  int64_t cost          = 0;
  struct char_data *tar = NULL;

  char arg[MAX_INPUT_LENGTH] = "";

  one_argument(argument, arg);

  if (!char_is_npc(ch)) {
    if (char_prf_flagged(ch, PRF_ARENAWATCH)) {
      char_prf_set(ch, PRF_ARENAWATCH, false);
      char_set_arenawatch(ch, -1);
      send_to_char(ch, "You stop watching the arena action.\r\n");
    }
  }
  if (strcasecmp(char_get_name(ch), "Anubis")) {
    send_to_char(ch, "You do not even know how to perform that skill!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
    send_to_char(ch, "@rYou are in a pocket dimension!@n\r\n");
    return;
  }
  if (!*arg) {
    send_to_char(ch, "Who or where do you want to shimmer to? [target | planet-(planet name) | afterlife]\r\n");
    send_to_char(ch, "Example: shimmer goku\nExample 2: shimmer planet-earth\r\n");
    return;
  }

  cost = getMaxKI(ch) / 40;

  if ((getCurKI(ch)) - cost < 0) {
    send_to_char(ch, "You do not have enough ki to instantaneously move.\r\n");
    return;
  }

  perc  = axion_dice(0);
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
    send_to_char(ch, "Who or where do you want to shimmer to? [target | planet-(planet name) | afterlife]\r\n");
    send_to_char(ch, "Example: shimmer goku\nExample 2: shimmer planet-earth\r\n");
    return;
  }

  if (skill < perc || (char_get_fighting(ch) && rand_number(1, 2) <= 1)) {
    if (tar != NULL) {
      if (tar != ch) {
        send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
        decCurKI(ch, cost, 0);
        char_set_wait(ch, PULSE_2SEC);
        return;
      }
      send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
      return;
    }
    send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  reveal_hiding(ch, 0);
  char_set_wait(ch, PULSE_2SEC);
  if (tar != NULL) {
    if (tar == ch) {
      send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
      return;
    }
    if (char_get_grappling(ch) && char_get_grappling(ch) == tar) {
      send_to_char(ch, "You are already in the same room with them and are grappling with them!\r\n");
      return;
    }
    if (char_get_admlevel(tar) > 0 && char_get_admlevel(ch) < 1) {
      send_to_char(ch, "That immortal prevents you from reaching them.\r\n");
      return;
    }
    if (rroom_flagged(char_get_in_room(tar), ROOM_NOINSTANT)) {
      send_to_char(ch, "You can not go there as it is a protected area!\r\n");
      return;
    }
    if (char_get_grappling(ch) && char_aff_flagged(char_get_grappling(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    }
    if (char_get_drag(ch) && char_aff_flagged(char_get_drag(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    }
    if (char_get_grappled(ch) && char_aff_flagged(char_get_grappled(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    }

    decCurKI(ch, cost, 0);
    act("@wYour body begins to fade away almost appearing ghost like, before a ripple passes through your image and your are gone in an instant!@n", TRUE, ch, 0, tar, TO_CHAR);
    act("@w$n@w appears in an instant out of nowhere right next to you!@n", TRUE, ch, 0, tar, TO_VICT);
    act("@w$n@w body begins to fade away almost appearing ghost like, before a ripple passes through $s image and $e is gone in an instant!@n", TRUE, ch, 0, tar, TO_NOTVICT);
    char_plr_set(ch, PLR_TRANSMISSION, true);
    handle_teleport(ch, tar, 0);
  } else {
    decCurKI(ch, cost, 0);
    act("@wYour body begins to fade away almost appearing ghost like, before a ripple passes through your image and your are gone in an instant!@n", TRUE, ch, 0, tar, TO_CHAR);
    act("@w$n@w body begins to fade away almost appearing ghost like, before a ripple passes through $s image and $e is gone in an instant!@n", TRUE, ch, 0, tar, TO_NOTVICT);
    handle_teleport(ch, NULL, location);
  }
}

ACMD(do_channel)
{

  if (!char_is_demon(ch) || char_get_skills_base(ch, SKILL_STYLE) < 40) {
    send_to_char(ch, "You are not a Demon!\r\n");
    return;
  }

  if (char_get_skills_base(ch, SKILL_STYLE) < 40) {
    send_to_char(ch, "This requires a fighting style at level 40 or more!!\r\n");
    return;
  }

  int64_t cost = getMaxKI(ch) * 0.15;

  int chance = axion_dice(0), skill = char_get_skill(ch, SKILL_STYLE);

  if (cost > (getCurKI(ch))) {
    send_to_char(ch, "You do not have enough ki to channel with!\r\n");
    return;
  }

  struct obj_data *obj, *next_obj = NULL, *ruby = NULL;
  int found = FALSE;

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (found == FALSE && obj_get_vnum(obj) == 6600) {
      if (!obj_extra_flagged(obj, ITEM_HOT)) {
        found = TRUE;
        ruby  = obj;
      }
    }
  }

  if (found == FALSE) {
    send_to_char(ch, "You do not have any uncharged blood rubies.\r\n");
    return;
  }

  if (rroom_get_geffect(char_get_in_room(ch)) <= 0) {
    send_to_char(ch, "There is no lava here!\r\n");
    return;
  }

  if (ruby) {
    if (skill < chance) {
      act("@RAs you move your ki through the lava you begin to draw heat away from it into the ruby. You screw up the rate of heating though and cause the ruby to crumble to dust!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@RAs $n@R moves $s ki through the lava $e begins to draw heat away from it into a blood ruby. However $e screws up the rate of heating and causes the ruby to crumble to dust!@n", TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(ruby);
    } else {
      act("@RAs you move your ki through the lava you begin to draw heat away from it into the ruby. You do so at an even rate and end up with a glowing red hot blood ruby!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@RAs $n@R moves $s ki through the lava $e begins to draw heat away from it into a blood ruby. The ruby glows red hot as $e finishes the process of channeling the heat!@n", TRUE, ch, 0, 0, TO_ROOM);
      rroom_set_geffect(char_get_in_room(ch), 0);
      obj_extra_set(ruby, ITEM_HOT, true);
    }
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_1SEC);
  }
}

ACMD(do_hydromancy)
{

  if (!char_is_tsuna(ch) || char_get_skills_base(ch, SKILL_STYLE) <= 0) {
    send_to_char(ch, "You know nothing about hydromancy!\r\n");
    return;
  }

  int skill = char_get_skills_base(ch, SKILL_STYLE), chance = axion_dice(0);
  int64_t cost = 0;

  cost = (getMaxKI(ch) / 12) - (char_get_aff_abils_intel(ch) * char_get_level(ch));

  if (rroom_get_geffect(char_get_in_room(ch)) >= 0 && rroom_sect(char_get_in_room(ch)) != SECT_WATER_SWIM && rroom_sect(char_get_in_room(ch)) != SECT_WATER_NOSWIM) {
    if (rroom_sect(char_get_in_room(ch)) != SECT_UNDERWATER) {
      send_to_char(ch, "There is not sufficient water here.\r\n");
      return;
    }
    send_to_char(ch, "There is too much water here to control!\r\n");
    return;
  }

  if (cost <= 0)
    cost = 100;

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to manipulate any water around you.\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(ch, "You must wait a short period before concentrating again.\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax 1: hydromancy flood (direction)\r\n");
    send_to_char(ch, "Example: hydromancy flood nw\r\n");
    send_to_char(ch, "\nSyntax 2: hydromancy spike\r\n");
    return;
  }

  int attempt = 0;

  if (!strcasecmp(arg, "spike")) {
    struct obj_data *obj = NULL;

    cost = 100 + (char_get_skill(ch, SKILL_STYLE) / (1 + (getMaxKI(ch) * 0.5)));

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to form an ice spike.\r\n");
      return;
    }

    if (skill < chance) {
      decCurKI(ch, cost, 0);
      act("@CYou press your palms together in front of your body but you fail to produce the proper control to form the spike!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C presses $s palms together and then slowly pulls them apart. Nothing important appears to have happened.", TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_STYLE, 2);
      return;
    }

    if (skill >= 100) {
      obj = read_object(19058, VIRTUAL);
    } else if (skill >= 50) {
      obj = read_object(19057, VIRTUAL);
    } else if (skill >= 1) {
      obj = read_object(19056, VIRTUAL);
    }

    if (!obj) {
      send_to_char(ch, "You fail to form an ice spike.\r\n");
      return;
    }

    decCurKI(ch, cost, 0);
    act("@CYou press your palms together in front of your body and focusing ki you force water up along your body. That water pools between your palms and as pull your palms apart a @c$p@C forms!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@c$n@C presses $s palms together in front of $s body and water begins to flow up $s body and pools between $s palms. Slowly pulling them apart reveals a @c$p@C as it forms between them!@n", TRUE, ch, obj, 0, TO_VICT);
    if (obj_get_weight(obj) + (getCurCarriedWeight(ch)) <= getMaxCarryWeight(ch))
      obj_to_char(obj, ch);
    else {
      send_to_char(ch, "You are unable to hold it and so let it go at your feet.\r\n");
      act("@C$n@w drops an ice spike.@n", TRUE, ch, 0, 0, TO_ROOM);
      obj_to_room(obj, char_get_in_room(ch));
    }
    improve_skill(ch, SKILL_STYLE, 1);
    char_set_con_cooldown(ch, 10);
  } else if (!strcasecmp(arg, "flood")) {
    if (!*arg2) {
      send_to_char(ch, "Syntax 1: hydromancy flood (direction)\r\n");
      send_to_char(ch, "Example: hydromancy flood nw\r\n");
      send_to_char(ch, "\nSyntax 2: hydromancy spike\r\n");
      return;
    }

    attempt = search_block(arg2, dirs, FALSE);

    if (char_can_go(ch, attempt)) {
      struct char_data *vict, *next_v;

      int last = char_get_lastattack(ch);
      char_set_lastattack(ch, 500);
      char bun[MAX_STRING_LENGTH], bunn[MAX_STRING_LENGTH];

      if (skill < chance) {
        act("@BUsing your ki you attempt to create a rush of water! @RYou fail!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@b$n@B seems to attempt to create water with $s ki! @RHowever, $e fails!@n", TRUE, ch, 0, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
        char_set_wait(ch, PULSE_2SEC);
      } else {
        decCurKI(ch, cost, 0);
        snprintf(bun, sizeof(bun), "@BUsing your ki you create a rush of water flooding away toward the @C%s@B!@n", dirs[attempt]);
        snprintf(bunn, sizeof(bunn), "@B$n@B uses $s ki to create a rush of water flooding away toward the @C%s@B!@n", dirs[attempt]);
        act(bun, TRUE, ch, 0, 0, TO_CHAR);
        act(bunn, TRUE, ch, 0, 0, TO_ROOM);

        for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
          next_v = vict->next_in_room;
          if (vict == ch)
            continue;
          if (!can_kill(ch, vict, NULL, 1)) {
            act("@CYou are protected from the water!@n", TRUE, vict, 0, 0, TO_VICT);
            act("@C$n@C is protected from the water!@n", TRUE, vict, 0, 0, TO_ROOM);
          } else if (char_is_kanassan(vict)) {
            act("@CYou effortlessly swim against the current.@n", TRUE, vict, 0, 0, TO_CHAR);
            act("@C$n@C effortlessly swims against the current.@n", TRUE, vict, 0, 0, TO_ROOM);
          } else if (char_get_skills_base(vict, SKILL_BALANCE) >= axion_dice(-10)) {
            act("@CYou manage to keep your balance and are not swept away!@n", TRUE, vict, 0, 0, TO_CHAR);
            act("@C$n@C manages to keep $s balance and is not swept away!@n", TRUE, vict, 0, 0, TO_ROOM);
          } else if (char_aff_flagged(ch, AFF_FLYING)) {
            act("@CYou fly above the rushing waters and are untouched.@n", TRUE, vict, 0, 0, TO_CHAR);
            act("@C$n@C flies above the rushing waters and is untouched.@n", TRUE, vict, 0, 0, TO_ROOM);
          } else {
            act("@cYou are caught by the rushing waters and sent tumbling away!@n", TRUE, vict, 0, 0, TO_CHAR);
            act("@c$n@c is caught by the rushing waters and sent tumbling away!@n", TRUE, vict, 0, 0, TO_ROOM);
            do_simple_move(vict, attempt, TRUE);
            hurt(0, 0, ch, vict, NULL, cost * 4, 1);
          }
        }
        rroom_set_geffect(char_get_exit(ch, attempt)->to_room, -3);
        char_set_lastattack(ch, last);
        char_set_wait(ch, PULSE_2SEC);
        char_set_con_cooldown(ch, 15);
      }
    } else {
      send_to_char(ch, "You can not flood the water that direction!\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Syntax 1: hydromancy (flood) (direction)\r\n");
    send_to_char(ch, "Example: hydromancy flood nw\r\n");
    send_to_char(ch, "\nSyntax 2: hydromancy spike\r\n");
    return;
  }
}

ACMD(do_kanso)
{

  if (char_is_npc(ch)) /* No mobs */
    return;

  if (strcasecmp(char_get_name(ch), "Levanthoth")) { /* NOT the right player */
    send_to_char(ch, "You do not know how to perform that technique. \r\n");
    return;
  }

  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Perform Kanso Suru on who?\r\n");
    return;
  }

  if (char_is_android(vict)) {
    send_to_char(ch, "Mechanical beings are not effected by this technique.\r\n");
    return;
  }

  if (!can_kill(ch, vict, NULL, 0)) {
    return;
  }

  int64_t cost = getMaxKI(ch) / char_get_aff_abils_intel(ch);
  int dice = axion_dice(-5), skill = char_get_aff_abils_intel(ch), pdice = axion_dice(0);
  int dam = rand_number(1, 4);
  struct affected_type af;

  /* Bonus based on wisdom */
  if (char_get_aff_abils_wis(ch) > axion_dice(-5))
    dam += 1;

  if ((getCurKI(ch)) < cost) { /* Not enough ki */
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }

  if (skill < dice) {                                                                                                                                                                            /* Failed the technique, user's fault */
    act("You close your eyes and focus, before bounding effortlessly toward $N. Closing the distance, you place your hands on $N's chest but nothing happens!\r\n", TRUE, ch, 0, vict, TO_CHAR); /* Message Character $n sees */
    act("$n closes $s eyes and bounds effortlessly towards you. Closing the distance, $e places $s hands on your chest but nothing happens!\r\n", TRUE, ch, 0, vict, TO_VICT);                   /* Message Vict $N Sees */
    act("$n closes $s eyes and bounds toward $N. Smirking, $e puts $m hands on $N's chest but nothing seems to happen.\r\n", TRUE, ch, 0, vict, TO_NOTVICT);                                     /* Message everyone else sees */
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_2SEC); /* 2 second lag for the technique */
    return;
  } else { /* Success! */
    /* Main Attack Success Messages */
    act("You close your eyes and focus, before effortlessly bounding towards $N. Closing the distance, you smirk at $N and place your hands on their chest. Electricity flows into their body as you draw water out of their very cells!\r\n", TRUE, ch, 0, vict, TO_CHAR);                                               /* Message Character $n sees */
    act("$n closes $s eyes and focuses, before effortlessly bounding toward you. Closing the distance, $e smirks and places both of $s hands on your chest. Electricity begins to pulse through your body and a great thirst takes hold, as if $n is drawing the water from your body!\r\n", TRUE, ch, 0, vict, TO_VICT); /* Message Vict $N Sees */
    act("$n closes $s eyes, before effortlessly bounding toward $N. Closing the distance, $n smirks and places both $s hands on $N's chest. Electricity seems to pass from $n's body to $N's!\r\n", TRUE, ch, 0, vict, TO_NOTVICT);                                                                                       /* Message everyone else sees */
                                                                                                                                                                                                                                                                                                                          /* End Main Messages */

    decCurKI(ch, cost, 0);

    /* Handle the thirst aspect */
    if (char_get_conditions(vict, THIRST) - dam >= 0)
      char_set_conditions(vict, THIRST, char_get_conditions(vict, THIRST) - (dam));
    else
      char_set_conditions(vict, THIRST, 0);
    if (char_get_conditions(ch, THIRST) + dam <= 48)
      char_set_conditions(ch, THIRST, char_get_conditions(ch, THIRST) + (dam));
    else
      char_set_conditions(ch, THIRST, 48);

    /* Heal the user */
    incCurHealth(ch, (getEffMaxPL(ch) * .01) * dam, true);

    char_set_wait(ch, PULSE_2SEC); /* 2 second lag for the technique */

    if (!is_affected(vict, AFF_HYDROZAP)) { /* Drop their AGL/CON if not already lowered */
      send_to_char(vict, "@RYou feel less agile and your muscles ache!@n\r\n");
      assign_affect(vict, AFF_HYDROZAP, 0, -1, 0, -4, -4, 0, 0, 0);
      save_char(vict);
    }

    if (skill > pdice && !char_aff_flagged(vict, AFF_PARA)) {                    /* Paralyze them too */
      act("@R$N@W is paralyzed by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);   /* Message Character $n sees */
      act("@RYou are paralyzed by the attack!@n", TRUE, ch, 0, vict, TO_VICT);   /* Message Vict $N Sees */
      act("@R$N@Wis paralyzed by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT); /* Message everyone else sees */
      af.type      = SKILL_PARALYZE;
      af.duration  = rand_number(1, 3);
      af.modifier  = 0;
      af.location  = APPLY_NONE;
      af.bitvector = AFF_PARA;
      affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
    }
  }
}

ACMD(do_hayasa)
{

  if (!char_is_npc(ch) && !char_get_skill(ch, SKILL_HAYASA)) {
    send_to_char(ch, "You do not know how to perform this technique!\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_HAYASA)) {
    send_to_char(ch, "You are already focusing ki to continually speed up your movements.\r\n");
    return;
  }

  int skill = char_get_skill(ch, SKILL_HAYASA), prob = axion_dice(0);
  int64_t cost = getMaxKI(ch) / (skill / 2);
  int duration = 1;

  if (skill >= 100) {
    duration = 6;
  } else if (skill >= 80) {
    duration = 5;
  } else if (skill >= 50) {
    duration = 4;
  } else if (skill >= 25) {
    duration = 3;
  } else {
    duration = 2;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }
  if (skill < prob) {
    decCurKI(ch, cost, 0);
    act("@CYou close your eyes for a brief moment and focus your ki around your body as a soft blue glow. The glow disappears though as you fail to maintain the effect...@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C closes $s eyes for a brief moment and a soft blue glow begins to form around $s body. The glow disappears a second later though and $e frowns.@n", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_HAYASA, 1);
    char_set_wait(ch, PULSE_2SEC);
  } else {
    struct affected_type af;

    decCurKI(ch, cost, 0);
    af.type      = SPELL_HAYASA;
    af.duration  = duration;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_HAYASA;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    char_set_speedboost(ch, char_get_speedcalc(ch) * 0.5);
    reveal_hiding(ch, 0);
    act("@CYou close your eyes for a brief moment and focus your ki around your body as a soft blue glow. All your movements are faster now!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C closes $s eyes for a brief moment and a soft blue glow begins to form around $s body. The glow pulsates gently as $e opens his eyes and smiles.@n", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_HAYASA, 1);
    char_set_wait(ch, PULSE_2SEC);
  }
}

ACMD(do_ensnare)
{

  if (!know_skill(ch, SKILL_ENSNARE)) {
    return;
  }

  struct obj_data *weave, *obj = NULL, *next_obj;
  int found = FALSE;

  for (weave = ch->carrying; weave; weave = next_obj) {
    next_obj = weave->next_content;
    if (found == FALSE && valid_silk(weave) && !obj_extra_flagged(weave, ITEM_FORGED)) {
      found = TRUE;
      obj   = weave;
    }
  }

  if (found == FALSE) {
    send_to_char(ch, "You do not have a bundle of silk to ensnare an opponent with!\r\n");
    return;
  } else {
    int prob = char_get_skill(ch, SKILL_ENSNARE), perc = axion_dice(0);
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
    }
    if (char_aff_flagged(vict, AFF_ENSNARED)) {
      send_to_char(ch, "They are already ensnared!\r\n");
      return;
    }
    if (!char_has_arms(vict)) {
      send_to_char(ch, "They don't have arms to ensnare!\r\n");
      return;
    }
    if (prob <= perc) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately you miss and lose the bundle...@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately $e misses and loses the bundle...@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately $e misses and loses the bundle...@n", TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
    } else if (char_aff_flagged(vict, AFF_ZANZOKEN) && !char_aff_flagged(ch, AFF_ZANZOKEN)) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately @c$N@W zanzokens away avoiding it and you lose the bundle...@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately you zanzoken away avoiding it and @C$n@W loses the bundle...@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately @c$N@W zanzokens away avoiding it and @C$n@W loses the bundle...@n", TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
      char_aff_set(vict, AFF_ZANZOKEN, false);
    } else if (char_aff_flagged(vict, AFF_ZANZOKEN) && char_aff_flagged(ch, AFF_ZANZOKEN)) {
      if (char_get_speedi(ch) + rand_number(1, 100) < char_get_speedi(vict) + rand_number(1, 100)) {
        act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! You both zanzoken! Unfortunately @c$N@W manages to avoid it and you lose the bundle...@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! You both zanzoken! Fortunately you manage to avoid it and @C$n@W loses the bundle...@n", TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! They both zanzoken! Fortunately @c$N@W manages to avoid it and @C$n@W loses the bundle...@n", TRUE, ch, 0, vict, TO_NOTVICT);
        extract_obj(obj);
        char_set_wait(ch, PULSE_3SEC);
        improve_skill(ch, SKILL_ENSNARE, 0);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        char_aff_set(ch, AFF_ZANZOKEN, false);
      } else {
        act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! You both zanzoken! Quickly you spin around $M and ensnare $S arms with the silk!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! You both zanzoken! Quickly $e spins around you and ensnares your arms with the silk!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! They both zanzoken! Quickly $e spins around @c$N@W and ensnares $S arms with the silk!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        extract_obj(obj);
        char_aff_set(vict, AFF_ENSNARED, true);
        char_set_wait(ch, PULSE_3SEC);
        improve_skill(ch, SKILL_ENSNARE, 0);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        char_aff_set(ch, AFF_ZANZOKEN, false);
      }
    } else if (char_aff_flagged(ch, AFF_ZANZOKEN) && !char_aff_flagged(vict, AFF_ZANZOKEN)) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! Quickly you zanzoken and spin around $M and ensnare $S arms with the silk!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! Quickly $e zanzokens and spins around you and ensnares your arms with the silk!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! Quickly $e zanzokens and spins around @c$N@W and ensnares $S arms with the silk!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_aff_set(vict, AFF_ENSNARED, true);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
      char_aff_set(ch, AFF_ZANZOKEN, false);
    } else if (char_get_speedi(ch) + rand_number(1, 100) < char_get_speedi(vict) + rand_number(1, 100)) {
      act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately @c$N@W manages to avoid it and you lose the bundle...@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately you manage to avoid it and @C$n@W loses the bundle...@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately @c$N@W manages to avoid it and @C$n@W loses the bundle...@n", TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
    } else {
      act("@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! Quickly you spin around $M and ensnare $S arms with the silk!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! Quickly $e spins around you and ensnares your arms with the silk!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! Quickly $e spins around @c$N@W and ensnares $S arms with the silk!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      extract_obj(obj);
      char_aff_set(vict, AFF_ENSNARED, true);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(ch, SKILL_ENSNARE, 0);
    }
  } /* Main else function */
}

/* Let's an Arlian trade Stamina for either PL or Ki*/
ACMD(do_adrenaline)
{

  if (!char_is_arlian(ch) && (!char_is_bio(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) != 6 && char_get_genome(ch, 1) != 6)))) {
    send_to_char(ch, "You are not an arlian and do not possess this ability\r\n");
    return;
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
      send_to_char(ch, "Syntax: adrenaline (pl or ki) (percent)\r\nExample: adrenaline pl 10\r\n");
      return;
    } else {
      if (atoi(arg2) < 0 || atoi(arg2) > 100) {
        send_to_char(ch, "The percent must be between 1 and 100%s.\r\n", "%");
        return;
      }

      double percent = atoi(arg2) * 0.01;

      if ((getCurST(ch) - (getBasePL(ch) * percent)) < 0) {
        send_to_char(ch, "You do not have enough stamina to trade for adrenaline!\r\n");
        return;
      }

      int64_t trade = getBaseST(ch) * percent;

      if (!strcasecmp(arg, "pl")) {
        act("@GYou focus your mind and begin to overwork your powerful adrenal glands and your wounds begin to heal!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@g$n@G seems to concentrate and $s wounds begin to heal!@n", TRUE, ch, 0, 0, TO_ROOM);

        if (getCurPL(ch) + trade > getEffMaxPL(ch))
          send_to_char(ch, "Some of your stamina was wasted because your powerlevel maxed out.\r\n");
        incCurHealth(ch, trade, true);
        decCurST(ch, trade, 0);

      } else if (!strcasecmp(arg, "ki")) {
        act("@GYou focus your mind and begin to overwork your powerful adrenal glands and you feel your ki replenish!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@g$n@G seems to concentrate and $e appears energized!@n", TRUE, ch, 0, 0, TO_ROOM);

        if ((getCurKI(ch)) + trade > getMaxKI(ch))
          send_to_char(ch, "Some of your stamina was wasted because your ki maxed out.\r\n");
        incCurKI(ch, trade, true);
        decCurST(ch, trade, 0);
      }
    } /* End inner else */

  } /* end main else */
}

/* This allows a player to cover their body in a shield of fire. */
ACMD(do_fireshield)
{

  if (!know_skill(ch, SKILL_FIRESHIELD)) {
    return;
  }

  if (char_aff_flagged(ch, AFF_FIRESHIELD)) {
    send_to_char(ch, "You are already covered in a fireshield!\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SANCTUARY)) {
    send_to_char(ch, "You are covered in a barrier!\r\n");
    return;
  }

  if (rroom_sunken(char_get_in_room(ch))) {
    send_to_char(ch, "There is way too much water here!\r\n");
    return;
  }

  int64_t cost = getMaxKI(ch) * 0.03;

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki!\r\n");
    return;
  }

  int skill = init_skill(ch, SKILL_FIRESHIELD), prob = axion_dice(0);

  if (skill <= prob) {
    act("@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover your body. Yet you screw up and the technique fails!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover $s body. Yet $e seems to screw up and the technique fails!@n", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_FIRESHIELD, 0);
    decCurKI(ch, cost, 0);
    return;
  }
  act("@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover your body. The ki you have gathered pours out of your body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover your entire body in a protective layer!", TRUE, ch, 0, 0, TO_CHAR);
  act("@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover $s body. The ki $e has gathered pours out of $s body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover $s entire body in a protective layer!", TRUE, ch, 0, 0, TO_ROOM);
  improve_skill(ch, SKILL_FIRESHIELD, 0);
  decCurKI(ch, cost, 0);
  char_aff_set(ch, AFF_FIRESHIELD, true);
  return;
}

/* This allows a player to warp from one ocean/sea to another. */
ACMD(do_warppool)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(ch, SKILL_WARP)) {
    return;
  }

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(ch) || char_get_absorbby(ch)) {
    send_to_char(ch, "You are struggling with someone!\r\n");
    return;
  }

  if (char_get_sits(ch)) {
    send_to_char(ch, "You should get up first.\r\n");
    return;
  }

  int perc = char_get_skill(ch, SKILL_WARP);
  int prob = axion_dice(0), cost = getMaxKI(ch) / 20, pass = FALSE;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What planet are you wanting to warp to?\n[ earth | frigid | kanassa | namek | aether ]\r\n");
    return;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }

  if (rroom_get_vnum(char_get_in_room(ch)) >= 4600 && rroom_get_vnum(char_get_in_room(ch)) < 4700) {
    pass = TRUE;
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 795 && rroom_get_vnum(char_get_in_room(ch)) < 1099) {
    pass = TRUE;
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 15100 && rroom_get_vnum(char_get_in_room(ch)) < 15299) {
    pass = TRUE;
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 13155 && rroom_get_vnum(char_get_in_room(ch)) < 13199) {
    pass = TRUE;
  } else if (rroom_flagged(char_get_in_room(ch), ROOM_NAMEK) && rroom_sect(char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
    pass = TRUE;
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 12103 && rroom_get_vnum(char_get_in_room(ch)) < 12289) {
    pass = TRUE;
  }

  if (pass == FALSE) {
    send_to_char(ch, "You must be on or in a sea or ocean for warp pool to work.\r\n");
    return;
  }

  if (!strcasecmp("earth", arg) && rroom_flagged(char_get_in_room(ch), ROOM_EARTH)) {
    send_to_char(ch, "You are already on Earth!\r\n");
    return;
  }
  if (!strcasecmp("frigid", arg) && rroom_flagged(char_get_in_room(ch), ROOM_FRIGID)) {
    send_to_char(ch, "You are already on Frigid!\r\n");
    return;
  }
  if (!strcasecmp("kanassa", arg) && rroom_flagged(char_get_in_room(ch), ROOM_KANASSA)) {
    send_to_char(ch, "You are already on Kanasssa!\r\n");
    return;
  }
  if (!strcasecmp("namek", arg) && rroom_flagged(char_get_in_room(ch), ROOM_NAMEK)) {
    send_to_char(ch, "You are already on Namek!\r\n");
    return;
  }
  if (!strcasecmp("aether", arg) && rroom_flagged(char_get_in_room(ch), ROOM_AETHER)) {
    send_to_char(ch, "You are already on Aether!\r\n");
    return;
  }
  if (!strcasecmp("earth", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, real_room(850));
      act("@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
    }
  } else if (!strcasecmp("frigid", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, real_room(4609));
      act("@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
    }
  } else if (!strcasecmp("namek", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, real_room(10904));
      act("@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
    }
  } else if (!strcasecmp("kanassa", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, real_room(15100));
      act("@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
    }
  } else if (!strcasecmp("aether", arg)) {
    if (prob > perc) {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_WARP, 1);
    } else {
      act("@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_WARP, 1);
      char_from_room(ch);
      char_to_room(ch, real_room(12252));
      act("@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
    }
  } else {
    send_to_char(ch, "That is not an acceptable choice. It must be a planet with a large body of water.\n[ earth | frigid | kanassa | namek | aether ]\r\n");
    return;
  }
}

/* This allows a player to block off the exit of a room */
ACMD(do_obstruct)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(ch, SKILL_HYOGA_KABE)) {
    return;
  }

  if (rroom_flagged(char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "You can not use this in such a peaceful area.\r\n");
    return;
  }

  if (rroom_sect(char_get_in_room(ch)) == SECT_SPACE || rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) {
    send_to_char(ch, "You can not wall off the vastness of space.\r\n");
    return;
  }

  if (rroom_sect(char_get_in_room(ch)) == SECT_FLYING) {
    send_to_char(ch, "You can not create gravity defying glacial walls.\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH];
  int skill = char_get_skill(ch, SKILL_HYOGA_KABE);
  int prob = axion_dice(0), cost = ((getMaxKI(ch) / skill) * 2.5);

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What direction are you wanting to block off?\n[ N | E | S | W | NE | NW | SE | SW | U | D | I | O ]\r\n");
    return;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }

  int dir = -1, dir2 = -1;

  if (!strcasecmp("n", arg) || !strcasecmp("N", arg)) {
    dir  = 0;
    dir2 = 2;
  } else if (!strcasecmp("e", arg) || !strcasecmp("E", arg)) {
    dir  = 1;
    dir2 = 3;
  } else if (!strcasecmp("s", arg) || !strcasecmp("S", arg)) {
    dir  = 2;
    dir2 = 0;
  } else if (!strcasecmp("w", arg) || !strcasecmp("W", arg)) {
    dir  = 3;
    dir2 = 1;
  } else if (!strcasecmp("u", arg) || !strcasecmp("U", arg)) {
    dir  = 4;
    dir2 = 5;
  } else if (!strcasecmp("d", arg) || !strcasecmp("D", arg)) {
    dir  = 5;
    dir2 = 4;
  } else if (!strcasecmp("i", arg) || !strcasecmp("I", arg)) {
    dir  = 10;
    dir2 = 11;
  } else if (!strcasecmp("o", arg) || !strcasecmp("O", arg)) {
    dir  = 11;
    dir2 = 10;
  } else if (!strcasecmp("nw", arg) || !strcasecmp("NW", arg)) {
    dir  = 6;
    dir2 = 8;
  } else if (!strcasecmp("ne", arg) || !strcasecmp("NE", arg)) {
    dir  = 7;
    dir2 = 9;
  } else if (!strcasecmp("se", arg) || !strcasecmp("SE", arg)) {
    dir  = 8;
    dir2 = 6;
  } else if (!strcasecmp("sw", arg) || !strcasecmp("SW", arg)) {
    dir  = 9;
    dir2 = 7;
  } else {
    send_to_char(ch, "That is not an acceptable direction.\n[ N | E | S | W | NE | NW | SE | SW | U | D | I | O ]\r\n");
    return;
  }

  if (!char_get_exit(ch, dir)) {
    send_to_char(ch, "That direction does not exist here.\r\n");
    return;
  }
  if (skill < prob) {
    act("@CYou channel your ki and start to create a wall of water, but lose your concentration and the water promptly disappears.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C channels $s ki and starts to create a wall of water, but loses $s concentration and the water promptly disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, cost, 0);
    improve_skill(ch, SKILL_HYOGA_KABE, 0);
    return;
  }
  struct obj_data *obj;
  int newroom = world[ch->in_room].dir_option[dir]->to_room;

  if (rroom_flagged(newroom, ROOM_PEACEFUL)) {
    send_to_char(ch, "You can not block off a peaceful area.\r\n");
    return;
  }

  for (obj = world[newroom].contents; obj; obj = obj->next_content) {
    if (obj_get_vnum(obj) == 79) {
      if (obj_get_cost(obj) == dir2) {
        if (skill < prob) {
          act("@CYou place your hands on the glacial wall and concentrate. You fail to undo the composition of the wall!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@c$n@C places $s hands on the glacial wall and concentrates. Nothing happens...@n", TRUE, ch, 0, 0, TO_ROOM);
          decCurKI(ch, cost / 2, 0);
        } else {
          act("@CYou place your hands on the glacial wall and concentrate. You unfreeze the wall and evaporate the water effortlessly.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@c$n@C places $s hands on the glacial wall and concentrates. Suddenly the wall melts and then evaporates!@n", TRUE, ch, 0, 0, TO_ROOM);
          decCurKI(ch, cost / 2, 0);
          extract_obj(obj);
        }
        return;
      }
    }
  }

  struct obj_data *obj2, *obj3;

  obj2 = read_object(79, VIRTUAL);
  obj_to_room(obj2, newroom);
  obj3 = read_object(79, VIRTUAL);
  obj_to_room(obj3, char_get_in_room(ch));

  int64_t strength = (((char_get_aff_abils_intel(ch) * skill) * char_get_aff_abils_wis(ch)) * 20) + (getMaxKI(ch) * 0.001);

  if (strength > getEffMaxPL(ch) * 20) {
    strength = getEffMaxPL(ch) + (strength / 20);
  } else if (strength > getEffMaxPL(ch) * 15) {
    strength = getEffMaxPL(ch) + (strength / 15);
  } else if (strength > getEffMaxPL(ch) * 10) {
    strength = getEffMaxPL(ch) + (strength / 10);
  } else if (strength > getEffMaxPL(ch) * 5) {
    strength = getEffMaxPL(ch) + (strength / 5);
  } else if (strength > getEffMaxPL(ch) * 2) {
    strength = getEffMaxPL(ch) + (strength / 2);
  }

  obj_set_cost(obj2, dir2);
  obj_set_weight(obj2, strength);
  obj_set_cost(obj3, dir);
  obj_set_weight(obj3, strength);
  obj_set_fellow_wall(obj2, obj3);
  obj_set_fellow_wall(obj3, obj2);
  act("@CYou concentrate and channel your ki. A wall of water starts to form in such a way to block off the direction of your choice. As the wall becomes complete it freezes solid by your will!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@c$n@C concentrates and channels $s ki. A wall of water starts to form in such a way to block off one of the directions of this area. As the wall becomes complete it freezes solid by @c$n's@C will!@n", TRUE, ch, 0, 0, TO_ROOM);
  send_to_room(newroom, "@cA wall of water forms slowly upward blocking off the %s direction. This wall of water then freezes instantly once it stops growing.@n\r\n", dirs[dir2]);
  improve_skill(ch, SKILL_HYOGA_KABE, 0);
  decCurKI(ch, cost, 0);
  return;
}

/* This allows a player to flood a room. */
ACMD(do_dimizu)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(ch, SKILL_DIMIZU)) {
    return;
  }

  int skill = char_get_skill(ch, SKILL_DIMIZU);
  int prob  = axion_dice(0);

  if (rroom_get_geffect(char_get_in_room(ch)) < 0) {
    act("@CYou concentrate and distabilie the water, separating the hydrogen and oxygen. The gases dissipate quickly.", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C concentrates and the water filling the area seems to shudder. Suddenly the water begins to evaporate as the hydrogen and oxygen are separated.", TRUE, ch, 0, 0, TO_ROOM);
    rroom_set_geffect(char_get_in_room(ch), 0);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  if (rroom_sect(char_get_in_room(ch)) == SECT_UNDERWATER) {
    send_to_char(ch, "The area is already underwater!\r\n");
    return;
  }
  if (rroom_sect(char_get_in_room(ch)) == SECT_SPACE || rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) {
    send_to_char(ch, "You can't flood space!\r\n");
    return;
  }
  if ((getCurKI(ch)) < getMaxKI(ch) / 12) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }
  if (skill < prob) {
    act("@CYou gather your ki and concentrate on creating water from it. Water begins to flow upward around the entire area, but you lose your concentration and it all goes flooding away!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C gathers $s ki and concentrates on creating water from it. Water begins to flow upward around the entire area, but $e loses $s concentration and all the water goes flooding away!@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, getMaxKI(ch) / 12, 0);
    improve_skill(ch, SKILL_DIMIZU, 0);
    return;
  }
  act("@CYou gather your ki and concentrate on creating water from it. Water begins to flow upward around the entire area. You form the water into a perfect cube with barely any ripples in its walls. It will maintain this form for a while.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@c$n@C gathers $s ki and concentrates on creating water from it. Water begins to flow upward around the entire area. @c$n@C forms the water into a perfect cube with barely any ripples in its walls. It appears the water will maintain this form for a while.@n", TRUE, ch, 0, 0, TO_ROOM);
  decCurKI(ch, getMaxKI(ch) / 12, 0);
  rroom_set_geffect(char_get_in_room(ch), -3);
  improve_skill(ch, SKILL_DIMIZU, 0);
  return;
}

/* Allows a player to place a "beacon" on a room they want to return to if
 * they revive from death. */
ACMD(do_beacon)
{

  if (char_is_npc(ch))
    return;

  if (char_aff_flagged(ch, AFF_SPIRIT)) {
    send_to_char(ch, "You are dead. You can not stake out a room to return to upon revival.\r\n");
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) >= 0 && rroom_get_vnum(char_get_in_room(ch)) <= 14) {
    send_to_char(ch, "You can not stake out an immortal room to be revived in.\r\n");
    return;
  }
  send_to_char(ch, "You stake out the room you are in and will return to it if you die and are revived.\r\n");
  char_set_droom(ch, rroom_get_vnum(char_get_in_room(ch)));
  return;
}
