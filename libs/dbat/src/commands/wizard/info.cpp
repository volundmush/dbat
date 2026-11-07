/* ************************************************************************
*   File: wizard/info.c                                  Part of CircleMUD *
*  Usage: God commands that inspect the world                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <climits>

#include <unistd.h>

#include "dbat/act.wizard.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/time.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/extract.hpp"
#include "dbat/affect.hpp"
#include "dbat/config.hpp"
#include "dbat/act.other.hpp"
#include "dbat/maputils.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/handler.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/weather.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/house.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/races.hpp"
#include "dbat/class.hpp"
#include "dbat/spells.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/objsave.hpp"
#include "dbat/feats.hpp"
#include "dbat/fight.hpp"
#include "dbat/genolc.hpp"
#include "dbat/screen.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void do_stat_room(GameWorld *g, struct char_data *ch);
static void do_stat_object(GameWorld *g, struct char_data *ch, struct obj_data *j);
static void do_stat_character(GameWorld *g, struct char_data *ch, struct char_data *k);
static size_t print_zone_to_buf(GameWorld *g, char *bufptr, size_t left, zone_rnum zone, int listall);
static void mob_checkload(GameWorld *g, struct char_data *ch, mob_vnum mvnum);
static void obj_checkload(GameWorld *g, struct char_data *ch, obj_vnum ovnum);
static void trg_checkload(GameWorld *g, struct char_data *ch, trig_vnum tvnum);

/* do_finddoor, finds the door(s) that a key goes to */
ACMD(do_finddoor)
{
  int d, num = 0;
  obj_vnum vnum = NOTHING;
  size_t len;
  int nlen;
  room_rnum i;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH] = {0};
  struct char_data *tmp_char;
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Format: finddoor <obj/vnum>\r\n");
  } else if (is_number(arg)) {
    vnum = atoi(arg);
    if (real_object(g, vnum) == NOTHING) {
      send_to_char(g, ch, "There is no object with that vnum.\r\n");
      return;
    }
    obj = &g->obj_proto[real_object(g, vnum)];
  } else {
    generic_find(g, arg,
                 FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP,
                 ch, &tmp_char, &obj);
    if (!obj)
      send_to_char(g, ch, "What key do you want to find a door for?\r\n");
    else
      vnum = obj_get_vnum(g, obj);
  }
  if (vnum != NOTHING) {
    len = snprintf(buf, sizeof(buf), "Doors unlocked by key [%d] %s are:\r\n",
                   vnum, obj_get_short_description(obj));
    for (i = 0; i <= g->top_room; i++) {
      for (d = 0; d < NUM_OF_DIRS; d++) {
        if (rroom_get_dir_option(g, i, d) && rroom_get_dir_option(g, i, d)->key &&
            rroom_get_dir_option(g, i, d)->key == vnum) {
          nlen = snprintf(buf + len, sizeof(buf) - len,
                          "[%3d] Room %d, %s (%s)\r\n",
                          ++num, rroom_get_vnum(g, i),
                          dirs[d], rroom_get_dir_option(g, i, d)->keyword);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      } /* for all directions */
    } /* for all rooms */
    if (num > 0)
      send_to_char(g, ch, "%s", buf);
    else
      send_to_char(g, ch, "No doors were found for key [%d] %s.\r\n",
                   vnum, obj_get_short_description(obj));
  }
}

ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj") && !is_abbrev(buf, "mat") && !is_abbrev(buf, "wtype") && !is_abbrev(buf, "atype"))) {
    send_to_char(g, ch, "Usage: vnum { atype | material | mob | obj | wtype } <name>\r\n");
    return;
  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(g, buf2, ch))
      send_to_char(g, ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(g, buf2, ch))
      send_to_char(g, ch, "No objects by that name.\r\n");

  if (is_abbrev(buf, "mat"))
    if (!vnum_material(g, buf2, ch))
      send_to_char(g, ch, "No materials by that name.\r\n");

  if (is_abbrev(buf, "wtype"))
    if (!vnum_weapontype(g, buf2, ch))
      send_to_char(g, ch, "No weapon types by that name.\r\n");

  if (is_abbrev(buf, "atype"))
    if (!vnum_armortype(g, buf2, ch))
      send_to_char(g, ch, "No armor types by that name.\r\n");
}

#define ZOCMD g->zones[zrnum].cmd[subcmd]

