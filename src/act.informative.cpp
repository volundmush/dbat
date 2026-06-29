/* ************************************************************************
 *   File: act.informative.c                             Part of CircleMUD *
 *  Usage: Player-level commands of an informative nature                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "command.h"
#include "config.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/appearance.h"
#include "consts/applies.h"
#include "consts/attacks.h"
#include "consts/constates.h"
#include "consts/exitflags.h"
#include "consts/fightprefs.h"
#include "consts/itemdata.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "consts/sizes.h"
#include "consts/skills.h"
#include "consts/songs.h"
#include "consts/time.h"
#include "consts/weapons.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dgscript_impl.h"
#include "flags.h"
#include "help.h"
#include "object_impl.h"
#include "object_macros.h"
#include "races.h"
#include "room_api.h"
#include "room_impl.h"
#include "room_macros.h"
#include "skills.h"
#include "stringutils.h"
#include "time_info.h"
#include "util_macros.h"
#include "weather_db.h"

#include "class.h"
#include "db.h"
#include "dg_scripts.h"
#include "extract.h"
#include "fileop.h"
#include "interpreter.h"
#include "iterate.hpp"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_utils.h"
#include "search.h"

#include "act.informative.h"
#include "act.item.h"
#include "act.social.h"
#include "act.wizard.h"
#include "character_utils.h"
#include "comm.h"
#include "config.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "handler.h"
#include "log.h"
#include "maputils.h"
#include "object_api.h"
#include "object_db.h"
#include "room_db.h"
#include "spells.h"
#include "vehicles.h"

#include "boards.h"

#include "clan.h"
#include "guild.h"
#include "mail.h"
#include "screen.h"

#include "iterate.hpp"
#include <vector>

/* local functions */
static void gen_map(struct char_data *ch, int num);
static void bringdesc(struct char_data *ch, struct char_data *tch);
static void see_plant(struct obj_data *obj, struct char_data *ch);
static double terrain_bonus(struct char_data *ch);
static void search_room(struct char_data *ch);
static void bonus_status(struct char_data *ch);
static int sort_commands_helper(const void *a, const void *b);
static void print_object_location(int num, struct obj_data *obj,
                                  struct char_data *ch, int recur);
static void show_obj_to_char(struct obj_data *obj, struct char_data *ch,
                             int mode);
static void list_obj_to_char(struct inventory_data list, struct char_data *ch,
                             int mode, bool show);
static void trans_check(struct char_data *ch, struct char_data *vict);
static int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
static void perform_mortal_where(struct char_data *ch, char *arg);
static void perform_immort_where(struct char_data *ch, char *arg);
static void diag_char_to_char(struct char_data *i, struct char_data *ch);
static void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);
static void look_at_char(struct char_data *i, struct char_data *ch);
static void list_one_char(struct char_data *i, struct char_data *ch);
static void list_char_to_char(struct room_data *room, struct char_data *ch);
static void look_in_direction(struct char_data *ch, int dir);
static void look_in_obj(struct char_data *ch, char *arg);
static void look_out_window(struct char_data *ch, char *arg);
static void look_at_target(struct char_data *ch, char *arg, int read);
static void search_in_direction(struct char_data *ch, int dir);
static void do_auto_exits(struct room_data *target_room, struct char_data *ch,
                          int exit_mode);
static void do_auto_exits2(struct room_data *target_room, struct char_data *ch);
static void display_spells(struct char_data *ch, struct obj_data *obj);
static void display_scroll(struct char_data *ch, struct obj_data *obj);
static void space_to_minus(char *str);
static void free_history(struct char_data *ch, int type);
static int yesrace(int num);
static void map_draw_room(char map[9][10], int x, int y, struct room_data *room,
                          struct char_data *ch);
// definitions
/* do_evolve has been moved to lua/characters/commands/advancement/evolve.lua */

static void see_plant(struct obj_data *obj, struct char_data *ch) {

  int water = GET_OBJ_VAL(obj, VAL_WATERLEVEL);

  if (water >= 0) {
    switch (GET_OBJ_VAL(obj, VAL_MATURITY)) {
    case 0:
      send_to_char(ch,
                   "@wA @G%s@y seed@w has been planted here. @D(@C%d Water "
                   "Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    case 1:
      send_to_char(ch,
                   "@wA very young @G%s@w has sprouted from a planter here. "
                   "@D(@C%d Water Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    case 2:
      send_to_char(ch,
                   "@wA half grown @G%s@w is in a planter here. @D(@C%d Water "
                   "Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    case 3:
      send_to_char(ch,
                   "@wA mature @G%s@w is growing in a planter here. @D(@C%d "
                   "Water Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    case 4:
      send_to_char(ch,
                   "@wA mature @G%s@w is flowering in a planter here. @D(@C%d "
                   "Water Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    case 5:
      send_to_char(ch,
                   "@wA mature @G%s@w that is close to harvestable is here. "
                   "@D(@C%d Water Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    case 6:
      send_to_char(ch,
                   "@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d "
                   "Water Hours@D)@n\r\n",
                   obj->short_description, water);
      break;
    default:
      break;
    }
  } else {
    if (water > -4) {
      send_to_char(ch,
                   "@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n",
                   obj->short_description);
    } else if (water > -10) {
      send_to_char(
          ch, "@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n",
          obj->short_description);
    } else if (water <= -10) {
      send_to_char(ch,
                   "@yA @G%s@y that is completely @rdead@y and @rwithered@y, "
                   "is here.@n\r\n",
                   obj->short_description);
    }
  }
}

/* This is used to determine the terrain bonus for search_room - Iovan
 * 12/16/2012*/
static double terrain_bonus(struct char_data *ch) {

  double bonus = 0.0;

  struct room_data *room = char_room_get(ch);

  switch (room_sector_type_get(room)) {
  case SECT_FOREST:
    bonus += 0.5;
    break;
  case SECT_SPACE:
    bonus += -0.5;
    break;
  case SECT_WATER_NOSWIM:
    bonus += 0.25;
    break;
  case SECT_MOUNTAIN:
    bonus += 0.1;
    break;
  default:
    bonus += 0.0;
    break;
  }

  if (room_flagged(room, ROOM_SPACE)) {
    bonus += -0.5;
  }

  return (bonus);
}

/* This is used to find hidden people in a room with search - Iovan 12/16/2012
 */
static void search_room(struct char_data *ch) {

  struct char_data *vict, *next_v;
  int perc = (GET_INT(ch) * 0.6) + GET_SKILL(ch, SKILL_SPOT) +
             GET_SKILL(ch, SKILL_SEARCH) + GET_SKILL(ch, SKILL_LISTEN);
  int prob = 0, found = 0;
  double bonus = 1.0, terrain = 1.0;

  if ((getCurST(ch)) < GET_MAX_MOVE(ch) * 0.001) {
    send_to_char(ch, "You do not have enough stamina.\r\n");
    return;
  }

  if (GET_SKILL(ch, SKILL_SENSE)) {
    bonus += (GET_SKILL(ch, SKILL_SENSE) * 0.01);
  }

  reveal_hiding(ch, 0);
  act("@y$n@Y begins searching the room carefully.@n", TRUE, ch, 0, 0, TO_ROOM);
  WAIT_STATE(ch, PULSE_1SEC);

  room_people_iterate(char_room_get(ch), [&](auto vict) {
    if (!AFF_FLAGGED(vict, AFF_HIDE) || vict == ch) {
      return true;
    }
    if (GET_SUPPRESS(vict) >= 1) {
      perc *= (GET_SUPPRESS(vict) * 0.01);
    }
    prob = GET_DEX(vict) + (GET_INT(vict) * 0.6) +
           GET_SKILL(vict, SKILL_HIDE) + GET_SKILL(vict, SKILL_MOVE_SILENTLY);

    if (AFF_FLAGGED(vict, AFF_LIQUEFIED)) {
      prob *= 1.5;
    }
    if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4)) {
      perc += 5;
    }
    if (IS_MUTANT(vict) &&
        (GET_GENOME(vict, 0) == 5 || GET_GENOME(vict, 1) == 5)) {
      prob += 10;
    }
    terrain += terrain_bonus(vict);
    if (perc * bonus >= prob * terrain) { /* Found them. */
      act("@YYou find @y$N@Y hiding nearby!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@y$n@Y has found your hiding spot!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@y$n@Y has found @y$N's@Y hiding spot!@n", TRUE, ch, 0, vict,
          TO_NOTVICT);
      reveal_hiding(vict, 4);
      found++;
    }
    return true;
  });

  room_contents_iterate(char_room_get(ch), [&](auto obj) {
    if (OBJ_FLAGGED(obj, ITEM_BURIED) && perc * bonus > rand_number(50, 200)) {
      act("@YYou uncover @y$p@Y, which had been burried here.@n", TRUE, ch, obj,
          0, TO_CHAR);
      act("@y$n@Y uncovers @y$p@Y, which had burried here.@n", TRUE, ch, obj, 0,
          TO_ROOM);
      REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BURIED);
      found++;
    }
    return true;
  });
  decCurSTPercent(ch, .001);

  if (found == 0) {
    send_to_char(ch, "You find nothing hidden.\r\n");
    return;
  }
}

static const char *weapon_disp[6] = {"Sword", "Dagger", "Spear",
                                     "Club",  "Gun",    "Brawling"};



/* do_kyodaika moved to lua/characters/commands/namek/kyodaika.lua */

/* do_table moved to lua/characters/commands/cardgame/table.lua */

/* do_draw moved to lua/characters/commands/cardgame/draw.lua */

/* do_shuffle moved to lua/characters/commands/cardgame/shuffle.lua */

/* do_hand moved to lua/characters/commands/cardgame/hand.lua */

ACMD(do_post) {

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct obj_data *obj2;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: post (obj name)\n"
                     "        post (obj name) (target obj name)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
    send_to_char(ch, "You don't seem to have that.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_NOTE) {
    send_to_char(ch, "You can only post notepaper.\r\n");
    return;
  }

  struct room_data *room = char_room_get(ch);

  if (room_flagged(room, ROOM_GARDEN1) || room_flagged(room, ROOM_GARDEN2)) {
    send_to_char(ch, "You can not post on things in a garden.\r\n");
    return;
  }

  int sect = room_sector_type_get(room);

  if (!*arg2) {
    if (sect != SECT_INSIDE && sect != SECT_CITY) {
      send_to_char(
          ch, "You are not near any general structure you can post it on.\r\n");
      return;
    }
    act("@WYou post $p@W on a nearby structure.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W posts $p@W on a nearby structure.@n", TRUE, ch, obj, 0,
        TO_ROOM);
    obj_from_char(obj);
    obj_to_room(obj, char_room_get(ch));
    GET_OBJ_POSTTYPE(obj) = 1;
    return;
  } else {
    if (!(obj2 = get_obj_in_list_vis(ch, arg2, NULL,
                                     inv_for_room(char_room_get(ch))))) {
      send_to_char(
          ch, "You can't seem to find the thing you want to post it on.\r\n");
      return;
    } else if (GET_OBJ_POSTED(obj2)) {
      send_to_char(ch, "It already has something posted on it. Get that first "
                       "if you want to post.\r\n");
      return;
    } else if (GET_OBJ_TYPE(obj2) == ITEM_BOARD) {
      send_to_char(ch,
                   "Boards come with their own means of posting messages.\r\n");
      return;
    } else {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "@C$n@W posts %s@W on %s@W.@n", obj->short_description,
              obj2->short_description);
      send_to_char(ch, "@WYou post %s@W on %s@W.@n\r\n", obj->short_description,
                   obj2->short_description);
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
      obj_from_char(obj);
      obj_to_room(obj, char_room_get(ch));
      GET_OBJ_POSTTYPE(obj) = 2;
      GET_OBJ_POSTED(obj) = obj2;
      GET_OBJ_POSTED(obj2) = obj;
      return;
    }
  }
}

/* do_play moved to lua/characters/commands/cardgame/play.lua */

/* Nickname an object */
ACMD(do_nickname) {
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: nickname (object) (nickname)\n");
    send_to_char(ch, "Syntax: nickname ship (nickname)\n");
    return;
  }

  if (strcasecmp(arg, "ship")) {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
      send_to_char(ch, "You don't have that item to nickname.\r\n");
      return;
    }
  }
  if (strlen(arg2) > 20) {
    send_to_char(ch, "You can't nickname items with any name longer than 20 "
                     "characters.\r\n");
    return;
  }

  if (!strcasecmp(arg, "ship")) {
    struct obj_data *ship = NULL, *ship2 = NULL;
    int found = FALSE;
    room_contents_iterate(char_room_get(ch), [&](auto ship) {
      if (GET_OBJ_VNUM(ship) >= 45000 && GET_OBJ_VNUM(ship) <= 45999 &&
          found == FALSE) {
        found = TRUE;
        ship2 = ship;
      }
      return true;
    });
    if (found == TRUE) {
      if (strstr(arg2, "@")) {
        send_to_char(
            ch, "You can't nickname a ship and use color codes. Sorry.\r\n");
        return;
      } else {
        char nick[MAX_INPUT_LENGTH];
        sprintf(nick, "%s", CAP(arg2));
        ship2->action_description = strdup(nick);
        obj_iterate_all([&](struct obj_data *k) {
          if (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(ship2) + 1000) {
            extract_obj(k);
            struct room_data *was_in = obj_room_get(ship2);
            obj_from_room(ship2);
            obj_to_room(ship2, was_in);
          }
          return true;
        });
      }
    }
    return;
  }

  if (strstr(obj->short_description, "nicknamed")) {
    send_to_char(ch, "%s@w has already been nicknamed.@n\r\n",
                 obj->short_description);
    return;
  } else if (strstr(obj->name, "corpse")) {
    send_to_char(ch, "%s@w is a corpse!@n\r\n", obj->short_description);
    return;
  } else {
    send_to_char(ch, "@wYou nickname %s@w as '@C%s@w'.@n\r\n",
                 obj->short_description, arg2);
    char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH];
    sprintf(nick, "%s @wnicknamed @D(@C%s@D)@n", obj->short_description,
            CAP(arg2));
    sprintf(nick2, "%s %s", obj->name, arg2);
    obj->short_description = strdup(nick);
    obj->name = strdup(nick2);
    return;
  }
}

/* local globals */
int *cmd_sort_info;

/* Portal appearance types */
static const char *portal_appearance[] = {
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive "
    "today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and "
    "lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its "
    "gaze upon you.",
    "\n"};

/* do_showoff moved to lua/characters/commands/items/show.lua */

/* Used for checking if you know the character in question. */
void introCreate(struct char_data *ch) {
  char fname[40];
  FILE *fl;
  /* Write Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mud_log("ERROR: could not save user, %s, to filename, %s.", GET_NAME(ch),
        fname);
    return;
  }

  fprintf(fl, "Gibbles Gibbles\n");

  fclose(fl);
  return;
}

int readIntro(struct char_data *ch, struct char_data *vict) {
  char fname[40], filler[50], scrap[100], line[256];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */
  if (vict == NULL) {
    return 0;
  }

  if (IS_NPC(ch) || IS_NPC(vict)) {
    return 1;
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch))) {
    introCreate(ch);
  }
  if (!(fl = fopen(fname, "r"))) {
    return 2;
  }
  if (vict == ch) {
    fclose(fl);
    return 0;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%s %s\n", filler, scrap);
    if (!strcasecmp(GET_NAME(vict), filler)) {
      known = TRUE;
    }
  }
  fclose(fl);

  if (known == TRUE)
    return 1;
  else
    return 0;
}

void introWrite(struct char_data *ch, struct char_data *vict, char *name) {
  FILE *file;
  char fname[40], filler[50], scrap[100], line[256];
  char *names[500] = {""}, *alias[500] = {""};
  FILE *fl;
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch))) {
    introCreate(ch);
  }
  if (!(file = fopen(fname, "r"))) {
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%s %s\n", filler, scrap);
    names[count] = strdup(filler);
    alias[count] = strdup(scrap);
    count++;
    *filler = '\0';
    *scrap = '\0';
  }
  fclose(file);

  /* Write Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mud_log("ERROR: could not save intro file, %s, to filename, %s.", GET_NAME(ch),
        fname);
    return;
  }

  while (x < count) {
    if (x == 0 || strcasecmp(names[x - 1], names[x])) {
      if (strcasecmp(names[x], GET_NAME(vict))) {
        fprintf(fl, "%s %s\n", names[x], alias[x]);
      }
    }
    x++;
  }

  x = 0;
  while (x < count) {
    if (names[x] != NULL) {
      free(names[x]);
    }
    if (alias[x] != NULL) {
      free(alias[x]);
    }
    x++;
  }

  fprintf(fl, "%s %s\n", GET_NAME(vict), CAP(name));

  fclose(fl);
  return;
}

ACMD(do_intro) {

  if (IS_NPC(ch))
    return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct char_data *vict;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: dub (target) (name)\r\nWho do you want to dub "
                     "and what do you want to name them?\r\n");
    return;
  }

  if (!*arg2) {
    send_to_char(ch, "Syntax: dub (target) (name)\r\nWhat name do you wish to "
                     "know them by?\r\n");
    return;
  }

  if (strlen(arg2) > 20) {
    send_to_char(ch, "Limit the name to 20 characters.\r\n");
    return;
  }
  if (strlen(arg2) < 3) {
    send_to_char(ch, "Limit the name to at least 3 characters.\r\n");
    return;
  }

  if (strstr(arg2, "$") || strstr(arg2, "@") || strstr(arg2, "%")) {
    send_to_char(ch, "Illegal character. No symbols.\r\n");
    return;
  }

  if (!(vict = get_player_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person.\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(ch, "That seems rather odd.\r\n");
    return;
  }

  if (IS_NPC(vict)) {
    send_to_char(ch, "That seems rather unwise.\r\n");
    return;
  }

  if (readIntro(vict, ch) == 2) {
    send_to_char(
        ch, "There seems to have been an error, report this to Iovan.\r\n");
    return;
  } else if (readIntro(ch, vict) == 1 && strstr(RACE(vict), arg)) {
    send_to_char(ch, "You have already dubbed them a name. If you want to "
                     "redub them target the name you know them by.\r\n");
    return;
  } else {
    introWrite(ch, vict, arg2);
    act("You decide to call $M, $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n seems to decide something about you.", TRUE, ch, 0, vict, TO_VICT);
    act("$n seems to decide something about $N.", TRUE, ch, 0, vict,
        TO_NOTVICT);
    return;
  }
}

/* Used when checking status or looking at a character */
static void bringdesc(struct char_data *ch, struct char_data *tch) {

  if (ch != NULL && tch != NULL && IS_HUMANOID(tch)) {

    if (ch != tch && PLR_FLAGGED(tch, PLR_DISGUISED)) {
      send_to_char(
          ch, "            @D[@cHair Length @D: @WHidden.         @D]@n\r\n");
      send_to_char(
          ch, "            @D[@cHair Color  @D: @WHidden.         @D]@n\r\n");
      send_to_char(
          ch, "            @D[@cHair Style  @D: @WHidden.         @D]@n\r\n");
      send_to_char(
          ch, "            @D[@cEye Color   @D: @WHidden.         @D]@n\r\n");
      if (GET_SKIN(tch) == SKIN_WHITE) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_TAN) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_BLACK) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_GREEN) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_ORANGE) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_YELLOW) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_RED) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_GREY) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_BLUE) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_AQUA) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_PINK) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
      } else if (GET_SKIN(tch) == SKIN_PURPLE) {
        send_to_char(
            ch, "            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
      }
      return;
    }

    if (IS_HUMAN(tch) || IS_SAIYAN(tch) || IS_KONATSU(tch) || IS_MUTANT(tch) ||
        IS_ANDROID(tch) || IS_KAI(tch) || IS_HALFBREED(tch) ||
        IS_TRUFFLE(tch) || IS_HOSHIJIN(tch)) {
      if ((!IS_SAIYAN(tch) && !IS_HALFBREED(tch)) ||
          ((IS_SAIYAN(tch) || IS_HALFBREED(tch)) && !IS_TRANSFORMED(tch))) {
        if (GET_HAIRL(tch) == HAIRL_LONG) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
        } else if (GET_HAIRL(tch) == HAIRL_BALD) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
        } else if (GET_HAIRL(tch) == HAIRL_SHORT) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
        } else if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
        } else if (GET_HAIRL(tch) == HAIRL_RLONG) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
        }
        if (GET_HAIRS(tch) == HAIRS_PLAIN) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WPlain.        @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_MOHAWK) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WMohawk.       @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_SPIKY) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_CURLY) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WCurly.        @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_UNEVEN) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WUneven.       @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_PONYTAIL) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WPony Tail.    @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_AFRO) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WAfro.         @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_FADE) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WFade.         @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_CREW) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WCrew Cut.     @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_FEATHERED) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WFeathered.    @D]@n\r\n");
        } else if (GET_HAIRS(tch) == HAIRS_DRED) {
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WDread Locks.  @D]@n\r\n");
        }
        if (GET_HAIRC(tch) == HAIRC_BLACK) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WBlack.        @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_BROWN) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WBrown.        @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_BLONDE) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WBlonde.       @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_GREY) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WGrey.         @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_RED) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WRed.          @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_ORANGE) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WOrange.       @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_GREEN) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WGreen.        @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_BLUE) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WBlue.         @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_PINK) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WPink.         @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_PURPLE) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WPurple.       @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_SILVER) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WSilver.       @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_CRIMSON) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WCrimson.      @D]@n\r\n");
        } else if (GET_HAIRC(tch) == HAIRC_WHITE) {
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WWhite.        @D]@n\r\n");
        }
      } else if (IS_SAIYAN(tch) || IS_HALFBREED(tch)) {
        if (PLR_FLAGGED(tch, PLR_TRANS1)) {
          if (GET_HAIRL(tch) == HAIRL_LONG) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_BALD) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_SHORT) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_RLONG) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
          }
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
        } else if (PLR_FLAGGED(tch, PLR_TRANS2)) {
          if (GET_HAIRL(tch) == HAIRL_LONG) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_BALD) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_SHORT) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
          } else if (GET_HAIRL(tch) == HAIRL_RLONG) {
            send_to_char(
                ch,
                "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
          }
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WSharp Spikes. @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
        } else if (PLR_FLAGGED(tch, PLR_TRANS3)) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cEye Color   @D: @WAqua Green.   @D]@n\r\n");
        } else if (PLR_FLAGGED(tch, PLR_TRANS4)) {
          send_to_char(
              ch, "            @D[@cHair Length @D: @WLong.        @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cHair Style  @D: @WSoft Spikes. @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cHair Color  @D: @WBlack.       @D]@n\r\n");
          send_to_char(
              ch, "            @D[@cEye Color   @D: @WAmber.       @D]@n\r\n");
        }
      }
    }
    if (IS_DEMON(tch) || IS_ICER(tch)) {
      if (GET_HAIRL(tch) == HAIRL_BALD) {
        send_to_char(
            ch, "            @D[@cHorn Length @D: @WNone.         @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_SHORT) {
        send_to_char(
            ch, "            @D[@cHorn Length @D: @WShort.        @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
        send_to_char(
            ch, "            @D[@cHorn Length @D: @WMedium.       @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_LONG) {
        send_to_char(
            ch, "            @D[@cHorn Length @D: @WLong.         @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_RLONG) {
        send_to_char(
            ch, "            @D[@cHorn Length @D: @WReally Long.  @D]@n\r\n");
      }
    }
    if (IS_NAMEK(tch) || IS_ARLIAN(tch)) {
      if (GET_HAIRL(tch) == HAIRL_BALD) {
        send_to_char(
            ch, "            @D[@cAnt. Length @D: @WTiny.        @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_SHORT) {
        send_to_char(
            ch, "            @D[@cAnt. Length @D: @WShort.       @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
        send_to_char(
            ch, "            @D[@cAnt. Length @D: @WMedium.      @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_LONG) {
        send_to_char(
            ch, "            @D[@cAnt. Length @D: @WLong.        @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_RLONG) {
        send_to_char(
            ch, "            @D[@cAnt. Length @D: @WR. Long.     @D]@n\r\n");
      }
    }
    if (IS_ARLIAN(tch) && IS_FEMALE(tch)) {
      if (GET_HAIRC(tch) == HAIRC_BLACK) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WBlack.        @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_BROWN) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WBrown.        @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_BLONDE) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WBlonde.       @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_GREY) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WGrey.         @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_RED) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WRed.          @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_ORANGE) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WOrange.       @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_GREEN) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WGreen.        @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_BLUE) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WBlue.         @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_PINK) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WPink.         @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_PURPLE) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WPurple.       @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_SILVER) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WSilver.       @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_CRIMSON) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WCrimson.      @D]@n\r\n");
      } else if (GET_HAIRC(tch) == HAIRC_WHITE) {
        send_to_char(
            ch, "            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
      }
    } else if (IS_ARLIAN(tch) && !IS_FEMALE(tch)) {
      send_to_char(
          ch, "            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
    }
    if (IS_MAJIN(tch)) {
      if (GET_HAIRL(tch) == HAIRL_BALD) {
        send_to_char(
            ch, "            @D[@cFor. Length @D: @WTiny.         @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_SHORT) {
        send_to_char(
            ch, "            @D[@cFor. Length @D: @WShort.        @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
        send_to_char(
            ch, "            @D[@cFor. Length @D: @WMedium.       @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_LONG) {
        send_to_char(
            ch, "            @D[@cFor. Length @D: @WLong.         @D]@n\r\n");
      }
      if (GET_HAIRL(tch) == HAIRL_RLONG) {
        send_to_char(
            ch, "            @D[@cFor. Length @D: @WR. Long.      @D]@n\r\n");
      }
    }
    if ((!IS_SAIYAN(tch) && !IS_HALFBREED(tch)) ||
        ((IS_SAIYAN(tch) || IS_HALFBREED(tch)) && !IS_TRANSFORMED(tch))) {
      if (GET_EYE(tch) == EYE_BLUE) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WBlue.         @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_BLACK) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WBlack.        @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_GREEN) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WGreen.        @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_BROWN) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WBrown.        @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_RED) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WRed.          @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_AQUA) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WAqua.         @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_PINK) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WPink.         @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_PURPLE) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WPurple.       @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_CRIMSON) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WCrimson.      @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_GOLD) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WGold.         @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_AMBER) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WAmber.        @D]@n\r\n");
      } else if (GET_EYE(tch) == EYE_EMERALD) {
        send_to_char(
            ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
      }
    }
    if (GET_SKIN(tch) == SKIN_WHITE) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_TAN) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_BLACK) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_GREEN) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_ORANGE) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_YELLOW) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_RED) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_GREY) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_BLUE) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_AQUA) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_PINK) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
    } else if (GET_SKIN(tch) == SKIN_PURPLE) {
      send_to_char(
          ch, "            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
    }
    if (char_condition_has(tch, "majinized") != 0 && char_condition_number_get(tch, "majinized", "lord") != 3) {
      send_to_char(
          ch, "            @D[@cForehead    @D: @mMajin Symbol  @D]@n\r\n");
    }
  } else if (!IS_HUMANOID(tch)) {
    /* Display nothing */
    return;
  } else {
    send_to_char(ch, "Error in bring-desc, please report.\r\n");
  }
}

