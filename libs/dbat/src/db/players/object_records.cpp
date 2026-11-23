/*
 * Object records, shared by rent files (plrobjs/<bucket>/<name>.new) and house files (house/<vnum>.house):
 *
 *   #<vnum>                            -1 for an object of its own, with no prototype
 *   <locate> <values 0-7> <4 flags words> <values 8-15>
 *   XAP
 *   <keywords>~
 *   <short description>~
 *   <long description>~
 *   <action description>~
 *   <type> <4 wear flags> <weight> <cost> <cost per day>
 *   G / U / Z / A / E / S, each followed by its line(s)
 *
 * The lines are read by get_line()'s rules, a line left as it was at the end of the file, and a few of the readers'
 * numbers last from one record to the next (quirk): a values line cut short keeps the rest of the record before's.
 */
#include <cstdlib>
#include <string>

#include "dbat/object_records.hpp"
#include "dbat/affect.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/handler.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/relocate.hpp"
#include "dbat/stringutils.hpp"

io::loaded<void> read_object_records(GameWorld *g, io::line_cursor &in, record_kind kind,
                                     const std::function<void(obj_data *, int)> &loaded)
{
  const char *file_kind = kind == record_kind::rent ? "rent" : "house";
  int t[30]             = {}; /* a damaged line leaves some of these unread */
  std::string f[4];

  while (!in.eof()) {
    if (!in.line().starts_with('#')) {
      in.next();
      continue;
    }
    int nr = 0;
    if (!io::fields(std::string_view(in.line()).substr(1)).number(nr)) {
      in.next(); /* skip it, or the loop never moves on */
      continue;
    }
    obj_data *temp;
    if (nr == NOTHING) { /* then it is unique */
      temp = create_obj(g);
      obj_set_item_number(temp, NOTHING);
      if (kind == record_kind::rent)
        obj_set_size(temp, SIZE_MEDIUM);
    } else if (nr < 0 || nr >= 999999 || !(temp = read_object(g, nr))) {
      in.next();
      continue;
    }

    in.next();
    io::fields values(in.line());
    bool read = true;
    for (int i = 0; i < 9 && read; i++)
      read = values.number(t[i]);
    for (int i = 0; i < 4 && read; i++)
      read = values.word(f[i]);
    for (int i = 13; i < 21 && read; i++)
      read = values.number(t[i]);
    int locate = t[0];
    for (int i = 0; i < 8; i++)
      obj_set_value(temp, i, t[i + 1]);
    for (int i = 0; i < 4; i++) /* quirk: a house file's fourth flags word is its third again */
      obj_get_extra_flags(temp)[i] = io::ascii_flags(f[kind == record_kind::house && i == 3 ? 2 : i], 0, "asciiflag_conv");
    for (int i = 8; i < 16; i++)
      obj_set_value(temp, i, t[i + 5]);
    if (kind == record_kind::house) {
      obj_set_posted_to(temp, nullptr);
      obj_set_posttype(temp, 0);
    }

    in.next();
    if (in.line() == "XAP") { /* then this is a Xap Obj, requires special care */
      io::loaded<std::string> text[4];
      for (int i = 0; i < 4; i++)
        if (!(text[i] = in.string(str_format("{} object #{:d}", file_kind, nr))))
          return std::unexpected(text[i].error());
      obj_set_name(temp, text[0]->empty() ? std::string("undefined") : *text[0]);
      obj_set_short_description(temp, text[1]->empty() ? std::string("undefined") : *text[1]);
      obj_set_description(temp, text[2]->empty() ? std::string("undefined") : *text[2]);
      obj_set_action_description(temp, *text[3]);

      bool all = in.next();
      io::fields numbers(in.line());
      for (int i = 0; i < 8 && all; i++)
        all = numbers.number(t[i]);
      if (!all)
        return std::unexpected(in.fail(str_format("{} object #{:d}: expected its type, wear flags, weight and costs",
                                                  file_kind, nr)));
      obj_set_type_flag(temp, t[0]);
      for (int i = 0; i < 4; i++)
        obj_get_wear_flags(temp)[i] = t[i + 1];
      obj_set_weight(temp, t[5]);
      obj_set_cost(temp, t[6]);
      if (kind == record_kind::house) /* quirk: a rented object's cost per day is its prototype's */
        obj_set_cost_per_day(temp, t[7]);

      for (int j = 0; j < MAX_OBJ_AFFECT; j++) /* we're clearing these for good luck */
        obj_get_affected(temp)[j] = {.location = APPLY_NONE, .specific = 0, .modifier = 0};
      obj_ex_description_slot(temp)->clear();

      in.next();
      /* quirk: affects and spellbook pages share one count */
      for (int j = 0, done = 0; !done && !in.eof();) {
        switch (in.line().empty() ? '\0' : in.line()[0]) {
          case 'E': {
            extra_descr_data descr;
            io::loaded<std::string> keyword = in.string(str_format("{} object #{:d}'s extra description", file_kind, nr));
            if (!keyword)
              return std::unexpected(keyword.error());
            io::loaded<std::string> words = in.string(str_format("{} object #{:d}'s extra description", file_kind, nr));
            if (!words)
              return std::unexpected(words.error());
            descr.keyword     = *keyword;
            descr.description = *words;
            obj_ex_description_slot(temp)->push_front(std::move(descr)); /* quirk: the order turns round */
            in.next();
            break;
          }
          case 'A': {
            if (j >= MAX_OBJ_AFFECT)
              mud_log("SYSERR: Too many object affectations in loading {} file", file_kind);
            in.next();
            io::fields a(in.line());
            a.number(t[0]) && a.number(t[1]) && a.number(t[2]);
            if (j < MAX_OBJ_AFFECT) {
              obj_get_affected(temp)[j].location = t[0];
              obj_get_affected(temp)[j].modifier = t[1];
              obj_get_affected(temp)[j].specific = affect_checked_specific(t[0], t[2]);
              j++;
            }
            in.next();
            break;
          }
          case 'G': {
            in.next();
            long long generation = 0;
            if (io::fields(in.line()).number(generation))
              obj_set_generation(temp, generation);
            in.next();
            break;
          }
          case 'U': {
            in.next();
            long long uid = obj_get_unique_id(temp);
            io::fields(in.line()).number(uid);
            /* -1 is an object not yet given one; no real one is negative. */
            if (uid < -1) {
              mud_log("SYSERR: Object #{:d} has unique id {:d}, which can't be; giving it a new one.",
                      obj_get_vnum(g, temp), uid);
              uid = -1;
            }
            obj_set_unique_id(temp, uid);
            in.next();
            break;
          }
          case 'S': {
            if (j >= SPELLBOOK_SIZE)
              mud_log("SYSERR: Too many spells in spellbook loading rent file");
            in.next();
            io::fields s(in.line());
            s.number(t[0]) && s.number(t[1]);
            /* quirk: a rented spellbook's pages aren't read; a house's are, as many as the spellbook holds (past
               them, the old reader wrote past its end) */
            if (kind == record_kind::house && j < SPELLBOOK_SIZE) {
              if (!obj_get_sbinfo(temp))
                obj_make_sbinfo(temp);
              obj_get_sbinfo(temp)[j].spellname = t[0];
              obj_get_sbinfo(temp)[j].pages     = t[1];
            }
            j++;
            in.next();
            break;
          }
          case 'Z': {
            in.next();
            int size = obj_get_size(temp);
            io::fields(in.line()).number(size);
            obj_set_size(temp, size);
            in.next();
            break;
          }
          default: /* '#', '$' or anything else ends the object */
            done = 1;
            break;
        }
      }
    }
    loaded(temp, locate);
  }
  return {};
}