static void list_zone_commands_room(GameWorld *g, struct char_data *ch, room_vnum rvnum)
{
  zone_rnum zrnum = real_zone_by_thing(g, rvnum);
  room_rnum rrnum = real_room(g, rvnum), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == NOWHERE || rrnum == NOWHERE) {
    send_to_char(g, ch, "No zone information available.\r\n");
    return;
  }

  send_to_char(g, ch, "Zone commands in this room:@y\r\n");
  while (ZOCMD.command != 'S') {
    switch (ZOCMD.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = ZOCMD.arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = ZOCMD.arg1;
        break;
      default:
        break;
    }
    if (cmd_room == rrnum) {
      count++;
      /* start listing */
      switch (ZOCMD.command) {
        case 'M':
          send_to_char(g, ch, "%sLoad %s@y [@c%d@y], MaxMud : %d, MaxR : %d, Chance : %d\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       char_get_sdesc(&g->mob_proto[ZOCMD.arg1]),
                       g->mob_index[ZOCMD.arg1].vnum, ZOCMD.arg2,
                       ZOCMD.arg4, ZOCMD.arg5);
          break;
        case 'G':
          send_to_char(g, ch, "%sGive it %s@y [@c%d@y], Max : %d, Chance : %d\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(&g->obj_proto[ZOCMD.arg1]),
                       g->obj_index[ZOCMD.arg1].vnum,
                       ZOCMD.arg2, ZOCMD.arg5);
          break;
        case 'O':
          send_to_char(g, ch, "%sLoad %s@y [@c%d@y], Max : %d, MaxR : %d, Chance : %d\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(&g->obj_proto[ZOCMD.arg1]),
                       g->obj_index[ZOCMD.arg1].vnum,
                       ZOCMD.arg2, ZOCMD.arg4, ZOCMD.arg5);
          break;
        case 'E':
          send_to_char(g, ch, "%sEquip with %s@y [@c%d@y], %s, Max : %d, Chance : %d\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(&g->obj_proto[ZOCMD.arg1]),
                       g->obj_index[ZOCMD.arg1].vnum,
                       equipment_types[ZOCMD.arg3],
                       ZOCMD.arg2, ZOCMD.arg5);
          break;
        case 'P':
          send_to_char(g, ch, "%sPut %s@y [@c%d@y] in %s@y [@c%d@y], Max : %d, Chance : %d\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(&g->obj_proto[ZOCMD.arg1]),
                       g->obj_index[ZOCMD.arg1].vnum,
                       obj_get_short_description(&g->obj_proto[ZOCMD.arg3]),
                       g->obj_index[ZOCMD.arg3].vnum,
                       ZOCMD.arg2, ZOCMD.arg5);
          break;
        case 'R':
          send_to_char(g, ch, "%sRemove %s@y [@c%d@y] from room.\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(&g->obj_proto[ZOCMD.arg2]),
                       g->obj_index[ZOCMD.arg2].vnum);
          break;
        case 'D':
          send_to_char(g, ch, "%sSet door %s as %s.\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       dirs[ZOCMD.arg2],
                       ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open");
          break;
        case 'T':
          send_to_char(g, ch, "%sAttach trigger @c%s@y [@c%d@y] to %s\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       g->trig_index[ZOCMD.arg2]->proto->name,
                       g->trig_index[ZOCMD.arg2]->vnum,
                       ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" : ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))));
          break;
        case 'V':
          send_to_char(g, ch, "%sAssign global %s:%d to %s = %s\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       ZOCMD.sarg1, ZOCMD.arg2,
                       ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" : ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))),
                       ZOCMD.sarg2);
          break;
        default:
          send_to_char(g, ch, "<Unknown Command>\r\n");
          break;
      }
    }
    subcmd++;
  }
  send_to_char(g, ch, "@n");
  if (!count)
    send_to_char(g, ch, "None!\r\n");
}
#undef ZOCMD

static void do_stat_room(GameWorld *g, struct char_data *ch)
{
  char buf2[MAX_STRING_LENGTH];
  struct extra_descr_data *desc;
  struct room_data *rm = &g->rooms[char_get_in_room(ch)];
  int i, found, column;
  struct obj_data *j;
  struct char_data *k;

  send_to_char(g, ch, "Room name: @c%s@n\r\n", room_get_name(rm));

  sprinttype(room_get_sector_type(rm), sector_types, buf2, sizeof(buf2));
  send_to_char(g, ch, "Zone: [%3d], VNum: [@g%5d@n], RNum: [%5d], IDNum: [%5ld], Type: %s\r\n",
               g->zones[room_get_zone(rm)].number, room_get_number(rm), char_get_in_room(ch),
               (long)room_get_number(rm) + ROOM_ID_BASE, buf2);

  sprintbitarray(room_get_room_flags(rm), room_bits, RF_ARRAY_MAX, buf2, sizeof(buf2));
  send_to_char(g, ch, "Room Damage: %d, Room Effect: %d\r\n", room_get_dmg(rm), room_get_geffect(rm));
  send_to_char(g, ch, "SpecProc: %s, Flags: %s\r\n", room_get_func(rm) == nullptr ? "None" : "Exists", buf2);

  send_to_char(g, ch, "Description:\r\n%s", room_get_description(rm) ? room_get_description(rm) : "  None.\r\n");

  if (room_get_ex_description(rm)) {
    send_to_char(g, ch, "Extra descs:");
    for (desc = room_get_ex_description(rm); desc; desc = desc->next)
      send_to_char(g, ch, " [@c%s@n]", desc->keyword);
    send_to_char(g, ch, "\r\n");
  }

  send_to_char(g, ch, "Chars present:");
  column = 14; /* ^^^ strlen ^^^ */
  for (found = false, k = room_get_people(rm); k; k = char_get_next_in_room(k)) {
    if (!char_can_see(g, ch, k))
      continue;

    column += send_to_char(g, ch, "%s @y%s@n(%s)", found++ ? "," : "", char_get_name(k),
                           !char_is_npc(k) ? "PC" : (!char_is_mob(g, k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(g, ch, "%s\r\n", char_get_next_in_room(k) ? "," : "");
      found  = false;
      column = 0;
    }
  }

  if (room_get_contents(rm)) {
    send_to_char(g, ch, "Contents:@g");
    column = 9; /* ^^^ strlen ^^^ */

    for (found = 0, j = room_get_contents(rm); j; j = obj_get_next_content(j)) {
      if (!char_can_see_obj(g, ch, j))
        continue;

      column += send_to_char(g, ch, "%s %s", found++ ? "," : "", obj_get_short_description(j));
      if (column >= 62) {
        send_to_char(g, ch, "%s\r\n", obj_get_next_content(j) ? "," : "");
        found  = false;
        column = 0;
      }
    }
    send_to_char(g, ch, "@n");
  }

  for (i = 0; i < NUM_OF_DIRS; i++) {
    char buf1[128];

    if (!room_get_dir_option(rm, i))
      continue;

    if (room_get_dir_option(rm, i)->to_room == NOWHERE)
      snprintf(buf1, sizeof(buf1), " @cNONE@n");
    else
      snprintf(buf1, sizeof(buf1), "@c%5d@n", rroom_get_vnum(g, room_get_dir_option(rm, i)->to_room));

    sprintbit(room_get_dir_option(rm, i)->exit_info, exit_bits, buf2, sizeof(buf2));

    send_to_char(g, ch, "Exit @c%-5s@n:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n  DC Lock: [%2d], DC Hide: [%2d], DC Skill: [%4s], DC Move: [%2d]\r\n%s",
                 dirs[i], buf1,
                 room_get_dir_option(rm, i)->key == NOTHING ? -1 : room_get_dir_option(rm, i)->key,
                 room_get_dir_option(rm, i)->keyword ? room_get_dir_option(rm, i)->keyword : "None", buf2,
                 room_get_dir_option(rm, i)->dclock, room_get_dir_option(rm, i)->dchide,
                 room_get_dir_option(rm, i)->dcskill == 0 ? "None" : g->spell_info[room_get_dir_option(rm, i)->dcskill].name, room_get_dir_option(rm, i)->dcmove,
                 room_get_dir_option(rm, i)->general_description ? room_get_dir_option(rm, i)->general_description : "  No exit description.\r\n");
  }

  /* check the room for a script */
  do_sstat_room(g, ch, rm);

  list_zone_commands_room(g, ch, room_get_number(rm));
}

static void do_stat_object(GameWorld *g, struct char_data *ch, struct obj_data *j)
{
  int i, found;
  obj_vnum vnum;
  struct obj_data *j2;
  struct char_data *sitter;
  struct extra_descr_data *desc;
  char buf[MAX_STRING_LENGTH];

  vnum = obj_get_vnum(g, j);
  if (obj_get_lload(j) > 0) {
    char *tmstr;
    time_t last_load             = obj_get_lload(j);
    tmstr                        = asctime(localtime(&last_load));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    send_to_char(g, ch, "LOADED DROPPED: [%s]\r\n", tmstr);
  }
  if (obj_get_vnum(g, j) == 65) {
    send_to_char(g, ch, "Healing Tank Charge Level: [%d]\r\n", obj_get_healcharge(j));
  }
  send_to_char(g, ch, "Name: '%s', Keywords: %s, Size: %s\r\n",
               obj_get_short_description(j) ? obj_get_short_description(j) : "<None>", obj_get_name(j),
               size_names[obj_get_size(j)]);

  sprinttype(obj_get_type_flag(j), item_types, buf, sizeof(buf));
  send_to_char(g, ch, "VNum: [@g%5d@n], RNum: [%5d], Idnum: [%5d], Type: %s, SpecProc: %s\r\n",
               vnum, obj_get_item_number(j), obj_get_id(j), buf, obj_get_spec(g, j) ? "Exists" : "None");

  time_t generation = obj_get_generation(j);
  send_to_char(g, ch, "Generation time: @g%s@nUnique ID: @g%" PRId64 "@n\r\n",
               ctime(&generation), obj_get_unique_id(j));

  send_to_char(g, ch, "Object Hit Points: [ @g%3d@n/@g%3d@n]\r\n",
               obj_get_value(j, VAL_ALL_HEALTH), obj_get_value(j, VAL_ALL_MAXHEALTH));

  send_to_char(g, ch, "Object loaded in room: @y%d@n\r\n",
               obj_get_room_loaded(j));

  send_to_char(g, ch, "Object Material: @y%s@n\r\n",
               material_names[obj_get_value(j, 7)]);

  if (obj_get_sitting(j)) {
    sitter = obj_get_sitting(j);
    send_to_char(g, ch, "HOLDING: %s\r\n", char_get_name(sitter));
  }

  if (obj_get_ex_description(j)) {
    send_to_char(g, ch, "Extra descs:");
    for (desc = obj_get_ex_description(j); desc; desc = desc->next)
      send_to_char(g, ch, " [@g%s@n]", desc->keyword);
    send_to_char(g, ch, "\r\n");
  }

  sprintbitarray(obj_get_wear_flags(j), wear_bits, TW_ARRAY_MAX, buf, sizeof(buf));
  send_to_char(g, ch, "Can be worn on: %s\r\n", buf);

  sprintbitarray(obj_get_bitvector(j), affected_bits, AF_ARRAY_MAX, buf, sizeof(buf));
  send_to_char(g, ch, "Set char bits : %s\r\n", buf);

  sprintbitarray(obj_get_extra_flags(j), extra_bits, EF_ARRAY_MAX, buf, sizeof(buf));
  send_to_char(g, ch, "Extra flags   : %s\r\n", buf);

  send_to_char(g, ch, "Weight: %" PRId64 ", Value: %d, Cost/day: %d, Timer: %d, Min Level: %d\r\n",
               obj_get_weight(j), obj_get_cost(j), obj_get_cost_per_day(j), obj_get_timer(j), obj_get_level(j));

  send_to_char(g, ch, "In room: %d (%s), ", rroom_get_vnum(g, obj_get_in_room(j)),
               obj_get_in_room(j) == NOWHERE ? "Nowhere" : rroom_get_name(g, obj_get_in_room(j)));

  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  send_to_char(g, ch, "In object: %s, ", obj_get_in_obj(j) ? obj_get_short_description(obj_get_in_obj(j)) : "None");
  send_to_char(g, ch, "Carried by: %s, ", obj_get_carried_by(j) ? char_get_name(obj_get_carried_by(j)) : "Nobody");
  send_to_char(g, ch, "Worn by: %s\r\n", obj_get_worn_by(j) ? char_get_name(obj_get_worn_by(j)) : "Nobody");

  switch (obj_get_type_flag(j)) {
    case ITEM_LIGHT:
      if (obj_get_value(j, VAL_LIGHT_HOURS) == -1)
        send_to_char(g, ch, "Hours left: Infinite\r\n");
      else
        send_to_char(g, ch, "Hours left: [%d]\r\n", obj_get_value(j, VAL_LIGHT_HOURS));
      break;
    case ITEM_SCROLL:
      send_to_char(g, ch, "Spell: (Level %d) %s\r\n", obj_get_value(j, VAL_SCROLL_LEVEL),
                   skill_name(g, obj_get_value(j, VAL_SCROLL_SPELL1)));
      break;
    case ITEM_POTION:
      send_to_char(g, ch, "Spells: (Level %d) %s, %s, %s\r\n", obj_get_value(j, VAL_POTION_LEVEL),
                   skill_name(g, obj_get_value(j, VAL_POTION_SPELL1)),
                   skill_name(g, obj_get_value(j, VAL_POTION_SPELL2)),
                   skill_name(g, obj_get_value(j, VAL_POTION_SPELL3)));
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      send_to_char(g, ch, "Spell: %s at level %d, %d (of %d) charges remaining\r\n",
                   skill_name(g, obj_get_value(j, VAL_STAFF_SPELL)), obj_get_value(j, VAL_STAFF_LEVEL),
                   obj_get_value(j, VAL_STAFF_CHARGES), obj_get_value(j, VAL_STAFF_MAXCHARGES));
      break;
    case ITEM_WEAPON:
      send_to_char(g, ch, "Weapon Type: %s, Todam: %dd%d, Message type: %d\r\n",
                   weapon_type[obj_get_value(j, VAL_WEAPON_SKILL)],
                   obj_get_value(j, VAL_WEAPON_DAMDICE),
                   obj_get_value(j, VAL_WEAPON_DAMSIZE),
                   obj_get_value(j, VAL_WEAPON_DAMTYPE));
      send_to_char(g, ch, "Average damage per round %.1f\r\n", ((obj_get_value(j, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * obj_get_value(j, VAL_WEAPON_DAMDICE));
      send_to_char(g, ch, "Crit type: %s, Crit range: %d-20\r\n",
                   crit_type[obj_get_value(j, 6)], 20 - obj_get_value(j, 8));
      break;
    case ITEM_ARMOR:
      send_to_char(g, ch, "Armor Type: %s, AC-apply: [%d]\r\n", armor_type[obj_get_value(j, VAL_ARMOR_SKILL)], obj_get_value(j, VAL_ARMOR_APPLYAC));
      send_to_char(g, ch, "Max dex bonus: %d, Armor penalty: %d, Spell failure: %d\r\n",
                   obj_get_value(j, VAL_ARMOR_MAXDEXMOD), obj_get_value(j, VAL_ARMOR_CHECK), obj_get_value(j, VAL_ARMOR_SPELLFAIL));
      break;
    case ITEM_TRAP:
      send_to_char(g, ch, "Spell: %d, - Hitpoints: %d\r\n", obj_get_value(j, VAL_TRAP_SPELL), obj_get_value(j, VAL_TRAP_HITPOINTS));
      break;
    case ITEM_CONTAINER:
      sprintbit(obj_get_value(j, VAL_CONTAINER_FLAGS), container_bits, buf, sizeof(buf));
      send_to_char(g, ch, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n",
                   obj_get_value(j, VAL_CONTAINER_CAPACITY), buf, obj_get_value(j, VAL_CONTAINER_KEY),
                   YESNO(obj_get_value(j, VAL_CONTAINER_CORPSE)));
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      sprinttype(obj_get_value(j, VAL_DRINKCON_LIQUID), drinks, buf, sizeof(buf));
      send_to_char(g, ch, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n",
                   obj_get_value(j, VAL_DRINKCON_CAPACITY), obj_get_value(j, VAL_DRINKCON_HOWFULL), YESNO(obj_get_value(j, VAL_DRINKCON_POISON)), buf);
      break;
    case ITEM_NOTE:
      send_to_char(g, ch, "Tongue: %d\r\n", obj_get_value(j, VAL_NOTE_LANGUAGE));
      break;
    case ITEM_KEY:
      /* Nothing */
      break;
    case ITEM_FOOD:
      send_to_char(g, ch, "Makes full: %d, Poisoned: %s\r\n", obj_get_value(j, VAL_FOOD_FOODVAL), YESNO(obj_get_value(j, VAL_FOOD_POISON)));
      break;
    case ITEM_MONEY:
      send_to_char(g, ch, "Coins: %d\r\n", obj_get_value(j, VAL_MONEY_SIZE));
      break;
    default:
      send_to_char(g, ch, "Values 0-12: [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]\r\n",
                   obj_get_value(j, 0), obj_get_value(j, 1),
                   obj_get_value(j, 2), obj_get_value(j, 3),
                   obj_get_value(j, 4), obj_get_value(j, 5),
                   obj_get_value(j, 6), obj_get_value(j, 7),
                   obj_get_value(j, 8), obj_get_value(j, 9),
                   obj_get_value(j, 10), obj_get_value(j, 11));
      break;
  }

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (obj_get_contains(j)) {
    int column;

    send_to_char(g, ch, "\r\nContents:@g");
    column = 9; /* ^^^ strlen ^^^ */

    for (found = 0, j2 = obj_get_contains(j); j2; j2 = obj_get_next_content(j2)) {
      column += send_to_char(g, ch, "%s %s", found++ ? "," : "", obj_get_short_description(j2));
      if (column >= 62) {
        send_to_char(g, ch, "%s\r\n", obj_get_next_content(j2) ? "," : "");
        found  = false;
        column = 0;
      }
    }
    send_to_char(g, ch, "@n");
  }

  found = false;
  send_to_char(g, ch, "Affections:");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj_get_affected(j)[i].modifier) {
      sprinttype(obj_get_affected(j)[i].location, apply_types, buf, sizeof(buf));
      send_to_char(g, ch, "%s %+d to %s", found++ ? "," : "", obj_get_affected(j)[i].modifier, buf);
      switch (obj_get_affected(j)[i].location) {
        case APPLY_FEAT:
          send_to_char(g, ch, " (%s)", g->feat_list[obj_get_affected(j)[i].specific].name);
          break;
        case APPLY_SKILL:
          send_to_char(g, ch, " (%s)", g->spell_info[obj_get_affected(j)[i].specific].name);
          break;
      }
    }
  if (!found)
    send_to_char(g, ch, " None");

  send_to_char(g, ch, "\r\n");

  /* check the object for a script */
  do_sstat_object(g, ch, j);
}

static void do_stat_character(GameWorld *g, struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i, i2, column, found = false;
  struct obj_data *j;
  struct obj_data *chair;
  struct follow_type *fol;
  struct affected_type *aff;

  if (char_is_npc(k)) {
    char *tmstr;
    time_t last_played           = char_get_lastpl(k);
    tmstr                        = asctime(localtime(&last_played));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    send_to_char(g, ch, "LOADED AT: [%s]\r\n", tmstr);
  }
  sprinttype(char_get_sex(k), genders, buf, sizeof(buf));
  send_to_char(g, ch, "%s %s '%s'  IDNum: [%5d], In room [%5d], Loadroom : [%5d]\r\n",
               buf, (!char_is_npc(k) ? "PC" : (!char_is_mob(g, k) ? "NPC" : "MOB")),
               char_get_name(k), char_is_npc(k) ? char_get_id(k) : char_get_idnum(k), rroom_get_vnum(g, char_get_in_room(k)), char_is_npc(k) ? char_get_hometown(k) : char_get_load_room(g, k));

  send_to_char(g, ch, "DROOM: [%5d]\r\n", char_get_droom(k));
  if (char_is_mob(g, k)) {
    if (char_get_master_id(k) > -1)
      snprintf(buf, sizeof(buf), ", Master: %s", get_name_by_id(g, char_get_master_id(k)));
    else
      buf[0] = 0;
    send_to_char(g, ch, "Keyword: %s, VNum: [%5d], RNum: [%5d]%s\r\n", char_get_alias(k),
                 char_get_mob_vnum(g, k), char_get_nr(k), buf);
  } else

    send_to_char(g, ch, "Title: %s\r\n", char_get_raw_title(k) ? char_get_raw_title(k) : "<None>");

  send_to_char(g, ch, "L-Des: %s@n", char_get_ldesc(k) ? char_get_ldesc(k) : "<None>\r\n");
  if (CONFIG_ALLOW_MULTICLASS) {
    strlcpy(buf, class_desc_str(k, 1, 0), sizeof(buf));
  } else {
    snprintf(buf, sizeof(buf), "%s", char_sensei_name(k));
  }
  snprintf(buf2, sizeof(buf2), "%s", char_true_race_name(k));
  send_to_char(g, ch, "Class: %s, Race: %s, Lev: [@y%2d(%dHD+%dcl+%d)@n], XP: [@y%" PRId64 "@n]\r\n",
               buf, buf2, char_get_level(k), char_get_race_level(k),
               char_get_class_level(k), char_get_level_adj(k), char_get_exp(k));

  if (!char_is_npc(k)) {
    char buf1[64], cmbuf2[64];
    time_t created = char_get_time_created(k), logon = char_get_time_logon(k);

    strlcpy(buf1, asctime(localtime(&created)), sizeof(buf1));
    strlcpy(cmbuf2, asctime(localtime(&logon)), sizeof(cmbuf2));
    buf1[10] = cmbuf2[10] = '\0';

    send_to_char(g, ch, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
                 buf1, cmbuf2, (int)char_get_time_played(k) / 3600,
                 (int)((char_get_time_played(k) % 3600) / 60), age(g, k)->year);

    if (char_get_desc(k) != nullptr) {
      send_to_char(g, ch, "@YOwned by User@D: [@C%s@D]@n\r\n", char_get_user(k));
    } else {
      send_to_char(g, ch, "@YOwned by User@D: [@C%s@D]@n\r\n", char_get_loguser(k));
    }
    if (!char_is_npc(k)) {
      send_to_char(g, ch, "@RCharacter Deaths@D: @r%d@n\r\n", char_get_dcount(k));
    }

    send_to_char(g, ch, "Hometown: [%d], Align: [%4d]", char_get_hometown(k),
                 char_get_alignment(k));

    /*. Display OLC zone for immorts .*/
    if (char_get_admlevel(k) >= ADMLVL_BUILDER) {
      if (char_get_olc_zone(g, k) == AEDIT_PERMISSION)
        send_to_char(g, ch, ", OLC[@cActions@n]");
      else if (char_get_olc_zone(g, k) == HEDIT_PERMISSION)
        send_to_char(g, ch, ", OLC[@cHedit@n]");
      else if (char_get_olc_zone(g, k) == NOWHERE)
        send_to_char(g, ch, ", OLC[@cOFF@n]");
      else
        send_to_char(g, ch, ", OLC: [@c%d@n]", char_get_olc_zone(g, k));
    }
    send_to_char(g, ch, "\r\n");
  }
  send_to_char(g, ch, "Str: [@c%d@n]  Int: [@c%d@n]  Wis: [@c%d@n]  "
                   "Dex: [@c%d@n]  Con: [@c%d@n]  Cha: [@c%d@n]\r\n",
               char_get_aff_abils_str(k), char_get_aff_abils_intel(k), char_get_aff_abils_wis(k), char_get_aff_abils_dex(k), char_get_aff_abils_con(k), char_get_aff_abils_cha(k));

  send_to_char(g, ch, "PL :[@g%12s@n]  KI :[@g%12s@n]  ST :[@g%12s@n]\r\n",
               add_commas(getCurPL(g, k)), add_commas((getCurKI(g, k))), add_commas((getCurST(g, k))));
  send_to_char(g, ch, "MPL:[@g%12s@n]  MKI:[@g%12s@n]  MST:[@g%12s@n]\r\n",
               add_commas(getEffMaxPL(g, k)), add_commas(getMaxKI(g, k)), add_commas(getMaxST(g, k)));
  send_to_char(g, ch, "BPL:[@g%12s@n]  BKI:[@g%12s@n]  BST:[@g%12s@n]\r\n",
               add_commas((getBasePL(k))), add_commas((getBaseKI(k))), add_commas((getBaseST(k))));
  send_to_char(g, ch, "LF :[@g%12s@n]  MLF:[@g%12s@n]  LFP:[@g%3d@n]\r\n",
               add_commas((getCurLF(g, k))), add_commas((getMaxLF(g, k))), char_get_lifeperc(k));

  if (char_get_admlevel(k))
    send_to_char(g, ch, "Admin Level: [@y%d - %s@n]\r\n", char_get_admlevel(k), admin_level_names[char_get_admlevel(k)]);

  send_to_char(g, ch, "Coins: [%9d], Bank: [%9d] (Total: %d)\r\n",
               char_get_gold(k), char_get_bank_gold(k), char_get_gold(k) + char_get_bank_gold(k));

  send_to_char(g, ch, "Armor: [%d ]\r\n", char_get_armor(k));

  sprinttype(char_get_position(k), position_types, buf, sizeof(buf));
  send_to_char(g, ch, "Pos: %s, Fighting: %s", buf, char_get_fighting(k) ? char_get_name(char_get_fighting(k)) : "Nobody");

  if (char_get_desc(k)) {
    sprinttype(desc_get_connected(char_get_desc(k)), connected_types, buf, sizeof(buf));
    send_to_char(g, ch, ", Connected: %s", buf);
  }

  if (char_is_npc(k)) {
    sprinttype(char_get_mob_specials_default_pos(k), position_types, buf, sizeof(buf));
    send_to_char(g, ch, ", Default position: %s\r\n", buf);
    sprintbitarray(char_get_act(k), action_bits, PM_ARRAY_MAX, buf, sizeof(buf));
    send_to_char(g, ch, "NPC flags: @c%s@n\r\n", buf);
  } else {
    send_to_char(g, ch, ", Idle Timer (in tics) [%d]\r\n", char_get_timer(k));

    sprintbitarray(char_get_act(k), player_bits, PM_ARRAY_MAX, buf, sizeof(buf));
    send_to_char(g, ch, "PLR: @c%s@n\r\n", buf);

    sprintbitarray(char_get_pref(g, k), preference_bits, PR_ARRAY_MAX, buf, sizeof(buf));
    send_to_char(g, ch, "PRF: @g%s@n\r\n", buf);
  }

  if (char_is_mob(g, k)) {
    send_to_char(g, ch, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
                 (g->mob_index[char_get_nr(k)].func ? "Exists" : "None"),
                 char_get_mob_specials_damnodice(k), char_get_mob_specials_damsizedice(k));
    send_to_char(g, ch, "Average damage per round %.1f (%.1f [BHD] + %d [STR MOD])\r\n", (((char_get_mob_specials_damsizedice(k)) + 1) / 2.0) * (char_get_mob_specials_damnodice(k)) + ability_mod_value(char_get_aff_abils_str(k)), (((char_get_mob_specials_damsizedice(k)) + 1) / 2.0) * (char_get_mob_specials_damnodice(k)), ability_mod_value(char_get_aff_abils_str(k)));
  }

  int counts = 0, total = 0;
  for (i = 0, j = char_get_inventory(k); j; j = obj_get_next_content(j), i++) {
    counts += check_insidebag(j, 0.5);
    counts++;
  }
  total = counts;
  total += i;
  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(k, i)) {
      i2++;
      total += check_insidebag(char_get_equipment(k, i), 0.5) + 1;
    }
  send_to_char(g, ch, "Carried: weight: %d, Total Items (includes bagged items): %d, EQ: %d\r\n", char_get_carry_weight(k), total, i2);

  if (!char_is_npc(k))
    send_to_char(g, ch, "Hunger: %d, Thirst: %d, Drunk: %d\r\n", char_get_conditions(g, k, HUNGER), char_get_conditions(g, k, THIRST), char_get_conditions(g, k, DRUNK));

  column = send_to_char(g, ch, "Master is: %s, Followers are:", char_get_master(k) ? char_get_name(char_get_master(k)) : "<none>");
  if (!char_get_followers(k))
    send_to_char(g, ch, " <none>\r\n");
  else {
    for (fol = char_get_followers(k); fol; fol = fol->next) {
      column += send_to_char(g, ch, "%s %s", found++ ? "," : "", char_pers(g, fol->follower, ch));
      if (column >= 62) {
        send_to_char(g, ch, "%s\r\n", fol->next ? "," : "");
        found  = false;
        column = 0;
      }
    }
    if (column != 0)
      send_to_char(g, ch, "\r\n");
  }

  if (char_get_sits(k)) {
    chair = char_get_sits(k);
    send_to_char(g, ch, "Is on: %s@n\r\n", obj_get_short_description(chair));
  }

  /* Showing the bitvector */
  sprintbitarray(char_get_affected_by(k), affected_bits, AF_ARRAY_MAX, buf, sizeof(buf));
  send_to_char(g, ch, "AFF: @y%s@n\r\n", buf);

  /* Routine to show what spells a char is affected by */
  if (char_get_affected(k)) {
    for (aff = char_get_affected(k); aff; aff = aff->next) {
      send_to_char(g, ch, "SPL: (%3dhr) @c%-21s@n ", aff->duration + 1, skill_name(g, aff->type));

      if (aff->modifier)
        send_to_char(g, ch, "%+d to %s", aff->modifier, apply_types[(int)aff->location]);

      if (aff->bitvector) {
        if (aff->modifier)
          send_to_char(g, ch, ", ");

        strcpy(buf, affected_bits[aff->bitvector]);
        send_to_char(g, ch, "sets %s", buf);
      }
      send_to_char(g, ch, "\r\n");
    }
  }

  /* check mobiles for a script */
  if (char_is_npc(k)) {
    do_sstat_character(g, ch, k);
    if (char_get_memory(k)) {
      struct script_memory *mem = char_get_memory(k);
      send_to_char(g, ch, "Script memory:\r\n  Remember             Command\r\n");
      while (mem) {
        struct char_data *mc = find_char(g, mem->id);
        if (!mc)
          send_to_char(g, ch, "  ** Corrupted!\r\n");
        else {
          if (mem->cmd)
            send_to_char(g, ch, "  %-20.20s%s\r\n", char_get_name(mc), mem->cmd);
          else
            send_to_char(g, ch, "  %-20.20s <default>\r\n", char_get_name(mc));
        }
        mem = mem->next;
      }
    }
  } else {
    int x, track = 0;

    send_to_char(g, ch, "Bonuses/Negatives:\r\n");

    for (x = 0; x < 30; x++) {
      if (x < 15) {
        if (char_get_bonuses(k, x) > 0) {
          send_to_char(g, ch, "@c%s@n\n", list_bonus[x]);
          track += 1;
        }
      } else {
        if (char_get_bonuses(k, x) > 0) {
          send_to_char(g, ch, "@r%s@n\n", list_bonus[x]);
          track += 1;
        }
      }
    }
    if (track <= 0) {
      send_to_char(g, ch, "@wNone.@n\r\n");
    }
    send_to_char(g, ch, "To see player variables use varstat now.\r\n");
  }
}

ACMD(do_varstat)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "That player is not in the game.\r\n");
    return;
  }
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "Just use stat for an NPC\r\n");
    return;
  }
  /* Display their global variables */
  if (char_get_script(vict) && char_get_script(vict)->global_vars) {
    struct trig_var_data *tv;
    char uname[MAX_INPUT_LENGTH];

    send_to_char(g, ch, "%s's Global Variables:\r\n", char_get_name(vict));

    /* currently, variable context for players is always 0, so it is */
    /* not displayed here. in the future, this might change */
    for (tv = char_get_script(vict)->global_vars; tv; tv = tv->next) {
      if (*(tv->value) == UID_CHAR) {
        find_uid_name(g, tv->value, uname, sizeof(uname));
        send_to_char(g, ch, "    %10s:  [UID]: %s\r\n", tv->name, uname);
      } else
        send_to_char(g, ch, "    %10s:  %s\r\n", tv->name, tv->value);
    }
  }
}

ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *object;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(g, ch, "Stats on who or what or where?\r\n");
    return;
  }
  if (is_abbrev(buf1, "room")) {
    do_stat_room(g, ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char(g, ch, "Stats on which mobile?\r\n");
    else {
      if ((victim = get_char_vis(g, ch, buf2, nullptr, FIND_CHAR_WORLD)) != nullptr)
        do_stat_character(g, ch, victim);
      else
        send_to_char(g, ch, "No such mobile around.\r\n");
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char(g, ch, "Stats on which player?\r\n");
    } else {
      if ((victim = get_player_vis(g, ch, buf2, nullptr, FIND_CHAR_WORLD)) != nullptr)
        do_stat_character(g, ch, victim);
      else
        send_to_char(g, ch, "No such player around.\r\n");
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2)
      send_to_char(g, ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(g, ch, buf2, nullptr, FIND_CHAR_WORLD)) != nullptr)
      do_stat_character(g, ch, victim);
    else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(*char_player_specials_slot(victim), struct player_special_data, 1);
      if (load_char(g, buf2, victim) >= 0) {
        char_to_room(g, victim, 0);
        if (char_get_admlevel(victim) > char_get_admlevel(ch))
          send_to_char(g, ch, "Sorry, you can't do that.\r\n");
        else
          do_stat_character(g, ch, victim);
        extract_char_final(g, victim);
      } else {
        send_to_char(g, ch, "There is no such player.\r\n");
        free_char(g, victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char(g, ch, "Stats on which object?\r\n");
    else {
      if ((object = get_obj_vis(g, ch, buf2, nullptr)) != nullptr)
        do_stat_object(g, ch, object);
      else
        send_to_char(g, ch, "No such object around.\r\n");
    }
  } else if (is_abbrev(buf1, "zone")) {
    if (!*buf2) {
      send_to_char(g, ch, "Stats on which zone?\r\n");
      return;
    }
    print_zone(g, ch, atoi(buf2));
    return;
  } else {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(g, ch, name, &number, char_get_equipment_array(ch))) != nullptr)
      do_stat_object(g, ch, object);
    else if ((object = get_obj_in_list_vis(g, ch, name, &number, char_get_inventory(ch))) != nullptr)
      do_stat_object(g, ch, object);
    else if ((victim = get_char_vis(g, ch, name, &number, FIND_CHAR_ROOM)) != nullptr)
      do_stat_character(g, ch, victim);
    else if ((object = get_obj_in_list_vis(g, ch, name, &number, rroom_get_contents(g, char_get_in_room(ch)))) != nullptr)
      do_stat_object(g, ch, object);
    else if ((victim = get_char_vis(g, ch, name, &number, FIND_CHAR_WORLD)) != nullptr)
      do_stat_character(g, ch, victim);
    else if ((object = get_obj_vis(g, ch, name, &number)) != nullptr)
      do_stat_object(g, ch, object);
    else
      send_to_char(g, ch, "Nothing around by that name.\r\n");
  }
}

ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(g, ch, "Usage: vstat { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(g, ch, "That's not a valid number.\r\n");
    return;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_rnum r_num;

    if ((r_num = real_mobile(g, atoi(buf2))) == NOBODY) {
      send_to_char(g, ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(g, r_num, REAL);
    char_to_room(g, mob, 0);
    do_stat_character(g, ch, mob);
    extract_char(g, mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(g, atoi(buf2))) == NOTHING) {
      send_to_char(g, ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(g, r_num, REAL);
    do_stat_object(g, ch, obj);
    extract_obj(g, obj);
  } else
    send_to_char(g, ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = game_now(g);
  else
    mytime = g->boot_time;

  tmstr                        = asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    send_to_char(g, ch, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = game_now(g) - g->boot_time;
    d      = mytime / 86400;
    h      = (mytime / 3600) % 24;
    m      = (mytime / 60) % 60;

    send_to_char(g, ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
  }
}

ACMD(do_last)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = nullptr;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(g, ch, "For whom do you wish to search?\r\n");
    return;
  }
  CREATE(vict, struct char_data, 1);
  clear_char(vict);
  CREATE(*char_player_specials_slot(vict), struct player_special_data, 1);
  if (load_char(g, arg, vict) < 0) {
    send_to_char(g, ch, "There is no such player.\r\n");
    free_char(g, vict);
    return;
  }
  if ((char_get_admlevel(vict) > char_get_admlevel(ch)) && (char_get_admlevel(ch) < ADMLVL_IMPL)) {
    send_to_char(g, ch, "You are not sufficiently godly for that!\r\n");
    return;
  }

  time_t logon = char_get_time_logon(vict);
  send_to_char(g, ch, "[%5d] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
               char_get_idnum(vict), char_get_level(vict),
               char_race_abbr(vict), char_class_abbr(vict),
               char_get_name(vict), char_get_player_specials(vict)->host && *char_get_player_specials(vict)->host ? char_get_player_specials(vict)->host : "(NOHOST)",
               ctime(&logon));
  free_char(g, vict);
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

/* FIXME: overflow possible */
static size_t print_zone_to_buf(GameWorld *g, char *bufptr, size_t left, zone_rnum zone, int listall)
{
  size_t tmp;

  if (listall) {
    IDXTYPE i;
    int j, k, l, m, n, o;

    tmp = snprintf(bufptr, left,
                   "%3d %-30.30s By: %-10.10s Age: %3d; Reset: %3d (%1d); Range: %5d-%5d\r\n",
                   g->zones[zone].number, g->zones[zone].name, g->zones[zone].builders,
                   g->zones[zone].age, g->zones[zone].lifespan,
                   g->zones[zone].reset_mode,
                   g->zones[zone].bot, g->zones[zone].top);
    i = j = k = l = m = n = o = 0;

    for (i = 0; i < g->top_room; i++)
      if (rroom_get_vnum(g, i) >= g->zones[zone].bot && rroom_get_vnum(g, i) <= g->zones[zone].top)
        j++;

    for (i = 0; i < g->top_obj; i++)
      if (g->obj_index[i].vnum >= g->zones[zone].bot && g->obj_index[i].vnum <= g->zones[zone].top)
        k++;

    for (i = 0; i < g->top_mob; i++)
      if (g->mob_index[i].vnum >= g->zones[zone].bot && g->mob_index[i].vnum <= g->zones[zone].top)
        l++;

    m = count_shops(g, g->zones[zone].bot, g->zones[zone].top);

    for (i = 0; i < g->top_of_trigt; i++)
      if (g->trig_index[i]->vnum >= g->zones[zone].bot && g->trig_index[i]->vnum <= g->zones[zone].top)
        n++;

    o = count_guilds(g, g->zones[zone].bot, g->zones[zone].top);

    tmp += snprintf(bufptr + tmp, left - tmp,
                    "       Zone stats:\r\n"
                    "       ---------------\r\n"
                    "         Rooms:    %2d\r\n"
                    "         Objects:  %2d\r\n"
                    "         Mobiles:  %2d\r\n"
                    "         Shops:    %2d\r\n"
                    "         Triggers: %2d\r\n"
                    "         Guilds:   %2d\r\n",
                    j, k, l, m, n, o);

    return tmp;
  }

  return snprintf(bufptr, left,
                  "%3d %-*s By: %-10.10s Range: %5d-%5d\r\n", g->zones[zone].number,
                  count_color_chars(g->zones[zone].name) + 30, g->zones[zone].name,
                  g->zones[zone].builders, g->zones[zone].bot, g->zones[zone].top);
}

ACMD(do_show)
{
  room_rnum i;
  int j, k, l, con; /* i, j, k to specifics? */
  size_t len;
  int nlen;
  zone_rnum zrn;
  zone_vnum zvn;
  obj_vnum low, high;
  int8_t self            = false;
  struct char_data *vict = nullptr;
  struct obj_data *obj;
  struct descriptor_data *d;
  struct affected_type *aff;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], *strp,
      arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
      {"nothing",    0            }, /* 0 */
      {"zones",      ADMLVL_IMMORT}, /* 1 */
      {"player",     ADMLVL_GOD   },
      {"rent",       ADMLVL_GRGOD },
      {"stats",      ADMLVL_IMMORT},
      {"errors",     ADMLVL_IMPL  }, /* 5 */
      {"death",      ADMLVL_GOD   },
      {"godrooms",   ADMLVL_IMMORT},
      {"shops",      ADMLVL_IMMORT},
      {"houses",     ADMLVL_GOD   },
      {"snoop",      ADMLVL_GRGOD }, /* 10 */
      {"assemblies", ADMLVL_IMMORT},
      {"guilds",     ADMLVL_GOD   },
      {"levels",     ADMLVL_GRGOD },
      {"uniques",    ADMLVL_GRGOD },
      {"affect",     ADMLVL_GRGOD }, /* 15 */
      {"affectv",    ADMLVL_GRGOD },
      {"\n",         0            }
  };

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(g, ch, "Game Info options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= char_get_admlevel(ch))
        send_to_char(g, ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(g, ch, "\r\n");
    return;
  }

  strcpy(arg, two_arguments(argument, field, value)); /* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (char_get_admlevel(ch) < fields[l].level) {
    send_to_char(g, ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = true;
  buf[0] = '\0';

  switch (l) {
    /* show zone */
    case 1:
      /* tightened up by JE 4/6/93 */
      if (self)
        print_zone_to_buf(g, buf, sizeof(buf), rroom_get_zone(g, char_get_in_room(ch)), 1);
      else if (*value && is_number(value)) {
        for (zvn = atoi(value), zrn = 0; zrn <= g->top_zone && g->zones[zrn].number != zvn; zrn++);
        if (zrn <= g->top_zone)
          print_zone_to_buf(g, buf, sizeof(buf), zrn, 1);
        else {
          send_to_char(g, ch, "That is not a valid zone.\r\n");
          return;
        }
      } else
        for (len = zrn = 0; zrn <= g->top_zone; zrn++) {
          nlen = print_zone_to_buf(g, buf + len, sizeof(buf) - len, zrn, 0);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      send_to_char(g, ch, "%s", buf);
      break;

    /* show player */
    case 2:
      if (!*value) {
        send_to_char(g, ch, "A name would help.\r\n");
        return;
      }

      CREATE(vict, struct char_data, 1);
      clear_char(vict);
      CREATE(*char_player_specials_slot(vict), struct player_special_data, 1);
      if (load_char(g, value, vict) < 0) {
        send_to_char(g, ch, "There is no such player.\r\n");
        free_char(g, vict);
        return;
      }
      send_to_char(g, ch, "Player: %-12s (%s) [%2d %s %s]\r\n", char_get_name(vict),
                   genders[(int)char_get_sex(vict)], char_get_level(vict), char_class_abbr(vict), char_race_abbr(vict));
      send_to_char(g, ch, "Au: %-8d  Bal: %-8d  Exp: %" PRId64 "  Align: %-5d\r\n",
                   char_get_gold(vict), char_get_bank_gold(vict), char_get_exp(vict),
                   char_get_alignment(vict));
      if (CONFIG_ALLOW_MULTICLASS)
        send_to_char(g, ch, "Class ranks: %s\r\n", class_desc_str(vict, 1, 0));

      /* ctime() uses static buffer: do not combine. */
      {
        time_t created = char_get_time_created(vict), logon = char_get_time_logon(vict);
        send_to_char(g, ch, "Started: %-20.16s  ", ctime(&created));
        send_to_char(g, ch, "Last: %-20.16s  Played: %3dh %2dm\r\n",
                     ctime(&logon),
                     (int)(char_get_time_played(vict) / 3600),
                     (int)(char_get_time_played(vict) / 60 % 60));
      }
      free_char(g, vict);
      break;

    /* show rent */
    case 3:
      if (!*value) {
        send_to_char(g, ch, "A name would help.\r\n");
        return;
      }
      Crash_listrent(g, ch, value);
      break;

    /* show stats */
    case 4:
      i   = 0;
      j   = 0;
      k   = 0;
      con = 0;
      for (vict = g->characters; vict; vict = char_get_next(vict)) {
        if (char_is_npc(vict))
          j++;
        else if (char_can_see(g, ch, vict)) {
          i++;
          if (char_get_desc(vict))
            con++;
        }
      }
      for (obj = g->objects; obj; obj = obj_get_next(obj))
        k++;
      send_to_char(g, ch,
                   "             @D---   @CCore Stats   @D---\r\n"
                   "  @Y%5d@W players in game  @y%5d@W connected\r\n"
                   "  @Y%5d@W registered\r\n"
                   "  @Y%5d@W mobiles          @y%5d@W prototypes\r\n"
                   "  @Y%5d@W objects          @y%5d@W prototypes\r\n"
                   "  @Y%5d@W rooms            @y%5d@W zones\r\n"
                   "  @Y%5d@W triggers\r\n"
                   "  @Y%5d@W large bufs\r\n"
                   "  @Y%5d@W buf switches     @y%5d@W overflows\r\n"
                   "             @D--- @CMiscellaneous  @D---\r\n"
                   "  @Y%5s@W Mob ki attacks this boot\r\n"
                   "  @Y%5s@W Asssassins Generated@n\r\n"
                   "  @Y%5d@W Wish Selfishness Meter@n\r\n",
                   i, con,
                   g->top_of_p_table + 1,
                   j, g->top_mob + 1,
                   k, g->top_obj + 1,
                   g->top_room + 1, g->top_zone + 1,
                   g->top_of_trigt + 1,
                   g->buf_largecount,
                   g->buf_switches, g->buf_overflows,
                   add_commas(g->mob_specials_used),
                   add_commas(g->number_of_assassins),
                   g->SELFISHMETER);
      break;

    /* show errors */
    case 5:
      len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
      for (i = 0, k = 0; i <= g->top_room; i++)
        for (j = 0; j < NUM_OF_DIRS; j++) {
          if (!rroom_get_dir_option(g, i, j))
            continue;
          if (rroom_get_dir_option(g, i, j)->to_room == 0) {
            nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k, rroom_get_vnum(g, i), count_color_chars(rroom_get_name(g, i)) + 40, rroom_get_name(g, i), QNRM, dirs[j]);
            if (len + nlen >= sizeof(buf) || nlen < 0)
              break;
            len += nlen;
          }
          if (rroom_get_dir_option(g, i, j)->to_room == NOWHERE && !rroom_get_dir_option(g, i, j)->general_description) {
            nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-*s%s (%s)\r\n", ++k, rroom_get_vnum(g, i), count_color_chars(rroom_get_name(g, i)) + 40, rroom_get_name(g, i), QNRM, dirs[j]);
            if (len + nlen >= sizeof(buf) || nlen < 0)
              break;
            len += nlen;
          }
        }
      send_to_char(g, ch, "%s", buf);
      break;

    /* show death */
    case 6:
      len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
      for (i = 0, j = 0; i <= g->top_room; i++)
        if (rroom_flagged(g, i, ROOM_DEATH)) {
          nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, rroom_get_vnum(g, i), rroom_get_name(g, i));
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      send_to_char(g, ch, "%s", buf);
      break;

    /* show godrooms */
    case 7:
      len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
      for (i = 0, j = 0; i <= g->top_room; i++)
        if (rroom_flagged(g, i, ROOM_GODROOM)) {
          nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, rroom_get_vnum(g, i), rroom_get_name(g, i));
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      send_to_char(g, ch, "%s", buf);
      break;

    /* show shops */
    case 8:
      show_shops(g, ch, value);
      break;

    /* show houses */
    case 9:
      hcontrol_list_houses(g, ch);
      break;

    /* show snoop */
    case 10:
      i = 0;
      send_to_char(g, ch, "People currently snooping:\r\n--------------------------\r\n");
      for (d = g->descriptors; d; d = d->next) {
        if (d->snooping == nullptr || d->character == nullptr)
          continue;
        if (desc_get_connected(d) != CON_PLAYING || char_get_admlevel(ch) < char_get_admlevel(d->character))
          continue;
        if (!char_can_see(g, ch, d->character) || char_get_in_room(d->character) == NOWHERE)
          continue;
        i++;
        send_to_char(g, ch, "%-10s - snooped by %s.\r\n", char_get_name(d->snooping->character), char_get_name(d->character));
      }
      if (i == 0)
        send_to_char(g, ch, "No one is currently snooping.\r\n");
      break;
    /* show assembly */
    case 11:
      assemblyListToChar(g, ch);
      break;
    /* show guilds */
    case 12:
      show_guild(g, ch, value);
      break;

    /* show level tables */
    case 13:
      send_to_char(g, ch, "This is not used currently.\r\n");
      break;

    case 14:
      if (*value) {
        if (sscanf(value, "%d-%d", &low, &high) != 2) {
          if (sscanf(value, "%d", &low) != 1) {
            send_to_char(g, ch, "Usage: show uniques, show uniques [vnum], or show uniques [low-high]\r\n");
            return;
          }
          high = low;
        }
      } else {
        low  = -1;
        high = 9999999;
      }
      strp = sprintuniques(g, low, high);
      send_to_char(g, ch, "%s", strp);
      free(strp);
      break;

    case 15:
    case 16:
      if (!*value) {
        low  = 1;
        vict = g->affect_list;
      } else {
        low = 0;
        if (!(vict = get_char_world_vis(g, ch, value, nullptr))) {
          send_to_char(g, ch, "Cannot find that character.\r\n");
          return;
        }
      }
      k = MAX_STRING_LENGTH;
      CREATE(strp, char, k);
      strp[0] = j = 0;
      if (!vict) {
        send_to_char(g, ch, "None.\r\n");
        return;
      }
      do {
        if ((k - j) < (MAX_INPUT_LENGTH * 8)) {
          k *= 2;
          RECREATE(strp, char, k);
        }
        j += snprintf(strp + j, k - j, "Name: %s\r\n", char_get_name(vict));
        aff = char_get_affected(vict);
        for (; aff; aff = aff->next) {
          j += snprintf(strp + j, k - j, "SPL: (%3d%s) @c%-21s@n ", aff->duration + 1,
                        (l == 15) ? "hr" : "rd", skill_name(g, aff->type));

          if (aff->modifier)
            j += snprintf(strp + j, k - j, "%+d to %s", aff->modifier,
                          apply_types[(int)aff->location]);

          if (aff->bitvector) {
            if (aff->modifier)
              j += snprintf(strp + j, k - j, ", ");

            strcpy(field, affected_bits[aff->bitvector]);
            j += snprintf(strp + j, k - j, "sets %s", field);
          }
          j += snprintf(strp + j, k - j, "\r\n");
        }
        vict = char_get_next_affect(vict);
      } while (low && vict);
      send_to_char(g, ch, "%s", strp);
      free(strp);
      break;

    /* show what? */
    default:
      send_to_char(g, ch, "Sorry, I don't understand that.\r\n");
      break;
  }
}

#define PLIST_FORMAT \
  "players [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-m]"

ACMD(do_plist)
{
  int i, len = 0, count = 0;
  char mode, buf[MAX_STRING_LENGTH], name_search[MAX_INPUT_LENGTH], time_str[MAX_STRING_LENGTH];
  struct time_info_data time_away;
  int low = 0, high = 100, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24;

  skip_spaces(&argument);
  strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      if (sscanf(arg, "%d-%d", &low, &high) == 1)
        high = low;
      strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode) {
        case 'l':
          half_chop(buf1, arg, buf);
          sscanf(arg, "%d-%d", &low, &high);
          break;
        case 'n':
          half_chop(buf1, name_search, buf);
          break;
        case 'i':
          strcpy(buf, buf1);
          low = 1;
          break;
        case 'm':
          strcpy(buf, buf1);
          high = 100;
          break;
        case 'd':
          half_chop(buf1, arg, buf);
          if (sscanf(arg, "%d-%d", &low_day, &high_day) == 1)
            high_day = low_day;
          break;
        case 'h':
          half_chop(buf1, arg, buf);
          if (sscanf(arg, "%d-%d", &low_hr, &high_hr) == 1)
            high_hr = low_hr;
          break;
        default:
          send_to_char(g, ch, "%s\r\n", PLIST_FORMAT);
          return;
      }
    } else {
      send_to_char(g, ch, "%s\r\n", PLIST_FORMAT);
      return;
    }
  }

  len = 0;
  len += snprintf(buf + len, sizeof(buf) - len, "@W[ Id] (Lv) Name         Last@n\r\n"
                                                "%s-----------------------------------------------%s\r\n",
                  CCCYN(g, ch, C_NRM),
                  CCNRM(g, ch, C_NRM));

  for (i = 0; i <= g->top_of_p_table; i++) {
    if (IS_SET(g->player_table[i].flags, PINDEX_DELETED)) {
      len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] <DELETED> --Will be removed next boot.\r\n", g->player_table[i].id);
      continue;
    }

    if (g->player_table[i].level < low || g->player_table[i].level > high)
      continue;

    time_away = *real_time_passed(game_now(g), g->player_table[i].last);

    if (*name_search && strcasecmp(name_search, g->player_table[i].name))
      continue;

    if (time_away.day > high_day || time_away.day < low_day)
      continue;
    if (time_away.hours > high_hr || time_away.hours < low_hr)
      continue;

    strcpy(time_str, asctime(localtime(&g->player_table[i].last)));
    time_str[strlen(time_str) - 1] = '\0';

    len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] (%2d) %-12s %s\r\n",
                    g->player_table[i].id, g->player_table[i].level,
                    CAP(strdup(g->player_table[i].name)), time_str);
    count++;
  }
  snprintf(buf + len, sizeof(buf) - len, "%s-----------------------------------------------%s\r\n"
                                         "%d players listed.\r\n",
           CCCYN(g, ch, C_NRM), CCNRM(g, ch, C_NRM), count);
  send_to_char(g, ch, "%s", buf);
}

