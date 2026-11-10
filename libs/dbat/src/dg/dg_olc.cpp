/**************************************************************************
*  File: dg_olc.c                                                         *
*                                                                         *
*  Usage: this source file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
*  Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_  *
*                                                                         *
*  $Author: Chris Jacobsen/Mark A. Heilpern/egreen/Welcor $               *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/oasis.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/modify.hpp"
#include "dbat/genzon.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void trigedit_save(GameWorld *g, struct descriptor_data *d);
static void trigedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct index_data *rtrg_num);
static void trigedit_setup_new(struct descriptor_data *d);

/* local functions */
static void trigedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void trigedit_disp_types(GameWorld *g, struct descriptor_data *d);

/* ***********************************************************************
 * trigedit
 * ***********************************************************************/

ACMD(do_oasis_trigedit)
{
  IDXTYPE number;
  struct index_data *real_num;
  struct descriptor_data *d;

  /*
   * Parse any arguments.
   */
  argument = skip_spaces(argument);
  if (argument.empty() || !isdigit(argument[0])) {
    send_to_char(g, ch, "Specify a trigger VNUM to edit.\r\n");
    return;
  }

  number = parse_int(argument);

  /*
   * Check that it isn't already being edited.
   */
  for (descriptor_data *dd : each_desc(g)) {
    if (desc_get_connected(dd) == CON_TRIGEDIT) {
      if (dd->olc && OLC_NUM(dd) == number) {
        send_to_char(g, ch, "That trigger is currently being edited by %s.\r\n",
                     char_get_name(dd->character));
        return;
      }
    }
  }
  d = char_get_desc(ch);
  /*
   * Give descriptor an OLC structure.
   */
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
           "SYSERR: do_oasis_trigedit: Player already had olc structure.");
    delete d->olc;
  }
  d->olc = new oasis_olc_data();

  /*
   * Find the zone.
   */
  if ((OLC_ZNUM(d) = find_zone_by_thing(g, number)) == nullptr) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  /*
   * Everyone but IMPLs can only edit zones they have been assigned.
   */
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_cannot_edit(g, ch, OLC_ZNUM(d)->number);
    delete d->olc;
    d->olc = nullptr;
    return;
  }
  OLC_NUM(d) = number;

  /*
   *  If this is a new trigger, setup a new one,
   *  otherwise, setup the a copy of the existing trigger
   */
  if ((real_num = find_trigger(g, number)) == nullptr)
    trigedit_setup_new(d);
  else
    trigedit_setup_existing(g, d, real_num);
  int disp = 0;
  if (disp == 0) {
    trigedit_disp_menu(g, d);
    desc_set_connected(d, CON_TRIGEDIT);
    disp = 1;
  }

  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d [trigger](allowed zone %d)",
         char_get_name(ch), OLC_ZNUM(d)->number, char_get_olc_zone(g, ch));
}

/* called when a mob or object is being saved to disk, so its script can */
/* be saved */
void script_save_to_disk(FILE *fp, void *item, int type)
{
  struct trig_proto_list *t;

  if (type == MOB_TRIGGER)
    t = char_get_proto_script((struct char_data *)item);
  else if (type == OBJ_TRIGGER)
    t = obj_get_proto_script((struct obj_data *)item);
  else if (type == WLD_TRIGGER)
    t = room_get_proto_script((struct room_data *)item);
  else {
    mud_log("SYSERR: Invalid type passed to script_save_to_disk()");
    return;
  }

  while (t) {
    fprintf(fp, "T %d\n", t->vnum);
    t = t->next;
  }
}

static void trigedit_setup_new(struct descriptor_data *d)
{
  struct trig_data *trig;

  /*
   * Allocate a scratch trigger structure
   */
  trig = new trig_data();

  trig->index = nullptr;

  /*
   * Set up some defaults
   */
  trig->name         = "new trigger";
  trig->trigger_type = MTRIG_GREET;

  /* cmdlist will be a large char string until the trigger is saved */
  OLC_STORAGE(d) = "%echo% This trigger commandlist is not complete!\r\n";
  trig->narg = 100;

  OLC_TRIG(d) = trig;
  OLC_VAL(d)  = 0; /* Has changed flag. (It hasn't so far, we just made it.) */
}