static char sect_to_map_char(int sect, int geffect, bool sunken) {
  if (sunken) return '=';
  if (geffect >= 1) {
    switch (sect) {
    case SECT_INSIDE:   return '2';
    case SECT_FIELD:    return '2';
    case SECT_DESERT:   return '7';
    case SECT_CITY:     return '1';
    case SECT_FOREST:   return '6';
    case SECT_MOUNTAIN: return '5';
    case SECT_HILLS:    return '3';
    default: break;
    }
  }
  switch (sect) {
  case SECT_INSIDE:       return 'i';
  case SECT_FIELD:        return 'p';
  case SECT_DESERT:       return '!';
  case SECT_CITY:         return '(';
  case SECT_FOREST:       return 'f';
  case SECT_MOUNTAIN:     return '^';
  case SECT_HILLS:        return 'h';
  case SECT_FLYING:       return 's';
  case SECT_WATER_NOSWIM: return '`';
  case SECT_WATER_SWIM:   return '+';
  case SECT_SHOP:         return '&';
  case SECT_IMPORTANT:    return '*';
  default:                return '-';
  }
}

static void map_draw_room(char map[9][10], int x, int y, struct room_data *room,
                          struct char_data *ch) {
  static const struct { int door; int dy; int dx; } dir_offsets[] = {
    {NORTH,     -1,  0},
    {EAST,       0, +1},
    {SOUTH,     +1,  0},
    {WEST,       0, -1},
    {NORTHEAST, -1, +1},
    {NORTHWEST, -1, -1},
    {SOUTHEAST, +1, +1},
    {SOUTHWEST, +1, -1},
  };

  room_exits_iterate(room, [&](auto door, auto exit) {
    auto dest = exit_dest_get(exit);
    if (!dest) return true;

    int dy = 0, dx = 0;
    for (auto &e : dir_offsets) {
      if (e.door == door) { dy = e.dy; dx = e.dx; break; }
    }

    if (exit_flagged(exit, EX_CLOSED) && !exit_flagged(exit, EX_SECRET)) {
      map[y + dy][x + dx] = '8';
    } else if (!exit_flagged(exit, EX_CLOSED)) {
      map[y + dy][x + dx] = sect_to_map_char(
          room_sector_type_get(dest),
          room_geffect_get(dest),
          room_is_sunken(dest));
    }
    return true;
  });
}

ACMD(do_map) { gen_map(ch, 1); }

static void gen_map(struct char_data *ch, int num) {
  int door, i;
  char map[9][10] = {{'-'}, {'-'}};
  char buf2[MAX_INPUT_LENGTH];

  if (num == 1) {
    /* Map Key */
    send_to_char(ch, "@W               @D-[@CArea Map@D]-\r\n");
    send_to_char(ch, "@D-------------------------------------------@w\r\n");
    send_to_char(
        ch, "@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
    send_to_char(
        ch, "@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
    send_to_char(ch,
                 "@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
    send_to_char(ch,
                 "@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
    send_to_char(ch, "@WLastly @RX@W = You.\r\n");
    send_to_char(ch, "@D-------------------------------------------\r\n");
    send_to_char(ch, "@D                  @CNorth@w\r\n");
    send_to_char(ch, "@D                    @c^@w\r\n");
    send_to_char(ch, "@D             @CWest @c< O > @CEast@w\r\n");
    send_to_char(ch, "@D                    @cv@w\r\n");
    send_to_char(ch, "@D                  @CSouth@w\r\n");
    send_to_char(ch, "@D                ---------@w\r\n");
  }

  /* blank the map */
  for (i = 0; i < 9; i++) {
    strcpy(map[i], "         ");
  }

  struct room_data *room = char_room_get(ch);

  /* print out exits */
  map_draw_room(map, 4, 4, room, ch);

  room_exits_iterate(room, [&](auto door, auto exit) {
    auto dest = char_can_go_exit(ch, exit);
    if(!dest) return true;
    switch (door) {
      case NORTH:
        map_draw_room(map, 4, 3, dest, ch);
        break;
      case EAST:
        map_draw_room(map, 5, 4, dest, ch);
        break;
      case SOUTH:
        map_draw_room(map, 4, 5, dest, ch);
        break;
      case WEST:
        map_draw_room(map, 3, 4, dest, ch);
        break;
      case NORTHEAST:
        map_draw_room(map, 5, 3, dest, ch);
        break;
      case NORTHWEST:
        map_draw_room(map, 3, 3, dest, ch);
        break;
      case SOUTHEAST:
        map_draw_room(map, 5, 5, dest, ch);
        break;
      case SOUTHWEST:
        map_draw_room(map, 3, 5, dest, ch);
        break;
      }

    return true;
  });

  /* make it obvious what room they are in */
  map[4][4] = 'x';

  /* print out the map */
  int key = 0;
  *buf2 = '\0';
  for (i = 2; i < 9; i++) {
    if (i > 6) {
      continue;
    }
    if (num == 1) {
      sprintf(buf2, "@w                %s\r\n", map[i]);
    } else {
      if (i == 2) {
        sprintf(
            buf2, "@w       @w|%s@w|           %s",
            (room_dir_option_get(room, 0) && !exit_flagged(room_dir_option_get(room, 0), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 0), EX_CLOSED) ? " @rN " : " @CN ")
                : "   ",
            map[i]);
      }
      if (i == 3) {
        sprintf(
            buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
            (room_dir_option_get(room, 6) && !exit_flagged(room_dir_option_get(room, 6), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 6), EX_CLOSED) ? " @rNW" : " @CNW")
                : "   ",
            (room_dir_option_get(room, 4) && !exit_flagged(room_dir_option_get(room, 4), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 4), EX_CLOSED) ? " @yU " : " @YU ")
                : "   ",
            (room_dir_option_get(room, 7) && !exit_flagged(room_dir_option_get(room, 7), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 7), EX_SECRET) ? "@rNE " : "@CNE ")
                : "   ",
            map[i]);
      }
      if (i == 4) {
        sprintf(
            buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
            (room_dir_option_get(room, 3) && !exit_flagged(room_dir_option_get(room, 3), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 3), EX_CLOSED) ? "  @rW" : "  @CW")
                : "   ",
            (room_dir_option_get(room, 10) && !exit_flagged(room_dir_option_get(room, 10), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 10), EX_CLOSED) ? " @rI "
                                                             : " @mI ")
                : ((room_dir_option_get(room, 11) &&
                    !exit_flagged(room_dir_option_get(room, 11), EX_SECRET))
                       ? (exit_flagged(room_dir_option_get(room, 11), EX_CLOSED) ? "@rOUT"
                                                                    : "@mOUT")
                       : "@r{ }"),
            (room_dir_option_get(room, 1) && !exit_flagged(room_dir_option_get(room, 1), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 1), EX_CLOSED) ? "@rE  " : "@CE  ")
                : "   ",
            map[i]);
      }
      if (i == 5) {
        sprintf(
            buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
            (room_dir_option_get(room, 9) && !exit_flagged(room_dir_option_get(room, 9), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 9), EX_CLOSED) ? " @rSW" : " @CSW")
                : "   ",
            (room_dir_option_get(room, 5) && !exit_flagged(room_dir_option_get(room, 5), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 5), EX_CLOSED) ? " @yD " : " @YD ")
                : "   ",
            (room_dir_option_get(room, 8) && !exit_flagged(room_dir_option_get(room, 8), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 8), EX_SECRET) ? "@rSE " : "@CSE ")
                : "   ",
            map[i]);
      }
      if (i == 6) {
        sprintf(
            buf2, "@w       @w|%s@w|           %s",
            (room_dir_option_get(room, 2) && !exit_flagged(room_dir_option_get(room, 2), EX_SECRET))
                ? (exit_flagged(room_dir_option_get(room, 2), EX_CLOSED) ? " @rS " : " @CS ")
                : "   ",
            map[i]);
      }
    }
    search_replace(buf2, "x", "@RX");
    search_replace(buf2, "&", "@m$");
    search_replace(buf2, "*", "@m#");
    search_replace(buf2, "+", "@c~");
    search_replace(buf2, "s", "@CS");
    search_replace(buf2, "i", "@wI");
    search_replace(buf2, "(", "@WC");
    search_replace(buf2, "^", "@DM");
    search_replace(buf2, "h", "@yH");
    search_replace(buf2, "`", "@BW");
    search_replace(buf2, "=", "@bU");
    search_replace(buf2, "p", "@GP");
    search_replace(buf2, "f", "@gF");
    search_replace(buf2, "!", "@YD");
    search_replace(buf2, "-", "@w:");
    /* ------- Do Lava Rooms ------- */
    search_replace(buf2, "1", "@4@YC@n");
    search_replace(buf2, "2", "@4@YP@n");
    search_replace(buf2, "3", "@4@YH@n");
    search_replace(buf2, "7", "@4@YD@n");
    search_replace(buf2, "5", "@4@YM@n");
    search_replace(buf2, "6", "@4@YF@n");
    /* ------- Do Closed Rooms------- */
    search_replace(buf2, "8", "@1 @n");

    if (num != 1) {
      if (key == 0) {
        send_to_char(ch, "%s    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n",
                     buf2);
      }
      if (key == 1) {
        send_to_char(ch,
                     "%s    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n",
                     buf2);
      }
      if (key == 2) {
        send_to_char(ch,
                     "%s    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n",
                     buf2);
      }
      if (key == 3) {
        send_to_char(ch,
                     "%s    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n",
                     buf2);
      }
      if (key == 4) {
        send_to_char(
            ch, "%s    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n",
            buf2);
      }
      key += 1;
    } else {
      send_to_char(ch, "%s", buf2);
    }
  }
  if (num == 1) {
    send_to_char(ch, "@D                ---------@w\r\n");
  }
}

static void display_spells(struct char_data *ch, struct obj_data *obj) {

  return;
}

static void display_scroll(struct char_data *ch, struct obj_data *obj) {
  send_to_char(ch, "The scroll contains the following spell:\r\n");
  send_to_char(
      ch, "@c---@wSpell "
          "Name@c---------------------------------------------------@n\r\n");
  send_to_char(ch, "@y%-20s@n\r\n",
               skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
  return;
}

static void show_obj_to_char(struct obj_data *obj, struct char_data *ch,
                             int mode) {
  if (!obj || !ch) {
    mud_log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    /*  SYSERR_DESC:
     *  Somehow a NULL pointer was sent to show_obj_to_char() in either the
     *  'obj' or the 'ch' variable.  The error will indicate which was NULL
     *  be listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger.
     */
    return;
  }

  int spotted = FALSE;

  if (GET_SKILL(ch, SKILL_SPOT) > rand_number(20, 110)) {
    spotted = TRUE;
  }

  struct room_data *oroom = NULL;

  switch (mode) {
  case SHOW_OBJ_LONG:
    /*
     * hide objects starting with . from non-holylighted people
     * Idea from Elaseth of TBA
     */
    if (*obj->description == '.' &&
        (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
      return;
    if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE &&
        char_room_vnum_get(ch) == GET_OBJ_VAL(obj, 0)) {
      return;
    }
    if (SITTING(obj) && GET_ADMLEVEL(ch) < 1) {
      return;
    }
    if (SITTING(obj) && GET_ADMLEVEL(ch) >= 1) {
      send_to_char(ch, "@D(@YBeing Used@D)@w");
    }
    oroom = obj_room_get(obj);
    if (GET_OBJ_TYPE(obj) == ITEM_PLANT &&
        (room_flagged(oroom, ROOM_GARDEN1) ||
         room_flagged(oroom, ROOM_GARDEN2))) {
      see_plant(obj, ch);
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BURIED)) {
      char bury[MAX_INPUT_LENGTH];
      if (!IS_CORPSE(obj)) {
        if (GET_OBJ_WEIGHT(obj) < 10) {
          sprintf(bury, "small mound of");
        } else if (GET_OBJ_WEIGHT(obj) < 50) {
          sprintf(bury, "medium sized mound of");
        } else if (GET_OBJ_WEIGHT(obj) < 1000) {
          sprintf(bury, "large mound of");
        } else {
          sprintf(bury, "gigantic mound of");
        }
      } else {
        sprintf(bury, "recent grave covered by");
      }
      int sect = room_sector_type_get(obj_room_get(obj));
      if (spotted == TRUE && sect != SECT_DESERT) {
        send_to_char(ch, "@yA %s soft dirt is here.@n\r\n", bury);
        return;
      } else if (spotted == TRUE && sect == SECT_DESERT) {
        send_to_char(ch, "@YA %s soft sand is here.@n\r\n", bury);
        return;
      } else {
        return;
      }
    }
    if (GET_OBJ_VNUM(obj) == 11) {
      send_to_char(ch,
                   "@wA gravity generator, set to %sx gravity, is built here",
                   add_commas(GET_OBJ_WEIGHT(obj)));
    } else if (GET_OBJ_VNUM(obj) == 79) {
      send_to_char(ch,
                   "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is "
                   "blocking access to the @G%s@w direction",
                   add_commas(GET_OBJ_WEIGHT(obj)), dirs[GET_OBJ_COST(obj)]);
    } else {
      send_to_char(ch, "@w");
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
        if (GET_OBJ_POSTED(obj) == NULL) {
          send_to_char(ch, "@D[@G%d@D]@w ", GET_OBJ_VNUM(obj));
          if (SCRIPT(obj))
            send_to_char(ch, "@D[@wT%d@D]@w ", obj->proto_script->id);
        } else {
          if (GET_OBJ_POSTTYPE(obj) <= 0) {
            send_to_char(ch, "@D[@G%d@D]@w ", GET_OBJ_VNUM(obj));
            if (SCRIPT(obj))
              send_to_char(ch, "@D[@wT%d@D]@w ", obj->proto_script->id);
          }
        }
      }

      if (GET_OBJ_POSTTYPE(obj) > 0) {
        if (GET_OBJ_POSTED(obj)) {
          return;
        } else {
          send_to_char(ch, "%s@w, has been posted here.@n",
                       obj->short_description);
        }
      } else {
        if (!OBJ_FLAGGED(obj, ITEM_BURIED)) {
          send_to_char(ch, "%s@n", obj->description);
        }
      }

      if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
        if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && GET_OBJ_VNUM(obj) > 19199)
          send_to_char(ch, "\r\n@c...its outer hatch is open@n");
        else if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) &&
                 GET_OBJ_VNUM(obj) <= 19199)
          send_to_char(ch, "\r\n@c...its door is open@n");
      }
      if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !IS_CORPSE(obj)) {
        if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
          send_to_char(ch, ". @D[@G-open-@D]@n");
        else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
          send_to_char(ch, ". @D[@rclosed@D]@n");
      }
      if (GET_OBJ_TYPE(obj) == ITEM_HATCH) {
        if (!OBJVAL_FLAGGED(obj, CONT_CLOSED))
          send_to_char(ch, ", it is open");
        else if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
          send_to_char(ch, ", it is closed");
        if (OBJVAL_FLAGGED(obj, CONT_LOCKED))
          send_to_char(ch, " and locked@n");
        else
          send_to_char(ch, "@n");
      }
      if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
        if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
          send_to_char(ch, ", and it has been ate on@n");
        }
      }
    }
    break;

  case SHOW_OBJ_SHORT:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
      send_to_char(ch, "[%d] ", GET_OBJ_VNUM(obj));
      if (SCRIPT(obj))
        send_to_char(ch, "[T%d] ", obj->proto_script->id);
    }

    if (PRF_FLAGGED(ch, PRF_IHEALTH)) {
      send_to_char(ch, "@D<@gH@D: @C%d@D>@w %s",
                   GET_OBJ_VAL(obj, VAL_ALL_HEALTH), obj->short_description);
    } else {
      send_to_char(ch, "%s", obj->short_description);
    }
    if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
      if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
        send_to_char(ch, ", and it has been ate on.@n");
      }
    }
    if (GET_OBJ_VNUM(obj) == 255) {
      switch (GET_OBJ_VAL(obj, 0)) {
      case 0:
      case 1:
        send_to_char(ch, " @D[@wQuality @RC@D]@n");
        break;
      case 2:
        send_to_char(ch, " @D[@wQuality @RC+@D]@n");
        break;
      case 3:
        send_to_char(ch, " @D[@wQuality @yC++@D]@n");
        break;
      case 4:
        send_to_char(ch, " @D[@wQuality @yB@D]@n");
        break;
      case 5:
        send_to_char(ch, " @D[@wQuality @CB+@D]@n");
        break;
      case 6:
        send_to_char(ch, " @D[@wQuality @CB++@D]@n");
        break;
      case 7:
        send_to_char(ch, " @D[@wQuality @CA@D]@n");
        break;
      case 8:
        send_to_char(ch, " @D[@wQuality @GA+@D]@n");
        break;
      }
    }

    if (GET_OBJ_VNUM(obj) == 3424) {
      send_to_char(ch, " @D[@bInk Remaining@D: @w%d@D]@n", GET_OBJ_VAL(obj, 6));
    }
    if (GET_OBJ_VNUM(obj) == 3423) {
      send_to_char(ch, " @D[@B%d@D/@B24 Inks@D]@n", GET_OBJ_VAL(obj, 6));
    }
    if (OBJ_FLAGGED(obj, ITEM_THROW)) {
      send_to_char(ch, " @D[@RThrow Only@D]@n");
    }
    if (GET_OBJ_TYPE(obj) == ITEM_PLANT && !OBJ_FLAGGED(obj, ITEM_MATURE)) {
      if (GET_OBJ_VAL(obj, VAL_WATERLEVEL) < -9) {
        send_to_char(ch, "@D[@RDead@D]@n");
      } else {
        switch (GET_OBJ_VAL(obj, VAL_MATURITY)) {
        case 0:
          send_to_char(ch, " @D[@ySeed@D]@n");
          break;
        case 1:
          send_to_char(ch, " @D[@GSprout@D]@n");
          break;
        case 2:
          send_to_char(ch, " @D[@GYoung@D]@n");
          break;
        case 3:
          send_to_char(ch, " @D[@GMature@D]@n");
          break;
        case 4:
          send_to_char(ch, " @D[@GBudding@D]@n");
          break;
        case 5:
          send_to_char(ch, "@D[@GClose Harvest@D]@n");
          break;
        case 6:
          send_to_char(ch, "@D[@gHarvest@D]@n");
          break;
        }
      }
    }
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !IS_CORPSE(obj)) {
      if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
        send_to_char(ch, " @D[@G-open-@D]@n");
      else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
        send_to_char(ch, " @D[@rclosed@D]@n");
    }
    if (OBJ_FLAGGED(obj, ITEM_DUPLICATE)) {
      send_to_char(ch, " @D[@YDuplicate@D]@n");
    }
    break;

  case SHOW_OBJ_ACTION:
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_NOTE:
      if (obj->action_description) {
        char notebuf[MAX_NOTE_LENGTH];

        snprintf(notebuf, sizeof(notebuf),
                 "There is something written on it:\r\n\r\n%s",
                 obj->action_description);
        send_to_char(ch, "%s", notebuf);
      } else
        send_to_char(ch, "There appears to be nothing written on it.\r\n");
      return;

    case ITEM_BOARD:
      show_board(GET_OBJ_VNUM(obj), ch);
      break;

    case ITEM_CONTROL:
      send_to_char(ch, "@RFUEL@D: %s%s@n\r\n",
                   GET_FUEL(obj) >= 200   ? "@G"
                   : GET_FUEL(obj) >= 100 ? "@Y"
                                          : "@r",
                   add_commas(GET_FUEL(obj)));
      break;

    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.\r\n");
      break;

    case ITEM_LIGHT:
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) == -1)
        send_to_char(ch, "Light Cycles left: Infinite\r\n");
      else
        send_to_char(ch, "Light Cycles left: [%d]\r\n",
                     GET_OBJ_VAL(obj, VAL_LIGHT_HOURS));
      break;

    case ITEM_FOOD:
      if (FOOB(obj) >= 4) {
        if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj) / 4) {
          send_to_char(ch, "Condition of the food: Almost gone.\r\n");
        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj) / 2) {
          send_to_char(ch, "Condition of the food: Half Eaten.");
        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
          send_to_char(ch, "Condition of the food: Partially Eaten.");
        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) == FOOB(obj)) {
          send_to_char(ch, "Condition of the food: Whole.");
        }
      } else if (FOOB(obj) > 0) {
        if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
          send_to_char(ch, "Condition of the food: Almost gone.");
        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) == FOOB(obj)) {
          send_to_char(ch, "Condition of the food: Whole.");
        }
      } else {
        send_to_char(ch, "Condition of the food: Insignificant.");
      }
      break;

    case ITEM_SPELLBOOK:
      send_to_char(ch, "It looks like an arcane tome.\r\n");
      display_spells(ch, obj);
      break;

    case ITEM_SCROLL:
      send_to_char(ch, "It looks like an arcane scroll.\r\n");
      display_scroll(ch, obj);
      break;

    case ITEM_VEHICLE:
      if (GET_OBJ_VNUM(obj) > 19199) {
        send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
        send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
        send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
        send_to_char(ch, "@YSyntax@D: @CEnter hatch\r\n");
      } else {
        send_to_char(ch, "@YSyntax@D: @CUnlock door\r\n");
        send_to_char(ch, "@YSyntax@D: @COpen door\r\n");
        send_to_char(ch, "@YSyntax@D: @CClose door\r\n");
        send_to_char(ch, "@YSyntax@D: @CEnter door\r\n");
      }
      break;

    case ITEM_HATCH:
      if (GET_OBJ_VNUM(obj) > 19199) {
        send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
        send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
        send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
        send_to_char(ch, "@YSyntax@D: @CLeave@n\r\n");
      } else {
        send_to_char(ch, "@YSyntax@D: @CUnlock door\r\n");
        send_to_char(ch, "@YSyntax@D: @COpen door\r\n");
        send_to_char(ch, "@YSyntax@D: @CClose door\r\n");
        send_to_char(ch, "@YSyntax@D: @CEnter door\r\n");
      }
      break;

    case ITEM_WINDOW:
      look_out_window(ch, obj->name);
      return;
      break;

    default:
      if (!IS_CORPSE(obj)) {
        send_to_char(ch, "You see nothing special..\r\n");
      } else {
        int mention = FALSE;
        send_to_char(ch, "This corpse has ");

        if (GET_OBJ_VAL(obj, VAL_CORPSE_HEAD) == 0) {
          send_to_char(ch, "no head,");
          mention = TRUE;
        }

        if (GET_OBJ_VAL(obj, VAL_CORPSE_RARM) == 0) {
          send_to_char(ch, "no right arm, ");
          mention = TRUE;
        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_RARM) == 2) {
          send_to_char(ch, "a broken right arm, ");
          mention = TRUE;
        }

        if (GET_OBJ_VAL(obj, VAL_CORPSE_LARM) == 0) {
          send_to_char(ch, "no left arm, ");
          mention = TRUE;
        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_LARM) == 2) {
          send_to_char(ch, "a broken left arm, ");
          mention = TRUE;
        }

        if (GET_OBJ_VAL(obj, VAL_CORPSE_RLEG) == 0) {
          send_to_char(ch, "no right leg, ");
          mention = TRUE;
        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_RLEG) == 2) {
          send_to_char(ch, "a broken right leg, ");
          mention = TRUE;
        }

        if (GET_OBJ_VAL(obj, VAL_CORPSE_LLEG) == 0) {
          send_to_char(ch, "no left leg, ");
          mention = TRUE;
        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_LLEG) == 2) {
          send_to_char(ch, "a broken left leg, ");
          mention = TRUE;
        }

        if (mention == FALSE) {
          send_to_char(ch, "nothing missing from it but life.");
        } else {
          send_to_char(ch, "and is dead.");
        }

        send_to_char(ch, "\r\n");
      }
      break;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
      int num = 0;
      if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
        num = 1;
      } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                 TYPE_SLASH - TYPE_HIT) {
        num = 0;
      } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                 TYPE_CRUSH - TYPE_HIT) {
        num = 3;
      } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
        num = 2;
      } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                 TYPE_BLAST - TYPE_HIT) {
        num = 4;
      } else {
        num = 5;
      }
      send_to_char(ch, "The weapon type of %s@n is '%s'.\r\n",
                   GET_OBJ_SHORT(obj), weapon_disp[num]);
      send_to_char(ch, "You could wield it %s.\r\n",
                   wield_names[wield_type(get_size(ch), obj)]);
    }
    diag_obj_to_char(obj, ch);
    send_to_char(ch, "It appears to be made of %s, and weighs %s",
                 material_names[GET_OBJ_MATERIAL(obj)],
                 add_commas(GET_OBJ_WEIGHT(obj)));
    break;

  default:
    mud_log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    /*  SYSERR_DESC:
     *  show_obj_to_char() has some predefined 'mode's (argument #3) to tell
     *  it what to display to the character when it is called.  If the mode
     *  is not one of these, it will output this error, and indicate what
     *  mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode.
     */
    return;
  }

  if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
    send_to_char(ch, "\r\n");
}