/******************************************************************************/
/*                         Zone Checker Code below                            */
/******************************************************************************/

/*mob limits*/
#define MAX_DAMAGE_MOD_ALLOWED MAX(char_get_level(mob) / 5, 2)
#define MAX_GOLD_ALLOWED       char_get_level(mob) * 3000
#define MAX_EXP_ALLOWED        char_get_level(mob) * char_get_level(mob) * 120
constexpr int MAX_LEVEL_ALLOWED = (100);
#define GET_OBJ_AVG_DAM(obj) (((obj_get_value(obj, 2) + 1) / 2.0) * obj_get_value(obj, 1))
/* arbitrary limit for per round dam */
constexpr int MAX_MOB_DAM_ALLOWED = 500;

#define ZCMD2 g->zones[zone].cmd[cmd_no] /*from DB.C*/

/*item limits*/
constexpr int MAX_DAM_ALLOWED     = 50; /* for weapons  - avg. dam*/
constexpr int MAX_AFFECTS_ALLOWED = 3;

/* Armor class limits*/
constexpr int TOTAL_WEAR_CHECKS = (NUM_ITEM_WEARS - 2); /*minus Wield and Take*/
static const struct zcheck_armor {
  bitvector_t bitvector; /* from Structs.h                       */
  int ac_allowed;        /* Max. AC allowed for this body part  */
  const char *message;   /* phrase for error message            */
} zarmor[] = {
    {ITEM_WEAR_FINGER, 10, "Ring"         },
    {ITEM_WEAR_NECK,   10, "Necklace"     },
    {ITEM_WEAR_BODY,   10, "Body armor"   },
    {ITEM_WEAR_HEAD,   10, "Head gear"    },
    {ITEM_WEAR_LEGS,   10, "Legwear"      },
    {ITEM_WEAR_FEET,   10, "Footwear"     },
    {ITEM_WEAR_HANDS,  10, "Glove"        },
    {ITEM_WEAR_ARMS,   10, "Armwear"      },
    {ITEM_WEAR_SHIELD, 10, "Shield"       },
    {ITEM_WEAR_ABOUT,  10, "Cloak"        },
    {ITEM_WEAR_WAIST,  10, "Belt"         },
    {ITEM_WEAR_WRIST,  10, "Wristwear"    },
    {ITEM_WEAR_HOLD,   10, "Held item"    },
    {ITEM_WEAR_PACK,   10, "Backpack item"},
    {ITEM_WEAR_EAR,    10, "Earring item" },
    {ITEM_WEAR_SH,     10, "Shoulder item"},
    {ITEM_WEAR_EYE,    10, "Eye item"     }
};

