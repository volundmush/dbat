/* rp.c: intros, RP bank and display, mimic, nickname, card games
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"
#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/disabled.h"
#include "informative_internal.h"

ACMD(do_mimic)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_get_skill(ch, SKILL_MIMIC)) {
    send_to_char(ch, "You do not know how to mimic the appearance of other races.\r\n");
    return;
  }

  int count = 0;

  // generate a list of mimic'able races.
  bool can_mimic[NUM_RACES];
  for (int r = 0; r < NUM_RACES; r++) {
    can_mimic[r] = race_can_be_mimiced(r) && race_valid_sex(r, char_get_sex(ch));
  }

  if (!*arg) {
    send_to_char(ch, "@CMimic Menu\n@c--------------------@W\r\n");
    for (int r = 0; r < NUM_RACES; r++) {
      if (!can_mimic[r])
        continue;
      if (count == 2) {
        send_to_char(ch, "%s\n", race_name(r));
        count = 0;
      } else {
        send_to_char(ch, "%s\n", race_name(r));
        count++;
      }
    }
    send_to_char(ch, "Stop@n\r\n");
    if (ch->mimic) {
      send_to_char(ch, "You currently Mimic a %s", pc_race_types[ch->mimic]);
    }

    return;
  }

  if (!strcasecmp(arg, "stop")) {
    if (!ch->mimic) {
      send_to_char(ch, "You are not imitating another race.\r\n");
      return;
    }
    act("@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", TRUE, ch, 0, 0, TO_ROOM);
    ch->mimic = 0;
  }

  int race = RACE_UNDEFINED;
  for (int r = 0; r < NUM_RACES; r++) {
    if (can_mimic[r] && !strcasecmp(arg, race_name(r))) {
      race = r;
      break;
    }
  }
  if (race == RACE_UNDEFINED) {
    send_to_char(ch, "That is not a race you can change into. Enter mimic without arugments for the mimic menu.\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_MIMIC), perc = axion_dice(0);
  double mult  = 1 / prob;
  int64_t cost = getMaxKI(ch) * mult;

  if (race == ch->mimic) {
    send_to_char(ch, "You are already mimicing that race. To stop enter 'mimic stop'\r\n");
    return;
  } else if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  } else if (prob < perc) {
    decCurKI(ch, cost, 0);
    act("@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  } else {
    char buf[MAX_STRING_LENGTH];
    ch->mimic = race;
    decCurKI(ch, cost, 0);
    sprintf(buf, "@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be %s @M%s!@n", AN(juggleRaceName(ch, true)), juggleRaceName(ch, false));
    send_to_char(ch, "@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be %s %s.@n\r\n", AN(juggleRaceName(ch, true)), juggleRaceName(ch, false));
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
}

ACMD(do_kyodaika)
{

  if (char_is_npc(ch)) {
    return;
  }

  if (!char_is_namek(ch)) {
    send_to_char(ch, "You are not a namek!\r\n");
    return;
  }

  if (!is_affected(ch, AFF_KYODAIKA)) {
    act("@GYou growl as your body grows to ten times its normal size!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n@G growls as $s body grows to ten times its normal size!@n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "@cStrength@D: @C+5\r\n@cSpeed@D: @c-2@n\r\n");
    assign_affect(ch, AFF_KYODAIKA, 0, -1, 5, 0, 0, 0, 0, -2);
    save_char(ch);
    return;
  } else {
    act("@GYou growl as your body shrinks to its normal size!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n@G growls as $s body shrinks to its normal size!@n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "@cStrength@D: @C-5\r\n@cSpeed@D: @c+2@n\r\n");
    remove_affect(ch, AFF_KYODAIKA);
    save_char(ch);
    return;
  }
}

ACMD(do_table)
{
  struct obj_data *obj = NULL, *obj2 = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: table (red | blue | green | yellow) (card name)");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[char_get_in_room(ch)].contents))) {
    send_to_char(ch, "You don't see that table here.\r\n");
    return;
  }

  if (!(obj2 = get_obj_in_list_vis(ch, arg2, NULL, obj->contains))) {
    send_to_char(ch, "That card doesn't seem to be on that table.\r\n");
    return;
  }

  char buf[200];
  sprintf(buf, "$n looks at %s on %s.\r\n", obj2->short_description, obj->short_description);
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "%s", obj2->action_description);
}

ACMD(do_draw)
{
  if (!char_get_sits(ch)) {
    send_to_char(ch, "You are not sitting at a duel table.\r\n");
    return;
  }

  if (obj_get_vnum(char_get_sits(ch)) < 604 || obj_get_vnum(char_get_sits(ch)) > 607) {
    send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  struct obj_data *obj = NULL, *obj2 = NULL, *obj3 = NULL, *next_obj = NULL;
  int drawn = FALSE;

  if (!(obj = get_obj_in_list_vis(ch, (char[]){"case"}, NULL, ch->carrying))) {
    send_to_char(ch, "You don't have a case.\r\n");
    return;
  }
  for (obj2 = obj->contains; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    if (drawn == FALSE) {
      obj_from_obj(obj2);
      obj_to_char(obj2, ch);
      obj3  = obj2;
      drawn = TRUE;
    }
  }
  if (drawn == FALSE) {
    send_to_char(ch, "You don't have any cards in the case!\r\n");
    return;
  } else {
    act("$n draws a card from $s $p.\r\n", TRUE, ch, obj, 0, TO_ROOM);
    send_to_char(ch, "You draw a card.\r\n%s\r\n", obj3->action_description);
    return;
  }
}

ACMD(do_shuffle)
{

  if (!char_get_sits(ch)) {
    send_to_char(ch, "You are not sitting at a duel table.\r\n");
    return;
  }

  if (obj_get_vnum(char_get_sits(ch)) < 604 || obj_get_vnum(char_get_sits(ch)) > 607) {
    send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  struct obj_data *obj = NULL, *obj2 = NULL, *next_obj = NULL;
  int count = 0;

  if (!(obj = get_obj_in_list_vis(ch, (char[]){"case"}, NULL, ch->carrying))) {
    send_to_char(ch, "You don't have a case.\r\n");
    return;
  }

  for (obj2 = obj->contains; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    if (!obj_extra_flagged(obj2, ITEM_ANTI_HIEROPHANT)) {
      continue;
    }
    count += 1;
  }
  if (count <= 0) {
    send_to_char(ch, "You don't have any cards in the case!\r\n");
    return;
  }
  int total = count;
  for (obj2 = obj->contains; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    obj_from_obj(obj2);
    obj_to_room(obj2, real_room(48));
  }
  while (count > 0) {
    for (obj2 = world[real_room(48)].contents; obj2; obj2 = next_obj) {
      next_obj = obj2->next_content;
      if (!obj_extra_flagged(obj2, ITEM_ANTI_HIEROPHANT)) {
        continue;
      }
      if (obj2 && count > 1 && rand_number(1, 4) == 3) {
        count -= 1;
        obj_from_room(obj2);
        obj_to_obj(obj2, obj);
      } else if (obj2 && count == 1) {
        count -= 1;
        obj_from_room(obj2);
        obj_to_obj(obj2, obj);
      }
    }
  }
  send_to_char(ch, "You shuffle the cards carefully.\r\n");
  act("$n shuffles their deck.", TRUE, ch, 0, 0, TO_ROOM);
  send_to_room(char_get_in_room(ch), "There were %d cards in the deck.\r\n", total);
}

ACMD(do_hand)
{

  struct obj_data *obj, *next_obj;
  char arg[MAX_INPUT_LENGTH];
  int count = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: hand (look | show)\r\n");
    return;
  }

  if (!strcasecmp("look", arg)) {
    send_to_char(ch, "@CYour hand contains:\r\n@D---------------------------@n\r\n");
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (obj && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
        continue;
      }
      if (obj) {
        count += 1;
        send_to_char(ch, "%s\r\n", obj->short_description);
      }
    }
    act("$n looks at $s hand.", TRUE, ch, 0, 0, TO_ROOM);
    if (count == 0) {
      send_to_char(ch, "No cards.");
      act("There were no cards.", TRUE, ch, 0, 0, TO_ROOM);
    } else if (count > 7) {
      act("You have more than seven cards in your hand.", TRUE, ch, 0, 0, TO_CHAR);
      act("$n has more than seven cards in $s hand.", TRUE, ch, 0, 0, TO_ROOM);
    } else {
      char buf[200];
      sprintf(buf, "There are %d cards in the hand.", count);
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
    }
  } else if (!strcasecmp("show", arg)) {
    send_to_char(ch, "You show off your hand to the room.\r\n");
    act("@C$n's hand contains:\r\n@D---------------------------@n", TRUE, ch, 0, 0, TO_ROOM);
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (obj && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
        continue;
      }
      if (obj) {
        count += 1;
        act("$p", TRUE, ch, obj, 0, TO_ROOM);
      }
    }
    if (count == 0) {
      act("No cards.", TRUE, ch, 0, 0, TO_ROOM);
    }
    if (count > 7) {
      act("You have more than seven cards in your hand.", TRUE, ch, 0, 0, TO_CHAR);
      act("$n has more than seven cards in $s hand.", TRUE, ch, 0, 0, TO_ROOM);
    }
  } else {
    send_to_char(ch, "Syntax: hand (look | show)\r\n");
    return;
  }
}

ACMD(do_post)
{

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct obj_data *obj2;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: post (obj name)\n"
                     "        post (obj name) (target obj name)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have that.\r\n");
    return;
  }

  if (obj_get_type_flag(obj) != ITEM_NOTE) {
    send_to_char(ch, "You can only post notepaper.\r\n");
    return;
  }

  if (rroom_flagged(char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(char_get_in_room(ch), ROOM_GARDEN2)) {
    send_to_char(ch, "You can not post on things in a garden.\r\n");
    return;
  }

  if (!*arg2) {
    if (rroom_sect(char_get_in_room(ch)) != SECT_INSIDE && rroom_sect(char_get_in_room(ch)) != SECT_CITY) {
      send_to_char(ch, "You are not near any general structure you can post it on.\r\n");
      return;
    }
    act("@WYou post $p@W on a nearby structure.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W posts $p@W on a nearby structure.@n", TRUE, ch, obj, 0, TO_ROOM);
    obj_from_char(obj);
    obj_to_room(obj, char_get_in_room(ch));
    obj_set_posttype(obj, 1);
    return;
  } else {
    if (!(obj2 = get_obj_in_list_vis(ch, arg2, NULL, world[char_get_in_room(ch)].contents))) {
      send_to_char(ch, "You can't seem to find the thing you want to post it on.\r\n");
      return;
    } else if (obj_get_posted_to(obj2)) {
      send_to_char(ch, "It already has something posted on it. Get that first if you want to post.\r\n");
      return;
    } else if (obj_get_type_flag(obj2) == ITEM_BOARD) {
      send_to_char(ch, "Boards come with their own means of posting messages.\r\n");
      return;
    } else {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "@C$n@W posts %s@W on %s@W.@n", obj->short_description, obj2->short_description);
      send_to_char(ch, "@WYou post %s@W on %s@W.@n\r\n", obj->short_description, obj2->short_description);
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
      obj_from_char(obj);
      obj_to_room(obj, char_get_in_room(ch));
      obj_set_posttype(obj, 2);
      obj_set_posted_to(obj, obj2);
      obj_set_posted_to(obj2, obj);
      return;
    }
  }
}

ACMD(do_play)
{

  if (char_get_position(ch) != POS_SITTING) {
    send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  if (!char_get_sits(ch)) {
    send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  if (obj_get_vnum(char_get_sits(ch)) < 604 || obj_get_vnum(char_get_sits(ch)) > 607) {
    send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  struct obj_data *obj = NULL, *obj2 = NULL, *obj3 = NULL, *next_obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: play (card name)");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't have that card to play.\r\n");
    return;
  }

  for (obj3 = world[char_get_in_room(ch)].contents; obj3; obj3 = next_obj) {
    next_obj = obj3->next_content;
    if (obj_get_vnum(obj3) == obj_get_vnum(char_get_sits(ch)) - 4) {
      obj2 = obj3;
    }
  }

  if (obj2 == NULL) {
    send_to_char(ch, "Your table is missing. Inform an immortal of this problem.\r\n");
    return;
  }

  act("You play $p on your table.", TRUE, ch, obj, 0, TO_CHAR);
  act("$n plays $p on $s table.", TRUE, ch, obj, 0, TO_ROOM);
  obj_from_char(obj);
  obj_to_obj(obj, obj2);
}

/* Nickname an object */
ACMD(do_nickname)
{
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: nickname (object) (nickname)\n");
    send_to_char(ch, "Syntax: nickname ship (nickname)\n");
    return;
  }

  if (strcasecmp(arg, "ship")) {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that item to nickname.\r\n");
      return;
    }
  }
  if (strlen(arg2) > 20) {
    send_to_char(ch, "You can't nickname items with any name longer than 20 characters.\r\n");
    return;
  }

  if (!strcasecmp(arg, "ship")) {
    struct obj_data *ship = NULL, *next_obj = NULL, *ship2 = NULL;
    int found = FALSE;
    for (ship = world[char_get_in_room(ch)].contents; ship; ship = next_obj) {
      next_obj = ship->next_content;
      if (obj_get_vnum(ship) >= 45000 && obj_get_vnum(ship) <= 45999 && found == FALSE) {
        found = TRUE;
        ship2 = ship;
      }
    }
    if (found == TRUE) {
      if (strstr(arg2, "@")) {
        send_to_char(ch, "You can't nickname a ship and use color codes. Sorry.\r\n");
        return;
      } else {
        char nick[MAX_INPUT_LENGTH];
        sprintf(nick, "%s", CAP(arg2));
        ship2->action_description = strdup(nick);
        struct obj_data *k;
        for (k = object_list; k; k = k->next) {
          if (obj_get_vnum(k) == obj_get_vnum(ship2) + 1000) {
            extract_obj(k);
            int was_in = rroom_get_vnum(obj_get_in_room(ship2));
            obj_from_room(ship2);
            obj_to_room(ship2, real_room(was_in));
          }
        }
      }
    }
    return;
  }

  if (strstr(obj->short_description, "nicknamed")) {
    send_to_char(ch, "%s@w has already been nicknamed.@n\r\n", obj->short_description);
    return;
  } else if (strstr(obj->name, "corpse")) {
    send_to_char(ch, "%s@w is a corpse!@n\r\n", obj->short_description);
    return;
  } else {
    send_to_char(ch, "@wYou nickname %s@w as '@C%s@w'.@n\r\n", obj->short_description, arg2);
    char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH];
    snprintf(nick, sizeof(nick), "%s @wnicknamed @D(@C%s@D)@n", obj->short_description, CAP(arg2));
    snprintf(nick2, sizeof(nick2), "%s %s", obj->name, arg2);
    obj->short_description = strdup(nick);
    obj->name              = strdup(nick2);
    return;
  }
}

