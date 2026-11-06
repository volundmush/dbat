/*
 * Boots a game with one of its files damaged: the first byte picks which, and the rest is its contents.
 * The game may refuse to boot, but if it does boot then its world has to make sense.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../harness.h"
#include "fuzz.h"
#include "dbat/game_world.h"
#include "dbat/world_audit.h"

static const char *const files[] = {
    "world/wld/3.wld", "world/mob/3.mob", "world/obj/3.obj", "world/zon/3.zon", "world/trg/3.trg",
    "world/shp/3.shp", "world/gld/3.gld", "world/wld/index", "world/mob/index", "world/obj/index",
    "world/zon/index", "world/trg/index", "world/shp/index", "world/gld/index", "etc/config",
    "etc/assemblies", "etc/badsites", "etc/hcontrol", "etc/levels", "etc/time",
    "etc/auction", "etc/boards", "etc/plrmail", "plrfiles/index", "misc/socials",
    "misc/socials.new", "misc/messages", "text/help/help.hlp", "text/help/index", "disabled.cmds",
};
constexpr size_t NUM_FILES = sizeof(files) / sizeof(*files);

static const uint8_t *file_data;
static size_t file_size;
static const char *file_path;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *, size_t);

static void damage_file(void)
{
  fuzz_write_file(file_path, file_data, file_size);
}

static void audit_problem(void *, const char *problem)
{
  fprintf(stderr, "AUDIT: %s\n", problem);
  abort();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  static GameWorld *g;

  if (size < 1)
    return 0;
  file_path = files[data[0] % NUM_FILES];
  file_data = data + 1;
  file_size = size - 1;
  fuzz_can_refuse = true;
  if (!setjmp(fuzz_refused)) {
    th_before_boot(damage_file);
    g = th_boot_at(1800000000);
    fuzz_can_refuse = false;
    if (world_audit(g, audit_problem, nullptr))
      abort();
    th_pulses(g, 5);
    th_shutdown(g);
  } else {
    th_abandon();
  }
  fuzz_can_refuse = false;
  return 0;
}
