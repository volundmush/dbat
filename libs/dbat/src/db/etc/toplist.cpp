/*
 * The toplist, the 25 places of the leaderboard: a line each, "<name> <points>", in the intro directory.
 */
#include <cstring>
#include <optional>
#include <string>

#include "dbat/fileop.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

void topLoad(GameWorld *g)
{
  std::optional<std::string> path = io::player_file(INTRO_FILE, "toplist");
  io::loaded<io::text_reader> file = path ? io::text_reader::open(*path) : std::unexpected(io::load_error{});
  if (!file) {
    mud_log("ERROR: Toplist file does not exist.");
    return;
  }

  g->TOPLOADED = true;

  /* 25 places, a line each; any the file is short of are empty. A line whose points can't be read keeps them as
     they were. */
  int x = 0;
  for (std::optional<std::string_view> line; x < 25 && (line = file->line()); x++) {
    std::string name;
    long long points = 0;
    io::fields f(*line);
    if (f.word(name, 49) && f.number(points))
      g->toppoint[x] = points;
    g->topname[x] = strdup(name.c_str());
  }
  for (; x < 25; x++)
    g->topname[x] = strdup("");
}

void save_toplist(GameWorld *g)
{
  std::optional<std::string> path = io::player_file(INTRO_FILE, "toplist");
  if (!path)
    return;
  io::text_writer out(*path);
  for (int x = 0; x < 25; x++)
    out.print("{} {:d}\n", g->topname[x], g->toppoint[x]);
  if (!out.commit())
    mud_log("ERROR: could not save Toplist File, {}.", *path);
}
