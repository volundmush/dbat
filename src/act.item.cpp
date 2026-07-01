/* ************************************************************************
 *   File: act.item.c                                    Part of CircleMUD *
 *  Usage: object handling routines -- get/drop and container handling     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "act.item.h"

#include "character_db.h"
#include "consts/applies.h"
#include "consts/auction.h"
#include "consts/itemdata.h"
#include "consts/maximums.h"
#include "consts/pulse.h"
#include "consts/recipes.h"
#include "consts/roomflags.h"
#include "consts/search.h"
#include "consts/sectortypes.h"
#include "consts/skills.h"

#include "extract.h"
#include "iterate.hpp"
#include "interpreter.h"
#include "random.h"
#include "relocate.h"
#include "search.h"

#include "act.comm.h"
#include "act.informative.h"
#include "act.other.h"
#include "act.wizard.h"
#include "assemblies.h"
#include "boards.h"
#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "comm.h"
#include "config.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/applies.h"
#include "consts/constates.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/races.h"
#include "consts/sizes.h"
#include "consts/triggers.h"
#include "consts/weapons.h"
#include "consts/zoneflags.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dg_comm.h"
#include "dg_scripts.h"
#include "feats.h"
#include "flags.h"
#include "genzon.h"
#include "guild.h"
#include "handler.h"
#include "log.h"
#include "object_api.h"
#include "object_db.h"
#include "object_impl.h"
#include "object_macros.h"
#include "races.h"
#include "races_plus.h"
#include "room_api.h"
#include "room_db.h"
#include "room_utils.h"
#include "skills.h"
#include "spells.h"
#include "spec_procs.h"
#include "stringutils.h"
#include "util_macros.h"
#include "vehicles.h"
#include "zone_api.h"

/* global variables */
struct obj_data *obj_selling = NULL; /* current object for sale */
struct char_data *ch_selling = NULL; /* current character selling obj */
struct char_data *ch_buying = NULL;  /* current character buying the object */

/* local vvariables  */
static int curbid = 0;               /* current bid on item being auctioned */
static int aucstat = AUC_NULL_STATE; /* state of auction.. first_bid etc.. */

static const char *auctioneer[AUC_BID + 1] = {

    "@D[@CAUCTION@c: @C$n@W puts $p@W up for sale at @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @W$p@W at @Y%d@W zenni going once!@D]@n",
    "@D[@CAUCTION@c: @W$p@W at @Y%d@W zenni going twice!@D]@n",
    "@D[@CAUCTION@c: @WLast call: $p@W going for @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @WUnfortunately $p@W is unsold, returning it to $n. @D]@n",
    "@D[@CAUCTION@c: @WSOLD! $p@W to @C$n@W for @Y%d@W zenni!@D]@n",
    "@D[@CAUCTION@c: @WSorry, @C$n@W has cancelled the auction.@D]@n",
    "@D[@CAUCTION@c: @WSorry, @C$n@W has left us, the auction can't go "
    "on.@D]@n",
    "@D[@CAUCTION@c: @WSorry, $p@W has been confiscated, shame on you $n.@D]@n",
    "@D[@CAUCTION@c: @C$n@W is selling $p@W for @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @C$n@W bids @Y%d@W zenni on $p@W.@D]@n"};

/* local functions */
static int can_take_obj(struct char_data *ch, struct obj_data *obj);
static void get_check_money(struct char_data *ch, struct obj_data *obj);
static void get_from_room(struct char_data *ch, char *arg, int howmany);
static void perform_give_gold(struct char_data *ch, struct char_data *vict,
                              int amount);
static void perform_give(struct char_data *ch, struct char_data *vict,
                         struct obj_data *obj);
static int perform_drop(struct char_data *ch, struct obj_data *obj, int8_t mode,
                        const char *sname, struct room_data *RDR);
static void perform_drop_gold(struct char_data *ch, int amount, int8_t mode,
                              struct room_data *RDR);
static struct char_data *give_find_vict(struct char_data *ch, char *arg);
static void perform_put(struct char_data *ch, struct obj_data *obj,
                        struct obj_data *cont);
static void get_from_container(struct char_data *ch, struct obj_data *cont,
                               char *arg, int mode, int howmany);
static void wear_message(struct char_data *ch, struct obj_data *obj, int where);
static void perform_get_from_container(struct char_data *ch,
                                       struct obj_data *obj,
                                       struct obj_data *cont, int mode);
static int hands(struct char_data *ch);
static void start_auction(struct char_data *ch, struct obj_data *obj, int bid);
static void auc_stat(struct char_data *ch, struct obj_data *obj);
static void auc_send_to_all(char *messg, bool buyer);
static int has_housekey(struct char_data *ch, struct obj_data *obj);
static char *find_exdesc_keywords(char *word, struct extra_descr_data *list);

/* local variables */
static char buf[MAX_STRING_LENGTH];

// definitions

ACMD(do_refuel) {

  struct obj_data *controls;

  if (!(controls = find_control(ch))) {
    send_to_char(ch, "@wYou need to be in the cockpit to place a new fuel "
                     "canister into the ship.\r\n");
    return;
  }

  struct obj_data *fuel = char_inventory_search_vnum(ch, 17290, FALSE, 0);

  if (!fuel) {
    send_to_char(ch, "You do not have any fuel canisters on you.\r\n");
    return;
  }

  int max = 0;

  if (GET_OBJ_VNUM(controls) >= 44000 && GET_OBJ_VNUM(controls) <= 44199) {
    max = 300;
  } else if (GET_OBJ_VNUM(controls) >= 44200 &&
             GET_OBJ_VNUM(controls) <= 44499) {
    max = 500;
  } else if (GET_OBJ_VNUM(controls) >= 44200 &&
             GET_OBJ_VNUM(controls) <= 44999) {
    max = 1000;
  }

  if (GET_FUEL(controls) == max) {
    send_to_char(ch, "The ship is full on fuel!\r\n");
    return;
  } else {

    if (GET_FUEL(controls) + (GET_OBJ_WEIGHT(fuel) * 4) > max) {
      GET_FUEL(controls) = max;
    } else {
      GET_FUEL(controls) += (GET_OBJ_WEIGHT(fuel) * 4);
    }

    extract_obj(fuel);

    send_to_char(ch, "You place the fuel canister into the ship. Within "
                     "seconds the fuel has been extracted from the canister "
                     "into the ships' internal tanks.\r\n");
  }
}

static int has_housekey(struct char_data *ch, struct obj_data *obj) {

  bool found = false;
  char_inventory_iterate(ch, [&](auto obj2) {
    if (OBJ_FLAGGED(obj, ITEM_DUPLICATE))
      return true;
    if (GET_OBJ_VNUM(obj) == 18802) {
      if (GET_OBJ_VNUM(obj2) == 18800) {
        found = true;
        return false;
      }
    } else {
      if (GET_OBJ_VNUM(obj2) == GET_OBJ_VNUM(obj) - 1) {
        found = true;
        return false;
      }
    }
    return true;
  });

  if (found) return 1;
  return (0);
}

ACMD(do_pack) {

  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(
        ch, "Pack up which type of house capsule?\nSyntax: pack (target)\r\n");
    return;
  }

  if (!(obj =
            get_obj_in_list_vis(ch, arg, NULL, inv_for_room(char_room_get(ch))))) {
    send_to_char(ch, "That house item doesn't seem to be around.\r\n");
    return;
  } else {
    struct obj_data *packed = NULL;
    if ((GET_OBJ_VNUM(obj) >= 19090 && GET_OBJ_VNUM(obj) <= 19099) ||
        GET_OBJ_VNUM(obj) == 11) {
      act("@CYou push a hidden button on $p@C and a cloud of smoke erupts and "
          "covers it. As the smoke clears a small capsule can be seen on the "
          "ground.@n",
          TRUE, ch, obj, 0, TO_CHAR);
      act("@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts "
          "and covers it. As the smoke clears a small capsule can be seen on "
          "the ground.@n",
          TRUE, ch, obj, 0, TO_ROOM);
      if (GET_OBJ_VNUM(obj) == 11) {
        extract_obj(obj);
        packed = read_object(19085, VIRTUAL);
        obj_to_room(packed, char_room_get(ch));
      } else {
        int fnum = GET_OBJ_VNUM(obj) - 10;
        packed = read_object(fnum, VIRTUAL);
        extract_obj(obj);
        obj_to_room(packed, char_room_get(ch));
      }
      return;
    } else if (GET_OBJ_VNUM(obj) >= 18800 && GET_OBJ_VNUM(obj) <= 19199 &&
               GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
      if (!*arg2) {
        send_to_char(
            ch, "This will sell off your house and delete everything inside. "
                "Are you sure? If you are then enter the command again with a "
                "yes at the end.\nSyntax: pack (house) yes\r\n");
        return;
      } else if (strcasecmp(arg2, "yes")) {
        send_to_char(
            ch, "This will sell off your house and delete everything inside. "
                "Are you sure? If you are then enter the command again with a "
                "yes at the end.\nSyntax: pack (house) yes\r\n");
        return;
      } else if (has_housekey(ch, obj) == 0) {
        send_to_char(ch, "You do not own this house.\r\n");
        return;
      } else {
        struct obj_data *cont = NULL;
        act("@CYou push a hidden button on $p@C and a cloud of smoke erupts "
            "and covers it. As the smoke clears a pile of money can be seen on "
            "the ground!@n",
            TRUE, ch, obj, 0, TO_CHAR);
        act("@c$n@C pushes a hidden button on $p@C and a cloud of smokes "
            "erupts and covers it. As the smoke clears a pile of money can be "
            "seen on the ground!@n",
            TRUE, ch, obj, 0, TO_ROOM);
        int money = 0, count = 0, rnum = GET_OBJ_VNUM(obj);
        if (GET_OBJ_VNUM(obj) >= 18800 && GET_OBJ_VNUM(obj) <= 18899) {
          if (rnum == 18802) {
            rnum = 18800;
          } else {
            rnum = rnum - 1;
          }
          money = 65000;
          while (count < 4) {
            while (room_contents_get(room_by_id(rnum)))
              extract_obj(room_contents_get(room_by_id(rnum)));
            count++;
            rnum++;
          }
        } else if (GET_OBJ_VNUM(obj) >= 18900 && GET_OBJ_VNUM(obj) <= 18999) {
          rnum = rnum - 1;
          money = 150000;
          while (count < 4) {
            while (room_contents_get(room_by_id(rnum)))
              extract_obj(room_contents_get(room_by_id(rnum)));
            count++;
            rnum++;
          }
        } else if (GET_OBJ_VNUM(obj) >= 19100 && GET_OBJ_VNUM(obj) <= 19199) {
          rnum = rnum - 1;
          money = 1000000;
          while (count < 4) {
            while (room_contents_get(room_by_id(rnum)))
              extract_obj(room_contents_get(room_by_id(rnum)));
            count++;
            rnum++;
          }
        }
        char_inventory_iterate(ch, [&](auto obj2) {
          if (GET_OBJ_VNUM(obj) == 18802) {
            if (GET_OBJ_VNUM(obj2) == 18800) {
              extract_obj(obj2);
            }
          } else {
            if (GET_OBJ_VNUM(obj2) == GET_OBJ_VNUM(obj) - 1) {
              extract_obj(obj2);
            }
          }
          return true;
        });
        struct obj_data *money_obj = create_money(money);
        obj_to_room(money_obj, char_room_get(ch));
        extract_obj(obj);
        return;
      }
    } else {
      send_to_char(ch, "That isn't something you can pack up!\r\n");
      return;
    }
  }
}

int check_insidebag(struct obj_data *cont, double mult) {

  struct obj_data *inside = NULL, *next_obj2 = NULL;
  int count = 0, containers = 0;

  obj_contents_iterate(cont, [&](struct obj_data *inside) {
    if (GET_OBJ_TYPE(inside) == ITEM_CONTAINER) {
      count++;
      count += check_insidebag(inside, mult);
      containers++;
    } else {
      count++;
    }
    return true;
  });

  count = count * mult;
  count += containers;

  return (count);
}

int check_saveroom_count(struct char_data *ch, struct obj_data *cont) {
  struct obj_data *obj, *next_obj = NULL;
  int count = 0, was = 0;

  if (char_room_get(ch) == NULL)
    return 0;
  else if (!(char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_HOUSE)))
    return 0;

  room_contents_iterate(char_room_get(ch), [&](auto obj) {
    count++;
    if (!OBJ_FLAGGED(obj, ITEM_CARDCASE)) {
      count += check_insidebag(obj, 0.5);
    }
    return true;
  });

  was = count;

  if (cont != NULL) {
    if (!OBJ_FLAGGED(cont, ITEM_CARDCASE)) {
      count += check_insidebag(cont, 0.5);
    }
    count++;
  }

  return (count);
}

