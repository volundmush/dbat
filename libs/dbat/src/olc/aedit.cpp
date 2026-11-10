/*
 * File: aedit.c
 * Comment: OLC for MUDs -- this one edits socials
 * by Michael Scott <scottm@workcomm.net> -- 06/10/96
 * for use with OasisOLC
 * ftpable from ftp.circlemud.org:/pub/CircleMUD/contrib/code
 */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/aedit.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/social.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.social.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/oasis.hpp"
#include "dbat/genolc.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

static void aedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static int aedit_find_command(GameWorld *g, const char *txt);
static void aedit_save_to_disk(GameWorld *g, struct descriptor_data *d);

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

ACMD(do_oasis_aedit)
{
  std::string arg;
  struct descriptor_data *d;
  int i;

  if (CONFIG_NEW_SOCIALS == 0) {
    send_to_char(g, ch, "Socials cannot be edited at the moment.\r\n");
    return;
  }

  if (char_get_olc_zone(g, ch) != AEDIT_PERMISSION && char_get_admlevel(ch) < ADMLVL_BUILDER) {
    send_to_char(g, ch, "You don't have access to editing socials.\r\n");
    return;
  }

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_AEDIT) {
      send_to_char(g, ch, "Sorry, only one can edit socials at a time.\r\n");
      return;
    }

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Please specify a social to edit.\r\n");
    return;
  }

  d = char_get_desc(ch);

  if (str_eq_nocase("save", arg)) {
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves socials.", char_get_name(ch));
    send_to_char(g, ch, "Writing social file..\r\n");
    aedit_save_to_disk(g, d);
    send_to_char(g, ch, "Done.\r\n");
    return;
  }

  /*
   * Give descriptor an OLC structure.
   */
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis: Player already had olc structure.");
    delete d->olc;
  }
  d->olc = new oasis_olc_data();

  OLC_NUM(d)     = 0;
  OLC_STORAGE(d) = arg;

  for (OLC_INDEX(d) = 0; (OLC_INDEX(d) <= g->top_of_socialt); OLC_INDEX(d)++)
    if (is_abbrev(OLC_STORAGE(d).c_str(), g->soc_mess_list[OLC_INDEX(d)].command))
      break;

  if (OLC_INDEX(d) > g->top_of_socialt) {
    if ((i = aedit_find_command(g, OLC_STORAGE(d).c_str())) != -1) {
      send_to_char(g, ch, "The '{}' command already exists ({}).\r\n", OLC_STORAGE(d), g->complete_cmd_info[i].command);
      cleanup_olc(g, d, CLEANUP_ALL);
      return;
    }
    send_to_char(g, ch, "Do you wish to add the '{}' action? ", OLC_STORAGE(d));
    OLC_MODE(d) = AEDIT_CONFIRM_ADD;
  } else {
    send_to_char(g, ch, "Do you wish to edit the '{}' action? ", g->soc_mess_list[OLC_INDEX(d)].command);
    OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
  }
  desc_set_connected(d, CON_AEDIT);
  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);
  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} starts editing actions.", char_get_name(ch));
}

