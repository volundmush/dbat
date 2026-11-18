#include <cerrno>
#include <cstdlib>

#include <algorithm>
#include <cstring>
#include <optional>
#include <string>

#include "dbat/fileop.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/log.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large. The rules are io::is_skipped_line() and io::line_text(). */
int get_line(FILE *fl, char *buf)
{
  std::string raw;
  int lines = 0;

  do {
    if (!io::read_raw_line(fl, raw))
      return 0;
    lines++;
  } while (io::is_skipped_line(raw));

  std::string_view text = io::line_text(raw);
  memcpy(buf, text.data(), text.size());
  buf[text.size()] = '\0';
  return lines;
}

std::optional<std::string> player_path(int mode, const char *name)
{
  if (name == nullptr || *name == '\0') {
    static const char buffer = 0; /* stands for the buffer get_filename() was given, which the log line names */
    mud_log("SYSERR: NULL pointer or empty string passed to get_filename(), {} or {}.", as_pointer(name),
            as_pointer(&buffer));
    return std::nullopt;
  }
  return io::player_file(mode, name);
}

/* The file of kind mode for the player or account orig_name, as io::player_file() gives it, cut to fbufsize. */
bool get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  if (filename == nullptr) {
    mud_log("SYSERR: NULL pointer or empty string passed to get_filename(), {} or {}.", as_pointer(orig_name),
            as_pointer(filename));
    return false;
  }
  std::optional<std::string> path = player_path(mode, orig_name);
  if (!path)
    return false;
  if (fbufsize) { /* quirk: a name too long for the buffer is cut short, as snprintf() cut it */
    size_t n = std::min(path->size(), fbufsize - 1);
    memcpy(filename, path->data(), n);
    filename[n] = '\0';
  }
  return true;
}

/* the "touch" command, essentially. */
int touch(const char *path)
{
  return io::touch(path) ? 0 : -1;
}