/*
  Applies limits
  !! Very Important:  Keep these in the same order as in Structs.h
  To ignore an apply, set max_aff to -99.
  These will be ignored if MAX_APPLIES_LIMIT = 0
*/
static const struct zcheck_affs {
  int aff_type;        /*from Structs.h*/
  int min_aff;         /*min. allowed value*/
  int max_aff;         /*max. allowed value*/
  const char *message; /*phrase for error message*/
} zaffs[] = {
    {APPLY_NONE,        0,   -99, "unused0"         },
    {APPLY_STR,         -6,  6,   "strength"        },
    {APPLY_DEX,         -6,  6,   "dexterity"       },
    {APPLY_INT,         -6,  6,   "intelligence"    },
    {APPLY_WIS,         -6,  6,   "wisdom"          },
    {APPLY_CON,         -6,  6,   "constitution"    },
    {APPLY_CHA,         -6,  6,   "charisma"        },
    {APPLY_CLASS,       0,   0,   "class"           },
    {APPLY_LEVEL,       0,   0,   "level"           },
    {APPLY_AGE,         -10, 10,  "age"             },
    {APPLY_CHAR_WEIGHT, -50, 50,  "character weight"},
    {APPLY_CHAR_HEIGHT, -50, 50,  "character height"},
    {APPLY_MANA,        -50, 50,  "mana"            },
    {APPLY_HIT,         -50, 50,  "hit points"      },
    {APPLY_MOVE,        -50, 50,  "movement"        },
    {APPLY_GOLD,        0,   0,   "gold"            },
    {APPLY_EXP,         0,   0,   "experience"      },
    {APPLY_AC,          -10, 10,  "magical AC"      },
    {APPLY_ACCURACY,    0,   -99, "accuracy"        },
    {APPLY_DAMAGE,      0,   -99, "damage"          },
    {APPLY_REGEN,       0,   0,   "regen"           },
    {APPLY_TRAIN,       0,   0,   "train"           },
    {APPLY_LIFEMAX,     0,   0,   "lifemax"         },
    {APPLY_UNUSED3,     0,   0,   "unused"          },
    {APPLY_UNUSED4,     0,   0,   "unused"          },
    {APPLY_RACE,        0,   0,   "race"            },
    {APPLY_TURN_LEVEL,  -6,  6,   "turn level"      },
    {APPLY_SPELL_LVL_0, 0,   0,   "spell level 0"   },
    {APPLY_SPELL_LVL_1, 0,   0,   "spell level 1"   },
    {APPLY_SPELL_LVL_2, 0,   0,   "spell level 2"   },
    {APPLY_SPELL_LVL_3, 0,   0,   "spell level 3"   },
    {APPLY_SPELL_LVL_4, 0,   0,   "spell level 4"   },
    {APPLY_SPELL_LVL_5, 0,   0,   "spell level 5"   },
    {APPLY_SPELL_LVL_6, 0,   0,   "spell level 6"   },
    {APPLY_SPELL_LVL_7, 0,   0,   "spell level 7"   },
    {APPLY_SPELL_LVL_8, 0,   0,   "spell level 8"   },
    {APPLY_SPELL_LVL_9, 0,   0,   "spell level 9"   },
    {APPLY_KI,          0,   0,   "ki"              },
    {APPLY_FORTITUDE,   -4,  4,   "fortitude"       },
    {APPLY_REFLEX,      -4,  4,   "reflex"          },
    {APPLY_WILL,        -4,  4,   "will"            },
    {APPLY_SKILL,       -10, 10,  "skill"           },
    {APPLY_FEAT,        -10, 10,  "feat"            },
    {APPLY_ALLSAVES,    -4,  4,   "all 3 save types"},
    {APPLY_RESISTANCE,  -4,  4,   "resistance"      },
    {APPLY_ALL_STATS,   -6,  6,   "all stats"       }
};

