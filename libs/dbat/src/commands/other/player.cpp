/* player.c: general player commands (quit, save, group, title, toggles...)
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <cstdlib>
#include <iterator>

#include <sys/stat.h>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/auction.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.other.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/utils.hpp"
#include "dbat/flags.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/races.hpp"
#include "dbat/fileop.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/weather.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/graph.hpp"
#include "dbat/alias.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/shop.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"


ACMD(do_spar)
{
  if (char_is_npc(ch))
    return;
  if (char_plr_flagged(ch, PLR_SPAR)) {
    act(g, "@wYou cease your sparring stance.@n", false, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@w ceases $s sparring stance.@n", false, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "@wYou move into your sparring stance.@n", false, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@w moves into $s sparring stance.@n", false, ch, nullptr, nullptr, TO_ROOM);
  }
  char_plr_toggle(ch, PLR_SPAR);
}

namespace {
  /* Why ch can't quit here, if they can't */
  const char *cannot_quit(GameWorld *g, struct char_data *ch)
  {
    room_vnum room = char_get_in_room(ch);
    if (room_flagged(g, room, ROOM_PAST))
      return "This is the past, you can't quit here!\r\n";
    if (room >= 2002 && room <= 2011)
      return "You can't quit in the arena!\r\n";
    if (room >= 101 && room <= 139)
      return "You can't quit in the mud school!\r\n";
    if (room >= 19800 && room <= 19899)
      return "You can't quit in a pocket dimension!\r\n";
    if (room == 2069)
      return "You can't quit here!\r\n";
    return nullptr;
  }

  /* Where ch loads back in next time, if not where they were: anywhere but the void, the past or a pocket dimension */
  void remember_load_room(GameWorld *g, struct char_data *ch)
  {
    room_vnum room = char_get_in_room(ch);
    if (room != NOWHERE && room != 0 && room != 1 && (room < 19800 || room > 19899))
      char_set_load_room(g, ch, room);
  }
} // namespace

ACMD(do_quit)
{
  if (char_is_npc(ch) || !char_get_desc(ch))
    return;
  if (const char *why = cannot_quit(g, ch)) {
    send_to_char(g, ch, why);
    return;
  }
  if (char_get_mindlink(g, ch) && char_get_linker(ch) == 0) {
    send_to_char(g, ch, "@RYou feel like the mind that is linked with yours is preventing you from quiting!@n\r\n");
    if (char_get_in_room(char_get_mindlink(g, ch)) != NOWHERE) {
      look_at_room(g, char_get_in_room(char_get_mindlink(g, ch)), ch, 0);
      send_to_char(g, ch, "You get an impression of where this interference is originating from.\r\n");
    }
    return;
  }
  if (char_get_in_room(ch) == 2070) {
    send_to_char(g, ch, "You can't quit here!\r\n");
    return;
  }
  if (dball_count(g, ch)) {
    send_to_char(g, ch, "You can not quit while you have dragon balls! Place them somewhere first.");
    return;
  }

  if (subcmd != SCMD_QUIT)
    send_to_char(g, ch, "You have to type quit--no less, to quit!\r\n");
  else if (char_get_position(ch) == POS_FIGHTING)
    send_to_char(g, ch, "No way!  You're fighting for your life!\r\n");
  else if (char_get_position(ch) < POS_STUNNED) {
    send_to_char(g, ch, "You die before your time...\r\n");
    die(g, ch, nullptr);
  } else {
    act(g, "$n has left the game.", true, ch, nullptr, nullptr, TO_ROOM);
    mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} has quit the game.", char_get_name(ch));
    send_to_char(g, ch, "Goodbye, friend.. Come back soon!\r\n");
    if (char_get_followers(ch) || char_get_master(g, ch))
      die_follower(g, ch);
    if (ch == auction_seller(g))
      stop_auction(g, AUC_QUIT_CANCEL, nullptr);
    /* If someone is quitting in their house, let them load back here. (The past was refused above.) */
    remember_load_room(g, ch);
    Crash_rentsave(g, ch, 0);
    extract_char(g, ch); /* Char is saved before extracting. */
  }
  /* Remove any snoopers */
  if (char_get_desc(ch)->snoop_by) {
    write_to_output(g, char_get_desc(ch)->snoop_by, "Your victim is no longer among us.\r\n");
    char_get_desc(ch)->snoop_by->snooping = nullptr;
    char_get_desc(ch)->snoop_by           = nullptr;
  }
}

