/* rp.c: intros, RP bank and display, mimic, nickname, card games
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <cstdlib>
#include <ranges>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
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
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

ACMD(do_mimic)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_get_skill(ch, SKILL_MIMIC)) {
    send_to_char(g, ch, "You do not know how to mimic the appearance of other races.\r\n");
    return;
  }

  auto can_mimic = [&](int r) { return race_can_be_mimiced(r) && race_valid_sex(r, char_get_sex(ch)); };
  auto races     = std::views::iota(0, NUM_RACES);

  if (arg.empty()) {
    send_to_char(g, ch, "@CMimic Menu\n@c--------------------@W\r\n");
    for (int r : races | std::views::filter(can_mimic))
      send_to_char(g, ch, "{}\n", race_name(r));
    send_to_char(g, ch, "Stop@n\r\n");
    if (char_get_mimic(ch))
      send_to_char(g, ch, "You currently Mimic a {}", pc_race_types[char_get_mimic(ch)]);
    return;
  }

  if (str_eq_nocase(arg, "stop")) {
    if (!char_get_mimic(ch)) {
      send_to_char(g, ch, "You are not imitating another race.\r\n");
      return;
    }
    act(g, "@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", true, ch, nullptr, nullptr, act_to::actor);
    act(g, "@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", true, ch, nullptr, nullptr, act_to::room);
    char_set_mimic(ch, 0);
    /* quirk: goes on, to be told that "stop" is no race */
  }

  auto it = std::ranges::find_if(races, [&](int r) { return can_mimic(r) && str_eq_nocase(arg, race_name(r)); });
  if (it == races.end()) {
    send_to_char(g, ch, "That is not a race you can change into. Enter mimic without arugments for the mimic menu.\r\n");
    return;
  }
  int race = *it;

  int prob = char_get_skill(ch, SKILL_MIMIC), perc = axion_dice(g, 0);
  double mult  = 1 / prob; /* quirk: whole numbers, so it's free unless the skill is 1 */
  int64_t cost = getMaxKI(g, ch) * mult;

  if (race == char_get_mimic(ch)) {
    send_to_char(g, ch, "You are already mimicing that race. To stop enter 'mimic stop'\r\n");
    return;
  }
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }
  if (prob < perc) {
    decCurKI(g, ch, cost, 0);
    act(g, "@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n", true, ch, nullptr, nullptr, act_to::actor);
    act(g, "@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n", true, ch, nullptr, nullptr, act_to::room);
    return;
  }
  char_set_mimic(ch, race);
  decCurKI(g, ch, cost, 0);
  std::string buf = str_format("@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be {} @M{}!@n", AN(juggleRaceName(ch, true)), juggleRaceName(ch, false));
  send_to_char(g, ch, "@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be {} {}.@n\r\n", AN(juggleRaceName(ch, true)), juggleRaceName(ch, false));
  act(g, buf, true, ch, nullptr, nullptr, act_to::room);
}

ACMD(do_kyodaika)
{
  if (char_is_npc(ch))
    return;
  if (!char_is_namek(ch)) {
    send_to_char(g, ch, "You are not a namek!\r\n");
    return;
  }

  if (!is_affected(ch, aff_flag::kyodaika)) {
    act(g, "@GYou growl as your body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, act_to::actor);
    act(g, "@g$n@G growls as $s body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, act_to::room);
    send_to_char(g, ch, "@cStrength@D: @C+5\r\n@cSpeed@D: @c-2@n\r\n");
    assign_affect(g, ch, aff_flag::kyodaika, 0, -1, 5, 0, 0, 0, 0, -2);
  } else {
    act(g, "@GYou growl as your body shrinks to its normal size!@n", true, ch, nullptr, nullptr, act_to::actor);
    act(g, "@g$n@G growls as $s body shrinks to its normal size!@n", true, ch, nullptr, nullptr, act_to::room);
    send_to_char(g, ch, "@cStrength@D: @C-5\r\n@cSpeed@D: @c+2@n\r\n");
    remove_affect(g, ch, aff_flag::kyodaika);
  }
  save_char(g, ch);
}

ACMD(do_table)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: table (red | blue | green | yellow) (card name)");
    return;
  }
  auto table = find_in_room(g, ch, arg, "You don't see that table here.\r\n");
  if (reject(g, ch, table))
    return;
  auto card = find_inside(g, ch, arg2, *table, "That card doesn't seem to be on that table.\r\n");
  if (reject(g, ch, card))
    return;

  std::string buf = str_format("$n looks at {} on {}.\r\n", obj_get_short_description(*card), obj_get_short_description(*table));
  act(g, buf, true, ch, nullptr, nullptr, act_to::room);
  send_to_char(g, ch, "{}", obj_get_action_description(*card));
}

