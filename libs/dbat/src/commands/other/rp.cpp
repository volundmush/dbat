/* rp.c: roleplay points, immortal action logs, hints, poses
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <cstdlib>
#include <iterator>

#include <sys/stat.h>

#include "dbat/args.hpp"
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
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

// definitions
void log_imm_action(GameWorld *g, std::string_view text)
{

  FILE *fl;
  const char *filename;
  struct stat fbuf;

  filename = REQUEST_FILE;

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: log_imm_action");
    /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

    return;
  }
  time_t ct    = game_now(g);
  char *time_s = asctime(localtime(&ct));

  time_s[strlen(time_s) - 1] = '\0';

  file_print(fl, "{:<15.15} :: ", time_s + 4);
  fwrite(text.data(), 1, text.size(), fl);
  file_print(fl, "\n");

  fclose(fl);
}

void log_custom(GameWorld *g, struct descriptor_data *d, struct obj_data *obj)
{
  FILE *fl;
  const char *filename;
  struct stat fbuf;

  filename = CUSTOM_FILE;

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: log_custom");
    /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

    return;
  }

  file_print(fl, "@D[@cUser@W: @R{:<20} @cName@W: @C{:<20} @cCustom@W: @Y{}@D]\n", char_get_user(d->character), char_get_name(d->character), obj_get_short_description(obj));
  fclose(fl);
}

namespace {
  const char *const SHORT_OF_RPP = "You do not have enough RPP for that selection.\r\n";
  /* quirk: the stat and aura choices say this too */
  const char *const NOT_ACCEPTABLE = "That is not an acceptable option for changing alignment.\r\n";

  /* Sends `why` and returns true when ch can't pay. */
  bool short_of(GameWorld *g, struct char_data *ch, int pay, const char *why = SHORT_OF_RPP)
  {
    if (char_get_rp(ch) >= pay)
      return false;
    send_to_char(g, ch, "{}", why);
    return true;
  }

  void pay_rpp(GameWorld *g, struct char_data *ch, int pay)
  {
    char_set_rp(ch, char_get_rp(ch) - pay);
    char_get_desc(ch)->rpp = char_get_rp(ch);
    userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
    save_char(g, ch);
    send_to_char(g, ch, "@R{:d}@W RPP paid for your selection. Enjoy!@n\r\n", pay);
    send_to_imm(g, "RPP Purchase: {} {:d}", char_get_name(ch), pay);
  }

  int experience_price(struct char_data *ch) { return std::max(1, 1 + char_get_level(ch) / 40); }

  void show_rewards(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "@C                             Rewards Menu\n");
    send_to_char(g, ch, "@b  ------------------------------------------------------------------\n");
    send_to_char(g, ch, "  @C1@D)@R Disabled            @D[@G -- RPP @D]  @C2@D)@R Disabled              @D[@G -- RPP @D]@n\n");
    send_to_char(g, ch, "  @C3@D)@c Custom Equipment    @D[@G 20 RPP @D]  @C4@D)@c Alignment Change      @D[@G 20 RPP @D]\n");
    send_to_char(g, ch, "  @C5@D)@c 7,500 zenni         @D[@G  1 RPP @D]  @C6@D)@c +2 To A Stat          @D[@G  2 RPP @D]\n");
    send_to_char(g, ch, "  @C7@D)@c +750 PS             @D[@G  4 RPP @D]  @C8@D)@R Disabled               @D[@G -- RPP @D]\n");
    send_to_char(g, ch, "  @C9@D)@c 50{} TNL Exp         @D[@G{:>3d} RPP @D] @C10@D)@c Aura Change           @D[@G  2 RPP @D]\n", "%", experience_price(ch));
    send_to_char(g, ch, " @C11@D)@c Reach Softcap       @D[@G  5 RPP @D] @C12@D)@c RPP Store             @D[@G ?? RPP @D]\n");
    send_to_char(g, ch, " @C13@D)@c Extra Feature       @D[@G  1 RPP @D] @C14@D)@c Restring Equipment    @D[@G  1 RPP @D]\n");
    send_to_char(g, ch, " @C15@D)@c Extra Skillslot     @D[@G  3 RPP @D] @C16@D)@R Disabled              @D[@G -- RPP @D]@n\n");
    send_to_char(g, ch, "@b  ------------------------------------------------------------------@n\n");
    send_to_char(g, ch, "@D                           [@YYour RPP@D:@G {:>3d}@D]@n\n", char_get_rp(ch));
    send_to_char(g, ch, "\nSyntax: rpp (num)\n");
  }

  /* A reward does what it can and returns what it costs: 0 when it charged nothing. */
  using reward = int (*)(GameWorld *g, struct char_data *ch, std::string &arg);

  int disabled(GameWorld *, struct char_data *, std::string &) { return 0; }

  /* quirk: the menu calls this one disabled */
  int house_capsule(GameWorld *g, struct char_data *ch, std::string &)
  {
    if (short_of(g, ch, 6500, "Nice try but you don't have enough RPP for that.\r\n"))
      return 0;
    send_to_char(g, ch, "You now have an Excel House Capsule!\r\n");
    obj_to_char(g, read_object(g, 6), ch);
    return 6500;
  }

  int custom_equipment(GameWorld *g, struct char_data *ch, std::string &)
  {
    if (short_of(g, ch, 20, "You need at least 20 RPP to initiate a custom equipment build.\r\n"))
      return 0;
    struct descriptor_data *d = char_get_desc(ch);
    desc_set_connected(d, CON_POBJ);
    d->obj_name   = "Generic Armor Vest";
    d->obj_short  = "@cGeneric @DArmor @WVest@n";
    d->obj_long   = "@wA @cgeneric @Darmor @Wvest@w is lying here@n";
    d->obj_type   = 1;
    d->obj_weapon = 0;
    disp_custom_menu(g, d);
    d->obj_editflag = EDIT_CUSTOM;
    d->obj_editval  = EDIT_CUSTOM_MAIN;
    return 0;
  }

  struct alignment_choice {
    const char *word;
    const char *says;
    int value;
  };

  constexpr alignment_choice ALIGNMENTS[] = {
      {"evil",       "Evil",       -750},
      {"sorta-evil", "Sorta Evil", -50 },
      {"neutral",    "Neutral",    0   },
      {"sorta-good", "Sorta Good", 51  },
      {"good",       "Good",       300 },
  };

  int alignment(GameWorld *g, struct char_data *ch, std::string &arg)
  {
    if (short_of(g, ch, 20))
      return 0;
    if (arg.empty()) {
      send_to_char(g, ch, "What do you want to change your alignment to? (evil, sorta-evil, neutral, sorta-good, good)");
      return 0;
    }
    auto it = std::ranges::find_if(ALIGNMENTS, [&](const alignment_choice &a) { return str_eq_nocase(arg, a.word); });
    if (it == std::end(ALIGNMENTS)) {
      send_to_char(g, ch, "{}", NOT_ACCEPTABLE);
      return 0;
    }
    send_to_char(g, ch, "You change your alignment to {}.\r\n", it->says);
    char_set_alignment(ch, it->value);
    return 20;
  }

  int zenni(GameWorld *g, struct char_data *ch, std::string &)
  {
    if (short_of(g, ch, 1))
      return 0;
    char_set_bank_gold(ch, char_get_bank_gold(ch) + 7500);
    send_to_char(g, ch, "Your bank zenni has been increased by 7,500\r\n");
    return 1;
  }

  struct raised_stat {
    const char *word;
    const char *name;
    int negative; /* the trait that caps it at 70 */
    int8_t (*real)(const struct char_data *ch);
    void (*set_real)(struct char_data *ch, int8_t v);
    int8_t (*current)(const struct char_data *ch);
  };

  constexpr raised_stat RAISED_STATS[] = {
      {"str", "strength",     BONUS_WIMP,    char_get_real_abils_str,   char_set_real_abils_str,   char_get_aff_abils_str  },
      {"con", "constitution", BONUS_FRAIL,   char_get_real_abils_con,   char_set_real_abils_con,   char_get_aff_abils_con  },
      {"int", "intelligence", BONUS_DULL,    char_get_real_abils_intel, char_set_real_abils_intel, char_get_aff_abils_intel},
      {"wis", "wisdom",       BONUS_FOOLISH, char_get_real_abils_wis,   char_set_real_abils_wis,   char_get_aff_abils_wis  },
      {"spd", "speed",        BONUS_SLOW,    char_get_real_abils_cha,   char_set_real_abils_cha,   char_get_aff_abils_cha  },
      {"agl", "speed" /* quirk */, BONUS_CLUMSY, char_get_real_abils_dex, char_set_real_abils_dex, char_get_aff_abils_dex  },
  };

  int raise_stat(GameWorld *g, struct char_data *ch, std::string &arg)
  {
    if (short_of(g, ch, 2))
      return 0;
    if (arg.empty()) {
      send_to_char(g, ch, "What stat? (str, con, int, wis, spd, agl)");
      return 0;
    }
    auto it = std::ranges::find_if(RAISED_STATS, [&](const raised_stat &s) { return str_eq_nocase(arg, s.word); });
    if (it == std::end(RAISED_STATS)) {
      send_to_char(g, ch, "{}", NOT_ACCEPTABLE);
      return 0;
    }
    if (char_get_bonuses(ch, it->negative) > 0 && it->real(ch) >= 70) {
      send_to_char(g, ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
      return 0;
    }
    if (it->current(ch) >= 100) {
      send_to_char(g, ch, "100 is the maximum for any stat.\r\n");
      return 0;
    }
    send_to_char(g, ch, "You increase your {} by 2.\r\n", it->name);
    it->set_real(ch, it->real(ch) + 2);
    return 2;
  }

  int practice(GameWorld *g, struct char_data *ch, std::string &)
  {
    if (short_of(g, ch, 4))
      return 0;
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) + 750);
    send_to_char(g, ch, "Your practices have been increased by 750\r\n");
    return 4;
  }

  int experience(GameWorld *g, struct char_data *ch, std::string &)
  {
    int pay = experience_price(ch);
    if (short_of(g, ch, pay))
      return 0;
    if (char_get_level(ch) >= 100) {
      send_to_char(g, ch, "You can not buy experience anymore at your level. I think you know why.\r\n");
      return 0;
    }
    if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) < 0) {
      send_to_char(g, ch, "You can not buy experience anymore UNTIL you level.\r\n");
      return 0;
    }
    char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * .52));
    send_to_char(g, ch, "You gained 50{} of the entire experience needed for your next level.\r\n", "%");
    return pay;
  }

  /* An aura's number is its place here. */
  constexpr const char *AURAS[] = {"white", "blue", "red", "green", "pink", "purple", "yellow", "black", "orange"};

  int aura(GameWorld *g, struct char_data *ch, std::string &arg)
  {
    if (short_of(g, ch, 2))
      return 0;
    if (arg.empty()) {
      send_to_char(g, ch, "Change your aura to what? (white, blue, red, green, pink, purple, yellow, black, orange)");
      return 0;
    }
    auto it = std::ranges::find_if(AURAS, [&](const char *name) { return str_eq_nocase(arg, name); });
    if (it == std::end(AURAS)) {
      send_to_char(g, ch, "{}", NOT_ACCEPTABLE);
      return 0;
    }
    char_set_aura(ch, static_cast<int>(it - std::begin(AURAS)));
    send_to_char(g, ch, "You change your aura to {}.\r\n", *it);
    return 2;
  }

  int softcap(GameWorld *g, struct char_data *ch, std::string &)
  {
    if (short_of(g, ch, 5))
      return 0;
    if (char_get_level(ch) >= 100) {
      send_to_char(g, ch, "You can't use this at level 100.\r\n");
      return 0;
    }
    if (char_is_arlian(ch)) {
      send_to_char(g, ch, "This is not available to bugs.\r\n");
      return 0;
    }
    if (is_soft_cap(ch, 0, 1.0) && is_soft_cap(ch, 1, 1.0) && is_soft_cap(ch, 2, 1.0)) {
      send_to_char(g, ch, "You are already above your softcap for this level.\r\n");
      return 0;
    }
    bring_to_cap(ch);
    return 5;
  }

  int store(GameWorld *g, struct char_data *ch, std::string &arg)
  {
    if (arg.empty()) {
      disp_rpp_store(g, ch);
      return 0;
    }
    int choice = parse_int(arg);
    if (choice <= 0) {
      send_to_char(g, ch, "That is not a choice in the RPP store!\r\n");
      return 0;
    }
    handle_rpp_store(g, ch, choice);
    return 0;
  }

  int feature(GameWorld *g, struct char_data *ch, std::string &arg)
  {
    rpp_feature(g, ch, arg);
    return 0;
  }

  int restring(GameWorld *g, struct char_data *ch, std::string &arg)
  {
    if (short_of(g, ch, 1, "You need at least 1 RPP to initiate an equipment restring.\r\n"))
      return 0;
    struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
    if (!obj) {
      send_to_char(g, ch, "You don't have a that equipment to restring in your inventory.\r\n");
      send_to_char(g, ch, "Syntax: rpp 14 (obj name)\r\n");
      return 0;
    }
    if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
      send_to_char(g, ch, "You can not restring a custom piece. Why? Cause I say so. :P\r\n");
      return 0;
    }
    struct descriptor_data *d = char_get_desc(ch);
    desc_set_connected(d, CON_POBJ);
    d->obj_name      = str_format("{}", obj_get_name(obj));
    d->obj_was       = str_format("{}", obj_get_short_description(obj));
    d->obj_short     = d->obj_was;
    d->obj_long      = str_format("{}", obj_get_description(obj));
    d->obj_point_uid = obj_get_id(obj);
    d->obj_type      = 1;
    d->obj_weapon    = 0;
    disp_restring_menu(g, d);
    d->obj_editflag = EDIT_RESTRING;
    d->obj_editval  = EDIT_RESTRING_MAIN;
    return 0;
  }

  int skillslot(GameWorld *g, struct char_data *ch, std::string &)
  {
    if (short_of(g, ch, 3, "You do not have enough RPP in your bank for that selection.\r\n"))
      return 0;
    if (char_get_skill_slots(ch) >= (char_get_bonuses(ch, BONUS_GMEMORY) ? 65 : 60)) {
      send_to_char(g, ch, "You are already at your skillslot cap.\r\n");
      return 0;
    }
    char_set_skill_slots(ch, char_get_skill_slots(ch) + 1);
    return 3;
  }

  /* By menu number, from 1. */
  constexpr reward REWARDS[] = {
      house_capsule,
      disabled, /* custom skills */
      custom_equipment,
      alignment,
      zenni,
      raise_stat,
      practice,
      disabled, /* revival */
      experience,
      aura,
      softcap,
      store,
      feature,
      restring,
      skillslot,
  };
} // namespace

