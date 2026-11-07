/* rp.c: intros, RP bank and display, mimic, nickname, card games
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
#include "dbat/flags.hpp"
#include "dbat/affect.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/maputils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/boards.hpp"
#include "dbat/screen.hpp"
#include "dbat/mail.hpp"
#include "dbat/guild.hpp"
#include "dbat/disabled.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

ACMD(do_mimic)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_get_skill(ch, SKILL_MIMIC)) {
    send_to_char(g, ch, "You do not know how to mimic the appearance of other races.\r\n");
    return;
  }

  int count = 0;

  // generate a list of mimic'able races.
  bool can_mimic[NUM_RACES];
  for (int r = 0; r < NUM_RACES; r++) {
    can_mimic[r] = race_can_be_mimiced(r) && race_valid_sex(r, char_get_sex(ch));
  }

  if (!*arg) {
    send_to_char(g, ch, "@CMimic Menu\n@c--------------------@W\r\n");
    for (int r = 0; r < NUM_RACES; r++) {
      if (!can_mimic[r])
        continue;
      if (count == 2) {
        send_to_char(g, ch, "%s\n", race_name(r));
        count = 0;
      } else {
        send_to_char(g, ch, "%s\n", race_name(r));
        count++;
      }
    }
    send_to_char(g, ch, "Stop@n\r\n");
    if (char_get_mimic(ch)) {
      send_to_char(g, ch, "You currently Mimic a %s", pc_race_types[char_get_mimic(ch)]);
    }

    return;
  }

  if (!strcasecmp(arg, "stop")) {
    if (!char_get_mimic(ch)) {
      send_to_char(g, ch, "You are not imitating another race.\r\n");
      return;
    }
    act(g, "@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_mimic(ch, 0);
  }

  int race = RACE_UNDEFINED;
  for (int r = 0; r < NUM_RACES; r++) {
    if (can_mimic[r] && !strcasecmp(arg, race_name(r))) {
      race = r;
      break;
    }
  }
  if (race == RACE_UNDEFINED) {
    send_to_char(g, ch, "That is not a race you can change into. Enter mimic without arugments for the mimic menu.\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_MIMIC), perc = axion_dice(g, 0);
  double mult  = 1 / prob;
  int64_t cost = getMaxKI(g, ch) * mult;

  if (race == char_get_mimic(ch)) {
    send_to_char(g, ch, "You are already mimicing that race. To stop enter 'mimic stop'\r\n");
    return;
  }
  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }
  if (prob < perc) {
    decCurKI(g, ch, cost, 0);
    act(g, "@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  char buf[MAX_STRING_LENGTH];
  char_set_mimic(ch, race);
  decCurKI(g, ch, cost, 0);
  snprintf(buf, sizeof(buf), "@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be %s @M%s!@n", AN(juggleRaceName(ch, true)), juggleRaceName(ch, false));
  send_to_char(g, ch, "@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be %s %s.@n\r\n", AN(juggleRaceName(ch, true)), juggleRaceName(ch, false));
  act(g, buf, true, ch, nullptr, nullptr, TO_ROOM);
  return;
}

ACMD(do_kyodaika)
{

  if (char_is_npc(ch)) {
    return;
  }

  if (!char_is_namek(ch)) {
    send_to_char(g, ch, "You are not a namek!\r\n");
    return;
  }

  if (!is_affected(ch, AFF_KYODAIKA)) {
    act(g, "@GYou growl as your body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@g$n@G growls as $s body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, TO_ROOM);
    send_to_char(g, ch, "@cStrength@D: @C+5\r\n@cSpeed@D: @c-2@n\r\n");
    assign_affect(g, ch, AFF_KYODAIKA, 0, -1, 5, 0, 0, 0, 0, -2);
    save_char(g, ch);
    return;
  }
  act(g, "@GYou growl as your body shrinks to its normal size!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@g$n@G growls as $s body shrinks to its normal size!@n", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_char(g, ch, "@cStrength@D: @C-5\r\n@cSpeed@D: @c+2@n\r\n");
  remove_affect(g, ch, AFF_KYODAIKA);
  save_char(g, ch);
  return;
}

ACMD(do_table)
{
  struct obj_data *obj = nullptr, *obj2 = nullptr;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: table (red | blue | green | yellow) (card name)");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
    send_to_char(g, ch, "You don't see that table here.\r\n");
    return;
  }

  if (!(obj2 = get_obj_in_list_vis(g, ch, arg2, nullptr, obj_get_contains(obj)))) {
    send_to_char(g, ch, "That card doesn't seem to be on that table.\r\n");
    return;
  }

  char buf[200];
  snprintf(buf, sizeof(buf), "$n looks at %s on %s.\r\n", obj_get_short_description(obj2), obj_get_short_description(obj));
  act(g, buf, true, ch, nullptr, nullptr, TO_ROOM);
  send_to_char(g, ch, "%s", obj_get_action_description(obj2));
}

ACMD(do_draw)
{
  if (!char_get_sits(ch)) {
    send_to_char(g, ch, "You are not sitting at a duel table.\r\n");
    return;
  }

  if (obj_get_vnum(g, char_get_sits(ch)) < 604 || obj_get_vnum(g, char_get_sits(ch)) > 607) {
    send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  struct obj_data *obj = nullptr, *obj2 = nullptr, *obj3 = nullptr, *next_obj = nullptr;
  int drawn = false;

  if (!(obj = get_obj_in_list_vis(g, ch, writable{"case"}, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't have a case.\r\n");
    return;
  }
  for (obj2 = obj_get_contains(obj); obj2; obj2 = next_obj) {
    next_obj = obj_get_next_content(obj2);
    if (drawn == false) {
      obj_from_obj(g, obj2);
      obj_to_char(g, obj2, ch);
      obj3  = obj2;
      drawn = true;
    }
  }
  if (drawn == false) {
    send_to_char(g, ch, "You don't have any cards in the case!\r\n");
    return;
  }
  act(g, "$n draws a card from $s $p.\r\n", true, ch, obj, nullptr, TO_ROOM);
  send_to_char(g, ch, "You draw a card.\r\n%s\r\n", obj_get_action_description(obj3));
  return;
}

ACMD(do_shuffle)
{

  if (!char_get_sits(ch)) {
    send_to_char(g, ch, "You are not sitting at a duel table.\r\n");
    return;
  }

  if (obj_get_vnum(g, char_get_sits(ch)) < 604 || obj_get_vnum(g, char_get_sits(ch)) > 607) {
    send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  struct obj_data *obj = nullptr, *obj2 = nullptr, *next_obj = nullptr;
  int count = 0;

  if (!(obj = get_obj_in_list_vis(g, ch, writable{"case"}, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't have a case.\r\n");
    return;
  }

  for (obj2 = obj_get_contains(obj); obj2; obj2 = next_obj) {
    next_obj = obj_get_next_content(obj2);
    if (!obj_extra_flagged(obj2, ITEM_ANTI_HIEROPHANT)) {
      continue;
    }
    count += 1;
  }
  if (count <= 0) {
    send_to_char(g, ch, "You don't have any cards in the case!\r\n");
    return;
  }
  int total = count;
  for (obj2 = obj_get_contains(obj); obj2; obj2 = next_obj) {
    next_obj = obj_get_next_content(obj2);
    obj_from_obj(g, obj2);
    obj_to_room(g, obj2, real_room(g, 48));
  }
  while (count > 0) {
    for (obj2 = rroom_get_contents(g, real_room(g, 48)); obj2; obj2 = next_obj) {
      next_obj = obj_get_next_content(obj2);
      if (!obj_extra_flagged(obj2, ITEM_ANTI_HIEROPHANT)) {
        continue;
      }
      if (obj2 && count > 1 && rand_number(g, 1, 4) == 3) {
        count -= 1;
        obj_from_room(g, obj2);
        obj_to_obj(g, obj2, obj);
      } else if (obj2 && count == 1) {
        count -= 1;
        obj_from_room(g, obj2);
        obj_to_obj(g, obj2, obj);
      }
    }
  }
  send_to_char(g, ch, "You shuffle the cards carefully.\r\n");
  act(g, "$n shuffles their deck.", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_room(g, char_get_in_room(ch), "There were %d cards in the deck.\r\n", total);
}

ACMD(do_hand)
{

  struct obj_data *obj, *next_obj;
  char arg[MAX_INPUT_LENGTH];
  int count = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: hand (look | show)\r\n");
    return;
  }

  if (!strcasecmp("look", arg)) {
    send_to_char(g, ch, "@CYour hand contains:\r\n@D---------------------------@n\r\n");
    for (obj = char_get_inventory(ch); obj; obj = next_obj) {
      next_obj = obj_get_next_content(obj);
      if (obj && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
        continue;
      }
      if (obj) {
        count += 1;
        send_to_char(g, ch, "%s\r\n", obj_get_short_description(obj));
      }
    }
    act(g, "$n looks at $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
    if (count == 0) {
      send_to_char(g, ch, "No cards.");
      act(g, "There were no cards.", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (count > 7) {
      act(g, "You have more than seven cards in your hand.", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
    } else {
      char buf[200];
      snprintf(buf, sizeof(buf), "There are %d cards in the hand.", count);
      act(g, buf, true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else if (!strcasecmp("show", arg)) {
    send_to_char(g, ch, "You show off your hand to the room.\r\n");
    act(g, "@C$n's hand contains:\r\n@D---------------------------@n", true, ch, nullptr, nullptr, TO_ROOM);
    for (obj = char_get_inventory(ch); obj; obj = next_obj) {
      next_obj = obj_get_next_content(obj);
      if (obj && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
        continue;
      }
      if (obj) {
        count += 1;
        act(g, "$p", true, ch, obj, nullptr, TO_ROOM);
      }
    }
    if (count == 0) {
      act(g, "No cards.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    if (count > 7) {
      act(g, "You have more than seven cards in your hand.", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else {
    send_to_char(g, ch, "Syntax: hand (look | show)\r\n");
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
    send_to_char(g, ch, "Syntax: post (obj name)\n"
                     "        post (obj name) (target obj name)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't seem to have that.\r\n");
    return;
  }

  if (obj_get_type_flag(obj) != ITEM_NOTE) {
    send_to_char(g, ch, "You can only post notepaper.\r\n");
    return;
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2)) {
    send_to_char(g, ch, "You can not post on things in a garden.\r\n");
    return;
  }

  if (!*arg2) {
    if (rroom_sect(g, char_get_in_room(ch)) != SECT_INSIDE && rroom_sect(g, char_get_in_room(ch)) != SECT_CITY) {
      send_to_char(g, ch, "You are not near any general structure you can post it on.\r\n");
      return;
    }
    act(g, "@WYou post $p@W on a nearby structure.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W posts $p@W on a nearby structure.@n", true, ch, obj, nullptr, TO_ROOM);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
    obj_set_posttype(obj, 1);
    return;
  }
  if (!(obj2 = get_obj_in_list_vis(g, ch, arg2, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
    send_to_char(g, ch, "You can't seem to find the thing you want to post it on.\r\n");
    return;
  }
  if (obj_get_posted_to(obj2)) {
    send_to_char(g, ch, "It already has something posted on it. Get that first if you want to post.\r\n");
    return;
  }
  if (obj_get_type_flag(obj2) == ITEM_BOARD) {
    send_to_char(g, ch, "Boards come with their own means of posting messages.\r\n");
    return;
  }
  char buf[MAX_STRING_LENGTH];
  snprintf(buf, sizeof(buf), "@C$n@W posts %s@W on %s@W.@n", obj_get_short_description(obj), obj_get_short_description(obj2));
  send_to_char(g, ch, "@WYou post %s@W on %s@W.@n\r\n", obj_get_short_description(obj), obj_get_short_description(obj2));
  act(g, buf, true, ch, nullptr, nullptr, TO_ROOM);
  obj_from_char(g, obj);
  obj_to_room(g, obj, char_get_in_room(ch));
  obj_set_posttype(obj, 2);
  obj_set_posted_to(obj, obj2);
  obj_set_posted_to(obj2, obj);
  return;
}

ACMD(do_play)
{

  if (char_get_position(ch) != POS_SITTING) {
    send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  if (!char_get_sits(ch)) {
    send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  if (obj_get_vnum(g, char_get_sits(ch)) < 604 || obj_get_vnum(g, char_get_sits(ch)) > 607) {
    send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  struct obj_data *obj = nullptr, *obj2 = nullptr, *obj3 = nullptr, *next_obj = nullptr;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: play (card name)");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't have that card to play.\r\n");
    return;
  }

  for (obj3 = rroom_get_contents(g, char_get_in_room(ch)); obj3; obj3 = next_obj) {
    next_obj = obj_get_next_content(obj3);
    if (obj_get_vnum(g, obj3) == obj_get_vnum(g, char_get_sits(ch)) - 4) {
      obj2 = obj3;
    }
  }

  if (obj2 == nullptr) {
    send_to_char(g, ch, "Your table is missing. Inform an immortal of this problem.\r\n");
    return;
  }

  act(g, "You play $p on your table.", true, ch, obj, nullptr, TO_CHAR);
  act(g, "$n plays $p on $s table.", true, ch, obj, nullptr, TO_ROOM);
  obj_from_char(g, obj);
  obj_to_obj(g, obj, obj2);
}

/* Nickname an object */
ACMD(do_nickname)
{
  struct obj_data *obj = nullptr;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: nickname (object) (nickname)\n");
    send_to_char(g, ch, "Syntax: nickname ship (nickname)\n");
    return;
  }

  if (strcasecmp(arg, "ship")) {
    if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You don't have that item to nickname.\r\n");
      return;
    }
  }
  if (strlen(arg2) > 20) {
    send_to_char(g, ch, "You can't nickname items with any name longer than 20 characters.\r\n");
    return;
  }

  if (!strcasecmp(arg, "ship")) {
    struct obj_data *ship = nullptr, *next_obj = nullptr, *ship2 = nullptr;
    int found = false;
    for (ship = rroom_get_contents(g, char_get_in_room(ch)); ship; ship = next_obj) {
      next_obj = obj_get_next_content(ship);
      if (obj_get_vnum(g, ship) >= 45000 && obj_get_vnum(g, ship) <= 45999 && found == false) {
        found = true;
        ship2 = ship;
      }
    }
    if (found == true) {
      if (strstr(arg2, "@")) {
        send_to_char(g, ch, "You can't nickname a ship and use color codes. Sorry.\r\n");
        return;
      }
      char nick[MAX_INPUT_LENGTH];
      snprintf(nick, sizeof(nick), "%s", CAP(arg2));
      obj_set_action_description(ship2, strdup(nick));
      struct obj_data *k, *next_k;
      for (k = g->objects; k; k = next_k) {
        next_k = obj_get_next(k);
        if (obj_get_vnum(g, k) == obj_get_vnum(g, ship2) + 1000) {
          extract_obj(g, k);
          room_vnum was_in = rroom_get_vnum(g, obj_get_in_room(ship2));
          obj_from_room(g, ship2);
          obj_to_room(g, ship2, real_room(g, was_in));
        }
      }
    } else
      send_to_char(g, ch, "There's no ship here to nickname.\r\n");
    return;
  }

  if (strstr(obj_get_short_description(obj), "nicknamed")) {
    send_to_char(g, ch, "%s@w has already been nicknamed.@n\r\n", obj_get_short_description(obj));
    return;
  }
  if (strstr(obj_get_name(obj), "corpse")) {
    send_to_char(g, ch, "%s@w is a corpse!@n\r\n", obj_get_short_description(obj));
    return;
  }
  send_to_char(g, ch, "@wYou nickname %s@w as '@C%s@w'.@n\r\n", obj_get_short_description(obj), arg2);
  char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH];
  snprintf(nick, sizeof(nick), "%s @wnicknamed @D(@C%s@D)@n", obj_get_short_description(obj), CAP(arg2));
  snprintf(nick2, sizeof(nick2), "%s %s", obj_get_name(obj), arg2);
  obj_set_short_description(obj, strdup(nick));
  obj_set_name(obj, strdup(nick2));
  return;
}

