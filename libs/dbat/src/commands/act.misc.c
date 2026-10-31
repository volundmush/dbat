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

#include "dbat/dg_comm.h"
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

/* local functions  */
static void generate_multiform(struct char_data *ch, int count);
static void resolve_song(struct char_data *ch);
static int campfire_cook(int recipe);
static int valid_recipe(struct char_data *ch, int recipe, int type);
static int has_pole(struct char_data *ch);
static void catch_fish(struct char_data *ch, int quality);
static int valid_silk(struct obj_data *obj);

ACMD(do_spiritcontrol)
{

  if (!char_get_skill(ch, SKILL_SPIRITCONTROL)) {
    send_to_char(ch, "You do not know how to perform that technique.\r\n");
    return;
  } else {
    if (char_aff_flagged(ch, AFF_SPIRITCONTROL)) {
      send_to_char(ch, "You have already concentrated and have full control of your spirit.\r\n");
      return;
    } else {
      int64_t cost = getMaxKI(ch) * 0.2;
      if ((getCurST(ch)) < cost) {
        send_to_char(ch, "You need at least 20%s of your max ki in stamina to prepare this skill.\r\n", "%");
        return;
      } else {
        decCurST(ch, cost, 0);
        act("@YYou concentrate and quantify every last bit of your spiritual and mental energies. You have full control of them and can bring them forth in an instant.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@y$n@Y seems to concentrate hard for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);
        int duration = rand_number(2, 4);
        assign_affect(ch, AFF_SPIRITCONTROL, SKILL_SPIRITCONTROL, duration, 0, 0, 0, 0, 0, 0);
      }
    }
  }
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

ACMD(do_restring)
{

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int pay = 0;

  one_argument(argument, arg);

  if (rroom_get_vnum(char_get_in_room(ch)) >= 178 && rroom_get_vnum(char_get_in_room(ch)) <= 184) {
    pay = 5000;
    if (char_get_gold(ch) < pay) {
      send_to_char(ch, "You need at least 5,000 zenni to initiate an equipment restring.\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have a that equipment to restring in your inventory.\r\n");
      send_to_char(ch, "Syntax: restring (obj name)\r\n");
      return;
    } else if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
      send_to_char(ch, "You can not restring a custom piece. Why? Because you already restrung it you dummy.\r\n");
      return;
    } else {
      desc_set_connected(ch->desc, CON_POBJ);
      char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH], thelong[MAX_INPUT_LENGTH];

      *thename  = '\0';
      *theshort = '\0';
      *thelong  = '\0';

      sprintf(thename, "%s", obj->name);
      sprintf(theshort, "%s", obj->short_description);
      sprintf(thelong, "%s", obj->description);

      ch->desc->obj_name   = strdup(thename);
      ch->desc->obj_was    = strdup(theshort);
      ch->desc->obj_short  = strdup(theshort);
      ch->desc->obj_long   = strdup(thelong);
      ch->desc->obj_point  = obj;
      ch->desc->obj_type   = 1;
      ch->desc->obj_weapon = 0;
      disp_restring_menu(ch->desc);
      ch->desc->obj_editflag = EDIT_RESTRING;
      ch->desc->obj_editval  = EDIT_RESTRING_MAIN;
      return;
    }
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
  } else {
    send_to_char(ch, "Huh? Try help multiform\r\n");
    return;
  }
}

static void generate_multiform(struct char_data *ch, int count)
{
  char blamo[MAX_INPUT_LENGTH];
  sprintf(blamo, "p.%s", char_get_name(ch));

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

void handle_songs()
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if (!desc_is_playing(d))
      continue;
    if (!d->character)
      continue;
    if (char_get_powerattack(d->character) > 0) {
      resolve_song(d->character);
    }
  }
}

static void apply_shadow_sitch(struct char_data *ch, struct char_data *vict, int skill)
{
  act("@CYour forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@c$n's@C forboding music has caused YOUR shadows to stitch into YOUR body, slow YOUR actions down!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@c$n's@C forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  decCurKI(ch, getPercentOfMaxKI(ch, .001) + skill, 0);
  if (!char_is_npc(vict)) {
    char_set_wait(vict, PULSE_2SEC);
  } else {
    assign_affect(vict, AFF_SHADOW_STITCH, SKILL_MYSTICMUSIC, -1, 0, 0, 0, 0, 0, -2);
  }
}

static void resolve_song(struct char_data *ch)
{

  struct char_data *vict = NULL, *next_v = NULL;
  struct obj_data *obj2 = NULL, *next_obj;
  int diceroll          = axion_dice(0);
  int skill             = char_get_skill(ch, SKILL_MYSTICMUSIC);
  int instrument        = 0;

  char buf[MAX_INPUT_LENGTH];

  if (char_get_powerattack(ch) <= 0) {
    return;
  }

  for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    if (obj_get_vnum(obj2) == 8802 || obj_get_vnum(obj2) == 8807) {
      instrument = obj_get_vnum(obj2);
    }
  }

  if (instrument == 0) {
    send_to_char(ch, "You do not have an instrument.\r\n");
    act("@c$n@C stops playing $s song.@n", TRUE, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, 0);
    return;
  }

  if (skill > diceroll) {
    sprintf(buf, "@c$n@C continues playing @y'@Y%s@y'@C.@n", char_get_powerattack(ch) == SONG_SAFETY ? "Song of Safety" : (char_get_powerattack(ch) == SONG_SHIELDING ? "Song of Shielding" : (char_get_powerattack(ch) == SONG_SHADOW_STITCH ? "Shadow Stitch Minuet" : "Teleportation Melody")));
    act("@CYou continue playing your song.@n", TRUE, ch, 0, 0, TO_CHAR);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
  } else {
    act("@CYou mess up a portion of the song, but continue playing.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C messes up a portion of $s song, but continues to play.@n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    switch (char_get_powerattack(ch)) {
      case SONG_SAFETY:
        if ((ch->master == vict->master || ch == vict->master || vict == ch->master) || vict == ch) {
          if ((char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) || vict == ch) {
            if (ch == vict->master || ch->master == vict || ch->master == vict->master || vict == ch) {
              if (skill > diceroll) {
                int64_t restore = (10 * skill) + ((getMaxKI(ch) * 0.0004) * skill);
                if (vict != ch) {
                  act("@CYour skillfully playing of the Song of Safety has an effect on @c$N@C.@n", TRUE, ch, 0, vict, TO_CHAR);
                } else {
                  act("@CYour skillfully playing of the Song of Safety has an effect on your own body@C.@n", TRUE, ch, 0, vict, TO_CHAR);
                }
                incCurHealth(vict, restore, true);
                incCurST(vict, restore * .5, true);

                if (char_get_limb_condition(vict, (1) - 1) < 100) {
                  char_set_limb_condition(vict, (1) - 1, char_get_limb_condition(vict, (1) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (1) - 1) > 100) {
                    send_to_char(vict, "Your right arm is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (1) - 1, 100);
                  }
                }
                if (char_get_limb_condition(vict, (2) - 1) < 100) {
                  char_set_limb_condition(vict, (2) - 1, char_get_limb_condition(vict, (2) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (2) - 1) > 100) {
                    send_to_char(vict, "Your left arm is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (2) - 1, 100);
                  }
                }
                if (char_get_limb_condition(vict, (3) - 1) < 100) {
                  char_set_limb_condition(vict, (3) - 1, char_get_limb_condition(vict, (3) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (3) - 1) > 100) {
                    send_to_char(vict, "Your right leg is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (3) - 1, 100);
                  }
                }
                if (char_get_limb_condition(vict, (1) - 1) < 100) {
                  char_set_limb_condition(vict, (4) - 1, char_get_limb_condition(vict, (4) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (4) - 1) > 100) {
                    send_to_char(vict, "Your left leg is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (4) - 1, 100);
                  }
                }
                if (vict != ch) {
                  act("@c$n's@C soothing Song of Safety has recovered some of your powerlevel, stamina, and limb condition.", TRUE, ch, 0, vict, TO_VICT);
                }
                act("@c$n@C continues playing $s ocarina!@n", TRUE, ch, 0, vict, TO_NOTVICT);
                improve_skill(ch, SKILL_MYSTICMUSIC, 2);
                decCurKI(ch, (getMaxKI(ch) * .0003) + skill, 0);
              }
            }
          }
        }
        if ((getCurKI(ch)) <= 0) {
          send_to_char(ch, "You no longer have the ki necessary to play your song.\r\n");
          act("@c$n@C stops playing $s song.@n", TRUE, ch, 0, 0, TO_ROOM);
          char_set_powerattack(ch, 0);
          return;
        }
        break;
      case SONG_SHADOW_STITCH:
        if (ch->master && vict->master) {
          if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
            if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
              continue;
            } else if (skill > diceroll + 10) {
              apply_shadow_sitch(ch, vict, skill);
            }
          } else if (skill > diceroll + 10) {
            apply_shadow_sitch(ch, vict, skill);
          }
        } else if (skill > diceroll + 10) {
          apply_shadow_sitch(ch, vict, skill);
        }
        if ((getCurKI(ch)) <= 0) {
          send_to_char(ch, "You no longer have the ki necessary to play your song.\r\n");
          act("@c$n@C stops playing $s song.@n", TRUE, ch, 0, 0, TO_ROOM);
          char_set_powerattack(ch, 0);
          return;
        }
        break;
      case SONG_TELEPORT_EARTH:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Earth in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Earth in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(300));
            }
          }
        }
        break;
      case SONG_TELEPORT_VEGETA:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Vegeta in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Vegeta in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(2234));
            }
          }
        }
        break;
      case SONG_TELEPORT_FRIGID:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Frigid in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Frigid in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(4047));
            }
          }
        }
        break;
      case SONG_TELEPORT_KONACK:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Konack in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Konack in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(8003));
            }
          }
        }
        break;
      case SONG_TELEPORT_NAMEK:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Namek in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Namek in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(10182));
            }
          }
        }
        break;
      case SONG_TELEPORT_ARLIA:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Arlia in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Arlia in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(16087));
            }
          }
        }
        break;
      case SONG_TELEPORT_AETHER:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Aether in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Aether in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(12025));
            }
          }
        }
        break;
      case SONG_TELEPORT_KANASSA:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Kanassa in a flash!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Kanassa in a flash!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(14910));
            }
          }
        }
        break;
      case SONG_SHIELDING:
        if (vict == ch || (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP))) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master || vict == ch) {
            if (skill > diceroll) {
              if (vict != ch) {
                act("@CYour triumphant and soaring music has powered a barrier around @c$N@C!@n", TRUE, ch, 0, vict, TO_CHAR);
                act("@c$n's@C triumphant and soaring music has powered a barrier around you!@n", TRUE, ch, 0, vict, TO_VICT);
              } else {
                act("@CYour triumphant and soaring music has powered a barrier around yourself@C!@n", TRUE, ch, 0, vict, TO_CHAR);
              }
              act("@c$n's@C triumphant and soaring music has powered a barrier around @c$N@C!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_barrier(vict, char_get_barrier(vict) + (((getMaxKI(ch) * 0.005) * (skill * 0.25)) + skill));
              if (char_get_barrier(vict) >= getMaxKI(vict) * 0.75) {
                char_set_barrier(vict, getMaxKI(vict) * 0.75);
              }
              if (!char_aff_flagged(vict, AFF_SANCTUARY)) {
                char_aff_set(vict, AFF_SANCTUARY, true);
              }
              incCurKI(ch, getPercentOfMaxKI(ch, .02) + skill, true);
            }
          }
        }
        if ((getCurKI(ch)) <= 0) {
          send_to_char(ch, "You no longer have the ki necessary to play your song.\r\n");
          act("@c$n@C stops playing $s song.@n", TRUE, ch, 0, 0, TO_ROOM);
          char_set_powerattack(ch, 0);
          return;
        }
        break;
    }
  }

  if (char_get_powerattack(ch) >= 4 && skill > diceroll) {
    switch (char_get_powerattack(ch)) {
      case SONG_TELEPORT_EARTH:
        char_from_room(ch);
        char_to_room(ch, real_room(300));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Earth and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_VEGETA:
        char_from_room(ch);
        char_to_room(ch, real_room(2234));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Vegeta and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_FRIGID:
        char_from_room(ch);
        char_to_room(ch, real_room(4047));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Frigid and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_NAMEK:
        char_from_room(ch);
        char_to_room(ch, real_room(10182));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Namek and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_KANASSA:
        char_from_room(ch);
        char_to_room(ch, real_room(14910));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Kanassa and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_AETHER:
        char_from_room(ch);
        char_to_room(ch, real_room(12025));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Aether and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_ARLIA:
        char_from_room(ch);
        char_to_room(ch, real_room(16087));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Arlia and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_KONACK:
        char_from_room(ch);
        char_to_room(ch, real_room(8003));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Konack and stop your song.@n", TRUE, ch, 0, 0, TO_CHAR);
        break;
    }
  }
}

