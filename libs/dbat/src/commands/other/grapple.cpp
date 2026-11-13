/* grapple.c: grappling and other physical actions
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>

#include <sys/stat.h>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
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

namespace {
  /* How much ch's speed against vict's adds to their chance */
  int speed_modifier(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    double mine = char_get_speedi(g, ch), theirs = char_get_speedi(g, vict);
    if (mine > theirs * 2)
      return 5;
    if (mine > theirs)
      return 2;
    if (mine * 2 < theirs)
      return -5;
    if (mine < theirs)
      return -2;
    return 0;
  }

  /* What's said when vict zanzokens away from something, and when ch's own zanzoken catches them anyway */
  struct zanzoken_msgs {
    act_msgs dodged, caught;
  };

  /*
   * An icer (sometimes) or anyone prepared with zanzoken slips away unless ch's own zanzoken is faster, which costs
   * both theirs. Returns true if vict got away, having charged ch for trying.
   */
  bool zanzoken_escape(GameWorld *g, struct char_data *ch, struct char_data *vict, const zanzoken_msgs &msgs, int64_t cost)
  {
    if (!(((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          getCurST(g, vict) >= 1 && char_get_position(vict) != POS_SLEEPING))
      return false;
    if (!char_aff_flagged(ch, AFF_ZANZOKEN) || char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, vict) + rand_number(g, 1, 5)) {
      reveal_hiding(g, ch, 0);
      act_triple(g, msgs.dodged, ch, nullptr, vict);
      char_aff_set(ch, AFF_ZANZOKEN, false);
      char_aff_set(vict, AFF_ZANZOKEN, false);
      decCurST(g, ch, cost, 0);
      char_set_wait(ch, PULSE_4SEC);
      return true;
    }
    reveal_hiding(g, ch, 0);
    act_triple(g, msgs.caught, ch, nullptr, vict);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    char_aff_set(ch, AFF_ZANZOKEN, false);
    return false;
  }

  /* Ends whatever hold ch has on vict, or vict has on ch */
  void release_grapple(struct char_data *holder, struct char_data *held)
  {
    char_set_grap(holder, -1);
    char_set_grappling(holder, nullptr);
    char_set_grappled(held, nullptr);
    char_set_grap(held, -1);
  }

  void release_absorb(struct char_data *holder, struct char_data *held)
  {
    char_set_absorbing(holder, nullptr);
    char_set_absorbby(held, nullptr);
  }

  /* The holds grapple takes, by name */
  struct grapple_hold {
    const char *name;
    int grap;
    act_msgs msgs;
    bool revealed; /* whether ch comes out of hiding to do it */
  };
  constexpr grapple_hold GRAPPLE_HOLDS[] = {
      {"hold", 1, {"@RYou rush at @r$N@R and manage to get $M in a hold from behind!@n", "@r$n@R rushes at YOU and manages to get you in a hold from behind!@n", "@r$n@R rushes at @r$N@R and manages to get $M in a hold from behind!@n", true}, true},
      {"choke", 2, {"@RYou rush at @r$N@R and manage to grab $S throat with both hands!@n", "@r$n@R rushes at YOU and manages to grab your throat with both hands!@n", "@r$n@R rushes at @r$N@R and manages to grab $S throat with both hands!@n", true}, true},
      {"wrap", 4, {"@MMoving quickly you stretch your body out and wrap it around the length of @c$N's@M body! You tighten your body until you begin crushing @c$N@M!", "@C$n@M quickly stretches out $s body and wraps it around @RYOU@M! You feel $s body begin to crush your own!@n", "@C$n@M quickly stretches out $s body and wraps it around @c$N@M! It appears that @c$N's@M body is being crushed slowly!@n", true}, false},
      {"grab", 3, {"@RYou rush at @r$N@R and manage to lock your arm onto $S!@n", "@r$n@R rushes at YOU and manages to lock $s arm onto your's!@n", "@r$n@R rushes at @r$N@R and manages to lock $s arm onto @r$N's@R!@n", true}, true},
  };

  constexpr act_msgs OVERPOWERED{"@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", "@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", "@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", true};
  constexpr act_msgs AVOIDED{"@RYou try to grapple with @r$N@R, but $E manages to avoid it!@n", "@r$n@R tries to grapple with YOU, but you manage to avoid it!@n", "@r$n@R tries to grapple with @r$N@R, but $E manages to avoid it!@n", true};
  constexpr zanzoken_msgs GRAPPLE_ZANZOKEN{
      {"@C$N@c disappears, avoiding your grapple attempt before reappearing!@n", "@cYou disappear, avoiding @C$n's@c grapple attempt before reappearing!@n", "@C$N@c disappears, avoiding @C$n's@c grapple attempt before reappearing!@n"},
      {"@C$N@c disappears, trying to avoid your grapple but your zanzoken is faster!@n", "@cYou zanzoken to avoid the grapple attempt but @C$n's@c zanzoken is faster!@n", "@C$N@c disappears, trying to avoid @C$n's@c grapple attempt but @C$n's@c zanzoken is faster!@n"},
  };

  /* The end of every attempt to grapple that gets as far as a try */
  void grapple_tried(GameWorld *g, struct char_data *ch, int64_t cost)
  {
    decCurST(g, ch, cost, 0);
    improve_skill(g, ch, SKILL_GRAPPLE, 1);
    char_set_wait(ch, PULSE_4SEC);
  }

  void grapple_fails(GameWorld *g, struct char_data *ch, struct char_data *vict, const act_msgs &msgs, int64_t cost)
  {
    reveal_hiding(g, ch, 0);
    act_triple(g, msgs, ch, nullptr, vict);
    grapple_tried(g, ch, cost);
  }

  /* Why ch can't grapple at all just now, if they can't */
  const char *cannot_grapple(GameWorld *g, struct char_data *ch)
  {
    if (char_plr_flagged(ch, PLR_THANDW))
      return "Your are too busy wielding your weapon with two hands!\r\n";
    if (char_get_absorbing(g, ch))
      return "You are currently absorbing from someone!\r\n";
    if (char_get_absorbby(g, ch))
      return "You are currently being absorbed by someone! Try 'escape'!\r\n";
    return nullptr;
  }
} // namespace