ACMD(do_deploy) {

  struct obj_data *obj3, *obj = NULL;
  int capsule = FALSE, furniture = FALSE;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    char_inventory_iterate(ch, [&](auto obj4) {
      if (GET_OBJ_VNUM(obj4) == 4 || GET_OBJ_VNUM(obj4) == 5 ||
          GET_OBJ_VNUM(obj4) == 6) {
        obj = obj4;
        capsule = TRUE;
      }
      return true;
    });
  } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
    send_to_char(ch, "Syntax: deploy (no argument for houses)\nSyntax: deploy "
                     "(target) <-- For furniture\r\n");
    return;
  }

  if (capsule == FALSE && obj) {
    if (GET_OBJ_VNUM(obj) >= 19080 && GET_OBJ_VNUM(obj) <= 19099) {
      capsule = TRUE;
      furniture = TRUE;
    } else {
      send_to_char(ch, "That is not a furniture capsule!\r\n");
      return;
    }
  }

  struct room_data *room = char_room_get(ch);
  int sect = room_sector_type_get(room);

  if (capsule == FALSE) {
    send_to_char(ch,
                 "You do not have any house type capsules to deploy.@n\r\n");
    return;
  } else if (GET_RP(ch) < 10 && furniture == FALSE) {
    send_to_char(ch, "You are required to have (not spend) 10 RPP in order to "
                     "place a house.\r\n");
    return;
  } else if (furniture == TRUE && (!room_flagged(room, ROOM_HOUSE) ||
                                   room_flagged(room, ROOM_SHIP))) {
    send_to_char(ch, "You can't deploy house furniture capsules here.\r\n");
    return;
  } else if (furniture == TRUE && (room_flagged(room, ROOM_GARDEN1) ||
                                   room_flagged(room, ROOM_GARDEN2))) {
    send_to_char(ch, "You can't deploy house furniture capsules here.\r\n");
    return;
  } else if (furniture == FALSE &&
             (sect == SECT_INSIDE || sect == SECT_WATER_NOSWIM ||
              sect == SECT_WATER_SWIM || sect == SECT_SPACE)) {
    send_to_char(ch, "You can not deploy that in this kind of area. Try an "
                     "area more suitable for a house.\r\n");
    return;
  }

  if (furniture == TRUE) {
    int fnum = 0;
    if (GET_OBJ_VNUM(obj) == 19080) {
      fnum = 19090;
    } else if (GET_OBJ_VNUM(obj) == 19081) {
      fnum = 19091;
    } else if (GET_OBJ_VNUM(obj) == 19082) {
      fnum = 19092;
    } else if (GET_OBJ_VNUM(obj) == 19083) {
      fnum = 19093;
    } else if (GET_OBJ_VNUM(obj) == 19085) {
      fnum = 11;
    }
    if (fnum != 0) {
      struct obj_data *furn = read_object(fnum, VIRTUAL);
      act("@CYou click the capsule's button and toss it to the floor. A puff "
          "of smoke erupts immediately and quickly dissipates to reveal, "
          "$p@C.@n",
          TRUE, ch, furn, 0, TO_CHAR);
      act("@c$n@C clicks a capsule's button and tosses it to the floor. A puff "
          "of smoke erupts immediately and quickly dissipates to reveal, "
          "$p@C.@n",
          TRUE, ch, furn, 0, TO_ROOM);
      obj_to_room(furn, char_room_get(ch));
      extract_obj(obj);
      return;
    } else {
      send_to_imm("ERROR: Furniture failed to deploy at %d.",
                  char_room_vnum_get(ch));
      return;
    }
  }

  int rnum = 18800, giveup = FALSE, cont = FALSE, found = FALSE, type = 0;

  if (GET_OBJ_VNUM(obj) == 4) {
    type = 0;
  } else if (GET_OBJ_VNUM(obj) == 5) {
    rnum = 18900;
    type = 1;
  } else if (GET_OBJ_VNUM(obj) == 6) {
    rnum = 19100;
    type = 2;
  }

  int final = rnum + 99;

  while (giveup == FALSE && cont == FALSE) {
    room_contents_iterate(room_by_id(rnum), [&](auto obj3) {
      if (GET_OBJ_VNUM(obj3) == 18801) {
        found = TRUE;
      }
      return true;
    });
    if (found == TRUE && rnum < final) {
      if (type == 0) {
        rnum += 4;
      } else {
        rnum += 5;
      }
      found = FALSE;
    } else if (rnum >= final) {
      giveup = TRUE;
    } else {
      cont = TRUE;
    }
  } /* End while */

  if (cont == TRUE) {
    int hnum = char_room_vnum_get(ch);
    struct obj_data *door = read_object(18801, VIRTUAL);

    GET_OBJ_VAL(door, 6) = char_room_vnum_get(ch);
    if (rnum != 18800)
      GET_OBJ_VAL(door, 0) = rnum + 1;
    else
      GET_OBJ_VAL(door, 0) = 18802;
    GET_OBJ_VAL(door, 2) = rnum;
    obj_to_room(door, room_by_id(rnum));
    struct obj_data *key = read_object(rnum, VIRTUAL);
    obj_to_char(key, ch);
    act("@WYou click the capsule and toss it to the ground. A large cloud of "
        "smoke erupts from the capsule and after it clears a house is visible "
        "in its place!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W clicks a capsule and then tosses it to the ground. A large "
        "cloud of smoke erupts from the capsule and after it clears a house is "
        "visible in its place!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    struct obj_data *foun = read_object(18803, VIRTUAL);
    obj_to_room(foun, room_by_id(rnum + 1));
    extract_obj(obj);
  } else {
    send_to_char(ch,
                 "@ROOC@D: @wSorry for the inconvenience, but it appears there "
                 "are no houses available. Please contact Iovan.@n\r\n");
    return;
  }
}

ACMD(do_twohand) {

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
    send_to_char(ch, "You are busy grappling with someone!\r\n");
    return;
  }

  if (ABSORBING(ch) || ABSORBBY(ch)) {
    send_to_char(ch, "You are busy struggling with someone!\r\n");
    return;
  }

  if (!GET_EQ(ch, WEAR_WIELD1) && !PLR_FLAGGED(ch, PLR_THANDW)) {
    send_to_char(ch, "You need to wield a sword to use this.\r\n");
    return;
  } else if (GET_EQ(ch, WEAR_WIELD2) && !PLR_FLAGGED(ch, PLR_THANDW)) {
    send_to_char(ch, "You have something in your offhand already and can't two "
                     "hand wield your main weapon.\r\n");
    return;
  } else if ((GET_LIMBCOND(ch, 1) <= 0 || GET_LIMBCOND(ch, 2) <= 0) &&
             !PLR_FLAGGED(ch, PLR_THANDW)) {
    send_to_char(ch, "Kind of hard with only one arm...\r\n");
    return;
  } else if (PLR_FLAGGED(ch, PLR_THANDW)) {
    send_to_char(ch, "You stop wielding your weapon with both hands.\r\n");
    act("$n stops wielding $s weapon with both hands.", TRUE, ch, 0, 0,
        TO_ROOM);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THANDW);
    return;
  } else {
    send_to_char(ch, "You grab your weapon with both hands.\r\n");
    act("$n starts wielding $s weapon with both hands.", TRUE, ch, 0, 0,
        TO_ROOM);
    SET_BIT_AR(PLR_FLAGS(ch), PLR_THANDW);
    return;
  }
}

static void start_auction(struct char_data *ch, struct obj_data *obj, int bid) {
  /* Take object from character and set variables */

  obj_from_char(obj);
  obj_selling = obj;
  ch_selling = ch;
  ch_buying = NULL;
  curbid = bid;

  /* Tell th character where his item went */
  sprintf(buf, "%s magicly flies away from your hands to be auctioned!\r\n",
          obj_selling->short_description);
  CAP(buf);
  send_to_char(ch_selling, "%s", buf);

  /* Anounce the item is being sold */
  sprintf(buf, auctioneer[AUC_NULL_STATE], curbid);
  auc_send_to_all(buf, FALSE);

  aucstat = AUC_OFFERING;
}

void check_auction(void) {
  switch (aucstat) {
  case AUC_NULL_STATE:
    return;
  case AUC_OFFERING: {
    if (obj_selling == NULL) {
      auc_send_to_all("@RThe auction has stopped because someone has made off "
                      "with the auctioned object!@n\r\n",
                      FALSE);
      curbid = 0;
      ch_selling = NULL;
      ch_buying = NULL;
      aucstat = AUC_NULL_STATE;
      return;
    }
    sprintf(buf, auctioneer[AUC_OFFERING], curbid);
    CAP(buf);
    auc_send_to_all(buf, FALSE);
    aucstat = AUC_GOING_ONCE;
    return;
  }
  case AUC_GOING_ONCE: {
    if (obj_selling == NULL) {
      auc_send_to_all("@RThe auction has stopped because someone has made off "
                      "with the auctioned object!@n\r\n",
                      FALSE);
      curbid = 0;
      ch_selling = NULL;
      ch_buying = NULL;
      aucstat = AUC_NULL_STATE;
      return;
    }

    sprintf(buf, auctioneer[AUC_GOING_ONCE], curbid);
    CAP(buf);
    auc_send_to_all(buf, FALSE);
    aucstat = AUC_GOING_TWICE;
    return;
  }
  case AUC_GOING_TWICE: {
    if (obj_selling == NULL) {
      auc_send_to_all("@RThe auction has stopped because someone has made off "
                      "with the auctioned object!@n\r\n",
                      FALSE);
      curbid = 0;
      ch_selling = NULL;
      ch_buying = NULL;
      aucstat = AUC_NULL_STATE;
      return;
    }

    sprintf(buf, auctioneer[AUC_GOING_TWICE], curbid);
    CAP(buf);
    auc_send_to_all(buf, FALSE);
    aucstat = AUC_LAST_CALL;
    return;
  }
  case AUC_LAST_CALL: {
    if (obj_selling == NULL) {
      auc_send_to_all("@RThe auction has stopped because someone has made off "
                      "with the auctioned object!@n\r\n",
                      FALSE);
      curbid = 0;
      ch_selling = NULL;
      ch_buying = NULL;
      aucstat = AUC_NULL_STATE;
      return;
    }
    if (ch_buying == NULL) {

      sprintf(buf, auctioneer[AUC_LAST_CALL], curbid);

      CAP(buf);
      auc_send_to_all(buf, FALSE);

      sprintf(buf, "%s flies out the sky and into your hands.\r\n",
              obj_selling->short_description);
      CAP(buf);
      send_to_char(ch_selling, "%s", buf);
      obj_to_char(obj_selling, ch_selling);

      /* Reset auctioning values */
      obj_selling = NULL;
      ch_selling = NULL;
      ch_buying = NULL;
      curbid = 0;
      aucstat = AUC_NULL_STATE;
      return;
    } else {

      sprintf(buf, auctioneer[AUC_SOLD], curbid);
      auc_send_to_all(buf, TRUE);

      /* Give the object to the buyer */
      obj_to_char(obj_selling, ch_buying);
      sprintf(buf,
              "%s flies out the sky and into your hands, what a steal!\r\n",
              obj_selling->short_description);
      CAP(buf);
      send_to_char(ch_buying, "%s", buf);

      sprintf(buf, "Congrats! You have sold %s for @Y%d@W zenni!\r\n",
              obj_selling->short_description, curbid);
      send_to_char(ch_selling, "%s", buf);

      /* Give selling char the money for his stuff */
      if (GET_GOLD(ch_selling) + curbid > GOLD_CARRY(ch_selling)) {
        send_to_char(ch_buying, "You couldn't hold all the zenni, so some of "
                                "it was deposited for you.\r\n");
        int diff = 0;
        diff = (GET_GOLD(ch_selling) + curbid) - GOLD_CARRY(ch_selling);
        char_stat_set(ch_selling, "money", GOLD_CARRY(ch_selling));
        char_stat_mod(ch_selling, "money_bank", diff);
      } else if (GET_GOLD(ch_selling) + curbid <= GOLD_CARRY(ch_selling)) {
        char_stat_mod(ch_selling, "money", curbid);
      }
      /* Reset auctioning values */
      obj_selling = NULL;
      ch_selling = NULL;
      ch_buying = NULL;
      curbid = 0;
      aucstat = AUC_NULL_STATE;
      return;
    }
  }
  }
}

static bool _db_planet(room_vnum room) {
  struct room_data *rm = room_by_id(room);

  for (auto flag : {ROOM_EARTH, ROOM_VEGETA, ROOM_FRIGID, ROOM_AETHER,
                    ROOM_NAMEK, ROOM_KONACK, ROOM_YARDRAT}) {
    if (room_flagged(rm, flag)) {
      return true;
    }
  }
  return false;
}

