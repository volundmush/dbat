/* focus.c: the focus command (ki techniques)
 * Split from act.other.c (CircleMUD/DBAT; see license.doc).
 *
 * Most of what can be focused is a buff (or a curse) that the user lays on themselves, or on another: those are
 * focus_buffs, made by focus_buff_on(). The rest (wither, yoikominminken, vigor, cure, poison) are each their own. */
#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "dbat/act.offensive.hpp"
#include "dbat/act.other.hpp"
#include "dbat/affect.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/class.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/positions.hpp"
#include "dbat/consts/pulse.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/spells.hpp"
#include "dbat/db.hpp"
#include "dbat/fight.hpp"
#include "dbat/random.hpp"
#include "dbat/search.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

namespace {

/* to the user, then to the room, of something they do to themselves */
void act_self(GameWorld *g, const act_msgs &m, struct char_data *ch)
{
  act(g, m.to_char, true, ch, nullptr, nullptr, TO_CHAR);
  act(g, m.to_room, true, ch, nullptr, nullptr, TO_ROOM);
}

/* What it costs: a twentieth of the user's max ki */
int64_t focus_cost(GameWorld *g, struct char_data *ch)
{
  return getMaxKI(g, ch) / 20;
}

bool has_ki_for(GameWorld *g, struct char_data *ch, int64_t cost, const char *msg)
{
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "{}", msg);
    return false;
  }
  return true;
}

/* Another to focus on, in the room: or no one, having been told why */
struct char_data *focus_target(GameWorld *g, struct char_data *ch, std::string &name, const char *who, int kill_flag)
{
  struct char_data *vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM);

  if (!vict) {
    send_to_char(g, ch, "{}", who);
    return nullptr;
  }
  if (!can_kill(g, ch, vict, nullptr, kill_flag)) /* which turns the user away too */
    return nullptr;
  return vict;
}

bool worth_ki(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
    return false;
  }
  return true;
}

/* The body can't take a stat over 70 with this bonus (a flaw), and this rise in it */
struct focus_limit {
  int bonus{};
  int8_t (*stat)(const struct char_data *){};
  int rise{};
  const char *msg{};
};

struct focus_buff {
  int skill{};
  int aff{};
  int kill_flag{2};              /* can_kill()'s, on another */
  const char *who{};             /* when there is no one by that name */
  const char *self_has{};        /* already has it */
  const char *vict_has{};
  std::array<focus_limit, 2> self_limits{};
  std::array<focus_limit, 2> vict_limits{};
  bool demon_immune{};
  const char *self_no_ki{};
  const char *vict_no_ki{};
  act_msgs self_fail;            /* to_char and to_room */
  act_msgs vict_fail;
  bool vict_fail_unnamed{};      /* quirk: the user's message of a failure on another is act()ed without them */
  int duration_add{2};           /* roll_aff_duration(intelligence, this) */
  bool self_flat_duration{};     /* quirk: on themselves, it lasts intelligence / 20, unrolled */
  bool pays_first{};             /* the ki is spent before the duration is rolled, not after the affect */
  bool self_sets_flag{};         /* quirk: on themselves, the flag is set before the affect is */
  bool self_on_nobody{};         /* quirk: on themselves, the affect goes to no one */
  std::array<int, 6> mods{};     /* str, con, int, agl, wis, spd */
  act_msgs self_done;            /* to_char and to_room */
  act_msgs vict_done;
  void (*before_msgs)(GameWorld *g, struct char_data *ch, struct char_data *target, bool self){};
  void (*after)(GameWorld *g, struct char_data *ch, struct char_data *target, bool self){};
};

bool within_limits(GameWorld *g, struct char_data *ch, struct char_data *target, const std::array<focus_limit, 2> &limits)
{
  for (const focus_limit &l : limits)
    if (l.stat && char_get_bonuses(target, l.bonus) > 0 && l.stat(target) + l.rise > 70) {
      send_to_char(g, ch, "{}", l.msg);
      return false;
    }
  return true;
}