ACMD(do_grapple)
{
  if (!know_skill(g, ch, SKILL_GRAPPLE) || playing_song(g, ch))
    return;
  if (const char *why = cannot_grapple(g, ch)) {
    send_to_char(g, ch, why);
    return;
  }
  if (struct char_data *held = char_get_grappling(g, ch)) {
    act_triple(g, {"@RYou stop grappling with @r$N@R!@n", "@r$n@R stops grappling with @rYOU!!@n", "@r$n@R stops grappling with @r$N@R!@n", true}, ch, nullptr, held);
    release_grapple(ch, held);
    return;
  }
  if (char_get_grappled(g, ch) != nullptr) {
    send_to_char(g, ch, "You are currently a victim of grappling! Try 'escape' to break free!\r\n");
    return;
  }
  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: grapple (target) (hold | choke | grab)\r\n");
    return;
  }
  struct char_data *vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!vict && !(vict = char_get_fighting(g, ch))) {
    send_to_char(g, ch, "That target isn't here.\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  const char *refusal = nullptr;
  if (char_aff_flagged(vict, AFF_KNOCKED))
    refusal = "They are unconcious. What would be the point?\r\n";
  else if (char_get_grappled(g, vict) || char_get_absorbby(g, vict))
    refusal = "They are currently in someone else's grasp!\r\n";
  else if (char_get_absorbing(g, vict))
    refusal = "They are currently absorbing from someone!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  auto hold = std::ranges::find_if(GRAPPLE_HOLDS, [&](const grapple_hold &h) { return str_eq_nocase(h.name, arg2); });
  if (hold == std::end(GRAPPLE_HOLDS)) {
    send_to_char(g, ch, "Syntax: grapple (target) (hold | choke | grab | wrap)\r\n");
    return;
  }

  int perc = char_get_skill(ch, SKILL_GRAPPLE), prob = axion_dice(g, 0);
  int64_t cost = getMaxST(g, ch) / 100;
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough stamina to grapple!\r\n");
    return;
  }
  if (zanzoken_escape(g, ch, vict, GRAPPLE_ZANZOKEN, cost))
    return;
  perc += speed_modifier(g, ch, vict);
  if (getCurPL(g, ch) * 0.02 * char_get_aff_abils_str(ch) < getCurPL(g, vict) * 0.01 * char_get_aff_abils_str(vict) ||
      (getCurPL(g, ch) * 0.01 * char_get_aff_abils_str(ch) < getCurPL(g, vict) * 0.01 * char_get_aff_abils_str(vict) && rand_number(g, 1, 4) == 1)) {
    grapple_fails(g, ch, vict, OVERPOWERED, cost);
    return;
  }
  if (perc < prob) {
    grapple_fails(g, ch, vict, AVOIDED, cost);
    return;
  }
  if (hold->grap == 3 && !char_has_arms(g, vict)) {
    send_to_char(g, ch, "They don't even have an arm to grab onto!\r\n");
    return;
  }
  if (hold->grap == 4 && !char_is_majin(ch)) {
    send_to_char(g, ch, "Your body is not flexible enough to wrap around a target!\r\n");
    return;
  }
  if (hold->revealed)
    reveal_hiding(g, ch, 0);
  act_triple(g, hold->msgs, ch, nullptr, vict);
  char_set_grappling(ch, vict);
  char_set_grap(ch, hold->grap);
  char_set_grappled(vict, ch);
  char_set_grap(vict, hold->grap);
  grapple_tried(g, ch, cost);
}

