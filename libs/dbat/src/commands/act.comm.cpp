/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/weather.hpp"
#include "dbat/command.hpp"
#include "dbat/act.comm.hpp"
#include "dbat/act.social.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/boards.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/class.hpp"
#include "dbat/random.hpp"
#include "dbat/relocate.hpp"
#include "dbat/search.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void perform_tell(GameWorld *g, struct char_data *ch, struct char_data *vict, const char *arg);
static bool is_tell_ok(GameWorld *g, struct char_data *ch, struct char_data *vict);
static void handle_whisper(GameWorld *g, std::string &buf, struct char_data *ch, struct char_data *vict);
static const char *overhear(GameWorld *g, std::string &buf, int type);
static void list_languages(GameWorld *g, struct char_data *ch);
static void garble_text(GameWorld *g, std::string &string, int known, int lang);

static const char *const languages[] =
    {
        "common",
        "elven",
        "gnomish",
        "dwarven",
        "halfling",
        "orc",
        "druid",
        "draconic",
        "\n"};

static void list_languages(GameWorld *g, struct char_data *ch)
{
  int a = 0, i;

  send_to_char(g, ch, "Languages:\r\n[");
  for (i = MIN_LANGUAGES; i <= MAX_LANGUAGES; i++)
    if (char_get_skill(ch, i))
      send_to_char(g, ch, "{} {}{}{}",
                   a++ != 0 ? "," : "",
                   char_get_speaking(ch) == i ? "@r" : "@n",
                   languages[i - MIN_LANGUAGES], "@n");
  send_to_char(g, ch, "{} ]\r\n", a == 0 ? " None!" : "");
}

ACMD(do_voice)
{
  std::string text(skip_spaces(argument));

  if (char_is_npc(ch))
    return;

  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You're mute. You don't need to describe your voice.\r\n");
    return;
  }

  if (text.empty()) {
    send_to_char(g, ch, "What are you changing your voice description to?\r\n");
    return;
  }
  if (text.size() > 75) {
    send_to_char(g, ch, "Your voice description can not be longer than 75 characters.\r\n");
    return;
  }
  if (text.contains("@")) {
    send_to_char(g, ch, "You can not use colorcode in voice descriptions.\r\n");
    return;
  }
  if (char_get_voice(ch) != nullptr && char_get_rp(ch) < 1) {
    send_to_char(g, ch, "Your voice has already been set. You will need at least 1 RPP to be able to change it.\r\n");
    return;
  }
  if (char_get_voice(ch) != nullptr) {
    send_to_char(g, ch, "Your voice has now been set to: {}\r\n", text);
    char_set_voice(ch, text);
    char_set_rp(ch, char_get_rp(ch) - (1));
    char_get_desc(ch)->rpp = char_get_rp(ch);
    userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
    send_to_char(g, ch, "@D(@cRPP@W: @w-1@D)@n\n\n");
    return;
  }
  send_to_char(g, ch, "Your voice has now been set to: {}\r\n", text);
  char_set_voice(ch, text);
  return;
}

ACMD(do_languages)
{
  int i, found = false;
  std::string arg;

  if (CONFIG_ENABLE_LANGUAGES) {
    std::string_view rest = argument;
    arg                   = take_arg(rest);
    if (arg.empty())
      list_languages(g, ch);
    else {
      for (i = MIN_LANGUAGES; i <= MAX_LANGUAGES; i++) {
        if ((search_block(arg, languages, false) == i - MIN_LANGUAGES) && char_get_skill(ch, i)) {
          char_set_speaking(ch, i);
          send_to_char(g, ch, "You now speak {}.\r\n", languages[i - MIN_LANGUAGES]);
          found = true;
          break;
        }
      }
      if (!found) {
        send_to_char(g, ch, "You do not know of any such language.\r\n");
        return;
      }
    }
  } else {
    send_to_char(g, ch, "But everyone already understands everyone else!\r\n");
    return;
  }
}

static void garble_text(GameWorld *g, std::string &string, int known, int lang)
{
  char letters[50] = "";
  int i;

  switch (lang) {
    case SKILL_LANG_DWARVEN:
      strcpy(letters, "hprstwxyz");
      break;
    case SKILL_LANG_ELVEN:
      strcpy(letters, "aefhilnopstu");
      break;
    default:
      strcpy(letters, "aehiopstuwxyz");
      break;
  }

  for (i = 0; i < (int)string.size(); ++i) {
    if (isalpha(string[i]) && (!known)) {
      string[i] = letters[rand_number(g, 0, (int)strlen(letters) - 1)];
    }
  }
}

