/************************************************************************
 * OasisOLC - Objects / oedit.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/affect.h"
#include "dbat/shop.h"
#include "dbat/boards.h"
#include "dbat/config.h"
#include "dbat/dg_scripts.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/genolc.h"
#include "dbat/genobj.h"
#include "dbat/genzon.h"
#include "dbat/oasis.h"
#include "dbat/relocate.h"
#include "dbat/improved-edit.h"
#include "dbat/dg_olc.h"
#include "dbat/feats.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/races.h"
#include "dbat/fight.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static void oedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void oedit_disp_val3_menu(GameWorld *g, struct descriptor_data *d);
static void oedit_disp_val4_menu(GameWorld *g, struct descriptor_data *d);
static void oedit_disp_val5_menu(GameWorld *g, struct descriptor_data *d);
static void oedit_disp_val9_menu(GameWorld *g, struct descriptor_data *d);
static void oedit_setup_existing(GameWorld *g, struct descriptor_data *d, obj_rnum real_num);
static bool object_in_game(GameWorld *g, const struct obj_data *obj);
static void commit_iedit(GameWorld *g, struct obj_data *obj, const struct obj_data *edited);
static void oedit_setup_new(struct descriptor_data *d);

/*------------------------------------------------------------------------*/

/*
 * Handy macros.
 */

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