void dball_load() {
  int found1 = FALSE, found2 = FALSE, found3 = FALSE;
  int found4 = FALSE, found5 = FALSE, load = FALSE, num = -1;
  int found6 = FALSE, found7 = FALSE, room = 0, loaded = FALSE;
  int hunter1 = FALSE, hunter2 = FALSE;
  struct obj_data *k;

  if (SELFISHMETER >= 10) {
    return;
  }

  if (dballtime == 0) {
    struct char_data *hunter = NULL;
    struct mob_proto_data *proto = NULL;

    WISHTIME = 0;
    obj_iterate_all([&](struct obj_data *o) {
      if (OBJ_FLAGGED(o, ITEM_FORGED)) {
        return true;
      }
      if (GET_OBJ_VNUM(o) == 20) {
        found1 = TRUE;
      } else if (GET_OBJ_VNUM(o) == 21) {
        found2 = TRUE;
      } else if (GET_OBJ_VNUM(o) == 22) {
        found3 = TRUE;
      } else if (GET_OBJ_VNUM(o) == 23) {
        found4 = TRUE;
      } else if (GET_OBJ_VNUM(o) == 24) {
        found5 = TRUE;
      } else if (GET_OBJ_VNUM(o) == 25) {
        found6 = TRUE;
      } else if (GET_OBJ_VNUM(o) == 26) {
        found7 = TRUE;
      } else if (obj_room_get(o) != NULL &&
                 room_geffect_get(obj_room_get(o)) == 6 &&
                 !OBJ_FLAGGED(o, ITEM_UNBREAKABLE)) {
        send_to_room(obj_room_get(o), "@R%s@r melts in the lava!@n\r\n",
                     o->short_description);
        extract_obj(o);
      }
      return true;
    });
    if (found1 == FALSE) {
      load = FALSE;
      int zone = 0;
      while (load == FALSE) {
        if (auto room = room_by_id(num); room) {
          if (auto zone = room_zone_get(room); zone) {
            if (zone_flagged(zone, ZONE_DBALLS)) {
              // room = num;
              load = TRUE;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(200, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(20, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(20, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(20, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(20, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    if (found2 == FALSE) {
      load = FALSE;
      while (load == FALSE) {
        if (room_by_id(num)) {
          if (_db_planet(num)) {
            room = num;
            load = TRUE;
            num = rand_number(200, 20000);
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(20, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(21, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(21, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(21, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(21, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    if (found3 == FALSE) {
      load = FALSE;
      while (load == FALSE) {
        if (room_by_id(num)) {
          if (_db_planet(num)) {
            room = num;
            load = TRUE;
            num = rand_number(200, 20000);
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(20, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(22, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(22, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(22, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(22, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    if (found4 == FALSE) {
      load = FALSE;
      while (load == FALSE) {
        if (room_by_id(num)) {
          if (_db_planet(num)) {
            room = num;
            load = TRUE;
            num = rand_number(200, 20000);
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(20, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(23, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(23, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(23, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(23, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    if (found5 == FALSE) {
      load = FALSE;
      while (load == FALSE) {
        if (room_by_id(num)) {
          if (_db_planet(num)) {
            room = num;
            load = TRUE;
            num = rand_number(200, 20000);
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(20, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(24, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(24, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(24, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(24, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    if (found6 == FALSE) {
      load = FALSE;
      while (load == FALSE) {
        if (room_by_id(num)) {
          if (_db_planet(num)) {
            room = num;
            load = TRUE;
            num = rand_number(200, 20000);
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(20, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(25, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(25, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(25, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(25, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    if (found7 == FALSE) {
      load = FALSE;
      while (load == FALSE) {
        if (room_by_id(num)) {
          if (_db_planet(num)) {
            room = num;
            load = TRUE;
            num = rand_number(200, 20000);
          } else {
            num = rand_number(200, 20000);
          }
        } else {
          num = rand_number(20, 20000);
        }
      }
      if (rand_number(1, 10) > 8) {
        if (hunter1 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER1_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER1_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter1 = TRUE;
          DBALL_HUNTER1 = room;
          k = read_object(26, VIRTUAL);
          obj_to_char(k, hunter);
        } else if (hunter2 == FALSE) {
          if (!(proto = mob_proto_by_id(DBALL_HUNTER2_VNUM))) {
            return;
          }
          hunter = read_mobile(DBALL_HUNTER2_VNUM, VIRTUAL);
          char_to_room(hunter, room_by_id(room));
          hunter2 = TRUE;
          DBALL_HUNTER2 = room;
          k = read_object(26, VIRTUAL);
          obj_to_char(k, hunter);
        } else {
          k = read_object(26, VIRTUAL);
          obj_to_room(k, room_by_id(room));
        }
      } else {
        k = read_object(26, VIRTUAL);
        obj_to_room(k, room_by_id(room));
      }
      loaded = TRUE;
    }
    dballtime = 604800;
  } else if (dballtime == 518400 || dballtime == 432000 ||
             dballtime == 345600 || dballtime == 259200 ||
             dballtime == 172800 || dballtime == 86400) {
    dballtime -= 1;
  } else {
    if (WISHTIME == 0) {
      WISHTIME = dballtime - 1;
    } else if (WISHTIME > 0 && dballtime != WISHTIME) {
      dballtime = WISHTIME;
    }
    WISHTIME -= 1;
    dballtime -= 1;
  }
}

ACMD(do_auction) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int bid = 0;

  two_arguments(argument, arg1, arg2);

  struct room_data *room = char_room_get(ch);

  if (room_flagged(room, ROOM_HBTC)) {
    send_to_char(ch, "This is a different dimension!\r\n");
    return;
  }
  if (room_flagged(room, ROOM_PAST)) {
    send_to_char(ch, "You are in the past!\r\n");
    return;
  }
  if (PRF_FLAGGED(ch, PRF_HIDE)) {
    send_to_char(
        ch, "The auctioneer will not accept items from hidden people.\r\n");
    return;
  }

  if (!*arg1) {
    send_to_char(ch, "Auction what?\r\n");
    send_to_char(ch, "[ Auction: <item> | <cancel> ]\r\n");
    return;
  } else if (is_abbrev(arg1, "cancel") || is_abbrev(arg1, "stop")) {
    if ((ch != ch_selling && GET_ADMLEVEL(ch) <= ADMLVL_GRGOD) ||
        aucstat == AUC_NULL_STATE) {
      send_to_char(ch, "You're not even selling anything!\r\n");
      return;
    } else if (ch == ch_selling) {
      stop_auction(AUC_NORMAL_CANCEL, NULL);
      return;
    } else {
      stop_auction(AUC_WIZ_CANCEL, ch);
    }
  } else if (is_abbrev(arg1, "stats") || is_abbrev(arg1, "identify")) {
    auc_stat(ch, obj_selling);
    return;
  } else if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, inv_for_char(ch)))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    send_to_char(ch, "%s", buf);
    return;
  } else if (!*arg2) {
    sprintf(buf, "What should be the minimum bid?\r\n");
    send_to_char(ch, "%s", buf);
    return;
  } else if (*arg2 && (bid = atoi(arg2)) <= 0) {
    send_to_char(ch, "Come on? One zenni at least?\r\n");
    return;
  } else if (aucstat != AUC_NULL_STATE) {
    sprintf(buf, "Sorry, but %s is already auctioning %s at @Y%d@W zenni!\r\n",
            GET_NAME(ch_selling), obj_selling->short_description, bid);
    send_to_char(ch, "%s", buf);
    return;
  } else if (OBJ_FLAGGED(obj, ITEM_NOSELL)) {
    send_to_char(ch, "Sorry but you can't sell that!\r\n");
    return;
  } else if (GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) == 1) {
    send_to_char(ch, "Sorry but you can't sell that!\r\n");
    return;
  } else {
    send_to_char(ch, "Ok.\r\n");
    start_auction(ch, obj, bid);
    return;
  }
}

ACMD(do_bid) {
  struct obj_data *obj, *next_obj, *obj2 = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int found = FALSE, list = 0, masterList = 0;
  struct room_data *auct_room = room_by_id(80);

  if (IS_NPC(ch))
    return;

  if (!GET_EQ(ch, WEAR_EYE)) {
    send_to_char(ch, "You need a scouter to make an auction bid.\r\n");
    return;
  }

  struct room_data *room = char_room_get(ch);

  if (room_flagged(room, ROOM_HBTC)) {
    send_to_char(ch, "This is a different dimension!\r\n");
    return;
  }
  if (room_flagged(room, ROOM_PAST)) {
    send_to_char(ch, "This is the past, nothing is being auctioned!\r\n");
    return;
  }

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid "
                     "appraise (list number)\r\n");
    return;
  }
  room_contents_iterate(auct_room, [&](auto obj) {
    if (obj) {
      list++;
    }
    return true;
  });
  masterList = list;
  list = 0;

  if (!strcasecmp(arg, "list")) {
    send_to_char(ch, "@Y                                   Auction@n\r\n");
    send_to_char(ch, "@c-------------------------------------------------------"
                     "-----------------------@n\r\n");
    room_contents_iterate(auct_room, [&](auto obj) {
      if (obj) {
        if (GET_AUCTER(obj) <= 0) {
          return true;
        }
        list++;
        if (GET_AUCTIME(obj) + 86400 > time(0) && GET_CURBID(obj) <= -1) {
          send_to_char(ch,
                       "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: "
                       "@w%-*s@D][@GCost@W: @Y%s@D]@n\r\n",
                       list,
                       get_name_by_id(GET_AUCTER(obj)) != NULL
                           ? CAP(get_name_by_id(GET_AUCTER(obj)))
                           : "Nobody",
                       count_color_chars(obj->short_description) + 30,
                       obj->short_description, add_commas(GET_BID(obj)));
        } else if (GET_AUCTIME(obj) + 86400 > time(0) && GET_CURBID(obj) > -1) {
          send_to_char(ch,
                       "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: "
                       "@w%-*s@D][@RTop Bid@W: %s @Y%s@D]@n\r\n",
                       list,
                       get_name_by_id(GET_AUCTER(obj)) != NULL
                           ? CAP(get_name_by_id(GET_AUCTER(obj)))
                           : "Nobody",
                       count_color_chars(obj->short_description) + 30,
                       obj->short_description,
                       get_name_by_id(GET_CURBID(obj)) != NULL
                           ? CAP(get_name_by_id(GET_CURBID(obj)))
                           : "Nobody",
                       add_commas(GET_BID(obj)));
        } else if (GET_AUCTIME(obj) + 86400 < time(0) && GET_CURBID(obj) > -1) {
          send_to_char(ch,
                       "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: "
                       "@w%-*s@D][@RBid Winner@W: %s @Y%s@D]@n\r\n",
                       list,
                       get_name_by_id(GET_AUCTER(obj)) != NULL
                           ? CAP(get_name_by_id(GET_AUCTER(obj)))
                           : "Nobody",
                       count_color_chars(obj->short_description) + 30,
                       obj->short_description,
                       get_name_by_id(GET_CURBID(obj)) != NULL
                           ? CAP(get_name_by_id(GET_CURBID(obj)))
                           : "Nobody",
                       add_commas(GET_BID(obj)));
        } else {
          send_to_char(ch,
                       "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: "
                       "@w%-*s@D][@RClosed@D]@n\r\n",
                       list,
                       get_name_by_id(GET_AUCTER(obj)) != NULL
                           ? CAP(get_name_by_id(GET_AUCTER(obj)))
                           : "Nobody",
                       count_color_chars(obj->short_description) + 30,
                       obj->short_description);
        }
        found = TRUE;
      }
      return true;
    });
    if (found == FALSE) {
      send_to_char(ch, "No items are currently being auctioned.\r\n");
    }
    send_to_char(ch, "@c-------------------------------------------------------"
                     "-----------------------@n\r\n");
  } else if (!strcasecmp(arg, "appraise")) {
    if (!*arg2) {
      send_to_char(ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid "
                       "appraise (list number)\r\n");
      send_to_char(ch, "What item number did you want to appraise?\r\n");
      return;
    } else if (atoi(arg2) < 0 || atoi(arg2) > masterList) {
      send_to_char(ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid "
                       "appraise (list number)\r\n");
      send_to_char(ch, "That item number doesn't exist.\r\n");
      return;
    }

    room_contents_iterate(auct_room, [&](auto obj) {
      if (obj) {
        if (GET_AUCTER(obj) <= 0) {
          return true;
        }
        list++;
        if (atoi(arg2) == list) {
          obj2 = obj;
        }
      }
      return true;
    });
    if (!obj2) {
      send_to_char(ch, "That item number is not found.\r\n");
      return;
    } else {
      if (!GET_SKILL(ch, SKILL_APPRAISE)) {
        send_to_char(ch, "You are unskilled at appraising.\r\n");
        return;
      }
      improve_skill(ch, SKILL_APPRAISE, 1);
      if (GET_SKILL(ch, SKILL_APPRAISE) < rand_number(1, 101)) {
        send_to_char(ch,
                     "You look at the images for %s and fail to perceive its "
                     "worth..\r\n",
                     obj2->short_description);
        act("@c$n@w looks stumped about something they viewed on their scouter "
            "screen.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
      } else {
        send_to_char(ch,
                     "You look at images of the object on your scouter.\r\n");
        act("@c$n@w looks at something on their scouter screen.@n", TRUE, ch, 0,
            0, TO_ROOM);
        send_to_char(ch, "@c---------------------------------------------------"
                         "---------------------\n");
        send_to_char(ch, "@GOwner       @W: @w%s@n\n",
                     get_name_by_id(GET_AUCTER(obj2)) != NULL
                         ? CAP(get_name_by_id(GET_AUCTER(obj2)))
                         : "Nobody");
        send_to_char(ch, "@GItem Name   @W: @w%s@n\n", obj2->short_description);
        send_to_char(ch, "@GCurrent Bid @W: @Y%s@n\n",
                     add_commas(GET_BID(obj2)));
        send_to_char(ch, "@GStore Value @W: @Y%s@n\n",
                     add_commas(GET_OBJ_COST(obj2)));
        send_to_char(ch, "@GItem Min LVL@W: @w%d@n\n", GET_OBJ_LEVEL(obj2));
        if (GET_OBJ_VAL(obj2, VAL_ALL_HEALTH) >= 100) {
          send_to_char(ch, "@GCondition   @W: @C%d%s@n\n",
                       GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
        } else if (GET_OBJ_VAL(obj2, VAL_ALL_HEALTH) >= 50) {
          send_to_char(ch, "@GCondition   @W: @y%d%s@n\n",
                       GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
        } else if (GET_OBJ_VAL(obj2, VAL_ALL_HEALTH) >= 1) {
          send_to_char(ch, "@GCondition   @W: @r%d%s@n\n",
                       GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
        } else {
          send_to_char(ch, "@GCondition   @W: @D%d%s@n\n",
                       GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
        }
        send_to_char(ch, "@GItem Weight @W: @w%s@n\n",
                     add_commas(GET_OBJ_WEIGHT(obj2)));
        char bits[MAX_STRING_LENGTH];
        sprintbitarray(GET_OBJ_WEAR(obj2), wear_bits, TW_ARRAY_MAX, bits,
                       sizeof(bits));
        search_replace(bits, "TAKE", "");
        send_to_char(ch, "@GWear Loc.   @W:@w%s\n", bits);
        if (GET_OBJ_TYPE(obj2) == ITEM_WEAPON) {
          if (OBJ_FLAGGED(obj2, ITEM_WEAPLVL1)) {
            send_to_char(ch,
                         "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: "
                         "@D[@w5%s@D]@n\r\n",
                         "%");
          } else if (OBJ_FLAGGED(obj2, ITEM_WEAPLVL2)) {
            send_to_char(ch,
                         "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: "
                         "@D[@w10%s@D]@n\r\n",
                         "%");
          } else if (OBJ_FLAGGED(obj2, ITEM_WEAPLVL3)) {
            send_to_char(ch,
                         "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: "
                         "@D[@w20%s@D]@n\r\n",
                         "%");
          } else if (OBJ_FLAGGED(obj2, ITEM_WEAPLVL4)) {
            send_to_char(ch,
                         "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: "
                         "@D[@w30%s@D]@n\r\n",
                         "%");
          } else if (OBJ_FLAGGED(obj2, ITEM_WEAPLVL5)) {
            send_to_char(ch,
                         "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: "
                         "@D[@w50%s@D]@n\r\n",
                         "%");
          }
        }
        int i, found = FALSE;
        send_to_char(ch, "@GItem Size   @W:@w %s@n\r\n",
                     size_names[GET_OBJ_SIZE(obj2)]);
        send_to_char(ch, "@GItem Bonuses@W:@w");
        for (i = 0; i < MAX_OBJ_AFFECT; i++) {
          if (obj2->affected[i].modifier) {
            sprinttype(obj2->affected[i].location, apply_types, buf,
                       sizeof(buf));
            send_to_char(ch, "%s %+d to %s", found++ ? "," : "",
                         obj2->affected[i].modifier, buf);
            switch (obj2->affected[i].location) {
            case APPLY_FEAT:
              send_to_char(ch, " (%s)",
                           feat_list[obj2->affected[i].specific].name);
              break;
            case APPLY_SKILL:
              send_to_char(ch, " (%s)",
                           spell_info[obj2->affected[i].specific].name);
              break;
            }
          }
        }
        if (!found)
          send_to_char(ch, " None@n");
        else
          send_to_char(ch, "@n");
        char buf2[MAX_STRING_LENGTH];
        sprintbitarray(GET_OBJ_PERM(obj2), affected_bits, AF_ARRAY_MAX, buf2,
                       sizeof(buf2));
        send_to_char(ch, "\n@GSpecial     @W:@w %s\n", buf2);
        send_to_char(ch, "@c---------------------------------------------------"
                         "---------------------\n");
        return;
      }
    }
  } else {
    if (!*arg2) {
      send_to_char(ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid "
                       "appraise (list number)\r\n");
      send_to_char(ch, "What amount did you want to bid?\r\n");
      return;
    } else if (atoi(arg) < 0 || atoi(arg) > masterList) {
      send_to_char(ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid "
                       "appraise (list number)\r\n");
      send_to_char(ch, "That item number is not found.\r\n");
      return;
    }

    room_contents_iterate(auct_room, [&](auto obj) {
      if (obj) {
        if (GET_AUCTER(obj) <= 0) {
          return true;
        }
        list++;
        if (atoi(arg) == list) {
          obj2 = obj;
        }
      }
      return true;
    });

    if (!obj2) {
      send_to_char(ch, "That item number is not found.\r\n");
      return;
    } else if (GET_CURBID(obj2) == GET_ID(ch)) {
      send_to_char(ch, "You already have the highest bid.\r\n");
      return;
    } else if (GET_AUCTER(obj2) == GET_ID(ch)) {
      send_to_char(ch, "You auctioned the item, go to the auction house and "
                       "cancel if you can.\r\n");
      return;
    } else if (GET_CURBID(obj2) > 0 &&
               atoi(arg2) <= (GET_BID(obj2) + (GET_BID(obj2) * .1)) &&
               GET_CURBID(obj2) > -1) {
      send_to_char(
          ch, "You have to bid at least 10 percent over the current bid.\r\n");
      return;
    } else if (atoi(arg2) < GET_BID(obj2) && GET_CURBID(obj2) <= -1) {
      send_to_char(ch, "You have to bid at least the starting bid.\r\n");
      return;
    } else if (atoi(arg2) > (((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / 100) * 50) +
                                (GET_GOLD(ch) + GET_BANK_GOLD(ch))) {
      send_to_char(ch,
                   "You can not bid more than 150%s of your total money (on "
                   "hand and in the bank).\r\n",
                   "%");
      return;
    } else if (GET_AUCTIME(obj2) + 86400 <= time(0)) {
      send_to_char(ch, "Bidding on that object has been closed.\r\n");
      return;
    } else {
      GET_BID(obj2) = atoi(arg2);
      GET_CURBID(obj2) = GET_ID(ch);
      auc_save();
      struct descriptor_data *d;
      int bid = atoi(arg2);
      mud_log("AUCTION: %s has bid %s on %s", GET_NAME(ch), obj2->short_description,
          add_commas(bid));
      for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
          continue;
        if (d->character == ch) {
          if (GET_EQ(d->character, WEAR_EYE)) {
            send_to_char(d->character,
                         "@RScouter Auction News@D: @GYou have bid @Y%s@G on "
                         "@w%s@G@n\r\n",
                         add_commas(GET_BID(obj2)), obj2->short_description);
          }
          continue;
        }
        if (GET_EQ(d->character, WEAR_EYE)) {
          send_to_char(d->character,
                       "@RScouter Auction News@D: @GThe bid on, @w%s@G, has "
                       "been raised to @Y%s@n\r\n",
                       obj2->short_description, add_commas(GET_BID(obj2)));
        }
      }
    }
  }
}

void stop_auction(int type, struct char_data *ch) {
  if (obj_selling == NULL) {
    auc_send_to_all("@RThe auction has stopped because someone has made off "
                    "with the auctioned object!@n\r\n",
                    FALSE);
    curbid = 0;
    ch_selling = NULL;
    ch_buying = NULL;
    aucstat = AUC_NULL_STATE;
    return;
  }
  switch (type) {

  case AUC_NORMAL_CANCEL: {

    sprintf(buf, auctioneer[AUC_NORMAL_CANCEL]);
    auc_send_to_all(buf, FALSE);
    break;
  }
  case AUC_QUIT_CANCEL: {

    sprintf(buf, auctioneer[AUC_QUIT_CANCEL]);
    auc_send_to_all(buf, FALSE);
    break;
  }
  case AUC_WIZ_CANCEL: {

    sprintf(buf, auctioneer[AUC_WIZ_CANCEL]);
    auc_send_to_all(buf, FALSE);
    break;
  }
  default: {
    send_to_char(
        ch, "Sorry, that is an unrecognised cancel command, please report.");
    return;
  }
  }

  if (type != AUC_WIZ_CANCEL) {
    sprintf(buf, "%s flies out the sky and into your hands.\r\n",
            obj_selling->short_description);
    CAP(buf);
    send_to_char(ch_selling, "%s", buf);
    obj_to_char(obj_selling, ch_selling);
  } else {
    sprintf(buf, "%s flies out the sky and into your hands.\r\n",
            obj_selling->short_description);
    CAP(buf);
    send_to_char(ch, "%s", buf);
    obj_to_char(obj_selling, ch);
  }

  if (!(ch_buying == NULL))
    char_stat_mod(ch_buying, "money", curbid);

  obj_selling = NULL;
  ch_selling = NULL;
  ch_buying = NULL;
  curbid = 0;

  aucstat = AUC_NULL_STATE;
}

static void auc_stat(struct char_data *ch, struct obj_data *obj) {

  if (aucstat == AUC_NULL_STATE) {
    send_to_char(ch, "Nothing is being auctioned!\r\n");
    return;
  } else if (ch == ch_selling) {
    send_to_char(ch,
                 "You should have found that out BEFORE auctioning it!\r\n");
    return;
  } else if (GET_GOLD(ch) < 500) {
    send_to_char(
        ch,
        "You can't afford to find the stats on that, it costs 500 zenni!\r\n");
    return;
  } else {
    /* auctioneer tells the character the auction details */
    sprintf(buf, auctioneer[AUC_STAT], curbid);
    act(buf, TRUE, ch_selling, obj, ch, TO_VICT | TO_SLEEP);
    char_stat_mod(ch, "money", -500);

    /*call_magic(ch, NULL, obj_selling, SPELL_IDENTIFY, 30, CAST_SPELL);*/
  }
}

static void auc_send_to_all(char *messg, bool buyer) {
  struct descriptor_data *i;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    struct room_data *room = char_room_get(i->character);
    if (room_flagged(room, ROOM_HBTC))
      continue;
    if (room_flagged(room, ROOM_PAST))
      continue;
    if (buyer)
      act(messg, TRUE, ch_buying, obj_selling, i->character,
          TO_VICT | TO_SLEEP);
    else
      act(messg, TRUE, ch_selling, obj_selling, i->character,
          TO_VICT | TO_SLEEP);
  }
}

ACMD(do_assemble) {
  long lVnum = NOTHING;
  struct obj_data *pObject = NULL;
  char buf[MAX_STRING_LENGTH];

  int roll = 0;

  skip_spaces(&argument);

  struct obj_data *tool = NULL;

  char_inventory_iterate(ch, [&](auto tools) {
    if (GET_OBJ_VNUM(tools) == 386 && GET_OBJ_VAL(tools, VAL_ALL_HEALTH) > 0) {
      tool = tools;
      act("@WYou open up your toolkit and take out the necessary tools.@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W opens up $s toolkit and takes out the necessary tools.@n",
          TRUE, ch, 0, 0, TO_ROOM);
    }
    return true;
  });

  struct room_data *room = char_room_get(ch);

  if (*argument == '\0') {
    send_to_char(ch, "What would you like to %s?\r\n", CMD_NAME);
    return;
  } else if ((lVnum = assemblyFindAssembly(argument)) < 0) {
    send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                 argument);
    return;
  } else if (assemblyGetType(lVnum) != subcmd) {
    send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                 argument);
    return;
  } else if (!assemblyCheckComponents(lVnum, ch, FALSE)) {
    send_to_char(ch, "You haven't got all the things you need.\r\n");
    return;
  } else if (room_flagged(room, ROOM_SPACE)) {
    send_to_char(ch, "You can't do that in space.");
    return;
  } else if (!GET_SKILL(ch, SKILL_SURVIVAL) &&
             !strcasecmp(argument, "campfire")) {
    send_to_char(ch, "You know nothing about building campfires.\r\n");
    return;
  }

  if (strstr(argument, "Signal") || strstr(argument, "signal")) {
    if (GET_SKILL(ch, SKILL_BUILD) < 70) {
      send_to_char(ch, "You need at least a build skill level of 70.\r\n");
      return;
    }
  }

  if (tool == NULL) {
    send_to_char(ch, "You wish you had tools, but make the best out of what "
                     "you do have anyway...\r\n");
    roll = 20;
  }

  int sect = room_sector_type_get(room);

  if (strcasecmp(argument, "campfire")) {
    if (room_flagged(room, ROOM_SPACE) || sect == SECT_WATER_NOSWIM ||
        room_is_sunken(room)) {
      send_to_char(ch, "This area will not allow a fire to burn properly.\r\n");
      return;
    }

    if (GET_SKILL(ch, SKILL_SURVIVAL) >= 90) {
      roll += axion_dice(0);
    } else if (GET_SKILL(ch, SKILL_SURVIVAL) < 90) {
      roll += axion_dice(0);
    }
    improve_skill(ch, SKILL_BUILD, 1);
    if (GET_SKILL(ch, SKILL_BUILD) <= roll) {
      if ((pObject = read_object(lVnum, VIRTUAL)) == NULL) {
        send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                     argument);
        return;
      }

      extract_obj(pObject);
      send_to_char(ch, "You start to %s %s %s, but mess up royally!\r\n",
                   CMD_NAME, AN(argument), argument);

      if (assemblyCheckComponents(lVnum, ch, TRUE)) {
        roll = 9001; /* Just a place holder */
      }
      return;
    }
  } else {
    if (GET_SKILL(ch, SKILL_SURVIVAL) >= 90) {
      roll += axion_dice(0);
    } else if (GET_SKILL(ch, SKILL_SURVIVAL) < 90) {
      roll += axion_dice(-10);
    }
    improve_skill(ch, SKILL_BUILD, 1);
    if (GET_SKILL(ch, SKILL_SURVIVAL) <= roll) {
      if ((pObject = read_object(lVnum, VIRTUAL)) == NULL) {
        send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                     argument);
        return;
      }

      extract_obj(pObject);
      send_to_char(ch, "You start to %s %s %s, but mess up royally!\r\n",
                   CMD_NAME, AN(argument), argument);

      if (tool && rand_number(1, 3) == 3 &&
          GET_OBJ_VAL(tool, VAL_ALL_HEALTH) > 0) {
        GET_OBJ_VAL(tool, VAL_ALL_HEALTH) -= rand_number(1, 5);
        act("@RYour toolset is looking a bit more worn.@n", TRUE, ch, 0, 0,
            TO_CHAR);
        if (GET_OBJ_VAL(tool, VAL_ALL_HEALTH) <= 0) {
          GET_OBJ_VAL(tool, VAL_ALL_HEALTH) = 0;
        }
      }
      if (assemblyCheckComponents(lVnum, ch, TRUE)) {
        roll = 9001; /* Just a place holder */
      }
      return;
    }
  }

  if (axion_dice(0) - (GET_INT(ch) / 5) > 95) {
    if ((pObject = read_object(lVnum, VIRTUAL)) == NULL) {
      send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                   argument);
      return;
    }

    extract_obj(pObject);
    send_to_char(ch,
                 "You start to %s %s %s, but forget a couple of steps. You "
                 "take it apart and give up.\r\n",
                 CMD_NAME, AN(argument), argument);
    WAIT_STATE(ch, PULSE_6SEC);
    return;
  } else if (rand_number(1, 100) >= 92) {
    if ((pObject = read_object(lVnum, VIRTUAL)) == NULL) {
      send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                   argument);
      return;
    }

    extract_obj(pObject);
    send_to_char(ch,
                 "You start to %s %s %s, but put it together wrong and have to "
                 "stop. You take it apart and give up.\r\n",
                 CMD_NAME, AN(argument), argument);
    WAIT_STATE(ch, PULSE_4SEC);
    return;
  }

  /* Create the assembled object. */
  if ((pObject = read_object(lVnum, VIRTUAL)) == NULL) {
    send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument),
                 argument);
    return;
  }

  /* Now give the object to the character. */
  if (GET_OBJ_VNUM(pObject) != 1611) {
    obj_to_char(pObject, ch);
    if (IS_TRUFFLE(ch)) {
      int count = 0, plused = FALSE, hasstat = 0, failsafe = 0;

      while (count < 6) {
        if (pObject->affected[count].location > 0 && rand_number(1, 6) <= 2 &&
            plused == FALSE) {
          pObject->affected[count].modifier += rand_number(1, 3);
          plused = TRUE;
        } else if (pObject->affected[count].location > 0) {
          hasstat += 1;
        }
        failsafe++;
        count++;
        if (plused == TRUE) {
          send_to_char(ch, "@YYour intuitive skill with building has made this "
                           "item even better!@n\r\n");
          count = 6;
        } else if (failsafe >= 12) {
          send_to_char(ch, "@yIt seems this item could not be upgraded with "
                           "your truffle knowledge...@n\r\n");
          count = 6;
        } else if (failsafe == 11 && plused == FALSE) {
          send_to_char(ch, "@YYour intuitive skill with building has made this "
                           "item even better!@n\r\n");
          pObject->affected[count].location = rand_number(1, 6);
          pObject->affected[count].modifier += rand_number(1, 3);
          plused = TRUE;
          count = 6;
        } else if (count == 6 && hasstat > 0) {
          count = 0;
        }
      }
    }
  } else {
    obj_to_room(pObject, char_room_get(ch));
    GET_OBJ_TIMER(pObject) = (GET_SKILL(ch, SKILL_SURVIVAL) * 0.12);
  }

  /*if (wearable_obj(pObject)) {
   GET_OBJ_SIZE(pObject) = get_size(ch);
  }*/

  /* Tell the character they made something. */
  sprintf(buf, "You %s $p.", CMD_NAME);
  act(buf, FALSE, ch, pObject, NULL, TO_CHAR);

  /* Tell the room the character made something. */
  sprintf(buf, "$n %ss $p.", CMD_NAME);
  act(buf, FALSE, ch, pObject, NULL, TO_ROOM);

  if (assemblyCheckComponents(lVnum, ch, TRUE)) {
    roll = 9001; /* Just a place holder */
  }

  if (!IS_TRUFFLE(ch) && axion_dice(8) > GET_SKILL(ch, SKILL_BUILD)) {
    send_to_char(ch, "@yYou've made an inferior product. Its value will be "
                     "somewhat less.@n\r\n");
    GET_OBJ_COST(pObject) -= GET_OBJ_COST(pObject) * 0.25;
  } else if (IS_TRUFFLE(ch) && axion_dice(18) > GET_SKILL(ch, SKILL_BUILD)) {
    send_to_char(ch, "@yYou've made an inferior product. Its value will be "
                     "somewhat less.@n\r\n");
    GET_OBJ_COST(pObject) -= GET_OBJ_COST(pObject) * 0.12;
  } else if (IS_TRUFFLE(ch) < GET_SKILL(ch, SKILL_BUILD)) {
    send_to_char(ch, "@YYou've made an excellent product. Its value will be "
                     "somewhat more.@n\r\n");
    GET_OBJ_COST(pObject) += GET_OBJ_COST(pObject) * 0.12;
  }

  if (IS_TRUFFLE(ch) && rand_number(1, 5) >= 4 &&
      GET_OBJ_COST(pObject) >= 500) {
    if (GET_LEVEL(ch) < 100 &&
        level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
      int64_t gain = level_exp(ch, GET_LEVEL(ch) + 1) * 0.011;
      send_to_char(ch, "@RExp Bonus@D: @G%s@n\r\n", add_commas(gain));
      gain_exp(ch, gain);
    } else {
      gain_exp(ch, 1375000);
      send_to_char(ch, "@RExp Bonus@D: @G%s@n\r\n", add_commas(1375000));
    }
  }
}

static void perform_put(struct char_data *ch, struct obj_data *obj,
                        struct obj_data *cont) {

  int dball[7] = {20, 21, 22, 23, 24, 25, 26};

  if (!drop_otrigger(obj, ch))
    return;

  if (!obj) /* object might be extracted by drop_otrigger */
    return;
  if (OBJ_FLAGGED(cont, ITEM_FORGED)) {
    act("$P is forged and won't hold anything.", FALSE, ch, 0, cont, TO_CHAR);
    return;
  }
  if (OBJ_FLAGGED(cont, ITEM_SHEATH) && GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
    send_to_char(ch, "That is made to only hold weapons.\r\n");
    return;
  }
  if (OBJ_FLAGGED(cont, ITEM_SHEATH)) {
    struct obj_data *obj2 = NULL, *next_obj = NULL;
    int count = 0, minus = 0;
    obj_contents_iterate(cont, [&](struct obj_data *obj2) {
      minus += GET_OBJ_WEIGHT(obj2);
      count++;
      return true;
    });
    int holds = GET_OBJ_WEIGHT(cont) - minus;
    if (count >= holds) {
      send_to_char(ch, "It can only hold %d weapon%s at a time.\r\n", holds,
                   holds > 1 ? "s" : "");
      return;
    }
  }
  if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) &&
      (GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY) == 0))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else if (GET_OBJ_VNUM(cont) >= 600 && GET_OBJ_VNUM(cont) <= 603)
    send_to_char(
        ch,
        "You can't put cards on a duel table. You have to @Gplay@n them.\r\n");
  else if ((GET_OBJ_VNUM(cont) == 697 || GET_OBJ_VNUM(cont) == 698 ||
            GET_OBJ_VNUM(cont) == 682 || GET_OBJ_VNUM(cont) == 683 ||
            GET_OBJ_VNUM(cont) == 684 || OBJ_FLAGGED(cont, ITEM_CARDCASE)) &&
           !OBJ_FLAGGED(obj, ITEM_ANTI_HIEROPHANT))
    send_to_char(ch, "You can only put cards in a case.\r\n");
  else if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) &&
           (GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY) > 0) &&
           (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) >
            GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && obj_room_get(cont) != NULL)
    act("You can't get $p out of your hand.", FALSE, ch, obj, NULL, TO_CHAR);
  else if (GET_OBJ_VNUM(obj) == dball[0] || GET_OBJ_VNUM(obj) == dball[1] ||
           GET_OBJ_VNUM(obj) == dball[2] || GET_OBJ_VNUM(obj) == dball[3] ||
           GET_OBJ_VNUM(obj) == dball[4] || GET_OBJ_VNUM(obj) == dball[5] ||
           GET_OBJ_VNUM(obj) == dball[6])
    send_to_char(ch, "You can not bag dragon balls.\r\n");
  else if (OBJ_FLAGGED(obj, ITEM_NORENT))
    send_to_char(ch, "That isn't worth bagging. Better keep that close if you "
                     "wanna keep it at all.\r\n");
  else if (!cont->carried_by && check_saveroom_count(ch, obj) > 150) {
    send_to_char(ch, "The save room can not hold anymore items. (150 max, "
                     "count of items in containers is halved)\r\n");
  } else {
    obj_from_char(obj);
    obj_to_obj(obj, cont);

    if (!OBJ_FLAGGED(obj, ITEM_ANTI_HIEROPHANT)) {
      act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
    } else {
      act("$n puts an @DA@wd@cv@Ce@Wnt @DD@wu@ce@Cl @mC@Ma@Wr@wd@n in $P.",
          TRUE, ch, obj, cont, TO_ROOM);
    }

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP)) {
      SET_BIT_AR(GET_OBJ_EXTRA(cont), ITEM_NODROP);
      act("You get a strange feeling as you put $p in $P.", FALSE, ch, obj,
          cont, TO_CHAR);
    } else
      act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    /* If object placed in portal or vehicle, move it to the portal destination
     */
    if ((GET_OBJ_TYPE(cont) == ITEM_PORTAL) ||
        (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)) {
      obj_from_obj(obj);
      obj_to_room(obj, room_by_id(GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)));
      if (GET_OBJ_TYPE(cont) == ITEM_PORTAL) {
        act("What? $U$p disappears from $P in a puff of smoke!", TRUE, ch, obj,
            cont, TO_ROOM);
        act("What? $U$p disappears from $P in a puff of smoke!", FALSE, ch, obj,
            cont, TO_CHAR);
      }
    }
  }
}

/* The following put modes are supported by the code below:

        1) put <object> <container>
        2) put all.<object> <container>
        3) put all <container>

        <container> must be in inventory or on ground.
        all objects to be put into container must be in inventory.
*/

ACMD(do_put) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
  char *theobj, *thecont;

  one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

  if (!HAS_ARMS(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  if (*arg3 && is_number(arg1)) {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
  } else {
    theobj = arg1;
    thecont = arg2;
  }
  obj_dotmode = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char(ch, "Put what in what?\r\n");
  else if (cont_dotmode != FIND_INDIV)
    send_to_char(ch,
                 "You can only put things into one container at a time.\r\n");
  else if (!*thecont) {
    send_to_char(ch, "What do you want to put %s in?\r\n",
                 obj_dotmode == FIND_INDIV ? "it" : "them");
  } else {
    generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch,
                 &tmp_char, &cont);
    if (!cont)
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
    else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) &&
             (GET_OBJ_TYPE(cont) != ITEM_PORTAL) &&
             (GET_OBJ_TYPE(cont) != ITEM_VEHICLE))
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
      send_to_char(ch, "You'd better open it first!\r\n");
    else {
      if (obj_dotmode == FIND_INDIV) { /* put <obj> <container> */
        if (!(obj = get_obj_in_list_vis(ch, theobj, NULL, inv_for_char(ch))))
          send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(theobj),
                       theobj);
        else if (obj == cont && howmany == 1)
          send_to_char(ch, "You attempt to fold it into itself, but fail.\r\n");
        else {
          char_inventory_iterate(ch, [&](auto candidate) {
            if (howmany <= 0) return false;
            if (isname(theobj, candidate->name) && candidate != cont) {
              howmany--;
              perform_put(ch, candidate, cont);
            }
            return true;
          });
        }
      } else {
        char_inventory_iterate(ch, [&](auto obj) {
          if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
              (obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
            found = 1;
            perform_put(ch, obj, cont);
          }
          return true;
        });
        if (!found) {
          if (obj_dotmode == FIND_ALL)
            send_to_char(ch,
                         "You don't seem to have anything to put in it.\r\n");
          else
            send_to_char(ch, "You don't seem to have any %ss.\r\n", theobj);
        }
      }
    }
  }
}

static int can_take_obj(struct char_data *ch, struct obj_data *obj) {
  if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$p: your arms are full!", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if (((getCurCarriedWeight(ch)) + GET_OBJ_WEIGHT(obj)) >
             CAN_CARRY_W(ch)) {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if ((GET_OBJ_WEIGHT(obj) + room_gravity_get(char_room_get(ch))) +
                 (getCurCarriedWeight(ch)) >
             CAN_CARRY_W(ch)) {
    act("$p: you can't carry that much weight because of the gravity.", FALSE,
        ch, obj, 0, TO_CHAR);
    return (0);
  }
  return (1);
}

static void get_check_money(struct char_data *ch, struct obj_data *obj) {
  int value = GET_OBJ_VAL(obj, VAL_MONEY_SIZE);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  if (GET_GOLD(ch) + value > GOLD_CARRY(ch)) {
    send_to_char(ch,
                 "You can only carry %s zenni at your current level, and leave "
                 "the rest.\r\n",
                 add_commas(GOLD_CARRY(ch)));
    act("@w$n @wdrops some onto the ground.@n", FALSE, ch, 0, 0, TO_ROOM);
    extract_obj(obj);
    int diff = 0;
    diff = (GET_GOLD(ch) + value) - GOLD_CARRY(ch);
    obj = create_money(diff);
    obj_to_room(obj, char_room_get(ch));
    char_stat_set(ch, "money", GOLD_CARRY(ch));
    return;
  }

  char_stat_mod(ch, "money", value);
  extract_obj(obj);

  if (value == 1) {
    send_to_char(ch, "There was 1 zenni.\r\n");
  } else {
    send_to_char(ch, "There were %d zenni.\r\n", value);
    if (char_condition_has(ch, "group") && PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
      char split[MAX_INPUT_LENGTH];
      sprintf(split, "%d", value);
      char_cmd_execute(ch, "split", split);
    }
  }
}

static void perform_get_from_container(struct char_data *ch,
                                       struct obj_data *obj,
                                       struct obj_data *cont, int mode) {
  if (mode == FIND_OBJ_INV || mode == FIND_OBJ_EQUIP || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    if (SITS(ch) && GET_OBJ_VNUM(SITS(ch)) > 603 &&
        GET_OBJ_VNUM(SITS(ch)) < 608 &&
        GET_OBJ_VNUM(SITS(ch)) - 4 != GET_OBJ_VNUM(cont) &&
        GET_OBJ_VNUM(cont) > 599 && GET_OBJ_VNUM(cont) < 604) {
      send_to_char(ch, "You aren't playing at that table!\r\n");
      return;
    } else if (get_otrigger(obj, ch)) {
      obj_from_obj(obj);
      obj_to_char(obj, ch);
      if (OBJ_FLAGGED(cont, ITEM_SHEATH)) {
        act("You draw $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
        act("$n draws $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      } else {
        act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
        act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      }
      if (OBJ_FLAGGED(obj, ITEM_HOT)) {
        if (GET_BONUS(ch, BONUS_FIREPROOF) <= 0 && !IS_DEMON(ch)) {
          decCurHealthPercent(ch, .25);
          if (GET_BONUS(ch, BONUS_FIREPRONE) > 0)
            decCurHealthPercentFloored(ch, 1, 1);

          char_condition_add(ch, "burned", "attack", "fiery");
          act("@RYou are burned by it!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n@R is burned by it!@n", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
      if (IS_NPC(ch)) {
        item_check(obj, ch);
      }
      get_check_money(ch, obj);
    }
  }
}

static void get_from_container(struct char_data *ch, struct obj_data *cont,
                               char *arg, int mode, int howmany) {
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    {
      bool found = false;
      obj_contents_iterate(cont, [&](auto candidate) {
        if (howmany <= 0) return false;
        if (isname(arg, candidate->name) && CAN_SEE_OBJ(ch, candidate)) {
          found = true;
          howmany--;
          perform_get_from_container(ch, candidate, cont, mode);
        }
        return true;
      });
      if (!found) {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "There doesn't seem to be %s %s in $p.",
                 AN(arg), arg);
        act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    obj_contents_iterate(cont, [&](struct obj_data *obj) {
      if (CAN_SEE_OBJ(ch, obj) &&
          (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
        found = 1;
        perform_get_from_container(ch, obj, cont, mode);
      }
      return true;
    });
    if (!found) {
      if (obj_dotmode == FIND_ALL)
        act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else {
        char buf[MAX_STRING_LENGTH];

        snprintf(buf, sizeof(buf), "You can't seem to find any %ss in $p.",
                 arg);
        act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}

int perform_get_from_room(struct char_data *ch, struct obj_data *obj) {

  if (SITTING(obj)) {
    send_to_char(ch, "Someone is on that!\r\n");
    return (0);
  }

  if (OBJ_FLAGGED(obj, ITEM_BURIED)) {
    send_to_char(ch, "Get what?\r\n");
    return (0);
  }

  struct room_data *room = char_room_get(ch);

  if (room_flagged(room, ROOM_GARDEN1) || room_flagged(room, ROOM_GARDEN2)) {
    send_to_char(ch, "You can't get things from a garden. Help garden.\r\n");
    return (0);
  }

  if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
    obj_from_room(obj);
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);

    if (OBJ_FLAGGED(obj, ITEM_HOT)) {
      if (GET_BONUS(ch, BONUS_FIREPROOF) <= 0 && !IS_DEMON(ch)) {
        decCurHealthPercentFloored(ch, .25, 1);
        if (GET_BONUS(ch, BONUS_FIREPRONE) > 0)
          decCurHealthPercentFloored(ch, 1, 1);

        char_condition_add(ch, "burned", "attack", "fiery");
        act("@RYou are burned by it!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n@R is burned by it!@n", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (IS_NPC(ch))
      item_check(obj, ch);
    get_check_money(ch, obj);
  }
  return (0);
}

static char *find_exdesc_keywords(char *word, struct extra_descr_data *list) {
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->keyword);

  return (NULL);
}

static void get_from_room(struct char_data *ch, char *arg, int howmany) {
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;
  char *descword;

  /* Are they trying to take something in a room extra description? */
  if (find_exdesc(arg, room_ex_description_get(char_room_get(ch))) != NULL) {
    send_to_char(ch, "You can't take %s %s.\r\n", AN(arg), arg);
    return;
  }

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
    if ((descword = find_exdesc_keywords(
             arg, room_ex_description_get(char_room_get(ch)))) != NULL)
      send_to_char(ch, "%s: you can't take that!\r\n", fname(descword));
    else {
      int taken = 0;
      room_contents_iterate(char_room_get(ch), [&](auto candidate) {
        if (taken >= howmany) return false;
        if (isname(arg, candidate->name) && CAN_SEE_OBJ(ch, candidate)) {
          taken++;
          perform_get_from_room(ch, candidate);
        }
        return true;
      });
      if (!taken)
        send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    room_contents_iterate(char_room_get(ch), [&](auto obj) {
      if (CAN_SEE_OBJ(ch, obj) &&
          (dotmode == FIND_ALL || isname(arg, obj->name))) {
        found = 1;
        perform_get_from_room(ch, obj);
      }
      return true;
    });
    if (!found) {
      if (dotmode == FIND_ALL)
        send_to_char(ch, "There doesn't seem to be anything here.\r\n");
      else
        send_to_char(ch, "You don't see any %ss here.\r\n", arg);
    }
  }
}

ACMD(do_get) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

  if (!HAS_ARMS(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  if (!*arg1)
    send_to_char(ch, "Get what?\r\n");
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else {
    int amount = 1;
    if (is_number(arg1)) {
      amount = atoi(arg1);
      strcpy(arg1, arg2); /* strcpy: OK (sizeof: arg1 == arg2) */
      strcpy(arg2, arg3); /* strcpy: OK (sizeof: arg2 == arg3) */
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM,
                          ch, &tmp_char, &cont);
      if (!cont)
        send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
      else if (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)
        send_to_char(ch, "You will need to enter it first.\r\n");
      else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) &&
               !((GET_OBJ_TYPE(cont) == ITEM_PORTAL) &&
                 (OBJVAL_FLAGGED(cont, CONT_CLOSEABLE))))
        act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else
        get_from_container(ch, cont, arg1, mode, amount);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
        send_to_char(ch, "Get from all of what?\r\n");
        return;
      }
      char_inventory_iterate(ch, [&](auto cont) {
        if (CAN_SEE_OBJ(ch, cont) &&
            (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
          if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
            found = 1;
            get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
          } else if (cont_dotmode == FIND_ALLDOT) {
            found = 1;
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
          }
        }
        return true;
      });
      room_contents_iterate(char_room_get(ch), [&](auto cont) {
        if (CAN_SEE_OBJ(ch, cont) &&
            (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
          if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
            get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
            found = 1;
          } else if (cont_dotmode == FIND_ALLDOT) {
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
            found = 1;
          }
        }
        return true;
      });
      if (!found) {
        if (cont_dotmode == FIND_ALL)
          send_to_char(ch, "You can't seem to find any containers.\r\n");
        else
          send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
      }
    }
  }
}

static void perform_drop_gold(struct char_data *ch, int amount, int8_t mode,
                              struct room_data *RDR) {
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
  else if (GET_GOLD(ch) < amount)
    send_to_char(ch, "You don't have that many zenni!\r\n");
  else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_1SEC); /* to prevent zenni-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE) {
        send_to_char(ch, "You throw some zenni into the air where it "
                         "disappears in a puff of smoke!\r\n");
        act("$n throws some gold into the air where it disappears in a puff of "
            "smoke!",
            FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0,
            TO_ROOM);
      } else {
        char buf[MAX_STRING_LENGTH];

        if (!drop_wtrigger(obj, ch)) {
          extract_obj(obj);
          return;
        }

        if (!drop_wtrigger(obj, ch) && (obj)) { /* obj may be purged */
          extract_obj(obj);
          return;
        }

        snprintf(buf, sizeof(buf), "$n drops %s.", money_desc(amount));
        act(buf, TRUE, ch, 0, 0, TO_ROOM);

        send_to_char(ch, "You drop some zenni.\r\n");
        obj_to_room(obj, char_room_get(ch));
        if (GET_ADMLEVEL(ch) > 0) {
          send_to_imm("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch),
                      obj->short_description, obj_room_vnum_get(obj));
          log_imm_action("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch),
                         obj->short_description, obj_room_vnum_get(obj));
          if (check_insidebag(obj, 0.0) > 1) {
            send_to_imm("IMM DROP: Object contains %d other items.",
                        check_insidebag(obj, 0.0));
            log_imm_action("IMM DROP: Object contains %d other items.",
                           check_insidebag(obj, 0.0));
          }
        }
      }
    } else {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf),
               "$n drops %s which disappears in a puff of smoke!",
               money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      send_to_char(
          ch, "You drop some zenni which disappears in a puff of smoke!\r\n");
    }
    char_stat_mod(ch, "money", -amount);
  }
}

#define VANISH(mode)                                                           \
  ((mode == SCMD_DONATE || mode == SCMD_JUNK)                                  \
       ? "  It vanishes in a puff of smoke!"                                   \
       : "")

static int perform_drop(struct char_data *ch, struct obj_data *obj, int8_t mode,
                        const char *sname, struct room_data *RDR) {
  char buf[MAX_STRING_LENGTH];
  int value;

  if (!drop_otrigger(obj, ch))
    return 0;

  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
    return 0;

  if (GET_OBJ_VNUM(obj) == 17 || GET_OBJ_VNUM(obj) == 17998) {
    snprintf(buf, sizeof(buf),
             "You can't %s $p, it is grafted into your soul :P", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  struct room_data *room = char_room_get(ch);

  if (GET_OBJ_VNUM(obj) == 20 || GET_OBJ_VNUM(obj) == 21 ||
      GET_OBJ_VNUM(obj) == 22 || GET_OBJ_VNUM(obj) == 23 ||
      GET_OBJ_VNUM(obj) == 24 || GET_OBJ_VNUM(obj) == 25 ||
      GET_OBJ_VNUM(obj) == 26) {
    if (room_flagged(room, ROOM_SPACE)) {
      snprintf(buf, sizeof(buf), "You can't %s $p in space!", sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
    if (room_flagged(room, ROOM_GARDEN1) || room_flagged(room, ROOM_GARDEN2)) {
      snprintf(buf, sizeof(buf), "You can't %s $p in here. Read help garden.",
               sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
    if ((mode == SCMD_DROP) && OBJ_FLAGGED(obj, ITEM_NORENT)) {
      snprintf(buf, sizeof(buf), "You drop $p but it gets lost on the ground!");
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      obj_from_char(obj);
      extract_obj(obj);
      return (0);
    }
    if (room_flagged(room, ROOM_NOINSTANT)) {
      snprintf(buf, sizeof(buf), "You can't %s $p in this protected area!",
               sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
    if (room_flagged(room, ROOM_SHIP)) {
      snprintf(buf, sizeof(buf), "You can't %s $p on a private ship!", sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
    if (room_flagged(room, ROOM_HOUSE)) {
      snprintf(buf, sizeof(buf), "You can't %s $p in a private house!", sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
  }
  if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_ADMLEVEL(ch) < 1) {
    snprintf(buf, sizeof(buf), "You can't %s $p, it must be CURSED!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  if ((mode == SCMD_DONATE || mode == SCMD_JUNK) &&
      OBJ_FLAGGED(obj, ITEM_NODONATE)) {
    snprintf(buf, sizeof(buf), "You can't %s $p!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  snprintf(buf, sizeof(buf), "You %s $p.", sname);
  act(buf, FALSE, ch, obj, 0, TO_CHAR);

  snprintf(buf, sizeof(buf), "$n %ss $p.", sname);
  act(buf, TRUE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);

  switch (mode) {
  case SCMD_DROP:
    if (!OBJ_FLAGGED(obj, ITEM_UNBREAKABLE) && room_geffect_get(room) == 6) {
      act("$p melts in the lava!", FALSE, ch, obj, 0, TO_CHAR);
      act("$p melts in the lava!", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else if (room_geffect_get(room) == 6) {
      act("$p plops down on some cooled lava!", FALSE, ch, obj, 0, TO_CHAR);
      act("$p plops down on some cooled lava!", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_room(obj, char_room_get(ch));
      if (GET_ADMLEVEL(ch) > 0) {
        send_to_imm("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch),
                    obj->short_description, obj_room_vnum_get(obj));
        log_imm_action("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch),
                       obj->short_description, obj_room_vnum_get(obj));
      }
    } else {
      obj_to_room(obj, char_room_get(ch));
      if (GET_ADMLEVEL(ch) > 0) {
        send_to_imm("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch),
                    obj->short_description, obj_room_vnum_get(obj));
        log_imm_action("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch),
                       obj->short_description, obj_room_vnum_get(obj));
      }
    }
    return (0);
  case SCMD_DONATE:
    obj_to_room(obj, RDR);
    act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return (0);
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
  default:
    mud_log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    /*  SYSERR_DESC:
     *  This error comes from perform_drop() and is output when perform_drop()
     *  is called with an illegal 'mode' argument.
     */
    break;
  }

  return (0);
}

ACMD(do_drop) {
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct room_data *RDR = NULL;
  int8_t mode = SCMD_DROP;
  int dotmode, amount = 0, multi, num_don_rooms;
  const char *sname;

  if (!HAS_ARMS(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  struct room_data *room = char_room_get(ch);

  if (room_flagged(room, ROOM_GARDEN1) || room_flagged(room, ROOM_GARDEN2)) {
    send_to_char(ch, "You can not do that in a garden.\r\n");
    return;
  }

  switch (subcmd) {
  case SCMD_JUNK:
    sname = "junk";
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    /* fail + double chance for room 1   */
    num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 +
                    (CONFIG_DON_ROOM_2 != NOWHERE) +
                    (CONFIG_DON_ROOM_3 != NOWHERE) + 1;
    switch (rand_number(0, num_don_rooms)) {
    case 0:
      mode = SCMD_JUNK;
      break;
    case 1:
    case 2:
      RDR = room_by_id(CONFIG_DON_ROOM_1);
      break;
    case 3:
      RDR = room_by_id(CONFIG_DON_ROOM_2);
      break;
    case 4:
      RDR = room_by_id(CONFIG_DON_ROOM_3);
      break;
    }
    if (RDR == NULL) {
      send_to_char(ch, "Sorry, you can't donate anything right now.\r\n");
      return;
    }
    break;
  default:
    sname = "drop";
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", sname);
    return;
  } else if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!strcasecmp("zenni", arg) || !strcasecmp("gold", arg))
      perform_drop_gold(ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char(ch, "Yeah, that makes sense.\r\n");
    else if (!*arg)
      send_to_char(ch, "What do you want to %s %d of?\r\n", sname, multi);
    else {
      int count = 0;
      char_inventory_iterate(ch, [&](auto candidate) {
        if (count >= multi) return false;
        if (isname(arg, candidate->name)) {
          if (count == 0 && check_saveroom_count(ch, candidate) > 150) {
            send_to_char(ch, "The save room you are in can not hold anymore items! "
                             "(150 max, count of items in containers is halved)\r\n");
            return false;
          }
          amount += perform_drop(ch, candidate, mode, sname, RDR);
          count++;
        }
        return true;
      });
    }
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) &&
        (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
        send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
      else
        send_to_char(
            ch,
            "Go do the donation room if you want to donate EVERYTHING!\r\n");
      return;
    }
    if (dotmode == FIND_ALL) {
      int fail = FALSE;

      if (!char_inventory_count(ch, false))
        send_to_char(ch, "You don't seem to be carrying anything.\r\n");
      else {
        char_inventory_iterate(ch, [&](auto obj) {
          if (check_saveroom_count(ch, obj) > 150) {
            fail = TRUE;
          } else {
            amount += perform_drop(ch, obj, mode, sname, RDR);
          }
          return true;
        });
        if (fail == TRUE) {
          send_to_char(
              ch,
              "Some of the items couldn't be dropped into this save room. It "
              "is too full. (150 max, containers half the count inside)\r\n");
        }
      }
    } else if (dotmode == FIND_ALLDOT) {
      int fail = FALSE;

      if (!*arg) {
        send_to_char(ch, "What do you want to %s all of?\r\n", sname);
        return;
      }
      {
        bool found = false;
        char_inventory_iterate(ch, [&](auto candidate) {
          if (isname(arg, candidate->name)) {
            found = true;
            if (check_saveroom_count(ch, candidate) > 150) {
              fail = TRUE;
            } else {
              amount += perform_drop(ch, candidate, mode, sname, RDR);
            }
          }
          return true;
        });
        if (!found)
          send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
      }
      if (fail == TRUE) {
        send_to_char(
            ch, "Some of the items couldn't be dropped into this save room. It "
                "is too full. (150 max, containers half the count inside)\r\n");
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      } else if (check_saveroom_count(ch, obj) > 150) {
        send_to_char(
            ch, "The item couldn't be dropped into this save room. It is too "
                "full. (150 max, containers half the count inside)\r\n");
      } else {
        amount += perform_drop(ch, obj, mode, sname, RDR);
      }
    }
  }
}

static void perform_give(struct char_data *ch, struct char_data *vict,
                         struct obj_data *obj) {
  if (!give_otrigger(obj, ch, vict))
    return;
  if (!receive_mtrigger(vict, ch, obj))
    return;

  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
    act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
    if (IS_NPC(ch)) {
      act("$n@n drops $p because you can't carry anymore.", TRUE, ch, obj, vict,
          TO_VICT);
      act("$n@n drops $p on the ground since $N's unable to carry it.", TRUE,
          ch, obj, vict, TO_NOTVICT);
      obj_from_char(obj);
      obj_to_room(obj, char_room_get(ch));
    }
    return;
  }
  if (IS_NPC(vict) &&
      (OBJ_FLAGGED(obj, ITEM_FORGED) || OBJ_FLAGGED(obj, ITEM_FORGED))) {
    act("$n tries to hand $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
    do_say(vict, "I don't want that piece of junk.", 0, 0);
    return;
  }
  if (GET_OBJ_WEIGHT(obj) + (getCurCarriedWeight(vict)) > CAN_CARRY_W(vict)) {
    act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
    if (IS_NPC(ch)) {
      act("$n@n drops $p because you can't carry anymore.", TRUE, ch, obj, vict,
          TO_VICT);
      act("$n@n drops $p on the ground since $N's unable to carry it.", TRUE,
          ch, obj, vict, TO_NOTVICT);
      obj_from_char(obj);
      obj_to_room(obj, char_room_get(ch));
    }
    return;
  }
  if ((GET_OBJ_WEIGHT(obj) + room_gravity_get(char_room_get(vict))) +
          (getCurCarriedWeight(vict)) >
      CAN_CARRY_W(vict)) {
    act("$E can't carry that much weight because of the gravity.", FALSE, ch, 0,
        vict, TO_CHAR);
    if (IS_NPC(ch)) {
      act("$n@n drops $p because you can't carry anymore.", TRUE, ch, obj, vict,
          TO_VICT);
      act("$n@n drops $p on the ground since $N's unable to carry it.", TRUE,
          ch, obj, vict, TO_NOTVICT);
      obj_from_char(obj);
      obj_to_room(obj, char_room_get(ch));
    }
    return;
  }
  if (!IS_NPC(vict) && !IS_NPC(ch)) {
    if (PRF_FLAGGED(vict, PRF_NOGIVE)) {
      act("$N refuses to accept $p at this time.", FALSE, ch, obj, vict,
          TO_CHAR);
      act("$n tries to give you $p, but you are refusing to be handed things.",
          FALSE, ch, obj, vict, TO_VICT);
      act("$n tries to give $N, $p, but $E is refusing to be handed things "
          "right now.",
          FALSE, ch, obj, vict, TO_NOTVICT);
      return;
    }
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);

  if (OBJ_FLAGGED(obj, ITEM_HOT)) {
    if (GET_BONUS(vict, BONUS_FIREPROOF) <= 0 && !IS_DEMON(vict)) {
      decCurHealthPercentFloored(ch, .25, 1);
      if (GET_BONUS(vict, BONUS_FIREPRONE) > 0)
        decCurHealthPercentFloored(ch, 1, 1);

      char_condition_add(vict, "burned", "attack", "fiery");
      act("@RYou are burned by it!@n", TRUE, vict, 0, 0, TO_CHAR);
      act("@R$n@R is burned by it!@n", TRUE, vict, 0, 0, TO_ROOM);
    }
  }
}

/* utility function for give */
static struct char_data *give_find_vict(struct char_data *ch, char *arg) {
  struct char_data *vict;

  skip_spaces(&arg);
  if (!*arg)
    send_to_char(ch, "To who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    if (IS_NPC(ch))
      send_to_imm("Mob Give: Victim, %s, doesn't exist.", arg);
  } else if (vict == ch)
    send_to_char(ch, "What's the point of that?\r\n");
  else
    return (vict);

  return (NULL);
}

static void perform_give_gold(struct char_data *ch, struct char_data *vict,
                              int amount) {
  char buf[MAX_STRING_LENGTH];

  if (amount <= 0) {
    send_to_char(ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))) {
    send_to_char(ch, "You don't have that much zenni!\r\n");
    return;
  }
  if (GET_GOLD(vict) + amount > GOLD_CARRY(vict)) {
    send_to_char(ch, "They can't carry that much zenni.\r\n");
    return;
  }
  send_to_char(ch, "%s", CONFIG_OK);

  snprintf(buf, sizeof(buf), "$n gives you %d zenni.", amount);
  act(buf, FALSE, ch, 0, vict, TO_VICT);

  snprintf(buf, sizeof(buf), "$n gives %s to $N.", money_desc(amount));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

  if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))
    char_stat_mod(ch, "money", -amount);
  char_stat_mod(vict, "money", amount);

  bribe_mtrigger(vict, ch, amount);
}

ACMD(do_give) {
  char arg[MAX_STRING_LENGTH];
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj;

  argument = one_argument(argument, arg);

  if (!HAS_ARMS(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  reveal_hiding(ch, 0);
  if (!*arg)
    send_to_char(ch, "Give what to who?\r\n");
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!strcasecmp("zenni", arg) || !strcasecmp("gold", arg)) {
      one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)) != NULL) {
        perform_give_gold(ch, vict, amount);
        if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict)) {
          send_to_imm("IMM GIVE: %s has given %s zenni to %s.", GET_NAME(ch),
                      add_commas(amount), GET_NAME(vict));
          log_imm_action("IMM GIVE: %s has given %s zenni to %s.", GET_NAME(ch),
                         add_commas(amount), GET_NAME(vict));
        }
      }
      return;
    } else if (!*arg) /* Give multiple code. */
      send_to_char(ch, "What do you want to give %d of?\r\n", amount);
    else if (!(vict = give_find_vict(ch, argument)))
      return;
    else {
      int count = 0;
      char_inventory_iterate(ch, [&](auto candidate) {
        if (count >= amount) return false;
        if (isname(arg, candidate->name)) {
          perform_give(ch, vict, candidate);
          if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict)) {
            send_to_imm("IMM GIVE: %s has given %s to %s.", GET_NAME(ch),
                        candidate->short_description, GET_NAME(vict));
            log_imm_action("IMM GIVE: %s has given %s to %s.", GET_NAME(ch),
                           candidate->short_description, GET_NAME(vict));
          }
          count++;
        }
        return true;
      });
    }
  } else {
    char buf1[MAX_INPUT_LENGTH];

    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch))))
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else {
        perform_give(ch, vict, obj);
        if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict)) {
          send_to_imm("IMM GIVE: %s has given %s to %s.", GET_NAME(ch),
                      obj->short_description, GET_NAME(vict));
          log_imm_action("IMM GIVE: %s has given %s to %s.", GET_NAME(ch),
                         obj->short_description, GET_NAME(vict));
        }
      }
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
        send_to_char(ch, "All of what?\r\n");
        return;
      }
      if (!char_inventory_count(ch, false))
        send_to_char(ch, "You don't seem to be holding anything.\r\n");
      else
        char_inventory_iterate(ch, [&](auto obj) {
          if (CAN_SEE_OBJ(ch, obj) &&
              ((dotmode == FIND_ALL || isname(arg, obj->name)))) {
            perform_give(ch, vict, obj);
            if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict)) {
              send_to_imm("IMM GIVE: %s has given %s to %s.", GET_NAME(ch),
                          obj->short_description, GET_NAME(vict));
              log_imm_action("IMM GIVE: %s has given %s to %s.", GET_NAME(ch),
                             obj->short_description, GET_NAME(vict));
            }
          }
          return true;
        });
    }
  }
}

void weight_change_object(struct obj_data *obj, int weight) {
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj_room_get(obj) != NULL) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    mud_log("SYSERR: Unknown attempt to subtract weight from an object.");
    /*  SYSERR_DESC:
     *  weight_change_object() outputs this error when weight is attempted to
     *  be removed from an object that is not carried or in another object.
     */
  }
}

void name_from_drinkcon(struct obj_data *obj) {
  char *new_name, *cur_name, *next;
  const char *liqname;
  int liqlen, cpylen;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON &&
               GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  liqname = drinknames[GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)];
  if (!isname(liqname, obj->name)) {
    /*mud_log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname,
     * obj->name, obj->item_number);*/
    /*  SYSERR_DESC:
     *  From name_from_drinkcon(), this error comes about if the object
     *  noted (by keywords and item vnum) does not contain the liquid string
     *  being searched for.
     */
    return;
  }

  liqlen = strlen(liqname);
  CREATE(new_name, char,
         strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */

  for (cur_name = obj->name; cur_name; cur_name = next) {
    if (*cur_name == ' ')
      cur_name++;

    if ((next = strchr(cur_name, ' ')))
      cpylen = next - cur_name;
    else
      cpylen = strlen(cur_name);

    if (!strncasecmp(cur_name, liqname, liqlen))
      continue;

    if (*new_name)
      strcat(new_name, " ");             /* strcat: OK (size precalculated) */
    strncat(new_name, cur_name, cpylen); /* strncat: OK (size precalculated) */
  }

  if (obj->name)
    free(obj->name);
  obj->name = new_name;
}

void name_to_drinkcon(struct obj_data *obj, int type) {
  char *new_name;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON &&
               GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", obj->name, drinknames[type]); /* sprintf: OK */

  if (obj->name)
    free(obj->name);

  obj->name = new_name;
}

ACMD(do_pour) {
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount = 0;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) { /* No arguments */
      send_to_char(ch, "From what do you want to pour?\r\n");
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      send_to_char(ch, "You can't pour from that!\r\n");
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) { /* no arguments */
      send_to_char(
          ch,
          "What do you want to fill?  And what are you filling it from?\r\n");
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2) { /* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL,
                                         inv_for_room(char_room_get(ch))))) {
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg2),
                   arg2);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN &&
        !OBJ_FLAGGED(from_obj, ITEM_BROKEN)) {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    } else if (GET_OBJ_TYPE(from_obj) == ITEM_FOUNTAIN &&
               OBJ_FLAGGED(from_obj, ITEM_BROKEN)) {
      act("You can't fill something from a broken fountain.", FALSE, ch,
          from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) == 0) {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) { /* pour */
    if (!*arg2) {
      send_to_char(ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (!strcasecmp(arg2, "out")) {
      if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
        act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
        act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

        weight_change_object(
            from_obj, -GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL)); /* Empty */

        name_from_drinkcon(from_obj);
        GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) = 0;
        GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID) = 0;
        GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON) = 0;
      } else {
        send_to_char(ch, "You can't possibly pour that container out!\r\n");
      }

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
        (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      send_to_char(ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char(ch, "A most unproductive effort.\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) != 0) &&
      (GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) !=
       GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID))) {
    send_to_char(ch, "There is already another liquid in it!\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) < 0) ||
      (!(GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) <
         GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY)))) {
    send_to_char(ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR)
    send_to_char(ch, "You pour the %s into the %s.",
                 drinks[GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)], arg2);

  if (subcmd == SCMD_FILL) {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) =
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID);

  /* Then how much to pour */
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) -=
        (amount = (GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) -
                   GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL)));

    GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) =
        GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY);

    if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) <
        0) { /* There was too little */
      GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) +=
          GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
      amount += GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
      name_from_drinkcon(from_obj);
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) = 0;
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID) = 0;
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON) = 0;
    }
  } else {
    GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) =
        GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY);
  }

  /* Then the poison boogie */
  GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) =
      (GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) ||
       GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON));

  /* And the weight boogie for non-eternal from_objects */
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
    weight_change_object(from_obj, -amount);
  }
  weight_change_object(to_obj, amount); /* Add weight */
}

static void wear_message(struct char_data *ch, struct obj_data *obj,
                         int where) {
  const char *wear_messages[][2] = {
      {"$n lights $p and holds it.", "You light $p and hold it."},

      {"$n slides $p on to $s right ring finger.",
       "You slide $p on to your right ring finger."},

      {"$n slides $p on to $s left ring finger.",
       "You slide $p on to your left ring finger."},

      {"$n wears $p around $s neck.", "You wear $p around your neck."},

      {"$n wears $p around $s neck.", "You wear $p around your neck."},

      {"$n wears $p on $s body.", "You wear $p on your body."},

      {"$n wears $p on $s head.", "You wear $p on your head."},

      {"$n puts $p on $s legs.", "You put $p on your legs."},

      {"$n wears $p on $s feet.", "You wear $p on your feet."},

      {"$n puts $p on $s hands.", "You put $p on your hands."},

      {"$n wears $p on $s arms.", "You wear $p on your arms."},

      {"$n straps $p around $s arm as a shield.",
       "You start to use $p as a shield."},

      {"$n wears $p about $s body.", "You wear $p around your body."},

      {"$n wears $p around $s waist.", "You wear $p around your waist."},

      {"$n puts $p on around $s right wrist.",
       "You put $p on around your right wrist."},

      {"$n puts $p on around $s left wrist.",
       "You put $p on around your left wrist."},

      {"$n wields $p.", "You wield $p."},

      {"$n grabs $p.", "You grab $p."},

      {"$n wears $p on $s back.", "You wear $p on your back."},

      {"$n puts $p in $s right ear.", "You put $p in your right ear."},

      {"$n puts $p in $s left ear.", "You put $p in your left ear."},

      {"$n wears $p as a cape.", "You wear $p as a cape."},

      {"$n covers $s left eye with $p.", "You wear $p over your left eye."}

  };

  act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

static int hands(struct char_data *ch) {
  int x;

  if (GET_EQ(ch, WEAR_WIELD1)) {
    if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD1), ITEM_2H) ||
        wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD1)) == WIELD_TWOHAND) {
      x = 2;
    } else
      x = 1;
  } else
    x = 0;

  if (GET_EQ(ch, WEAR_WIELD2)) {
    if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD2), ITEM_2H) ||
        wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD2)) == WIELD_TWOHAND) {
      x += 2;
    } else
      x += 1;
  }

  return x;
}