static int show_obj_modifiers(struct obj_data *obj, struct char_data *ch) {
  int found = FALSE;

  if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
    send_to_char(ch, " (invisible)");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    send_to_char(ch, " ..It glows blue!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
    send_to_char(ch, " ..It glows yellow!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
    send_to_char(ch, " @D(@GGlowing@D)@n");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_HOT)) {
    send_to_char(ch, " @D(@RHOT@D)@n");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_HUM)) {
    send_to_char(ch, " @D(@RHumming@D)@n");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_SLOT2)) {
    if (OBJ_FLAGGED(obj, ITEM_SLOT_ONE) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
      send_to_char(ch, " @D[@m1/2 Tokens@D]@n");
    else if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
      send_to_char(ch, " @D[@m2/2 Tokens@D]@n");
    else
      send_to_char(ch, " @D[@m0/2 Tokens@D]@n");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_SLOT1)) {
    if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
      send_to_char(ch, " @D[@m1/1 Tokens@D]@n");
    else
      send_to_char(ch, " @D[@m0/1 Tokens@D]@n");
    found++;
  }
  if (KICHARGE(obj) > 0) {
    int num = (KIDIST(obj) * 20) + rand_number(1, 5);
    send_to_char(ch, " %d meters away", num);
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
    send_to_char(ch, " @D(@YCUSTOM@D)@n");
  }
  if (OBJ_FLAGGED(obj, ITEM_RESTRING)) {
    send_to_char(ch, " @D(@R%s@D)@n", GET_ADMLEVEL(ch) > 0 ? obj->name : "*");
  }
  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL ||
        GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL ||
        GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
      send_to_char(ch, ", and appears to be twisted and broken.");
    } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
      send_to_char(ch, ", and is broken into hundreds of splinters.");
    } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
      send_to_char(ch, ", and is shattered on the ground.");
    } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
      send_to_char(ch, ", and is a pile of rubble.");
    } else {
      send_to_char(ch, ", and is broken.");
    }
    found++;
  } else {
    if (GET_OBJ_TYPE(obj) != ITEM_BOARD) {
      if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
        send_to_char(ch, ".");
      }
      if (!IS_NPC(ch) && GET_OBJ_POSTED(obj) && GET_OBJ_POSTTYPE(obj) <= 0) {
        struct obj_data *obj2 = GET_OBJ_POSTED(obj);
        char dvnum[200];
        *dvnum = '\0';
        sprintf(dvnum, "@D[@G%d@D] @w", GET_OBJ_VNUM(obj2));
        send_to_char(ch, "\n...%s%s has been posted to it.",
                     PRF_FLAGGED(ch, PRF_ROOMFLAGS) ? dvnum : "",
                     obj2->short_description);
      }
    }
    found++;
  }
  return (found);
}

static void list_obj_to_char(struct inventory_data list, struct char_data *ch,
                             int mode, bool show) {
  bool found = FALSE;

  /* Build a snapshot of the object list from Zig maps */
  std::vector<struct obj_data *> objs;
  switch (list.entity_type) {
  case ENT_CHAR:
    char_inventory_iterate(list.entity.ch, [&](struct obj_data *o) {
      objs.push_back(o); return true;
    });
    break;
  case ENT_OBJ:
    obj_contents_iterate(list.entity.obj, [&](struct obj_data *o) {
      objs.push_back(o); return true;
    });
    break;
  case ENT_ROOM:
    room_contents_iterate(list.entity.room, [&](struct obj_data *o) {
      objs.push_back(o); return true;
    });
    break;
  }

  for (size_t ii = 0; ii < objs.size(); ii++) {
    auto i = objs[ii];
    if (i->description == NULL)
      continue;
    if (strcasecmp(i->description, "undefined") == 0)
      continue;
    int num = 0;
    struct obj_data *d = i;
    if (CONFIG_STACK_OBJS) {
      /* Check if any earlier object in the snapshot matches i */
      size_t jj;
      for (jj = 0; jj < ii; jj++) {
        auto j = objs[jj];
        if ((!strcasecmp(j->short_description, i->short_description) &&
             !strcasecmp(j->description, i->description)) &&
            (j->proto_id == i->proto_id) &&
            ((OBJ_FLAGGED(j, ITEM_BROKEN) && OBJ_FLAGGED(i, ITEM_BROKEN)) ||
             (!OBJ_FLAGGED(j, ITEM_BROKEN) && !OBJ_FLAGGED(i, ITEM_BROKEN))))
          if ((!SITTING(j) && !SITTING(i)))
            if (GET_OBJ_VAL(j, 6) == GET_OBJ_VAL(i, 6))
              if ((GET_OBJ_TYPE(j) != ITEM_PLANT &&
                   GET_OBJ_TYPE(i) != ITEM_PLANT) ||
                  (GET_OBJ_TYPE(j) == ITEM_PLANT &&
                   GET_OBJ_TYPE(i) == ITEM_PLANT &&
                   GET_OBJ_VAL(j, VAL_MATURITY) ==
                       GET_OBJ_VAL(i, VAL_MATURITY) &&
                   GET_OBJ_VAL(j, VAL_WATERLEVEL) ==
                       GET_OBJ_VAL(i, VAL_WATERLEVEL)))
                if ((!OBJ_FLAGGED(j, ITEM_DUPLICATE) &&
                     !OBJ_FLAGGED(i, ITEM_DUPLICATE)) ||
                    (OBJ_FLAGGED(j, ITEM_DUPLICATE) &&
                     OBJ_FLAGGED(i, ITEM_DUPLICATE)))
                  if (GET_OBJ_POSTTYPE(j) == 0 && GET_OBJ_POSTTYPE(i) == 0)
                    if (!GET_FELLOW_WALL(j) && !GET_FELLOW_WALL(i))
                      if ((GET_OBJ_VAL(j, 0) == GET_OBJ_VAL(i, 0) &&
                           GET_OBJ_VNUM(j) == 255 && GET_OBJ_VNUM(i) == 255) ||
                          (GET_OBJ_VNUM(j) != 255 && GET_OBJ_VNUM(i) != 255))
                        break;
      }
      if (jj < ii)  /* Earlier match found — this obj was already counted */
        continue;
      /* Count matching objects from position ii forward */
      for (size_t kk = ii; kk < objs.size(); kk++) {
        auto j = objs[kk];
        if ((!strcasecmp(j->short_description, i->short_description) &&
             !strcasecmp(j->description, i->description)) &&
            (j->proto_id == i->proto_id) &&
            ((OBJ_FLAGGED(j, ITEM_BROKEN) && OBJ_FLAGGED(i, ITEM_BROKEN)) ||
             (!OBJ_FLAGGED(j, ITEM_BROKEN) && !OBJ_FLAGGED(i, ITEM_BROKEN))))
          if ((!SITTING(j) && !SITTING(i)))
            if (GET_OBJ_POSTTYPE(j) == 0 && GET_OBJ_POSTTYPE(i) == 0)
              if (GET_OBJ_VAL(j, 6) == GET_OBJ_VAL(i, 6))
                if ((GET_OBJ_TYPE(j) != ITEM_PLANT &&
                     GET_OBJ_TYPE(i) != ITEM_PLANT) ||
                    (GET_OBJ_TYPE(j) == ITEM_PLANT &&
                     GET_OBJ_TYPE(i) == ITEM_PLANT &&
                     GET_OBJ_VAL(j, VAL_MATURITY) ==
                         GET_OBJ_VAL(i, VAL_MATURITY) &&
                     GET_OBJ_VAL(j, VAL_WATERLEVEL) ==
                         GET_OBJ_VAL(i, VAL_WATERLEVEL)))
                  if ((!OBJ_FLAGGED(j, ITEM_DUPLICATE) &&
                       !OBJ_FLAGGED(i, ITEM_DUPLICATE)) ||
                      (OBJ_FLAGGED(j, ITEM_DUPLICATE) &&
                       OBJ_FLAGGED(i, ITEM_DUPLICATE)))
                    if (!GET_FELLOW_WALL(j) && !GET_FELLOW_WALL(i))
                      if ((GET_OBJ_VAL(j, 0) == GET_OBJ_VAL(i, 0) &&
                           GET_OBJ_VNUM(i) == 255 && GET_OBJ_VNUM(j) == 255) ||
                          (GET_OBJ_VNUM(j) != 255 && GET_OBJ_VNUM(i) != 255))
                        if (CAN_SEE_OBJ(ch, j)) {
                          num++;
                          if (d == i && !CAN_SEE_OBJ(ch, d))
                            d = j;
                        }
      }
    }
    if ((CAN_SEE_OBJ(ch, d) &&
         ((*d->description != '.' && *d->short_description != '.') ||
          PRF_FLAGGED(ch, PRF_HOLYLIGHT))) ||
        (GET_OBJ_TYPE(d) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(ch, "@D(@Rx@Y%2i@D)@n ", num);
      show_obj_to_char(d, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
    send_to_char(ch, " Nothing.\r\n");
}

static void diag_obj_to_char(struct obj_data *obj, struct char_data *ch) {
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
      {100, "is in excellent condition."},
      {90, "has a few scuffs."},
      {75, "has some small scuffs and scratches."},
      {50, "has quite a few scratches."},
      {30, "has some big nasty scrapes and scratches."},
      {15, "looks pretty damaged."},
      {0, "is in awful condition."},
      {-1, "is in need of repair."},
  };
  int percent, ar_index;
  const char *objs = OBJS(obj, ch);

  if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) /
              GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0; /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "\r\n%c%s %s\r\n", UPPER(*objs), objs + 1,
               diagnosis[ar_index].text);
}

static void diag_char_to_char(struct char_data *i, struct char_data *ch) {
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
      {100, "@wis in @Gexcellent@w condition."},
      {90, "@whas a few @Rscratches@w."},
      {80, "@whas some small @Rwounds@w and @Rbruises@w."},
      {70, "@whas quite a few @Rwounds@w."},
      {60, "@whas some big @rnasty wounds@w and @Rscratches@w."},
      {50, "@wlooks pretty @rhurt@w."},
      {40, "@wis mainly @rinjured@w."},
      {30, "@wis a @rmess@w of @rinjuries@w."},
      {20, "@wis @rstruggling@w to @msurvive@w."},
      {10, "@wis in @mawful condition@w."},
      {0, "@Ris barely alive.@w"},
      {-1, "@ris nearly dead.@w"},
  };
  int percent, ar_index;

  percent = (int)(char_meter_get(i, "powerlevel") / 10000);

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent) {
      send_to_char(ch, "%s\r\n", diagnosis[ar_index].text);
      return;
    }
}