void focus_buff_on(GameWorld *g, struct char_data *ch, std::string &name, const focus_buff &b)
{
  if (!know_skill(g, ch, b.skill))
    return;

  const bool self        = name.empty();
  struct char_data *vict = nullptr;
  if (self) {
    if (char_aff_flagged(ch, b.aff)) {
      send_to_char(g, ch, "{}", b.self_has);
      return;
    }
    if (!within_limits(g, ch, ch, b.self_limits))
      return;
    if (b.demon_immune && char_is_demon(ch)) {
      send_to_char(g, ch, "You are immune to curses!\r\n");
      return;
    }
  } else {
    if (!(vict = focus_target(g, ch, name, b.who, b.kill_flag)))
      return;
    if (char_aff_flagged(vict, b.aff)) {
      send_to_char(g, ch, "{}", b.vict_has);
      return;
    }
    if (!within_limits(g, ch, vict, b.vict_limits) || !worth_ki(g, ch, vict))
      return;
    if (b.demon_immune && char_is_demon(vict)) {
      send_to_char(g, ch, "They are immune to curses!\r\n");
      return;
    }
  }
  if (!has_ki_for(g, ch, focus_cost(g, ch), self ? b.self_no_ki : b.vict_no_ki))
    return;

  if (char_get_skill(ch, b.skill) < axion_dice(g, 0)) {
    decCurKI(g, ch, focus_cost(g, ch), 0);
    reveal_hiding(g, ch, 0);
    if (self)
      act_self(g, b.self_fail, ch);
    else if (b.vict_fail_unnamed) {
      act(g, b.vict_fail.to_char, true, ch, nullptr, nullptr, TO_CHAR);
      act_triple(g, {nullptr, b.vict_fail.to_vict, b.vict_fail.to_room, true}, ch, nullptr, vict);
    } else
      act_triple(g, b.vict_fail, ch, nullptr, vict);
    return;
  }

  struct char_data *target = self ? ch : vict;
  struct char_data *gets   = self && b.self_on_nobody ? nullptr : target;
  const auto &[str, con, intel, agl, wis, spd] = b.mods;
  if (b.pays_first) {
    if (self && b.self_sets_flag)
      char_aff_set(ch, b.aff, true);
    decCurKI(g, ch, focus_cost(g, ch), 0);
    int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), b.duration_add);
    assign_affect(g, gets, b.aff, b.skill, duration, str, con, intel, agl, wis, spd);
  } else {
    int duration = self && b.self_flat_duration ? char_get_aff_abils_intel(ch) / 20
                                                : roll_aff_duration(g, char_get_aff_abils_intel(ch), b.duration_add);
    assign_affect(g, gets, b.aff, b.skill, duration, str, con, intel, agl, wis, spd);
    decCurKI(g, ch, focus_cost(g, ch), 0);
  }
  reveal_hiding(g, ch, 0);
  if (b.before_msgs)
    b.before_msgs(g, ch, target, self);
  if (self)
    act_self(g, b.self_done, ch);
  else
    act_triple(g, b.vict_done, ch, nullptr, vict);
  if (b.after)
    b.after(g, ch, target, self);
}

/* A kai grouped with another they helped may learn from it */
void kai_learns_from_another(GameWorld *g, struct char_data *ch, struct char_data *vict, bool self)
{
  if (self)
    return;
  if ((char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict)) && char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
    if (char_is_kai(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && rand_number(g, 1, 3) == 3) {
      char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.05));
    }
  }
}

