/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/spec_procs.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/spells.h"
#include "dbat/mail.h"
#include "dbat/act.movement.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/guild.h"
#include "dbat/races.h"
#include "dbat/act.comm.h"
#include "dbat/class.h"
#include "dbat/affect.h"
#include "dbat/act.informative.h"

/* local functions */

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[char_get_in_room(ch)].contents; k; k = world[char_get_in_room(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  for (k = world[char_get_in_room(ch)].contents; k; k = world[char_get_in_room(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, obj_get_cost(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (char_get_level(ch) < 3)
      gain_exp(ch, value);
    else
      char_set_gold(ch, char_get_gold(ch) + (value));
  }
  return (TRUE);
}

/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

int num_players_in_room(room_vnum room)
{
  struct descriptor_data *i;
  int num_players = 0;

  for (i = descriptor_list; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING)
      continue;
    if (!(i->character))
      continue;
    if (char_get_in_room(i->character) == NOWHERE || char_get_in_room(i->character) > top_of_world)
      continue;
    if (world[char_get_in_room(i->character)].number != room)
      continue;
    if ((char_get_admlevel(i->character) >= ADMLVL_IMMORT) && (char_prf_flagged(i->character, PRF_NOHASSLE))) /* Ignore Imms */
      continue;

    num_players++;
  }

  return num_players;
}

bool check_mob_in_room(mob_vnum mob, room_vnum room)
{
  struct char_data *i;
  bool found = FALSE;

  for (i = character_list; i; i = i->next)
    if (char_get_mob_vnum(i) == mob)
      if (world[i->in_room].number == room) found = TRUE;

  return found;
}

void npc_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (char_is_npc(victim))
    return;
  if (char_is_npc(ch))
    return;
  if (char_adm_flagged(victim, ADM_NOSTEAL))
    return;
  if (!char_can_see(ch, victim))
    return;

  if (char_is_awake(victim) && (rand_number(0, char_get_level(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal zenni from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (char_get_gold(victim) * rand_number(1, 10)) / 100;
    if (gold > 0) {
      char_set_gold(ch, char_get_gold(ch) + (gold));
      char_set_gold(victim, char_get_gold(victim) - (gold));
    }
  }
}

/*
 * Quite lethal to low-level characters.
 */

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

#define PET_PRICE(pet) (char_get_level(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = char_get_in_room(ch) + 1;

  if (CMD_IS("list")) {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!char_is_npc(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), char_get_name(pet));
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, NULL, pet_room)) || !char_is_npc(pet)) {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (char_get_gold(ch) < PET_PRICE(pet)) {
      send_to_char(ch, "You don't have enough zenni!\r\n");
      return (TRUE);
    }
    char_set_gold(ch, char_get_gold(ch) - (PET_PRICE(pet)));

    pet = read_mobile(char_get_nr(pet), REAL);
    char_set_exp(pet, 0);
    char_aff_set(pet, AFF_CHARM, true);

    if (*pet_name) {
      snprintf(buf, sizeof(buf), "%s %s", pet->name, pet_name);
      /* free(pet->name); don't free the prototype! */
      pet->name = strdup(buf);

      snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
               pet->description, pet_name);
      /* free(pet->description); don't free the prototype! */
      pet->description = strdup(buf);
    }
    char_to_room(pet, char_get_in_room(ch));
    add_follower(pet, ch);
    pet->master_id = char_get_idnum(ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    char_set_carry_weight(pet, 1000);
    char_set_carry_items(pet, 100);

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

SPECIAL(auction)
{
  room_rnum auct_room;
  struct obj_data *obj, *next_obj, *obj2 = NULL;
  int found = FALSE;

  /* Gross. */
  auct_room = real_room(80);

  if (CMD_IS("cancel")) {

    for (obj = world[auct_room].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (obj && obj_get_aucter(obj) == char_get_id(ch)) {
        obj2  = obj;
        found = TRUE;

        if (obj_get_curBidder(obj2) != -1 && obj_get_aucTime(obj2) + 518400 > time(0)) {
          send_to_char(ch, "Unable to cancel. Someone has already bid on it and their bid license hasn't expired.\r\n");
          time_t remain = (obj_get_aucTime(obj2) + 518400) - time(0);
          int day       = (int)((remain % 604800) / 86400);
          int hour      = (int)((remain % 86400) / 3600);
          int minu      = (int)((remain % 3600) / 60);
          send_to_char(ch, "Time Till License Expiration: %d day%s, %d hour%s, %d minute%s.\r\n", day, day > 1 ? "s" : "", hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
          continue;
        }

        send_to_char(ch, "@wYou cancel the auction of %s@w and it is returned to you.@n\r\n", obj2->short_description);
        struct descriptor_data *d;

        for (d = descriptor_list; d; d = d->next) {
          if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
            continue;
          if (d->character == ch)
            continue;
          if (char_get_equipment(d->character, WEAR_EYE)) {
            send_to_char(d->character, "@RScouter Auction News@D: @GThe auction of @w%s@G has been canceled.\r\n", obj2->short_description);
          }
        }

        obj_from_room(obj2);
        obj_to_char(obj2, ch);
        auc_save();
      }
    }

    if (found == FALSE) {
      send_to_char(ch, "There are no items being auctioned by you.\r\n");
    }

    return (TRUE);
  } else if (CMD_IS("pickup")) {
    struct descriptor_data *d;
    int founded = FALSE;

    for (obj = world[auct_room].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (obj && obj_get_curBidder(obj) == char_get_id(ch)) {
        obj2  = obj;
        found = TRUE;

        if (obj_get_aucter(obj) <= 0) {
          continue;
        }

        if (obj_get_bid(obj2) > char_get_gold(ch)) {
          send_to_char(ch, "Unable to purchase %s, you don't have enough money on hand.\r\n", obj2->short_description);
          continue;
        }

        if (obj_get_aucTime(obj2) + 86400 > time(0)) {
          time_t remain = (obj_get_aucTime(obj2) + 86400) - time(0);
          int hour      = (int)((remain % 86400) / 3600);
          int minu      = (int)((remain % 3600) / 60);
          send_to_char(ch, "Unable to purchase %s, minimum time to bid is 24 hours. %d hour%s and %d minute%s remain.\r\n", obj2->short_description, hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
          continue;
        }

        char_set_gold(ch, char_get_gold(ch) - (obj_get_bid(obj2)));
        obj_from_room(obj2);
        obj_to_char(obj2, ch);
        send_to_char(ch, "You pay %s zenni and receive the item.\r\n", add_commas(obj_get_bid(obj2)));
        auc_save();

        for (d = descriptor_list; d; d = d->next) {
          if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
            continue;
          if (d->character == ch)
            continue;
          if (char_get_idnum(d->character) == obj_get_aucter(obj2)) {
            founded = TRUE;
            char_set_bank_gold(d->character, char_get_bank_gold(d->character) + (obj_get_bid(obj2)));
            if (char_get_equipment(d->character, WEAR_EYE)) {
              send_to_char(d->character, "@RScouter Auction News@D: @GSomeone has purchased your @w%s@G and you had the money put in your bank account.\r\n", obj2->short_description);
            }
          } else if (char_get_equipment(d->character, WEAR_EYE)) {
            send_to_char(d->character, "@RScouter Auction News@D: @GSomeone has purchased the @w%s@G that was on auction.\r\n", obj2->short_description);
          }
        }

        if (founded == FALSE) {
          struct char_data *vict = NULL;
          int is_file = FALSE, player_i = 0;

          CREATE(vict, struct char_data, 1);
          clear_char(vict);
          CREATE(vict->player_specials, struct player_special_data, 1);
          char blam[50];
          sprintf(blam, "%s", obj_get_auctname(obj2));
          if ((player_i = load_char(blam, vict)) > -1) {
            is_file = TRUE;
          } else {
            free_char(vict);
            continue;
          }
          char_set_bank_gold(vict, char_get_bank_gold(vict) + (obj_get_bid(obj2)));

          char_set_pfilepos(vict, player_i);
          save_char(vict);
          if (is_file == TRUE)
            free_char(vict);
        }
      }
    }

    if (found == FALSE) {
      send_to_char(ch, "There are no items that you have bid on.\r\n");
    }
    return (TRUE);
  } else if (CMD_IS("auction")) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct descriptor_data *d;
    int value = 0;

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
      send_to_char(ch, "Auction what item and for how much?\r\n");
      return (TRUE);
    }

    value = atoi(arg2);

    if (!(obj2 = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that item to auction.\r\n");
      return (TRUE);
    }
    if (value <= 999) {
      send_to_char(ch, "Do not auction anything for less than 1,000 zenni.\r\n");
      return (TRUE);
    }

    if (obj_extra_flagged(obj2, ITEM_BROKEN)) {
      act("$P is broken and we will not accept it.", FALSE, ch, 0, obj2, TO_CHAR);
      return (TRUE);
    }

    if (obj_extra_flagged(obj2, ITEM_NODONATE)) {
      act("$P is junk and we will not accept it.", FALSE, ch, 0, obj2, TO_CHAR);
      return (TRUE);
    }

    obj_set_bid(obj2, value);
    obj_set_startbid(obj2, 0);
    obj_set_aucter(obj2, 0);
    if (obj_get_auctname(obj2))
      free(obj_get_auctname(obj2));
    obj_set_aucTime(obj2, 0);

    obj_set_bid(obj2, value);
    obj_set_startbid(obj2, obj_get_bid(obj2));
    obj_set_aucter(obj2, char_get_id(ch));
    obj_set_auctname(obj2, strdup(char_get_name(ch)));
    obj_set_aucTime(obj2, time(0));
    obj_set_curBidder(obj2, -1);
    obj_from_char(obj2);
    obj_to_room(obj2, auct_room);
    auc_save();
    send_to_char(ch, "You place %s on auction for %s zenni.\r\n", obj2->short_description, add_commas(obj_get_bid(obj2)));
    mud_log("AUCTION: %s places %s on auction for %s", char_get_name(ch), obj2->short_description, add_commas(obj_get_bid(obj2)));

    for (d = descriptor_list; d; d = d->next) {
      if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
        continue;
      if (d->character == ch)
        continue;
      if (char_get_equipment(d->character, WEAR_EYE)) {
        send_to_char(d->character, "@RScouter Auction News@D: @GThe item, @w%s@G, has been placed on auction for @Y%s@G zenni.@n\r\n", obj2->short_description, add_commas(obj_get_bid(obj2)));
      }
    }
    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

SPECIAL(healtank)
{
  struct obj_data *htank = NULL, *i;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  for (i = world[char_get_in_room(ch)].contents; i; i = i->next_content) {
    if (obj_get_vnum(i) == 65) {
      htank = i;
    } else {
      continue;
    }
  }

  if (CMD_IS("htank")) {
    if (!htank) {
      return (FALSE);
    }

    if (!*arg) {
      send_to_char(ch, "@WHealing Tank Commands:\r\n"
                       "htank [ enter | exit | check ]@n");
      return (TRUE);
    }

    if (!strcasecmp("enter", arg)) {
      if (char_plr_flagged(ch, PLR_HEALT)) {
        send_to_char(ch, "You are already inside a healing tank!\r\n");
        return (TRUE);
      }
      if (ch->master && ch->master != ch) {
        send_to_char(ch, "You can't enter it while following someone!\r\n");
        return (TRUE);
      } else if (char_is_android(ch)) {
        send_to_char(ch, "A healing tank will have no effect on you.\r\n");
        return (TRUE);
      } else if (obj_get_healcharge(htank) <= 0) {
        send_to_char(ch, "That healing tank needs to recharge, wait a while.\r\n");
        return (TRUE);
      } else if (obj_extra_flagged(htank, ITEM_BROKEN)) {
        send_to_char(ch, "It is broken! You will need to fix it yourself or wait for someone else to fix it.\r\n");
        return (TRUE);
      } else if (char_get_sits(ch)) {
        send_to_char(ch, "You are already on something.\r\n");
        return (TRUE);
      } else if (obj_get_sitting(htank)) {
        send_to_char(ch, "Someone else is already inside that healing tank!\r\n");
        return (TRUE);
      } else {
        char_set_charge(ch, 0);
        char_plr_set(ch, PLR_CHARGE, false);
        char_set_chargeto(ch, 0);
        char_set_barrier(ch, 0);
        act("@wYou step inside the healing tank and put on its breathing mask. A water like solution pours over your body until the tank is full.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w steps inside the healing tank and puts on its breathing mask. A water like solution pours over $s body until the tank is full.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_plr_set(ch, PLR_HEALT, true);
        char_set_sits(ch, htank);
        obj_set_sitting(htank, ch);
        return (TRUE);
      }

    } // End of Enter argument

    else if (!strcasecmp("exit", arg)) {
      if (!char_plr_flagged(ch, PLR_HEALT)) {
        send_to_char(ch, "You are not inside a healing tank.\r\n");
        return (TRUE);
      } else {
        act("@wThe healing tank drains and you exit it shortly after.", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w exits the healing tank after letting it drain.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_plr_set(ch, PLR_HEALT, false);
        obj_set_sitting(htank, NULL);
        char_set_sits(ch, NULL);
        return (TRUE);
      }
    } // End of Exit argument

    else if (!strcasecmp("check", arg)) {
      if (obj_get_healcharge(htank) < 20 && obj_get_healcharge(htank) > 0) {
        send_to_char(ch, "The healing tank has %d bars of energy displayed on its meter.\r\n", obj_get_healcharge(htank));
      } else if (obj_get_healcharge(htank) <= 0) {
        send_to_char(ch, "The healing tank has no energy displayed on its meter.\r\n");
      } else {
        send_to_char(ch, "The healing tank has full energy shown on its meter.\r\n");
      }
      return (TRUE);
    }

    else {
      send_to_char(ch, "@WHealing Tank Commands:\r\n"
                       "htank [ enter | exit | check ]@n");
      return (TRUE);
    }

  } // End of htank command
  else {
    return (FALSE);
  }
}

/* This controls stat augmenter functions */
SPECIAL(augmenter)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (CMD_IS("augment")) {
    int strength = ch->real_abils.str;
    int intel    = ch->real_abils.intel;
    int wisdom   = ch->real_abils.wis;
    int speed    = ch->real_abils.cha;
    int consti   = ch->real_abils.con;
    int agility  = ch->real_abils.dex;

    int strcost = strength * 1200;
    int intcost = intel * 1200;
    int concost = consti * 1200;
    int wiscost = wisdom * 1200;
    int agicost = agility * 1200;
    int specost = speed * 1200;

    if (!*arg) {
      send_to_char(ch, "@D                        -----@WBody Augmentations@D-----@n\r\n");
      send_to_char(ch, "@RStrength    @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", strength, add_commas(strcost));
      send_to_char(ch, "@BIntelligence@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", intel, add_commas(intcost));
      send_to_char(ch, "@CWisdom      @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", wisdom, add_commas(wiscost));
      send_to_char(ch, "@GConstitution@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", consti, add_commas(concost));
      send_to_char(ch, "@mAgility     @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", agility, add_commas(agicost));
      send_to_char(ch, "@YSpeed       @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", speed, add_commas(specost));
      send_to_char(ch, "\r\n");
      return (TRUE);
    } else if (!strcasecmp("strength", arg) || !strcasecmp("str", arg)) {
      if (strength >= 100)
        send_to_char(ch, "Your strength is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < strcost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        ch->real_abils.str += 1;
        char_set_gold(ch, char_get_gold(ch) - (strcost));
        save_char(ch);
      }
    } else if (!strcasecmp("intelligence", arg) || !strcasecmp("int", arg)) {
      if (intel >= 100)
        send_to_char(ch, "Your intelligence is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < intcost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        ch->real_abils.intel += 1;
        char_set_gold(ch, char_get_gold(ch) - (intcost));
        save_char(ch);
      }
    } else if (!strcasecmp("constitution", arg) || !strcasecmp("con", arg)) {
      if (consti >= 100)
        send_to_char(ch, "Your constitution is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < concost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        ch->real_abils.con += 1;
        char_set_gold(ch, char_get_gold(ch) - (concost));
        save_char(ch);
      }
    } else if (!strcasecmp("speed", arg) || !strcasecmp("spe", arg)) {
      if (speed >= 100)
        send_to_char(ch, "Your speed is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < specost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        ch->real_abils.cha += 1;
        char_set_gold(ch, char_get_gold(ch) - (specost));
        save_char(ch);
      }
    } else if (!strcasecmp("agility", arg) || !strcasecmp("agi", arg)) {
      if (agility >= 100)
        send_to_char(ch, "Your agility is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < agicost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        ch->real_abils.dex += 1;
        char_set_gold(ch, char_get_gold(ch) - (agicost));
        save_char(ch);
      }
    } else if (!strcasecmp("wisdom", arg) || !strcasecmp("wis", arg)) {
      if (wisdom >= 100)
        send_to_char(ch, "Your wisdom how somehow been measured is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < wiscost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        ch->real_abils.wis += 1;
        char_set_gold(ch, char_get_gold(ch) - (wiscost));
        save_char(ch);
      }
    } else {
      send_to_char(ch, "Syntax: augment [str | con | int | wis | agi | spe]\r\n");
    }
    return (TRUE);
  } else { /* They are not using the right command, ignore them. */
    return (FALSE);
  }
}

/* This controls gravity generator functions */
SPECIAL(gravity)
{
  struct obj_data *i, *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  int match = FALSE;

  one_argument(argument, arg);
  for (i = world[char_get_in_room(ch)].contents; i; i = i->next_content) {
    if (obj_get_vnum(i) == 11) {
      obj = i;
    } else {
      continue;
    }
  }
  if (CMD_IS("gravity") || CMD_IS("generator")) {
    if (!*arg) {
      send_to_char(ch, "@WGravity Commands:@n\r\n");
      send_to_char(ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
                       "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
      return (TRUE);
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It's broken!\r\n");
      return (TRUE);
    }
    if ((!strcasecmp("N", arg) || !strcasecmp("n", arg) || !strcasecmp("0", arg)) && obj_get_weight(obj) == 0) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("N", arg) || !strcasecmp("n", arg) || !strcasecmp("0", arg)) {
      send_to_char(ch, "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_VEGETA) || rroom_flagged(char_get_in_room(ch), ROOM_GRAVITYX10)) {
        rroom_set_gravity(char_get_in_room(ch), 10);
        obj_set_weight(obj, 0);
      } else {
        rroom_set_gravity(char_get_in_room(ch), 0);
        obj_set_weight(obj, 0);
      }
      match = TRUE;
    }
    if (!strcasecmp("10", arg) && obj_get_weight(obj) == 10) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("10", arg) && rroom_get_gravity(char_get_in_room(ch)) == 10 && (rroom_flagged(char_get_in_room(ch), ROOM_VEGETA) || rroom_flagged(char_get_in_room(ch), ROOM_GRAVITYX10))) {
      send_to_char(ch, "The gravity around you is already at that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("10", arg) && rroom_get_gravity(char_get_in_room(ch)) != 10 && (rroom_flagged(char_get_in_room(ch), ROOM_VEGETA) || rroom_flagged(char_get_in_room(ch), ROOM_GRAVITYX10))) {
      send_to_char(ch, "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 10);
      obj_set_weight(obj, 0);
      match = TRUE;
    } else if (!strcasecmp("10", arg)) {
      send_to_char(ch, "You punch in ten times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      rroom_set_gravity(char_get_in_room(ch), 10);
      obj_set_weight(obj, 10);
      match = TRUE;
    }
    if (!strcasecmp("20", arg) && obj_get_weight(obj) == 20) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("20", arg)) {
      send_to_char(ch, "You punch in twenty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 20);
      obj_set_weight(obj, 20);
      match = TRUE;
    }
    if (!strcasecmp("30", arg) && obj_get_weight(obj) == 30) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("30", arg)) {
      send_to_char(ch, "You punch in thirty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 30);
      obj_set_weight(obj, 30);
      match = TRUE;
    }
    if (!strcasecmp("40", arg) && obj_get_weight(obj) == 40) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("40", arg)) {
      send_to_char(ch, "You punch in fourty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 40);
      obj_set_weight(obj, 40);
      match = TRUE;
    }
    if (!strcasecmp("50", arg) && obj_get_weight(obj) == 50) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("50", arg)) {
      send_to_char(ch, "You punch in fifty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 50);
      obj_set_weight(obj, 50);
      match = TRUE;
    }
    if (!strcasecmp("100", arg) && obj_get_weight(obj) == 100) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("100", arg)) {
      send_to_char(ch, "You punch in one hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 100);
      obj_set_weight(obj, 100);
      match = TRUE;
    }
    if (!strcasecmp("200", arg) && obj_get_weight(obj) == 200) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("200", arg)) {
      send_to_char(ch, "You punch in two hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 200);
      obj_set_weight(obj, 200);
      match = TRUE;
    }
    if (!strcasecmp("300", arg) && obj_get_weight(obj) == 300) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("300", arg)) {
      send_to_char(ch, "You punch in three hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 300);
      obj_set_weight(obj, 300);
      match = TRUE;
    }
    if (!strcasecmp("400", arg) && obj_get_weight(obj) == 400) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("400", arg)) {
      send_to_char(ch, "You punch in four hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 400);
      obj_set_weight(obj, 400);
      match = TRUE;
    }
    if (!strcasecmp("500", arg) && obj_get_weight(obj) == 500) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("500", arg)) {
      send_to_char(ch, "You punch in five hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 500);
      obj_set_weight(obj, 500);
      match = TRUE;
    }
    if (!strcasecmp("1000", arg) && obj_get_weight(obj) == 1000) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("1000", arg)) {
      send_to_char(ch, "You punch in one thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 1000);
      obj_set_weight(obj, 1000);
      match = TRUE;
    }
    if (!strcasecmp("5000", arg) && obj_get_weight(obj) == 5000) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("5000", arg)) {
      send_to_char(ch, "You punch in five thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 5000);
      obj_set_weight(obj, 5000);
      match = TRUE;
    }
    if (!strcasecmp("10000", arg) && obj_get_weight(obj) == 10000) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("10000", arg)) {
      send_to_char(ch, "You punch in ten thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", TRUE, ch, 0, 0, TO_ROOM);
      if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(char_get_in_room(ch), 10000);
      obj_set_weight(obj, 10000);
      match = TRUE;
    } else if (match == FALSE) {
      send_to_char(ch, "That is not a proper command for this device.\r\n");
      send_to_char(ch, "@WGravity Commands:@n\r\n");
      send_to_char(ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
                       "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
      return (TRUE);
    }
    return (TRUE);
  } else {
    return (FALSE);
  }
}

SPECIAL(bank)
{
  int amount, num = 0;

  struct obj_data *i, *obj = NULL;

  for (i = world[char_get_in_room(ch)].contents; i; i = i->next_content) {
    if (obj_get_vnum(i) == 3034) {
      obj = i;
    } else {
      continue;
    }
  }

  if (CMD_IS("balance")) {
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if (char_get_bank_gold(ch) > 0)
      send_to_char(ch, "Your current balance is %d zenni.\r\n", char_get_bank_gold(ch));
    else
      send_to_char(ch, "You currently have no money deposited.\r\n");
    return (TRUE);
  } else if (CMD_IS("wire")) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct char_data *vict = NULL;

    two_arguments(argument, arg, arg2);

    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if ((amount = atoi(arg)) <= 0) {
      send_to_char(ch, "How much do you want to transfer?\r\n");
      return (TRUE);
    }
    if (char_get_bank_gold(ch) < amount + (amount / 100)) {
      send_to_char(ch, "You don't have that much zenni in the bank (plus 1%s charge)!\r\n", "%");
      return (TRUE);
    }
    if (!*arg2) {
      send_to_char(ch, "You want to transfer it to who?!\r\n");
      return (TRUE);
    }
    if (!(vict = get_player_vis(ch, arg2, NULL, FIND_CHAR_WORLD))) {
      int is_file = FALSE, player_i = 0;
      char name[MAX_INPUT_LENGTH];

      CREATE(vict, struct char_data, 1);
      clear_char(vict);
      CREATE(vict->player_specials, struct player_special_data, 1);

      sprintf(name, "%s", rIntro(ch, arg2));

      if ((player_i = load_char(name, vict)) > -1) {
        is_file = TRUE;
      } else {
        free_char(vict);
        send_to_char(ch, "That person doesn't exist.\r\n");
        return (TRUE);
      }
      if (ch->desc->user == NULL) {
        send_to_char(ch, "There is an error. Report to Iovan.");
        return (TRUE);
      }
      if (!strcasecmp(char_get_name(vict), ch->desc->tmp1) || !strcasecmp(char_get_name(vict), ch->desc->tmp2) || !strcasecmp(char_get_name(vict), ch->desc->tmp3) || !strcasecmp(char_get_name(vict), ch->desc->tmp4) || !strcasecmp(char_get_name(vict), ch->desc->tmp5)) {
        send_to_char(ch, "You can not transfer money to your own offline characters...");
        if (is_file == TRUE)
          free_char(vict);
        return (TRUE);
      }
      char_set_bank_gold(vict, char_get_bank_gold(vict) + (amount));
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + (amount / 100)));
      char_set_pfilepos(vict, player_i);
      mudlog(LOG_NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "EXCHANGE: %s gave %s zenni to user %s", char_get_name(ch), add_commas(amount), char_get_name(vict));
      save_char(vict);
      if (is_file == TRUE)
        free_char(vict);
    } else {
      char_set_bank_gold(vict, char_get_bank_gold(vict) + (amount));
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + (amount / 100)));
      send_to_char(vict, "@WYou have just had @Y%s@W zenni wired into your bank account.@n\r\n", add_commas(amount));
    }
    send_to_char(ch, "You transfer %s zenni to them.\r\n", add_commas(amount));
    act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("deposit")) {

    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (char_get_gold(ch) < amount) {
      send_to_char(ch, "You don't have that much zenni!\r\n");
      return (TRUE);
    }
    char_set_gold(ch, char_get_gold(ch) - (amount));
    char_set_bank_gold(ch, char_get_bank_gold(ch) + (amount));
    send_to_char(ch, "You deposit %d zenni.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("withdraw")) {

    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (char_get_bank_gold(ch) < amount) {
      send_to_char(ch, "You don't have that much zenni!\r\n");
      return (TRUE);
    }
    if (char_get_bank_gold(ch) - (amount + (1 + amount / 100)) < 0) {
      if (amount >= 100) {
        amount = amount + (amount / 100);
      } else if (amount < 100) {
        amount = amount + 1;
      }
      send_to_char(ch, "You need at least %s in the bank with the 1 percent withdraw fee.\r\n", add_commas(amount));
      return (TRUE);
    }
    if (char_get_gold(ch) + amount > char_gold_carry(ch)) {
      send_to_char(ch, "You can only carry %s zenni, you left the rest.\r\n", add_commas(char_gold_carry(ch)));
      int diff = (char_get_gold(ch) + amount) - char_gold_carry(ch);
      char_set_gold(ch, char_gold_carry(ch));
      amount -= diff;
      if (amount >= 100) {
        num = amount / 100;
        char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + num));
      } else if (amount < 100) {
        char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + 1));
      }
      send_to_char(ch, "You withdraw %s zenni,  and pay %s in withdraw fees.\r\n.\r\n", add_commas(amount), add_commas(num));
      act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
      return (TRUE);
    }
    char_set_gold(ch, char_get_gold(ch) + (amount));
    if (amount >= 100) {
      num = amount / 100;
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + num));
    } else if (amount < 100) {
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + 1));
    }
    send_to_char(ch, "You withdraw %s zenni, and pay %s in withdraw fees.\r\n", add_commas(amount), add_commas(num));
    act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
    return (TRUE);
  } else
    return (FALSE);
}
