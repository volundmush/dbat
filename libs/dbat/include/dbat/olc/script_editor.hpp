#pragma once
#include <string_view>
#include <vector>

#include "dbat/consts/types.hpp"

/*
 * The script editor: the submenu of redit, oedit and medit that lists the triggers a room, object or mobile starts
 * with, and adds and removes them. It edits its own copy of the list, which the owning editor puts into the world
 * when it saves, so leaving without saving leaves the prototype as it was.
 */
struct script_editor {
  enum class mode { main_menu, new_trigger, delete_trigger };

  int attach_type{};               /* MOB_TRIGGER, OBJ_TRIGGER or WLD_TRIGGER: what the triggers should be for */
  std::vector<trig_vnum> triggers; /* the list being edited */
  mode asking = mode::main_menu;

  /* Shows the list and its menu, and waits for a choice from it. */
  void menu(GameWorld *g, struct descriptor_data *d);

  /* A line typed at the submenu; false when the builder leaves it, for the owner's menu. */
  bool parse(GameWorld *g, struct descriptor_data *d, std::string_view line);

  /* The context help for what is being asked (a CONTEXT_* number). */
  int help_context() const;
};
