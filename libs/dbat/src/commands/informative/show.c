/* show.c: showing objects and characters to a viewer
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"
#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/disabled.h"
#include "informative_internal.h"

static void see_plant(struct obj_data *obj, struct char_data *ch);
static void list_one_char(struct char_data *i, struct char_data *ch);

static void see_plant(struct obj_data *obj, struct char_data *ch)
{

  int water = obj_get_value(obj, VAL_WATERLEVEL);

  if (water >= 0) {
    switch (obj_get_value(obj, VAL_MATURITY)) {
      case 0:
        send_to_char(ch, "@wA @G%s@y seed@w has been planted here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      case 1:
        send_to_char(ch, "@wA very young @G%s@w has sprouted from a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      case 2:
        send_to_char(ch, "@wA half grown @G%s@w is in a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      case 3:
        send_to_char(ch, "@wA mature @G%s@w is growing in a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      case 4:
        send_to_char(ch, "@wA mature @G%s@w is flowering in a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      case 5:
        send_to_char(ch, "@wA mature @G%s@w that is close to harvestable is here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      case 6:
        send_to_char(ch, "@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
        break;
      default:
        break;
    }
  } else {
    if (water > -4) {
      send_to_char(ch, "@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n", obj->short_description);
    } else if (water > -10) {
      send_to_char(ch, "@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n", obj->short_description);
    } else if (water <= -10) {
      send_to_char(ch, "@yA @G%s@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n", obj->short_description);
    }
  }
}

static const char *weapon_disp[6] = {
    "Sword",
    "Dagger",
    "Spear",
    "Club",
    "Gun",
    "Brawling"};

void display_spells(struct char_data *ch, struct obj_data *obj)
{
  int i;

  send_to_char(ch, "The spellbook contains the following spells:\r\n");
  send_to_char(ch, "@c---@wSpell Name@c------------------------------------@w# of pages@c-----@n\r\n");

  if (!obj->sbinfo) {
    return;
  }
  for (i = 0; i < SPELLBOOK_SIZE; i++) {
    if (obj->sbinfo[i].spellname != 0) {
      if (obj->sbinfo[i].spellname > MAX_SPELLS) {
        continue;
      }
      send_to_char(ch, "@y%-20s@n					[@R%2d@n]\r\n", spell_info[obj->sbinfo[i].spellname].name, obj->sbinfo[i].pages);
    }
  }
  return;
}

void display_scroll(struct char_data *ch, struct obj_data *obj)
{
  send_to_char(ch, "The scroll contains the following spell:\r\n");
  send_to_char(ch, "@c---@wSpell Name@c---------------------------------------------------@n\r\n");
  send_to_char(ch, "@y%-20s@n\r\n", skill_name(obj_get_value(obj, VAL_SCROLL_SPELL1)));
  return;
}

void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    mud_log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    /*  SYSERR_DESC:
     *  Somehow a NULL pointer was sent to show_obj_to_char() in either the
     *  'obj' or the 'ch' variable.  The error will indicate which was NULL
     *  be listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger.
     */
    return;
  }

  int spotted = FALSE;

  if (char_get_skill(ch, SKILL_SPOT) > rand_number(20, 110)) {
    spotted = TRUE;
  }

  switch (mode) {
    case SHOW_OBJ_LONG:
      /*
     * hide objects starting with . from non-holylighted people 
     * Idea from Elaseth of TBA 
     */
      if (*obj->description == '.' && (char_is_npc(ch) || !char_prf_flagged(ch, PRF_HOLYLIGHT)))
        return;
      if (obj_get_type_flag(obj) == ITEM_VEHICLE && rroom_get_vnum(char_get_in_room(ch)) == obj_get_value(obj, 0)) {
        return;
      }
      if (obj_get_sitting(obj) && char_get_admlevel(ch) < 1) {
        return;
      }
      if (obj_get_sitting(obj) && char_get_admlevel(ch) >= 1) {
        send_to_char(ch, "@D(@YBeing Used@D)@w");
      }
      if (obj_get_type_flag(obj) == ITEM_PLANT && (rroom_flagged(obj_get_in_room(obj), ROOM_GARDEN1) || rroom_flagged(obj_get_in_room(obj), ROOM_GARDEN2))) {
        see_plant(obj, ch);
        return;
      }
      if (obj_extra_flagged(obj, ITEM_BURIED)) {
        char bury[MAX_INPUT_LENGTH];
        if (!obj_is_corpse(obj)) {
          if (obj_get_weight(obj) < 10) {
            sprintf(bury, "small mound of");
          } else if (obj_get_weight(obj) < 50) {
            sprintf(bury, "medium sized mound of");
          } else if (obj_get_weight(obj) < 1000) {
            sprintf(bury, "large mound of");
          } else {
            sprintf(bury, "gigantic mound of");
          }
        } else {
          sprintf(bury, "recent grave covered by");
        }
        if (spotted == TRUE && rroom_sect(obj_get_in_room(obj)) != SECT_DESERT) {
          send_to_char(ch, "@yA %s soft dirt is here.@n\r\n", bury);
          return;
        } else if (spotted == TRUE && rroom_sect(obj_get_in_room(obj)) == SECT_DESERT) {
          send_to_char(ch, "@YA %s soft sand is here.@n\r\n", bury);
          return;
        } else {
          return;
        }
      }
      if (obj_get_vnum(obj) == 11) {
        send_to_char(ch, "@wA gravity generator, set to %sx gravity, is built here", add_commas(obj_get_weight(obj)));
      } else if (obj_get_vnum(obj) == 79) {
        send_to_char(ch, "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction", add_commas(obj_get_weight(obj)), dirs[obj_get_cost(obj)]);
      } else {
        send_to_char(ch, "@w");
        if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS)) {
          if (obj_get_posted_to(obj) == NULL) {
            send_to_char(ch, "@D[@G%d@D]@w ", obj_get_vnum(obj));
            if (SCRIPT(obj))
              send_to_char(ch, "@D[@wT%d@D]@w ", obj->proto_script->vnum);
          } else {
            if (obj_get_posttype(obj) <= 0) {
              send_to_char(ch, "@D[@G%d@D]@w ", obj_get_vnum(obj));
              if (SCRIPT(obj))
                send_to_char(ch, "@D[@wT%d@D]@w ", obj->proto_script->vnum);
            }
          }
        }

        if (obj_get_posttype(obj) > 0) {
          if (obj_get_posted_to(obj)) {
            return;
          } else {
            send_to_char(ch, "%s@w, has been posted here.@n", obj->short_description);
          }
        } else {
          if (!obj_extra_flagged(obj, ITEM_BURIED)) {
            send_to_char(ch, "%s@n", obj->description);
          }
        }

        if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
          if (!obj_val_flagged(obj, CONT_CLOSED) && obj_get_vnum(obj) > 19199)
            send_to_char(ch, "\r\n@c...its outer hatch is open@n");
          else if (!obj_val_flagged(obj, CONT_CLOSED) && obj_get_vnum(obj) <= 19199)
            send_to_char(ch, "\r\n@c...its door is open@n");
        }
        if (obj_get_type_flag(obj) == ITEM_CONTAINER && !obj_is_corpse(obj)) {
          if (!obj_val_flagged(obj, CONT_CLOSED) && !obj_extra_flagged(obj, ITEM_SHEATH))
            send_to_char(ch, ". @D[@G-open-@D]@n");
          else if (!obj_extra_flagged(obj, ITEM_SHEATH))
            send_to_char(ch, ". @D[@rclosed@D]@n");
        }
        if (obj_get_type_flag(obj) == ITEM_HATCH) {
          if (!obj_val_flagged(obj, CONT_CLOSED))
            send_to_char(ch, ", it is open");
          else if (obj_val_flagged(obj, CONT_CLOSED))
            send_to_char(ch, ", it is closed");
          if (obj_val_flagged(obj, CONT_LOCKED))
            send_to_char(ch, " and locked@n");
          else
            send_to_char(ch, "@n");
        }
        if (obj_get_type_flag(obj) == ITEM_FOOD) {
          if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
            send_to_char(ch, ", and it has been ate on@n");
          }
        }
      }
      break;

    case SHOW_OBJ_SHORT:
      if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS)) {
        send_to_char(ch, "[%d] ", obj_get_vnum(obj));
        if (SCRIPT(obj))
          send_to_char(ch, "[T%d] ", obj->proto_script->vnum);
      }

      if (char_prf_flagged(ch, PRF_IHEALTH)) {
        send_to_char(ch, "@D<@gH@D: @C%d@D>@w %s", obj_get_value(obj, VAL_ALL_HEALTH), obj->short_description);
      } else {
        send_to_char(ch, "%s", obj->short_description);
      }
      if (obj_get_type_flag(obj) == ITEM_FOOD) {
        if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
          send_to_char(ch, ", and it has been ate on.@n");
        }
      }
      if (obj_get_vnum(obj) == 255) {
        switch (obj_get_value(obj, 0)) {
          case 0:
          case 1:
            send_to_char(ch, " @D[@wQuality @RC@D]@n");
            break;
          case 2:
            send_to_char(ch, " @D[@wQuality @RC+@D]@n");
            break;
          case 3:
            send_to_char(ch, " @D[@wQuality @yC++@D]@n");
            break;
          case 4:
            send_to_char(ch, " @D[@wQuality @yB@D]@n");
            break;
          case 5:
            send_to_char(ch, " @D[@wQuality @CB+@D]@n");
            break;
          case 6:
            send_to_char(ch, " @D[@wQuality @CB++@D]@n");
            break;
          case 7:
            send_to_char(ch, " @D[@wQuality @CA@D]@n");
            break;
          case 8:
            send_to_char(ch, " @D[@wQuality @GA+@D]@n");
            break;
        }
      }

      if (obj_get_vnum(obj) == 3424) {
        send_to_char(ch, " @D[@bInk Remaining@D: @w%d@D]@n", obj_get_value(obj, 6));
      }
      if (obj_get_vnum(obj) == 3423) {
        send_to_char(ch, " @D[@B%d@D/@B24 Inks@D]@n", obj_get_value(obj, 6));
      }
      if (obj_extra_flagged(obj, ITEM_THROW)) {
        send_to_char(ch, " @D[@RThrow Only@D]@n");
      }
      if (obj_get_type_flag(obj) == ITEM_PLANT && !obj_extra_flagged(obj, ITEM_MATURE)) {
        if (obj_get_value(obj, VAL_WATERLEVEL) < -9) {
          send_to_char(ch, "@D[@RDead@D]@n");
        } else {
          switch (obj_get_value(obj, VAL_MATURITY)) {
            case 0:
              send_to_char(ch, " @D[@ySeed@D]@n");
              break;
            case 1:
              send_to_char(ch, " @D[@GSprout@D]@n");
              break;
            case 2:
              send_to_char(ch, " @D[@GYoung@D]@n");
              break;
            case 3:
              send_to_char(ch, " @D[@GMature@D]@n");
              break;
            case 4:
              send_to_char(ch, " @D[@GBudding@D]@n");
              break;
            case 5:
              send_to_char(ch, "@D[@GClose Harvest@D]@n");
              break;
            case 6:
              send_to_char(ch, "@D[@gHarvest@D]@n");
              break;
          }
        }
      }
      if (obj_get_type_flag(obj) == ITEM_CONTAINER && !obj_is_corpse(obj)) {
        if (!obj_val_flagged(obj, CONT_CLOSED) && !obj_extra_flagged(obj, ITEM_SHEATH))
          send_to_char(ch, " @D[@G-open-@D]@n");
        else if (!obj_extra_flagged(obj, ITEM_SHEATH))
          send_to_char(ch, " @D[@rclosed@D]@n");
      }
      if (obj_extra_flagged(obj, ITEM_DUPLICATE)) {
        send_to_char(ch, " @D[@YDuplicate@D]@n");
      }
      break;

    case SHOW_OBJ_ACTION:
      switch (obj_get_type_flag(obj)) {
        case ITEM_NOTE:
          if (obj->action_description) {
            char notebuf[MAX_NOTE_LENGTH];

            snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
            send_to_char(ch, "%s", notebuf);
          } else
            send_to_char(ch, "There appears to be nothing written on it.\r\n");
          return;

        case ITEM_BOARD:
          show_board(obj_get_vnum(obj), ch);
          break;

        case ITEM_CONTROL:
          send_to_char(ch, "@RFUEL@D: %s%s@n\r\n", obj_get_value(obj, 2) >= 200 ? "@G" : obj_get_value(obj, 2) >= 100 ? "@Y"
                                                                                                                      : "@r",
                       add_commas(obj_get_value(obj, 2)));
          break;

        case ITEM_DRINKCON:
          send_to_char(ch, "It looks like a drink container.\r\n");
          break;

        case ITEM_LIGHT:
          if (obj_get_value(obj, VAL_LIGHT_HOURS) == -1)
            send_to_char(ch, "Light Cycles left: Infinite\r\n");
          else
            send_to_char(ch, "Light Cycles left: [%d]\r\n", obj_get_value(obj, VAL_LIGHT_HOURS));
          break;

        case ITEM_FOOD:
          if (obj_get_foob(obj) >= 4) {
            if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj) / 4) {
              send_to_char(ch, "Condition of the food: Almost gone.\r\n");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj) / 2) {
              send_to_char(ch, "Condition of the food: Half Eaten.");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
              send_to_char(ch, "Condition of the food: Partially Eaten.");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) == obj_get_foob(obj)) {
              send_to_char(ch, "Condition of the food: Whole.");
            }
          } else if (obj_get_foob(obj) > 0) {
            if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
              send_to_char(ch, "Condition of the food: Almost gone.");
            } else if (obj_get_value(obj, VAL_FOOD_FOODVAL) == obj_get_foob(obj)) {
              send_to_char(ch, "Condition of the food: Whole.");
            }
          } else {
            send_to_char(ch, "Condition of the food: Insignificant.");
          }
          break;

        case ITEM_SPELLBOOK:
          send_to_char(ch, "It looks like an arcane tome.\r\n");
          display_spells(ch, obj);
          break;

        case ITEM_SCROLL:
          send_to_char(ch, "It looks like an arcane scroll.\r\n");
          display_scroll(ch, obj);
          break;

        case ITEM_VEHICLE:
          if (obj_get_vnum(obj) > 19199) {
            send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
            send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
            send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
            send_to_char(ch, "@YSyntax@D: @CEnter hatch\r\n");
          } else {
            send_to_char(ch, "@YSyntax@D: @CUnlock door\r\n");
            send_to_char(ch, "@YSyntax@D: @COpen door\r\n");
            send_to_char(ch, "@YSyntax@D: @CClose door\r\n");
            send_to_char(ch, "@YSyntax@D: @CEnter door\r\n");
          }
          break;

        case ITEM_HATCH:
          if (obj_get_vnum(obj) > 19199) {
            send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
            send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
            send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
            send_to_char(ch, "@YSyntax@D: @CLeave@n\r\n");
          } else {
            send_to_char(ch, "@YSyntax@D: @CUnlock door\r\n");
            send_to_char(ch, "@YSyntax@D: @COpen door\r\n");
            send_to_char(ch, "@YSyntax@D: @CClose door\r\n");
            send_to_char(ch, "@YSyntax@D: @CEnter door\r\n");
          }
          break;

        case ITEM_WINDOW:
          look_out_window(ch, obj->name);
          return;
          break;

        default:
          if (!obj_is_corpse(obj)) {
            send_to_char(ch, "You see nothing special..\r\n");
          } else {
            int mention = FALSE;
            send_to_char(ch, "This corpse has ");

            if (obj_get_value(obj, VAL_CORPSE_HEAD) == 0) {
              send_to_char(ch, "no head,");
              mention = TRUE;
            }

            if (obj_get_value(obj, VAL_CORPSE_RARM) == 0) {
              send_to_char(ch, "no right arm, ");
              mention = TRUE;
            } else if (obj_get_value(obj, VAL_CORPSE_RARM) == 2) {
              send_to_char(ch, "a broken right arm, ");
              mention = TRUE;
            }

            if (obj_get_value(obj, VAL_CORPSE_LARM) == 0) {
              send_to_char(ch, "no left arm, ");
              mention = TRUE;
            } else if (obj_get_value(obj, VAL_CORPSE_LARM) == 2) {
              send_to_char(ch, "a broken left arm, ");
              mention = TRUE;
            }

            if (obj_get_value(obj, VAL_CORPSE_RLEG) == 0) {
              send_to_char(ch, "no right leg, ");
              mention = TRUE;
            } else if (obj_get_value(obj, VAL_CORPSE_RLEG) == 2) {
              send_to_char(ch, "a broken right leg, ");
              mention = TRUE;
            }

            if (obj_get_value(obj, VAL_CORPSE_LLEG) == 0) {
              send_to_char(ch, "no left leg, ");
              mention = TRUE;
            } else if (obj_get_value(obj, VAL_CORPSE_LLEG) == 2) {
              send_to_char(ch, "a broken left leg, ");
              mention = TRUE;
            }

            if (mention == FALSE) {
              send_to_char(ch, "nothing missing from it but life.");
            } else {
              send_to_char(ch, "and is dead.");
            }

            send_to_char(ch, "\r\n");
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
        send_to_char(ch, "The weapon type of %s@n is '%s'.\r\n", obj_get_short_description(obj), weapon_disp[num]);
        send_to_char(ch, "You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), obj)]);
      }
      diag_obj_to_char(obj, ch);
      send_to_char(ch, "It appears to be made of %s, and weighs %s", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
      break;

    default:
      mud_log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
      /*  SYSERR_DESC:
     *  show_obj_to_char() has some predefined 'mode's (argument #3) to tell
     *  it what to display to the character when it is called.  If the mode
     *  is not one of these, it will output this error, and indicate what
     *  mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode.
     */
      return;
  }

  if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
    send_to_char(ch, "\r\n");
}

