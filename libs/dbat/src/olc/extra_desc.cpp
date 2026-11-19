/*
 * The extra descriptions submenu, shared by redit and oedit, which each had a copy of it.
 */
#include "dbat/boards.hpp"
#include "dbat/comm.hpp"
#include "dbat/genolc.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/modify.hpp"
#include "dbat/olc/extra_desc.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/stringutils.hpp"

void extra_desc_editor::open(GameWorld *g, struct descriptor_data *d, ex_desc_list &descs)
{
  list = &descs;
  if (list->empty())
    list->emplace_back();
  editing = &list->front();
  menu(g, d);
}

void extra_desc_editor::menu(GameWorld *g, struct descriptor_data *d)
{
  how->show(g, d, *editing, ex_desc_next(*list, editing) != nullptr);
  asking = mode::menu;
}

bool extra_desc_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
{
  if (asking == mode::keyword) {
    std::string keyword(line);
    smash_tilde(keyword);
    editing->keyword = text_or_undefined(keyword);
    menu(g, d);
    return true;
  }

  switch (parse_int(line)) {
    case 0:
      if (!finished()) /* unfinished: drop it */
        ex_desc_remove(*list, editing);
      return false;

    case 1:
      asking = mode::keyword;
      write_to_output(g, d, how->keyword_prompt);
      return true;

    case 2:
      asking = mode::description;
      send_editor_help(g, d);
      write_to_output(g, d, how->description_prompt);
      if (!editing->description.empty())
        write_to_output(g, d, editing->description);
      string_write(d, &editing->description, MAX_MESSAGE_LENGTH, 0);
      return true;

    case 3:
      if (finished()) {
        extra_descr_data *next = ex_desc_next(*list, editing);
        editing                = next ? next : &list->emplace_back();
      } else if (how->says_unfinished)
        write_to_output(g, d, "You can't edit the next extra description without completing this one.\r\n");
      menu(g, d);
      return true;

    default:
      if (how->others_leave) /* quirk: and an unfinished description stays */
        return false;
      menu(g, d);
      return true;
  }
}