static void trigedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct index_data *rtrg_num)
{
  struct trig_data *trig;
  struct cmdlist_element *c;
  /*
   * Allocate a scratch trigger structure
   */
  trig = new trig_data();

  trig_data_copy(trig, rtrg_num->proto);

  /* convert cmdlist to a string */
  OLC_STORAGE(d).clear();
  for (c = trig->cmdlist; c; c = c->next) {
    OLC_STORAGE(d) += c->cmd;
    OLC_STORAGE(d) += "\r\n";
  }
  /* now trig->cmdlist is something to pass to the text editor */
  /* it will be converted back to a real cmdlist_element list later */

  OLC_TRIG(d) = trig;
  OLC_VAL(d)  = 0; /* Has changed flag. (It hasn't so far, we just made it.) */

  /*trigedit_disp_menu(d);*/
}

static void trigedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  struct trig_data *trig = OLC_TRIG(d);
  const char *attach_type;
  char trgtypes[256];

  if (trig->attach_type == OBJ_TRIGGER) {
    attach_type = "Objects";
    sprintbit(trig->trigger_type, otrig_types, trgtypes, sizeof(trgtypes));
  } else if (trig->attach_type == WLD_TRIGGER) {
    attach_type = "Rooms";
    sprintbit(trig->trigger_type, wtrig_types, trgtypes, sizeof(trgtypes));
  } else {
    attach_type = "Mobiles";
    sprintbit(trig->trigger_type, trig_types, trgtypes, sizeof(trgtypes));
  }

  clear_screen(g, d);

  write_to_output(g, d,
                  "Trigger Editor [@c%d@n]\r\n\r\n"
                  "@g1@n) Name         : @y%s\r\n"
                  "@g2@n) Intended for : @y%s\r\n"
                  "@g3@n) Trigger types: @y%s\r\n"
                  "@g4@n) Numeric Arg  : @y%d\r\n"
                  "@g5@n) Arguments    : @y%s\r\n"
                  "@g6@n) Commands:\r\n@c%s\r\n"
                  "@gW@n) Copy Trigger\r\n"
                  "@gZ@n) Wiznet\r\n"
                  "@gQ@n) Quit\r\n"
                  "Enter Choice :",

                  OLC_NUM(d),              /* vnum on the title line */
                  trig->name.c_str(),      /* name                   */
                  attach_type,             /* attach type            */
                  trgtypes,                /* greet/drop/etc         */
                  trig->narg,              /* numeric arg            */
                  trig->arglist.c_str(),   /* strict arg             */
                  OLC_STORAGE(d).c_str()); /* the command list       */

  OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}