int show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  int found = FALSE;

  if (obj_extra_flagged(obj, ITEM_INVISIBLE)) {
    send_to_char(ch, " (invisible)");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_BLESS) && char_aff_flagged(ch, AFF_DETECT_ALIGN)) {
    send_to_char(ch, " ..It glows blue!");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_MAGIC) && char_aff_flagged(ch, AFF_DETECT_MAGIC)) {
    send_to_char(ch, " ..It glows yellow!");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_GLOW)) {
    send_to_char(ch, " @D(@GGlowing@D)@n");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_HOT)) {
    send_to_char(ch, " @D(@RHOT@D)@n");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_HUM)) {
    send_to_char(ch, " @D(@RHumming@D)@n");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_SLOT2)) {
    if (obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED))
      send_to_char(ch, " @D[@m1/2 Tokens@D]@n");
    else if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED))
      send_to_char(ch, " @D[@m2/2 Tokens@D]@n");
    else
      send_to_char(ch, " @D[@m0/2 Tokens@D]@n");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_SLOT1)) {
    if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED))
      send_to_char(ch, " @D[@m1/1 Tokens@D]@n");
    else
      send_to_char(ch, " @D[@m0/1 Tokens@D]@n");
    found++;
  }
  if (obj_get_kicharge(obj) > 0) {
    int num = (obj_get_distance(obj) * 20) + rand_number(1, 5);
    send_to_char(ch, " %d meters away", num);
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
    send_to_char(ch, " @D(@YCUSTOM@D)@n");
  }
  if (obj_extra_flagged(obj, ITEM_RESTRING)) {
    send_to_char(ch, " @D(@R%s@D)@n", char_get_admlevel(ch) > 0 ? obj->name : "*");
  }
  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL || obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL || obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
      send_to_char(ch, ", and appears to be twisted and broken.");
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
      send_to_char(ch, ", and is broken into hundreds of splinters.");
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
      send_to_char(ch, ", and is shattered on the ground.");
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
      send_to_char(ch, ", and is a pile of rubble.");
    } else {
      send_to_char(ch, ", and is broken.");
    }
    found++;
  } else {
    if (obj_get_type_flag(obj) != ITEM_BOARD) {
      if (obj_get_type_flag(obj) != ITEM_CONTAINER) {
        send_to_char(ch, ".");
      }
      if (!char_is_npc(ch) && obj_get_posted_to(obj) && obj_get_posttype(obj) <= 0) {
        struct obj_data *obj2 = obj_get_posted_to(obj);
        char dvnum[200];
        *dvnum = '\0';
        sprintf(dvnum, "@D[@G%d@D] @w", obj_get_vnum(obj2));
        send_to_char(ch, "\n...%s%s has been posted to it.", char_prf_flagged(ch, PRF_ROOMFLAGS) ? dvnum : "", obj2->short_description);
      }
    }
    found++;
  }
  return (found);
}