static void look_at_char(struct char_data *i, struct char_data *ch) {
  int found, clan = FALSE;
  char buf[100];

  if (!ch->desc) {
    return;
  }
  if (i->description) {
    send_to_char(ch, "%s", i->description);
  }
  if (!MOB_FLAGGED(i, MOB_JUSTDESC)) {
    bringdesc(ch, i);
  }
  send_to_char(ch, "\r\n");
  if (!IS_NPC(i)) {
    if (GET_LIMBCOND(i, 1) >= 50 && !PLR_FLAGGED(i, PLR_CRARM)) {
      send_to_char(
          ch,
          "            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n",
          GET_LIMBCOND(i, 1), "%", "%");
    } else if (GET_LIMBCOND(i, 1) > 0 && !PLR_FLAGGED(i, PLR_CRARM)) {
      send_to_char(ch,
                   "            @D[@cRight Arm   @D: @rBroken "
                   "@y%2d%s@D/@g100%s @D]@n\r\n",
                   GET_LIMBCOND(i, 1), "%", "%");
    } else if (GET_LIMBCOND(i, 1) > 0 && PLR_FLAGGED(i, PLR_CRARM)) {
      send_to_char(ch,
                   "            @D[@cRight Arm   @D: @cCybernetic "
                   "@G%2d%s@D/@G100%s@D]@n\r\n",
                   GET_LIMBCOND(i, 1), "%", "%");
    } else if (GET_LIMBCOND(i, 1) <= 0) {
      send_to_char(
          ch,
          "            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
    }
    if (GET_LIMBCOND(i, 2) >= 50 && !PLR_FLAGGED(i, PLR_CLARM)) {
      send_to_char(
          ch,
          "            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n",
          GET_LIMBCOND(i, 2), "%", "%");
    } else if (GET_LIMBCOND(i, 2) > 0 && !PLR_FLAGGED(i, PLR_CLARM)) {
      send_to_char(ch,
                   "            @D[@cLeft Arm    @D: @rBroken "
                   "@y%2d%s@D/@g100%s @D]@n\r\n",
                   GET_LIMBCOND(i, 2), "%", "%");
    } else if (GET_LIMBCOND(i, 2) > 0 && PLR_FLAGGED(i, PLR_CLARM)) {
      send_to_char(ch,
                   "            @D[@cLeft Arm    @D: @cCybernetic "
                   "@G%2d%s@D/@G100%s@D]@n\r\n",
                   GET_LIMBCOND(i, 2), "%", "%");
    } else if (GET_LIMBCOND(i, 2) <= 0) {
      send_to_char(
          ch,
          "            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
    }
    if (GET_LIMBCOND(i, 3) >= 50 && !PLR_FLAGGED(i, PLR_CLARM)) {
      send_to_char(
          ch,
          "            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n",
          GET_LIMBCOND(i, 3), "%", "%");
    } else if (GET_LIMBCOND(i, 3) > 0 && !PLR_FLAGGED(i, PLR_CRLEG)) {
      send_to_char(ch,
                   "            @D[@cRight Leg   @D: @rBroken "
                   "@y%2d%s@D/@g100%s @D]@n\r\n",
                   GET_LIMBCOND(i, 3), "%", "%");
    } else if (GET_LIMBCOND(i, 3) > 0 && PLR_FLAGGED(i, PLR_CRLEG)) {
      send_to_char(ch,
                   "            @D[@cRight Leg   @D: @cCybernetic "
                   "@G%2d%s@D/@G100%s@D]@n\r\n",
                   GET_LIMBCOND(i, 3), "%", "%");
    } else if (GET_LIMBCOND(i, 3) <= 0) {
      send_to_char(
          ch,
          "            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
    }
    if (GET_LIMBCOND(i, 4) >= 50 && !PLR_FLAGGED(i, PLR_CLLEG)) {
      send_to_char(
          ch,
          "            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n",
          GET_LIMBCOND(i, 4), "%", "%");
    } else if (GET_LIMBCOND(i, 4) > 0 && !PLR_FLAGGED(i, PLR_CLLEG)) {
      send_to_char(ch,
                   "            @D[@cLeft Leg    @D: @rBroken "
                   "@y%2d%s@D/@g100%s @D]@n\r\n",
                   GET_LIMBCOND(i, 4), "%", "%");
    } else if (GET_LIMBCOND(i, 4) > 0 && PLR_FLAGGED(i, PLR_CLLEG)) {
      send_to_char(ch,
                   "            @D[@cLeft Leg    @D: @cCybernetic "
                   "@G%2d%s@D/@G100%s@D]@n\r\n",
                   GET_LIMBCOND(i, 4), "%", "%");
    } else if (GET_LIMBCOND(i, 4) <= 0) {
      send_to_char(
          ch,
          "            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
    }
    if (PLR_FLAGGED(i, PLR_HEAD)) {
      send_to_char(
          ch,
          "            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
    }
    if (!PLR_FLAGGED(i, PLR_HEAD)) {
      send_to_char(
          ch,
          "            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
    }
    if (((IS_SAIYAN(i) || IS_HALFBREED(i)) && PLR_FLAGGED(i, PLR_STAIL)) &&
        !PLR_FLAGGED(i, PLR_TAILHIDE)) {
      send_to_char(
          ch,
          "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
    }
    if ((IS_SAIYAN(i) || IS_HALFBREED(i)) && (!PLR_FLAGGED(i, PLR_STAIL)) &&
        (!PLR_FLAGGED(i, PLR_TAILHIDE))) {
      send_to_char(
          ch,
          "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
    }
    if ((IS_ICER(i) || IS_BIO(i)) && PLR_FLAGGED(i, PLR_TAIL)) {
      send_to_char(
          ch,
          "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
    }
    if ((IS_ICER(i) || IS_BIO(i)) && !PLR_FLAGGED(i, PLR_TAIL)) {
      send_to_char(
          ch,
          "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
    }
  }
  send_to_char(ch, "\r\n");
  if (GET_CLAN(i) != NULL && !strstr(GET_CLAN(i), "None")) {
    sprintf(buf, "%s", GET_CLAN(i));
    clan = TRUE;
  }
  if (GET_CLAN(i) == NULL) {
    clan = FALSE;
  }
  if (!IS_NPC(i)) {
    send_to_char(ch, "            @D[@mClan        @D: @W%-20s@D]@n\r\n",
                 clan ? buf : "None.");
  }
  if (!IS_NPC(i)) {
    send_to_char(
        ch, "\r\n         @D----------------------------------------@n\r\n");
    trans_check(ch, i);
    send_to_char(ch,
                 "         @D----------------------------------------@n\r\n");
  }
  send_to_char(ch, "\r\n");

  if ((!PLR_FLAGGED(i, PLR_DISGUISED) &&
       (readIntro(ch, i) == 1 && !IS_NPC(i)))) {
    if (GET_SEX(i) == SEX_NEUTRAL)
      send_to_char(ch, "%s appears to be %s %s, ", get_i_name(ch, i),
                   AN(RACE(i)), LRACE(i));
    else
      send_to_char(ch, "%s appears to be %s %s %s, ", get_i_name(ch, i),
                   AN(MAFE(i)), MAFE(i), LRACE(i));
  } else if (ch == i || IS_NPC(i)) {
    if (GET_SEX(i) == SEX_NEUTRAL)
      send_to_char(ch, "%c%s appears to be %s %s, ", UPPER(*GET_NAME(i)),
                   GET_NAME(i) + 1, AN(RACE(i)), LRACE(i));
    else
      send_to_char(ch, "%c%s appears to be %s %s %s, ", UPPER(*GET_NAME(i)),
                   GET_NAME(i) + 1, AN(MAFE(i)), MAFE(i), LRACE(i));
  } else {
    if (GET_SEX(i) == SEX_NEUTRAL)
      send_to_char(ch, "Appears to be %s %s, ", AN(RACE(i)), LRACE(i));
    else
      send_to_char(ch, "Appears to be %s %s %s, ", AN(MAFE(i)), MAFE(i),
                   LRACE(i));
  }

  if (IS_NPC(i)) {
    send_to_char(ch, "is %s sized, and\r\n", size_names[get_size(i)]);
  }
  
  if (!IS_NPC(i)) {
    send_to_char(ch, "is %s sized, about %ldcm tall,\r\nabout %ldkg heavy,",
                   size_names[get_size(i)], GET_PC_HEIGHT(i), GET_PC_WEIGHT(i));

    if (i == ch) {
      send_to_char(ch, " and ");
    } else if (GET_AGE(ch) >= GET_AGE(i) + 30) {
      send_to_char(ch, " appears to be very much younger than you, and ");
    } else if (GET_AGE(ch) >= GET_AGE(i) + 25) {
      send_to_char(ch, " appears to be much younger than you, and ");
    } else if (GET_AGE(ch) >= GET_AGE(i) + 15) {
      send_to_char(ch, " appears to be a good amount younger than you, and ");
    } else if (GET_AGE(ch) >= GET_AGE(i) + 10) {
      send_to_char(ch, " appears to be about a decade younger than you, and ");
    } else if (GET_AGE(ch) >= GET_AGE(i) + 5) {
      send_to_char(ch, " appears to be several years younger than you, and ");
    } else if (GET_AGE(ch) >= GET_AGE(i) + 2) {
      send_to_char(ch, " appears to be a bit younger than you, and ");
    } else if (GET_AGE(ch) > GET_AGE(i)) {
      send_to_char(ch, " appears to be slightly younger than you, and ");
    } else if (GET_AGE(ch) == GET_AGE(i)) {
      send_to_char(ch, " appears to be the same age as you, and ");
    }
    if (GET_AGE(i) >= GET_AGE(ch) + 30) {
      send_to_char(ch, " appears to be very much older than you, and ");
    } else if (GET_AGE(i) >= GET_AGE(ch) + 25) {
      send_to_char(ch, " appears to be much older than you, and ");
    } else if (GET_AGE(i) >= GET_AGE(ch) + 15) {
      send_to_char(ch, " appears to be a good amount older than you, and ");
    } else if (GET_AGE(i) >= GET_AGE(ch) + 10) {
      send_to_char(ch, " appears to be about a decade older than you, and ");
    } else if (GET_AGE(i) >= GET_AGE(ch) + 5) {
      send_to_char(ch, " appears to be several years older than you, and ");
    } else if (GET_AGE(i) >= GET_AGE(ch) + 2) {
      send_to_char(ch, " appears to be a bit older than you, and ");
    } else if (GET_AGE(i) > GET_AGE(ch)) {
      send_to_char(ch, " appears to be slightly older than you, and ");
    }
  }
  diag_char_to_char(i, ch);
  found = FALSE;
  char_equipment_iterate(i, [&](auto slot, auto eq) {
    if (CAN_SEE_OBJ(ch, eq)) {
      found = TRUE;
      return false;
    }
    return true;
  });

  if (found && (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOEQSEE))) {
    send_to_char(ch, "\r\n"); /* act() does capitalization. */
    if (!PLR_FLAGGED(i, PLR_DISGUISED)) {
      act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    } else {
      act("The disguised person is using:", FALSE, i, 0, ch, TO_VICT);
    }
    char_equipment_iterate(i, [&](auto slot, auto eq) {
      if (CAN_SEE_OBJ(ch, eq) && slot != WEAR_WIELD1 && slot != WEAR_WIELD2) {
        send_to_char(ch, "%s", wear_where[slot]);
        show_obj_to_char(eq, ch, SHOW_OBJ_SHORT);
        if (OBJ_FLAGGED(eq, ITEM_SHEATH)) {
          obj_contents_iterate(eq, [&](struct obj_data *obj2) {
            send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
            show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
            return true;
          });
        }
      } else if (CAN_SEE_OBJ(ch, eq) &&
                 !PLR_FLAGGED(i, PLR_THANDW)) {
        send_to_char(ch, "%s", wear_where[slot]);
        show_obj_to_char(eq, ch, SHOW_OBJ_SHORT);
        if (OBJ_FLAGGED(eq, ITEM_SHEATH)) {
          obj_contents_iterate(eq, [&](struct obj_data *obj2) {
            send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
            show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
            return true;
          });
        }
      } else if (CAN_SEE_OBJ(ch, eq) &&
                 PLR_FLAGGED(i, PLR_THANDW)) {
        send_to_char(ch, "@c<@CWielded by B. Hands@c>@n ");
        show_obj_to_char(eq, ch, SHOW_OBJ_SHORT);
      }
      return true;
    });
  }
  if (ch != i && ((GET_SKILL(ch, SKILL_KEEN) && AFF_FLAGGED(ch, AFF_SNEAK)) ||
                  GET_ADMLEVEL(ch))) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    if (CAN_SEE(i, ch))
      act("$n tries to evaluate what you have in your inventory.", TRUE, ch, 0,
          i, TO_VICT);
    if (GET_SKILL(ch, SKILL_KEEN) > axion_dice(0) &&
        (!IS_NPC(i) || GET_ADMLEVEL(ch) > 1)) {
      char_inventory_iterate(i, [&](auto tmp_obj) {
        if (CAN_SEE_OBJ(ch, tmp_obj) &&
            (ADM_FLAGGED(ch, ADM_SEEINV) ||
             (rand_number(0, 20) < GET_LEVEL(ch)))) {
          show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
          found = TRUE;
        }
        return true;
      });
      improve_skill(ch, SKILL_KEEN, 1);
    } else if (IS_NPC(i) && GET_ADMLEVEL(ch) < 2) {
      return;
    } else {
      act("You are unsure about $s inventory.", FALSE, i, 0, ch, TO_VICT);
      if (CAN_SEE(i, ch))
        act("$n didn't seem to get a good enough look.", TRUE, ch, 0, i,
            TO_VICT);
      improve_skill(ch, SKILL_KEEN, 1);
      return;
    }
    if (!found) {
      send_to_char(ch, "You can't see anything.\r\n");
      improve_skill(ch, SKILL_KEEN, 1);
    }
  }
}

static void list_one_char(struct char_data *i, struct char_data *ch) {
  struct obj_data *chair = NULL;
  int count = FALSE;
  const char *positions[] = {" is dead",
                             " is mortally wounded",
                             " is lying here, incapacitated",
                             " is lying here, stunned",
                             " is sleeping here",
                             " is resting here",
                             " is sitting here",
                             "!FIGHTING!",
                             " is standing here"};

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i))
    send_to_char(ch, "@D[@G%d@D]@w %s", GET_MOB_VNUM(i),
                 SCRIPT(i) ? "[TRIG] " : "");

  if (IS_NPC(i) && i->long_descr && GET_POS(i) == GET_DEFAULT_POS(i) &&
      !FIGHTING(i)) {
    send_to_char(ch, "%s", i->long_descr);

    if (IS_NPC(i)) {
      double health = char_meter_get(i, "powerlevel") / 1000000.0;
      if (health <= 0.1) {
        act("@R...Should be DEAD soon.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.2) {
        act("@R...Is on $s last leg.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.3) {
        act("@R...Is absolutely covered in wounds.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.4) {
        act("@R...$s body is in terrible shape.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.5) {
        act("@R...Blood is seeping from the wounds on $s body.@w", TRUE, i, 0,
            ch, TO_VICT);
      } else if (health <= 0.6) {
        act("@R...Horrible wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.7) {
        act("@R...Quite a few wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.8) {
        act("@R...Many wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health <= 0.9) {
        act("@R...A few wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if (health < 1.0) {
        act("@R...Some slight wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      }
    }

    if (GET_EAVESDROP(i) > 0) {
      char eaves[300];
      sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.",
              dirs[GET_EAVESDIR(i)]);
      act(eaves, TRUE, i, 0, ch, TO_VICT);
    }
    if (char_condition_has(i, "flying") && GET_ALT(i) == 1)
      act("...$e is in the air!", FALSE, i, 0, ch, TO_VICT);
    if (char_condition_has(i, "flying") && GET_ALT(i) == 2)
      act("...$e is high in the air!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_SANCTUARY) && !GET_SKILL(i, SKILL_AQUA_BARRIER))
      act("...$e has a barrier around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FIRESHIELD))
      act("...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", FALSE, i, 0, ch,
          TO_VICT);
    if (AFF_FLAGGED(i, AFF_SANCTUARY) && GET_SKILL(i, SKILL_AQUA_BARRIER))
      act("...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!",
          FALSE, i, 0, ch, TO_VICT);
    if (char_condition_has(i, "spiral"))
      act("...$e is spinning in a vortex!", FALSE, i, 0, ch, TO_VICT);
    if (GET_CHARGE(i))
      act("...$e has a bright %s aura around $s body!", FALSE, i, 0, ch,
          TO_VICT);
    if (char_condition_has(i, "dark_metamorphosis"))
      act("@w...$e has a dark, @rred@w aura and menacing presence.", FALSE, i,
          0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_HAYASA))
      act("@w...$e has a soft @cblue@w glow around $s body!", FALSE, i, 0, ch,
          TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (GET_FEATURE(i)) {
      char woo[MAX_STRING_LENGTH];
      sprintf(woo, "@C%s@n", GET_FEATURE(i));
      act(woo, FALSE, i, 0, ch, TO_VICT);
    }

    return;
  }

  if (IS_NPC(i) && !FIGHTING(i) && GET_POS(i) != POS_SITTING &&
      GET_POS(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s", UPPER(*i->short_descr), i->short_descr + 1);
  else if (IS_NPC(i) && GRAPPLED(i) && GRAPPLED(i) == ch)
    send_to_char(ch, "@w%c%s is being grappled with by YOU!",
                 UPPER(*i->short_descr), i->short_descr + 1);
  else if (IS_NPC(i) && GRAPPLED(i) && GRAPPLED(i) != ch)
    send_to_char(ch, "@w%c%s is being absorbed from by %s!",
                 UPPER(*i->short_descr), i->short_descr + 1,
                 readIntro(ch, GRAPPLED(i)) == 1 ? get_i_name(ch, GRAPPLED(i))
                                                 : AN(RACE(GRAPPLED(i))));
  else if (IS_NPC(i) && ABSORBBY(i) && ABSORBBY(i) == ch)
    send_to_char(ch, "@w%c%s is being absorbed from by YOU!",
                 UPPER(*i->short_descr), i->short_descr + 1);
  else if (IS_NPC(i) && ABSORBBY(i) && ABSORBBY(i) != ch)
    send_to_char(ch, "@w%c%s is being absorbed from by %s!",
                 UPPER(*i->short_descr), i->short_descr + 1,
                 readIntro(ch, ABSORBBY(i)) == 1 ? get_i_name(ch, ABSORBBY(i))
                                                 : AN(RACE(ABSORBBY(i))));
  else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) != ch &&
           GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING &&
           is_sparring(i))
    send_to_char(ch, "@w%c%s is sparring with %s!", UPPER(*i->short_descr),
                 i->short_descr + 1,
                 GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i))
                                  : (readIntro(ch, FIGHTING(i)) == 1
                                         ? get_i_name(ch, FIGHTING(i))
                                         : LRACE(FIGHTING(i))));
  else if (IS_NPC(i) && FIGHTING(i) && is_sparring(i) && FIGHTING(i) == ch &&
           GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is sparring with you!", UPPER(*i->short_descr),
                 i->short_descr + 1);
  else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) != ch &&
           GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is fighting %s!", UPPER(*i->short_descr),
                 i->short_descr + 1,
                 GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i))
                                  : (readIntro(ch, FIGHTING(i)) == 1
                                         ? get_i_name(ch, FIGHTING(i))
                                         : LRACE(FIGHTING(i))));
  else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) == ch &&
           GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is fighting YOU!", UPPER(*i->short_descr),
                 i->short_descr + 1);
  else if (IS_NPC(i) && FIGHTING(i) && GET_POS(i) == POS_SITTING)
    send_to_char(ch, "@w%c%s is sitting here.", UPPER(*i->short_descr),
                 i->short_descr + 1);
  else if (IS_NPC(i) && FIGHTING(i) && GET_POS(i) == POS_SLEEPING)
    send_to_char(ch, "@w%c%s is sleeping here.", UPPER(*i->short_descr),
                 i->short_descr + 1);
  else if (IS_NPC(i))
    send_to_char(ch, "@w%c%s", UPPER(*i->short_descr), i->short_descr + 1);
  else if (!IS_NPC(i)) {
    if (IS_MAJIN(i) && AFF_FLAGGED(i, AFF_LIQUEFIED)) {
      send_to_char(ch, "@wSeveral blobs of %s colored goo spread out here.@n\n",
                   skin_types[(int)GET_SKIN(i)]);
      return;
    }
    if ((GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(i) > 0) || IS_NPC(ch)) {
      send_to_char(ch, "@w%s", i->name);
    } else if ((!PLR_FLAGGED(i, PLR_DISGUISED) && readIntro(ch, i) == 1)) {
      send_to_char(ch, "@w%s", get_i_name(ch, i));
    } else if (!PLR_FLAGGED(i, PLR_DISGUISED) && readIntro(ch, i) != 1) {
      if (GET_DISTFEA(i) == DISTFEA_EYE) {
        send_to_char(ch, "@wA %s eyed %s %s", eye_types[(int)GET_EYE(i)],
                     MAFE(i), LRACE(i));
      } else if (GET_DISTFEA(i) == DISTFEA_HAIR) {
        if (IS_MAJIN(i)) {
          send_to_char(ch, "@wA %s majin, with a %s forelock,", MAFE(i),
                       FHA_types[(int)GET_HAIRL(i)]);
        } else if (IS_NAMEK(i)) {
          send_to_char(ch, "@wA namek, with %s antennae,",
                       FHA_types[(int)GET_HAIRL(i)]);
        } else if (IS_ARLIAN(i)) {
          send_to_char(ch, "@wA arlian, with %s antennae,",
                       FHA_types[(int)GET_HAIRL(i)]);
        } else if (IS_ICER(i) || IS_DEMON(i)) {
          send_to_char(ch, "@wA %s %s, with %s horns", MAFE(i), LRACE(i),
                       FHA_types[(int)GET_HAIRL(i)]);
        } else {
          char blarg[MAX_INPUT_LENGTH];
          sprintf(blarg, "%s %s hair %s", hairl_types[(int)GET_HAIRL(i)],
                  hairc_types[(int)GET_HAIRC(i)],
                  hairs_types[(int)GET_HAIRS(i)]);
          send_to_char(ch, "@wA %s %s, with %s", MAFE(i), LRACE(i),
                       GET_HAIRL(i) == 0 ? "a bald head" : (blarg));
        }
      } else if (GET_DISTFEA(i) == DISTFEA_SKIN) {
        send_to_char(ch, "@wA %s skinned %s %s", skin_types[(int)GET_SKIN(i)],
                     MAFE(i), LRACE(i));
      } else if (GET_DISTFEA(i) == DISTFEA_HEIGHT) {
        char *height;
        if (IS_TRUFFLE(i)) {
          if (GET_PC_HEIGHT(i) > 70) {
            height = strdup("very tall");
          } else if (GET_PC_HEIGHT(i) > 55) {
            height = strdup("tall");
          } else if (GET_PC_HEIGHT(i) > 35) {
            height = strdup("average height");
          } else {
            height = strdup("short");
          }
        } else if (PLR_FLAGGED(i, PLR_OOZARU) || GET_GENOME(i, 0) == 11) {
          if (GET_PC_HEIGHT(i) * 10 > 2000) {
            height = strdup("very tall");
          } else if (GET_PC_HEIGHT(i) * 10 > 1800) {
            height = strdup("tall");
          } else if (GET_PC_HEIGHT(i) * 10 > 1500) {
            height = strdup("average height");
          } else {
            height = strdup("short");
          }
        } else {
          if (GET_PC_HEIGHT(i) > 200) {
            height = strdup("very tall");
          } else if (GET_PC_HEIGHT(i) > 180) {
            height = strdup("tall");
          } else if (GET_PC_HEIGHT(i) > 150) {
            height = strdup("average height");
          } else if (GET_PC_HEIGHT(i) > 120) {
            height = strdup("short");
          } else {
            height = strdup("very short");
          }
        }
        send_to_char(ch, "@wA %s %s %s", height, MAFE(i), LRACE(i));
        if (height) {
          free(height);
        }
      } else if (GET_DISTFEA(i) == DISTFEA_WEIGHT) {
        char *height;
        if (IS_TRUFFLE(i)) {
          if (GET_PC_WEIGHT(i) > 35) {
            height = strdup("very heavy");
          } else if (GET_PC_WEIGHT(i) > 25) {
            height = strdup("heavy");
          } else if (GET_PC_WEIGHT(i) > 15) {
            height = strdup("average weight");
          } else {
            height = strdup("welterweight");
          }
        } else if (PLR_FLAGGED(i, PLR_OOZARU) || GET_GENOME(i, 0) == 11) {
          if (GET_PC_WEIGHT(i) * 50 > 6000) {
            height = strdup("very heavy");
          } else if (GET_PC_WEIGHT(i) * 50 > 5000) {
            height = strdup("heavy");
          } else if (GET_PC_WEIGHT(i) * 50 > 4000) {
            height = strdup("average weight");
          } else if (GET_PC_WEIGHT(i) * 50 > 3000) {
            height = strdup("lightweight");
          } else {
            height = strdup("welterweight");
          }
        } else {
          if (GET_PC_WEIGHT(i) > 120) {
            height = strdup("very heavy");
          } else if (GET_PC_WEIGHT(i) > 100) {
            height = strdup("heavy");
          } else if (GET_PC_WEIGHT(i) > 80) {
            height = strdup("average weight");
          } else if (GET_PC_WEIGHT(i) > 60) {
            height = strdup("lightweight");
          } else {
            height = strdup("welterweight");
          }
        }
        send_to_char(ch, "@wA %s %s %s", height, MAFE(i), LRACE(i));
        if (height) {
          free(height);
        }
      }
    } else {
      send_to_char(ch, "@wA disguised %s %s", MAFE(i), LRACE(i));
    }
  }

  if (!IS_NPC(i) || !FIGHTING(i)) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE)) {
      send_to_char(ch, ", is invisible");
      count = TRUE;
    }
    if (AFF_FLAGGED(i, AFF_ETHEREAL)) {
      send_to_char(ch, ", has a halo");
      count = TRUE;
    }
    if (AFF_FLAGGED(i, AFF_HIDE) && i != ch) {
      send_to_char(ch, ", is hiding");
      if (GET_SKILL(i, SKILL_HIDE) && !IS_NPC(ch) && i != ch) {
        improve_skill(i, SKILL_HIDE, 1);
      }
      count = TRUE;
    }
    if (!IS_NPC(i) && !i->desc) {
      send_to_char(ch, ", has a blank stare");
      count = TRUE;
    }
    if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING)) {
      send_to_char(ch, ", is writing");
      count = TRUE;
    }
    if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK)) {
      send_to_char(ch, ", is buildwalking");
      count = TRUE;
    }
    if (!IS_NPC(i) && ABSORBING(i) && ABSORBING(i) != ch) {
      send_to_char(ch, ", is absorbing from %s", GET_NAME(ABSORBING(i)));
      count = TRUE;
    }
    if (!IS_NPC(i) && GRAPPLING(i) && GRAPPLING(i) != ch) {
      send_to_char(ch, ", is grappling with %s",
                   readIntro(ch, GRAPPLING(i)) == 1
                       ? get_i_name(ch, GRAPPLING(i))
                       : introd_calc(GRAPPLING(i)));
      count = TRUE;
    }
    if (!IS_NPC(i) && CARRYING(i) && CARRYING(i) != ch) {
      send_to_char(ch, ", is carrying %s",
                   readIntro(ch, CARRYING(i)) == 1 ? get_i_name(ch, CARRYING(i))
                                                   : introd_calc(CARRYING(i)));
      count = TRUE;
    }
    if (!IS_NPC(i) && CARRIED_BY(i) && CARRIED_BY(i) != ch) {
      send_to_char(ch, ", is being carried by %s",
                   readIntro(ch, CARRIED_BY(i)) == 1
                       ? get_i_name(ch, CARRIED_BY(i))
                       : introd_calc(CARRIED_BY(i)));
      count = TRUE;
    }
    if (!IS_NPC(i) && GRAPPLING(i) && GRAPPLING(i) == ch) {
      send_to_char(ch, ", is grappling with YOU");
      count = TRUE;
    }
    if (!IS_NPC(i) && ABSORBING(i) && ABSORBING(i) == ch) {
      send_to_char(ch, ", is absorbing from YOU");
      count = TRUE;
    }
    if (!IS_NPC(i) && ABSORBING(ch) && ABSORBING(ch) == i) {
      send_to_char(ch, ", is being absorbed from by YOU");
      count = TRUE;
    }
    if (!IS_NPC(i) && GRAPPLING(ch) && GRAPPLING(ch) == i) {
      send_to_char(ch, ", is being grappled with by YOU");
      count = TRUE;
    }
    if (!IS_NPC(i) && CARRYING(ch) && CARRYING(ch) == i) {
      send_to_char(ch, ", is being carried by you");
      count = TRUE;
    }
    if (!IS_NPC(ch) && !IS_NPC(i) && FIGHTING(i)) {
      if (!PLR_FLAGGED(i, PLR_SPAR) ||
          (PLR_FLAGGED(i, PLR_SPAR) &&
           (!PLR_FLAGGED(FIGHTING(i), PLR_SPAR) || IS_NPC(FIGHTING(i))))) {
        send_to_char(ch, ", is here fighting ");
      }
      if (PLR_FLAGGED(i, PLR_SPAR) && PLR_FLAGGED(FIGHTING(i), PLR_SPAR)) {
        send_to_char(ch, ", is here sparring ");
      }
      if (FIGHTING(i) == ch) {
        send_to_char(ch, "@rYOU@w");
        count = TRUE;
      } else {
        if (char_room_get(i) == char_room_get(FIGHTING(i))) {
          send_to_char(ch, "%s",
                       GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i))
                                        : (readIntro(ch, FIGHTING(i)) == 1
                                               ? get_i_name(ch, FIGHTING(i))
                                               : LRACE(FIGHTING(i))));
          count = TRUE;
        } else {
          send_to_char(ch, "someone who has already left!");
        }
      }
    }
  }
  if (SITS(i)) {
    chair = SITS(i);
    if (PLR_FLAGGED(i, PLR_HEALT)) {
      send_to_char(ch, "@w is floating inside a healing tank.");
    } else if (count == TRUE) {
      send_to_char(ch, ",@w and%s on %s.", positions[(int)GET_POS(i)],
                   chair->short_description);
    } else if (count == FALSE) {
      send_to_char(ch, "@w%s on %s.", positions[(int)GET_POS(i)],
                   chair->short_description);
    }
  } else if (!PLR_FLAGGED(i, PLR_PILOTING) && !SITS(i) &&
             (!IS_NPC(i) || !FIGHTING(i))) {
    if (count == TRUE) {
      send_to_char(ch, "@w, and%s.", positions[(int)GET_POS(i)]);
    }
    if (count == FALSE) {
      send_to_char(ch, "@w%s.", positions[(int)GET_POS(i)]);
    }
  } else if (PLR_FLAGGED(i, PLR_PILOTING)) {
    send_to_char(ch, "@w, is sitting in the pilot's chair.\r\n");
  } else {

    if (FIGHTING(i) && !IS_NPC(ch) && !IS_NPC(i)) {
      if (!PLR_FLAGGED(i, PLR_SPAR)) {
        send_to_char(ch, ", is here fighting ");
      }
      if (PLR_FLAGGED(i, PLR_SPAR)) {
        send_to_char(ch, ", is here sparring ");
      }
      if (FIGHTING(i) == ch)
        send_to_char(ch, "@rYOU@w!");
      else {
        if (char_room_get(i) == char_room_get(FIGHTING(i)))
          send_to_char(ch, "%s!",
                       GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i))
                                        : (readIntro(ch, FIGHTING(i)) == 1
                                               ? get_i_name(ch, FIGHTING(i))
                                               : LRACE(FIGHTING(i))));
        else
          send_to_char(ch, "someone who has already left!");
      }
    } else if (!IS_NPC(i)) { /* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
    }
  }

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      send_to_char(ch, " (@rRed@[3] Aura)");
    else if (IS_GOOD(i))
      send_to_char(ch, " (@bBlue@[3] Aura)");
  }
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_AFK))
    send_to_char(ch, " @D(@RAFK@D)");
  else if (!IS_NPC(i) && i->timer > 3)
    send_to_char(ch, " @D(@RIDLE@D)");
  send_to_char(ch, "@n\r\n");

  if (GET_EAVESDROP(i) > 0) {
    char eaves[300];
    sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.",
            dirs[GET_EAVESDIR(i)]);
    act(eaves, TRUE, i, 0, ch, TO_VICT);
  }
  if (!IS_NPC(i)) {
    if (char_condition_has(i, "fishing")) {
      act("@w...$e is @Cfishing@w.@n", TRUE, i, 0, ch, TO_VICT);
    }
  }
  if (PLR_FLAGGED(i, PLR_AURALIGHT)) {
    char bloom[MAX_INPUT_LENGTH];
    sprintf(bloom, "...is surrounded by a bright %s aura.@n",
            aura_types[GET_AURA(i)]);
    act(bloom, TRUE, i, 0, ch, TO_VICT);
  }
  if (AFF_FLAGGED(i, AFF_SANCTUARY) && !GET_SKILL(i, SKILL_AQUA_BARRIER))
    act("@w...$e has a @bbarrier@w around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (AFF_FLAGGED(i, AFF_FIRESHIELD))
    act("@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", FALSE, i, 0, ch,
        TO_VICT);
  if (char_condition_has(i, "healing_glow"))
    act("@w...$e has a serene @Cblue@Y glow@w around $s body.", TRUE, i, 0, ch,
        TO_VICT);
  if (char_condition_has(i, "ethereal_armor"))
    act("@w...$e has ghostly @Ggreen@w ethereal armor around $s body.", TRUE, i,
        0, ch, TO_VICT);
  if (AFF_FLAGGED(i, AFF_SANCTUARY) && GET_SKILL(i, SKILL_AQUA_BARRIER))
    act("@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!",
        TRUE, i, 0, ch, TO_VICT);
  if (char_condition_has(i, "flying") && GET_ALT(i) == 1)
    act("@w...$e is in the air!", TRUE, i, 0, ch, TO_VICT);
  if (char_condition_has(i, "flying") && GET_ALT(i) == 2)
    act("@w...$e is high in the air!", TRUE, i, 0, ch, TO_VICT);
  if (GET_KAIOKEN(i) > 0)
    act("@w...@r$e has a red aura around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_condition_has(i, "spiral"))
    act("@w...$e is spinning in a vortex!", FALSE, i, 0, ch, TO_VICT);
  if (IS_TRANSFORMED(i) && !IS_ANDROID(i) && !IS_SAIYAN(i) && !IS_HALFBREED(i))
    act("@w...$e has energy crackling around $s body!", TRUE, i, 0, ch,
        TO_VICT);
  if (GET_CHARGE(i) && !IS_SAIYAN(i) && !IS_HALFBREED(i)) {
    char aura[MAX_INPUT_LENGTH];
    sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!",
            aura_types[GET_AURA(i)]);
    act(aura, TRUE, i, 0, ch, TO_VICT);
  }
  if (!PLR_FLAGGED(i, PLR_OOZARU) && GET_CHARGE(i) && IS_TRANSFORMED(i) &&
      (IS_SAIYAN(i) || IS_HALFBREED(i)))
    act("@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!",
        TRUE, i, 0, ch, TO_VICT);
  if (!PLR_FLAGGED(i, PLR_OOZARU) && GET_CHARGE(i) && !IS_TRANSFORMED(i) &&
      (IS_SAIYAN(i) || IS_HALFBREED(i))) {
    char aura[MAX_INPUT_LENGTH];
    sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!",
            aura_types[GET_AURA(i)]);
    act(aura, TRUE, i, 0, ch, TO_VICT);
  }
  if (!PLR_FLAGGED(i, PLR_OOZARU) && !GET_CHARGE(i) && IS_TRANSFORMED(i) &&
      (IS_SAIYAN(i) || IS_HALFBREED(i)))
    act("@w...$e has energy crackling around $s body!", TRUE, i, 0, ch,
        TO_VICT);
  if (PLR_FLAGGED(i, PLR_OOZARU) && GET_CHARGE(i) &&
      (IS_SAIYAN(i) || IS_HALFBREED(i)))
    act("@w...$e is in the form of a @rgreat ape@w!", TRUE, i, 0, ch, TO_VICT);
  if (char_condition_has(i, "kyodaika"))
    act("@w...$e has expanded $s body size@w!", TRUE, i, 0, ch, TO_VICT);
  if (AFF_FLAGGED(i, AFF_HAYASA))
    act("@w...$e has a soft @cblue@w glow around $s body!", FALSE, i, 0, ch,
        TO_VICT);
  if (PLR_FLAGGED(i, PLR_OOZARU) && !GET_CHARGE(i) &&
      (IS_SAIYAN(i) || IS_HALFBREED(i)))
    act("@w...$e has energy crackling around $s @rgreat ape@w body!", TRUE, i,
        0, ch, TO_VICT);
  if (GET_FEATURE(i)) {
    char woo[MAX_STRING_LENGTH];
    sprintf(woo, "@C%s@n", GET_FEATURE(i));
    act(woo, FALSE, i, 0, ch, TO_VICT);
  }

  if (GET_RDISPLAY(i)) {
    if (GET_RDISPLAY(i) != "Empty") {
      char rdis[MAX_STRING_LENGTH];
      sprintf(rdis, "...%s", GET_RDISPLAY(i));
      act(rdis, FALSE, i, 0, ch, TO_VICT);
    }
  }
}

static void list_char_to_char(struct room_data *room, struct char_data *ch) {
  struct hide_node {
    struct hide_node *next;
    struct char_data *hidden;
  } *hideinfo, *lasthide, *tmphide;

  hideinfo = lasthide = NULL;

  room_people_iterate(room, [&](auto i) {
    if (AFF_FLAGGED(i, AFF_HIDE) &&
        roll_resisted(i, SKILL_HIDE, ch, SKILL_SPOT)) {
      if (GET_SKILL(i, SKILL_HIDE) && !IS_NPC(ch) && i != ch) {
        improve_skill(i, SKILL_HIDE, 1);
      }
      CREATE(tmphide, struct hide_node, 1);
      tmphide->next = NULL;
      tmphide->hidden = i;
      if (!lasthide) {
        hideinfo = lasthide = tmphide;
      } else {
        lasthide->next = tmphide;
        lasthide = tmphide;
      }
    }
    return true;
  });

  std::vector<struct char_data *> people;
  room_people_iterate(room, [&](auto p) { people.push_back(p); return true; });

  for (size_t ii = 0; ii < people.size(); ii++) {
    auto i = people[ii];
    /* hide npcs whose description starts with a '.' from non-holylighted people
    - Idea from Elaseth of TBA */
    if ((ch == i) || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
                      IS_NPC(i) && i->long_descr && *i->long_descr == '.'))
      continue;

    for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
      if (tmphide->hidden == i)
        break;
    if (tmphide)
      continue;

    int num;
    if (CAN_SEE(ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurrences of this mob are there? */
        size_t jj;
        for (jj = 0; jj < ii; jj++) {
          auto j = people[jj];
          if ((i->proto_id == j->proto_id) && (GET_POS(i) == GET_POS(j)) &&
              (AFF_FLAGS(i)[0] == AFF_FLAGS(j)[0]) &&
              (AFF_FLAGS(i)[1] == AFF_FLAGS(j)[1]) &&
              (AFF_FLAGS(i)[2] == AFF_FLAGS(j)[2]) &&
              (AFF_FLAGS(i)[3] == AFF_FLAGS(j)[3]) &&
              (!FIGHTING(i) && !FIGHTING(j)) &&
              (GET_HIT(i) == getMaxPL(i) && GET_HIT(j) == getMaxPL(j)) &&
              !strcmp(GET_NAME(i), GET_NAME(j))) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              break;
          }
        }
        if (jj < ii)
          /* This will be true where we have already found this
           * mob for an earlier "i".  The continue pops us out of
           * the main "i" for loop.
           */
          continue;
        for (size_t kk = ii; kk < people.size(); kk++) {
          auto j = people[kk];
          if ((i->proto_id == j->proto_id) && (GET_POS(i) == GET_POS(j)) &&
              (AFF_FLAGS(i)[0] == AFF_FLAGS(j)[0]) &&
              (AFF_FLAGS(i)[1] == AFF_FLAGS(j)[1]) &&
              (AFF_FLAGS(i)[2] == AFF_FLAGS(j)[2]) &&
              (AFF_FLAGS(i)[3] == AFF_FLAGS(j)[3]) &&
              (!FIGHTING(i) && !FIGHTING(j)) &&
              (GET_HIT(i) == GET_MAX_HIT(i) && GET_HIT(j) == GET_MAX_HIT(j)) &&
              !strcmp(GET_NAME(i), GET_NAME(j))) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              num++;
          }
        }
      }
      /* Now show this mob's name and other stuff */
      send_to_char(ch, "@w");
      if (num > 1)
        send_to_char(ch, "@D(@Rx@Y%2i@D)@n ", num);
      list_one_char(i, ch);
      send_to_char(ch, "@n");
    } /* processed a character we can see */
    else if (IS_DARK(char_room_get(ch)) && !CAN_SEE_IN_DARK(ch) &&
             AFF_FLAGGED(i, AFF_INFRAVISION))
      send_to_char(
          ch, "@wYou see a pair of glowing red eyes looking your way.@n\r\n");
  } /* loop through all characters in room */
}

