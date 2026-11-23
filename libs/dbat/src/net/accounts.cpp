/* ************************************************************************
*   File: accounts.c                                    Part of CircleMUD *
*  Usage: user account files                                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

#include "dbat/interpreter.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fileop.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/players.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "nanny_internal.hpp"

/* Reads name's account over a, saying so in the log if its file can't be read. */
static bool load_account(std::string_view name, account &a)
{
  auto path = player_path(USER_FILE, name);

  if (!path)
    return false;
  if (!read_account_file(*path, a)) {
    mud_log("ERROR: could not load user, {}, from filename, {}.", name, *path);
    return false;
  }
  return true;
}

/* Empties every slot holding name (used when a character is deleted or missing). */
void empty_account_slots_named(struct descriptor_data *d, std::string_view name)
{
  for (auto &slot : d->acct.slots)
    if (str_eq_nocase(name, slot))
      slot = "Empty";
}

void userLoad(struct descriptor_data *d, std::string_view name)
{
  load_account(name, d->acct);
}

/* Writes a new account's file: three slots, all empty, and no RPP. */
void userCreate(struct descriptor_data *d)
{
  account made{.user       = d->acct.user,
               .email      = d->acct.email,
               .pass       = d->acct.pass,
               .total      = 3,
               .rpp        = 0,
               .slots      = {"Empty", "Empty", "Empty", "Empty", "Empty"},
               .level      = 0,
               .customfile = 1,
               .rbank      = 0};

  CAP(made.user);
  if (!write_account(d->acct.user.c_str(), made))
    return;
  /* quirk: the level and customs written aren't taken; customCreate() then makes the customs file */
  d->acct.user  = made.user;
  d->acct.total = made.total;
  d->acct.rpp   = made.rpp;
  d->acct.rbank = made.rbank;
  d->acct.slots = made.slots;
  customCreate(d);
}

/* Delete A User Account */
void userDelete(GameWorld *g, struct descriptor_data *d)
{
  int player_i;
  auto path = player_path(USER_FILE, d->acct.user.c_str());

  if (!path) {
    write_to_output(g, d, "Error. Your user file doesn't even exist!\n");
    return;
  }
  for (const auto &slot : d->acct.slots)
    if (!str_eq_nocase(slot, "Empty") && (player_i = get_ptable_by_name(g, slot)) >= 0)
      remove_player(g, player_i);
  std::error_code ec;
  std::filesystem::remove(*path, ec);
}

/* Saves the account d is logged in to, the name capitalised. */
void save_account(struct descriptor_data *d)
{
  if (!d || d->acct.user.empty())
    return;

  account saved = d->acct;
  CAP(saved.user);
  for (auto &slot : saved.slots)
    if (slot.empty())
      slot = "Empty";
  if (write_account(d->acct.user.c_str(), saved))
    d->acct.user = saved.user;
}

/* Adds amt RPP to the file of an account no one is logged in to, unless it would go below 0. */
void reward_offline_user(GameWorld *g, const std::string &name, int amt)
{
  account a;

  if (!load_account(name, a))
    return;
  bool canceled = amt != 0 && a.rpp + amt < 0;
  if (!canceled)
    a.rpp += amt;
  if (write_account(name.c_str(), a) && canceled)
    send_to_imm(g, "RPP would be below 0, reward canceled.");
}

void fingerUser(GameWorld *g, struct char_data *ch, const std::string &name)
{
  account a;
  auto path = player_path(USER_FILE, name.c_str());

  if (!path) {
    send_to_char(g, ch, "That user doesn't exist.\r\n");
    return;
  }
  if (!read_account_file(*path, a)) {
    send_to_char(g, ch, "That user is bugged! Report to Iovan.\r\n");
    return;
  }

  send_to_char(g, ch, "@D[@gUsername   @D: @w{:<30}@D]@n\r\n", a.user);
  send_to_char(g, ch, "@D[@gEmail      @D: @w{:<30}@D]@n\r\n", a.email);
  if (char_get_admlevel(ch) > 4) {
    send_to_char(g, ch, "@D[@gPass       @D: @w{:<30}@D]@n\r\n", a.pass);
  } else if (char_get_admlevel(ch) > 0) {
    send_to_char(g, ch, "@D[@gPass       @D: @w{:<30}@D]@n\r\n", "??????????");
  }
  send_to_char(g, ch, "@D[@gTotal Slots@D: @w{:<30d}@D]@n\r\n", a.total);
  send_to_char(g, ch, "@D[@gRP Points  @D: @w{:<30d}@D]@n\r\n", a.rpp);
  if (char_get_admlevel(ch) > 0) {
    send_to_char(g, ch, "@D[@gRP Bank    @D: @w{:<30d}@D]@n (unused!)\r\n", a.rbank);
    for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++)
      send_to_char(g, ch, "@D[@gCh. Slot {:d} @D: @w{:<30}@D]@n\r\n", n, a.slots[n - 1]);
    send_to_char(g, ch, "\n");
    customRead(g, char_get_desc(ch), 1, name.c_str());
  }
}

void userRead(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "                 @RUser Menu@n\n");
  write_to_output(g, d, "@D=============================================@n\r\n");
  write_to_output(g, d, "@D|@gUser Account  @D: @w{:<27}@D|@n\n", d->acct.user);
  write_to_output(g, d, "@D|@gEmail Address @D: @w{:<27}@D|@n\n", d->acct.email);
  write_to_output(g, d, "@D|@gMax Characters@D: @w{:<27d}@D|@n\n", d->acct.total);
  write_to_output(g, d, "@D|@gRP Points     @D: @w{:<27d}@D|@n\n", d->acct.rpp);
  write_to_output(g, d, "@D=============================================@n\r\n\r\n");
  write_to_output(g, d, "      @D[@y----@YSelect A Character Slot@y----@D]@n\n");
  for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++)
    if (n <= 3 || d->acct.total >= n)
      write_to_output(g, d, "                @B(@W{:d}@B) @C{}@n\n", n, d->acct.slots[n - 1]);
  write_to_output(g, d, "\n");
  write_to_output(g, d, "      @D[@y---- @YSelect Another Option @y----@D]@n\n");
  write_to_output(g, d, "                @B(@WB@B) @CBuy New C. Slot @D(@R15 RPP@D)@n\n");
  write_to_output(g, d, "                @B(@WC@B) @CUser's Customs\n");
  write_to_output(g, d, "                @B(@WD@B) @RDelete User@n\n");
  write_to_output(g, d, "                @B(@WE@B) @CEmail@n\n");
  write_to_output(g, d, "                @B(@WP@B) @CNew Password@n\n");
  write_to_output(g, d, "                @B(@WQ@B) @CQuit@n\n");
  write_to_output(g, d, "\r\nMake your choice: \n");
}
