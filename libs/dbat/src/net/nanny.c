/* ************************************************************************
*   File: nanny.c                                       Part of CircleMUD *
*  Usage: connection state machine: login, account menu, game menu        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/interpreter.h"
#include "dbat/boards.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/config.h"
#include "dbat/act.informative.h"
#include "dbat/dg_scripts.h"
#include "dbat/objsave.h"
#include "dbat/ban.h"
#include "dbat/broadcast.h"
#include "dbat/act.wizard.h"
#include "dbat/mail.h"
#include "dbat/alias.h"
#include "dbat/players.h"
#include "dbat/assedit.h"
#include "dbat/obj_edit.h"
#include "dbat/improved-edit.h"
#include "dbat/aedit.h"
#include "dbat/gedit.h"
#include "dbat/hedit.h"
#include "dbat/hsedit.h"

#include <unistd.h>
#include "nanny_internal.h"

static void payout(int num);

/* Recounts the players online (above level 3) and updates the all-time record.
   Returns the record as it stood before the count raised it. */
static int update_player_count(void)
{
  int count = 0, oldcount = HIGHPCOUNT;

  for (struct descriptor_data *kd = descriptor_list; kd; kd = kd->next) {
    if (!kd->character) {
      continue;
    }
    if (!char_is_npc(kd->character) && char_get_level(kd->character) > 3) {
      count += 1;
    }

    if (count > PCOUNT) {
      PCOUNT = count;
    }

    if (PCOUNT >= HIGHPCOUNT) {
      oldcount   = HIGHPCOUNT;
      HIGHPCOUNT = PCOUNT;
      PCOUNTDATE = time(0);
    }
  }
  return oldcount;
}

/* RPP rewards for a player count near, at, or above the record. */
static void pay_player_count_rewards(int oldcount)
{
  if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4) {
    payout(0);
  }
  if (PCOUNT == HIGHPCOUNT) {
    payout(1);
  }
  if (PCOUNT > oldcount) {
    payout(2);
  }
}

/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */

/* This function needs to die. */
static int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}

#define RECON    1
#define USURP    2
#define UNSWITCH 3