namespace {
  constexpr zanzoken_msgs TRIP_ZANZOKEN{
      {"@C$N@c disappears, avoiding your trip before reappearing!@n", "@cYou disappear, avoiding @C$n's@c trip before reappearing!@n", "@C$N@c disappears, avoiding @C$n's@c trip before reappearing!@n"},
      {"@C$N@c disappears, trying to avoid your trip but your zanzoken is faster!@n", "@cYou zanzoken to avoid the trip but @C$n's@c zanzoken is faster!@n", "@C$N@c disappears, trying to avoid @C$n's@c trip but @C$n's@c zanzoken is faster!@n"},
  };
} // namespace

ACMD(do_trip)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!check_skill(g, ch, SKILL_TRIP) && !char_is_npc(ch))
    return;
  int64_t cost = getEffMaxPL(g, ch) / 200;
  if (cost > getCurST(g, ch)) {
    send_to_char(g, ch, "You don't have enough stamina.\r\n");
    return;
  }
  int perc = init_skill(g, ch, SKILL_TRIP), prob = rand_number(g, 1, 114);
  if (perc == 0)
    perc = char_get_level(ch) + rand_number(g, 1, 10);

  struct char_data *vict = arg.empty() ? nullptr : get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!vict) {
    struct char_data *foe = char_get_fighting(g, ch);
    if (!foe || char_get_in_room(foe) != char_get_in_room(ch)) {
      send_to_char(g, ch, "That target isn't here.\r\n");
      return;
    }
    vict = foe;
  }
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  const char *refusal = nullptr;
  if (char_aff_flagged(vict, AFF_FLYING))
    refusal = "They are flying and are not on their feet!\r\n";
  else if (char_get_position(vict) == POS_SITTING)
    refusal = "They are not on their feet!\r\n";
  else if (char_plr_flagged(vict, PLR_HEALT))
    refusal = "They are inside a healing tank!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  perc += speed_modifier(g, ch, vict);
  if (zanzoken_escape(g, ch, vict, TRIP_ZANZOKEN, cost))
    return;

  reveal_hiding(g, ch, 0);
  if (perc < prob) {
    act_triple(g, {"@mYou move to trip $N@m, but you screw up and $E keeps $S footing!@n", "@m$n@m moves to trip YOU, but $e screws up and you manage to keep your footing!@n", "@m$n@m moves to trip $N@m, but $e screws up and $N@m manages to keep $S footing!@n", true}, ch, nullptr, vict);
    improve_skill(g, ch, SKILL_TRIP, 0);
    decCurST(g, ch, cost, 0);
    char_set_wait(ch, PULSE_4SEC);
    if (char_get_fighting(g, ch) != vict)
      set_fighting(g, ch, vict);
    if (char_get_fighting(g, vict) != ch)
      set_fighting(g, vict, ch);
    return;
  }
  act_triple(g, {"@mYou move to trip $N@m, and manage to knock $M off $S feet!@n", "@m$n@m moves to trip YOU, and manages to knock you off your feet!@n", "@m$n@m moves to trip $N@m, and manages to knock $N@m off $S feet!@n", true}, ch, nullptr, vict);
  improve_skill(g, ch, SKILL_TRIP, 0);
  decCurST(g, ch, cost, 0);
  char_set_position(vict, POS_SITTING);
  char_set_wait(ch, PULSE_4SEC);
  if (char_get_fighting(g, ch) == nullptr)
    set_fighting(g, ch, vict);
  if (char_get_fighting(g, vict) == nullptr)
    set_fighting(g, vict, ch);
}

