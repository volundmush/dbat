/* ************************************************************************
*  File: obj_edit.c                                    Part of DBAT       *
*  Usage: Player level object editing                                     *
*                                                                         *
*  This is an original file created by me for Dragonball Advent Truth     *
*  to house all player level object editing functions -- Iovan 1/6/13     *
************************************************************************ */
#include <cstdlib>

#include "dbat/consts/attacks.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/spells.hpp"
#include "dbat/screen.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.other.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions  */

/* utility variables */

/* This is what equipment type it is */
static const char *const custom_types[17] = {
    "Error",
    "Armor Slot",
    "Gi Slot",
    "Wrist Slot",
    "Ear Slot",
    "Finger Slot",
    "Eye Slot",
    "Hands Slot",
    "Feet Slot",
    "Belt Slot",
    "Legs Slot",
    "Arms Slot",
    "Head Slot",
    "Neck Slot",
    "Back Slot",
    "Shoulder Slot",
    "Weapon"};

/* This shows what weapon type it is */
static const char *const custom_weapon[7] = {
    "Not A Weapon",
    "Sword",
    "Dagger",
    "Spear",
    "Club",
    "Gun",
    "Brawling"};

/* Below this is the custom equipment creation menu section of obj_edit.c */

/* This displays the custom equipment construction menu to the player. */
void disp_custom_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@GCustom Equipment Construction Menu@n\n");
  write_to_output(g, d, "@D-----------------------------------------@n\n");
  write_to_output(g, d, "@C1@D) @WKeyword Name     @D: @w%s@n\n", d->obj_name);
  write_to_output(g, d, "@C2@D) @WShort Description@D: @w%s@n\n", d->obj_short);
  write_to_output(g, d, "@C3@D) @WLong Description @D: @w%s@n\n", d->obj_long);
  write_to_output(g, d, "@C4@D) @WEquipment Type   @D: @w%s@n\n", custom_types[d->obj_type]);
  write_to_output(g, d, "@C5@D) @WWeapon Type      @D: @w%s@n\n", custom_weapon[d->obj_weapon]);
  write_to_output(g, d, "@CQ@D) @WQuit Menu@n\n");
}

/* This displays the equipment restring menu, as if you couldn't tell. */
void disp_restring_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@GEquipment Restring Menu@n\n");
  write_to_output(g, d, "@D-----------------------------------------@n\n");
  write_to_output(g, d, "@C1@D) @WKeyword Name     @D: @w%s@n\n", d->obj_name);
  write_to_output(g, d, "@C2@D) @WShort Description@D: @w%s@n\n", d->obj_short);
  write_to_output(g, d, "@C3@D) @WLong Description @D: @w%s@n\n", d->obj_long);
  write_to_output(g, d, "@CQ@D) @WQuit Menu@n\n");
}

