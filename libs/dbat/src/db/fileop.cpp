#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

#include "dbat/fileop.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/log.hpp"
#include "dbat/comm.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

std::optional<std::string> player_path(int mode, std::string_view name)
{
  if (name.empty()) {
    static const char buffer = 0; /* stands for the buffer get_filename() was given, which the log line names */
    mud_log("SYSERR: NULL pointer or empty string passed to get_filename(), {} or {}.", as_pointer(name.data()), /* (nil) only for a null view */
            as_pointer(&buffer));
    return std::nullopt;
  }
  return io::player_file(mode, name);
}

namespace {
  /* The toplist's sections, five places each, in the order g->topname and g->toppoint hold them */
  struct toplist_section {
    std::string_view label{};
    int64_t (*value)(GameWorld *g, char_data *ch);
    bool by_account; /* listed by the account's name, not the character's */
  };
  const auto toplist_sections = std::to_array<toplist_section>({
    {"powerlevel", [](GameWorld *g, char_data *ch) -> int64_t { return getEffMaxPL(g, ch); }, false},
    {"ki", [](GameWorld *g, char_data *ch) -> int64_t { return getMaxKI(g, ch); }, false},
    {"stamina", [](GameWorld *g, char_data *ch) -> int64_t { return getMaxST(g, ch); }, false},
    {"zenni", [](GameWorld *, char_data *ch) -> int64_t { return char_get_bank_gold(ch) + char_get_gold(ch); }, false},
    {"RPP", [](GameWorld *, char_data *ch) -> int64_t { return char_get_trp(ch); }, true},
  });
  constexpr int TOPLIST_PLACES = 5 * static_cast<int>(std::size(toplist_sections));

  void set_place(GameWorld *g, int x, int64_t points, std::string_view name)
  {
    g->toppoint[x] = points;
    g->topname[x]  = name;
  }
} // namespace

/* Puts ch on the toplist where they now belong, moving those below down a place, and saves it if anything changed */
void topWrite(GameWorld *g, struct char_data *ch)
{
  if (char_get_admlevel(ch) > 0 || char_is_npc(ch))
    return;
  if (g->TOPLOADED == false)
    return;
  if (!char_get_desc(ch))
    return;

  /* The places as they are right now, and an empty one past the end: someone listed twice in the last section can
     read one place past it as they move up (quirk), which was memory past the list's end */
  std::array<std::string, TOPLIST_PLACES + 1> positions;
  std::array<int64_t, TOPLIST_PLACES + 1> points{};
  for (int x = 0; x < TOPLIST_PLACES; x++) {
    positions[x] = g->topname[x];
    points[x]    = g->toppoint[x];
  }

  bool writeEm = false;
  for (size_t section = 0; section < std::size(toplist_sections); section++) {
    const toplist_section &listed = toplist_sections[section];
    std::string_view name         = listed.by_account ? char_get_user(ch) : char_get_name(ch);
    int start = static_cast<int>(section) * 5, finish = start + 5, location = -1;
    bool placed = false, progress = false;

    for (int x = start; x < finish; x++) {
      if (!placed) {
        if (!str_eq_nocase(g->topname[x], name)) { /* Name doesn't match */
          if (listed.value(g, ch) > g->toppoint[x]) {
            set_place(g, x, listed.value(g, ch), name);
            placed   = true;
            writeEm  = true;
            location = x;
          }
        } else { /* This is their spot already */
          placed   = true;
          location = finish;
        }
      } else if (location < finish) { /* They have placed: the rest move down */
        if (!str_eq_nocase(positions[location], name)) { /* This isn't their old spot */
          set_place(g, x, points[location], positions[location]);
          location += 1;
        } else { /* This IS their old spot */
          progress = true;
          location += 1;
          set_place(g, x, points[location], positions[location]);
          location += 1;
        }
      }
    }

    if (progress)
      send_to_all(g, "@D[@GToplist@W: @C{} @Whas moved up in rank in the {} section.@D]\r\n", name, listed.label);
    else if (placed && location != finish)
      send_to_all(g, "@D[@GToplist@W: @C{} @Whas placed in the {} section.@D]\r\n", name, listed.label);
  }

  if (writeEm)
    save_toplist(g);
}