static void capitalize_direction(char *out, size_t out_size, int door) {
  snprintf(out, out_size, "%s", dirs[door]);
  *out = toupper(*out);
}

static const char *exit_keyword_or_opening(struct room_direction_data *exit) {
  if (!exit_keyword_get(exit))
    return "opening";

  const char *keyword = fname(exit_keyword_get(exit));
  if (!keyword || !strcasecmp(keyword, "undefined"))
    return "opening";

  return keyword;
}

static bool
append_immortal_exit_door_details(char *line, size_t line_size,
                                  struct char_data *ch, int door,
                                  struct room_direction_data *exit) {
  if (!exit_flagged(exit, EX_ISDOOR) &&
      !exit_flagged(exit, EX_SECRET))
    return true;

  const char *keyword = fname(exit_keyword_get(exit));
  if (!keyword) {
    send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION %s@n\r\n",
                 dirs[door]);
    mud_log("ERROR: %s found error direction %s at room %d", GET_NAME(ch),
        dirs[door], char_room_vnum_get(ch));
    return false;
  }

  const char *door_name =
      strcasecmp(keyword, "undefined") ? keyword : "opening";
  char plural_probe[100];
  snprintf(plural_probe, sizeof(plural_probe), "%s ", door_name);

  snprintf(line + strlen(line), line_size - strlen(line),
           "                    The %s%s %s %s %s%s.\r\n",
           exit_flagged(exit, EX_SECRET) ? "@rsecret@w " : "", door_name,
           strstr(plural_probe, "s ") != NULL ? "are" : "is",
           exit_flagged(exit, EX_CLOSED) ? "closed" : "open",
           exit_flagged(exit, EX_LOCKED) ? "and locked" : "and unlocked",
           exit_flagged(exit, EX_PICKPROOF) ? " (pickproof)" : "");
  return true;
}

static bool character_has_light(struct char_data *ch) {
  if (PLR_FLAGGED(ch, PLR_AURALIGHT))
    return true;

  bool has_light = false;

  char_equipment_iterate(ch, [&](auto slot, auto eq) {
    if (GET_OBJ_TYPE(eq) == ITEM_LIGHT && GET_OBJ_VAL(eq, VAL_LIGHT_HOURS)) {
      has_light = true;
      return false;
    }
    return true;
  });

  return has_light;
}

static void show_auto_exit_room_details(struct room_data *target_room,
                                        struct char_data *ch) {
  struct room_data *room = target_room;
  const room_vnum target_vnum = room_vnum_get(room);
  const bool is_house = room_flagged(room, ROOM_HOUSE);
  const bool is_garden1 = room_flagged(room, ROOM_GARDEN1);
  const bool is_garden2 = room_flagged(room, ROOM_GARDEN2);

  send_to_char(ch, "@D---------------------------------------------------------"
                   "---------------@n\r\n");
  if (is_house && !is_garden1 && !is_garden2)
    send_to_char(ch, "@D[@GItems Stored@D: @g%d@D]@n\r\n",
                 check_saveroom_count(ch, NULL));
  if (is_house && is_garden1 && !is_garden2)
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n",
                 check_saveroom_count(ch, NULL));
  if (is_house && !is_garden1 && is_garden2)
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n",
                 check_saveroom_count(ch, NULL));

  const int buoys_here = (GET_RADAR1(ch) == target_vnum) +
                         (GET_RADAR2(ch) == target_vnum) +
                         (GET_RADAR3(ch) == target_vnum);
  if (buoys_here == 3 && target_room != 0)
    send_to_char(ch, "@CAll three of your buoys are floating here. Why?@n\r\n");
  else if (buoys_here == 2)
    send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
  else if (GET_RADAR1(ch) == target_vnum)
    send_to_char(ch, "@CYour @cBuoy #1@C is floating here.@n\r\n");
  else if (GET_RADAR2(ch) == target_vnum)
    send_to_char(ch, "@CYour @cBuoy #2@C is floating here.@n\r\n");
  else if (GET_RADAR3(ch) == target_vnum)
    send_to_char(ch, "@CYour @cBuoy #3@C is floating here.@n\r\n");
}

static void do_auto_exits(struct room_data *target_room, struct char_data *ch,
                          int exit_mode) {
  static const int display_order[] = {NORTHWEST, NORTH, NORTHEAST, EAST,
                                      SOUTHEAST, SOUTH, SOUTHWEST, WEST,
                                      UP,        DOWN,  INDIR,     OUTDIR};
  struct room_data *room = target_room;

  if (exit_mode == EXIT_OFF)
    send_to_char(ch, "@D-------------------------------------------------------"
                     "-----------------@n\r\n");

  const bool space =
      room_sector_type_get(room) == SECT_SPACE && room_vnum_get(room) >= 20000;
  if (exit_mode == EXIT_NORMAL && !space && char_room_get(ch) == target_room) {
    send_to_char(ch, "@D-------------------------------------------------------"
                     "-----------------@n\r\n");
    send_to_char(ch,
                 "@w      Compass           Auto-Map            Map Key\r\n");
    send_to_char(ch, "@R     ---------         ----------   "
                     "-----------------------------\r\n");
    gen_map(ch, 0);
    send_to_char(ch, "@D-------------------------------------------------------"
                     "-----------------@n\r\n");
  }

  if (exit_mode == EXIT_NORMAL && space) {
    send_to_char(ch, "@D------------------------------[@CRadar@D]--------------"
                     "-------------------@n\r\n");
    printmap(target_room, ch, 1, -1);
    send_to_char(ch, "     @D[@wTurn autoexit complete on for directions "
                     "instead of radar@D]@n\r\n");
    send_to_char(ch, "@D-------------------------------------------------------"
                     "-----------------@n\r\n");
  }

  if (exit_mode != EXIT_COMPLETE &&
      !(exit_mode == EXIT_NORMAL && !space && char_room_get(ch) != target_room))
    return;

  send_to_char(ch, "@D----------------------------[@gObvious "
                   "Exits@D]-----------------------------@n\r\n");
  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }
  if (PLR_FLAGGED(ch, PLR_EYEC)) {
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  if (IS_DARK(char_room_get(ch)) && !CAN_SEE_IN_DARK(ch) &&
      !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  }

  const bool immortal_view =
      ADM_FLAGGED(ch, ADM_SEESECRET) || GET_ADMLEVEL(ch) > 4;
  const bool has_light = character_has_light(ch);
  bool door_found = false;
  char exit_lines[NUM_OF_DIRS][500] = {};

  room_exits_iterate(room, [&](auto door, auto exit) {
    struct room_data *destination = exit_dest_get(exit);
    if(!destination) return true;

    char direction[32];
    capitalize_direction(direction, sizeof(direction), door);

    char *line = exit_lines[door];

    if (immortal_view) {
      door_found = true;
      snprintf(line, sizeof(exit_lines[door]), "@c%-9s @D- [@Y%5d@D]@w %s.\r\n",
               direction, room_vnum_get(destination),
               room_name_get(destination));
      if (!append_immortal_exit_door_details(line, sizeof(exit_lines[door]), ch,
                                             door, exit))
        return false;
    } else if (!exit_flagged(exit, EX_CLOSED)) {
      door_found = true;
      const char *destination_name =
          IS_DARK(exit_dest_get(exit)) && !CAN_SEE_IN_DARK(ch) && !has_light
              ? "@bToo dark to tell.@w"
              : room_name_get(destination);
      snprintf(line, sizeof(exit_lines[door]), "@c%-9s @D-@w %s\r\n", direction,
               destination_name);
    } else if (CONFIG_DISP_CLOSED_DOORS &&
               !exit_flagged(exit, EX_SECRET)) {
      door_found = true;
      snprintf(line, sizeof(exit_lines[door]),
               "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", direction,
               exit_keyword_or_opening(exit));
    }

    return true;
  });

  if (!door_found)
    send_to_char(ch, " None.\r\n");

  for (int door : display_order) {
    if (*exit_lines[door])
      send_to_char(ch, "%s", exit_lines[door]);
  }

  show_auto_exit_room_details(target_room, ch);
}

static void do_auto_exits2(struct room_data *room,
                           struct char_data *ch) {
  int slen = 0;

  send_to_char(ch, "\nExits: ");

  room_exits_iterate(room, [&](auto door, auto exit) {
    auto dest = exit_dest_get(exit);
    if(!dest) return true;
    if (exit_flagged(exit, EX_CLOSED))
      return true;

    send_to_char(ch, "%s ", abbr_dirs[door]);
    slen++;
    return true;
  });

  send_to_char(ch, "%s\r\n", slen ? "" : "None!");
}

ACMD(do_exits) {
  /* Why duplicate code? */
  if (!PRF_FLAGGED(ch, PRF_NODEC)) {
    do_auto_exits(char_room_get(ch), ch, EXIT_COMPLETE);
  } else {
    do_auto_exits2(char_room_get(ch), ch);
  }
}

static const char *exitlevels[] = {"off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit) {
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Your current autoexit level is %s.\r\n",
                 exitlevels[EXIT_LEV(ch)]);
    return;
  }
  if (((tp = search_block(arg, exitlevels, FALSE)) == -1)) {
    send_to_char(ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
  case EXIT_OFF:
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
    break;
  case EXIT_NORMAL:
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
    break;
  case EXIT_COMPLETE:
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
    break;
  }
  send_to_char(ch, "Your @rautoexit level@n is now %s.\r\n",
               exitlevels[EXIT_LEV(ch)]);
}

static bool room_is_zenith(room_vnum vnum) {
  return (vnum >= 3400 && vnum <= 3599) || (vnum >= 62900 && vnum <= 62999) ||
         vnum == 19600;
}

static void show_room_planet(struct char_data *ch, struct room_data *room,
                             room_vnum vnum) {
  struct PlanetLabel {
    int flag;
    const char *label;
  };

  static const PlanetLabel planet_labels[] = {
      {ROOM_EARTH, "@wPlanet: @GEarth@n\r\n"},
      {ROOM_CERRIA, "@wPlanet: @RCerria@n\r\n"},
  };

  for (const auto &planet : planet_labels) {
    if (room_flagged(room, planet.flag)) {
      send_to_char(ch, "%s", planet.label);
      return;
    }
  }

  if (room_is_zenith(vnum))
    send_to_char(ch, "@wPlanet: @BZenith@n\r\n");
  else if (room_flagged(room, ROOM_AETHER))
    send_to_char(ch, "@wPlanet: @MAether@n\r\n");
  else if (room_flagged(room, ROOM_FRIGID))
    send_to_char(ch, "@wPlanet: @CFrigid@n\r\n");
  else if (room_flagged(room, ROOM_SPACE))
    send_to_char(ch, "@wPlanet: @DNone@n\r\n");
  else if (room_flagged(room, ROOM_VEGETA))
    send_to_char(ch, "@wPlanet: @YVegeta@n\r\n");
  else if (room_flagged(room, ROOM_NAMEK))
    send_to_char(ch, "@wPlanet: @gNamek@n\r\n");
  else if (room_flagged(room, ROOM_KONACK))
    send_to_char(ch, "@wPlanet: @MKonack@n\r\n");
  else if (room_flagged(room, ROOM_NEO))
    send_to_char(ch, "@wPlanet: @WNeo Nirvana@n\r\n");
  else if (room_flagged(room, ROOM_AL))
    send_to_char(ch, "@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
  else if (room_flagged(room, ROOM_HELL))
    send_to_char(ch, "@wDimension: @RPunishment Hell@n\r\n");
  else if (room_flagged(room, ROOM_RHELL))
    send_to_char(ch, "@wDimension: @RH@re@Dl@Rl@n\r\n");
  else if (room_flagged(room, ROOM_YARDRAT))
    send_to_char(ch, "@wPlanet: @mYardrat@n\r\n");
  else if (room_flagged(room, ROOM_KANASSA))
    send_to_char(ch, "@wPlanet: @BKanassa@n\r\n");
  else if (room_flagged(room, ROOM_ARLIA))
    send_to_char(ch, "@wPlanet: @GArlia@n\r\n");
  else
    send_to_char(ch, "@wPlanet: @WUNKNOWN@n\r\n");
}

static void show_room_gravity(struct char_data *ch, int gravity) {
  switch (gravity) {
  case 10:
    send_to_char(ch, "@wGravity: @W10x@n\r\n");
    break;
  case 20:
    send_to_char(ch, "@wGravity: @W20x@n\r\n");
    break;
  case 30:
    send_to_char(ch, "@wGravity: @W30x@n\r\n");
    break;
  case 40:
    send_to_char(ch, "@wGravity: @W40x@n\r\n");
    break;
  case 50:
    send_to_char(ch, "@wGravity: @W50x@n\r\n");
    break;
  case 100:
    send_to_char(ch, "@wGravity: @W100x@n\r\n");
    break;
  case 200:
    send_to_char(ch, "@wGravity: @W200x@n\r\n");
    break;
  case 300:
    send_to_char(ch, "@wGravity: @W300x@n\r\n");
    break;
  case 400:
    send_to_char(ch, "@wGravity: @W400x@n\r\n");
    break;
  case 500:
    send_to_char(ch, "@wGravity: @W500x@n\r\n");
    break;
  case 1000:
    send_to_char(ch, "@wGravity: @W1,000x@n\r\n");
    break;
  case 5000:
    send_to_char(ch, "@wGravity: @W5,000x@n\r\n");
    break;
  case 10000:
    send_to_char(ch, "@wGravity: @W10,000x@n\r\n");
    break;
  default:
    if (gravity <= 0)
      send_to_char(ch, "@wGravity: @WNormal@n\r\n");
    break;
  }
}

static const char *room_damage_text(int damage, const char *const *messages) {
  if (damage <= 2)
    return messages[0];
  if (damage <= 4)
    return messages[1];
  if (damage <= 6)
    return messages[2];
  if (damage <= 10)
    return messages[3];
  if (damage <= 20)
    return messages[4];
  if (damage <= 30)
    return messages[5];
  if (damage <= 50)
    return messages[6];
  if (damage <= 75)
    return messages[7];
  if (damage <= 99)
    return messages[8];
  return messages[9];
}

static void show_room_damage(struct char_data *ch, int sector, int damage) {
  static const char *const inside_damage[] = {
      "@wA small hole with chunks of debris that can be seen scarring the "
      "floor.@n",
      "@wA couple small holes with chunks of debris that can be seen scarring "
      "the floor.@n",
      "@wA few small holes with chunks of debris that can be seen scarring the "
      "floor.@n",
      "@wThere are several small holes with chunks of debris that can be seen "
      "scarring the floor.@n",
      "@wMany holes fill the floor of this area, many of which have burn "
      "marks.@n",
      "@wThe floor is severely damaged with many large holes.@n",
      "@wBattle damage covers the entire area. Displayed as a tribute to the "
      "battles that have\r\nbeen waged here.@n",
      "@wThis entire area is falling apart, it has been damaged so badly.@n",
      "@wThis area can not withstand much more damage. Everything has been "
      "damaged so badly it\r\nis hard to recognise any particular details "
      "about their former quality.@n",
      "@wThis area is completely destroyed. Nothing is recognisable. Chunks of "
      "debris\r\nlitter the ground, filling up holes, and overflowing onto "
      "what is left of the\r\nfloor. A haze of smoke is wafting through the "
      "air, creating a chilling atmosphere..@n",
  };
  static const char *const ground_damage[] = {
      "@wA small hole with chunks of debris that can be seen scarring the "
      "ground.@n",
      "@wA couple small craters with chunks of debris that can be seen "
      "scarring the ground.@n",
      "@wA few small craters with chunks of debris that can be seen scarring "
      "the ground.@n",
      "@wThere are several small craters with chunks of debris that can be "
      "seen scarring the ground.@n",
      "@wMany craters fill the ground of this area, many of which have burn "
      "marks.@n",
      "@wThe ground is severely damaged with many large craters.@n",
      "@wBattle damage covers the entire area. Displayed as a tribute to the "
      "battles that have\r\nbeen waged here.@n",
      "@wThis entire area is falling apart, it has been damaged so badly.@n",
      "@wThis area can not withstand much more damage. Everything has been "
      "damaged so badly it\r\nis hard to recognise any particular details "
      "about their former quality.@n",
      "@wThis area is completely destroyed. Nothing is recognisable. Chunks of "
      "debris\r\nlitter the ground, filling up craters, and overflowing onto "
      "what is left of the\r\nground. A haze of smoke is wafting through the "
      "air, creating a chilling atmosphere..@n",
  };
  static const char *const forest_damage[] = {
      "@wA small tree sits in a little crater here.@n",
      "@wTrees have been uprooted by craters in the ground.@n",
      "@wSeveral trees have been reduced to chunks of debris and are\r\nlaying "
      "in a few craters here. @n",
      "@wA large patch of trees have been destroyed and are laying in craters "
      "here.@n",
      "@wSeveral craters have merged into one large crater in one part of this "
      "forest.@n",
      "@wThe open sky can easily be seen through a hole of trees "
      "destroyed\r\nand resting at the bottom of several craters here.@n",
      "@wA good deal of burning tree pieces can be found strewn across the "
      "cratered ground here.@n",
      "@wVery few trees are left standing in this area, replaced instead by "
      "large craters.@n",
      "@wSingle solitary trees can be found still standing here or there in "
      "the area.\r\nThe rest have been almost completely obliterated in recent "
      "conflicts.@n",
      "@w  One massive crater fills this area. This desolate crater leaves "
      "no\r\nevidence of what used to be found in the area. Smoke slowly wafts "
      "into\r\nthe sky from the central point of the crater, creating an "
      "oppressive\r\natmosphere.@n",
  };
  static const char *const mountain_damage[] = {
      "@wA small crater has been burned into the side of this mountain.@n",
      "@wA couple craters have been burned into the side of this mountain.@n",
      "@wBurned bits of boulders can be seen lying at the bottom of a few "
      "nearby craters.@n",
      "@wSeveral bad craters can be seen in the side of the mountain here.@n",
      "@wLarge boulders have rolled down the mountain side and collected in "
      "many nearby craters.@n",
      "@wMany craters are covering the mountainside here.@n",
      "@wThe mountain side has partially collapsed, shedding rubble down "
      "towards its base.@n",
      "@wA peak of the mountain has been blown off, leaving behind a "
      "smoldering tip.@n",
      "@wThe mountain side here has completely collapsed, shedding dangerous "
      "rubble down to its base.@n",
      "@w  Half the mountain has been blown away, leaving a scarred and "
      "jagged\r\nrock in its place. Billowing smoke wafts up from several "
      "parts of the\r\nmountain, filling the nearby skies and blotting out the "
      "sun.@n",
  };

  const char *const *messages = nullptr;
  if (sector == SECT_INSIDE)
    messages = inside_damage;
  else if (sector == SECT_CITY || sector == SECT_FIELD ||
           sector == SECT_HILLS || sector == SECT_IMPORTANT)
    messages = ground_damage;
  else if (sector == SECT_FOREST)
    messages = forest_damage;
  else if (sector == SECT_MOUNTAIN)
    messages = mountain_damage;

  if (!messages || damage <= 0)
    return;

  send_to_char(ch, "\r\n");
  send_to_char(ch, "%s", room_damage_text(damage, messages));
  send_to_char(ch, "\r\n");
}

static bool room_description_survives_total_damage(int sector, int geffect) {
  return sector == SECT_WATER_SWIM || geffect < 0 ||
         sector == SECT_UNDERWATER || sector == SECT_FLYING ||
         sector == SECT_SHOP || sector == SECT_IMPORTANT;
}

void look_at_room(struct room_data *target_room, struct char_data *ch,
                  int ignore_brief) {
  struct room_data *trm = target_room;
  const room_vnum vnum = room_vnum_get(trm);
  const int sector = room_sector_type_get(trm);
  const int damage = room_dmg_get(trm);
  const int gravity = room_gravity_get(trm);
  const int geffect = room_geffect_get(trm);
  const char *name = room_name_get(trm);
  const char *description = room_description_get(trm);

  trig_data *t;

  if (!ch->desc)
    return;

  if (IS_DARK(trm) && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  } else if (PLR_FLAGGED(ch, PLR_EYEC)) {
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];

    sprintbitarray(trm->room_flags, room_bits, RF_ARRAY_MAX, buf, sizeof(buf));
    sprinttype(sector, sector_types, buf2, sizeof(buf2));
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
      send_to_char(ch, "\r\n@wO------------------------------------------------"
                       "----------------------O@n\r\n");
    }

    send_to_char(ch, "@wLocation: @G%-70s@w\r\n", name);
    if (auto sc = room_script_get(trm)) {
      send_to_char(ch, "@D[@GTriggers");
      for (t = TRIGGERS(sc); t; t = t->next)
        send_to_char(ch, " %d", GET_TRIG_VNUM(t));
      send_to_char(ch, "@D] ");
    }
    sprintf(buf3, "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n",
            buf, buf2, vnum);
    send_to_char(ch, "@wFlags: %-70s@w\r\n", buf3);
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------"
                       "------------------O@n\r\n");
    }
  } else {
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------"
                       "------------------O@n\r\n");
    }
    send_to_char(ch, "@wLocation: %-70s@n\r\n", name);
    show_room_planet(ch, trm, vnum);
    show_room_gravity(ch, gravity);
    if (room_flagged(trm, ROOM_REGEN)) {
      send_to_char(ch,
                   "@CA feeling of calm and relaxation fills this room.@n\r\n");
    }
    if (room_flagged(trm, ROOM_AURA)) {
      send_to_char(ch,
                   "@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
    }
    if (room_flagged(trm, ROOM_HBTC)) {
      send_to_char(ch, "@rThis room feels like it opperates in a different "
                       "time frame.@n\r\n");
    }
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------"
                       "------------------O@n\r\n");
    }
  }

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) ||
      room_flagged(trm, ROOM_DEATH)) {
    if (damage <= 99 ||
        (damage == 100 &&
         room_description_survives_total_damage(sector, geffect)))
      send_to_char(ch, "@w%s@n", description);

    show_room_damage(ch, sector, damage);

    if (geffect >= 1 && geffect <= 5) {
      send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
    }
    if (geffect >= 6) {
      send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
    }
    if (geffect < 0) {
      send_to_char(ch, "@cThe entire area is flooded with a @Cmystical@c cube "
                       "of @Bwater!@n\r\n");
    }
  }
  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NODEC))
    do_auto_exits2(target_room, ch);
  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    do_auto_exits(target_room, ch, EXIT_LEV(ch));

  /* now list characters & objects */
  if (room_flagged(trm, ROOM_GARDEN1)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n",
                 check_saveroom_count(ch, NULL));
  } else if (room_flagged(trm, ROOM_GARDEN2)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n",
                 check_saveroom_count(ch, NULL));
  } else if (room_flagged(trm, ROOM_HOUSE)) {
    send_to_char(ch, "@D[@GItems Stored@D: @g%d@D]@n\r\n",
                 check_saveroom_count(ch, NULL));
  }
  list_obj_to_char(inv_for_room(trm), ch, SHOW_OBJ_LONG, FALSE);
  list_char_to_char(trm, ch);
}