namespace {
  /* Whether ch sits at one of the official duel tables. */
  bool at_duel_table(GameWorld *g, struct char_data *ch)
  {
    if (!char_get_sits(g, ch)) {
      send_to_char(g, ch, "You are not sitting at a duel table.\r\n");
      return false;
    }
    if (obj_get_vnum(g, char_get_sits(g, ch)) < 604 || obj_get_vnum(g, char_get_sits(g, ch)) > 607) {
      send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
      return false;
    }
    return true;
  }

  parsed<struct obj_data *> card_case(GameWorld *g, struct char_data *ch)
  {
    std::string what = "case";
    return find_carried(g, ch, what, "You don't have a case.\r\n");
  }

  bool is_card(struct obj_data *obj) { return obj_extra_flagged(obj, extra_flag::anti_hierophant); }
} // namespace

ACMD(do_draw)
{
  if (!at_duel_table(g, ch))
    return;
  auto box = card_case(g, ch);
  if (reject(g, ch, box))
    return;

  struct obj_data *card = find_first(obj_contents(g, *box), [](obj_data *) { return true; });
  if (!card) {
    send_to_char(g, ch, "You don't have any cards in the case!\r\n");
    return;
  }
  obj_from_obj(g, card);
  obj_to_char(g, card, ch);
  act(g, "$n draws a card from $s $p.\r\n", true, ch, *box, nullptr, act_to::room);
  send_to_char(g, ch, "You draw a card.\r\n{}\r\n", obj_get_action_description(card));
}

ACMD(do_shuffle)
{
  if (!at_duel_table(g, ch))
    return;
  auto box = card_case(g, ch);
  if (reject(g, ch, box))
    return;

  int count = static_cast<int>(std::ranges::count_if(obj_contents(g, *box), is_card));
  if (count <= 0) {
    send_to_char(g, ch, "You don't have any cards in the case!\r\n");
    return;
  }
  int total = count;
  /* quirk: everything in the case goes to room 48, but only the cards come back */
  for (obj_data *obj : obj_contents(g, *box)) {
    obj_from_obj(g, obj);
    obj_to_room(g, obj, 48);
  }
  /* Back in the case: each card a quarter of the time, round and round, until only one is left, which goes last. */
  while (count > 0) {
    for (obj_data *card : room_contents(g, 48) | std::views::filter(is_card)) {
      if (count > 1 && rand_number(g, 1, 4) != 3)
        continue;
      if (count < 1)
        continue;
      count -= 1;
      obj_from_room(g, card);
      obj_to_obj(g, card, *box);
    }
  }
  send_to_char(g, ch, "You shuffle the cards carefully.\r\n");
  act(g, "$n shuffles their deck.", true, ch, nullptr, nullptr, act_to::room);
  send_to_room(g, char_get_in_room(ch), "There were {:d} cards in the deck.\r\n", total);
}

