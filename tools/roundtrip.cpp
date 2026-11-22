/* Verification helper (not part of the server build).
 * Called from gdb after boot by verify.sh: loads every player and saves it back (with its script variables, aliases
 * and objects), then every account, then every zone's mobiles, objects, rooms, shops, guilds, triggers and zone file,
 * and last the files of etc/ the game writes (config, time, auction, assemblies, the player index, boards and houses),
 * so the resulting data/ tree can be diffed between two builds. The clock is pinned, as saves stamp the time. */
#include <cstdio>
#include <filesystem>

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
#include "dbat/alias.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/boards.hpp"
#include "dbat/cedit.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/house.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/objsave.hpp"
#include "dbat/game_world.hpp"

extern "C" long roundtrip_dump(GameWorld *g)
{
  int players = 0, zones = 0, accounts = 0;

  g->pinned_time = 1800000000;

  for (int i = 0; i <= g->top_of_p_table; i++) {
    struct char_data *pc;
    pc = new char_data();
    clear_char(pc);
    pc->player_specials.make();
    int pos = load_char(g, g->player_table[i].name.c_str(), pc);
    if (pos < 0)
      continue;
    char_set_pfilepos(pc, pos); /* save_char() silently skips characters without a player-file position */
    save_char(g, pc);
    read_saved_vars(pc); /* and their script variables, which are kept in a file of their own */
    save_char_vars(g, pc);
    read_aliases(g, pc);
    write_aliases(g, pc);
    if (Crash_load(g, pc) >= 0) /* and what they carry, in a file of its own */
      Crash_crashsave(g, pc);
    players++;
  }
  /* every account, by its file's name */
  std::error_code ec;
  for (const auto &entry : std::filesystem::recursive_directory_iterator("user", ec))
    if (entry.is_regular_file() && entry.path().extension() == ".usr") {
      auto *d = new descriptor_data();
      std::string name = entry.path().stem().string();
      userLoad(d, name.c_str());
      save_account(d);
      accounts++;
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
  save_config(g, nullptr);
  save_mud_time(g, &g->time_info);
  auc_save(g);
  assemblySaveAssemblies(g);
  save_player_index(g);
  for (struct board_info *b = g->bboards; b; b = b->next)
    save_board(g, b);
  House_save_all(g);
  House_save_control(g);
  fprintf(stderr, "ROUNDTRIP saved %d players, %d accounts, and the world files of %d zones\n", players, accounts, zones);
  return players;
}
