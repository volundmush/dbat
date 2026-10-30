/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/config.h"
#include "dbat/consts/maximums.h"
#include "dbat/utils.h"
#include "dbat/weather.h"
#include "dbat/command.h"

#include "dbat/act.comm.h"
#include "dbat/dg_comm.h"
#include "dbat/stringutils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/interpreter.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"'
#include "dbat/act.wizard.h"
#include "dbat/act.informative.h"
#include "dbat/dg_scripts.h"
#include "dbat/boards.h"
#include "dbat/improved-edit.h"
#include "dbat/class.h"
#include "dbat/random.h"
#include "dbat/relocate.h"
#include "dbat/search.h"

/* local functions */
static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
static int is_tell_ok(struct char_data *ch, struct char_data *vict);
static void handle_whisper(char *buf, struct char_data *ch, struct char_data *vict);
static char *overhear(char *buf, int type);
static void list_languages(struct char_data *ch);
static void garble_text(char *string, int known, int lang);



static const char *languages[] =
{
  "common",
  "elven",
  "gnomish",
  "dwarven",
  "halfling",
  "orc",
  "druid",
  "draconic",
  "\n"
};

static void list_languages(struct char_data *ch)
{
  int a = 0, i;

  send_to_char(ch, "Languages:\r\n[");
  for (i = MIN_LANGUAGES ; i <= MAX_LANGUAGES ; i++)
    if (char_get_skill(ch, i))
      send_to_char(ch, "%s %s%s%s",
        a++ != 0 ? "," : "",
        SPEAKING(ch) == i ? "@r": "@n",
        languages[i-MIN_LANGUAGES], "@n");
  send_to_char(ch, "%s ]\r\n", a== 0 ? " None!" : "");
}

ACMD(do_voice)
{
 skip_spaces(&argument);

 if (char_is_npc(ch))
  return;


 if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
  send_to_char(ch, "You're mute. You don't need to describe your voice.\r\n");
  return;
 }

 if (!*argument) {
  send_to_char(ch, "What are you changing your voice description to?\r\n");
  return;
 }
 else if (strlen(argument) > 75) {
  send_to_char(ch, "Your voice description can not be longer than 75 characters.\r\n");
  return;
 }
 else if (strstr(argument, "@")) {
  send_to_char(ch, "You can not use colorcode in voice descriptions.\r\n");
  return;
 }
 else if (char_get_voice(ch) != NULL && char_get_rp(ch) < 1) {
  send_to_char(ch, "Your voice has already been set. You will need at least 1 RPP to be able to change it.\r\n");
  return;
 }
 else if (char_get_voice(ch) != NULL) {
  send_to_char(ch, "Your voice has now been set to: %s\r\n", argument);
  if (char_get_voice(ch)) {
   free(char_get_voice(ch));
  }
  char_set_voice(ch, strdup(argument));
  char_set_rp(ch, char_get_rp(ch) - (1));
  ch->desc->rpp = char_get_rp(ch);
  userWrite(ch->desc, 0, 0, 0, "index");
  send_to_char(ch, "@D(@cRPP@W: @w-1@D)@n\n\n");
  return;
 }
 else {
  send_to_char(ch, "Your voice has now been set to: %s\r\n", argument);
  if (char_get_voice(ch)) {
   free(char_get_voice(ch));
  }
  char_set_voice(ch, strdup(argument));
  return;
 }

}

ACMD(do_languages)
{
  int i, found = FALSE;
  char arg[MAX_STRING_LENGTH];

  if (CONFIG_ENABLE_LANGUAGES) {
  one_argument(argument, arg);
  if (!*arg)
    list_languages(ch);
  else {
    for (i = MIN_LANGUAGES; i <= MAX_LANGUAGES; i++) {
      if ((search_block(arg, languages, FALSE) == i-MIN_LANGUAGES) && char_get_skill(ch, i)) {
        SPEAKING(ch) = i;
        send_to_char(ch, "You now speak %s.\r\n", languages[i-MIN_LANGUAGES]);
        found = TRUE;
        break;
      }
    }
    if (!found) {
      send_to_char(ch, "You do not know of any such language.\r\n");
      return;
    }
  }
  } else {
    send_to_char(ch, "But everyone already understands everyone else!\r\n");
    return;
  }

}

static void garble_text(char *string, int known, int lang)
{
  char letters[50] = "";
  int i;

  switch (lang) {
  case SKILL_LANG_DWARVEN:
    strcpy (letters, "hprstwxyz");
    break;
  case SKILL_LANG_ELVEN:
    strcpy (letters, "aefhilnopstu");
    break;
  default:
    strcpy (letters, "aehiopstuwxyz");
    break;
  }

  for (i = 0; i < (int) strlen(string); ++i) {
    if (isalpha(string[i]) && (!known)) {
      string[i] = letters[rand_number(0, (int) strlen(letters) - 1)];
    }
  }
}

