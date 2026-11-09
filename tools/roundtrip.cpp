/* Verification helper (not part of the server build).
 * Called from gdb after boot by verify.sh: loads every player and saves it back (and its script variables), then saves every
 * zone's mobiles, objects, rooms, shops, guilds, triggers and zone file, so the resulting data/ tree can be diffed
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
#include "dbat/dg_olc.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/genolc.hpp"
#include "dbat/game_world.hpp"

extern "C" long roundtrip_dump(GameWorld *g)
{
  int players = 0, zones = 0;

  for (int i = 0; i <= g->top_of_p_table; i++) {
    struct char_data *pc;
    pc = new char_data();
    clear_char(pc);
    pc->player_specials.make();
    int pos = load_char(g, g->player_table[i].name, pc);
    if (pos < 0)
      continue;
    char_set_pfilepos(pc, pos); /* save_char() silently skips characters without a player-file position */
    save_char(g, pc);
    read_saved_vars(pc); /* and their script variables, which are kept in a file of their own */
    save_char_vars(g, pc);
    players++;
  }
  for (struct zone_data *z : g->zones.all()) {
    bool ok = save_mobiles(g, z);
    ok      = save_objects(g, z) && ok;
    ok      = save_rooms(g, z) && ok;
    ok      = save_shops(g, z) && ok;
    ok      = save_guilds(g, z) && ok;
    if (!g->trig_index.vnums_in(genolc_zone_bottom(g, z), z->top).empty()) /* as on disk: no file for a zone without */
      ok = save_triggers(g, z) && ok;
    ok      = save_zone(g, z) && ok;
    zones += ok;
  }
  fprintf(stderr, "ROUNDTRIP saved %d players, and the world files of %d zones\n", players, zones);
  return players;
}
