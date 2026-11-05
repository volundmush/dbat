/*
 * Reads a damaged save file for one player, in one game that keeps running: the first byte picks the
 * kind of file, and the rest is its contents. Everything the player loaded goes before the next.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../harness.h"
#include "fuzz.h"
#include "dbat/alias.h"
#include "dbat/db.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/game_world.h"
#include "dbat/objsave.h"
#include "dbat/utils.h"
#include "dbat/character_utils.h"
#include "dbat/world_audit.h"

static GameWorld *g;

int LLVMFuzzerInitialize(int *, char ***);
int LLVMFuzzerTestOneInput(const uint8_t *, size_t);

int LLVMFuzzerInitialize(int *, char ***)
{
  struct char_data *ch;

  g = th_boot_at(1800000000);
  ch = th_player(g, "Fuzzy", ADMLVL_NONE);
  save_char(g, ch);
  return 0;
}

static void audit_problem(void *, const char *problem)
{
  fprintf(stderr, "AUDIT: %s\n", problem);
  abort();
}

static void read_player_file(const uint8_t *data, size_t size)
{
  struct char_data *pc;

  fuzz_write_file("plrfiles/F-J/fuzzy.plr", data, size);
  CREATE(pc, struct char_data, 1);
  clear_char(pc);
  CREATE(*char_player_specials_slot(pc), struct player_special_data, 1);
  load_char(g, "fuzzy", pc);
  free_char(g, pc);
}

static void read_file_in_game(int kind, const uint8_t *data, size_t size)
{
  static const char *const paths[] = {"plrobjs/F-J/fuzzy.new", "plralias/F-J/fuzzy.alias", "plrvars/F-J/fuzzy.mem"};
  struct char_data *ch = g->descriptors ? g->descriptors->character : nullptr;

  if (!ch)
    abort();
  fuzz_write_file(paths[kind], data, size);
  switch (kind) {
    case 0:
      Crash_load(g, ch);
      break;
    case 1:
      read_aliases(g, ch);
      break;
    default:
      read_saved_vars(ch);
      break;
  }
  while (char_get_inventory(ch))
    extract_obj(g, char_get_inventory(ch));
  for (int i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      extract_obj(g, char_get_equipment(ch, i));
  if (world_audit(g, audit_problem, nullptr))
    abort();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 1)
    return 0;
  fuzz_can_refuse = true;
  if (!setjmp(fuzz_refused)) {
    if (data[0] % 4 == 0)
      read_player_file(data + 1, size - 1);
    else
      read_file_in_game(data[0] % 4 - 1, data + 1, size - 1);
  }
  fuzz_can_refuse = false;
  return 0;
}