void perform_wear(struct char_data *ch, struct obj_data *obj, int where) {
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] = {
      ITEM_WEAR_TAKE,  ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
      ITEM_WEAR_NECK,  ITEM_WEAR_BODY,   ITEM_WEAR_HEAD,   ITEM_WEAR_LEGS,
      ITEM_WEAR_FEET,  ITEM_WEAR_HANDS,  ITEM_WEAR_ARMS,   ITEM_WEAR_SHIELD,
      ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST,  ITEM_WEAR_WRIST,  ITEM_WEAR_WRIST,
      ITEM_WEAR_TAKE,  ITEM_WEAR_TAKE,   ITEM_WEAR_PACK,   ITEM_WEAR_EAR,
      ITEM_WEAR_EAR,   ITEM_WEAR_SH,     ITEM_WEAR_EYE};

  const char *already_wearing[] = {
      "You're already using a light.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You're already wearing something on both of your ring fingers.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You can't wear anything else around your neck.\r\n",
      "You're already wearing something on your body.\r\n",
      "You're already wearing something on your head.\r\n",
      "You're already wearing something on your legs.\r\n",
      "You're already wearing something on your feet.\r\n",
      "You're already wearing something on your hands.\r\n",
      "You're already wearing something on your arms.\r\n",
      "You're already using a shield.\r\n",
      "You're already wearing something about your body.\r\n",
      "You already have something around your waist.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You're already wearing something around both of your wrists.\r\n",
      "You're already wielding a weapon.\r\n",
      "You're already holding something.\r\n",
      "You're already wearing something on your back.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You're already wearing something in both ears.\r\n",
      "You're already wearing something on your shoulders.\r\n",
      "You're already wearing something as a scouter.\r\n"};

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  /* do they even have that wear slot by race? */
  if (!BODY_FLAGGED(ch, where)) {
    send_to_char(ch,
                 "Seems like your body type doesn't really allow that.\r\n");
    return;
  }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
      (where == WEAR_WRIST_R) || (where == WEAR_EAR_R) ||
      (where == WEAR_WIELD1))
    if (GET_EQ(ch, where))
      where++;

  /* checks for 2H sanity */
  if ((OBJ_FLAGGED(obj, ITEM_2H) ||
       (wield_type(get_size(ch), obj) == WIELD_TWOHAND)) &&
      hands(ch) > 0) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }
  if (where == WEAR_WIELD2 && PLR_FLAGGED(ch, PLR_THANDW)) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (((where == WEAR_WIELD1) || (where == WEAR_WIELD2)) && (hands(ch) > 1)) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (GET_EQ(ch, where)) {
    send_to_char(ch, "%s", already_wearing[where]);
    return;
  }

  /* See if a trigger disallows it */
  if (!wear_otrigger(obj, ch, where) || (obj->carried_by != ch))
    return;

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
    if (GET_LEVEL(ch) < 20) {
      send_to_char(ch, "You are not experienced enough to hold that.\r\n");
      return;
    }
  }

  /*
  if (GET_OBJ_TYPE(obj) != ITEM_LIGHT && GET_OBJ_SIZE(obj) > get_size(ch)) {
   send_to_char(ch, "Seems like it is too big for you.\r\n");
   return;
  }
  if (GET_OBJ_SIZE(obj) < get_size(ch) && GET_OBJ_TYPE(obj) != ITEM_LIGHT) {
   send_to_char(ch, "Seems like it is too small for you.\r\n");
   return;
  }
   */

  wear_message(ch, obj, where);
  obj_from_char(obj);
  equip_char(ch, obj, where);
}