/* This handles the player level object construction process. */
void pobj_edit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{

  struct obj_data *obj = nullptr;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];

  /* This section is dedicated to equipment restringing */
  if (d->obj_editflag == EDIT_RESTRING) {
    switch (d->obj_editval) {
      case EDIT_RESTRING_MAIN:
        switch (*arg) {
          case '1': /* Name Editing */
            write_to_output(g, d, "@wCurrent Equipment's Keyword Name @D<@Y%s@D>@n\r\n", d->obj_name);
            write_to_output(g, d, "@wEnter Name @D(@RDo not use colorcode!@D)@w:@n \n");
            d->obj_editval = EDIT_RESTRING_NAME;
            break;

          case '2': /* Short Desc Editing */
            write_to_output(g, d, "@wCurrent Equipment's Short Desc @D<@Y%s@D>@n\r\n", d->obj_short);
            write_to_output(g, d, "@wEnter Short Desc@w:@n \n");
            d->obj_editval = EDIT_RESTRING_SDESC;
            break;

          case '3': /* Long Desc Editing */
            write_to_output(g, d, "@wCurrent Equipment's Long Desc @D<@Y%s@D>@n\r\n", d->obj_long);
            write_to_output(g, d, "@wEnter Long Desc@w:@n \n");
            d->obj_editval = EDIT_RESTRING_LDESC;
            break;

          case 'Q': /* Quit Editing */
          case 'q':
            write_to_output(g, d, "Save current changes and be charged?\r\nYes or No\r\n");
            d->obj_editval = EDIT_RESTRING_QUIT;
            break;

          default:
            disp_restring_menu(g, d);
            return;
        }
        break;

      case EDIT_RESTRING_NAME: /* Name menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
          disp_restring_menu(g, d);
          d->obj_editval = EDIT_RESTRING_MAIN;
          return;
        }
        if (strstr(arg, "@")) {
          write_to_output(g, d, "@RNO COLORCODE IN THE KEYWORD NAME.@n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (strlen(arg) > 100) {
          write_to_output(g, d, "@wToo long. Limit is 100 character.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (d->obj_name) {
          free(d->obj_name);
        }
        *buf        = '\0';
        d->obj_name = strdup(arg);
        disp_restring_menu(g, d);
        d->obj_editval = EDIT_RESTRING_MAIN;
        break;

      case EDIT_RESTRING_SDESC: /* Short Desc Menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_RESTRING_MAIN;
          return;
        }
        if (strlen(arg) > 150) {
          write_to_output(g, d, "@wToo long. Limit is 200 character.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (d->obj_short) {
          free(d->obj_short);
        }
        *buf         = '\0';
        d->obj_short = strdup(arg);
        disp_restring_menu(g, d);
        d->obj_editval = EDIT_RESTRING_MAIN;
        break;

      case EDIT_RESTRING_LDESC: /* Long Desc Menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_RESTRING_MAIN;
          return;
        }
        if (strlen(arg) > 200) {
          write_to_output(g, d, "@wToo long. Limit is 200 character.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (d->obj_long) {
          free(d->obj_long);
        }
        *buf        = '\0';
        d->obj_long = strdup(arg);
        disp_restring_menu(g, d);
        d->obj_editval = EDIT_RESTRING_MAIN;
        break;

      case EDIT_RESTRING_QUIT:
        if (!*arg) {
          write_to_output(g, d, "Save current changes and be charged?\r\nYes or No\r\n");
          return;
        }
        if (!strcasecmp(arg, "yes") || !strcasecmp(arg, "Yes") || !strcasecmp(arg, "y") || !strcasecmp(arg, "Y")) {
          obj = d->obj_point;

          *buf      = '\0';
          obj_set_name(obj, strdup(d->obj_name));

          *buf2                  = '\0';
          obj_set_short_description(obj, strdup(d->obj_short));

          *buf3            = '\0';
          obj_set_description(obj, strdup(d->obj_long));

          d->obj_editflag = EDIT_NONE;
          d->obj_editval  = EDIT_NONE;
          char_set_gold(d->character, char_get_gold(d->character) - 5000);
          obj_extra_set(obj, ITEM_RESTRING, true);
          write_to_output(g, d, "Purchase complete.");
          send_to_imm(g, "Restring Eq: %s has bought: %s, which was %s.", char_get_name(d->character), obj_get_short_description(obj), d->obj_was);
          desc_set_connected(d, CON_PLAYING);
        } else if (!strcasecmp(arg, "No") || !strcasecmp(arg, "no") || !strcasecmp(arg, "n") || !strcasecmp(arg, "N")) {
          write_to_output(g, d, "Canceling purchase at no cost.\r\n");
          send_to_imm(g, "Restring Eq: %s has canceled their equipment restring.", char_get_name(d->character));
          d->obj_editval = EDIT_NONE;
          desc_set_connected(d, CON_PLAYING);
        } else {
          write_to_output(g, d, "Save current changes and be charged?\r\nYes or No\r\n");
          return;
        }
        break;

      default: /* whoops */
        d->obj_editval = EDIT_RESTRING_MAIN;
        disp_restring_menu(g, d);
        return;
    } /* End main switch for restring */
  } /* End of equipment restring area */

  /* This section is dedicated to custom equipment construction */
  if (d->obj_editflag == EDIT_CUSTOM) {
    switch (d->obj_editval) {
      case EDIT_CUSTOM_MAIN: /* The main menu of custom equipment edit */
        switch (*arg) {
          case '1': /* Let them name it. */
            write_to_output(g, d, "@wEnter Equipment's Keyword Name @D(@RDo not use colorcode!@D)@w.@n\r\n");
            write_to_output(g, d, "@wEnter: @n\n");
            d->obj_editval = EDIT_CUSTOM_NAME;
            break;
          case '2': /* Let them give it a short description. */
            write_to_output(g, d, "@wEnter Equipment's Short Description. This is the colored name you see in your inventory or when the eq is used.@n\r\n");
            write_to_output(g, d, "@wEnter: @n\n");
            d->obj_editval = EDIT_CUSTOM_SDESC;
            break;
          case '3': /* Let them give it a long description. */
            write_to_output(g, d, "@wEnter Equipment's Long Description. This is the colored name you see when it's seen in a room. @D(@RNo punctuation at the end!@D)@n\r\n");
            write_to_output(g, d, "@wEnter: @n\n");
            d->obj_editval = EDIT_CUSTOM_LDESC;
            break;
          case '4': /* Let them choose what type of equipment it is. */
            write_to_output(g, d, "@wEnter number to select type of equipment you want it to be: @n\n");
            write_to_output(g, d, "@D[ @C--1@W) @cArmor Slot  @C--2@W) @cGi Slot     @C--3@W) @cWrist Slot   @D]@n\n");
            write_to_output(g, d, "@D[ @C--4@W) @cEar Slot    @C--5@W) @cFinger Slot @C--6@W) @cEye Slot     @D]@n\n");
            write_to_output(g, d, "@D[ @C--7@W) @cHands Slot  @C--8@W) @cFeet Slot   @C--9@W) @cBelt Slot    @D]@n\n");
            write_to_output(g, d, "@D[ @C-10@W) @cLegs Slot   @C-11@W) @cArms Slot   @C-12@W) @cHead Slot    @D]@n\n");
            write_to_output(g, d, "@D[ @C-13@W) @cNeck Slot   @C-14@W) @cBack Slot   @C-15@W) @cShoulder Slot@D]@n\n");
            write_to_output(g, d, "@D[ @C-16@W) @cWeapon@n\r\n");
            write_to_output(g, d, "@wEnter: @n\n");
            d->obj_editval = EDIT_CUSTOM_TYPE;
            break;
          case '5': /* If it's a weapon then let them choose what type. */
            if (d->obj_type != 16) {
              write_to_output(g, d, "@wYou can only use this part of the menu if you select the weapon type.@n\r\n");
              return;
            }
            write_to_output(g, d, "@wEnter number to select type of weapon you want it to be: @n\n");
            write_to_output(g, d, "@D[ @C--1@W) @cSword       @C--2@W) @cDagger      @C--3@W) @cSpear        @D]@n\n");
            write_to_output(g, d, "@D[ @C--4@W) @cClub        @C--5@W) @cGun         @C--6@W) @cBrawling     @D]@n\n");
            write_to_output(g, d, "@wEnter: @n\n");
            d->obj_editval = EDIT_CUSTOM_WEAPON;
            break;
          case 'q': /* They are exiting, time to finalize purchase or clear it. */
          case 'Q':
            write_to_output(g, d, "@wPurchase this custom piece? (Y or N)@n\r\n");
            d->obj_editval = EDIT_CUSTOM_QUIT;
            break;
          default: /* They entered nothing, show them the menu. */
            disp_custom_menu(g, d);
            return;
        }
        break;

      case EDIT_CUSTOM_NAME: /* Name menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_CUSTOM_MAIN;
          return;
        }
        if (strstr(arg, "@")) {
          write_to_output(g, d, "@RNO COLORCODE IN THE KEYWORD NAME.@n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (strlen(arg) > 100) {
          write_to_output(g, d, "@wToo long. Limit is 100 character.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (d->obj_name) {
          free(d->obj_name);
        }
        *buf        = '\0';
        d->obj_name = strdup(arg);
        disp_custom_menu(g, d);
        d->obj_editval = EDIT_CUSTOM_MAIN;
        break;

      case EDIT_CUSTOM_SDESC: /* Short Desc Menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_CUSTOM_MAIN;
          return;
        }
        if (strlen(arg) > 150) {
          write_to_output(g, d, "@wToo long. Limit is 200 character.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (d->obj_short) {
          free(d->obj_short);
        }
        *buf         = '\0';
        d->obj_short = strdup(arg);
        disp_custom_menu(g, d);
        d->obj_editval = EDIT_CUSTOM_MAIN;
        break;

      case EDIT_CUSTOM_LDESC: /* Long Desc Menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_CUSTOM_MAIN;
          return;
        }
        if (strlen(arg) > 200) {
          write_to_output(g, d, "@wToo long. Limit is 200 character.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          return;
        }
        if (d->obj_long) {
          free(d->obj_long);
        }
        *buf        = '\0';
        d->obj_long = strdup(arg);
        disp_custom_menu(g, d);
        d->obj_editval = EDIT_CUSTOM_MAIN;
        break;

      case EDIT_CUSTOM_TYPE: /* Type Menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Returning to main menu.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_CUSTOM_MAIN;
          return;
        }
        d->obj_type = atoi(arg);
        if (d->obj_type < 1 || d->obj_type > 16) {
          write_to_output(g, d, "@wValue must be between 1 and 16.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          d->obj_type = 1;
          return;
        }
        if (d->obj_type == 16) {
          d->obj_weapon = 1;
        }
        disp_custom_menu(g, d);
        d->obj_editval = EDIT_CUSTOM_MAIN;
        break;

      case EDIT_CUSTOM_WEAPON: /* Weapon Menu */
        if (!*arg) {
          write_to_output(g, d, "@wNothing entered. Returning to main menu.@n\r\n");
          disp_custom_menu(g, d);
          d->obj_editval = EDIT_CUSTOM_MAIN;
          return;
        }
        d->obj_weapon = atoi(arg);
        if (d->obj_weapon < 1 || d->obj_weapon > 6) {
          write_to_output(g, d, "@wValue must be between 1 and 6.@n\r\n");
          write_to_output(g, d, "@wEnter: @n\n");
          d->obj_weapon = 0;
          return;
        }
        disp_custom_menu(g, d);
        d->obj_editval = EDIT_CUSTOM_MAIN;
        break;

      case EDIT_CUSTOM_QUIT: /* Quiting */
        if (!*arg) {
          write_to_output(g, d, "@wPurchase this custom piece? (Y or N)@n\r\n");
          return;
        }
        if (!strcasecmp(arg, "y") || !strcasecmp(arg, "Y")) {
          write_to_output(g, d, "@wPurchase complete.@n\r\n");
          desc_set_connected(d, CON_PLAYING);
          if (d->obj_weapon == 0) {
            obj = read_object(g, 20099, VIRTUAL);
            obj_to_char(g, obj, d->character);
            switch (d->obj_type) {
              case 1:
                obj_wear_set(obj, ITEM_WEAR_BODY, true);
                break;
              case 2:
                obj_wear_set(obj, ITEM_WEAR_ABOUT, true);
                break;
              case 3:
                obj_wear_set(obj, ITEM_WEAR_WRIST, true);
                break;
              case 4:
                obj_wear_set(obj, ITEM_WEAR_EAR, true);
                break;
              case 5:
                obj_wear_set(obj, ITEM_WEAR_FINGER, true);
                break;
              case 6:
                obj_wear_set(obj, ITEM_WEAR_EYE, true);
                break;
              case 7:
                obj_wear_set(obj, ITEM_WEAR_HANDS, true);
                break;
              case 8:
                obj_wear_set(obj, ITEM_WEAR_FEET, true);
                break;
              case 9:
                obj_wear_set(obj, ITEM_WEAR_WAIST, true);
                break;
              case 10:
                obj_wear_set(obj, ITEM_WEAR_LEGS, true);
                break;
              case 11:
                obj_wear_set(obj, ITEM_WEAR_ARMS, true);
                break;
              case 12:
                obj_wear_set(obj, ITEM_WEAR_HEAD, true);
                break;
              case 13:
                obj_wear_set(obj, ITEM_WEAR_NECK, true);
                break;
              case 14:
                obj_wear_set(obj, ITEM_WEAR_PACK, true);
                break;
              case 15:
                obj_wear_set(obj, ITEM_WEAR_SH, true);
                break;
            }
            *buf                   = '\0';
            obj_set_name(obj, strdup(d->obj_name));
            *buf2                  = '\0';
            obj_set_short_description(obj, strdup(d->obj_short));
            *buf3                  = '\0';
            obj_set_description(obj, strdup(d->obj_long));
          } else {
            obj = read_object(g, 20098, VIRTUAL);
            obj_to_char(g, obj, d->character);
            *buf                   = '\0';
            obj_set_name(obj, strdup(d->obj_name));
            *buf2                  = '\0';
            obj_set_short_description(obj, strdup(d->obj_short));
            *buf3                  = '\0';
            obj_set_description(obj, strdup(d->obj_long));
            switch (d->obj_weapon) {
              case 1:
                obj_set_value(obj, VAL_WEAPON_DAMTYPE, TYPE_SLASH - TYPE_HIT);
                break;
              case 2:
                obj_set_value(obj, VAL_WEAPON_DAMTYPE, TYPE_PIERCE - TYPE_HIT);
                break;
              case 3:
                obj_set_value(obj, VAL_WEAPON_DAMTYPE, TYPE_STAB - TYPE_HIT);
                break;
              case 4:
                obj_set_value(obj, VAL_WEAPON_DAMTYPE, TYPE_CRUSH - TYPE_HIT);
                break;
              case 5:
                obj_set_value(obj, VAL_WEAPON_DAMTYPE, TYPE_BLAST - TYPE_HIT);
                break;
              case 6:
                obj_set_value(obj, VAL_WEAPON_DAMTYPE, TYPE_POUND - TYPE_HIT);
                break;
            }
            obj_set_level(obj, 20);
          }
          obj_extra_set(obj, ITEM_SLOT2, true);
          d->obj_editflag = EDIT_NONE;
          d->obj_editval  = EDIT_NONE;
          char_set_rp(d->character, char_get_rp(d->character) - 20);
          char_set_rp(d->character, char_get_rp(d->character));
          d->rpp -= 20;
          userWrite(g, d, 0, 0, 0, "index");
          obj_set_size(obj, get_size(d->character));
          obj_extra_set(obj, ITEM_CUSTOM, true);
          send_to_imm(g, "Custom Eq: %s has bought: %s.", char_get_name(d->character), obj_get_short_description(obj));
          customWrite(d->character, obj);
          log_custom(g, d, obj);
        } else if (!strcasecmp(arg, "n") || !strcasecmp(arg, "N")) {
          write_to_output(g, d, "Canceling purchase at no cost.\r\n");
          send_to_imm(g, "Custom Eq: %s has canceled their custom eq construction.", char_get_name(d->character));
          d->obj_editval = EDIT_NONE;

          desc_set_connected(d, CON_PLAYING);
        }
        break;

    } /* End main custom switch */
  } /* End Custom Area */

} /* End pobj_edit_parse */