ACMD(do_save)
{
  if (char_is_npc(ch) || !char_get_desc(ch))
    return;
  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    send_to_char(g, ch, "Saving.\r\n");
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy.
     */
    if (CONFIG_AUTO_SAVE && char_get_admlevel(ch) < 1) {
      write_aliases(g, ch);
      save_char(g, ch);
      Crash_crashsave(g, ch);
      remember_load_room(g, ch);
      return;
    }
  }
  write_aliases(g, ch);
  remember_load_room(g, ch);
  save_char(g, ch);
  Crash_crashsave(g, ch);
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(g, ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_practice)
{
  std::string_view rest = argument;
  if (!take_arg(rest).empty())
    send_to_char(g, ch, "You can only practice skills with your trainer.\r\n");
  else
    send_to_char(g, ch, "Use the skills command unless you are at your trainer.\r\n");
}

ACMD(do_skills)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (!char_is_npc(ch))
    list_skills(g, ch, arg);
}

ACMD(do_visible)
{
  if (char_get_admlevel(ch))
    perform_immort_vis(g, ch);
  else if (char_aff_flagged(ch, AFF_INVISIBLE)) {
    appear(g, ch);
    send_to_char(g, ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(g, ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  std::string text(skip_spaces(argument));
  delete_doubledollar(text);

  if (char_is_npc(ch))
    send_to_char(g, ch, "Your title is fine... go away.\r\n");
  else if (char_plr_flagged(ch, PLR_NOTITLE))
    send_to_char(g, ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (text.contains("(") || text.contains(")"))
    send_to_char(g, ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (text.size() > MAX_TITLE_LENGTH)
    send_to_char(g, ch, "Sorry, titles can't be longer than {:d} characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(g, ch, text.c_str());
    send_to_char(g, ch, "Okay, you're now {} {}.\r\n", char_get_name(ch), char_get_title(ch));
  }
}

namespace {
  /* vict joins ch's group, if they will */
  bool perform_group(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (char_aff_flagged(vict, AFF_GROUP) || !char_can_see(g, ch, vict))
      return false;
    if (char_get_bonuses(vict, BONUS_LONER) > 0) {
      act(g, "$n is the loner type and refuses to be in your group.", true, vict, nullptr, ch, TO_VICT);
      return false;
    }
    char_aff_set(vict, AFF_GROUP, true);
    act_triple(g, {ch != vict ? "$N is now a member of your group." : nullptr, "You are now a member of $n's group.", "$N is now a member of $n's group."}, ch, nullptr, vict);
    return true;
  }

  void kicked_out(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    act_triple(g, {ch != vict ? "$N is no longer a member of your group." : nullptr, "You have been kicked out of $n's group!", "$N has been kicked out of $n's group!"}, ch, nullptr, vict);
  }

  /* A member's line in the group list: L for the leader, F for a follower; red powerlevel when they're low */
  std::string group_line(GameWorld *g, struct char_data *k, char role, bool low)
  {
    return str_format("@g{}@D: @w$N @W- @D[@RPL@Y: @{}{} @CKi@Y: @c{} @GST@Y: @c{}@D] [@w{:>2d} {} {}@D]{}", role, low ? 'r' : 'c', add_commas(getCurPL(g, k)), add_commas(getCurKI(g, k)),
                      add_commas(getCurST(g, k)), char_get_level(k), char_class_abbr(k), char_race_abbr(k), role == 'L' ? "@n" : "");
  }

  void print_group(GameWorld *g, struct char_data *ch)
  {
    if (!char_aff_flagged(ch, AFF_GROUP)) {
      send_to_char(g, ch, "But you are not the member of a group!\r\n");
      return;
    }
    send_to_char(g, ch, "Your group consists of:\r\n");
    struct char_data *k = char_get_master(g, ch) ? char_get_master(g, ch) : ch;
    /* quirk: the leader is measured two ways, so between them gets no line, and when both hold, the red one */
    std::string buf;
    if (char_aff_flagged(k, AFF_GROUP)) {
      send_to_char(g, ch, "@D----------------@n\r\n");
      if (getCurPL(g, k) > getEffMaxPL(g, k) / 10)
        buf = group_line(g, k, 'L', false);
      if (getCurPL(g, k) <= (getEffMaxPL(g, k) - getCurCarriedWeight(k)) / 10)
        buf = group_line(g, k, 'L', true);
      act(g, buf.c_str(), false, ch, nullptr, k, TO_CHAR);
    }
    for (char_data *fol : char_followers(g, k)) {
      if (!char_aff_flagged(fol, AFF_GROUP))
        continue;
      send_to_char(g, ch, "@D----------------@n\r\n");
      bool low = getCurPL(g, fol) <= (getEffMaxPL(g, fol) - getCurCarriedWeight(fol)) / 10;
      buf      = group_line(g, fol, 'F', low);
      act(g, buf.c_str(), false, ch, nullptr, fol, TO_CHAR);
    }
    send_to_char(g, ch, "@D----------------@n\r\n");
  }
} // namespace

ACMD(do_group)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (char_get_bonuses(ch, BONUS_LONER) > 0) {
    send_to_char(g, ch, "You can not group as you prefer to be alone.\r\n");
    return;
  }
  if (buf.empty()) {
    print_group(g, ch);
    return;
  }
  if (char_get_master(g, ch)) {
    act(g, "You cannot enroll group members without being head of a group.", false, ch, nullptr, nullptr, TO_CHAR);
    return;
  }
  if (str_eq_nocase(buf, "all")) {
    perform_group(g, ch, ch);
    int found = 0;
    for (char_data *foll : char_followers(g, ch))
      found += perform_group(g, ch, foll);
    if (!found)
      send_to_char(g, ch, "Everyone following you is already in your group.\r\n");
    return;
  }
  struct char_data *vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_ROOM);
  if (!vict)
    send_to_char(g, ch, CONFIG_NOPERSON);
  else if (char_get_master(g, vict) != ch && vict != ch)
    act(g, "$N must follow you to enter your group.", false, ch, nullptr, vict, TO_CHAR);
  else if (char_aff_flagged(vict, AFF_GROUP)) {
    kicked_out(g, ch, vict);
    char_aff_set(vict, AFF_GROUP, false);
  } else {
    if (!char_aff_flagged(ch, AFF_GROUP)) {
      send_to_char(g, ch, "You form a group, with you as leader.\r\n");
      char_aff_set(ch, AFF_GROUP, true);
    }
    perform_group(g, ch, vict);
  }
}

ACMD(do_ungroup)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (buf.empty()) {
    if (char_get_master(g, ch) || !char_aff_flagged(ch, AFF_GROUP)) {
      send_to_char(g, ch, "But you lead no group!\r\n");
      return;
    }
    for (char_data *fol : char_followers(g, ch)) {
      if (!char_aff_flagged(fol, AFF_GROUP))
        continue;
      char_aff_set(fol, AFF_GROUP, false);
      act(g, "$N has disbanded the group.", true, fol, nullptr, ch, TO_CHAR);
      char_set_combatexpertise(fol, 0);
      if (!char_aff_flagged(fol, AFF_CHARM))
        stop_follower(g, fol);
    }
    char_aff_set(ch, AFF_GROUP, false);
    char_set_combatexpertise(ch, 0);
    send_to_char(g, ch, "You disband the group.\r\n");
    return;
  }
  auto found = find_char(g, ch, buf, FIND_CHAR_ROOM, "There is no such person!\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *tch = *found;
  if (char_get_master(g, tch) != ch) {
    send_to_char(g, ch, "That person is not following you!\r\n");
    return;
  }
  if (!char_aff_flagged(tch, AFF_GROUP)) {
    send_to_char(g, ch, "That person isn't in your group.\r\n");
    return;
  }
  char_aff_set(tch, AFF_GROUP, false);
  char_set_combatexpertise(tch, 0);
  kicked_out(g, ch, tch);
  if (!char_aff_flagged(tch, AFF_CHARM))
    stop_follower(g, tch);
}

ACMD(do_split)
{
  if (char_is_npc(ch))
    return;
  std::string_view args = argument;
  std::string buf       = take_arg(args);
  if (!is_number(buf)) {
    send_to_char(g, ch, "How much zenni do you wish to split with your group?\r\n");
    return;
  }
  int amount = parse_int(buf);
  if (amount <= 0) {
    send_to_char(g, ch, "Sorry, you can't do that.\r\n");
    return;
  }
  if (amount > char_get_gold(ch)) {
    send_to_char(g, ch, "You don't seem to have that much gold to split.\r\n");
    return;
  }
  char_set_gold(ch, char_get_gold(ch) - amount); /* quirk: before knowing there's anyone to share it with, so it's lost if not */

  /* Everyone in the group here shares: the leader, and the players following them (but ch, counted with the leader) */
  struct char_data *k = char_get_master(g, ch) ? char_get_master(g, ch) : ch;
  auto shares_here    = [&](struct char_data *fol) { return char_aff_flagged(fol, AFF_GROUP) && !char_is_npc(fol) && fol != ch && char_get_in_room(fol) == char_get_in_room(ch); };
  int num             = (char_aff_flagged(k, AFF_GROUP) && char_get_in_room(k) == char_get_in_room(ch) ? 1 : 0) + std::ranges::count_if(char_followers(g, k), shares_here);
  if (num <= 0 || !char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(g, ch, "With whom do you wish to share your gold?\r\n");
    return;
  }
  int share = amount / num, rest = amount % num;
  char_set_gold(ch, char_get_gold(ch) + share);

  std::string msg = str_format("{} splits {:d} zenni; you receive {:d}.\r\n", char_get_name(ch), amount, share);
  if (rest)
    msg += str_format("{:d} zenni {} not splitable, so {} keeps the money.\r\n", rest, rest == 1 ? "was" : "were", char_get_name(ch));
  if (char_aff_flagged(k, AFF_GROUP) && char_get_in_room(k) == char_get_in_room(ch) && !char_is_npc(k) && k != ch) {
    char_set_gold(k, char_get_gold(k) + share);
    send_to_char(g, k, "{}", msg);
  }
  for (char_data *foll : char_followers(g, k)) {
    if (!shares_here(foll))
      continue;
    char_set_gold(foll, char_get_gold(foll) + share);
    send_to_char(g, foll, "{}", msg);
  }
  send_to_char(g, ch, "You split {:d} zenni among {:d} members -- {:d} zenni each.\r\n", amount, num, share);
  if (rest) {
    send_to_char(g, ch, "{:d} zenni {} not splitable, so you keep the money.\r\n", rest, rest == 1 ? "was" : "were");
    char_set_gold(ch, char_get_gold(ch) + rest);
  }
}

namespace {
  /* What each letter of prompt shows, in the order all/on and none/off set them */
  struct prompt_part {
    char letter;
    int flag;
  };
  constexpr prompt_part PROMPT_PARTS[] = {{'p', PRF_DISPHP}, {'s', PRF_DISPMOVE}, {'k', PRF_DISPKI}, {'t', PRF_DISPTNL}, {'f', PRF_FURY}, {'h', PRF_DISTIME}, {'g', PRF_DISGOLD}, {'l', PRF_DISPRAC}, {'m', PRF_DISHUTH}, {'c', PRF_DISPERC}};
} // namespace

ACMD(do_display)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "Mosters don't need displays.  Go away.\r\n");
    return;
  }
  argument = skip_spaces(argument);
  if (argument.empty()) {
    send_to_char(g, ch, "Usage: prompt { P | K | T | S | F | H | G | L | C | M | all/on | none/off }\r\n");
    return;
  }
  bool all = str_eq_nocase(argument, "on") || str_eq_nocase(argument, "all");
  if (all || str_eq_nocase(argument, "off") || str_eq_nocase(argument, "none")) {
    for (const auto &part : PROMPT_PARTS)
      char_prf_set(g, ch, part.flag, all);
  } else
    for (char c : argument) {
      auto part = std::ranges::find(PROMPT_PARTS, (char)tolower(c), &prompt_part::letter);
      if (part == std::end(PROMPT_PARTS)) {
        send_to_char(g, ch, "Usage: prompt { P | K | T | S | F | H | G | L | all/on | none/off }\r\n");
        return;
      }
      if (part->flag == PRF_FURY && !char_is_halfbreed(ch))
        send_to_char(g, ch, "Only halfbreeds use fury.\r\n");
      char_prf_toggle(g, ch, part->flag);
    }
  send_to_char(g, ch, CONFIG_OK);
}