int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg) {
  int where = -1;

  const char *keywords[] = {
      "!RESERVED!",   "finger",     "!RESERVED!", "neck",  "!RESERVED!",
      "body",         "head",       "legs",       "feet",  "hands",
      "arms",         "shield",     "about",      "waist", "wrist",
      "!RESERVED!",   "!RESERVED!", "!RESERVED!", "back",  "ear",
      "\r!RESERVED!", "shoulders",  "scouter",    "\n"};

  if (!arg || !*arg) {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER) && BODY_FLAGGED(ch, WEAR_FINGER_R))
      return WEAR_FINGER_R;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK) && BODY_FLAGGED(ch, WEAR_NECK_1))
      return WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY) && BODY_FLAGGED(ch, WEAR_BODY))
      return WEAR_BODY;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD) && BODY_FLAGGED(ch, WEAR_HEAD))
      return WEAR_HEAD;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS) && BODY_FLAGGED(ch, WEAR_LEGS))
      return WEAR_LEGS;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET) && BODY_FLAGGED(ch, WEAR_FEET))
      return WEAR_FEET;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS) && BODY_FLAGGED(ch, WEAR_HANDS))
      return WEAR_HANDS;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS) && BODY_FLAGGED(ch, WEAR_ARMS))
      return WEAR_ARMS;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD) && BODY_FLAGGED(ch, WEAR_WIELD2))
      return WEAR_WIELD2;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT) && BODY_FLAGGED(ch, WEAR_ABOUT))
      return WEAR_ABOUT;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST) && BODY_FLAGGED(ch, WEAR_WAIST))
      return WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST) && BODY_FLAGGED(ch, WEAR_WRIST_R))
      return WEAR_WRIST_R;
    if (CAN_WEAR(obj, ITEM_WEAR_HOLD) && BODY_FLAGGED(ch, WEAR_WIELD2))
      return WEAR_WIELD2;
    if (CAN_WEAR(obj, ITEM_WEAR_PACK) && BODY_FLAGGED(ch, WEAR_BACKPACK))
      return WEAR_BACKPACK;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR) && BODY_FLAGGED(ch, WEAR_EAR_R))
      return WEAR_EAR_R;
    if (CAN_WEAR(obj, ITEM_WEAR_SH) && BODY_FLAGGED(ch, WEAR_SH))
      return WEAR_SH;
    if (CAN_WEAR(obj, ITEM_WEAR_EYE) && BODY_FLAGGED(ch, WEAR_EYE))
      return WEAR_EYE;
  } else if (((where = search_block(arg, keywords, FALSE)) < 0) ||
             (*arg == '!')) {
    send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n", arg);
    return -1;
  }
  return (where);
}