static void look_in_direction(struct char_data *ch, int dir) {
  if (EXIT(ch, dir)) {
    if (exit_general_description_get(EXIT(ch, dir)))
      send_to_char(ch, "%s", exit_general_description_get(EXIT(ch, dir)));
    else {
      struct obj_data *obj = char_inventory_search_vnum(ch, 17, FALSE, 0);
      if (!obj) {
        send_to_char(ch, "You were unable to discern anything about that "
                         "direction. Try looking again...\r\n");
        obj = read_object(17, VIRTUAL);
        obj_to_char(obj, ch);
      }
    }

    if (exit_flagged(EXIT(ch, dir), EX_ISDOOR) && exit_keyword_get(EXIT(ch, dir))) {
      if (!exit_flagged(EXIT(ch, dir), EX_SECRET) &&
          exit_flagged(EXIT(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is closed.\r\n",
                     fname(exit_keyword_get(EXIT(ch, dir))));
      else if (!exit_flagged(EXIT(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is open.\r\n", fname(exit_keyword_get(EXIT(ch, dir))));
    }
  } else
    send_to_char(ch, "Nothing special there...\r\n");
}

static void look_in_obj(struct char_data *ch, char *arg) {
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg,
                                 FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP,
                                 ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if (find_exdesc(arg, obj->ex_description) != NULL && !bits)
    send_to_char(ch, "There's nothing inside that!\r\n");
  else if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&
           !OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) {
    if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < 0) {
      /* You can look through the portal to the destination */
      /* where does this lead to? */
      struct room_data *portal_dest =
          room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
      if (!portal_dest) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else if (IS_DARK(portal_dest) && !CAN_SEE_IN_DARK(ch) &&
                 !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else {
        send_to_char(
            ch, "After seconds of concentration you see the image of %s.\r\n",
            room_name_get(portal_dest));
      }
    } else if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
      /* display the appropriate description from the list of descriptions
       */
      send_to_char(ch, "%s\r\n",
                   portal_appearance[GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR)]);
    } else {
      /* We shouldn't really get here, so give a default message */
      send_to_char(ch, "All you can see is the glow of the portal.\r\n");
    }
  } else if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else if (GET_OBJ_VAL(obj, VAL_VEHICLE_APPEAR) < 0) {
      /* You can look inside the vehicle */
      /* where does this lead to? */
      struct room_data *vehicle_inside =
          room_by_id(GET_OBJ_VAL(obj, VAL_VEHICLE_ROOM));
      if (!vehicle_inside) {
        send_to_char(ch, "You cannot see inside that.\r\n");
      } else if (IS_DARK(vehicle_inside) && !CAN_SEE_IN_DARK(ch) &&
                 !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "It is pitch black...\r\n");
      } else {
        send_to_char(ch, "You look inside and see:\r\n");
        look_at_room(vehicle_inside, ch, 0);
      }
    } else {
      send_to_char(ch, "You cannot see inside that.\r\n");
    }
  } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
    look_out_window(ch, arg);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
             (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
             (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) &&
             (GET_OBJ_TYPE(obj) != ITEM_PORTAL)) {
    send_to_char(ch, "There's nothing inside that!\r\n");
  } else if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) ||
             (GET_OBJ_TYPE(obj) == ITEM_PORTAL)) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else {
      send_to_char(ch, "%s", obj->short_description);
      if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER &&
          (GET_OBJ_VNUM(obj) == 697 || GET_OBJ_VNUM(obj) == 698 ||
           GET_OBJ_VNUM(obj) == 682 || GET_OBJ_VNUM(obj) == 683 ||
           GET_OBJ_VNUM(obj) == 684)) {
        act("$n looks in $p.", TRUE, ch, obj, 0, TO_ROOM);
      }
      switch (bits) {
      case FIND_OBJ_INV:
        send_to_char(ch, " (carried): \r\n");
        break;
      case FIND_OBJ_ROOM:
        send_to_char(ch, " (here): \r\n");
        break;
      case FIND_OBJ_EQUIP:
        send_to_char(ch, " (used): \r\n");
        break;
      }

      list_obj_to_char(inv_for_obj(obj), ch, SHOW_OBJ_SHORT, TRUE);
    }
  } else { /* item must be a fountain or drink container */
    if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) <= 0 &&
        (!GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) == 1))
      send_to_char(ch, "It is empty.\r\n");
    else {
      if (GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) < 0) {
        char buf2[MAX_STRING_LENGTH];
        sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2,
                   sizeof(buf2));
        send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
      } else if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) >
                 GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY)) {
        send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
      } else {
        char buf2[MAX_STRING_LENGTH];
        amt = GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
        int leftin = GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL);
        sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2,
                   sizeof(buf2));
        if (leftin == amt) {
          send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        } else if (leftin >= amt * .8) {
          send_to_char(ch, "It's almost full of a %s liquid.\r\n", buf2);
        } else if (leftin >= amt * .5) {
          send_to_char(ch, "It's about half full of a %s liquid.\r\n", buf2);
        } else if (leftin >= amt * .2) {
          send_to_char(ch, "It's less than half full of a %s liquid.\r\n",
                       buf2);
        } else if (leftin > 0) {
          send_to_char(ch, "It's barely filled with a %s liquid.\r\n", buf2);
        } else {
          send_to_char(ch, "It's empty.\r\n");
        }
      }
    }
  }
}

char *find_exdesc(char *word, struct extra_descr_data *list) {
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    /*if (isname(word, i->keyword))*/
    if (*i->keyword == '.' ? isname(word, i->keyword + 1)
                           : isname(word, i->keyword))
      return (i->description);

  return (NULL);
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
static void look_at_target(struct char_data *ch, char *arg, int cmread) {
  int bits, found = FALSE, fnum, i = 0, msg = 1;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;
  char number[MAX_STRING_LENGTH];

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  if (cmread) {
    char_inventory_iterate(ch, [&](auto inv_obj) {
      if (GET_OBJ_TYPE(inv_obj) == ITEM_BOARD) {
        found = TRUE;
        obj = inv_obj;
        return false;
      }
      return true;
    });
    if (!obj) {
      room_contents_iterate(char_room_get(ch), [&](auto content) {
        if (GET_OBJ_TYPE(content) == ITEM_BOARD) {
          found = TRUE;
          obj = content;
          return false;
        }
        return true;
      });
    }
    if (obj) {
      arg = one_argument(arg, number);
      if (!*number) {
        send_to_char(ch, "Read what?\r\n");
        return;
      }

      /* Okay, here i'm faced with the fact that the person could be
         entering in something like 'read 5' or 'read 4.mail' .. so, whats the
         difference between the two?  Well, there's a period in the second,
         so, we'll just stick with that basic difference */

      if (isname(number, obj->name)) {
        show_board(GET_OBJ_VNUM(obj), ch);
      } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
                 (strchr(number, '.'))) {
        sprintf(arg, "%s %s", number, arg);
        look_at_target(ch, arg, 0);
      } else {
        board_display_msg(GET_OBJ_VNUM(obj), ch, msg);
      }
    }
  } else {
    bits = generic_find(
        arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch,
        &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char != NULL) {
      look_at_char(found_char, ch);
      if (ch != found_char) {
        if (!AFF_FLAGGED(ch, AFF_HIDE)) {
          act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
          act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
        }
      }
      return;
    }

    /* Strip off "number." from 2.foo and friends. */
    if (!(fnum = get_number(&arg))) {
      send_to_char(ch, "Look at what?\r\n");
      return;
    }

    /* Does the argument match an extra desc in the room? */
    if ((desc = find_exdesc(arg, room_ex_description_get(char_room_get(ch)))) != NULL &&
        ++i == fnum) {
      send_to_char(ch, "%s", desc);
      return;
    }

    /* Does the argument match an extra desc in the char's equipment? */
    char_equipment_iterate(ch, [&](auto slot, auto eq) {
      if (CAN_SEE_OBJ(ch, eq))
        if ((desc = find_exdesc(arg, eq->ex_description)) != NULL &&
            ++i == fnum) {
          send_to_char(ch, "%s", desc);
          if (isname(arg, eq->name)) {
            if (GET_OBJ_TYPE(eq) == ITEM_WEAPON) {
              send_to_char(ch, "The weapon type of %s is a %s.\r\n",
                           GET_OBJ_SHORT(eq),
                           weapon_type[(int)GET_OBJ_VAL(eq,
                                                        VAL_WEAPON_SKILL)]);
            }
            if (GET_OBJ_TYPE(eq) == ITEM_SPELLBOOK) {
              display_spells(ch, eq);
            }
            if (GET_OBJ_TYPE(eq) == ITEM_SCROLL) {
              display_scroll(ch, eq);
            }
            diag_obj_to_char(eq, ch);
            send_to_char(ch, "It appears to be made of %s",
                         material_names[GET_OBJ_MATERIAL(eq)]);
          }
          found = TRUE;
          return false;
        }
      return true;
    });

    /* Does the argument match an extra desc in the char's inventory? */
    char_inventory_iterate(ch, [&](auto obj) {
      if (found) return false;
      if (CAN_SEE_OBJ(ch, obj))
        if ((desc = find_exdesc(arg, obj->ex_description)) != NULL &&
            ++i == fnum) {
          if (GET_OBJ_TYPE(obj) == ITEM_BOARD) {
            show_board(GET_OBJ_VNUM(obj), ch);
          } else {
            send_to_char(ch, "%s", desc);
            if (isname(arg, obj->name)) {
              if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
                send_to_char(
                    ch, "The weapon type of %s is a %s.\r\n",
                    GET_OBJ_SHORT(obj),
                    weapon_type[(int)GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
              }
              if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
                display_spells(ch, obj);
              }
              if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
                display_scroll(ch, obj);
              }
              diag_obj_to_char(obj, ch);
              send_to_char(ch, "It appears to be made of %s, and weights %s",
                           material_names[GET_OBJ_MATERIAL(obj)],
                           add_commas(GET_OBJ_WEIGHT(obj)));
            }
          }
          found = TRUE;
          return false;
        }
      return true;
    });

    /* Does the argument match an extra desc of an object in the room? */
    room_contents_iterate(char_room_get(ch), [&](auto obj) {
      if (found) return true;
      if (CAN_SEE_OBJ(ch, obj))
        if ((desc = find_exdesc(arg, obj->ex_description)) != NULL &&
            ++i == fnum) {
          if (GET_OBJ_TYPE(obj) == ITEM_BOARD) {
            show_board(GET_OBJ_VNUM(obj), ch);
          } else {
            send_to_char(ch, "%s", desc);
            if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CEnter hatch\r\n");
            } else if (GET_OBJ_TYPE(obj) == ITEM_HATCH) {
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CLeave@n\r\n");
            } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
              look_out_window(ch, obj->name);
            }

            if (GET_OBJ_TYPE(obj) == ITEM_CONTROL) {
              send_to_char(ch, "@RFUEL@D: %s%s@n\r\n",
                           GET_FUEL(obj) >= 200   ? "@G"
                           : GET_FUEL(obj) >= 100 ? "@Y"
                                                  : "@r",
                           add_commas(GET_FUEL(obj)));
            }
            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
              send_to_char(
                  ch, "The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj),
                  weapon_type[(int)GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
            }
            diag_obj_to_char(obj, ch);
            send_to_char(ch, "It appears to be made of %s, and weights %s",
                         material_names[GET_OBJ_MATERIAL(obj)],
                         add_commas(GET_OBJ_WEIGHT(obj)));
          }
          found = TRUE;
          return false;
        }
      return true;
    });

    /* If an object was found back in generic_find */
    if (bits) {
      if (!found)
        show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
      else {
        if (show_obj_modifiers(found_obj, ch))
          send_to_char(ch, "\r\n");
      }
    } else if (!found)
      send_to_char(ch, "You do not see that here.\r\n");
  }
}

static void look_out_window(struct char_data *ch, char *arg) {
  struct obj_data *i, *viewport = NULL, *vehicle = NULL;
  struct char_data *dummy = NULL;
  struct room_data *target_room = NULL;
  int bits, door;

  /* First, lets find something to look out of or through. */
  if (*arg) {
    /* Find this object and see if it is a window */
    if (!(bits =
              generic_find(arg, FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
                           ch, &dummy, &viewport))) {
      send_to_char(ch, "You don't see that here.\r\n");
      return;
    } else if (GET_OBJ_TYPE(viewport) != ITEM_WINDOW) {
      send_to_char(ch, "You can't look out that!\r\n");
      return;
    }
  } else if (OUTSIDE(ch)) {
    /* yeah, sure stupid */
    send_to_char(ch, "But you are already outside.\r\n");
    return;
  } else {
    /* Look for any old window in the room */
    room_contents_iterate(char_room_get(ch), [&](auto i) {
      if ((GET_OBJ_TYPE(i) == ITEM_WINDOW) && isname("window", i->name)) {
        viewport = i;
      }
      return true;
    });
  }
  if (!viewport) {
    /* Nothing suitable to look through */
    send_to_char(ch, "You don't seem to be able to see outside.\r\n");
  } else if (OBJVAL_FLAGGED(viewport, CONT_CLOSEABLE) &&
             OBJVAL_FLAGGED(viewport, CONT_CLOSED)) {
    /* The window is closed */
    send_to_char(ch, "It is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1) < 0) {
      /* We are looking out of the room */
      if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4) < 0) {
        /* Look for the default "outside" room */
        room_exits_iterate(char_room_get(ch), [&](auto door, auto exit) {
           auto dest = exit_dest_get(exit);
           if (!dest) return true;
           if (!room_flagged(dest, ROOM_INDOORS)) {
             target_room = dest;
             return false;
           }
           return true;
         });
      } else {
        target_room = room_by_id(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4));
      }
    } else {
      /* We are looking out of a vehicle */
      if ((vehicle =
               find_vehicle_by_vnum(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1))))
        target_room = obj_room_get(vehicle);
    }
    if (target_room == NULL) {
      send_to_char(ch, "You don't seem to be able to see outside.\r\n");
    } else {
      if (viewport->action_description)
        act(viewport->action_description, TRUE, ch, viewport, 0, TO_CHAR);
      else
        act("$n looks out the window.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You look outside and see:\r\n");
      look_at_room(target_room, ch, 0);
    }
  }
}

ACMD(do_finger) {

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What user are you wanting to look at?\r\n");
    return;
  }
  if (!readUserIndex(arg)) {
    send_to_char(ch, "That user does not exist\r\n");
    return;
  } else {
    fingerUser(ch, arg);
    return;
  }
}

ACMD(do_rptrans) {
  struct char_data *vict = NULL;
  struct descriptor_data *k;
  int amt = 0;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: exchange (target) (amount)\r\n");
    return;
  }

  amt = atoi(arg2);

  if (amt <= 0) {
    send_to_char(ch, "Are you being funny?\r\n");
    return;
  }

  if (amt > GET_RP(ch)) {
    send_to_char(ch, "@WYou only have @C%d@W RPP!@n\r\n", GET_RP(ch));
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(ch, "That is not a recognised user file.\r\n");
    return;
  }

  for (k = descriptor_list; k; k = k->next) {
    if (IS_NPC(k->character))
      continue;
    if (STATE(k) != CON_PLAYING)
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }
  if (vict == NULL) {
    userWrite(NULL, 0, amt, 0, arg);
  } else {
    GET_RP(vict) += amt;
    vict->desc->rpp += amt;
    userWrite(vict->desc, 0, 0, 0, "index");
    GET_TRP(vict) += amt;
    save_char(vict);
    send_to_char(vict, "@W%s gives @C%d@W of their RPP to you. How nice!\r\n",
                 GET_NAME(ch), amt);
  }
  GET_RP(ch) -= amt;
  ch->desc->rpp -= amt;
  userWrite(ch->desc, 0, 0, 0, "index");
  send_to_char(
      ch,
      "@WYou exchange @C%d@W RPP to user @c%s@W for a warm fuzzy feeling.\r\n",
      amt, CAP(arg));
  mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
         "EXCHANGE: %s gave %d RPP to user %s", GET_NAME(ch), amt, arg);
  save_char(ch);
}

ACMD(do_rdisplay) {
  skip_spaces(&argument);

  if (IS_NPC(ch)) {
    return;
  }

  if (!*argument) {
    send_to_char(ch, "Clearing room display.\r\n");
    GET_RDISPLAY(ch) = "Empty";
  } else {
    char derp[MAX_STRING_LENGTH];

    strcpy(derp, argument);

    send_to_char(ch, "You set your display to; %s\r\n", derp);
    GET_RDISPLAY(ch) = strdup(derp);
  }
}

int perf_skill(int skill) {

  if (!skill) {
    return 0;
  }

  switch (skill) {
  case 464:
  case 441:
  case 444:
  case 475:
  case 474:
  case 476:
  case 488:
  case 472:
  case 485:
  case 442:
  case 510:
  case 533:
    return 1;
    break;
  default:
    return 0;
    break;
  }
}

ACMD(do_perf) {
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int i, skill = 1, found = FALSE, type = 0;

  two_arguments(argument, arg, arg2);

  if (IS_NPC(ch) || GET_ADMLEVEL(ch) > 0) {
    send_to_char(ch, "I don't think so.\r\n");
    return;
  }

  if (!*arg || !*arg2) {
    send_to_char(ch, "@WType @G1@D: @wOver Charged@n\r\n");
    send_to_char(ch, "@WType @G2@D: @wAccurate@n\r\n");
    send_to_char(ch, "@WType @G3@D: @wEfficient@n\r\n");
    send_to_char(ch, "Syntax: perfect (skillname) (type 1/2/or 3)\r\n");
    return;
  }
  if (strlen(arg) < 4) {
    send_to_char(ch,
                 "The skill name should be longer than 3 characters...\r\n");
    return;
  }
  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (spell_info[i].skilltype != SKTYPE_SKILL)
      continue;

    if (found == TRUE)
      continue;

    if (strstr(spell_info[i].name, arg)) {
      skill = i;
      found = TRUE;
    }
  }
  if (found == FALSE) {
    send_to_char(ch, "The skill %s doesn't exist.\r\n", arg);
    return;
  }
  if (!GET_SKILL(ch, skill)) {
    send_to_char(ch, "You don't know %s.\r\n", arg);
    return;
  }
  if (GET_SKILL(ch, skill) < 100) {
    send_to_char(
        ch, "You have not mastered the skill %s and thus can't perfect it.\r\n",
        arg);
    return;
  }
  if (GET_SKILL_PERF(ch, skill) > 0) {
    send_to_char(ch,
                 "You have already mastered the skill %s and chosen how to "
                 "perfect it.\r\n",
                 arg);
    return;
  }
  if (!perf_skill(skill)) {
    send_to_char(ch, "You can't perfect that type of skill.\r\n");
    return;
  }
  if (atoi(arg2) < 1 || atoi(arg2) > 3) {
    send_to_char(ch, "@WType @G1@D: @wOver Charged@n\r\n");
    send_to_char(ch, "@WType @G2@D: @wAccurate@n\r\n");
    send_to_char(ch, "@WType @G3@D: @wEfficient@n\r\n");
    send_to_char(ch, "@RType must be a number between 1 and 3.@n\r\n");
    return;
  } else {
    type = atoi(arg2);
    switch (type) {
    case 1:
      send_to_char(
          ch, "You perfect the skill %s so that you can over charge it!\r\n",
          spell_info[skill].name);
      GET_SKILL_PERF(ch, skill) = 1;
      break;
    case 2:
      send_to_char(ch,
                   "You perfect the skill %s so that you have supreme accuracy "
                   "with it!\r\n",
                   spell_info[skill].name);
      GET_SKILL_PERF(ch, skill) = 2;
      break;
    case 3:
      send_to_char(ch,
                   "You perfect the skill %s so that you require a lower "
                   "minimum charge for it!\r\n",
                   spell_info[skill].name);
      GET_SKILL_PERF(ch, skill) = 3;
      break;
    }
  }
}