ACMD(do_hand)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  int count             = 0;

  if (str_eq_nocase("look", arg)) {
    send_to_char(g, ch, "@CYour hand contains:\r\n@D---------------------------@n\r\n");
    for (obj_data *card : char_inventory(g, ch) | std::views::filter(is_card)) {
      count += 1;
      send_to_char(g, ch, "{}\r\n", obj_get_short_description(card));
    }
    act(g, "$n looks at $s hand.", true, ch, nullptr, nullptr, act_to::room);
    if (count == 0) {
      send_to_char(g, ch, "No cards.");
      act(g, "There were no cards.", true, ch, nullptr, nullptr, act_to::room);
    } else if (count > 7) {
      act(g, "You have more than seven cards in your hand.", true, ch, nullptr, nullptr, act_to::actor);
      act(g, "$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, act_to::room);
    } else {
      act(g, str_format("There are {:d} cards in the hand.", count), true, ch, nullptr, nullptr, act_to::room);
    }
  } else if (str_eq_nocase("show", arg)) {
    send_to_char(g, ch, "You show off your hand to the room.\r\n");
    act(g, "@C$n's hand contains:\r\n@D---------------------------@n", true, ch, nullptr, nullptr, act_to::room);
    for (obj_data *card : char_inventory(g, ch) | std::views::filter(is_card)) {
      count += 1;
      act(g, "$p", true, ch, card, nullptr, act_to::room);
    }
    if (count == 0)
      act(g, "No cards.", true, ch, nullptr, nullptr, act_to::room);
    if (count > 7) {
      act(g, "You have more than seven cards in your hand.", true, ch, nullptr, nullptr, act_to::actor);
      act(g, "$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, act_to::room);
    }
  } else {
    send_to_char(g, ch, "Syntax: hand (look | show)\r\n");
  }
}

ACMD(do_post)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: post (obj name)\n        post (obj name) (target obj name)\r\n");
    return;
  }
  auto note = find_carried(g, ch, arg, "You don't seem to have that.\r\n");
  if (reject(g, ch, note))
    return;
  struct obj_data *obj = *note;
  if (obj_get_type_flag(obj) != ITEM_NOTE) {
    send_to_char(g, ch, "You can only post notepaper.\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::garden1) || room_flagged(g, char_get_in_room(ch), room_flag::garden2)) {
    send_to_char(g, ch, "You can not post on things in a garden.\r\n");
    return;
  }

  if (arg2.empty()) {
    if (room_sect(g, char_get_in_room(ch)) != SECT_INSIDE && room_sect(g, char_get_in_room(ch)) != SECT_CITY) {
      send_to_char(g, ch, "You are not near any general structure you can post it on.\r\n");
      return;
    }
    act(g, "@WYou post $p@W on a nearby structure.@n", true, ch, obj, nullptr, act_to::actor);
    act(g, "@C$n@W posts $p@W on a nearby structure.@n", true, ch, obj, nullptr, act_to::room);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
    obj_set_posttype(obj, 1);
    return;
  }
  auto target = find_in_room(g, ch, arg2, "You can't seem to find the thing you want to post it on.\r\n");
  if (reject(g, ch, target))
    return;
  struct obj_data *obj2 = *target;
  if (obj_get_posted_to(g, obj2)) {
    send_to_char(g, ch, "It already has something posted on it. Get that first if you want to post.\r\n");
    return;
  }
  if (obj_get_type_flag(obj2) == ITEM_BOARD) {
    send_to_char(g, ch, "Boards come with their own means of posting messages.\r\n");
    return;
  }
  std::string buf = str_format("@C$n@W posts {}@W on {}@W.@n", obj_get_short_description(obj), obj_get_short_description(obj2));
  send_to_char(g, ch, "@WYou post {}@W on {}@W.@n\r\n", obj_get_short_description(obj), obj_get_short_description(obj2));
  act(g, buf, true, ch, nullptr, nullptr, act_to::room);
  obj_from_char(g, obj);
  obj_to_room(g, obj, char_get_in_room(ch));
  obj_set_posttype(obj, 2);
  obj_set_posted_to(obj, obj2);
  obj_set_posted_to(obj2, obj);
}

ACMD(do_play)
{
  if (char_get_position(ch) != POS_SITTING || !char_get_sits(g, ch) || obj_get_vnum(g, char_get_sits(g, ch)) < 604 ||
      obj_get_vnum(g, char_get_sits(g, ch)) > 607) {
    send_to_char(g, ch, "You need to be sitting at an official table to play.\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: play (card name)");
    return;
  }
  auto card = find_carried(g, ch, arg, "You don't have that card to play.\r\n");
  if (reject(g, ch, card))
    return;

  /* the seat's own table, 4 vnums below it: the last one in the room */
  obj_vnum table_vnum    = obj_get_vnum(g, char_get_sits(g, ch)) - 4;
  struct obj_data *table = nullptr;
  for (obj_data *obj : room_contents(g, char_get_in_room(ch)) | std::views::filter([&](obj_data *o) { return obj_get_vnum(g, o) == table_vnum; }))
    table = obj;
  if (!table) {
    send_to_char(g, ch, "Your table is missing. Inform an immortal of this problem.\r\n");
    return;
  }

  act(g, "You play $p on your table.", true, ch, *card, nullptr, act_to::actor);
  act(g, "$n plays $p on $s table.", true, ch, *card, nullptr, act_to::room);
  obj_from_char(g, *card);
  obj_to_obj(g, *card, table);
}

namespace {
  void nickname_ship(GameWorld *g, struct char_data *ch, std::string nick)
  {
    struct obj_data *ship = find_first(room_contents(g, char_get_in_room(ch)), [&](obj_data *o) { return obj_get_vnum(g, o) >= 45000 && obj_get_vnum(g, o) <= 45999; });
    if (!ship) {
      send_to_char(g, ch, "There's no ship here to nickname.\r\n");
      return;
    }
    if (nick.find('@') != std::string::npos) {
      send_to_char(g, ch, "You can't nickname a ship and use color codes. Sorry.\r\n");
      return;
    }
    obj_set_action_description(ship, CAP(nick));
    /* its interior, 1000 vnums on: gone, and the ship put back to show its new name */
    for (obj_data *k : each_obj(g) | std::views::filter([&](obj_data *o) { return obj_get_vnum(g, o) == obj_get_vnum(g, ship) + 1000; })) {
      extract_obj(g, k);
      room_vnum was_in = obj_get_in_room(ship);
      obj_from_room(g, ship);
      obj_to_room(g, ship, was_in);
    }
  }
} // namespace