ACMD(do_showoff)
{
  struct obj_data *obj   = NULL;
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  *arg  = '\0';
  *arg2 = '\0';

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "You want to show what item to what character?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have that.\r\n");
    return;
  } else if (!(vict = get_player_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person around.\r\n");
    return;
  } else { /* Ok show that target the object! */
    act("@WYou hold up $p@W for @C$N@W to see:@n", TRUE, ch, obj, vict, TO_CHAR);
    act("@C$n@W holds up $p@W for you to see:@n", TRUE, ch, obj, vict, TO_VICT);
    act("@C$n@W holds up $p@W for @c$N@W to see.@n", TRUE, ch, obj, vict, TO_NOTVICT);
    show_obj_to_char(obj, vict, SHOW_OBJ_ACTION);
    return;
  }
}

/* Used for checking if you know the character in question. */
void introCreate(struct char_data *ch)
{
  char fname[40];
  FILE *fl;
  /* Write Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mud_log("ERROR: could not save user, %s, to filename, %s.", char_get_name(ch), fname);
    return;
  }

  fprintf(fl, "Gibbles Gibbles\n");

  fclose(fl);
  return;
}

int readIntro(struct char_data *ch, struct char_data *vict)
{
  char fname[40], filler[50], scrap[100], line[256];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */
  if (vict == NULL) {
    return 0;
  }

  if (char_is_npc(ch) || char_is_npc(vict)) {
    return 1;
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
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
    if (!strcasecmp(char_get_name(vict), filler)) {
      known = TRUE;
    }
  }
  fclose(fl);

  if (known == TRUE)
    return 1;
  else
    return 0;
}

