/* ******************************************************************** *
 * FILE : assedit.c                     Copyright (C) 1999 Del Minturn  *
 * USAGE: Olc for assembly engine by Geoff Davis.                       *
 *        Oasis OLC by George Greer.
 * -------------------------------------------------------------------- *
 * 1999 July 25 caminturn@earthlink.net                                *
 * ******************************************************************** */

#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/assedit.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/oasis.hpp"
#include "dbat/game_world.hpp"

static void assedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void assedit_edit_extract(GameWorld *g, struct descriptor_data *d);
static void assedit_edit_inroom(GameWorld *g, struct descriptor_data *d);
static void assedit_setup(GameWorld *g, struct descriptor_data *d, obj_vnum number);
static void nodigit(GameWorld *g, struct descriptor_data *d);

/*-------------------------------------------------------------------*
 * Nasty internal macros to clean up the code.
 *-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*
 * Assedit command
 *-------------------------------------------------------------------*/

ACMD(do_assedit)
{
  struct descriptor_data *d = char_get_desc(ch);
  std::string buf;
  std::string buf2;

  if (char_is_npc(ch))
    return;

  for (descriptor_data *dd : each_desc(g)) {
    if (dd->connected == CON_ASSEDIT) {
      send_to_char_printf(g, ch, "Assemblies are already being editted by someone.\r\n");
      return;
    }
  }

  std::string_view rest = argument;
  buf                   = take_arg(rest);
  buf2                  = take_arg(rest);

  d = char_get_desc(ch);

  if (buf.empty()) {
    nodigit(g, d);
    return;
  }

  if (!isdigit(buf[0])) {
    if (strncasecmp("new", buf.c_str(), 3) == 0) {
      if (!isdigit(buf2[0])) {
        nodigit(g, d);
      } else if (!obj_proto_of(g, parse_int(buf2))) {
        send_to_char_printf(g, d->character, "You need to create the assembly object before you can create the new assembly.\r\n");
        return;
      } else {
        assemblyCreate(g, parse_int(buf2), 0);
        send_to_char_printf(g, d->character, "Assembly Created.\r\n");
        assemblySaveAssemblies(g);
        return;
      }
    } else if (strncasecmp("delete", buf.c_str(), 6) == 0) {
      if (!isdigit(buf2[0]))
        nodigit(g, d);
      else {
        assemblyDestroy(g, parse_int(buf2));
        send_to_char_printf(g, d->character, "Assembly Deleted.\r\n");
        assemblySaveAssemblies(g);
        return;
      }
    } else {
      nodigit(g, d);
      return;
    }
  } else if (isdigit(buf[0])) {
    d = char_get_desc(ch);
    d->olc = new oasis_olc_data();
    assedit_setup(g, d, parse_int(buf));
  }
  return;
}

/*-------------------------------------------------------------------*
 * Assedit Functions
 *-------------------------------------------------------------------*/

static void assedit_setup(GameWorld *g, struct descriptor_data *d, obj_vnum number)
{

  ASSEMBLY *pOldAssembly = nullptr;
  OLC_ASSEDIT(d)         = new ASSEMBLY();

  if ((pOldAssembly = assemblyGetAssemblyPtr(g, number)) == nullptr) {
    send_to_char_printf(g, d->character, "That assembly does not exist\r\n");
    cleanup_olc(g, d, CLEANUP_ALL);
    return;
  }
  /* Copy the old assembly. */
  OLC_ASSEDIT(d)->lVnum           = pOldAssembly->lVnum;
  OLC_ASSEDIT(d)->uchAssemblyType = pOldAssembly->uchAssemblyType;
  OLC_ASSEDIT(d)->lNumComponents  = pOldAssembly->lNumComponents;

  if (OLC_ASSEDIT(d)->lNumComponents > 0) {
    OLC_ASSEDIT(d)->pComponents = new COMPONENT[OLC_ASSEDIT(d)->lNumComponents]();
    memmove(OLC_ASSEDIT(d)->pComponents, pOldAssembly->pComponents,
            OLC_ASSEDIT(d)->lNumComponents * sizeof(COMPONENT));
  }

  /*
     * At this point, pNewAssembly is now the address of a freshly allocated copy of all
     * the data contained in the original assembly structure.
     */

  if (!obj_proto_of(g, g->lRnum = OLC_ASSEDIT(d)->lVnum)) {
    send_to_char_printf(g, d->character, "Assembled item may not exist, check the vnum and assembles (show assemblies). \r\n");
    cleanup_olc(g, d, CLEANUP_ALL); /* for right now we just get out! */
    return;
  }

  desc_set_connected(d, CON_ASSEDIT);
  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(d->character, PLR_WRITING, true);
  assedit_disp_menu(g, d);
}

