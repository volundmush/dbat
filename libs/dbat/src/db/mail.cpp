/* ************************************************************************
*   File: mail.c                                        Part of CircleMUD *
*  Usage: Internal funcs and player spec-procs of mud-mail system         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/******* MUD MAIL SYSTEM MAIN FILE ***************************************

Written by Jeremy Elson (jelson@circlemud.org)

*************************************************************************/
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/mail.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/players.hpp"
#include "dbat/fileop.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local globals */

/* local functions */
static void postmaster_send_mail(GameWorld *g, struct char_data *ch, struct char_data *mailman, int cmd, std::string_view arg);
static void postmaster_check_mail(GameWorld *g, struct char_data *ch, struct char_data *mailman, int cmd, std::string_view arg);
static void postmaster_receive_mail(GameWorld *g, struct char_data *ch, struct char_data *mailman, int cmd, std::string_view arg);
static bool mail_recip_ok(GameWorld *g, std::string_view name);

/* -------------------------------------------------------------------------- */

static bool mail_recip_ok(GameWorld *g, std::string_view name)
{
  int player_i, ret = false;

  if ((player_i = get_ptable_by_name(g, name)) >= 0) {
    if (!IS_SET(g->player_table[player_i].flags, PINDEX_DELETED))
      ret = true;
  }
  return ret;
}

/* Whether recipient has a letter waiting */
bool has_mail(GameWorld *g, long recipient)
{
  if (recipient < 0) {
    mud_log("SYSERR: Mail system -- non fatal error #1 (searchee == {:d}).", recipient);
    return false;
  }
  return std::ranges::any_of(g->mail, [&](const mail_letter &m) { return m.to == recipient; });
}

/* Writes the letters to the mail file, or disables mail if it can't */
static void commit_mail(GameWorld *g)
{
  if (!save_mail_file(g)) {
    mud_log("SYSERR: Mail system disabled! Unable to write mail file '{}'.", MAIL_FILE);
    g->no_mail = true;
  }
}

/*
 * void store_mail(long #1, long #2, std::string #3)
 * #1 - id number of the person to mail to.
 * #2 - id number of the person the mail is from.
 * #3 - The actual message to send.
 */
void store_mail(GameWorld *g, long to, long from, std::string message)
{
  if ((from < 0 && from != -1337) || to < 0 || message.empty()) {
    mud_log("SYSERR: Mail system -- non-fatal error #5. (from == {:d}, to == {:d})", from, to);
    return;
  }
  g->mail.push_back({.to = to, .from = from, .time = game_now(g), .text = std::move(message)});
  commit_mail(g);
}

/*
 * char *read_delete(long #1)
 * #1 - The id number of the person we're checking mail for.
 * Returns the message text of the mail received.
 *
 * Retrieves the first letter waiting for a player, which is then discarded.
 */
static std::string read_delete(GameWorld *g, long recipient)
{
  if (recipient < 0) {
    mud_log("SYSERR: Mail system -- non-fatal error #6. (recipient: {:d})", recipient);
    return {};
  }
  auto letter = std::ranges::find_if(g->mail, [&](const mail_letter &m) { return m.to == recipient; });
  if (letter == g->mail.end()) {
    mud_log("SYSERR: Mail system -- post office spec_proc error?  Error #7. (invalid character in index)");
    return {};
  }
  mail_letter m = std::move(*letter);
  g->mail.erase(letter);

  std::string_view stamp = asctime(localtime(&m.time));
  stamp.remove_suffix(1); /* its '\n' */

  /* a player's name capitalised, or what to call them if they're gone */
  auto name_of = [g](long id, std::string_view otherwise) {
    std::string text(player_name_or(g, id, otherwise));
    return CAP(text);
  };
  std::string from = m.from != -1337 ? name_of(m.from, "Unknown") : std::string("Auctioneer");
  std::string to   = name_of(recipient, "Unknown");

  std::string buf = str_format(" @D* * * * @CGalactic Mail System @D* * * *\r\n@cDate@D:@w {}\r\n  @cTo@D:@G {}\r\n@cFrom@D:@R {}\r\n\r\n@w{}@n",
                               stamp, to, from, m.text);
  commit_mail(g);
  return buf;
}

/****************************************************************
* Below is the spec_proc for a postmaster using the above       *
* routines.  Written by Jeremy Elson (jelson@circlemud.org) *
****************************************************************/