/* A jinto's enlightenment, for 15 practices, may give whoever is enlightened a share of their next level */
void jinto_enlightens(GameWorld *g, struct char_data *ch, struct char_data *target, bool self)
{
  static constexpr struct {
    int skill;
    double share;
  } SHARES[] = {{100, 0.15}, {60, 0.10}, {40, 0.05}};

  if (char_is_jinto(ch) && level_exp(target, char_get_level(target) + 1) - char_get_exp(target) > 0 && char_get_practice_points(g, ch) >= 15 && rand_number(g, 1, 4) >= 3) {
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (15));
    int skill = char_get_skill(ch, SKILL_ENLIGHTEN);
    auto tier = std::ranges::find_if(SHARES, [skill](const auto &s) { return skill >= s.skill; });
    if (tier == std::end(SHARES))
      return;
    int64_t gain = level_exp(target, char_get_level(target) + 1) * tier->share;
    char_set_exp(target, char_get_exp(target) + (gain));
    if (self)
      send_to_char(g, target, "@GYou gain @g{}@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
    else
      send_to_char(g, target, "@GYou gain @g{}@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
  }
}

/* How well the blessed heal: as well as a kabito blesses themselves, or a kai another (quirk) */
void bless_level(GameWorld *g, struct char_data *ch, struct char_data *target, bool self)
{
  if (self ? char_is_kabito(ch) : char_is_kai(ch)) {
    char_set_blesslvl(target, char_get_skill(ch, SKILL_BLESS));
  } else {
    char_set_blesslvl(target, 0);
  }
}

void blessed(GameWorld *g, struct char_data *ch, struct char_data *target, bool self)
{
  kai_learns_from_another(g, ch, target, self);
  if (char_aff_flagged(target, AFF_CURSE)) {
    send_to_char(g, target, "Your cursing was nullified!\r\n");
    null_affect(g, target, AFF_CURSE);
  }
}

void cursed(GameWorld *g, struct char_data *ch, struct char_data *target, bool self)
{
  if (char_aff_flagged(target, AFF_BLESS)) {
    send_to_char(g, target, "Your blessing was nullified!\r\n");
    null_affect(g, target, AFF_BLESS);
  }
}

constexpr focus_buff TOUGH = {
    .skill              = SKILL_TSKIN,
    .aff                = AFF_STONESKIN,
    .who                = "Focus your ki into who's skin?\r\n",
    .self_has           = "You already have tough skin!\r\n",
    .vict_has           = "They already have tough skin!\r\n",
    .self_no_ki         = "You do not have enough ki to infuse into your skin.\r\n",
    .vict_no_ki         = "You do not have enough ki to infuse into their skin.\r\n",
    .self_fail          = {"You focus ki into your skin, but fail in making it tough!", nullptr, "$n focuses ki into $s skin, but fails in making it tough!", true},
    .vict_fail          = {"You focus ki into $N's skin, but fail in making it tough!", "$n focuses ki into your skin, but fails in making it tough!", "$n focuses ki into $N's skin, but fails in making it tough!", true},
    .self_flat_duration = true,
    .self_done          = {"You focus ki into your skin, making it tough!", nullptr, "$n focuses ki into $s skin, making it tough!", true},
    .vict_done          = {"You focus ki into $N's skin, making it tough!", "$n focuses ki into your skin, making it tough!", "$n focuses ki into $N's skin, making it tough!", true},
};

constexpr focus_buff MIGHT = {
    .skill          = SKILL_MIGHT,
    .aff            = AFF_MIGHT,
    .who            = "Focus your ki into who's muscles?\r\n",
    .self_has       = "You already have mighty muscles!\r\n",
    .vict_has       = "They already have mighty muscles!\r\n",
    .self_limits    = {{{BONUS_WIMP, char_get_aff_abils_str, 10, "Your body is not able to withstand increasing its strength beyond 70.\r\n"},
                        {BONUS_FRAIL, char_get_aff_abils_str, 2, "Your body is not able to withstand increasing its strength beyond 70.\r\n"}}}, /* quirk: strength, not constitution */
    .vict_limits    = {{{BONUS_WIMP, char_get_aff_abils_str, 10, "Their body is not able to withstand increasing its strength beyond 70.\r\n"},
                        {BONUS_FRAIL, char_get_aff_abils_con, 2, "Their body is not able to withstand increasing its constitution beyond 70.\r\n"}}},
    .self_no_ki     = "You do not have enough ki to infuse into your muscles.\r\n",
    .vict_no_ki     = "You do not have enough ki to infuse into their muscles.\r\n",
    .self_fail      = {"You focus ki into your muscles, but fail in making them mighty!", nullptr, "$n focuses ki into $s muscles, but fails in making them mighty!", true},
    .vict_fail      = {"You focus ki into $N's muscles, but fail in making them mighty!", "$n focuses ki into your muscles, but fails in making them mighty!", "$n focuses ki into $N's muscles, but fails in making them mighty!", true},
    .pays_first     = true,
    .self_sets_flag = true,
    .mods           = {10, 2, 0, 0, 0, 0},
    .self_done      = {"You focus ki into your muscles, making them mighty!", nullptr, "$n focuses ki into $s muscles, making them mighty!", true},
    .vict_done      = {"You focus ki into $N's muscles, making them mighty!", "$n focuses ki into your muscles, making them mighty!", "$n focuses ki into $N's muscles, making them mighty!", true},
};

constexpr focus_buff ENLIGHTEN = {
    .skill       = SKILL_ENLIGHTEN,
    .aff         = AFF_ENLIGHTEN,
    .who         = "Focus your ki into who's mind?\r\n",
    .self_has    = "You already have superior wisdom!\r\n",
    .vict_has    = "They already have superior wisdom!\r\n",
    .self_limits = {{{BONUS_FOOLISH, char_get_aff_abils_wis, 10, "You're not able to withstand increasing your wisdom beyond 70.\r\n"}}},
    .vict_limits = {{{BONUS_FOOLISH, char_get_aff_abils_wis, 10, "They're not able to withstand increasing their wisdom beyond 70.\r\n"}}},
    .self_no_ki  = "You do not have enough ki to use this skill.\r\n",
    .vict_no_ki  = "You do not have enough ki to use this skill.\r\n",
    .self_fail   = {"You focus ki into your mind, but fail in awakening it to cosmic wisdom!", nullptr, "$n focuses ki into $s mind, but fails in awakening it to cosmic wisdom!", true},
    .vict_fail   = {"You focus ki into $N's mind, but fail in awakening it to cosmic wisdom!", "$n focuses ki into your mind, but fails in awakening it to cosmic wisdom!", "$n focuses ki into $N's mind, but fails in awakening it to cosmic wisdom!", true},
    .mods        = {0, 0, 0, 0, 10, 0},
    .self_done   = {"You focus ki into your mind, awakening it to cosmic wisdom!", nullptr, "$n focuses ki into $s mind, awakening it to cosmic wisdom!", true},
    .vict_done   = {"You focus ki into $N's mind, awakening it to cosmic wisdom!", "$n focuses ki into your mind, awakening it to cosmic wisdom!", "$n focuses ki into $N's mind, awakening it to cosmic wisdom!", true},
    .after       = jinto_enlightens,
};

constexpr focus_buff GENIUS = {
    .skill       = SKILL_GENIUS,
    .aff         = AFF_GENIUS,
    .who         = "Focus your ki into who's mind?\r\n",
    .self_has    = "You already have superior intelligence!\r\n",
    .vict_has    = "They already have superior intelligence!\r\n",
    .self_limits = {{{BONUS_DULL, char_get_aff_abils_intel, 10, "You're not able to withstand increasing your intelligence beyond 70.\r\n"}}},
    .vict_limits = {{{BONUS_DULL, char_get_aff_abils_intel, 10, "They're not able to withstand increasing their intelligence beyond 70.\r\n"}}},
    .self_no_ki  = "You do not have enough ki to infuse into your mind.\r\n",
    .vict_no_ki  = "You do not have enough ki to infuse into their mind.\r\n",
    .self_fail   = {"You focus ki into your mind, but fail in making it work faster!", nullptr, "$n focuses ki into $s muscles, but fails in making it work faster!", true},
    .vict_fail   = {"You focus ki into $N's mind, but fail in making it work faster!", "$n focuses ki into your mind, but fails in making it work faster!", "$n focuses ki into $N's mind, but fails in making it work faster!", true},
    .mods        = {0, 0, 10, 0, 0, 0},
    .self_done   = {"You focus ki into your mind, making it work faster!", nullptr, "$n focuses ki into $s mind, making it work faster!", true},
    .vict_done   = {"You focus ki into $N's mind, making it work faster!", "$n focuses ki into your mind, making it work faster!", "$n focuses ki into $N's mind, making it work faster!", true},
    .after       = kai_learns_from_another,
};

constexpr focus_buff FLEX = {
    .skill       = SKILL_FLEX,
    .aff         = AFF_FLEX,
    .who         = "Focus your ki into who's limbs?\r\n",
    .self_has    = "You already have superior agility!\r\n",
    .vict_has    = "They already have superior agility!\r\n",
    .self_limits = {{{BONUS_CLUMSY, char_get_aff_abils_dex, 10, "You're not able to withstand increasing your agility beyond 70.\r\n"}}},
    .vict_limits = {{{BONUS_CLUMSY, char_get_aff_abils_dex, 3, "They're not able to withstand increasing their agility beyond 70.\r\n"}}}, /* quirk: 3, not 10 */
    .self_no_ki  = "You do not have enough ki to infuse into your limbs.\r\n",
    .vict_no_ki  = "You do not have enough ki to infuse into their limbs.\r\n",
    .self_fail   = {"You focus ki into your limbs, but fail in making them more flexible!", nullptr, "$n focuses ki into $s muscles, but fails in making them more flexible!", true},
    .vict_fail   = {"You focus ki into $N's limbs, but fail in making them more flexible!", "$n focuses ki into your limbs, but fails in making them more flexible!", "$n focuses ki into $N's limbs, but fails in making them more flexible!", true},
    .pays_first  = true,
    .mods        = {0, 0, 0, 10, 0, 0},
    .self_done   = {"You focus ki into your limbs, making them more flexible!", nullptr, "$n focuses ki into $s limbs, making them more flexible!", true},
    .vict_done   = {"You focus ki into $N's limbs, making them more flexible!", "$n focuses ki into your limbs, making them more flexible!", "$n focuses ki into $N's limbs, making them more flexible!", true},
    .after       = kai_learns_from_another,
};

constexpr focus_buff BLESS = {
    .skill             = SKILL_BLESS,
    .aff               = AFF_BLESS,
    .who               = "Bless who?\r\n",
    .self_has          = "You already are blessed!\r\n",
    .vict_has          = "They already have been blessed!\r\n",
    .self_no_ki        = "You do not have enough ki to bless.\r\n",
    .vict_no_ki        = "You do not have enough ki to bless.\r\n",
    .self_fail         = {"You focus ki while chanting spiritual words. Your blessing does nothing though, you must have messed up!", nullptr, "$n focuses ki while chanting spiritual words. $n seems disappointed.", true},
    .vict_fail         = {"You focus ki while chanting spiritual words. Your blessing fails!", "$n focuses ki while chanting spiritual words. $n places a hand on your head, but nothing happens!", "$n focuses ki while chanting spiritual words. $n places a hand on $N's head, but nothing happens!", true},
    .vict_fail_unnamed = true,
    .duration_add      = 3,
    .self_done         = {"You focus ki while chanting spiritual words. You feel your body recovering at above normal speed!", nullptr, "$n focuses ki while chanting spiritual words. $n smiles after finishing $s chant.", true},
    .vict_done         = {"You focus ki while chanting spiritual words. Blessing $N with faster regeneration!", "$n focuses ki while chanting spiritual words. $n then places a hand on your head, blessing you!", "$n focuses ki while chanting spiritual words. $n then places a hand on $N's head, blessing them!", true},
    .before_msgs       = bless_level,
    .after             = blessed,
};

constexpr focus_buff CURSE = {
    .skill             = SKILL_CURSE,
    .aff               = AFF_CURSE,
    .kill_flag         = 0,
    .who               = "Curse who?\r\n",
    .self_has          = "You already are cursed!\r\n", /* never: do_focus turns the cursed away */
    .vict_has          = "They already have been cursed!\r\n",
    .demon_immune      = true,
    .self_no_ki        = "You do not have enough ki to CURSE.\r\n",
    .vict_no_ki        = "You do not have enough ki to CURSE.\r\n",
    .self_fail         = {"You focus ki while chanting demonic words. Your cursing does nothing though, you must have messed up!", nullptr, "$n focuses ki while chanting demonic words. $n seems disappointed.", true},
    .vict_fail         = {"You focus ki while chanting demonic words. Your cursing fails!", "$n focuses ki while chanting demonic words. $n places a hand on your head, but nothing happens!", "$n focuses ki while chanting demonic words. $n places a hand on $N's head, but nothing happens!", true},
    .vict_fail_unnamed = true,
    .duration_add      = 3,
    .self_on_nobody    = true,
    .self_done         = {"You focus ki while chanting demonic words. You feel your body recovering at below normal speed!", nullptr, "$n focuses ki while chanting demonic words. $n grins after finishing $s chant.", true},
    .vict_done         = {"You focus ki while chanting demonic words. cursing $N with slower regeneration!", "$n focuses ki while chanting demonic words. $n then places a hand on your head, cursing you!", "$n focuses ki while chanting demonic words. $n then places a hand on $N's head, cursing them!", true},
    .after             = cursed,
};

template <const focus_buff &B>
void buff(GameWorld *g, struct char_data *ch, std::string &name)
{
  focus_buff_on(g, ch, name, B);
}

void wither(GameWorld *g, struct char_data *ch, std::string &name)
{
  if (!know_skill(g, ch, SKILL_WITHER))
    return;
  struct char_data *vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM);
  if (!vict) {
    send_to_char(g, ch, "Focus your ki into who's muscles?\r\n");
    return;
  }
  if (ch == vict) {
    send_to_char(g, ch, "You don't want to wither your own body!\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 2))
    return;
  if (is_affected(vict, AFF_WITHER)) { /* quirk: never so of a wither, whose affects are on strength and speed */
    send_to_char(g, ch, "They already have been withered!\r\n");
    return;
  }
  if (!has_ki_for(g, ch, focus_cost(g, ch), "You do not have enough ki to wither them.\r\n"))
    return;
  if (char_get_skill(ch, SKILL_WITHER) < axion_dice(g, 0)) {
    decCurKI(g, ch, focus_cost(g, ch), 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"You focus ki into $N's body, but fail in withering it!", "$n focuses ki into your body, but fails in withering it!", "$n focuses ki into $N's body, but fails in withering it!", true}, ch, nullptr, vict);
    return;
  }
  decCurKI(g, ch, focus_cost(g, ch), 0);
  assign_affect(g, vict, AFF_WITHER, SKILL_WITHER, -1, -3, 0, 0, 0, 0, -3);
  save_char(g, vict);
  reveal_hiding(g, ch, 0);
  act_triple(g, {"You focus ki into $N's body, and succeed in withering it!", "$n focuses ki into your body, and succeeds in withering it!", "$n focuses ki into $N's body, and succeeds in withering it!", true}, ch, nullptr, vict);
}

void yoikominminken(GameWorld *g, struct char_data *ch, std::string &name)
{
  if (!know_skill(g, ch, SKILL_YOIK))
    return;
  struct char_data *vict = focus_target(g, ch, name, "Use Yoikominminken on who?\r\n", 0);
  if (!vict)
    return;
  if (char_aff_flagged(vict, AFF_SLEEP)) {
    send_to_char(g, ch, "They already have been put to sleep!\r\n");
    return;
  }
  if (char_plr_flagged(vict, PLR_EYEC)) {
    send_to_char(g, ch, "Their eyes are closed!\r\n");
    return;
  }
  if (char_aff_flagged(vict, AFF_BLIND)) {
    send_to_char(g, ch, "They appear to be blind!\r\n");
    return;
  }
  if (!has_ki_for(g, ch, focus_cost(g, ch), "You do not have enough ki to use Yoikominminken.\r\n"))
    return;
  if (char_get_bonuses(vict, BONUS_INSOMNIAC)) {
    decCurKI(g, ch, focus_cost(g, ch), 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"You focus ki while moving your hands in lulling patterns, but $N doesn't look the least bit sleepy!", "$n focuses ki while moving $s hands in a lulling pattern, but you just don't feel tired.", "$n focuses ki while moving $s hands in a lulling pattern, but $N doesn't look the least bit sleepy!", true}, ch, nullptr, vict);
    return;
  }
  if (char_get_skill(ch, SKILL_YOIK) < axion_dice(g, 0) || (char_get_aff_abils_intel(ch) + rand_number(g, 1, 3) < char_get_aff_abils_intel(vict) + rand_number(g, 1, 5))) {
    decCurKI(g, ch, focus_cost(g, ch), 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"You focus ki while moving your hands in lulling patterns, but fail to put $N to sleep!", "$n focuses ki while moving $s hands in a lulling pattern, but you resist the technique!", "$n focuses ki while moving $s hands in a lulling pattern, but $N resists the technique!", true}, ch, nullptr, vict);
    return;
  }
  int duration = rand_number(g, 1, 2);
  assign_affect(g, vict, AFF_SLEEP, SKILL_YOIK, duration, 0, 0, 0, 0, 0, 0);
  decCurKI(g, ch, focus_cost(g, ch), 0);
  reveal_hiding(g, ch, 0);
  act_triple(g, {"You focus ki while moving your hands in lulling patterns, putting $N to sleep!", "$n focuses ki while moving $s hands in a lulling pattern, before you realise it you are asleep!", "$n focuses ki while moving $s hands in a lulling pattern, putting $N to sleep!", true}, ch, nullptr, vict);
  char_set_position(vict, POS_SLEEPING);
  if (char_aff_flagged(vict, AFF_FLYING)) {
    char_aff_set(vict, AFF_FLYING, false);
    char_set_altitude(vict, 0);
  }
}