void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show)
{
  struct obj_data *i, *j, *d;
  bool found = FALSE;
  int num;

  /* Loop through all objects in the list */
  for (i = list; i; i = i->next_content) {
    if (i->description == NULL)
      continue;
    if (strcasecmp(i->description, "undefined") == 0)
      continue;
    num = 0;
    d   = i;
    if (CONFIG_STACK_OBJS) {
      for (j = list; j != i; j = j->next_content)
        if ((!strcasecmp(j->short_description, i->short_description) && !strcasecmp(j->description, i->description)) &&
            (j->item_number == i->item_number) && ((obj_extra_flagged(j, ITEM_BROKEN) && obj_extra_flagged(i, ITEM_BROKEN)) || (!obj_extra_flagged(j, ITEM_BROKEN) && !obj_extra_flagged(i, ITEM_BROKEN))))
          if ((!obj_get_sitting(j) && !obj_get_sitting(i)))
            if (obj_get_value(j, 6) == obj_get_value(i, 6))
              if ((obj_get_type_flag(j) != ITEM_PLANT && obj_get_type_flag(i) != ITEM_PLANT) || (obj_get_type_flag(j) == ITEM_PLANT && obj_get_type_flag(i) == ITEM_PLANT && obj_get_value(j, VAL_MATURITY) == obj_get_value(i, VAL_MATURITY) && obj_get_value(j, VAL_WATERLEVEL) == obj_get_value(i, VAL_WATERLEVEL)))
                if ((!obj_extra_flagged(j, ITEM_DUPLICATE) && !obj_extra_flagged(i, ITEM_DUPLICATE)) || (obj_extra_flagged(j, ITEM_DUPLICATE) && obj_extra_flagged(i, ITEM_DUPLICATE)))
                  if (obj_get_posttype(j) == 0 && obj_get_posttype(i) == 0)
                    if (!obj_get_fellow_wall(j) && !obj_get_fellow_wall(i))
                      if ((obj_get_value(j, 0) == obj_get_value(i, 0) && obj_get_vnum(j) == 255 && obj_get_vnum(i) == 255) || (obj_get_vnum(j) != 255 && obj_get_vnum(i) != 255))
                        break;
      if (j != i)
        continue;
      for (d = j = i; j; j = j->next_content)
        if ((!strcasecmp(j->short_description, i->short_description) && !strcasecmp(j->description, i->description)) &&
            (j->item_number == i->item_number) && ((obj_extra_flagged(j, ITEM_BROKEN) && obj_extra_flagged(i, ITEM_BROKEN)) || (!obj_extra_flagged(j, ITEM_BROKEN) && !obj_extra_flagged(i, ITEM_BROKEN))))
          if ((!obj_get_sitting(j) && !obj_get_sitting(i)))
            if (obj_get_posttype(j) == 0 && obj_get_posttype(i) == 0)
              if (obj_get_value(j, 6) == obj_get_value(i, 6))
                if ((obj_get_type_flag(j) != ITEM_PLANT && obj_get_type_flag(i) != ITEM_PLANT) || (obj_get_type_flag(j) == ITEM_PLANT && obj_get_type_flag(i) == ITEM_PLANT && obj_get_value(j, VAL_MATURITY) == obj_get_value(i, VAL_MATURITY) && obj_get_value(j, VAL_WATERLEVEL) == obj_get_value(i, VAL_WATERLEVEL)))
                  if ((!obj_extra_flagged(j, ITEM_DUPLICATE) && !obj_extra_flagged(i, ITEM_DUPLICATE)) || (obj_extra_flagged(j, ITEM_DUPLICATE) && obj_extra_flagged(i, ITEM_DUPLICATE)))
                    if (!obj_get_fellow_wall(j) && !obj_get_fellow_wall(i))
                      if ((obj_get_value(j, 0) == obj_get_value(i, 0) && obj_get_vnum(i) == 255 && obj_get_vnum(j) == 255) || (obj_get_vnum(j) != 255 && obj_get_vnum(i) != 255))
                        if (char_can_see_obj(ch, j)) {
                          num++;
                          if (d == i && !char_can_see_obj(ch, d))
                            d = j;
                        }
    }
    if ((char_can_see_obj(ch, d) && ((*d->description != '.' && *d->short_description != '.') || char_prf_flagged(ch, PRF_HOLYLIGHT))) || (obj_get_type_flag(d) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(ch, "@D(@Rx@Y%2i@D)@n ", num);
      show_obj_to_char(d, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
    send_to_char(ch, " Nothing.\r\n");
}

void diag_obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
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
  const char *objs = obj_shortdesc_for(obj, ch);

  if (obj_get_value(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * obj_get_value(obj, VAL_ALL_HEALTH)) / obj_get_value(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0; /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "\r\n%c%s %s\r\n", toupper(*objs), objs + 1, diagnosis[ar_index].text);
}

void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
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

  percent = (int)(i->health * 100.0);

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent) {
      send_to_char(ch, "%s\r\n", diagnosis[ar_index].text);
      return;
    }
}

void look_at_char(struct char_data *i, struct char_data *ch)
{
  int j, found, clan = FALSE;
  char buf[100];
  struct obj_data *tmp_obj;

  if (!ch->desc) {
    return;
  }
  if (i->description) {
    send_to_char(ch, "%s", i->description);
  }
  if (!char_mob_flagged(i, MOB_JUSTDESC)) {
    bringdesc(ch, i);
  }
  send_to_char(ch, "\r\n");
  if (!char_is_npc(i)) {
    if (char_get_limb_condition(i, (1) - 1) >= 50 && !char_plr_flagged(i, PLR_CRARM)) {
      send_to_char(ch, "            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (1) - 1) > 0 && !char_plr_flagged(i, PLR_CRARM)) {
      send_to_char(ch, "            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (1) - 1) > 0 && char_plr_flagged(i, PLR_CRARM)) {
      send_to_char(ch, "            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (1) - 1) <= 0) {
      send_to_char(ch, "            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
    }
    if (char_get_limb_condition(i, (2) - 1) >= 50 && !char_plr_flagged(i, PLR_CLARM)) {
      send_to_char(ch, "            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (2) - 1) > 0 && !char_plr_flagged(i, PLR_CLARM)) {
      send_to_char(ch, "            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (2) - 1) > 0 && char_plr_flagged(i, PLR_CLARM)) {
      send_to_char(ch, "            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (2) - 1) <= 0) {
      send_to_char(ch, "            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
    }
    if (char_get_limb_condition(i, (3) - 1) >= 50 && !char_plr_flagged(i, PLR_CLARM)) {
      send_to_char(ch, "            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (3) - 1) > 0 && !char_plr_flagged(i, PLR_CRLEG)) {
      send_to_char(ch, "            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (3) - 1) > 0 && char_plr_flagged(i, PLR_CRLEG)) {
      send_to_char(ch, "            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (3) - 1) <= 0) {
      send_to_char(ch, "            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
    }
    if (char_get_limb_condition(i, (4) - 1) >= 50 && !char_plr_flagged(i, PLR_CLLEG)) {
      send_to_char(ch, "            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (4) - 1) > 0 && !char_plr_flagged(i, PLR_CLLEG)) {
      send_to_char(ch, "            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (4) - 1) > 0 && char_plr_flagged(i, PLR_CLLEG)) {
      send_to_char(ch, "            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(i, (4) - 1) <= 0) {
      send_to_char(ch, "            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
    }
    if (char_plr_flagged(i, PLR_HEAD)) {
      send_to_char(ch, "            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
    }
    if (!char_plr_flagged(i, PLR_HEAD)) {
      send_to_char(ch, "            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
    }
    if (((char_is_saiyan(i) || char_is_halfbreed(i)) && char_plr_flagged(i, PLR_STAIL)) && !char_plr_flagged(i, PLR_TAILHIDE)) {
      send_to_char(ch, "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
    }
    if ((char_is_saiyan(i) || char_is_halfbreed(i)) && (!char_plr_flagged(i, PLR_STAIL)) && (!char_plr_flagged(i, PLR_TAILHIDE))) {
      send_to_char(ch, "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
    }
    if ((char_is_icer(i) || char_is_bio(i)) && char_plr_flagged(i, PLR_TAIL)) {
      send_to_char(ch, "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
    }
    if ((char_is_icer(i) || char_is_bio(i)) && !char_plr_flagged(i, PLR_TAIL)) {
      send_to_char(ch, "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
    }
  }
  send_to_char(ch, "\r\n");
  clan = FALSE;
  if (!char_is_npc(i)) {
    send_to_char(ch, "            @D[@mClan        @D: @W%-20s@D]@n\r\n", clan ? buf : "None.");
  }
  if (!char_is_npc(i)) {
    send_to_char(ch, "\r\n         @D----------------------------------------@n\r\n");
    trans_check(ch, i);
    send_to_char(ch, "         @D----------------------------------------@n\r\n");
  }
  send_to_char(ch, "\r\n");

  if ((!char_plr_flagged(i, PLR_DISGUISED) && (readIntro(ch, i) == 1 && !char_is_npc(i)))) {
    if (char_get_sex(i) == SEX_NEUTRAL)
      send_to_char(ch, "%s appears to be %s %s, ", get_i_name(ch, i), AN(juggleRaceName(i, true)), juggleRaceName(i, false));
    else
      send_to_char(ch, "%s appears to be %s %s %s, ", get_i_name(ch, i), AN(char_mafe(i)), char_mafe(i), juggleRaceName(i, false));
  } else if (ch == i || char_is_npc(i)) {
    if (char_get_sex(i) == SEX_NEUTRAL)
      send_to_char(ch, "%c%s appears to be %s %s, ", toupper(*char_get_name(i)), char_get_name(i) + 1, AN(juggleRaceName(i, true)), juggleRaceName(i, false));
    else
      send_to_char(ch, "%c%s appears to be %s %s %s, ", toupper(*char_get_name(i)), char_get_name(i) + 1, AN(char_mafe(i)), char_mafe(i), juggleRaceName(i, false));
  } else {
    if (char_get_sex(i) == SEX_NEUTRAL)
      send_to_char(ch, "Appears to be %s %s, ", AN(juggleRaceName(i, true)), juggleRaceName(i, false));
    else
      send_to_char(ch, "Appears to be %s %s %s, ", AN(char_mafe(i)), char_mafe(i), juggleRaceName(i, false));
  }

  if (char_is_npc(i)) {
    send_to_char(ch, "is %s sized, and\r\n", size_names[get_size(i)]);
  }
  if (!char_is_npc(i)) {
    if (!char_plr_flagged(i, PLR_OOZARU) && (!char_is_icer(i) || !char_is_transformed(i)) && char_get_genome(i, 0) < 11) {
      send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], char_get_pc_height(i), char_get_pc_weight(i));
    } else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS1)) {
      int num1 = char_get_pc_height(i) * 3;
      int num2 = char_get_pc_weight(i) * 4;
      send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS2)) {
      int num1 = char_get_pc_height(i) * 3;
      int num2 = char_get_pc_weight(i) * 4;
      send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS3)) {
      int num1 = char_get_pc_height(i) * 1.5;
      int num2 = char_get_pc_weight(i) * 2;
      send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS4)) {
      int num1 = char_get_pc_height(i) * 2;
      int num2 = char_get_pc_weight(i) * 3;
      send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
    } else if (char_plr_flagged(i, PLR_OOZARU) || char_get_genome(i, 0) == 11) {
      int num1 = char_get_pc_height(i) * 10;
      int num2 = char_get_pc_weight(i) * 50;
      send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
    }
    if (i == ch) {
      send_to_char(ch, " and ");
    } else if (char_get_age_years(ch) >= char_get_age_years(i) + 30) {
      send_to_char(ch, " appears to be very much younger than you, and ");
    } else if (char_get_age_years(ch) >= char_get_age_years(i) + 25) {
      send_to_char(ch, " appears to be much younger than you, and ");
    } else if (char_get_age_years(ch) >= char_get_age_years(i) + 15) {
      send_to_char(ch, " appears to be a good amount younger than you, and ");
    } else if (char_get_age_years(ch) >= char_get_age_years(i) + 10) {
      send_to_char(ch, " appears to be about a decade younger than you, and ");
    } else if (char_get_age_years(ch) >= char_get_age_years(i) + 5) {
      send_to_char(ch, " appears to be several years younger than you, and ");
    } else if (char_get_age_years(ch) >= char_get_age_years(i) + 2) {
      send_to_char(ch, " appears to be a bit younger than you, and ");
    } else if (char_get_age_years(ch) > char_get_age_years(i)) {
      send_to_char(ch, " appears to be slightly younger than you, and ");
    } else if (char_get_age_years(ch) == char_get_age_years(i)) {
      send_to_char(ch, " appears to be the same age as you, and ");
    }
    if (char_get_age_years(i) >= char_get_age_years(ch) + 30) {
      send_to_char(ch, " appears to be very much older than you, and ");
    } else if (char_get_age_years(i) >= char_get_age_years(ch) + 25) {
      send_to_char(ch, " appears to be much older than you, and ");
    } else if (char_get_age_years(i) >= char_get_age_years(ch) + 15) {
      send_to_char(ch, " appears to be a good amount older than you, and ");
    } else if (char_get_age_years(i) >= char_get_age_years(ch) + 10) {
      send_to_char(ch, " appears to be about a decade older than you, and ");
    } else if (char_get_age_years(i) >= char_get_age_years(ch) + 5) {
      send_to_char(ch, " appears to be several years older than you, and ");
    } else if (char_get_age_years(i) >= char_get_age_years(ch) + 2) {
      send_to_char(ch, " appears to be a bit older than you, and ");
    } else if (char_get_age_years(i) > char_get_age_years(ch)) {
      send_to_char(ch, " appears to be slightly older than you, and ");
    }
  }
  diag_char_to_char(i, ch);
  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)))
      found = TRUE;

  if (found && (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NOEQSEE))) {
    send_to_char(ch, "\r\n"); /* act() does capitalization. */
    if (!char_plr_flagged(i, PLR_DISGUISED)) {
      act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    } else {
      act("The disguised person is using:", FALSE, i, 0, ch, TO_VICT);
    }
    for (j = 0; j < NUM_WEARS; j++)
      if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)) && (j != WEAR_WIELD1 && j != WEAR_WIELD2)) {
        send_to_char(ch, "%s", wear_where[j]);
        show_obj_to_char(char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
        if (obj_extra_flagged(char_get_equipment(i, j), ITEM_SHEATH)) {
          struct obj_data *obj2 = NULL, *next_obj = NULL, *sheath = char_get_equipment(i, j);
          for (obj2 = sheath->contains; obj2; obj2 = next_obj) {
            next_obj = obj2->next_content;
            if (obj2) {
              send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
              show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
            }
          }
          obj2 = NULL;
        }
      } else if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)) && (!char_plr_flagged(i, PLR_THANDW))) {
        send_to_char(ch, "%s", wear_where[j]);
        show_obj_to_char(char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
        if (obj_extra_flagged(char_get_equipment(i, j), ITEM_SHEATH)) {
          struct obj_data *obj2 = NULL, *next_obj = NULL, *sheath = char_get_equipment(i, j);
          for (obj2 = sheath->contains; obj2; obj2 = next_obj) {
            next_obj = obj2->next_content;
            if (obj2) {
              send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
              show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
            }
          }
          obj2 = NULL;
        }
      } else if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)) && (char_plr_flagged(i, PLR_THANDW))) {
        send_to_char(ch, "@c<@CWielded by B. Hands@c>@n ");
        show_obj_to_char(char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
      }
  }
  if (ch != i && ((char_get_skill(ch, SKILL_KEEN) && char_aff_flagged(ch, AFF_SNEAK)) || char_get_admlevel(ch))) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    if (char_can_see(i, ch))
      act("$n tries to evaluate what you have in your inventory.", TRUE, ch, 0, i, TO_VICT);
    if (char_get_skill(ch, SKILL_KEEN) > axion_dice(0) && (!char_is_npc(i) || char_get_admlevel(ch) > 1)) {
      for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
        if (char_can_see_obj(ch, tmp_obj) &&
            (char_adm_flagged(ch, ADM_SEEINV) || (rand_number(0, 20) < char_get_level(ch)))) {
          show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
          found = TRUE;
        }
      }
      improve_skill(ch, SKILL_KEEN, 1);
    } else if (char_is_npc(i) && char_get_admlevel(ch) < 2) {
      return;
    } else {
      act("You are unsure about $s inventory.", FALSE, i, 0, ch, TO_VICT);
      if (char_can_see(i, ch))
        act("$n didn't seem to get a good enough look.", TRUE, ch, 0, i, TO_VICT);
      improve_skill(ch, SKILL_KEEN, 1);
      return;
    }
    if (!found) {
      send_to_char(ch, "You can't see anything.\r\n");
      improve_skill(ch, SKILL_KEEN, 1);
    }
  }
}

static void list_one_char(struct char_data *i, struct char_data *ch)
{
  struct obj_data *chair  = NULL;
  int count               = FALSE;
  const char *positions[] = {
      " is dead",
      " is mortally wounded",
      " is lying here, incapacitated",
      " is lying here, stunned",
      " is sleeping here",
      " is resting here",
      " is sitting here",
      "!FIGHTING!",
      " is standing here"};

  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS) && char_is_npc(i))
    send_to_char(ch, "@D[@G%d@D]@w %s", char_get_mob_vnum(i), SCRIPT(i) ? "[TRIG] " : "");

  if (char_is_npc(i) && i->long_descr && char_get_position(i) == char_get_mob_specials_default_pos(i) && !char_get_fighting(i)) {
    send_to_char(ch, "%s", i->long_descr);

    if (char_is_npc(i)) {
      double health = i->health;
      if (health <= 0.1) {
        act("@R...Should be DEAD soon.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.2) {
        act("@R...Is on $s last leg.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.3) {
        act("@R...Is absolutely covered in wounds.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.4) {
        act("@R...$s body is in terrible shape.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.5) {
        act("@R...Blood is seeping from the wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.6) {
        act("@R...Horrible wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.7) {
        act("@R...Quite a few wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.8) {
        act("@R...Many wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.9) {
        act("@R...A few wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health < 1.0) {
        act("@R...Some slight wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      }
    }

    if (char_get_listenroom(i) > 0) {
      char eaves[300];
      sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.", dirs[char_get_eavesdir(i)]);
      act(eaves, TRUE, i, 0, ch, TO_VICT);
    }
    if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 1)
      act("...$e is in the air!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 2)
      act("...$e is high in the air!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_SANCTUARY) && !char_get_skill(i, SKILL_AQUA_BARRIER))
      act("...$e has a barrier around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_FIRESHIELD))
      act("...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_SANCTUARY) && char_get_skill(i, SKILL_AQUA_BARRIER))
      act("...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (!char_is_npc(i) && char_plr_flagged(i, PLR_SPIRAL))
      act("...$e is spinning in a vortex!", FALSE, i, 0, ch, TO_VICT);
    if (char_get_charge(i))
      act("...$e has a bright %s aura around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_METAMORPH))
      act("@w...$e has a dark, @rred@w aura and menacing presence.", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_HAYASA))
      act("@w...$e has a soft @cblue@w glow around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_FAERIE_FIRE))
      act("@m...$e @mis outlined with purple fire!@m", FALSE, i, 0, ch, TO_VICT);
    if (char_get_feature(i)) {
      char woo[MAX_STRING_LENGTH];
      sprintf(woo, "@C%s@n", char_get_feature(i));
      act(woo, FALSE, i, 0, ch, TO_VICT);
    }

    return;
  }

  if (char_is_npc(i) && !char_get_fighting(i) && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_grappled(i) && char_get_grappled(i) == ch)
    send_to_char(ch, "@w%c%s is being grappled with by YOU!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_grappled(i) && char_get_grappled(i) != ch)
    send_to_char(ch, "@w%c%s is being absorbed from by %s!", toupper(*i->short_descr), i->short_descr + 1, readIntro(ch, char_get_grappled(i)) == 1 ? get_i_name(ch, char_get_grappled(i)) : AN(juggleRaceName(char_get_grappled(i), true)));
  else if (char_is_npc(i) && char_get_absorbby(i) && char_get_absorbby(i) == ch)
    send_to_char(ch, "@w%c%s is being absorbed from by YOU!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_absorbby(i) && char_get_absorbby(i) != ch)
    send_to_char(ch, "@w%c%s is being absorbed from by %s!", toupper(*i->short_descr), i->short_descr + 1, readIntro(ch, char_get_absorbby(i)) == 1 ? get_i_name(ch, char_get_absorbby(i)) : AN(juggleRaceName(char_get_absorbby(i), true)));
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_fighting(i) != ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING && is_sparring(i))
    send_to_char(ch, "@w%c%s is sparring with %s!", toupper(*i->short_descr), i->short_descr + 1, char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : juggleRaceName(char_get_fighting(i), false)));
  else if (char_is_npc(i) && char_get_fighting(i) && is_sparring(i) && char_get_fighting(i) == ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is sparring with you!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_fighting(i) != ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is fighting %s!", toupper(*i->short_descr), i->short_descr + 1, char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : juggleRaceName(char_get_fighting(i), false)));
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_fighting(i) == ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is fighting YOU!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_position(i) == POS_SITTING)
    send_to_char(ch, "@w%c%s is sitting here.", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_position(i) == POS_SLEEPING)
    send_to_char(ch, "@w%c%s is sleeping here.", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i))
    send_to_char(ch, "@w%c%s", toupper(*i->short_descr), i->short_descr + 1);
  else if (!char_is_npc(i)) {
    if (char_is_majin(i) && char_aff_flagged(i, AFF_LIQUEFIED)) {
      send_to_char(ch, "@wSeveral blobs of %s colored goo spread out here.@n\n", skin_types[(int)char_get_skin(i)]);
      return;
    }
    if ((char_get_admlevel(ch) > 0 || char_get_admlevel(i) > 0) || char_is_npc(ch)) {
      send_to_char(ch, "@w%s", i->name);
    } else if ((!char_plr_flagged(i, PLR_DISGUISED) && readIntro(ch, i) == 1)) {
      send_to_char(ch, "@w%s", get_i_name(ch, i));
    } else if (!char_plr_flagged(i, PLR_DISGUISED) && readIntro(ch, i) != 1) {
      if (char_get_distfea(i) == DISTFEA_EYE) {
        send_to_char(ch, "@wA %s eyed %s %s", eye_types[(int)char_get_eye(i)], char_mafe(i), juggleRaceName(i, false));
      } else if (char_get_distfea(i) == DISTFEA_HAIR) {
        if (char_is_majin(i)) {
          send_to_char(ch, "@wA %s majin, with a %s forelock,", char_mafe(i), FHA_types[(int)char_get_hairl(i)]);
        } else if (char_is_namek(i)) {
          send_to_char(ch, "@wA namek, with %s antennae,", FHA_types[(int)char_get_hairl(i)]);
        } else if (char_is_arlian(i)) {
          send_to_char(ch, "@wA arlian, with %s antennae,", FHA_types[(int)char_get_hairl(i)]);
        } else if (char_is_icer(i) || char_is_demon(i)) {
          send_to_char(ch, "@wA %s %s, with %s horns", char_mafe(i), juggleRaceName(i, false), FHA_types[(int)char_get_hairl(i)]);
        } else {
          char blarg[MAX_INPUT_LENGTH];
          sprintf(blarg, "%s %s hair %s", hairl_types[(int)char_get_hairl(i)], hairc_types[(int)char_get_hairc(i)], hairs_types[(int)char_get_hairs(i)]);
          send_to_char(ch, "@wA %s %s, with %s", char_mafe(i), juggleRaceName(i, false), char_get_hairl(i) == 0 ? "a bald head" : (blarg));
        }
      } else if (char_get_distfea(i) == DISTFEA_SKIN) {
        send_to_char(ch, "@wA %s skinned %s %s", skin_types[(int)char_get_skin(i)], char_mafe(i), juggleRaceName(i, false));
      } else if (char_get_distfea(i) == DISTFEA_HEIGHT) {
        char *height;
        if (char_is_truffle(i)) {
          if (char_get_pc_height(i) > 70) {
            height = strdup("very tall");
          } else if (char_get_pc_height(i) > 55) {
            height = strdup("tall");
          } else if (char_get_pc_height(i) > 35) {
            height = strdup("average height");
          } else {
            height = strdup("short");
          }
        } else if (char_plr_flagged(i, PLR_OOZARU) || char_get_genome(i, 0) == 11) {
          if (char_get_pc_height(i) * 10 > 2000) {
            height = strdup("very tall");
          } else if (char_get_pc_height(i) * 10 > 1800) {
            height = strdup("tall");
          } else if (char_get_pc_height(i) * 10 > 1500) {
            height = strdup("average height");
          } else {
            height = strdup("short");
          }
        } else {
          if (char_get_pc_height(i) > 200) {
            height = strdup("very tall");
          } else if (char_get_pc_height(i) > 180) {
            height = strdup("tall");
          } else if (char_get_pc_height(i) > 150) {
            height = strdup("average height");
          } else if (char_get_pc_height(i) > 120) {
            height = strdup("short");
          } else {
            height = strdup("very short");
          }
        }
        send_to_char(ch, "@wA %s %s %s", height, char_mafe(i), juggleRaceName(i, false));
        if (height) {
          free(height);
        }
      } else if (char_get_distfea(i) == DISTFEA_WEIGHT) {
        char *height;
        if (char_is_truffle(i)) {
          if (char_get_pc_weight(i) > 35) {
            height = strdup("very heavy");
          } else if (char_get_pc_weight(i) > 25) {
            height = strdup("heavy");
          } else if (char_get_pc_weight(i) > 15) {
            height = strdup("average weight");
          } else {
            height = strdup("welterweight");
          }
        } else if (char_plr_flagged(i, PLR_OOZARU) || char_get_genome(i, 0) == 11) {
          if (char_get_pc_weight(i) * 50 > 6000) {
            height = strdup("very heavy");
          } else if (char_get_pc_weight(i) * 50 > 5000) {
            height = strdup("heavy");
          } else if (char_get_pc_weight(i) * 50 > 4000) {
            height = strdup("average weight");
          } else if (char_get_pc_weight(i) * 50 > 3000) {
            height = strdup("lightweight");
          } else {
            height = strdup("welterweight");
          }
        } else {
          if (char_get_pc_weight(i) > 120) {
            height = strdup("very heavy");
          } else if (char_get_pc_weight(i) > 100) {
            height = strdup("heavy");
          } else if (char_get_pc_weight(i) > 80) {
            height = strdup("average weight");
          } else if (char_get_pc_weight(i) > 60) {
            height = strdup("lightweight");
          } else {
            height = strdup("welterweight");
          }
        }
        send_to_char(ch, "@wA %s %s %s", height, char_mafe(i), juggleRaceName(i, false));
        if (height) {
          free(height);
        }
      }
    } else {
      send_to_char(ch, "@wA disguised %s %s", char_mafe(i), juggleRaceName(i, false));
    }
  }

  if (!char_is_npc(i) || !char_get_fighting(i)) {
    if (char_aff_flagged(i, AFF_INVISIBLE)) {
      send_to_char(ch, ", is invisible");
      count = TRUE;
    }
    if (char_aff_flagged(i, AFF_ETHEREAL)) {
      send_to_char(ch, ", has a halo");
      count = TRUE;
    }
    if (char_aff_flagged(i, AFF_HIDE) && i != ch) {
      send_to_char(ch, ", is hiding");
      if (char_get_skill(i, SKILL_HIDE) && !char_is_npc(ch) && i != ch) {
        improve_skill(i, SKILL_HIDE, 1);
      }
      count = TRUE;
    }
    if (!char_is_npc(i) && !i->desc) {
      send_to_char(ch, ", has a blank stare");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_plr_flagged(i, PLR_WRITING)) {
      send_to_char(ch, ", is writing");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_prf_flagged(i, PRF_BUILDWALK)) {
      send_to_char(ch, ", is buildwalking");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_absorbing(i) && char_get_absorbing(i) != ch) {
      send_to_char(ch, ", is absorbing from %s", char_get_name(char_get_absorbing(i)));
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_grappling(i) && char_get_grappling(i) != ch) {
      send_to_char(ch, ", is grappling with %s", readIntro(ch, char_get_grappling(i)) == 1 ? get_i_name(ch, char_get_grappling(i)) : introd_calc(char_get_grappling(i)));
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_carrying(i) && char_get_carrying(i) != ch) {
      send_to_char(ch, ", is carrying %s", readIntro(ch, char_get_carrying(i)) == 1 ? get_i_name(ch, char_get_carrying(i)) : introd_calc(char_get_carrying(i)));
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_carried_by(i) && char_get_carried_by(i) != ch) {
      send_to_char(ch, ", is being carried by %s", readIntro(ch, char_get_carried_by(i)) == 1 ? get_i_name(ch, char_get_carried_by(i)) : introd_calc(char_get_carried_by(i)));
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_grappling(i) && char_get_grappling(i) == ch) {
      send_to_char(ch, ", is grappling with YOU");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_absorbing(i) && char_get_absorbing(i) == ch) {
      send_to_char(ch, ", is absorbing from YOU");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_absorbing(ch) && char_get_absorbing(ch) == i) {
      send_to_char(ch, ", is being absorbed from by YOU");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_grappling(ch) && char_get_grappling(ch) == i) {
      send_to_char(ch, ", is being grappled with by YOU");
      count = TRUE;
    }
    if (!char_is_npc(i) && char_get_carrying(ch) && char_get_carrying(ch) == i) {
      send_to_char(ch, ", is being carried by you");
      count = TRUE;
    }
    if (!char_is_npc(ch) && !char_is_npc(i) && char_get_fighting(i)) {
      if (!char_plr_flagged(i, PLR_SPAR) || (char_plr_flagged(i, PLR_SPAR) && (!char_plr_flagged(char_get_fighting(i), PLR_SPAR) || char_is_npc(char_get_fighting(i))))) {
        send_to_char(ch, ", is here fighting ");
      }
      if (char_plr_flagged(i, PLR_SPAR) && char_plr_flagged(char_get_fighting(i), PLR_SPAR)) {
        send_to_char(ch, ", is here sparring ");
      }
      if (char_get_fighting(i) == ch) {
        send_to_char(ch, "@rYOU@w");
        count = TRUE;
      } else {
        if (char_get_in_room(i) == char_get_in_room(char_get_fighting(i))) {
          send_to_char(ch, "%s", char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : juggleRaceName(char_get_fighting(i), false)));
          count = TRUE;
        } else {
          send_to_char(ch, "someone who has already left!");
        }
      }
    }
  }
  if (char_get_sits(i)) {
    chair = char_get_sits(i);
    if (char_plr_flagged(i, PLR_HEALT)) {
      send_to_char(ch, "@w is floating inside a healing tank.");
    } else if (count == TRUE) {
      send_to_char(ch, ",@w and%s on %s.", positions[(int)char_get_position(i)], chair->short_description);
    } else if (count == FALSE) {
      send_to_char(ch, "@w%s on %s.", positions[(int)char_get_position(i)], chair->short_description);
    }
  } else if (!char_plr_flagged(i, PLR_PILOTING) && !char_get_sits(i) && (!char_is_npc(i) || !char_get_fighting(i))) {
    if (count == TRUE) {
      send_to_char(ch, "@w, and%s.", positions[(int)char_get_position(i)]);
    }
    if (count == FALSE) {
      send_to_char(ch, "@w%s.", positions[(int)char_get_position(i)]);
    }
  } else if (char_plr_flagged(i, PLR_PILOTING)) {
    send_to_char(ch, "@w, is sitting in the pilot's chair.\r\n");
  } else {

    if (char_get_fighting(i) && !char_is_npc(ch) && !char_is_npc(i)) {
      if (!char_plr_flagged(i, PLR_SPAR)) {
        send_to_char(ch, ", is here fighting ");
      }
      if (char_plr_flagged(i, PLR_SPAR)) {
        send_to_char(ch, ", is here sparring ");
      }
      if (char_get_fighting(i) == ch)
        send_to_char(ch, "@rYOU@w!");
      else {
        if (char_get_in_room(i) == char_get_in_room(char_get_fighting(i)))
          send_to_char(ch, "%s!", char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : juggleRaceName(char_get_fighting(i), false)));
        else
          send_to_char(ch, "someone who has already left!");
      }
    } else if (!char_is_npc(i)) { /* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
    }
  }

  if (char_aff_flagged(ch, AFF_DETECT_ALIGN)) {
    if (char_is_evil(i))
      send_to_char(ch, " (@rRed@[3] Aura)");
    else if (char_is_good(i))
      send_to_char(ch, " (@bBlue@[3] Aura)");
  }
  if (!char_is_npc(i) && char_prf_flagged(i, PRF_AFK))
    send_to_char(ch, " @D(@RAFK@D)");
  else if (!char_is_npc(i) && i->timer > 3)
    send_to_char(ch, " @D(@RIDLE@D)");
  send_to_char(ch, "@n\r\n");

  if (char_get_listenroom(i) > 0) {
    char eaves[300];
    sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.", dirs[char_get_eavesdir(i)]);
    act(eaves, TRUE, i, 0, ch, TO_VICT);
  }
  if (!char_is_npc(i)) {
    if (char_plr_flagged(i, PLR_FISHING)) {
      act("@w...$e is @Cfishing@w.@n", TRUE, i, 0, ch, TO_VICT);
    }
  }
  if (char_plr_flagged(i, PLR_AURALIGHT)) {
    char bloom[MAX_INPUT_LENGTH];
    sprintf(bloom, "...is surrounded by a bright %s aura.@n", aura_types[char_get_aura(i)]);
    act(bloom, TRUE, i, 0, ch, TO_VICT);
  }
  if (char_aff_flagged(i, AFF_SANCTUARY) && !char_get_skill(i, SKILL_AQUA_BARRIER))
    act("@w...$e has a @bbarrier@w around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_FIRESHIELD))
    act("@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", FALSE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_HEALGLOW))
    act("@w...$e has a serene @Cblue@Y glow@w around $s body.", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_EARMOR))
    act("@w...$e has ghostly @Ggreen@w ethereal armor around $s body.", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_SANCTUARY) && char_get_skill(i, SKILL_AQUA_BARRIER))
    act("@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 1)
    act("@w...$e is in the air!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 2)
    act("@w...$e is high in the air!", TRUE, i, 0, ch, TO_VICT);
  if (char_get_kaioken(i) > 0)
    act("@w...@r$e has a red aura around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (!char_is_npc(i) && char_plr_flagged(i, PLR_SPIRAL))
    act("@w...$e is spinning in a vortex!", FALSE, i, 0, ch, TO_VICT);
  if (char_is_transformed(i) && !char_is_android(i) && !char_is_saiyan(i) && !char_is_halfbreed(i))
    act("@w...$e has energy crackling around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_get_charge(i) && !char_is_saiyan(i) && !char_is_halfbreed(i)) {
    char aura[MAX_INPUT_LENGTH];
    sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!", aura_types[char_get_aura(i)]);
    act(aura, TRUE, i, 0, ch, TO_VICT);
  }
  if (!char_plr_flagged(i, PLR_OOZARU) && char_get_charge(i) && char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (!char_plr_flagged(i, PLR_OOZARU) && char_get_charge(i) && !char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i))) {
    char aura[MAX_INPUT_LENGTH];
    sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!", aura_types[char_get_aura(i)]);
    act(aura, TRUE, i, 0, ch, TO_VICT);
  }
  if (!char_plr_flagged(i, PLR_OOZARU) && !char_get_charge(i) && char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e has energy crackling around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_plr_flagged(i, PLR_OOZARU) && char_get_charge(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e is in the form of a @rgreat ape@w!", TRUE, i, 0, ch, TO_VICT);
  if (is_affected(i, AFF_KYODAIKA))
    act("@w...$e has expanded $s body size@w!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_HAYASA))
    act("@w...$e has a soft @cblue@w glow around $s body!", FALSE, i, 0, ch, TO_VICT);
  if (char_plr_flagged(i, PLR_OOZARU) && !char_get_charge(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e has energy crackling around $s @rgreat ape@w body!", TRUE, i, 0, ch, TO_VICT);
  if (char_get_feature(i)) {
    char woo[MAX_STRING_LENGTH];
    sprintf(woo, "@C%s@n", char_get_feature(i));
    act(woo, FALSE, i, 0, ch, TO_VICT);
  }

  if (char_get_rdisplay(i)) {
    if (strcmp(char_get_rdisplay(i), "Empty")) {
      char rdis[MAX_STRING_LENGTH];
      sprintf(rdis, "...%s", char_get_rdisplay(i));
      act(rdis, FALSE, i, 0, ch, TO_VICT);
    }
  }
}

void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i, *j;
  struct hide_node {
    struct hide_node *next;
    struct char_data *hidden;
  } *hideinfo, *lasthide, *tmphide;
  int num;

  hideinfo = lasthide = NULL;

  for (i = list; i; i = i->next_in_room) {
    if (char_aff_flagged(i, AFF_HIDE) && roll_resisted(i, SKILL_HIDE, ch, SKILL_SPOT)) {
      if (char_get_skill(i, SKILL_HIDE) && !char_is_npc(ch) && i != ch) {
        improve_skill(i, SKILL_HIDE, 1);
      }
      CREATE(tmphide, struct hide_node, 1);
      tmphide->next   = NULL;
      tmphide->hidden = i;
      if (!lasthide) {
        hideinfo = lasthide = tmphide;
      } else {
        lasthide->next = tmphide;
        lasthide       = tmphide;
      }
      continue;
    }
  }

  for (i = list; i; i = i->next_in_room) {
    /* hide npcs whose description starts with a '.' from non-holylighted people
    - Idea from Elaseth of TBA */
    if ((ch == i) || (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_HOLYLIGHT) && char_is_npc(i) && i->long_descr && *i->long_descr == '.'))
      continue;

    for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
      if (tmphide->hidden == i)
        break;
    if (tmphide)
      continue;

    if (char_can_see(ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurences of this mob are there? */
        for (j = list; j != i; j = j->next_in_room)
          if ((i->nr == j->nr) &&
              (char_get_position(i) == char_get_position(j)) &&
              (char_get_affected_by(i)[0] == char_get_affected_by(j)[0]) &&
              (char_get_affected_by(i)[1] == char_get_affected_by(j)[1]) &&
              (char_get_affected_by(i)[2] == char_get_affected_by(j)[2]) &&
              (char_get_affected_by(i)[3] == char_get_affected_by(j)[3]) &&
              (!char_get_fighting(i) && !char_get_fighting(j)) &&
              (getCurPL(i) == getEffMaxPL(i) && getCurPL(j) == getEffMaxPL(j)) &&
              !strcmp(char_get_name(i), char_get_name(j))) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              break;
          }
        if (j != i)
          /* This will be true where we have already found this
	   * mob for an earlier "i".  The continue pops us out of
	   * the main "i" for loop.
	   */
          continue;
        for (j = i; j; j = j->next_in_room)
          if ((i->nr == j->nr) &&
              (char_get_position(i) == char_get_position(j)) &&
              (char_get_affected_by(i)[0] == char_get_affected_by(j)[0]) &&
              (char_get_affected_by(i)[1] == char_get_affected_by(j)[1]) &&
              (char_get_affected_by(i)[2] == char_get_affected_by(j)[2]) &&
              (char_get_affected_by(i)[3] == char_get_affected_by(j)[3]) &&
              (!char_get_fighting(i) && !char_get_fighting(j)) &&
              (getCurPL(i) == getEffMaxPL(i) && getCurPL(j) == getEffMaxPL(j)) &&
              !strcmp(char_get_name(i), char_get_name(j))) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              num++;
          }
      }
      /* Now show this mob's name and other stuff */
      send_to_char(ch, "@w");
      if (num > 1)
        send_to_char(ch, "@D(@Rx@Y%2i@D)@n ", num);
      list_one_char(i, ch);
      send_to_char(ch, "@n");
    } /* processed a character we can see */
    else if (room_is_dark(char_get_in_room(ch)) && !char_can_see_in_dark(ch) &&
             char_aff_flagged(i, AFF_INFRAVISION))
      send_to_char(ch, "@wYou see a pair of glowing red eyes looking your way.@n\r\n");
  } /* loop through all characters in room */
}