ACMD(do_rip)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Rip the tail off who?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "That target isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (!char_plr_flagged(vict, PLR_TAIL) && !char_plr_flagged(vict, PLR_STAIL)) {
    send_to_char(g, ch, "They do not have a tail to rip off!\r\n");
    return;
  }
  if (ch == vict) {
    reveal_hiding(g, ch, 0);
    act(g, "@rYou grab your own tail and yank it off!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n@r grabs $s own tail and yanks it off!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_lose_tail(g, vict);
    return;
  }
  if (getCurST(g, ch) < getMaxST(g, ch) / 20) {
    send_to_char(g, ch, "You are too tired to manage to grab their tail!\r\n");
    return;
  }
  if (char_get_position(ch) <= POS_SLEEPING) { /* reaching over in their sleep; quirk: costs stamina by their max ki */
    decCurST(g, ch, getMaxKI(g, ch) / 20, 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"@rYou reach and grab @R$N's@r tail! With a powerful tug you pull it off!@n", "@RYou feel your tail pulled off!@n", "@R$n@R reaches and grabs @R$N's@r tail! With a powerful tug $e pulls it off!@n", true}, ch, nullptr, vict);
    char_lose_tail(g, vict);
    return;
  }
  decCurST(g, ch, getMaxST(g, ch) / 20, 0);
  reveal_hiding(g, ch, 0);
  if (char_get_speedi(g, ch) <= char_get_speedi(g, vict))
    act_triple(g, {"@rYou rush at @R$N@r and try to grab $S tail, but fail!@n", "@R$n@r rushes at YOU and tries to grab your tail, but fails!@n", "@R$n@R rushes at @R$N@r and tries to grab $S tail, but fails!@n", true}, ch, nullptr, vict);
  else if (getCurPL(g, ch) > getCurPL(g, vict) * 2) {
    act_triple(g, {"@rYou rush at @R$N@r and grab $S tail! With a powerful tug you pull it off!@n", "@R$n@r rushes at YOU and grabs your tail! With a powerful tug $e pulls it off!@n", "@R$n@R rushes at @R$N@r and grab $S tail! With a powerful tug $e pulls it off!@n", true}, ch, nullptr, vict);
    char_lose_tail(g, vict);
  } else
    act_triple(g, {"@rYou rush at @R$N@r and grab $S tail! You are too weak to pull it off though!@n", "@R$n@r rushes at YOU and grabs your tail! $e is too weak to pull it off though!@n", "@R$n@R rushes at @R$N@r and grab $S tail! $e is too weak to pull it off though!@n", true}, ch, nullptr, vict);
}

ACMD(do_drag)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch))
    return;
  if (struct char_data *dragged = char_get_drag(g, ch)) {
    char_set_drag(ch, nullptr);
    char_set_dragged(dragged, nullptr);
    act(g, "@wYou stop dragging @C$N@W.@n", true, ch, nullptr, dragged, TO_CHAR);
    act(g, "@C$n@W stops dragging @c$N@W.@n", true, ch, nullptr, dragged, TO_ROOM);
    return;
  }
  const char *refusal = nullptr;
  if (char_plr_flagged(ch, PLR_PILOTING))
    refusal = "You are busy piloting a ship!\r\n";
  else if (char_get_carrying(g, ch))
    refusal = "You are busy carrying someone at the moment.\r\n";
  else if (arg.empty())
    refusal = "Who do you want to drag?\r\n";
  else if (char_get_fighting(g, ch))
    refusal = "You are a bit busy fighting right now!\r\n";
  else if (room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM || room_sect(g, char_get_in_room(ch)) == SECT_WATER_SWIM)
    refusal = "You decide to not be a tugboat instead.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Drag who?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (vict == ch)
    refusal = "You can't drag yourself.\r\n";
  else if (char_get_dragged(g, vict))
    refusal = "They are already being dragged!\r\n";
  else if (char_is_npc(vict) && char_mob_flagged(vict, MOB_NOKILL))
    refusal = "They are not to be touched!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  reveal_hiding(g, ch, 0);
  if (char_get_position(vict) != POS_SLEEPING) {
    act_triple(g, {"@wYou try to grab and pull @C$N@W with you, but $E resists!@n", "@C$n@W tries to grab and pull you! However you resist!@n", "@C$n@W tries to grab and pull @c$N@W but $E resists!@n", true}, ch, nullptr, vict);
    if (char_is_npc(vict) && !char_get_fighting(g, vict))
      set_fighting(g, vict, ch);
    return;
  }
  if (char_get_pc_weight(g, vict) + char_get_carry_weight(vict) > getMaxCarryWeight(g, ch)) {
    act(g, "@wYou try to grab and pull @C$N@W with you, but $E is too heavy!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@C$n@W tries to grab and pull @c$N@W but $E is too heavy!@n", true, ch, nullptr, vict, TO_ROOM);
    return;
  }
  act_triple(g, {"@wYou grab and start dragging @C$N@W.@n", nullptr, "@C$n@W grabs and starts dragging @c$N@W.@n", true}, ch, nullptr, vict);
  char_set_drag(ch, vict);
  char_set_dragged(vict, ch);
  /* quirk: the roll is never 0, so whoever isn't knocked out or magically asleep always feels it */
  if (!char_aff_flagged(vict, AFF_KNOCKED) && !char_aff_flagged(vict, AFF_SLEEP) && rand_number(g, 1, 3)) {
    send_to_char(g, vict, "You feel your sleeping body being moved.\r\n");
    if (char_is_npc(vict) && !char_get_fighting(g, vict))
      set_fighting(g, vict, ch);
  }
}