/* Ki into stamina: a tenth of the user's max ki for as much, or an eighth for a healer */
void vigor(GameWorld *g, struct char_data *ch, std::string &name)
{
  if (!know_skill(g, ch, SKILL_VIGOR))
    return;
  const bool self        = name.empty();
  struct char_data *vict = nullptr;
  if (!self && (!(vict = focus_target(g, ch, name, "VIGOR who?\r\n", 2)) || !worth_ki(g, ch, vict)))
    return;
  if (!has_ki_for(g, ch, getMaxKI(g, ch) / 10, "You do not have enough ki to use vigor.\r\n"))
    return;
  /* on another, whether they need it is asked before the roll; on themselves, after it (quirk) */
  if (!self && getCurST(g, vict) >= getMaxST(g, vict)) {
    send_to_char(g, ch, "They already have full stamina.\r\n");
    return;
  }
  if (char_get_skill(ch, SKILL_VIGOR) < axion_dice(g, 0)) {
    decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
    reveal_hiding(g, ch, 0);
    if (self)
      act_self(g, {"You focus ki into your very cells, but fail at re-engerizing them!", nullptr, "$n focuses ki and glows green for a moment, $e then frowns."}, ch);
    else
      act_triple(g, {"You focus ki into $N's very cells, and fail at re-energizing them!", "$n focuses ki into your very cells, but nothing happens!", "$n focuses ki and $N glows green for a moment, $N frowns.", true}, ch, nullptr, vict);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  if (self && getCurST(g, ch) >= getMaxST(g, ch)) {
    send_to_char(g, ch, "You already have full stamina.\r\n");
    return;
  }
  struct char_data *target = self ? ch : vict;
  if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
    incCurST(g, target, getMaxKI(g, target) / 8, true); /* quirk: the target's max ki, not the user's */
    decCurKI(g, ch, getMaxKI(g, ch) / 8, 0);
  } else {
    incCurST(g, target, getMaxKI(g, target) / 10, true);
    decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
  }
  reveal_hiding(g, ch, 0);
  if (self)
    act_self(g, {"You focus ki into your very cells, and manage to re-energize them!", nullptr, "$n focuses ki and glows green for a moment, $e then smiles."}, ch);
  else
    act_triple(g, {"You focus ki into $N's very cells, and manage to re-energize them!", "$n focuses ki into your very cells, and manages to re-energize them!", "$n focuses ki and $N glows green for a moment, $N smiles.", true}, ch, nullptr, vict);
  char_set_wait(ch, PULSE_2SEC);
}