/* This function seems a bit over-extended. */
static int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode                 = 0;
  int id                   = char_get_idnum(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (char_get_idnum(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      desc_set_connected(k, CON_CLOSE);
      if (!target) {
        target = k->original;
        mode   = UNSWITCH;
      }
      if (k->character)
        k->character->desc = NULL;
      k->character = NULL;
      k->original  = NULL;
    } else if (k->character && char_get_idnum(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && char_get_idnum(k->character) == id) {
      /* Character taking over their own body. */

      if (!target && desc_get_connected(k) == CON_PLAYING) {
        write_to_output(k, "\r\nThis body has been usurped!\r\n");
        if (k->snoop_by) {
          k->snoop_by->snooping = d;
          d->snoop_by           = k->snoop_by;
          k->snoop_by           = NULL;
        }
        target = k->character;
        mode   = USURP;
      }
      k->character->desc = NULL;
      k->character       = NULL;
      k->original        = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      desc_set_connected(k, CON_CLOSE);
    }
  }

  /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (char_is_npc(ch))
      continue;
    if (char_get_idnum(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode   = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (char_get_in_room(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target)
    return (0);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character        = target;
  d->character->desc  = d;
  d->original         = NULL;
  d->character->timer = 0;
  char_plr_set(d->character, PLR_MAILING, false);
  char_plr_set(d->character, PLR_WRITING, false);
  char_aff_set(d->character, AFF_GROUP, false);
  desc_set_connected(d, CON_PLAYING);

  switch (mode) {
    case RECON:
      write_to_output(d, "Reconnecting.\r\n");
      pay_player_count_rewards(update_player_count());
      d->character->time.logon = time(0);
      act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
      mudlog(LOG_NRM, MAX(ADMLVL_NONE, char_get_invis_level(d->character)), TRUE, "%s [%s] has reconnected.", char_get_name(d->character), d->host);
      d->character->rp = d->rpp;
      if (has_mail(char_get_idnum(d->character)))
        write_to_output(d, "You have mail waiting.\r\n");
      board_login_notices(d->character);
      if (NEWSUPDATE > char_get_lastpl(d->character))
        send_to_char(d->character, "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n", LASTNEWS);

      if (LASTINTEREST != 0 && LASTINTEREST > char_get_lastint(d->character)) {
        int diff = (LASTINTEREST - char_get_lastint(d->character));
        int days = MAX(1, MIN(3, diff / 86400)); /* one to three days' interest, as at login */
        char_set_lastint(d->character, LASTINTEREST);
        int base_interest = char_get_bank_interest(d->character);
        if (base_interest) {
          int total_interest = base_interest * days;
          char_set_bank_gold(d->character, char_get_bank_gold(d->character) + (total_interest));
          send_to_char(d->character, "Interest happened while you were away, %d times.\r\n"
                                     "@cBank Interest@D: @Y%s@n\r\n",
                       days, add_commas(total_interest));
        }
      }
      break;
    case USURP:
      write_to_output(d, "You take over your own body, already in use!\r\n");
      act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
          "$n's body has been taken over by a new spirit!",
          TRUE, d->character, 0, 0, TO_ROOM);
      d->character->rp = d->rpp;
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(d->character)), TRUE,
             "%s has re-logged in ... disconnecting old socket.", char_get_name(d->character));
      break;
    case UNSWITCH:
      write_to_output(d, "Reconnecting to unswitched char.");
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(d->character)), TRUE, "%s [%s] has reconnected.", char_get_name(d->character), d->host);
      break;
  }

  return (1);
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game(struct descriptor_data *d)
{
  int load_result;
  IDXTYPE load_room;
  struct char_data *check;

  reset_char(d->character);
  read_aliases(d->character);

  racial_body_parts(d->character);

  if (char_plr_flagged(d->character, PLR_INVSTART))
    char_set_invis_level(d->character, char_get_level(d->character));

  /*
       * We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE.
       */

  if ((load_room = char_get_load_room(d->character)) != NOWHERE)
    load_room = real_room(load_room);

  /* If char was saved with NOWHERE, or real_room above failed... */
  if (load_room == NOWHERE) {
    if (char_get_admlevel(d->character))
      load_room = real_room(CONFIG_IMMORTAL_START);
    else
      load_room = real_room(CONFIG_MORTAL_START);
  }

  if (char_plr_flagged(d->character, PLR_FROZEN))
    load_room = real_room(CONFIG_FROZEN_START);

  d->character->next = character_list;
  character_list     = d->character;
  char_to_room(d->character, load_room);
  load_result = Crash_load(d->character);
  if (d->character->player_specials->host) {
    free(d->character->player_specials->host);
    d->character->player_specials->host = NULL;
  }
  d->character->player_specials->host = strdup(d->host);
  char_set_id(d->character, char_get_idnum(d->character));
  /* find_char helper */
  add_to_lookup_table(char_get_id(d->character), MOB_TRIGGER, (void *)d->character);
  read_saved_vars(d->character);
  /*load_char_pets(d->character);*/
  for (check = character_list; check; check = check->next)
    if (!check->master && char_is_npc(check) && check->master_id == char_get_idnum(d->character) &&
        char_aff_flagged(check, AFF_CHARM) && !circle_follow(check, d->character))
      add_follower(check, d->character);
  save_char(d->character);

  if (d->customfile != 1) {
    customCreate(d);
    userWrite(d, 0, 0, 0, "index");
  }

  if (char_plr_flagged(d->character, PLR_RARM)) {
    char_set_limb_condition(d->character, (1) - 1, 100);
    char_plr_set(d->character, PLR_RARM, false);
  }
  if (char_plr_flagged(d->character, PLR_LARM)) {
    char_set_limb_condition(d->character, (2) - 1, 100);
    char_plr_set(d->character, PLR_LARM, false);
  }
  if (char_plr_flagged(d->character, PLR_LLEG)) {
    char_set_limb_condition(d->character, (4) - 1, 100);
    char_plr_set(d->character, PLR_LLEG, false);
  }
  if (char_plr_flagged(d->character, PLR_RLEG)) {
    char_set_limb_condition(d->character, (3) - 1, 100);
    char_plr_set(d->character, PLR_RLEG, false);
  }
  char_set_combine(d->character, -1);
  char_set_sleeptime(d->character, 8);
  char_set_foodr(d->character, 2);
  if (char_aff_flagged(d->character, AFF_FLYING)) {
    char_set_altitude(d->character, 1);
  } else {
    char_set_altitude(d->character, 0);
  }
  if (char_aff_flagged(d->character, AFF_POSITION)) {
    char_aff_set(d->character, AFF_POSITION, false);
  }
  if (char_aff_flagged(d->character, AFF_SANCTUARY)) {
    char_aff_set(d->character, AFF_SANCTUARY, false);
  }
  if (char_aff_flagged(d->character, AFF_ZANZOKEN)) {
    char_aff_set(d->character, AFF_ZANZOKEN, false);
  }
  if (char_plr_flagged(d->character, PLR_KNOCKED)) {
    char_plr_set(d->character, PLR_KNOCKED, false);
  }
  if (char_is_android(d->character) && !char_aff_flagged(d->character, AFF_INFRAVISION)) {
    char_aff_set(d->character, AFF_INFRAVISION, true);
  }
  char_set_absorbing(d->character, NULL);
  char_set_absorbby(d->character, NULL);
  char_set_sits(d->character, NULL);
  char_set_blocked(d->character, NULL);
  char_set_blocks(d->character, NULL);
  char_set_overf(d->character, FALSE);
  char_set_spam(d->character, 0);
  char_set_rage_meter(d->character, 0);
  if (!d->character->affected) {
    if (char_aff_flagged(d->character, AFF_HEALGLOW)) {
      char_aff_set(d->character, AFF_HEALGLOW, false);
    }
  }
  if (char_aff_flagged(d->character, AFF_HAYASA)) {
    char_set_speedboost(d->character, char_get_speedcalc(d->character) * 0.5);
  } else {
    char_set_speedboost(d->character, 0);
  }
  if (char_get_trp(d->character) < char_get_rp(d->character)) {
    char_set_trp(d->character, char_get_rp(d->character));
  }

  if (char_is_namek(d->character) && char_get_conditions(d->character, HUNGER) >= 0) {
    char_set_conditions(d->character, HUNGER, -1);
  }

  if (char_plr_flagged(d->character, PLR_HEALT)) {
    char_plr_set(d->character, PLR_HEALT, false);
  }

  if (readIntro(d->character, d->character) == 2) {
    introCreate(d->character);
  }
  if (read_sense_memory(d->character, d->character) == 2) {
    senseCreate(d->character);
  }
  if (char_get_admlevel(d->character) > 0) {
    d->level = 1;
  }

  d->character->rp    = d->rpp;
  d->character->rbank = d->rbank;
  if (char_moon_ok(d->character)) {
    oozaru_transform(d->character);
  } else {
    oozaru_revert(d->character);
  }
  if (char_is_hoshijin(d->character)) {
    if (time_info.day <= 14) {
      star_phase(d->character, 1);
    } else if (time_info.day <= 21) {
      star_phase(d->character, 2);
    } else {
      star_phase(d->character, 0);
    }
  }

  if (char_is_icer(d->character) && !char_get_skill(d->character, SKILL_TAILWHIP)) {
    int numb = rand_number(20, 30);
    char_set_skills_base(d->character, SKILL_TAILWHIP, numb);
  } else if (!char_is_icer(d->character) && char_get_skill(d->character, SKILL_TAILWHIP)) {
    char_set_skills_base(d->character, SKILL_TAILWHIP, 0);
  }

  if (char_is_mutant(d->character) && (char_get_genome(d->character, 0) == 9 || char_get_genome(d->character, 1) == 9) && !char_get_skill(d->character, SKILL_TELEPATHY)) {
    char_set_skills_base(d->character, SKILL_TELEPATHY, 50);
  }

  if (char_is_bio(d->character) && (char_get_genome(d->character, 0) == 7 || char_get_genome(d->character, 1) == 7) && !char_get_skill(d->character, SKILL_TELEPATHY) && !char_get_skill(d->character, SKILL_FOCUS)) {
    char_set_skills_base(d->character, SKILL_TELEPATHY, 30);
    char_set_skills_base(d->character, SKILL_FOCUS, 30);
  }

  char_set_combo(d->character, -1);
  return load_result;
}

