/* show.c: showing objects and characters to a viewer
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <cstdlib>
#include <vector>

#include "dbat/iterate.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
#include "dbat/affect.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/maputils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/boards.hpp"
#include "dbat/screen.hpp"
#include "dbat/mail.hpp"
#include "dbat/guild.hpp"
#include "dbat/disabled.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

static void see_plant(GameWorld *g, struct obj_data *obj, struct char_data *ch);
static void list_one_char(GameWorld *g, struct char_data *i, struct char_data *ch);

static void see_plant(GameWorld *g, struct obj_data *obj, struct char_data *ch)
{

  int water = obj_get_value(obj, VAL_WATERLEVEL);

  if (water >= 0) {
    switch (obj_get_value(obj, VAL_MATURITY)) {
      case 0:
        send_to_char(g, ch, "@wA @G{}@y seed@w has been planted here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      case 1:
        send_to_char(g, ch, "@wA very young @G{}@w has sprouted from a planter here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      case 2:
        send_to_char(g, ch, "@wA half grown @G{}@w is in a planter here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      case 3:
        send_to_char(g, ch, "@wA mature @G{}@w is growing in a planter here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      case 4:
        send_to_char(g, ch, "@wA mature @G{}@w is flowering in a planter here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      case 5:
        send_to_char(g, ch, "@wA mature @G{}@w that is close to harvestable is here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      case 6:
        send_to_char(g, ch, "@wA @Rharvestable @G{}@w is in the planter here. @D(@C{:d} Water Hours@D)@n\r\n", obj_get_short_description(obj), water);
        break;
      default:
        break;
    }
  } else {
    if (water > -4) {
      send_to_char(g, ch, "@yA @G{}@y that is looking a bit @rdry@y, is here.@n\r\n", obj_get_short_description(obj));
    } else if (water > -10) {
      send_to_char(g, ch, "@yA @G{}@y that is looking extremely @rdry@y, is here.@n\r\n", obj_get_short_description(obj));
    } else if (water <= -10) {
      send_to_char(g, ch, "@yA @G{}@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n", obj_get_short_description(obj));
    }
  }
}

static constexpr std::string_view weapon_disp[6] = {
    "Sword",
    "Dagger",
    "Spear",
    "Club",
    "Gun",
    "Brawling"};

void display_spells(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  int i;

  send_to_char(g, ch, "The spellbook contains the following spells:\r\n");
  send_to_char(g, ch, "@c---@wSpell Name@c------------------------------------@w# of pages@c-----@n\r\n");

  if (!obj_get_sbinfo(obj)) {
    return;
  }
  for (i = 0; i < SPELLBOOK_SIZE; i++) {
    if (obj_get_sbinfo(obj)[i].spellname != 0) {
      if (obj_get_sbinfo(obj)[i].spellname > MAX_SPELLS) {
        continue;
      }
      send_to_char(g, ch, "@y{:<20}@n					[@R{:>2d}@n]\r\n", g->spell_info[obj_get_sbinfo(obj)[i].spellname].name, obj_get_sbinfo(obj)[i].pages);
    }
  }
  return;
}

void display_scroll(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  send_to_char(g, ch, "The scroll contains the following spell:\r\n");
  send_to_char(g, ch, "@c---@wSpell Name@c---------------------------------------------------@n\r\n");
  send_to_char(g, ch, "@y{:<20}@n\r\n", skill_name(g, obj_get_value(obj, VAL_SCROLL_SPELL1)));
  return;
}

void show_obj_to_char(GameWorld *g, struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    mud_log("SYSERR: NULL pointer in show_obj_to_char(): obj={} ch={}", as_pointer(obj), as_pointer(ch));
    /*  SYSERR_DESC:
     *  Somehow a NULL pointer was sent to show_obj_to_char() in either the
     *  'obj' or the 'ch' variable.  The error will indicate which was NULL
     *  be listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger.
     */
    return;
  }

  int spotted = false;

  if (char_get_skill(ch, SKILL_SPOT) > rand_number(g, 20, 110)) {
    spotted = true;
  }

  switch (mode) {
    case SHOW_OBJ_LONG:
      /*
     * hide objects starting with . from non-holylighted people 
     * Idea from Elaseth of TBA 
     */
      if (obj_get_description(obj).starts_with('.') && (char_is_npc(ch) || !char_prf_flagged(g, ch, pref_flag::holylight)))
        return;
      if (obj_get_type_flag(obj) == ITEM_VEHICLE && char_get_in_room(ch) == obj_get_value(obj, 0)) {
        return;
      }
      if (obj_get_sitting(g, obj) && char_get_admlevel(ch) < 1) {
        return;
      }
      if (obj_get_sitting(g, obj) && char_get_admlevel(ch) >= 1) {
        send_to_char(g, ch, "@D(@YBeing Used@D)@w");
      }
      if (obj_get_type_flag(obj) == ITEM_PLANT && (room_flagged(g, obj_get_in_room(obj), room_flag::garden1) || room_flagged(g, obj_get_in_room(obj), room_flag::garden2))) {
        see_plant(g, obj, ch);
        return;
      }
      if (obj_extra_flagged(obj, extra_flag::buried)) {
        std::string bury;
        if (!obj_is_corpse(obj)) {
          if (obj_get_weight(obj) < 10) {
            bury = "small mound of";
          } else if (obj_get_weight(obj) < 50) {
            bury = "medium sized mound of";
          } else if (obj_get_weight(obj) < 1000) {
            bury = "large mound of";
          } else {
            bury = "gigantic mound of";
          }
        } else {
          bury = "recent grave covered by";
        }
        if (spotted == true && room_sect(g, obj_get_in_room(obj)) != SECT_DESERT) {
          send_to_char(g, ch, "@yA {} soft dirt is here.@n\r\n", bury);
          return;
        }
        if (spotted == true && room_sect(g, obj_get_in_room(obj)) == SECT_DESERT) {
          send_to_char(g, ch, "@YA {} soft sand is here.@n\r\n", bury);
          return;
        }
        return;
      }
      if (obj_get_vnum(g, obj) == 11) {
        send_to_char(g, ch, "@wA gravity generator, set to {}x gravity, is built here", add_commas(obj_get_weight(obj)));
      } else if (obj_get_vnum(g, obj) == 79) {
        send_to_char(g, ch, "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C{}@D]@w is blocking access to the @G{}@w direction", add_commas(obj_get_weight(obj)), dirs[obj_get_cost(obj)]);
      } else {
        send_to_char(g, ch, "@w");
        if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::roomflags)) {
          if (obj_get_posted_to(g, obj) == nullptr) {
            send_to_char(g, ch, "@D[@G{:d}@D]@w ", obj_get_vnum(g, obj));
            if (obj_get_script(obj) && !obj_get_proto_script(obj).empty())
              send_to_char(g, ch, "@D[@wT{:d}@D]@w ", obj_get_proto_script(obj).front());
          } else {
            if (obj_get_posttype(obj) <= 0) {
              send_to_char(g, ch, "@D[@G{:d}@D]@w ", obj_get_vnum(g, obj));
              if (obj_get_script(obj) && !obj_get_proto_script(obj).empty())
                send_to_char(g, ch, "@D[@wT{:d}@D]@w ", obj_get_proto_script(obj).front());
            }
          }
        }

        if (obj_get_posttype(obj) > 0) {
          if (obj_get_posted_to(g, obj)) {
            return;
          }
          send_to_char(g, ch, "{}@w, has been posted here.@n", obj_get_short_description(obj));
        } else {
          if (!obj_extra_flagged(obj, extra_flag::buried)) {
            send_to_char(g, ch, "{}@n", obj_get_description(obj));
          }
        }

        if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
          if (!obj_val_flagged(obj, container_flag::closed) && obj_get_vnum(g, obj) > 19199)
            send_to_char(g, ch, "\r\n@c...its outer hatch is open@n");
          else if (!obj_val_flagged(obj, container_flag::closed) && obj_get_vnum(g, obj) <= 19199)
            send_to_char(g, ch, "\r\n@c...its door is open@n");
        }
        if (obj_get_type_flag(obj) == ITEM_CONTAINER && !obj_is_corpse(obj)) {
          if (!obj_val_flagged(obj, container_flag::closed) && !obj_extra_flagged(obj, extra_flag::sheath))
            send_to_char(g, ch, ". @D[@G-open-@D]@n");
          else if (!obj_extra_flagged(obj, extra_flag::sheath))
            send_to_char(g, ch, ". @D[@rclosed@D]@n");
        }
        if (obj_get_type_flag(obj) == ITEM_HATCH) {
          if (!obj_val_flagged(obj, container_flag::closed))
            send_to_char(g, ch, ", it is open");
          else if (obj_val_flagged(obj, container_flag::closed))
            send_to_char(g, ch, ", it is closed");
          if (obj_val_flagged(obj, container_flag::locked))
            send_to_char(g, ch, " and locked@n");
          else
            send_to_char(g, ch, "@n");
        }
        if (obj_get_type_flag(obj) == ITEM_FOOD) {
          if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
            send_to_char(g, ch, ", and it has been ate on@n");
          }
        }
      }
      break;

    case SHOW_OBJ_SHORT:
      if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::roomflags)) {
        send_to_char(g, ch, "[{:d}] ", obj_get_vnum(g, obj));
        if (obj_get_script(obj) && !obj_get_proto_script(obj).empty())
          send_to_char(g, ch, "[T{:d}] ", obj_get_proto_script(obj).front());
      }

      if (char_prf_flagged(g, ch, pref_flag::ihealth)) {
        send_to_char(g, ch, "@D<@gH@D: @C{:d}@D>@w {}", obj_get_value(obj, VAL_ALL_HEALTH), obj_get_short_description(obj));
      } else {
        send_to_char(g, ch, "{}", obj_get_short_description(obj));
      }
      if (obj_get_type_flag(obj) == ITEM_FOOD) {
        if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
          send_to_char(g, ch, ", and it has been ate on.@n");
        }
      }
      if (obj_get_vnum(g, obj) == 255) {
        switch (obj_get_value(obj, 0)) {
          case 0:
          case 1:
            send_to_char(g, ch, " @D[@wQuality @RC@D]@n");
            break;
          case 2:
            send_to_char(g, ch, " @D[@wQuality @RC+@D]@n");
            break;
          case 3:
            send_to_char(g, ch, " @D[@wQuality @yC++@D]@n");
            break;
          case 4:
            send_to_char(g, ch, " @D[@wQuality @yB@D]@n");
            break;
          case 5:
            send_to_char(g, ch, " @D[@wQuality @CB+@D]@n");
            break;
          case 6:
            send_to_char(g, ch, " @D[@wQuality @CB++@D]@n");
            break;
          case 7:
            send_to_char(g, ch, " @D[@wQuality @CA@D]@n");
            break;
          case 8:
            send_to_char(g, ch, " @D[@wQuality @GA+@D]@n");
            break;
        }
      }

      if (obj_get_vnum(g, obj) == 3424) {
        send_to_char(g, ch, " @D[@bInk Remaining@D: @w{:d}@D]@n", obj_get_value(obj, 6));
      }
      if (obj_get_vnum(g, obj) == 3423) {
        send_to_char(g, ch, " @D[@B{:d}@D/@B24 Inks@D]@n", obj_get_value(obj, 6));
      }
      if (obj_extra_flagged(obj, extra_flag::throw_)) {
        send_to_char(g, ch, " @D[@RThrow Only@D]@n");
      }
      if (obj_get_type_flag(obj) == ITEM_PLANT && !obj_extra_flagged(obj, extra_flag::mature)) {
        if (obj_get_value(obj, VAL_WATERLEVEL) < -9) {
          send_to_char(g, ch, "@D[@RDead@D]@n");
        } else {
          switch (obj_get_value(obj, VAL_MATURITY)) {
            case 0:
              send_to_char(g, ch, " @D[@ySeed@D]@n");
              break;
            case 1:
              send_to_char(g, ch, " @D[@GSprout@D]@n");
              break;
            case 2:
              send_to_char(g, ch, " @D[@GYoung@D]@n");
              break;
            case 3:
              send_to_char(g, ch, " @D[@GMature@D]@n");
              break;
            case 4:
              send_to_char(g, ch, " @D[@GBudding@D]@n");
              break;
            case 5:
              send_to_char(g, ch, "@D[@GClose Harvest@D]@n");
              break;
            case 6:
              send_to_char(g, ch, "@D[@gHarvest@D]@n");
              break;
          }
        }
      }
      if (obj_get_type_flag(obj) == ITEM_CONTAINER && !obj_is_corpse(obj)) {
        if (!obj_val_flagged(obj, container_flag::closed) && !obj_extra_flagged(obj, extra_flag::sheath))
          send_to_char(g, ch, " @D[@G-open-@D]@n");
        else if (!obj_extra_flagged(obj, extra_flag::sheath))
          send_to_char(g, ch, " @D[@rclosed@D]@n");
      }
      if (obj_extra_flagged(obj, extra_flag::duplicate)) {
        send_to_char(g, ch, " @D[@YDuplicate@D]@n");
      }
      break;

    case SHOW_OBJ_ACTION:
      switch (obj_get_type_flag(obj)) {
        case ITEM_NOTE:
          if (!obj_get_action_description(obj).empty()) {
            std::string notebuf = str_format("There is something written on it:\r\n\r\n{}", obj_get_action_description(obj));
            send_to_char(g, ch, notebuf);
          } else
            send_to_char(g, ch, "There appears to be nothing written on it.\r\n");
          return;

        case ITEM_BOARD:
          show_board(g, obj_get_vnum(g, obj), ch);
          break;

        case ITEM_CONTROL:
          send_to_char(g, ch, "@RFUEL@D: {}{}@n\r\n", obj_get_value(obj, 2) >= 200 ? "@G" : obj_get_value(obj, 2) >= 100 ? "@Y"
                                                                                                                         : "@r",
                       add_commas(obj_get_value(obj, 2)));
          break;

        case ITEM_DRINKCON:
          send_to_char(g, ch, "It looks like a drink container.\r\n");
          break;

        case ITEM_LIGHT:
          if (obj_get_value(obj, VAL_LIGHT_HOURS) == -1)
            send_to_char(g, ch, "Light Cycles left: Infinite\r\n");
          else
            send_to_char(g, ch, "Light Cycles left: [{:d}]\r\n", obj_get_value(obj, VAL_LIGHT_HOURS));
          break;

        case ITEM_FOOD:
          if (obj_get_foob(obj) >= 4) {
            if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj) / 4) {
              send_to_char(g, ch, "Condition of the food: Almost gone.\r\n");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj) / 2) {
              send_to_char(g, ch, "Condition of the food: Half Eaten.");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
              send_to_char(g, ch, "Condition of the food: Partially Eaten.");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) == obj_get_foob(obj)) {
              send_to_char(g, ch, "Condition of the food: Whole.");
            }
          } else if (obj_get_foob(obj) > 0) {
            if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
              send_to_char(g, ch, "Condition of the food: Almost gone.");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) == obj_get_foob(obj)) {
              send_to_char(g, ch, "Condition of the food: Whole.");
            }
          } else {
            send_to_char(g, ch, "Condition of the food: Insignificant.");
          }
          break;

        case ITEM_SPELLBOOK:
          send_to_char(g, ch, "It looks like an arcane tome.\r\n");
          display_spells(g, ch, obj);
          break;

        case ITEM_SCROLL:
          send_to_char(g, ch, "It looks like an arcane scroll.\r\n");
          display_scroll(g, ch, obj);
          break;

        case ITEM_VEHICLE:
          if (obj_get_vnum(g, obj) > 19199) {
            send_to_char(g, ch, "@YSyntax@D: @CUnlock hatch\r\n");
            send_to_char(g, ch, "@YSyntax@D: @COpen hatch\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CClose hatch\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CEnter hatch\r\n");
          } else {
            send_to_char(g, ch, "@YSyntax@D: @CUnlock door\r\n");
            send_to_char(g, ch, "@YSyntax@D: @COpen door\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CClose door\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CEnter door\r\n");
          }
          break;

        case ITEM_HATCH:
          if (obj_get_vnum(g, obj) > 19199) {
            send_to_char(g, ch, "@YSyntax@D: @CUnlock hatch\r\n");
            send_to_char(g, ch, "@YSyntax@D: @COpen hatch\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CClose hatch\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CLeave@n\r\n");
          } else {
            send_to_char(g, ch, "@YSyntax@D: @CUnlock door\r\n");
            send_to_char(g, ch, "@YSyntax@D: @COpen door\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CClose door\r\n");
            send_to_char(g, ch, "@YSyntax@D: @CEnter door\r\n");
          }
          break;

        case ITEM_WINDOW:
          look_out_window(g, ch, obj_get_name(obj));
          return;
          break;

        default:
          if (!obj_is_corpse(obj)) {
            send_to_char(g, ch, "You see nothing special..\r\n");
          } else {
            int mention = false;
            send_to_char(g, ch, "This corpse has ");

            if (obj_get_value(obj, VAL_CORPSE_HEAD) == 0) {
              send_to_char(g, ch, "no head,");
              mention = true;
            }

            if (obj_get_value(obj, VAL_CORPSE_RARM) == 0) {
              send_to_char(g, ch, "no right arm, ");
              mention = true;
            } else if (obj_get_value(obj, VAL_CORPSE_RARM) == 2) {
              send_to_char(g, ch, "a broken right arm, ");
              mention = true;
            }

            if (obj_get_value(obj, VAL_CORPSE_LARM) == 0) {
              send_to_char(g, ch, "no left arm, ");
              mention = true;
            } else if (obj_get_value(obj, VAL_CORPSE_LARM) == 2) {
              send_to_char(g, ch, "a broken left arm, ");
              mention = true;
            }

            if (obj_get_value(obj, VAL_CORPSE_RLEG) == 0) {
              send_to_char(g, ch, "no right leg, ");
              mention = true;
            } else if (obj_get_value(obj, VAL_CORPSE_RLEG) == 2) {
              send_to_char(g, ch, "a broken right leg, ");
              mention = true;
            }

            if (obj_get_value(obj, VAL_CORPSE_LLEG) == 0) {
              send_to_char(g, ch, "no left leg, ");
              mention = true;
            } else if (obj_get_value(obj, VAL_CORPSE_LLEG) == 2) {
              send_to_char(g, ch, "a broken left leg, ");
              mention = true;
            }

            if (mention == false) {
              send_to_char(g, ch, "nothing missing from it but life.");
            } else {
              send_to_char(g, ch, "and is dead.");
            }

            send_to_char(g, ch, "\r\n");
          }
          break;
      }

      if (obj_get_type_flag(obj) == ITEM_WEAPON) {
        int num = 0;
        if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
          num = 1;
        } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
          num = 0;
        } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
          num = 3;
        } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
          num = 2;
        } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
          num = 4;
        } else {
          num = 5;
        }
        send_to_char(g, ch, "The weapon type of {}@n is '{}'.\r\n", obj_get_short_description(obj), weapon_disp[num]);
        send_to_char(g, ch, "You could wield it {}.\r\n", wield_names[wield_type(get_size(ch), obj)]);
      }
      diag_obj_to_char(g, obj, ch);
      send_to_char(g, ch, "It appears to be made of {}, and weighs {}", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
      break;

    default:
      mud_log("SYSERR: Bad display mode ({:d}) in show_obj_to_char().", mode);
      /*  SYSERR_DESC:
     *  show_obj_to_char() has some predefined 'mode's (argument #3) to tell
     *  it what to display to the character when it is called.  If the mode
     *  is not one of these, it will output this error, and indicate what
     *  mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode.
     */
      return;
  }

  if ((show_obj_modifiers(g, obj, ch) || (mode != SHOW_OBJ_ACTION)))
    send_to_char(g, ch, "\r\n");
}

