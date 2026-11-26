/*
 * Opens one of the OLC editors, as the first byte says, and types the rest at it, a line at a time: an implementor
 * in the fixture world answering the editor's menus with whatever the fuzzer likes. Whatever the editor does, the
 * world must stay consistent, during and after, and after leaving it.
 */
#include <array>
#include <cstring>

#include "dbat/comm.hpp"
#include "dbat/consts/admlevel.hpp"
#include "dbat/consts/constates.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/handler.hpp"
#include "dbat/relocate.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/world_audit.hpp"

#include "../harness.hpp"
#include "fuzz.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *, size_t);

constexpr int MAX_LINES      = 200; /* so that one input can't take the whole of a timeout */
constexpr size_t MAX_LINE    = 512;

/* What opens each editor, on something that exists and on something new */
static const auto OPENERS = std::to_array<const char *const>({
    "redit 300",    "redit 306",  "oedit 300",    "oedit 303",     "medit 300",  "medit 303",     "zedit",
    "zedit new 5 500 599",         "sedit 300",    "sedit 303",     "gedit 300",  "gedit 303",     "trigedit 300",
    "trigedit 302", "aedit nod",  "aedit newsocial", "hedit fixture", "hedit newhelp", "hsedit 300", "cedit",
    "assedit 300",  "iedit sword", "tedit motd",  "rpp 3",         "rpp 14 sword",
});

static void audit_problem(void *, const char *problem)
{
  file_print(stderr, "AUDIT: {}\n", problem);
  abort();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 1)
    return 0;
  fuzz_can_refuse = true;
  if (setjmp(fuzz_refused)) {
    th_abandon();
    fuzz_can_refuse = false;
    return 0;
  }
  GameWorld *g         = th_boot_at(1800000000);
  struct char_data *ch = th_player(g, "Fuzzy", ADMLVL_IMPL);
  char_set_rp(ch, 400);
  char_set_gold(ch, 100000);
  obj_to_char(g, read_object(g, 301), ch); /* the sword iedit and rpp 14 work on */
  th_type(g, ch, OPENERS[data[0] % std::size(OPENERS)]);

  size_t used = 1;
  std::array<char, MAX_LINE> line;
  for (int lines = 0; used < size && lines < MAX_LINES; lines++) {
    size_t length = 0;
    while (used < size && data[used] != '\n' && length < MAX_LINE - 1) {
      if (data[used])
        line[length++] = (char)data[used];
      used++;
    }
    used++;
    line[length] = '\0';
    th_type_at(g, char_get_desc(ch), line.data());
    th_take_output(g, ch);
    if (world_audit(g, audit_problem, nullptr))
      abort();
  }
  /* leaving whatever editor it's in, without saving, and the game carrying on after */
  for (const char *leave : {"/s", "q", "n", "q", "n", "0", "q", "n"})
    if (desc_get_connected(char_get_desc(ch)) != CON_PLAYING)
      th_type_at(g, char_get_desc(ch), leave);
  th_pulses(g, 20);
  if (world_audit(g, audit_problem, nullptr))
    abort();
  th_shutdown(g);
  fuzz_can_refuse = false;
  return 0;
}