ACMD(do_osay) {
 
 skip_spaces(&argument);

 if (char_is_npc(ch))
  return;

 if(!*argument) {
   send_to_char(ch, "Yes, but WHAT do you want to osay?\r\n");
   return;
 } else {
   char buf[MAX_INPUT_LENGTH];
   char buf2[MAX_INPUT_LENGTH];

   sprintf(buf, "@WYou @D[@mOSAY@D] @W'@w%s@W'@n", argument);
   if (!char_prf_flagged(ch, PRF_HIDE)) {
    sprintf(buf2, "@W%s @D[@mOSAY@D] @W'@w%s@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : ch->desc->user, argument);
   }
   if (char_prf_flagged(ch, PRF_HIDE)) {
    sprintf(buf2, "@WAnonymous @D[@mOSAY@D] @W'@w%s@W'@n", argument);
   }
   act(buf, FALSE, ch, 0, 0, TO_CHAR);
   act(buf2, FALSE, ch, 0, 0, TO_ROOM);
 }
}

ACMD(do_say)
{
  struct descriptor_data *d;
  struct char_data *wch = NULL, *wch2 = NULL, *wch3 = NULL, *tch = NULL, *sch = NULL;
  struct obj_data *obj = NULL;
  int granted = FALSE, found = FALSE;
  char buf2[MAX_INPUT_LENGTH];
  *buf2 = '\0';

  skip_spaces(&argument);

  if (char_get_bonuses(ch, BONUS_MUTE) > 0 && rroom_get_vnum(char_get_in_room(ch)) > 160) {
   send_to_char(ch, "You are mute and unable to talk though.\r\n");
   return;
  } else if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
   send_to_char(ch, "You are mute and unable to talk though. You will be allowed to just for MUD School.");
  }

  if(!*argument) {
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
    return;
  } else {
    char buf[MAX_INPUT_LENGTH + 70];
    char verb[10];

    if (argument[strlen(argument)-1] == '!') {
      strcpy(verb, "exclaim");
    } else if(argument[strlen(argument)-1] == '?') {
      strcpy(verb, "ask");
    } else {
      strcpy(verb, "say");
    }

    for (tch = world[char_get_in_room(ch)].people; tch; tch = tch->next_in_room) {
      if (tch != ch && tch->desc) {
       char sayto[100];
       sprintf(sayto, "to %s ", char_get_name(tch));
       if (strstr(argument, sayto)) {
          char saytoo[200];
          *verb = '\0';
          sprintf(saytoo, "says to @g%s@W", char_get_name(tch));
          search_replace(argument, sayto, "");
          strcpy(verb, saytoo);
          sch = tch;
       }
       else if (!char_is_npc(tch) && !char_is_npc(ch)) {
        if (readIntro(ch, tch) == 1) {
         sprintf(sayto, "to %s ", get_i_name(ch, tch));
        }
        if (strstr(argument, sayto)) {
          char saytoo[200];
          *verb = '\0';
          sprintf(saytoo, "says to @g%s@W", char_get_name(tch));
          search_replace(argument, sayto, "");
          strcpy(verb, saytoo);
          sch = tch;
        }
       }
      }
     }
      if (!sch) {
       snprintf(buf, sizeof(buf), "@w$n @W%ss, '@C%s@W'@n", verb, argument);
       act(buf, TRUE, ch, 0, 0, TO_ROOM);
      }
      else {
       snprintf(buf, sizeof(buf), "@w$n @Wsays to @g$N@W, '@C%s@W'@n", argument);
       snprintf(buf2, sizeof(buf2), "@w$n @Wsays to @gyou@W, '@C%s@W'@n", argument);
       act(buf2, TRUE, ch, 0, sch, TO_VICT);
       act(buf, TRUE, ch, 0, sch, TO_NOTVICT);
      }
      if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NOREPEAT)) {
        send_to_char(ch, "%s", CONFIG_OK);
      } else {
        if (strstr(verb, "says to")) {
          char saytoo[200];
          *verb = '\0';
          sprintf(saytoo, "say to @g%s@W", char_get_name(sch));
          strcpy(verb, saytoo);
        }
        snprintf(buf, sizeof(buf), "@WYou %s, '@C%s@W'@n\r\n", verb, argument);
        send_to_char(ch, "%s", buf);
        add_history(ch, buf, HIST_SAY);
       if (SHENRON == TRUE) {
       if (rroom_get_vnum(char_get_in_room(ch)) == DRAGONR && rroom_get_vnum(char_get_in_room(EDRAGON)) == DRAGONR) {
        if (strstr(argument, "wish")) {

          for (d = descriptor_list; d; d = d->next) {
           if (desc_get_connected(d) != CON_PLAYING)
            continue;

           if (strstr(argument, char_get_name(d->character)) && wch == NULL) {
             wch = d->character;
             found = TRUE;
           }
           else if (strstr(argument, char_get_name(d->character)) && wch2 == NULL) {
             wch2 = d->character;
           }
           else if (strstr(argument, char_get_name(d->character)) && wch3 == NULL) {
             wch3 = d->character;
           }
          } /* end repeat for */

         if (wch == NULL && strstr(argument, "myself")) {
          wch = ch;
         }
         if (wch == NULL) {
          return;
         }

         if (granted == FALSE && strstr(argument, "knowledge")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now has more knowledge!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           GET_PRACTICES(wch, char_get_class(wch)) += rand_number(2000, 5000);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a knowledge wish on %s.", char_get_name(ch), char_get_name(wch));;
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end knowledge wish if */

         if (granted == FALSE && strstr(argument, "speed")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now faster!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           wch->real_abils.cha += 10;
           if (wch->real_abils.cha > 100) {
            wch->real_abils.cha = 100;
           }
           save_char(wch);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a speed wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end speed wish if */

         if (granted == FALSE && strstr(argument, "tough")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now tougher!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           char_set_armor(wch, char_get_armor(wch) + (5000));
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a tough wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end tough wish if */

         if (granted == FALSE && strstr(argument, "strength")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s has more strength!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           wch->real_abils.str += 10;
           if (wch->real_abils.str > 100) {
            wch->real_abils.str = 100;
           }
           save_char(wch);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a strength wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end strength wish if */

         if (granted == FALSE && strstr(argument, "intelligence")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now smarter!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           wch->real_abils.intel += 10;
           if (wch->real_abils.intel > 100) {
            wch->real_abils.intel = 100;
           }
           save_char(wch);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a intelligence wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end intelligence wish if */

         if (granted == FALSE && strstr(argument, "wisdom")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now wiser!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           wch->real_abils.wis += 10;
           if (wch->real_abils.wis > 100) {
            wch->real_abils.wis = 100;
           }
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a wisdom wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end wisdom wish if */

         if (granted == FALSE && strstr(argument, "agility")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now more agile!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
          wch->real_abils.dex += 10;
          if (wch->real_abils.dex > 100) {
           wch->real_abils.dex = 100;
          }
           save_char(wch);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a agility wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end agility wish if */

         if (granted == FALSE && strstr(argument, "constitution")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s has more guts!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
          wch->real_abils.con += 10;
          if (wch->real_abils.con > 100) {
           wch->real_abils.con = 100;
          }
           save_char(wch);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a constitutionwish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end constitution wish if */

         if (granted == FALSE && strstr(argument, "skill")) {
          if (wch != NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s has more skill!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
          int roll = rand_number(1, 3);
          send_to_char(wch, "@GYou suddenly feel like you could learn %d more skills!@n\r\n", roll);
          char_set_skill_slots(wch, char_get_skill_slots(wch) + (roll));
           save_char(wch);
           granted = TRUE;
           SELFISHMETER += 1;
		 mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a skill wish on %s.", char_get_name(ch), char_get_name(wch));
		 WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end skill wish if */
/* Rillao: transloc, add new transes here */
         if (granted == FALSE && strstr(argument, "power")) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish cannot be granted, You might want to try something else instead, mortal!@w'@n\r\n");
          /* send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now has grown stronger!%s@w'@n\r\n", GET_NAME(wch), WISH[0] ? "" : " Now make your second wish.");
           double mult = 1;
           if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
            mult = 3;
           }
           else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
            mult = 4;
           }
           else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
            mult = 5;
           }
           else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
            mult = 2;
           }
           else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
            mult = 3;
           }
           else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
            mult = 3.5;
           }
           else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS4)) {
            mult = 4;
           }
           else if (IS_MAJIN(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
            mult = 2;
           }
           else if (IS_MAJIN(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
            mult = 3;
           }
           else if (IS_MAJIN(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
            mult = 4.5;
           }
           GET_MAX_HIT(wch) += (GET_BASE_PL(wch) * .15) * mult;
           GET_MAX_MANA(wch) += (GET_BASE_KI(wch) * .15) * mult;
           GET_MAX_MOVE(wch) += (GET_BASE_ST(wch) * .15) * mult;
           GET_BASE_PL(wch) += GET_BASE_PL(wch) * .15;
           GET_BASE_KI(wch) += GET_BASE_KI(wch) * .15;
           GET_BASE_ST(wch) += GET_BASE_ST(wch) * .15;
           granted = TRUE;
           SELFISHMETER += 2;
		  mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a power wish on %s.", GET_NAME(ch), GET_NAME(wch)); */
		  WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end power wish if */

         if (granted == FALSE && strstr(argument, "money")) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now has become richer!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           char_set_bank_gold(wch, char_get_bank_gold(wch) + (1000000));
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a money wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end money wish if */

         if (granted == FALSE && strstr(argument, "immunity")) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now has immunity to Burn, Freezing, Mind Break, Poison, Blindness, Yoikominminken, and Paralysis!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           SET_BIT_AR(char_get_affected_by(wch), AFF_IMMUNITY);
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a immunity wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end money wish if */

         if (granted == FALSE && strstr(argument, "vitality")) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish cannot be granted, You might want to try something else instead, mortal!%s@w'@n\r\n");
           /*send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now will never hunger or thirst again!%s@w'@n\r\n", GET_NAME(wch), WISH[0] ? "" : " Now make your second wish.");
           GET_COND(ch, HUNGER) = -1;
           GET_COND(ch, THIRST) = -1;
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a vitality wish on %s.", GET_NAME(ch), GET_NAME(wch));*/
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end vitality if */

         if (granted == FALSE && strstr(argument, "revive")) {
          int count = 0;
          if (wch != NULL) {
           count += 1;
          }
          if (wch2 != NULL) {
           count += 1;
          }
          if (wch3 != NULL) {
           count += 1;
          }
          if (count == 1) {
           if (!char_aff_flagged(wch, AFF_SPIRIT)) {
            send_to_room(real_room(DRAGONR), "@wShenron says, '@C%s is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
           }
           else {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s has returned to life!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           if (real_room(char_get_droom(wch)) == NOWHERE) {
            char_set_droom(wch, 300);
           }
           if (real_room(char_get_droom(wch)) != NOWHERE) {
            char_from_room(wch);
            if (char_get_droom(wch) > 0) {
             char_to_room(wch, real_room(char_get_droom(wch)));
            } else {
             char_to_room(wch, real_room(300));
            }
            look_at_room(char_get_in_room(wch), wch, 0);
            send_to_char(wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
            REMOVE_BIT_AR(char_get_affected_by(wch), AFF_SPIRIT);
            REMOVE_BIT_AR(char_get_affected_by(wch), AFF_ETHEREAL);
           }
           granted = TRUE;
           SELFISHMETER -= 2;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a revive wish on %s.", char_get_name(ch), char_get_name(wch));
           }
          } /* is there a target for the wish? */
          if (count == 2) {
           if (!char_aff_flagged(wch, AFF_SPIRIT)) {
            send_to_room(real_room(DRAGONR), "@wShenron says, '@C%s is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
           }
           if (!char_aff_flagged(wch2, AFF_SPIRIT)) {
            send_to_room(real_room(DRAGONR), "@wShenron says, '@C%s is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch2));
           }
           else if (char_aff_flagged(wch, AFF_SPIRIT) && char_aff_flagged(wch2, AFF_SPIRIT)) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s and %s have returned to life!%s@w'@n\r\n", char_get_name(wch), char_get_name(wch2), WISH[0] ? "" : " Now make your second wish.");
           if (real_room(char_get_droom(wch)) == NOWHERE) {
            char_set_droom(wch, 300);
           }
           if (real_room(char_get_droom(wch)) != NOWHERE) {
            char_from_room(wch);
            char_to_room(wch, real_room(char_get_droom(wch)));
            look_at_room(char_get_in_room(wch), wch, 0);
            send_to_char(wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
            REMOVE_BIT_AR(char_get_affected_by(wch), AFF_SPIRIT);
            REMOVE_BIT_AR(char_get_affected_by(wch), AFF_ETHEREAL);
           }
           if (real_room(char_get_droom(wch2)) == NOWHERE) {
            char_set_droom(wch2, 300);
           }
           if (real_room(char_get_droom(wch2)) != NOWHERE) {
            char_from_room(wch2);
            char_to_room(wch2, real_room(char_get_droom(wch2)));
            look_at_room(char_get_in_room(wch2), wch2, 0);
            send_to_char(wch2, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
            REMOVE_BIT_AR(char_get_affected_by(wch2), AFF_SPIRIT);
            REMOVE_BIT_AR(char_get_affected_by(wch2), AFF_ETHEREAL);
           }
           granted = TRUE;
           SELFISHMETER -= 3;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a revive wish on %s.", char_get_name(ch), char_get_name(wch2));
		   WAIT_STATE(ch, PULSE_4SEC);
           }
          } /* is there two targets for the wish? */
          if (count == 3) {
           if (!char_aff_flagged(wch, AFF_SPIRIT)) {
            send_to_room(real_room(DRAGONR), "@wShenron says, '@C%s is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
           }
           if (!char_aff_flagged(wch2, AFF_SPIRIT)) {
            send_to_room(real_room(DRAGONR), "@wShenron says, '@C%s is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch2));
           }
           if (!char_aff_flagged(wch3, AFF_SPIRIT)) {
            send_to_room(real_room(DRAGONR), "@wShenron says, '@C%s is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch3));
           }
           else if (char_aff_flagged(wch, AFF_SPIRIT) && char_aff_flagged(wch2, AFF_SPIRIT) && char_aff_flagged(wch3, AFF_SPIRIT)) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s, %s, and %s have returned to life!!%s@w'@n\r\n", char_get_name(wch), char_get_name(wch2), char_get_name(wch3), WISH[0] ? "" : " Now make your second wish.");
           if (real_room(char_get_droom(wch)) == NOWHERE) {
            char_set_droom(wch, 300);
           }
           if (real_room(char_get_droom(wch)) != NOWHERE) {
            char_from_room(wch);
            char_to_room(wch, real_room(char_get_droom(wch)));
            look_at_room(char_get_in_room(wch), wch, 0);
            send_to_char(wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
            REMOVE_BIT_AR(char_get_affected_by(wch), AFF_SPIRIT);
            REMOVE_BIT_AR(char_get_affected_by(wch), AFF_ETHEREAL);
           }
           if (real_room(char_get_droom(wch2)) == NOWHERE) {
            char_set_droom(wch2, 300);
           }
           if (real_room(char_get_droom(wch2)) != NOWHERE) {
            char_from_room(wch2);
            char_to_room(wch2, real_room(char_get_droom(wch2)));
            look_at_room(char_get_in_room(wch2), wch2, 0);
            send_to_char(wch2, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
            REMOVE_BIT_AR(char_get_affected_by(wch2), AFF_SPIRIT);
            REMOVE_BIT_AR(char_get_affected_by(wch2), AFF_ETHEREAL);
           }
           if (real_room(char_get_droom(wch3)) == NOWHERE) {
            char_set_droom(wch3, 300);
           }
           if (real_room(char_get_droom(wch3)) != NOWHERE) {
            char_from_room(wch3);
            char_to_room(wch3, real_room(char_get_droom(wch3)));
            look_at_room(char_get_in_room(wch3), wch3, 0);
            send_to_char(wch3, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
            REMOVE_BIT_AR(char_get_affected_by(wch3), AFF_SPIRIT);
            REMOVE_BIT_AR(char_get_affected_by(wch3), AFF_ETHEREAL);
           }
           granted = TRUE;
           SELFISHMETER -= 3;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a revive wish on %s and %s.", char_get_name(ch), char_get_name(wch2), char_get_name(wch3));
		   WAIT_STATE(ch, PULSE_4SEC);
           }
          } /* is there three targets for the wish? */
         } /* end revival if */

         if (granted == FALSE && strstr(argument, "immortal") && WISH[0] == 0) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now immortal!@w'@n\r\n" , char_get_name(wch));
           SET_BIT_AR(char_get_act(wch), PLR_IMMORTAL);
           WISH[0] = 1;
           WISH[1] = 1;
           granted = TRUE;
           SELFISHMETER += 4;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a immortal wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end immortal wish if */

         if (granted == FALSE && strstr(argument, "immortal") && WISH[0] == 1) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CI can not grant that wish, there is not enough remaining power in this summoning!@w'@n\r\n");
          } /* is there a target for the wish? */
         }

         if (granted == FALSE && strstr(argument, " mortal")) {
          if (wch != NULL) {
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s is now mortal!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           REMOVE_BIT_AR(char_get_act(wch), PLR_IMMORTAL);
           granted = TRUE;
           SELFISHMETER += 4;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a mortal wish on %s.", char_get_name(ch), char_get_name(wch));
		   WAIT_STATE(ch, PULSE_4SEC);
          } /* is there a target for the wish? */
         } /* end mortal wish if */

         if (granted == FALSE && strstr(argument, "senzu")) {
          if (wch != NULL) {
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           obj = read_object(1, VIRTUAL);
           obj_to_char(obj, ch);
           send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now possesses 10 senzus!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a senzu wish.", char_get_name(ch));
          } /* is there a target for the wish? */
         } /* end senzu wish if */
		 
		 if (granted == FALSE && strstr(argument, "roleplay")) {
		  if(wch != NULL) {
			send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s!%s@w'@n\r\n", char_get_name(wch), WISH[0] ? "" : " Now make your second wish.");
			granted = TRUE;
			mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a roleplay wish.", char_get_name(ch));
			WAIT_STATE(ch, PULSE_4SEC);
		 }
		}
		
         if (granted == TRUE) {
           if (WISH[0] == 1) {
            WISH[1] = 1;
           }
           else {
            WISH[0] = 1;
           } /*end WISH if */
           if (wch != NULL) {
			save_char(wch);
           }
           if (wch2 != NULL) {
            save_char(wch);			
           }
           if (wch3 != NULL) {
			save_char(wch);
           }
           save_mud_time(&time_info);
          }
          else if (wch == NULL) {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CThat person does not exist, make another wish.'@n\r\n");
          }
          else {
          send_to_room(real_room(DRAGONR), "@wShenron says, '@CDo not waste my time with wishes I can not grant...@w'@n\r\n");
          }
         } 
        } /* end DRAGONR if */
       } /* end SHENRON if */

      }
    }

    /* trigger check */
    speech_mtrigger(ch, argument);
    speech_wtrigger(ch, argument);
    if (SHENRON == FALSE || (SHENRON == TRUE && char_get_in_room(ch) != real_room(DRAGONR))) {
     mob_talk(ch, argument);
    }
}

ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;
  char blah[MAX_INPUT_LENGTH];

  skip_spaces(&argument);

  if (!char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not the member of a group!\r\n");
    return;
  }
  if (char_in_arena(ch)) {
    send_to_char(ch, "Lol, no.\r\n");
    return;
  }
  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    if (ch->master)
      k = ch->master;
    else
      k = ch;

  strcpy(buf, argument);

  sprintf(blah, "$n@W tells the group @W'@G%s@W'@n\r\n", buf);

  if (char_aff_flagged(k, AFF_GROUP) && (k != ch) && char_is_awake(k)) {
    if (CONFIG_ENABLE_LANGUAGES) {
      send_to_char(k, "%s@W tells the group%s @W'@G%s@W'@n\r\n", char_can_see(k, ch) ? char_get_name(ch) : "Someone", char_get_skill(k, SPEAKING(ch)) ? "," : ", in an unfamiliar tongue,", buf);
    } else {
      act(blah, TRUE, ch, 0, k, TO_VICT);
    }
  }
    for (f = k->followers; f; f = f->next)
      if (char_aff_flagged(f->follower, AFF_GROUP) && (f->follower != ch) && char_is_awake(f->follower)) {
        if (!char_is_npc(ch) && !char_is_npc(f->follower) && CONFIG_ENABLE_LANGUAGES) {
          garble_text(buf, char_get_skill(f->follower, SPEAKING(ch)), SPEAKING(ch));
        } else {
          garble_text(buf, 1, MIN_LANGUAGES);
        }
        if (CONFIG_ENABLE_LANGUAGES) {
          send_to_char(f->follower, "%s@W tells the group%s @W'%s@W'@n\r\n", char_can_see(f->follower, ch) ? char_get_name(ch) : "Someone", char_get_skill(f->follower, SPEAKING(ch)) ? "," : ", in an unfamiliar tongue,", buf);
        } else {
          act(blah, TRUE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
        }
      }

    if (char_prf_flagged(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "@WYou tell the group, '@G%s@W'@n\r\n", argument);
  }
}

static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  strcpy(buf, arg);

  if (CONFIG_ENABLE_LANGUAGES) {
    snprintf(buf2, sizeof(buf2), "@[13]%s tells you%s '%s@[13]'@n\r\n", char_can_see(vict, ch) ? char_get_name(ch) : "Someone", char_get_skill(vict, SPEAKING(ch)) ? "," : ", in an unfamiliar tongue,", buf);
    send_to_char(vict, "%s", buf2);
    add_history(vict, buf2, HIST_TELL);
  } else if (!char_is_npc(ch) && char_get_admlevel(vict) < 1) {
    snprintf(buf2, sizeof(buf2),  "@Y%s@Y tells you '%s'@n\r\n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : ch->desc->user, buf);
    send_to_char(vict, "%s", buf2);
    add_history(vict, buf2, HIST_TELL);
  } else if (!char_is_npc(ch) && char_get_admlevel(vict) >= 1) {
    snprintf(buf2, sizeof(buf2),  "@Y%s(%s)@Y tells you '%s'@n\r\n", ch->desc->user, char_get_name(ch), buf);
    send_to_char(vict, "%s", buf2);
    add_history(vict, buf2, HIST_TELL);
  } else if (char_is_npc(ch)) {
    snprintf(buf2, sizeof(buf2),  "@Y%s@Y tells you '%s'@n\r\n", char_get_name(ch), buf);
    send_to_char(vict, "%s", buf2);
  }

  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NOREPEAT)) {
    send_to_char(ch, "%s", CONFIG_OK);
  } else {
    if (!char_is_npc(ch)) {
    snprintf(buf2, sizeof(buf2),  "@YYou tell %s, '%s'@n\r\n", char_get_admlevel(vict) > 0 ? char_get_name(vict) : (vict->desc->user ? vict->desc->user : "ERROR"), arg);
    if (char_get_admlevel(ch) < 5 && char_get_admlevel(vict) < 5 && !char_is_npc(ch) && !char_is_npc(vict)) {
     send_to_imm("@GTELL: @C%s@G tells @c%s, @W'@w%s@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch),  char_get_admlevel(vict) > 0 ? char_get_name(vict) : char_get_user(vict), arg);
    }
    send_to_char(ch, "%s", buf2);
    add_history(ch, buf2, HIST_TELL);
    }
    else {
     send_to_char(ch, "%s", CONFIG_OK);
    }
  }

  if (!char_is_npc(vict) && !char_is_npc(ch))
    GET_LAST_TELL(vict) = char_get_idnum(ch);
}

static int is_tell_ok(struct char_data *ch, struct char_data *vict)
{

  if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (!char_is_npc(ch) && char_get_level(ch) < 3 && char_get_admlevel(vict) < 1)
    send_to_char(ch, "You need to be level 3 or higher to send or receive tells");
  else if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_AFK) && char_get_admlevel(ch) < 1)
    send_to_char(ch, "You can't send tells when AFK.\r\n");
  else if (!char_is_npc(ch) && char_prf_flagged(vict, PRF_AFK) && char_get_admlevel(ch) < 1)
    send_to_char(ch, "They are AFK right now, try later.\r\n");
  else if (!char_is_npc(ch) && char_prf_flagged(vict, PRF_AFK) && char_get_admlevel(ch) >= 1)
    return (TRUE);
  else if (!char_is_npc(vict) && char_get_level(vict) < 3 && char_get_admlevel(ch) < 1)
    send_to_char(ch, "They need to be level 3 or higher to send or receive tells");
  else if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NOTELL) && char_get_admlevel(vict) < 1)
    send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
  else if (rroom_flagged(char_get_in_room(ch), ROOM_SOUNDPROOF) && char_get_admlevel(vict) < 1)
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (char_is_npc(vict))
    send_to_char(ch, "You can't send tells to mobs.\r\n");
  else if (!char_is_npc(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (char_plr_flagged(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!char_is_npc(vict)  && char_get_admlevel(ch) < 1 && char_prf_flagged(vict, PRF_NOTELL)) || rroom_flagged(char_get_in_room(vict), ROOM_SOUNDPROOF))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char(ch, "Who do you wish to tell what??\r\n");
    return;
  }
  struct descriptor_data *k;
  int found = FALSE;
  if (!char_is_npc(ch)) {
  sprintf(buf, "%s", CAP(buf));
  for (k = descriptor_list; k; k = k->next) {
   if (char_is_npc(k->character))
    continue;
   if (desc_get_connected(k) != CON_PLAYING)
    continue;
   if (k->user == NULL)
    continue;
   if (found == FALSE && !char_is_npc(ch) && (!strcasecmp(k->user, buf) || strstr(k->user, buf))) {
    vict = k->character;
    found = TRUE;
   }
   else if (!char_is_npc(ch) && found == FALSE && (!strcasecmp(char_get_name(k->character), buf) || strstr(char_get_name(k->character), buf)) && char_get_admlevel(k->character) > 0) {
    vict = k->character;
    found = TRUE;
   }
   }
  }
  if (found == FALSE && !char_is_npc(ch))
     send_to_char(ch, "No user around with that name.");
  else if (char_is_npc(ch) && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (is_tell_ok(ch, vict))
    perform_tell(ch, vict, buf2);

}

ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (char_is_npc(ch))
    return;

  if (rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
    send_to_char(ch, "This is a different dimension!\r\n");
    return;
   }
  if (rroom_flagged(char_get_in_room(ch), ROOM_PAST)) {
    send_to_char(ch, "This is the past, you can't send tells!\r\n");
    return;
   }

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    /*
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (char_is_npc(tch) || char_get_idnum(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char(ch, "They are no longer playing.\r\n");
    else if (is_tell_ok(ch, tch))
      perform_tell(ch, tch, argument);
  }
}

ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
   send_to_char(ch, "You are mute and unable to talk though.\r\n");
   return;
  }

  switch (subcmd) {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    char buf1[MAX_STRING_LENGTH];
    char obuf[MAX_STRING_LENGTH];

    if (CONFIG_ENABLE_LANGUAGES) {
      strcpy(obuf, buf2);
      garble_text(obuf, char_get_skill(vict, SPEAKING(ch)), SPEAKING(ch));
      snprintf(buf1, sizeof(buf1), "$n %s you%s '%s'", action_plur, char_get_skill(vict, SPEAKING(ch)) ? "," : ", in an unfamiliar tongue,", obuf);
    } else {
      snprintf(buf1, sizeof(buf1), "@c$n @W%s you '@m%s@W'@n", action_plur, buf2);
    }

    act(buf1, FALSE, ch, 0, vict, TO_VICT);

    if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NOREPEAT)) {
      send_to_char(ch, "%s", CONFIG_OK);
    } else {
      char blum[MAX_INPUT_LENGTH];
      sprintf(blum, "@WYou %s @C$N@W, '@m%s@W'@n\r\n", action_sing, buf2);
      act(blum, TRUE, ch, 0, vict, TO_CHAR);
    }

    if (subcmd == SCMD_WHISPER) {
     act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
     handle_whisper(buf2, ch, vict);
    } else {
     act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
    }
  }
}

static void handle_whisper(char *buf, struct char_data *ch, struct char_data *vict)
{
 struct char_data *tch;

 for (tch = world[char_get_in_room(ch)].people; tch; tch = tch->next_in_room) {
  if (char_is_npc(tch)) {
   continue;
  }
  if (tch == ch) {
   continue;
  }
  if (tch == vict) {
   continue;
  }
  if (!char_get_skill(tch, SKILL_LISTEN)) {
   continue;
  }
  if (char_get_skill(tch, SKILL_LISTEN)) {
   int skill = char_get_skill(tch, SKILL_LISTEN);
   int roll1 = rand_number(10, 30);
   int roll = rand_number(roll1, 110);

   if (skill >= roll) {
    send_to_char(tch, "@WYou overhear everything whispered, @W'@m%s@W'@n\r\n", overhear(buf, 3));
   } else if (skill + 10 >= roll) {
    send_to_char(tch, "@WYou overhear a lot of what is whispered, @W'@m%s@W'@n\r\n", overhear(buf, 2));
   } else if (skill + 20 >= roll) {
    send_to_char(tch, "@WYou overhear some of what is whispered, @W'@m%s@W'@n\r\n", overhear(buf, 1));
   } else if (skill + 30 >= roll) {
    send_to_char(tch, "@WYou overhear little of what is whispered, @W'@m%s@W'@n\r\n", overhear(buf, 0));
   } else {
    send_to_char(tch, "@WYou were unable to overhear anything that was whispered.@n\r\n");
   }
  }
 }

}

static char *overhear(char *buf, int type)
{

 switch (type) {
  case 0:
   if (rand_number(1, 10) >= 5) {
    search_replace(buf, "a", ".."); search_replace(buf, "A", "..");
    search_replace(buf, "h", ".."); search_replace(buf, "H", "..");
    search_replace(buf, "e", ".."); search_replace(buf, "E", "..");
    search_replace(buf, "m", ".."); search_replace(buf, "M", "..");
    search_replace(buf, "o", ".."); search_replace(buf, "O", "..");
    search_replace(buf, "p", ".."); search_replace(buf, "P", "..");
    search_replace(buf, "y", ".."); search_replace(buf, "Y", "..");
    search_replace(buf, "j", ".."); search_replace(buf, "J", "..");
    search_replace(buf, "k", ".."); search_replace(buf, "K", "..");
    search_replace(buf, "d", ".."); search_replace(buf, "D", "..");
    search_replace(buf, "w", ".."); search_replace(buf, "W", "..");
   } else if (rand_number(1, 10) >= 5) {
    search_replace(buf, "e", ".."); search_replace(buf, "E", "..");
    search_replace(buf, "r", ".."); search_replace(buf, "R", "..");
    search_replace(buf, "k", ".."); search_replace(buf, "K", "..");
    search_replace(buf, "m", ".."); search_replace(buf, "M", "..");
    search_replace(buf, "o", ".."); search_replace(buf, "O", "..");
    search_replace(buf, "p", ".."); search_replace(buf, "P", "..");
    search_replace(buf, "y", ".."); search_replace(buf, "Y", "..");
    search_replace(buf, "j", ".."); search_replace(buf, "J", "..");
    search_replace(buf, "k", ".."); search_replace(buf, "K", "..");
    search_replace(buf, "d", ".."); search_replace(buf, "D", "..");
    search_replace(buf, "w", ".."); search_replace(buf, "W", "..");
   } else {
    search_replace(buf, "s", ".."); search_replace(buf, "S", "..");
    search_replace(buf, "r", ".."); search_replace(buf, "R", "..");
    search_replace(buf, "c", ".."); search_replace(buf, "C", "..");
    search_replace(buf, "q", ".."); search_replace(buf, "Q", "..");
    search_replace(buf, "l", ".."); search_replace(buf, "L", "..");
    search_replace(buf, "u", ".."); search_replace(buf, "U", "..");
    search_replace(buf, "i", ".."); search_replace(buf, "I", "..");
    search_replace(buf, "z", ".."); search_replace(buf, "Z", "..");
    search_replace(buf, "t", ".."); search_replace(buf, "T", "..");
   }
   return buf;
   break;
  case 1:
   if (rand_number(1, 10) >= 5) {
    search_replace(buf, "b", ".."); search_replace(buf, "B", "..");
    search_replace(buf, "f", ".."); search_replace(buf, "F", "..");
    search_replace(buf, "g", ".."); search_replace(buf, "G", "..");
    search_replace(buf, "v", ".."); search_replace(buf, "V", "..");
    search_replace(buf, "j", ".."); search_replace(buf, "J", "..");
    search_replace(buf, "k", ".."); search_replace(buf, "K", "..");
   } else if (rand_number(1, 10) >= 5) {
    search_replace(buf, "d", ".."); search_replace(buf, "D", "..");
    search_replace(buf, "y", ".."); search_replace(buf, "Y", "..");
    search_replace(buf, "m", ".."); search_replace(buf, "M", "..");
    search_replace(buf, "h", ".."); search_replace(buf, "H", "..");
    search_replace(buf, "s", ".."); search_replace(buf, "S", "..");
    search_replace(buf, "t", ".."); search_replace(buf, "T", "..");
   } else {
    search_replace(buf, "a", ".."); search_replace(buf, "A", "..");
    search_replace(buf, "r", ".."); search_replace(buf, "R", "..");
    search_replace(buf, "n", ".."); search_replace(buf, "N", "..");
    search_replace(buf, "o", ".."); search_replace(buf, "O", "..");
   }
   return buf;
   break;
  case 2:
   if (rand_number(1, 10) >= 5) {
    search_replace(buf, "q", ".."); search_replace(buf, "Q", "..");
    search_replace(buf, "o", ".."); search_replace(buf, "O", "..");
    search_replace(buf, "i", ".."); search_replace(buf, "I", "..");
    search_replace(buf, "g", ".."); search_replace(buf, "G", "..");
   } else if (rand_number(1, 10) >= 5) {
    search_replace(buf, "a", ".."); search_replace(buf, "A", "..");
    search_replace(buf, "e", ".."); search_replace(buf, "E", "..");
    search_replace(buf, "i", ".."); search_replace(buf, "I", "..");
    search_replace(buf, "o", ".."); search_replace(buf, "O", "..");
   } else {
    search_replace(buf, "k", ".."); search_replace(buf, "K", "..");
    search_replace(buf, "m", ".."); search_replace(buf, "M", "..");
    search_replace(buf, "b", ".."); search_replace(buf, "B", "..");
   }
   return buf;
   break;
  case 3:
   return buf;
   break;
 }

 return ("Nothing");
}

/*
 * buf1, buf2 = MAX_OBJECT_NAME_LENGTH
 *	(if it existed)
 */
ACMD(do_write)
{
  extern struct index_data *obj_index;
  struct obj_data *paper, *pen = NULL, *obj;
  char *papername, *penname;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  /* before we do anything, lets see if there's a board involved. */
  for (obj = ch->carrying; obj;obj=obj->next_content) {
    if(obj_get_type_flag(obj) == ITEM_BOARD) {
      break;
    }
  }
  
  if(!obj) {
    for (obj = world[char_get_in_room(ch)].contents; obj;obj=obj->next_content) {
      if(obj_get_type_flag(obj) == ITEM_BOARD) {
	break;
      }
    }
  }
  
  if(obj) {                /* then there IS a board! */
    write_board_message(obj_get_vnum(obj),ch,argument);
    act ("$n begins to write a note on $p.", TRUE, ch, obj, 0, TO_ROOM);
    return;
  }
  
  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char(ch, "write on [what] with [what pen?]\r\n");
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (obj_get_type_flag(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = NULL;
    } else if (obj_get_type_flag(paper) != ITEM_NOTE) {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }
    /* One object was found.. now for the other one. */
    if (!char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      return;
    }
    if (!char_can_see_obj(ch, char_get_equipment(ch, WEAR_WIELD2))) {
      send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = char_get_equipment(ch, WEAR_WIELD2);
    else
      pen = char_get_equipment(ch, WEAR_WIELD2);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (obj_get_type_flag(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (obj_get_type_flag(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else {
    char *backstr = NULL;
 
    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      backstr = strdup(paper->action_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, "%s", paper->action_description);
    }
 
    /* we can write - hooray! */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    SET_BIT_AR(obj_get_extra_flags(paper),ITEM_UNIQUE_SAVE);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, buf2);

  if (char_is_npc(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!strcasecmp(arg, "all")) {
      if (char_adm_flagged(ch, ADM_TELLALL)) {
	for (d = descriptor_list; d; d = d->next)
	  if (desc_get_connected(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (char_prf_flagged(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", CONFIG_OK);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], *msg;
  
  *buf1 = '\0';
  *buf2 = '\0';

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    PRF_NOMUSIC,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    0
  };
  int hist_type[] = {
    HIST_HOLLER,
    HIST_SHOUT,
    HIST_GOSSIP,
    HIST_AUCTION,
    HIST_GRATS,
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = {
    {"You cannot music!!\r\n",
      "@D[@mMUSIC@D]",
      "You aren't even on the channel!\r\n",
      "@[10]"},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
      "@[9]"},

    {"You cannot ooc!!\r\n",
      "@D[@BOOC@D]",
      "You aren't even on the channel!\r\n",
      "@[10]"},

    {"You cannot newbie!!\r\n",
      "newbie",
      "You aren't even on the channel!\r\n",
      "@[11]"},

    {"You cannot congratulate!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n",
      "@[12]"}
  };

  /* to keep pets, etc from being ordered to shout */
  if (!ch->desc)
    return;

  if (char_plr_flagged(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (rroom_flagged(char_get_in_room(ch), ROOM_SOUNDPROOF)) {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }

  if (subcmd == SCMD_SHOUT && char_get_bonuses(ch, BONUS_MUTE) > 0) {
   send_to_char(ch, "You are mute and are incapable of speech.\r\n");
   return;
  }

  /* skip leading spaces */
  skip_spaces(&argument);

  if (subcmd == SCMD_GOSSIP && (*argument == '*')) { 
    subcmd = SCMD_GEMOTE; 
  } 

  if (subcmd == SCMD_GEMOTE) { 
    ACMD(do_gmote); 
    if (*argument == '*' || *argument == ':') 
      do_gmote(ch, argument + 1, 0, 1); 
    else 
      do_gmote(ch, argument, 0, 1); 

    return; 
  } 

  /* level_can_shout defined in config.c */
  if (char_get_level(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* make sure the char is on the channel */
  if (!char_is_npc(ch) && char_prf_flagged(ch, channels[subcmd])) {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }

  /* make sure that there is something there to say! */
  if (!*argument) {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  delete_doubledollar(argument);
  /* set up the color on code */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* first, set up strings to be given to the communicator */
  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NOREPEAT)) {
    send_to_char(ch, "%s", CONFIG_OK);
  } else {
    snprintf(buf2, sizeof(buf2), "%s@WYou %s@W, '@w%s@W'%s@n\r\n", color_on, com_msgs[subcmd][1], argument, color_on);
    send_to_char(ch, "%s", buf2);
    add_history(ch, buf2, hist_type[subcmd]);
  }

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (desc_get_connected(i) == CON_PLAYING && i != ch->desc && i->character &&
       (char_is_npc(i->character) || !char_prf_flagged(i->character, channels[subcmd])) &&
       (char_is_npc(i->character) || !char_plr_flagged(i->character, PLR_WRITING)) &&
	!rroom_flagged(char_get_in_room(i->character), ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[char_get_in_room(ch)].zone != world[char_get_in_room(i->character)].zone) ||
	   !char_is_awake(i->character)))
	continue;

      if (CONFIG_ENABLE_LANGUAGES) {
        garble_text(argument, char_get_skill(i->character, SPEAKING(ch)), SPEAKING(ch));
        snprintf(buf1, sizeof(buf1), "%s%s %ss%s '%s@n'%s", color_on, char_get_admlevel(ch) > 0 ? char_get_name(ch) :  ch->desc->user, com_msgs[subcmd][1], char_get_skill(i->character, SPEAKING(ch)) ? "," : ", in an unfamiliar tongue,", argument, color_on);
      } else if (subcmd == SCMD_SHOUT && char_get_in_room(i->character) != char_get_in_room(ch)) {
        snprintf(buf1, sizeof(buf1), "%s@WSomeone nearby %ss@W, '@w%s@W'@n%s", color_on, com_msgs[subcmd][1], argument, color_on);
      } else if (subcmd == SCMD_SHOUT && char_get_in_room(i->character) == char_get_in_room(ch)) {
        snprintf(buf1, sizeof(buf1), "%s@W$n@W %ss@W, '@w%s@W'@n%s", color_on, com_msgs[subcmd][1], argument, color_on);
      } else {
        if (char_get_admlevel(ch) > 0) {
         snprintf(buf1, sizeof(buf1), "%s@W$n %ss@W, '@w%s@W'@n%s", color_on, com_msgs[subcmd][1], argument, color_on);
        }
        else if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_HIDE) && char_get_admlevel(i->character) < ADMLVL_IMMORT && ch != i->character) {
        snprintf(buf1, sizeof(buf1), "%s@WAnonymous Player %ss@W, '@w%s@W'@n%s", color_on, com_msgs[subcmd][1], argument, color_on);
        }
        else if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_HIDE) && char_get_admlevel(i->character) >= ADMLVL_IMMORT && ch != i->character) {
        snprintf(buf1, sizeof(buf1), "%s@W%s(H) %ss@W, '@w%s@W'@n%s", color_on, char_get_admlevel(i->character) > 0 ? char_get_name(ch) : ch->desc->user, com_msgs[subcmd][1], argument, color_on);
        }
        else if (char_get_admlevel(i->character) > 0) {
         snprintf(buf1, sizeof(buf1), "%s@W%s ($n) %ss@W, '@w%s@W'@n%s", color_on, ch->desc->user, com_msgs[subcmd][1], argument, color_on);
        }
        else {
        snprintf(buf1, sizeof(buf1), "%s@W%s %ss@W, '@w%s@W'@n%s", color_on, char_get_admlevel(i->character) > 0 ? char_get_name(ch) : ch->desc->user, com_msgs[subcmd][1], argument, color_on);
        }
      }

      msg = act(buf1, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
      add_history(i->character, msg, hist_type[subcmd]);
    }
  }
      if (char_get_spam(ch) >= 3 && char_get_admlevel(ch) < 1) {
       send_to_imm("SPAMMING: %s has been frozen for spamming!\r\n", char_get_name(ch));
       send_to_all("@rSPAMMING@D: @C%s@w has been frozen for spamming, let that be a lesson to 'em.@n\r\n", char_get_name(ch));
       SET_BIT_AR(char_get_act(ch), PLR_FROZEN);
       GET_FREEZE_LEV(ch) = 1;
      }
      else if (char_get_spam(ch) < 3) {
       char_set_spam(ch, char_get_spam(ch) + (1));
      }
}

ACMD(do_qcomm)
{
  if (!char_prf_flagged(ch, PRF_QUEST)) {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", toupper(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else {
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;

    if (char_prf_flagged(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else if (subcmd == SCMD_QSAY) {
      snprintf(buf, sizeof(buf), "You quest-say, '%s'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    } else
      act(argument, FALSE, ch, 0, argument, TO_CHAR);

    if (subcmd == SCMD_QSAY)
      snprintf(buf, sizeof(buf), "$n quest-says, '%s'", argument);
    else
      strlcpy(buf, argument, sizeof(buf));

    for (i = descriptor_list; i; i = i->next)
      if (desc_get_connected(i) == CON_PLAYING && i != ch->desc && char_prf_flagged(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}

ACMD(do_respond) {
  int found=0,mnum=0;
  struct obj_data *obj;
  char number[MAX_STRING_LENGTH];
  
  if(char_is_npc(ch)) {
    send_to_char(ch,"As a mob, you never bothered to learn to read or write.\r\n");
    return;
  }
  
  for (obj = ch->carrying; obj;obj=obj->next_content) {
    if(obj_get_type_flag(obj) == ITEM_BOARD) {
      found=1;
      break;
    }
  }
  if(!obj) {
    for (obj = world[char_get_in_room(ch)].contents; obj;obj=obj->next_content) {
      if(obj_get_type_flag(obj) == ITEM_BOARD) {
	found=1;
	break;
      }
    }
  }
  if (obj) {
    argument = one_argument(argument, number);
    if (!*number) {
      send_to_char(ch,"Respond to what?\r\n");
      return;
    }
    if (!isdigit(*number) || (!(mnum = atoi(number)))) {
      send_to_char(ch,"You must type the number of the message you wish to reply to.\r\n");
      return;
    }
    board_respond(obj_get_vnum(obj), ch, mnum);
  }
  
  /* No board in the room? Send generic message -spl */
  if (found == 0) {
    send_to_char(ch,"Sorry, you may only reply to messages posted on a board.\r\n");
  }
}