ACMD(do_stop)
{
  if (char_is_npc(ch))
    return;
  if (!char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are not even fighting!\r\n");
    return;
  }
  act(g, "@CYou move out of your fighting posture.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@c$n@C moves out of $s fighting posture.@n", true, ch, nullptr, nullptr, TO_ROOM);
  stop_fighting(g, ch);
  char_set_wait(ch, PULSE_2SEC);
}

namespace {
  /* Said by the holder (ch of the act) of the held (its victim): to the rest of the room, the held, then the holder */
  constexpr act_msgs BROKE_LOOSE{"@c$N@W manages to break loose of your hold!@n", "@WYou manage to break loose of @C$n's@W hold!@n", "@c$N@W manages to break loose of @C$n's@W hold!@n", true};
  constexpr act_msgs STRUGGLES{"@c$N@W struggles to break loose of your hold!@n", "@WYou struggle to break loose of @C$n's@W hold!@n", "@c$N@W struggles to break loose of @C$n's@W hold!@n", true};

  void tell_of_hold(GameWorld *g, const act_msgs &m, struct char_data *holder, struct char_data *held)
  {
    act(g, m.to_room, m.hide, holder, nullptr, held, TO_NOTVICT);
    act(g, m.to_vict, m.hide, holder, nullptr, held, TO_VICT);
    act(g, m.to_char, m.hide, holder, nullptr, held, TO_CHAR);
  }

  /* After a failed try, a one in three chance of hurting the holder enough to break free if it knocks them out */
  template <class Holder, class Release>
  void struggle(GameWorld *g, struct char_data *ch, struct char_data *holder, Holder holder_now, Release release)
  {
    tell_of_hold(g, STRUGGLES, holder, ch);
    if (rand_number(g, 1, 3) == 3) {
      hurt(g, 0, 0, ch, holder, nullptr, getEffMaxPL(g, ch) * 0.025, 0);
      /* the blow can end the hold itself, knocking out or killing the holder; it used to crash here then */
      struct char_data *still = holder_now();
      if (still && char_get_position(still) == POS_SLEEPING) {
        tell_of_hold(g, BROKE_LOOSE, still, ch);
        release(still);
      }
    }
    char_set_wait(ch, PULSE_2SEC);
  }

  void fight_back(GameWorld *g, struct char_data *ch, struct char_data *holder)
  {
    if (char_get_fighting(g, ch) == nullptr)
      set_fighting(g, ch, holder);
    if (char_get_fighting(g, holder) == nullptr)
      set_fighting(g, holder, ch);
  }
} // namespace