ACMD(do_wear) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Wear what?\r\n");
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char(
        ch,
        "You can't specify the same body location for more than one item!\r\n");
    return;
  }
  if (dotmode == FIND_ALL) {
    char_inventory_iterate(ch, [&](auto obj) {
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
        if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj)) {
          act("$p: you are not experienced enough to use that.", FALSE, ch, obj,
              0, TO_CHAR);
          send_to_char(ch, "You need to be at least %d level to use it.\r\n",
                       GET_OBJ_LEVEL(obj));
        } else if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
          act("$p: it seems to be broken.", FALSE, ch, obj, 0, TO_CHAR);
        } else if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
          act("$p: it seems to be fake...", FALSE, ch, obj, 0, TO_CHAR);
        } else {
          items_worn++;
          if (!is_proficient_with_armor(ch,
                                        GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) &&
              GET_OBJ_TYPE(obj) == ITEM_ARMOR)
            send_to_char(
                ch,
                "You have no proficiency with this type of armor.\r\nYour "
                "fighting and physical skills will be greatly impeded.\r\n");
          perform_wear(ch, obj, where);
        }
      }
      return true;
    });
    if (!items_worn)
      send_to_char(ch, "You don't seem to have anything wearable.\r\n");
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char(ch, "Wear all of what?\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, inv_for_char(ch))))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else
      while (obj) {
        {
          struct obj_data *search = obj;
          bool found_next = false;
          next_obj = nullptr;
          char_inventory_iterate(ch, [&](auto candidate) {
            if (candidate == search) {
              found_next = true;
              return true;
            }
            if (found_next && isname(arg1, candidate->name)) {
              next_obj = candidate;
              return false;
            }
            return true;
          });
        }
        if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
          if (!is_proficient_with_armor(ch,
                                        GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) &&
              GET_OBJ_TYPE(obj) == ITEM_ARMOR)
            send_to_char(
                ch,
                "You have no proficiency with this type of armor.\r\nYour "
                "fighting and physical skills will be greatly impeded.\r\n");
          perform_wear(ch, obj, where);
        } else
          act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
        obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, inv_for_char(ch))))
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
      send_to_char(ch, "But it seems to be broken!\r\n");
    else if (OBJ_FLAGGED(obj, ITEM_FORGED))
      send_to_char(ch, "But it seems to be fake!\r\n");
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0) {
        if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) &&
            GET_OBJ_TYPE(obj) == ITEM_ARMOR)
          send_to_char(
              ch, "You have no proficiency with this type of armor.\r\nYour "
                  "fighting and physical skills will be greatly impeded.\r\n");
        perform_wear(ch, obj, where);
      } else if (!*arg2)
        act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}

