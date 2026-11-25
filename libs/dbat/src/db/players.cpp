/* ************************************************************************
*   File: players.c                                     Part of CircleMUD *
*  Usage: Player loading/saving and utility routines                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <array>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dbat/arrays.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/pfdefaults.hpp"
#include "dbat/feats.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/comm.hpp"
#include "dbat/genmob.hpp"
#include "dbat/class.hpp"
#include "dbat/config.hpp"
#include "dbat/fileop.hpp"
#include "dbat/affect.hpp"
#include "dbat/genolc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static constexpr auto pindex_bits_entries = std::to_array<std::string_view>({"DELETED", "NODELETE", "SELFDELETE", "NOWIZLIST"});
static_assert(std::size(pindex_bits_entries) == flag_count<pindex_flag>);
constinit const name_table pindex_bits(pindex_bits_entries);

bool pindex_flagged(const player_index_element &entry, pindex_flag flag)
{
  return entry.flags.test(flag);
}

void pindex_set_flag(player_index_element &entry, pindex_flag flag, bool on)
{
  entry.flags.set(flag, on);
}

/* The player index, plrfiles/index: a line for each player,
 *
 *   <id> <name> <level> <flags> <last logon> <admin level> <ship> <ship room> <time played>
 *
 * and "~". Lines starting with '~' aren't counted, but the first that many lines are read (quirk: a '~' line before
 * the end is read as a player, with the name of the one before, and the last player is left out); a line short of
 * its name or flags has the one before's (quirk). */
void build_player_index(GameWorld *g)
{
  io::loaded<io::text_reader> file = io::text_reader::open(str_format("{}{}", LIB_PLRFILES, INDEX_FILE));
  if (!file) {
    g->top_of_p_table = -1;
    mud_log("No player index file!  First new char will be IMP!");
    return;
  }

  std::vector<std::string_view> lines;
  int rec_count = 0;
  while (std::optional<std::string_view> line = file->line()) {
    lines.push_back(*line);
    if (!line->starts_with('~'))
      rec_count++;
  }

  if (rec_count == 0) {
    g->player_table   = nullptr;
    g->top_of_p_table = -1;
    return;
  }

  g->player_table = new player_index_element[rec_count]();
  std::string name, bits;
  int i;
  for (i = 0; i < rec_count; i++) {
    player_index_element &p = g->player_table[i];
    p.admlevel              = ADMLVL_NONE; /* In case they're not in the index yet */
    long long id = 0, last = 0, played = 0;
    io::fields f(lines[i]);
    if (f.number(id))
      p.id = static_cast<long>(id);
    f.word(name, 79) && f.number(p.level) && f.word(bits, 63) && f.number(last) && f.number(p.admlevel) &&
        f.number(p.ship) && f.number(p.shiproom) && f.number(played);
    p.last        = last;
    p.played      = played;
    p.name        = name;
    p.flags.set_word(0, io::ascii_flags(bits, 0, "asciiflag_conv"));
    g->top_idnum  = MAX(g->top_idnum, p.id);
  }
  g->top_of_p_file = g->top_of_p_table = i - 1;
}

/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(GameWorld *g, std::string_view name)
{
  int i, pos;

  if (g->top_of_p_table == -1) { /* no table */
    g->player_table = new player_index_element[1]();
    pos = g->top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(g, name)) == -1) { /* new name */
    i = ++g->top_of_p_table + 1;

    resize_array(g->player_table, i - 1, i);
    pos = g->top_of_p_table;
  }

  /* a reused slot still holds the old character's values */
  g->player_table[pos]      = player_index_element{};
  g->player_table[pos].name = to_lower(name);

  return (pos);
}

