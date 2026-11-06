/************************************************************************
 * OasisOLC - Mobiles / medit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/medit.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/shop.h"
#include "dbat/genolc.h"
#include "dbat/genmob.h"
#include "dbat/genzon.h"
#include "dbat/genshp.h"
#include "dbat/oasis.h"
#include "dbat/handler.h"
#include "dbat/improved-edit.h"
#include "dbat/dg_olc.h"
#include "dbat/screen.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/act.wizard.h"
#include "dbat/modify.h"
#include "dbat/dg_scripts.h"
#include "dbat/consts/npc.h"
#include "dbat/config.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static void init_mobile(GameWorld *g, struct char_data *mob);
static void medit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void medit_setup_existing(GameWorld *g, struct descriptor_data *d, mob_rnum rmob_num);
static void medit_setup_new(GameWorld *g, struct descriptor_data *d);

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

ACMD(do_oasis_medit)
{
  IDXTYPE number = NOBODY;
  int save       = 0;
  mob_rnum real_num;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  two_arguments(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(g, ch, "Specify a mobile VNUM to edit.\r\n");
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
  /** If a numeric argument was given (like a room number), get it.          **/
  /****************************************************************************/
  if (number == NOBODY)
    number = atoi(buf1);

  /****************************************************************************/
  /** Check that whatever it is isn't already being edited.                  **/
  /****************************************************************************/
  for (d = g->descriptors; d; d = d->next) {
    if (desc_get_connected(d) == CON_MEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(g, ch, "That mobile is currently being edited by %s.\r\n",
                     char_get_name(d->character));
        return;
      }
    }
  }

  d = char_get_desc(ch);

  /****************************************************************************/
  /** Give descriptor an OLC structure.                                      **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
           "SYSERR: do_oasis_medit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  OLC_ZNUM(d) = save ? real_zone(g, number) : real_zone_by_thing(g, number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_cannot_edit(g, ch, g->zones[OLC_ZNUM(d)].number);
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** If save is TRUE, save the mobiles.                                     **/
  /****************************************************************************/
  if (save) {
    send_to_char(g, ch, "Saving all mobiles in zone %d.\r\n",
                 g->zones[OLC_ZNUM(d)].number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
           "OLC: %s saves mobile info for zone %d.",
           char_get_name(ch), g->zones[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Save the mobiles.                                                    **/
    /**************************************************************************/
    save_mobiles(g, OLC_ZNUM(d));

    /**************************************************************************/
    /** Free the olc structure stored in the descriptor.                     **/
    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  /****************************************************************************/
  /** If this is a new mobile, setup a new one, otherwise, setup the         **/
  /** existing mobile.                                                       **/
  /****************************************************************************/
  if ((real_num = real_mobile(g, number)) == NOBODY)
    medit_setup_new(g, d);
  else
    medit_setup_existing(g, d, real_num);

  medit_disp_menu(g, d);
  desc_set_connected(d, CON_MEDIT);

  /****************************************************************************/
  /** Display the OLC messages to the players in the same room as the        **/
  /** builder and also log it.                                               **/
  /****************************************************************************/
  act(g, "$n starts using OLC.", true, d->character, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
         char_get_name(ch), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));
}

static void medit_save_to_disk(GameWorld *g, zone_vnum foo)
{
  save_mobiles(g, real_zone(g, foo));
}

static void medit_setup_new(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure.  
   */
  CREATE(mob, struct char_data, 1);

  init_mobile(g, mob);

  char_set_nr(mob, NOBODY);
  /*
   * Set up some default strings.
   */
  char_set_alias(mob, strdup("mob unfinished"));
  char_set_sdesc(mob, strdup("the unfinished mob"));
  char_set_ldesc(mob, strdup("An unfinished mob stands here.\r\n"));
  char_set_ddesc(mob, strdup("It looks unfinished.\r\n"));
  char_set_race(mob, RACE_HUMAN);
  OLC_SCRIPT(d) = nullptr;

  OLC_MOB(d) = mob;
  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_VAL(d)       = false;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
}

/*-------------------------------------------------------------------*/

static void medit_setup_existing(GameWorld *g, struct descriptor_data *d, mob_rnum rmob_num)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure. 
   */
  CREATE(mob, struct char_data, 1);

  copy_mobile(g, mob, g->mob_proto + rmob_num);

  OLC_MOB(d)       = mob;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
  dg_olc_script_copy(d);
  /*
   * The edited mob must not have a script.
   * It will be assigned to the updated mob later, after editing.
   */
  char_set_script(mob, nullptr);
  char_set_proto_script(OLC_MOB(d), nullptr);
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
static void init_mobile(GameWorld *g, struct char_data *mob)
{
  clear_char(mob);

  char_set_sex(mob, SEX_MALE);
  char_set_class(mob, CLASS_NPC_COMMONER);

  char_set_weight(mob, rand_number(g, 100, 200));
  char_set_height(mob, rand_number(g, 100, 200));

  {
    int8_t mental = rand_number(g, 8, 16);
    char_set_real_abils_wis(mob, mental);
    char_set_real_abils_intel(mob, mental);
    char_set_real_abils_str(mob, mental);
  }
  {
    int8_t physical = rand_number(g, 8, 16);
    char_set_real_abils_cha(mob, physical);
    char_set_real_abils_con(mob, physical);
    char_set_real_abils_dex(mob, physical);
  }
  char_reset_aff_abils(mob);

  char_mob_set(mob, MOB_ISNPC, true);
}

/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
static void medit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  int i;
  mob_rnum new_rnum;
  struct descriptor_data *dsc;
  struct char_data *mob;

  i = (real_mobile(g, OLC_NUM(d)) == NOBODY);

  if ((new_rnum = add_mobile(g, OLC_MOB(d), OLC_NUM(d))) == NOBODY) {
    mud_log("medit_save_internally: add_mobile failed.");
    return;
  }

  /* Update triggers */
  /* Free old proto list  */
  if (char_get_proto_script(&g->mob_proto[new_rnum]) &&
      char_get_proto_script(&g->mob_proto[new_rnum]) != OLC_SCRIPT(d))
    free_proto_script(&g->mob_proto[new_rnum], MOB_TRIGGER);

  char_set_proto_script(&g->mob_proto[new_rnum], OLC_SCRIPT(d));

  /* this takes care of the mobs currently in-game */
  for (mob = g->characters; mob; mob = char_get_next(mob)) {
    if (char_get_nr(mob) != new_rnum)
      continue;

    /* remove any old scripts */
    if (char_get_script(mob))
      extract_script(g, mob, MOB_TRIGGER);

    free_proto_script(mob, MOB_TRIGGER);
    copy_proto_script(&g->mob_proto[new_rnum], mob, MOB_TRIGGER);
    assign_triggers(g, mob, MOB_TRIGGER);
  }
  /* end trigger update */

  if (!i) /* Only renumber on new mobiles. */
    return;

  /*
   * Update keepers in shops being edited and other mobs being edited.
   */
  for (dsc = g->descriptors; dsc; dsc = dsc->next) {
    if (desc_get_connected(dsc) == CON_SEDIT) {
      if (shop_get_keeper(OLC_SHOP(dsc)) != NOTHING && shop_get_keeper(OLC_SHOP(dsc)) >= new_rnum)
        shop_set_keeper(OLC_SHOP(dsc), shop_get_keeper(OLC_SHOP(dsc)) + 1);
    } else if (desc_get_connected(dsc) == CON_MEDIT)
      char_set_nr(OLC_MOB(dsc), char_get_nr(OLC_MOB(dsc)) + ((char_get_nr(OLC_MOB(dsc)) != NOTHING && char_get_nr(OLC_MOB(dsc)) >= new_rnum)));
  }

  /*
   * Update other people in zedit too. From: C.Raehl 4/27/99
   */
  for (dsc = g->descriptors; dsc; dsc = dsc->next)
    if (desc_get_connected(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        if (OLC_ZONE(dsc)->cmd[i].command == 'M')
          if (OLC_ZONE(dsc)->cmd[i].arg1 >= new_rnum)
            OLC_ZONE(dsc)->cmd[i].arg1++;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
static void medit_disp_positions(GameWorld *g, struct descriptor_data *d)
{
  int i;

  clear_screen(g, d);

  for (i = 0; *position_types[i] != '\n'; i++) {
    write_to_output(g, d, "@g%2d@n) %s\r\n", i, position_types[i]);
  }
  write_to_output(g, d, "Enter position number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
static void medit_disp_sex(GameWorld *g, struct descriptor_data *d)
{
  int i;

  clear_screen(g, d);

  for (i = 0; i < NUM_GENDERS; i++) {
    write_to_output(g, d, "@g%2d@n) %s\r\n", i, genders[i]);
  }
  write_to_output(g, d, "Enter gender number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
static void medit_disp_mob_flags(GameWorld *g, struct descriptor_data *d)
{
  char flags[MAX_STRING_LENGTH];

  clear_screen(g, d);
  olc_list_columns(g, d, action_bits, NUM_MOB_FLAGS, 1, "  ", 2);
  sprintbitarray(char_get_act(OLC_MOB(d)), action_bits, AF_ARRAY_MAX, flags, sizeof(flags));
  write_to_output(g, d, "\r\nCurrent flags : @c%s@n\r\nEnter mob flags (0 to quit) : ",
                  flags);
}

/*-------------------------------------------------------------------*/

static void medit_disp_personality(GameWorld *g, struct descriptor_data *d)
{

  write_to_output(g, d, "@GPersonalities\n");
  write_to_output(g, d, "@D--------------@n\n");
  for (int i = 0; i < MAX_PERSONALITIES; i++)
    write_to_output(g, d, "@w%d@D) @W%s@n\n", i + 1, npc_personality[i]);
}

/*
 * Display affection flags menu.
 */
static void medit_disp_aff_flags(GameWorld *g, struct descriptor_data *d)
{
  char flags[MAX_STRING_LENGTH];

  clear_screen(g, d);
  olc_list_columns(g, d, affected_bits + 1, NUM_AFF_FLAGS, 1, "  ", 2);
  sprintbitarray(char_get_affected_by(OLC_MOB(d)), affected_bits, AF_ARRAY_MAX, flags, sizeof(flags));
  write_to_output(g, d, "\r\nCurrent flags   : @c%s@n\r\nEnter aff flags (0 to quit) : ",
                  flags);
}

/*-------------------------------------------------------------------*/

/*
 * Display class menu.
 */
static void medit_disp_class(GameWorld *g, struct descriptor_data *d)
{
  int i;
  char buf[MAX_INPUT_LENGTH];
  clear_screen(g, d);

  for (i = 0; i < NUM_CLASSES; i++) {
    if (!sensei_exists(i))
      continue;
    sprintf(buf, "@g%2d@n) %s\r\n", i, sensei_name(i));
    write_to_output(g, d, "%s", buf);
  }
  write_to_output(g, d, "Enter class number : ");
}
/*-------------------------------------------------------------------*/
/*
 * Display race menu.
 */
static void medit_disp_race(GameWorld *g, struct descriptor_data *d)
{
  int columns = 0;
  char buf[MAX_INPUT_LENGTH];

  clear_screen(g, d);
  for (int r = 0; r < NUM_RACES; r++) {
    sprintf(buf, "@g%2d@n) %-20.20s  %s", r, race_name(r),
            !(++columns % 2) ? "\r\n" : "");
    write_to_output(g, d, "%s", buf);
  }
  write_to_output(g, d, "Enter race number : ");
}

/*-------------------------------------------------------------------*/
/*
 * Display size menu.
 */
static void medit_disp_size(GameWorld *g, struct descriptor_data *d)
{
  int i, columns = 0;
  char buf[MAX_INPUT_LENGTH];

  clear_screen(g, d);
  for (i = -1; i < NUM_SIZES; i++) {
    sprintf(buf, "@g%2d@n) %-20.20s  %s", i,
            (i == SIZE_UNDEFINED) ? "DEFAULT" : size_names[i],
            !(++columns % 2) ? "\r\n" : "");
    write_to_output(g, d, "%s", buf);
  }
  write_to_output(g, d, "Enter size number (-1 for default): ");
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
static void medit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *mob;
  char flags[MAX_STRING_LENGTH], flag2[MAX_STRING_LENGTH];

  mob = OLC_MOB(d);
  clear_screen(g, d);

  write_to_output(g, d,
                  "-- Mob Number:  [@c%d@n]\r\n"
                  "@g1@n) Sex: @y%-7.7s@n	         @g2@n) Alias: @y%s\r\n"
                  "@g3@n) S-Desc: @y%s\r\n"
                  "@g4@n) L-Desc:-\r\n@y%s"
                  "@g5@n) D-Desc:-\r\n@y%s"
                  "@g6@n) Level:       [@c%4d@n],  @g7@n) Alignment:    [@c%5d@n]\r\n"
                  "@g8@n) Accuracy Mod:[@c%4d@n],  @g9@n) Damage Mod:   [@c%5d@n]\r\n"
                  "@gA@n) NumDamDice:  [@c%4d@n],  @gB@n) SizeDamDice:  [@c%5d@n]\r\n"
                  "@gC@n) Num HP Dice: [@c%4" PRId64 "@n],  @gD@n) Size HP Dice: [@c%5" PRId64 "@n],  @gE@n) HP Bonus: [@c%5" PRId64 "@n]\r\n"
                  "@gF@n) Armor Class: [@c%4d@n],  @gG@n) Exp:      [@c%" PRId64 "@n],  @gH@n) Gold:  [@c%8d@n]\r\n",

                  OLC_NUM(d), genders[(int)char_get_sex(mob)], char_get_alias(mob),
                  char_get_sdesc(mob), char_get_ldesc(mob), char_get_ddesc(mob), char_get_race_level(mob),
                  char_get_alignment(mob), char_get_accuracy_mod(mob), 0 /* damage mod: unused */,
                  char_get_mob_specials_damnodice(mob), char_get_mob_specials_damsizedice(mob), getCurPL(g, mob), (getCurKI(g, mob)),
                  (getCurST(g, mob)), char_get_armor(mob), char_get_exp(mob), char_get_gold(mob));
  sprintbitarray(char_get_act(mob), action_bits, AF_ARRAY_MAX, flags, sizeof(flags));
  sprintbitarray(char_get_affected_by(mob), affected_bits, AF_ARRAY_MAX, flag2, sizeof(flag2));
  write_to_output(g, d,
                  "@gI@n) Position   : @y%-10s@n,	 @gJ@n) Default   : @y%-10s\r\n"
                  "@gK@n) Personality: @Y%s@n\r\n"
                  "@gL@n) NPC Flags  : @c%s\r\n"
                  "@gM@n) AFF Flags  : @c%s\r\n"
                  "@gN@n) Class      : @y%-10s@n,	 @gO@n) Race      : @y%-10s\r\n"
                  "@gS@n) Script     : @c%s\r\n"
                  "@gW@n) Copy mob              ,	 @gX@n) Delete mob\r\n"
                  "@gY@n) Size       : @y%s\r\n"
                  "@gZ@n) Wiznet     :\r\n"
                  "@gQ@n) Quit\r\n"
                  "Enter choice : ",

                  position_types[(int)char_get_position(mob)],
                  position_types[(int)char_get_mob_specials_default_pos(mob)],
                  npc_personality[char_get_personality(mob)],
                  flags, flag2, char_sensei_name(mob),
                  char_true_race_name(mob),
                  OLC_SCRIPT(d) ? "Set." : "Not Set.", size_names[get_size(mob)]);

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *			The GARGANTAUN event handler			*
 ************************************************************************/

void medit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int i         = -1;
  mob_rnum rnum;
  char *oldtext = nullptr;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
    i = atoi(arg);
    if (!*arg || !(isdigit(arg[0]) || (arg[0] == '-' && isdigit(arg[1])))) {
      write_to_output(g, d, "Field must be numerical, try again : ");
      return;
    }
  } else { /* String response. */
    if (!genolc_checkstring(d, arg))
      return;
  }

  switch (OLC_MODE(d)) {
      /*-------------------------------------------------------------------*/
    case MEDIT_CONFIRM_SAVESTRING:
      /*
     * Ensure mob has MOB_ISNPC set or things will go pear shaped.
     */
      char_mob_set(OLC_MOB(d), MOB_ISNPC, true);
      switch (*arg) {
        case 'y':
        case 'Y':
          /*
       * Save the mob in memory and to disk.
       */
          medit_save_internally(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                 "OLC: %s edits mob %d", char_get_name(d->character), OLC_NUM(d));
          if (CONFIG_OLC_SAVE) {
            medit_save_to_disk(g, g->zones[real_zone_by_thing(g, OLC_NUM(d))].number);
            write_to_output(g, d, "Mobile saved to disk.\r\n");
          } else
            write_to_output(g, d, "Mobile saved to memory.\r\n");
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case 'n':
        case 'N':
          /* If not saving, we must free the script_proto list. We do so by
       * assigning it to the edited mob and letting free_mobile in
       * cleanup_olc handle it. */
          char_set_proto_script(OLC_MOB(d), OLC_SCRIPT(d));
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        default:
          write_to_output(g, d, "Invalid choice!\r\n");
          write_to_output(g, d, "Do you wish to save the mobile? : ");
          return;
      }
      break;

      /*-------------------------------------------------------------------*/
    case MEDIT_MAIN_MENU:
      i = 0;
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /* Anything been changed? */
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case '1':
          OLC_MODE(d) = MEDIT_SEX;
          medit_disp_sex(g, d);
          return;
        case '2':
          OLC_MODE(d) = MEDIT_ALIAS;
          i--;
          break;
        case '3':
          OLC_MODE(d) = MEDIT_S_DESC;
          i--;
          break;
        case '4':
          OLC_MODE(d) = MEDIT_L_DESC;
          i--;
          break;
        case '5':
          OLC_MODE(d) = MEDIT_D_DESC;
          send_editor_help(g, d);
          write_to_output(g, d, "Enter mob description:\r\n\r\n");
          if (char_get_ddesc(OLC_MOB(d))) {
            write_to_output(g, d, "%s", char_get_ddesc(OLC_MOB(d)));
            oldtext = strdup(char_get_ddesc(OLC_MOB(d)));
          }
          string_write(d, char_description_slot(OLC_MOB(d)), MAX_MOB_DESC, 0, oldtext);
          OLC_VAL(d) = 1;
          return;
        case '6':
          OLC_MODE(d) = MEDIT_LEVEL;
          i++;
          break;
        case '7':
          OLC_MODE(d) = MEDIT_ALIGNMENT;
          i++;
          break;
        case '8':
          OLC_MODE(d) = MEDIT_ACCURACY;
          i++;
          break;
        case '9':
          OLC_MODE(d) = MEDIT_DAMAGE;
          i++;
          break;
        case 'a':
        case 'A':
          OLC_MODE(d) = MEDIT_NDD;
          i++;
          break;
        case 'b':
        case 'B':
          OLC_MODE(d) = MEDIT_SDD;
          i++;
          break;
        case 'c':
        case 'C':
          OLC_MODE(d) = MEDIT_NUM_HP_DICE;
          i++;
          break;
        case 'd':
        case 'D':
          OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
          i++;
          break;
        case 'e':
        case 'E':
          OLC_MODE(d) = MEDIT_ADD_HP;
          i++;
          break;
        case 'f':
        case 'F':
          OLC_MODE(d) = MEDIT_AC;
          i++;
          break;
        case 'g':
        case 'G':
          OLC_MODE(d) = MEDIT_EXP;
          i++;
          break;
        case 'h':
        case 'H':
          OLC_MODE(d) = MEDIT_GOLD;
          i++;
          break;
        case 'i':
        case 'I':
          OLC_MODE(d) = MEDIT_POS;
          medit_disp_positions(g, d);
          return;
        case 'j':
        case 'J':
          OLC_MODE(d) = MEDIT_DEFAULT_POS;
          medit_disp_positions(g, d);
          return;
        case 'k':
        case 'K':
          OLC_MODE(d) = MEDIT_PERSONALITY;
          medit_disp_personality(g, d);
          return;
        case 'l':
        case 'L':
          OLC_MODE(d) = MEDIT_NPC_FLAGS;
          medit_disp_mob_flags(g, d);
          return;
        case 'm':
        case 'M':
          OLC_MODE(d) = MEDIT_AFF_FLAGS;
          medit_disp_aff_flags(g, d);
          return;
        case 'n':
        case 'N':
          OLC_MODE(d) = MEDIT_CLASS;
          medit_disp_class(g, d);
          return;
        case 'o':
        case 'O':
          OLC_MODE(d) = MEDIT_RACE;
          medit_disp_race(g, d);
          return;
        case 's':
        case 'S':
          OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
          dg_script_menu(g, d);
          return;
        case 'w':
        case 'W':
          write_to_output(g, d, "Copy what mob? ");
          OLC_MODE(d) = MEDIT_COPY;
          return;
        case 'x':
        case 'X':
          write_to_output(g, d, "Are you sure you want to delete this mobile? ");
          OLC_MODE(d) = MEDIT_DELETE;
          return;
        case 'y':
        case 'Y':
          OLC_MODE(d) = MEDIT_SIZE;
          medit_disp_size(g, d);
          return;
        case 'Z':
        case 'z':
          search_replace(arg, "z ", "");
          do_wiznet(g, d->character, arg, 0, 0);
          break;
        default:
          medit_disp_menu(g, d);
          return;
      }
      if (i == 0)
        break;
      else if (i == 1)
        write_to_output(g, d, "\r\nEnter new value : ");
      else if (i == -1)
        write_to_output(g, d, "\r\nEnter new text :\r\n] ");
      else
        write_to_output(g, d, "Oops...\r\n");
      return;
      /*-------------------------------------------------------------------*/
    case OLC_SCRIPT_EDIT:
      if (dg_script_edit_parse(g, d, arg)) return;
      break;
      /*-------------------------------------------------------------------*/
    case MEDIT_ALIAS:
      smash_tilde(arg);
      if (char_get_alias(OLC_MOB(d)))
        free(char_get_alias(OLC_MOB(d)));
      char_set_alias(OLC_MOB(d), str_udup(arg));
      break;
      /*-------------------------------------------------------------------*/
    case MEDIT_S_DESC:
      smash_tilde(arg);
      if (char_get_sdesc(OLC_MOB(d)))
        free(char_get_sdesc(OLC_MOB(d)));
      char_set_sdesc(OLC_MOB(d), str_udup(arg));
      break;
      /*-------------------------------------------------------------------*/
    case MEDIT_L_DESC:
      smash_tilde(arg);
      if (char_get_ldesc(OLC_MOB(d)))
        free(char_get_ldesc(OLC_MOB(d)));
      if (arg && *arg) {
        char_set_ldesc(OLC_MOB(d), str_printf("%s\r\n", arg));
      } else
        char_set_ldesc(OLC_MOB(d), strdup("undefined"));

      break;
      /*-------------------------------------------------------------------*/
    case MEDIT_D_DESC:
      /*
     * We should never get here.
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: medit_parse(): Reached D_DESC case!");
      write_to_output(g, d, "Oops...\r\n");
      break;
      /*-------------------------------------------------------------------*/
    case MEDIT_NPC_FLAGS:
      if ((i = atoi(arg)) <= 0)
        break;
      else if (i <= NUM_MOB_FLAGS)
        char_mob_toggle(OLC_MOB(d), i - 1);
      medit_disp_mob_flags(g, d);
      return;
      /*-------------------------------------------------------------------*/
    case MEDIT_PERSONALITY:
      if ((i = atoi(arg)) <= 0)
        break;
      else if (i <= MAX_PERSONALITIES) /* menu choice 1..N is npc_personality[0..N-1] */
        char_set_personality(OLC_MOB(d), i - 1);
      medit_disp_personality(g, d);
      return;
      /*-------------------------------------------------------------------*/
    case MEDIT_AFF_FLAGS:
      if ((i = atoi(arg)) <= 0)
        break;
      else if (i <= NUM_AFF_FLAGS)
        char_aff_toggle(OLC_MOB(d), i);
      /* Remove unwanted bits right away. */
      char_aff_set(OLC_MOB(d), AFF_CHARM | AFF_POISON | AFF_GROUP | AFF_SLEEP, false);
      medit_disp_aff_flags(g, d);
      return;
      /*-------------------------------------------------------------------*/

      /*
 * Numerical responses.
 */

    case MEDIT_SEX:
      char_set_sex(OLC_MOB(d), LIMIT(i, 0, NUM_GENDERS - 1));
      break;

    case MEDIT_ACCURACY:
      char_set_accuracy_mod(OLC_MOB(d), LIMIT(i, 0, 50));
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_DAMAGE: /* damage mod no longer exists; input is ignored */
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_NDD:
      char_set_mob_specials_damnodice(OLC_MOB(d), LIMIT(i, 0, 30));
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_SDD:
      char_set_mob_specials_damsizedice(OLC_MOB(d), LIMIT(i, 0, 127));
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_NUM_HP_DICE:
      //GET_HIT(OLC_MOB(d)) = LIMIT(i, 0, CONFIG_LEVEL_CAP);
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_SIZE_HP_DICE:
      //GET_MANA(OLC_MOB(d)) = LIMIT(i, 0, 1000);
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_ADD_HP:
      //GET_MOVE(OLC_MOB(d)) = LIMIT(i, 0, 30000);
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_AC:
      char_set_armor(OLC_MOB(d), LIMIT(i, 10, 200000));
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_EXP:
      char_set_exp(OLC_MOB(d), LIMIT(i, 0, MAX_MOB_EXP));
      if (char_mob_flagged(OLC_MOB(d), MOB_AUTOBALANCE)) {
        char_mob_toggle(OLC_MOB(d), MOB_AUTOBALANCE);
      }
      break;

    case MEDIT_GOLD:
      char_set_gold(OLC_MOB(d), LIMIT(i, 0, MAX_MOB_GOLD));
      break;

    case MEDIT_POS:
      char_set_position(OLC_MOB(d), LIMIT(i, 0, NUM_POSITIONS - 1));
      break;

    case MEDIT_DEFAULT_POS:
      char_set_mob_specials_default_pos(OLC_MOB(d), LIMIT(i, 0, NUM_POSITIONS - 1));
      break;

    case MEDIT_ATTACK:
      char_set_mob_specials_attack_type(OLC_MOB(d), LIMIT(i, 0, NUM_ATTACK_TYPES - 1));
      break;

    case MEDIT_LEVEL:
      char_set_race_level(OLC_MOB(d), LIMIT(i, 1, 150));
      /* Try to add some baseline defaults based on level choice. */
      break;

    case MEDIT_ALIGNMENT:
      char_set_alignment(OLC_MOB(d), LIMIT(i, -1000, 1000));
      break;

    case MEDIT_CLASS:
      if (!char_get_class(OLC_MOB(d))) {
        char_set_class(OLC_MOB(d), CLASS_NPC_COMMONER);
      };
      /* Change size HP dice based on class choice. */
      //GET_MANA(OLC_MOB(d)) = class_hit_die_size[GET_CLASS(OLC_MOB(d))];
      break;

    case MEDIT_COPY:
      if ((rnum = real_mobile(g, atoi(arg))) != NOBODY) {
        medit_setup_existing(g, d, rnum);
      } else
        write_to_output(g, d, "That mob does not exist.\r\n");
      break;

    case MEDIT_DELETE:
      if (*arg == 'y' || *arg == 'Y') {
        if (delete_mobile(g, char_get_nr(OLC_MOB(d))) != NOBODY)
          write_to_output(g, d, "Mobile deleted.\r\n");
        else
          write_to_output(g, d, "Couldn't delete the mobile!\r\n");

        cleanup_olc(g, d, CLEANUP_ALL);
        return;
      }
      if (*arg == 'n' || *arg == 'N') {
        medit_disp_menu(g, d);
        OLC_MODE(d) = MEDIT_MAIN_MENU;
        return;
      } else
        write_to_output(g, d, "Please answer 'Y' or 'N': ");
      break;

    case MEDIT_RACE:
      if (!race_exists(i)) {
        write_to_output(g, d, "That's not a race!");
        break;
      }
      char_set_race(OLC_MOB(d), i);
      /*  Change racial size based on race choice. */
      char_set_size(OLC_MOB(d), race_get_size(char_get_race(OLC_MOB(d))));
      break;

    case MEDIT_SIZE:
      char_set_size(OLC_MOB(d), LIMIT(i, -1, NUM_SIZES - 1));
      break;

      /*-------------------------------------------------------------------*/
    default:
      /*
     * We should never get here.
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: medit_parse(): Reached default case!");
      write_to_output(g, d, "Oops...\r\n");
      break;
  }
  /*-------------------------------------------------------------------*/

  /*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
 */

  OLC_VAL(d) = true;
  medit_disp_menu(g, d);
}

void medit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {

    case MEDIT_D_DESC:
    default:
      medit_disp_menu(g, d);
      break;
  }
}
