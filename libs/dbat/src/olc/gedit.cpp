/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  gedit.c:  Olc written for shoplike guildmasters, code by             *
 *             Jason Goodwin                                             *
 *    Made for Circle3.0 bpl11, its copyright applies                    *
 *                                                                       *
 *  Made for Oasis OLC                                                   *
 *  Copyright 1996 Harvey Gilpin.                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/guild.hpp"
#include "dbat/config.hpp"
#include "dbat/gedit.hpp"
#include "dbat/genzon.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/oasis.hpp"
#include "dbat/gengld.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/spells.hpp"
#include "dbat/feats.hpp"
#include "dbat/genolc.hpp"
#include "dbat/shop.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void gedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void gedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct guild_data *guild);
static void gedit_setup_new(GameWorld *g, struct descriptor_data *d);

/*
 * Should check more things.
 */
static void gedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  OLC_GUILD(d)->vnum = OLC_NUM(d);
  add_guild(g, OLC_GUILD(d));
}

static void gedit_save_to_disk(GameWorld *g, zone_rnum num)
{
  save_guilds(g, num);
}

/*-------------------------------------------------------------------*\
  utility functions 
 \*-------------------------------------------------------------------*/

ACMD(do_oasis_gedit)
{
  IDXTYPE number = NOWHERE;
  int save       = 0;
  struct guild_data *existing;
  struct descriptor_data *d;
  char buf1[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  two_arguments(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(g, ch, "Specify a guild VNUM to edit.\r\n");
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
  /** Check that the guild isn't already being edited.                        **/
  /****************************************************************************/
  for (descriptor_data *dd : each_desc(g)) {
    if (desc_get_connected(dd) == CON_GEDIT) {
      if (dd->olc && OLC_NUM(dd) == number) {
        send_to_char(g, ch, "That guild is currently being edited by %s.\r\n",
                     char_pers(g, dd->character, ch));
        return;
      }
    }
  }

  /****************************************************************************/
  /** Point d to the builder's descriptor.                                   **/
  /****************************************************************************/
  d = char_get_desc(ch);

  /****************************************************************************/
  /** Give the descriptor an OLC structure.                                  **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
           "SYSERR: do_oasis_gedit: Player already had olc structure.");
    delete d->olc;
  }

  d->olc = new oasis_olc_data();

  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  if ((OLC_ZNUM(d) = real_zone_by_thing(g, number)) == NOWHERE) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_cannot_edit(g, ch, g->zones[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Free the OLC structure.                                              **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  if (save) {
    send_to_char(g, ch, "Saving all guilds in zone %d.\r\n",
                 g->zones[OLC_ZNUM(d)].number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
           "OLC: %s saves guild info for zone %d.",
           char_get_name(ch), g->zones[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Save the guild to the guild file.                                    **/
    /**************************************************************************/
    gedit_save_to_disk(g, OLC_ZNUM(d));

    /**************************************************************************/
    /** Free the OLC structure.                                              **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  if ((existing = find_guild(g, number)) != nullptr)
    gedit_setup_existing(g, d, existing);
  else
    gedit_setup_new(g, d);

  desc_set_connected(d, CON_GEDIT);

  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
         char_get_name(ch), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));
}

static void gedit_setup_new(GameWorld *g, struct descriptor_data *d)
{
  int i;
  struct guild_data *guilddata;

  /*. Alloc some guild shaped space . */
  guilddata = new guild_data();

  /*. Some default values . */
  guild_set_gm(guilddata, -1);
  guild_set_open(guilddata, 0);
  guild_set_close(guilddata, 28);
  guild_set_charge(guilddata, 1.0);
  for (i = 0; i < GW_ARRAY_MAX; i++)
    guild_get_with_who(guilddata)
        [i] = 0;
  guild_set_func(guilddata, nullptr);
  guild_set_minlvl(guilddata, 0);

  /*. Some default strings . */
  guild_set_no_such_skill(guilddata, strdup(guild_default_no_skill));
  guild_set_not_enough_gold(guilddata, strdup(guild_default_no_gold));

  /* init the wasteful skills and spells table */

  for (i = 0; i < SKILL_TABLE_SIZE; i++)
    if (g->spell_info[i].skilltype == SKTYPE_SKILL &&
        strcmp(g->spell_info[i].name, "!UNUSED!")) {
      guild_set_skill(guilddata, i, 0);
    }

  /* init the feats table */
  for (i = 0; i < NUM_FEATS_DEFINED; i++)
    if (g->feat_list[i].in_game) {
      guild_set_feat(guilddata, i, 0);
    }

  OLC_GUILD(d) = guilddata;
  gedit_disp_menu(g, d);
}

/*-------------------------------------------------------------------*/

static void gedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct guild_data *guild)
{
  /*. Alloc some guild shaped space . */
  OLC_GUILD(d) = new guild_data();
  copy_guild(OLC_GUILD(d), guild);
  gedit_disp_menu(g, d);
}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions 
**************************************************************************/

/*-------------------------------------------------------------------*/

static void gedit_select_skills_menu(GameWorld *g, struct descriptor_data *d)
{
  int i, j = 0, found = 0;
  struct guild_data *guilddata;

  guilddata = OLC_GUILD(d);
  clear_screen(g, d);

  write_to_output(g, d, "Skills known:\r\n");

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (g->spell_info[i].skilltype == SKTYPE_SKILL &&
        strcmp(g->spell_info[i].name, "!UNUSED!")) {
      write_to_output(g, d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                      YESNO(guild_get_skill(guilddata, i)), i, g->spell_info[i].name);
      j++;
      found = 1;
    }
    if (found && !(j % 3)) {
      found = 0;
      write_to_output(g, d, "\r\n");
    }
  }
  write_to_output(g, d, "\r\nEnter skill num, 0 to quit:  ");

  OLC_MODE(d) = GEDIT_SELECT_SKILLS;
}