void cure(GameWorld *g, struct char_data *ch, std::string &name)
{
  if (!know_skill(g, ch, SKILL_CURE))
    return;
  const bool self        = name.empty();
  struct char_data *vict = nullptr;
  if (!self && !(vict = focus_target(g, ch, name, "cure who?\r\n", 2)))
    return;
  struct char_data *target = self ? ch : vict;
  if (!char_aff_flagged(target, AFF_POISON)) {
    send_to_char(g, ch, self ? "You are not poisoned!\r\n" : "They are not poisoned!\r\n");
    return;
  }
  if (!has_ki_for(g, ch, focus_cost(g, ch), "You do not have enough ki to cure.\r\n"))
    return;
  if (char_get_skill(ch, SKILL_CURE) < axion_dice(g, 0)) {
    decCurKI(g, ch, focus_cost(g, ch), 0);
    reveal_hiding(g, ch, 0);
    if (self)
      act_self(g, {"You focus ki and aim a pulsing light at your body. Nothing happens!", nullptr, "$n focuses ki and aims a pulsing light at $s body. Nothing seems to happen."}, ch);
    else
      act_triple(g, {"You focus ki and aim a pulsing light at $N's body. Nothing happens.", "$n focuses ki and aims a pulsing light at your body. You are STILL poisoned!", "$n focuses ki and aims a pulsing light at $N's body. $N looks disappointed.", true}, ch, nullptr, vict);
    return;
  }
  affect_from_char(g, target, SPELL_POISON);
  decCurKI(g, ch, focus_cost(g, ch), 0);
  reveal_hiding(g, ch, 0);
  if (self)
    act_self(g, {"You focus ki and aim a pulsing light at your body. You feel the poison in your blood disappear!", nullptr, "$n focuses ki and aims a pulsing light at $s body. $n smiles."}, ch);
  else
    act_triple(g, {"You focus ki and aim a pulsing light at $N's body. $e is cured.", "$n focuses ki and aims a pulsing light at your body. You have been cured of your poison!", "$n focuses ki and aims a pulsing light at $N's body. $N smiles.", true}, ch, nullptr, vict);
  null_affect(g, target, AFF_POISON);
}