ACMD(do_escape)
{
  if (!char_get_absorbby(g, ch) && !char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are not in anyone's grasp!\r\n");
    return;
  }
  int num                    = char_get_aff_abils_str(ch);
  struct char_data *offender = char_get_absorbby(g, ch) ? char_get_absorbby(g, ch) : char_get_grappled(g, ch);
  int64_t opl = getCurPL(g, offender), dpl = getCurPL(g, ch);
  int skill = char_get_absorbby(g, ch) ? char_get_skill(char_get_absorbby(g, ch), SKILL_ABSORB) : char_get_skill(char_get_grappled(g, ch), SKILL_GRAPPLE);

  if (dpl > opl * 10)
    num += rand_number(g, 10, 15);
  else if (dpl > opl * 5)
    num += rand_number(g, 6, 10);
  else if (dpl > opl * 2)
    num += rand_number(g, 4, 8);
  else if (dpl > opl)
    num += rand_number(g, 2, 5);
  else if (dpl * 10 <= opl)
    skill -= rand_number(g, 10, 15);
  else if (dpl * 5 <= opl)
    skill -= rand_number(g, 6, 10);
  else if (dpl * 2 <= opl)
    skill -= rand_number(g, 4, 8);
  else if (dpl < opl)
    skill -= rand_number(g, 2, 5);
  bool escaped = (skill - num) < axion_dice(g, 0);

  if (struct char_data *absorber = char_get_absorbby(g, ch)) {
    if (escaped) {
      tell_of_hold(g, BROKE_LOOSE, absorber, ch);
      fight_back(g, ch, absorber);
      release_absorb(absorber, ch);
    } else
      struggle(g, ch, absorber, [&] { return char_get_absorbby(g, ch); }, [&](char_data *holder) { release_absorb(holder, ch); });
  }

  if (struct char_data *grappler = char_get_grappled(g, ch)) {
    if (!escaped) {
      struggle(g, ch, grappler, [&] { return char_get_grappled(g, ch); }, [&](char_data *holder) { release_grapple(holder, ch); });
      return;
    }
    if (char_get_grap(grappler) == 4) {
      act_triple(g, {"@c$N@M flexes with all $S might and causes your body to explode outward into gooey chunks!@n", "@MYou flex with all your might and cause @C$n's@M body to explode outward into gooey chunks!@n", "@c$N@M flexes with all $S might and causes @C$n's@M body to explode outward into gooey chunks!@n", true}, grappler, nullptr, ch);
      act(g, "@MYou reform your body mere moments later.@n", true, grappler, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@M reforms $s body mere moments later.", true, grappler, nullptr, nullptr, TO_ROOM);
    } else
      tell_of_hold(g, BROKE_LOOSE, grappler, ch);
    fight_back(g, ch, grappler);
    release_grapple(grappler, ch);
  }
}

namespace {
  void stop_blocking(GameWorld *g, struct char_data *ch, struct char_data *blocked)
  {
    act_triple(g, {"@wYou stop blocking @c$N@w.@n", "@C$n@w stops blocking you.@n", "@C$n@w stops blocking @c$N@w.@n", true}, ch, nullptr, blocked);
    char_set_blocked(blocked, nullptr);
    char_set_blocks(ch, nullptr);
  }
} // namespace

ACMD(do_block)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch))
    return;
  struct char_data *blocking = char_get_blocks(g, ch);
  if (arg.empty()) {
    if (blocking)
      stop_blocking(g, ch, blocking);
    else
      send_to_char(g, ch, "You want to block who?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "You do not see the target here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  const char *refusal    = nullptr;
  if (blocking == vict)
    refusal = "They are already blocked by you!\r\n";
  else if (ch == vict)
    refusal = "You can't block yourself, are you mental?\r\n";
  else if (char_get_blocked(g, vict))
    refusal = "They are already blocked by someone else!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (blocking)
    stop_blocking(g, ch, blocking);
  char_set_blocks(ch, vict);
  char_set_blocked(vict, ch);
  reveal_hiding(g, ch, 0);
  act_triple(g, {"@wYou start blocking @c$N's@w escape.@n", "@C$n@w starts blocking your escape.@n", "@C$n@w starts blocking @c$N's@w escape.@n", true}, ch, nullptr, vict);
}

namespace {
  /* Why obj can't be stolen, if it can't; worn things are checked a little differently */
  const char *unstealable(GameWorld *g, struct obj_data *obj, bool worn)
  {
    constexpr const char *CANT = "You can't steal that!\r\n", *KEYS = "No stealing keys!\r\n";
    obj_vnum vnum = obj_get_vnum(g, obj);
    /* quirk: only worn things are kept safe by these two ranges */
    if (vnum >= 20000 || (worn && ((vnum >= 18800 && vnum <= 18999) || (vnum >= 19100 && vnum <= 19199))))
      return CANT;
    /* quirk: a worn key is called a key, a carried one that can't be stolen isn't */
    if (worn && obj_get_type_flag(obj) == ITEM_KEY)
      return KEYS;
    if (obj_extra_flagged(obj, ITEM_NOSTEAL))
      return CANT;
    if (obj_get_type_flag(obj) == ITEM_KEY)
      return KEYS;
    return nullptr;
  }

  /* Why ch can't take obj from vict, if they can't */
  const char *cannot_take(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj, bool worn)
  {
    if (!give_otrigger(g, obj, vict, ch) || !receive_mtrigger(g, ch, vict, obj))
      return "Impossible!\r\n";
    if (const char *why = unstealable(g, obj, worn))
      return why;
    if (obj_get_weight(obj) + getCurCarriedWeight(ch) > getMaxCarryWeight(g, ch))
      return "You can't carry that much weight.\r\n";
    if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch))
      return "You don't have the room for it right now!\r\n";
    return nullptr;
  }

  /* Caught at it: ch waits, an npc fights, and both learn from it */
  void caught_stealing(GameWorld *g, struct char_data *ch, struct char_data *vict, bool stands)
  {
    char_set_wait(ch, PULSE_3SEC);
    if (char_is_npc(vict)) {
      if (stands)
        char_set_position(vict, POS_STANDING);
      set_fighting(g, vict, ch);
    }
    improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 2);
    improve_skill(g, vict, SKILL_SPOT, 1);
  }

  void theft_logged(GameWorld *g, struct char_data *ch, struct char_data *vict, std::string_view what)
  {
    if (char_is_npc(vict))
      return;
    char_plr_set(vict, PLR_STOLEN, true);
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "THEFT: {} has stolen {}@n from {}", char_get_name(ch), what, char_get_name(vict));
  }

  void steal_zenni(GameWorld *g, struct char_data *ch, struct char_data *vict, int prob, int perc)
  {
    if (prob <= perc) {
      reveal_hiding(g, ch, 0);
      act_triple(g, {"@rYou are caught trying to steal zenni from @R$N@r!@n", "@rYou catch @R$n's@r hand trying to snatch your zenni!@n", "@R$N@r catches @R$n's@r hand trying to snatch $S zenni!@n", true}, ch, nullptr, vict);
      caught_stealing(g, ch, vict, false);
      return;
    }
    if (char_get_gold(vict) <= 0) {
      send_to_char(g, ch, "It appears like they are broke...\r\n");
      return;
    }
    int gold = char_get_gold(vict) > 100 ? (char_get_gold(vict) / 100) * rand_number(g, 1, 10) : char_get_gold(vict);
    if (gold + char_get_gold(ch) > char_gold_carry(ch)) {
      send_to_char(g, ch, "You can't hold that much more zenni on your person!\r\n");
      return;
    }
    char_set_gold(vict, char_get_gold(vict) - gold);
    char_set_gold(ch, char_get_gold(ch) + gold);
    theft_logged(g, ch, vict, str_format("{} zenni", add_commas(gold)));
    if (gold > 1)
      send_to_char(g, ch, "Bingo!  You got {:d} zenni.\r\n", gold);
    else
      send_to_char(g, ch, "You manage to swipe a solitary zenni.\r\n");
    if (axion_dice(g, 0) > prob) {
      send_to_char(g, ch, "You think that your movements might have been a bit obvious.\r\n");
      reveal_hiding(g, ch, 0);
      act(g, "@R$n@r just stole zenni from @R$N@r!@n", true, ch, nullptr, vict, TO_ROOM);
      send_to_char(g, vict, "You feel like something may be missing...\r\n");
      if (char_is_npc(vict) && rand_number(g, 1, 3) == 3)
        set_fighting(g, vict, ch);
      improve_skill(g, vict, SKILL_SPOT, 1);
    }
    improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 1);
  }

  /* Something vict wears, which can only be taken while they sleep */
  void steal_worn(GameWorld *g, struct char_data *ch, struct char_data *vict, std::string &arg, int prob, int perc)
  {
    int eq_pos = 0;
    while (eq_pos < NUM_WEARS && !(char_get_equipment(vict, eq_pos) && isname(arg, obj_get_name(char_get_equipment(vict, eq_pos))) && char_can_see_obj(g, ch, char_get_equipment(vict, eq_pos))))
      eq_pos++;
    if (eq_pos == NUM_WEARS) {
      act(g, "$E isn't wearing that item.", false, ch, nullptr, vict, TO_CHAR);
      return;
    }
    struct obj_data *obj = char_get_equipment(vict, eq_pos);
    if (char_get_position(vict) > POS_SLEEPING) {
      send_to_char(g, ch, "Steal worn equipment from them while they are awake? That's a stupid idea...\r\n");
      return;
    }
    if (const char *why = cannot_take(g, ch, vict, obj, true)) {
      send_to_char(g, ch, why);
      return;
    }
    if (prob <= perc) {
      reveal_hiding(g, ch, 0);
      char_set_position(vict, POS_SITTING);
      act_triple(g, {"@rYou are caught trying to steal $p@r from @R$N@r!@n", "@rYou feel your body being shifted while you sleep and wake up to find @R$n@r trying to steal $p@r from you!@n", "@R$N@r catches @R$n's@r trying to $p@r from $M during $S sleep!@n", true}, ch, obj, vict);
      caught_stealing(g, ch, vict, true);
      return;
    }
    act(g, "You unequip $p and steal it.", false, ch, obj, vict, TO_CHAR);
    if (axion_dice(g, 0) > prob) {
      send_to_char(g, ch, "You think that your movements might have been a bit obvious.\r\n");
      reveal_hiding(g, ch, 0);
      act(g, "@R$n@r just stole $p@r from @R$N@r!@n", true, ch, obj, vict, TO_ROOM);
      send_to_char(g, vict, "You feel your body being disturbed.\r\n");
      improve_skill(g, vict, SKILL_SPOT, 1);
    }
    /* quirk: unlike anything else stolen, this isn't logged and doesn't mark the victim */
    obj_to_char(g, unequip_char(g, vict, eq_pos), ch);
    improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 1);
  }

  void steal_carried(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int prob, int perc)
  {
    if (const char *why = cannot_take(g, ch, vict, obj, false)) {
      send_to_char(g, ch, why);
      return;
    }
    if (prob <= perc) {
      reveal_hiding(g, ch, 0);
      act_triple(g, {"@rYou are caught trying to steal $p@r from @R$N@r!@n", "@rYou catch @R$n@r trying to steal $p@r from you!@n", "@R$N@r catches @R$n's@r trying to $p@r!@n", true}, ch, obj, vict);
      caught_stealing(g, ch, vict, true);
      return;
    }
    act(g, "You steal $p from $N.", false, ch, obj, vict, TO_CHAR);
    obj_from_char(g, obj);
    obj_to_char(g, obj, ch);
    theft_logged(g, ch, vict, obj_get_short_description(obj));
    if (axion_dice(g, 0) > prob) {
      reveal_hiding(g, ch, 0);
      send_to_char(g, ch, "You think that your movements might have been a bit obvious.\r\n");
      act(g, "@R$n@r just stole $p@r from @R$N@r!@n", true, ch, obj, vict, TO_ROOM);
      send_to_char(g, vict, "You feel like something may be missing...\r\n");
      if (char_is_npc(vict) && rand_number(g, 1, 3) == 3)
        set_fighting(g, vict, ch);
      improve_skill(g, vict, SKILL_SPOT, 1);
    }
    improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 1);
  }
} // namespace