bool show_obj_modifiers(GameWorld *g, struct obj_data *obj, struct char_data *ch)
{
  bool found = false;

  if (obj_extra_flagged(obj, extra_flag::invisible)) {
    send_to_char(g, ch, " (invisible)");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::bless) && char_aff_flagged(ch, aff_flag::detect_align)) {
    send_to_char(g, ch, " ..It glows blue!");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::magic) && char_aff_flagged(ch, aff_flag::detect_magic)) {
    send_to_char(g, ch, " ..It glows yellow!");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::glow)) {
    send_to_char(g, ch, " @D(@GGlowing@D)@n");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::hot)) {
    send_to_char(g, ch, " @D(@RHOT@D)@n");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::hum)) {
    send_to_char(g, ch, " @D(@RHumming@D)@n");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::slot2)) {
    if (obj_extra_flagged(obj, extra_flag::slot_one) && !obj_extra_flagged(obj, extra_flag::slots_filled))
      send_to_char(g, ch, " @D[@m1/2 Tokens@D]@n");
    else if (obj_extra_flagged(obj, extra_flag::slots_filled))
      send_to_char(g, ch, " @D[@m2/2 Tokens@D]@n");
    else
      send_to_char(g, ch, " @D[@m0/2 Tokens@D]@n");
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::slot1)) {
    if (obj_extra_flagged(obj, extra_flag::slots_filled))
      send_to_char(g, ch, " @D[@m1/1 Tokens@D]@n");
    else
      send_to_char(g, ch, " @D[@m0/1 Tokens@D]@n");
    found = true;
  }
  if (obj_get_kicharge(obj) > 0) {
    int num = (obj_get_distance(obj) * 20) + rand_number(g, 1, 5);
    send_to_char(g, ch, " {:d} meters away", num);
    found = true;
  }
  if (obj_extra_flagged(obj, extra_flag::custom)) {
    send_to_char(g, ch, " @D(@YCUSTOM@D)@n");
  }
  if (obj_extra_flagged(obj, extra_flag::restring)) {
    send_to_char(g, ch, " @D(@R{}@D)@n", char_get_admlevel(ch) > 0 ? obj_get_name(obj) : "*");
  }
  if (obj_extra_flagged(obj, extra_flag::broken)) {
    if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL || obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL || obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
      send_to_char(g, ch, ", and appears to be twisted and broken.");
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
      send_to_char(g, ch, ", and is broken into hundreds of splinters.");
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
      send_to_char(g, ch, ", and is shattered on the ground.");
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
      send_to_char(g, ch, ", and is a pile of rubble.");
    } else {
      send_to_char(g, ch, ", and is broken.");
    }
    found = true;
  } else {
    if (obj_get_type_flag(obj) != ITEM_BOARD) {
      if (obj_get_type_flag(obj) != ITEM_CONTAINER) {
        send_to_char(g, ch, ".");
      }
      if (!char_is_npc(ch) && obj_get_posted_to(g, obj) && obj_get_posttype(obj) <= 0) {
        struct obj_data *obj2 = obj_get_posted_to(g, obj);
        std::string dvnum;
        dvnum = str_format("@D[@G{:d}@D] @w", obj_get_vnum(g, obj2));
        send_to_char(g, ch, "\n...{}{} has been posted to it.", char_prf_flagged(g, ch, pref_flag::roomflags) ? dvnum.c_str() : "", obj_get_short_description(obj2));
      }
    }
    found = true;
  }
  return found;
}