void poison(GameWorld *g, struct char_data *ch, std::string &name)
{
  if (!know_skill(g, ch, SKILL_POISON))
    return;
  struct char_data *vict = focus_target(g, ch, name, "Poison who?\r\n", 0);
  if (!vict)
    return;
  if (char_is_npc(vict) && char_mob_flagged(vict, MOB_NOPOISON)) {
    send_to_char(g, ch, "You get the feeling that this being is immune to poison.\r\n");
    return;
  }
  if (char_aff_flagged(vict, AFF_POISON)) {
    send_to_char(g, ch, "They already have been poisoned!\r\n");
    return;
  }
  if (!has_ki_for(g, ch, focus_cost(g, ch), "You do not have enough ki to poison.\r\n"))
    return;
  if (char_get_skill(ch, SKILL_POISON) < axion_dice(g, 0)) {
    decCurKI(g, ch, focus_cost(g, ch), 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"You focus ki and fling poison at $N. You missed!", "$n focuses ki and flings poison at you, but misses!", "$n focuses ki and flings poison at $N, but misses!", true}, ch, nullptr, vict);
    return;
  }
  decCurKI(g, ch, focus_cost(g, ch), 0);
  reveal_hiding(g, ch, 0);
  act_triple(g, {"You focus ki and fling poison at $N! The poison burns into $s skin!", "$n focuses ki and flings poison at you! The poison burns into your skin!", "$n focuses ki and flings poison at $N! The poison burns into $s skin!", true}, ch, nullptr, vict);
  if (char_is_npc(vict))
    set_fighting(g, vict, ch);
  if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 7 || char_get_genome(vict, 1) == 7)) {
    act_triple(g, {"However $N seems unaffected by the poison.", "Your natural immunity to poison prevents it from affecting you.", "However $N seems unaffected by the poison.", true}, ch, nullptr, vict);
    return;
  }
  char_set_poisonby(vict, ch);
  if (char_get_charge(vict) > 0) {
    send_to_char(g, vict, "You lose your concentration and release your charged ki!\r\n");
    do_charge(g, vict, "release", 0, 0);
  }
  int duration = char_get_aff_abils_intel(ch) / 20;
  assign_affect(g, vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
}

