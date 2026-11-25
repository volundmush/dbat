/*
 * The level configuration, etc/levels: sections, each "#<name>" and its lines, up to a line starting with '$'.
 *
 *   #version      the line after it is the file's version
 *   #vernum       its version number
 *   #experience   the experience multiplier
 *   #fortitude, #reflex, #will, #basehit
 *                 "<class number> <value>" a line: a saving throw, or the base hit, for each class
 *
 * The lines are read as fgets() into a READ_SIZE buffer read them, with their line ends: no line is passed over but
 * a '*' comment (quirk: a blank line in #version is the version).
 */
#include <array>
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include "dbat/class.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/log.hpp"
#include "dbat/stringutils.hpp"

namespace {

static constexpr std::array<std::string_view, NUM_CONFIG_SECTIONS> config_sect = {{"version", "experience", "vernum", "fortitude",
                                                          "reflex",  "will",       "basehit"}};

/* The keywords above, in order. */
enum {
  CONFIG_LEVEL_VERSION,
  CONFIG_LEVEL_EXPERIENCE,
  CONFIG_LEVEL_VERNUM,
  CONFIG_LEVEL_FORTITUDE,
  CONFIG_LEVEL_REFLEX,
  CONFIG_LEVEL_WILL,
  CONFIG_LEVEL_BASEHIT,
};

bool digit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }

int as_atoi(std::string_view text) { return static_cast<int>(io::c_atol(text)); }

} // namespace

int load_levels(GameWorld *g)
{
  io::loaded<io::text_reader> file = io::text_reader::open(LEVEL_CONFIG);
  if (!file) {
    mud_log("SYSERR: Could not open level configuration file, error: {}!", io::why_unreadable(LEVEL_CONFIG));
    return -1;
  }

  for (int cls = 0; cls < NUM_CLASSES; cls++) {
    for (int tp = 0; tp <= SAVING_WILL; tp++)
      g->save_classes[tp][cls] = 0;
    g->basehit_classes[cls] = 0;
  }

  std::string sect_name;
  int sect_type = -1;
  std::string_view rest = file->text();
  for (int linenum = 1;; linenum++) {
    if (rest.empty()) {
      mud_log("SYSERR: Unexpected EOF in file {}.", LEVEL_CONFIG);
      return -1;
    }
    std::string_view piece = io::fgets_piece(rest);
    std::string_view line  = piece.substr(0, piece.find('\0'));
    if (line.starts_with('$')) /* end of file */
      break;
    if (line.starts_with('*')) /* comment line */
      continue;

    if (line.starts_with('#')) { /* start of a section */
      std::string word;
      if (!io::fields(line.substr(1)).word(word)) {
        mud_log("SYSERR: Format error in file {}, line number {:d} - text: {}.", LEVEL_CONFIG, linenum, line);
        return -1;
      }
      sect_name.clear();
      for (char c : word) /* as search_block() left it */
        sect_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      if ((sect_type = search_block(sect_name, config_sect, false)) == -1) {
        mud_log("SYSERR: Invalid section in file {}, line number {:d}: {}.", LEVEL_CONFIG, linenum, sect_name);
        return -1;
      }
    } else if (sect_type == CONFIG_LEVEL_VERSION) {
      if (line.starts_with("Suntzu")) {
        mud_log("SYSERR: Suntzu {} config files are not compatible with rasputin", LEVEL_CONFIG);
        return -1;
      }
      g->level_version = until_nul(line).substr(0, READ_SIZE - 1);
    } else if (sect_type == CONFIG_LEVEL_VERNUM) {
      g->level_vernum = as_atoi(line);
    } else if (sect_type == CONFIG_LEVEL_EXPERIENCE) {
      g->exp_multiplier = static_cast<float>(as_atoi(line));
    } else if ((sect_type >= CONFIG_LEVEL_FORTITUDE && sect_type <= CONFIG_LEVEL_WILL) ||
               sect_type == CONFIG_LEVEL_BASEHIT) {
      size_t at = 0;
      while (at < line.size() && !digit(line[at]))
        at++;
      if (at >= line.size()) {
        mud_log("SYSERR: Cannot find class number in file {}, line number {:d}, section {}.", LEVEL_CONFIG,
                linenum, sect_name);
        return -1;
      }
      int cls = as_atoi(line.substr(at));
      while (at < line.size() && digit(line[at]))
        at++;
      while (at < line.size() && !digit(line[at]))
        at++;
      if (at >= line.size()) {
        mud_log("SYSERR: Need 1 value in {}, line number {:d}, section {}.", LEVEL_CONFIG, linenum, sect_name);
        return -1;
      }
      int tp = as_atoi(line.substr(at));
      if (cls < 0 || cls >= NUM_CLASSES) {
        mud_log("SYSERR: Invalid class number {:d} in file {}, line number {:d}.", cls, LEVEL_CONFIG, linenum);
        return -1;
      }
      if (sect_type == CONFIG_LEVEL_BASEHIT)
        g->basehit_classes[cls] = tp;
      else
        g->save_classes[SAVING_FORTITUDE + sect_type - CONFIG_LEVEL_FORTITUDE][cls] = tp;
    } else {
      mud_log("Unsupported level config option");
    }
  }
  return 0;
}
