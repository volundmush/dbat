/* ************************************************************************
 *  File: assedit.c                                                       *
 *  Online editing of assemblies (crafting recipes): what an object is   *
 *  assembled from.                                                       *
 ************************************************************************ */
#include <cstdlib>
#include <vector>

#include "dbat/args.hpp"
#include "dbat/assedit.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/assemblytypes.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/oasis.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/stringutils.hpp"

namespace {
  /* The assembly editor: one assembly, what it's assembled from and how; saved however the builder leaves */
  struct assembly_editor final : olc_session {
    enum class mode { main_menu, add_component, edit_component, delete_component, edit_extract, edit_inroom, edit_types };

    mode asking = mode::main_menu;
    obj_vnum vnum{};                       /* the object assembled */
    unsigned char type{};                  /* ASSM_xxx */
    std::vector<component_data> components;
    size_t editing{};                      /* the component being edited */

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override
    {
      log_stops_editing(g, d, NOWHERE);
    }

    void menu(GameWorld *g, struct descriptor_data *d);
  };

  void usage(GameWorld *g, struct descriptor_data *d)
  {
    send_to_char(g, d->character, "Usage: assedit <vnum>\r\n");
    send_to_char(g, d->character, "     : assedit new <vnum>\r\n");
    send_to_char(g, d->character, "     : assedit delete <vnum>\r\n");
  }

  /* A component's number in the menu, if the line is one: from 1 */
  int component_number(std::string_view line) { return isdigit(first_char(line)) ? atoi(std::string(line).c_str()) : 0; }

  void assembly_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    send_to_char(g, d->character, "Assembly Number: @c{:d}@n\r\nAssembly Name  : @y{}@n\r\nAssembly Type  : @y{}@n\r\nComponents:\r\n", vnum,
                 obj_get_short_description(obj_proto_of(g, vnum)), sprinttype(type, AssemblyTypes));

    if (components.empty())
      send_to_char(g, d->character, "   < NONE > \r\n");
    for (size_t i = 0; i < components.size(); i++) {
      const component_data &part = components[i];
      if (struct obj_data *proto = obj_proto_of(g, part.lVnum))
        send_to_char(g, d->character, "@g{:>2d}@n) [@c{:>5d}@n] {:<20.20}  In room: @c{:<3.3}@n    Extract: @y{:<3.3}@n\r\n", i + 1, part.lVnum,
                     obj_get_short_description(proto), part.bInRoom ? "Yes" : "No", part.bExtract ? "Yes" : "No");
      else
        send_to_char(g, d->character, "@g{:>2d}@n) @y ERROR --- Contact an Implementor @n\r\n ", i + 1);
    }
    send_to_char(g, d->character,
                 "@gA@n) Add a new component.\r\n@gE@n) Edit a component.\r\n@gD@n) Delete a component.\r\n@gT@n) Change Assembly Type.\r\n@gQ@n) Quit.\r\n\r\nEnter your choice : ");
    asking = mode::main_menu;
  }

  void assembly_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char answer = first_char(line);
    const int number  = component_number(line);

    switch (asking) {
      case mode::main_menu:
        switch (answer) {
          case 'q':
          case 'Q':
            /* due to the infrequent use of this code and restricted use, I decided to copy over changes regardless. */
            assemblyDestroy(g, vnum);
            assemblyCreate(g, vnum, type);
            for (const component_data &part : components)
              assemblyAddComponent(g, vnum, part.lVnum, part.bExtract, part.bInRoom);
            send_to_char(g, d->character, "\r\nSaving all assemblies\r\n");
            assemblySaveAssemblies(g);
            leave_olc(g, d);
            return;
          case 't':
          case 'T':
            for (int counter = 0; counter < MAX_ASSM; counter++)
              send_to_char(g, d->character, "@g{:>2d}@n) {:<20.20} {}", counter + 1, AssemblyTypes[counter], counter % 2 ? "\r\n" : "");
            send_to_char(g, d->character, "Enter the assembly type : ");
            asking = mode::edit_types;
            return;
          case 'a':
          case 'A':
            send_to_char(g, d->character, "\r\nWhat is the vnum of the new component?");
            asking = mode::add_component;
            return;
          case 'e':
          case 'E':
            send_to_char(g, d->character, "\r\nEdit which component? ");
            asking = mode::edit_component;
            return;
          case 'd':
          case 'D':
            send_to_char(g, d->character, "\r\nWhich component do you wish to remove?");
            asking = mode::delete_component;
            return;
          default:
            menu(g, d);
            return;
        }

      case mode::edit_types:
        if (!isdigit(answer))
          menu(g, d);
        else if (number >= 1 && number <= MAX_ASSM) {
          type = number - 1;
          menu(g, d);
        } /* quirk: any other number is ignored, and the type is asked for again without asking */
        return;

      case mode::add_component:
        if (!isdigit(answer)) {
          send_to_char(g, d->character, "That object does not exist. Please try again\r\n");
          menu(g, d);
          return;
        }
        /* quirk: whether the object exists isn't checked; the menu says ERROR for one that doesn't */
        components.push_back({.bExtract = true, .bInRoom = false, .lVnum = number});
        menu(g, d);
        return;

      case mode::edit_component:
        if (number >= 1 && number <= std::ssize(components)) {
          editing = number - 1;
          send_to_char(g, d->character, "Is the item to be extracted when the assembly is created? (Y/N):");
          asking = mode::edit_extract;
        } else
          menu(g, d);
        return;

      case mode::delete_component:
        if (number >= 1 && number <= std::ssize(components))
          components.erase(components.begin() + (number - 1));
        menu(g, d);
        return;

      case mode::edit_extract:
      case mode::edit_inroom: {
        const bool extract = asking == mode::edit_extract;
        if (answer != 'y' && answer != 'Y' && answer != 'n' && answer != 'N') {
          send_to_char(g, d->character, extract ? "Is the item to be extracted when the assembly is created? (Y/N)"
                                                : "Object in the room when assembly is created? (n =  in inventory):");
          return;
        }
        (extract ? components[editing].bExtract : components[editing].bInRoom) = answer == 'y' || answer == 'Y';
        if (extract) {
          send_to_char(g, d->character, "Should the object be in the room when assembly is created (n = in inventory)?");
          asking = mode::edit_inroom;
        } else
          menu(g, d);
        return;
      }
    }
  }
} // namespace