static void payout(int num)
{
  struct descriptor_data *k;
  if (LASTPAYOUT == 0) {
    LASTPAYOUT  = time(0) + 86400;
    LASTPAYTYPE = num;
  } else if (num > LASTPAYTYPE) {
    LASTPAYOUT  = time(0) + 86400;
    LASTPAYTYPE = num;
  } else if (LASTPAYOUT <= time(0)) {
    LASTPAYOUT  = time(0) + 86400;
    LASTPAYTYPE = num;
  }
  for (k = descriptor_list; k; k = k->next) {
    if (char_get_admlevel(k->character) <= 0 && desc_is_playing(k) && char_get_rewtime(k->character) < LASTPAYOUT) {
      if (num == 0) {
        k->rpp += 1;
        char_set_rp(k->character, k->rpp);
        char_set_trp(k->character, char_get_trp(k->character) + (1));
        userWrite(k, 0, 0, 0, "index");
        send_to_char(k->character, "@D[@G+ 1 RPP@D] @cA total logon count within 4 of the highest has been achieved.@n\r\n");
      } else if (num == 1) {
        k->rpp += 2;
        char_set_rp(k->character, k->rpp);
        char_set_trp(k->character, char_get_trp(k->character) + (2));
        userWrite(k, 0, 0, 0, "index");
        send_to_char(k->character, "@D[@G+ 2 RPP@D] @cThe total logon count has tied with the highest ever.@n\r\n");
      } else {
        k->rpp += 3;
        char_set_rp(k->character, k->rpp);
        char_set_trp(k->character, char_get_trp(k->character) + (3));
        userWrite(k, 0, 0, 0, "index");
        send_to_char(k->character, "@D[@G+ 3 RPP@D] @cA new logon count record has been achieved!@n\r\n");
      }
      char_set_rewtime(k->character, LASTPAYOUT);
    }
  }
}