ACMD(do_oasis_oedit)
{
  IDXTYPE number = NOWHERE;
  int save       = 0;
  obj_rnum real_num;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  two_arguments(argument, buf1, buf2);

  /****************************************************************************/
  /** If there aren't any arguments...well...they can't modify nothing now   **/
  /** can they?                                                              **/
  /****************************************************************************/
  if (!*buf1) {
    send_to_char(g, ch, "Specify an object VNUM to edit.\r\n");
    return;
  }
  if (!isdigit(*buf1)) {
    if (strcasecmp("save", buf1) != 0) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = true;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (char_get_olc_zone(g, ch) > 0) {
      zone_rnum zlok;

      if ((zlok = real_zone(g, char_get_olc_zone(g, ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(g, zlok);
    }

    if (number == NOWHERE) {
      send_to_char(g, ch, "Save which zone?\r\n");
      return;
    }
  }

  /****************************************************************************/
  /** If a numeric argument was given, get it.                               **/
  /****************************************************************************/
  if (number == NOWHERE)
    number = atoi(buf1);

  /****************************************************************************/
  /** Check that whatever it is isn't already being edited.                  **/
  /****************************************************************************/
  for (d = g->descriptors; d; d = d->next) {
    if (desc_get_connected(d) == CON_OEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(g, ch, "That object is currently being edited by %s.\r\n",
                     char_pers(g, d->character, ch));
        return;
      }
    }
  }

  /****************************************************************************/
  /** Point d to the builder's descriptor (for easier typing later).         **/
  /****************************************************************************/
  d = char_get_desc(ch);

  /****************************************************************************/
  /** Give the descriptor an OLC structure.                                  **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
           "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  OLC_ZNUM(d) = save ? real_zone(g, number) : real_zone_by_thing(g, number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_cannot_edit(g, ch, g->zones[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** If we need to save, save the objects.                                  **/
  /****************************************************************************/
  if (save) {
    send_to_char(g, ch, "Saving all objects in zone %d.\r\n",
                 g->zones[OLC_ZNUM(d)].number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
           "OLC: %s saves object info for zone %d.", char_get_name(ch),
           g->zones[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Save the objects in this zone.                                       **/
    /**************************************************************************/
    save_objects(g, OLC_ZNUM(d));

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  /****************************************************************************/
  /** If this is a new object, setup a new object, otherwise setup the       **/
  /** existing object.                                                       **/
  /****************************************************************************/
  if ((real_num = real_object(g, number)) != NOTHING)
    oedit_setup_existing(g, d, real_num);
  else
    oedit_setup_new(d);

  oedit_disp_menu(g, d);
  desc_set_connected(d, CON_OEDIT);

  /****************************************************************************/
  /** Send the OLC message to the players in the same room as the builder.   **/
  /****************************************************************************/
  act(g, "$n starts using OLC.", true, d->character, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  /****************************************************************************/
  /** Log the OLC message.                                                   **/
  /****************************************************************************/
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
         char_get_name(ch), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));
}

static void oedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_OBJ(d), struct obj_data, 1);

  clear_object(OLC_OBJ(d));
  obj_set_name(OLC_OBJ(d), strdup("unfinished object"));
  obj_set_description(OLC_OBJ(d), strdup("An unfinished object is lying here."));
  obj_set_short_description(OLC_OBJ(d), strdup("an unfinished object"));
  obj_wear_set(OLC_OBJ(d), ITEM_WEAR_TAKE, true);
  OLC_VAL(d)       = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  obj_set_type_flag(OLC_OBJ(d), ITEM_WORN);
  obj_set_value(OLC_OBJ(d), VAL_ALL_HEALTH, 100);
  obj_set_value(OLC_OBJ(d), VAL_ALL_MAXHEALTH, 100);
  obj_set_value(OLC_OBJ(d), VAL_ALL_MATERIAL, MATERIAL_STEEL);
  obj_set_size(OLC_OBJ(d), SIZE_MEDIUM);

  OLC_SCRIPT(d) = nullptr;
}

/*------------------------------------------------------------------------*/

static void oedit_setup_existing(GameWorld *g, struct descriptor_data *d, obj_rnum real_num)
{
  struct obj_data *obj;

  /*
   * Allocate object in memory.
   */
  CREATE(obj, struct obj_data, 1);
  copy_object(g, obj, &g->obj_proto[real_num]);

  /*
   * Attach new object to player's descriptor.
   */
  OLC_OBJ(d)       = obj;
  OLC_VAL(d)       = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  /*
   * The edited obj must not have a script.
   * It will be assigned to the updated obj later, after editing.
   */
  obj_set_script(obj, nullptr);
  obj_set_proto_script(OLC_OBJ(d), nullptr);
}

/*------------------------------------------------------------------------*/

static void oedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  int i;
  obj_rnum robj_num;
  struct descriptor_data *dsc;
  struct obj_data *obj;

  i = (real_object(g, OLC_NUM(d)) == NOTHING);

  if ((robj_num = add_object(g, OLC_OBJ(d), OLC_NUM(d))) == NOTHING) {
    mud_log("oedit_save_internally: add_object failed.");
    return;
  }

  /* Update triggers : */
  /* Free old proto list  */
  if (obj_get_proto_script(&g->obj_proto[robj_num]) &&
      obj_get_proto_script(&g->obj_proto[robj_num]) != OLC_SCRIPT(d))
    free_proto_script(&g->obj_proto[robj_num], OBJ_TRIGGER);
  /* this will handle new instances of the object: */
  obj_set_proto_script(&g->obj_proto[robj_num], OLC_SCRIPT(d));

  /* this takes care of the objects currently in-game */
  for (obj = g->objects; obj; obj = obj_get_next(obj)) {
    if (obj_get_item_number(obj) != robj_num)
      continue;
    /* remove any old scripts */
    if (obj_get_script(obj))
      extract_script(g, obj, OBJ_TRIGGER);

    free_proto_script(obj, OBJ_TRIGGER);
    copy_proto_script(&g->obj_proto[robj_num], obj, OBJ_TRIGGER);
    assign_triggers(g, obj, OBJ_TRIGGER);
  }
  /* end trigger update */

  if (!i) /* If it's not a new object, don't renumber. */
    return;

  /*
   * Renumber produce in shops being edited.
   */
  for (dsc = g->descriptors; dsc; dsc = dsc->next)
    if (desc_get_connected(dsc) == CON_SEDIT)
      for (i = 0; shop_get_product(OLC_SHOP(dsc), i) != NOTHING; i++)
        if (shop_get_product(OLC_SHOP(dsc), i) >= robj_num)
          shop_set_product(OLC_SHOP(dsc), i, shop_get_product(OLC_SHOP(dsc), i) + 1);

  /* Update other people in zedit too. From: C.Raehl 4/27/99 */
  for (dsc = g->descriptors; dsc; dsc = dsc->next)
    if (desc_get_connected(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        switch (OLC_ZONE(dsc)->cmd[i].command) {
          case 'P':
            OLC_ZONE(dsc)->cmd[i].arg3 += (OLC_ZONE(dsc)->cmd[i].arg3 >= robj_num);
            /* Fall through. */
          case 'E':
          case 'G':
          case 'O':
            OLC_ZONE(dsc)->cmd[i].arg1 += (OLC_ZONE(dsc)->cmd[i].arg1 >= robj_num);
            break;
          case 'R':
            OLC_ZONE(dsc)->cmd[i].arg2 += (OLC_ZONE(dsc)->cmd[i].arg2 >= robj_num);
            break;
          default:
            break;
        }
}

/*------------------------------------------------------------------------*/

static void oedit_save_to_disk(GameWorld *g, zone_rnum zone_num)
{
  save_objects(g, zone_num);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For container flags.
 */
static void oedit_disp_container_flags_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  clear_screen(g, d);

  sprintbit(obj_get_value(OLC_OBJ(d), 1), container_bits, bits, sizeof(bits));
  write_to_output(g, d,
                  "@g1@n) CLOSEABLE\r\n"
                  "@g2@n) PICKPROOF\r\n"
                  "@g3@n) CLOSED\r\n"
                  "@g4@n) LOCKED\r\n"
                  "Container flags: @c%s@n\r\n"
                  "Enter flag, 0 to quit : ",
                  bits);
}

/*
 * For extra descriptions.
 */
static void oedit_disp_extradesc_menu(GameWorld *g, struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  clear_screen(g, d);
  write_to_output(g, d,
                  "Extra desc menu\r\n"
                  "@g1@n) Keyword: @y%s@n\r\n"
                  "@g2@n) Description:\r\n@y%s@n\r\n"
                  "@g3@n) Goto next description: %s\r\n"
                  "@g0@n) Quit\r\n"
                  "Enter choice : ",

                  (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
                  (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
                  !extra_desc->next ? "Not set." : "Set.");
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
static void oedit_disp_prompt_apply_menu(GameWorld *g, struct descriptor_data *d)
{
  char apply_buf[MAX_STRING_LENGTH];
  int counter;

  clear_screen(g, d);

  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
    if (obj_get_affected(OLC_OBJ(d))[counter].modifier) {
      sprinttype(obj_get_affected(OLC_OBJ(d))[counter].location, apply_types, apply_buf, sizeof(apply_buf));
      write_to_output(g, d, " @g%d@n) %+d to @b%s@n", counter + 1,
                      obj_get_affected(OLC_OBJ(d))[counter].modifier, apply_buf);
      switch (obj_get_affected(OLC_OBJ(d))[counter].location) {
        case APPLY_FEAT:
          write_to_output(g, d, " (%s)", g->feat_list[obj_get_affected(OLC_OBJ(d))[counter].specific].name);
          break;
        case APPLY_SKILL:
          write_to_output(g, d, " (%s)", g->spell_info[obj_get_affected(OLC_OBJ(d))[counter].specific].name);
          break;
      }
      write_to_output(g, d, "\r\n");
    } else {
      write_to_output(g, d, " @g%d@n) None.\r\n", counter + 1);
    }
  }
  write_to_output(g, d, "\r\nEnter affection to modify (0 to quit) : ");
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

static void oedit_disp_prompt_spellbook_menu(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(g, d);

  for (counter = 0; counter < SPELLBOOK_SIZE; counter++) {
    if (obj_get_sbinfo(OLC_OBJ(d)) && obj_get_sbinfo(OLC_OBJ(d))[counter].spellname != 0 &&
        obj_get_sbinfo(OLC_OBJ(d))[counter].spellname < MAX_SPELLS) {
      write_to_output(g, d, " @g%3d@n) %-20.20s %s", counter + 1,
                      g->spell_info[obj_get_sbinfo(OLC_OBJ(d))[counter].spellname].name, !(++columns % 3) ? "\r\n" : "");
    } else {
      write_to_output(g, d, " @g%3d@n) None.%s", counter + 1, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(g, d, "\r\nEnter spell to modify (0 to quit) : ");
  OLC_MODE(d) = OEDIT_PROMPT_SPELLBOOK;
}

static void oedit_disp_spellbook_menu(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(g, d);

  for (counter = 0; counter < SKILL_TABLE_SIZE; counter++) {
    if (g->spell_info[counter].skilltype == SKTYPE_SPELL)
      write_to_output(g, d, "@g%3d@n) @y%-20.20s@n%s", counter,
                      g->spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(g, d, "@n\r\nEnter spell number (0 is no spell) : ");
  OLC_MODE(d) = OEDIT_SPELLBOOK;
}

/*
 * Some applies require parameters (skills, feats)
 */
static void oedit_disp_apply_spec_menu(GameWorld *g, struct descriptor_data *d)
{
  const char *buf;

  switch (obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].location) {
    case APPLY_FEAT:
      buf = "What feat should be modified : ";
      break;
    case APPLY_SKILL:
      buf = "What skill should be modified : ";
      break;
    default:
      oedit_disp_prompt_apply_menu(g, d);
      return;
  }

  write_to_output(g, d, "\r\n%s", buf);
  OLC_MODE(d) = OEDIT_APPLYSPEC;
}

/*
 * Ask for liquid type.
 */
static void oedit_liquid_type(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(g, d);

  for (counter = 0; counter < NUM_LIQ_TYPES; counter++) {
    write_to_output(g, d, " @g%2d@n) @y%-20.20s@n%s", counter,
                    drinks[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(g, d, "\r\n@nEnter drink type : ");
  OLC_MODE(d) = OEDIT_VALUE_3;
}

/*
 * The actual apply to set.
 */
static void oedit_disp_apply_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  olc_list_columns(g, d, apply_types, NUM_APPLIES, 0, " ", 3);
  write_to_output(g, d, "\r\nEnter apply type (0 is no apply) : ");
  OLC_MODE(d) = OEDIT_APPLY;
}

/*
 * Weapon critical type.
 */
static void oedit_disp_crittype_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  olc_list_columns(g, d, crit_type, MAX_CRIT_TYPE + 1, 0, " ", 3);
  write_to_output(g, d, "\r\nEnter critical type : ");
}

/*
 * Weapon type.
 */
static void oedit_disp_weapon_menu(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(g, d);

  for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) {
    write_to_output(g, d, "@g%2d@n) %-20.20s %s", counter,
                    attack_hit_text[counter].singular,
                    !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(g, d, "\r\nEnter weapon type : ");
}

/*
 * Armor type.
 */
static void oedit_disp_armor_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  olc_list_columns(g, d, armor_type, MAX_ARMOR_TYPES + 1, 0, " ", 3);
  write_to_output(g, d, "\r\nEnter armor proficiency type : ");
}

/*
 * Spell type.
 */
static void oedit_disp_spells_menu(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(g, d);

  for (counter = 0; counter < SKILL_TABLE_SIZE; counter++) {
    if (IS_SET(skill_type(g, counter), SKTYPE_SPELL))
      write_to_output(g, d, "@g%2d@n) @y%-20.20s@n%s", counter,
                      g->spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(g, d, "\r\n@nEnter spell choice (-1 for none) : ");
}

/*
 * Material type.
 */
static void oedit_disp_material_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  olc_list_columns(g, d, material_names, NUM_MATERIALS, 0, "", 3);
  write_to_output(g, d, "\r\n@nEnter material type : ");
}

/*
 * Object value #1
 */
static void oedit_disp_val1_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_1;
  switch (obj_get_type_flag(OLC_OBJ(d))) {
    case ITEM_LIGHT:
      /*
     * values 0 and 1 are unused.. jump to 2 
     */
      oedit_disp_val3_menu(g, d);
      break;
    case ITEM_SCROLL:
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_POTION:
      write_to_output(g, d, "Spell level : ");
      break;
    case ITEM_WEAPON:
      /*
     * This is now used to control the weapon type used by the weapon object
     */
      olc_list_columns(g, d, weapon_type, MAX_WEAPON_TYPES + 1, 0, " ", 3);
      write_to_output(g, d, "\r\nEnter the weapon type for determining proficiencies: \r\n");
      break;
    case ITEM_ARMOR:
      write_to_output(g, d, "Apply to AC : ");
      break;
    case ITEM_CONTAINER:
      write_to_output(g, d, "Max weight to contain (-1 for unlimited) : ");
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      write_to_output(g, d, "Max drink units (-1 for unlimited) : ");
      break;
    case ITEM_FOOD:
      write_to_output(g, d, "Hours to fill stomach : ");
      break;
    case ITEM_MONEY:
      write_to_output(g, d, "Number of zenni : ");
      break;
    case ITEM_NOTE:
      /*
     * This is supposed to be language, but it's unused.
     */
      break;
    case ITEM_VEHICLE:
      write_to_output(g, d, "Enter room vnum of vehicle interior : ");
      break;
    case ITEM_HATCH:
      write_to_output(g, d, "Enter vnum of the vehicle this hatch belongs to : ");
      break;
    case ITEM_WINDOW:
      write_to_output(g, d, "Enter vnum of the vehicle this window belongs to, or -1 to specify the viewport room : ");
      break;
    case ITEM_CONTROL:
      write_to_output(g, d, "Enter vnum of the vehicle these controls belong to : ");
      break;
    case ITEM_PORTAL:
      write_to_output(g, d, "Which room number is the destination? : ");
      break;
    case ITEM_BOARD:
      write_to_output(g, d, "Enter the minimum admin level to read this board (0 for mortals) : ");
      break;
    default:
      oedit_disp_val5_menu(g, d);
  }
}

/*
 * Object value #2
 */
static void oedit_disp_val2_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_2;
  switch (obj_get_type_flag(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      oedit_disp_spells_menu(g, d);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      write_to_output(g, d, "Max number of charges : ");
      break;
    case ITEM_WEAPON:
      write_to_output(g, d, "Number of damage dice : ");
      break;
    case ITEM_ARMOR:
      oedit_disp_armor_menu(g, d);
      break;
    case ITEM_FOOD:
      /*
     * Values 2 and 3 are unused, jump to 4...Odd.
     */
      oedit_disp_val4_menu(g, d);
      break;
    case ITEM_CONTROL:
      write_to_output(g, d, "Enter Engine Type ( 1, 2, 3) : ");
      break;
    case ITEM_CONTAINER:
    case ITEM_VEHICLE:
    case ITEM_HATCH:
    case ITEM_WINDOW:
    case ITEM_PORTAL:
      /*
     * These are flags, needs a bit of special handling.
     */
      oedit_disp_container_flags_menu(g, d);
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      write_to_output(g, d, "Initial drink units : ");
      break;
    case ITEM_BOARD:
      write_to_output(g, d, "Minimum admin level to write (0 for mortals) : ");
      break;
    default:
      oedit_disp_val5_menu(g, d);
  }
}

/*
 * Object value #3
 */
static void oedit_disp_val3_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_3;
  switch (obj_get_type_flag(OLC_OBJ(d))) {
    case ITEM_LIGHT:
      write_to_output(g, d, "Number of hours (0 = burnt, -1 is infinite) : ");
      break;
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      write_to_output(g, d, "Number of charges remaining : ");
      break;
    case ITEM_WEAPON:
      write_to_output(g, d, "Size of damage dice : ");
      break;
    case ITEM_ARMOR:
      write_to_output(g, d, "Max dex bonus : ");
      break;
    case ITEM_CONTAINER:
      write_to_output(g, d, "Vnum of key to open container (-1 for no key) : ");
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      oedit_liquid_type(g, d);
      break;
    case ITEM_VEHICLE:
      write_to_output(g, d, "Vnum of key to unlock vehicle (-1 for no key) : ");
      break;
    case ITEM_HATCH:
      write_to_output(g, d, "Vnum of key to unlock hatch (-1 for no key) : ");
      break;
    case ITEM_WINDOW:
      write_to_output(g, d, "Vnum of key to unlock window (-1 for no key) : ");
      break;
    case ITEM_PORTAL:
      write_to_output(g, d, "Vnum of the key to unlock portal (-1 for no key) : ");
      break;
    case ITEM_BOARD:
      write_to_output(g, d, "Minimum admin level to remove messages (0 for mortals) : ");
      break;
    default:
      oedit_disp_val5_menu(g, d);
  }
}

/*
 * Object value #4
 */
static void oedit_disp_val4_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_4;
  switch (obj_get_type_flag(OLC_OBJ(d))) {
    case ITEM_WAND:
    case ITEM_STAFF:
      oedit_disp_spells_menu(g, d);
      break;
    case ITEM_WEAPON:
      oedit_disp_weapon_menu(g, d);
      break;
    case ITEM_ARMOR:
      write_to_output(g, d, "Armor check penalty : ");
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      write_to_output(g, d, "Poisoned (0 = not poison) : ");
      break;
    case ITEM_VEHICLE:
      write_to_output(g, d, "What is the vehicle's appearance? (-1 for transparent) : ");
      break;
    case ITEM_HATCH:
      write_to_output(g, d, "Enter default vehicle load room : ");
      break;
    case ITEM_PORTAL:
      write_to_output(g, d, "What is the portal's appearance? (-1 for transparent) : ");
      break;
    case ITEM_WINDOW:
      if (obj_get_value(OLC_OBJ(d), 0) < 0)
        write_to_output(g, d, "What is the viewport room vnum (-1 for default location) : ");
      else
        oedit_disp_menu(g, d);
      break;
    default:
      oedit_disp_val5_menu(g, d);
  }
}

/*
 * Object value #5
 */
static void oedit_disp_val5_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_5;
  write_to_output(g, d, "Enter object default quality percentage (100%% MAX): ");
}

/*
 * Object value #7
 */
static void oedit_disp_val7_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_7;

  switch (obj_get_type_flag(OLC_OBJ(d))) {
    case ITEM_WEAPON:
      oedit_disp_crittype_menu(g, d);
      break;
    case ITEM_ARMOR:
      write_to_output(g, d, "Arcane spell failure %% : ");
      break;
    default:
      oedit_disp_val9_menu(g, d);
      break;
  }
}

/*
 * Object value #9
 */
static void oedit_disp_val9_menu(GameWorld *g, struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_9;

  switch (obj_get_type_flag(OLC_OBJ(d))) {
    case ITEM_WEAPON:
      write_to_output(g, d, "Default crit is only on natural 20. Extend this range by: ");
      break;
    default:
      oedit_disp_menu(g, d);
      break;
  }
}

/*
 * Object type.
 */
static void oedit_disp_type_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  olc_list_columns(g, d, item_types, NUM_ITEM_TYPES, 0, " ", 3);
  write_to_output(g, d, "\r\nEnter object type : ");
}

/*
 * Object extra flags.
 */
static void oedit_disp_extra_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];

  clear_screen(g, d);

  olc_list_columns(g, d, extra_bits, NUM_ITEM_FLAGS, 1, " ", 3);
  sprintbitarray(obj_get_extra_flags(OLC_OBJ(d)), extra_bits, EF_ARRAY_MAX, bits, sizeof(bits));
  write_to_output(g, d, "\r\nObject flags: @c%s@n\r\n"
                     "Enter object extra flag (0 to quit) : ",
                  bits);
}

/*
 * Object perm flags.
 */
static void oedit_disp_perm_menu(GameWorld *g, struct descriptor_data *d)
{
  char bitbuf[MAX_STRING_LENGTH];
  int counter, columns = 0;

  clear_screen(g, d);

  for (counter = 1; counter < NUM_AFF_FLAGS; counter++) {
    /* Setting AFF_CHARM on objects like this is dangerous. */
    if (counter == AFF_CHARM)
      continue;
    write_to_output(g, d, "@g%2d@n) %-20.20s%s", counter,
                    affected_bits[counter], !(++columns % 3) ? "\r\n" : "");
  }
  sprintbitarray(obj_get_bitvector(OLC_OBJ(d)), affected_bits, EF_ARRAY_MAX, bitbuf, sizeof(bitbuf));
  write_to_output(g, d, "\r\nObject permanent flags: @c%s@n\r\n"
                     "Enter object perm flag (0 to quit) : ",
                  bitbuf);
}

/*
 * Object size
 */
static void oedit_disp_size_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  olc_list_columns(g, d, size_names, NUM_SIZES, 1, "", 3);
  write_to_output(g, d, "\r\nEnter object size : ");
}

/*
 * Object wear flags.
 */
static void oedit_disp_wear_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];

  clear_screen(g, d);

  olc_list_columns(g, d, wear_bits, NUM_ITEM_WEARS, 1, " ", 3);
  sprintbitarray(obj_get_wear_flags(OLC_OBJ(d)), wear_bits, TW_ARRAY_MAX, bits, sizeof(bits));
  write_to_output(g, d, "\r\nWear flags: @c%s@n\r\n"
                     "Enter wear flag, 0 to quit : ",
                  bits);
}

/*
 * Display main menu.
 */
static void oedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  char tbitbuf[MAX_INPUT_LENGTH], ebitbuf[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  obj = OLC_OBJ(d);
  clear_screen(g, d);

  /*
   * Build buffers for first part of menu.
   */
  sprinttype(obj_get_type_flag(obj), item_types, tbitbuf, sizeof(tbitbuf));
  sprintbitarray(obj_get_extra_flags(obj), extra_bits, EF_ARRAY_MAX, ebitbuf, sizeof(ebitbuf));

  /*
   * Build first half of menu.
   */
  write_to_output(g, d,
                  "-- Item number : [@c%d@n]\r\n"
                  "@g1@n) Namelist : @y%s@n\r\n"
                  "@g2@n) S-Desc   : @y%s@n\r\n"
                  "@g3@n) L-Desc   :-\r\n@y%s@n\r\n"
                  "@g4@n) A-Desc   :-\r\n@y%s@n"
                  "@g5@n) Type        : @c%s@n\r\n"
                  "@g6@n) Extra flags : @c%s@n\r\n",

                  OLC_NUM(d),
                  (obj_get_name(obj) && *obj_get_name(obj)) ? obj_get_name(obj) : "undefined",
                  (obj_get_short_description(obj) && *obj_get_short_description(obj)) ? obj_get_short_description(obj) : "undefined",
                  (obj_get_description(obj) && *obj_get_description(obj)) ? obj_get_description(obj) : "undefined",
                  (obj_get_action_description(obj) && *obj_get_action_description(obj)) ? obj_get_action_description(obj) : "Not Set.\r\n",
                  tbitbuf,
                  ebitbuf);
  /*
   * Send first half.
   */

  /*
   * Build second half of menu.
   */
  sprintbitarray(obj_get_wear_flags(OLC_OBJ(d)), wear_bits, EF_ARRAY_MAX, tbitbuf, sizeof(tbitbuf));
  sprintbitarray(obj_get_bitvector(OLC_OBJ(d)), affected_bits, EF_ARRAY_MAX, ebitbuf, sizeof(ebitbuf));

  write_to_output(g, d,
                  "@g7@n) Wear flags  : @c%s@n\r\n"
                  "@g8@n) Weight      : @c%-4" PRId64 "@n, 	@g9@n) Cost        : @c%-4d@n\r\n"
                  "@gA@n) Cost/Day    : @c%-4d@n, 	@gB@n) Timer       : @c%-4d@n\r\n"
                  "@gC@n) Values      : @c%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d@n\r\n"
                  "@gD@n) Applies menu@n\r\n"
                  "@gE@n) Extra descriptions menu %s\r\n"
                  "@gM@n) Min Level   : @c%d@n\r\n"
                  "@gN@n) Material    : @c%s@n\r\n"
                  "@gP@n) Perm Affects: @c%s@n\r\n"
                  "@gS@n) Script      : @c%s@n\r\n"
                  "@gT@n) Spellbook menu\r\n"
                  "@gW@n) Copy object        ,	@gX@n) Delete object\r\n"
                  "@gY@n) Size        : @c%s@n\r\n"
                  "@gZ@n) Wiznet      :\r\n"
                  "@gQ@n) Quit\r\n"
                  "Enter choice : ",

                  tbitbuf, obj_get_weight(obj), obj_get_cost(obj), obj_get_cost_per_day(obj),
                  obj_get_timer(obj), obj_get_value(obj, 0), obj_get_value(obj, 1),
                  obj_get_value(obj, 2), obj_get_value(obj, 3), obj_get_value(obj, 4),
                  obj_get_value(obj, 5), obj_get_value(obj, 6), obj_get_value(obj, 7),
                  obj_get_value(obj, 8), obj_get_value(obj, 9), obj_get_value(obj, 10),
                  obj_get_value(obj, 11), obj_get_value(obj, 12), obj_get_value(obj, 13),
                  obj_get_value(obj, 14), obj_get_value(obj, 15), obj_get_extra_flags(obj) ? "Set." : "Not Set.",
                  obj_get_level(obj), material_names[(int)obj_get_value(obj, 7)],
                  ebitbuf, OLC_SCRIPT(d) ? "Set." : "Not Set.",
                  size_names[obj_get_size(obj)]);
  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/

void oedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int number;
  obj_rnum rnum;
  int max_val, min_val;
  char *oldtext = nullptr;
  struct board_info *tmp;
  struct obj_data *obj;
  obj_rnum robj;

  switch (OLC_MODE(d)) {

    case OEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          oedit_save_internally(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                 "OLC: %s edits obj %d", char_get_name(d->character), OLC_NUM(d));
          if (CONFIG_OLC_SAVE) {
            oedit_save_to_disk(g, real_zone_by_thing(g, OLC_NUM(d)));
            write_to_output(g, d, "Object saved to disk.\r\n");
          } else
            write_to_output(g, d, "Object saved to memory.\r\n");
          if (obj_get_type_flag(OLC_OBJ(d)) == ITEM_BOARD) {
            if ((tmp = locate_board(g, obj_get_vnum(g, OLC_OBJ(d)))) != nullptr)
              save_board(g, tmp);
            else
              create_new_board(g, obj_get_vnum(g, OLC_OBJ(d)));
          }
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case 'n':
        case 'N':
          /* If not saving, we must free the script_proto list. */
          obj_set_proto_script(OLC_OBJ(d), OLC_SCRIPT(d));
          free_proto_script(OLC_OBJ(d), OBJ_TRIGGER);
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case 'a': /* abort quit */
        case 'A':
          oedit_disp_menu(g, d);
          return;
        default:
          write_to_output(g, d, "Invalid choice!\r\n");
          write_to_output(g, d, "Do you wish to save your changes? : \r\n");
          return;
      }

    case OEDIT_COPY:
      if ((rnum = real_object(g, atoi(arg))) != NOTHING) {
        oedit_setup_existing(g, d, rnum);
      } else
        write_to_output(g, d, "That object does not exist.\r\n");
      break;

    case OEDIT_DELETE:
      if (*arg == 'y' || *arg == 'Y') {
        if (delete_object(g, obj_get_item_number(OLC_OBJ(d))) != NOTHING)
          write_to_output(g, d, "Object deleted.\r\n");
        else
          write_to_output(g, d, "Couldn't delete the object!\r\n");

        cleanup_olc(g, d, CLEANUP_ALL);
      } else if (*arg == 'n' || *arg == 'N') {
        oedit_disp_menu(g, d);
        OLC_MODE(d) = OEDIT_MAIN_MENU;
      } else
        write_to_output(g, d, "Please answer 'Y' or 'N': ");
      return;

    case OEDIT_MAIN_MENU:
      /*
     * Throw us out to whichever edit mode based on user input.
     */
      switch (*arg) {
        case 'q':
        case 'Q':
          if (desc_get_connected(d) != CON_IEDIT) {
            if (OLC_VAL(d)) { /* Something has been modified. */
              write_to_output(g, d, "Do you wish to save your changes? : ");
              OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
            } else
              cleanup_olc(g, d, CLEANUP_ALL);
          } else {
            obj = OLC_IOBJ(d);
            if (!object_in_game(g, obj)) { /* it was extracted while being edited */
              send_to_char(g, d->character, "\r\nThat object no longer exists. Changes discarded.\r\n");
              cleanup_olc(g, d, CLEANUP_ALL);
              return;
            }
            send_to_char(g, d->character, "\r\nCommitting iedit changes.\r\n");
            /* obj gets a new UID below; drop its old lookup entry first */
            remove_from_lookup_table(g, obj_get_id(obj));
            /* remove any old scripts */
            if (obj_get_script(obj)) {
              extract_script(g, obj, OBJ_TRIGGER);
              obj_set_script(obj, nullptr);
            }
            commit_iedit(g, obj, OLC_OBJ(d));
            obj_set_id(obj, dg_new_obj_uid(g));
            /* find_obj helper */
            add_to_lookup_table(g, obj_get_id(obj), OBJ_TRIGGER, (void *)obj);
            if (obj_get_vnum(g, obj) != NOTHING) {
              free_proto_script(obj, OBJ_TRIGGER);
              robj = real_object(g, obj_get_vnum(g, obj));
              copy_proto_script(&g->obj_proto[robj], obj, OBJ_TRIGGER);
              assign_triggers(g, obj, OBJ_TRIGGER);
            }
            obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);
            mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                   "OLC: %s iedit a unique #%d", char_get_name(d->character), obj_get_vnum(g, obj));
            if (d->character) {
              char_plr_set(d->character, PLR_WRITING, false);
              desc_set_connected(d, CON_PLAYING);
              act(g, "$n stops using OLC.", true, d->character, 0, 0, TO_ROOM);
            }
            free(OLC_OBJ(d)); /* its strings now belong to obj */
            free(d->olc);
            d->olc = nullptr;
          }
          return;
        case '1':
          write_to_output(g, d, "Enter namelist : ");
          OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
          break;
        case '2':
          write_to_output(g, d, "Enter short desc : ");
          OLC_MODE(d) = OEDIT_SHORTDESC;
          break;
        case '3':
          write_to_output(g, d, "Enter long desc :-\r\n| ");
          OLC_MODE(d) = OEDIT_LONGDESC;
          break;
        case '4':
          OLC_MODE(d) = OEDIT_ACTDESC;
          send_editor_help(g, d);
          write_to_output(g, d, "Enter action description:\r\n\r\n");
          if (obj_get_action_description(OLC_OBJ(d))) {
            write_to_output(g, d, "%s", obj_get_action_description(OLC_OBJ(d)));
            oldtext = strdup(obj_get_action_description(OLC_OBJ(d)));
          }
          string_write(d, obj_action_description_slot(OLC_OBJ(d)), MAX_MESSAGE_LENGTH, 0, oldtext);
          OLC_VAL(d) = 1;
          break;
        case '5':
          oedit_disp_type_menu(g, d);
          OLC_MODE(d) = OEDIT_TYPE;
          break;
        case '6':
          oedit_disp_extra_menu(g, d);
          OLC_MODE(d) = OEDIT_EXTRAS;
          break;
        case '7':
          oedit_disp_wear_menu(g, d);
          OLC_MODE(d) = OEDIT_WEAR;
          break;
        case '8':
          write_to_output(g, d, "Enter weight : ");
          OLC_MODE(d) = OEDIT_WEIGHT;
          break;
        case '9':
          write_to_output(g, d, "Enter cost : ");
          OLC_MODE(d) = OEDIT_COST;
          break;
        case 'a':
        case 'A':
          write_to_output(g, d, "Enter cost per day : ");
          OLC_MODE(d) = OEDIT_COSTPERDAY;
          break;
        case 'b':
        case 'B':
          write_to_output(g, d, "Enter timer : ");
          OLC_MODE(d) = OEDIT_TIMER;
          break;
        case 'c':
        case 'C':
          /*
       * Clear any old values  
       */
          obj_set_value(OLC_OBJ(d), 0, 0);
          obj_set_value(OLC_OBJ(d), 1, 0);
          obj_set_value(OLC_OBJ(d), 2, 0);
          obj_set_value(OLC_OBJ(d), 3, 0);
          obj_set_value(OLC_OBJ(d), 4, 0);
          obj_set_value(OLC_OBJ(d), 5, 0);
          obj_set_value(OLC_OBJ(d), 6, 0);
          OLC_VAL(d) = 1;
          oedit_disp_val1_menu(g, d);
          break;
        case 'd':
        case 'D':
          oedit_disp_prompt_apply_menu(g, d);
          break;
        case 'e':
        case 'E':
          /*
       * If extra descriptions don't exist.
       */
          if (obj_get_ex_description(OLC_OBJ(d)) == nullptr) {
            CREATE(*obj_ex_description_slot(OLC_OBJ(d)), struct extra_descr_data, 1);
          }
          OLC_DESC(d) = obj_get_ex_description(OLC_OBJ(d));
          oedit_disp_extradesc_menu(g, d);
          break;
        case 'm':
        case 'M':
          write_to_output(g, d, "Enter new minimum level: ");
          OLC_MODE(d) = OEDIT_LEVEL;
          break;
        case 'n':
        case 'N':
          OLC_MODE(d) = OEDIT_MATERIAL;
          oedit_disp_material_menu(g, d);
          break;
        case 'p':
        case 'P':
          oedit_disp_perm_menu(g, d);
          OLC_MODE(d) = OEDIT_PERM;
          break;
        case 's':
        case 'S':
          if (desc_get_connected(d) != CON_IEDIT) {
            OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
            dg_script_menu(g, d);
          } else {
            write_to_output(g, d, "\r\nScripts cannot be modified on individual objects.\r\nEnter choice : ");
          }
          return;
        case 't':
        case 'T':
          oedit_disp_prompt_spellbook_menu(g, d);
          break;
        case 'w':
        case 'W':
          if (desc_get_connected(d) == CON_IEDIT) { /* it would turn this object into a different prototype */
            write_to_output(g, d, "\r\nYou can't copy another object onto an individual object.\r\nEnter choice : ");
            return;
          }
          write_to_output(g, d, "Copy what object? ");
          OLC_MODE(d) = OEDIT_COPY;
          break;
        case 'x':
        case 'X':
          write_to_output(g, d, "Are you sure you want to delete this object? ");
          OLC_MODE(d) = OEDIT_DELETE;
          break;
        case 'y':
        case 'Y':
          oedit_disp_size_menu(g, d);
          OLC_MODE(d) = OEDIT_SIZE;
          break;
        case 'Z':
        case 'z':
          search_replace(arg, "z ", "");
          do_wiznet(g, d->character, arg, 0, 0);
          break;
        default:
          oedit_disp_menu(g, d);
          break;
      }
      return; /*
				 * end of OEDIT_MAIN_MENU 
				 */

    case OLC_SCRIPT_EDIT:
      if (dg_script_edit_parse(g, d, arg)) return;
      break;

    case OEDIT_EDIT_NAMELIST:
      if (!genolc_checkstring(d, arg))
        break;
      if (obj_get_name(OLC_OBJ(d)))
        free(obj_get_name(OLC_OBJ(d)));
      obj_set_name(OLC_OBJ(d), str_udup(arg));
      break;

    case OEDIT_SHORTDESC:
      if (!genolc_checkstring(d, arg))
        break;
      if (obj_get_short_description(OLC_OBJ(d)))
        free(obj_get_short_description(OLC_OBJ(d)));
      obj_set_short_description(OLC_OBJ(d), str_udup(arg));
      break;

    case OEDIT_LONGDESC:
      if (!genolc_checkstring(d, arg))
        break;
      if (obj_get_description(OLC_OBJ(d)))
        free(obj_get_description(OLC_OBJ(d)));
      obj_set_description(OLC_OBJ(d), str_udup(arg));
      break;

    case OEDIT_TYPE:
      number = atoi(arg);
      if ((number < 1) || (number >= NUM_ITEM_TYPES)) {
        write_to_output(g, d, "Invalid choice, try again : ");
        return;
      } else
        obj_set_type_flag(OLC_OBJ(d), number);
      /* what's the boundschecking worth if we don't do this ? -- Welcor */
      obj_set_value(OLC_OBJ(d), 3, 0);
      obj_set_value(OLC_OBJ(d), 2, 0);
      obj_set_value(OLC_OBJ(d), 1, 0);
      obj_set_value(OLC_OBJ(d), 0, 0);
      break;

    case OEDIT_EXTRAS:
      number = atoi(arg);
      if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
        oedit_disp_extra_menu(g, d);
        return;
      } else if (number == 0)
        break;
      else {
        obj_extra_toggle(OLC_OBJ(d), number - 1);
        oedit_disp_extra_menu(g, d);
        return;
      }

    case OEDIT_WEAR:
      number = atoi(arg);
      if ((number < 0) || (number > NUM_ITEM_WEARS)) {
        write_to_output(g, d, "That's not a valid choice!\r\n");
        oedit_disp_wear_menu(g, d);
        return;
      } else if (number == 0) /* Quit. */
        break;
      else {
        obj_wear_toggle(OLC_OBJ(d), (number - 1));
        oedit_disp_wear_menu(g, d);
        return;
      }

    case OEDIT_WEIGHT:
      obj_set_weight(OLC_OBJ(d), LIMIT(atoi(arg), 0, MAX_OBJ_WEIGHT));
      break;

    case OEDIT_COST:
      obj_set_cost(OLC_OBJ(d), LIMIT(atoi(arg), 0, MAX_OBJ_COST));
      break;

    case OEDIT_COSTPERDAY:
      obj_set_cost_per_day(OLC_OBJ(d), LIMIT(atoi(arg), 0, MAX_OBJ_RENT));
      break;

    case OEDIT_TIMER:
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_PORTAL:
          obj_set_timer(OLC_OBJ(d), LIMIT(atoi(arg), -1, MAX_OBJ_TIMER));
          break;
        default:
          obj_set_timer(OLC_OBJ(d), LIMIT(atoi(arg), 0, MAX_OBJ_TIMER));
          break;
      }
      break;

    case OEDIT_LEVEL:
      obj_set_level(OLC_OBJ(d), MAX(atoi(arg), 0));
      break;

    case OEDIT_MATERIAL:
      obj_set_value(OLC_OBJ(d), 7, LIMIT(atoi(arg), 0, NUM_MATERIALS));
      break;

    case OEDIT_PERM:
      if ((number = atoi(arg)) == 0)
        break;
      if (number > 0 && number <= NUM_AFF_FLAGS) {
        /* Setting AFF_CHARM on objects like this is dangerous. */
        if (number != AFF_CHARM) {
          obj_aff_toggle(OLC_OBJ(d), number);
        }
      }
      oedit_disp_perm_menu(g, d);
      return;

    case OEDIT_SIZE:
      number = atoi(arg) - 1;
      obj_set_size(OLC_OBJ(d), LIMIT(number, 0, NUM_SIZES - 1));
      break;

    case OEDIT_VALUE_1:
      /*
     * Lucky, I don't need to check any of these for out of range values.
     * Hmm, I'm not so sure - Rv  
     */
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_WEAPON:
          obj_set_value(OLC_OBJ(d), 0, LIMIT(atoi(arg), WEAPON_TYPE_UNARMED, MAX_WEAPON_TYPES));
          break;
        case ITEM_CONTAINER:
          obj_set_value(OLC_OBJ(d), 0, LIMIT(atoi(arg), -1, MAX_CONTAINER_SIZE));
          break;
        default:
          obj_set_value(OLC_OBJ(d), 0, atoi(arg));
      }
      /*
     * proceed to menu 2 
     */
      oedit_disp_val2_menu(g, d);
      return;
    case OEDIT_VALUE_2:
      /*
     * Here, I do need to check for out of range values.
     */
      number = atoi(arg);
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
          if (number == 0 || number == -1)
            obj_set_value(OLC_OBJ(d), 1, -1);
          else
            obj_set_value(OLC_OBJ(d), 1, LIMIT(number, 1, SKILL_TABLE_SIZE - 1));
          oedit_disp_val3_menu(g, d);
          break;
        case ITEM_CONTROL:
          if (number <= 0)
            obj_set_value(OLC_OBJ(d), 1, 1);
          else if (number > 3)
            obj_set_value(OLC_OBJ(d), 1, 3);
          else
            obj_set_value(OLC_OBJ(d), 1, number);
          oedit_disp_val5_menu(g, d);
          break;
        case ITEM_CONTAINER:
        case ITEM_VEHICLE:
        case ITEM_WINDOW:
        case ITEM_HATCH:
        case ITEM_PORTAL:
          /*
       * Needs some special handling since we are dealing with flag values
       * here.
       */
          if (number < 0 || number > 4)
            oedit_disp_container_flags_menu(g, d);
          else if (number != 0) {
            obj_set_value(OLC_OBJ(d), 1, obj_get_value(OLC_OBJ(d), 1) ^ (1U << (number - 1)));
            OLC_VAL(d) = 1;
            oedit_disp_val2_menu(g, d);
          } else
            oedit_disp_val3_menu(g, d);
          break;
        case ITEM_WEAPON:
          obj_set_value(OLC_OBJ(d), 1, LIMIT(number, 1, MAX_WEAPON_NDICE));
          oedit_disp_val3_menu(g, d);
          break;

        default:
          obj_set_value(OLC_OBJ(d), 1, number);
          oedit_disp_val3_menu(g, d);
      }
      return;

    case OEDIT_VALUE_3:
      number = atoi(arg);
      /*
     * Quick'n'easy error checking.
     */
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_WEAPON:
          min_val = 1;
          max_val = MAX_WEAPON_SDICE;
          break;
        case ITEM_ARMOR:
          min_val = 0;
          max_val = 100;
          break;
        case ITEM_WAND:
        case ITEM_STAFF:
          min_val = 0;
          max_val = 20;
          break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
          min_val = 0;
          max_val = NUM_LIQ_TYPES - 1;
          break;
        case ITEM_KEY:
          min_val = 0;
          max_val = 60000;
          break;
        default:
          min_val = -32000;
          max_val = 60000;
      }
      obj_set_value(OLC_OBJ(d), 2, LIMIT(number, min_val, max_val));
      oedit_disp_val4_menu(g, d);
      return;

    case OEDIT_VALUE_4:
      number = atoi(arg);
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_HATCH:
          min_val = 1;
          max_val = 60000;
          break;
        case ITEM_WAND:
        case ITEM_STAFF:
          min_val = 1;
          max_val = SKILL_TABLE_SIZE - 1;
          break;
        case ITEM_WEAPON:
          min_val = 0;
          max_val = NUM_ATTACK_TYPES - 1;
          break;
        case ITEM_ARMOR:
          if (number < 0) /* We're storing armor checks as positive numbers */
            number = 0 - number;
          min_val = 0;
          max_val = 20;
          break;
        default:
          min_val = -32000;
          max_val = 32000;
          break;
      }
      obj_set_value(OLC_OBJ(d), 3, LIMIT(number, min_val, max_val));
      oedit_disp_val5_menu(g, d);
      return;

    case OEDIT_VALUE_5:
      min_val = 1;
      max_val = 100;
      obj_set_value(OLC_OBJ(d), 4, LIMIT(atoi(arg), min_val, max_val));
      obj_set_value(OLC_OBJ(d), 5, max_val);
      oedit_disp_val7_menu(g, d);
      return;

    case OEDIT_VALUE_7:
      number = atoi(arg);
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_WEAPON:
          min_val = 0;
          max_val = MAX_CRIT_TYPE;
          break;
        case ITEM_ARMOR:
          min_val = -100; /* Want to allow weird armor that improves casting */
          max_val = 100;
          break;
        default:
          min_val = -32000;
          max_val = 32000;
          break;
      }
      obj_set_value(OLC_OBJ(d), 6, LIMIT(atoi(arg), min_val, max_val));
      oedit_disp_val9_menu(g, d);
      return;

    case OEDIT_VALUE_9:
      number = atoi(arg);
      switch (obj_get_type_flag(OLC_OBJ(d))) {
        case ITEM_WEAPON:
          min_val = 0;
          max_val = 19;
          break;
        default:
          min_val = -32000;
          max_val = 32000;
          break;
      }
      obj_set_value(OLC_OBJ(d), 8, LIMIT(atoi(arg), min_val, max_val));
      break;

    case OEDIT_PROMPT_APPLY:
      if ((number = atoi(arg)) == 0)
        break;
      else if (number < 0 || number > MAX_OBJ_AFFECT) {
        oedit_disp_prompt_apply_menu(g, d);
        return;
      }
      OLC_VAL(d)  = number - 1;
      OLC_MODE(d) = OEDIT_APPLY;
      oedit_disp_apply_menu(g, d);
      return;

    case OEDIT_APPLY:
      if ((number = atoi(arg)) == 0) {
        obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].location = 0;
        obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].modifier = 0;
        oedit_disp_prompt_apply_menu(g, d);
      } else if (number < 0 || number >= NUM_APPLIES)
        oedit_disp_apply_menu(g, d);
      else {
        int counter;

        /* add in check here if already applied.. deny builders another */
        if (char_get_admlevel(d->character) < ADMLVL_GRGOD) {
          for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
            if (obj_get_affected(OLC_OBJ(d))[counter].location == number) {
              write_to_output(g, d, "Object already has that apply.");
              return;
            }
          }
        }

        obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].location = number;
        write_to_output(g, d, "Modifier : ");
        OLC_MODE(d) = OEDIT_APPLYMOD;
      }
      return;

    case OEDIT_APPLYMOD:
      obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].modifier = atoi(arg);
      oedit_disp_apply_spec_menu(g, d);
      return;

    case OEDIT_APPLYSPEC:
      if (isdigit(*arg))
        obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].specific = affect_checked_specific(obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].location, atoi(arg));
      else switch (obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].location) {
          case APPLY_SKILL:
            number = find_skill_num(g, arg, SKTYPE_SKILL);
            if (number > -1)
              obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].specific = number;
            break;
          case APPLY_FEAT:
            number = find_feat_num(g, arg);
            if (number > -1)
              obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].specific = number;
            break;
          default:
            obj_get_affected(OLC_OBJ(d))[OLC_VAL(d)].specific = 0;
            break;
        }
      oedit_disp_prompt_apply_menu(g, d);
      return;

    case OEDIT_EXTRADESC_KEY:
      if (genolc_checkstring(d, arg)) {
        if (OLC_DESC(d)->keyword)
          free(OLC_DESC(d)->keyword);
        OLC_DESC(d)->keyword = str_udup(arg);
      }
      oedit_disp_extradesc_menu(g, d);
      return;

    case OEDIT_EXTRADESC_MENU:
      switch ((number = atoi(arg))) {
        case 0:
          if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
            struct extra_descr_data *temp;

            if (OLC_DESC(d)->keyword)
              free(OLC_DESC(d)->keyword);
            if (OLC_DESC(d)->description)
              free(OLC_DESC(d)->description);

            /*
	 * Clean up pointers  
	 */
            REMOVE_FROM_LIST(OLC_DESC(d), *obj_ex_description_slot(OLC_OBJ(d)), next, temp);
            free(OLC_DESC(d));
            OLC_DESC(d) = nullptr;
          }
          break;

        case 1:
          OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
          write_to_output(g, d, "Enter keywords, separated by spaces :-\r\n| ");
          return;

        case 2:
          OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
          send_editor_help(g, d);
          write_to_output(g, d, "Enter the extra description:\r\n\r\n");
          if (OLC_DESC(d)->description) {
            write_to_output(g, d, "%s", OLC_DESC(d)->description);
            oldtext = strdup(OLC_DESC(d)->description);
          }
          string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
          OLC_VAL(d) = 1;
          return;

        case 3:
          /*
       * Only go to the next description if this one is finished.
       */
          if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
            struct extra_descr_data *new_extra;

            if (OLC_DESC(d)->next)
              OLC_DESC(d) = OLC_DESC(d)->next;
            else { /* Make new extra description and attach at end. */
              CREATE(new_extra, struct extra_descr_data, 1);
              OLC_DESC(d)->next = new_extra;
              OLC_DESC(d)       = OLC_DESC(d)->next;
            }
          }
          /*
       * No break - drop into default case.
       */
          [[fallthrough]];
        default:
          oedit_disp_extradesc_menu(g, d);
          return;
      }
      break;

    case OEDIT_PROMPT_SPELLBOOK:
      if ((number = atoi(arg)) == 0)
        break;
      else if (number < 0 || number > SPELLBOOK_SIZE) { /* a slot in sbinfo[] */
        oedit_disp_prompt_spellbook_menu(g, d);
        return;
      }
      OLC_VAL(d)  = number - 1;
      OLC_MODE(d) = OEDIT_SPELLBOOK;
      oedit_disp_spellbook_menu(g, d);
      return;

    case OEDIT_SPELLBOOK:
      if ((number = atoi(arg)) == 0) {
        if (obj_get_sbinfo(OLC_OBJ(d))) {
          obj_get_sbinfo(OLC_OBJ(d))[OLC_VAL(d)].spellname = 0;
          obj_get_sbinfo(OLC_OBJ(d))[OLC_VAL(d)].pages     = 0;
        } else {
          CREATE(*obj_sbinfo_slot(OLC_OBJ(d)), struct obj_spellbook_spell, SPELLBOOK_SIZE);
        }
        oedit_disp_prompt_spellbook_menu(g, d);
      } else if (number < 0 || number >= SKILL_TABLE_SIZE) {
        oedit_disp_spellbook_menu(g, d);
      } else {
        int counter;

        /* add in check here if already applied.. deny builders another */
        if (char_get_level(d->character) < ADMLVL_IMPL) {
          for (counter = 0; counter < SPELLBOOK_SIZE; counter++) {
            if (obj_get_sbinfo(OLC_OBJ(d)) && obj_get_sbinfo(OLC_OBJ(d))[counter].spellname == number) {
              write_to_output(g, d, "Object already has that spell.");
              return;
            }
          }
        }

        if (!obj_get_sbinfo(OLC_OBJ(d))) {
          CREATE(*obj_sbinfo_slot(OLC_OBJ(d)), struct obj_spellbook_spell, SPELLBOOK_SIZE);
        }

        obj_get_sbinfo(OLC_OBJ(d))[OLC_VAL(d)].spellname = number;
        obj_get_sbinfo(OLC_OBJ(d))[OLC_VAL(d)].pages     = MAX(1, g->spell_info[number].spell_level * 2);
        oedit_disp_prompt_spellbook_menu(g, d);
      }
      return;

    default:
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: Reached default case in oedit_parse()!");
      write_to_output(g, d, "Oops...\r\n");
      break;
  }

  /*
   * If we get here, we have changed something.  
   */
  OLC_VAL(d) = 1;
  oedit_disp_menu(g, d);
}

void oedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    case OEDIT_ACTDESC:
      oedit_disp_menu(g, d);
      break;
    case OEDIT_EXTRADESC_DESCRIPTION:
      oedit_disp_extradesc_menu(g, d);
      break;
  }
}

/* this is all iedit stuff */
static bool object_in_game(GameWorld *g, const struct obj_data *obj)
{
  for (const struct obj_data *k = g->objects; k; k = obj_get_next(k))
    if (k == obj)
      return true;
  return false;
}

/* Replaces what obj is with what the editor made of it, but not where it is or who is using it. */
static void commit_iedit(GameWorld *g, struct obj_data *obj, const struct obj_data *edited)
{
  struct obj_data live = *obj;

  if (obj_get_item_number(obj) != NOTHING)
    free_object_strings_proto(g, obj);
  else
    free_object_strings(g, obj);

  *obj = *edited;
  obj_set_in_room(obj, obj_get_in_room(&live));
  obj_set_carried_by(obj, obj_get_carried_by(&live));
  obj_set_worn_by(obj, obj_get_worn_by(&live));
  obj_set_worn_on(obj, obj_get_worn_on(&live));
  obj_set_in_obj(obj, obj_get_in_obj(&live));
  obj_set_contains(obj, obj_get_contains(&live));
  obj_set_next_content(obj, obj_get_next_content(&live));
  obj_set_next(obj, obj_get_next(&live));
  obj_set_sitting(obj, obj_get_sitting(&live));
  obj_set_user(obj, obj_get_user(&live));
  obj_set_target(obj, obj_get_target(&live));
  obj_weight_changed(g, obj, obj_get_weight(obj) - obj_get_weight(&live));
}