void write_object_record(GameWorld *g, io::text_writer &out, obj_data *obj, int locate)
{
  std::string flags[4];
  for (int i = 0; i < 4; i++)
    flags[i] = flag_letters(obj_get_extra_flags(obj)[i]);
  out.print("#{:d}\n{:d}", obj_get_vnum(g, obj), locate);
  for (int i = 0; i < 8; i++)
    out.print(" {:d}", obj_get_value(obj, i));
  out.print(" {} {} {} {}", flags[0], flags[1], flags[2], flags[3]);
  for (int i = 8; i < 16; i++)
    out.print(" {:d}", obj_get_value(obj, i));
  out.write("\n");

  /* every object saves its XAP data (an early return for most once sat here, but its condition was never true) */
  std::string action(obj_get_action_description(obj));
  strip_cr(action);
  out.print("XAP\n{}~\n{}~\n{}~\n{}~\n", or_if_empty(obj_get_name(obj), "undefined"),
            or_if_empty(obj_get_short_description(obj), "undefined"),
            or_if_empty(obj_get_description(obj), "undefined"), action);
  out.print("{:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}\n", obj_get_type_flag(obj),
            static_cast<int>(obj_get_wear_flags(obj)[0]), /* as signed, as always */
            static_cast<int>(obj_get_wear_flags(obj)[1]), static_cast<int>(obj_get_wear_flags(obj)[2]),
            static_cast<int>(obj_get_wear_flags(obj)[3]), obj_get_weight(obj), obj_get_cost(obj),
            obj_get_cost_per_day(obj));

  if (obj_get_generation(obj))
    out.print("G\n{:d}\n", static_cast<long long>(obj_get_generation(obj)));
  if (obj_get_unique_id(obj))
    out.print("U\n{:d}\n", obj_get_unique_id(obj));
  out.print("Z\n{:d}\n", obj_get_size(obj));

  for (int i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj_get_affected(obj)[i].modifier)
      out.print("A\n{:d} {:d} {:d}\n", obj_get_affected(obj)[i].location, obj_get_affected(obj)[i].modifier,
                obj_get_affected(obj)[i].specific);

  for (const extra_descr_data &ex : obj_get_ex_description(obj)) {
    if (ex.keyword.empty() || ex.description.empty())
      continue;
    std::string text = ex.description;
    strip_cr(text);
    out.print("E\n{}~\n{}~\n", ex.keyword, text);
  }

  if (obj_get_sbinfo(obj))
    for (int i = 0; i < SPELLBOOK_SIZE && obj_get_sbinfo(obj)[i].spellname != 0; i++)
      out.print("S\n{:d} {:d}\n", obj_get_sbinfo(obj)[i].spellname, obj_get_sbinfo(obj)[i].pages);
}