/* CON_GET_NAME: wait for input of name */
static void state_get_name(struct descriptor_data *d, char *arg)
{
  int player_i;

  if (d->character) {
    free_char(d->character);
  }
  if (!d->character) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
    char_prf_set(d->character, PRF_COLOR, true);
  }
  /*  Check for other chars already logged in */
  /* Jamdog - 3rd May 2007 */
  char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];
  if (d->writenew > 0) {
    if (!*arg) {
      write_to_output(d, "Enter name: ");
      return;
    }
    free(d->loadplay);
    d->loadplay = strdup(arg);
  }
  if ((_parse_name(d->loadplay, tmp_name)) || strlen(tmp_name) < 2 ||
      strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
      fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) { /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
    write_to_output(d, "Invalid name, please try another.\r\nName: ");
    return;
  }
  if (d->writenew > 0 && (player_i = load_char(tmp_name, d->character)) > -1) {
    userRead(d);
    write_to_output(d, "That character is already taken.\r\n");
    d->writenew = 0;
    desc_set_connected(d, CON_UMENU);
    return;
  } else {
    if ((player_i = load_char(tmp_name, d->character)) > -1) {
      if (d->writenew > 0) {
        write_to_output(d, "That character is already taken.\r\n");
        userRead(d);
        desc_set_connected(d, CON_UMENU);
        return;
      }
      char_set_pfilepos(d->character, player_i);
      if (char_plr_flagged(d->character, PLR_DELETED)) {

        /* make sure old files are removed so the new player doesn't get
	     the deleted player's equipment (this should probably be a
	     stock behavior)
	  */
        if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
          remove_player(player_i);

        /* We get a false positive from the original deleted character. */
        free_char(d->character);
        /* Check for multiple creations... */
        if (!Valid_Name(tmp_name)) {
          write_to_output(d, "@YInvalid name@n, please try @Canother.@n\r\nName: ");
          return;
        }
        CREATE(d->character, struct char_data, 1);
        clear_char(d->character);
        CREATE(d->character->player_specials, struct player_special_data, 1);
        d->character->desc = d;
        CREATE(d->character->name, char, strlen(tmp_name) + 1);
        strcpy(d->character->name, CAP(tmp_name)); /* strcpy: OK (size checked above) */
        char_set_pfilepos(d->character, player_i);
        char_prf_set(d->character, PRF_COLOR, true);
        display_races(d);
        d->character->rp = d->rpp;
        desc_set_connected(d, CON_QRACE);
      } else {
        /* undo it just in case they are set */
        char_plr_set(d->character, PLR_WRITING, false);
        char_plr_set(d->character, PLR_MAILING, false);
        char_plr_set(d->character, PLR_CRYO, false);
        char_aff_set(d->character, AFF_GROUP, false);
        if (isbanned(d->host) == BAN_SELECT &&
            !char_plr_flagged(d->character, PLR_SITEOK)) {
          write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n");
          mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "Connection attempt for %s denied from %s", char_get_name(d->character), d->host);
          desc_set_connected(d, CON_CLOSE);
          return;
        } else if (char_get_level(d->character) < circle_restrict && circle_restrict < 101) {
          userRead(d);
          write_to_output(d, "The game is temporarily restricted to at least %d level.\r\n", circle_restrict);
          desc_set_connected(d, CON_UMENU);
          mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "Request for character load denied for %s [%s] (wizlock)", char_get_name(d->character), d->host);
          return;
        }
        /* check and make sure no other copies of this player are logged in */
        else if (perform_dupe_check(d)) {
          return;
        } else {
          d->idle_tics = 0;
          write_to_output(d, "\r\n%s\r\n%s", motd, CONFIG_MENU);
          d->character->rp = d->rpp;
          desc_set_connected(d, CON_MENU);
        }
      }
    } else {
      if (d->writenew <= 0) {
        empty_account_slots_named(d, d->loadplay);
        free(d->loadplay);
        d->loadplay = NULL;
        userWrite(d, 0, 0, 0, "index");
        userRead(d);
        write_to_output(d, "Character missing. Emptying slot.\r\n");
        desc_set_connected(d, CON_UMENU);
      } else {
        /* player unknown -- make new character */

        /* Check for multiple creations of a character. */
        if (!Valid_Name(tmp_name)) {
          write_to_output(d, "Invalid name, please try another.\r\nName: ");
          return;
        }
        CREATE(d->character->name, char, strlen(tmp_name) + 1);
        strcpy(d->character->name, CAP(tmp_name)); /* strcpy: OK (size checked above) */

        display_races(d);
        d->character->rp = d->rpp;
        set_account_slot(d, d->writenew, d->character->name);
        userWrite(d, 0, 0, 0, "index");
        desc_set_connected(d, CON_QRACE);
      }
    }
  }
}

/* CON_NAME_CNFRM: wait for conf. of new name */
static void state_name_cnfrm(struct descriptor_data *d, char *arg)
{
  if (toupper(*arg) == 'Y') {
    if (isbanned(d->host) >= BAN_NEW) {
      mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "Request for new char %s denied from [%s] (siteban)", char_get_pc_name(d->character), d->host);
      write_to_output(d, "Sorry, new characters are not allowed from your site!\r\n");
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    if (circle_restrict) {
      write_to_output(d, "Sorry, new players can't be created at the moment.\r\n");
      mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "Request for new char %s denied from [%s] (wizlock)", char_get_pc_name(d->character), d->host);
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    d->idle_tics = 0;
    write_to_output(d, "@MNew character.@n\r\nGive me a @gpassword@n for @C%s@n: ", char_get_pc_name(d->character));
    desc_set_connected(d, CON_NEWPASSWD);
  } else if (*arg == 'n' || *arg == 'N') {
    write_to_output(d, "Okay, what IS it, then? ");
    free(d->character->name);
    d->character->name = NULL;
    desc_set_connected(d, CON_GET_NAME);
  } else
    write_to_output(d, "Please type Yes or No: ");
}