static void assedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  int i                             = 0;
  char szAssmType[MAX_INPUT_LENGTH] = {'\0'};

  sprinttype(OLC_ASSEDIT(d)->uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType));

#if defined(CLEAR_SCREEN)
  send_to_char_printf(g, d->character, "%c[H%c[J", 27, 27);
#endif

  send_to_char_printf(g, d->character,
                      "Assembly Number: @c%d@n\r\n"
                      "Assembly Name  : @y%s@n\r\n"
                      "Assembly Type  : @y%s@n\r\n"
                      "Components:\r\n",
                      OLC_ASSEDIT(d)->lVnum,
                      obj_get_short_description(obj_proto_of(g, OLC_ASSEDIT(d)->lVnum)),
                      szAssmType);

  if (OLC_ASSEDIT(d)->lNumComponents <= 0)
    send_to_char_printf(g, d->character, "   < NONE > \r\n");
  else {
    for (i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
      if (!obj_proto_of(g, g->lRnum = OLC_ASSEDIT(d)->pComponents[i].lVnum)) {
        send_to_char_printf(g, d->character, "@g%2d@n) @y ERROR --- Contact an Implementor @n\r\n ", i + 1);
      } else {
        send_to_char_printf(g, d->character,
                            "@g%2d@n) [@c%5d@n] %-20.20s  In room: @c%-3.3s@n    Extract: @y%-3.3s@n\r\n",
                            i + 1, OLC_ASSEDIT(d)->pComponents[i].lVnum,
                            obj_get_short_description(obj_proto_of(g, g->lRnum)),
                            (OLC_ASSEDIT(d)->pComponents[i].bInRoom ? "Yes" : "No"),
                            (OLC_ASSEDIT(d)->pComponents[i].bExtract ? "Yes" : "No"));
      }
    }
  }
  send_to_char_printf(g, d->character,
                      "@gA@n) Add a new component.\r\n"
                      "@gE@n) Edit a component.\r\n"
                      "@gD@n) Delete a component.\r\n"
                      "@gT@n) Change Assembly Type.\r\n"
                      "@gQ@n) Quit.\r\n"
                      "\r\nEnter your choice : ");

  OLC_MODE(d) = ASSEDIT_MAIN_MENU;

  return;
}

/***************************************************
   Command Parse
 ***************************************************/

void assedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int pos = 0, i = 0, counter, columns = 0;

  COMPONENT *pTComponents = nullptr;

  switch (OLC_MODE(d)) {

    case ASSEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q': /* do the quit stuff */
          /* Ok, Time to save it back to the original stuff and get out */
          /* due to the infrequent use of this code and restricted use  */
          /* I decided to copy over changes regarless.                  */
          assemblyDestroy(g, OLC_ASSEDIT(d)->lVnum);
          assemblyCreate(g, OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->uchAssemblyType);
          for (i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
            assemblyAddComponent(g, OLC_ASSEDIT(d)->lVnum,
                                 OLC_ASSEDIT(d)->pComponents[i].lVnum,
                                 OLC_ASSEDIT(d)->pComponents[i].bExtract,
                                 OLC_ASSEDIT(d)->pComponents[i].bInRoom);
          }
          send_to_char_printf(g, d->character, "\r\nSaving all assemblies\r\n");
          assemblySaveAssemblies(g);

          /*       free(pTComponents);
       free(OLC_ASSEDIT(d));
*/
          cleanup_olc(g, d, CLEANUP_ALL); /* for right now we just get out! */
          break;

        case 't':
        case 'T':
#if defined(CLEAR_SCREEN)
          send_to_char_printf(g, d->character, "%c[H%c[J", 27, 27);
#endif
          for (counter = 0; counter < MAX_ASSM; counter++) {
            send_to_char_printf(g, d->character, "@g%2d@n) %-20.20s %s", counter + 1,
                                AssemblyTypes[counter], !(++columns % 2) ? "\r\n" : "");
          }
          send_to_char_printf(g, d->character, "Enter the assembly type : ");
          OLC_MODE(d) = ASSEDIT_EDIT_TYPES;

          break;
        case 'a':
        case 'A': /* add a new component */
          send_to_char_printf(g, d->character, "\r\nWhat is the vnum of the new component?");
          OLC_MODE(d) = ASSEDIT_ADD_COMPONENT;
          break;

        case 'e':
        case 'E': /* edit a component */
          send_to_char_printf(g, d->character, "\r\nEdit which component? ");
          OLC_MODE(d) = ASSEDIT_EDIT_COMPONENT;
          break;
        case 'd':
        case 'D': /* delete a component */
          if ((pos < 0) || pos > OLC_ASSEDIT(d)->lNumComponents) {
            send_to_char_printf(g, d->character, "\r\nWhich component do you wish to remove?");
            assedit_disp_menu(g, d);
          } else {
            send_to_char_printf(g, d->character, "\r\nWhich component do you wish to remove?");
            OLC_MODE(d) = ASSEDIT_DELETE_COMPONENT;
          }
          break;

        default:
          assedit_disp_menu(g, d);
      }
      break;

    case ASSEDIT_EDIT_TYPES:
      if (isdigit(*arg)) {
        pos = atoi(arg) - 1;
        if ((pos >= 0) && (pos < MAX_ASSM)) {
          OLC_ASSEDIT(d)->uchAssemblyType = pos;
          assedit_disp_menu(g, d);
          break;
        }
      } else
        assedit_disp_menu(g, d);

      break;
    case ASSEDIT_ADD_COMPONENT: /* add a new component */
      if (isdigit(*arg)) {
        pos = atoi(arg);
        if ((pos) <= NOTHING) /* does the object exist? */
          break;

        for (i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
          if (OLC_ASSEDIT(d)->pComponents[i].lVnum == pos)
            break;
        }

        pTComponents = new COMPONENT[OLC_ASSEDIT(d)->lNumComponents + 1]();

        if (OLC_ASSEDIT(d)->pComponents != nullptr) { /* Copy from olc to temp */
          memmove(pTComponents, OLC_ASSEDIT(d)->pComponents,
                  OLC_ASSEDIT(d)->lNumComponents * sizeof(COMPONENT));
          /*        free(OLC_ASSEDIT(d)->pComponents); */
        }

        OLC_ASSEDIT(d)->pComponents                                          = pTComponents;
        OLC_ASSEDIT(d)->pComponents[OLC_ASSEDIT(d)->lNumComponents].lVnum    = pos;
        OLC_ASSEDIT(d)->pComponents[OLC_ASSEDIT(d)->lNumComponents].bExtract = true;
        OLC_ASSEDIT(d)->pComponents[OLC_ASSEDIT(d)->lNumComponents].bInRoom  = false;
        OLC_ASSEDIT(d)->lNumComponents += 1;

        assedit_disp_menu(g, d);

      } else {
        send_to_char_printf(g, d->character, "That object does not exist. Please try again\r\n");
        assedit_disp_menu(g, d);
      }
      break;

    case ASSEDIT_EDIT_COMPONENT:
      pos = atoi(arg);
      if (isdigit(*arg) && pos >= 1 && pos <= OLC_ASSEDIT(d)->lNumComponents) {
        pos--;
        OLC_VAL(d) = pos;
        assedit_edit_extract(g, d);
        break;
      } else
        assedit_disp_menu(g, d);
      break;

    case ASSEDIT_DELETE_COMPONENT:

      if (isdigit(*arg) && atoi(arg) >= 1 && atoi(arg) <= OLC_ASSEDIT(d)->lNumComponents) {
        pos = atoi(arg);
        pos -= 1;

        pTComponents = new COMPONENT[OLC_ASSEDIT(d)->lNumComponents](); /* one spare, as there may be none left */

        if (pos > 0)
          memmove(pTComponents, OLC_ASSEDIT(d)->pComponents, pos * sizeof(COMPONENT));

        if (pos < OLC_ASSEDIT(d)->lNumComponents - 1)
          memmove(pTComponents + pos, OLC_ASSEDIT(d)->pComponents + pos + 1,
                  (OLC_ASSEDIT(d)->lNumComponents - pos - 1) * sizeof(COMPONENT));

        delete[] OLC_ASSEDIT(d)->pComponents;
        OLC_ASSEDIT(d)->pComponents = pTComponents;
        OLC_ASSEDIT(d)->lNumComponents -= 1;

        assedit_disp_menu(g, d);
        break;
      } else
        assedit_disp_menu(g, d);
      break;

    case ASSEDIT_EDIT_EXTRACT:
      switch (*arg) {
        case 'y':
        case 'Y':
          OLC_ASSEDIT(d)->pComponents[OLC_VAL(d)].bExtract = true;
          assedit_edit_inroom(g, d);
          break;

        case 'n':
        case 'N':
          OLC_ASSEDIT(d)->pComponents[OLC_VAL(d)].bExtract = false;
          assedit_edit_inroom(g, d);
          break;

        default:
          send_to_char_printf(g, d->character, "Is the item to be extracted when the assembly is created? (Y/N)");
          break;
      }
      break;
    case ASSEDIT_EDIT_INROOM:
      switch (*arg) {
        case 'y':
        case 'Y':
          OLC_ASSEDIT(d)->pComponents[OLC_VAL(d)].bInRoom = true;
          assedit_disp_menu(g, d);
          break;

        case 'n':
        case 'N':
          OLC_ASSEDIT(d)->pComponents[OLC_VAL(d)].bInRoom = false;
          assedit_disp_menu(g, d);
          break;

        default:
          send_to_char_printf(g, d->character, "Object in the room when assembly is created? (n =  in inventory):");
          break;
      }
      break;

    default: /* default for whole assedit parse function */
             /* we should never get here */
             mudlog_printf(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC assedit_parse(): Reached default case!");
             send_to_char_printf(g, d->character, "Opps...\r\n");
             desc_set_connected(d, CON_PLAYING);
             break;
  }
}
/* End of Assedit Parse */

static void assedit_edit_extract(GameWorld *g, struct descriptor_data *d)
{
  send_to_char_printf(g, d->character, "Is the item to be extracted when the assembly is created? (Y/N):");
  OLC_MODE(d) = ASSEDIT_EDIT_EXTRACT;
  return;
}

static void assedit_edit_inroom(GameWorld *g, struct descriptor_data *d)
{
  send_to_char_printf(g, d->character, "Should the object be in the room when assembly is created (n = in inventory)?");
  OLC_MODE(d) = ASSEDIT_EDIT_INROOM;
  return;
}

static void nodigit(GameWorld *g, struct descriptor_data *d)
{
  send_to_char_printf(g, d->character, "Usage: assedit <vnum>\r\n");
  send_to_char_printf(g, d->character, "     : assedit new <vnum>\r\n");
  send_to_char_printf(g, d->character, "     : assedit delete <vnum>\r\n");
  return;
}