static void aedit_setup_new(GameWorld *g, struct descriptor_data *d)
{
  OLC_ACTION(d)                      = new social_messg();
  OLC_ACTION(d)->command             = strdup(OLC_STORAGE(d).c_str());
  OLC_ACTION(d)->sort_as             = strdup(OLC_STORAGE(d).c_str());
  OLC_ACTION(d)->min_victim_position = POS_STANDING;
  OLC_ACTION(d)->min_char_position   = POS_STANDING;
  OLC_ACTION(d)->char_no_arg         = strdup("This action is unfinished.");
  OLC_ACTION(d)->others_no_arg       = strdup("This action is unfinished.");
  aedit_disp_menu(g, d);
  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

static void aedit_setup_existing(GameWorld *g, struct descriptor_data *d, int real_num)
{
  OLC_ACTION(d)                      = new social_messg();
  OLC_ACTION(d)->command             = strdup(g->soc_mess_list[real_num].command);
  OLC_ACTION(d)->sort_as             = strdup(g->soc_mess_list[real_num].sort_as);
  OLC_ACTION(d)->hide                = g->soc_mess_list[real_num].hide;
  OLC_ACTION(d)->min_victim_position = g->soc_mess_list[real_num].min_victim_position;
  OLC_ACTION(d)->min_char_position   = g->soc_mess_list[real_num].min_char_position;
  OLC_ACTION(d)->min_level_char      = g->soc_mess_list[real_num].min_level_char;
  if (g->soc_mess_list[real_num].char_no_arg)
    OLC_ACTION(d)->char_no_arg = strdup(g->soc_mess_list[real_num].char_no_arg);
  if (g->soc_mess_list[real_num].others_no_arg)
    OLC_ACTION(d)->others_no_arg = strdup(g->soc_mess_list[real_num].others_no_arg);
  if (g->soc_mess_list[real_num].char_found)
    OLC_ACTION(d)->char_found = strdup(g->soc_mess_list[real_num].char_found);
  if (g->soc_mess_list[real_num].others_found)
    OLC_ACTION(d)->others_found = strdup(g->soc_mess_list[real_num].others_found);
  if (g->soc_mess_list[real_num].vict_found)
    OLC_ACTION(d)->vict_found = strdup(g->soc_mess_list[real_num].vict_found);
  if (g->soc_mess_list[real_num].not_found)
    OLC_ACTION(d)->not_found = strdup(g->soc_mess_list[real_num].not_found);
  if (g->soc_mess_list[real_num].char_auto)
    OLC_ACTION(d)->char_auto = strdup(g->soc_mess_list[real_num].char_auto);
  if (g->soc_mess_list[real_num].others_auto)
    OLC_ACTION(d)->others_auto = strdup(g->soc_mess_list[real_num].others_auto);
  if (g->soc_mess_list[real_num].char_body_found)
    OLC_ACTION(d)->char_body_found = strdup(g->soc_mess_list[real_num].char_body_found);
  if (g->soc_mess_list[real_num].others_body_found)
    OLC_ACTION(d)->others_body_found = strdup(g->soc_mess_list[real_num].others_body_found);
  if (g->soc_mess_list[real_num].vict_body_found)
    OLC_ACTION(d)->vict_body_found = strdup(g->soc_mess_list[real_num].vict_body_found);
  if (g->soc_mess_list[real_num].char_obj_found)
    OLC_ACTION(d)->char_obj_found = strdup(g->soc_mess_list[real_num].char_obj_found);
  if (g->soc_mess_list[real_num].others_obj_found)
    OLC_ACTION(d)->others_obj_found = strdup(g->soc_mess_list[real_num].others_obj_found);
  OLC_VAL(d) = 0;
  aedit_disp_menu(g, d);
}

static void aedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  struct social_messg *new_soc_mess_list = nullptr;
  int i;

  /* add a new social into the list */
  if (OLC_INDEX(d) > g->top_of_socialt) {
    new_soc_mess_list = new social_messg[g->top_of_socialt + 2]();
    for (i = 0; i <= g->top_of_socialt; i++)
      new_soc_mess_list[i] = g->soc_mess_list[i];
    new_soc_mess_list[++g->top_of_socialt] = *OLC_ACTION(d);
    delete[] g->soc_mess_list;
    g->soc_mess_list = new_soc_mess_list;
  }
  /* pass the editted action back to the list - no need to add */
  else {
    i                     = aedit_find_command(g, OLC_ACTION(d)->command);
    OLC_ACTION(d)->act_nr = g->soc_mess_list[OLC_INDEX(d)].act_nr;
    /* why did i do this..? hrm */
    free_action(g->soc_mess_list + OLC_INDEX(d));
    g->soc_mess_list[OLC_INDEX(d)] = *OLC_ACTION(d);
  }

  create_command_list(g);
  sort_commands(g);

  add_to_save_list(g, AEDIT_PERMISSION, SL_ACT);
  aedit_save_to_disk(g, d); /* autosave by Rumble */
}

/*------------------------------------------------------------------------*/