/*-------------------------------------------------------------------*/

static void gedit_select_spells_menu(GameWorld *g, struct descriptor_data *d)
{
  int i, j = 0, found = 0;
  struct guild_data *guilddata;

  guilddata = OLC_GUILD(d);
  clear_screen(g, d);

  write_to_output(g, d, "Spells known:\r\n");

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (IS_SET(g->spell_info[i].skilltype, SKTYPE_SPELL) &&
        strcmp(g->spell_info[i].name, "!UNUSED!")) {
      write_to_output(g, d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                      YESNO(guild_get_skill(guilddata, i)), i, g->spell_info[i].name);
      j++;
      found = 1;
    }
    if (found && !(j % 3)) {
      found = 0;
      write_to_output(g, d, "\r\n");
    }
  }
  write_to_output(g, d, "\r\nEnter spell num, 0 to quit:  ");

  OLC_MODE(d) = GEDIT_SELECT_SPELLS;
}

/*-------------------------------------------------------------------*/

static void gedit_select_feats_menu(GameWorld *g, struct descriptor_data *d)
{
  int i, j = 0, found = 0;
  struct guild_data *guilddata;

  guilddata = OLC_GUILD(d);
  clear_screen(g, d);

  write_to_output(g, d, "Feats known:\r\n");

  for (i = 0; i < NUM_FEATS_DEFINED; i++) {
    if (g->feat_list[i].in_game) {
      write_to_output(g, d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                      YESNO(guild_get_feat(guilddata, i)), i, g->feat_list[i].name);
      j++;
      found = 1;
    }
    if (found && !(j % 3)) {
      found = 0;
      write_to_output(g, d, "\r\n");
    }
  }
  write_to_output(g, d, "\r\nEnter feat num, 0 to quit:  ");

  OLC_MODE(d) = GEDIT_SELECT_FEATS;
}

/*-------------------------------------------------------------------*/

