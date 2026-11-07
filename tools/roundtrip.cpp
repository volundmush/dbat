/* Verification helper (not part of the server build).
 * Called from gdb after boot by verify.sh: loads every player and saves it back, then saves every
 * zone's mobiles, objects, rooms, shops, guilds and zone file, so the resulting data/ tree can be diffed
 * between two builds. */
#include <cstdio>

#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/players.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/genmob.hpp"
#include "dbat/gengld.hpp"
#include "dbat/genobj.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genwld.hpp"
#include "dbat/genzon.hpp"
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
  for (struct zone_data *z : g->zones.all()) {
    bool ok = save_mobiles(g, z);
    ok      = save_objects(g, z) && ok;
    ok      = save_rooms(g, z) && ok;
    ok      = save_shops(g, z) && ok;
    ok      = save_guilds(g, z) && ok;
    ok      = save_zone(g, z) && ok;
    zones += ok;
  }
  fprintf(stderr, "ROUNDTRIP saved %d players, and the world files of %d zones\n", players, zones);
  return players;
}