ACMD(do_song)
{

  if (!char_get_skill(ch, SKILL_MYSTICMUSIC)) {
    send_to_char(ch, "You do not know how to play mystical music.\r\n");
    return;
  }

  struct obj_data *obj2 = NULL, *next_obj;
  int instrument        = 0;

  for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    if (obj_get_vnum(obj2) == 8802 || obj_get_vnum(obj2) == 8807) {
      instrument = obj_get_vnum(obj2);
    }
  }

  if (instrument == 0) {
    send_to_char(ch, "You do not have an instrument.\r\n");
    return;
  }

  if (char_get_powerattack(ch)) {
    act("@cYou stop playing your ocarina.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n stops playing their ocarina.@n", TRUE, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, 0);
    return;
  } else {

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int skill = char_get_skill(ch, SKILL_MYSTICMUSIC);

    two_arguments(argument, arg, arg2);
    if (!*arg) {
      send_to_char(ch, "@YSongs Known\n@c-------------------@n\r\n");
      send_to_char(ch, "@W%s%s%sSong of Safety\r\n", skill > 99 ? "Song of Shielding\n" : "", skill > 80 ? "Melody of Teleportation\n" : "", skill > 50 ? "Shadow Stitch Minuet\n" : "");
      send_to_char(ch, "@wSyntax: song (shielding | safety | teleport | shadow )\r\n");
      return;
    } else {
      int64_t cost = getMaxKI(ch) * 0.01;
      int modifier = 1;
      if (!strcasecmp(arg, "shielding")) {
        modifier = 20;
        cost *= modifier;
      } else if (!strcasecmp(arg, "teleport")) {
        modifier = 50;
        cost *= modifier;
      } else if (!strcasecmp(arg, "shadow")) {
        modifier = 8;
        cost *= modifier;
      } else if (!strcasecmp(arg, "safety")) {
        modifier = 3;
        cost *= modifier;
      }

      if (instrument == 8802) {
        cost -= cost * 0.5;
      }

      if (modifier == 0) {
        send_to_char(ch, "@wSyntax: song (shielding | safety | teleport | shadow )\r\n");
        return;
      } else if (modifier == 3 && (getCurKI(ch)) < cost) {
        send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
        return;
      } else if (modifier == 3) {
        act("@CYou begin to play the Song of Safety! Your fingers lightly glide over the ocarina and as you blow into it sweet music similar to a lullaby issues forth from the intrument.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@c$n@C begins to play a song on $s ocarina. The music seems to be some sort of lullaby.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_set_powerattack(ch, SONG_SAFETY);
        decCurKI(ch, cost, 0);
        return;
      } else if (modifier == 8 && (getCurKI(ch)) < cost) {
        send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
        return;
      } else if (modifier == 8 && skill <= 49) {
        send_to_char(ch, "You do not posess the skill to play such a song!\r\n");
        return;
      } else if (modifier == 8) {
        act("@CYou begin to play the Shadow Stitch Minuet! Your fingers lightly glide over the ocarina and as you blow into it forboding low toned music issues forth.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@c$n@C begins to play a song on $s ocarina. Depressing low toned music issues forth from the ocarina.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_set_powerattack(ch, SONG_SHADOW_STITCH);
        decCurKI(ch, cost, 0);
        return;
      } else if (modifier == 50 && (getCurKI(ch)) < cost) {
        send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
        return;
      } else if (modifier == 50 && skill <= 79) {
        send_to_char(ch, "You do not posess the skill to play such a song!\r\n");
        return;
      } else if (modifier == 50) {
        if (!*arg2) {
          send_to_char(ch, "Where would you like to teleport to?\nSyntax: song teleport (earth | vegeta | kanassa | arlia | aether | namek | konack| frigid)\r\n");
          return;
        } else if (char_aff_flagged(ch, AFF_SPIRIT)) {
          send_to_char(ch, "Not while you're dead!\r\n");
          return;
        } else if (!strcasecmp(arg2, "earth")) {
          char_set_powerattack(ch, SONG_TELEPORT_EARTH);
        } else if (!strcasecmp(arg2, "frigid")) {
          char_set_powerattack(ch, SONG_TELEPORT_FRIGID);
        } else if (!strcasecmp(arg2, "vegeta")) {
          char_set_powerattack(ch, SONG_TELEPORT_VEGETA);
        } else if (!strcasecmp(arg2, "namek")) {
          char_set_powerattack(ch, SONG_TELEPORT_NAMEK);
        } else if (!strcasecmp(arg2, "arlia")) {
          char_set_powerattack(ch, SONG_TELEPORT_ARLIA);
        } else if (!strcasecmp(arg2, "kanassa")) {
          char_set_powerattack(ch, SONG_TELEPORT_KANASSA);
        } else if (!strcasecmp(arg2, "konack")) {
          char_set_powerattack(ch, SONG_TELEPORT_KONACK);
        } else if (!strcasecmp(arg2, "aether")) {
          char_set_powerattack(ch, SONG_TELEPORT_AETHER);
        } else {
          send_to_char(ch, "Syntax: song teleport (earth | vegeta | namek | aether | konack | kanassa | arlia | frigid)\r\n");
          return;
        }
        act("@CYou begin to play the Melody of Teleportation! Your fingers lightly glide over the ocarina and as you blow into it a repeating light hearted melody issues forth.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@c$n@C begins to play a song on $s ocarina. A light hearted melody can be heard sounding from the ocarina as it is played.@n", TRUE, ch, 0, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
        return;
      } else if (modifier == 20 && (getCurKI(ch)) < cost) {
        send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
        return;
      } else if (modifier == 20 && skill <= 98) {
        send_to_char(ch, "You do not posess the skill to play such a song!\r\n");
        return;
      } else if (modifier == 20) {
        act("@CYou begin to play the Song of Shielding! Your fingers lightly glide over the ocarina and as you blow into it a triumphant series of notes issues forth.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@c$n@C begins to play a song on $s ocarina. A triumphant song full of soaring sounds from the ocarina as it is played.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_set_powerattack(ch, SONG_SHIELDING);
        decCurKI(ch, cost, 0);
        return;
      }
    }
  }
}

ACMD(do_preference)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
    return;
  }

  if (char_get_preference(ch) > 0) {
    send_to_char(ch, "You've already chosen a specialization. No going back.\r\n");
    return;
  }

  if (!strcasecmp(arg, "throw")) {
    send_to_char(ch, "You will now favor throwing weapons as fighting specialization. You're sure to nail it.\r\n");
    char_set_preference(ch, PREFERENCE_THROWING);
    if (char_get_skills_base(ch, SKILL_THROW) <= 90) {
      char_set_skills_base(ch, SKILL_THROW, char_get_skills_base(ch, SKILL_THROW) + (10));
    } else if (char_get_skills_base(ch, SKILL_THROW) < 100) {
      char_set_skills_base(ch, SKILL_THROW, 100);
    }
    return;
  } else if (!strcasecmp(arg, "hand")) {
    send_to_char(ch, "You will now favor your body as your fighting specialization. Your body is ready.\r\n");
    char_set_preference(ch, PREFERENCE_H2H);
    return;
  } else if (!strcasecmp(arg, "ki")) {
    send_to_char(ch, "You will now favor your ki energy as your fighting specialization. I expect more than a few smoldering craters.\r\n");
    char_set_preference(ch, PREFERENCE_KI);
    return;
  } else if (!strcasecmp(arg, "weapon")) {
    send_to_char(ch, "You will now favor your weapons as your fighting specialization. Let the blood fly!\r\n");
    char_set_preference(ch, PREFERENCE_WEAPON);
    return;
  } else {
    send_to_char(ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
    return;
  }
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
  } else if (axion_dice(0) > char_get_aff_abils_con(ch) + rand_number(1, 10)) {
    act("@mYou crouch down and begin to focus on your body's carapace cells encouraging them to multiply! However your control is lacking and you ultimately fail to grow your armor very much.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n@m crouches down and seems to strain for a moment before giving up and resuming $s normal stance.@n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  } else {
    act("@mYou crouch down and begin to focus on your body's carapace cells, encouraging them to multiply! Very quickly millions of new carapace cells have been born and your armored carapace extends over all parts of your body!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n@m crouches down and after a few moments of straining $s body's carapace armor starts to grow thicker and extends to cover all parts of $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurSTPercent(ch, .2, 0);
    char_aff_set(ch, AFF_SHELL, true);
    return;
  }
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
    } else {
      act("@MYour body starts to become loose and sag. It continues to droop down until it begins to run down like a river of goo flowing from where your body was.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag. Much of $s body begins to pour down and scatter around as pools of goo.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
      char_aff_set(ch, AFF_LIQUEFIED, true);
      return;
    }
  } else if (!strcasecmp(arg, "explode")) {
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
    } else if ((getCurKI(ch)) < (getMaxKI(ch) * 0.10) + 150) {
      send_to_char(ch, "You do not have enough ki for that action!@n\r\n");
      return;
    } else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "That target isn't here.\r\n");
      return;
    } else if (!can_kill(ch, vict, NULL, 1)) {
      send_to_char(ch, "You can't kill them!\r\n");
      return;
    } else if (axion_dice(0) > char_get_level(ch)) {
      act("@MYour body starts to become loose and sag, but you lose focus and it reverts to its original shape!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@m$n@M's body starts to become loose and sag, but $e seems to return normal a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
      char_set_wait(ch, PULSE_3SEC);
      return;
    } else if (char_get_speedi(ch) < char_get_speedi(vict)) {
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
    } else if (getCurPL(ch) < getCurPL(vict) * 2) {
      act("@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However as you force yourself in through $S mouth $E successfully resists and forces your back out!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you think quickly and force $m out before $e has a chance to get fully into your body!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However as $e forces $mself in through @R$N's@M mouth $E manages to resist and force @m$n@M back out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, (getMaxKI(ch) * .002) + 150, 0);
      int64_t dmg = getEffMaxPL(ch) * 0.08;
      hurt(0, 0, ch, vict, NULL, dmg, 0);
      char_set_wait(ch, PULSE_3SEC);
      return;
    } else {
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
  } else {
    send_to_char(ch, "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n");
    return;
  }
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
  } else if (setting <= 0) {
    send_to_char(ch, "Your will just isn't in the fight, huh?\nYou will not use up life force to maintain your PL period.\r\n");
    char_set_lifeperc(ch, 0);
    return;
  } else {
    send_to_char(ch, "Your life force will automatically kick in at %d%s of your optimal PL.\r\n", setting, "%");
    char_set_lifeperc(ch, setting);
    return;
  }
}

ACMD(do_defend)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg && char_get_defending(ch) == NULL) {
    send_to_char(ch, "Defend who?\r\n");
    return;
  } else if (!*arg && char_get_defending(ch)) {
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
  } else if (vict == ch) {
    send_to_char(ch, "Well hopefully you are smart enough to defend yourself.\r\n");
    return;
  } else {
    act("@YYou start defending @y$N@Y.@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@y$n@Y starts defending you.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@y$n@Y starts defending @y$N@Y.@n", TRUE, ch, 0, vict, TO_NOTVICT);
    char_set_defender(vict, ch);
    char_set_defending(ch, vict);
    return;
  }
}

