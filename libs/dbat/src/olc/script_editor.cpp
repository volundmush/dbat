/*
 * The script editor: the triggers a room, object or mobile starts with, as redit, oedit and medit edit them.
 * Once dg_olc.c's dg_script_menu() and dg_script_edit_parse(), by Chris Jacobson, Mark Heilpern and Welcor.
 */
#include <algorithm>
#include <cctype>
#include <optional>

#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/olc/script_editor.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/context_help.hpp"

namespace {
  /* A number as sscanf()'s %d reads one: blanks, a sign and at least one digit; what follows it is left in s */
  std::optional<int> scan_int(std::string_view &s)
  {
    size_t i = 0;
    while (i < s.size() && isspace((unsigned char)s[i]))
      i++;
    const size_t start = i;
    if (i < s.size() && (s[i] == '+' || s[i] == '-'))
      i++;
    const size_t digits = i;
    while (i < s.size() && isdigit((unsigned char)s[i]))
      i++;
    if (i == digits)
      return std::nullopt;
    const int value = parse_int(s.substr(start, i - start));
    s.remove_prefix(i);
    return value;
  }

  /* "position, vnum", or a vnum alone for the end of the list: what sscanf(line, "%d, %d") read */
  std::pair<int, trig_vnum> position_and_vnum(std::string_view line)
  {
    const std::optional<int> first = scan_int(line);
    if (!first)
      return {0, -1}; /* which aborts the entry */
    if (line.starts_with(',')) {
      line.remove_prefix(1);
      if (const std::optional<int> second = scan_int(line))
        return {*first, *second};
    }
    return {999, *first};
  }
} // namespace

void script_editor::menu(GameWorld *g, struct descriptor_data *d)
{
  asking = mode::main_menu;

  clear_screen(g, d);
  write_to_output(g, d, "     Script Editor\r\n\r\n     Trigger List:\r\n");

  int i = 0;
  for (trig_vnum vnum : triggers) {
    const trig_data *proto = find_trigger(g, vnum)->proto;
    write_to_output(g, d, "     {:>2d}) [@c{:d}@n] @c{}@n", ++i, vnum, proto->name);
    write_to_output(g, d, proto->attach_type != attach_type ? "   @g** Mis-matched Trigger Type **@n\r\n" : "\r\n");
  }
  if (i == 0)
    write_to_output(g, d, "     <none>\r\n");

  write_to_output(g, d,
                  "\r\n @gN@n)  New trigger for this script\r\n @gD@n)  Delete a trigger in this script\r\n @gX@n)  Exit Script Editor\r\n\r\n     Enter choice :");
}

bool script_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
{
  switch (asking) {
    case mode::main_menu:
      switch (tolower(first_char(line))) {
        case 'x':
          return false;
        case 'n':
          write_to_output(g, d, "\r\nPlease enter position, vnum   (ex: 1, 200):");
          asking = mode::new_trigger;
          break;
        case 'd':
          write_to_output(g, d, "     Which entry should be deleted?  0 to abort :");
          asking = mode::delete_trigger;
          break;
        default:
          menu(g, d);
          break;
      }
      return true;

    case mode::new_trigger: {
      const auto [pos, vnum] = position_and_vnum(line);
      if (pos <= 0 || vnum == 0) /* this aborts a new trigger entry */
        break;
      if (find_trigger(g, vnum) == nullptr) {
        write_to_output(g, d, "Invalid Trigger VNUM!\r\nPlease enter position, vnum   (ex: 1, 200):");
        return true;
      }
      /* quirk: position 1 is the first, but any other n goes after the nth, or at the end */
      triggers.insert(triggers.begin() + (pos == 1 ? 0 : std::min((size_t)pos, triggers.size())), vnum);
      break;
    }

    case mode::delete_trigger:
      if (const int pos = parse_int(line); pos > 0 && (size_t)pos <= triggers.size())
        triggers.erase(triggers.begin() + (pos - 1));
      break;
  }

  menu(g, d);
  return true;
}

int script_editor::help_context() const
{
  switch (asking) {
    case mode::main_menu: return CONTEXT_SCRIPT_MAIN_MENU;
    case mode::new_trigger: return CONTEXT_SCRIPT_NEW_TRIGGER;
    case mode::delete_trigger: return CONTEXT_SCRIPT_DEL_TRIGGER;
  }
  return NOTHING;
}
