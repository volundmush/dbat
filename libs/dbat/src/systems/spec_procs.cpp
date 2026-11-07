/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/spec_procs.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/spells.hpp"
#include "dbat/mail.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/guild.hpp"
#include "dbat/races.hpp"
#include "dbat/act.comm.hpp"
#include "dbat/class.hpp"
#include "dbat/affect.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions */

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = rroom_get_contents(g, char_get_in_room(ch)); k; k = rroom_get_contents(g, char_get_in_room(ch))) {
    act(g, "$p vanishes in a puff of smoke!", false, nullptr, k, nullptr, TO_ROOM);
    extract_obj(g, k);
  }

  if (!CMD_IS(g, "drop"))
    return (false);

  do_drop(g, ch, argument, cmd, SCMD_DROP);

  for (k = rroom_get_contents(g, char_get_in_room(ch)); k; k = rroom_get_contents(g, char_get_in_room(ch))) {
    act(g, "$p vanishes in a puff of smoke!", false, nullptr, k, nullptr, TO_ROOM);
    value += MAX(1, MIN(50, obj_get_cost(k) / 10));
    extract_obj(g, k);
  }

  if (value) {
    send_to_char(g, ch, "You are awarded for outstanding performance.\r\n");
    act(g, "$n has been awarded for being a good citizen.", true, ch, nullptr, nullptr, TO_ROOM);

    if (char_get_level(ch) < 3)
      gain_exp(g, ch, value);
    else
      char_set_gold(ch, char_get_gold(ch) + (value));
  }
  return (true);
}

/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

int num_players_in_room(GameWorld *g, room_vnum room)
{
  int num_players = 0;

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING)
      continue;
    if (!(i->character))
      continue;
    if (char_get_in_room(i->character) == NOWHERE || char_get_in_room(i->character) > g->top_room)
      continue;
    if (rroom_get_vnum(g, char_get_in_room(i->character)) != room)
      continue;
    if ((char_get_admlevel(i->character) >= ADMLVL_IMMORT) && (char_prf_flagged(g, i->character, PRF_NOHASSLE))) /* Ignore Imms */
      continue;

    num_players++;
  }

  return num_players;
}

bool check_mob_in_room(GameWorld *g, mob_vnum mob, room_vnum room)
{
  bool found = false;

  for (char_data *i : each_char(g))
    if (char_get_mob_vnum(g, i) == mob)
      if (rroom_get_vnum(g, char_get_in_room(i)) == room) found = true;

  return found;
}

void npc_steal(GameWorld *g, struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (char_is_npc(victim))
    return;
  if (char_is_npc(ch))
    return;
  if (char_adm_flagged(victim, ADM_NOSTEAL))
    return;
  if (!char_can_see(g, ch, victim))
    return;

  if (char_is_awake(victim) && (rand_number(g, 0, char_get_level(ch)) == 0)) {
    act(g, "You discover that $n has $s hands in your wallet.", false, ch, nullptr, victim, TO_VICT);
    act(g, "$n tries to steal zenni from $N.", true, ch, nullptr, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (char_get_gold(victim) * rand_number(g, 1, 10)) / 100;
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
  char buf[MAX_STRING_LENGTH], pet_name[MAX_INPUT_LENGTH];
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = char_get_in_room(ch) + 1;

  if (CMD_IS(g, "list")) {
    send_to_char(g, ch, "Available pets are:\r\n");
    for (char_data *tch : room_people(g, pet_room)) {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!char_is_npc(tch))
        continue;
      send_to_char(g, ch, "%8d - %s\r\n", PET_PRICE(tch), char_get_name(tch));
    }
    return (true);
  }
  if (CMD_IS(g, "buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(g, buf, nullptr, pet_room)) || !char_is_npc(pet)) {
      send_to_char(g, ch, "There is no such pet!\r\n");
      return (true);
    }
    if (char_get_gold(ch) < PET_PRICE(pet)) {
      send_to_char(g, ch, "You don't have enough zenni!\r\n");
      return (true);
    }
    char_set_gold(ch, char_get_gold(ch) - (PET_PRICE(pet)));

    pet = read_mobile(g, char_get_nr(pet), REAL);
    char_set_exp(pet, 0);
    char_aff_set(pet, AFF_CHARM, true);

    if (*pet_name) {
      snprintf(buf, sizeof(buf), "%s %s", char_get_alias(pet), pet_name);
      /* free(pet->name); don't free the prototype! */
      char_set_alias(pet, strdup(buf));

      snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
               char_get_ddesc(pet), pet_name);
      /* free(pet->description); don't free the prototype! */
      char_set_ddesc(pet, strdup(buf));
    }
    char_to_room(g, pet, char_get_in_room(ch));
    add_follower(g, pet, ch);
    char_set_master_id(pet, char_get_idnum(ch));

    /* Be certain that pets can't get/carry/use/wield/wear items */
    char_set_carry_weight(pet, 1000);
    char_set_carry_items(pet, 100);

    send_to_char(g, ch, "May you enjoy your pet.\r\n");
    act(g, "$n buys $N as a pet.", false, ch, nullptr, pet, TO_ROOM);

    return (true);
  }

  /* All commands except list and buy */
  return (false);
}