ACMD(do_fish)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: fish ( cast | hook | reel | apply | stop)\r\n");
    return;
  }

  if (!strcasecmp(arg, "cast")) {
    if (char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(ch, "You are already fishing! Syntax: fish stop\r\n");
      return;
    } else if (!rroom_flagged(char_get_in_room(ch), ROOM_FISHING)) {
      send_to_char(ch, "This is not an area you can fish at.\r\n");
      return;
    } else if (char_aff_flagged(ch, AFF_FLYING)) {
      send_to_char(ch, "You can't fish while flying.\r\n");
      return;
    } else if (char_get_fighting(ch)) {
      send_to_char(ch, "You can't fish while fighting!\r\n");
      return;
    } else if (!char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You are not holding a fishing pole.\r\n");
      return;
    } else {
      struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
      if (obj_get_type_flag(pole) != ITEM_FISHPOLE) {
        send_to_char(ch, "You do not have a fishing pole in your hand!\r\n");
        return;
      } else if (obj_get_value(pole, 0) == 0) {
        send_to_char(ch, "There is no bait on your line!\r\n");
        return;
      }
      reveal_hiding(ch, 0);
      act("@CYou pull your arm back and then spring it forward, casting the baited line. A moment later there is a splash as the hook enters the water.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C pulls $s arm back and then springs it foward, casting the line of $s fishing pole into the water.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_accuracy_mod(ch, rand_number(30, 80));
      char_plr_set(ch, PLR_FISHING, true);
      send_to_char(ch, "@D[@wDistance@D: @Y%d@D]@n\r\n", char_get_accuracy_mod(ch));
      return;
    }
  } else if (!strcasecmp(arg, "hook")) {
    if (!char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(ch, "You are not even fishing!\r\n");
      return;
    } else if (char_get_fishstate(ch) == FISH_NOFISH) {
      send_to_char(ch, "You do not have a fish biting on your line.\r\n");
      return;
    } else if (char_get_fishstate(ch) == FISH_REELING) {
      send_to_char(ch, "You are already trying to reel the fish in!\r\n");
      return;
    } else if (char_get_fishstate(ch) == FISH_HOOKED) {
      send_to_char(ch, "You already have the fish hooked! Reel it in!\r\n");
      return;
    } else if (axion_dice(-18) > char_get_accuracy(ch)) {
      reveal_hiding(ch, 0);
      act("@CYou pull hard but the fish spits the hook out a second before you pull! You return to waiting for a bite...@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C pulls hard on $s fishing line, but a moment later $e frowns and returns to fishing.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_fishstate(ch, FISH_NOFISH);
      return;
    } else {
      reveal_hiding(ch, 0);
      act("@CYou pull hard on your line and feel that you have managed to hook the fish and start reeling it in!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C pulls hard on $s fishing line and starts to struggle with the fish on the other end!@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_fishstate(ch, FISH_REELING);
      return;
    }
  } else if (!strcasecmp(arg, "apply")) {
    if (!char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You are not holding a fishing pole.\r\n");
      return;
    } else {
      struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
      if (obj_get_type_flag(pole) != ITEM_FISHPOLE) {
        send_to_char(ch, "You do not have a fishing pole in your hand!\r\n");
        return;
      } else if (obj_get_value(pole, 0) != 0) {
        send_to_char(ch, "Your fishing pole already has bait on its hook.\r\n");
        return;
      } else {
        struct obj_data *bait;

        if (!*arg2) {
          send_to_char(ch, "Syntax: fish apply (bait)\r\n");
          return;
        }
        if (!(bait = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
          send_to_char(ch, "You don't have that bait.\r\n");
          return;
        } else if (obj_get_type_flag(bait) != ITEM_FISHBAIT) {
          send_to_char(ch, "That isn't fishing bait!\r\n");
          return;
        } else {
          reveal_hiding(ch, 0);
          act("@CYou carefully apply the $p@C to your hook.@n", TRUE, ch, bait, 0, TO_CHAR);
          act("@c$n@C carefully applies $p@C to $s fishing pole's hook.@n", TRUE, ch, bait, 0, TO_ROOM);
          obj_set_value(pole, 0, obj_get_cost(bait));
          extract_obj(bait);
          return;
        } /* End we applied bait */
      } /* End Applying bait */
    } /* end has pole */
  } else if (!strcasecmp(arg, "stop")) {
    if (!char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(ch, "You are not even fishing!\r\n");
      return;
    } else {
      reveal_hiding(ch, 0);
      act("@CYou reel in your line and stop fishing.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reels in $s fishing line and stops fishing.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_plr_set(ch, PLR_FISHING, false);
      char_set_fishstate(ch, FISH_NOFISH);
      char_set_accuracy_mod(ch, 0);
      return;
    }
  } else {
    send_to_char(ch, "Syntax: fish ( cast | hook | reel | apply | stop )\r\n");
    return;
  }
} /* End fish */

static int has_pole(struct char_data *ch)
{

  if (char_get_equipment(ch, WEAR_WIELD2)) {
    struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
    if (obj_get_type_flag(pole) == ITEM_FISHPOLE) {
      return (TRUE);
    }
  }

  return (FALSE);
}

void fish_update(void)
{

  struct char_data *i, *next_char, *ch = NULL;
  int quality = 0;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if (rroom_flagged(char_get_in_room(i), ROOM_FISHING)) {
      if (char_plr_flagged(i, PLR_FISHING) && has_pole(i) == TRUE) {
        ch = i;
        if (char_get_accuracy_mod(ch) <= 0 && char_get_fishstate(ch) == FISH_REELING) { /* We've caught it */
          if (char_get_accuracy(ch) >= rand_number(60, 100)) {
            quality = rand_number(0, 3) + rand_number(0, 3) + rand_number(0, 3);
          } else if (char_get_accuracy(ch) >= rand_number(45, 60)) {
            quality = rand_number(0, 3) + rand_number(0, 3);
          } else {
            quality = rand_number(0, 3);
          }
          catch_fish(ch, quality);
        } else if (rand_number(1, 5) >= 3) { /* Reeling section */
          if (char_get_fishstate(ch) == FISH_REELING && rand_number(1, 100) <= 80) {
            if (char_get_accuracy(ch) >= 80) {
              char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - (rand_number(6, 10)));
            } else if (char_get_accuracy(ch) >= 40) {
              char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - (rand_number(5, 8)));
            } else {
              char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - (rand_number(1, 4)));
            }
            act("@CYou reel the line on your pole some.@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C reels the line on $s pole slowly.@n", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char(ch, "@D[@wDistance@D: @Y%d@D]@n\r\n", char_get_accuracy_mod(ch) > 0 ? char_get_accuracy_mod(ch) : 0);
          } else if (char_get_fishstate(ch) == FISH_REELING && rand_number(1, 58) <= 55) {
            act("@CYou struggle as the fish fights against your attempts to reel it in!@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C struggles with the fish on the end of $s pole!@n", TRUE, ch, 0, 0, TO_ROOM);
          } else if (char_get_fishstate(ch) == FISH_REELING) { /* Lose the fish */
            act("@CYou feel the line go slack and realize you've lost the fish! You reel your line back in...@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C frowns and then begins to reel in $s line.@n", TRUE, ch, 0, 0, TO_ROOM);
            char_set_accuracy_mod(ch, 0);
            char_set_fishstate(ch, FISH_NOFISH);
            char_plr_set(ch, PLR_FISHING, false);
            if (has_pole(ch) == TRUE) {
              struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
              obj_set_value(pole, 0, 0);
            }
          } else if (char_get_fishstate(ch) == FISH_HOOKED && rand_number(1, 20) >= 12) {
            act("@CYou feel the line go slack and realize you've lost the fish! You reel your line back in...@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C frowns and then begins to reel in $s line.@n", TRUE, ch, 0, 0, TO_ROOM);
            char_set_accuracy_mod(ch, 0);
            char_set_fishstate(ch, FISH_NOFISH);
            char_plr_set(ch, PLR_FISHING, false);
          } else if (char_get_fishstate(ch) == FISH_BITE && rand_number(1, 20) >= 12) {
            act("@CYou feel as if the fish has stopped biting...@n", TRUE, ch, 0, 0, TO_CHAR);
            char_set_fishstate(ch, FISH_NOFISH);
          } else if (char_get_fishstate(ch) != FISH_HOOKED && char_get_fishstate(ch) != FISH_BITE && ((rroom_flagged(char_get_in_room(ch), ROOM_FISHFRESH) && rand_number(1, 10) >= 8) || (!rroom_flagged(char_get_in_room(ch), ROOM_FISHFRESH) && rand_number(1, 20) >= 18))) {
            act("@CYou feel a fish biting on your line! Better @Ghook@C it!@n", TRUE, ch, 0, 0, TO_CHAR);
            char_set_fishstate(ch, FISH_BITE);
          }
        } /* End reel section */
      } else if (char_plr_flagged(i, PLR_FISHING) && has_pole(i) == FALSE) { /* End of, Is Fishing */
        char_plr_set(i, PLR_FISHING, false);
        char_set_accuracy_mod(i, 0);
        char_set_fishstate(i, FISH_NOFISH);
      }
    } else { /* Is not in a fishing room */
      if (char_plr_flagged(i, PLR_FISHING)) {
        char_plr_set(i, PLR_FISHING, false);
        char_set_accuracy_mod(i, 0);
        char_set_fishstate(i, FISH_NOFISH);
      }
    }
  } /* End of for */
}

static void catch_fish(struct char_data *ch, int quality)
{
  struct obj_data *fish = NULL;
  int num               = 1000;

  if (rroom_flagged(char_get_in_room(ch), ROOM_FISHFRESH)) {
    if (rroom_flagged(char_get_in_room(ch), ROOM_EARTH)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1000;
          break;
        case 5:
        case 6:
        case 7:
          num = 1001;
          break;
        case 8:
        case 9:
          num = 1002;
          break;
        case 10:
          num = 1003;
          break;
      }
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_AETHER)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1012;
          break;
        case 5:
        case 6:
        case 7:
          num = 1013;
          break;
        case 8:
        case 9:
          num = 1014;
          break;
        case 10:
          num = 1015;
          break;
      }
    }
  } else {
    if (rroom_flagged(char_get_in_room(ch), ROOM_EARTH)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1004;
          break;
        case 5:
        case 6:
        case 7:
          num = 1005;
          break;
        case 8:
        case 9:
          num = 1006;
          break;
        case 10:
          num = 1007;
          break;
      }
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_NAMEK)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1008;
          break;
        case 5:
        case 6:
        case 7:
          num = 1009;
          break;
        case 8:
        case 9:
          num = 1010;
          break;
        case 10:
          num = 1011;
          break;
      }
    }
  }

  fish = read_object(num, VIRTUAL);

  if (!fish) {
    send_to_imm("Error: Fish success with no fish! Report to Iovan!\r\n");
    return;
  }

  act("@CYou manage to pull a $p@C from the water and onto the ground in front of you!@n", TRUE, ch, fish, 0, TO_CHAR);
  act("@c$n@C manages to pull a $p@C from the water and onto the ground in front of $m!@n", TRUE, ch, fish, 0, TO_ROOM);

  int weight = 1;

  if (quality <= 0 && rand_number(1, 20) >= 17) {
    quality += rand_number(2, 7);
  }

  struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);

  if (obj_get_value(pole, 0) * 2 >= axion_dice(0)) {
    quality += 2;
  } else if (obj_get_value(pole, 0) >= axion_dice(0)) {
    quality += 1;
  }

  switch (quality) {
    case 0:
    case 1:
      weight = rand_number(0, 2);
      break;
    case 2:
    case 3:
      weight = rand_number(3, 4);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 0.20));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + (1));
      break;
    case 4:
    case 5:
    case 6:
      weight = rand_number(5, 9);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 0.5));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + (3));
      break;
    default:
      weight = rand_number(10, 15);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 2));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + (5));
      break;
  }

  obj_set_weight(fish, obj_get_weight(fish) + (weight));

  obj_set_value(pole, 0, 0);
  obj_to_room(fish, char_get_in_room(ch));
  do_get(ch, "fish", 0, 0);
  send_to_char(ch, "@D[@cFish Weight@D: @G%" I64T "@D]@n\r\n", obj_get_weight(fish));
  char_plr_set(ch, PLR_FISHING, false);
  char_set_accuracy_mod(ch, 0);
  char_set_fishstate(ch, FISH_NOFISH);
}