ACMD(do_wield) {
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Wield what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch))))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char(ch, "You can't wield that.\r\n");
    else if (GET_OBJ_WEIGHT(obj) > max_carry_weight(ch))
      send_to_char(ch, "It's too heavy for you to use.\r\n");
    else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
      send_to_char(ch, "But it seems to be broken!\r\n");
    else if (OBJ_FLAGGED(obj, ITEM_FORGED))
      send_to_char(ch, "But it seems to be fake!\r\n");
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else if (PLR_FLAGGED(ch, PLR_THANDW))
      send_to_char(ch,
                   "You are holding a weapon with two hands right now!\r\n");
    else {
      if (!IS_NPC(ch) &&
          !is_proficient_with_weapon(ch, GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)) &&
          GET_OBJ_TYPE(obj) == ITEM_ARMOR)
        send_to_char(
            ch, "You have no proficiency with this type of weapon.\r\nYour "
                "attack accuracy will be greatly reduced.\r\n");
      perform_wear(ch, obj, WEAR_WIELD1);
    }
  }
}

ACMD(do_grab) {
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hold what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch))))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
    send_to_char(ch, "You are not experienced enough to use that.\r\n");
  else if (PLR_FLAGGED(ch, PLR_THANDW))
    send_to_char(ch,
                 "You are wielding a weapon with both hands currently.\r\n");
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
      perform_wear(ch, obj, WEAR_WIELD2);
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) > 0 ||
          GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) < 0) {
        act("@wYou light $p@w.", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@w lights $p@w.", TRUE, ch, obj, 0, TO_ROOM);
      }
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) == 0) {
        act("@wYou try to light $p@w but it is burnt out.", TRUE, ch, obj, 0,
            TO_CHAR);
        act("@C$n@w tries to light $p@w but nothing happens.", TRUE, ch, obj, 0,
            TO_ROOM);
      }
    } else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
          GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
          GET_OBJ_TYPE(obj) != ITEM_POTION)
        send_to_char(ch, "You can't hold that.\r\n");
      else
        perform_wear(ch, obj, WEAR_WIELD2);
    }
  }
}