void save_player_index(GameWorld *g)
{
  io::text_writer out(str_format("{}{}", LIB_PLRFILES, INDEX_FILE), true);
  for (int i = 0; i <= g->top_of_p_table; i++) {
    const player_index_element &p = g->player_table[i];
    if (p.name.empty())
      continue;
    out.print("{:d} {} {:d} {} {:d} {:d} {:d} {:d} {:d}\n", p.id, p.name, p.level, flag_letters(p.flags.word(0)),
              static_cast<long long>(p.last), p.admlevel, p.ship, p.shiproom, static_cast<long long>(p.played));
  }
  out.write("~\n");
  out.commit();
}

void free_player_index(GameWorld *g)
{
  if (!g->player_table)
    return;

  delete[] g->player_table;
  g->player_table   = nullptr;
  g->top_of_p_table = 0;
}

long get_ptable_by_name(GameWorld *g, std::string_view name)
{
  int i;

  for (i = 0; i <= g->top_of_p_table; i++)
    if (str_eq_nocase(g->player_table[i].name, name))
      return (i);

  return (-1);
}

long get_id_by_name(GameWorld *g, std::string_view name)
{
  int i;

  for (i = 0; i <= g->top_of_p_table; i++)
    if (str_eq_nocase(g->player_table[i].name, name))
      return (g->player_table[i].id);

  return (-1);
}

const std::string *get_name_by_id(GameWorld *g, long id)
{
  for (int i = 0; i <= g->top_of_p_table; i++)
    if (g->player_table[i].id == id)
      return &g->player_table[i].name;
  return nullptr;
}

std::string_view player_name_or(GameWorld *g, long id, std::string_view otherwise)
{
  const std::string *name = get_name_by_id(g, id);
  return name ? std::string_view(*name) : otherwise;
}

/*************************************************************************
*  stuff related to the player file cleanup system			 *
*************************************************************************/

/*
 * remove_player() removes all files associated with a player who is
 * self-deleted, deleted by an immortal, or deleted by the auto-wipe
 * system (if enabled).
 */
void remove_player(GameWorld *g, int pfilepos)
{
  if (g->player_table[pfilepos].name.empty())
    return;

  /* every file the player has */
  for (int i = 0; i < MAX_FILES; i++)
    if (std::optional<std::string> path = player_path(i, g->player_table[pfilepos].name.c_str())) {
      std::error_code ec;
      std::filesystem::remove(*path, ec);
    }

  std::string last = asctime(localtime(&g->player_table[pfilepos].last));
  if (!last.empty() && last.back() == '\n')
    last.pop_back();
  mud_log("PCLEAN: {} Lev: {:d} Last: {}", g->player_table[pfilepos].name, g->player_table[pfilepos].level, last);
  g->player_table[pfilepos].name.clear();
  save_player_index(g);
}

void clean_pfiles(GameWorld *g)
{
  int i, ci;

  for (i = 0; i <= g->top_of_p_table; i++) {
    /*
     * We only want to go further if the player isn't protected
     * from deletion and hasn't already been deleted.
     */
    if (!g->player_table[i].flags.test(pindex_flag::nodelete) &&
        !g->player_table[i].name.empty()) {
      /*
       * If the player is already flagged for deletion, then go
       * ahead and get rid of him.
       */
      if (g->player_table[i].flags.test(pindex_flag::deleted)) {
        remove_player(g, i);
      } else {
        /*
         * Now we check to see if the player has overstayed his
         * welcome based on level.
         */
        for (ci = 0; pclean_criteria[ci].level > -1; ci++) {
          if (g->player_table[i].admlevel > 1)
            continue;
          else if (g->player_table[i].level <= pclean_criteria[ci].level && ((game_now(g) - g->player_table[i].last) >= (pclean_criteria[ci].days * SECS_PER_REAL_DAY))) {
            remove_player(g, i);
            break;
          }
        }
        /*
         * If we got this far and the players hasn't been kicked out,
         * then he can stay a little while longer.
         */
      }
    }
  }
  /*
   * After everything is done, we should rebuild player_index and
   * remove the entries of the players that were just deleted.
   */
}