/* CON_GET_USER */
static void state_get_user(struct descriptor_data *d, char *arg)
{
  if (isbanned(d->host)) {
    write_to_output(d, "You have been banned. Have a nice day.\r\n");
    desc_set_connected(d, CON_CLOSE);
    return;
  }
  if (!*arg) {
    write_to_output(d, "Enter your desired username or the username you have already made.\nUsername?\r\n");
    return;
  } else if (!strcasecmp(arg, "index")) {
    write_to_output(d, "Try again, username?\r\n");
    return;
  } else if (!Valid_Name(CAP(arg))) {
    write_to_output(d, "Invalid name. Username?\r\n");
    return;
  } else if (strstr(arg, " ")) {
    write_to_output(d, "No spaces. Username?\r\n");
    return;
  } else if (strstr(arg, "1") || strstr(arg, "2") || strstr(arg, "3") || strstr(arg, "4") || strstr(arg, "5") || strstr(arg, "6") || strstr(arg, "7") || strstr(arg, "8") || strstr(arg, "9") || strstr(arg, "0")) {
    write_to_output(d, "No numbers. Username?\r\n");
    return;
  } else if (strstr(arg, ".") || strstr(arg, ",") || strstr(arg, "!") || strstr(arg, "?") || strstr(arg, ";") || strstr(arg, ":") || strstr(arg, "'")) {
    write_to_output(d, "No punctuation. Username?\r\n");
    return;
  } else if (strlen(arg) < 3) {
    write_to_output(d, "Name must at least be 3 characters long, username?\r\n");
    return;
  } else if (strlen(arg) > 10) {
    write_to_output(d, "Name must be at most 10 characters long, username?\r\n");
    return;
  } else {
    if (!readUserIndex(arg)) {
      if (circle_restrict == 101) {
        write_to_output(d, "The mud is locked for mortals at this time.\r\n");
        write_to_output(d, "Please check: www.advent-truth.com/forum for further details.\r\n");
        send_to_imm("%s rejected by wizlock", CAP(arg));
        desc_set_connected(d, CON_CLOSE);
      } else {
        if (d->user) {
          free(d->user);
          d->user = NULL;
        }
        d->user = strdup(arg);
        write_to_output(d, "You want you user name to be, %s?\r\n", d->user);
        write_to_output(d, "Yes or no: \n");
        desc_set_connected(d, CON_USER_CONF);
      }
    } else {
      if (circle_restrict == 101) {
        if (d->user) {
          free(d->user);
          d->user = NULL;
        }
        d->user = strdup(arg);
        userLoad(d, d->user);
        if (d->level == 0) {
          write_to_output(d, "The mud is locked for mortals at this time.\r\n");
          send_to_imm("%s rejected by wizlock", d->user);
          desc_set_connected(d, CON_CLOSE);
        } else {
          write_to_output(d, "Password: \r\n");
          send_to_imm("Username, %s, logging in.", CAP(arg));
          desc_set_connected(d, CON_PASSWORD);
        }
      } else {
        if (d->user) {
          free(d->user);
          d->user = NULL;
        }
        userLoad(d, arg);
        write_to_output(d, "Password: \r\n");
        send_to_imm("Username, %s, logging in.", CAP(arg));
        desc_set_connected(d, CON_PASSWORD);
      }
    }
  }
}

/* CON_USER_CONF */
static void state_user_conf(struct descriptor_data *d, char *arg)
{
  if (!*arg) {
    write_to_output(d, "You want your user name to be, %s?\r\n", d->user);
    write_to_output(d, "Yes or no: \n");
    return;
  } else if (!strcasecmp(arg, "yes") || !strcasecmp(arg, "y")) {
    write_to_output(d, "User Account, %s, created.\r\nEnter Email:\n", d->user);
    write_to_output(d, "Remember your email must be valid and matching the example given.\n");
    write_to_output(d, "[Example: iovan@@advent-truth.com]\n");
    send_to_imm("Username, %s, creating.", CAP(d->user));
    desc_set_connected(d, CON_GET_EMAIL);
  } else if (!strcasecmp(arg, "no") || !strcasecmp(arg, "n")) {
    if (d->user) {
      free(d->user);
      d->user = NULL;
    }
    d->user = strdup("Empty");
    write_to_output(d, "Enter Username: \n");
    desc_set_connected(d, CON_GET_USER);
  } else {
    write_to_output(d, "You want you user name to be, %s?\r\n", d->user);
    write_to_output(d, "Yes or no: \n");
    return;
  }
}

/* User files hold one value per line, read back as a single word; get_line() skips lines starting with '*'. */
static bool fits_user_file(const char *value, size_t max_length)
{
  return *value != '*' && strlen(value) <= max_length && !strpbrk(value, " \t");
}

/* CON_GET_EMAIL */
static void state_get_email(struct descriptor_data *d, char *arg)
{
  if (readUserIndex(d->user) && !*arg) {
    write_to_output(d, "Email, or M for menu?\r\n");
    return;
  } else if (!*arg) {
    write_to_output(d, "Email?\r\n");
    return;
  } else if (!strcasecmp(arg, "M") && readUserIndex(d->user)) {
    userRead(d);
    desc_set_connected(d, CON_UMENU);
  } else if (!strstr(arg, ".com") && !strstr(arg, ".net") && !strstr(arg, ".org")) {
    write_to_output(d, "Improper email format missing '.com' or '.net' or '.org'. Email?\r\n");
    return;
  } else if (!fits_user_file(arg, MAX_EMAIL_LENGTH)) {
    write_to_output(d, "Emails can be at most %d characters, with no spaces, and can't start with '*'. Email?\r\n", MAX_EMAIL_LENGTH);
    return;
  } else if (readUserIndex(d->user)) {
    if (strstr(arg, "@")) {
      search_replace(arg, "@", "<AT>");
    }
    if (d->email) {
      free(d->email);
      d->email = NULL;
    }
    d->email = strdup(arg);
    userWrite(d, 0, 0, 0, "index");
    userRead(d);
    desc_set_connected(d, CON_UMENU);
  } else {
    if (strstr(arg, "@")) {
      search_replace(arg, "@", "<AT>");
    }
    write_to_output(d, "Your email is: %s\n", arg);
    if (d->email) {
      free(d->email);
      d->email = NULL;
    }
    d->email = strdup(arg);
    write_to_output(d, "Password: \r\n");
    desc_set_connected(d, CON_NEWPASSWD);
  }
}