void perform_remove(struct char_data *ch, int pos) {
  struct obj_data *obj;

  int64_t previous = GET_HIT(ch);

  if (!(obj = GET_EQ(ch, pos)))
    mud_log("SYSERR: perform_remove: bad pos %d passed.", pos);
  /*  SYSERR_DESC:
   *  This error occurs when perform_remove() is passed a bad 'pos'
   *  (location) to remove an object from.
   */
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_ADMLEVEL(ch) < 1)
    act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    act("$p: your arms are full!", FALSE, ch, obj, 0, TO_CHAR);
  else {
    if (!remove_otrigger(obj, ch))
      return;

    if (pos == WEAR_WIELD1 && PLR_FLAGGED(ch, PLR_THANDW)) {
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THANDW);
    }
    obj_to_char(unequip_char(ch, pos), ch);
    act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
    if (previous > GET_HIT(ch)) {
      char drop[MAX_INPUT_LENGTH];
      sprintf(
          drop,
          "@RYour powerlevel has dropped from removing $p@R! @D[@r-%s@D]\r\n",
          add_commas(previous - GET_HIT(ch)));
      act(drop, FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}

ACMD(do_remove) {
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  int i, dotmode, found = 0, msg;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Remove what?\r\n");
    return;
  }

  if (!(obj = char_inventory_search_type(ch, ITEM_BOARD, FALSE, 0))) {
    obj = obj_contents_search_type(room_contents_get(char_room_get(ch)), ITEM_BOARD,
                                   FALSE, 0);
  }
  found = obj ? 1 : 0;

  if (found) {
    if (!isdigit(*arg) || (!(msg = atoi(arg)))) {
      found = 0;
    } else {
      remove_board_msg(GET_OBJ_VNUM(obj), ch, msg);
    }
  }
  if (!found) {
    dotmode = find_all_dots(arg);

    if (dotmode == FIND_ALL) {
      found = 0;
      char_equipment_iterate(ch, [&](auto i, auto eq) {
        perform_remove(ch, i);
        found = 1;
        return true;
      });
      if (!found) {
        send_to_char(ch, "You're not using anything.\r\n");
      }
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
        send_to_char(ch, "Remove all of what?\r\n");
      } else {
        found = 0;
        char_equipment_iterate(ch, [&](auto i, auto eq) {
          if (CAN_SEE_OBJ(ch, eq) &&
              isname(arg, eq->name)) {
            perform_remove(ch, i);
            found = 1;
          }
          return true;
        });
        if (!found) {
          send_to_char(ch, "You don't seem to be using any %ss.\r\n", arg);
        }
      }
    } else {
      if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0) {
        send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
      } else {
        perform_remove(ch, i);
      }
    }
  }
}

ACMD(do_sac) {
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *j;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Sacrifice what?\n\r");
    return;
  }

  if (!(j = get_obj_in_list_vis(ch, arg, NULL, inv_for_room(char_room_get(ch)))) &&
      (!(j = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch))))) {
    send_to_char(ch, "It doesn't seem to be here.\n\r");
    return;
  }

  if (!CAN_WEAR(j, ITEM_WEAR_TAKE)) {
    send_to_char(ch, "You can't sacrifice that!\n\r");
    return;
  }

  act("$n sacrifices $p.", FALSE, ch, j, 0, TO_ROOM);

  if (!GET_OBJ_COST(j) && !IS_CORPSE(j)) {
    send_to_char(ch,
                 "Zizazat mocks your sacrifice. Try again, try harder.\r\n");
    return;
  }

  if (!IS_CORPSE(j)) {
    switch (rand_number(0, 5)) {
    case 0:
      send_to_char(ch,
                   "You sacrifice %s to the Gods.\r\nYou receive one zenni for "
                   "your humility.\r\n",
                   GET_OBJ_SHORT(j));
      char_stat_mod(ch, "money", 1);
      break;
    case 1:
      send_to_char(ch,
                   "You sacrifice %s to the Gods.\r\nThe Gods ignore your "
                   "sacrifice.\r\n",
                   GET_OBJ_SHORT(j));
      break;
    case 2:
      send_to_char(ch,
                   "You sacrifice %s to the Gods.\r\nZizazat gives you %d "
                   "experience points.\r\n",
                   GET_OBJ_SHORT(j), (2 * GET_OBJ_COST(j)));
      char_stat_mod(ch, "experience", (2 * GET_OBJ_COST(j)));
      break;
    case 3:
      send_to_char(ch,
                   "You sacrifice %s to the Gods.\r\nYou receive %d experience "
                   "points.\r\n",
                   GET_OBJ_SHORT(j), GET_OBJ_COST(j));
      char_stat_mod(ch, "experience", GET_OBJ_COST(j));
      break;
    case 4:
      send_to_char(ch,
                   "Your sacrifice to the Gods is rewarded with %d zenni.\r\n",
                   GET_OBJ_COST(j));
      char_stat_mod(ch, "money", GET_OBJ_COST(j));
      break;
    case 5:
      send_to_char(ch,
                   "Your sacrifice to the Gods is rewarded with %d zenni\r\n",
                   (2 * GET_OBJ_COST(j)));
      char_stat_mod(ch, "money", (2 * GET_OBJ_COST(j)));
      break;
    default:
      send_to_char(ch,
                   "You sacrifice %s to the Gods.\r\nYou receive one zenni for "
                   "your humility.\r\n",
                   GET_OBJ_SHORT(j));
      char_stat_mod(ch, "money", 1);
      break;
    }
  } else {
    /* No longer transfer corpse contents to room. Sac it, sac it all. */
    send_to_char(ch, "You send the corpse on to the next life!\r\n");
  }
  extract_obj(j);
}

extern "C" void char_item_legacy_command(struct char_data *ch,
                                          const char *command,
                                          const char *argument) {
  char arg[MAX_INPUT_LENGTH];

  snprintf(arg, sizeof(arg), "%s", argument ? argument : "");

  if (!command || !*command)
    return;

  if (!strcasecmp(command, "refuel")) {
    do_refuel(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "twohand")) {
    do_twohand(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "put")) {
    do_put(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "get") || !strcasecmp(command, "take")) {
    do_get(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "drop")) {
    if (!dump_drop_special_try(ch, arg))
      do_drop(ch, arg, 0, SCMD_DROP);
  } else if (!strcasecmp(command, "donate")) {
    do_drop(ch, arg, 0, SCMD_DONATE);
  } else if (!strcasecmp(command, "junk")) {
    do_drop(ch, arg, 0, SCMD_JUNK);
  } else if (!strcasecmp(command, "give")) {
    do_give(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "pour")) {
    do_pour(ch, arg, 0, SCMD_POUR);
  } else if (!strcasecmp(command, "fill")) {
    do_pour(ch, arg, 0, SCMD_FILL);
  } else if (!strcasecmp(command, "wear")) {
    do_wear(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "wield")) {
    do_wield(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "grab") || !strcasecmp(command, "hold")) {
    do_grab(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "remove")) {
    do_remove(ch, arg, 0, 0);
  } else if (!strcasecmp(command, "sac") ||
             !strcasecmp(command, "sacrifice")) {
    do_sac(ch, arg, 0, 0);
  }
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const int max_carry_load[] = {
    0,   10,  20,  30,  40,  50,   60,   70,   80,   90,  100,
    115, 130, 150, 175, 200, 230,  260,  300,  350,  400, 460,
    520, 600, 700, 800, 920, 1040, 1200, 1400, 1640,
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int64_t max_carry_weight(struct char_data *ch) {
  int64_t abil;
  int total;
  /*  abil = MAX(0, MIN(100, GET_STR(ch)));
    total = 1;
    while (abil > 30) {
      abil -= 10;
      total *= 4;
    }*/

  abil = (GET_MAX_HIT(ch) / 200) + (GET_STR(ch) * 50);
  total = 1;
  return (total * abil);
}