ACMD(do_showoff)
{
  struct obj_data *obj   = nullptr;
  struct char_data *vict = nullptr;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  *arg  = '\0';
  *arg2 = '\0';

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "You want to show what item to what character?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't seem to have that.\r\n");
    return;
  }
  if (!(vict = get_player_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "There is no such person around.\r\n");
    return;
  } else { /* Ok show that target the object! */
    act(g, "@WYou hold up $p@W for @C$N@W to see:@n", true, ch, obj, vict, TO_CHAR);
    act(g, "@C$n@W holds up $p@W for you to see:@n", true, ch, obj, vict, TO_VICT);
    act(g, "@C$n@W holds up $p@W for @c$N@W to see.@n", true, ch, obj, vict, TO_NOTVICT);
    show_obj_to_char(g, obj, vict, SHOW_OBJ_ACTION);
    return;
  }
}

/* Used for checking if you know the character in question. */
void introCreate(struct char_data *ch)
{
  struct atomic_file af;
  char fname[40];
  FILE *fl;
  /* Write Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch)))
    return;

  if (!(fl = atomic_open(&af, fname, true))) {
    mud_log("ERROR: could not save user, %s, to filename, %s.", char_get_name(ch), fname);
    return;
  }

  fprintf(fl, "Gibbles Gibbles\n");

  atomic_commit(&af);
  return;
}

int readIntro(struct char_data *ch, struct char_data *vict)
{
  char fname[40], filler[50], line[256];
  int known = false;
  FILE *fl;

  /* Read Introduction File */
  if (vict == nullptr) {
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

  while (get_line(fl, line)) {
    if (sscanf(line, "%49s", filler) == 1 && !strcasecmp(char_get_name(vict), filler)) {
      known = true;
    }
  }
  fclose(fl);

  if (known == true)
    return 1;
  else
    return 0;
}

static void introWrite(struct char_data *ch, struct char_data *vict, char *name)
{
  struct atomic_file af;
  FILE *file;
  char fname[40], filler[50], scrap[100], line[256];
  char *names[500] = {nullptr}, *alias[500] = {nullptr};
  FILE *fl;
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
    introCreate(ch);
  }
  if (!(file = fopen(fname, "r"))) {
    return;
  }
  /* one slot is kept free for the new introduction written below */
  while (count < 499 && get_line(file, line)) {
    *filler = *scrap = '\0';
    if (sscanf(line, "%49s %99s", filler, scrap) < 1)
      continue;
    names[count] = strdup(filler);
    alias[count] = strdup(scrap);
    count++;
  }
  fclose(file);

  /* Write Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch)))
    return;

  if (!(fl = atomic_open(&af, fname, true))) {
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
    if (names[x] != nullptr) {
      free(names[x]);
    }
    if (alias[x] != nullptr) {
      free(alias[x]);
    }
    x++;
  }

  fprintf(fl, "%s %s\n", char_get_name(vict), CAP(name));

  atomic_commit(&af);
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
    send_to_char(g, ch, "Syntax: dub (target) (name)\r\nWho do you want to dub and what do you want to name them?\r\n");
    return;
  }

  if (!*arg2) {
    send_to_char(g, ch, "Syntax: dub (target) (name)\r\nWhat name do you wish to know them by?\r\n");
    return;
  }

  if (strlen(arg2) > 20) {
    send_to_char(g, ch, "Limit the name to 20 characters.\r\n");
    return;
  }
  if (strlen(arg2) < 3) {
    send_to_char(g, ch, "Limit the name to at least 3 characters.\r\n");
    return;
  }

  if (strstr(arg2, "$") || strstr(arg2, "@") || strstr(arg2, "%")) {
    send_to_char(g, ch, "Illegal character. No symbols.\r\n");
    return;
  }

  if (!(vict = get_player_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "There is no such person.\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(g, ch, "That seems rather odd.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(g, ch, "That seems rather unwise.\r\n");
    return;
  }

  if (readIntro(vict, ch) == 2) {
    send_to_char(g, ch, "There seems to have been an error, report this to Iovan.\r\n");
    return;
  }
  if (readIntro(ch, vict) == 1 && strstr(juggleRaceName(vict, true), arg)) {
    send_to_char(g, ch, "You have already dubbed them a name. If you want to redub them target the name you know them by.\r\n");
    return;
  }
  introWrite(ch, vict, arg2);
  act(g, "You decide to call $M, $N.", true, ch, nullptr, vict, TO_CHAR);
  act(g, "$n seems to decide something about you.", true, ch, nullptr, vict, TO_VICT);
  act(g, "$n seems to decide something about $N.", true, ch, nullptr, vict, TO_NOTVICT);
  return;
}

ACMD(do_rptrans)
{
  struct char_data *vict = nullptr;
  struct descriptor_data *k;
  int amt = 0;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: exchange (target) (amount)\r\n");
    return;
  }

  amt = atoi(arg2);

  if (amt <= 0) {
    send_to_char(g, ch, "Are you being funny?\r\n");
    return;
  }

  if (amt > char_get_rp(ch)) {
    send_to_char(g, ch, "@WYou only have @C%d@W RPP!@n\r\n", char_get_rp(ch));
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(g, ch, "That is not a recognised user file.\r\n");
    return;
  }

  for (k = g->descriptors; k; k = k->next) {
    if (desc_get_connected(k) != CON_PLAYING)
      continue;
    if (char_is_npc(k->character))
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }
  if (vict == nullptr) {
    userWrite(g, nullptr, 0, amt, 0, arg);
  } else {
    char_set_rp(vict, char_get_rp(vict) + (amt));
    char_get_desc(vict)->rpp += amt;
    userWrite(g, char_get_desc(vict), 0, 0, 0, "index");
    char_set_trp(vict, char_get_trp(vict) + (amt));
    save_char(g, vict);
    send_to_char(g, vict, "@W%s gives @C%d@W of their RPP to you. How nice!\r\n", char_get_name(ch), amt);
  }
  char_set_rp(ch, char_get_rp(ch) - (amt));
  char_get_desc(ch)->rpp -= amt;
  userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
  send_to_char(g, ch, "@WYou exchange @C%d@W RPP to user @c%s@W for a warm fuzzy feeling.\r\n", amt, CAP(arg));
  mudlog(g, LOG_NRM, MAX(ADMLVL_IMPL, char_get_invis_level(g, ch)), true, "EXCHANGE: %s gave %d RPP to user %s", char_get_name(ch), amt, arg);
  save_char(g, ch);
}

ACMD(do_rdisplay)
{
  skip_spaces(&argument);

  if (char_is_npc(ch)) {
    return;
  }

  if (!*argument) {
    send_to_char(g, ch, "Clearing room display.\r\n");
    char_set_rdisplay(ch, "Empty");
  } else {
    char derp[MAX_STRING_LENGTH];

    strcpy(derp, argument);

    send_to_char(g, ch, "You set your display to; %s\r\n", derp);
    char_set_rdisplay(ch, derp);
  }
}