static void iedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct obj_data *real_num)
{
  struct obj_data *obj;

  /* A private copy, as in oedit_setup_existing(): it isn't in the game, and shares no script. */
  CREATE(obj, struct obj_data, 1);
  copy_object(g, obj, real_num);
  obj_set_script(obj, nullptr);

  OLC_OBJ(d)  = obj;
  OLC_IOBJ(d) = real_num;
  OLC_VAL(d)  = 0;
  oedit_disp_menu(g, d);
}

ACMD(do_iedit)
{
  struct obj_data *k;
  int found = 0;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg || !*argument) {
    send_to_char(g, ch, "You must supply an object name.\r\n");
  }

  if ((k = get_obj_in_equip_vis(g, ch, arg, nullptr, char_get_equipment_array(ch)))) {
    found = 1;
  } else if ((k = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    found = 1;
  } else if ((k = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
    found = 1;
  } else if ((k = get_obj_vis(g, ch, arg, nullptr))) {
    found = 1;
  }

  if (!found) {
    send_to_char(g, ch, "Couldn't find that object. Sorry.\r\n");
    return;
  }

  /* set up here */
  CREATE(OLC(char_get_desc(ch)), struct oasis_olc_data, 1);
  obj_extra_set(k, ITEM_UNIQUE_SAVE, true);

  char_plr_set(ch, PLR_WRITING, true);
  iedit_setup_existing(g, char_get_desc(ch), k);
  OLC_VAL(char_get_desc(ch)) = 0;

  act(g, "$n starts using OLC.", true, ch, 0, 0, TO_ROOM);

  desc_set_connected(char_get_desc(ch), CON_IEDIT);

  return;
}
