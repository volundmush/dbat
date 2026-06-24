/* ************************************************************************
 *   File: spec_procs.c                                  Part of CircleMUD *
 *  Usage: implementation of special procedures for mobiles/objects/rooms  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "spec_procs.h"

#include "act.comm.h"
#include "act.informative.h"
#include "act.item.h"
#include "act.social.h"
#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "comm.h"
#include "consts/admlevel.h"
#include "consts/constates.h"
#include "consts/gauntlet.h"
#include "consts/itemdata.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/prefflags.h"
#include "consts/races.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "extract.h"
#include "flags.h"
#include "guild.h"
#include "handler.h"
#include "interpreter.h"
#include "log.h"
#include "mail.h"
#include "object_api.h"
#include "object_impl.h"
#include "object_macros.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_api.h"
#include "room_db.h"
#include "consts/roomflags.h"
#include "search.h"
#include "spells.h"
#include "stringutils.h"
#include "util_macros.h"
#include "weather_db.h"

#include "iterate.hpp"

#include <cstring>

/* local functions */

/* ********************************************************************
 *  Special procedures for mobiles                                     *
 ******************************************************************** */

SPECIAL(dump) {
  struct obj_data *k;
  int value = 0;

  room_contents_iterate(char_room_get(ch), [&](auto k) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
    return true;
  });

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  room_contents_iterate(char_room_get(ch), [&](auto k) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
    return true;
  });

  if (value) {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0,
        TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      char_stat_mod(ch, "money", value);
  }
  return (TRUE);
}

/* ********************************************************************
 *  General special procedures for mobiles                             *
 ******************************************************************** */

int num_players_in_room(room_vnum room) {
  struct descriptor_data *i;
  int num_players = 0;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (!(i->character))
      continue;
    if (char_room_vnum_get(i->character) != room)
      continue;
    if ((GET_ADMLEVEL(i->character) >= ADMLVL_IMMORT) &&
        (PRF_FLAGGED(i->character, PRF_NOHASSLE))) /* Ignore Imms */
      continue;

    num_players++;
  }

  return num_players;
}

bool check_mob_in_room(mob_vnum mob, room_vnum room) {
  bool found = FALSE;

  char_iterate_all([&](struct char_data *i) {
    if (GET_MOB_VNUM(i) == mob && char_room_vnum_get(i) == room) {
      found = TRUE;
      return false;
    }
    return true;
  });

  return found;
}

bool check_obj_in_room(obj_vnum obj, room_vnum room) {
  bool found = FALSE;
  struct room_data *r_room = room_by_id(room);

  room_contents_iterate(r_room, [&](struct obj_data *i) {
    if (GET_OBJ_VNUM(i) == obj) {
      found = TRUE;
      return false;
    }
    return true;
  });
  return found;
}

void npc_steal(struct char_data *ch, struct char_data *victim) {
  int gold;

  if (IS_NPC(victim))
    return;
  if (IS_NPC(ch))
    return;
  if (ADM_FLAGGED(victim, ADM_NOSTEAL))
    return;
  if (!CAN_SEE(ch, victim))
    return;

  if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0,
        victim, TO_VICT);
    act("$n tries to steal zenni from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
    if (gold > 0) {
      char_stat_mod(ch, "money", gold);
      char_stat_mod(victim, "money", -gold);
    }
  }
}

SPECIAL(thief) {
  struct char_data *cons;

  if (IS_NPC(ch))
    return (FALSE);

  if (cmd || GET_POS(ch) != POS_STANDING)
    return (FALSE);

  bool handled = false;

  room_people_iterate(char_room_get(ch), [&](auto cons) {
    if (!IS_NPC(cons) && !ADM_FLAGGED(cons, ADM_NOSTEAL) &&
        !rand_number(0, 4)) {
      npc_steal(ch, cons);
      handled = true;
      return false;
    }
    return true;
  });

  return handled;
}