static void gedit_select_lang_menu(GameWorld *g, struct descriptor_data *d)
{
  int i, j = 0, found = 0;
  struct guild_data *guilddata;

  guilddata = OLC_GUILD(d);
  clear_screen(g, d);

  write_to_output(g, d, "Skills known:\r\n");

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (IS_SET(g->spell_info[i].skilltype, SKTYPE_LANG) &&
        strcmp(g->spell_info[i].name, "!UNUSED!")) {
      write_to_output(g, d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                      YESNO(guild_get_skill(guilddata, i)), i, g->spell_info[i].name);
      j++;
      found = 1;
    }
    if (found && !(j % 3)) {
      found = 0;
      write_to_output(g, d, "\r\n");
    }
  }
  write_to_output(g, d, "\r\nEnter skill num, 0 to quit:  ");

  OLC_MODE(d) = GEDIT_SELECT_LANGS;
}

/*-------------------------------------------------------------------*/

static void gedit_select_wp_menu(GameWorld *g, struct descriptor_data *d)
{
  int i, j = 0, found = 0;
  struct guild_data *guilddata;

  guilddata = OLC_GUILD(d);
  clear_screen(g, d);

  write_to_output(g, d, "Skills known:\r\n");

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (IS_SET(g->spell_info[i].skilltype, SKTYPE_WEAPON) &&
        strcmp(g->spell_info[i].name, "!UNUSED!")) {
      write_to_output(g, d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                      YESNO(guild_get_skill(guilddata, i)), i, g->spell_info[i].name);
      j++;
      found = 1;
    }
    if (found && !(j % 3)) {
      found = 0;
      write_to_output(g, d, "\r\n");
    }
  }
  write_to_output(g, d, "\r\nEnter skill num, 0 to quit:  ");

  OLC_MODE(d) = GEDIT_SELECT_WPS;
}

/*-------------------------------------------------------------------*/

static void gedit_no_train_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  int i, count = 0;
  struct guild_data *guilddata;

  guilddata = OLC_GUILD(d);
  clear_screen(g, d);

  for (i = 0; i < NUM_TRADERS; i++) {
    write_to_output(g, d, "@g%2d@n) %-20.20s   %s", i + 1, trade_letters[i],
                    !(++count % 2) ? "\r\n" : "");
  }

  sprintbitarray(guild_get_with_who(guilddata), trade_letters, 4, bits, sizeof(bits));
  write_to_output(g, d, "\r\nCurrent train flags: @c%s@n\r\n"
                     "Enter choice, 0 to quit : ",
                  bits);
  OLC_MODE(d) = GEDIT_NO_TRAIN;
}

/*-------------------------------------------------------------------*/
/*. Display main menu . */

static void gedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  struct guild_data *guilddata;
  char buf1[MAX_STRING_LENGTH];

  guilddata = OLC_GUILD(d);

  clear_screen(g, d);

  sprintbitarray(guild_get_with_who(guilddata), trade_letters, 4, buf1, sizeof(buf1));

  write_to_output(g, d,
                  "-- Guild Number: [@c%d@n]\r\n"
                  "@g 0@n) Guild Master : [@c%d@n] @y%s\r\n"
                  "@g 1@n) Doesn't know skill:\r\n @y%s\r\n"
                  "@g 2@n) Player no gold:\r\n @y%s\r\n"
                  "@g 3@n) Open   :  [@c%d@n]\r\n"
                  "@g 4@n) Close  :  [@c%d@n]\r\n"
                  "@g 5@n) Charge :  [@c%3.1f@n]\r\n"
                  "@g 6@n) Minlvl :  [@c%d@n]\r\n"
                  "@g 7@n) Who to Train:  @c%s\r\n"
                  "@g 8@n) Feats Menu\r\n"
                  "@g 9@n) Skills Menu\r\n"
                  "@g B@n) Languages Menu\r\n"
                  "@g Q@n) Quit\r\n"
                  "Enter Choice : ",

                  OLC_NUM(d),
                  guild_get_gm(guilddata) == NOBODY ? -1 : g->mob_index[guild_get_gm(guilddata)].vnum,
                  guild_get_gm(guilddata) == NOBODY ? "None" : char_get_sdesc(&g->mob_proto[guild_get_gm(guilddata)]),
                  guild_get_no_such_skill(guilddata),
                  guild_get_not_enough_gold(guilddata),
                  guild_get_open(guilddata),
                  guild_get_close(guilddata),
                  guild_get_charge(guilddata),
                  guild_get_minlvl(guilddata),
                  buf1);

  OLC_MODE(d) = GEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
