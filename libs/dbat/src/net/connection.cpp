/*************************************************************************
*   File: connection.cpp                                Part of CircleMUD *
*  Usage: Connections: the player limit, accepting, closing, idle checks *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"

#include "comm_internal.hpp"

int get_max_players(GameWorld *g)
{
  constexpr std::string_view how = "rlimit";
  int max_descs             = CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS;

  /* the most files the process may have open, raised as far as it goes */
  if (std::optional<long long> limit = net::raise_descriptor_limit())
    max_descs = (int)std::min<long long>(max_descs, *limit);

  /* now calculate max _players_ based on max descs */
  max_descs = std::min(CONFIG_MAX_PLAYING, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    mud_log("SYSERR: Non-positive max player limit!  (Set at {:d} using {}).",
            max_descs, how);
    exit(1);
  }
  mud_log("   Setting player limit to {:d} using {}.", max_descs, how);
  return (max_descs);
}

/* Empty the queues before closing connection */
static void flush_queues(GameWorld *g, struct descriptor_data *d)
{
  reset_output(g, d->out);
  flush_input_queue(d);
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */

/* Initialize a descriptor */
void init_descriptor(GameWorld *g, struct descriptor_data *newd, net::socket sock)
{

  newd->sock       = std::move(sock);
  newd->login_time = game_now(g);
  newd->has_prompt = 1; /* prompt is part of greetings */
  desc_set_connected(newd, CON_GET_USER);
  /* Numbers run from 1 to 999 and wrap, skipping any still in use. */
  do {
    if (++g->last_desc == 1000)
      g->last_desc = 1;
  } while (g->descriptors.find(g->last_desc) && g->descriptors.size() < 999);
  newd->desc_num = g->last_desc;
}

struct descriptor_data *add_descriptor(GameWorld *g, net::native_handle socket, std::string_view host)
{
  auto *d = new descriptor_data();
  d->host  = host.substr(0, HOST_LENGTH);
  init_descriptor(g, d, net::socket(socket));
  g->descriptors.add(d->desc_num, d);
  return d;
}

std::string_view desc_output(const struct descriptor_data *d) { return d->out.text; }

void desc_discard_output(GameWorld *g, struct descriptor_data *d) { reset_output(g, d->out); }

void desc_rewrite_output(struct descriptor_data *d, std::string_view text) { d->out.text = text; }

net::native_handle desc_socket(const struct descriptor_data *d) { return d->sock.handle(); }

/* What a connection holds before it has logged in to an account. */
void set_empty_account(struct descriptor_data *d)
{
  d->acct.user  = "Empty";
  d->acct.pass  = "Empty";
  d->acct.email = "Empty";
  d->acct.slots.fill("Empty");
}

static void set_color(GameWorld *g, struct descriptor_data *d)
{
  if (d->character == nullptr) {
    d->character = new char_data();
    clear_char(d->character);
    char_make_player_specials(d->character);
    char_set_desc(d->character, d);
  }
  char_prf_set(g, d->character, PRF_COLOR, true);
  write_to_output(g, d, g->GREETANSI);
  write_to_output(g, d, "\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
  write_to_output(g, d, "@D                 ---(@CPeak Logon Count Today@W: @w{:>4d}@D)---@n\r\n", g->PCOUNT);
  write_to_output(g, d, "@D                 ---(@CHighest Logon Count   @W: @w{:>4d}@D)---@n\r\n", g->HIGHPCOUNT);
  write_to_output(g, d, "@D                 ---(@CTotal Era {:d} Characters@W: @w{:>4}@D)---@n\r\n", CURRENT_ERA, add_commas(g->ERAPLAYERS));
  write_to_output(g, d, "\r\n@cEnter your desired username or the username you have already made.\n@CEnter Username:@n\r\n");
  set_empty_account(d);
  return;
}

int new_descriptor(GameWorld *g)
{
  std::optional<net::accepted> peer = net::accept_from(g->listener);
  if (!peer)
    return (-1);
  return game_add_connection(g, peer->sock.release(), peer->host);
}

/* Takes on a connected socket, unless the game is full or the site isn't welcome. */
int game_add_connection(GameWorld *g, net::native_handle desc, std::string_view host)
{
  net::socket sock(desc); /* closed on the way out, unless the connection takes it */

  /* make sure we have room for it */
  if ((int)count_members(each_desc(g)) >= CONFIG_MAX_PLAYING) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n");
    return (0);
  }

  std::string site(host.substr(0, HOST_LENGTH));

  /* one host may not take all the slots */
  int from_same_ip = 0;
  for (descriptor_data *other : each_desc(g))
    if (other->host == site)
      from_same_ip++;
  if (from_same_ip >= MAX_CONNECTIONS_PER_IP) {
    write_to_descriptor(desc, "Too many connections from your address. Please close some and try again.\r\n");
    sock.close();
    mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Connection from [{}] refused: already {:d} connections from it", site, from_same_ip);
    return (0);
  }

  /* determine if the site is banned */
  if (isbanned(g, site) == BAN_ALL) {
    sock.close();
    mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Connection attempt denied from [{}]", site);
    return (0);
  }

  auto *newd = new descriptor_data();
  newd->host = std::move(site);
  init_descriptor(g, newd, std::move(sock));
  g->descriptors.add(newd->desc_num, newd);

  set_color(g, newd);

  return (0);
}

void close_all_sockets(GameWorld *g)
{
  for (descriptor_data *d : each_desc(g))
    close_socket(g, d);
}

void close_socket(GameWorld *g, struct descriptor_data *d)
{
  g->descriptors.remove(d->desc_num, d);
  d->sock.close();
  flush_queues(g, d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = nullptr;

  if (d->snoop_by) {
    write_to_output(g, d->snoop_by, "Your victim is no longer among us.\r\n");
    d->snoop_by->snooping = nullptr;
  }

  if (d->character) {
    /* If we're switched, this resets the mobile taken. */
    char_set_desc(d->character, nullptr);

    /* Plug memory leak, from Eric Green. */
    if (!char_is_npc(d->character) && char_plr_flagged(d->character, PLR_MAILING) && d->str) {
      d->str = nullptr; /* the letter being written, in d->draft */
    }
    if (desc_is_playing(d) || desc_get_connected(d) == CON_DISCONNECT) {
      struct char_data *link_challenged = d->original ? d->original : d->character;

      /* We are guaranteed to have a person. */
      act(g, "$n has lost $s link.", true, link_challenged, nullptr, nullptr, TO_ROOM);
      save_char(g, link_challenged);
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, link_challenged)), true, "Closing link to: {}.", char_get_name(link_challenged));
    } else {
      free_char(g, d->character);
      d->character = nullptr; /* (free_char() can't clear it: ->desc was cut above) */
    }
  } else
    mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "Losing descriptor without char.");

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && char_get_desc(d->original))
    char_set_desc(d->original, nullptr);

  /* the account's strings go with d; only a real one (not set_color()'s "Empty") is worth a word */
  if (!d->acct.user.empty() && !str_eq_nocase(d->acct.user, "Empty"))
    mud_log("Freeing User: {}", d->acct.user);

  /*. Kill any OLC stuff (every editor, and character creation's stat points, keep theirs in d->olc) .*/
  leave_olc(g, d);

  delete d;
}

/* Connections idle in one of states: the first time they're found, they're given a tic's grace (and warning, if there
   is one); the next, they're closed */
static void time_out_idle(GameWorld *g, std::initializer_list<int> states, std::string_view warning)
{
  for (descriptor_data *d : each_desc(g)) {
    if (std::ranges::find(states, desc_get_connected(d)) == states.end())
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      if (!warning.empty())
        write_to_output(g, d, warning);
      continue;
    }
    write_to_output(g, d, "\r\nTimed out... goodbye.\r\n");
    desc_set_connected(d, CON_CLOSE);
  }
}

void check_idle_passwords(GameWorld *g) { time_out_idle(g, {CON_PASSWORD, CON_GET_EMAIL, CON_NEWPASSWD}, {}); }

void check_idle_menu(GameWorld *g)
{
  time_out_idle(g, {CON_MENU, CON_GET_USER, CON_UMENU},
                "\r\nYou are about to be disconnected due to inactivity in 60 seconds.\r\n");
}
