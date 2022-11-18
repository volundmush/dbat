/* ******************************************************************** *
 * FILE : assedit.c                     Copyright (C) 1999 Del Minturn  *
 * USAGE: Olc for assembly engine by Geoff Davis.                       *
 *        Oasis OLC by George Greer.
 * -------------------------------------------------------------------- *
 * 1999 July 25 caminturn@earthlink.net                                *
 * ******************************************************************** */

#include "assedit.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "oasis.h"
#include "assemblies.h"
#include "constants.h"


/*-------------------------------------------------------------------*
 * Nasty internal macros to clean up the code.
 *-------------------------------------------------------------------*/
long lRnum = 0;

/*-------------------------------------------------------------------*
 * Assedit command
 *-------------------------------------------------------------------*/

ACMD (do_assedit)
{
  struct descriptor_data *d = ch->desc;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

 *buf = '\0';  /* If I run into problems then take this sucker out */
 *buf2 = '\0';

 if (IS_NPC(ch))
      return;

 for (d = descriptor_list; d; d = d->next) {
     if (d->connected == CON_ASSEDIT) {
     send_to_char(ch, "Assemblies are already being editted by someone.\r\n");
     return;
     }
   }

 two_arguments(argument, buf, buf2);

 d= ch->desc;

 if(!*buf) {
    nodigit(d);
    return;
    }

 if (!isdigit(*buf)) {
    if (strncasecmp("new", buf, 3) == 0) {
      if (!isdigit(*buf2)) {
            nodigit(d);
      } else if (real_object(atoi(buf2)) == NOTHING) {
        send_to_char(d->character, "You need to create the assembly object before you can create the new assembly.\r\n");
        return;
      } else {
            assemblyCreate(atoi(buf2), 0);
            send_to_char(d->character, "Assembly Created.\r\n");
            assemblySaveAssemblies();
            return;
            }
       }
    else
    if (strncasecmp("delete", buf, 6) == 0) {
         if (!isdigit(*buf2))
            nodigit(d);
         else {
             assemblyDestroy(atoi(buf2));
             send_to_char(d->character, "Assembly Deleted.\r\n");
             assemblySaveAssemblies();
             return;
             }
      }
    else {
     nodigit(d);
     return;
     }
 } else
   if (isdigit(*buf)) {
     d = ch->desc;
     CREATE (d->olc, struct oasis_olc_data, 1);
     assedit_setup(d, atoi(buf));

     }
  return;
}

/*-------------------------------------------------------------------*
 * Assedit Functions
 *-------------------------------------------------------------------*/