static void introWrite(struct char_data *ch, struct char_data *vict, char *name)
{
  FILE *file;
  char fname[40], filler[50], scrap[100], line[256];
  char *names[500] = {NULL}, *alias[500] = {NULL};
  FILE *fl;
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
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
    *scrap  = '\0';
  }
  fclose(file);

  /* Write Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mud_log("ERROR: could not save intro file, %s, to filename, %s.", char_get_name(ch), fname);
    return;
  }

  while (x < count) {
    if (x == 0 || strcasecmp(names[x - 1], names[x])) {
      if (strcasecmp(names[x], char_get_name(vict))) {
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

  fprintf(fl, "%s %s\n", char_get_name(vict), CAP(name));

  fclose(fl);
  return;
}

ACMD(do_intro)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct char_data *vict;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: dub (target) (name)\r\nWho do you want to dub and what do you want to name them?\r\n");
    return;
  }

  if (!*arg2) {
    send_to_char(ch, "Syntax: dub (target) (name)\r\nWhat name do you wish to know them by?\r\n");
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

  if (char_is_npc(vict)) {
    send_to_char(ch, "That seems rather unwise.\r\n");
    return;
  }

  if (readIntro(vict, ch) == 2) {
    send_to_char(ch, "There seems to have been an error, report this to Iovan.\r\n");
    return;
  } else if (readIntro(ch, vict) == 1 && strstr(juggleRaceName(vict, true), arg)) {
    send_to_char(ch, "You have already dubbed them a name. If you want to redub them target the name you know them by.\r\n");
    return;
  } else {
    introWrite(ch, vict, arg2);
    act("You decide to call $M, $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n seems to decide something about you.", TRUE, ch, 0, vict, TO_VICT);
    act("$n seems to decide something about $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    return;
  }
}

ACMD(do_rptrans)
{
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

  if (amt > char_get_rp(ch)) {
    send_to_char(ch, "@WYou only have @C%d@W RPP!@n\r\n", char_get_rp(ch));
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(ch, "That is not a recognised user file.\r\n");
    return;
  }

  for (k = descriptor_list; k; k = k->next) {
    if (char_is_npc(k->character))
      continue;
    if (desc_get_connected(k) != CON_PLAYING)
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }
  if (vict == NULL) {
    userWrite(NULL, 0, amt, 0, arg);
  } else {
    char_set_rp(vict, char_get_rp(vict) + (amt));
    vict->desc->rpp += amt;
    userWrite(vict->desc, 0, 0, 0, "index");
    char_set_trp(vict, char_get_trp(vict) + (amt));
    save_char(vict);
    send_to_char(vict, "@W%s gives @C%d@W of their RPP to you. How nice!\r\n", char_get_name(ch), amt);
  }
  char_set_rp(ch, char_get_rp(ch) - (amt));
  ch->desc->rpp -= amt;
  userWrite(ch->desc, 0, 0, 0, "index");
  send_to_char(ch, "@WYou exchange @C%d@W RPP to user @c%s@W for a warm fuzzy feeling.\r\n", amt, CAP(arg));
  mudlog(LOG_NRM, MAX(ADMLVL_IMPL, char_get_invis_level(ch)), TRUE, "EXCHANGE: %s gave %d RPP to user %s", char_get_name(ch), amt, arg);
  save_char(ch);
}

/*By Andros*/
ACMD(do_rpbank)
{
  int amt = 0;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: rpbank <amount>\r\n");
    send_to_char(ch, "Current RPP Bank: %d\r\n", char_get_rbank(ch));
    return;
  }

  amt = atoi(arg);

  if (amt <= 0) {
    send_to_char(ch, "You cannot withdraw from the RPP Bank.\r\n");
    return;
  }

  if (amt > char_get_rp(ch)) {
    send_to_char(ch, "You do not have that much RPP to send to the Bank!\r\n");
    return;
  }

  char_set_rp(ch, char_get_rp(ch) - (amt));
  ch->desc->rpp -= amt;
  char_set_rbank(ch, char_get_rbank(ch) + (amt));
  ch->desc->rbank += amt;
  send_to_char(ch, "You send %d to your RPP Bank. Your total is now %d.\r\n", amt, char_get_rbank(ch));
  mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "RPP Bank: %s has put %d RPP into their bank", char_get_name(ch), amt);
}

