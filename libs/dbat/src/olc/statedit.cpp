/************************************************************************
 *  --Statedit  Part of UrathMud                                v1.0    *
 *  Copyright 1999 Karl N. Matthias.  All rights Reserved.              *
 *  You may freely distribute, modify, or sell this code                *
 *  as long as this copyright remains intact.                           *
 *                                                                      *
 *  Based on code by Jeremy Elson, Harvey Gilpin, and George Greer.     *
 ************************************************************************/

/* --relistan 2/22/99 - 2/24/99: player configurable stats, in character creation */
#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/statedit.hpp"
#include "dbat/utils.hpp"

namespace {
  /* A base stat: the menu's letter for it, and how it's read and set */
  struct stat_row {
    char key;
    int8_t (*get)(const struct char_data *ch);
    void (*set)(struct char_data *ch, int8_t value);
  };

  constexpr stat_row STATS[] = {
      {'s', char_get_real_abils_str,   char_set_real_abils_str  },
      {'i', char_get_real_abils_intel, char_set_real_abils_intel},
      {'w', char_get_real_abils_wis,   char_set_real_abils_wis  },
      {'d', char_get_real_abils_dex,   char_set_real_abils_dex  },
      {'n', char_get_real_abils_con,   char_set_real_abils_con  },
      {'c', char_get_real_abils_cha,   char_set_real_abils_cha  },
  };

  /* Character creation's stat points: a pool the new character's base stats are bought from */
  struct stat_points final : olc_session {
    int points{};                     /* left in the pool */
    const stat_row *asking = nullptr; /* the stat a value is being asked for, or none for the menu */

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override { parse_stats(g, d, line); }
    /* only on a disconnection: creation ends the points itself */
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override
    {
      log_stops_editing(g, d, NOWHERE);
    }

    void menu(GameWorld *g, struct descriptor_data *d);
    int assign(GameWorld *g, int stat, std::string_view line);
  };

  void stat_points::menu(GameWorld *g, struct descriptor_data *d)
  {
    struct char_data *ch = d->character;
    send_to_char(g, ch,
                 "\r\n@W-<[@y==========@B[ @YCWG @B]@y==========@W]>-\r\n <| Total Points Left: @m{:>3d}@W    |>     You should select the letter of the score you\r\n <|                           |>     wish to adjust.  When prompted, enter the new score,\r\n <| = Select a stat:          |>     NOT the amount to add.  NOTE: If you quit before you\r\n <| @BS@W) @rStrength     : @m{:>3d}@W     |>     assign all the points, you will lose them forever.\r\n <| @BD@W) @rDexterity    : @m{:>3d}@W     |>     If your points are at zero, you may still reassign\r\n <| @BN@W) @rConstitution : @m{:>3d}@W     |>     points by lowering any statistic, then add those\r\n <| @BI@W) @rIntelligence : @m{:>3d}@W     |>     points to the statistic of your choice.\r\n <| @BW@W) @rWisdom       : @m{:>3d}@W     |>\r\n <| @BC@W) @rCharisma     : @m{:>3d}@W     |>\r\n <| @BQ@W) @CQuit@W                   |>\r\n-<[@y===========================@W]>-@n\r\n\r\n",
                 points, char_get_real_abils_str(ch), char_get_real_abils_dex(ch), char_get_real_abils_con(ch), char_get_real_abils_intel(ch),
                 char_get_real_abils_wis(ch), char_get_real_abils_cha(ch));
    send_to_char(g, ch, "Enter Letter to Change: ");
    asking = nullptr;
  }

  /* The new value of a stat, from what's typed: its old value goes back into the pool, and the new one comes out */
  int stat_points::assign(GameWorld *g, int stat, std::string_view line)
  {
    if (stat > 0)
      points += stat;

    int wanted = atoi(std::string(line).c_str());
    int value  = wanted > points ? points : wanted;
    if (value > 100)
      value = points < 100 ? points : 100;
    if (value < 3)
      value = 3;
    /* This should throw an error! */
    if (points <= 0) {
      value  = 0;
      points = 0;
      mudlog(g, LOG_NRM, ADMLVL_IMMORT, true, "Stat total below 0: possible code error");
    }
    points -= value;
    return value;
  }
} // namespace

void start_stat_points(GameWorld *g, struct descriptor_data *d, int points)
{
  if (!d->olc)
    d->olc = std::make_unique<stat_points>();
  session_of<stat_points>(d).points = points;
}

void stats_disp_menu(GameWorld *g, struct descriptor_data *d) { session_of<stat_points>(d).menu(g, d); }

bool parse_stats(GameWorld *g, struct descriptor_data *d, std::string_view line)
{
  stat_points &pool = session_of<stat_points>(d);

  if (pool.asking) {
    const stat_row &stat = *pool.asking;
    stat.set(d->character, pool.assign(g, stat.get(d->character), line));
    pool.menu(g, d);
    return false;
  }

  const char choice = tolower(first_char(line));
  if (choice == 'q')
    return true;
  if (auto stat = std::ranges::find(STATS, choice, &stat_row::key); choice && stat != std::end(STATS)) {
    pool.asking = stat;
    send_to_char(g, d->character, "Enter new value: ");
  } else
    pool.menu(g, d);
  return false;
}
