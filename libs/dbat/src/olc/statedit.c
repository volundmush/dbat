/************************************************************************
 *  --Statedit  Part of UrathMud                                v1.0    *
 *  Copyright 1999 Karl N. Matthias.  All rights Reserved.              *
 *  You may freely distribute, modify, or sell this code                *
 *  as long as this copyright remains intact.                           *
 *                                                                      *
 *  Based on code by Jeremy Elson, Harvey Gilpin, and George Greer.     *
 ************************************************************************/

/* --relistan 2/22/99 - 2/24/99 */

#include "dbat/statedit.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/oasis.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static bool parse_stat_menu(GameWorld *g, struct descriptor_data *d, char *arg);
static int stats_assign_stat(GameWorld *g, int abil, char *arg, struct descriptor_data *d);

/* --relistan 2/22/99 for player configurable stats */
bool parse_stats(GameWorld *g, struct descriptor_data *d, char *arg)
{
  struct char_data *ch;

  ch = d->character;
  switch (OLC_MODE(d)) {
    case STAT_QUIT:
      return true;
    case STAT_PARSE_MENU:
      if (parse_stat_menu(g, d, arg))
        return true;
      break;
    case STAT_GET_STR:
      char_set_real_abils_str(ch, stats_assign_stat(g, char_get_real_abils_str(ch), arg, d));
      stats_disp_menu(g, d);
      break;
    case STAT_GET_INT:
      char_set_real_abils_intel(ch, stats_assign_stat(g, char_get_real_abils_intel(ch), arg, d));
      stats_disp_menu(g, d);
      break;
    case STAT_GET_WIS:
      char_set_real_abils_wis(ch, stats_assign_stat(g, char_get_real_abils_wis(ch), arg, d));
      stats_disp_menu(g, d);
      break;
    case STAT_GET_DEX:
      char_set_real_abils_dex(ch, stats_assign_stat(g, char_get_real_abils_dex(ch), arg, d));
      stats_disp_menu(g, d);
      break;
    case STAT_GET_CON:
      char_set_real_abils_con(ch, stats_assign_stat(g, char_get_real_abils_con(ch), arg, d));
      stats_disp_menu(g, d);
      break;
    case STAT_GET_CHA:
      char_set_real_abils_cha(ch, stats_assign_stat(g, char_get_real_abils_cha(ch), arg, d));
      stats_disp_menu(g, d);
      break;
    default:
      OLC_MODE(d) = stats_disp_menu(g, d);
      break;
  }
  return false;
}

bool stats_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  send_to_char(g, d->character,
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
               "\r\n",
               OLC_VAL(d),
               char_get_real_abils_str(d->character),
               char_get_real_abils_dex(d->character),
               char_get_real_abils_con(d->character),
               char_get_real_abils_intel(d->character),
               char_get_real_abils_wis(d->character),
               char_get_real_abils_cha(d->character));

  send_to_char(g, d->character, "Enter Letter to Change: ");

  OLC_MODE(d) = STAT_PARSE_MENU;

  return true;
}

static bool parse_stat_menu(GameWorld *g, struct descriptor_data *d, char *arg)
{
  /* Main parse loop */
  *arg = tolower(*arg);
  switch (*arg) {
    case 's':
      OLC_MODE(d) = STAT_GET_STR;
      send_to_char(g, d->character, "Enter new value: ");
      break;
    case 'i':
      OLC_MODE(d) = STAT_GET_INT;
      send_to_char(g, d->character, "Enter new value: ");
      break;
    case 'w':
      OLC_MODE(d) = STAT_GET_WIS;
      send_to_char(g, d->character, "Enter new value: ");
      break;
    case 'd':
      OLC_MODE(d) = STAT_GET_DEX;
      send_to_char(g, d->character, "Enter new value: ");
      break;
    case 'n':
      OLC_MODE(d) = STAT_GET_CON;
      send_to_char(g, d->character, "Enter new value: ");
      break;
    case 'c':
      OLC_MODE(d) = STAT_GET_CHA;
      send_to_char(g, d->character, "Enter new value: ");
      break;
    case 'q':
      OLC_MODE(d) = STAT_QUIT;
      return true;
    default:
      stats_disp_menu(g, d);
  }
  return false;
}

static int stats_assign_stat(GameWorld *g, int abil, char *arg, struct descriptor_data *d)
{
  int temp;

  if (abil > 0) {
    OLC_VAL(d) = OLC_VAL(d) + abil;
    abil       = 0;
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
    temp       = 0;
    OLC_VAL(d) = 0;
    mudlog(g, LOG_NRM, ADMLVL_IMMORT, true, "Stat total below 0: possible code error");
  }
  abil = temp;
  OLC_VAL(d) -= temp;

  return abil;
}
