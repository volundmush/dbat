#include "act.misc.h"
#include "consts/maximums.h"

#include "interpreter.h"
#include "search.h"

#include "comm.h"

#include "character_api.h"
#include "character_db.h"
#include "character_macros.h"
#include "character_utils.h"

#include "consts/constates.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "obj_edit.h"
#include "object_macros.h"

#include "room_db.h"
#include "stringutils.h"
#include "util_macros.h"
#include "flags.h"
#include "search.hpp"
#include "iterate.hpp"

#include <cstdlib>

ACMD(do_restring) {

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int pay = 0;

  one_argument(argument, arg);

  if (char_room_vnum_get(ch) >= 178 && char_room_vnum_get(ch) <= 184) {
    pay = 5000;
    if (GET_GOLD(ch) < pay) {
      send_to_char(ch, "You need at least 5,000 zenni to initiate an equipment "
                       "restring.\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
      send_to_char(
          ch,
          "You don't have a that equipment to restring in your inventory.\r\n");
      send_to_char(ch, "Syntax: restring (obj name)\r\n");
      return;
    } else if (OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
      send_to_char(ch, "You can not restring a custom piece. Why? Because you "
                       "already restrung it you dummy.\r\n");
      return;
    } else {
      STATE(ch->desc) = CON_POBJ;
      char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH],
          thelong[MAX_INPUT_LENGTH];

      *thename = '\0';
      *theshort = '\0';
      *thelong = '\0';

      sprintf(thename, "%s", obj->name);
      sprintf(theshort, "%s", obj->short_description);
      sprintf(thelong, "%s", obj->description);

      ch->desc->obj_name = strdup(thename);
      ch->desc->obj_was = strdup(theshort);
      ch->desc->obj_short = strdup(theshort);
      ch->desc->obj_long = strdup(thelong);
      ch->desc->obj_point = obj;
      ch->desc->obj_type = 1;
      ch->desc->obj_weapon = 0;
      disp_restring_menu(ch->desc);
      ch->desc->obj_editflag = EDIT_RESTRING;
      ch->desc->obj_editval = EDIT_RESTRING_MAIN;
      return;
    }
  }
}

void handle_multi_merge(struct char_data *form) {
  struct char_data *ch = GET_ORIGINAL(form);

  if (ch == NULL)
    return;

  char_multiform_clone_set(form, NULL);  // clear original pointer to prevent re-entry

  send_to_char(ch, "@YYou merge with one of your forms!@n\r\n");
  act("@y$n@Y merges with one of his multiforms!@n\r\n", TRUE, ch, 0, 0,
      TO_ROOM);

  char_clone_remove(ch, form);
  if (char_clone_count(ch) == 0) {
    char_condition_remove(ch, "multiform_original", "merge");
  }
  // Caller is responsible for extracting form.
}