ACMD(do_look) {
  int look_type;

  if (!ch->desc)
    return;
  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (PLR_FLAGGED(ch, PLR_EYEC))
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
  else if (IS_DARK(char_room_get(ch)) && !CAN_SEE_IN_DARK(ch) &&
           !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(char_room_get(ch), ch); /* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[200];

    if (subcmd == SCMD_READ) {
      one_argument(argument, arg);
      if (!*arg)
        send_to_char(ch, "Read what?\r\n");
      else
        look_at_target(ch, arg, 1);
      return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
      if (subcmd == SCMD_SEARCH) {
        search_room(ch);
      } else {
        look_at_room(char_room_get(ch), ch, 1);
        if (GET_ADMLEVEL(ch) < 1 && !AFF_FLAGGED(ch, AFF_HIDE)) {
          // act("@w$n@w looks around the room.@n", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg, "inside") && EXIT(ch, INDIR) && !*arg2) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, INDIR);
      else
        look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
      search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") || is_abbrev(arg, "into") ||
               is_abbrev(arg, "onto")) {
      look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") || is_abbrev(arg, "through") ||
                is_abbrev(arg, "thru")) &&
               (subcmd == SCMD_LOOK) && *arg2) {
      look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") && (subcmd == SCMD_LOOK) &&
               !EXIT(ch, OUTDIR)) {
      look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, FALSE)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, FALSE)) >= 0) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, FALSE)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, FALSE)) >= 0)) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
        look_at_target(ch, arg2, 0);
    } else if (is_abbrev(arg, "around")) {
      struct extra_descr_data *i;
      int found = 0;

      for (i = room_ex_description_get(char_room_get(ch)); i; i = i->next) {
        if (*i->keyword != '.') {
          send_to_char(ch, "%s%s:\r\n%s", (found ? "\r\n" : ""), i->keyword,
                       i->description);
          found = 1;
        }
      }
      if (!found)
        send_to_char(ch, "You couldn't find anything noticeable.\r\n");
    } else if (find_exdesc(arg, room_ex_description_get(char_room_get(ch))) != NULL) {
      look_at_target(ch, arg, 0);
    } else {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
        look_at_target(ch, arg, 0);
    }
  }
}

ACMD(do_examine) {
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg), 0); /* strcpy: OK */

  generic_find(arg,
               FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP,
               ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
        (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
        (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}

static void trans_check(struct char_data *ch, struct char_data *vict) {
  /* Rillao: transloc, add new transes here */
  if (IS_HUMAN(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CSuper Human First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Human Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CSuper Human Third@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Human Fourth@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_HOSHIJIN(vict)) {
    if (GET_MIMIC(vict) == 0 || vict == ch) {
      if (GET_PHASE(vict) == 1) {
        send_to_char(
            ch, "         @cCurrent Transformation@D: @CBirth Phase@n\r\n");
      } else if (GET_PHASE(vict) == 2) {
        send_to_char(ch,
                     "         @cCurrent Transformation@D: @CLife Phase@n\r\n");
      } else {
        send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
      }
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if ((IS_SAIYAN(vict) || IS_HALFBREED(vict)) &&
             !PLR_FLAGGED(vict, PLR_LSSJ)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1) && !PLR_FLAGGED(vict, PLR_FPSSJ)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Saiyan First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS1) && PLR_FLAGGED(vict, PLR_FPSSJ)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @YFull Powered "
                       "@CSuper Saiyan@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Saiyan Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Saiyan Third@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Saiyan Fourth@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_OOZARU)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @COozaru@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_SAIYAN(vict) && PLR_FLAGGED(vict, PLR_LSSJ)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1) && !PLR_FLAGGED(vict, PLR_FPSSJ)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Saiyan First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS1) && PLR_FLAGGED(vict, PLR_FPSSJ)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @YFull Powered "
                       "@CSuper Saiyan@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @YLegendary "
                       "@CSuper Saiyan@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_OOZARU)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @COozaru@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_NAMEK(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CSuper Namek First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Namek Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CSuper Namek Third@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(
          ch,
          "         @cCurrent Transformation@D: @CSuper Namek Fourth@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_ICER(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CTransform First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CTransform Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CTransform Third@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CTransform Fourth@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_KONATSU(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CShadow First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CShadow Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CShadow Third@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CShadow Fourth@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_MUTANT(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CMutate First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CMutate Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CMutate Third@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_BIO(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @CMature@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSemi-perfect@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @CPerfect@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CSuper Perfect@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_ANDROID(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSeries 1.0@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSeries 2.0@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSeries 3.0@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS4)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSeries 4.0@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS5)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSeries 5.0@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS6)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CSeries 6.0@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_MAJIN(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @CAffinity@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @CSuper@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch, "         @cCurrent Transformation@D: @CTrue@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_TRUFFLE(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CAscend First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CAscend Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CAscend Third@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (IS_KAI(vict)) {
    if (PLR_FLAGGED(vict, PLR_TRANS1)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CMystic First@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS2)) {
      send_to_char(
          ch, "         @cCurrent Transformation@D: @CMystic Second@n\r\n");
    } else if (PLR_FLAGGED(vict, PLR_TRANS3)) {
      send_to_char(ch,
                   "         @cCurrent Transformation@D: @CMystic Third@n\r\n");
    } else {
      send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else {
    send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }

} // End trans check

/* do_time moved to lua/characters/pcommands/info/time.lua */
/* do_weather moved to lua/characters/pcommands/info/weather.lua */

extern "C" void obj_from_container(struct obj_data *obj) { obj_from_obj(obj); }
extern "C" void obj_to_container(struct obj_data *obj, struct obj_data *container) { obj_to_obj(obj, container); }
extern "C" void obj_show_action_to_char(struct obj_data *obj, struct char_data *ch) { show_obj_to_char(obj, ch, SHOW_OBJ_ACTION); }

/* puts -'s instead of spaces */
static void space_to_minus(char *str) {
  while ((str = strchr(str, ' ')) != NULL)
    *str = '-';
}

int search_help(const char *argument, int level) {
  int chk, bot, top, mid, minlen;

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  while (bot <= top) {
    mid = (bot + top) / 2;

    if (!(chk = strncasecmp(argument, help_table[mid].keywords, minlen))) {
      while ((mid > 0) &&
             !strncasecmp(argument, help_table[mid - 1].keywords, minlen))
        mid--;

      while (level < help_table[mid].min_level && mid < (bot + top) / 2)
        mid++;

      if (strncasecmp(argument, help_table[mid].keywords, minlen))
        break;

      return mid;
    } else if (chk > 0)
      bot = mid + 1;
    else
      top = mid - 1;
  }
  return NOWHERE;
}

ACMD(do_help) {
  char buf[MAX_STRING_LENGTH * 4];
  int mid = 0;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  if (!*argument) {
    if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
      send_to_char(ch, "%s", help);
    else
      send_to_char(ch, "%s", ihelp);
    return;
  }

  space_to_minus(argument);

  if ((mid = search_help(argument, GET_ADMLEVEL(ch))) == NOWHERE) {
    int i, found = 0;
    send_to_char(ch, "There is no help on that word.\r\n");
    if (GET_ADMLEVEL(ch) < 3) {
      mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
             "%s tried to get help on %s", GET_NAME(ch), argument);
    }
    for (i = 0; i <= top_of_helpt; i++) {
      if (help_table[i].min_level > GET_ADMLEVEL(ch))
        continue;
      /* To help narrow down results, if they don't start with the same letters,
       * move on */
      if (*argument != *help_table[i].keywords)
        continue;
      if (levenshtein_distance(argument, help_table[i].keywords) <= 2) {
        if (!found) {
          send_to_char(ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(ch, "  %s\r\n", help_table[i].keywords);
      }
    }
    return;
  }
  if (help_table[mid].min_level > GET_ADMLEVEL(ch)) {
    send_to_char(ch, "There is no help on that word.\r\n");
    return;
  }
  sprintf(buf, "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
               "~~~~~~~@n\n");
  sprintf(buf + strlen(buf), "%s", help_table[mid].entry);
  sprintf(buf + strlen(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                             "~~~~~~~~~~~~~~~~~~~~~@n\n");
  if (GET_ADMLEVEL(ch) > 0) {
    sprintf(buf + strlen(buf), "@WHelp File Level@w: @D(@R%d@D)@n\n",
            help_table[mid].min_level);
  }
  send_to_char(ch, "%s", buf);
}

#define WHO_FORMAT                                                             \
  "Usage: who [minlev[-maxlev]] [-k] [-n name] [-q] [-r] [-s] [-z]\r\n"

/* Written by Rhade */
ACMD(do_who) {
  struct descriptor_data *d;
  struct char_data *tch;
  int i, num_can_see = 0;
  char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int low = 0, high = CONFIG_LEVEL_CAP, localwho = 0, questwho = 0, hide = 0;
  int showclass = 0, short_list = 0, outlaws = 0;
  int who_room = 0, showgroup = 0, showleader = 0;
  char *line_color = "@n";

  skip_spaces(&argument);
  strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  struct {
    char *disp;
    int min_level;
    int max_level;
    int count; /* must always start as 0 */
  } rank[] = {
      {"\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs  "
       " @D]  @c------------@n\r\n",
       ADMLVL_IMMORT, ADMLVL_IMPL, 0},
      {"\r\n@D[@wx@D]@yxxxxxxxxxx@W  [    @GImmortals   @W]  "
       "@yxxxxxxxxxx@D[@wx@D]@n\r\n",
       ADMLVL_IMMORT + 8, ADMLVL_GRGOD + 8, 0},
      {"\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  "
       "@c------------@n\r\n",
       0, ADMLVL_IMMORT - 1, 0}
      /*{ "\r\n@GAdministrators@n\r\n\r\n", ADMLVL_GRGOD, ADMLVL_IMPL, 0},
      { "\r\n@GImmortals@n\r\n\r\n"     , ADMLVL_IMMORT, ADMLVL_GRGOD - 1, 0},
      { "\r\n@GMortal@n\r\n\r\n"        , 0, ADMLVL_IMMORT - 1, 0 }*/
  };
  char *tmstr;
  tmstr = (char *)asctime(localtime(&PCOUNTDATE));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  int num_ranks = sizeof(rank) / sizeof(rank[0]);
  send_to_char(ch, "\r\n      @r{@b===============  @D[  "
                   "@DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt "
                   "@DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");
  for (d = descriptor_list; d && !short_list; d = d->next) {
    if (!IS_PLAYING(d))
      continue;
    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (GET_ADMLEVEL(tch) >= ADMLVL_IMMORT)
      line_color = "@w";
    else
      line_color = "@w";

    if (CAN_SEE(ch, tch) && IS_PLAYING(d)) {
      if (*name_search && strcasecmp(GET_NAME(tch), name_search) &&
          !strstr(GET_TITLE(tch), name_search))
        continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
        continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
          !PLR_FLAGGED(tch, PLR_THIEF))
        continue;
      if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
        continue;
      if (localwho && room_zone_vnum_get(char_room_get(ch)) != room_zone_vnum_get(char_room_get(tch)))
        continue;
      if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch &&
          GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
        hide += 1;
        continue;
      }
      if (who_room && (char_room_get(tch) != char_room_get(ch)))
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
        continue;
      if (showgroup && (!MASTER(tch) || !char_condition_has(tch, "group")))
        continue;
      for (i = 0; i < num_ranks; i++)
        if (GET_ADMLEVEL(tch) >= rank[i].min_level &&
            GET_ADMLEVEL(tch) <= rank[i].max_level)
          rank[i].count++;
    }
  }

  for (i = 0; i < num_ranks; i++) {
    if (!rank[i].count && !short_list)
      continue;

    if (short_list)
      send_to_char(ch, "Players\r\n-------\r\n");
    else
      send_to_char(ch, "%s", rank[i].disp);

    for (d = descriptor_list; d; d = d->next) {
      if (!IS_PLAYING(d))
        continue;
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if ((GET_ADMLEVEL(tch) < rank[i].min_level ||
           GET_ADMLEVEL(tch) > rank[i].max_level) &&
          !short_list)
        continue;
      if (!IS_PLAYING(d))
        continue;
      if (*name_search && strcasecmp(GET_NAME(tch), name_search) &&
          !strstr(GET_TITLE(tch), name_search))
        continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
        continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
          !PLR_FLAGGED(tch, PLR_THIEF))
        continue;
      if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
        continue;
      if (localwho && room_zone_vnum_get(char_room_get(ch)) != room_zone_vnum_get(char_room_get(tch)))
        continue;
      if (who_room && (char_room_get(tch) != char_room_get(ch)))
        continue;
      if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch &&
          GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
        continue;
      if (showgroup && (!MASTER(tch) || !char_condition_has(tch, "group")))
        continue;
      if (showleader && (!char_follower_count(tch) || !char_condition_has(tch, "group")))
        continue;

      if (short_list) {
        send_to_char(ch,
                     "               @B[@W%3d @Y%s @C%s@B]@W %-12.12s@n%s@n",
                     GET_LEVEL(tch), RACE_ABBR(tch), CLASS_ABBR(tch),
                     GET_NAME(tch), ((!(++num_can_see % 4)) ? "\r\n" : ""));
      } else {
        num_can_see++;

        char usr[100];
        sprintf(usr, "@W(@R%s@W)%s", tch->desc->user,
                PLR_FLAGGED(tch, PLR_BIOGR) ? ""
                                            : (SPOILED(tch) ? " @R*@n" : ""));
        send_to_char(ch, "%s               @D<@C%-12s@D> %s@w%s", line_color,
                     GET_ADMLEVEL(ch) > 0
                         ? GET_NAME(tch)
                         : (GET_ADMLEVEL(tch) > 0
                                ? GET_NAME(tch)
                                : (GET_USER(tch) ? GET_USER(tch) : "NULL")),
                     GET_ADMLEVEL(ch) > 0 ? usr : "", line_color);

        if (GET_ADMLEVEL(tch)) {
          send_to_char(ch, " (%s)", admin_level_names[GET_ADMLEVEL(tch)]);
        }

        if (d->snooping && d->snooping->character != ch &&
            GET_ADMLEVEL(ch) >= 3)
          send_to_char(ch, " (Snoop: %s)", GET_NAME(d->snooping->character));
        if (GET_INVIS_LEV(tch))
          send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
        else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
          send_to_char(ch, " (invis)");

        if (PLR_FLAGGED(tch, PLR_MAILING))
          send_to_char(ch, " (mailing)");
        else if (d->olc)
          send_to_char(ch, " (OLC)");
        else if (PLR_FLAGGED(tch, PLR_WRITING))
          send_to_char(ch, " (writing)");

        if (d->original)
          send_to_char(ch, " (out of body)");

        if (d->connected == CON_OEDIT)
          send_to_char(ch, " (O Edit)");
        if (d->connected == CON_MEDIT)
          send_to_char(ch, " (M Edit)");
        if (d->connected == CON_ZEDIT)
          send_to_char(ch, " (Z Edit)");
        if (d->connected == CON_SEDIT)
          send_to_char(ch, " (S Edit)");
        if (d->connected == CON_REDIT)
          send_to_char(ch, " (R Edit)");
        if (d->connected == CON_TEDIT)
          send_to_char(ch, " (T Edit)");
        if (d->connected == CON_TRIGEDIT)
          send_to_char(ch, " (T Edit)");
        if (d->connected == CON_AEDIT)
          send_to_char(ch, " (S Edit)");
        if (d->connected == CON_CEDIT)
          send_to_char(ch, " (C Edit)");
        if (d->connected == CON_HEDIT)
          send_to_char(ch, " (H Edit)");
        if (PRF_FLAGGED(tch, PRF_DEAF))
          send_to_char(ch, " (DEAF)");
        if (PRF_FLAGGED(tch, PRF_NOTELL))
          send_to_char(ch, " (NO TELL)");
        if (PRF_FLAGGED(tch, PRF_NOGOSS))
          send_to_char(ch, " (NO OOC)");
        if (PLR_FLAGGED(tch, PLR_NOSHOUT))
          send_to_char(ch, " (MUTED)");
        if (PRF_FLAGGED(tch, PRF_HIDE))
          send_to_char(ch, " (WH)");
        if (PRF_FLAGGED(tch, PRF_BUILDWALK))
          send_to_char(ch, " (Buildwalking)");
        if (PRF_FLAGGED(tch, PRF_AFK))
          send_to_char(ch, " (AFK)");
        if (char_condition_has(tch, "fishing") && GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
          send_to_char(ch, " (@BFISHING@n)");
        if (PRF_FLAGGED(tch, PRF_NOWIZ))
          send_to_char(ch, " (NO WIZ)");
        send_to_char(ch, "@n\r\n");
      }
    }
    send_to_char(ch, "\r\n");
    if (short_list)
      break;
  }

  if (!num_can_see)
    send_to_char(ch, "                            Nobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(
        ch, "                         One lonely character displayed.\r\n");
  else {
    send_to_char(ch,
                 "                           @Y%d@w characters displayed.\r\n",
                 num_can_see);
    if (hide > 0) {
      int bam = FALSE;
      if (hide > 1) {
        bam = TRUE;
      }
      send_to_char(
          ch, "                           and @Y%d@w character%s hidden.\r\n",
          hide, bam ? "s" : "");
    }
  }
  if (circle_restrict > 0 && circle_restrict <= 100) {
    send_to_char(
        ch,
        "                      @rThe mud has been wizlocked to lvl %d@n\r\n",
        circle_restrict);
  }
  if (circle_restrict == 101) {
    send_to_char(ch, "                      @rThe mud has been wizlocked to "
                     "IMMs only.@n\r\n");
  }
  send_to_char(ch, "      "
                   "@r{@b======================================================"
                   "===========@r}@n\r\n");
  send_to_char(ch, "           @cHighest Logon Count Ever@D: @Y%d@w, on %s\r\n",
               HIGHPCOUNT, tmstr);
  send_to_char(
      ch, "                        @cHighest Logon Count Today@D: @Y%d@n\r\n",
      PCOUNT);
}

#define USERS_FORMAT                                                           \
  "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users) {
  char line[200], line2[220], idletime[10];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
  while (*buf) {
    char buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
        outlaws = 1;
        playing = 1;
        strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'p':
        playing = 1;
        strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'd':
        deadweight = 1;
        strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'l':
        playing = 1;
        half_chop(buf1, arg, buf);
        sscanf(arg, "%d-%d", &low, &high);
        break;
      case 'n':
        playing = 1;
        half_chop(buf1, name_search, buf);
        break;
      case 'h':
        playing = 1;
        half_chop(buf1, host_search, buf);
        break;
      default:
        send_to_char(ch, "%s", USERS_FORMAT);
        return;
      } /* end of switch */

    } else { /* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  } /* end while (parser) */
  send_to_char(ch, "Num Name                 User-name            State        "
                   "  Idl Login    C\r\n"
                   "--- -------------------- -------------------- "
                   "-------------- --- -------- -\r\n");

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d)) {
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if (*host_search && !strstr(d->host, host_search))
        continue;
      if (*name_search && strcasecmp(GET_NAME(tch), name_search))
        continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
        continue;
      if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch &&
          GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
        continue;
      }
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
          !PLR_FLAGGED(tch, PLR_THIEF))
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
        continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (GET_INVIS_LEV(tch) > GET_ADMLEVEL(ch))
        continue;
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[STATE(d)]);

    if (d->character && STATE(d) == CON_PLAYING &&
        GET_ADMLEVEL(d->character) <= GET_ADMLEVEL(ch))
      sprintf(idletime, "%3d",
              d->character->timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    sprintf(line, "%3d %-20s %-20s %-14s %-3s %-8s %1s ", d->desc_num,
            d->original && d->original->name     ? d->original->name
            : d->character && d->character->name ? d->character->name
                                                 : "UNDEFINED",
            d->user ? d->user : "UNKNOWN", state, idletime, timeptr, "N");
    if (d->host && *d->host)
      sprintf(line + strlen(line), "\n%3d [%s Site: %s]\r\n", d->desc_num,
              d->user ? d->user : "UNKNOWN", d->host);
    else
      sprintf(line + strlen(line), "\n%3d [%s Site: Hostname unknown]\r\n",
              d->desc_num, d->user ? d->user : "UNKNOWN");

    if (STATE(d) != CON_PLAYING) {
      sprintf(line2, "@g%s@n", line);
      strcpy(line, line2);
    }
    if (STATE(d) != CON_PLAYING ||
        (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps) {
  char arg[MAX_INPUT_LENGTH];
  char bum[10000];
  one_argument(argument, arg);

  switch (subcmd) {
  case SCMD_CREDITS:
    send_to_char(ch, "%s", credits);
    break;
  case SCMD_NEWS:
    send_to_char(ch, "%s", news);
    GET_LPLAY(ch) = time(0);
    break;
  case SCMD_INFO:
    send_to_char(ch, "%s", info);
    break;
  case SCMD_WIZLIST:
    send_to_char(ch, "%s", wizlist);
    break;
  case SCMD_IMMLIST:
    send_to_char(ch, "%s", immlist);
    break;
  case SCMD_HANDBOOK:
    send_to_char(ch, "%s", handbook);
    break;
  case SCMD_POLICIES:
    sprintf(bum, "--------------------\r\n%s\r\n--------------------\r\n",
            policies);
    send_to_char(ch, "%s", bum);
    break;
  case SCMD_MOTD:
    send_to_char(ch, "%s", motd);
    break;
  case SCMD_IMOTD:
    send_to_char(ch, "%s", imotd);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", GET_NAME(ch));
    break;
  default:
    mud_log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    /*  SYSERR_DESC:
     *  General page string function for such things as 'credits', 'news',
     *  'wizlist', 'clear', 'version'.  This occurs when a call is made to
     *  this routine that is not one of the predefined calls.  To correct
     *  it, either a case needs to be added into the function to account for
     *  the subcmd that is being passed to it, or the call to the function
     *  needs to have the correct subcmd put into place.
     */
    return;
  }
}

static void perform_mortal_where(struct char_data *ch, char *arg) {
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
        continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
        continue;
      if (char_room_get(i) == NULL || !CAN_SEE(ch, i))
        continue;
      if (room_zone_vnum_get(char_room_get(ch)) != room_zone_vnum_get(char_room_get(i)))
        continue;
      send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i), room_name_get(char_room_get(i)));
    }
  } else { /* print only FIRST char, not all. */
    bool found_one = false;
    char_iterate_all_newest([&](struct char_data *tch) {
      if (char_room_get(tch) == NULL || tch == ch)
        return true;
      if (!CAN_SEE(ch, tch) || room_zone_vnum_get(char_room_get(tch)) != room_zone_vnum_get(char_room_get(ch)))
        return true;
      if (!isname(arg, tch->name))
        return true;
      send_to_char(ch, "%-25s - %s\r\n", GET_NAME(tch), room_name_get(char_room_get(tch)));
      found_one = true;
      return false;
    });
    if (!found_one)
      send_to_char(ch, "Nobody around by that name.\r\n");
  }
}

static void print_object_location(int num, struct obj_data *obj,
                                  struct char_data *ch, int recur) {
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (SCRIPT(obj))
    send_to_char(ch, "[T%d]", obj->proto_script->id);

  if (obj_room_get(obj) != NULL)
    send_to_char(ch, "[%5d] %s\r\n", obj_room_vnum_get(obj),
                 room_name_get(obj_room_get(obj)));
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s in room [%d]\r\n",
                 PERS(obj->carried_by, ch),
                 char_room_vnum_get(obj->carried_by));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s in room [%d]\r\n", PERS(obj->worn_by, ch),
                 char_room_vnum_get(obj->worn_by));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description,
                 (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}

static void perform_immort_where(struct char_data *ch, char *arg) {
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, num2 = 0, found = 0;

  const char *planet[11] = {"@GEarth@n",   "@CFrigid@n", "@YVegeta@n",
                            "@MKonack@n",  "@gNamek@n",  "@mAether@n",
                            "@mArlia@n",   "@CZenith@n", "@YYardrat@n",
                            "@cKanassa@n", "@RUNKOWN@n"};

  if (!*arg) {
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
           "GODCMD: %s has checked where to check player locations",
           GET_NAME(ch));
    send_to_char(
        ch,
        "Players                  Vnum    Planet        Location\r\n-------    "
        "             ------   ----------    ----------------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (IS_PLAYING(d)) {
        if (auto room = char_room_get(d->character); room) {
          if ((room &&
               room_flagged(room, ROOM_EARTH))) {
            num2 = 0;
          } else if ((room &&
                      room_flagged(room, ROOM_FRIGID))) {
            num2 = 1;
          } else if ((room &&
                      room_flagged(room, ROOM_VEGETA))) {
            num2 = 2;
          } else if ((room &&
                      room_flagged(room, ROOM_KONACK))) {
            num2 = 3;
          } else if ((room &&
                      room_flagged(room, ROOM_NAMEK))) {
            num2 = 4;
          } else if ((room &&
                      room_flagged(room, ROOM_AETHER))) {
            num2 = 5;
          } else if ((room &&
                      room_flagged(room, ROOM_ARLIA))) {
            num2 = 6;
          } else if (char_planet_zenith(d->character)) {
            num2 = 7;
          } else if ((room &&
                      room_flagged(room,
                                   ROOM_YARDRAT))) {
            num2 = 8;
          } else if ((room &&
                      room_flagged(room,
                                   ROOM_KANASSA))) {
            num2 = 9;
          } else {
            num2 = 10;
          }
        }
        i = (d->original ? d->original : d->character);
        if (i && CAN_SEE(ch, i) && (char_room_get(i) != NULL)) {
          if (d->original)
            send_to_char(ch, "%-20s - [%5d]   %s (in %s)\r\n", GET_NAME(i),
                         char_room_vnum_get(d->character),
                         room_name_get(char_room_get(d->character)),
                         GET_NAME(d->character));
          else {
            send_to_char(ch, "%-20s - [%5d]   %-14s %s\r\n", GET_NAME(i),
                         char_room_vnum_get(i), planet[num2],
                         room_name_get(char_room_get(i)));
          }
        }
      }
  } else {
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
           "GODCMD: %s has checked where for the location of %s", GET_NAME(ch),
           arg);
    char_iterate_all_newest([&](struct char_data *tch) {
      if (CAN_SEE(ch, tch) && char_room_get(tch) != NULL &&
          isname(arg, tch->name)) {
        found = 1;
        send_to_char(ch, "M%3d. %-25s - [%5d] %-25s", ++num, GET_NAME(tch),
                     char_room_vnum_get(tch), room_name_get(char_room_get(tch)));
        if (IS_NPC(tch) && SCRIPT(tch)) {
          if (!TRIGGERS(SCRIPT(tch))->next)
            send_to_char(ch, "[T%5d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(tch))));
          else
            send_to_char(ch, "[TRIGS] ");
        }
        send_to_char(ch, "\r\n");
      }
      return true;
    });
    obj_iterate_all_newest([&](struct obj_data *tobj) {
      if (CAN_SEE_OBJ(ch, tobj) && isname(arg, tobj->name)) {
        found = 1;
        print_object_location(++num, tobj, ch, TRUE);
      }
      return true;
    });
    if (!found) {
      send_to_char(ch, "Couldn't find any such thing.\r\n");
    } else {
      send_to_char(ch, "\r\nFound %d matches.\r\n", num);
    }
  }
}