/* Nickname an object */
ACMD(do_nickname)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: nickname (object) (nickname)\n");
    send_to_char(g, ch, "Syntax: nickname ship (nickname)\n");
    return;
  }
  bool ship = str_eq_nocase(arg, "ship");
  parsed<struct obj_data *> item;
  if (!ship && reject(g, ch, item = find_carried(g, ch, arg, "You don't have that item to nickname.\r\n")))
    return;
  if (arg2.size() > 20) {
    send_to_char(g, ch, "You can't nickname items with any name longer than 20 characters.\r\n");
    return;
  }
  if (ship) {
    nickname_ship(g, ch, arg2);
    return;
  }

  struct obj_data *obj = *item;
  if (obj_get_short_description(obj).contains("nicknamed")) {
    send_to_char(g, ch, "{}@w has already been nicknamed.@n\r\n", obj_get_short_description(obj));
    return;
  }
  if (obj_get_name(obj).contains("corpse")) {
    send_to_char(g, ch, "{}@w is a corpse!@n\r\n", obj_get_short_description(obj));
    return;
  }
  send_to_char(g, ch, "@wYou nickname {}@w as '@C{}@w'.@n\r\n", obj_get_short_description(obj), arg2);
  std::string nick  = str_format("{} @wnicknamed @D(@C{}@D)@n", obj_get_short_description(obj), CAP(arg2));
  std::string nick2 = str_format("{} {}", obj_get_name(obj), arg2);
  obj_set_short_description(obj, nick);
  obj_set_name(obj, nick2);
}

constexpr act_msgs SHOWOFF = {"@WYou hold up $p@W for @C$N@W to see:@n", "@C$n@W holds up $p@W for you to see:@n", "@C$n@W holds up $p@W for @c$N@W to see.@n", true};

ACMD(do_showoff)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "You want to show what item to what character?\r\n");
    return;
  }
  auto obj = find_carried(g, ch, arg, "You don't seem to have that.\r\n");
  if (reject(g, ch, obj))
    return;
  auto vict = find_player(g, ch, arg2, find_flag::char_room, "There is no such person around.\r\n");
  if (reject(g, ch, vict))
    return;
  act_triple(g, SHOWOFF, ch, *obj, *vict);
  show_obj_to_char(g, *obj, *vict, SHOW_OBJ_ACTION);
}

/* Used for checking if you know the character in question. */
void introCreate(struct char_data *ch)
{
  std::optional<std::string> path = player_path(INTRO_FILE, char_get_name(ch));
  if (!path)
    return;
  io::text_writer out(*path, true);
  out.write("Gibbles Gibbles\n");
  if (!out.commit())
    mud_log("ERROR: could not save user, {}, to filename, {}.", char_get_name(ch), *path);
}

int readIntro(struct char_data *ch, struct char_data *vict)
{
  int known = false;

  /* Read Introduction File */
  if (vict == nullptr) {
    return 0;
  }

  if (char_is_npc(ch) || char_is_npc(vict)) {
    return 1;
  }

  std::optional<std::string> path = player_path(INTRO_FILE, char_get_name(ch));
  if (!path) {
    introCreate(ch);
    return 2;
  }
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file)
    return 2;
  if (vict == ch)
    return 0;

  while (std::optional<std::string_view> line = file->line()) {
    io::fields first(until_nul(*line));
    if (std::string filler; first.word(filler, 49) && str_eq_nocase(char_get_name(vict), filler)) {
      known = true;
    }
  }

  if (known == true)
    return 1;
  else
    return 0;
}

