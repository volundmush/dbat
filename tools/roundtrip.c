/* Verification helper (not part of the server build).
 * Called from gdb after boot by verify.sh: loads every player and saves it back, then saves every
 * zone's mobiles, so the resulting data/ tree can be diffed between two builds. */
#include <stdio.h>
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/players.h"
#include "dbat/character_utils.h"
#include "dbat/genmob.h"

long roundtrip_dump(void)
{
  int players = 0, zones = 0;

  for (int i = 0; i <= top_of_p_table; i++) {
    struct char_data *pc;
    CREATE(pc, struct char_data, 1);
    clear_char(pc);
    CREATE(pc->player_specials, struct player_special_data, 1);
    int pos = load_char(player_table[i].name, pc);
    if (pos < 0)
      continue;
    char_set_pfilepos(pc, pos); /* save_char() silently skips characters without a player-file position */
    save_char(pc);
    players++;
  }
  for (zone_rnum z = 0; z <= top_of_zone_table; z++)
    if (save_mobiles(z))
      zones++;
  fprintf(stderr, "ROUNDTRIP saved %d players and mobs for %d zones\n", players, zones);
  return players;
}