void bag_rows::put(obj_data *obj)
{
  if (ch_)
    obj_to_char(g_, obj, ch_);
  else
    obj_to_room(g_, obj, room_);
}

void bag_rows::take(obj_data *obj)
{
  if (ch_)
    obj_from_char(g_, obj);
  else
    obj_from_room(g_, obj);
}

/* the objects of a row whose container has gone: back to the inventory, or the room */
void bag_rows::spill(int row)
{
  for (obj_data *obj : rows_[row])
    put(obj);
  rows_[row].clear();
}

void bag_rows::fill(obj_data *container, int row)
{
  obj_contains_slot(container)->clear(); /* should be empty - but who knows */
  for (obj_data *obj : rows_[row])
    obj_to_obj(g_, obj, container);
  rows_[row].clear();
}

/*
 * If there are objects waiting a row or more deeper than obj, their container is gone, and they go back to the
 * inventory or the room. If there are objects waiting just one row deeper, obj is their container: it is filled
 * (taken out first, so that its weight is right when it goes back), or if it isn't a container, they spill out.
 * An object inside a container (locate < 0) then waits in its own row, at the end, in the order the file has them.
 */
void bag_rows::place(obj_data *obj, int locate)
{
  if (locate > 0 && ch_) { /* worn, or carried if it couldn't be */
    for (int j = MAX_BAG_ROWS - 1; j > 0; j--)
      if (!rows_[j].empty())
        spill(j);
    if (!rows_[0].empty()) {
      if (obj_get_type_flag(obj) == ITEM_CONTAINER) {
        bool worn = locate <= NUM_WEARS && char_get_equipment(ch_, locate - 1) == obj;
        if (worn) /* off, filled and on again */
          obj = unequip_char(g_, ch_, locate - 1);
        fill(obj, 0);
        if (worn)
          equip_char(g_, ch_, obj, locate - 1);
      } else
        spill(0);
    }
    return;
  }
  if (locate > 0) /* nothing is worn in a room; the old house reader read before its rows for this */
    locate = 0;

  int j;
  for (j = MAX_BAG_ROWS - 1; j > -locate; j--)
    if (!rows_[j].empty())
      spill(j);
  if (j == -locate && !rows_[j].empty()) {
    if (obj_get_type_flag(obj) == ITEM_CONTAINER) {
      take(obj);
      fill(obj, j);
      put(obj);
    } else
      spill(j);
  }
  if (locate < 0 && locate >= -MAX_BAG_ROWS) {
    take(obj);
    rows_[-locate - 1].push_back(obj);
  }
}
