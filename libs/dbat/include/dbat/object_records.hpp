#pragma once
/*
 * Objects as rent files and house files hold them (see db/players/object_records.cpp): a record for each object,
 * with where it was, and the containers refilled as they load.
 */
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "dbat/consts/types.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/objsave.hpp"

struct GameWorld;
struct char_data;
struct obj_data;

/* The readers' differences, kept as they were */
enum class record_kind { rent, house };

/* Reads object records from the line in.line() on, to the end of the file, calling loaded(obj, where it was) for
   each object made; an error if a record is broken partway through */
io::loaded<void> read_object_records(GameWorld *g, io::line_cursor &in, record_kind kind,
                                     const std::function<void(obj_data *, int)> &loaded);

/* Writes obj's record, where it is being kept at locate: a wear position + 1, 0 for carried or in the room, or -n
   for n containers deep */
void write_object_record(GameWorld *g, io::text_writer &out, obj_data *obj, int locate);

/* Puts each object back where the file says it was, as it loads: worn by ch, carried by ch, or in room, and the
   objects that were in a container into it once the container itself has loaded (a container's record comes after
   what it holds) */
class bag_rows {
public:
  bag_rows(GameWorld *g, char_data *ch, room_vnum room) : g_(g), ch_(ch), room_(room) {}
  /* obj, already worn, carried or in the room as it loaded, at locate */
  void place(obj_data *obj, int locate);

private:
  void put(obj_data *obj);
  void take(obj_data *obj);
  void spill(int row);
  void fill(obj_data *container, int row);

  GameWorld *g_;
  char_data *ch_;
  room_vnum room_;
  std::vector<obj_data *> rows_[MAX_BAG_ROWS]; /* each list in file order */
};