ACMD(do_steal)
{
  if (!char_get_skill(ch, SKILL_SLEIGHT_OF_HAND)) {
    if (slot_count(ch) + 1 > char_get_skill_slots(ch)) {
      send_to_char(g, ch, "You can't learn any more skills and thus can not steal right now!\r\n");
      return;
    }
    send_to_char(g, ch, "You learn the very veeeery basics of theft. Which is don't get caught.\r\n");
    char_set_skills_base(ch, SKILL_SLEIGHT_OF_HAND, 1);
  }

  int prob              = char_get_skill(ch, SKILL_SLEIGHT_OF_HAND), perc = 0;
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (playing_song(g, ch))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "An important basic of theft is actually having a victim!\r\n");
    return;
  }
  auto found = find_char(g, ch, arg2, FIND_CHAR_ROOM, "Steal what from who?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (vict == ch) {
    send_to_char(g, ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  const char *refusal = nullptr;
  if (char_get_level(ch) <= 8)
    refusal = "You are trapped inside the newbie shield until level 9 and can't piss off those bigger and better than you. Awww...\r\n";
  else if (char_mob_flagged(vict, MOB_NOKILL) && char_get_admlevel(ch) == ADMLVL_NONE)
    refusal = "That isn't such a good idea...\r\n";
  else if (getCurST(g, ch) < (getMaxST(g, ch) / 40) + char_get_carry_weight(ch))
    refusal = "You do not have enough stamina.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }

  /* How watchful vict is (perc) against how deft ch is (prob) */
  if (!char_is_npc(vict) && char_get_skill(vict, SKILL_SPOT))
    perc = char_get_skill(vict, SKILL_SPOT) + char_get_aff_abils_intel(vict);
  else {
    perc = rand_number(g, char_get_aff_abils_intel(vict), char_get_aff_abils_intel(vict) + 10);
    if (char_is_npc(vict))
      perc += char_get_level(vict) * 0.25;
  }
  if (char_get_position(vict) == POS_SITTING)
    perc -= 5;
  if (char_get_position(vict) == POS_RESTING)
    perc -= 10;
  if (char_get_position(vict) <= POS_SLEEPING)
    perc -= 25;
  prob += char_get_aff_abils_dex(ch);
  perc += rand_number(g, -5, 5);
  prob += rand_number(g, -5, 5);

  bool awake = char_get_position(vict) != POS_SLEEPING;
  if (axion_dice(g, 0) > 100 && awake) { /* Unlucky! */
    reveal_hiding(g, ch, 0);
    act_triple(g, {"@r$N@R just happens to glance in your direction! What terrible luck!@n", "@RYou just happen to glance behind you and spot @r$n@R trying to STEAL from you!@n", "@r$N@R just happens to glance in @r$n's@R direction and catches $m trying to STEAL!@n", true}, ch, nullptr, vict);
    prob = -1000;
  }
  if (prob + 20 < perc && awake) { /* Critical failure! */
    reveal_hiding(g, ch, 0);
    act_triple(g, {"@rYou are caught trying to stick your hand in @R$N's@r possessions!@n", "@rYou catch @R$n@r trying to rummage through your possessions!@n", "@R$n@R is caught by @R$N@r as $e sticks $s hand in @R$N's@r possessions!@n", true}, ch, nullptr, vict);
    char_set_wait(ch, PULSE_3SEC);
    if (char_is_npc(vict))
      set_fighting(g, vict, ch);
    improve_skill(g, vict, SKILL_SPOT, 1);
    return;
  }
  if (str_eq_nocase(arg, "zenni"))
    steal_zenni(g, ch, vict, prob, perc);
  else if (struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(vict)))
    steal_carried(g, ch, vict, obj, prob, perc);
  else
    steal_worn(g, ch, vict, arg, prob, perc);
}
