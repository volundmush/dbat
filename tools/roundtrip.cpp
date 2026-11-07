/* Verification helper (not part of the server build).
 * Called from gdb after boot by verify.sh: loads every player and saves it back, then saves every
 * zone's mobiles, so the resulting data/ tree can be diffed between two builds. */
#include <cstdio>

#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/players.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/genmob.hpp"
#include "dbat/game_world.hpp"

extern "C" long roundtrip_dump(GameWorld *g)
{
  int players = 0, zones = 0;

  for (int i = 0; i <= g->top_of_p_table; i++) {
    struct char_data *pc;
    pc = new char_data();
    clear_char(pc);
    pc->player_specials = new player_special_data();
    int pos = load_char(g, g->player_table[i].name, pc);
    if (pos < 0)
      continue;
    char_set_pfilepos(pc, pos); /* save_char() silently skips characters without a player-file position */
    save_char(g, pc);
    players++;
  }
  for (zone_rnum z = 0; z <= g->top_zone; z++)
    if (save_mobiles(g, z))
      zones++;
  fprintf(stderr, "ROUNDTRIP saved %d players and mobs for %d zones\n", players, zones);
  return players;
}