ACMD(do_extract)
{

  if (!char_get_skill(ch, SKILL_EXTRACT)) {
    send_to_char(ch, "You do not know how to extract!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], argu[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL, *obj2 = NULL;
  int skill = char_get_skill(ch, SKILL_EXTRACT), chance = axion_dice(0);

  half_chop(argument, arg, argu);
  two_arguments(argu, arg2, arg3);

  if (!*arg) {
    send_to_char(ch, "Syntax: extract (object)\r\n");
    send_to_char(ch, "Syntax: extract combine (bottle1) (bottle2)\r\n");
    return;
  }

  if (!strcasecmp(arg, "combine")) {
    if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You do not have the first bottle that you were wanting to combine.\r\n");
      return;
    }
    if (!(obj2 = get_obj_in_list_vis(ch, arg3, NULL, ch->carrying))) {
      send_to_char(ch, "You do not have the second bottle that you were wanting to combine.\r\n");
      return;
    }
    if (obj && obj2) {
      if (obj_get_vnum(obj) != 3424) {
        send_to_char(ch, "That is not an ink bottle!\r\n");
        return;
      } else if (obj_get_vnum(obj2) != 3424) {
        send_to_char(ch, "That is not an ink bottle!\r\n");
        return;
      } else if (obj_get_value(obj, 6) <= 0) {
        send_to_char(ch, "There isn't any ink in the first bottle!\r\n");
        return;
      } else if (obj_get_value(obj2, 6) <= 0) {
        send_to_char(ch, "There isn't any ink in the second bottle!\r\n");
        return;
      } else {
        if (obj_get_value(obj, 6) >= obj_get_value(obj2, 6)) {
          obj_set_value(obj, 6, obj_get_value(obj, 6) + (obj_get_value(obj2, 6)));
          if (obj_get_value(obj, 6) > 24) {
            obj_set_value(obj, 6, 24);
          }
          send_to_char(ch, "You combine the ink of the two bottles into one bottle, and discard the leftovers.\r\n");
          extract_obj(obj2);
        } else if (obj_get_value(obj2, 6) > obj_get_value(obj, 6)) {
          obj_set_value(obj2, 6, obj_get_value(obj2, 6) + (obj_get_value(obj, 6)));
          if (obj_get_value(obj2, 6) > 24) {
            obj_set_value(obj2, 6, 24);
          }
          send_to_char(ch, "You combine the ink of the two bottles into one bottle, and discard the leftovers.\r\n");
          extract_obj(obj);
        }
        return;
      }
    }
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You do not have that item.\r\n");
    return;
  } else {
    if (obj_get_vnum(obj) == 3425) {
      if (obj_get_value(obj, VAL_MAXMATURE) != 0 && obj_get_value(obj, VAL_MATURITY) < obj_get_value(obj, VAL_MAXMATURE)) {
        send_to_char(ch, "It's not mature enough to extract from!\r\n");
        return;
      }
      struct obj_data *bottle = NULL, *next_obj, *obj2;
      int found               = FALSE;

      for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
        next_obj = obj2->next_content;
        if (obj_get_vnum(obj2) == 3423) {
          bottle = obj2;
          found  = TRUE;
        }
      }

      int64_t cost = ((getMaxKI(ch) * 0.35) + 500);

      if (found == FALSE) {
        send_to_char(ch, "You do not have an empty bottle to put the extracted ink in.\r\n");
        return;
      }

      int64_t extra = 0;

      if (obj_get_value(bottle, 6) + 4 >= 24)
        extra = getMaxKI(ch) * 0.5;

      cost += extra;

      if ((getCurKI(ch)) < cost) {
        send_to_char(ch, "You do not have enough ki! @D[@rNeeded@D: @R%s@D]@n\r\n", add_commas(cost));
        return;
      } else if (skill < chance) {
        decCurKI(ch, cost, 0);
        act("@WWith your ki flowing carefully into your hands you take a hold of the @G$p@W and begin to strip it of its leaves. Once it has been stripped you go to squeeze the ink carefully from the leaves into the bottle, but unfortunately the ink explodes into a mess instead!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W takes a hold of the @G$p@W and begins to strip it of its leaves. Once it has been stripped $e bundles up the leaves in $s hands and begins to squeeze. A nasty explosion of a mess is all that follows!@n", TRUE, ch, obj, 0, TO_ROOM);
        improve_skill(ch, SKILL_EXTRACT, 0);
        extract_obj(obj);
        char_set_wait(ch, PULSE_3SEC);
        return;
      } else {
        decCurKI(ch, cost, 0);
        act("@WWith your ki flowing carefully into your hands you take a hold of the @G$p@W and begin to strip it of its leaves. Once it has been stripped you go to squeeze the ink carefully from the leaves into the bottle, and manage to get every last drop of ink into it.@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W takes a hold of the @G$p@W and begins to strip it of its leaves. Once it has been stripped $e bundles up the leaves in $s hands and begins to squeeze ink carefully from the leaves into a bottle.@n", TRUE, ch, obj, 0, TO_ROOM);
        extract_obj(obj);
        obj_set_value(bottle, 6, obj_get_value(bottle, 6) + (rand_number(4, 6)));
        if (obj_get_value(bottle, 6) >= 24) {
          struct obj_data *filled = read_object(3424, VIRTUAL);
          extract_obj(bottle);
          obj_set_value(filled, 6, 24);
          obj_to_char(filled, ch);
          decCurKI(ch, 0, 0);
          act("@GAs the last of the ink fills the bottle you infuse a final burst of ki into the bottle.@n", TRUE, ch, filled, 0, TO_CHAR);
          act("@GAs the last of the ink fills the bottle @g$n@G infuses a final burst of ki into the bottle.@n ", TRUE, ch, filled, 0, TO_ROOM);
        } else {
          send_to_char(ch, "You will need to fill the bottle before giving it a final infusion of ki to complete the process.\r\n");
        }
        improve_skill(ch, SKILL_EXTRACT, 0);
        char_set_wait(ch, PULSE_3SEC);
      }
    } else {
      send_to_char(ch, "That is not something you can extract from.\r\n");
      return;
    }
  }
}