/* Rillao: An attempt was made. */
ACMD(do_rbanktrans)
{
  struct char_data *vict = NULL;
  struct descriptor_data *k;
  int amt = 0;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: bexchange (target) (amount)\r\n");
    return;
  }

  amt = atoi(arg2);

  if (amt <= 0) {
    send_to_char(ch, "Are you being funny?\r\n");
    return;
  }

  if (amt > char_get_rbank(ch)) {
    send_to_char(ch, "@WYou only have @C%d@W Banked RPP!@n\r\n", char_get_rbank(ch));
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(ch, "That is not a recognised user file.\r\n");
    return;
  }

  for (k = descriptor_list; k; k = k->next) {
    if (char_is_npc(k->character))
      continue;
    if (desc_get_connected(k) != CON_PLAYING)
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }
  if (vict == NULL) {
    userWrite(NULL, 0, amt, 0, arg);
  } else {
    char_set_rbank(vict, char_get_rbank(vict) + (amt));
    vict->desc->rbank += amt;
    userWrite(vict->desc, 0, 0, 0, "index");
    save_char(vict);
    send_to_char(vict, "@W%s gives @C%d@W of their Banked RPP to you. How nice!\r\n", char_get_name(ch), amt);
  }
  char_set_rbank(ch, char_get_rbank(ch) - (amt));
  ch->desc->rbank -= amt;
  userWrite(ch->desc, 0, 0, 0, "index");
  send_to_char(ch, "@wWELL @xGOLLY @xGEE @xWILLICKERS@x! @wYOU @RZ@YIM @RZ@YAMMED @C%d @wRIPROOZLES TO @C%s!@n\r\n", amt, CAP(arg));
  mudlog(LOG_NRM, MAX(ADMLVL_IMPL, char_get_invis_level(ch)), TRUE, "EXCHANGE: %s gave %d Banked RPP to user %s", char_get_name(ch), amt, arg);
  save_char(ch);
}

ACMD(do_rdisplay)
{
  skip_spaces(&argument);

  if (char_is_npc(ch)) {
    return;
  }

  if (!*argument) {
    send_to_char(ch, "Clearing room display.\r\n");
    char_set_rdisplay(ch, "Empty");
  } else {
    char derp[MAX_STRING_LENGTH];

    strcpy(derp, argument);

    send_to_char(ch, "You set your display to; %s\r\n", derp);
    char_set_rdisplay(ch, strdup(derp));
  }
}