/* Let's Reward Those Roleplayers! - Iovan*/
ACMD(do_rpp)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string arg2(skip_spaces(rest));

  if (char_is_npc(ch))
    return;

  if (arg.empty()) {
    show_rewards(g, ch);
    return;
  }

  int selection = parse_int(arg);
  if (selection <= 0 || selection > std::ssize(REWARDS)) {
    send_to_char(g, ch, "You must choose a number from the menu. Enter the command again with no arguments for the menu.\r\n");
    return;
  }
  if (int pay = REWARDS[selection - 1](g, ch, arg2); pay > 0)
    pay_rpp(g, ch, pay);
}

ACMD(do_commune)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(g, ch, SKILL_COMMUNE)) {
    return;
  }

  if ((getCurST(g, ch)) >= getMaxST(g, ch)) {
    send_to_char(g, ch, "Your stamina is already at full.\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_COMMUNE), perc = axion_dice(g, 0);
  int64_t cost = getMaxST(g, ch) * .05;

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki to commune with the Eldritch Star.\r\n");
    return;
  }
  if (prob < perc) {
    decCurKI(g, ch, cost, 0);
    reveal_hiding(g, ch, 0);
    act(g, "@cYou close your eyes and try to commune with the Eldritch Star. You are unable to concentrate though.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@W$n closes $s eyes for a moment. Then $e reopens them and frowns.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  decCurKI(g, ch, cost, 0);
  decCurST(g, ch, cost, 0);
  reveal_hiding(g, ch, 0);
  act(g, "@cYou close your eyes and commune with the Eldritch Star spiritually. You feel your stamina replenish some.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@W$n closes $s eyes for a moment. Then $e reopens them and smiles.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_wait(ch, PULSE_2SEC);
  return;
}

ACMD(do_pose)
{

  if (!know_skill(g, ch, SKILL_POSE)) {
    return;
  }

  if ((getCurST(g, ch)) < getMaxST(g, ch) / 40) {
    send_to_char(g, ch, "You do not have enough stamina to pull off such an exciting pose!\r\n");
    return;
  }

  if (is_affected(ch, AFF_SPECIAL_POSE)) {
    send_to_char(g, ch, "You are already feeling good and confident from a previous pose.\r\n");
    return;
  }

  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are too busy to pose right now!\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_POSE);
  int perc = rand_number(g, 1, 70);

  if (char_get_real_abils_str(ch) + 8 > 70 && char_get_bonuses(ch, BONUS_WIMP) > 0) {
    send_to_char(g, ch, "You can't handle having your strength increased beyond 70.\r\n");
    return;
  }
  if (char_get_real_abils_dex(ch) + 8 > 70 && char_get_bonuses(ch, BONUS_CLUMSY) > 0) {
    send_to_char(g, ch, "You can't handle having your agility increased beyond 70.\r\n");
    return;
  }

  if (prob < perc) {
    reveal_hiding(g, ch, 0);
    act(g, "@WYou attempt to strike an awe inspiring pose, but end up falling on your face!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W attempts to strike an awe inspiring pose, but ends up falling on $s face!@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurST(g, ch, getMaxST(g, ch) / 40, 0);
    improve_skill(g, ch, SKILL_POSE, 0);
    return;
  }
  reveal_hiding(g, ch, 0);
  switch (rand_number(g, 1, 4)) {
    case 1:
      act(g, "@WYou turn around with your back to everyone. You bend forward dramatically and put your head between your legs!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W turns around with $s back to you. $e bends forward dramatically and puts $s head between $s legs. Strange...@n", true, ch, nullptr, nullptr, TO_ROOM);
      break;
    case 2:
      act(g, "@WYou turn to the side while flexing your muscles and extend your arms up at an angle dramatically!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W turns to the side while flexing $s muscles and extending $s arms up at an angle dramatically!@n", true, ch, nullptr, nullptr, TO_ROOM);
      break;
    case 3:
      act(g, "@WYou extend one leg outward while you bend forward, balancing on a single leg!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W extends one leg outward while $e bends forward, balancing on a single leg!@n", true, ch, nullptr, nullptr, TO_ROOM);
      break;
    case 4:
      act(g, "@WYou drop down to one knee while angling your arms up to either side and slanting your hands down like wings!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W drops down to one knee while angling $s arms up to either side and slanting $s hands down like wings!@n", true, ch, nullptr, nullptr, TO_ROOM);
      break;
  }
  send_to_char(g, ch, "@WYou feel your confidence increase! @G+8 Str @Wand@G +8 Agl!@n\r\n");
  assign_affect(g, ch, AFF_SPECIAL_POSE, SKILL_POSE, -1, 8, 0, 0, 8, 0, 0);
  save_char(g, ch);
  int64_t before = (getMaxLF(g, ch));
  incCurLF(g, ch, (getMaxLF(g, ch)) - before, true);
  decCurST(g, ch, getMaxST(g, ch) / 40, 0);
  improve_skill(g, ch, SKILL_POSE, 0);
  return;
}

/* End of do_fury for halfbreeds to release their raaage, rawrg! */

void hint_system(GameWorld *g, struct char_data *ch, int num)
{
  const char *hints[22] = {"Remember to save often.",                                                                                                                 /* 0 */
                           "Remember to eat or drink if you want to stay alive.",                                                                                     /* 1 */
                           "It is a good idea to save up PS for learning skills instead of just practicing them.",                                                    /* 2 */
                           "A good way to save up money is with the bank.",                                                                                           /* 3 */
                           "If you want to stay alive in this rough world you will need to be mindful of your surroundings.",                                         /* 4 */
                           "Knowing when to rest and recover can be the difference between life and death.",                                                          /* 5 */
                           "Not every battle can be won. Great warriors know how to pick their fights.",                                                              /* 6 */
                           "It is a good idea to experiment with skills fully before deciding their worth.",                                                          /* 7 */
                           "Having a well balanced repertoire of skills can help you out of any situation.",                                                          /* 8 */
                           "You can become hidden from your enemies on who and ooc with the whohide command.",                                                        /* 9 */
                           "You can value an item at a shopkeeper with the value command.",                                                                           /* 10 */
                           "There are ways to earn money through jobs, try looking for a job. Bum.",                                                                  /* 11 */
                           "You never know what may be hidden nearby. You should always check out anything you can.",                                                 /* 12 */
                           "You should check for a help file on any subject you can, you never know how the info may 'help' you.",                                    /* 13 */
                           "Until you are capable of taking care of yourself for long periods of time you should stick near your sensei.",                            /* 14 */
                           "You shouldn't travel to other planets until you have a stable supply of money.",                                                          /* 15 */
                           "There is a vast galaxy out there that you may not be able to reach by public ship.",                                                      /* 16 */
                           "Score is used to view the various statistics about your character.",                                                                      /* 17 */
                           "Status is used to view what is influencing your character and its characteristics.",                                                      /* 18 */
                           "You will need a scouter in order to use the Scouter Network (SNET).",                                                                     /* 19 */
                           "The DBAT forum is a great resource for finding out information and for conversing\r\nwith fellow players. http://advent-truth.com/forum", /* 20 */
                           "Found a bug or have a suggestion? Log into our forums and post in the relevant section."};
  if (num == 0) {
    num = rand_number(g, 0, 21);
  }

  if (!char_is_android(ch) && !char_is_namek(ch)) {
    send_to_char(g, ch, "@D[@GHint@D] @G{}@n\r\n", hints[num]);
  } else {
    if (num == 1) {
      num = 0;
    }
    send_to_char(g, ch, "@D[@GHint@D] @G{}@n\r\n", hints[num]);
  }
  send_to_char(g, ch, "@D(@gYou can turn off hints with the command 'hints'@D)@n\r\n");
}

ACMD(do_think)
{
  argument = skip_spaces(argument);

  if (char_is_npc(ch)) {
    return;
  }
  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "Lol, no.\r\n");
    return;
  }
  if (char_get_skill(ch, SKILL_TELEPATHY)) {
    send_to_char(g, ch, "You can just use telepathy.\r\n");
    return;
  }
  if (!char_get_mindlink(g, ch)) {
    send_to_char(g, ch, "No one has linked with your mind.\r\n");
    return;
  }
  if (argument.empty()) {
    send_to_char(g, ch, "Syntax: think (message)\r\n");
    return;
  }
  struct char_data *tch;
  tch = char_get_mindlink(g, ch);
  send_to_char(g, ch, "@c{}@w reads your thoughts, '@C{}@w'@n\r\n", char_get_name(tch), argument);
  send_to_char(g, tch, "@c{}@w thinks, '@C{}@w'@n\r\n", char_get_name(ch), argument);
  send_to_imm(g, "@GTELEPATHY: @C{}@G telepaths @c{}, @W'@w{}@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), char_get_admlevel(tch) > 0 ? char_get_name(tch) : char_get_user(tch), argument);
  return;
}