/* ********************************************************************
 *  Special procedures for mobiles                                      *
 ******************************************************************** */

SPECIAL(janitor) {
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  {
    bool found = false;
    room_contents_iterate(char_room_get(ch), [&](auto i) {
      if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
        return true;
      if (GET_OBJ_TYPE(i) == ITEM_DRINKCON || GET_OBJ_COST(i) >= 100)
        return true;
      act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
      obj_from_room(i);
      obj_to_char(i, ch);
      found = true;
      return false;
    });
    if (found) return (TRUE);
  }

  return (FALSE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(auction) {
  struct room_data *auct_room = room_by_id(80);
  struct obj_data *obj, *next_obj, *obj2 = NULL;
  int found = FALSE;

  /* Gross. */

  if (CMD_IS("cancel")) {

    room_contents_iterate(auct_room, [&](auto obj) {
      if (obj && GET_AUCTER(obj) == GET_ID(ch)) {
        obj2 = obj;
        found = TRUE;

        if (GET_CURBID(obj2) != -1 && GET_AUCTIME(obj2) + 518400 > time(0)) {
          send_to_char(ch, "Unable to cancel. Someone has already bid on it "
                           "and their bid license hasn't expired.\r\n");
          time_t remain = (GET_AUCTIME(obj2) + 518400) - time(0);
          int day = (int)((remain % 604800) / 86400);
          int hour = (int)((remain % 86400) / 3600);
          int minu = (int)((remain % 3600) / 60);
          send_to_char(ch,
                       "Time Till License Expiration: %d day%s, %d hour%s, %d "
                       "minute%s.\r\n",
                       day, day > 1 ? "s" : "", hour, hour > 1 ? "s" : "", minu,
                       minu > 1 ? "s" : "");
          return true;
        }

        send_to_char(
            ch,
            "@wYou cancel the auction of %s@w and it is returned to you.@n\r\n",
            obj2->short_description);
        struct descriptor_data *d;

        for (d = descriptor_list; d; d = d->next) {
          if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
            continue;
          if (d->character == ch)
            continue;
          if (GET_EQ(d->character, WEAR_EYE)) {
            send_to_char(d->character,
                         "@RScouter Auction News@D: @GThe auction of @w%s@G "
                         "has been canceled.\r\n",
                         obj2->short_description);
          }
        }

        obj_from_room(obj2);
        obj_to_char(obj2, ch);
        auc_save();
      }
      return true;
    });

    if (found == FALSE) {
      send_to_char(ch, "There are no items being auctioned by you.\r\n");
    }

    return (TRUE);
  } else if (CMD_IS("pickup")) {
    struct descriptor_data *d;
    int founded = FALSE;

    room_contents_iterate(auct_room, [&](auto obj) {
      if (obj && GET_CURBID(obj) == GET_ID(ch)) {
        obj2 = obj;
        found = TRUE;

        if (GET_AUCTER(obj) <= 0) {
          return true;
        }

        if (GET_BID(obj2) > GET_GOLD(ch)) {
          send_to_char(
              ch,
              "Unable to purchase %s, you don't have enough money on hand.\r\n",
              obj2->short_description);
          return true;
        }

        if (GET_AUCTIME(obj2) + 86400 > time(0)) {
          time_t remain = (GET_AUCTIME(obj2) + 86400) - time(0);
          int hour = (int)((remain % 86400) / 3600);
          int minu = (int)((remain % 3600) / 60);
          send_to_char(ch,
                       "Unable to purchase %s, minimum time to bid is 24 "
                       "hours. %d hour%s and %d minute%s remain.\r\n",
                       obj2->short_description, hour, hour > 1 ? "s" : "", minu,
                       minu > 1 ? "s" : "");
          return true;
        }

        char_stat_mod(ch, "money", -GET_BID(obj2));
        obj_from_room(obj2);
        obj_to_char(obj2, ch);
        send_to_char(ch, "You pay %s zenni and receive the item.\r\n",
                     add_commas(GET_BID(obj2)));
        auc_save();

        for (d = descriptor_list; d; d = d->next) {
          if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
            continue;
          if (d->character == ch)
            continue;
          if (GET_IDNUM(d->character) == GET_AUCTER(obj2)) {
            founded = TRUE;
            char_stat_mod(d->character, "money_bank", GET_BID(obj2));
            if (GET_EQ(d->character, WEAR_EYE)) {
              send_to_char(
                  d->character,
                  "@RScouter Auction News@D: @GSomeone has purchased your "
                  "@w%s@G and you had the money put in your bank account.\r\n",
                  obj2->short_description);
            }
          } else if (GET_EQ(d->character, WEAR_EYE)) {
            send_to_char(d->character,
                         "@RScouter Auction News@D: @GSomeone has purchased "
                         "the @w%s@G that was on auction.\r\n",
                         obj2->short_description);
          }
        }

        if (founded == FALSE) {
          struct char_data *vict = NULL;
          int is_file = FALSE, player_i = 0;

          CREATE(vict, struct char_data, 1);
          clear_char(vict);
          char blam[50];
          sprintf(blam, "%s", GET_AUCTERN(obj2));
          if ((player_i = load_char(blam, vict)) > -1) {
            is_file = TRUE;
          } else {
            free_char(vict);
            return true;
          }
          char_stat_mod(vict, "money_bank", GET_BID(obj2));

          GET_PFILEPOS(vict) = player_i;
          save_char(vict);
          if (is_file == TRUE)
            free_char(vict);
        }
      }
      return true;
    });

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

    if (!(obj2 = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You don't have that item to auction.\r\n");
      return (TRUE);
    }
    if (value <= 999) {
      send_to_char(ch,
                   "Do not auction anything for less than 1,000 zenni.\r\n");
      return (TRUE);
    }

    if (OBJ_FLAGGED(obj2, ITEM_BROKEN)) {
      act("$P is broken and we will not accept it.", FALSE, ch, 0, obj2,
          TO_CHAR);
      return (TRUE);
    }

    if (OBJ_FLAGGED(obj2, ITEM_NODONATE)) {
      act("$P is junk and we will not accept it.", FALSE, ch, 0, obj2, TO_CHAR);
      return (TRUE);
    }

    GET_BID(obj2) = value;
    GET_STARTBID(obj2) = 0;
    GET_AUCTER(obj2) = 0;
    if (GET_AUCTERN(obj2))
      free(GET_AUCTERN(obj2));
    GET_AUCTIME(obj2) = 0;

    GET_BID(obj2) = value;
    GET_STARTBID(obj2) = GET_BID(obj2);
    GET_AUCTER(obj2) = GET_ID(ch);
    GET_AUCTERN(obj2) = strdup(GET_NAME(ch));
    GET_AUCTIME(obj2) = time(0);
    GET_CURBID(obj2) = -1;
    obj_from_char(obj2);
    obj_to_room(obj2, auct_room);
    auc_save();
    send_to_char(ch, "You place %s on auction for %s zenni.\r\n",
                 obj2->short_description, add_commas(GET_BID(obj2)));
    mud_log("AUCTION: %s places %s on auction for %s", GET_NAME(ch),
        obj2->short_description, add_commas(GET_BID(obj2)));

    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
        continue;
      if (d->character == ch)
        continue;
      if (GET_EQ(d->character, WEAR_EYE)) {
        send_to_char(d->character,
                     "@RScouter Auction News@D: @GThe item, @w%s@G, has been "
                     "placed on auction for @Y%s@G zenni.@n\r\n",
                     obj2->short_description, add_commas(GET_BID(obj2)));
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

SPECIAL(healtank) {
  struct obj_data *htank = NULL, *i;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  room_contents_iterate(char_room_get(ch), [&](auto i) {
    if (GET_OBJ_VNUM(i) == 65) {
      htank = i;
    }
    return true;
  });

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
      if (PLR_FLAGGED(ch, PLR_HEALT)) {
        send_to_char(ch, "You are already inside a healing tank!\r\n");
        return (TRUE);
      }
      if (MASTER(ch) && MASTER(ch) != ch) {
        send_to_char(ch, "You can't enter it while following someone!\r\n");
        return (TRUE);
      } else if (IS_ANDROID(ch)) {
        send_to_char(ch, "A healing tank will have no effect on you.\r\n");
        return (TRUE);
      } else if (HCHARGE(htank) <= 0) {
        send_to_char(ch,
                     "That healing tank needs to recharge, wait a while.\r\n");
        return (TRUE);
      } else if (OBJ_FLAGGED(htank, ITEM_BROKEN)) {
        send_to_char(ch, "It is broken! You will need to fix it yourself or "
                         "wait for someone else to fix it.\r\n");
        return (TRUE);
      } else if (SITS(ch)) {
        send_to_char(ch, "You are already on something.\r\n");
        return (TRUE);
      } else if (SITTING(htank)) {
        send_to_char(ch,
                     "Someone else is already inside that healing tank!\r\n");
        return (TRUE);
      } else {
        char_condition_remove(ch, "charge", "tank");
        char_condition_remove(ch, "barrier", "tank");
        act("@wYou step inside the healing tank and put on its breathing mask. "
            "A water like solution pours over your body until the tank is "
            "full.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w steps inside the healing tank and puts on its breathing "
            "mask. A water like solution pours over $s body until the tank is "
            "full.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        SET_BIT_AR(PLR_FLAGS(ch), PLR_HEALT);
        SITS(ch) = htank;
        SITTING(htank) = ch;
        return (TRUE);
      }

    } // End of Enter argument

    else if (!strcasecmp("exit", arg)) {
      if (!PLR_FLAGGED(ch, PLR_HEALT)) {
        send_to_char(ch, "You are not inside a healing tank.\r\n");
        return (TRUE);
      } else {
        act("@wThe healing tank drains and you exit it shortly after.", TRUE,
            ch, 0, 0, TO_CHAR);
        act("@C$n@w exits the healing tank after letting it drain.@n", TRUE, ch,
            0, 0, TO_ROOM);
        REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_HEALT);
        SITTING(htank) = NULL;
        SITS(ch) = NULL;
        return (TRUE);
      }
    } // End of Exit argument

    else if (!strcasecmp("check", arg)) {
      if (HCHARGE(htank) < 20 && HCHARGE(htank) > 0) {
        send_to_char(ch,
                     "The healing tank has %d bars of energy displayed on its "
                     "meter.\r\n",
                     HCHARGE(htank));
      } else if (HCHARGE(htank) <= 0) {
        send_to_char(
            ch, "The healing tank has no energy displayed on its meter.\r\n");
      } else {
        send_to_char(
            ch, "The healing tank has full energy shown on its meter.\r\n");
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
SPECIAL(augmenter) {
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (CMD_IS("augment")) {
    int strength = char_stat_get(ch, "strength");
    int intel = char_stat_get(ch, "intelligence");
    int wisdom = char_stat_get(ch, "wisdom");
    int speed = char_stat_get(ch, "speed");
    int consti = char_stat_get(ch, "constitution");
    int agility = char_stat_get(ch, "agility");

    int strcost = strength * 1200;
    int intcost = intel * 1200;
    int concost = consti * 1200;
    int wiscost = wisdom * 1200;
    int agicost = agility * 1200;
    int specost = speed * 1200;

    if (!*arg) {
      send_to_char(
          ch,
          "@D                        -----@WBody Augmentations@D-----@n\r\n");
      send_to_char(ch,
                   "@RStrength    @y: @WCurrently measured at @w%d@W, cost to "
                   "augment @Y%s@W.@n\r\n",
                   strength, add_commas(strcost));
      send_to_char(ch,
                   "@BIntelligence@y: @WCurrently measured at @w%d@W, cost to "
                   "augment @Y%s@W.@n\r\n",
                   intel, add_commas(intcost));
      send_to_char(ch,
                   "@CWisdom      @y: @WCurrently measured at @w%d@W, cost to "
                   "augment @Y%s@W.@n\r\n",
                   wisdom, add_commas(wiscost));
      send_to_char(ch,
                   "@GConstitution@y: @WCurrently measured at @w%d@W, cost to "
                   "augment @Y%s@W.@n\r\n",
                   consti, add_commas(concost));
      send_to_char(ch,
                   "@mAgility     @y: @WCurrently measured at @w%d@W, cost to "
                   "augment @Y%s@W.@n\r\n",
                   agility, add_commas(agicost));
      send_to_char(ch,
                   "@YSpeed       @y: @WCurrently measured at @w%d@W, cost to "
                   "augment @Y%s@W.@n\r\n",
                   speed, add_commas(specost));
      send_to_char(ch, "\r\n");
      return (TRUE);
    } else if (!strcasecmp("strength", arg) || !strcasecmp("str", arg)) {
      if (strength >= 100)
        send_to_char(
            ch, "Your strength is already as high as it can possibly go.\r\n");
      else if (GET_GOLD(ch) < strcost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with "
            "microscopic attachments.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly "
            "operates on $s body.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        char_stat_mod(ch, "strength", 1);
        char_stat_mod(ch, "money", -strcost);
        save_char(ch);
      }
    } else if (!strcasecmp("intelligence", arg) || !strcasecmp("int", arg)) {
      if (intel >= 100)
        send_to_char(
            ch,
            "Your intelligence is already as high as it can possibly go.\r\n");
      else if (GET_GOLD(ch) < intcost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with "
            "microscopic attachments.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly "
            "operates on $s body.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        char_stat_mod(ch, "intelligence", 1);
        char_stat_mod(ch, "money", -intcost);
        save_char(ch);
      }
    } else if (!strcasecmp("constitution", arg) || !strcasecmp("con", arg)) {
      if (consti >= 100)
        send_to_char(
            ch,
            "Your constitution is already as high as it can possibly go.\r\n");
      else if (GET_GOLD(ch) < concost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with "
            "microscopic attachments.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly "
            "operates on $s body.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        char_stat_mod(ch, "constitution", 1);
        char_stat_mod(ch, "money", -concost);
        save_char(ch);
      }
    } else if (!strcasecmp("speed", arg) || !strcasecmp("spe", arg)) {
      if (speed >= 100)
        send_to_char(
            ch, "Your speed is already as high as it can possibly go.\r\n");
      else if (GET_GOLD(ch) < specost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with "
            "microscopic attachments.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly "
            "operates on $s body.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        char_stat_mod(ch, "speed", 1);
        char_stat_mod(ch, "money", -specost);
        save_char(ch);
      }
    } else if (!strcasecmp("agility", arg) || !strcasecmp("agi", arg)) {
      if (agility >= 100)
        send_to_char(
            ch, "Your agility is already as high as it can possibly go.\r\n");
      else if (GET_GOLD(ch) < agicost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with "
            "microscopic attachments.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly "
            "operates on $s body.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        char_stat_mod(ch, "agility", 1);
        char_stat_mod(ch, "money", -agicost);
        save_char(ch);
      }
    } else if (!strcasecmp("wisdom", arg) || !strcasecmp("wis", arg)) {
      if (wisdom >= 100)
        send_to_char(ch, "Your wisdom how somehow been measured is already as "
                         "high as it can possibly go.\r\n");
      else if (GET_GOLD(ch) < wiscost)
        send_to_char(ch, "You can not afford the price!\r\n");
      else { /* They can augment it! */
        act("@WThe machine's arm moves out and quickly augments your body with "
            "microscopic attachments.@n",
            TRUE, ch, 0, 0, TO_CHAR);
        act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly "
            "operates on $s body.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        char_stat_mod(ch, "wisdom", 1);
        char_stat_mod(ch, "money", -wiscost);
        save_char(ch);
      }
    } else {
      send_to_char(ch,
                   "Syntax: augment [str | con | int | wis | agi | spe]\r\n");
    }
    return (TRUE);
  } else { /* They are not using the right command, ignore them. */
    return (FALSE);
  }
}

/* This controls gravity generator functions */
SPECIAL(gravity) {
  struct obj_data *i, *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  int match = FALSE;

  one_argument(argument, arg);
  room_contents_iterate(char_room_get(ch), [&](auto i) {
    if (GET_OBJ_VNUM(i) == 11) {
      obj = i;
    }
    return true;
  });
  if (CMD_IS("gravity") || CMD_IS("generator")) {
    if (!*arg) {
      send_to_char(ch, "@WGravity Commands:@n\r\n");
      send_to_char(
          ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
              "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
      return (TRUE);
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It's broken!\r\n");
      return (TRUE);
    }
    struct room_data *room = char_room_get(ch);
    if ((!strcasecmp("N", arg) || !strcasecmp("n", arg) ||
         !strcasecmp("0", arg)) &&
        GET_OBJ_WEIGHT(obj) == 0) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("N", arg) || !strcasecmp("n", arg) ||
               !strcasecmp("0", arg)) {
      send_to_char(
          ch,
          "You punch in normal gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_VEGETA) ||
          room_flagged(room, ROOM_GRAVITYX10)) {
        room_gravity_set(room, 10);
        GET_OBJ_WEIGHT(obj) = 0;
      } else {
        room_gravity_set(room, 0);
        GET_OBJ_WEIGHT(obj) = 0;
      }
      match = TRUE;
    }
    if (!strcasecmp("10", arg) && GET_OBJ_WEIGHT(obj) == 10) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("10", arg) && room_gravity_get(room) == 10 &&
               (room_flagged(room, ROOM_VEGETA) ||
                room_flagged(room, ROOM_GRAVITYX10))) {
      send_to_char(ch, "The gravity around you is already at that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("10", arg) && room_gravity_get(room) != 10 &&
               (room_flagged(room, ROOM_VEGETA) ||
                room_flagged(room, ROOM_GRAVITYX10))) {
      send_to_char(
          ch,
          "You punch in normal gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(char_room_get(ch), 10);
      GET_OBJ_WEIGHT(obj) = 0;
      match = TRUE;
    } else if (!strcasecmp("10", arg)) {
      send_to_char(
          ch,
          "You punch in ten times gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      room_gravity_set(char_room_get(ch), 10);
      GET_OBJ_WEIGHT(obj) = 10;
      match = TRUE;
    }
    if (!strcasecmp("20", arg) && GET_OBJ_WEIGHT(obj) == 20) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("20", arg)) {
      send_to_char(
          ch,
          "You punch in twenty times gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 20);
      GET_OBJ_WEIGHT(obj) = 20;
      match = TRUE;
    }
    if (!strcasecmp("30", arg) && GET_OBJ_WEIGHT(obj) == 30) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("30", arg)) {
      send_to_char(
          ch,
          "You punch in thirty times gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 30);
      GET_OBJ_WEIGHT(obj) = 30;
      match = TRUE;
    }
    if (!strcasecmp("40", arg) && GET_OBJ_WEIGHT(obj) == 40) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("40", arg)) {
      send_to_char(
          ch,
          "You punch in fourty times gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 40);
      GET_OBJ_WEIGHT(obj) = 40;
      match = TRUE;
    }
    if (!strcasecmp("50", arg) && GET_OBJ_WEIGHT(obj) == 50) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("50", arg)) {
      send_to_char(
          ch,
          "You punch in fifty times gravity on the generator. It hums for a "
          "moment\r\nbefore you feel the pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 50);
      GET_OBJ_WEIGHT(obj) = 50;
      match = TRUE;
    }
    if (!strcasecmp("100", arg) && GET_OBJ_WEIGHT(obj) == 100) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("100", arg)) {
      send_to_char(ch, "You punch in one hundred times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 100);
      GET_OBJ_WEIGHT(obj) = 100;
      match = TRUE;
    }
    if (!strcasecmp("200", arg) && GET_OBJ_WEIGHT(obj) == 200) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("200", arg)) {
      send_to_char(ch, "You punch in two hundred times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 200);
      GET_OBJ_WEIGHT(obj) = 200;
      match = TRUE;
    }
    if (!strcasecmp("300", arg) && GET_OBJ_WEIGHT(obj) == 300) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("300", arg)) {
      send_to_char(ch, "You punch in three hundred times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 300);
      GET_OBJ_WEIGHT(obj) = 300;
      match = TRUE;
    }
    if (!strcasecmp("400", arg) && GET_OBJ_WEIGHT(obj) == 400) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("400", arg)) {
      send_to_char(ch, "You punch in four hundred times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 400);
      GET_OBJ_WEIGHT(obj) = 400;
      match = TRUE;
    }
    if (!strcasecmp("500", arg) && GET_OBJ_WEIGHT(obj) == 500) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("500", arg)) {
      send_to_char(ch, "You punch in five hundred times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 500);
      GET_OBJ_WEIGHT(obj) = 500;
      match = TRUE;
    }
    if (!strcasecmp("1000", arg) && GET_OBJ_WEIGHT(obj) == 1000) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("1000", arg)) {
      send_to_char(ch, "You punch in one thousand times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 1000);
      GET_OBJ_WEIGHT(obj) = 1000;
      match = TRUE;
    }
    if (!strcasecmp("5000", arg) && GET_OBJ_WEIGHT(obj) == 5000) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("5000", arg)) {
      send_to_char(ch, "You punch in five thousand times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 5000);
      GET_OBJ_WEIGHT(obj) = 5000;
      match = TRUE;
    }
    if (!strcasecmp("10000", arg) && GET_OBJ_WEIGHT(obj) == 10000) {
      send_to_char(ch, "The gravity generator is already set to that.\r\n");
      return (TRUE);
    } else if (!strcasecmp("10000", arg)) {
      send_to_char(ch, "You punch in ten thousand times gravity on the "
                       "generator. It hums for a moment\r\nbefore you feel the "
                       "pressure on your body change.\r\n");
      act("@W$n@w pushes some buttons on the gravity generator, and you feel a "
          "change in pressure on your body.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (room_flagged(room, ROOM_AURA)) {
        room_flag_set(room, ROOM_AURA, FALSE);
        send_to_room(room,
                     "The increased gravity forces the aura to disappear.\r\n");
      }
      room_gravity_set(room, 10000);
      GET_OBJ_WEIGHT(obj) = 10000;
      match = TRUE;
    } else if (match == FALSE) {
      send_to_char(ch, "That is not a proper command for this device.\r\n");
      send_to_char(ch, "@WGravity Commands:@n\r\n");
      send_to_char(
          ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
              "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
      return (TRUE);
    }
    return (TRUE);
  } else {
    return (FALSE);
  }
}

SPECIAL(bank) {
  int amount, num = 0;

  struct obj_data *i, *obj = NULL;

  room_contents_iterate(char_room_get(ch), [&](auto i) {
    if (GET_OBJ_VNUM(i) == 3034) {
      obj = i;
    }
    return true;
  });

  if (CMD_IS("balance")) {
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if (GET_BANK_GOLD(ch) > 0)
      send_to_char(ch, "Your current balance is %d zenni.\r\n",
                   GET_BANK_GOLD(ch));
    else
      send_to_char(ch, "You currently have no money deposited.\r\n");
    return (TRUE);
  } else if (CMD_IS("wire")) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct char_data *vict = NULL;

    two_arguments(argument, arg, arg2);

    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if ((amount = atoi(arg)) <= 0) {
      send_to_char(ch, "How much do you want to transfer?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount + (amount / 100)) {
      send_to_char(
          ch,
          "You don't have that much zenni in the bank (plus 1%s charge)!\r\n",
          "%");
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
      if (!strcasecmp(GET_NAME(vict), ch->desc->tmp1) ||
          !strcasecmp(GET_NAME(vict), ch->desc->tmp2) ||
          !strcasecmp(GET_NAME(vict), ch->desc->tmp3) ||
          !strcasecmp(GET_NAME(vict), ch->desc->tmp4) ||
          !strcasecmp(GET_NAME(vict), ch->desc->tmp5)) {
        send_to_char(
            ch, "You can not transfer money to your own offline characters...");
        if (is_file == TRUE)
          free_char(vict);
        return (TRUE);
      }
      char_stat_mod(vict, "money_bank", amount);
      char_stat_mod(ch, "money_bank", -(amount + (amount / 100)));
      GET_PFILEPOS(vict) = player_i;
      mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
             "EXCHANGE: %s gave %s zenni to user %s", GET_NAME(ch),
             add_commas(amount), GET_NAME(vict));
      save_char(vict);
      if (is_file == TRUE)
        free_char(vict);
    } else {
      char_stat_mod(vict, "money_bank", amount);
      char_stat_mod(ch, "money_bank", -(amount + (amount / 100)));
      send_to_char(vict,
                   "@WYou have just had @Y%s@W zenni wired into your bank "
                   "account.@n\r\n",
                   add_commas(amount));
    }
    send_to_char(ch, "You transfer %s zenni to them.\r\n", add_commas(amount));
    act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("deposit")) {

    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that much zenni!\r\n");
      return (TRUE);
    }
    char_stat_mod(ch, "money", -amount);
    char_stat_mod(ch, "money_bank", amount);
    send_to_char(ch, "You deposit %d zenni.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("withdraw")) {

    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "The ATM is broken!\r\n");
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that much zenni!\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) - (amount + (1 + amount / 100)) < 0) {
      if (amount >= 100) {
        amount = amount + (amount / 100);
      } else if (amount < 100) {
        amount = amount + 1;
      }
      send_to_char(ch,
                   "You need at least %s in the bank with the 1 percent "
                   "withdraw fee.\r\n",
                   add_commas(amount));
      return (TRUE);
    }
    if (GET_GOLD(ch) + amount > GOLD_CARRY(ch)) {
      send_to_char(ch, "You can only carry %s zenni, you left the rest.\r\n",
                   add_commas(GOLD_CARRY(ch)));
      int diff = (GET_GOLD(ch) + amount) - GOLD_CARRY(ch);
      char_stat_set(ch, "money", GOLD_CARRY(ch));
      amount -= diff;
      if (amount >= 100) {
        num = amount / 100;
        char_stat_mod(ch, "money_bank", -(amount + num));
      } else if (amount < 100) {
        char_stat_mod(ch, "money_bank", -(amount + 1));
      }
      send_to_char(
          ch, "You withdraw %s zenni,  and pay %s in withdraw fees.\r\n.\r\n",
          add_commas(amount), add_commas(num));
      act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
      return (TRUE);
    }
    char_stat_mod(ch, "money", amount);
    if (amount >= 100) {
      num = amount / 100;
      char_stat_mod(ch, "money_bank", -(amount + num));
    } else if (amount < 100) {
      char_stat_mod(ch, "money_bank", -(amount + 1));
    }
    send_to_char(ch, "You withdraw %s zenni, and pay %s in withdraw fees.\r\n",
                 add_commas(amount), add_commas(num));
    act("$n makes a bank transaction.", TRUE, ch, 0, nullptr, TO_ROOM);
    return (TRUE);
  } else
    return (FALSE);
}