SPECIAL(postmaster)
{
  if (!char_get_desc(ch) || char_is_npc(ch))
    return (0); /* so mobs don't get caught here */

  if (!(CMD_IS(g, "mail") || CMD_IS(g, "check") || CMD_IS(g, "receive")))
    return (0);

  if (g->no_mail) {
    send_to_char(g, ch, "Sorry, the mail system is having technical difficulties.\r\n");
    return (0);
  }

  if (CMD_IS(g, "mail")) {
    postmaster_send_mail(g, ch, (struct char_data *)me, cmd, argument);
    return (1);
  }
  if (CMD_IS(g, "check")) {
    postmaster_check_mail(g, ch, (struct char_data *)me, cmd, argument);
    return (1);
  }
  if (CMD_IS(g, "receive")) {
    postmaster_receive_mail(g, ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else
    return (0);
}

static void postmaster_send_mail(GameWorld *g, struct char_data *ch, struct char_data *mailman,
                                 int cmd, std::string_view arg)
{
  long recipient;
  std::string buf;

  if (char_get_level(ch) < MIN_MAIL_LEVEL && char_get_admlevel(ch) < ADMLVL_IMMORT) {
    buf = str_format("$n tells you, 'Sorry, you have to be level {:d} to send mail!'", MIN_MAIL_LEVEL);
    act(g, buf, false, mailman, nullptr, ch, TO_VICT);
    return;
  }
  std::string name = take_arg(arg);

  if (name.empty()) { /* you'll get no argument from me! */
    act(g, "$n tells you, 'You need to specify an addressee!'",
        false, mailman, nullptr, ch, TO_VICT);
    return;
  }
  if (char_get_gold(ch) < STAMP_PRICE && !char_adm_flagged(ch, ADM_MONEY)) {
    buf = str_format("$n tells you, 'A stamp costs {:d} zenni.'\r\n$n tells you, '...which I see you can't afford.'",
                     STAMP_PRICE);
    act(g, buf, false, mailman, nullptr, ch, TO_VICT);
    return;
  }
  if ((recipient = get_id_by_name(g, name)) < 0 || !mail_recip_ok(g, name)) {
    act(g, "$n tells you, 'No one by that name is registered here!'",
        false, mailman, nullptr, ch, TO_VICT);
    return;
  }
  act(g, "$n starts to write some mail.", true, ch, nullptr, nullptr, TO_ROOM);
  buf = str_format("$n tells you, 'I'll take {:d} zenni for the stamp.'\r\n$n tells you, 'Write your message. (/s saves /h for help).'",
                   STAMP_PRICE);

  act(g, buf, false, mailman, nullptr, ch, TO_VICT);
  act(g, "@C$n@w starts writing a letter.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_gold(ch, char_get_gold(ch) - (STAMP_PRICE));
  char_plr_set(ch, PLR_MAILING, true); /* string_write() sets writing. */

  /* Start writing! */
  char_get_desc(ch)->draft.clear();
  string_write(char_get_desc(ch), &char_get_desc(ch)->draft, MAX_MAIL_SIZE, recipient);
}

static void postmaster_check_mail(GameWorld *g, struct char_data *ch, struct char_data *mailman,
                                  int cmd, std::string_view arg)
{
  if (has_mail(g, char_get_idnum(ch)))
    act(g, "$n tells you, 'You have mail waiting.'", false, mailman, nullptr, ch, TO_VICT);
  else
    act(g, "$n tells you, 'Sorry, you don't have any mail waiting.'", false, mailman, nullptr, ch, TO_VICT);
}

static void postmaster_receive_mail(GameWorld *g, struct char_data *ch, struct char_data *mailman,
                                    int cmd, std::string_view arg)
{
  std::string buf;
  struct obj_data *obj;

  if (!has_mail(g, char_get_idnum(ch)) && mailman != nullptr) {
    buf = "$n tells you, 'Sorry, you don't have any mail waiting.'";
    act(g, buf, false, mailman, nullptr, ch, TO_VICT);
    return;
  }
  while (has_mail(g, char_get_idnum(ch))) {
    obj              = create_obj(g);
    obj_set_item_number(obj, NOTHING);

    obj_set_type_flag(obj, ITEM_NOTE);
    obj_get_wear_flags(obj) = {};
    obj_wear_set(obj, ITEM_WEAR_TAKE, true);
    obj_wear_set(obj, ITEM_WEAR_HOLD, true);
    obj_set_weight(obj, 1);
    obj_set_cost(obj, 30);
    obj_set_cost_per_day(obj, 10);
    obj_set_action_description(obj, read_delete(g, char_get_idnum(ch)));
    std::string bla;
    std::string blm;
    bla = "@WA piece of mail@n";
    blm = "@WSomeone has left a piece of mail here@n";
    obj_set_short_description(obj, bla);
    obj_set_description(obj, blm);
    obj_set_name(obj, "mail paper letter");
    bla.clear();
    blm.clear();
    obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);
    add_unique_id(g, obj);
    obj_set_value(obj, 7, MATERIAL_PAPER);
    obj_set_value(obj, VAL_NOTE_HEALTH, 100);
    obj_set_value(obj, VAL_NOTE_MAXHEALTH, 100);

    if (obj_get_action_description(obj).empty())
      obj_set_action_description(obj, "Mail system error - please report.  Error #11.\r\n");

    /* so it saves */
    obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);

    if (desc_is_playing(char_get_desc(ch)) && mailman != nullptr) {
      obj_to_char(g, obj, ch);
      act(g, "$n gives you a piece of mail.", false, mailman, nullptr, ch, TO_VICT);
      act(g, "$N gives $n a piece of mail.", false, ch, nullptr, mailman, TO_ROOM);
    } else {
      extract_obj(g, obj);
    }
  }
}

void notify_if_playing(GameWorld *g, struct char_data *from, int recipient_id)
{

  for (descriptor_data *d : each_desc(g)) {
    if ((desc_is_playing(d)) && (char_get_idnum(d->character) == recipient_id) && (has_mail(g, char_get_idnum(d->character)))) {
      send_to_char(g, d->character, "\r\n\007\007\007@G@lYou have new mudmail from {}.@n\r\n", char_get_name(from));
    }
  }
}
