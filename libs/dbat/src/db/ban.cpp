/* ************************************************************************
*   File: ban.c                                         Part of CircleMUD *
*  Usage: banning/unbanning/checking sites and player names               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cerrno>
#include <cstdlib>
#include <string>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/ban.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/fileop.hpp"
#include "dbat/log.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"

/* local globals */

/* local functions */

static const char *const ban_types[] = {
    "no",
    "new",
    "select",
    "all",
    "ERROR"};

void free_ban_list(GameWorld *g)
{
  while (g->ban_list) {
    struct ban_list_element *next = g->ban_list->next;
    delete g->ban_list;
    g->ban_list = next;
  }
}

int isbanned(GameWorld *g, std::string_view hostname)
{
  int i = 0;

  if (hostname.empty())
    return (0);

  std::string host(hostname);
  for (char &c : host)
    c = tolower(c);

  for (struct ban_list_element *banned_node = g->ban_list; banned_node; banned_node = banned_node->next)
    if (host.find(banned_node->site) != std::string::npos) /* if hostname is a substring */
      i = MAX(i, banned_node->type);

  return (i);
}

#define BAN_LIST_FORMAT "{:<40.40}  {:<8.8}  {:<10.10}  {:<16.16}\r\n"
ACMD(do_ban)
{
  char *nextchar;
  std::string flag, site;
  char timestr[16];
  int i;
  struct ban_list_element *ban_node;

  if (argument.empty()) {
    if (!g->ban_list) {
      send_to_char(g, ch, "No sites are banned.\r\n");
      return;
    }
    send_to_char(g, ch, BAN_LIST_FORMAT,
                 "Banned Site Name",
                 "Ban Type",
                 "Banned On",
                 "Banned By");
    send_to_char(g, ch, BAN_LIST_FORMAT,
                 "---------------------------------",
                 "---------------------------------",
                 "---------------------------------",
                 "---------------------------------");

    for (ban_node = g->ban_list; ban_node; ban_node = ban_node->next) {
      if (ban_node->date) {
        copy_cstr(timestr, asctime(localtime(&(ban_node->date))), 10);
        timestr[10] = '\0';
      } else
        strcpy(timestr, "Unknown"); /* strcpy: OK (strlen("Unknown") < 16) */

      send_to_char(g, ch, BAN_LIST_FORMAT, ban_node->site, ban_types[ban_node->type], timestr, ban_node->name);
    }
    return;
  }

  std::string_view rest = argument;
  flag                  = take_arg(rest);
  site                  = take_arg(rest);
  if (site.empty() || flag.empty()) {
    send_to_char(g, ch, "Usage: ban {all | select | new} site_name\r\n");
    return;
  }
  if (!(str_eq_nocase(flag, "select") || str_eq_nocase(flag, "all") || str_eq_nocase(flag, "new"))) {
    send_to_char(g, ch, "Flag must be ALL, SELECT, or NEW.\r\n");
    return;
  }
  for (ban_node = g->ban_list; ban_node; ban_node = ban_node->next) {
    if (str_eq_nocase(ban_node->site, site.c_str())) {
      send_to_char(g, ch, "That site has already been banned -- unban it to change the ban type.\r\n");
      return;
    }
  }

  ban_node = new ban_list_element();
  copy_cstr(ban_node->site, site.c_str(), sizeof(ban_node->site));
  for (nextchar = ban_node->site; *nextchar; nextchar++)
    *nextchar = tolower(*nextchar);
  copy_cstr(ban_node->name, char_get_name(ch), sizeof(ban_node->name));
  ban_node->date = game_now(g);

  for (i = BAN_NEW; i <= BAN_ALL; i++)
    if (str_eq_nocase(flag, ban_types[i]))
      ban_node->type = i;

  ban_node->next = g->ban_list;
  g->ban_list       = ban_node;

  mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "{} has banned {} for {} players.",
         char_get_name(ch), site, ban_types[ban_node->type]);
  send_to_char(g, ch, "Site banned.\r\n");
  save_ban_list(g);
}
#undef BAN_LIST_FORMAT

ACMD(do_unban)
{
  std::string site;
  struct ban_list_element *ban_node, *temp;
  int found = 0;

  std::string_view rest = argument;
  site                  = take_arg(rest);
  if (site.empty()) {
    send_to_char(g, ch, "A site to unban might help.\r\n");
    return;
  }
  ban_node = g->ban_list;
  while (ban_node && !found) {
    if (str_eq_nocase(ban_node->site, site.c_str()))
      found = 1;
    else
      ban_node = ban_node->next;
  }

  if (!found) {
    send_to_char(g, ch, "That site is not currently banned.\r\n");
    return;
  }
  REMOVE_FROM_LIST(ban_node, g->ban_list, next, temp);
  send_to_char(g, ch, "Site unbanned.\r\n");
  mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "{} removed the {}-player ban on {}.",
         char_get_name(ch), ban_types[ban_node->type], ban_node->site);

  delete ban_node;
  save_ban_list(g);
}

/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/



bool Valid_Name(GameWorld *g, char *newname)
{
  int i, wovels = 0;
  char tempname[MAX_INPUT_LENGTH];

  /*
   * Make sure someone isn't trying to create this same name.  We want to
   * do a 'strcasecmp' so people can't do 'Bob' and 'BoB'.  The creating login
   * will not have a character name yet and other people sitting at the
   * prompt won't have characters yet.
   *
   * New, unindexed characters (i.e., characters who are in the process of creating)
   * will have an idnum of -1, set by clear_char() in db.c.  If someone is creating a
   *character by the same name as the one we are checking, then the name is invalid,
   * to prevent character duping.
   * THIS SHOULD FIX THE 'invalid name' if disconnected from OLC-bug - WELCOR 9/00
   */
  for (descriptor_data *dt : each_desc(g))
    if (dt->character && char_get_name(dt->character) && str_eq_nocase(char_get_name(dt->character), newname))
      if (char_get_idnum(dt->character) == -1)
        return (desc_is_playing(dt));

  /* count wovels */
  for (i = 0; newname[i]; i++) {
    if (strchr("aeiouyAEIOUY", newname[i]))
      wovels++;
  }

  /* return invalid if no wovels */
  if (!wovels)
    return false;

  /* return valid if list doesn't exist */
  if (g->invalid_list.empty())
    return true;

  /* change to lowercase */
  copy_cstr(tempname, newname, sizeof(tempname));
  for (i = 0; tempname[i]; i++)
    tempname[i] = tolower(tempname[i]);

  /* Does the desired name contain a string in the invalid list? */
  return std::ranges::none_of(g->invalid_list, [&](const std::string &part) { return std::string_view(tempname).contains(part); });
}

/* What's with the wacky capitalization in here? */
void Free_Invalid_List(GameWorld *g)
{
  g->invalid_list.clear();
}