static void aedit_save_to_disk(GameWorld *g, struct descriptor_data *d)
{
  struct atomic_file af;
  FILE *fp;
  int i;
  if (!(fp = atomic_open(&af, SOCMESS_FILE_NEW, false))) {
    std::string error = str_format("Can't open socials file '{}'", SOCMESS_FILE);
    perror(error.c_str());
    exit(1);
  }

  for (i = 0; i <= g->top_of_socialt; i++) {
    fprintf(fp, "~%s %s %d %d %d %d\n",
            g->soc_mess_list[i].command,
            g->soc_mess_list[i].sort_as,
            g->soc_mess_list[i].hide,
            g->soc_mess_list[i].min_char_position,
            g->soc_mess_list[i].min_victim_position,
            g->soc_mess_list[i].min_level_char);
    fprintf(fp, "%s\n%s\n%s\n%s\n",
            ((g->soc_mess_list[i].char_no_arg) ? g->soc_mess_list[i].char_no_arg : "#"),
            ((g->soc_mess_list[i].others_no_arg) ? g->soc_mess_list[i].others_no_arg : "#"),
            ((g->soc_mess_list[i].char_found) ? g->soc_mess_list[i].char_found : "#"),
            ((g->soc_mess_list[i].others_found) ? g->soc_mess_list[i].others_found : "#"));
    fprintf(fp, "%s\n%s\n%s\n%s\n",
            ((g->soc_mess_list[i].vict_found) ? g->soc_mess_list[i].vict_found : "#"),
            ((g->soc_mess_list[i].not_found) ? g->soc_mess_list[i].not_found : "#"),
            ((g->soc_mess_list[i].char_auto) ? g->soc_mess_list[i].char_auto : "#"),
            ((g->soc_mess_list[i].others_auto) ? g->soc_mess_list[i].others_auto : "#"));
    fprintf(fp, "%s\n%s\n%s\n",
            ((g->soc_mess_list[i].char_body_found) ? g->soc_mess_list[i].char_body_found : "#"),
            ((g->soc_mess_list[i].others_body_found) ? g->soc_mess_list[i].others_body_found : "#"),
            ((g->soc_mess_list[i].vict_body_found) ? g->soc_mess_list[i].vict_body_found : "#"));
    fprintf(fp, "%s\n%s\n\n",
            ((g->soc_mess_list[i].char_obj_found) ? g->soc_mess_list[i].char_obj_found : "#"),
            ((g->soc_mess_list[i].others_obj_found) ? g->soc_mess_list[i].others_obj_found : "#"));
  }

  fprintf(fp, "$\n");
  if (atomic_commit(&af)) /* otherwise still listed as unsaved */
    remove_from_save_list(g, AEDIT_PERMISSION, SL_ACT);
}

/*------------------------------------------------------------------------*/

/* Menu functions */

/* the main menu */
static void aedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  struct social_messg *action = OLC_ACTION(d);

  write_to_output(g, d,
                  "@n-- Action editor\r\n@gn@n) Command         : @y{:<15.15}@n @g1@n) Sort as Command  : @y{:<15.15}@n\r\n@g2@n) Min Position[CH]: @c{:<8.8}        @g3@n) Min Position [VT]: @c{:<8.8}\r\n@g4@n) Min Level   [CH]: @c{:<3d}             @g5@n) Show if Invisible: @c{}\r\n@ga@n) Char    [NO ARG]: @c{}\r\n@gb@n) Others  [NO ARG]: @c{}\r\n@gc@n) Char [NOT FOUND]: @c{}\r\n@gd@n) Char  [ARG SELF]: @c{}\r\n@ge@n) Others[ARG SELF]: @c{}\r\n@gf@n) Char      [VICT]: @c{}\r\n@gg@n) Others    [VICT]: @c{}\r\n@gh@n) Victim    [VICT]: @c{}\r\n@gi@n) Char  [BODY PRT]: @c{}\r\n@gj@n) Others[BODY PRT]: @c{}\r\n@gk@n) Victim[BODY PRT]: @c{}\r\n@gl@n) Char       [OBJ]: @c{}\r\n@gm@n) Others     [OBJ]: @c{}\r\n@gq@n) Quit\r\nEnter Choice:",
                  action->command,
                  action->sort_as,
                  position_types[action->min_char_position],
                  position_types[action->min_victim_position],
                  action->min_level_char,
                  (action->hide ? "HIDDEN" : "NOT HIDDEN"),
                  action->char_no_arg ? action->char_no_arg : "<Null>",
                  action->others_no_arg ? action->others_no_arg : "<Null>",
                  action->not_found ? action->not_found : "<Null>",
                  action->char_auto ? action->char_auto : "<Null>",
                  action->others_auto ? action->others_auto : "<Null>",
                  action->char_found ? action->char_found : "<Null>",
                  action->others_found ? action->others_found : "<Null>",
                  action->vict_found ? action->vict_found : "<Null>",
                  action->char_body_found ? action->char_body_found : "<Null>",
                  action->others_body_found ? action->others_body_found : "<Null>",
                  action->vict_body_found ? action->vict_body_found : "<Null>",
                  action->char_obj_found ? action->char_obj_found : "<Null>",
                  action->others_obj_found ? action->others_obj_found : "<Null>");

  OLC_MODE(d) = AEDIT_MAIN_MENU;
}