ACMD(do_assedit)
{
  if (char_is_npc(ch))
    return;

  for (descriptor_data *dd : each_desc(g))
    if (dd->connected == CON_ASSEDIT) {
      send_to_char(g, ch, "Assemblies are already being editted by someone.\r\n");
      return;
    }

  std::string_view rest     = argument;
  std::string buf           = take_arg(rest);
  std::string buf2          = take_arg(rest);
  struct descriptor_data *d = char_get_desc(ch);

  if (buf.empty() || !isdigit(buf[0])) {
    if (strncasecmp("new", buf.c_str(), 3) == 0 && !buf.empty()) {
      if (!isdigit(buf2[0]))
        usage(g, d);
      else if (!obj_proto_of(g, parse_int(buf2)))
        send_to_char(g, d->character, "You need to create the assembly object before you can create the new assembly.\r\n");
      else {
        assemblyCreate(g, parse_int(buf2), 0);
        send_to_char(g, d->character, "Assembly Created.\r\n");
        assemblySaveAssemblies(g);
      }
    } else if (strncasecmp("delete", buf.c_str(), 6) == 0 && !buf.empty() && isdigit(buf2[0])) {
      assemblyDestroy(g, parse_int(buf2));
      send_to_char(g, d->character, "Assembly Deleted.\r\n");
      assemblySaveAssemblies(g);
    } else
      usage(g, d);
    return;
  }

  open_session(g, d, std::make_unique<assembly_editor>(), {});
  assembly_editor &editor = session_of<assembly_editor>(d);

  ASSEMBLY *existing = assemblyGetAssemblyPtr(g, parse_int(buf));
  if (!existing) {
    send_to_char(g, d->character, "That assembly does not exist\r\n");
    leave_olc(g, d);
    return;
  }
  editor.vnum = existing->lVnum;
  editor.type = existing->uchAssemblyType;
  editor.components.assign(existing->pComponents, existing->pComponents + std::max(existing->lNumComponents, 0L));

  if (!obj_proto_of(g, editor.vnum)) {
    send_to_char(g, d->character, "Assembled item may not exist, check the vnum and assembles (show assemblies). \r\n");
    leave_olc(g, d); /* for right now we just get out! */
    return;
  }

  begin_editing(g, d, CON_ASSEDIT);
  editor.menu(g, d);
}