/* Write the toplist to file */
void topWrite(GameWorld *g, struct char_data *ch)
{
  if (char_get_admlevel(ch) > 0 || char_is_npc(ch))
    return;

  if (g->TOPLOADED == false) {
    return;
  }

  char *positions[25];
  int64_t points[25] = {0};
  int x = 0, writeEm = false, placed = false, start = 0, finish = 25, location = -1;
  int progress = false;

  if (!ch) {
    return;
  }

  if (!char_get_desc(ch) || !char_get_user(ch)) {
    return;
  }

  for (x = start; x < finish; x++) { /* Save the places as they are right now */
    positions[x] = strdup(g->topname[x]);
    points[x]    = g->toppoint[x];
  }

  /* Powerlevel Section */
  /* Set the start and finish for this section */
  start  = 0;
  finish = 5;

  for (x = start; x < finish; x++) {                   /* Save the new spots */
    if (placed == false) {                             /* They Haven't Placed */
      if (strcasecmp(g->topname[x], char_get_name(ch))) { /* Name doesn't match */
        if (getEffMaxPL(g, ch) > g->toppoint[x]) {
          free(g->topname[x]);
          g->toppoint[x] = getEffMaxPL(g, ch);
          g->topname[x]  = strdup(char_get_name(ch));
          placed      = true;
          writeEm     = true;
          location    = x;
        }
      } else { /* This is their spot already */
        placed   = true;
        location = finish;
      }
    } else { /* They have placed */
      if (x < finish && location < finish) {
        if (strcasecmp(positions[location], char_get_name(ch))) { /* This isn't their old spot */
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        } else { /* This IS their old spot */
          progress = true;
          location += 1;
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        }
      }
    }
  } /* End Save New Spots*/

  if (progress == true) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas moved up in rank in the powerlevel section.@D]\r\n", char_get_name(ch));
  } else if (placed == true && location != finish) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas placed in the powerlevel section.@D]\r\n", char_get_name(ch));
  }

  location = -1;
  placed   = false;
  progress = false;
  /* Ki Section         */
  /* Set the start and finish for this section */
  start  = 5;
  finish = 10;

  for (x = start; x < finish; x++) {                   /* Save the new spots */
    if (placed == false) {                             /* They Haven't Placed */
      if (strcasecmp(g->topname[x], char_get_name(ch))) { /* Name doesn't match */
        if (getMaxKI(g, ch) > g->toppoint[x]) {
          free(g->topname[x]);
          g->toppoint[x] = getMaxKI(g, ch);
          g->topname[x]  = strdup(char_get_name(ch));
          placed      = true;
          writeEm     = true;
          location    = x;
        }
      } else { /* This is their spot already */
        placed   = true;
        location = finish;
      }
    } else { /* They have placed */
      if (x < finish && location < finish) {
        if (strcasecmp(positions[location], char_get_name(ch))) { /* This isn't their old spot */
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        } else { /* This IS their old spot */
          progress = true;
          location += 1;
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        }
      }
    }
  } /* End Save New Spots*/

  if (progress == true) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas moved up in rank in the ki section.@D]\r\n", char_get_name(ch));
  } else if (placed == true && location != finish) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas placed in the ki section.@D]\r\n", char_get_name(ch));
  }

  location = -1;
  placed   = false;
  progress = false;

  /* Stamina Section    */
  /* Set the start and finish for this section */
  start  = 10;
  finish = 15;

  for (x = start; x < finish; x++) {                   /* Save the new spots */
    if (placed == false) {                             /* They Haven't Placed */
      if (strcasecmp(g->topname[x], char_get_name(ch))) { /* Name doesn't match */
        if (getMaxST(g, ch) > g->toppoint[x]) {
          free(g->topname[x]);
          g->toppoint[x] = getMaxST(g, ch);
          g->topname[x]  = strdup(char_get_name(ch));
          placed      = true;
          writeEm     = true;
          location    = x;
        }
      } else { /* This is their spot already */
        placed   = true;
        location = finish;
      }
    } else { /* They have placed */
      if (x < finish && location < finish) {
        if (strcasecmp(positions[location], char_get_name(ch))) { /* This isn't their old spot */
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        } else { /* This IS their old spot */
          progress = true;
          location += 1;
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        }
      }
    }
  } /* End Save New Spots*/

  if (progress == true) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas moved up in rank in the stamina section.@D]\r\n", char_get_name(ch));
  } else if (placed == true && location != finish) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas placed in the stamina section.@D]\r\n", char_get_name(ch));
  }

  location = -1;
  placed   = false;
  progress = false;

  /* Zenni Section      */
  /* Set the start and finish for this section */
  start  = 15;
  finish = 20;

  for (x = start; x < finish; x++) {                   /* Save the new spots */
    if (placed == false) {                             /* They Haven't Placed */
      if (strcasecmp(g->topname[x], char_get_name(ch))) { /* Name doesn't match */
        if (char_get_bank_gold(ch) + char_get_gold(ch) > g->toppoint[x]) {
          free(g->topname[x]);
          g->toppoint[x] = char_get_bank_gold(ch) + char_get_gold(ch);
          g->topname[x]  = strdup(char_get_name(ch));
          placed      = true;
          writeEm     = true;
          location    = x;
        }
      } else { /* This is their spot already */
        placed   = true;
        location = finish;
      }
    } else { /* They have placed */
      if (x < finish && location < finish) {
        if (strcasecmp(positions[location], char_get_name(ch))) { /* This isn't their old spot */
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        } else { /* This IS their old spot */
          progress = true;
          location += 1;
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        }
      }
    }
  } /* End Save New Spots*/

  if (progress == true) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas moved up in rank in the zenni section.@D]\r\n", char_get_name(ch));
  } else if (placed == true && location != finish) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas placed in the zenni section.@D]\r\n", char_get_name(ch));
  }

  location = -1;
  placed   = false;
  progress = false;

  /* RPP Section        */
  /* Set the start and finish for this section */
  start  = 20;
  finish = 25;

  for (x = start; x < finish; x++) {                   /* Save the new spots */
    if (placed == false) {                             /* They Haven't Placed */
      if (strcasecmp(g->topname[x], char_get_user(ch))) { /* Name doesn't match */
        if (char_get_trp(ch) > g->toppoint[x]) {
          free(g->topname[x]);
          g->toppoint[x] = char_get_trp(ch);
          g->topname[x]  = strdup(char_get_user(ch));
          placed      = true;
          writeEm     = true;
          location    = x;
        }
      } else { /* This is their spot already */
        placed   = true;
        location = finish;
      }
    } else { /* They have placed */
      if (x < finish && location < finish) {
        if (strcasecmp(positions[location], char_get_user(ch))) { /* This isn't their old spot */
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        } else { /* This IS their old spot */
          progress = true;
          location += 1;
          free(g->topname[x]);
          g->toppoint[x] = points[location];
          g->topname[x]  = strdup(positions[location]);
          location += 1;
        }
      }
    }
  } /* End Save New Spots*/

  if (progress == true) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas moved up in rank in the RPP section.@D]\r\n", char_get_user(ch));
  } else if (placed == true && location != finish) {
    send_to_all(g, "@D[@GToplist@W: @C{} @Whas placed in the RPP section.@D]\r\n", char_get_user(ch));
  }

  location = -1;
  placed   = false;
  progress = false;

  for (x = 0; x < 25; x++) {
    free(positions[x]);
  }

  if (writeEm == true) {
    save_toplist(g);
  }
  return;
}