void list_obj_to_char(GameWorld *g, const id_list &list, struct char_data *ch, int mode, int show)
{
  struct obj_data *d;
  bool found = false;
  int num;

  /* Loop through all objects in the list */
  for (obj_data *i : obj_list(g, list)) {
    if (obj_get_description(i).empty())
      continue;
    if (str_eq_nocase(obj_get_description(i), "undefined"))
      continue;
    num = 0;
    d   = i;
    if (CONFIG_STACK_OBJS) {
      auto stacks = [&](obj_data *j) {
        return ((str_eq_nocase(obj_get_short_description(j), obj_get_short_description(i)) && str_eq_nocase(obj_get_description(j), obj_get_description(i))) && (obj_get_item_number(j) == obj_get_item_number(i)) && ((obj_extra_flagged(j, extra_flag::broken) && obj_extra_flagged(i, extra_flag::broken)) || (!obj_extra_flagged(j, extra_flag::broken) && !obj_extra_flagged(i, extra_flag::broken)))) &&
               ((!obj_get_sitting(g, j) && !obj_get_sitting(g, i))) &&
               (obj_get_value(j, 6) == obj_get_value(i, 6)) &&
               ((obj_get_type_flag(j) != ITEM_PLANT && obj_get_type_flag(i) != ITEM_PLANT) || (obj_get_type_flag(j) == ITEM_PLANT && obj_get_type_flag(i) == ITEM_PLANT && obj_get_value(j, VAL_MATURITY) == obj_get_value(i, VAL_MATURITY) && obj_get_value(j, VAL_WATERLEVEL) == obj_get_value(i, VAL_WATERLEVEL))) &&
               ((!obj_extra_flagged(j, extra_flag::duplicate) && !obj_extra_flagged(i, extra_flag::duplicate)) || (obj_extra_flagged(j, extra_flag::duplicate) && obj_extra_flagged(i, extra_flag::duplicate))) &&
               (obj_get_posttype(j) == 0 && obj_get_posttype(i) == 0) &&
               (!obj_get_fellow_wall(g, j) && !obj_get_fellow_wall(g, i)) &&
               ((obj_get_value(j, 0) == obj_get_value(i, 0) && obj_get_vnum(g, j) == 255 && obj_get_vnum(g, i) == 255) || (obj_get_vnum(g, j) != 255 && obj_get_vnum(g, i) != 255));
      };
      /* an earlier copy has already been shown, with the count */
      if (find_first(obj_list(g, list), [&](obj_data *j) { return j == i || stacks(j); }) != i)
        continue;
      for (obj_data *j : obj_list_from(g, list, i))
        if (stacks(j) && char_can_see_obj(g, ch, j)) {
          num++;
          if (d == i && !char_can_see_obj(g, ch, d))
            d = j;
        }
    }
    if ((char_can_see_obj(g, ch, d) && ((!obj_get_description(d).starts_with('.') && !obj_get_short_description(d).starts_with('.')) || char_prf_flagged(g, ch, pref_flag::holylight))) || (obj_get_type_flag(d) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(g, ch, "@D(@Rx@Y{:>2d}@D)@n ", num);
      show_obj_to_char(g, d, ch, mode);
      found = true;
    }
  }
  if (!found && show)
    send_to_char(g, ch, " Nothing.\r\n");
}

void diag_obj_to_char(GameWorld *g, struct obj_data *obj, struct char_data *ch)
{
  struct {
    int percent{};
    std::string_view text{};
  } diagnosis[] = {
      {100, "is in excellent condition."               },
      {90,  "has a few scuffs."                        },
      {75,  "has some small scuffs and scratches."     },
      {50,  "has quite a few scratches."               },
      {30,  "has some big nasty scrapes and scratches."},
      {15,  "looks pretty damaged."                    },
      {0,   "is in awful condition."                   },
      {-1,  "is in need of repair."                    },
  };
  int percent, ar_index;
  std::string_view objs = obj_shortdesc_for(g, obj, ch);

  if (obj_get_value(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * obj_get_value(obj, VAL_ALL_HEALTH)) / obj_get_value(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0; /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(g, ch, "\r\n{} {}\r\n", upper_first(objs), diagnosis[ar_index].text);
}

void diag_char_to_char(GameWorld *g, struct char_data *i, struct char_data *ch)
{
  struct {
    int percent{};
    std::string_view text{};
  } diagnosis[] = {
      {100, "@wis in @Gexcellent@w condition."                  },
      {90,  "@whas a few @Rscratches@w."                        },
      {80,  "@whas some small @Rwounds@w and @Rbruises@w."      },
      {70,  "@whas quite a few @Rwounds@w."                     },
      {60,  "@whas some big @rnasty wounds@w and @Rscratches@w."},
      {50,  "@wlooks pretty @rhurt@w."                          },
      {40,  "@wis mainly @rinjured@w."                          },
      {30,  "@wis a @rmess@w of @rinjuries@w."                  },
      {20,  "@wis @rstruggling@w to @msurvive@w."               },
      {10,  "@wis in @mawful condition@w."                      },
      {0,   "@Ris barely alive.@w"                              },
      {-1,  "@ris nearly dead.@w"                               },
  };
  int percent, ar_index;

  percent = (int)(clampHealth(char_get_health(i)) * 100.0);

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent) {
      send_to_char(g, ch, "{}\r\n", diagnosis[ar_index].text);
      return;
    }
}

void look_at_char(GameWorld *g, struct char_data *i, struct char_data *ch)
{
  int j, found;

  if (!char_get_desc(ch)) {
    return;
  }
  if (!char_get_ddesc(i).empty()) {
    send_to_char(g, ch, "{}", char_get_ddesc(i));
  }
  if (!char_mob_flagged(i, mob_flag::justdesc)) {
    bringdesc(g, ch, i);
  }
  send_to_char(g, ch, "\r\n");
  if (!char_is_npc(i)) {
    if (char_get_limb_condition(i, (1) - 1) >= 50 && !char_plr_flagged(i, player_flag::crarm)) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @G{:>2d}{}@D/@g100{}        @D]@n\r\n", char_get_limb_condition(i, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (1) - 1) > 0 && !char_plr_flagged(i, player_flag::crarm)) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @rBroken @y{:>2d}{}@D/@g100{} @D]@n\r\n", char_get_limb_condition(i, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (1) - 1) > 0 && char_plr_flagged(i, player_flag::crarm)) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @cCybernetic @G{:>2d}{}@D/@G100{}@D]@n\r\n", char_get_limb_condition(i, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (1) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
    }
    if (char_get_limb_condition(i, (2) - 1) >= 50 && !char_plr_flagged(i, player_flag::clarm)) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @G{:>2d}{}@D/@g100{}        @D]@n\r\n", char_get_limb_condition(i, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (2) - 1) > 0 && !char_plr_flagged(i, player_flag::clarm)) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @rBroken @y{:>2d}{}@D/@g100{} @D]@n\r\n", char_get_limb_condition(i, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (2) - 1) > 0 && char_plr_flagged(i, player_flag::clarm)) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @cCybernetic @G{:>2d}{}@D/@G100{}@D]@n\r\n", char_get_limb_condition(i, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (2) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
    }
    if (char_get_limb_condition(i, (3) - 1) >= 50 && !char_plr_flagged(i, player_flag::clarm)) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @G{:>2d}{}@D/@g100{}        @D]@n\r\n", char_get_limb_condition(i, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (3) - 1) > 0 && !char_plr_flagged(i, player_flag::crleg)) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @rBroken @y{:>2d}{}@D/@g100{} @D]@n\r\n", char_get_limb_condition(i, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (3) - 1) > 0 && char_plr_flagged(i, player_flag::crleg)) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @cCybernetic @G{:>2d}{}@D/@G100{}@D]@n\r\n", char_get_limb_condition(i, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (3) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
    }
    if (char_get_limb_condition(i, (4) - 1) >= 50 && !char_plr_flagged(i, player_flag::clleg)) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @G{:>2d}{}@D/@g100{}        @D]@n\r\n", char_get_limb_condition(i, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (4) - 1) > 0 && !char_plr_flagged(i, player_flag::clleg)) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @rBroken @y{:>2d}{}@D/@g100{} @D]@n\r\n", char_get_limb_condition(i, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (4) - 1) > 0 && char_plr_flagged(i, player_flag::clleg)) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @cCybernetic @G{:>2d}{}@D/@G100{}@D]@n\r\n", char_get_limb_condition(i, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (4) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
    }
    if (char_plr_flagged(i, player_flag::head)) {
      send_to_char(g, ch, "            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
    }
    if (!char_plr_flagged(i, player_flag::head)) {
      send_to_char(g, ch, "            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
    }
    if (((char_is_saiyan(i) || char_is_halfbreed(i)) && char_plr_flagged(i, player_flag::stail)) && !char_plr_flagged(i, player_flag::tailhide)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
    }
    if ((char_is_saiyan(i) || char_is_halfbreed(i)) && (!char_plr_flagged(i, player_flag::stail)) && (!char_plr_flagged(i, player_flag::tailhide))) {
      send_to_char(g, ch, "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
    }
    if ((char_is_icer(i) || char_is_bio(i)) && char_plr_flagged(i, player_flag::tail)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
    }
    if ((char_is_icer(i) || char_is_bio(i)) && !char_plr_flagged(i, player_flag::tail)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
    }
  }
  send_to_char(g, ch, "\r\n");
  if (!char_is_npc(i)) { /* clans are gone */
    send_to_char(g, ch, "            @D[@mClan        @D: @W{:<20}@D]@n\r\n", "None.");
  }
  if (!char_is_npc(i)) {
    send_to_char(g, ch, "\r\n         @D----------------------------------------@n\r\n");
    trans_check(g, ch, i);
    send_to_char(g, ch, "         @D----------------------------------------@n\r\n");
  }
  send_to_char(g, ch, "\r\n");

  if ((!char_plr_flagged(i, player_flag::disguised) && (readIntro(ch, i) == 1 && !char_is_npc(i)))) {
    if (char_get_sex(i) == SEX_NEUTRAL)
      send_to_char(g, ch, "{} appears to be {} {}, ", get_i_name(ch, i), AN(juggleRaceName(i, true)), juggleRaceName(i, false));
    else
      send_to_char(g, ch, "{} appears to be {} {} {}, ", get_i_name(ch, i), AN(char_mafe(i)), char_mafe(i), juggleRaceName(i, false));
  } else if (ch == i || char_is_npc(i)) {
    if (char_get_sex(i) == SEX_NEUTRAL)
      send_to_char(g, ch, "{} appears to be {} {}, ", upper_first(char_get_name(i)), AN(juggleRaceName(i, true)), juggleRaceName(i, false));
    else
      send_to_char(g, ch, "{} appears to be {} {} {}, ", upper_first(char_get_name(i)), AN(char_mafe(i)), char_mafe(i), juggleRaceName(i, false));
  } else {
    if (char_get_sex(i) == SEX_NEUTRAL)
      send_to_char(g, ch, "Appears to be {} {}, ", AN(juggleRaceName(i, true)), juggleRaceName(i, false));
    else
      send_to_char(g, ch, "Appears to be {} {} {}, ", AN(char_mafe(i)), char_mafe(i), juggleRaceName(i, false));
  }

  if (char_is_npc(i)) {
    send_to_char(g, ch, "is {} sized, and\r\n", size_names[get_size(i)]);
  }
  if (!char_is_npc(i)) {
    if (!char_plr_flagged(i, player_flag::oozaru) && (!char_is_icer(i) || !char_is_transformed(i)) && char_get_genome(i, 0) < 11) {
      send_to_char(g, ch, "is {} sized, about {:d}cm tall,\r\nabout {:d}kg heavy,", size_names[get_size(i)], char_get_pc_height(g, i), char_get_pc_weight(g, i));
    } else if (char_is_icer(i) && char_plr_flagged(i, player_flag::trans1)) {
      int num1 = char_get_pc_height(g, i) * 3;
      int num2 = char_get_pc_weight(g, i) * 4;
      send_to_char(g, ch, "is {} sized, about {:d}cm tall,\r\nabout {:d}kg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_is_icer(i) && char_plr_flagged(i, player_flag::trans2)) {
      int num1 = char_get_pc_height(g, i) * 3;
      int num2 = char_get_pc_weight(g, i) * 4;
      send_to_char(g, ch, "is {} sized, about {:d}cm tall,\r\nabout {:d}kg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_is_icer(i) && char_plr_flagged(i, player_flag::trans3)) {
      int num1 = char_get_pc_height(g, i) * 1.5;
      int num2 = char_get_pc_weight(g, i) * 2;
      send_to_char(g, ch, "is {} sized, about {:d}cm tall,\r\nabout {:d}kg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_is_icer(i) && char_plr_flagged(i, player_flag::trans4)) {
      int num1 = char_get_pc_height(g, i) * 2;
      int num2 = char_get_pc_weight(g, i) * 3;
      send_to_char(g, ch, "is {} sized, about {:d}cm tall,\r\nabout {:d}kg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_plr_flagged(i, player_flag::oozaru) || char_get_genome(i, 0) == 11) {
      int num1 = char_get_pc_height(g, i) * 10;
      int num2 = char_get_pc_weight(g, i) * 50;
      send_to_char(g, ch, "is {} sized, about {:d}cm tall,\r\nabout {:d}kg heavy,", size_names[get_size(i)], num1, num2);
    }
    if (i == ch) {
      send_to_char(g, ch, " and ");
    } else if (char_get_age_years(g, ch) >= char_get_age_years(g, i) + 30) {
      send_to_char(g, ch, " appears to be very much younger than you, and ");
    } else if (char_get_age_years(g, ch) >= char_get_age_years(g, i) + 25) {
      send_to_char(g, ch, " appears to be much younger than you, and ");
    } else if (char_get_age_years(g, ch) >= char_get_age_years(g, i) + 15) {
      send_to_char(g, ch, " appears to be a good amount younger than you, and ");
    } else if (char_get_age_years(g, ch) >= char_get_age_years(g, i) + 10) {
      send_to_char(g, ch, " appears to be about a decade younger than you, and ");
    } else if (char_get_age_years(g, ch) >= char_get_age_years(g, i) + 5) {
      send_to_char(g, ch, " appears to be several years younger than you, and ");
    } else if (char_get_age_years(g, ch) >= char_get_age_years(g, i) + 2) {
      send_to_char(g, ch, " appears to be a bit younger than you, and ");
    } else if (char_get_age_years(g, ch) > char_get_age_years(g, i)) {
      send_to_char(g, ch, " appears to be slightly younger than you, and ");
    } else if (char_get_age_years(g, ch) == char_get_age_years(g, i)) {
      send_to_char(g, ch, " appears to be the same age as you, and ");
    }
    if (char_get_age_years(g, i) >= char_get_age_years(g, ch) + 30) {
      send_to_char(g, ch, " appears to be very much older than you, and ");
    } else if (char_get_age_years(g, i) >= char_get_age_years(g, ch) + 25) {
      send_to_char(g, ch, " appears to be much older than you, and ");
    } else if (char_get_age_years(g, i) >= char_get_age_years(g, ch) + 15) {
      send_to_char(g, ch, " appears to be a good amount older than you, and ");
    } else if (char_get_age_years(g, i) >= char_get_age_years(g, ch) + 10) {
      send_to_char(g, ch, " appears to be about a decade older than you, and ");
    } else if (char_get_age_years(g, i) >= char_get_age_years(g, ch) + 5) {
      send_to_char(g, ch, " appears to be several years older than you, and ");
    } else if (char_get_age_years(g, i) >= char_get_age_years(g, ch) + 2) {
      send_to_char(g, ch, " appears to be a bit older than you, and ");
    } else if (char_get_age_years(g, i) > char_get_age_years(g, ch)) {
      send_to_char(g, ch, " appears to be slightly older than you, and ");
    }
  }
  diag_char_to_char(g, i, ch);
  found = false;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (char_get_equipment(i, j) && char_can_see_obj(g, ch, char_get_equipment(i, j)))
      found = true;

  if (found && (!char_is_npc(ch) && !char_prf_flagged(g, ch, pref_flag::noeqsee))) {
    send_to_char(g, ch, "\r\n"); /* act() does capitalization. */
    if (!char_plr_flagged(i, player_flag::disguised)) {
      act(g, "$n is using:", false, i, nullptr, ch, act_to::vict);
    } else {
      act(g, "The disguised person is using:", false, i, nullptr, ch, act_to::vict);
    }
    for (j = 0; j < NUM_WEARS; j++)
      if (char_get_equipment(i, j) && char_can_see_obj(g, ch, char_get_equipment(i, j)) && (j != WEAR_WIELD1 && j != WEAR_WIELD2)) {
        send_to_char(g, ch, "{}", wear_where[j]);
        show_obj_to_char(g, char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
        if (obj_extra_flagged(char_get_equipment(i, j), extra_flag::sheath)) {
          struct obj_data *sheath = char_get_equipment(i, j);
          for (obj_data *thing : obj_contents(g, sheath)) {
            if (thing) {
              send_to_char(g, ch, "@D  ---- @YSheathed@D ----@c> @n");
              show_obj_to_char(g, thing, ch, SHOW_OBJ_SHORT);
            }
          }
        }
      } else if (char_get_equipment(i, j) && char_can_see_obj(g, ch, char_get_equipment(i, j)) && (!char_plr_flagged(i, player_flag::thandw))) {
        send_to_char(g, ch, "{}", wear_where[j]);
        show_obj_to_char(g, char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
        if (obj_extra_flagged(char_get_equipment(i, j), extra_flag::sheath)) {
          struct obj_data *sheath = char_get_equipment(i, j);
          for (obj_data *item : obj_contents(g, sheath)) {
            if (item) {
              send_to_char(g, ch, "@D  ---- @YSheathed@D ----@c> @n");
              show_obj_to_char(g, item, ch, SHOW_OBJ_SHORT);
            }
          }
        }
      } else if (char_get_equipment(i, j) && char_can_see_obj(g, ch, char_get_equipment(i, j)) && (char_plr_flagged(i, player_flag::thandw))) {
        send_to_char(g, ch, "@c<@CWielded by B. Hands@c>@n ");
        show_obj_to_char(g, char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
      }
  }
  if (ch != i && ((char_get_skill(ch, SKILL_KEEN) && char_aff_flagged(ch, aff_flag::sneak)) || char_get_admlevel(ch))) {
    found = false;
    act(g, "\r\nYou attempt to peek at $s inventory:", false, i, nullptr, ch, act_to::vict);
    if (char_can_see(g, i, ch))
      act(g, "$n tries to evaluate what you have in your inventory.", true, ch, nullptr, i, act_to::vict);
    if (char_get_skill(ch, SKILL_KEEN) > axion_dice(g, 0) && (!char_is_npc(i) || char_get_admlevel(ch) > 1)) {
      for (obj_data *tmp_obj : char_inventory(g, i)) {
        if (char_can_see_obj(g, ch, tmp_obj) &&
            (char_adm_flagged(ch, admin_flag::seeinv) || (rand_number(g, 0, 20) < char_get_level(ch)))) {
          show_obj_to_char(g, tmp_obj, ch, SHOW_OBJ_SHORT);
          found = true;
        }
      }
      improve_skill(g, ch, SKILL_KEEN, 1);
    } else if (char_is_npc(i) && char_get_admlevel(ch) < 2) {
      return;
    } else {
      act(g, "You are unsure about $s inventory.", false, i, nullptr, ch, act_to::vict);
      if (char_can_see(g, i, ch))
        act(g, "$n didn't seem to get a good enough look.", true, ch, nullptr, i, act_to::vict);
      improve_skill(g, ch, SKILL_KEEN, 1);
      return;
    }
    if (!found) {
      send_to_char(g, ch, "You can't see anything.\r\n");
      improve_skill(g, ch, SKILL_KEEN, 1);
    }
  }
}

static void list_one_char(GameWorld *g, struct char_data *i, struct char_data *ch)
{
  struct obj_data *chair  = nullptr;
  int count               = false;
  static constexpr std::string_view positions[] = {
      " is dead",
      " is mortally wounded",
      " is lying here, incapacitated",
      " is lying here, stunned",
      " is sleeping here",
      " is resting here",
      " is sitting here",
      "!FIGHTING!",
      " is standing here"};

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::roomflags) && char_is_npc(i))
    send_to_char(g, ch, "@D[@G{:d}@D]@w {}", char_get_mob_vnum(g, i), char_get_script(i) ? "[TRIG] " : "");

  if (char_is_npc(i) && !char_get_ldesc(i).empty() && char_get_position(i) == char_get_mob_specials_default_pos(i) && !char_get_fighting(g, i)) {
    send_to_char(g, ch, "{}", char_get_ldesc(i));

    if (char_is_npc(i)) {
      double health = char_get_health(i);
      if (health <= 0.1) {
        act(g, "@R...Should be DEAD soon.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.2) {
        act(g, "@R...Is on $s last leg.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.3) {
        act(g, "@R...Is absolutely covered in wounds.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.4) {
        act(g, "@R...$s body is in terrible shape.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.5) {
        act(g, "@R...Blood is seeping from the wounds on $s body.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.6) {
        act(g, "@R...Horrible wounds on $s body.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.7) {
        act(g, "@R...Quite a few wounds on $s body.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.8) {
        act(g, "@R...Many wounds on $s body.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health <= 0.9) {
        act(g, "@R...A few wounds on $s body.@w", true, i, nullptr, ch, act_to::vict);
      } else if (health < 1.0) {
        act(g, "@R...Some slight wounds on $s body.@w", true, i, nullptr, ch, act_to::vict);
      }
    }

    if (char_get_listenroom(i) > 0) {
      std::string eaves = str_format("@w...$e is spying on everything to the @c{}@w.", dirs[char_get_eavesdir(i)]);
      act(g, eaves, true, i, nullptr, ch, act_to::vict);
    }
    if (char_aff_flagged(i, aff_flag::flying) && char_get_altitude(i) == 1)
      act(g, "...$e is in the air!", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::flying) && char_get_altitude(i) == 2)
      act(g, "...$e is high in the air!", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::sanctuary) && !char_get_skill(i, SKILL_AQUA_BARRIER))
      act(g, "...$e has a barrier around $s body!", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::fireshield))
      act(g, "...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::sanctuary) && char_get_skill(i, SKILL_AQUA_BARRIER))
      act(g, "...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!", false, i, nullptr, ch, act_to::vict);
    if (!char_is_npc(i) && char_plr_flagged(i, player_flag::spiral))
      act(g, "...$e is spinning in a vortex!", false, i, nullptr, ch, act_to::vict);
    if (char_get_charge(i))
      act(g, "...$e has a bright %s aura around $s body!", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::metamorph))
      act(g, "@w...$e has a dark, @rred@w aura and menacing presence.", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::hayasa))
      act(g, "@w...$e has a soft @cblue@w glow around $s body!", false, i, nullptr, ch, act_to::vict);
    if (char_aff_flagged(i, aff_flag::blind))
      act(g, "...$e is groping around blindly!", false, i, nullptr, ch, act_to::vict);
    if (affected_by_spell(i, SPELL_FAERIE_FIRE))
      act(g, "@m...$e @mis outlined with purple fire!@m", false, i, nullptr, ch, act_to::vict);
    if (!char_get_feature(i).empty()) {
      std::string woo = str_format("@C{}@n", char_get_feature(i));
      act(g, woo, false, i, nullptr, ch, act_to::vict);
    }

    return;
  }

  if (char_is_npc(i) && !char_get_fighting(g, i) && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(g, ch, "@w{}", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i) && char_get_grappled(g, i) && char_get_grappled(g, i) == ch)
    send_to_char(g, ch, "@w{} is being grappled with by YOU!", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i) && char_get_grappled(g, i) && char_get_grappled(g, i) != ch)
    send_to_char(g, ch, "@w{} is being absorbed from by {}!", upper_first(char_get_sdesc(i)), readIntro(ch, char_get_grappled(g, i)) == 1 ? get_i_name(ch, char_get_grappled(g, i)) : AN(juggleRaceName(char_get_grappled(g, i), true)));
  else if (char_is_npc(i) && char_get_absorbby(g, i) && char_get_absorbby(g, i) == ch)
    send_to_char(g, ch, "@w{} is being absorbed from by YOU!", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i) && char_get_absorbby(g, i) && char_get_absorbby(g, i) != ch)
    send_to_char(g, ch, "@w{} is being absorbed from by {}!", upper_first(char_get_sdesc(i)), readIntro(ch, char_get_absorbby(g, i)) == 1 ? get_i_name(ch, char_get_absorbby(g, i)) : AN(juggleRaceName(char_get_absorbby(g, i), true)));
  else if (char_is_npc(i) && char_get_fighting(g, i) && char_get_fighting(g, i) != ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING && is_sparring(i))
    send_to_char(g, ch, "@w{} is sparring with {}!", upper_first(char_get_sdesc(i)), char_get_admlevel(ch) ? char_get_name(char_get_fighting(g, i)) : (readIntro(ch, char_get_fighting(g, i)) == 1 ? get_i_name(ch, char_get_fighting(g, i)) : juggleRaceName(char_get_fighting(g, i), false)));
  else if (char_is_npc(i) && char_get_fighting(g, i) && is_sparring(i) && char_get_fighting(g, i) == ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(g, ch, "@w{} is sparring with you!", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i) && char_get_fighting(g, i) && char_get_fighting(g, i) != ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(g, ch, "@w{} is fighting {}!", upper_first(char_get_sdesc(i)), char_get_admlevel(ch) ? char_get_name(char_get_fighting(g, i)) : (readIntro(ch, char_get_fighting(g, i)) == 1 ? get_i_name(ch, char_get_fighting(g, i)) : juggleRaceName(char_get_fighting(g, i), false)));
  else if (char_is_npc(i) && char_get_fighting(g, i) && char_get_fighting(g, i) == ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(g, ch, "@w{} is fighting YOU!", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i) && char_get_fighting(g, i) && char_get_position(i) == POS_SITTING)
    send_to_char(g, ch, "@w{} is sitting here.", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i) && char_get_fighting(g, i) && char_get_position(i) == POS_SLEEPING)
    send_to_char(g, ch, "@w{} is sleeping here.", upper_first(char_get_sdesc(i)));
  else if (char_is_npc(i))
    send_to_char(g, ch, "@w{}", upper_first(char_get_sdesc(i)));
  else if (!char_is_npc(i)) {
    if (char_is_majin(i) && char_aff_flagged(i, aff_flag::liquefied)) {
      send_to_char(g, ch, "@wSeveral blobs of {} colored goo spread out here.@n\n", skin_types[(int)char_get_skin(i)]);
      return;
    }
    if ((char_get_admlevel(ch) > 0 || char_get_admlevel(i) > 0) || char_is_npc(ch)) {
      send_to_char(g, ch, "@w{}", char_get_alias(i));
    } else if ((!char_plr_flagged(i, player_flag::disguised) && readIntro(ch, i) == 1)) {
      send_to_char(g, ch, "@w{}", get_i_name(ch, i));
    } else if (!char_plr_flagged(i, player_flag::disguised) && readIntro(ch, i) != 1) {
      if (char_get_distfea(i) == DISTFEA_EYE) {
        send_to_char(g, ch, "@wA {} eyed {} {}", eye_types[(int)char_get_eye(i)], char_mafe(i), juggleRaceName(i, false));
      } else if (char_get_distfea(i) == DISTFEA_HAIR) {
        if (char_is_majin(i)) {
          send_to_char(g, ch, "@wA {} majin, with a {} forelock,", char_mafe(i), FHA_types[(int)char_get_hairl(i)]);
        } else if (char_is_namek(i)) {
          send_to_char(g, ch, "@wA namek, with {} antennae,", FHA_types[(int)char_get_hairl(i)]);
        } else if (char_is_arlian(i)) {
          send_to_char(g, ch, "@wA arlian, with {} antennae,", FHA_types[(int)char_get_hairl(i)]);
        } else if (char_is_icer(i) || char_is_demon(i)) {
          send_to_char(g, ch, "@wA {} {}, with {} horns", char_mafe(i), juggleRaceName(i, false), FHA_types[(int)char_get_hairl(i)]);
        } else {
          std::string blarg = str_format("{} {} hair {}", hairl_types[(int)char_get_hairl(i)], hairc_types[(int)char_get_hairc(i)], hairs_types[(int)char_get_hairs(i)]);
          send_to_char(g, ch, "@wA {} {}, with {}", char_mafe(i), juggleRaceName(i, false), char_get_hairl(i) == 0 ? "a bald head" : (blarg));
        }
      } else if (char_get_distfea(i) == DISTFEA_SKIN) {
        send_to_char(g, ch, "@wA {} skinned {} {}", skin_types[(int)char_get_skin(i)], char_mafe(i), juggleRaceName(i, false));
      } else if (char_get_distfea(i) == DISTFEA_HEIGHT) {
        std::string_view height;
        if (char_is_truffle(i)) {
          if (char_get_pc_height(g, i) > 70) {
            height = "very tall";
          } else if (char_get_pc_height(g, i) > 55) {
            height = "tall";
          } else if (char_get_pc_height(g, i) > 35) {
            height = "average height";
          } else {
            height = "short";
          }
        } else if (char_plr_flagged(i, player_flag::oozaru) || char_get_genome(i, 0) == 11) {
          if (char_get_pc_height(g, i) * 10 > 2000) {
            height = "very tall";
          } else if (char_get_pc_height(g, i) * 10 > 1800) {
            height = "tall";
          } else if (char_get_pc_height(g, i) * 10 > 1500) {
            height = "average height";
          } else {
            height = "short";
          }
        } else {
          if (char_get_pc_height(g, i) > 200) {
            height = "very tall";
          } else if (char_get_pc_height(g, i) > 180) {
            height = "tall";
          } else if (char_get_pc_height(g, i) > 150) {
            height = "average height";
          } else if (char_get_pc_height(g, i) > 120) {
            height = "short";
          } else {
            height = "very short";
          }
        }
        send_to_char(g, ch, "@wA {} {} {}", height, char_mafe(i), juggleRaceName(i, false));
      } else if (char_get_distfea(i) == DISTFEA_WEIGHT) {
        std::string_view height;
        if (char_is_truffle(i)) {
          if (char_get_pc_weight(g, i) > 35) {
            height = "very heavy";
          } else if (char_get_pc_weight(g, i) > 25) {
            height = "heavy";
          } else if (char_get_pc_weight(g, i) > 15) {
            height = "average weight";
          } else {
            height = "welterweight";
          }
        } else if (char_plr_flagged(i, player_flag::oozaru) || char_get_genome(i, 0) == 11) {
          if (char_get_pc_weight(g, i) * 50 > 6000) {
            height = "very heavy";
          } else if (char_get_pc_weight(g, i) * 50 > 5000) {
            height = "heavy";
          } else if (char_get_pc_weight(g, i) * 50 > 4000) {
            height = "average weight";
          } else if (char_get_pc_weight(g, i) * 50 > 3000) {
            height = "lightweight";
          } else {
            height = "welterweight";
          }
        } else {
          if (char_get_pc_weight(g, i) > 120) {
            height = "very heavy";
          } else if (char_get_pc_weight(g, i) > 100) {
            height = "heavy";
          } else if (char_get_pc_weight(g, i) > 80) {
            height = "average weight";
          } else if (char_get_pc_weight(g, i) > 60) {
            height = "lightweight";
          } else {
            height = "welterweight";
          }
        }
        send_to_char(g, ch, "@wA {} {} {}", height, char_mafe(i), juggleRaceName(i, false));
      }
    } else {
      send_to_char(g, ch, "@wA disguised {} {}", char_mafe(i), juggleRaceName(i, false));
    }
  }

  if (!char_is_npc(i) || !char_get_fighting(g, i)) {
    if (char_aff_flagged(i, aff_flag::invisible)) {
      send_to_char(g, ch, ", is invisible");
      count = true;
    }
    if (char_aff_flagged(i, aff_flag::ethereal)) {
      send_to_char(g, ch, ", has a halo");
      count = true;
    }
    if (char_aff_flagged(i, aff_flag::hide) && i != ch) {
      send_to_char(g, ch, ", is hiding");
      if (char_get_skill(i, SKILL_HIDE) && !char_is_npc(ch) && i != ch) {
        improve_skill(g, i, SKILL_HIDE, 1);
      }
      count = true;
    }
    if (!char_is_npc(i) && !char_get_desc(i)) {
      send_to_char(g, ch, ", has a blank stare");
      count = true;
    }
    if (!char_is_npc(i) && char_plr_flagged(i, player_flag::writing)) {
      send_to_char(g, ch, ", is writing");
      count = true;
    }
    if (!char_is_npc(i) && char_prf_flagged(g, i, pref_flag::buildwalk)) {
      send_to_char(g, ch, ", is buildwalking");
      count = true;
    }
    if (!char_is_npc(i) && char_get_absorbing(g, i) && char_get_absorbing(g, i) != ch) {
      send_to_char(g, ch, ", is absorbing from {}", char_get_name(char_get_absorbing(g, i)));
      count = true;
    }
    if (!char_is_npc(i) && char_get_grappling(g, i) && char_get_grappling(g, i) != ch) {
      send_to_char(g, ch, ", is grappling with {}", readIntro(ch, char_get_grappling(g, i)) == 1 ? get_i_name(ch, char_get_grappling(g, i)) : introd_calc(char_get_grappling(g, i)));
      count = true;
    }
    if (!char_is_npc(i) && char_get_carrying(g, i) && char_get_carrying(g, i) != ch) {
      send_to_char(g, ch, ", is carrying {}", readIntro(ch, char_get_carrying(g, i)) == 1 ? get_i_name(ch, char_get_carrying(g, i)) : introd_calc(char_get_carrying(g, i)));
      count = true;
    }
    if (!char_is_npc(i) && char_get_carried_by(g, i) && char_get_carried_by(g, i) != ch) {
      send_to_char(g, ch, ", is being carried by {}", readIntro(ch, char_get_carried_by(g, i)) == 1 ? get_i_name(ch, char_get_carried_by(g, i)) : introd_calc(char_get_carried_by(g, i)));
      count = true;
    }
    if (!char_is_npc(i) && char_get_grappling(g, i) && char_get_grappling(g, i) == ch) {
      send_to_char(g, ch, ", is grappling with YOU");
      count = true;
    }
    if (!char_is_npc(i) && char_get_absorbing(g, i) && char_get_absorbing(g, i) == ch) {
      send_to_char(g, ch, ", is absorbing from YOU");
      count = true;
    }
    if (!char_is_npc(i) && char_get_absorbing(g, ch) && char_get_absorbing(g, ch) == i) {
      send_to_char(g, ch, ", is being absorbed from by YOU");
      count = true;
    }
    if (!char_is_npc(i) && char_get_grappling(g, ch) && char_get_grappling(g, ch) == i) {
      send_to_char(g, ch, ", is being grappled with by YOU");
      count = true;
    }
    if (!char_is_npc(i) && char_get_carrying(g, ch) && char_get_carrying(g, ch) == i) {
      send_to_char(g, ch, ", is being carried by you");
      count = true;
    }
    if (!char_is_npc(ch) && !char_is_npc(i) && char_get_fighting(g, i)) {
      if (!char_plr_flagged(i, player_flag::spar) || (char_plr_flagged(i, player_flag::spar) && (!char_plr_flagged(char_get_fighting(g, i), player_flag::spar) || char_is_npc(char_get_fighting(g, i))))) {
        send_to_char(g, ch, ", is here fighting ");
      }
      if (char_plr_flagged(i, player_flag::spar) && char_plr_flagged(char_get_fighting(g, i), player_flag::spar)) {
        send_to_char(g, ch, ", is here sparring ");
      }
      if (char_get_fighting(g, i) == ch) {
        send_to_char(g, ch, "@rYOU@w");
        count = true;
      } else {
        if (char_get_in_room(i) == char_get_in_room(char_get_fighting(g, i))) {
          send_to_char(g, ch, "{}", char_get_admlevel(ch) ? char_get_name(char_get_fighting(g, i)) : (readIntro(ch, char_get_fighting(g, i)) == 1 ? get_i_name(ch, char_get_fighting(g, i)) : juggleRaceName(char_get_fighting(g, i), false)));
          count = true;
        } else {
          send_to_char(g, ch, "someone who has already left!");
        }
      }
    }
  }
  if (char_get_sits(g, i)) {
    chair = char_get_sits(g, i);
    if (char_plr_flagged(i, player_flag::healt)) {
      send_to_char(g, ch, "@w is floating inside a healing tank.");
    } else if (count == true) {
      send_to_char(g, ch, ",@w and{} on {}.", positions[(int)char_get_position(i)], obj_get_short_description(chair));
    } else if (count == false) {
      send_to_char(g, ch, "@w{} on {}.", positions[(int)char_get_position(i)], obj_get_short_description(chair));
    }
  } else if (!char_plr_flagged(i, player_flag::piloting) && !char_get_sits(g, i) && (!char_is_npc(i) || !char_get_fighting(g, i))) {
    if (count == true) {
      send_to_char(g, ch, "@w, and{}.", positions[(int)char_get_position(i)]);
    }
    if (count == false) {
      send_to_char(g, ch, "@w{}.", positions[(int)char_get_position(i)]);
    }
  } else if (char_plr_flagged(i, player_flag::piloting)) {
    send_to_char(g, ch, "@w, is sitting in the pilot's chair.\r\n");
  } else {

    if (char_get_fighting(g, i) && !char_is_npc(ch) && !char_is_npc(i)) {
      if (!char_plr_flagged(i, player_flag::spar)) {
        send_to_char(g, ch, ", is here fighting ");
      }
      if (char_plr_flagged(i, player_flag::spar)) {
        send_to_char(g, ch, ", is here sparring ");
      }
      if (char_get_fighting(g, i) == ch)
        send_to_char(g, ch, "@rYOU@w!");
      else {
        if (char_get_in_room(i) == char_get_in_room(char_get_fighting(g, i)))
          send_to_char(g, ch, "{}!", char_get_admlevel(ch) ? char_get_name(char_get_fighting(g, i)) : (readIntro(ch, char_get_fighting(g, i)) == 1 ? get_i_name(ch, char_get_fighting(g, i)) : juggleRaceName(char_get_fighting(g, i), false)));
        else
          send_to_char(g, ch, "someone who has already left!");
      }
    } else if (!char_is_npc(i)) { /* NIL fighting pointer */
      send_to_char(g, ch, " is here struggling with thin air.");
    }
  }

  if (char_aff_flagged(ch, aff_flag::detect_align)) {
    if (char_is_evil(i))
      send_to_char(g, ch, " (@rRed@[3] Aura)");
    else if (char_is_good(i))
      send_to_char(g, ch, " (@bBlue@[3] Aura)");
  }
  if (!char_is_npc(i) && char_prf_flagged(g, i, pref_flag::afk))
    send_to_char(g, ch, " @D(@RAFK@D)");
  else if (!char_is_npc(i) && char_get_timer(i) > 3)
    send_to_char(g, ch, " @D(@RIDLE@D)");
  send_to_char(g, ch, "@n\r\n");

  if (char_get_listenroom(i) > 0) {
    std::string eaves = str_format("@w...$e is spying on everything to the @c{}@w.", dirs[char_get_eavesdir(i)]);
    act(g, eaves, true, i, nullptr, ch, act_to::vict);
  }
  if (!char_is_npc(i)) {
    if (char_plr_flagged(i, player_flag::fishing)) {
      act(g, "@w...$e is @Cfishing@w.@n", true, i, nullptr, ch, act_to::vict);
    }
  }
  if (char_plr_flagged(i, player_flag::auralight)) {
    std::string bloom = str_format("...is surrounded by a bright {} aura.@n", aura_types[char_get_aura(i)]);
    act(g, bloom, true, i, nullptr, ch, act_to::vict);
  }
  if (char_aff_flagged(i, aff_flag::sanctuary) && !char_get_skill(i, SKILL_AQUA_BARRIER))
    act(g, "@w...$e has a @bbarrier@w around $s body!", true, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::fireshield))
    act(g, "@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", false, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::healglow))
    act(g, "@w...$e has a serene @Cblue@Y glow@w around $s body.", true, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::earmor))
    act(g, "@w...$e has ghostly @Ggreen@w ethereal armor around $s body.", true, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::sanctuary) && char_get_skill(i, SKILL_AQUA_BARRIER))
    act(g, "@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!", true, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::flying) && char_get_altitude(i) == 1)
    act(g, "@w...$e is in the air!", true, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::flying) && char_get_altitude(i) == 2)
    act(g, "@w...$e is high in the air!", true, i, nullptr, ch, act_to::vict);
  if (char_get_kaioken(i) > 0)
    act(g, "@w...@r$e has a red aura around $s body!", true, i, nullptr, ch, act_to::vict);
  if (!char_is_npc(i) && char_plr_flagged(i, player_flag::spiral))
    act(g, "@w...$e is spinning in a vortex!", false, i, nullptr, ch, act_to::vict);
  if (char_is_transformed(i) && !char_is_android(i) && !char_is_saiyan(i) && !char_is_halfbreed(i))
    act(g, "@w...$e has energy crackling around $s body!", true, i, nullptr, ch, act_to::vict);
  if (char_get_charge(i) && !char_is_saiyan(i) && !char_is_halfbreed(i)) {
    std::string aura;
    aura = str_format("@w...$e has a @Ybright@w {} aura around $s body!", aura_types[char_get_aura(i)]);
    act(g, aura, true, i, nullptr, ch, act_to::vict);
  }
  if (!char_plr_flagged(i, player_flag::oozaru) && char_get_charge(i) && char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act(g, "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!", true, i, nullptr, ch, act_to::vict);
  if (!char_plr_flagged(i, player_flag::oozaru) && char_get_charge(i) && !char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i))) {
    std::string aura;
    aura = str_format("@w...$e has a @Ybright@w {} aura around $s body!", aura_types[char_get_aura(i)]);
    act(g, aura, true, i, nullptr, ch, act_to::vict);
  }
  if (!char_plr_flagged(i, player_flag::oozaru) && !char_get_charge(i) && char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act(g, "@w...$e has energy crackling around $s body!", true, i, nullptr, ch, act_to::vict);
  if (char_plr_flagged(i, player_flag::oozaru) && char_get_charge(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act(g, "@w...$e is in the form of a @rgreat ape@w!", true, i, nullptr, ch, act_to::vict);
  if (is_affected(i, aff_flag::kyodaika))
    act(g, "@w...$e has expanded $s body size@w!", true, i, nullptr, ch, act_to::vict);
  if (char_aff_flagged(i, aff_flag::hayasa))
    act(g, "@w...$e has a soft @cblue@w glow around $s body!", false, i, nullptr, ch, act_to::vict);
  if (char_plr_flagged(i, player_flag::oozaru) && !char_get_charge(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act(g, "@w...$e has energy crackling around $s @rgreat ape@w body!", true, i, nullptr, ch, act_to::vict);
  if (!char_get_feature(i).empty()) {
    std::string woo = str_format("@C{}@n", char_get_feature(i));
    act(g, woo, false, i, nullptr, ch, act_to::vict);
  }

  if (!char_get_rdisplay(i).empty()) {
    if (char_get_rdisplay(i) != "Empty") {
      std::string rdis = str_format("...{}", char_get_rdisplay(i));
      act(g, rdis, false, i, nullptr, ch, act_to::vict);
    }
  }
}

void list_char_to_char(GameWorld *g, const id_list &list, struct char_data *ch)
{
  std::vector<struct char_data *> hidden;
  int num;

  for (char_data *i : char_list(g, list)) {
    if (char_aff_flagged(i, aff_flag::hide) && roll_resisted(g, i, SKILL_HIDE, ch, SKILL_SPOT)) {
      if (char_get_skill(i, SKILL_HIDE) && !char_is_npc(ch) && i != ch) {
        improve_skill(g, i, SKILL_HIDE, 1);
      }
      hidden.push_back(i);
    }
  }
  auto is_hidden = [&](const char_data *x) { return std::find(hidden.begin(), hidden.end(), x) != hidden.end(); };

  for (char_data *i : char_list(g, list)) {
    /* hide npcs whose description starts with a '.' from non-holylighted people
    - Idea from Elaseth of TBA */
    if ((ch == i) || (!char_is_npc(ch) && !char_prf_flagged(g, ch, pref_flag::holylight) && char_is_npc(i) && char_get_ldesc(i).starts_with('.')))
      continue;
    if (is_hidden(i))
      continue;

    if (char_can_see(g, ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurences of this mob are there? */
        auto stacks = [&](char_data *j) {
          return (char_get_nr(i) == char_get_nr(j)) &&
                 (char_get_position(i) == char_get_position(j)) &&
                 (char_get_affected_by(i) == char_get_affected_by(j)) &&
                 (!char_get_fighting(g, i) && !char_get_fighting(g, j)) &&
                 (getCurPL(g, i) == getEffMaxPL(g, i) && getCurPL(g, j) == getEffMaxPL(g, j)) &&
                 char_get_name(i) == char_get_name(j) &&
                 !is_hidden(j);
        };
        /* an earlier occurrence has already been shown, with the count */
        if (find_first(char_list(g, list), [&](char_data *j) { return j == i || stacks(j); }) != i)
          continue;
        for (char_data *j : char_list_from(g, list, i))
          if (stacks(j))
            num++;
      }
      /* Now show this mob's name and other stuff */
      send_to_char(g, ch, "@w");
      if (num > 1)
        send_to_char(g, ch, "@D(@Rx@Y{:>2d}@D)@n ", num);
      list_one_char(g, i, ch);
      send_to_char(g, ch, "@n");
    } /* processed a character we can see */
    else if (room_is_dark(g, char_get_in_room(ch)) && !char_can_see_in_dark(g, ch) &&
             char_aff_flagged(i, aff_flag::infravision))
      send_to_char(g, ch, "@wYou see a pair of glowing red eyes looking your way.@n\r\n");
  } /* loop through all characters in room */
}