/* These are ABS() values. */
constexpr int MAX_APPLY_ACCURCY_MOD_TOTAL = 5;
constexpr int MAX_APPLY_DAMAGE_MOD_TOTAL  = 5;

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
static const int offlimit_zones[]  = {0, 12, 13, 14, -1}; /*what zones can no room connect to (virtual num) */
constexpr int MIN_ROOM_DESC_LENGTH = 80;                  /* at least one line - set to 0 to not care. */
constexpr int MAX_COLOUMN_WIDTH    = 80;                  /* at most 80 chars per line */

ACMD(do_zcheck)
{
  zone_rnum zrnum;
  struct obj_data *obj;
  struct char_data *mob = nullptr;
  room_vnum exroom      = 0;
  int ac                = 0;
  int affs              = 0, tohit, todam, value;
  IDXTYPE i             = 0;
  int j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
  char buf[MAX_STRING_LENGTH];
  float avg_dam;
  size_t len = 0;
  struct extra_descr_data *ext, *ext2;
  one_argument(argument, buf);

  if (!*buf || !strcmp(buf, "."))
    zrnum = rroom_get_zone(g, char_get_in_room(ch));
  else
    zrnum = real_zone(g, atoi(buf));

  if (zrnum == NOWHERE) {
    send_to_char(g, ch, "Check what zone ?\r\n");
    return;
  } else
    send_to_char(g, ch, "Checking zone %d!\r\n", g->zones[zrnum].number);

  /************** Check mobs *****************/

  send_to_char(g, ch, "Checking Mobs for limits...\r\n");
  /*check mobs first*/
  for (i = 0; i < g->top_mob; i++) {
    if (real_zone_by_thing(g, g->mob_index[i].vnum) == zrnum) { /*is mob in this zone?*/
      mob = &g->mob_proto[i];
      if (!strcmp(char_get_alias(mob), "mob unfinished") && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Alias hasn't been set.\r\n");

      if (!strcmp(char_get_sdesc(mob), "the unfinished mob") && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Short description hasn't been set.\r\n");

      if (!strncmp(char_get_ldesc(mob), "An unfinished mob stands here.", 30) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Long description hasn't been set.\r\n");

      if (char_get_ddesc(mob) && *char_get_ddesc(mob)) {
        if (!strncmp(char_get_ddesc(mob), "It looks unfinished.", 20) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Description hasn't been set.\r\n");
        else if (strncmp(char_get_ddesc(mob), "   ", 3) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Description hasn't been formatted. (/fi)\r\n");
      }

      if (char_get_level(mob) > MAX_LEVEL_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Is level %d (limit: 1-%d)\r\n",
                        char_get_level(mob), MAX_LEVEL_ALLOWED);

      /* avg. dam per round of combat */
      avg_dam = ((char_get_mob_specials_damsizedice(mob) / 2.0) * char_get_mob_specials_damnodice(mob));
      if (avg_dam > MAX_MOB_DAM_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- average damage of %4.1f is too high (limit: %d)\r\n",
                        avg_dam, MAX_MOB_DAM_ALLOWED);

      if (char_get_mob_specials_damsizedice(mob) == 1 &&
          char_get_mob_specials_damnodice(mob) == 1 &&
          char_get_level(mob) == 0 &&
          (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Needs to be fixed - %sAutogenerate!%s\r\n", CCYEL(g, ch, C_NRM), CCNRM(g, ch, C_NRM));

      if (char_mob_flagged(mob, MOB_AGGRESSIVE) &&
          char_mob_flagged(mob, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Both aggresive and agressive to align.\r\n");

      if ((char_get_gold(mob) > MAX_GOLD_ALLOWED) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Set to %d Gold (limit : %d).\r\n",
                        char_get_gold(mob),
                        MAX_GOLD_ALLOWED);

      if (char_get_exp(mob) > MAX_EXP_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Has %" PRId64 " experience (limit: %d)\r\n",
                        char_get_exp(mob), MAX_EXP_ALLOWED);
      if (char_aff_flagged(mob, AFF_GROUP | AFF_CHARM | AFF_POISON) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Has illegal affection bits set (%s %s %s)\r\n",
                        char_aff_flagged(mob, AFF_GROUP) ? "GROUP" : "",
                        char_aff_flagged(mob, AFF_CHARM) ? "CHARM" : "",
                        char_aff_flagged(mob, AFF_POISON) ? "POISON" : "");

      /*if (!MOB_FLAGGED(mob, MOB_SENTINEL) && !MOB_FLAGGED(mob, MOB_STAY_ZONE) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                            "- Neither SENTINEL nor STAY_ZONE bits set.\r\n");*/

      if (char_mob_flagged(mob, MOB_SPEC) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- SPEC flag needs to be removed.\r\n");

      /*****ADDITIONAL MOB CHECKS HERE*****/
      if (found) {
        send_to_char(g, ch,
                     "%s[%5d]%s %-30s: %s\r\n%s",
                     CCCYN(g, ch, C_NRM), char_get_mob_vnum(g, mob),
                     CCYEL(g, ch, C_NRM), char_get_name(mob),
                     CCNRM(g, ch, C_NRM), buf);
      }
      /* reset buffers and found flag */
      strcpy(buf, "");
      found = 0;
      len   = 0;
    } /*mob is in zone*/
  } /*check mobs*/

  /************** Check objects *****************/
  send_to_char(g, ch, "\r\nChecking Objects for limits...\r\n");
  for (i = 0; i < g->top_obj; i++) {
    if (real_zone_by_thing(g, g->obj_index[i].vnum) == zrnum) { /*is object in this zone?*/
      obj = &g->obj_proto[i];
      switch (obj_get_type_flag(obj)) {
        case ITEM_MONEY:
          if ((value = obj_get_value(obj, 1)) > MAX_GOLD_ALLOWED && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Is worth %d (money limit %d coins).\r\n",
                            value, MAX_GOLD_ALLOWED);
          break;
        case ITEM_WEAPON:
          if (obj_get_value(obj, 3) >= NUM_ATTACK_TYPES && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- has out of range attack type %d.\r\n",
                            obj_get_value(obj, 3));

          if (GET_OBJ_AVG_DAM(obj) > MAX_DAM_ALLOWED && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Damroll is %2.1f (limit %d)\r\n",
                            GET_OBJ_AVG_DAM(obj), MAX_DAM_ALLOWED);
          break;
        case ITEM_ARMOR:
          ac = obj_get_value(obj, 0);
          for (j = 0; j < TOTAL_WEAR_CHECKS; j++) {
            if (obj_wear_flagged(obj, zarmor[j].bitvector) && (ac > zarmor[j].ac_allowed) && (found = 1))
              len += snprintf(buf + len, sizeof(buf) - len,
                              "- Has AC %d (%s limit is %d)\r\n",
                              ac, zarmor[j].message, zarmor[j].ac_allowed);
          }
          break;

      } /*switch on Item_Type*/

      if (!obj_wear_flagged(obj, ITEM_WEAR_TAKE)) {
        if ((obj_get_cost(obj) || (obj_get_weight(obj) && obj_get_type_flag(obj) != ITEM_FOUNTAIN) ||
             obj_get_cost_per_day(obj)) &&
            (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- is NO_TAKE, but has cost (%d) weight (%" PRId64 ") or rent (%d) set.\r\n",
                          obj_get_cost(obj), obj_get_weight(obj), obj_get_cost_per_day(obj));
      } else {
        if (obj_get_cost(obj) == 0 && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 cost (min. 1).\r\n");

        if (obj_get_weight(obj) == 0 && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 weight (min. 1).\r\n");

        if (obj_get_weight(obj) > MAX_OBJ_WEIGHT && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "  Weight is too high: %" PRId64 " (limit  %d).\r\n",
                          obj_get_weight(obj), MAX_OBJ_WEIGHT);

        if (obj_get_cost(obj) > MAX_OBJ_COST && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has %d cost (max %d).\r\n",
                          obj_get_cost(obj), MAX_OBJ_COST);
      }

      if (obj_get_level(obj) > ADMLVL_IMMORT - 1 && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has min level set to %d (max %d).\r\n",
                        obj_get_level(obj), ADMLVL_IMMORT - 1);

      if (obj_get_action_description(obj) && *obj_get_action_description(obj) &&
          obj_get_type_flag(obj) != ITEM_STAFF &&
          obj_get_type_flag(obj) != ITEM_WAND &&
          obj_get_type_flag(obj) != ITEM_SCROLL &&
          obj_get_type_flag(obj) != ITEM_NOTE && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has action_description set, but is inappropriate type.\r\n");

      /*first check for over-all affections*/
      for (affs = 0, j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj_get_affected(obj)[j].modifier) affs++;

      if (affs > MAX_AFFECTS_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has %d affects (limit %d).\r\n",
                        affs, MAX_AFFECTS_ALLOWED);

      /*check for out of range affections. */
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        if (zaffs[(int)obj_get_affected(obj)[j].location].max_aff != -99 && /* only care if a range is set */
            (obj_get_affected(obj)[j].modifier > zaffs[(int)obj_get_affected(obj)[j].location].max_aff ||
             obj_get_affected(obj)[j].modifier < zaffs[(int)obj_get_affected(obj)[j].location].min_aff ||
             zaffs[(int)obj_get_affected(obj)[j].location].min_aff == zaffs[(int)obj_get_affected(obj)[j].location].max_aff) &&
            (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- apply to %s is %d (limit %d - %d).\r\n",
                          zaffs[(int)obj_get_affected(obj)[j].location].message,
                          obj_get_affected(obj)[j].modifier,
                          zaffs[(int)obj_get_affected(obj)[j].location].min_aff,
                          zaffs[(int)obj_get_affected(obj)[j].location].max_aff);

      /* special handling of +hit and +dam because of +hit_n_dam */
      for (todam = 0, tohit = 0, j = 0; j < MAX_OBJ_AFFECT; j++) {
        if (obj_get_affected(obj)[j].location == APPLY_DAMAGE)
          todam += obj_get_affected(obj)[j].modifier;
      }
      if (abs(todam) > MAX_APPLY_DAMAGE_MOD_TOTAL && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- total damage mod %d out of range (limit +/-%d.\r\n",
                        todam, MAX_APPLY_DAMAGE_MOD_TOTAL);
      if (abs(tohit) > MAX_APPLY_ACCURCY_MOD_TOTAL && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- total accurcy mod %d out of range (limit +/-%d).\r\n",
                        tohit, MAX_APPLY_ACCURCY_MOD_TOTAL);

      for (ext2 = nullptr, ext = obj_get_ex_description(obj); ext; ext = ext->next)
        if (strncmp(ext->description, "   ", 3))
          ext2 = ext;

      if (ext2 && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has unformatted extra description\r\n");
      /*****ADDITIONAL OBJ CHECKS HERE*****/
      if (found) {
        send_to_char(g, ch, "[%5d] %-30s: \r\n%s", obj_get_vnum(g, obj), obj_get_short_description(obj), buf);
      }
      strcpy(buf, "");
      len   = 0;
      found = 0;
    } /*object is in zone*/
  } /*check objects*/

  /************** Check rooms *****************/
  send_to_char(g, ch, "\r\nChecking Rooms for limits...\r\n");
  for (i = 0; i < g->top_room; i++) {
    if (rroom_get_zone(g, i) == zrnum) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
        /*check for exit, but ignore off limits if you're in an offlimit zone*/
        if (!rroom_get_dir_option(g, i, j))
          continue;
        exroom = rroom_get_dir_option(g, i, j)->to_room;
        if (exroom == NOWHERE)
          continue;
        if (rroom_get_zone(g, exroom) == zrnum)
          continue;
        if (rroom_get_zone(g, exroom) == rroom_get_zone(g, i))
          continue;

        for (k = 0; offlimit_zones[k] != -1; k++) {
          if (rroom_get_zone(g, exroom) == real_zone(g, offlimit_zones[k]) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Exit %s cannot connect to %d (zone off limits).\r\n",
                            dirs[j], rroom_get_vnum(g, exroom));
        } /* for (k.. */
      } /* cycle directions */

      if (rroom_flagged(g, i, ROOM_ATRIUM | ROOM_HOUSE | ROOM_HOUSE_CRASH | ROOM_OLC | ROOM_BFS_MARK))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Has illegal affection bits set (%s %s %s %s %s)\r\n",
                        rroom_flagged(g, i, ROOM_ATRIUM) ? "ATRIUM" : "",
                        rroom_flagged(g, i, ROOM_HOUSE) ? "HOUSE" : "",
                        rroom_flagged(g, i, ROOM_HOUSE_CRASH) ? "HCRSH" : "",
                        rroom_flagged(g, i, ROOM_OLC) ? "OLC" : "",
                        rroom_flagged(g, i, ROOM_BFS_MARK) ? "*" : "");

      if ((MIN_ROOM_DESC_LENGTH) && strlen(rroom_get_description(g, i)) < MIN_ROOM_DESC_LENGTH && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description is too short. (%4.4zu of min. %d characters).\r\n",
                        strlen(rroom_get_description(g, i)), MIN_ROOM_DESC_LENGTH);

      if (strncmp(rroom_get_description(g, i), "   ", 3) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not formatted with indent (/fi in the editor).\r\n");

      /* strcspan = size of text in first arg before any character in second arg */
      if ((strcspn(rroom_get_description(g, i), "\r\n") > MAX_COLOUMN_WIDTH) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not wrapped at %d chars (/fi in the editor).\r\n",
                        MAX_COLOUMN_WIDTH);

      for (ext2 = nullptr, ext = rroom_get_ex_description(g, i); ext; ext = ext->next)
        if (strncmp(ext->description, "   ", 3))
          ext2 = ext;

      if (ext2 && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has unformatted extra description\r\n");

      if (found) {
        send_to_char(g, ch, "[%5d] %-30s: \r\n%s",
                     rroom_get_vnum(g, i), rroom_get_name(g, i) ? rroom_get_name(g, i) : "An unnamed room", buf);
        strcpy(buf, "");
        len   = 0;
        found = 0;
      }
    } /*is room in this zone?*/
  } /*checking rooms*/

  for (i = 0; i < g->top_room; i++) {
    if (rroom_get_zone(g, i) == zrnum) {
      m++;
      for (j = 0, k = 0; j < NUM_OF_DIRS; j++)
        if (!rroom_get_dir_option(g, i, j))
          k++;

      if (k == NUM_OF_DIRS)
        l++;
    }
  }
  if (l * 3 > m)
    send_to_char(g, ch, "More than 1/3 of the rooms are not linked.\r\n");
}
/**********************************************************************************/

static void mob_checkload(GameWorld *g, struct char_data *ch, mob_vnum mvnum)
{
  int cmd_no, count = 0;
  zone_rnum zone;
  mob_rnum mrnum = real_mobile(g, mvnum);

  if (mrnum == NOBODY) {
    send_to_char(g, ch, "That mob does not exist.\r\n");
    return;
  }

  send_to_char(g, ch, "Checking load info for the mob [%d] %s...\r\n",
               mvnum, char_get_sdesc(&g->mob_proto[mrnum]));

  for (zone = 0; zone <= g->top_zone; zone++) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      if (ZCMD2.command != 'M')
        continue;

      /* read a mobile */
      if (ZCMD2.arg1 == mrnum) {
        send_to_char(g, ch, "  [%5d] %s (%d MAX)\r\n",
                     rroom_get_vnum(g, ZCMD2.arg3),
                     rroom_get_name(g, ZCMD2.arg3),
                     ZCMD2.arg2);
        count += 1;
      }
    }
  }
  if (count > 0)
    send_to_char(g, ch, "@D[@nTotal counted: %s.@D]@n\r\n", add_commas(count));
}

static void obj_checkload(GameWorld *g, struct char_data *ch, obj_vnum ovnum)
{
  int cmd_no, count = 0;
  zone_rnum zone;
  obj_rnum ornum       = real_object(g, ovnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0;
  mob_rnum lastmob_r   = 0;

  if (ornum == NOTHING) {
    send_to_char(g, ch, "That object does not exist.\r\n");
    return;
  }

  send_to_char(g, ch, "Checking load info for the obj [%d] %s...\r\n",
               ovnum, obj_get_short_description(&g->obj_proto[ornum]));

  for (zone = 0; zone <= g->top_zone; zone++) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = rroom_get_vnum(g, ZCMD2.arg3);
          lastroom_r = ZCMD2.arg3;
          lastmob_r  = ZCMD2.arg1;
          break;
        case 'O': /* read an object */
          lastroom_v = rroom_get_vnum(g, ZCMD2.arg3);
          lastroom_r = ZCMD2.arg3;
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [%5d] %s (%d Max)\r\n",
                         lastroom_v,
                         rroom_get_name(g, lastroom_r),
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'P': /* object to object */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [%5d] %s (Put in another object [%d Max])\r\n",
                         lastroom_v,
                         rroom_get_name(g, lastroom_r),
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'G': /* obj_to_char */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [%5d] %s (Given to %s [%d][%d Max])\r\n",
                         lastroom_v,
                         rroom_get_name(g, lastroom_r),
                         char_get_sdesc(&g->mob_proto[lastmob_r]),
                         g->mob_index[lastmob_r].vnum,
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'E': /* object to equipment list */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [%5d] %s (Equipped to %s [%d][%d Max])\r\n",
                         lastroom_v,
                         rroom_get_name(g, lastroom_r),
                         char_get_sdesc(&g->mob_proto[lastmob_r]),
                         g->mob_index[lastmob_r].vnum,
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'R': /* rem obj from room */
          lastroom_v = rroom_get_vnum(g, ZCMD2.arg1);
          lastroom_r = ZCMD2.arg1;
          if (ZCMD2.arg2 == ornum) {
            send_to_char(g, ch, "  [%5d] %s (Removed from room)\r\n",
                         lastroom_v,
                         rroom_get_name(g, lastroom_r));
            count += 1;
          }
          break;
      } /* switch */
    } /*for cmd_no......*/
  } /*for zone...*/

  if (count > 0)
    send_to_char(g, ch, "@D[@nTotal counted: %s.@D]@n\r\n", add_commas(count));
}

static void trg_checkload(GameWorld *g, struct char_data *ch, trig_vnum tvnum)
{
  int cmd_no, found = 0;
  zone_rnum zone;
  trig_rnum trnum      = real_trigger(g, tvnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0, k;
  mob_rnum lastmob_r   = 0, i;
  obj_rnum lastobj_r   = 0, j;
  struct trig_proto_list *tpl;

  if (trnum == NOTHING) {
    send_to_char(g, ch, "That trigger does not exist.\r\n");
    return;
  }

  send_to_char(g, ch, "Checking load info for the %s trigger [%d] '%s':\r\n",
               g->trig_index[trnum]->proto->attach_type == MOB_TRIGGER ? "mobile" : (g->trig_index[trnum]->proto->attach_type == OBJ_TRIGGER ? "object" : "room"),
               tvnum, g->trig_index[trnum]->proto->name);

  for (zone = 0; zone <= g->top_zone; zone++) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = rroom_get_vnum(g, ZCMD2.arg3);
          lastroom_r = ZCMD2.arg3;
          lastmob_r  = ZCMD2.arg1;
          break;
        case 'O': /* read an object */
          lastroom_v = rroom_get_vnum(g, ZCMD2.arg3);
          lastroom_r = ZCMD2.arg3;
          lastobj_r  = ZCMD2.arg1;
          break;
        case 'P': /* object to object */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'G': /* obj_to_char */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'E': /* object to equipment list */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'R': /* rem obj from room */
          lastroom_v = 0;
          lastroom_r = 0;
          lastobj_r  = 0;
          lastmob_r  = 0;
          break;
        case 'T': /* trigger to something */
          if (ZCMD2.arg2 != trnum)
            break;
          if (ZCMD2.arg1 == MOB_TRIGGER) {
            send_to_char(g, ch, "mob [%5d] %-60s (zedit room %5d)\r\n",
                         g->mob_index[lastmob_r].vnum,
                         char_get_sdesc(&g->mob_proto[lastmob_r]),
                         lastroom_v);
            found = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER) {
            send_to_char(g, ch, "obj [%5d] %-60s  (zedit room %d)\r\n",
                         g->obj_index[lastobj_r].vnum,
                         obj_get_short_description(&g->obj_proto[lastobj_r]),
                         lastroom_v);
            found = 1;
          } else if (ZCMD2.arg1 == WLD_TRIGGER) {
            send_to_char(g, ch, "room [%5d] %-60s (zedit)\r\n",
                         lastroom_v,
                         rroom_get_name(g, lastroom_r));
            found = 1;
          }
          break;
      } /* switch */
    } /*for cmd_no......*/
  } /*for zone...*/

  for (i = 0; i < g->top_mob; i++) {
    if (!char_get_proto_script(&g->mob_proto[i]))
      continue;

    for (tpl = char_get_proto_script(&g->mob_proto[i]); tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(g, ch, "mob [%5d] %s\r\n",
                     g->mob_index[i].vnum,
                     char_get_sdesc(&g->mob_proto[i]));
        found = 1;
      }
  }

  for (j = 0; j < g->top_obj; j++) {
    if (!obj_get_proto_script(&g->obj_proto[j]))
      continue;

    for (tpl = obj_get_proto_script(&g->obj_proto[j]); tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(g, ch, "obj [%5d] %s\r\n",
                     g->obj_index[j].vnum,
                     obj_get_short_description(&g->obj_proto[j]));
        found = 1;
      }
  }

  for (k = 0; k < g->top_room; k++) {
    if (!rroom_get_proto_script(g, k))
      continue;

    for (tpl = rroom_get_proto_script(g, k); tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(g, ch, "room[%5d] %s\r\n",
                     rroom_get_vnum(g, k),
                     rroom_get_name(g, k));
        found = 1;
      }
  }

  if (!found)
    send_to_char(g, ch, "This trigger is not attached to anything.\r\n");
}

ACMD(do_checkloadstatus)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf1, buf2);

  if ((!*buf1) || (!*buf2) || (!isdigit(*buf2))) {
    send_to_char(g, ch, "Checkload <M | O | T> <vnum>\r\n");
    return;
  }

  if (tolower(*buf1) == 'm') {
    mob_checkload(g, ch, atoi(buf2));
    return;
  }

  if (tolower(*buf1) == 'o') {
    obj_checkload(g, ch, atoi(buf2));
    return;
  }

  if (tolower(*buf1) == 't') {
    trg_checkload(g, ch, atoi(buf2));
    return;
  }
}
/**************************************************************************************/
/*                         Zone Checker code above                                    */
/**************************************************************************************/

/* do_findkey, finds where the key to a door loads to using do_checkloadstatus() */
ACMD(do_findkey)
{
  int dir;
  obj_vnum key;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

  any_one_arg(argument, arg); /* Because "in" is a valid direction */

  if (!*arg) {
    send_to_char(g, ch, "Format: findkey <dir>\r\n");
  } else if ((dir = search_block(arg, dirs, false)) >= 0 ||
             (dir = search_block(arg, abbr_dirs, false)) >= 0) {
    if (!char_get_exit(g, ch, dir)) {
      send_to_char(g, ch, "There's no exit in that direction!\r\n");
    } else if ((key = char_get_exit(g, ch, dir)->key) == NOTHING || key == 0) {
      send_to_char(g, ch, "There's no key for that exit.\r\n");
    } else {
      snprintf(buf, sizeof(buf), "obj %d", key);
      do_checkloadstatus(g, ch, buf, 0, 0);
    }
  } else {
    send_to_char(g, ch, "What direction is that?!?\r\n");
  }
}

ACMD(do_spells)
{
  int i, qend;

  send_to_char(g, ch, "The following spells are in the game:\r\n");

  for (qend = 0, i = 0; i < MAX_SPELLS; i++) {
    if (g->spell_info[i].name == unused_spellname) /* This is valid. */
      continue;
    send_to_char(g, ch, "%18s", g->spell_info[i].name);
    if (qend++ % 4 == 3)
      send_to_char(g, ch, "\r\n");
  }
  if (qend % 4 != 0)
    send_to_char(g, ch, "\r\n");
  return;
}

struct audit_reader {
  GameWorld *g;
  struct char_data *ch;
};

static void audit_report_to_char(void *context, const char *problem)
{
  struct audit_reader *reader = static_cast<struct audit_reader *>(context);

  send_to_char(reader->g, reader->ch, "%s\r\n", problem);
}

ACMD(do_audit)
{
  struct audit_reader reader = {g, ch};
  int problems               = world_audit(g, audit_report_to_char, &reader);

  send_to_char(g, ch, "World audit: %d problem%s found.\r\n", problems, problems == 1 ? "" : "s");
}
