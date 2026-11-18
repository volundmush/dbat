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

/* A file's lines as get_line() read them for the rent and house readers: at the end of the file the line is left as
   it was, and eof() says the end has been met, as feof() did */
class record_lines {
public:
  explicit record_lines(io::text_reader &in) : in_(in) {}
  /* The next line; false, leaving line() as it was, at the end of the file */
  bool next();
  const std::string &line() const { return line_; }
  bool eof() const { return eof_; }
  /* A '~' string, as fread_string() read one */
  io::loaded<std::string> string(std::string_view what);
  io::load_error fail(std::string message) const { return in_.fail(std::move(message)); }

private:
  io::text_reader &in_;
  std::string line_{};
  bool eof_{};
};

/* The readers' differences, kept as they were */
enum class record_kind { rent, house };

/* Reads object records from the line in.line() on, to the end of the file, calling loaded(obj, where it was) for
   each object made; an error if a record is broken partway through */
io::loaded<void> read_object_records(GameWorld *g, record_lines &in, record_kind kind,
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