/* CON_NEWPASSWD */
static void state_newpasswd(struct descriptor_data *d, char *arg)
{
  if (readUserIndex(d->user) && !*arg) {
    write_to_output(d, "Password, or M for menu?\r\n");
    return;
  } else if (!*arg) {
    write_to_output(d, "Password?\r\n");
    return;
  } else if (!strcasecmp(arg, "M") && readUserIndex(d->user)) {
    userRead(d);
    desc_set_connected(d, CON_UMENU);
  } else if (strlen(arg) > MAX_PWD_LENGTH && readUserIndex(d->user)) {
    write_to_output(d, "Password is too long. Password, or M for menu?\r\n");
    return;
  } else if (strlen(arg) > MAX_PWD_LENGTH) {
    write_to_output(d, "Password is too long. Password?\r\n");
    return;
  } else if (!fits_user_file(arg, MAX_PWD_LENGTH)) {
    write_to_output(d, "Passwords can't contain spaces or start with '*'. Password%s?\r\n", readUserIndex(d->user) ? ", or M for menu" : "");
    return;
  } else if (readUserIndex(d->user)) {
    write_to_output(d, "Your password and user account have been fully saved.\r\n");
    if (d->pass) {
      free(d->pass);
      d->pass = NULL;
    }
    d->pass = strdup(arg);
    userWrite(d, 0, 0, 0, "index");
    userRead(d);
    desc_set_connected(d, CON_UMENU);
  } else {
    write_to_output(d, "Your password and user account have been fully saved.\r\n");
    if (d->pass) {
      free(d->pass);
      d->pass = NULL;
    }
    d->pass = strdup(arg);
    userCreate(d);
    userRead(d);
    desc_set_connected(d, CON_UMENU);
  }
}

/* CON_PASSWORD */
static void state_password(struct descriptor_data *d, char *arg)
{
  struct descriptor_data *k;

  if (!*arg) {
    write_to_output(d, "Enter Password or Return:\r\n");
    write_to_output(d, "(Return will ask for a different username)\r\n");
    return;
  }
  if (!strcasecmp("return", arg)) {
    if (d->user) {
      free(d->user);
      d->user = NULL;
    }
    d->user = strdup("Empty");
    write_to_output(d, "Username?\r\n");
    desc_set_connected(d, CON_GET_USER);
    return;
  }
  if (!strcasecmp(d->pass, arg)) {
    for (k = descriptor_list; k; k = k->next) {
      if (k == d)
        continue;
      if (!k->user)
        continue;
      if (!d->user)
        continue;
      if (!strcasecmp(k->user, d->user)) {
        if (desc_get_connected(k) == CON_PLAYING) {
          desc_set_connected(k, CON_DISCONNECT);
          write_to_output(k, "Your account has been usurped by someone who knows its password!@n");
        } else {
          desc_set_connected(k, CON_CLOSE);
          write_to_output(k, "Your account has been usurped by someone who knows its password!@n");
        }
      }
    }
    userRead(d);
    desc_set_connected(d, CON_UMENU);
  } else {
    write_to_output(d, "Password is wrong. Password or Return?\r\n");
    write_to_output(d, "(Return will ask for a different username)\r\n");
    send_to_imm("Username, %s, password failure!", CAP(d->user));
    mud_log("%s BAD PASSWORD", CAP(d->user));
    return;
  }
}

/* Account menu B: a fourth or fifth character slot for 15 RPP. */
static void buy_slot(struct descriptor_data *d)
{
  if (d->total != 3 && d->total != 4) {
    userRead(d);
    write_to_output(d, "You can't have more than 5 character slots.\n");
  } else if (d->rpp < 15) {
    userRead(d);
    write_to_output(d, "You need at least 15 RPP to purchase a new character slot.\n");
  } else {
    d->rpp -= 15;
    d->total++;
    userWrite(d, 0, 0, 0, "index");
    userRead(d);
    write_to_output(d, "New slot purchased, -15 RPP.\n");
  }
}

/* Account menu 1-5: create a character in an empty slot, or load the one in it. */
static void choose_slot(struct descriptor_data *d, int n)
{
  const char *name;

  if (n < 1 || n > MAX_ACCOUNT_SLOTS) {
    userRead(d);
    write_to_output(d, "That is not an option.\n");
    return;
  }
  if (n > 3 && d->total < n) {
    userRead(d);
    write_to_output(d, "You only have %d character slots avaialable!\r\n", d->total);
    return;
  }
  name = *account_slot(d, n);
  if (!strcasecmp(name, "Empty")) {
    write_to_output(d, "Enter New Character Name: \n");
    d->writenew = n;
    desc_set_connected(d, CON_GET_NAME);
  } else if (lockRead(name) && d->level <= 0) {
    write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
  } else {
    write_to_output(d, "Loading character, %s.\r\n***Press Return***\n", name);
    free(d->loadplay);
    d->loadplay = strdup(name);
    d->writenew = 0;
    desc_set_connected(d, CON_GET_NAME);
  }
}

/* CON_UMENU */
static void state_umenu(struct descriptor_data *d, char *arg)
{
  if (!*arg) {
    userRead(d);
    return;
  }
  if (!strcasecmp(arg, "Q")) {
    write_to_output(d, "Thanks for visiting!\n");
    desc_set_connected(d, CON_CLOSE);
  } else if (!strcasecmp(arg, "P")) {
    write_to_output(d, "Enter New Password, or M for menu::\n");
    desc_set_connected(d, CON_NEWPASSWD);
  } else if (!strcasecmp(arg, "C")) {
    write_to_output(d, "\n");
    customRead(d, 0, NULL);
    write_to_output(d, "\r\n@n--Press Enter--\n@n");
    return;
  } else if (!strcasecmp(arg, "E")) {
    write_to_output(d, "Enter New Email, or M for menu:\n");
    desc_set_connected(d, CON_GET_EMAIL);
  } else if (!strcasecmp(arg, "D")) {
    write_to_output(d, "Are you sure you want to delete your user file and all its characters? Yes or no:\n");
    desc_set_connected(d, CON_DELCNF1);
  } else if (!strcasecmp(arg, "B")) {
    buy_slot(d);
  } else {
    choose_slot(d, atoi(arg));
  }
}