ACMD(do_osay)
{

  argument = skip_spaces(argument);

  if (char_is_npc(ch))
    return;

  if (argument.empty()) {
    send_to_char(g, ch, "Yes, but WHAT do you want to osay?\r\n");
    return;
  }
  std::string buf;
  std::string buf2;

  buf = str_format("@WYou @D[@mOSAY@D] @W'@w{}@W'@n", argument);
  if (!char_prf_flagged(g, ch, PRF_HIDE)) {
    buf2 = str_format("@W{} @D[@mOSAY@D] @W'@w{}@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), argument);
  }
  if (char_prf_flagged(g, ch, PRF_HIDE)) {
    buf2 = str_format("@WAnonymous @D[@mOSAY@D] @W'@w{}@W'@n", argument);
  }
  act(g, buf.c_str(), false, ch, nullptr, nullptr, TO_CHAR);
  act(g, buf2.c_str(), false, ch, nullptr, nullptr, TO_ROOM);
}

ACMD(do_say)
{
  struct char_data *wch = nullptr, *wch2 = nullptr, *wch3 = nullptr, *sch = nullptr;
  struct obj_data *obj = nullptr;
  int granted          = false;
  std::string buf2;

  std::string text(skip_spaces(argument));

  if (char_get_bonuses(ch, BONUS_MUTE) > 0 && char_get_in_room(ch) > 160) {
    send_to_char(g, ch, "You are mute and unable to talk though.\r\n");
    return;
  }
  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You are mute and unable to talk though. You will be allowed to just for MUD School.");
  }

  if (text.empty()) {
    send_to_char(g, ch, "Yes, but WHAT do you want to say?\r\n");
    return;
  } else {
    std::string buf;
    std::string verb;

    if (text[text.size() - 1] == '!') {
      verb = "exclaim";
    } else if (text[text.size() - 1] == '?') {
      verb = "ask";
    } else {
      verb = "say";
    }

    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (tch != ch && char_get_desc(tch)) {
        std::string sayto = str_format("to {} ", char_get_name(tch));
        if (strstr(text.c_str(), sayto.c_str())) {
          search_replace(text, sayto.c_str(), "");
          sch = tch;
        } else if (!char_is_npc(tch) && !char_is_npc(ch)) {
          if (readIntro(ch, tch) == 1) {
            sayto = str_format("to {} ", get_i_name(ch, tch));
          }
          if (strstr(text.c_str(), sayto.c_str())) {
            search_replace(text, sayto.c_str(), "");
            sch = tch;
          }
        }
      }
    }
    if (!sch) {
      buf = str_format("@w$n @W{}s, '@C{}@W'@n", verb, text);
      act(g, buf.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
    } else {
      buf  = str_format("@w$n @Wsays to @g$N@W, '@C{}@W'@n", text);
      buf2 = str_format("@w$n @Wsays to @gyou@W, '@C{}@W'@n", text);
      act(g, buf2.c_str(), true, ch, nullptr, sch, TO_VICT);
      act(g, buf.c_str(), true, ch, nullptr, sch, TO_NOTVICT);
    }
    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT)) {
      send_to_char(g, ch, CONFIG_OK);
    } else {
      if (strstr(verb.c_str(), "says to")) {
        verb = str_format("say to @g{}@W", char_get_name(sch));
      }
      buf = str_format("@WYou {}, '@C{}@W'@n\r\n", verb, text);
      send_to_char(g, ch, buf);
      add_history(g, ch, buf.c_str(), HIST_SAY);
      if (g->SHENRON == true) {
        if (char_get_in_room(ch) == g->DRAGONR && char_by_id(g, g->EDRAGON_uid) && char_get_in_room(char_by_id(g, g->EDRAGON_uid)) == g->DRAGONR) {
          if (text.contains("wish")) {

            for (descriptor_data *d : each_desc(g)) {
              if (desc_get_connected(d) != CON_PLAYING)
                continue;

              if (strstr(text.c_str(), char_get_name(d->character)) && wch == nullptr) {
                wch = d->character;
              } else if (strstr(text.c_str(), char_get_name(d->character)) && wch2 == nullptr) {
                wch2 = d->character;
              } else if (strstr(text.c_str(), char_get_name(d->character)) && wch3 == nullptr) {
                wch3 = d->character;
              }
            } /* end repeat for */

            if (wch == nullptr && text.contains("myself")) {
              wch = ch;
            }
            if (wch == nullptr) {
              return;
            }

            if (granted == false && text.contains("knowledge")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} now has more knowledge!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_practice_points(g, wch, char_get_practice_points(g, wch) + (rand_number(g, 2000, 5000)));
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a knowledge wish on {}.", char_get_name(ch), char_get_name(wch));
                ;
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end knowledge wish if */

            if (granted == false && text.contains("speed")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now faster!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_real_abils_cha(wch, char_get_real_abils_cha(wch) + 10);
                if (char_get_real_abils_cha(wch) > 100) {
                  char_set_real_abils_cha(wch, 100);
                }
                save_char(g, wch);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a speed wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end speed wish if */

            if (granted == false && text.contains("tough")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now tougher!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_armor(wch, char_get_armor(wch) + (5000));
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a tough wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end tough wish if */

            if (granted == false && text.contains("strength")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} has more strength!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_real_abils_str(wch, char_get_real_abils_str(wch) + 10);
                if (char_get_real_abils_str(wch) > 100) {
                  char_set_real_abils_str(wch, 100);
                }
                save_char(g, wch);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a strength wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end strength wish if */

            if (granted == false && text.contains("intelligence")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now smarter!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_real_abils_intel(wch, char_get_real_abils_intel(wch) + 10);
                if (char_get_real_abils_intel(wch) > 100) {
                  char_set_real_abils_intel(wch, 100);
                }
                save_char(g, wch);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a intelligence wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end intelligence wish if */

            if (granted == false && text.contains("wisdom")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now wiser!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_real_abils_wis(wch, char_get_real_abils_wis(wch) + 10);
                if (char_get_real_abils_wis(wch) > 100) {
                  char_set_real_abils_wis(wch, 100);
                }
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a wisdom wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end wisdom wish if */

            if (granted == false && text.contains("agility")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now more agile!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_real_abils_dex(wch, char_get_real_abils_dex(wch) + 10);
                if (char_get_real_abils_dex(wch) > 100) {
                  char_set_real_abils_dex(wch, 100);
                }
                save_char(g, wch);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a agility wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end agility wish if */

            if (granted == false && text.contains("constitution")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} has more guts!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_real_abils_con(wch, char_get_real_abils_con(wch) + 10);
                if (char_get_real_abils_con(wch) > 100) {
                  char_set_real_abils_con(wch, 100);
                }
                save_char(g, wch);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a constitutionwish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end constitution wish if */

            if (granted == false && text.contains("skill")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} has more skill!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                int roll = rand_number(g, 1, 3);
                send_to_char(g, wch, "@GYou suddenly feel like you could learn {:d} more skills!@n\r\n", roll);
                char_set_skill_slots(wch, char_get_skill_slots(wch) + (roll));
                save_char(g, wch);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a skill wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end skill wish if */
            /* Rillao: transloc, add new transes here */
            if (granted == false && text.contains("power")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish cannot be granted, You might want to try something else instead, mortal!@w'@n\r\n");
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
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end power wish if */

            if (granted == false && text.contains("money")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} now has become richer!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_set_bank_gold(wch, char_get_bank_gold(wch) + (1000000));
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a money wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end money wish if */

            if (granted == false && text.contains("immunity")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} now has immunity to Burn, Freezing, Mind Break, Poison, Blindness, Yoikominminken, and Paralysis!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_aff_set(wch, AFF_IMMUNITY, true);
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a immunity wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end money wish if */

            if (granted == false && text.contains("vitality")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish cannot be granted, You might want to try something else instead, mortal!@w'@n\r\n");
                /*send_to_room(real_room(DRAGONR), "@wShenron says, '@CYour wish has been granted, %s now will never hunger or thirst again!%s@w'@n\r\n", GET_NAME(wch), WISH[0] ? "" : " Now make your second wish.");
           GET_COND(ch, HUNGER) = -1;
           GET_COND(ch, THIRST) = -1;
           granted = TRUE;
           SELFISHMETER += 1;
		   mudlog(NRM, ADMLVL_GOD, TRUE, "Shenron: %s has made a vitality wish on %s.", GET_NAME(ch), GET_NAME(wch));*/
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end vitality if */

            if (granted == false && text.contains("revive")) {
              int count = 0;
              if (wch != nullptr) {
                count += 1;
              }
              if (wch2 != nullptr) {
                count += 1;
              }
              if (wch3 != nullptr) {
                count += 1;
              }
              if (count == 1) {
                if (!char_aff_flagged(wch, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
                } else {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} has returned to life!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                  if (!room_of(g, char_get_droom(wch))) {
                    char_set_droom(wch, 300);
                  }
                  if (room_of(g, char_get_droom(wch))) {
                    char_from_room(g, wch);
                    if (char_get_droom(wch) > 0) {
                      char_to_room(g, wch, char_get_droom(wch));
                    } else {
                      char_to_room(g, wch, 300);
                    }
                    look_at_room(g, char_get_in_room(wch), wch, 0);
                    send_to_char(g, wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
                    char_aff_set(wch, AFF_SPIRIT, false);
                    char_aff_set(wch, AFF_ETHEREAL, false);
                  }
                  granted = true;
                  g->SELFISHMETER -= 2;
                  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a revive wish on {}.", char_get_name(ch), char_get_name(wch));
                }
              } /* is there a target for the wish? */
              if (count == 2) {
                if (!char_aff_flagged(wch, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
                }
                if (!char_aff_flagged(wch2, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch2));
                } else if (char_aff_flagged(wch, AFF_SPIRIT) && char_aff_flagged(wch2, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} and {} have returned to life!{}@w'@n\r\n", char_get_name(wch), char_get_name(wch2), g->WISH[0] ? "" : " Now make your second wish.");
                  if (!room_of(g, char_get_droom(wch))) {
                    char_set_droom(wch, 300);
                  }
                  if (room_of(g, char_get_droom(wch))) {
                    char_from_room(g, wch);
                    char_to_room(g, wch, char_get_droom(wch));
                    look_at_room(g, char_get_in_room(wch), wch, 0);
                    send_to_char(g, wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
                    char_aff_set(wch, AFF_SPIRIT, false);
                    char_aff_set(wch, AFF_ETHEREAL, false);
                  }
                  if (!room_of(g, char_get_droom(wch2))) {
                    char_set_droom(wch2, 300);
                  }
                  if (room_of(g, char_get_droom(wch2))) {
                    char_from_room(g, wch2);
                    char_to_room(g, wch2, char_get_droom(wch2));
                    look_at_room(g, char_get_in_room(wch2), wch2, 0);
                    send_to_char(g, wch2, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
                    char_aff_set(wch2, AFF_SPIRIT, false);
                    char_aff_set(wch2, AFF_ETHEREAL, false);
                  }
                  granted = true;
                  g->SELFISHMETER -= 3;
                  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a revive wish on {}.", char_get_name(ch), char_get_name(wch2));
                  char_set_wait(ch, PULSE_4SEC);
                }
              } /* is there two targets for the wish? */
              if (count == 3) {
                if (!char_aff_flagged(wch, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
                }
                if (!char_aff_flagged(wch2, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch2));
                }
                if (!char_aff_flagged(wch3, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch3));
                } else if (char_aff_flagged(wch, AFF_SPIRIT) && char_aff_flagged(wch2, AFF_SPIRIT) && char_aff_flagged(wch3, AFF_SPIRIT)) {
                  send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {}, {}, and {} have returned to life!!{}@w'@n\r\n", char_get_name(wch), char_get_name(wch2), char_get_name(wch3), g->WISH[0] ? "" : " Now make your second wish.");
                  if (!room_of(g, char_get_droom(wch))) {
                    char_set_droom(wch, 300);
                  }
                  if (room_of(g, char_get_droom(wch))) {
                    char_from_room(g, wch);
                    char_to_room(g, wch, char_get_droom(wch));
                    look_at_room(g, char_get_in_room(wch), wch, 0);
                    send_to_char(g, wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
                    char_aff_set(wch, AFF_SPIRIT, false);
                    char_aff_set(wch, AFF_ETHEREAL, false);
                  }
                  if (!room_of(g, char_get_droom(wch2))) {
                    char_set_droom(wch2, 300);
                  }
                  if (room_of(g, char_get_droom(wch2))) {
                    char_from_room(g, wch2);
                    char_to_room(g, wch2, char_get_droom(wch2));
                    look_at_room(g, char_get_in_room(wch2), wch2, 0);
                    send_to_char(g, wch2, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
                    char_aff_set(wch2, AFF_SPIRIT, false);
                    char_aff_set(wch2, AFF_ETHEREAL, false);
                  }
                  if (!room_of(g, char_get_droom(wch3))) {
                    char_set_droom(wch3, 300);
                  }
                  if (room_of(g, char_get_droom(wch3))) {
                    char_from_room(g, wch3);
                    char_to_room(g, wch3, char_get_droom(wch3));
                    look_at_room(g, char_get_in_room(wch3), wch3, 0);
                    send_to_char(g, wch3, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
                    char_aff_set(wch3, AFF_SPIRIT, false);
                    char_aff_set(wch3, AFF_ETHEREAL, false);
                  }
                  granted = true;
                  g->SELFISHMETER -= 3;
                  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a revive wish on {} and {}.", char_get_name(ch), char_get_name(wch2), char_get_name(wch3));
                  char_set_wait(ch, PULSE_4SEC);
                }
              } /* is there three targets for the wish? */
            } /* end revival if */

            if (granted == false && text.contains("immortal") && g->WISH[0] == 0) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now immortal!@w'@n\r\n", char_get_name(wch));
                char_plr_set(wch, PLR_IMMORTAL, true);
                g->WISH[0] = 1;
                g->WISH[1] = 1;
                granted = true;
                g->SELFISHMETER += 4;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a immortal wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end immortal wish if */

            if (granted == false && text.contains("immortal") && g->WISH[0] == 1) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CI can not grant that wish, there is not enough remaining power in this summoning!@w'@n\r\n");
              } /* is there a target for the wish? */
            }

            if (granted == false && text.contains(" mortal")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now mortal!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                char_plr_set(wch, PLR_IMMORTAL, false);
                granted = true;
                g->SELFISHMETER += 4;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a mortal wish on {}.", char_get_name(ch), char_get_name(wch));
                char_set_wait(ch, PULSE_4SEC);
              } /* is there a target for the wish? */
            } /* end mortal wish if */

            if (granted == false && text.contains("senzu")) {
              if (wch != nullptr) {
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                obj = read_object(g, 1);
                obj_to_char(g, obj, ch);
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} now possesses 10 senzus!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                granted = true;
                g->SELFISHMETER += 1;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a senzu wish.", char_get_name(ch));
              } /* is there a target for the wish? */
            } /* end senzu wish if */

            if (granted == false && text.contains("roleplay")) {
              if (wch != nullptr) {
                send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {}!{}@w'@n\r\n", char_get_name(wch), g->WISH[0] ? "" : " Now make your second wish.");
                granted = true;
                mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a roleplay wish.", char_get_name(ch));
                char_set_wait(ch, PULSE_4SEC);
              }
            }

            if (granted == true) {
              if (g->WISH[0] == 1) {
                g->WISH[1] = 1;
              } else {
                g->WISH[0] = 1;
              } /*end WISH if */
              if (wch != nullptr) {
                save_char(g, wch);
              }
              if (wch2 != nullptr) {
                save_char(g, wch);
              }
              if (wch3 != nullptr) {
                save_char(g, wch);
              }
              save_mud_time(g, &g->time_info);
            } else if (wch == nullptr) {
              send_to_room(g, g->DRAGONR, "@wShenron says, '@CThat person does not exist, make another wish.'@n\r\n");
            } else {
              send_to_room(g, g->DRAGONR, "@wShenron says, '@CDo not waste my time with wishes I can not grant...@w'@n\r\n");
            }
          }
        } /* end DRAGONR if */
      } /* end SHENRON if */
    }
  }

  /* trigger check */
  speech_mtrigger(g, ch, text.data());
  speech_wtrigger(g, ch, text.data());
  if (g->SHENRON == false || (g->SHENRON == true && char_get_in_room(ch) != g->DRAGONR)) {
    mob_talk(g, ch, text.c_str());
  }
}

ACMD(do_gsay)
{
  struct char_data *k;
  std::string blah;

  argument = skip_spaces(argument);

  if (!char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(g, ch, "But you are not the member of a group!\r\n");
    return;
  }
  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "Lol, no.\r\n");
    return;
  }
  if (argument.empty())
    send_to_char(g, ch, "Yes, but WHAT do you want to group-say?\r\n");
  else {
    std::string buf;

    if (char_get_master(g, ch))
      k = char_get_master(g, ch);
    else
      k = ch;

    buf = std::string(argument);

    blah = str_format("$n@W tells the group @W'@G{}@W'@n\r\n", buf);

    if (char_aff_flagged(k, AFF_GROUP) && (k != ch) && char_is_awake(k)) {
      if (CONFIG_ENABLE_LANGUAGES) {
        send_to_char(g, k, "{}@W tells the group{} @W'@G{}@W'@n\r\n", char_can_see(g, k, ch) ? char_get_name(ch) : "Someone", char_get_skill(k, char_get_speaking(ch)) ? "," : ", in an unfamiliar tongue,", buf);
      } else {
        act(g, blah.c_str(), true, ch, nullptr, k, TO_VICT);
      }
    }
    for (char_data *fol : char_followers(g, k))
      if (char_aff_flagged(fol, AFF_GROUP) && (fol != ch) && char_is_awake(fol)) {
        if (!char_is_npc(ch) && !char_is_npc(fol) && CONFIG_ENABLE_LANGUAGES) {
          garble_text(g, buf, char_get_skill(fol, char_get_speaking(ch)), char_get_speaking(ch));
        } else {
          garble_text(g, buf, 1, MIN_LANGUAGES);
        }
        if (CONFIG_ENABLE_LANGUAGES) {
          send_to_char(g, fol, "{}@W tells the group{} @W'{}@W'@n\r\n", char_can_see(g, fol, ch) ? char_get_name(ch) : "Someone", char_get_skill(fol, char_get_speaking(ch)) ? "," : ", in an unfamiliar tongue,", buf);
        } else {
          act(g, blah.c_str(), true, ch, nullptr, fol, TO_VICT | TO_SLEEP);
        }
      }

    if (char_prf_flagged(g, ch, PRF_NOREPEAT))
      send_to_char(g, ch, CONFIG_OK);
    else
      send_to_char(g, ch, "@WYou tell the group, '@G{}@W'@n\r\n", argument);
  }
}

static void perform_tell(GameWorld *g, struct char_data *ch, struct char_data *vict, const char *arg)
{
  std::string buf;
  std::string buf2;

  buf = arg;

  if (CONFIG_ENABLE_LANGUAGES) {
    buf2 = str_format("@[13]{} tells you{} '{}@[13]'@n\r\n", char_can_see(g, vict, ch) ? char_get_name(ch) : "Someone", char_get_skill(vict, char_get_speaking(ch)) ? "," : ", in an unfamiliar tongue,", buf);
    send_to_char(g, vict, buf2);
    add_history(g, vict, buf2.c_str(), HIST_TELL);
  } else if (!char_is_npc(ch) && char_get_admlevel(vict) < 1) {
    buf2 = str_format("@Y{}@Y tells you '{}'@n\r\n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), buf);
    send_to_char(g, vict, buf2);
    add_history(g, vict, buf2.c_str(), HIST_TELL);
  } else if (!char_is_npc(ch) && char_get_admlevel(vict) >= 1) {
    buf2 = str_format("@Y{}({})@Y tells you '{}'@n\r\n", char_get_user(ch), char_get_name(ch), buf);
    send_to_char(g, vict, buf2);
    add_history(g, vict, buf2.c_str(), HIST_TELL);
  } else if (char_is_npc(ch)) {
    buf2 = str_format("@Y{}@Y tells you '{}'@n\r\n", char_get_name(ch), buf);
    send_to_char(g, vict, buf2);
  }

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT)) {
    send_to_char(g, ch, CONFIG_OK);
  } else {
    if (!char_is_npc(ch)) {
      buf2 = str_format("@YYou tell {}, '{}'@n\r\n", char_get_admlevel(vict) > 0 ? char_get_name(vict) : char_get_user(vict), arg);
      if (char_get_admlevel(ch) < 5 && char_get_admlevel(vict) < 5 && !char_is_npc(ch) && !char_is_npc(vict)) {
        send_to_imm(g, "@GTELL: @C{}@G tells @c{}, @W'@w{}@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), char_get_admlevel(vict) > 0 ? char_get_name(vict) : char_get_user(vict), arg);
      }
      send_to_char(g, ch, buf2);
      add_history(g, ch, buf2.c_str(), HIST_TELL);
    } else {
      send_to_char(g, ch, CONFIG_OK);
    }
  }

  if (!char_is_npc(vict) && !char_is_npc(ch))
    char_set_last_tell(g, vict, char_get_idnum(ch));
}

static bool is_tell_ok(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  if (ch == vict)
    send_to_char(g, ch, "You try to tell yourself something.\r\n");
  else if (!char_is_npc(ch) && char_get_level(ch) < 3 && char_get_admlevel(vict) < 1)
    send_to_char(g, ch, "You need to be level 3 or higher to send or receive tells");
  else if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_AFK) && char_get_admlevel(ch) < 1)
    send_to_char(g, ch, "You can't send tells when AFK.\r\n");
  else if (!char_is_npc(ch) && char_prf_flagged(g, vict, PRF_AFK) && char_get_admlevel(ch) < 1)
    send_to_char(g, ch, "They are AFK right now, try later.\r\n");
  else if (!char_is_npc(ch) && char_prf_flagged(g, vict, PRF_AFK) && char_get_admlevel(ch) >= 1)
    return (true);
  else if (!char_is_npc(vict) && char_get_level(vict) < 3 && char_get_admlevel(ch) < 1)
    send_to_char(g, ch, "They need to be level 3 or higher to send or receive tells");
  else if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOTELL) && char_get_admlevel(vict) < 1)
    send_to_char(g, ch, "You can't tell other people while you have notell on.\r\n");
  else if (room_flagged(g, char_get_in_room(ch), ROOM_SOUNDPROOF) && char_get_admlevel(vict) < 1)
    send_to_char(g, ch, "The walls seem to absorb your words.\r\n");
  else if (char_is_npc(vict))
    send_to_char(g, ch, "You can't send tells to mobs.\r\n");
  else if (!char_is_npc(vict) && !char_get_desc(vict)) /* linkless */
    act(g, "$E's linkless at the moment.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
  else if (char_plr_flagged(vict, PLR_WRITING))
    act(g, "$E's writing a message right now; try again later.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
  else if ((!char_is_npc(vict) && char_get_admlevel(ch) < 1 && char_prf_flagged(g, vict, PRF_NOTELL)) || room_flagged(g, char_get_in_room(vict), ROOM_SOUNDPROOF))
    act(g, "$E can't hear you.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
  else
    return (true);

  return (false);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = nullptr;
  std::string buf, buf2;

  std::string_view rest = argument;
  buf                   = take_word(rest);
  buf2                  = skip_spaces(rest);

  if (buf.empty() || buf2.empty()) {
    send_to_char(g, ch, "Who do you wish to tell what??\r\n");
    return;
  }
  int found = false;
  if (!char_is_npc(ch)) {
    CAP(buf);
    for (descriptor_data *k : each_desc(g)) {
      if (desc_get_connected(k) != CON_PLAYING)
        continue;
      if (char_is_npc(k->character))
        continue;
      if (k->user.empty())
        continue;
      if (found == false && !char_is_npc(ch) && (str_eq_nocase(k->user, buf) || strstr(k->user.c_str(), buf.c_str()))) {
        vict  = k->character;
        found = true;
      } else if (!char_is_npc(ch) && found == false && (str_eq_nocase(char_get_name(k->character), buf) || strstr(char_get_name(k->character), buf.c_str())) && char_get_admlevel(k->character) > 0) {
        vict  = k->character;
        found = true;
      }
    }
  }
  if (found == false && !char_is_npc(ch))
    send_to_char(g, ch, "No user around with that name.");
  else if (char_is_npc(ch) && !(vict = get_player_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, CONFIG_NOPERSON);
  else if (is_tell_ok(g, ch, vict))
    perform_tell(g, ch, vict, buf2.c_str());
}

ACMD(do_reply)
{
  struct char_data *tch;

  if (char_is_npc(ch))
    return;

  if (room_flagged(g, char_get_in_room(ch), ROOM_HBTC)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), ROOM_PAST)) {
    send_to_char(g, ch, "This is the past, you can't send tells!\r\n");
    return;
  }

  std::string text(skip_spaces(argument));

  if (char_get_last_tell(g, ch) == NOBODY)
    send_to_char(g, ch, "You have nobody to reply to!\r\n");
  else if (text.empty())
    send_to_char(g, ch, "What is your reply?\r\n");
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
    tch = find_first(each_char(g), [&](char_data *c) { return !char_is_npc(c) && char_get_idnum(c) == char_get_last_tell(g, ch); });

    if (tch == nullptr)
      send_to_char(g, ch, "They are no longer playing.\r\n");
    else if (is_tell_ok(g, ch, tch))
      perform_tell(g, ch, tch, text.c_str());
  }
}

ACMD(do_spec_comm)
{
  std::string buf, buf2;
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You are mute and unable to talk though.\r\n");
    return;
  }

  switch (subcmd) {
    case SCMD_WHISPER:
      action_sing   = "whisper to";
      action_plur   = "whispers to";
      action_others = "$n whispers something to $N.";
      break;

    case SCMD_ASK:
      action_sing   = "ask";
      action_plur   = "asks";
      action_others = "$n asks $N a question.";
      break;

    default:
      action_sing   = "oops";
      action_plur   = "oopses";
      action_others = "$n is tongue-tied trying to speak with $N.";
      break;
  }

  std::string_view rest = argument;
  buf                   = take_word(rest);
  buf2                  = skip_spaces(rest);

  if (buf.empty() || buf2.empty())
    send_to_char(g, ch, "Whom do you want to {}.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_ROOM)))
    send_to_char(g, ch, CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(g, ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    std::string buf1;
    std::string obuf;

    if (CONFIG_ENABLE_LANGUAGES) {
      obuf = buf2;
      garble_text(g, obuf, char_get_skill(vict, char_get_speaking(ch)), char_get_speaking(ch));
      buf1 = str_format("$n {} you{} '{}'", action_plur, char_get_skill(vict, char_get_speaking(ch)) ? "," : ", in an unfamiliar tongue,", obuf);
    } else {
      buf1 = str_format("@c$n @W{} you '@m{}@W'@n", action_plur, buf2);
    }

    act(g, buf1.c_str(), false, ch, nullptr, vict, TO_VICT);

    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT)) {
      send_to_char(g, ch, CONFIG_OK);
    } else {
      std::string blum = str_format("@WYou {} @C$N@W, '@m{}@W'@n\r\n", action_sing, buf2);
      act(g, blum.c_str(), true, ch, nullptr, vict, TO_CHAR);
    }

    if (subcmd == SCMD_WHISPER) {
      act(g, action_others, false, ch, nullptr, vict, TO_NOTVICT);
      handle_whisper(g, buf2, ch, vict);
    } else {
      act(g, action_others, false, ch, nullptr, vict, TO_NOTVICT);
    }
  }
}