SPECIAL(auction)
{
  room_rnum auct_room;
  struct obj_data *obj2 = nullptr;
  int found = false;

  /* Gross. */
  auct_room = real_room(g, 80);

  if (CMD_IS(g, "cancel")) {

    for (obj_data *obj : room_contents(g, auct_room)) {
      if (obj && obj_get_aucter(obj) == char_get_id(ch)) {
        obj2  = obj;
        found = true;

        if (obj_get_curBidder(obj2) != -1 && obj_get_aucTime(obj2) + 518400 > game_now(g)) {
          send_to_char(g, ch, "Unable to cancel. Someone has already bid on it and their bid license hasn't expired.\r\n");
          time_t remain = (obj_get_aucTime(obj2) + 518400) - game_now(g);
          int day       = (int)((remain % 604800) / 86400);
          int hour      = (int)((remain % 86400) / 3600);
          int minu      = (int)((remain % 3600) / 60);
          send_to_char(g, ch, "Time Till License Expiration: %d day%s, %d hour%s, %d minute%s.\r\n", day, day > 1 ? "s" : "", hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
          continue;
        }

        send_to_char(g, ch, "@wYou cancel the auction of %s@w and it is returned to you.@n\r\n", obj_get_short_description(obj2));

        for (descriptor_data *d : each_desc(g)) {
          if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
            continue;
          if (d->character == ch)
            continue;
          if (char_get_equipment(d->character, WEAR_EYE)) {
            send_to_char(g, d->character, "@RScouter Auction News@D: @GThe auction of @w%s@G has been canceled.\r\n", obj_get_short_description(obj2));
          }
        }

        obj_from_room(g, obj2);
        obj_to_char(g, obj2, ch);
        auc_save(g);
      }
    }

    if (found == false) {
      send_to_char(g, ch, "There are no items being auctioned by you.\r\n");
    }

    return (true);
  }
  if (CMD_IS(g, "pickup")) {
    int founded = false;

    for (obj_data *obj : room_contents(g, auct_room)) {
      if (obj && obj_get_curBidder(obj) == char_get_id(ch)) {
        obj2  = obj;
        found = true;

        if (obj_get_aucter(obj) <= 0) {
          continue;
        }

        if (obj_get_bid(obj2) > char_get_gold(ch)) {
          send_to_char(g, ch, "Unable to purchase %s, you don't have enough money on hand.\r\n", obj_get_short_description(obj2));
          continue;
        }

        if (obj_get_aucTime(obj2) + 86400 > game_now(g)) {
          time_t remain = (obj_get_aucTime(obj2) + 86400) - game_now(g);
          int hour      = (int)((remain % 86400) / 3600);
          int minu      = (int)((remain % 3600) / 60);
          send_to_char(g, ch, "Unable to purchase %s, minimum time to bid is 24 hours. %d hour%s and %d minute%s remain.\r\n", obj_get_short_description(obj2), hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
          continue;
        }

        char_set_gold(ch, char_get_gold(ch) - (obj_get_bid(obj2)));
        obj_from_room(g, obj2);
        obj_to_char(g, obj2, ch);
        send_to_char(g, ch, "You pay %s zenni and receive the item.\r\n", add_commas(obj_get_bid(obj2)));
        auc_save(g);

        for (descriptor_data *d : each_desc(g)) {
          if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
            continue;
          if (d->character == ch)
            continue;
          if (char_get_idnum(d->character) == obj_get_aucter(obj2)) {
            founded = true;
            char_set_bank_gold(d->character, char_get_bank_gold(d->character) + (obj_get_bid(obj2)));
            if (char_get_equipment(d->character, WEAR_EYE)) {
              send_to_char(g, d->character, "@RScouter Auction News@D: @GSomeone has purchased your @w%s@G and you had the money put in your bank account.\r\n", obj_get_short_description(obj2));
            }
          } else if (char_get_equipment(d->character, WEAR_EYE)) {
            send_to_char(g, d->character, "@RScouter Auction News@D: @GSomeone has purchased the @w%s@G that was on auction.\r\n", obj_get_short_description(obj2));
          }
        }

        if (founded == false) {
          struct char_data *vict = nullptr;
          int is_file = false, player_i = 0;

          vict = new char_data();
          clear_char(vict);
          *char_player_specials_slot(vict) = new player_special_data();
          char blam[50];
          sprintf(blam, "%s", obj_get_auctname(obj2));
          if ((player_i = load_char(g, blam, vict)) > -1) {
            is_file = true;
          } else {
            free_char(g, vict);
            continue;
          }
          char_set_bank_gold(vict, char_get_bank_gold(vict) + (obj_get_bid(obj2)));

          char_set_pfilepos(vict, player_i);
          save_char(g, vict);
          if (is_file == true)
            free_char(g, vict);
        }
      }
    }

    if (found == false) {
      send_to_char(g, ch, "There are no items that you have bid on.\r\n");
    }
    return (true);
  }
  if (CMD_IS(g, "auction")) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int value = 0;

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
      send_to_char(g, ch, "Auction what item and for how much?\r\n");
      return (true);
    }

    value = atoi(arg2);

    if (!(obj2 = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You don't have that item to auction.\r\n");
      return (true);
    }
    if (value <= 999) {
      send_to_char(g, ch, "Do not auction anything for less than 1,000 zenni.\r\n");
      return (true);
    }

    if (obj_extra_flagged(obj2, ITEM_BROKEN)) {
      act(g, "$P is broken and we will not accept it.", false, ch, nullptr, obj2, TO_CHAR);
      return (true);
    }

    if (obj_extra_flagged(obj2, ITEM_NODONATE)) {
      act(g, "$P is junk and we will not accept it.", false, ch, nullptr, obj2, TO_CHAR);
      return (true);
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
    obj_set_aucTime(obj2, game_now(g));
    obj_set_curBidder(obj2, -1);
    obj_from_char(g, obj2);
    obj_to_room(g, obj2, auct_room);
    auc_save(g);
    send_to_char(g, ch, "You place %s on auction for %s zenni.\r\n", obj_get_short_description(obj2), add_commas(obj_get_bid(obj2)));
    mud_log("AUCTION: %s places %s on auction for %s", char_get_name(ch), obj_get_short_description(obj2), add_commas(obj_get_bid(obj2)));

    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
        continue;
      if (d->character == ch)
        continue;
      if (char_get_equipment(d->character, WEAR_EYE)) {
        send_to_char(g, d->character, "@RScouter Auction News@D: @GThe item, @w%s@G, has been placed on auction for @Y%s@G zenni.@n\r\n", obj_get_short_description(obj2), add_commas(obj_get_bid(obj2)));
      }
    }
    return (true);
  }

  /* All commands except list and buy */
  return (false);
}

/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

SPECIAL(healtank)
{
  struct obj_data *htank = nullptr;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  for (obj_data *i : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_vnum(g, i) == 65) {
      htank = i;
    } else {
      continue;
    }
  }

  if (CMD_IS(g, "htank")) {
    if (!htank) {
      return (false);
    }

    if (!*arg) {
      send_to_char(g, ch, "@WHealing Tank Commands:\r\n"
                       "htank [ enter | exit | check ]@n");
      return (true);
    }

    if (!strcasecmp("enter", arg)) {
      if (char_plr_flagged(ch, PLR_HEALT)) {
        send_to_char(g, ch, "You are already inside a healing tank!\r\n");
        return (true);
      }
      if (char_get_master(ch) && char_get_master(ch) != ch) {
        send_to_char(g, ch, "You can't enter it while following someone!\r\n");
        return (true);
      }
      if (char_is_android(ch)) {
        send_to_char(g, ch, "A healing tank will have no effect on you.\r\n");
        return (true);
      }
      if (obj_get_healcharge(htank) <= 0) {
        send_to_char(g, ch, "That healing tank needs to recharge, wait a while.\r\n");
        return (true);
      }
      if (obj_extra_flagged(htank, ITEM_BROKEN)) {
        send_to_char(g, ch, "It is broken! You will need to fix it yourself or wait for someone else to fix it.\r\n");
        return (true);
      }
      if (char_get_sits(ch)) {
        send_to_char(g, ch, "You are already on something.\r\n");
        return (true);
      }
      if (obj_get_sitting(htank)) {
        send_to_char(g, ch, "Someone else is already inside that healing tank!\r\n");
        return (true);
      }
      char_set_charge(ch, 0);
      char_plr_set(ch, PLR_CHARGE, false);
      char_set_chargeto(ch, 0);
      char_set_barrier(ch, 0);
      act(g, "@wYou step inside the healing tank and put on its breathing mask. A water like solution pours over your body until the tank is full.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@w steps inside the healing tank and puts on its breathing mask. A water like solution pours over $s body until the tank is full.@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_plr_set(ch, PLR_HEALT, true);
      char_set_sits(ch, htank);
      obj_set_sitting(htank, ch);
      return (true);

    } // End of Enter argument

    else if (!strcasecmp("exit", arg)) {
      if (!char_plr_flagged(ch, PLR_HEALT)) {
        send_to_char(g, ch, "You are not inside a healing tank.\r\n");
        return (true);
      }
      act(g, "@wThe healing tank drains and you exit it shortly after.", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@w exits the healing tank after letting it drain.@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_plr_set(ch, PLR_HEALT, false);
      obj_set_sitting(htank, nullptr);
      char_set_sits(ch, nullptr);
      return (true);
    } // End of Exit argument

    else if (!strcasecmp("check", arg)) {
      if (obj_get_healcharge(htank) < 20 && obj_get_healcharge(htank) > 0) {
        send_to_char(g, ch, "The healing tank has %d bars of energy displayed on its meter.\r\n", obj_get_healcharge(htank));
      } else if (obj_get_healcharge(htank) <= 0) {
        send_to_char(g, ch, "The healing tank has no energy displayed on its meter.\r\n");
      } else {
        send_to_char(g, ch, "The healing tank has full energy shown on its meter.\r\n");
      }
      return (true);
    }

    else {
      send_to_char(g, ch, "@WHealing Tank Commands:\r\n"
                       "htank [ enter | exit | check ]@n");
      return (true);
    }

  } // End of htank command
  else {
    return (false);
  }
}

/* This controls stat augmenter functions */
SPECIAL(augmenter)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (CMD_IS(g, "augment")) {
    int strength = char_get_real_abils_str(ch);
    int intel    = char_get_real_abils_intel(ch);
    int wisdom   = char_get_real_abils_wis(ch);
    int speed    = char_get_real_abils_cha(ch);
    int consti   = char_get_real_abils_con(ch);
    int agility  = char_get_real_abils_dex(ch);

    int strcost = strength * 1200;
    int intcost = intel * 1200;
    int concost = consti * 1200;
    int wiscost = wisdom * 1200;
    int agicost = agility * 1200;
    int specost = speed * 1200;

    if (!*arg) {
      send_to_char(g, ch, "@D                        -----@WBody Augmentations@D-----@n\r\n");
      send_to_char(g, ch, "@RStrength    @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", strength, add_commas(strcost));
      send_to_char(g, ch, "@BIntelligence@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", intel, add_commas(intcost));
      send_to_char(g, ch, "@CWisdom      @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", wisdom, add_commas(wiscost));
      send_to_char(g, ch, "@GConstitution@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", consti, add_commas(concost));
      send_to_char(g, ch, "@mAgility     @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", agility, add_commas(agicost));
      send_to_char(g, ch, "@YSpeed       @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", speed, add_commas(specost));
      send_to_char(g, ch, "\r\n");
      return (true);
    }
    if (!strcasecmp("strength", arg) || !strcasecmp("str", arg)) {
      if (strength >= 100)
        send_to_char(g, ch, "Your strength is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < strcost)
        send_to_char(g, ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act(g, "@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_real_abils_str(ch, char_get_real_abils_str(ch) + 1);
        char_set_gold(ch, char_get_gold(ch) - (strcost));
        save_char(g, ch);
      }
    } else if (!strcasecmp("intelligence", arg) || !strcasecmp("int", arg)) {
      if (intel >= 100)
        send_to_char(g, ch, "Your intelligence is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < intcost)
        send_to_char(g, ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act(g, "@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_real_abils_intel(ch, char_get_real_abils_intel(ch) + 1);
        char_set_gold(ch, char_get_gold(ch) - (intcost));
        save_char(g, ch);
      }
    } else if (!strcasecmp("constitution", arg) || !strcasecmp("con", arg)) {
      if (consti >= 100)
        send_to_char(g, ch, "Your constitution is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < concost)
        send_to_char(g, ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act(g, "@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_real_abils_con(ch, char_get_real_abils_con(ch) + 1);
        char_set_gold(ch, char_get_gold(ch) - (concost));
        save_char(g, ch);
      }
    } else if (!strcasecmp("speed", arg) || !strcasecmp("spe", arg)) {
      if (speed >= 100)
        send_to_char(g, ch, "Your speed is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < specost)
        send_to_char(g, ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act(g, "@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_real_abils_cha(ch, char_get_real_abils_cha(ch) + 1);
        char_set_gold(ch, char_get_gold(ch) - (specost));
        save_char(g, ch);
      }
    } else if (!strcasecmp("agility", arg) || !strcasecmp("agi", arg)) {
      if (agility >= 100)
        send_to_char(g, ch, "Your agility is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < agicost)
        send_to_char(g, ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act(g, "@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_real_abils_dex(ch, char_get_real_abils_dex(ch) + 1);
        char_set_gold(ch, char_get_gold(ch) - (agicost));
        save_char(g, ch);
      }
    } else if (!strcasecmp("wisdom", arg) || !strcasecmp("wis", arg)) {
      if (wisdom >= 100)
        send_to_char(g, ch, "Your wisdom how somehow been measured is already as high as it can possibly go.\r\n");
      else if (char_get_gold(ch) < wiscost)
        send_to_char(g, ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act(g, "@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_real_abils_wis(ch, char_get_real_abils_wis(ch) + 1);
        char_set_gold(ch, char_get_gold(ch) - (wiscost));
        save_char(g, ch);
      }
    } else {
      send_to_char(g, ch, "Syntax: augment [str | con | int | wis | agi | spe]\r\n");
    }
    return (true);
  } else { /* They are not using the right command, ignore them. */
    return (false);
  }
}

/* This controls gravity generator functions */
SPECIAL(gravity)
{
  struct obj_data *obj = nullptr;
  char arg[MAX_INPUT_LENGTH];
  int match = false;

  one_argument(argument, arg);
  for (obj_data *i : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_vnum(g, i) == 11) {
      obj = i;
    } else {
      continue;
    }
  }
  if (CMD_IS(g, "gravity") || CMD_IS(g, "generator")) {
    if (!*arg) {
      send_to_char(g, ch, "@WGravity Commands:@n\r\n");
      send_to_char(g, ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
                       "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
      return (true);
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It's broken!\r\n");
      return (true);
    }
    if ((!strcasecmp("N", arg) || !strcasecmp("n", arg) || !strcasecmp("0", arg)) && obj_get_weight(obj) == 0) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("N", arg) || !strcasecmp("n", arg) || !strcasecmp("0", arg)) {
      send_to_char(g, ch, "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_VEGETA) || rroom_flagged(g, char_get_in_room(ch), ROOM_GRAVITYX10)) {
        rroom_set_gravity(g, char_get_in_room(ch), 10);
        obj_set_weight(obj, 0);
      } else {
        rroom_set_gravity(g, char_get_in_room(ch), 0);
        obj_set_weight(obj, 0);
      }
      match = true;
    }
    if (!strcasecmp("10", arg) && obj_get_weight(obj) == 10) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("10", arg) && rroom_get_gravity(g, char_get_in_room(ch)) == 10 && (rroom_flagged(g, char_get_in_room(ch), ROOM_VEGETA) || rroom_flagged(g, char_get_in_room(ch), ROOM_GRAVITYX10))) {
      send_to_char(g, ch, "The gravity around you is already at that.\r\n");
      return (true);
    }
    if (!strcasecmp("10", arg) && rroom_get_gravity(g, char_get_in_room(ch)) != 10 && (rroom_flagged(g, char_get_in_room(ch), ROOM_VEGETA) || rroom_flagged(g, char_get_in_room(ch), ROOM_GRAVITYX10))) {
      send_to_char(g, ch, "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 10);
      obj_set_weight(obj, 0);
      match = true;
    } else if (!strcasecmp("10", arg)) {
      send_to_char(g, ch, "You punch in ten times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      rroom_set_gravity(g, char_get_in_room(ch), 10);
      obj_set_weight(obj, 10);
      match = true;
    }
    if (!strcasecmp("20", arg) && obj_get_weight(obj) == 20) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("20", arg)) {
      send_to_char(g, ch, "You punch in twenty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 20);
      obj_set_weight(obj, 20);
      match = true;
    }
    if (!strcasecmp("30", arg) && obj_get_weight(obj) == 30) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("30", arg)) {
      send_to_char(g, ch, "You punch in thirty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 30);
      obj_set_weight(obj, 30);
      match = true;
    }
    if (!strcasecmp("40", arg) && obj_get_weight(obj) == 40) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("40", arg)) {
      send_to_char(g, ch, "You punch in fourty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 40);
      obj_set_weight(obj, 40);
      match = true;
    }
    if (!strcasecmp("50", arg) && obj_get_weight(obj) == 50) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("50", arg)) {
      send_to_char(g, ch, "You punch in fifty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 50);
      obj_set_weight(obj, 50);
      match = true;
    }
    if (!strcasecmp("100", arg) && obj_get_weight(obj) == 100) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("100", arg)) {
      send_to_char(g, ch, "You punch in one hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 100);
      obj_set_weight(obj, 100);
      match = true;
    }
    if (!strcasecmp("200", arg) && obj_get_weight(obj) == 200) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("200", arg)) {
      send_to_char(g, ch, "You punch in two hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 200);
      obj_set_weight(obj, 200);
      match = true;
    }
    if (!strcasecmp("300", arg) && obj_get_weight(obj) == 300) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("300", arg)) {
      send_to_char(g, ch, "You punch in three hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 300);
      obj_set_weight(obj, 300);
      match = true;
    }
    if (!strcasecmp("400", arg) && obj_get_weight(obj) == 400) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("400", arg)) {
      send_to_char(g, ch, "You punch in four hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 400);
      obj_set_weight(obj, 400);
      match = true;
    }
    if (!strcasecmp("500", arg) && obj_get_weight(obj) == 500) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("500", arg)) {
      send_to_char(g, ch, "You punch in five hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 500);
      obj_set_weight(obj, 500);
      match = true;
    }
    if (!strcasecmp("1000", arg) && obj_get_weight(obj) == 1000) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("1000", arg)) {
      send_to_char(g, ch, "You punch in one thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 1000);
      obj_set_weight(obj, 1000);
      match = true;
    }
    if (!strcasecmp("5000", arg) && obj_get_weight(obj) == 5000) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("5000", arg)) {
      send_to_char(g, ch, "You punch in five thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 5000);
      obj_set_weight(obj, 5000);
      match = true;
    }
    if (!strcasecmp("10000", arg) && obj_get_weight(obj) == 10000) {
      send_to_char(g, ch, "The gravity generator is already set to that.\r\n");
      return (true);
    }
    if (!strcasecmp("10000", arg)) {
      send_to_char(g, ch, "You punch in ten thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
      act(g, "@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
        rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, false);
        send_to_room(g, char_get_in_room(ch), "The increased gravity forces the aura to disappear.\r\n");
      }
      rroom_set_gravity(g, char_get_in_room(ch), 10000);
      obj_set_weight(obj, 10000);
      match = true;
    } else if (match == false) {
      send_to_char(g, ch, "That is not a proper command for this device.\r\n");
      send_to_char(g, ch, "@WGravity Commands:@n\r\n");
      send_to_char(g, ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
                       "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
      return (true);
    }
    return (true);
  }
  return (false);
}

SPECIAL(bank)
{
  int amount, num = 0;

  struct obj_data *obj = nullptr; /* an ATM here, if any: this also runs for a carried cashcard */

  for (obj_data *i : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_vnum(g, i) == 3034) {
      obj = i;
    } else {
      continue;
    }
  }

  if (CMD_IS(g, "balance")) {
    if ((obj && obj_extra_flagged(obj, ITEM_BROKEN))) {
      send_to_char(g, ch, "The ATM is broken!\r\n");
      return (true);
    }

    if (char_get_bank_gold(ch) > 0)
      send_to_char(g, ch, "Your current balance is %d zenni.\r\n", char_get_bank_gold(ch));
    else
      send_to_char(g, ch, "You currently have no money deposited.\r\n");
    return (true);
  }
  if (CMD_IS(g, "wire")) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct char_data *vict = nullptr;

    two_arguments(argument, arg, arg2);

    if ((obj && obj_extra_flagged(obj, ITEM_BROKEN))) {
      send_to_char(g, ch, "The ATM is broken!\r\n");
      return (true);
    }

    if ((amount = atoi(arg)) <= 0) {
      send_to_char(g, ch, "How much do you want to transfer?\r\n");
      return (true);
    }
    if (char_get_bank_gold(ch) < amount + (amount / 100)) {
      send_to_char(g, ch, "You don't have that much zenni in the bank (plus 1%s charge)!\r\n", "%");
      return (true);
    }
    if (!*arg2) {
      send_to_char(g, ch, "You want to transfer it to who?!\r\n");
      return (true);
    }
    if (!(vict = get_player_vis(g, ch, arg2, nullptr, FIND_CHAR_WORLD))) {
      int is_file = false, player_i = 0;
      char name[MAX_INPUT_LENGTH];

      vict = new char_data();
      clear_char(vict);
      *char_player_specials_slot(vict) = new player_special_data();

      sprintf(name, "%s", rIntro(ch, arg2));

      if ((player_i = load_char(g, name, vict)) > -1) {
        is_file = true;
      } else {
        free_char(g, vict);
        send_to_char(g, ch, "That person doesn't exist.\r\n");
        return (true);
      }
      if (char_get_desc(ch)->user == nullptr) {
        send_to_char(g, ch, "There is an error. Report to Iovan.");
        return (true);
      }
      if (!strcasecmp(char_get_name(vict), char_get_desc(ch)->tmp1) || !strcasecmp(char_get_name(vict), char_get_desc(ch)->tmp2) || !strcasecmp(char_get_name(vict), char_get_desc(ch)->tmp3) || !strcasecmp(char_get_name(vict), char_get_desc(ch)->tmp4) || !strcasecmp(char_get_name(vict), char_get_desc(ch)->tmp5)) {
        send_to_char(g, ch, "You can not transfer money to your own offline characters...");
        if (is_file == true)
          free_char(g, vict);
        return (true);
      }
      char_set_bank_gold(vict, char_get_bank_gold(vict) + (amount));
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + (amount / 100)));
      char_set_pfilepos(vict, player_i);
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMPL, char_get_invis_level(g, ch)), true, "EXCHANGE: %s gave %s zenni to user %s", char_get_name(ch), add_commas(amount), char_get_name(vict));
      save_char(g, vict);
      if (is_file == true)
        free_char(g, vict);
    } else {
      char_set_bank_gold(vict, char_get_bank_gold(vict) + (amount));
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + (amount / 100)));
      send_to_char(g, vict, "@WYou have just had @Y%s@W zenni wired into your bank account.@n\r\n", add_commas(amount));
    }
    send_to_char(g, ch, "You transfer %s zenni to them.\r\n", add_commas(amount));
    act(g, "$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
    return (true);
  }
  if (CMD_IS(g, "deposit")) {

    if ((obj && obj_extra_flagged(obj, ITEM_BROKEN))) {
      send_to_char(g, ch, "The ATM is broken!\r\n");
      return (true);
    }

    if ((amount = atoi(argument)) <= 0) {
      send_to_char(g, ch, "How much do you want to deposit?\r\n");
      return (true);
    }
    if (char_get_gold(ch) < amount) {
      send_to_char(g, ch, "You don't have that much zenni!\r\n");
      return (true);
    }
    char_set_gold(ch, char_get_gold(ch) - (amount));
    char_set_bank_gold(ch, char_get_bank_gold(ch) + (amount));
    send_to_char(g, ch, "You deposit %d zenni.\r\n", amount);
    act(g, "$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
    return (true);
  }
  if (CMD_IS(g, "withdraw")) {

    if ((obj && obj_extra_flagged(obj, ITEM_BROKEN))) {
      send_to_char(g, ch, "The ATM is broken!\r\n");
      return (true);
    }

    if ((amount = atoi(argument)) <= 0) {
      send_to_char(g, ch, "How much do you want to withdraw?\r\n");
      return (true);
    }
    if (char_get_bank_gold(ch) < amount) {
      send_to_char(g, ch, "You don't have that much zenni!\r\n");
      return (true);
    }
    if (char_get_bank_gold(ch) - (amount + (1 + amount / 100)) < 0) {
      if (amount >= 100) {
        amount = amount + (amount / 100);
      } else if (amount < 100) {
        amount = amount + 1;
      }
      send_to_char(g, ch, "You need at least %s in the bank with the 1 percent withdraw fee.\r\n", add_commas(amount));
      return (true);
    }
    if (char_get_gold(ch) + amount > char_gold_carry(ch)) {
      send_to_char(g, ch, "You can only carry %s zenni, you left the rest.\r\n", add_commas(char_gold_carry(ch)));
      int diff = (char_get_gold(ch) + amount) - char_gold_carry(ch);
      char_set_gold(ch, char_gold_carry(ch));
      amount -= diff;
      if (amount >= 100) {
        num = amount / 100;
        char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + num));
      } else if (amount < 100) {
        char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + 1));
      }
      send_to_char(g, ch, "You withdraw %s zenni,  and pay %s in withdraw fees.\r\n.\r\n", add_commas(amount), add_commas(num));
      act(g, "$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
      return (true);
    }
    char_set_gold(ch, char_get_gold(ch) + (amount));
    if (amount >= 100) {
      num = amount / 100;
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + num));
    } else if (amount < 100) {
      char_set_bank_gold(ch, char_get_bank_gold(ch) - (amount + 1));
    }
    send_to_char(g, ch, "You withdraw %s zenni, and pay %s in withdraw fees.\r\n", add_commas(amount), add_commas(num));
    act(g, "$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
    return (true);
  } else
    return (false);
}