/* CON_RMOTD: read CR after printing motd */
static void state_rmotd(struct descriptor_data *d, char *arg)
{
  write_to_output(d, "%s", CONFIG_MENU);
  desc_set_connected(d, CON_MENU);
}

/* CON_MENU: get selection from main menu */
static void state_menu(struct descriptor_data *d, char *arg)
{
  int load_result = -1; /* Overloaded variable */
  int oldcount;

  switch (*arg) {
    case '0':
      write_to_output(d, "Goodbye.\r\n");
      desc_set_connected(d, CON_CLOSE);
      break;

    case '1':
      if (lockRead(char_get_name(d->character)) && d->level <= 0) {
        write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
        return;
      } else {
        load_result = enter_player_game(d);
        send_to_char(d->character, "%s", CONFIG_WELC_MESSG);
        act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
      }

      oldcount = update_player_count();

      d->character->time.logon = time(0);
      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      desc_set_connected(d, CON_PLAYING);
      pay_player_count_rewards(oldcount);

      if (char_get_level(d->character) == 0) {
        do_start(d->character);
        send_to_char(d->character, "%s", CONFIG_START_MESSG);
      }
      if (rroom_get_vnum(char_get_in_room(d->character)) <= 1 && char_get_load_room(d->character) != NOWHERE) {
        char_from_room(d->character);
        char_to_room(d->character, real_room(char_get_load_room(d->character)));
      } else if (rroom_get_vnum(char_get_in_room(d->character)) <= 1) {
        char_from_room(d->character);
        char_to_room(d->character, real_room(300));
      } else {
        look_at_room(char_get_in_room(d->character), d->character, 0);
      }
      if (has_mail(char_get_idnum(d->character)))
        send_to_char(d->character, "\r\nYou have mail waiting.\r\n");
      board_login_notices(d->character);
      if (NEWSUPDATE > char_get_lastpl(d->character))
        send_to_char(d->character, "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n", LASTNEWS);
      if (LASTINTEREST != 0 && LASTINTEREST > char_get_lastint(d->character)) {
        int diff = (LASTINTEREST - char_get_lastint(d->character));
        int mult = 0;
        while (diff > 0) {
          if ((diff - 86400) < 0 && mult == 0) {
            mult = 1;
          } else if ((diff - 86400) >= 0) {
            diff -= 86400;
            mult++;
          } else {
            diff = 0;
          }
        }
        if (mult > 3) {
          mult = 3;
        }
        char_set_lastint(d->character, LASTINTEREST);
        if (char_get_bank_gold(d->character) > 0) {
          int inc = ((char_get_bank_gold(d->character) / 100) * 2);
          if (inc >= 7500) {
            inc = 7500;
          }
          inc *= mult;
          char_set_bank_gold(d->character, char_get_bank_gold(d->character) + (inc));
          send_to_char(d->character, "Interest happened while you were away, %d times.\r\n"
                                     "@cBank Interest@D: @Y%s@n\r\n",
                       mult, add_commas(inc));
        }
      }
      if (!char_is_android(d->character)) {
        char buf3[MAX_INPUT_LENGTH];
        send_to_sense(0, "You sense someone appear suddenly", d->character);
        sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has suddenly entered your scouter detection range!@n.",
                add_commas(getCurPL(d->character)));
        send_to_scouter(buf3, d->character, 0, 0);
      }
      if (load_result == 2) { /* rented items lost */
        send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
                                   "Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      char_prf_set(d->character, PRF_BUILDWALK, false);
      if (!char_get_equipment(d->character, WEAR_WIELD1) && char_plr_flagged(d->character, PLR_THANDW)) {
        char_plr_set(d->character, PLR_THANDW, false);
      }
      break;

    case '2':
      if (d->character->description) {
        write_to_output(d, "Current description:\r\n%s", d->character->description);
        /*
	 * Don't free this now... so that the old description gets loaded
	 * as the current buffer in the editor.  Do setup the ABORT buffer
	 * here, however.
	 *
	 * free(d->character->description);
	 * d->character->description = NULL;
	 */
        d->backstr = strdup(d->character->description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(d);
      d->str     = &d->character->description;
      d->max_str = EXDSCR_LENGTH;
      desc_set_connected(d, CON_EXDESC);
      break;

    case '3':
      userRead(d);
      desc_set_connected(d, CON_UMENU);
      break;

    case '4':
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
                         "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
                         "Please type \"yes\" to confirm: ");
      desc_set_connected(d, CON_DELCNF2);
      break;

    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s\r\n%s", motd, CONFIG_MENU);
      break;
  }
}

/* CON_DELCNF1 */
static void state_delcnf1(struct descriptor_data *d, char *arg)
{
  if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
    write_to_output(d, "Your user and character files have been deleted. Good bye.\n");
    userDelete(d);
    desc_set_connected(d, CON_CLOSE);
  } else if (!strcmp(arg, "no") || !strcmp(arg, "NO")) {
    userRead(d);
    write_to_output(d, "Nothing was deleted. Phew.\n");
    desc_set_connected(d, CON_UMENU);
  } else {
    write_to_output(d, "Clearly type yes or no. Yes to delete and no to return to the menu.\nYes or no:\n");
    return;
  }
}