ACMD(do_where) {
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (ADM_FLAGGED(ch, ADM_FULLWHERE) || GET_ADMLEVEL(ch) > 4)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}

/* do_levels moved to lua/characters/pcommands/info/levels.lua */
/* do_consider moved to lua/characters/pcommands/info/consider.lua */

ACMD(do_diagnose) {
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else {
      send_to_char(ch, "%s",
                   GET_SEX(vict) == SEX_MALE
                       ? "He "
                       : (GET_SEX(vict) == SEX_FEMALE ? "She " : "It "));
      diag_char_to_char(vict, ch);
    }
  } else {
    if (FIGHTING(ch)) {
      send_to_char(
          ch, "%s",
          GET_SEX(FIGHTING(ch)) == SEX_MALE
              ? "He "
              : (GET_SEX(FIGHTING(ch)) == SEX_FEMALE ? "She " : "It "));
      diag_char_to_char(FIGHTING(ch), ch);
    } else {
      send_to_char(ch, "Diagnose who?\r\n");
    }
  }
}

static const char *ctypes[] = {"off", "on", "\n"};

char *cchoice_to_str(char *col) {
  static char buf[READ_SIZE];
  char *s = NULL;
  int i = 0;
  int fg = 0;
  int needfg = 0;
  int bold = 0;

  if (!col) {
    buf[0] = 0;
    return buf;
  }
  while (*col) {
    if (strchr(ANSISTART, *col)) {
      col++;
    } else {
      switch (*col) {
      case ANSISEP:
      case ANSIEND:
        s = NULL;
        break;
      case '0':
        s = NULL;
        break;
      case '1':
        bold = 1;
        s = NULL;
        break;
      case '5':
        s = "blinking";
        break;
      case '7':
        s = "reverse";
        break;
      case '8':
        s = "invisible";
        break;
      case '3':
        col++;
        fg = 1;
        switch (*col) {
        case '0':
          s = bold ? (char *)"grey" : (char *)"black";
          bold = 0;
          fg = 1;
          break;
        case '1':
          s = "red";
          fg = 1;
          break;
        case '2':
          s = "green";
          fg = 1;
          break;
        case '3':
          s = "yellow";
          fg = 1;
          break;
        case '4':
          s = "blue";
          fg = 1;
          break;
        case '5':
          s = "magenta";
          fg = 1;
          break;
        case '6':
          s = "cyan";
          fg = 1;
          break;
        case '7':
          s = "white";
          fg = 1;
          break;
        case 0:
          s = NULL;
          break;
        }
        break;
      case '4':
        col++;
        switch (*col) {
        case '0':
          s = "on black";
          needfg = 1;
          bold = 0;
        case '1':
          s = "on red";
          needfg = 1;
          bold = 0;
        case '2':
          s = "on green";
          needfg = 1;
          bold = 0;
        case '3':
          s = "on yellow";
          needfg = 1;
          bold = 0;
        case '4':
          s = "on blue";
          needfg = 1;
          bold = 0;
        case '5':
          s = "on magenta";
          needfg = 1;
          bold = 0;
        case '6':
          s = "on cyan";
          needfg = 1;
          bold = 0;
        case '7':
          s = "on white";
          needfg = 1;
          bold = 0;
        default:
          s = "underlined";
          break;
        }
        break;
      default:
        s = NULL;
        break;
      }
      if (s) {
        if (needfg && !fg) {
          i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
          fg = 1;
        }
        if (i)
          i += snprintf(buf + i, sizeof(buf) - i, " ");
        if (bold) {
          i += snprintf(buf + i, sizeof(buf) - i, "bright ");
          bold = 0;
        }
        i += snprintf(buf + i, sizeof(buf) - i, "%s", s ? s : "null 1");
        s = NULL;
      }
      col++;
    }
  }
  if (!fg)
    i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
  return buf;
}

int str_to_cchoice(char *str, char *choice) {
  char buf[MAX_STRING_LENGTH];
  int bold = 0, blink = 0, uline = 0, rev = 0, invis = 0, fg = 0, bg = 0,
      error = 0;
  int i, len = MAX_INPUT_LENGTH;
  struct {
    char *name;
    int *ptr;
  } attribs[] = {
      {"bright", &bold}, {"bold", &bold},      {"underlined", &uline},
      {"reverse", &rev}, {"blinking", &blink}, {"invisible", &invis},
      {NULL, NULL}};
  struct {
    char *name;
    int val;
    int bold;
  } colors[] = {{"default", -1, 0}, {"normal", -1, 0}, {"black", 0, 0},
                {"red", 1, 0},      {"green", 2, 0},   {"yellow", 3, 0},
                {"blue", 4, 0},     {"magenta", 5, 0}, {"cyan", 6, 0},
                {"white", 7, 0},    {"grey", 0, 1},    {"gray", 0, 1},
                {NULL, 0, 0}};
  skip_spaces(&str);
  if (isdigit(*str)) { /* Accept a raw code */
    strcpy(choice, str);
    for (i = 0; choice[i] && (isdigit(choice[i]) || choice[i] == ';'); i++)
      ;
    error = choice[i] != 0;
    choice[i] = 0;
    return error;
  }
  while (*str) {
    str = any_one_arg(str, buf);
    if (!strcmp(buf, "on")) {
      bg = 1;
      continue;
    }
    if (!fg) {
      for (i = 0; attribs[i].name; i++)
        if (!strncmp(attribs[i].name, buf, strlen(buf)))
          break;
      if (attribs[i].name) {
        *(attribs[i].ptr) = 1;
        continue;
      }
    }
    for (i = 0; colors[i].name; i++)
      if (!strncmp(colors[i].name, buf, strlen(buf)))
        break;
    if (!colors[i].name) {
      error = 1;
      continue;
    }
    if (colors[i].val != -1) {
      if (bg == 1) {
        bg = 40 + colors[i].val;
      } else {
        fg = 30 + colors[i].val;
        if (colors[i].bold)
          bold = 1;
      }
    }
  }
  choice[0] = i = 0;
  if (bold)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_BOLD);
  if (uline)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "",
                  AA_UNDERLINE);
  if (blink)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_BLINK);
  if (rev)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_REVERSE);
  if (invis)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_INVIS);
  if (!i)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_NORMAL);
  if (fg && fg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "", fg);
  if (bg && bg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "", bg);

  return error;
}

char *default_color_choices[NUM_COLOR + 1] = {
    /* COLOR_NORMAL */ AA_NORMAL,
    /* COLOR_ROOMNAME */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMOBJS */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_ROOMPEOPLE */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_HITYOU */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUHIT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_OTHERHIT */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_CRITICAL */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_HOLLER */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_SHOUT */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_GOSSIP */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_AUCTION */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_CONGRAT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_TELL */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUSAY */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMSAY */ AA_NORMAL ANSISEPSTR AF_WHITE,
    NULL};

ACMD(do_color) {
  char arg[MAX_INPUT_LENGTH];
  char *p;
  int tp;

  /*if (IS_NPC(ch))
    return;*/

  p = any_one_arg(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  switch (tp) {
  case C_OFF:
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR);
    break;
  case C_ON:
    SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR);
    break;
  }
  send_to_char(ch, "Your color is now @o%s@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle) {
  char buf2[4];

  if (IS_NPC(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF"); /* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch)); /* sprintf: OK */

  if (GET_ADMLEVEL(ch)) {
    send_to_char(ch,
                 "      Buildwalk: %-3s    "
                 "Clear Screen in OLC: %-3s\r\n",
                 ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
                 ONOFF(PRF_FLAGGED(ch, PRF_CLS)));

    send_to_char(ch,
                 "      No Hassle: %-3s    "
                 "      Holylight: %-3s    "
                 "     Room Flags: %-3s\r\n",
                 ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
                 ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)));
  }

  send_to_char(
      ch,
      "Hit Pnt Display: %-3s    "
      "     Brief Mode: %-3s    "
      " Summon Protect: %-3s\r\n"

      "   Move Display: %-3s    "
      "   Compact Mode: %-3s    "
      "       On Quest: %-3s\r\n"

      "    Exp Display: %-3s    "
      "         NoTell: %-3s    "
      "   Repeat Comm.: %-3s\r\n"

      "     Ki Display: %-3s    "
      "           Deaf: %-3s    "
      "     Wimp Level: %-3s\r\n"

      " Gossip Channel: %-3s    "
      "Auction Channel: %-3s    "
      "  Grats Channel: %-3s\r\n"

      "      Auto Loot: %-3s    "
      "      Auto Gold: %-3s    "
      "    Color Level: %s\r\n"

      "     Auto Split: %-3s    "
      "       Auto Sac: %-3s    "
      "       Auto Mem: %-3s\r\n"

      "     View Order: %-3s    "
      "    Auto Assist: %-3s    "
      " Auto Show Exit: %-3s\r\n"

      "    TNL Display: %-3s    ",

      ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)), ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
      ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

      ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)), ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
      YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

      ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)), ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
      YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

      ONOFF(PRF_FLAGGED(ch, PRF_DISPKI)), YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
      buf2,

      ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)), ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
      ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

      ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
      ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)), ctypes[COLOR_LEV(ch)],

      ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
      ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),

      ONOFF(PRF_FLAGGED(ch, PRF_VIEWORDER)),
      ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
      ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),

      ONOFF(PRF_FLAGGED(ch, PRF_DISPTNL)));

  if (CONFIG_ENABLE_COMPRESSION) {
    send_to_char(ch, "    Compression: %-3s\r\n",
                 ONOFF(!PRF_FLAGGED(ch, PRF_NOCOMPRESS)));
  }
}

static int sort_commands_helper(const void *a, const void *b) {
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void) {
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++; /* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

ACMD(do_commands) {
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) ||
        IS_NPC(vict)) {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char(
          ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
               wizhelp ? "privileged " : "", socials ? "socials" : "commands",
               vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1;
       complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n';
       cmd_num++) {
    i = cmd_sort_info[cmd_num];

    if (complete_cmd_info[i].minimum_level < 0 ||
        GET_LEVEL(vict) < complete_cmd_info[i].minimum_level)
      continue;

    if (complete_cmd_info[i].minimum_admlevel < 0 ||
        GET_ADMLEVEL(vict) < complete_cmd_info[i].minimum_admlevel)
      continue;

    if ((complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp &&
        socials != (complete_cmd_info[i].command_pointer == do_action ||
                    complete_cmd_info[i].command_pointer == do_insult))
      continue;

    if (check_disabled(&complete_cmd_info[i]))
      sprintf(arg, "(%s)", complete_cmd_info[i].command);
    else
      sprintf(arg, "%s", complete_cmd_info[i].command);

    send_to_char(ch, "%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");
  }

  if (no % 7 != 1)
    send_to_char(ch, "\r\n");
}

static void free_history(struct char_data *ch, int type) {
  struct txt_block *tmp = GET_HISTORY(ch, type), *ftmp;

  while ((ftmp = tmp)) {
    tmp = tmp->next;
    if (ftmp->text)
      free(ftmp->text);
    free(ftmp);
  }
  GET_HISTORY(ch, type) = NULL;
}

ACMD(do_history) {
  char arg[MAX_INPUT_LENGTH];
  int type;

  one_argument(argument, arg);

  type = search_block(arg, history_types, FALSE);
  if (!*arg || type < 0) {
    int i;

    send_to_char(ch, "Usage: history <");
    for (i = 0; *history_types[i] != '\n'; i++) {
      if ((i != 3 && GET_ADMLEVEL(ch) <= 0) || GET_ADMLEVEL(ch) >= 1) {
        send_to_char(ch, " %s ", history_types[i]);
      }
      if (*history_types[i + 1] == '\n') {
        send_to_char(ch, ">\r\n");
      } else {
        if ((i != 3 && GET_ADMLEVEL(ch) <= 0) || GET_ADMLEVEL(ch) >= 1) {
          send_to_char(ch, "|");
        }
      }
    }
    return;
  }

  if (GET_HISTORY(ch, type) && GET_HISTORY(ch, type)->text &&
      *GET_HISTORY(ch, type)->text) {
    struct txt_block *tmp;
    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next)
      send_to_char(ch, "%s", tmp->text);
/* Make this a 1 if you want history to cear after viewing */
#if 0 
      free_history(ch, type);
#endif
  } else
    send_to_char(ch, "You have no history in that channel.\r\n");
}

void add_history(struct char_data *ch, char *str, int type) {
  int i = 0;
  char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  struct txt_block *tmp;
  time_t ct;

  if (IS_NPC(ch))
    return;

  tmp = GET_HISTORY(ch, type);
  ct = time(0);
  strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

  sprintf(buf, "%s%s", time_str, str);

  if (!tmp) {
    CREATE(GET_HISTORY(ch, type), struct txt_block, 1);
    GET_HISTORY(ch, type)->text = strdup(buf);
  } else {
    while (tmp->next)
      tmp = tmp->next;
    CREATE(tmp->next, struct txt_block, 1);
    tmp->next->text = strdup(buf);

    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next, i++)
      ;

    for (; i > HIST_LENGTH && GET_HISTORY(ch, type); i--) {
      tmp = GET_HISTORY(ch, type);
      GET_HISTORY(ch, type) = tmp->next;
      if (tmp->text)
        free(tmp->text);
      free(tmp);
    }
  }
  /* add this history message to ALL */
  if (type != HIST_ALL)
    add_history(ch, str, HIST_ALL);
}

ACMD(do_scan) {
  int i, newroom;
  char *dirnames[] = {"North",     "East",      "South",     "West",
                      "Up",        "Down",      "Northwest", "Northeast",
                      "Southeast", "Southwest", "Inside",    "Outside"};

  if (GET_POS(ch) < POS_SLEEPING) {
    send_to_char(ch, "You can't see anything but stars!\n\r");
    return;
  }
  if (!AWAKE(ch)) {
    send_to_char(ch, "You must be dreaming.\n\r");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see a damn thing, you're blind!\n\r");
    return;
  }
  if (PLR_FLAGGED(ch, PLR_EYEC)) {
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  auto room = char_room_get(ch);
  for (i = 0; i < 10; i++) {
    if (auto ex = EXIT(ch, i)) {
      if (IS_DARK(room) && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
          (!AFF_FLAGGED(ch, AFF_INFRAVISION))) {
        send_to_char(ch, "%s: DARK\n\r", dirnames[i]);
        continue;
      }
      if (auto nrm = CAN_GO(ch, i); nrm) {
        send_to_char(ch, "@w-----------------------------------------@n\r\n");
        send_to_char(ch, "          %s%s: %s %s\n\r", CCCYN(ch, C_NRM),
                     dirnames[i],
                      room_name_get(nrm) ? room_name_get(nrm)
                                : "You don't think you saw what you just saw.",
                     CCNRM(ch, C_NRM));
        send_to_char(ch, "@W          -----------------          @n\r\n");

        list_obj_to_char(inv_for_room(nrm), ch, SHOW_OBJ_LONG, FALSE);
        list_char_to_char(nrm, ch);
        if (room_geffect_get(nrm) >= 1 && room_geffect_get(nrm) <= 5) {
          send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
        }
        if (room_geffect_get(nrm) >= 6) {
          send_to_char(ch,
                       "@RLava@r covers pretty much the entire area!@n\r\n");
        }
        /* Check 2nd room away */
        if (auto ne2 = room_dir_option_get(nrm, i); ne2 && exit_to_room_vnum_get(ne2)) {
          auto nrm2 = char_can_go_exit(ch, ne2);

          if (nrm2) {
            if (!IS_DARK(nrm2)) {
              send_to_char(ch,
                           "@w-----------------------------------------@n\r\n");
              send_to_char(ch, "          %sFar %s: %s %s\n\r",
                           CCCYN(ch, C_NRM), dirnames[i],
                           room_name_get(nrm2)
                               ? room_name_get(nrm2)
                               : "You don't think you saw what you just saw.",
                           CCNRM(ch, C_NRM));
              send_to_char(ch, "@W          -----------------          @n\r\n");

              list_obj_to_char(inv_for_room(nrm2), ch, SHOW_OBJ_LONG, FALSE);
              list_char_to_char(nrm2, ch);
              if (room_geffect_get(nrm2) >= 1 && room_geffect_get(nrm2) <= 5) {
                send_to_char(ch,
                             "@rLava@w is pooling in someplaces here...@n\r\n");
              }
              if (room_geffect_get(nrm2) >= 6) {
                send_to_char(
                    ch, "@RLava@r covers pretty much the entire area!@n\r\n");
              }
            } else {
              send_to_char(ch, "%s<-> %sFar %s: Too dark to tell! %s<->%s\r\n",
                           QMAG, QCYN, dirnames[i], QMAG, QNRM);
            }
          }
        }
      }
    }
  }
  send_to_char(ch, "@w-----------------------------------------@n\r\n");
}

ACMD(do_toplist) {
  if (IS_NPC(ch))
    return;

  FILE *file;
  char fname[40], filler[50], line[256];
  int64_t points[25] = {0}, stats;
  char *title[25] = {""};
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
    send_to_char(ch, "The toplist file does not exist.");
    return;
  } else if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The toplist file does not exist.");
    return;
  }
  while (!feof(file) || count < 25) {
    get_line(file, line);
    switch (count) {
    default:
      sscanf(line, "%s %" I64T "\n", filler, &stats);
      break;
    }
    title[count] = strdup(filler);
    points[count] = stats;
    count++;
    *filler = '\0';
  }
  send_to_char(ch, "@D-=[@BDBAT Top Lists for @REra@C %d@D]=-@n\r\n",
               CURRENT_ERA);
  while (x <= count) {
    switch (x) {
    /* Powerlevel Area */
    case 0:
      send_to_char(ch, "       @D-@RPowerlevel@D-@n\r\n");
      send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 1:
      send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 2:
      send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 3:
      send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 4:
      send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    /* Ki Area */
    case 5:
      send_to_char(ch, "       @D-@BKi        @D-@n\r\n");
      send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 6:
      send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 7:
      send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 8:
      send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 9:
      send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    /* Stamina Area */
    case 10:
      send_to_char(ch, "       @D-@GStamina   @D-@n\r\n");
      send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 11:
      send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 12:
      send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 13:
      send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 14:
      send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    /* Stamina Area */
    case 15:
      send_to_char(ch, "       @D-@gZenni     @D-@n\r\n");
      send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 16:
      send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 17:
      send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 18:
      send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    case 19:
      send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
      free(title[x]);
      break;
    /* Rpp Area */
    case 20:
      /*send_to_char(ch, "       @D-@mRPP       @D-@n\r\n");
      send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);*/
      free(title[x]);
      break;
    case 21:
      /*send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);*/
      free(title[x]);
      break;
    case 22:
      /*send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);*/
      free(title[x]);
      break;
    case 23:
      /*send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);*/
      free(title[x]);
      break;
    case 24:
      /*send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);*/
      free(title[x]);
      break;
    }
    x++;
  }
  fclose(file);
}

ACMD(do_whois) {
  char buf[MAX_INPUT_LENGTH];
  int clan = FALSE;
  const char *immlevels[ADMLVL_IMPL + 2] = {
      "[Mortal]",               /* lowest admin level */
      "[Enforcer]",             /* lowest admin level +1 */
      "[First Class Enforcer]", /* lowest admin level +2 */
      "[High Enforcer]",        /* lowest admin level +3 */
      "[Vice Admin]",           /* lowest admin level +4 */
      "[Administrator]",        /* lowest admin level +5 */
      "[Implementor]",
  };

  struct char_data *victim = 0;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Who?\r\n");
  } else {
    CREATE(victim, struct char_data, 1);
    clear_char(victim);
    if (load_char(argument, victim) >= 0) {
      if (GET_CLAN(victim) != NULL) {
        if (!strstr(GET_CLAN(victim), "None")) {
          sprintf(buf, "%s", GET_CLAN(victim));
          clan = TRUE;
        }
        if (strstr(GET_CLAN(victim), "Applying")) {
          sprintf(buf, "%s", GET_CLAN(victim));
          clan = TRUE;
        }
      }
      if (GET_CLAN(victim) == NULL || strstr(GET_CLAN(victim), "None")) {
        clan = FALSE;
      }
      send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                       "~~~~~~~~~~~@n\r\n");
      if (GET_ADMLEVEL(victim) >= ADMLVL_IMMORT) {
        send_to_char(ch, "@cName     @D: @G%s\r\n", GET_NAME(victim));
        send_to_char(ch, "@cImm Level@D: @G%s\r\n",
                     immlevels[GET_ADMLEVEL(victim)]);
        send_to_char(ch, "@cTitle    @D: @G%s\r\n", GET_TITLE(victim));
      } else {
        send_to_char(ch,
                     "@cName  @D: @w%s\r\n@cSensei@D: @w%s\r\n@cRace  @D: "
                     "@w%s\r\n@cTitle @D: @w%s@n\r\n@cClan  @D: @w%s@n\r\n",
                     GET_NAME(victim), SENSEI_NAME(victim), TRUE_RACE(victim),
                     GET_TITLE(victim), clan ? buf : "None.");
        if (clan == TRUE && !strstr(GET_CLAN(victim), "Applying")) {
          if (checkCLAN(victim) == TRUE) {
            clanRANKD(GET_CLAN(victim), ch, victim);
          }
        }
      }
      send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                       "~~~~~~~~~~~@n\r\n");
    } else {
      send_to_char(ch, "There is no such player.\r\n");
    }
    free(victim);
  }
}

#define DOOR_DCHIDE(ch, door) exit_dchide_get(EXIT(ch, door))

static void search_in_direction(struct char_data *ch, int dir) {
  int check = FALSE, skill_lvl, dchide = 20;

  send_to_char(ch, "You search for secret doors.\r\n");
  act("$n searches the area intently.", TRUE, ch, 0, 0, TO_ROOM);

  /* SEARCHING is allowed untrained */
  skill_lvl = GET_SKILL(ch, SKILL_SEARCH);
  if (IS_TRUFFLE(ch) || IS_HUMAN(ch))
    skill_lvl = skill_lvl + 2;
  if (IS_HALFBREED(ch))
    skill_lvl = skill_lvl + 1;

  auto ex = EXIT(ch, dir);
  if (ex)
    dchide = DOOR_DCHIDE(ch, dir);

  if (skill_lvl > dchide)
    check = TRUE;

  if (ex) {
    if (exit_general_description_get(ex) &&
        !exit_flagged(ex, EX_SECRET))
      send_to_char(ch, "%s", exit_general_description_get(ex));
    else if (!exit_flagged(ex, EX_SECRET))
      send_to_char(ch, "There is a normal exit there.\r\n");
    else if (exit_flagged(ex, EX_ISDOOR) &&
             exit_flagged(ex, EX_SECRET) && exit_keyword_get(ex) &&
             (check == TRUE))
      send_to_char(ch, "There is a hidden door keyword: '%s' %sthere.\r\n",
                   fname(exit_keyword_get(ex)),
                   (exit_flagged(ex, EX_CLOSED)) ? "" : "open ");
    else
      send_to_char(ch, "There is no exit there.\r\n");
  } else
    send_to_char(ch, "There is no exit there.\r\n");
}