ACMD(do_runic)
{

  if (!char_get_skill(ch, SKILL_RUNIC)) {
    send_to_char(ch, "You do not know how to write down runes.\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int skill = char_get_skill(ch, SKILL_RUNIC), bonus = 0;

  two_arguments(argument, arg, arg2);

  if (char_get_fighting(ch)) {
    send_to_char(ch, "You are too busy fighting to write runes!\r\n");
    return;
  }

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: runic (target) (skill)\r\n");
    send_to_char(ch, "@D----@GRunic Skills@D----@n\r\n");
    send_to_char(ch, "@Rkenaz\n%s\n%s\n%s\n%s\n%s\n%s@n\n", skill >= 40 ? "@Galgiz" : "", skill >= 40 ? "@moagaz" : "", skill >= 50 ? "@CLaguz" : "", skill >= 60 ? "@Ywunjo" : "", skill >= 80 ? "@rpurisaz" : "", skill >= 100 ? "@mgebo" : "");
    return;
  }

  struct obj_data *obj, *next_obj, *bottle = NULL;
  int found = FALSE, amount = 0, brush = FALSE;

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj_get_vnum(obj) == 3424) {
      if (obj_get_value(obj, 6) > amount) {
        bottle = obj;
        found  = TRUE;
        amount = obj_get_value(bottle, 6);
      }
    }
  }

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj_get_vnum(obj) == 3427) {
      brush = TRUE;
    }
  }

  if (found == FALSE) {
    send_to_char(ch, "You do not have a bottle with enough ink in it.\r\n");
    return;
  } else if (brush == FALSE) {
    send_to_char(ch, "You do not have a brush!\r\n");
    return;
  }

  int64_t cost = getMaxKI(ch) * 0.05;
  int inkcost  = 0;

  if (char_is_hoshijin(ch))
    bonus = 10;
  else
    inkcost += 2;

  struct char_data *vict;

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "You can't seem to find that person.\r\n");
    return;
  } else if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to write runes.\r\n");
    return;
  } else if (skill + bonus < axion_dice(0) && rand_number(1, 5) == 5) {
    act("@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up destroying the ink bottle!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before a look of panic dons $s face. Just at that moment the bottle of ink explodes! Strange...@n", TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(bottle);
    improve_skill(ch, SKILL_RUNIC, 1);
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (skill + bonus < axion_dice(0)) {
    decCurKI(ch, cost, 0);
    act("@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up evaporating some ink!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before some ink evaporates. Strange...@n", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_RUNIC, 1);
    obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (rand_number(1, 3)));
    if (obj_get_value(bottle, 6) < 0)
      obj_set_value(bottle, 6, 0);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "kenaz") || !strcasecmp(arg2, "Kenaz")) {
    inkcost += 1;
    if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CKenaz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CKenaz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.16;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou can now see in the dark! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_INFRAVISION, SKILL_RUNIC, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CKenaz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CKenaz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CKenaz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.16;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou can now see in the dark! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_INFRAVISION, SKILL_RUNIC, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "algiz") || !strcasecmp(arg2, "Algiz")) {
    inkcost += 2;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CAlgiz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CAlgiz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.05;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now have Ethereal Armor! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_EARMOR, SKILL_PUNCH, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CAlgiz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CAlgiz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CAlgiz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.05;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now have Ethereal Armor! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_EARMOR, SKILL_PUNCH, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "oagaz") || !strcasecmp(arg2, "Oagaz")) {
    inkcost += 3;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@COagaz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@COagaz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.04;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now are protected by Ethereal Chains! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_ECHAINS, SKILL_KNEE, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
  } else if (!strcasecmp(arg2, "laguz") || !strcasecmp(arg2, "Laguz")) {
    inkcost += 4;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CLaguz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CLaguz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.04;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now have water breathing! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_WATERBREATH, SKILL_SBC, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@COagaz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@COagaz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@COagaz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.04;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now are protected by Ethereal Chains! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_ECHAINS, SKILL_KNEE, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "wunjo") || !strcasecmp(arg2, "Wunjo")) {
    inkcost += 4;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CWunjo@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CWunjo@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.08;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou are now blessed with a deeper understanding of things you experience! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_WUNJO, SKILL_SLAM, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CWunjo@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CWunjo@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CWunjo@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.08;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou are now blessed with a deeper understanding of things you experience! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_WUNJO, SKILL_SLAM, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "purisaz") || !strcasecmp(arg2, "Purisaz")) {
    inkcost += 4;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CPurisaz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CPurisaz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.06;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou feel as if your inner energy is more potent! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_POTENT, SKILL_HEELDROP, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CPurisaz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CPurisaz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CPUrisaz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.06;
      send_to_char(vict, "@GYou feel as if your inner energy is more potent! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      if (duration < 1)
        duration = 1;
      assign_affect(vict, AFF_POTENT, SKILL_HEELDROP, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "gebo") || !strcasecmp(arg2, "Gebo")) {
    inkcost += 10;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CGebo@D'@B rune on your skin! The rune flashes out of existence immediately!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on $s skin. The rune flashes out of existence immediately!@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      char_set_practice_points(vict, char_get_practice_points(vict) + (125));
      send_to_char(vict, "@GYou feel like you've just gained a lot of knowledge. Now if only you could apply it. @D[@m+125 PS@D]@n\r\n");
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CGebo@D'@B rune on @b$N's@B skin! The rune flashes out of existence immediately!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on @RYOUR@B skin. The rune flashes out of existence immediately!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on @b$N's@B skin. The rune flashes out of existence immediately!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      char_set_practice_points(vict, char_get_practice_points(vict) + (125));
      send_to_char(vict, "@GYou feel like you've just gained a lot of knowledge. Now if only you could apply it. @D[@m+125 PS@D]@n\r\n");
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else {
    send_to_char(ch, "Syntax: runic (target) (skill)\r\n");
    send_to_char(ch, "@D----@GRunic Skills@D----@n\r\n");
    send_to_char(ch, "@Rkenaz\n%s\n%s\n%s\n%s\n%s@n\n", skill >= 40 ? "@Galgiz" : "", skill >= 40 ? "@moagaz" : "", skill >= 60 ? "@Ywunjo" : "", skill >= 80 ? "@rpurisaz" : "", skill >= 100 ? "@mgebo" : "");
    return;
  }
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
  } else {
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
  } else if (rroom_sunken(char_get_in_room(ch))) {
    send_to_char(ch, "You can not create an ashcloud here, because it is too wet.\r\n");
    return;
  } else if (rroom_sect(char_get_in_room(ch)) == SECT_SPACE) {
    send_to_char(ch, "You can not create an ashcloud in space.\r\n");
    return;
  } else if (char_get_aff_abils_intel(ch) < axion_dice(-10)) {
    reveal_hiding(ch, 0);
    act("@RYou take a handful of ashes, and when you go to blow flames at it you lose focus. The ashes are blown from your hands by your huge gust of breath.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@r$n@R takes a handful of ashes from $s belongings and blows it out of $s hands with a strong gust of air. @YStrange.@n", TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(ash);
    decCurKI(ch, cost, 0);
    return;
  } else {
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
}

ACMD(do_resize)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  two_arguments(argument, arg, arg2);

  if (!char_get_skill(ch, SKILL_BUILD)) {
    send_to_char(ch, "You do not have the skill to resize equipment!\r\n");
    return;
  } else if (char_get_skill(ch, SKILL_BUILD) < 80) {
    send_to_char(ch, "Your build skill must be at least level 80 before you can resize equipment.\r\n");
    return;
  } else {
    if (!*arg || !*arg2) {
      send_to_char(ch, "Syntax: resize (obj) (small | medium)\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that object!\r\n");
      return;
    } else {
      if (!wearable_obj(obj)) {
        send_to_char(ch, "That is not equipment! You can only resize equipment.\r\n");
        return;
      } else {
        if ((getCurST(ch)) < obj_get_weight(obj) + (getMaxST(ch) / 40)) {
          send_to_char(ch, "You do not have enough stamina to resize this object at this time.\r\n");
          return;
        } else if (!strcasecmp(arg2, "small")) {
          if (obj_get_size(obj) == SIZE_SMALL) {
            send_to_char(ch, "The equipment is already small sized.\r\n");
            return;
          } else {
            act("@WYou carefully adjust the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_CHAR);
            act("@C$n@W carefully adjusts the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_ROOM);
            obj_set_size(obj, SIZE_SMALL);
            decCurST(ch, obj_get_weight(obj) + (getMaxST(ch) / 40), 0);
          }
        } else if (!strcasecmp(arg2, "medium")) {
          if (obj_get_size(obj) == SIZE_MEDIUM) {
            send_to_char(ch, "The equipment is already medium sized.\r\n");
            return;
          } else {
            act("@WYou carefully adjust the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_CHAR);
            act("@C$n@W carefully adjusts the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_ROOM);
            obj_set_size(obj, SIZE_MEDIUM);
            decCurST(ch, obj_get_weight(obj) + (getMaxST(ch) / 40), 0);
          }
        } else {
          send_to_char(ch, "Syntax: resize (obj) (small | medium)\r\n");
        }
      }
    }
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
  } else if (char_aff_flagged(vict, AFF_HEALGLOW)) {
    send_to_char(ch, "They already have a healing glow surrounding their body.\r\n");
    return;
  }

  if (char_get_fighting(vict) && vict == ch) {
    send_to_char(ch, "You are too busy fighting!@n\r\n");
    return;
  } else if (char_get_fighting(vict)) {
    send_to_char(ch, "They are too busy fighting!@n\r\n");
    return;
  }

  int64_t cost = getMaxKI(ch) * 0.5;

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki. It requires at least 50%s of your ki in cost.\r\n", "%");
    return;
  } else {
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
  } else {
    act("'@RDark@W...' An explosion of sanguine aura erupts over the surface of your body, your eyes darkening to a bleeding crimson. The flaring glow emanating from your body pronounces the shadows cast, a darkening umbrage that threatens a malicious promise. Fists clench tightly, muscles bulking as you hiss; You complete the transition, relaxing visibly, '...@RMetamorphosis@W'@n", TRUE, ch, 0, 0, TO_CHAR);
    act("'@RDark@W...' An explosion of sanguine aura erupts over the surface of @C$n@W's body, $s eyes darkening to a bleeding crimson. The flaring glow emanating from $s body pronounces the shadows cast, a darkening umbrage that threatens a malicious promise. Fists clench tightly, muscles bulking as $e hisses; $e completes the transition, relaxing visibly, '...@RMetamorphosis@W'@n", TRUE, ch, 0, 0, TO_ROOM);

    int duration = char_get_aff_abils_intel(ch) / 12;
    assign_affect(ch, AFF_METAMORPH, SKILL_METAMORPH, duration, 0, 0, 0, 0, 0, 0);
    incCurHealthPercent(ch, .6, true);
    return;
  }
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
  } else if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(ch, "You are busy piloting a ship!\r\n");
    return;
  } else if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return;
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
    send_to_char(ch, "@rYou are in a pocket dimension!@n\r\n");
    return;
  } else if (!*arg) {
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
      } else {
        send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
        return;
      }
    } else {
      send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
      decCurKI(ch, cost, 0);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
  }

  reveal_hiding(ch, 0);
  char_set_wait(ch, PULSE_2SEC);
  if (tar != NULL) {
    if (tar == ch) {
      send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
      return;
    } else if (char_get_grappling(ch) && char_get_grappling(ch) == tar) {
      send_to_char(ch, "You are already in the same room with them and are grappling with them!\r\n");
      return;
    } else if (char_get_admlevel(tar) > 0 && char_get_admlevel(ch) < 1) {
      send_to_char(ch, "That immortal prevents you from reaching them.\r\n");
      return;
    } else if (rroom_flagged(char_get_in_room(tar), ROOM_NOINSTANT)) {
      send_to_char(ch, "You can not go there as it is a protected area!\r\n");
      return;
    } else if (char_get_grappling(ch) && char_aff_flagged(char_get_grappling(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    } else if (char_get_drag(ch) && char_aff_flagged(char_get_drag(ch), AFF_SPIRIT)) {
      send_to_char(ch, "You can not take the dead with you!\r\n");
      return;
    } else if (char_get_grappled(ch) && char_aff_flagged(char_get_grappled(ch), AFF_SPIRIT)) {
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
    } else {
      send_to_char(ch, "There is too much water here to control!\r\n");
      return;
    }
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
        sprintf(bun, "@BUsing your ki you create a rush of water flooding away toward the @C%s@B!@n", dirs[attempt]);
        sprintf(bunn, "@B$n@B uses $s ki to create a rush of water flooding away toward the @C%s@B!@n", dirs[attempt]);
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

void rpp_feature(struct char_data *ch, const char *arg)
{
  int cost = 0, change = FALSE;

  if (!*arg) {
    send_to_char(ch, "Syntax: rpp 13 (description)\nExample: rpp 13 a large red scar on his face\nDisplayed to others: He has a large red scar on his face.\r\n");
    return;
  }

  if (strlen(arg) > 60) {
    send_to_char(ch, "Please limit it to 60 characters.\r\n");
    return;
  }

  if (!char_get_feature(ch)) {
    cost = 2;
  } else {
    cost   = 2;
    change = TRUE;
  }

  if (cost > char_get_rbank(ch)) {
    send_to_char(ch, "You do not have enough RPP in your Bank for that!\r\n");
    return;
  } else {
    char sex[128], buf8[MAX_INPUT_LENGTH];
    sprintf(sex, "%s", char_get_sex(ch) == SEX_FEMALE ? "She" : char_get_sex(ch) == SEX_MALE ? "He"
                                                                                             : "It");

    char_set_rbank(ch, char_get_rbank(ch) - (cost));
    sprintf(buf8, "...%s has %s.", sex, arg);
    send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
    send_to_char(ch, "You now have the following line underneath you when someone sees you in a room as:\n@C%s@n\r\n", buf8);
    char_set_feature(ch, strdup(buf8));
    if (change == TRUE) {
      send_to_imm("%s has altered their extra description. Make sure the reason is legit! If it is then reimb them 2 RPP.\r\n", char_get_user(ch));
      send_to_char(ch, "The immortals have been notified about this change. It had better have been for a good reason.\r\n");
    }
    mud_log("%s RPP Feature: '%s' Check for rule compliance.", char_get_user(ch), buf8);
    return;
  }
}

ACMD(do_instill)
{

  if (char_is_npc(ch))
    return;

  struct obj_data *obj, *token;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(token = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!obj_extra_flagged(token, ITEM_TOKEN)) {
    send_to_char(ch, "That is not a token.\r\n");
    return;
  }

  if (obj_extra_flagged(token, ITEM_FORGED)) {
    send_to_char(ch, "That token is a forgery!\r\n");
    return;
  }

  if (!wearable_obj(obj)) {
    send_to_char(ch, "You can only instill tokens into equipment.\r\n");
    return;
  }

  if (!obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOT2)) {
    send_to_char(ch, "That piece of equipment does not have any slots.\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "That piece of equipment has already had its token slots filled. This can not be reversed.");
    return;
  } else { /* It has at least one open slot. */
    int stat = 0, raise = 0;
    stat = token->affected[0].location;

    if (obj->affected[0].location != 0 && obj->affected[1].location != 0 && obj->affected[2].location != 0 && obj->affected[3].location != 0 && obj->affected[4].location != 0 && obj->affected[5].location != 0) {
      if (obj->affected[0].location != stat && obj->affected[1].location != stat && obj->affected[2].location != stat && obj->affected[3].location != stat && obj->affected[4].location != stat && obj->affected[5].location != stat) {
        send_to_char(ch, "This already has as many different stats as it can hold.\r\n");
        return;
      }
    }

    act("@GYou instill the token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@g$n@G instills a token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", TRUE, ch, obj, 0, TO_ROOM);
    raise = token->affected[0].modifier;
    extract_obj(token);

    if (obj_extra_flagged(obj, ITEM_SLOT1))
      obj_extra_set(obj, ITEM_SLOTS_FILLED, true);
    else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOT_ONE))
      obj_extra_set(obj, ITEM_SLOT_ONE, true);
    else if (obj_extra_flagged(obj, ITEM_SLOT2) && obj_extra_flagged(obj, ITEM_SLOT_ONE))
      obj_extra_set(obj, ITEM_SLOTS_FILLED, true);

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
    } else if (obj->affected[0].location == 0) { /* It's empty, put it here regardless */
      obj->affected[0].location = stat;
      obj->affected[0].modifier = raise;
    } else if (obj->affected[1].location == 0) { /* It's empty, put it here regardless */
      obj->affected[1].location = stat;
      obj->affected[1].modifier = raise;
    } else if (obj->affected[2].location == 0) { /* It's empty, put it here regardless */
      obj->affected[2].location = stat;
      obj->affected[2].modifier = raise;
    } else if (obj->affected[3].location == 0) { /* It's empty, put it here regardless */
      obj->affected[3].location = stat;
      obj->affected[3].modifier = raise;
    } else if (obj->affected[4].location == 0) { /* It's empty, put it here regardless */
      obj->affected[4].location = stat;
      obj->affected[4].modifier = raise;
    } else if (obj->affected[5].location == 0) { /* It's empty, put it here regardless */
      obj->affected[5].location = stat;
      obj->affected[5].modifier = raise;
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
  } else if (skill < prob) {
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

/* This is the mortal dig command. */
ACMD(do_bury)
{

  if (!char_has_arms(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(ch, "You are busy grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(ch) || char_get_absorbby(ch)) {
    send_to_char(ch, "You are busy struggling with someone!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }

  if (rroom_sect(char_get_in_room(ch)) != SECT_FIELD && rroom_sect(char_get_in_room(ch)) != SECT_HILLS && rroom_sect(char_get_in_room(ch)) != SECT_FOREST && rroom_sect(char_get_in_room(ch)) != SECT_DESERT && rroom_sect(char_get_in_room(ch)) != SECT_MOUNTAIN) {
    send_to_char(ch, "You are not in a room with enough available dirt or sand to dig.\r\n");
    return;
  }

  struct obj_data *obj = NULL, *buried = NULL, *fobj = NULL, *next_obj;

  for (buried = world[char_get_in_room(ch)].contents; buried; buried = next_obj) {
    next_obj = buried->next_content;
    if (obj_extra_flagged(buried, ITEM_BURIED)) {
      fobj = buried;
    }
  }

  if (!strcasecmp(arg, "bury")) {
    if (!*arg2) {
      send_to_char(ch, "Bury what?\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that object to bury.\r\n");
      return;
    } else if (fobj != NULL) {
      send_to_char(ch, "There is already something buried near here.\r\n");
      return;
    } else {
      if (rroom_sect(char_get_in_room(ch)) != SECT_DESERT) {
        act("@yYou start digging in a spot of soft dirt. Once you have an appropriately sized hole you drop @G$p@y in and then cover it.@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@y starts digging in a spot of soft dirt. Once $e has an appropriately sized hole $e drops @G$p@y in and then covers it.@n", TRUE, ch, obj, 0, TO_ROOM);
      } else {
        act("@YYou start digging in a spot of soft sand. Once you have an appropriately sized hole you drop @G$p@Y in and then cover it.@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@Y starts digging in a spot of soft sand. Once $e has an appropriately sized hole $e drops @G$p@Y in and then covers it.@n", TRUE, ch, obj, 0, TO_ROOM);
      }
      obj_from_char(obj);
      obj_to_room(obj, char_get_in_room(ch));
      obj_extra_set(obj, ITEM_BURIED, true);
    }
  } else if (!strcasecmp(arg, "uncover")) {
    if (fobj == NULL) {
      send_to_char(ch, "There is nothing buried here.\r\n");
      return;
    } else {
      if (rroom_sect(char_get_in_room(ch)) != SECT_DESERT) {
        act("@yYou slowly dig and reveal @G$p@y buried in the dirt! You pull it out and set it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_CHAR);
        act("@C$n@y starts digging and shortly reveals @G$p@y buried in the dirt! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_ROOM);
      } else {
        act("@YYou slowly dig and reveal @G$p@Y buried in the sand! You pull it out and set it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_CHAR);
        act("@C$n@Y starts digging and shortly reveals @G$p@Y buried in the sand! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_ROOM);
      }
      obj_extra_set(fobj, ITEM_BURIED, false);
    }
  } else {
    send_to_char(ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }
}

ACMD(do_arena)
{

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_in_arena(ch)) {
    send_to_char(ch, "You are too busy competing to be a spectator.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Syntax: arena (fighter number of participant)\r\n        arena look\r\n        arena scan\r\n        arena stop\r\n");
    return;
  } else if (!strcasecmp(arg, "stop")) {
    send_to_char(ch, "You stop viewing what's going on in the arena.\r\n");
    char_prf_set(ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    return;
  } else if (rroom_get_vnum(char_get_in_room(ch)) != 17875) {
    send_to_char(ch, "You are not close enough to the arena floor to see it.\r\n");
    return;
  } else if (!strcasecmp(arg, "look")) {
    if (!char_prf_flagged(ch, PRF_ARENAWATCH)) {
      send_to_char(ch, "You are not even watching anyone in the arena.\r\n");
      return;
    } else if (arena_watch(ch) != NOWHERE) {
      look_at_room(real_room(arena_watch(ch)), ch, 0);
    }
  } else if (!strcasecmp(arg, "scan")) {
    if (rroom_get_vnum(char_get_in_room(ch)) == 17875) {
      int found = FALSE;
      struct descriptor_data *d;

      send_to_char(ch, "@D---@CFighters in the arena@D---@n\r\n");
      for (d = descriptor_list; d; d = d->next) {
        if (desc_get_connected(d) != CON_PLAYING)
          continue;

        if (char_in_arena(d->character)) {
          char buf[MAX_INPUT_LENGTH];
          sprintf(buf, "@YFighter Number@D: @w%d, $N.@n", char_get_idnum(d->character));
          act(buf, TRUE, ch, 0, d->character, TO_CHAR);
          found = TRUE;
        }
      }

      if (found == FALSE) {
        send_to_char(ch, "@wNone.@n\r\n");
      }
    } else {
      send_to_char(ch, "You are not close enough to see what fighters are in the arena.\r\n");
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
        if (desc_get_connected(d) != CON_PLAYING)
          continue;

        if (char_get_idnum(d->character) == num) {
          if (char_in_arena(d->character)) {
            found = TRUE;
          }
        }
      }

      if (found == TRUE) {
        act("@wYou start watching the action surrounding that particular fighter in the arena.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w starts watching the action in the arena.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_prf_set(ch, PRF_ARENAWATCH, true);
        char_set_arenawatch(ch, num);
      } else {
        send_to_char(ch, "A fighter with such a number was not found in the arena.\r\n");
        return;
      }
    } /* Secondary else end */
  } /* Main Else end */
} /* End of Arena Function */

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
    } else if (char_aff_flagged(vict, AFF_ENSNARED)) {
      send_to_char(ch, "They are already ensnared!\r\n");
      return;
    } else if (!char_has_arms(vict)) {
      send_to_char(ch, "They don't have arms to ensnare!\r\n");
      return;
    } else if (prob <= perc) {
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

/* This determines of an object is a suitable bundle of silk or not */
static int valid_silk(struct obj_data *obj)
{
  int value = 0;

  switch (obj_get_vnum(obj)) {
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

ACMD(do_silk)
{

  if (!know_skill(ch, SKILL_SILK)) {
    return;
  }

  struct obj_data *obj = NULL, *weave = NULL, *next_obj = NULL, *weaved = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);
  int prob = char_get_skill(ch, SKILL_SILK), perc = rand_number(1, 120);

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

    for (weave = ch->carrying; weave; weave = next_obj) {
      next_obj = weave->next_content;
      if (found == FALSE && valid_silk(weave) && !obj_extra_flagged(weave, ITEM_FORGED)) {
        found = TRUE;
        obj   = weave;
      }
    }

    if (found == FALSE) {
      send_to_char(ch, "You do not have an acceptable bundle of silk in your inventory!\r\n");
      return;
    } else {
      if (!strcasecmp(arg2, "head")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16705, VIRTUAL);
          obj_to_room(weaved, char_get_in_room(ch));
          if (obj_get_vnum(obj) == 16708) {
            armor *= 20;
            str    = 4;
            intel  = 4;
            price  = 4;
            olevel = 80;
          } else if (obj_get_vnum(obj) == 16700) {
            armor *= 15;
            str    = 3;
            intel  = 3;
            price  = 4;
            olevel = 75;
          } else if (obj_get_vnum(obj) == 16701) {
            armor *= 10;
            str    = 3;
            intel  = 3;
            price  = 3;
            olevel = 50;
          } else if (obj_get_vnum(obj) == 16702) {
            armor *= 6;
            str    = 2;
            intel  = 2;
            price  = 2;
            olevel = 25;
          } else if (obj_get_vnum(obj) == 16703) {
            armor *= 4;
            str    = 1;
            intel  = 1;
            price  = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          obj_set_cost(weaved, obj_get_cost(weaved) * (price));
          obj_set_value(weaved, 0, olevel);
          weaved->level = olevel;
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n", TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, "headsash", 0, 0);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
        }
      } else if (!strcasecmp(arg2, "wrist")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16706, VIRTUAL);
          obj_to_room(weaved, char_get_in_room(ch));
          if (obj_get_vnum(obj) == 16708) {
            armor *= 20;
            str    = 4;
            intel  = 4;
            price  = 4;
            olevel = 80;
          } else if (obj_get_vnum(obj) == 16700) {
            armor *= 15;
            str    = 3;
            intel  = 3;
            price  = 4;
            olevel = 75;
          } else if (obj_get_vnum(obj) == 16701) {
            armor *= 10;
            str    = 3;
            intel  = 3;
            price  = 3;
            olevel = 50;
          } else if (obj_get_vnum(obj) == 16702) {
            armor *= 6;
            str    = 2;
            intel  = 2;
            price  = 2;
            olevel = 25;
          } else if (obj_get_vnum(obj) == 16703) {
            armor *= 4;
            str    = 1;
            intel  = 1;
            price  = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          obj_set_cost(weaved, obj_get_cost(weaved) * (price));
          obj_set_value(weaved, 0, olevel);
          weaved->level = olevel;
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n", TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, "wristband", 0, 0);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
        }
      } else if (!strcasecmp(arg2, "belt")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16707, VIRTUAL);
          obj_to_room(weaved, char_get_in_room(ch));
          if (obj_get_vnum(obj) == 16708) {
            armor *= 20;
            str    = 4;
            intel  = 4;
            price  = 4;
            olevel = 80;
          } else if (obj_get_vnum(obj) == 16700) {
            armor *= 15;
            str    = 3;
            intel  = 3;
            price  = 4;
            olevel = 75;
          } else if (obj_get_vnum(obj) == 16701) {
            armor *= 10;
            str    = 3;
            intel  = 3;
            price  = 3;
            olevel = 50;
          } else if (obj_get_vnum(obj) == 16702) {
            armor *= 6;
            str    = 2;
            intel  = 2;
            price  = 2;
            olevel = 25;
          } else if (obj_get_vnum(obj) == 16703) {
            armor *= 4;
            str    = 1;
            intel  = 1;
            price  = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          obj_set_cost(weaved, obj_get_cost(weaved) * (price));
          obj_set_value(weaved, 0, olevel);
          weaved->level = olevel;
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n", TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, "belt", 0, 0);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
        }
      } else {
        send_to_char(ch, "Syntax: silk weave (head | wrist | belt)");
        return;
      }
      return;
    } ////

  } else if (!strcasecmp(arg, "bundle")) {
    int64_t cost = ((getMaxKI(ch) * 0.01) * (prob * 0.20)) + (char_get_aff_abils_intel(ch) * char_get_level(ch));

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to weave any bundles of silk.\r\n");
      return;
    } else {
      char_set_wait(ch, PULSE_3SEC);
      int super = FALSE, superoll = rand_number(1, 100);
      if (char_is_kurzak(ch)) {
        if (char_get_skill(ch, SKILL_SILK) >= 100) {
          if (8 > superoll) {
            super = TRUE;
          }
        } else if (char_get_skill(ch, SKILL_SILK) >= 60) {
          if (6 > superoll) {
            super = TRUE;
          }
        } else if (char_get_skill(ch, SKILL_SILK) >= 40) {
          if (3 > superoll) {
            super = TRUE;
          }
        }
      }
      if (super == TRUE) {
        obj = read_object(16708, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@YYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@Y piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        send_to_char(ch, "@YIt's SUPER grand!@n\r\n");
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 100) { /* Second Best Quality */
        obj = read_object(16700, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 90) { /* Great Quality */
        obj = read_object(16701, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 80) { /* Good Quality */
        obj = read_object(16702, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 50) { /* Decent Quality */
        obj = read_object(16703, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc) { /* Bad Quality */
        obj = read_object(16704, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else {
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You end up making a poorly formed puddle of goo...@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a poorly formed puddle of goo...@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
        improve_skill(ch, SKILL_SILK, 1);
      }
    }
  } else {
    send_to_char(ch, "Syntax: silk (weave | bundle)\r\n");
    return;
  }
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

/* This handles displaying the rpp item store to a player. */
void disp_rpp_store(struct char_data *ch)
{

  send_to_char(ch, "@m                        RPP Item Store@n\n");
  send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  send_to_char(ch, "@GItem Name                      @gRPP Cost        @cChoice Number   @yMin Lvl@n\n");
  send_to_char(ch, "@WStardust Equipment Set         @D[@Y20@D]            @D[ @C1@D]            @w50@n\n");
  send_to_char(ch, "@WPlatinum Masamune (Sword Skill)@D[@Y 5@D]            @D[ @C2@D]            @w40@n\n");
  send_to_char(ch, "@WObsidian Dirk (Dagger Skill)   @D[@Y 5@D]            @D[ @C3@D]            @w40@n\n");
  send_to_char(ch, "@WEmerald Javelin (Spear Skill)  @D[@Y 5@D]            @D[ @C4@D]            @w40@n\n");
  send_to_char(ch, "@WIvory Cane (Club Skill)        @D[@Y 5@D]            @D[ @C5@D]            @w40@n\n");
  send_to_char(ch, "@WHyper X65 Cannon (Gun Skill)   @D[@Y 5@D]            @D[ @C6@D]            @w40@n\n");
  send_to_char(ch, "@WJagged Rock (Brawl skill)      @D[@Y 5@D]            @D[ @C7@D]            @w40@n\n");
  send_to_char(ch, "@WKachin Mountain                @D[@Y 8@D]            @D[ @C8@D]@n\n");
  send_to_char(ch, "@WSpar Booster                   @D[@Y15@D]            @D[ @C9@D]@n\n");
  send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");

  send_to_char(ch, "@wSyntax: rpp 12 (choice number)@n\r\n");
}

/* This handles buying an item from the rpp item store. */
void handle_rpp_store(struct char_data *ch, int choice)
{
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

  if (char_get_rp(ch) < cost) { /* They can't afford it. */
    send_to_char(ch, "You do not have enough RPP to afford that option.\r\n");
    return;
  } else { /* They can afford it. */
    switch (choice) {
      case 1:
        if (char_get_carry_weight(ch) + 26 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 13 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 50) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          for (objnum = 1110; objnum < 1120; objnum++) {
            if (objnum <= 1116) {
              obj = read_object(objnum, VIRTUAL);
              obj_to_char(obj, ch);
              obj_set_size(obj, get_size(ch));
              obj = NULL;
            } else {
              obj = read_object(objnum, VIRTUAL);
              obj_to_char(obj, ch);
              obj_set_size(obj, get_size(ch));
              obj = NULL;
              obj = read_object(objnum, VIRTUAL);
              obj_to_char(obj, ch);
              obj_set_size(obj, get_size(ch));
            }
          }
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 2:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1120, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 3:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1121, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 4:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1122, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 5:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", cost);
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1123, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 6:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1124, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 7:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1125, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 8:
        if (char_get_carry_weight(ch) + 10000000 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else {
          obj = read_object(1126, VIRTUAL);
          obj_set_weight(obj, 10000000);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 9:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else {
          obj = read_object(1127, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
    } /* End switch */
  }
}

static int valid_recipe(struct char_data *ch, int recipe, int type)
{
  /* Plant Variables */
  int tomato = -1, cucumber = -1, onion = -1, greenbean = -1, garlic = -1, redpep = -1;
  int potato = -1, carrot = -1, brownmush = -1, lettuce = -1;
  /* Meat Variables  */
  int normmeat = -1, goodmeat = -1, normfish = -1, goodfish = -1, greatfish = -1, bestfish = -1;
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
      potato    = 1;
      tomato    = 1;
      carrot    = 1;
      greenbean = 1;
      onion     = 1;
      break;
    case RECIPE_MEAT_STEW:
      normmeat = 1;
      potato   = 1;
      tomato   = 1;
      garlic   = 1;
      break;
    case RECIPE_ROAST:
      normmeat  = 1;
      potato    = 2;
      garlic    = 1;
      onion     = 1;
      greenbean = 3;
      break;
    case RECIPE_CHILI_SOUP:
      normmeat = 1;
      redpep   = 4;
      tomato   = 2;
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
      rice     = 1;
      normfish = 1;
      break;
    case RECIPE_BREAD:
      flour = 1;
      break;
    case RECIPE_SALAD:
      tomato   = 1;
      cucumber = 1;
      carrot   = 1;
      lettuce  = 1;
      break;
    case RECIPE_APPLEPLUM:
      flour     = 1;
      appleplum = 1;
      break;
    case RECIPE_FBERRY_MUFFIN:
      flour  = 1;
      fberry = 1;
      break;
    case RECIPE_CARAMBOLA_BREAD:
      flour     = 1;
      carambola = 1;
      break;
  }

  if (type == 0) {
    /* Check for ingredients in inventory */
    for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
      next_obj = obj2->next_content;
      switch (obj_get_vnum(obj2)) {
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
    }
  } else { /* We know the ingredients are there, remove and exit. */
    for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
      next_obj = obj2->next_content;
      switch (obj_get_vnum(obj2)) {
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
    }
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
      if (potato == 0 && tomato == 0 && carrot == 0 && greenbean == 0 && onion == 0) {
        pass = TRUE;
      }
      break;
    case RECIPE_MEAT_STEW:
      if (normmeat == 0 && potato == 0 && tomato == 0 && garlic == 0)
        pass = TRUE;
      break;
    case RECIPE_ROAST:
      if (normmeat == 0 && potato == 0 && garlic == 0 && onion == 0 && greenbean == 0)
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
      send_to_char(ch, "@WYou need @m%d@W tomato%s for this recipe.@n\r\n", tomato, tomato > 1 ? "es" : "");
    }
    if (potato > 0) {
      send_to_char(ch, "@WYou need @m%d@W potato%s for this recipe.@n\r\n", potato, potato > 1 ? "es" : "");
    }
    if (onion > 0) {
      send_to_char(ch, "@WYou need @m%d@W onion%s for this recipe.@n\r\n", onion, onion > 1 ? "s" : "");
    }
    if (appleplum > 0) {
      send_to_char(ch, "@WYou need @m%d@W appleplum%s for this recipe.@n\r\n", appleplum, appleplum > 1 ? "s" : "");
    }
    if (fberry > 0) {
      send_to_char(ch, "@WYou need @m%d@W frozen berry%s for this recipe.@n\r\n", fberry, fberry > 1 ? "s" : "");
    }
    if (carambola > 0) {
      send_to_char(ch, "@WYou need @m%d@W carambola%s for this recipe.@n\r\n", carambola, carambola > 1 ? "s" : "");
    }
    if (lettuce > 0) {
      send_to_char(ch, "@WYou need @m%d@W head%s of lettuce for this recipe.@n\r\n", lettuce, lettuce > 1 ? "s" : "");
    }
    if (flour > 0) {
      send_to_char(ch, "@WYou need @m%d@W cup%s of white flour for this recipe.@n\r\n", flour, flour > 1 ? "s" : "");
    }
    if (rice > 0) {
      send_to_char(ch, "@WYou need @m%d@W cup%s of white rice for this recipe.@n\r\n", rice, rice > 1 ? "s" : "");
    }
    if (garlic > 0) {
      send_to_char(ch, "@WYou need @m%d@W garlic clove%s for this recipe.@n\r\n", garlic, garlic > 1 ? "s" : "");
    }
    if (carrot > 0) {
      send_to_char(ch, "@WYou need @m%d@W carrot%s for this recipe.@n\r\n", carrot, carrot > 1 ? "s" : "");
    }
    if (cucumber > 0) {
      send_to_char(ch, "@WYou need @m%d@W cucumber%s for this recipe.@n\r\n", cucumber, cucumber > 1 ? "s" : "");
    }
    if (greenbean > 0) {
      send_to_char(ch, "@WYou need @m%d@W green bean%s for this recipe.@n\r\n", greenbean, greenbean > 1 ? "s" : "");
    }
    if (normmeat > 0) {
      send_to_char(ch, "@WYou need @m%d@W normal raw steak%s for this recipe.@n\r\n", normmeat, normmeat > 1 ? "s" : "");
    }
    if (goodmeat > 0) {
      send_to_char(ch, "@WYou need @m%d@W good raw steak%s for this recipe.@n\r\n", goodmeat, goodmeat > 1 ? "s" : "");
    }
    if (redpep > 0) {
      send_to_char(ch, "@WYou need @m%d@W chili pepper%s for this recipe.@n\r\n", redpep, redpep > 1 ? "s" : "");
    }
    if (normfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W black bass, flounder, narri, or gravel reboi for this recipe.@n\r\n", normfish);
    }
    if (goodfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W silver trout, silver eel, valbish, or voos pike for this recipe.@n\r\n", goodfish);
    }
    if (greatfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W striped bass, cobia, gusblat, or shadowfish for this recipe.@n\r\n", greatfish);
    }
    if (bestfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W blue catfish, tambor, repeeil, or shadeeel for this recipe.@n\r\n", bestfish);
    }
    if (brownmush > 0) {
      send_to_char(ch, "@WYou need @m%d@W brown mushroom%s for this recipe.@n\r\n", brownmush, brownmush > 1 ? "s" : "");
    }
    return (FALSE);
  } else {
    return (TRUE);
  }
}

static int campfire_cook(int recipe)
{

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

ACMD(do_cook)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  int cook_elem = cook_element(char_get_in_room(ch));

  if (!cook_elem) {
    send_to_char(ch, "You need a campfire or Flambus Stove nearby to cook.\r\n");
    return;
  }

  if (!char_get_skill(ch, SKILL_COOKING)) {
    send_to_char(ch, "You don't even know the basics!\r\n");
    return;
  }

  int skill = char_get_skill(ch, SKILL_COOKING), prob = axion_dice(0);

  if (!*arg) {
    send_to_char(ch, "@D---------------------@RCooking@D---------------------@n\r\n");
    send_to_char(ch, "@Y 1@B) @CCooked Steak		@Y17@B) @CCarambola Bread@n\r\n");
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
        act("@wYou screw up the preparation of the recipe and end up wasting the ingredients!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w starts to prepare some food, but ends up ruining the ingredients instead!@n", TRUE, ch, 0, 0, TO_ROOM);
        improve_skill(ch, SKILL_COOKING, 0);
        char_set_wait(ch, PULSE_2SEC);
        return;
      } /* End failed to cook it right */

      int psbonus = 0, expbonus = 0;

      switch (num) {
        case 1:
          meal = read_object(MEAL_STEAK, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 5;
          break;
        case 2:
          meal = read_object(MEAL_TOMATO_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 15;
          break;
        case 3:
          meal = read_object(MEAL_POTATO_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 20;
          break;
        case 4:
          meal = read_object(MEAL_VEGETABLE_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 3;
          expbonus = 45;
          break;
        case 5:
          meal = read_object(MEAL_MEAT_STEW, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 50;
          break;
        case 6:
          meal = read_object(MEAL_CHILI_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 0;
          expbonus = 100;
          break;
        case 7:
          meal = read_object(MEAL_NORM_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 12;
          break;
        case 8:
          meal = read_object(MEAL_GOOD_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 3;
          expbonus = 40;
          break;
        case 9:
          meal = read_object(MEAL_GREAT_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 5;
          expbonus = 80;
          break;
        case 10:
          meal = read_object(MEAL_BEST_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 7;
          expbonus = 125;
          break;
        case 11:
          meal = read_object(MEAL_COOKED_RICE, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 8;
          break;
        case 12:
          meal = read_object(MEAL_SUSHI, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 20;
          break;
        case 13:
          meal = read_object(MEAL_BREAD, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 8;
          break;
        case 14:
          meal = read_object(MEAL_SALAD, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 5;
          expbonus = 8;
          break;
        case 15:
          meal = read_object(MEAL_APPLEPLUM, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 9;
          break;
        case 16:
          meal = read_object(MEAL_FBERRY_MUFFIN, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 3;
          expbonus = 12;
          break;
        case 17:
          meal = read_object(MEAL_CARAMBOLA_BREAD, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 9;
          break;
        default:
          send_to_char(ch, "That is not a valid dish!\r\n");
          return;
      }
      if (char_get_bonuses(ch, BONUS_RECIPE)) {
        psbonus += 1;
        expbonus += 3;
      }
      act("@wYou carefully prepare the ingredients and then start cooking them. After a while of patience  and skillful care you successfully make @D'@C$p@D'@w!@n", TRUE, ch, meal, 0, TO_CHAR);
      act("@C$n@w carefully prepares some ingredients and starts cooking them. After a while of patience and skillful care $e succeeds in making @D'@C$p@D'@w!@n", TRUE, ch, meal, 0, TO_ROOM);
      improve_skill(ch, SKILL_COOKING, 0);

      if (psbonus > 0) {
        if (skill * 0.10 > 0)
          psbonus = (skill * 0.10) * psbonus;
      }
      if (expbonus > 0) {
        expbonus = skill * expbonus;
      }

      obj_set_value(meal, VAL_FOOD_PSBONUS, psbonus);
      obj_set_value(meal, VAL_FOOD_EXPBONUS, expbonus);

      char_set_wait(ch, PULSE_2SEC);
    } /* End has ingredients */
  }
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
  } else {
    act("@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover your body. The ki you have gathered pours out of your body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover your entire body in a protective layer!", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover $s body. The ki $e has gathered pours out of $s body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover $s entire body in a protective layer!", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_FIRESHIELD, 0);
    decCurKI(ch, cost, 0);
    char_aff_set(ch, AFF_FIRESHIELD, true);
    return;
  }
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
  } else if (!strcasecmp("frigid", arg) && rroom_flagged(char_get_in_room(ch), ROOM_FRIGID)) {
    send_to_char(ch, "You are already on Frigid!\r\n");
    return;
  } else if (!strcasecmp("kanassa", arg) && rroom_flagged(char_get_in_room(ch), ROOM_KANASSA)) {
    send_to_char(ch, "You are already on Kanasssa!\r\n");
    return;
  } else if (!strcasecmp("namek", arg) && rroom_flagged(char_get_in_room(ch), ROOM_NAMEK)) {
    send_to_char(ch, "You are already on Namek!\r\n");
    return;
  } else if (!strcasecmp("aether", arg) && rroom_flagged(char_get_in_room(ch), ROOM_AETHER)) {
    send_to_char(ch, "You are already on Aether!\r\n");
    return;
  } else if (!strcasecmp("earth", arg)) {
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
  } else if (skill < prob) {
    act("@CYou channel your ki and start to create a wall of water, but lose your concentration and the water promptly disappears.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C channels $s ki and starts to create a wall of water, but loses $s concentration and the water promptly disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, cost, 0);
    improve_skill(ch, SKILL_HYOGA_KABE, 0);
    return;
  } else {
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
  } else if (rroom_sect(char_get_in_room(ch)) == SECT_UNDERWATER) {
    send_to_char(ch, "The area is already underwater!\r\n");
    return;
  } else if (rroom_sect(char_get_in_room(ch)) == SECT_SPACE || rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) {
    send_to_char(ch, "You can't flood space!\r\n");
    return;
  } else if ((getCurKI(ch)) < getMaxKI(ch) / 12) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  } else if (skill < prob) {
    act("@CYou gather your ki and concentrate on creating water from it. Water begins to flow upward around the entire area, but you lose your concentration and it all goes flooding away!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C gathers $s ki and concentrates on creating water from it. Water begins to flow upward around the entire area, but $e loses $s concentration and all the water goes flooding away!@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, getMaxKI(ch) / 12, 0);
    improve_skill(ch, SKILL_DIMIZU, 0);
    return;
  } else {
    act("@CYou gather your ki and concentrate on creating water from it. Water begins to flow upward around the entire area. You form the water into a perfect cube with barely any ripples in its walls. It will maintain this form for a while.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@C gathers $s ki and concentrates on creating water from it. Water begins to flow upward around the entire area. @c$n@C forms the water into a perfect cube with barely any ripples in its walls. It appears the water will maintain this form for a while.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, getMaxKI(ch) / 12, 0);
    rroom_set_geffect(char_get_in_room(ch), -3);
    improve_skill(ch, SKILL_DIMIZU, 0);
    return;
  }
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
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 0 && rroom_get_vnum(char_get_in_room(ch)) <= 14) {
    send_to_char(ch, "You can not stake out an immortal room to be revived in.\r\n");
    return;
  } else {
    send_to_char(ch, "You stake out the room you are in and will return to it if you die and are revived.\r\n");
    char_set_droom(ch, rroom_get_vnum(char_get_in_room(ch)));
    return;
  }
}

/* Feed senzu to someone you are grouped with. Why? TEAMWORK! */
ACMD(do_feed)
{

  if (char_is_npc(ch))
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

  if (char_is_android(vict)) {
    send_to_char(ch, "They are unaffected by senzu beans.\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
    send_to_char(ch, "You need to give them a senzu.\r\n");
    return;
  }

  if (obj_get_type_flag(obj) != ITEM_POTION) {
    send_to_char(ch, "You can only feed senzu beans.\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_FORGED)) {
    send_to_char(ch, "They can't swallow that, it is fake!\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(ch, "They can't swallow that, it is broken!\r\n");
    return;
  }

  if (char_get_fighting(vict)) {
    send_to_char(ch, "They are a bit busy at the moment!\r\n");
    return;
  }

  if (vict->master != ch && ch->master != vict && ch->master != vict->master) {
    send_to_char(ch, "You need to be grouped with them first.\r\n");
    return;
  }

  if (!char_aff_flagged(vict, AFF_GROUP) || !char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(ch, "You need to be grouped with them first.\r\n");
    return;
  }

  act("@WYou take $p@W and pop it into @C$N@W's mouth!@n", TRUE, ch, obj, vict, TO_CHAR);
  act("@C$n@W takes $p@W and pops it into YOUR mouth!@n", TRUE, ch, obj, vict, TO_VICT);
  act("@C$n@W takes $p@W and pops it into @c$N@W's mouth!@n", TRUE, ch, obj, vict, TO_NOTVICT);
  mag_objectmagic(vict, obj, "");
}

/* This allows players to decapitate a corpse for a sick trophy. */
ACMD(do_spoil)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int type = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What corpse do you want to decapitate?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[char_get_in_room(ch)].contents))) {
    send_to_char(ch, "No corpse around here by that name.\r\n");
    return;
  }

  if (obj_get_value(obj, VAL_CORPSE_HEAD) == 0) {
    send_to_char(ch, "That corpse is already missing its head.\r\n");
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      type = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      type = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      type = 1;
    }
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      type = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      type = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      type = 2;
    }
  }

  if (type == 0) {
    act("@C$n@W reaches down and @rtears@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    act("@WYou reach down and @rtear@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
  } else if (type == 1) {
    act("@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    act("@WYou reach down and @rcut@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
  } else if (type == 2) {
    act("@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    act("@WYou reach down and @rcut@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
  }

  obj_set_value(obj, VAL_CORPSE_HEAD, 0);

  struct obj_data *body_part;
  char part[1000];
  char buf[1000];
  char buf2[1000];
  char buf3[1000];

  *part = '\0';
  *buf  = '\0';
  *buf2 = '\0';
  *buf3 = '\0';

  body_part              = create_obj();
  body_part->item_number = NOTHING;
  obj_set_in_room(body_part, NOWHERE);
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
  snprintf(buf2, sizeof(buf2), "@wThe bloody head of %s@w is lying here@n", part);
  snprintf(buf3, sizeof(buf3), "@wThe bloody head of %s@w@n", part);

  body_part->name              = strdup(buf);
  body_part->description       = strdup(buf2);
  body_part->short_description = strdup(buf3);

  obj_set_type_flag(body_part, ITEM_OTHER);
  obj_wear_set(body_part, ITEM_WEAR_TAKE, true);
  obj_extra_set(body_part, ITEM_UNIQUE_SAVE, true);
  obj_set_value(body_part, 0, 0);
  obj_set_value(body_part, 1, 0);
  obj_set_value(body_part, 2, 0);
  obj_set_value(body_part, 3, 0);
  obj_set_value(body_part, 4, 1);
  obj_set_value(body_part, 5, 1);
  obj_set_weight(body_part, rand_number(4, 10));
  obj_set_cost_per_day(body_part, 0);
  add_unique_id(body_part);
  obj_to_room(body_part, char_get_in_room(ch));
  obj_from_room(body_part);
  obj_to_char(body_part, ch);
}