/* CON_DELCNF2 */
static void state_delcnf2(struct descriptor_data *d, char *arg)
{
  int player_i;

  if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
    if (char_plr_flagged(d->character, PLR_FROZEN)) {
      write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
                         "Character not deleted.\r\n\r\n");
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    if (char_get_admlevel(d->character) < ADMLVL_GRGOD)
      char_plr_set(d->character, PLR_DELETED, true);
    save_char(d->character);
    Crash_delete_file(char_get_name(d->character));
    /* If the selfdelete_fastwipe flag is set (in config.c), remove all
     the player's immediately
  */
    if (selfdelete_fastwipe)
      if ((player_i = get_ptable_by_name(char_get_name(d->character))) >= 0) {
        SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
        remove_player(player_i);
      }
    delete_aliases(char_get_name(d->character));
    delete_variables(char_get_name(d->character));
    delete_inv_backup(d->character);
    write_to_output(d, "Character '%s' deleted!\r\n", char_get_name(d->character));
    if (char_get_level(d->character) > 19 && !char_is_restricted_race(d->character) && !char_is_cheap_race(d->character)) {
      int refund = char_get_level(d->character) / 10;
      refund *= 2;
      write_to_output(d, "@D[@g%d RPP refunded to your account for your character's levels.@D]@n\r\n", refund);
      d->rpp += refund;
    }
    if (char_get_level(d->character) > 40 && char_is_cheap_race(d->character)) {
      int refund = char_get_level(d->character) / 10;
      refund *= 2;
      write_to_output(d, "@D[@g%d RPP refunded to your account for your character's levels.@D]@n\r\n", refund);
      d->rpp += refund;
    }
    if (char_get_level(d->character) <= 40 && char_is_cheap_race(d->character)) {
      write_to_output(d, "@D[@gSince your race doesn't cost RPP to level before 40 you are refunded 0 RPP.@D]@n\r\n");
    }
    int refund = race_rpp_refund(d->character->race);
    if (refund && char_get_level(d->character) > 1) {
      write_to_output(d, "@D[@g%d RPP refunded to your account for your %s character.@D]@n\r\n", refund, char_true_race_name(d->character));
      d->rpp += refund;
    }
    mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "User %s has deleted character %s (lev %d).", d->user, char_get_name(d->character), char_get_level(d->character));

    empty_account_slots_named(d, char_get_name(d->character));
    userWrite(d, 0, 0, 0, "index");
    userRead(d);
    desc_set_connected(d, CON_UMENU);
    return;
  } else {
    write_to_output(d, "\r\nCharacter not deleted.\r\n%s\r\n%s", motd, CONFIG_MENU);
    desc_set_connected(d, CON_MENU);
  }
}

static const struct nanny_state login_states[] = {
    {CON_GET_NAME,   state_get_name  },
    {CON_NAME_CNFRM, state_name_cnfrm},
    {CON_GET_USER,   state_get_user  },
    {CON_USER_CONF,  state_user_conf },
    {CON_GET_EMAIL,  state_get_email },
    {CON_NEWPASSWD,  state_newpasswd },
    {CON_PASSWORD,   state_password  },
    {CON_UMENU,      state_umenu     },
    {CON_RMOTD,      state_rmotd     },
    {CON_MENU,       state_menu      },
    {CON_DELCNF1,    state_delcnf1   },
    {CON_DELCNF2,    state_delcnf2   },
};

void nanny(struct descriptor_data *d, char *arg)
{
  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char *);
  } olc_functions[] = {
      {CON_OEDIT,    oedit_parse    },
      {CON_IEDIT,    oedit_parse    },
      {CON_ZEDIT,    zedit_parse    },
      {CON_SEDIT,    sedit_parse    },
      {CON_MEDIT,    medit_parse    },
      {CON_REDIT,    redit_parse    },
      {CON_CEDIT,    cedit_parse    },
      {CON_AEDIT,    aedit_parse    },
      {CON_TRIGEDIT, trigedit_parse },
      {CON_ASSEDIT,  assedit_parse  },
      {CON_GEDIT,    gedit_parse    },
      {CON_LEVELUP,  levelup_parse  },
      {CON_HEDIT,    hedit_parse    },
      {CON_HSEDIT,   hsedit_parse   },
      {CON_POBJ,     pobj_edit_parse},
      {-1,           NULL           }
  };

  skip_spaces(&arg);

  if (d->character == NULL) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
  }

  /*
   * Quick check for the OLC states.
   */
  for (int i = 0; olc_functions[i].state >= 0; i++)
    if (desc_get_connected(d) == olc_functions[i].state) {
      /* send context-sensitive help if need be */
      if (context_help(d, arg)) return;
      (*olc_functions[i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  for (size_t i = 0; i < sizeof(login_states) / sizeof(login_states[0]); i++)
    if (desc_get_connected(d) == login_states[i].state) {
      login_states[i].handler(d, arg);
      return;
    }
  if (chargen_parse(d, arg))
    return;

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  if (desc_get_connected(d) == CON_CLOSE)
    return;

  mud_log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
          desc_get_connected(d), d->character ? char_get_name(d->character) : "<unknown>");
  desc_set_connected(d, CON_DISCONNECT); /* Safest to do. */
}
