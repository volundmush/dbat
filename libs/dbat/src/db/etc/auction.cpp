/*
 * The auction house's file, etc/auction: a line for each object up for auction in room 80,
 *
 *   <unique id> <seller's name> <seller's id> <highest bidder's id> <starting bid> <bid> <when it ends>
 *
 * and "~END~". Each object, as it loads, looks itself up in it.
 */
#include <cstring>
#include <optional>
#include <string>

#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/iterate.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"

/* save the auction file */
void auc_save(GameWorld *g)
{
  io::text_writer out(AUCTION_FILE);
  for (obj_data *obj : room_contents(g, 80))
    if (obj)
      out.print("{:d} {} {:d} {:d} {:d} {:d} {:d}\n", obj_get_unique_id(obj), obj_get_auctname(obj),
                obj_get_aucter(obj), obj_get_curBidder(obj), obj_get_startbid(obj), obj_get_bid(obj),
                static_cast<long long>(obj_get_aucTime(obj)));
  out.write("~END~\n");
  out.commit();
}

/* load from auction file */
void auc_load(struct obj_data *obj)
{
  io::loaded<io::text_reader> file = io::text_reader::open(AUCTION_FILE);
  if (!file) {
    mud_log("SYSERR: Can't read from '{}' auction file.", AUCTION_FILE);
    return;
  }

  /* quirk: a line that is cut short keeps the rest of the line before it, as sscanf() left them */
  long long id = 0, timer = 0;
  bool have_id = false;
  std::string seller;
  int aID = 0, bID = 0, cost = 0, startc = 0;
  while (std::optional<std::string_view> line = file->line()) {
    io::fields f(*line);
    if (f.number(id))
      have_id = true;
    f.word(seller, 49) && f.number(aID) && f.number(bID) && f.number(startc) && f.number(cost) && f.number(timer);
    if (have_id && obj_get_unique_id(obj) == id) {
      obj_set_auctname(obj, strdup(seller.c_str()));
      obj_set_aucter(obj, aID);
      obj_set_curBidder(obj, bID);
      obj_set_startbid(obj, startc);
      obj_set_bid(obj, cost);
      obj_set_aucTime(obj, static_cast<time_t>(timer));
    }
  }
}