/*
 * The main loop
 */

void aedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int i;

  switch (OLC_MODE(d)) {
    case AEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          aedit_save_internally(g, d);
          mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "OLC: {} edits action {}",
                 char_get_name(d->character), OLC_ACTION(d)->command);

          /* do not free the strings.. just the structure */
          cleanup_olc(g, d, CLEANUP_STRUCTS);
          write_to_output(g, d, "Action saved to disk.\r\n");
          break;
        case 'n':
        case 'N':
          /* free everything up, including strings etc */
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes? : ");
          break;
      }
      return; /* end of AEDIT_CONFIRM_SAVESTRING */

    case AEDIT_CONFIRM_EDIT:
      switch (*arg) {
        case 'y':
        case 'Y':
          aedit_setup_existing(g, d, OLC_INDEX(d));
          break;
        case 'q':
        case 'Q':
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        case 'n':
        case 'N':
          OLC_INDEX(d)
          ++;
          for (; (OLC_INDEX(d) <= g->top_of_socialt); OLC_INDEX(d)++)
            if (is_abbrev(OLC_STORAGE(d).c_str(), g->soc_mess_list[OLC_INDEX(d)].command))
              break;

          if (OLC_INDEX(d) > g->top_of_socialt) {
            if (aedit_find_command(g, OLC_STORAGE(d).c_str()) != -1) {
              cleanup_olc(g, d, CLEANUP_ALL);
              break;
            }
            write_to_output(g, d, "Do you wish to add the '{}' action? ",
                            OLC_STORAGE(d));
            OLC_MODE(d) = AEDIT_CONFIRM_ADD;
          } else {
            write_to_output(g, d, "Do you wish to edit the '{}' action? ",
                            g->soc_mess_list[OLC_INDEX(d)].command);
            OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
          }
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to edit the '{}' action? ",
                          g->soc_mess_list[OLC_INDEX(d)].command);
          break;
      }
      return;

    case AEDIT_CONFIRM_ADD:
      switch (*arg) {
        case 'y':
        case 'Y':
          aedit_setup_new(g, d);
          break;
        case 'n':
        case 'N':
        case 'q':
        case 'Q':
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to add the '{}' action? ",
                          OLC_STORAGE(d));
          break;
      }
      return;

    case AEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /* Something was modified */
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = AEDIT_CONFIRM_SAVESTRING;
          } else cleanup_olc(g, d, CLEANUP_ALL);
          break;
        case 'n':
          write_to_output(g, d, "Enter action name: ");
          OLC_MODE(d) = AEDIT_ACTION_NAME;
          return;
        case '1':
          write_to_output(g, d, "Enter sort info for this action (for the command listing): ");
          OLC_MODE(d) = AEDIT_SORT_AS;
          return;
        case '2':
          write_to_output(g, d, "Enter the minimum position the Character has to be in to activate social:\r\n");
          {
            for (i = POS_DEAD; i <= POS_STANDING; i++)
              write_to_output(g, d, "   {:d}) {}\r\n", i, position_types[i]);

            write_to_output(g, d, "Enter choice: ");
          }
          OLC_MODE(d) = AEDIT_MIN_CHAR_POS;
          return;
        case '3':
          write_to_output(g, d, "Enter the minimum position the Victim has to be in to activate social:\r\n");
          {
            for (i = POS_DEAD; i <= POS_STANDING; i++)
              write_to_output(g, d, "   {:d}) {}\r\n", i, position_types[i]);

            write_to_output(g, d, "Enter choice: ");
          }
          OLC_MODE(d) = AEDIT_MIN_VICT_POS;
          return;
        case '4':
          write_to_output(g, d, "Enter new minimum level for social: ");
          OLC_MODE(d) = AEDIT_MIN_CHAR_LEVEL;
          return;
        case '5':
          OLC_ACTION(d)->hide = !OLC_ACTION(d)->hide;
          aedit_disp_menu(g, d);
          OLC_VAL(d) = 1;
          break;
        case 'a':
        case 'A':
          write_to_output(g, d, "Enter social shown to the Character when there is no argument supplied.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->char_no_arg) ? OLC_ACTION(d)->char_no_arg : "NULL"));
          OLC_MODE(d) = AEDIT_NOVICT_CHAR;
          return;
        case 'b':
        case 'B':
          write_to_output(g, d, "Enter social shown to Others when there is no argument supplied.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->others_no_arg) ? OLC_ACTION(d)->others_no_arg : "NULL"));
          OLC_MODE(d) = AEDIT_NOVICT_OTHERS;
          return;
        case 'c':
        case 'C':
          write_to_output(g, d, "Enter text shown to the Character when his victim isnt found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->not_found) ? OLC_ACTION(d)->not_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_NOT_FOUND;
          return;
        case 'd':
        case 'D':
          write_to_output(g, d, "Enter social shown to the Character when it is its own victim.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->char_auto) ? OLC_ACTION(d)->char_auto : "NULL"));

          OLC_MODE(d) = AEDIT_SELF_CHAR;
          return;
        case 'e':
        case 'E':
          write_to_output(g, d, "Enter social shown to Others when the Char is its own victim.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->others_auto) ? OLC_ACTION(d)->others_auto : "NULL"));

          OLC_MODE(d) = AEDIT_SELF_OTHERS;
          return;
        case 'f':
        case 'F':
          write_to_output(g, d, "Enter normal social shown to the Character when the victim is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->char_found) ? OLC_ACTION(d)->char_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_CHAR_FOUND;
          return;
        case 'g':
        case 'G':
          write_to_output(g, d, "Enter normal social shown to Others when the victim is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->others_found) ? OLC_ACTION(d)->others_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_OTHERS_FOUND;
          return;
        case 'h':
        case 'H':
          write_to_output(g, d, "Enter normal social shown to the Victim when the victim is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->vict_found) ? OLC_ACTION(d)->vict_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_VICT_FOUND;
          return;
        case 'i':
        case 'I':
          write_to_output(g, d, "Enter 'body part' social shown to the Character when the victim is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->char_body_found) ? OLC_ACTION(d)->char_body_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_CHAR_BODY_FOUND;
          return;
        case 'j':
        case 'J':
          write_to_output(g, d, "Enter 'body part' social shown to Others when the victim is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->others_body_found) ? OLC_ACTION(d)->others_body_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_OTHERS_BODY_FOUND;
          return;
        case 'k':
        case 'K':
          write_to_output(g, d, "Enter 'body part' social shown to the Victim when the victim is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->vict_body_found) ? OLC_ACTION(d)->vict_body_found : "NULL"));

          OLC_MODE(d) = AEDIT_VICT_VICT_BODY_FOUND;
          return;
        case 'l':
        case 'L':
          write_to_output(g, d, "Enter 'object' social shown to the Character when the object is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->char_obj_found) ? OLC_ACTION(d)->char_obj_found : "NULL"));

          OLC_MODE(d) = AEDIT_OBJ_CHAR_FOUND;
          return;
        case 'm':
        case 'M':
          write_to_output(g, d, "Enter 'object' social shown to the Room when the object is found.\r\n[OLD]: {}\r\n[NEW]: ",
                          ((OLC_ACTION(d)->others_obj_found) ? OLC_ACTION(d)->others_obj_found : "NULL"));

          OLC_MODE(d) = AEDIT_OBJ_OTHERS_FOUND;
          return;
        default:
          aedit_disp_menu(g, d);
          break;
      }
      return;

    case AEDIT_ACTION_NAME:
      if (!*arg || strchr(arg, ' ')) {
        aedit_disp_menu(g, d);
        return;
      }
      if (OLC_ACTION(d)->command)
        free(OLC_ACTION(d)->command);
      OLC_ACTION(d)->command = strdup(arg);

      break;

    case AEDIT_SORT_AS:
      if (!*arg || strchr(arg, ' ')) {
        aedit_disp_menu(g, d);
        return;
      }
      if (OLC_ACTION(d)->sort_as) {
        free(OLC_ACTION(d)->sort_as);
        OLC_ACTION(d)->sort_as = strdup(arg);
      }
      break;

    case AEDIT_MIN_CHAR_POS:
    case AEDIT_MIN_VICT_POS:
      if (!*arg) {
        aedit_disp_menu(g, d);
        return;
      }
      i = atoi(arg);
      if ((i < POS_DEAD) || (i > POS_STANDING)) {
        aedit_disp_menu(g, d);
        return;
      }
      if (OLC_MODE(d) == AEDIT_MIN_CHAR_POS)
        OLC_ACTION(d)->min_char_position = i;
      else
        OLC_ACTION(d)->min_victim_position = i;
      break;

    case AEDIT_MIN_CHAR_LEVEL:
      if (!*arg) {
        aedit_disp_menu(g, d);
        return;
      }
      i = atoi(arg);
      if (i < 0) {
        aedit_disp_menu(g, d);
        return;
      }
      OLC_ACTION(d)->min_level_char = i;
      break;

    case AEDIT_NOVICT_CHAR:
      if (OLC_ACTION(d)->char_no_arg)
        free(OLC_ACTION(d)->char_no_arg);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_no_arg = strdup(arg);
      } else
        OLC_ACTION(d)->char_no_arg = nullptr;
      break;

    case AEDIT_NOVICT_OTHERS:
      if (OLC_ACTION(d)->others_no_arg)
        free(OLC_ACTION(d)->others_no_arg);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_no_arg = strdup(arg);
      } else
        OLC_ACTION(d)->others_no_arg = nullptr;
      break;

    case AEDIT_VICT_CHAR_FOUND:
      if (OLC_ACTION(d)->char_found)
        free(OLC_ACTION(d)->char_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_found = strdup(arg);
      } else
        OLC_ACTION(d)->char_found = nullptr;
      break;

    case AEDIT_VICT_OTHERS_FOUND:
      if (OLC_ACTION(d)->others_found)
        free(OLC_ACTION(d)->others_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_found = strdup(arg);
      } else
        OLC_ACTION(d)->others_found = nullptr;
      break;

    case AEDIT_VICT_VICT_FOUND:
      if (OLC_ACTION(d)->vict_found)
        free(OLC_ACTION(d)->vict_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->vict_found = strdup(arg);
      } else
        OLC_ACTION(d)->vict_found = nullptr;
      break;

    case AEDIT_VICT_NOT_FOUND:
      if (OLC_ACTION(d)->not_found)
        free(OLC_ACTION(d)->not_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->not_found = strdup(arg);
      } else
        OLC_ACTION(d)->not_found = nullptr;
      break;

    case AEDIT_SELF_CHAR:
      if (OLC_ACTION(d)->char_auto)
        free(OLC_ACTION(d)->char_auto);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_auto = strdup(arg);
      } else
        OLC_ACTION(d)->char_auto = nullptr;
      break;

    case AEDIT_SELF_OTHERS:
      if (OLC_ACTION(d)->others_auto)
        free(OLC_ACTION(d)->others_auto);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_auto = strdup(arg);
      } else
        OLC_ACTION(d)->others_auto = nullptr;
      break;

    case AEDIT_VICT_CHAR_BODY_FOUND:
      if (OLC_ACTION(d)->char_body_found)
        free(OLC_ACTION(d)->char_body_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_body_found = strdup(arg);
      } else
        OLC_ACTION(d)->char_body_found = nullptr;
      break;

    case AEDIT_VICT_OTHERS_BODY_FOUND:
      if (OLC_ACTION(d)->others_body_found)
        free(OLC_ACTION(d)->others_body_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_body_found = strdup(arg);
      } else
        OLC_ACTION(d)->others_body_found = nullptr;
      break;

    case AEDIT_VICT_VICT_BODY_FOUND:
      if (OLC_ACTION(d)->vict_body_found)
        free(OLC_ACTION(d)->vict_body_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->vict_body_found = strdup(arg);
      } else
        OLC_ACTION(d)->vict_body_found = nullptr;
      break;

    case AEDIT_OBJ_CHAR_FOUND:
      if (OLC_ACTION(d)->char_obj_found)
        free(OLC_ACTION(d)->char_obj_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->char_obj_found = strdup(arg);
      } else
        OLC_ACTION(d)->char_obj_found = nullptr;
      break;

    case AEDIT_OBJ_OTHERS_FOUND:
      if (OLC_ACTION(d)->others_obj_found)
        free(OLC_ACTION(d)->others_obj_found);
      if (*arg) {
        delete_doubledollar(arg);
        OLC_ACTION(d)->others_obj_found = strdup(arg);
      } else
        OLC_ACTION(d)->others_obj_found = nullptr;
      break;

    default:
      /* we should never get here */
      break;
  }
  OLC_VAL(d) = 1;
  aedit_disp_menu(g, d);
}

ACMD(do_astat)
{
  int i, real = false;
  std::string arg;

  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Astat which social?\r\n");
    return;
  }

  for (i = 0; i <= g->top_of_socialt; i++) {
    if (is_abbrev(arg, g->soc_mess_list[i].command)) {
      real = true;
      break;
    }
  }

  if (!real) {
    send_to_char(g, ch, "No such social.\r\n");
    return;
  }

  send_to_char(g, ch,
               "n) Command         : @y{:<15.15}@n 1) Sort as Command : @y{:<15.15}@n\r\n2) Min Position[CH]: @c{:<8.8}@n        3) Min Position[VT]: @c{:<8.8}@n\r\n4) Min Level   [CH]: @c{:<3d}@n             5) Show if Invis   : @c{}@n\r\na) Char    [NO ARG]: @c{}@n\r\nb) Others  [NO ARG]: @c{}@n\r\nc) Char [NOT FOUND]: @c{}@n\r\nd) Char  [ARG SELF]: @c{}@n\r\ne) Others[ARG SELF]: @c{}@n\r\nf) Char      [VICT]: @c{}@n\r\ng) Others    [VICT]: @c{}@n\r\nh) Victim    [VICT]: @c{}@n\r\ni) Char  [BODY PRT]: @c{}@n\r\nj) Others[BODY PRT]: @c{}@n\r\nk) Victim[BODY PRT]: @c{}@n\r\nl) Char       [OBJ]: @c{}@n\r\nm) Others     [OBJ]: @c{}@n\r\n",

               g->soc_mess_list[i].command,
               g->soc_mess_list[i].sort_as,
               position_types[g->soc_mess_list[i].min_char_position],
               position_types[g->soc_mess_list[i].min_victim_position],
               g->soc_mess_list[i].min_level_char,
               (g->soc_mess_list[i].hide ? "HIDDEN" : "NOT HIDDEN"),
               g->soc_mess_list[i].char_no_arg ? g->soc_mess_list[i].char_no_arg : "",
               g->soc_mess_list[i].others_no_arg ? g->soc_mess_list[i].others_no_arg : "",
               g->soc_mess_list[i].not_found ? g->soc_mess_list[i].not_found : "",
               g->soc_mess_list[i].char_auto ? g->soc_mess_list[i].char_auto : "",
               g->soc_mess_list[i].others_auto ? g->soc_mess_list[i].others_auto : "",
               g->soc_mess_list[i].char_found ? g->soc_mess_list[i].char_found : "",
               g->soc_mess_list[i].others_found ? g->soc_mess_list[i].others_found : "",
               g->soc_mess_list[i].vict_found ? g->soc_mess_list[i].vict_found : "",
               g->soc_mess_list[i].char_body_found ? g->soc_mess_list[i].char_body_found : "",
               g->soc_mess_list[i].others_body_found ? g->soc_mess_list[i].others_body_found : "",
               g->soc_mess_list[i].vict_body_found ? g->soc_mess_list[i].vict_body_found : "",
               g->soc_mess_list[i].char_obj_found ? g->soc_mess_list[i].char_obj_found : "",
               g->soc_mess_list[i].others_obj_found ? g->soc_mess_list[i].others_obj_found : "");
}

static int aedit_find_command(GameWorld *g, const char *txt)
{
  int cmd;

  for (cmd = 1; *g->complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(g->complete_cmd_info[cmd].sort_as, txt, strlen(txt)) ||
        !strcmp(g->complete_cmd_info[cmd].command, txt))
      return (cmd);
  return (-1);
}