namespace {
  /* Where bug, typo and idea (report) are filed */
  const char *filed_in(int subcmd)
  {
    switch (subcmd) {
      case SCMD_BUG:
        return BUG_FILE;
      case SCMD_TYPO:
        return TYPO_FILE;
      case SCMD_IDEA:
        return IDEA_FILE;
    }
    return nullptr;
  }
} // namespace

ACMD(do_gen_write)
{
  const char *filename = filed_in(subcmd);
  if (!filename)
    return;
  time_t ct = game_now(g);
  char *tmp = asctime(localtime(&ct));
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }
  std::string text(skip_spaces(argument));
  delete_doubledollar(text);
  if (text.empty()) {
    send_to_char(g, ch, "That must be a mistake...\r\n");
    return;
  }
  send_to_imm(g, "[A new {} has been filed by: {}]\r\n", CMD_NAME, char_get_name(ch));

  struct stat fbuf;
  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
    send_to_char(g, ch, "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  FILE *fl = fopen(filename, "a");
  if (!fl) {
    perror("SYSERR: do_gen_write");
    send_to_char(g, ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  file_print(fl, "@D[@WUser: @c{:<10}@D] [@WChar: @C{:<10}@D] [@WRoom: @G{:<4d}@D] [@WDate: @Y{:>6.6}@D]@b \n-----------@w\n{}\n", char_get_user(ch) ? char_get_user(ch) : "ERR", char_get_name(ch), char_get_in_room(ch), (tmp + 4), text);
  file_print(fl, "@D-------------------------------@n\n");
  fclose(fl);
  send_to_char(g, ch, "Okay.  Thanks!\r\n");
}


namespace {
  /* A toggle flips one flag and reports the new state.  `before` may refuse
   * (it sends why and returns true); `after` adds to what was said. */
  struct gen_toggle {
    int subcmd;
    bool (*flip)(GameWorld *g, struct char_data *ch);
    const char *off;
    const char *on;
    bool (*before)(GameWorld *g, struct char_data *ch) = nullptr;
    void (*after)(GameWorld *g, struct char_data *ch)  = nullptr;
  };

  template <int Flag>
  bool prf(GameWorld *g, struct char_data *ch) { return char_prf_toggle(g, ch, Flag); }

  template <int Flag>
  bool aff(GameWorld *, struct char_data *ch) { return char_aff_toggle(ch, Flag); }

  bool slow_nameserver(GameWorld *g, struct char_data *) { return (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW); }
  bool track_doors(GameWorld *g, struct char_data *) { return (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS); }

  void afk_announce(GameWorld *g, struct char_data *ch)
  {
    if (char_prf_flagged(g, ch, PRF_AFK)) {
      act(g, "$n has gone AFK.", true, ch, nullptr, nullptr, TO_ROOM);
      return;
    }
    act(g, "$n has come back from AFK.", true, ch, nullptr, nullptr, TO_ROOM);
    if (has_mail(g, char_get_idnum(ch)))
      send_to_char(g, ch, "You have mail waiting.\r\n");
  }

  bool immortals_only(GameWorld *g, struct char_data *ch)
  {
    if (char_get_admlevel(ch) >= ADMLVL_IMMORT)
      return false;
    send_to_char(g, ch, "Immortals only, sorry.\r\n");
    return true;
  }

  void buildwalk_log(GameWorld *g, struct char_data *ch)
  {
    mudlog(g, LOG_CMP, char_get_level(ch), true, "OLC: {} turned buildwalk {}. Allowed zone {:d}",
           char_get_name(ch), char_prf_flagged(g, ch, PRF_BUILDWALK) ? "on" : "off", char_get_olc_zone(g, ch));
  }

  bool cannot_hide(GameWorld *g, struct char_data *ch)
  {
    bool ki_user = char_get_preference(ch) == PREFERENCE_KI;
    if ((char_get_charge(ch) > 0 && !ki_user) || (char_get_charge(ch) > getMaxKI(g, ch) * 0.1 && ki_user) ||
        char_plr_flagged(ch, PLR_POWERUP) || char_aff_flagged(ch, AFF_FLYING)) {
      send_to_char(g, ch, "You stand out too much to hide right now!\r\n");
      return true;
    }
    if (char_plr_flagged(ch, PLR_HEALT)) {
      send_to_char(g, ch, "You are inside a healing tank!\r\n");
      return true;
    }
    if (char_get_skill(ch, SKILL_HIDE))
      return false;
    if (slot_count(ch) + 1 > char_get_skill_slots(ch)) {
      send_to_char(g, ch, "@RYou need more skill slots in order to learn this skill.@n\r\n");
      return true;
    }
    send_to_char(g, ch, "@GYou learn the very minimal basics to hiding.@n\r\n");
    char_set_skills_base(ch, SKILL_HIDE, rand_number(g, 1, 5));
    return false;
  }

  bool compression_disabled(GameWorld *g, struct char_data *ch)
  {
    if (CONFIG_ENABLE_COMPRESSION)
      return false;
    send_to_char(g, ch, "Sorry, compression is globally disabled.\r\n");
    return true;
  }

  /* Testing mode reports itself, so it never reaches the table's messages. */
  bool test_mode(GameWorld *g, struct char_data *ch)
  {
    if (char_get_admlevel(ch) < 1) {
      send_to_char(g, ch, "You are not an immortal.\r\n");
      return true;
    }
    char_prf_toggle(g, ch, PRF_TEST);
    send_to_char(g, ch, "Okay. Testing is now: {}\r\n", char_prf_flagged(g, ch, PRF_TEST) ? "On" : "Off");
    if (char_prf_flagged(g, ch, PRF_TEST))
      send_to_char(g, ch, "Make sure to remove nohassle as well.\r\n");
    return true;
  }

  constexpr gen_toggle GEN_TOGGLES[] = {
      {SCMD_NOSUMMON, prf<PRF_SUMMONABLE>, "You are now safe from summoning by other players.\r\n", "You may now be summoned by other players.\r\n"},
      {SCMD_NOHASSLE, prf<PRF_NOHASSLE>, "Nohassle disabled.\r\n", "Nohassle enabled.\r\n"},
      {SCMD_BRIEF, prf<PRF_BRIEF>, "Brief mode off.\r\n", "Brief mode on.\r\n"},
      {SCMD_COMPACT, prf<PRF_COMPACT>, "Compact mode off.\r\n", "Compact mode on.\r\n"},
      {SCMD_NOTELL, prf<PRF_NOTELL>, "You can now hear tells.\r\n", "You are now deaf to tells.\r\n"},
      {SCMD_NOAUCTION, prf<PRF_NOAUCT>, "You can now hear newbie.\r\n", "You are now deaf to newbie.\r\n"},
      {SCMD_DEAF, prf<PRF_DEAF>, "You can now hear shouts.\r\n", "You are now deaf to shouts.\r\n"},
      {SCMD_NOGOSSIP, prf<PRF_NOGOSS>, "You can now hear ooc.\r\n", "You are now deaf to ooc.\r\n"},
      {SCMD_NOGRATZ, prf<PRF_NOGRATZ>, "You can now hear the congratulation messages.\r\n", "You are now deaf to the congratulation messages.\r\n"},
      {SCMD_NOWIZ, prf<PRF_NOWIZ>, "You can now hear the Wiz-channel.\r\n", "You are now deaf to the Wiz-channel.\r\n"},
      {SCMD_QUEST, prf<PRF_QUEST>, "You are no longer part of the Quest.\r\n", "Okay, you are part of the Quest!\r\n"},
      {SCMD_ROOMFLAGS, prf<PRF_ROOMFLAGS>, "You will no longer see the room flags.\r\n", "You will now see the room flags.\r\n"},
      {SCMD_NOREPEAT, prf<PRF_NOREPEAT>, "You will now have your communication repeated.\r\n", "You will no longer have your communication repeated.\r\n"},
      {SCMD_HOLYLIGHT, prf<PRF_HOLYLIGHT>, "HolyLight mode off.\r\n", "HolyLight mode on.\r\n"},
      {SCMD_SLOWNS, slow_nameserver, "Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
       "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
      {SCMD_AUTOEXIT, prf<PRF_AUTOEXIT>, "Autoexits disabled.\r\n", "Autoexits enabled.\r\n"},
      {SCMD_TRACK, track_doors, "Will no longer track through doors.\r\n", "Will now track through doors.\r\n"},
      {SCMD_BUILDWALK, prf<PRF_BUILDWALK>, "Buildwalk Off.\r\n", "Buildwalk On.\r\n", immortals_only, buildwalk_log},
      {SCMD_AFK, prf<PRF_AFK>, "AFK flag is now off.\r\n", "AFK flag is now on.\r\n", nullptr, afk_announce},
      {SCMD_AUTOASSIST, prf<PRF_AUTOASSIST>, "You will no longer Auto-Assist.\r\n", "You will now Auto-Assist.\r\n"},
      {SCMD_AUTOLOOT, prf<PRF_AUTOLOOT>, "Autoloot disabled.\r\n", "Autoloot enabled.\r\n"},
      {SCMD_AUTOGOLD, prf<PRF_AUTOGOLD>, "Autogold disabled.\r\n", "Autogold enabled.\r\n"},
      {SCMD_CLS, prf<PRF_CLS>, "Will no longer clear screen in OLC.\r\n", "Will now clear screen in OLC.\r\n"},
      {SCMD_AUTOSPLIT, prf<PRF_AUTOSPLIT>, "Autosplit disabled.\r\n", "Autosplit enabled.\r\n"},
      {SCMD_AUTOSAC, prf<PRF_AUTOSAC>, "Autosac disabled.\r\n", "Autosac enabled.\r\n"},
      {SCMD_SNEAK, aff<AFF_SNEAK>, "You will no longer attempt to be sneaky.\r\n", "You will try to move as silently as you can.\r\n"},
      {SCMD_HIDE, aff<AFF_HIDE>, "You will no longer attempt to stay hidden.\r\n", "You will try to stay hidden.\r\n", cannot_hide},
      {SCMD_AUTOMEM, prf<PRF_AUTOMEM>, "You will no longer automatically memorize spells in your list.\r\n",
       "You will automatically memorize spells in your list.\r\n"},
      {SCMD_VIEWORDER, prf<PRF_VIEWORDER>, "Viewing newest board messages at top of list.\r\n", "Viewing newest board messages at bottom of list.\r\n"},
      {SCMD_NOCOMPRESS, prf<PRF_NOCOMPRESS>, "Compression will be used if your client supports it.\r\n",
       "Compression will not be used even if your client supports it.\r\n", compression_disabled},
      {SCMD_TEST, nullptr, "", "", test_mode},
      {SCMD_WHOHIDE, prf<PRF_HIDE>, "You are no longer hidden from view on the who list and public channels.\r\n",
       "You are now hidden from view on the who list and public channels.\r\n"},
      {SCMD_NMWARN, prf<PRF_NMWARN>, "You will now be told that you have mail on prompt.\r\n", "You will no longer be told that you have mail on prompt.\r\n"},
      {SCMD_HINTS, prf<PRF_HINTS>, "You will no longer receive automatic hints.\r\n", "You will now receive automatic hints.\r\n"},
      {SCMD_NODEC, prf<PRF_NODEC>, "Screen Reader Friendly Mode Deactivated.\r\n", "Screen Reader Friendly Mode Activated..\r\n"},
      {SCMD_NOEQSEE, prf<PRF_NOEQSEE>, "You will now see equipment when looking at someone.\r\n", "You will no longer see equipment when looking at someone.\r\n"},
      {SCMD_NOMUSIC, prf<PRF_NOMUSIC>, "You will now listen to the music channel.\r\n", "You will no longer listen to the music channel.\r\n"},
      {SCMD_NOPARRY, prf<PRF_NOPARRY>, "You will now parry attacks.\r\n", "You will no longer parry attacks.\r\n"},
      {SCMD_LKEEP, prf<PRF_LKEEP>, "You will no longer keep cybernetic limbs with death.\r\n", "You will now keep cybernetic limbs with death.\r\n"},
      {SCMD_CARVE, prf<PRF_CARVE>, "You will no longer worry about acquiring steaks from animals.\r\n", "You will now acquire steaks from animal if you can.\r\n"},
      {SCMD_NOGIVE, prf<PRF_NOGIVE>, "You will now accept things being given to you.\r\n", "You will no longer accept things being given to you.\r\n"},
      {SCMD_INSTRUCT, prf<PRF_INSTRUCT>, "You will no longer instruct those you spar with.\r\n", "You will now instruct those you spar with.\r\n"},
      {SCMD_GHEALTH, prf<PRF_GHEALTH>, "You will no longer view group health.\r\n", "You will now view group health.\r\n"},
      {SCMD_IHEALTH, prf<PRF_IHEALTH>, "You will no longer view item health.\r\n", "You will now view item health.\r\n"},
  };

  static_assert([] {
    for (int i = 0; i < std::ssize(GEN_TOGGLES); ++i)
      if (GEN_TOGGLES[i].subcmd != i)
        return false;
    return true;
  }(), "GEN_TOGGLES is indexed by subcmd");
} // namespace

ACMD(do_gen_tog)
{
  if (char_is_npc(ch))
    return;

  if (subcmd < 0 || subcmd >= std::ssize(GEN_TOGGLES)) {
    mud_log("SYSERR: Unknown subcmd {:d} in do_gen_toggle.", subcmd);
    return;
  }
  const gen_toggle &toggle = GEN_TOGGLES[subcmd];

  if (toggle.before && toggle.before(g, ch))
    return;
  bool on = toggle.flip(g, ch);
  if (toggle.after)
    toggle.after(g, ch);
  send_to_char(g, ch, "{}", on ? toggle.on : toggle.off);
}

namespace {
  /* What file can show, to immortals of at least the level */
  constexpr struct {
    const char *cmd;
    int level;
    const char *file;
  } FILES[] = {
      {"none", 6, "Does Nothing"},
      {"bug", ADMLVL_IMMORT, "../data/misc/bugs"},
      {"typo", ADMLVL_IMMORT, "../data/misc/typos"},
      {"report", ADMLVL_IMMORT, "../data/misc/ideas"},
      {"xnames", 4, "../data/misc/xnames"},
      {"levels", 4, "../log/levels"},
      {"rip", 4, "../log/rip"},
      {"players", 4, "../log/newplayers"},
      {"rentgone", 4, "../log/rentgone"},
      {"errors", 4, "../log/errors"},
      {"godcmds", 4, "../log/godcmds"},
      {"syslog", ADMLVL_IMMORT, "../syslog"},
      {"crash", ADMLVL_IMMORT, "../syslog.CRASH"},
      {"immlog", ADMLVL_IMMORT, "../data/misc/request"},
      {"customs", ADMLVL_IMMORT, "../data/misc/customs"},
      {"todo", 5, "../todo"},
  };
} // namespace

ACMD(do_file)
{
  argument = skip_spaces(argument);
  if (argument.empty()) {
    std::string buf = "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n";
    for (const auto &f : FILES | std::views::drop(1)) /* quirk: listed by level, not admin level */
      if (f.level <= char_get_level(ch))
        buf += str_format("{:<15}{}\r\n", f.cmd, f.file);
    send_to_char(g, ch, buf);
    return;
  }
  std::string_view rest = argument;
  std::string field     = take_arg(rest);
  std::string value     = take_arg(rest);

  auto f = std::ranges::find_if(FILES, [&](const auto &each) { return std::string_view(each.cmd).starts_with(field); });
  if (f == std::end(FILES)) {
    send_to_char(g, ch, "That is not a valid option!\r\n");
    return;
  }
  if (char_get_admlevel(ch) < f->level) {
    send_to_char(g, ch, "You are not godly enough to view that file!\r\n");
    return;
  }
  int req_lines = value.empty() ? 15 : parse_int(value); /* default is the last 15 lines */

  FILE *req_file = fopen(f->file, "r");
  if (!req_file) {
    mudlog(g, LOG_BRF, ADMLVL_IMPL, true, "SYSERR: Error opening file {} using 'file' command.", f->file);
    return;
  }
  char line[READ_SIZE];
  int num_lines = 0;
  get_line(req_file, line);
  while (!feof(req_file)) {
    num_lines++;
    get_line(req_file, line);
  }
  rewind(req_file);
  req_lines = std::min({req_lines, num_lines, 5000});

  std::string buf;
  int cur_line = 0;
  get_line(req_file, line);
  while (!feof(req_file)) {
    if (++cur_line > num_lines - req_lines)
      buf += str_format("{}\r\n", line);
    get_line(req_file, line);
  }
  fclose(req_file);
  send_to_char(g, ch, buf);
}
