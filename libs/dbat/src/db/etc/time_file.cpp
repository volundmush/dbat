/*
 * The time file, etc/time: the game's clock and the dates and counters that last across reboots, a number a line in
 * the order of time_fields[]. The first is the second the game's time began, as the clock stood when it was saved.
 *
 * A number that can't be read leaves it, and every one after it, as it was (quirk: as fscanf() stuck there).
 */
#include <array>
#include <ctime>
#include <variant>

#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/time.hpp"

namespace {

using time_field = std::variant<std::nullptr_t, time_t GameWorld::*, int GameWorld::*>;

/* nullptr is the beginning of the game's time */
const auto time_fields = std::to_array<time_field>({
    nullptr,
    &GameWorld::NEWSUPDATE,
    &GameWorld::BOARDNEWMORT,
    &GameWorld::BOARDNEWDUO,
    &GameWorld::BOARDNEWCOD,
    &GameWorld::BOARDNEWBUI,
    &GameWorld::BOARDNEWIMM,
    &GameWorld::INTERESTTIME,
    &GameWorld::LASTINTEREST,
    &GameWorld::HIGHPCOUNT,
    &GameWorld::PCOUNTDATE,
    &GameWorld::WISHTIME,
    &GameWorld::PCOUNT,
    &GameWorld::LASTPAYOUT,
    &GameWorld::LASTPAYTYPE,
    &GameWorld::LASTNEWS,
    &GameWorld::dballtime,
    &GameWorld::SELFISHMETER,
    &GameWorld::SHADOW_DRAGON1,
    &GameWorld::SHADOW_DRAGON2,
    &GameWorld::SHADOW_DRAGON3,
    &GameWorld::SHADOW_DRAGON4,
    &GameWorld::SHADOW_DRAGON5,
    &GameWorld::SHADOW_DRAGON6,
    &GameWorld::SHADOW_DRAGON7,
    &GameWorld::ERAPLAYERS,
});

struct field_reader {
  GameWorld *g;
  io::fields &in;
  time_t &beginning;

  void operator()(std::nullptr_t) const { read(beginning); }
  void operator()(time_t GameWorld::*f) const { read(g->*f); }
  void operator()(int GameWorld::*f) const
  {
    int n = 0;
    if (in.number(n))
      g->*f = n;
  }
  void read(time_t &t) const
  {
    long long n = 0;
    if (in.number(n))
      t = static_cast<time_t>(n);
  }
};

} // namespace

void read_time_file(GameWorld *g, time_t &beginning)
{
  io::loaded<io::text_reader> file = io::text_reader::open(TIME_FILE);
  if (!file) {
    mud_log("SYSERR: Can't read from '{}' time file.", TIME_FILE);
    return;
  }
  io::fields in(file->text());
  for (const time_field &field : time_fields)
    std::visit(field_reader{g, in, beginning}, field);
}

/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(GameWorld *g, struct time_info_data *when)
{
  io::text_writer out(TIME_FILE);
  for (const time_field &field : time_fields) {
    if (std::holds_alternative<std::nullptr_t>(field))
      out.print("{:d}\n", static_cast<long long>(mud_time_to_secs(g, when)));
    else if (auto *t = std::get_if<time_t GameWorld::*>(&field))
      out.print("{:d}\n", static_cast<long long>(g->**t));
    else
      out.print("{:d}\n", g->*std::get<int GameWorld::*>(field));
  }
  out.commit();
}