static void trigedit_disp_types(GameWorld *g, struct descriptor_data *d)
{
  int i, columns = 0;
  const char *const *types;
  char bitbuf[MAX_STRING_LENGTH];

  switch (OLC_TRIG(d)->attach_type) {
    case WLD_TRIGGER:
      types = wtrig_types;
      break;
    case OBJ_TRIGGER:
      types = otrig_types;
      break;
    case MOB_TRIGGER:
    default:
      types = trig_types;
      break;
  }

  clear_screen(g, d);

  for (i = 0; i < NUM_TRIG_TYPE_FLAGS; i++) {
    write_to_output(g, d, "@g%2d@n) %-20.20s  %s", i + 1, types[i],
                    !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(OLC_TRIG(d)->trigger_type, types, bitbuf, sizeof(bitbuf));
  write_to_output(g, d, "\r\nCurrent types : @c%s@n\r\nEnter type (0 to quit) : ", bitbuf);
}

void trigedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int i = 0;
  struct index_data *rnum;

  switch (OLC_MODE(d)) {
    case TRIGEDIT_MAIN_MENU:
      switch (tolower(*arg)) {
        case 'q':
          if (OLC_VAL(d)) { /* Anything been changed? */
            if (!OLC_TRIG(d)->trigger_type) {
              write_to_output(g, d, "Invalid Trigger Type! Answer a to abort quit!\r\n");
            }
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case '1':
          OLC_MODE(d) = TRIGEDIT_NAME;
          write_to_output(g, d, "Name: ");
          break;
        case '2':
          OLC_MODE(d) = TRIGEDIT_INTENDED;
          write_to_output(g, d, "0: Mobiles, 1: Objects, 2: Rooms: ");
          break;
        case '3':
          OLC_MODE(d) = TRIGEDIT_TYPES;
          trigedit_disp_types(g, d);
          break;
        case '4':
          OLC_MODE(d) = TRIGEDIT_NARG;
          write_to_output(g, d, "Numeric argument: ");
          break;
        case '5':
          OLC_MODE(d) = TRIGEDIT_ARGUMENT;
          write_to_output(g, d, "Argument: ");
          break;
        case '6':
          OLC_MODE(d) = TRIGEDIT_COMMANDS;
          write_to_output(g, d, "Enter trigger commands: (/s saves /h for help)\r\n\r\n");
          write_to_output(g, d, "%s", OLC_STORAGE(d).c_str());
          string_edit_begin(d, &OLC_STORAGE(d), MAX_CMD_LENGTH);
          d->mail_to = 0;
          OLC_VAL(d) = 1;

          break;
        case 'w':
        case 'W':
          write_to_output(g, d, "Copy what trigger? ");
          OLC_MODE(d) = TRIGEDIT_COPY;
          break;
        case 'Z':
        case 'z':
          search_replace(arg, "z ", "");
          do_wiznet(g, d->character, arg, 0, 0);
          break;
        default:
          trigedit_disp_menu(g, d);
          return;
      }
      return;

    case TRIGEDIT_CONFIRM_SAVESTRING:
      switch (tolower(*arg)) {
        case 'y':
          trigedit_save(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                 "OLC: %s edits trigger %d", char_get_name(d->character),
                 OLC_NUM(d));
          [[fallthrough]];
        case 'n':
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case 'a': /* abort quitting */
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\n");
          write_to_output(g, d, "Do you wish to save the trigger? : ");
          return;
      }
      break;

    case TRIGEDIT_NAME:
      smash_tilde(arg);
      OLC_TRIG(d)->name = (arg && *arg) ? arg : "undefined";
      OLC_VAL(d)
      ++;
      break;

    case TRIGEDIT_INTENDED:
      if ((atoi(arg) >= MOB_TRIGGER) || (atoi(arg) <= WLD_TRIGGER))
        OLC_TRIG(d)->attach_type = atoi(arg);
      OLC_VAL(d)
      ++;
      break;

    case TRIGEDIT_NARG:
      OLC_TRIG(d)->narg = LIMIT(atoi(arg), 0, 100);
      OLC_VAL(d)
      ++;
      break;

    case TRIGEDIT_ARGUMENT:
      smash_tilde(arg);
      OLC_TRIG(d)->arglist = arg;
      OLC_VAL(d)
      ++;
      break;

    case TRIGEDIT_TYPES:
      if ((i = atoi(arg)) == 0)
        break;
      else if (!((i < 0) || (i > NUM_TRIG_TYPE_FLAGS)))
        TOGGLE_BIT((OLC_TRIG(d)->trigger_type), 1U << (i - 1));
      OLC_VAL(d)
      ++;
      trigedit_disp_types(g, d);
      return;

    case TRIGEDIT_COMMANDS:
      break;

    case TRIGEDIT_COPY:
      if ((rnum = find_trigger(g, atoi(arg))) != nullptr) {
        trigedit_setup_existing(g, d, rnum);
      } else
        write_to_output(g, d, "That trigger does not exist.\r\n");
      break;
  }

  OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
  trigedit_disp_menu(g, d);
}

/* Builds trig->cmdlist from the editor text s, one element per line. */
/* Makes the trigger's command list from the script, a line to a command. */
static void trigedit_compile_script(trig_data *trig, std::string script)
{
  struct cmdlist_element *cmd;
  char *s;

  trig->cmdlist = new cmdlist_element();
  /* strtok returns NULL if the script is "\r\n" */
  char *t            = strtok(script.data(), "\n\r");
  trig->cmdlist->cmd = t ? t : "* No script";
  cmd                = trig->cmdlist;

  while ((s = strtok(nullptr, "\n\r"))) {
    cmd->next = new cmdlist_element();
    cmd       = cmd->next;
    cmd->cmd  = s;
  }
}

/* Replaces the trigger prototype with the edited trigger, and updates every live copy of it. */
static void trigedit_replace_existing(GameWorld *g, struct descriptor_data *d, struct index_data *rnum)
{
  trig_data *trig  = OLC_TRIG(d);
  trig_data *proto = rnum->proto;
  struct cmdlist_element *cmd, *next_cmd;

  for (cmd = proto->cmdlist; cmd; cmd = next_cmd) {
    next_cmd = cmd->next;
    delete cmd;
  }

  /* Recompile the command list from the new script */
  trigedit_compile_script(trig, OLC_STORAGE(d));

  /* make the prorotype look like what we have */
  trig_data_copy(proto, trig);

  /* go through the mud and replace existing triggers         */
  g->triggers.visit([&](long, struct trig_data *live_trig) {
    if (live_trig->index == rnum) {
      live_trig->arglist = proto->arglist;
      live_trig->name    = proto->name;

      /* anything could have happened so we don't want to keep these */
      if (live_trig->wait_event) {
        event_cancel(g, live_trig->wait_event);
        live_trig->wait_event = nullptr;
      }
      if (live_trig->var_list) {
        free_varlist(live_trig->var_list);
        live_trig->var_list = nullptr;
      }

      live_trig->cmdlist      = proto->cmdlist;
      live_trig->curr_state   = live_trig->cmdlist;
      live_trig->trigger_type = proto->trigger_type;
      live_trig->attach_type  = proto->attach_type;
      live_trig->narg         = proto->narg;
      live_trig->data_type    = proto->data_type;
      live_trig->depth        = 0;
    }
  });
}

/* Adds the edited trigger to the trigger index. */
static void trigedit_insert_new(GameWorld *g, struct descriptor_data *d)
{
  trig_data *trig          = OLC_TRIG(d);
  struct index_data *index = g->trig_index.emplace(OLC_NUM(d));

  /* Recompile the command list from the new script */
  trigedit_compile_script(trig, OLC_STORAGE(d));

  trig->index  = index;
  index->vnum  = OLC_NUM(d);
  index->proto = new trig_data();
  trig_data_copy(index->proto, trig);
}

/* Writes every trigger of the zone to <zone>.trg. Returns false, having logged why, if it couldn't. */
bool save_triggers(GameWorld *g, struct zone_data *in_zone)
{
  IDXTYPE i;
  struct index_data *rnum;
  trig_data *trig;
  struct cmdlist_element *cmd;
  FILE *trig_file;
  zone_vnum zone;
  room_vnum top;
  char bitBuf[MAX_INPUT_LENGTH];
  struct atomic_file af;
  char fname[MAX_INPUT_LENGTH];

  zone = in_zone->number;
  top  = in_zone->top;

  snprintf(fname, sizeof(fname), "%s%d.trg", TRG_PREFIX, zone);
  if (!(trig_file = atomic_open(&af, fname, false))) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Can't open trig file \"%s\"", fname);
    return false;
  }

  for (i = in_zone->bot; i <= top; i++) {
    if ((rnum = find_trigger(g, i)) != nullptr) {
      trig = rnum->proto;

      if (fprintf(trig_file, "#%d\n", i) < 0) {
        mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Can't write trig file!");
        atomic_abort(&af);
        return false;
      }
      sprintascii(bitBuf, trig->trigger_type);
      fprintf(trig_file, "%s%c\n"
                         "%d %s %d\n"
                         "%s%c\n",
              !trig->name.empty() ? trig->name.c_str() : "unknown trigger", STRING_TERMINATOR,
              trig->attach_type,
              *bitBuf ? bitBuf : "0", trig->narg,
              trig->arglist.c_str(), STRING_TERMINATOR);

      /* The script, one line per command. Written directly: scripts can exceed any fixed buffer. */
      if (!trig->cmdlist)
        fputs("* Empty script", trig_file);
      for (cmd = trig->cmdlist; cmd; cmd = cmd->next)
        fprintf(trig_file, "%s\n", cmd->cmd.c_str());

      fprintf(trig_file, "%c\n", STRING_TERMINATOR);
    }
  }

  fprintf(trig_file, "$%c\n", STRING_TERMINATOR);
  if (!atomic_commit(&af))
    return false;
  create_world_index(g, zone, "trg");
  return true;
}

/*
 * Writes the editor's zone's triggers to disk.
 * note: we write this to disk NOW instead of letting the builder
 * have control because if we lose this after having assigned a
 * new trigger to an item, we will get SYSERR's upton reboot that
 * could make things hard to debug.
 */
static void trigedit_write_zone_file(GameWorld *g, struct descriptor_data *d)
{
  if (save_triggers(g, OLC_ZNUM(d)))
    write_to_output(g, d, "Trigger saved to disk.\r\n");
  else
    write_to_output(g, d, "Could not write the trigger file; nothing was changed.\r\n");
}

/* save the zone's triggers to internal memory and to disk */
static void trigedit_save(GameWorld *g, struct descriptor_data *d)
{
  struct index_data *rnum;

  if ((rnum = find_trigger(g, OLC_NUM(d))) != nullptr)
    trigedit_replace_existing(g, d, rnum);
  else
    trigedit_insert_new(g, d);

  dg_lint_trigger(g, find_trigger(g, OLC_NUM(d))->proto, d);
  trigedit_write_zone_file(g, d);
}

void dg_olc_script_copy(struct descriptor_data *d)
{
  struct trig_proto_list *origscript, *editscript;

  if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
    origscript = char_get_proto_script(OLC_MOB(d));
  else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
    origscript = obj_get_proto_script(OLC_OBJ(d));
  else origscript = room_get_proto_script(OLC_ROOM(d));

  if (origscript) {
    editscript    = new trig_proto_list();
    OLC_SCRIPT(d) = editscript;

    while (origscript) {
      editscript->vnum = origscript->vnum;
      origscript       = origscript->next;
      if (origscript)
        editscript->next = new trig_proto_list();
      editscript = editscript->next;
    }
  } else
    OLC_SCRIPT(d) = nullptr;
}

void dg_script_menu(GameWorld *g, struct descriptor_data *d)
{
  struct trig_proto_list *editscript;
  int i = 0;

  /* make sure our input parser gets used */
  OLC_MODE(d)             = OLC_SCRIPT_EDIT;
  OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

  clear_screen(g, d);
  write_to_output(g, d, "     Script Editor\r\n\r\n     Trigger List:\r\n");

  editscript = OLC_SCRIPT(d);

  while (editscript) {
    write_to_output(g, d, "     %2d) [@c%d@n] @c%s@n", ++i, editscript->vnum,
                    find_trigger(g, editscript->vnum)->proto->name.c_str());
    if (find_trigger(g, editscript->vnum)->proto->attach_type != OLC_ITEM_TYPE(d))
      write_to_output(g, d, "   @g** Mis-matched Trigger Type **@n\r\n");
    else
      write_to_output(g, d, "\r\n");

    editscript = editscript->next;
  }
  if (i == 0)
    write_to_output(g, d, "     <none>\r\n");

  write_to_output(g, d, "\r\n"
                     " @gN@n)  New trigger for this script\r\n"
                     " @gD@n)  Delete a trigger in this script\r\n"
                     " @gX@n)  Exit Script Editor\r\n\r\n"
                     "     Enter choice :");
}

bool dg_script_edit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  struct trig_proto_list *trig, *currtrig;
  int count, pos;
  trig_vnum vnum;

  switch (OLC_SCRIPT_EDIT_MODE(d)) {
    case SCRIPT_MAIN_MENU:
      switch (tolower(*arg)) {
        case 'x':
          /* this was buggy.
             First we created a copy of a thing, but maintained pointers to scripts,
             then if we altered the scripts, we freed the pointers and added new ones
             to the OLC_THING. If we then chose _NOT_ to save the changes, the
             pointers in the original thing pointed to garbage. If we saved changes
             the pointers were updated correctly.

             Solution:
             Here we just point the working copies to the new proto_scripts
             We only update the original when choosing to save internally,
             then free the unused memory there.

             Welcor

             Thanks to
             Jeremy Stanley - fungi@yuggoth.org and
             Torgny Bjers - artovil@arcanerealms.org
             for the bug report.

             After updating to OasisOLC 2.0.3 I discovered some malfunctions
             in this code, so I restructured it a bit. Now things work like this:
             OLC_SCRIPT(d) is assigned a copy of the edited things' proto_script.
             OLC_OBJ(d), etc.. are initalized with proto_script = NULL;
             On save, the saved copy is updated with OLC_SCRIPT(d) as new proto_script (freeing the old one).
             On quit/nosave, OLC_SCRIPT is free()'d, and the prototype not touched.

           */
          return false;
        case 'n':
          write_to_output(g, d, "\r\nPlease enter position, vnum   (ex: 1, 200):");
          OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
          break;
        case 'd':
          write_to_output(g, d, "     Which entry should be deleted?  0 to abort :");
          OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
          break;
        default:
          dg_script_menu(g, d);
          break;
      }
      return true;

    case SCRIPT_NEW_TRIGGER:
      vnum  = -1;
      pos   = 0; /* stays that way if arg isn't a number, which aborts the entry */
      count = sscanf(arg, "%d, %d", &pos, &vnum);
      if (count == 1) {
        vnum = pos;
        pos  = 999;
      }

      if (pos <= 0) break; /* this aborts a new trigger entry */

      if (vnum == 0) break; /* this aborts a new trigger entry */

      if (find_trigger(g, vnum) == nullptr) {
        write_to_output(g, d, "Invalid Trigger VNUM!\r\n"
                           "Please enter position, vnum   (ex: 1, 200):");
        return true;
      }

      /* add the new info in position */
      currtrig = OLC_SCRIPT(d);
      trig       = new trig_proto_list();
      trig->vnum = vnum;

      if (pos == 1 || !currtrig) {
        trig->next    = OLC_SCRIPT(d);
        OLC_SCRIPT(d) = trig;
      } else {
        while (currtrig->next && --pos) {
          currtrig = currtrig->next;
        }
        trig->next     = currtrig->next;
        currtrig->next = trig;
      }
      OLC_VAL(d)
      ++;
      break;

    case SCRIPT_DEL_TRIGGER:
      pos = atoi(arg);
      if (pos <= 0) break;

      if (pos == 1 && OLC_SCRIPT(d)) {
        OLC_VAL(d)
        ++;
        currtrig      = OLC_SCRIPT(d);
        OLC_SCRIPT(d) = currtrig->next;
        delete currtrig;
        break;
      }

      pos--;
      currtrig = OLC_SCRIPT(d);
      while (--pos && currtrig) currtrig = currtrig->next;
      /* now curtrig points one before the target */
      if (currtrig && currtrig->next) {
        OLC_VAL(d)
        ++;
        trig           = currtrig->next;
        currtrig->next = trig->next;
        delete trig;
      }
      break;
  }

  dg_script_menu(g, d);
  return true;
}

void trigedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    case TRIGEDIT_COMMANDS:
      trigedit_disp_menu(g, d);
      break;
  }
}

bool format_script(GameWorld *g, struct descriptor_data *d)
{
  char nsc[MAX_CMD_LENGTH], *t, line[READ_SIZE];
  char *sc;
  size_t len = 0, nlen = 0;
  int llen   = 0;
  int indent = 0, indent_next = false, found_case = false, i, line_num = 0;

  if (d->edit_text.empty())
    return false;

  sc   = strdup(d->edit_text.c_str()); /* we work on a copy, because of strtok() */
  t    = strtok(sc, "\n\r");
  *nsc = '\0';

  while (t) {
    line_num++;
    skip_spaces(&t);
    if (!strncasecmp(t, "if ", 3) ||
        !strncasecmp(t, "switch ", 7)) {
      indent_next = true;
    } else if (!strncasecmp(t, "while ", 6)) {
      found_case  = true; /* so you can 'break' a loop without complains */
      indent_next = true;
    } else if (!strncasecmp(t, "end", 3) ||
               !strncasecmp(t, "done", 4)) {
      if (!indent) {
        write_to_output(g, d, "Unmatched 'end' or 'done' (line %d)!\r\n", line_num);
        free(sc);
        return false;
      }
      indent--;
      indent_next = false;
    } else if (!strncasecmp(t, "else", 4)) {
      if (!indent) {
        write_to_output(g, d, "Unmatched 'else' (line %d)!\r\n", line_num);
        free(sc);
        return false;
      }
      indent--;
      indent_next = true;
    } else if (!strncasecmp(t, "case", 4) ||
               !strncasecmp(t, "default", 7)) {
      if (!indent) {
        write_to_output(g, d, "Case/default outside switch (line %d)!\r\n", line_num);
        free(sc);
        return false;
      }
      if (!found_case) /* so we don't indent multiple case statements without a break */
        indent_next = true;
      found_case = true;
    } else if (!strncasecmp(t, "break", 5)) {
      if (!found_case || !indent) {
        write_to_output(g, d, "Break not in case (line %d)!\r\n", line_num);
        free(sc);
        return false;
      }
      found_case = false;
      indent--;
    }

    *line = '\0';
    for (nlen = 0, i = 0; i < indent && nlen + 2 < (int)sizeof(line); i++) {
      strcat(line, "  ");
      nlen += 2;
    }
    llen = snprintf(line + nlen, sizeof(line) - nlen, "%s\r\n", t);
    if (llen < 0 || llen + nlen + len > d->max_str - 1) {
      write_to_output(g, d, "String too long, formatting aborted\r\n");
      free(sc);
      return false;
    }
    len = len + nlen + llen;
    strcat(nsc, line); /* strcat OK, size checked above */

    if (indent_next) {
      indent++;
      indent_next = false;
    }
    t = strtok(nullptr, "\n\r");
  }

  if (indent)
    write_to_output(g, d, "Unmatched if, while or switch ignored.\r\n");

  d->edit_text = nsc;
  free(sc);

  return true;
}