**************************************************************************/

/* Stores a builder-written guild message, or tells the builder why it can't be used. */
static void gedit_set_message(GameWorld *g, struct descriptor_data *d, char **msg, char *arg)
{
  const char *problem = gedit_modify_string(msg, arg);

  if (problem)
    write_to_output(g, d,
                    "Invalid message: %s. It starts with %%s, the customer's name (added if you leave it off), "
                    "and can have no other %% codes.\r\n",
                    problem);
}

void gedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  IDXTYPE i;

  if (OLC_MODE(d) > GEDIT_NUMERICAL_RESPONSE) {
    if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
      write_to_output(g, d, "Field must be numerical, try again : ");
      return;
    }
  }
  switch (OLC_MODE(d)) {
      /*-------------------------------------------------------------------*/
    case GEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          send_to_char(g, d->character, "Saving Guild to memory.\r\n");
          gedit_save_internally(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                 "OLC: %s edits guild %d", char_get_name(d->character), OLC_NUM(d));
          if (CONFIG_OLC_SAVE) {
            gedit_save_to_disk(g, real_zone_by_thing(g, OLC_NUM(d)));
            write_to_output(g, d, "Guild %d saved to disk.\r\n", OLC_NUM(d));
          } else
            write_to_output(g, d, "Guild %d saved to memory.\r\n", OLC_NUM(d));
          cleanup_olc(g, d, CLEANUP_STRUCTS);
          return;
        case 'n':
        case 'N':
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to save the guild? : ");
          return;
      }
      break;

      /*-------------------------------------------------------------------*/
    case GEDIT_MAIN_MENU:
      i = 0;
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /*. Anything been changed? . */
            write_to_output(g, d, "Do you wish to save the changes to the Guild? (y/n) : ");
            OLC_MODE(d) = GEDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case '0':
          OLC_MODE(d) = GEDIT_TRAINER;
          write_to_output(g, d, "Enter vnum of guild master : ");
          return;
        case '1':
          OLC_MODE(d) = GEDIT_NO_SKILL;
          i--;
          break;
        case '2':
          OLC_MODE(d) = GEDIT_NO_CASH;
          i--;
          break;
        case '3':
          OLC_MODE(d) = GEDIT_OPEN;
          write_to_output(g, d, "When does this shop open (a day has 28 hours) ? ");
          i++;
          break;
        case '4':
          OLC_MODE(d) = GEDIT_CLOSE;
          write_to_output(g, d, "When does this shop close (a day has 28 hours) ? ");
          i++;
          break;
        case '5':
          OLC_MODE(d) = GEDIT_CHARGE;
          i++;
          break;
        case '6':
          OLC_MODE(d) = GEDIT_MINLVL;
          write_to_output(g, d, "Minumum Level will Train: ");
          i++;
          return;
        case '7':
          OLC_MODE(d) = GEDIT_NO_TRAIN;
          gedit_no_train_menu(g, d);
          return;
        case '8':
          OLC_MODE(d) = GEDIT_SELECT_FEATS;
          gedit_select_feats_menu(g, d);
          return;
        case '9':
          OLC_MODE(d) = GEDIT_SELECT_SKILLS;
          gedit_select_skills_menu(g, d);
          return;
        /*case 'a':
				case 'A':
					OLC_MODE(d) = GEDIT_SELECT_WPS;
					gedit_select_wp_menu(d);
					return; */
        case 'b':
        case 'B':
          OLC_MODE(d) = GEDIT_SELECT_LANGS;
          gedit_select_lang_menu(g, d);
          return;
        default:
          gedit_disp_menu(g, d);
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
      /*. String edits . */
    case GEDIT_NO_SKILL:
      gedit_set_message(g, d, &((OLC_GUILD(d))->no_such_skill), arg);
      break;
    case GEDIT_NO_CASH:
      gedit_set_message(g, d, &((OLC_GUILD(d))->not_enough_gold), arg);
      break;

      /*-------------------------------------------------------------------*/
      /*. Numerical responses . */

    case GEDIT_TRAINER:
      if (isdigit(*arg)) {
        i = atoi(arg);
        if ((i = atoi(arg)) != -1)
          if ((i = real_mobile(g, i)) == NOBODY) {
            write_to_output(g, d, "That mobile does not exist, try again : ");
            return;
          }
        guild_set_gm(OLC_GUILD(d), i);
        if (i == -1)
          break;
        /*. Fiddle with special procs . */
        guild_set_func(OLC_GUILD(d), g->mob_index[i].func != guild ? g->mob_index[i].func : nullptr);
        g->mob_index[i].func = guild;
        break;
      }
      write_to_output(g, d, "Invalid response.\r\n");
      gedit_disp_menu(g, d);
      return;
    case GEDIT_OPEN:
      guild_set_open(OLC_GUILD(d), LIMIT(atoi(arg), 0, 28));
      break;
    case GEDIT_CLOSE:
      guild_set_close(OLC_GUILD(d), LIMIT(atoi(arg), 0, 28));
      ;
      break;
    case GEDIT_CHARGE:
      sscanf(arg, "%f", &((OLC_GUILD(d))->charge));
      break;
    case GEDIT_NO_TRAIN:
      if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS - 1)) > 0) {
        guild_trade_toggle(OLC_GUILD(d), i - 1);
        gedit_no_train_menu(g, d);
        return;
      }
      break;

    case GEDIT_MINLVL:
      guild_set_minlvl(OLC_GUILD(d), MAX(atoi(arg), 0));
      break;

    case GEDIT_SELECT_SPELLS:
      i = atoi(arg);
      if (i == 0)
        break;
      i = MAX(0, MIN(i, SKILL_TABLE_SIZE - 1));
      guild_set_skill(OLC_GUILD(d), i, !guild_get_skill(OLC_GUILD(d), i));
      gedit_select_spells_menu(g, d);
      return;

    case GEDIT_SELECT_FEATS:
      i = atoi(arg);
      if (i == 0)
        break;
      i = MAX(0, MIN(i, NUM_FEATS_DEFINED - 1));
      guild_set_feat(OLC_GUILD(d), i, !guild_get_feat(OLC_GUILD(d), i));
      gedit_select_feats_menu(g, d);
      return;

    case GEDIT_SELECT_SKILLS:
      i = atoi(arg);
      if (i == 0)
        break;
      i = MAX(0, MIN(i, SKILL_TABLE_SIZE - 1));
      guild_set_skill(OLC_GUILD(d), i, !guild_get_skill(OLC_GUILD(d), i));
      gedit_select_skills_menu(g, d);
      return;

    case GEDIT_SELECT_WPS:
      i = atoi(arg);
      if (i == 0)
        break;
      i = MAX(0, MIN(i, SKILL_TABLE_SIZE - 1));
      guild_set_skill(OLC_GUILD(d), i, !guild_get_skill(OLC_GUILD(d), i));
      gedit_select_wp_menu(g, d);
      return;

    case GEDIT_SELECT_LANGS:
      i = atoi(arg);
      if (i == 0)
        break;
      i = MAX(0, MIN(i, SKILL_TABLE_SIZE - 1));
      guild_set_skill(OLC_GUILD(d), i, !guild_get_skill(OLC_GUILD(d), i));
      gedit_select_lang_menu(g, d);
      return;

      /*-------------------------------------------------------------------*/
    default:
      /*. We should never get here . */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: gedit_parse(): "
                                            "Reached default case!");
      write_to_output(g, d, "Oops...\r\n");
      break;
  }
  /*-------------------------------------------------------------------*/
  /*. END OF CASE 
  If we get here, we have probably changed something, and now want to
  return to main menu.  Use OLC_VAL as a 'has changed' flag . */

  OLC_VAL(d) = 1;
  gedit_disp_menu(g, d);
}