static void introWrite(struct char_data *ch, struct char_data *vict, std::string name)
{
  std::vector<std::pair<std::string, std::string>> known; /* each name, and what ch calls them */

  /* Read Introduction File */
  std::optional<std::string> path = player_path(INTRO_FILE, char_get_name(ch));
  if (!path) {
    introCreate(ch);
    return;
  }
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file)
    return;
  /* one slot is kept free for the new introduction written below */
  while (known.size() < 499) {
    std::optional<std::string_view> line = file->line();
    if (!line)
      break;
    std::string filler, scrap;
    io::fields words(until_nul(*line));
    words.word(filler, 49) && words.word(scrap, 99);
    if (words.count() < 1)
      continue;
    known.emplace_back(filler, scrap);
  }

  /* Write Introduction File */
  io::text_writer out(*path, true);
  for (size_t x = 0; x < known.size(); x++) {
    if (x == 0 || !str_eq_nocase(known[x - 1].first, known[x].first)) {
      if (!str_eq_nocase(known[x].first, char_get_name(vict))) {
        out.print("{} {}\n", known[x].first, known[x].second);
      }
    }
  }
  out.print("{} {}\n", char_get_name(vict), CAP(name));

  if (!out.commit())
    mud_log("ERROR: could not save intro file, {}, to filename, {}.", char_get_name(ch), *path);
}

constexpr act_msgs DUBBED = {"You decide to call $M, $N.", "$n seems to decide something about you.", "$n seems to decide something about $N.", true};

ACMD(do_intro)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: dub (target) (name)\r\nWho do you want to dub and what do you want to name them?\r\n");
    return;
  }
  if (arg2.empty()) {
    send_to_char(g, ch, "Syntax: dub (target) (name)\r\nWhat name do you wish to know them by?\r\n");
    return;
  }
  if (arg2.size() > 20) {
    send_to_char(g, ch, "Limit the name to 20 characters.\r\n");
    return;
  }
  if (arg2.size() < 3) {
    send_to_char(g, ch, "Limit the name to at least 3 characters.\r\n");
    return;
  }
  if (arg2.find_first_of("$@%") != std::string::npos) {
    send_to_char(g, ch, "Illegal character. No symbols.\r\n");
    return;
  }

  auto found = find_player(g, ch, arg, find_flag::char_room, "There is no such person.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
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
  if (readIntro(ch, vict) == 1 && juggleRaceName(vict, true).contains(arg)) {
    send_to_char(g, ch, "You have already dubbed them a name. If you want to redub them target the name you know them by.\r\n");
    return;
  }
  introWrite(ch, vict, arg2);
  act_triple(g, DUBBED, ch, nullptr, vict);
}

ACMD(do_rptrans)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: exchange (target) (amount)\r\n");
    return;
  }
  int amt = parse_int(arg2);
  if (amt <= 0) {
    send_to_char(g, ch, "Are you being funny?\r\n");
    return;
  }
  if (amt > char_get_rp(ch)) {
    send_to_char(g, ch, "@WYou only have @C{:d}@W RPP!@n\r\n", char_get_rp(ch));
    return;
  }
  if (!readUserIndex(arg)) {
    send_to_char(g, ch, "That is not a recognised user file.\r\n");
    return;
  }

  /* the user's character in the game, the last if there are more */
  struct char_data *vict = nullptr;
  for (descriptor_data *k : each_desc(g)) {
    if (desc_get_connected(k) == CON_PLAYING && !char_is_npc(k->character) && str_eq_nocase(k->acct.user, arg))
      vict = k->character;
  }
  if (vict == nullptr) {
    reward_offline_user(g, arg, amt);
  } else {
    char_set_rp(vict, char_get_rp(vict) + amt);
    char_get_desc(vict)->acct.rpp += amt;
    save_account(char_get_desc(vict));
    char_set_trp(vict, char_get_trp(vict) + amt);
    save_char(g, vict);
    send_to_char(g, vict, "@W{} gives @C{:d}@W of their RPP to you. How nice!\r\n", char_get_name(ch), amt);
  }
  char_set_rp(ch, char_get_rp(ch) - amt);
  char_get_desc(ch)->acct.rpp -= amt;
  save_account(char_get_desc(ch));
  send_to_char(g, ch, "@WYou exchange @C{:d}@W RPP to user @c{}@W for a warm fuzzy feeling.\r\n", amt, CAP(arg));
  mudlog(g, LOG_NRM, MAX(ADMLVL_IMPL, char_get_invis_level(g, ch)), true, "EXCHANGE: {} gave {:d} RPP to user {}", char_get_name(ch), amt, arg);
  save_char(g, ch);
}

ACMD(do_rdisplay)
{
  if (char_is_npc(ch))
    return;

  std::string text(skip_spaces(argument));
  if (text.empty()) {
    send_to_char(g, ch, "Clearing room display.\r\n");
    char_set_rdisplay(ch, "Empty");
  } else {
    send_to_char(g, ch, "You set your display to; {}\r\n", text);
    char_set_rdisplay(ch, text);
  }
}