void assedit_setup(struct descriptor_data *d, int number)
{

    ASSEMBLY    *pOldAssembly = nullptr;
    CREATE(OLC_ASSEDIT(d), ASSEMBLY, 1 );


    if( (pOldAssembly = assemblyGetAssemblyPtr( number )) == nullptr ) {
      send_to_char(d->character, "That assembly does not exist\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    } else {
        /* Copy the old assembly. */
        OLC_ASSEDIT(d)->lVnum = pOldAssembly->lVnum;
        OLC_ASSEDIT(d)->uchAssemblyType = pOldAssembly->uchAssemblyType;
        OLC_ASSEDIT(d)->lNumComponents = pOldAssembly->lNumComponents;

        if( OLC_ASSEDIT(d)->lNumComponents > 0 )  {
            CREATE(OLC_ASSEDIT(d)->pComponents, COMPONENT, OLC_ASSEDIT(d)->lNumComponents);
            memmove(OLC_ASSEDIT(d)->pComponents, pOldAssembly->pComponents,
            OLC_ASSEDIT(d)->lNumComponents * sizeof( COMPONENT ) );
           }

    }

    /*
     * At this point, pNewAssembly is now the address of a freshly allocated copy of all
     * the data contained in the original assembly structure.
     */


    if ( (lRnum = real_object( OLC_ASSEDIT(d)->lVnum ) ) < 0)
      {
       send_to_char(d->character, "Assembled item may not exist, check the vnum and assembles (show assemblies). \r\n");
       cleanup_olc(d, CLEANUP_ALL);    /* for right now we just get out! */
       return;
      }

 STATE(d) = CON_ASSEDIT;
 act("$n starts using OLC.", TRUE, d->character, nullptr, nullptr, TO_ROOM);
 SET_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
 assedit_disp_menu(d);

}

void assedit_disp_menu(struct descriptor_data *d)
{
 int i = 0;
 char szAssmType[MAX_INPUT_LENGTH] = { '\0' };

 sprinttype(OLC_ASSEDIT(d)->uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType));

  #if defined(CLEAR_SCREEN)
  send_to_char(d->character, "%c[H%c[J", 27, 27);
  #endif

  send_to_char(d->character,
      "Assembly Number: @c%ld@n\r\n"
      "Assembly Name  : @y%s@n\r\n"
      "Assembly Type  : @y%s@n\r\n"
      "Components:\r\n",
        OLC_ASSEDIT(d)->lVnum,
        obj_proto[real_object(OLC_ASSEDIT(d)->lVnum)].short_description,
        szAssmType
       );

  if (OLC_ASSEDIT(d)->lNumComponents <= 0)
    send_to_char(d->character, "   < NONE > \r\n");
  else {
   for (i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++ ) {
     if ((lRnum = real_object(OLC_ASSEDIT(d)->pComponents[i].lVnum)) < 0) {
        send_to_char(d->character, "@g%2d@n) @y ERROR --- Contact an Implementor @n\r\n ", i+1);
       }  else   {
        send_to_char(d->character,
          "@g%2d@n) [@c%5ld@n] %-20.20s  In room: @c%-3.3s@n    Extract: @y%-3.3s@n\r\n",
           i+1, OLC_ASSEDIT(d)->pComponents[i].lVnum,
           obj_proto[lRnum].short_description,
           (OLC_ASSEDIT(d)->pComponents[i].bInRoom  ? "Yes" : "No"),
           (OLC_ASSEDIT(d)->pComponents[i].bExtract ? "Yes" : "No"));
       }
      }
     }
  send_to_char(d->character,
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

void assedit_parse(struct descriptor_data *d, char *arg)
{
 int pos = 0, i = 0,  counter, columns = 0;

   COMPONENT   *pTComponents = nullptr;

 switch (OLC_MODE(d)) {

   case ASSEDIT_MAIN_MENU:
     switch (*arg) {
     case 'q':
     case 'Q':                /* do the quit stuff */
       /* Ok, Time to save it back to the original stuff and get out */
       /* due to the infrequent use of this code and restricted use  */
       /* I decided to copy over changes regarless.                  */
       assemblyDestroy(OLC_ASSEDIT(d)->lVnum);
       assemblyCreate(OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->uchAssemblyType);
       for( i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
           assemblyAddComponent(OLC_ASSEDIT(d)->lVnum,
                                OLC_ASSEDIT(d)->pComponents[i].lVnum,
                                OLC_ASSEDIT(d)->pComponents[i].bExtract,
                                OLC_ASSEDIT(d)->pComponents[i].bInRoom
                                );
            }
       send_to_char(d->character, "\r\nSaving all assemblies\r\n");
       assemblySaveAssemblies();

/*       free(pTComponents);
       free(OLC_ASSEDIT(d));
*/
       cleanup_olc(d, CLEANUP_ALL);    /* for right now we just get out! */
      break;

     case 't':
     case 'T':
#if defined(CLEAR_SCREEN)
send_to_char(d->character, "%c[H%c[J", 27, 27);
#endif
      for (counter = 0; counter < MAX_ASSM; counter++) {
           send_to_char(d->character, "@g%2d@n) %-20.20s %s", counter + 1,
                AssemblyTypes[counter], !(++columns % 2) ? "\r\n" : "");
           }
           send_to_char(d->character, "Enter the assembly type : ");
     OLC_MODE(d) = ASSEDIT_EDIT_TYPES;

     break;
     case 'a':
     case 'A':                /* add a new component */
       send_to_char(d->character, "\r\nWhat is the vnum of the new component?");
       OLC_MODE(d) = ASSEDIT_ADD_COMPONENT;
      break;

     case 'e':
     case 'E':                /* edit a component */
       send_to_char(d->character, "\r\nEdit which component? ");
       OLC_MODE(d) = ASSEDIT_EDIT_COMPONENT;
      break;
     case 'd':
     case 'D':                /* delete a component */
       if ((pos < 0) || pos > OLC_ASSEDIT(d)->lNumComponents) {
           send_to_char(d->character, "\r\nWhich component do you wish to remove?");
           assedit_disp_menu(d);
       } else {
           send_to_char(d->character, "\r\nWhich component do you wish to remove?");
           OLC_MODE(d) = ASSEDIT_DELETE_COMPONENT;
       }
      break;

     default:
       assedit_disp_menu(d);
    }
   break;

 case ASSEDIT_EDIT_TYPES:
   if (isdigit(*arg)){
    pos = atoi(arg) - 1;
     if( (pos >= 0) || (pos < MAX_ASSM)) {
           OLC_ASSEDIT(d)->uchAssemblyType = pos;
     assedit_disp_menu(d);
     break;
    }
   }
   else
   assedit_disp_menu(d);

 break;
 case ASSEDIT_ADD_COMPONENT:              /* add a new component */
   if (isdigit(*arg)){
      pos = atoi(arg);
#if CIRCLE_UNSIGNED_INDEX
     if ((real_object(pos)) == NOTHING)    /* does the object exist? */
         break;
#else
     if ((real_object(pos)) <= NOTHING)    /* does the object exist? */
         break;
#endif

     for ( i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
        if(OLC_ASSEDIT(d)->pComponents[i].lVnum == pos)
          break;
       }

     CREATE( pTComponents, COMPONENT, OLC_ASSEDIT(d)->lNumComponents + 1);

     if(OLC_ASSEDIT(d)->pComponents != nullptr) {          /* Copy from olc to temp */
        memmove(pTComponents, OLC_ASSEDIT(d)->pComponents,
                  OLC_ASSEDIT(d)->lNumComponents * sizeof(COMPONENT) );
/*        free(OLC_ASSEDIT(d)->pComponents); */
       }

     OLC_ASSEDIT(d)->pComponents = pTComponents;
     OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].lVnum = pos;
     OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].bExtract = YES;
     OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].bInRoom = NO;
     OLC_ASSEDIT(d)->lNumComponents += 1;

     assedit_disp_menu(d);

   } else {
     send_to_char(d->character, "That object does not exist. Please try again\r\n");
     assedit_disp_menu(d);
     }
  break;

 case ASSEDIT_EDIT_COMPONENT:
   pos = atoi(arg);
   if (isdigit(*arg)) {
      pos--;
      OLC_VAL(d) = pos;
      assedit_edit_extract(d);
      break;
    }
   else
      assedit_disp_menu(d);
   break;

 case ASSEDIT_DELETE_COMPONENT:

  if (isdigit(*arg)) {
    pos = atoi(arg);
    pos -= 1;

    CREATE( pTComponents, COMPONENT, OLC_ASSEDIT(d)->lNumComponents -1);

    if( pos > 0 )
      memmove( pTComponents, OLC_ASSEDIT(d)->pComponents, pos * sizeof( COMPONENT ) );

    if( pos < OLC_ASSEDIT(d)->lNumComponents - 1 )
       memmove( pTComponents + pos, OLC_ASSEDIT(d)->pComponents + pos + 1,
            (OLC_ASSEDIT(d)->lNumComponents - pos - 1) * sizeof(COMPONENT) );

    free(OLC_ASSEDIT(d)->pComponents );
    OLC_ASSEDIT(d)->pComponents = pTComponents;
    OLC_ASSEDIT(d)->lNumComponents -= 1;

    assedit_disp_menu(d);
    break;
  } else
    assedit_disp_menu(d);
  break;

 case ASSEDIT_EDIT_EXTRACT:
  switch (*arg) {
    case 'y':
    case 'Y':
      OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bExtract = TRUE;
      assedit_edit_inroom(d);
    break;

    case 'n':
    case 'N':
      OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bExtract = FALSE;
      assedit_edit_inroom(d);
    break;

    default:
      send_to_char(d->character, "Is the item to be extracted when the assembly is created? (Y/N)");
    break;
    }
   break;
 case ASSEDIT_EDIT_INROOM:
  switch (*arg) {
    case 'y':
    case 'Y':
      OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bInRoom = TRUE;
      assedit_disp_menu(d);
    break;

    case 'n':
    case 'N':
      OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bInRoom = FALSE;
      assedit_disp_menu(d);
    break;

    default:
      send_to_char(d->character, "Object in the room when assembly is created? (n =  in inventory):");
    break;
    }
 break;

 default:                        /* default for whole assedit parse function */
                                 /* we should never get here */
 mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: OLC assedit_parse(): Reached default case!");
 send_to_char(d->character, "Opps...\r\n");
 STATE(d) = CON_PLAYING;
 break;
 }
}
/* End of Assedit Parse */

void assedit_delete(struct descriptor_data *d)
{
 send_to_char(d->character, "Which item number do you wish to delete from this assembly?");
 OLC_MODE(d) = ASSEDIT_DELETE_COMPONENT;
 return;
}


void assedit_edit_extract(struct descriptor_data *d)
{
 send_to_char(d->character, "Is the item to be extracted when the assembly is created? (Y/N):");
 OLC_MODE(d) = ASSEDIT_EDIT_EXTRACT;
 return;
}

void assedit_edit_inroom(struct descriptor_data *d)
{
 send_to_char(d->character, "Should the object be in the room when assembly is created (n = in inventory)?");
 OLC_MODE(d) = ASSEDIT_EDIT_INROOM;
 return;
}

void nodigit(struct descriptor_data *d)
{
  send_to_char(d->character, "Usage: assedit <vnum>\r\n");
  send_to_char(d->character, "     : assedit new <vnum>\r\n");
  send_to_char(d->character, "     : assedit delete <vnum>\r\n");
  return;
}