struct focus_kind {
  std::string_view name;
  void (*focus)(GameWorld *g, struct char_data *ch, std::string &name);
};

constexpr focus_kind FOCUSES[] = {
    {"tough",          buff<TOUGH>    },
    {"might",          buff<MIGHT>    },
    {"wither",         wither         },
    {"enlighten",      buff<ENLIGHTEN>},
    {"genius",         buff<GENIUS>   },
    {"flex",           buff<FLEX>     },
    {"bless",          buff<BLESS>    },
    {"curse",          buff<CURSE>    }, /* quirk: on themselves, a success gives the curse to no one, and crashes */
    {"yoikominminken", yoikominminken },
    {"yoik",           yoikominminken },
    {"vigor",          vigor          },
    {"cure",           cure           },
    {"poison",         poison         },
};

} // namespace

ACMD(do_focus)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string name      = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Yes but what do you want to focus?\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_CURSE)) {
    send_to_char(g, ch, "You are cursed and can't focus!\r\n");
    return;
  }
  auto focus = std::ranges::find(FOCUSES, std::string_view(arg), &focus_kind::name);
  if (focus == std::end(FOCUSES)) {
    send_to_char(g, ch, "What do you want to focus?\r\n");
    return;
  }
  focus->focus(g, ch, name);
}
