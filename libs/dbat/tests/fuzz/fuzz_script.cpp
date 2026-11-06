/*
 * Runs a DG script on a mob, an object or a room, as the first byte says. The rest is the script, which
 * goes in through the game's own trigger file, so that it's parsed and attached as any other would be.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../harness.h"
#include "fuzz.h"
#include "dbat/db.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/game_world.h"
#include "dbat/handler.h"
#include "dbat/relocate.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/world_audit.h"

static const uint8_t *script;
static size_t script_size;
static int attach_type;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *, size_t);

/* A trigger ends at a line with ~ on it, so the script can't run on into whatever comes next in the file. */
static void add_trigger_to_file(void)
{
  FILE *fl = fopen("world/trg/3.trg", "r+");
  long end;

  if (!fl)
    abort();
  fseek(fl, 0, SEEK_END);
  end = ftell(fl);
  /* over the $~ at the end of the file */
  fseek(fl, end - 3, SEEK_SET);
  fprintf(fl, "#399\nFuzzed~\n%d c %d\nfuzz~\n", attach_type, attach_type == 1 ? 2 : 100);
  for (size_t i = 0; i < script_size; i++)
    if (script[i] && script[i] != '~')
      putc(script[i], fl);
  fprintf(fl, "\n~\n$~\n");
  fclose(fl);
}

static void audit_problem(void *, const char *problem)
{
  fprintf(stderr, "AUDIT: %s\n", problem);
  abort();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  GameWorld *g;
  struct char_data *ch;
  trig_rnum number;

  if (size < 1)
    return 0;
  attach_type = data[0] % 3;
  script      = data + 1;
  script_size = size - 1;
  fuzz_can_refuse = true;
  if (setjmp(fuzz_refused)) {
    th_abandon();
    fuzz_can_refuse = false;
    return 0;
  }
  th_before_boot(add_trigger_to_file);
  g = th_boot_at(1800000000);
  ch = th_player(g, "Fuzzy", ADMLVL_NONE);
  if ((number = real_trigger(g, 399)) != NOTHING) {
    if (attach_type == 0) {
      struct char_data *mob = read_mobile(g, 300, VIRTUAL);

      char_to_room(g, mob, char_get_in_room(ch));
      add_trigger(g, dg_ensure_script(char_script_slot(mob)), read_trigger(g, number), -1);
    } else if (attach_type == 1) {
      struct obj_data *obj = read_object(g, 350, VIRTUAL);

      obj_to_char(g, obj, ch);
      add_trigger(g, dg_ensure_script(obj_script_slot(obj)), read_trigger(g, number), -1);
    } else {
      add_trigger(g, dg_ensure_script(room_script_slot(&g->rooms[char_get_in_room(ch)])), read_trigger(g, number), -1);
    }
    th_type(g, ch, "fuzz");
    if (world_audit(g, audit_problem, nullptr))
      abort();
    th_pulses(g, 20); /* for anything that's waiting */
    if (world_audit(g, audit_problem, nullptr))
      abort();
  }
  th_shutdown(g);
  fuzz_can_refuse = false;
  return 0;
}