static void handle_whisper(GameWorld *g, std::string &buf, struct char_data *ch, struct char_data *vict)
{

  for (char_data *tch : room_people(g, char_get_in_room(ch))) {
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
      int roll1 = rand_number(g, 10, 30);
      int roll  = rand_number(g, roll1, 110);

      if (skill >= roll) {
        send_to_char(g, tch, "@WYou overhear everything whispered, @W'@m{}@W'@n\r\n", overhear(g, buf, 3));
      } else if (skill + 10 >= roll) {
        send_to_char(g, tch, "@WYou overhear a lot of what is whispered, @W'@m{}@W'@n\r\n", overhear(g, buf, 2));
      } else if (skill + 20 >= roll) {
        send_to_char(g, tch, "@WYou overhear some of what is whispered, @W'@m{}@W'@n\r\n", overhear(g, buf, 1));
      } else if (skill + 30 >= roll) {
        send_to_char(g, tch, "@WYou overhear little of what is whispered, @W'@m{}@W'@n\r\n", overhear(g, buf, 0));
      } else {
        send_to_char(g, tch, "@WYou were unable to overhear anything that was whispered.@n\r\n");
      }
    }
  }
}

static const char *overhear(GameWorld *g, std::string &buf, int type)
{

  switch (type) {
    case 0:
      if (rand_number(g, 1, 10) >= 5) {
        search_replace(buf, "a", "..");
        search_replace(buf, "A", "..");
        search_replace(buf, "h", "..");
        search_replace(buf, "H", "..");
        search_replace(buf, "e", "..");
        search_replace(buf, "E", "..");
        search_replace(buf, "m", "..");
        search_replace(buf, "M", "..");
        search_replace(buf, "o", "..");
        search_replace(buf, "O", "..");
        search_replace(buf, "p", "..");
        search_replace(buf, "P", "..");
        search_replace(buf, "y", "..");
        search_replace(buf, "Y", "..");
        search_replace(buf, "j", "..");
        search_replace(buf, "J", "..");
        search_replace(buf, "k", "..");
        search_replace(buf, "K", "..");
        search_replace(buf, "d", "..");
        search_replace(buf, "D", "..");
        search_replace(buf, "w", "..");
        search_replace(buf, "W", "..");
      } else if (rand_number(g, 1, 10) >= 5) {
        search_replace(buf, "e", "..");
        search_replace(buf, "E", "..");
        search_replace(buf, "r", "..");
        search_replace(buf, "R", "..");
        search_replace(buf, "k", "..");
        search_replace(buf, "K", "..");
        search_replace(buf, "m", "..");
        search_replace(buf, "M", "..");
        search_replace(buf, "o", "..");
        search_replace(buf, "O", "..");
        search_replace(buf, "p", "..");
        search_replace(buf, "P", "..");
        search_replace(buf, "y", "..");
        search_replace(buf, "Y", "..");
        search_replace(buf, "j", "..");
        search_replace(buf, "J", "..");
        search_replace(buf, "k", "..");
        search_replace(buf, "K", "..");
        search_replace(buf, "d", "..");
        search_replace(buf, "D", "..");
        search_replace(buf, "w", "..");
        search_replace(buf, "W", "..");
      } else {
        search_replace(buf, "s", "..");
        search_replace(buf, "S", "..");
        search_replace(buf, "r", "..");
        search_replace(buf, "R", "..");
        search_replace(buf, "c", "..");
        search_replace(buf, "C", "..");
        search_replace(buf, "q", "..");
        search_replace(buf, "Q", "..");
        search_replace(buf, "l", "..");
        search_replace(buf, "L", "..");
        search_replace(buf, "u", "..");
        search_replace(buf, "U", "..");
        search_replace(buf, "i", "..");
        search_replace(buf, "I", "..");
        search_replace(buf, "z", "..");
        search_replace(buf, "Z", "..");
        search_replace(buf, "t", "..");
        search_replace(buf, "T", "..");
      }
      return buf.c_str();
      break;
    case 1:
      if (rand_number(g, 1, 10) >= 5) {
        search_replace(buf, "b", "..");
        search_replace(buf, "B", "..");
        search_replace(buf, "f", "..");
        search_replace(buf, "F", "..");
        search_replace(buf, "g", "..");
        search_replace(buf, "G", "..");
        search_replace(buf, "v", "..");
        search_replace(buf, "V", "..");
        search_replace(buf, "j", "..");
        search_replace(buf, "J", "..");
        search_replace(buf, "k", "..");
        search_replace(buf, "K", "..");
      } else if (rand_number(g, 1, 10) >= 5) {
        search_replace(buf, "d", "..");
        search_replace(buf, "D", "..");
        search_replace(buf, "y", "..");
        search_replace(buf, "Y", "..");
        search_replace(buf, "m", "..");
        search_replace(buf, "M", "..");
        search_replace(buf, "h", "..");
        search_replace(buf, "H", "..");
        search_replace(buf, "s", "..");
        search_replace(buf, "S", "..");
        search_replace(buf, "t", "..");
        search_replace(buf, "T", "..");
      } else {
        search_replace(buf, "a", "..");
        search_replace(buf, "A", "..");
        search_replace(buf, "r", "..");
        search_replace(buf, "R", "..");
        search_replace(buf, "n", "..");
        search_replace(buf, "N", "..");
        search_replace(buf, "o", "..");
        search_replace(buf, "O", "..");
      }
      return buf.c_str();
      break;
    case 2:
      if (rand_number(g, 1, 10) >= 5) {
        search_replace(buf, "q", "..");
        search_replace(buf, "Q", "..");
        search_replace(buf, "o", "..");
        search_replace(buf, "O", "..");
        search_replace(buf, "i", "..");
        search_replace(buf, "I", "..");
        search_replace(buf, "g", "..");
        search_replace(buf, "G", "..");
      } else if (rand_number(g, 1, 10) >= 5) {
        search_replace(buf, "a", "..");
        search_replace(buf, "A", "..");
        search_replace(buf, "e", "..");
        search_replace(buf, "E", "..");
        search_replace(buf, "i", "..");
        search_replace(buf, "I", "..");
        search_replace(buf, "o", "..");
        search_replace(buf, "O", "..");
      } else {
        search_replace(buf, "k", "..");
        search_replace(buf, "K", "..");
        search_replace(buf, "m", "..");
        search_replace(buf, "M", "..");
        search_replace(buf, "b", "..");
        search_replace(buf, "B", "..");
      }
      return buf.c_str();
      break;
    case 3:
      return buf.c_str();
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
  struct obj_data *paper, *pen = nullptr, *obj;
  std::string papername, penname;

  /* before we do anything, lets see if there's a board involved. */
  auto is_board = [&](obj_data *o) { return obj_get_type_flag(o) == ITEM_BOARD; };
  obj           = find_first(char_inventory(g, ch), is_board);
  if (!obj)
    obj = find_first(room_contents(g, char_get_in_room(ch)), is_board);

  if (obj) { /* then there IS a board! */
    write_board_message(g, obj_get_vnum(g, obj), ch, argument);
    return;
  }

  std::string_view rest = argument;
  papername             = take_arg(rest);
  penname               = take_arg(rest);

  if (!char_get_desc(ch))
    return;

  if (papername.empty()) { /* nothing was delivered */
    send_to_char(g, ch, "write on [what] with [what pen?]\r\n");
    return;
  }
  if (!penname.empty()) { /* there were two arguments */
    if (!(paper = get_obj_in_list_vis(g, ch, papername, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You have no {}.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(g, ch, penname, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You have no {}.\r\n", penname);
      return;
    }
  } else { /* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(g, ch, papername, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "There is no {} in your inventory.\r\n", papername);
      return;
    }
    if (obj_get_type_flag(paper) == ITEM_PEN) { /* oops, a pen.. */
      pen   = paper;
      paper = nullptr;
    } else if (obj_get_type_flag(paper) != ITEM_NOTE) {
      send_to_char(g, ch, "That thing has nothing to do with writing.\r\n");
      return;
    }
    /* One object was found.. now for the other one. */
    if (!char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(g, ch, "You can't write with {} {} alone.\r\n", AN(papername), papername);
      return;
    }
    if (!char_can_see_obj(g, ch, char_get_equipment(ch, WEAR_WIELD2))) {
      send_to_char(g, ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = char_get_equipment(ch, WEAR_WIELD2);
    else
      pen = char_get_equipment(ch, WEAR_WIELD2);
  }

  /* ok.. now let's see what kind of stuff we've found */
  if (obj_get_type_flag(pen) != ITEM_PEN)
    act(g, "$p is no good for writing with.", false, ch, pen, nullptr, TO_CHAR);
  else if (obj_get_type_flag(paper) != ITEM_NOTE)
    act(g, "You can't write on $p.", false, ch, paper, nullptr, TO_CHAR);
  else {
    /* Something on it, display it as that's in input buffer. */
    if (obj_get_action_description(paper)) {
      send_to_char(g, ch, "There's something written on it already:\r\n");
      send_to_char(g, ch, "{}", obj_get_action_description(paper));
    }

    /* we can write - hooray! */
    act(g, "$n begins to jot down a note.", true, ch, nullptr, nullptr, TO_ROOM);
    obj_extra_set(paper, ITEM_UNIQUE_SAVE, true);
    send_editor_help(g, char_get_desc(ch));
    string_write(char_get_desc(ch), obj_action_description_slot(paper), MAX_NOTE_LENGTH, 0);
  }
}

ACMD(do_page)
{
  struct char_data *vict;
  std::string buf2, arg;

  std::string_view rest = argument;
  arg                   = take_word(rest);
  buf2                  = skip_spaces(rest);

  if (char_is_npc(ch))
    send_to_char(g, ch, "Monsters can't page.. go away.\r\n");
  else if (arg.empty())
    send_to_char(g, ch, "Whom do you wish to page?\r\n");
  else {
    std::string buf = str_format("\007\007*$n* {}", buf2);
    if (str_eq_nocase(arg, "all")) {
      if (char_adm_flagged(ch, ADM_TELLALL)) {
        for (descriptor_data *d : each_desc(g))
          if (desc_get_connected(d) == CON_PLAYING && d->character)
            act(g, buf.c_str(), false, ch, nullptr, d->character, TO_VICT);
      } else
        send_to_char(g, ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)) != nullptr) {
      act(g, buf.c_str(), false, ch, nullptr, vict, TO_VICT);
      if (char_prf_flagged(g, ch, PRF_NOREPEAT))
        send_to_char(g, ch, CONFIG_OK);
      else
        act(g, buf.c_str(), false, ch, nullptr, vict, TO_CHAR);
    } else
      send_to_char(g, ch, "There is no such person in the game!\r\n");
  }
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  std::string color_on;
  std::string buf1;
  std::string buf2;
  char *msg;

  buf1.clear();
  buf2.clear();

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
      PRF_NOMUSIC,
      PRF_DEAF,
      PRF_NOGOSS,
      PRF_NOAUCT,
      PRF_NOGRATZ,
      0};
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
       "@[9]" },

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
       "@[12]"},

      {"You cannot gemote!!\r\n",
       "gemote",
       "You aren't even on the channel!\r\n",
       "@[10]"}
  };

  /* to keep pets, etc from being ordered to shout */
  if (!char_get_desc(ch))
    return;

  if (char_plr_flagged(ch, PLR_NOSHOUT)) {
    send_to_char(g, ch, "{}", com_msgs[subcmd][0]);
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), ROOM_SOUNDPROOF)) {
    send_to_char(g, ch, "The walls seem to absorb your words.\r\n");
    return;
  }

  if (subcmd == SCMD_SHOUT && char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You are mute and are incapable of speech.\r\n");
    return;
  }

  /* skip leading spaces */
  std::string text(skip_spaces(argument));

  if (subcmd == SCMD_GOSSIP && (text[0] == '*')) {
    subcmd = SCMD_GEMOTE;
  }

  if (subcmd == SCMD_GEMOTE) {
    if (text[0] == '*' || text[0] == ':')
      do_gmote(g, ch, std::string_view(text).substr(1), 0, 1);
    else
      do_gmote(g, ch, text, 0, 1);

    return;
  }

  /* level_can_shout defined in config.c */
  if (char_get_level(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(g, ch, "You must be at least level {:d} before you can {}.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* make sure the char is on the channel */
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, channels[subcmd])) {
    send_to_char(g, ch, "{}", com_msgs[subcmd][2]);
    return;
  }

  /* make sure that there is something there to say! */
  if (text.empty()) {
    send_to_char(g, ch, "Yes, {}, fine, {} we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  delete_doubledollar(text);
  /* set up the color on code */
  color_on = com_msgs[subcmd][3];

  /* first, set up strings to be given to the communicator */
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT)) {
    send_to_char(g, ch, CONFIG_OK);
  } else {
    buf2 = str_format("{}@WYou {}@W, '@w{}@W'{}@n\r\n", color_on, com_msgs[subcmd][1], text, color_on);
    send_to_char(g, ch, buf2);
    add_history(g, ch, buf2.c_str(), hist_type[subcmd]);
  }

  /* now send all the strings out */
  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) == CON_PLAYING && i != char_get_desc(ch) && i->character &&
        (char_is_npc(i->character) || !char_prf_flagged(g, i->character, channels[subcmd])) &&
        (char_is_npc(i->character) || !char_plr_flagged(i->character, PLR_WRITING)) &&
        !room_flagged(g, char_get_in_room(i->character), ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
          ((room_get_zone(g, char_get_in_room(ch)) != room_get_zone(g, char_get_in_room(i->character))) ||
           !char_is_awake(i->character)))
        continue;

      if (CONFIG_ENABLE_LANGUAGES) {
        garble_text(g, text, char_get_skill(i->character, char_get_speaking(ch)), char_get_speaking(ch));
        buf1 = str_format("{}{} {}s{} '{}@n'{}", color_on, char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), com_msgs[subcmd][1], char_get_skill(i->character, char_get_speaking(ch)) ? "," : ", in an unfamiliar tongue,", text, color_on);
      } else if (subcmd == SCMD_SHOUT && char_get_in_room(i->character) != char_get_in_room(ch)) {
        buf1 = str_format("{}@WSomeone nearby {}s@W, '@w{}@W'@n{}", color_on, com_msgs[subcmd][1], text, color_on);
      } else if (subcmd == SCMD_SHOUT && char_get_in_room(i->character) == char_get_in_room(ch)) {
        buf1 = str_format("{}@W$n@W {}s@W, '@w{}@W'@n{}", color_on, com_msgs[subcmd][1], text, color_on);
      } else {
        if (char_get_admlevel(ch) > 0) {
          buf1 = str_format("{}@W$n {}s@W, '@w{}@W'@n{}", color_on, com_msgs[subcmd][1], text, color_on);
        } else if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_HIDE) && char_get_admlevel(i->character) < ADMLVL_IMMORT && ch != i->character) {
          buf1 = str_format("{}@WAnonymous Player {}s@W, '@w{}@W'@n{}", color_on, com_msgs[subcmd][1], text, color_on);
        } else if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_HIDE) && char_get_admlevel(i->character) >= ADMLVL_IMMORT && ch != i->character) {
          buf1 = str_format("{}@W{}(H) {}s@W, '@w{}@W'@n{}", color_on, char_get_admlevel(i->character) > 0 ? char_get_name(ch) : char_get_user(ch), com_msgs[subcmd][1], text, color_on);
        } else if (char_get_admlevel(i->character) > 0) {
          buf1 = str_format("{}@W{} ($n) {}s@W, '@w{}@W'@n{}", color_on, char_get_user(ch), com_msgs[subcmd][1], text, color_on);
        } else {
          buf1 = str_format("{}@W{} {}s@W, '@w{}@W'@n{}", color_on, char_get_admlevel(i->character) > 0 ? char_get_name(ch) : char_get_user(ch), com_msgs[subcmd][1], text, color_on);
        }
      }

      msg = act(g, buf1.c_str(), false, ch, nullptr, i->character, TO_VICT | TO_SLEEP);
      add_history(g, i->character, msg, hist_type[subcmd]);
    }
  }
  if (char_get_spam(ch) >= 3 && char_get_admlevel(ch) < 1) {
    send_to_imm(g, "SPAMMING: {} has been frozen for spamming!\r\n", char_get_name(ch));
    send_to_all(g, "@rSPAMMING@D: @C{}@w has been frozen for spamming, let that be a lesson to 'em.@n\r\n", char_get_name(ch));
    char_plr_set(ch, PLR_FROZEN, true);
    char_set_freeze_level(g, ch, 1);
  } else if (char_get_spam(ch) < 3) {
    char_set_spam(ch, char_get_spam(ch) + (1));
  }
}

ACMD(do_respond)
{
  int found = 0, mnum = 0;
  struct obj_data *obj;

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "As a mob, you never bothered to learn to read or write.\r\n");
    return;
  }

  auto is_board = [&](obj_data *o) { return obj_get_type_flag(o) == ITEM_BOARD; };
  obj           = find_first(char_inventory(g, ch), is_board);
  if (!obj)
    obj = find_first(room_contents(g, char_get_in_room(ch)), is_board);
  if (obj)
    found = 1;
  if (obj) {
    std::string_view rest = argument;
    std::string number    = take_arg(rest);
    if (number.empty()) {
      send_to_char(g, ch, "Respond to what?\r\n");
      return;
    }
    if (!isdigit(number[0]) || (!(mnum = parse_int(number)))) {
      send_to_char(g, ch, "You must type the number of the message you wish to reply to.\r\n");
      return;
    }
    board_respond(g, obj_get_vnum(g, obj), ch, mnum);
  }

  /* No board in the room? Send generic message -spl */
  if (found == 0) {
    send_to_char(g, ch, "Sorry, you may only reply to messages posted on a board.\r\n");
  }
}
