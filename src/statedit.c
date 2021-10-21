/************************************************************************
 *  --Statedit  Part of UrathMud                                v1.0    *
 *  Copyright 1999 Karl N. Matthias.  All rights Reserved.              *
 *  You may freely distribute, modify, or sell this code                *
 *  as long as this copyright remains intact.                           *
 *                                                                      *
 *  Based on code by Jeremy Elson, Harvey Gilpin, and George Greer.     *
 ************************************************************************/

/* --relistan 2/22/99 - 2/24/99 */

#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "oasis.h"

int parse_stat_menu(struct descriptor_data *d, char *arg);
int stats_disp_menu(struct descriptor_data *d);
int stats_assign_stat(int abil, char *arg, struct descriptor_data *d);

/* --relistan 2/22/99 for player configurable stats */
int parse_stats(struct descriptor_data *d, char *arg)
{
  struct char_data *ch;

  ch = d->character;
  switch(OLC_MODE(d)) {
    case STAT_QUIT: 
      return 1;
    case STAT_PARSE_MENU:
      if(parse_stat_menu(d, arg)) 
        return 1;
    break;
    case STAT_GET_STR:
      ch->real_abils.str = stats_assign_stat(ch->real_abils.str, arg, d);
      stats_disp_menu(d);
    break;
    case STAT_GET_INT:
      ch->real_abils.intel = stats_assign_stat(ch->real_abils.intel, arg, d);
      stats_disp_menu(d);
    break;
    case STAT_GET_WIS:
      ch->real_abils.wis = stats_assign_stat(ch->real_abils.wis, arg, d);
      stats_disp_menu(d);
    break;
    case STAT_GET_DEX:
      ch->real_abils.dex = stats_assign_stat(ch->real_abils.dex, arg, d);
      stats_disp_menu(d);
    break;
    case STAT_GET_CON:
      ch->real_abils.con = stats_assign_stat(ch->real_abils.con, arg, d);
      stats_disp_menu(d);
    break;
    case STAT_GET_CHA:
      ch->real_abils.cha = stats_assign_stat(ch->real_abils.cha, arg, d);
      stats_disp_menu(d);
    break;
    default:
      OLC_MODE(d) = stats_disp_menu(d); 
      break;
  }
  return 0;
}

int stats_disp_menu(struct descriptor_data *d)
{
  send_to_char(d->character,
    "\r\n"
    "@W-<[@y==========@B[ @YCWG @B]@y==========@W]>-\r\n"
    " <| Total Points Left: @m%3d@W    |>     You should select the letter of the score you\r\n"
    " <|                           |>     wish to adjust.  When prompted, enter the new score,\r\n"
    " <| = Select a stat:          |>     NOT the amount to add.  NOTE: If you quit before you\r\n"
    " <| @BS@W) @rStrength     : @m%3d@W     |>     assign all the points, you will lose them forever.\r\n"
    " <| @BD@W) @rDexterity    : @m%3d@W     |>     If your points are at zero, you may still reassign\r\n"
    " <| @BN@W) @rConstitution : @m%3d@W     |>     points by lowering any statistic, then add those\r\n"
    " <| @BI@W) @rIntelligence : @m%3d@W     |>     points to the statistic of your choice.\r\n"
    " <| @BW@W) @rWisdom       : @m%3d@W     |>\r\n"
    " <| @BC@W) @rCharisma     : @m%3d@W     |>\r\n"
    " <| @BQ@W) @CQuit@W                   |>\r\n"
    "-<[@y===========================@W]>-@n\r\n"
    "\r\n", OLC_VAL(d),
    d->character->real_abils.str,
    d->character->real_abils.dex,
    d->character->real_abils.con,
    d->character->real_abils.intel,
    d->character->real_abils.wis,
    d->character->real_abils.cha);

  send_to_char(d->character, "Enter Letter to Change: ");

  OLC_MODE(d) = STAT_PARSE_MENU;

  return 1;
}

int parse_stat_menu(struct descriptor_data *d, char *arg)
{
  /* Main parse loop */
  *arg = LOWER(*arg);
  switch (*arg) {
    case 's': 
      OLC_MODE(d) = STAT_GET_STR;
      send_to_char(d->character, "Enter new value: ");
    break;
    case 'i': 
      OLC_MODE(d) = STAT_GET_INT;
      send_to_char(d->character, "Enter new value: ");
    break;
    case 'w': 
      OLC_MODE(d) = STAT_GET_WIS;
      send_to_char(d->character, "Enter new value: ");
    break;
    case 'd': 
      OLC_MODE(d) = STAT_GET_DEX;
      send_to_char(d->character, "Enter new value: ");
    break;
    case 'n': 
      OLC_MODE(d) = STAT_GET_CON;
      send_to_char(d->character, "Enter new value: ");
    break;
    case 'c': 
      OLC_MODE(d) = STAT_GET_CHA;
      send_to_char(d->character, "Enter new value: ");
    break;
    case 'q': 
      OLC_MODE(d) = STAT_QUIT; 
      return 1;
    default: 
      stats_disp_menu(d);
  }
  return 0;
}

int stats_assign_stat(int abil, char *arg, struct descriptor_data *d)
{
  int temp;

  if (abil > 0) {
      OLC_VAL(d) = OLC_VAL(d)
          + abil;
      abil = 0;
  }

  if (atoi(arg) > OLC_VAL(d))
    temp = OLC_VAL(d);
  else
    temp = atoi(arg);
  if (temp > 100) {
    if (OLC_VAL(d) < 100)
      temp = OLC_VAL(d);
    else temp = 100;
  }
  if (temp < 3) {
    temp = 3;
  }
  /* This should throw an error! */
  if (OLC_VAL(d) <= 0) {
    temp = 0;
    OLC_VAL(d) = 0;
    mudlog(NRM, ADMLVL_IMMORT, TRUE, "Stat total below 0: possible code error");
  }
  abil = temp;
  OLC_VAL(d) -= temp;

  return abil;
}

