/* abilities.c: racial and special abilities
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <climits>
#include <cstdlib>

#include <sys/stat.h>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
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
#include "dbat/act.offensive.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

ACMD(do_willpower)
{
  if (char_is_npc(ch))
    return;
  if (char_get_majinize(ch) <= 0) {
    send_to_char(g, ch, "You are not majinized and have no need to reclaim full control of your own will.\r\n");
    return;
  }
  /* each that's lacking is said */
  bool fail = false;
  if (char_get_practice_points(g, ch) < (char_get_level(ch) >= 100 ? 200 : 100)) {
    send_to_char(g, ch, "You do not have enough PS to focus your attempt to break free.\r\n");
    fail = true;
  }
  if (char_get_exp(ch) < level_exp(ch, char_get_level(ch) + 1) && char_get_level(ch) < 100) {
    send_to_char(g, ch, "You need a full level's worth of experience stored up to try and break free.\r\n");
    fail = true;
  }
  if (fail)
    return;

  auto pay = [&] {
    char_set_exp(ch, 0);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (char_get_level(ch) >= 100 ? 200 : 100));
  };
  pay();
  bool failed = rand_number(g, 10, 100) - char_get_aff_abils_intel(ch) > 60;
  if (failed) {
    reveal_hiding(g, ch, 0); /* which rolls dice of its own, so after the roll */
    act(g, "@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments you give up, having failed to overcome the majinization!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments $n seems to give up and the commotion dies down.@n", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  pay(); /* quirk: success costs it all again */
  reveal_hiding(g, ch, 0);
  act(g, "@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments the ground splits beneath you and while letting out a piercing scream the M disappears from your forehead! You are free while still keeping the boost you had recieved from the majinization!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments the ground beneath $n splits and $e lets out a piercing scream. The M on $s forehead disappears!@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_majinize(ch, 3);
}

ACMD(do_infuse)
{
  if (!know_skill(g, ch, SKILL_INFUSE))
    return;
  if (char_aff_flagged(ch, AFF_INFUSE)) {
    act(g, "You stop infusing ki into your attacks.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n stops infusing ki into $s attacks.", true, ch, nullptr, nullptr, TO_ROOM);
    char_aff_set(ch, AFF_INFUSE, false);
    return;
  }
  if (getCurKI(g, ch) < getMaxKI(g, ch) / 100) {
    send_to_char(g, ch, "You don't have enough ki to infuse into your attacks!\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  act(g, "You start infusing ki into your attacks.", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "$n starts infusing ki into $s attacks.", true, ch, nullptr, nullptr, TO_ROOM);
  char_aff_set(ch, AFF_INFUSE, true);
  decCurKI(g, ch, getMaxKI(g, ch) / 100, 0);
}

ACMD(do_paralyze)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!know_skill(g, ch, SKILL_PARALYZE))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Who are you wanting to paralyze?\r\n");
    return;
  }
  if (!limb_ok(g, ch, 0))
    return;
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "That target isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  if (char_aff_flagged(vict, AFF_PARA)) {
    send_to_char(g, ch, "They are already partially paralyzed!\r\n");
    return;
  }
  if (getCurKI(g, ch) < getCurPL(g, vict) / 10 + getMaxKI(g, ch) / 20) {
    send_to_char(g, ch, "You realize you can't paralyze them. You don't have enough ki to restrain them!\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_PARALYZE), perc = axion_dice(g, 0);
  if (char_get_speedi(g, ch) * 2 < char_get_speedi(g, vict))
    prob -= 10;
  if (char_get_speedi(g, ch) + char_get_speedi(g, ch) / 2 < char_get_speedi(g, vict))
    prob -= 5;
  int64_t cost = getCurPL(g, vict) / 6 + getMaxKI(g, ch) / 20;

  if (char_get_bonuses(vict, BONUS_INSOMNIAC)) {
    decCurKI(g, ch, cost, 0);
    act_triple(g, {"@RYou focus ki and point both your arms at @r$N@R. However $N seems to shake off your paralysis attack!@n", "@r$n @Rfocuses ki and points both $s arms at YOU! Your insomnia makes you immune to $s feeble paralysis attempt.@n", "@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $N seems to shake off $s paralysis attack!@n", true}, ch, nullptr, vict);
    return;
  }
  reveal_hiding(g, ch, 0);
  if (prob < perc)
    act_triple(g, {"@RYou focus ki and point both your arms at @r$N@R. However $E manages to avoid your attempt to paralyze $M!@n", "@r$n @Rfocuses ki and points both $s arms at YOU! You manage to avoid $s technique though...@n", "@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $E manages to avoid @r$n's@R attempted technique...@n", true}, ch, nullptr, vict);
  else {
    act_triple(g, {"@RYou focus ki and point both your arms at @r$N@R. Your ki flows into $S body and partially paralyzes $M!@n", "@r$n @Rfocuses ki and points both $s arms at YOU! You are caught in $s paralysis technique and now can barely move!@n", "@r$n @Rfocuses ki and points both $s arms at @r$N@R. @r$n's@R ki flows into @r$N@R body and partially paralyzes $M!@n", true}, ch, nullptr, vict);
    assign_affect(g, vict, AFF_PARA, SKILL_PARALYZE, char_get_aff_abils_intel(ch) / 15, 0, 0, 0, 0, 0, 0);
  }
  decCurKI(g, ch, cost, 0);
  improve_skill(g, ch, SKILL_PARALYZE, 0);
}

ACMD(do_taisha)
{
  if (char_is_npc(ch) || !know_skill(g, ch, SKILL_TAISHA))
    return;
  const char *refusal = nullptr;
  if (room_flagged(g, char_get_in_room(ch), ROOM_AURA))
    refusal = "This area already has an aura of regeneration around it.\r\n";
  else if (room_get_gravity(g, char_get_in_room(ch)) > 0)
    refusal = "This area's gravity is too hostile to an aura.\r\n";
  else if (getCurKI(g, ch) < getMaxKI(g, ch) / 3)
    refusal = "You don't have enough ki.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  int prob = char_get_skill(ch, SKILL_TAISHA), perc = axion_dice(g, 0);
  decCurKI(g, ch, getMaxKI(g, ch) / 3, 0);
  reveal_hiding(g, ch, 0);
  if (prob < perc) {
    act(g, "@WYou hold up your hands while channeling ki. Your technique fails to produce an aura though....@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@g$n@W holds up $s hands while channeling ki. $s technique fails to produce an aura though....", true, ch, nullptr, nullptr, TO_ROOM);
    improve_skill(g, ch, SKILL_TAISHA, 1);
    return;
  }
  act(g, "@WYou hold up your hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@g$n holds up $s hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", true, ch, nullptr, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_TAISHA, 1);
  room_set_flag(g, char_get_in_room(ch), ROOM_AURA, true);
}

ACMD(do_kura)
{
  if (!know_skill(g, ch, SKILL_KURA))
    return;
  if (getCurKI(g, ch) >= getMaxKI(g, ch)) {
    send_to_char(g, ch, "Your ki is already maxed out!\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: kuraiiro (1-100).\r\n");
    return;
  }
  int num = parse_int(arg), skill = char_get_skill(ch, SKILL_KURA);
  if (num > skill) {
    send_to_char(g, ch, "The number can not be greater than your skill.\r\n");
    return;
  }
  if (num <= 0) {
    send_to_char(g, ch, "The number can not be less than 1.\r\n");
    return;
  }
  int64_t cost = (getMaxKI(g, ch) / 100) * num;
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough stamina for that high a number.\r\n");
    return;
  }
  bool failed = skill <= axion_dice(g, 0);
  decCurST(g, ch, cost, 0);
  if (!failed)
    incCurKI(g, ch, cost, true);
  reveal_hiding(g, ch, 0);
  if (failed) {
    act(g, "You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies but you fail!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@w crouches down and screams as $s eyes turn red and $e attempts to tap into dark energies but fails!", true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies and succeed as a rush of energy explodes around you!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@w crouches down and screams as $s eyes turn red. Suddenly $e manages to tap into dark energies and a rush of energy explodes around $m!", true, ch, nullptr, nullptr, TO_ROOM);
  }
  improve_skill(g, ch, SKILL_KURA, 0);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_candy)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_is_majin(ch)) {
    send_to_char(g, ch, "You are not a majin, how can you do that?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Turn who into candy?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  int64_t ch_max = getMaxPLTrans(g, ch), vict_max = getMaxPLTrans(g, vict);
  const char *refusal = nullptr;
  if (!char_is_npc(vict))
    refusal = "You can't turn them into candy.\r\n";
  else if (vict_max > ch_max * 2)
    refusal = "They are too powerful.\r\n";
  else if ((vict_max < ch_max * .25 && char_get_level(ch) < 100) || (vict_max < ch_max * .09 && char_get_level(ch) == 100))
    refusal = "They are too weak.\r\n";
  else if (getCurKI(g, ch) < ch_max / 15)
    refusal = "You do not have enough ki.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  bool dodged = rand_number(g, 1, 6) == 6;
  decCurKI(g, ch, getMaxKI(g, ch) / 15, 0);
  reveal_hiding(g, ch, 0);
  if (dodged) {
    act_triple(g, {"@cYou aim your forelock at @R$N@c and fire a beam of energy but it is dodged!@n", nullptr, "@C$n@c aims $s forelock at @R$N@c and fires a beam of energy but the beam is dodged!@n", true}, ch, nullptr, vict);
    if (!char_get_fighting(g, ch))
      set_fighting(g, ch, vict);
    if (!char_get_fighting(g, vict))
      set_fighting(g, vict, ch);
    char_set_wait(ch, PULSE_3SEC);
    return;
  }
  act_triple(g, {"@cYou aim your forelock at @R$N@c and fire a beam of energy that envelopes $S entire body and changes $M into candy!@n", nullptr, "@C$n@c aims $s forelock at @R$N@c and fires a beam of energy that envelopes $S entire body and changes $M into candy!@n ", true}, ch, nullptr, vict);
  /* the better the candy, the stronger they were */
  obj_vnum candy = vict_max >= ch_max * 1.5 ? 95 : vict_max >= ch_max ? 94 : vict_max >= ch_max * (char_get_level(ch) < 100 ? .5 : .1) ? 93 : 53;
  send_to_char(g, ch, "You grab the candy as it falls.\r\n");
  obj_to_char(g, read_object(g, candy), ch);
  char_mob_set(vict, MOB_HUSK, true);
  die(g, vict, ch);
}

ACMD(do_future)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch) || !char_is_kanassan(ch)) {
    send_to_char(g, ch, "You are incapable of this ability.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Bestow advance future sight on who?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Bestow advance future sight on who?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  bool self              = vict == ch;
  const char *refusal    = nullptr;
  if (char_aff_flagged(vict, AFF_FUTURE))
    refusal = "They already can see the future.\r\n";
  else if (char_is_npc(vict))
    refusal = "You can't target them, there would be no point.\r\n";
  else if (getCurKI(g, ch) < getMaxKI(g, ch) / 40)
    refusal = "You do not have enough ki.\r\n";
  else if (char_get_practice_points(g, ch) < 100)
    refusal = "You do not have enough PS to activate or pass on this ability.\r\n";
  else if (char_get_real_abils_cha(vict) + 5 > 70 && char_get_bonuses(vict, BONUS_SLOW) > 0)
    refusal = self ? "You can't handle having your speed increased beyond 70.\r\n" : "They can't handle having their speed increased beyond 70.\r\n";
  else if (char_get_real_abils_intel(vict) + 2 > 70 && char_get_bonuses(vict, BONUS_DULL) > 0)
    refusal = self ? "You can't handle having your intelligence increased beyond 70.\r\n" : "They can't handle having their intelligence increased beyond 70.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - 100);
  reveal_hiding(g, ch, 0);
  if (self)
    act_triple(g, {"@CYou focus your energy into your mind and awaken your latent Future Sight powers!@n", "@C$n focuses $s energy while closing $s eyes for a moment.@n", "@C$n focuses $s energy while closing $s eyes for a moment.@n", true}, ch, nullptr, vict);
  else
    act_triple(g, {"@CYou focus your energy into your fingers before stabbing your claws into $N and bestowing the power of Future Sight upon $M. Shortly after $E passes out.@n", "@C$n focuses $s energy into $s fingers before stabbing $s claws into YOUR neck and bestowing the power of Future Sight upon you! Soon after you pass out!@n", "@C$n focuses $s energy into $s fingers before stabbing $s claws into $N's neck and bestowing the power of Future Sight upon $M! Soon after $E passes out!@n", true}, ch, nullptr, vict);
  char_aff_set(vict, AFF_FUTURE, true);
  char_set_real_abils_cha(vict, char_get_real_abils_cha(vict) + 5);
  char_set_real_abils_intel(vict, char_get_real_abils_intel(vict) + 2);
  char_set_position(vict, POS_SLEEPING);
  save_char(g, vict);
}

ACMD(do_suppress)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch))
    return;
  const char *refusal = nullptr;
  if (char_is_android(ch))
    refusal = "You are unable to suppress your powerlevel.\r\n";
  else if (char_get_bonuses(ch, BONUS_ARROGANT) > 0)
    refusal = "You are far too arrogant to hide your strength.\r\n";
  else if (char_plr_flagged(ch, PLR_POWERUP))
    refusal = "You are currently powering up, can't suppress.\r\n";
  else if (char_get_kaioken(ch))
    refusal = "You are currently concentrating on kaioken!\r\n";
  else if (arg.empty())
    refusal = "Suppress to what percent?\r\nSyntax: suppress (1 - 99 | release)\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (str_eq_nocase(arg, "release")) {
    if (!char_get_suppression(ch)) {
      send_to_char(g, ch, "You are not suppressing!\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@GYou stop suppressing your current powerlevel!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@G$n smiles as a rush of power erupts around $s body briefly.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_suppression(ch, 0);
    return;
  }
  int num = parse_int(arg);
  if (num > 99 || num <= 0) {
    send_to_char(g, ch, "Out of suppression range.\r\nSyntax: suppress (1 - 99 | release)\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  act(g, char_get_suppression(ch) != 0 ? "@GYou alter your suppression level!@n" : "@GYou suppress your current powerlevel!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@G$n seems to concentrate for a moment.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_suppression(ch, num);
}

ACMD(do_hass)
{
  if (!check_skill(g, ch, SKILL_HASSHUKEN))
    return;
  if (getCurST(g, ch) < getMaxST(g, ch) / 30) {
    send_to_char(g, ch, "You do not have enough stamina.\r\n");
    return;
  }
  int perc = init_skill(g, ch, SKILL_HASSHUKEN), prob = axion_dice(g, 0);
  reveal_hiding(g, ch, 0);
  if (perc < prob) {
    act(g, "@WYou try to move your arms at incredible speeds but screw up and waste some of your stamina.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W tries to move $s arms at incredible speeds but screws up and wastes some of $s stamina.@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "@WYou concentrate and start to move your arms at incredible speeds.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W concentrates and starts to move $s arms at incredible speeds.@n", true, ch, nullptr, nullptr, TO_ROOM);
    assign_affect(g, ch, AFF_HASS, SKILL_HASSHUKEN, perc / 15, 0, 0, 0, 0, 0, 0);
  }
  decCurST(g, ch, getMaxST(g, ch) / 30, 0);
  improve_skill(g, ch, SKILL_HASSHUKEN, 0);
}

namespace {
  /* The limbs implant replaces: its word, the limb's index, the flag it gets, and what it's called */
  constexpr struct {
    const char *word;
    int limb;
    int flag;
    const char *name;
  } IMPLANTS[] = {{"rarm", 0, PLR_CRARM, "right arm"}, {"larm", 1, PLR_CLARM, "left arm"}, {"rleg", 2, PLR_CRLEG, "right leg"}, {"lleg", 3, PLR_CLLEG, "left leg"}};
} // namespace

ACMD(do_implant)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (!limb_ok(g, ch, 0))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: implant (rarm | larm | rleg | lleg) (target)\r\n");
    return;
  }
  struct char_data *vict = ch;
  if (!arg2.empty()) {
    auto found = find_char(g, ch, arg2, FIND_CHAR_ROOM, "That person isn't here.\r\n");
    if (reject(g, ch, found))
      return;
    vict = *found;
  }
  struct obj_data *limb = find_first(char_inventory(g, ch), [&](obj_data *obj) {
    return obj_get_vnum(g, obj) == 66 && !obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED);
  });
  if (!limb) {
    send_to_char(g, ch, "You do not have a cybernetic limb to implant.\r\n");
    return;
  }
  auto implant = std::ranges::find_if(IMPLANTS, [&](const auto &i) { return arg == i.word; });
  if (implant == std::end(IMPLANTS)) {
    send_to_char(g, ch, "Syntax: implant (rarm | larm | rleg | rleg)\r\n");
    return;
  }
  if (char_get_limb_condition(vict, implant->limb) >= 1) {
    send_to_char(g, ch, "{} already have a {}!\r\n", vict == ch ? "You" : "They", implant->name);
    return;
  }
  reveal_hiding(g, ch, 0);
  if (vict != ch)
    act_triple(g, {str_format("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new {}!@n", implant->name).c_str(), str_format("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new {}!@n", implant->name).c_str(), str_format("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new {}!@n", implant->name).c_str(), true}, ch, limb, vict);
  else {
    act(g, str_format("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new {}!@n", implant->name).c_str(), true, ch, limb, nullptr, TO_CHAR);
    act(g, str_format("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new {}!@n", implant->name).c_str(), true, ch, limb, nullptr, TO_ROOM);
  }
  char_plr_set(vict, implant->flag, true);
  obj_from_char(g, limb);
  extract_obj(g, limb);
}

/* do_fury for halfbreeds to release their raaaage, rawrg! */
ACMD(do_fury)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  const char *refusal = nullptr;
  if (!char_is_halfbreed(ch) || char_is_npc(ch))
    refusal = "You are furious, but you'll get over it.\r\n";
  else if (char_plr_flagged(ch, PLR_FURY))
    refusal = "You are already furious, your next attack will devestate, hurry use it!\r\n";
  else if (char_get_fury(ch) < 100)
    refusal = "You do not have enough anger to release your fury upon your foes!\r\n";
  else if (!arg.empty() && !str_eq_nocase(arg, "attack"))
    refusal = "Syntax: fury (attack) <--- this will not use up your LF to restore PL.\n        fury <--- fury by itself will do both LF to PL restore and attack boost.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (arg.empty()) { /* life force to powerlevel, too */
    if (getCurPL(g, ch) < getEffMaxPL(g, ch)) {
      if (getCurLF(g, ch) >= getMaxLF(g, ch) * 0.2) {
        restoreHealth(ch, false);
        decCurLFPercent(g, ch, .2, 0);
      } else {
        incCurHealth(g, ch, getCurLF(g, ch), true);
        decCurLFPercent(g, ch, 2, -1);
      }
    }
    char_set_fury(ch, 0);
  } else
    char_set_fury(ch, 50);
  reveal_hiding(g, ch, 0);
  act(g, "You release your fury! Your very next attack is guaranteed to rip your foes a new one!", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "$n screams furiously as a look of anger appears on $s face!", true, ch, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_FURY, true);
}

ACMD(do_majinize)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_is_majin(ch)) {
    send_to_char(g, ch, "You are not a majin and can not majinize anyone.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Who do you want to majinize?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "That target isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  int alignmentTotal     = char_get_alignment(ch) - char_get_alignment(vict);
  bool ours              = char_get_majinize(vict) > 0 && char_get_majinize(vict) == char_get_id(ch);
  const char *refusal    = nullptr;
  if (char_is_npc(vict))
    refusal = "Why would you waste your time majinizing them?\r\n";
  else if (vict == ch)
    refusal = "You can't majinize yourself.\r\n";
  else if (char_plr_flagged(vict, PLR_PR))
    refusal = "You can't majinize them their potential has been released!\r\n";
  else if (char_get_majinize(vict) > 0 && !ours)
    refusal = "They are already majinized before by someone else.\r\n";
  else if (char_get_master(g, vict) != ch)
    refusal = "They must be following you in order for you to majinize them.\r\n";
  else if (alignmentTotal < -1500 || alignmentTotal > 1500)
    refusal = "Their alignment is so opposed to your's that they resist your attempts to enslave them!\r\n";
  else if (getEffMaxPL(g, vict) > getEffMaxPL(g, ch) * 4)
    refusal = "Their powerlevel is so much higher than yours they resist your attempts to enslave them!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (ours) { /* undone, taking back what it gave */
    reveal_hiding(g, ch, 0);
    act_triple(g, {"You remove $N's majinization, freeing them from your influence, but also weakening them.", "$n removes your majinization, freeing you from their influence, and weakening you!", "$n waves a hand at $N, and instantly the glowing M on $S forehead disappears!", true}, ch, nullptr, vict);
    char_set_majinize(vict, 0);
    char_set_boosts(ch, char_get_boosts(ch) + 1);
    if (char_get_majinizer(vict) == 0)
      char_set_majinizer(vict, getBasePL(vict) * .4);
    loseBasePL(vict, char_get_majinizer(vict), false);
    return;
  }
  if (char_get_boosts(ch) == 0) {
    send_to_char(g, ch, "You are incapable of majinizing{}.\r\n", char_get_level(ch) < 100 ? " right now" : " anymore");
    if (char_get_level(ch) < 100)
      send_to_char(g, ch, "Your next available majinize will be at level {:d}\r\n", (char_get_level(ch) / 25 + 1) * 25);
    return;
  }
  reveal_hiding(g, ch, 0);
  act_triple(g, {"You focus your power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", "$n focuses power into you, influencing your mind and increasing your strength! After the struggle in your mind ends a glowing purple M forms on your forehead.", "$n focuses power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", true}, ch, nullptr, vict);
  char_set_majinize(vict, char_get_id(ch));
  char_set_boosts(ch, char_get_boosts(ch) - 1);
  char_set_majinizer(vict, getBasePL(vict) * .4);
  gainBasePLPercent(vict, .4, true);
}

ACMD(do_spit)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!know_skill(g, ch, SKILL_SPIT))
    return;
  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You can't manage to spit in this fight!\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Yes but who do you want to petrify?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "That target isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  if (char_aff_flagged(vict, AFF_PARALYZE)) {
    act(g, "$N has already been turned to stone.", true, ch, nullptr, vict, TO_CHAR);
    return;
  }
  if (char_get_fighting(g, vict)) {
    send_to_char(g, vict, "You can't manage to spit on them, they are moving around too much!\r\n"); /* quirk: told to vict */
    return;
  }
  int cost = getMaxKI(g, ch) / MAX(char_get_skill(ch, SKILL_SPIT) / 4, 1) + getMaxKI(g, ch) / 100;
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to petrifiy with your spit!\r\n");
    return;
  }
  if (char_get_skill(ch, SKILL_SPIT) < axion_dice(g, 0)) {
    decCurKI(g, ch, cost, 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"@WGathering spit you concentrate ki into a wicked loogie and let it loose, but it falls short of hitting @c$N@W!@n", "@C$n@W seems to focus ki before hawking a loogie at you! Fortunatly the loogie falls short.@n", "@C$n@W seems to focus ki before hawking a loogie at @c$N@W! Fortunatly for @c$N@W the loogie falls short.@n", true}, ch, nullptr, vict);
    improve_skill(g, ch, SKILL_SPIT, 1);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  if (char_aff_flagged(vict, AFF_ZANZOKEN) && getCurST(g, vict) >= 1 && char_get_position(vict) != POS_SLEEPING) {
    decCurKI(g, ch, cost, 0);
    reveal_hiding(g, ch, 0);
    act_triple(g, {"@C$N@c disappears, avoiding your spit before reappearing!@n", "@cYou disappear, avoiding @C$n's@c @rstone spit@c before reappearing!@n", "@C$N@c disappears, avoiding @C$n's@c @rstone spit@c before reappearing!@n"}, ch, nullptr, vict);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    char_set_wait(ch, PULSE_2SEC);
    improve_skill(g, ch, SKILL_SPIT, 1);
    return;
  }
  struct affected_type af = {};
  af.type                 = SPELL_PARALYZE;
  af.duration             = rand_number(g, 1, 2);
  af.location             = APPLY_NONE;
  af.bitvector            = AFF_PARALYZE;
  affect_join(g, vict, &af, false, false, false, false);
  decCurKI(g, ch, cost, 0);
  reveal_hiding(g, ch, 0);
  act_triple(g, {"@WGathering spit you concentrate ki into a wicked loogie and let it loose, and it smacks into @c$N@W turning $M into stone!@n", "@C$n@W seems to focus ki before hawking a loogie at you! It manages to hit and you instantly turn to stone!@n", "@C$n@W seems to focus ki before hawking a loogie at @c$N@W! It manages to hit and $E instantly turns to stone!@n", true}, ch, nullptr, vict);
  improve_skill(g, ch, SKILL_SPIT, 1);
  char_set_wait(ch, PULSE_2SEC);
}

namespace {
  constexpr act_msgs ABSORB_AVOIDED{"@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", "@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", "@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true};

  void fight_each_other(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (!char_get_fighting(g, ch))
      set_fighting(g, ch, vict);
    if (!char_get_fighting(g, vict))
      set_fighting(g, vict, ch);
  }

  /* An android grabbing on to drain vict */
  void android_absorb(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (!char_is_npc(ch) && !char_plr_flagged(ch, PLR_ABSORB)) {
      send_to_char(g, ch, "You are not an absorbtion model.\r\n");
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0))
      return;
    if (char_get_absorbby(g, vict)) {
      send_to_char(g, ch, "{} is already absorbing from them!", char_get_name(char_get_absorbby(g, vict)));
      return;
    }
    const char *refusal = nullptr;
    if (getEffMaxPL(g, vict) > getEffMaxPL(g, ch) * 2)
      refusal = "They are too strong for you to absorb from.\r\n";
    else if (getEffMaxPL(g, vict) * 20 < getEffMaxPL(g, ch))
      refusal = "They are too weak for you to bother absorbing from.\r\n";
    else if (getCurST(g, vict) < getMaxST(g, vict) / 20 && getCurKI(g, vict) < getMaxKI(g, vict) / 20)
      refusal = "They have nothing to absorb right now, they are drained...\r\n";
    if (refusal) {
      send_to_char(g, ch, refusal);
      return;
    }
    reveal_hiding(g, ch, 0);
    if (init_skill(g, ch, SKILL_ABSORB) < axion_dice(g, 0)) {
      act_triple(g, ABSORB_AVOIDED, ch, nullptr, vict);
      improve_skill(g, ch, SKILL_ABSORB, 1);
      if (char_is_npc(vict) && char_is_humanoid(vict) && rand_number(g, 1, 3) == 3)
        fight_each_other(g, ch, vict);
    } else {
      act_triple(g, {"@WYou rush at @c$N@W and try to absorb from them, and manage to grab on!@n", "@C$n@W rushes at you and tries to grab you, and manages to grab on!@n", "@C$n@w rushes at @c$N@W and tries to grab $M, and manages to grab on!@n", true}, ch, nullptr, vict);
      improve_skill(g, ch, SKILL_ABSORB, 1);
      char_set_absorbing(ch, vict);
      char_set_absorbby(vict, ch);
    }
    char_set_wait(ch, PULSE_3SEC);
  }

  /* A bio-android swallowing vict whole with its tail */
  void bio_swallow(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (char_get_absorbby(g, vict)) {
      send_to_char(g, ch, "{} is already absorbing from them!", char_get_name(char_get_absorbby(g, vict)));
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0))
      return;
    if (char_get_absorbs(ch) < 1) {
      send_to_char(g, ch, "You already have already absorbed 3 people.\r\n");
      return;
    }
    if (getEffMaxPL(g, vict) >= getBasePL(ch) * 3) {
      send_to_char(g, ch, "You are too weak to absorb them into your cellular structure!\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    if (char_get_skill(ch, SKILL_ABSORB) < axion_dice(g, 0)) {
      act_triple(g, ABSORB_AVOIDED, ch, nullptr, vict);
      improve_skill(g, ch, SKILL_ABSORB, 1);
      fight_each_other(g, ch, vict);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    act_triple(g, {"@WYou rush at @c$N@W and your tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing $m!@n", "@C$n@W rushes at you and $s tail engulfs you! $e quickly sucks your squirming body into $s tail, absorbing you!@n", "@C$n@w rushes at @c$N@W and $s tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing @c$N@W!@n", true}, ch, nullptr, vict);
    char_set_absorbs(ch, char_get_absorbs(ch) - 1);
    int64_t stam = getBaseST(vict) / 5, ki = getBaseKI(vict) / 5, pl = getBasePL(vict) / 5;
    gainBasePL(ch, pl, true);
    gainBaseST(ch, stam, true);
    gainBaseKI(ch, ki, true);
    if (!char_is_npc(vict) && !char_is_npc(ch)) {
      send_to_imm(g, "[PK] {} killed {} at room [{:d}]\r\n", char_get_name(ch), char_get_name(vict), char_get_in_room(vict));
      char_plr_set(vict, PLR_ABSORBED, true);
    }
    send_to_char(g, ch, "@D[@gABSORB@D] @rPL@W: @D(@y{}@D) @cKi@W: @D(@y{}@D) @gSt@W: @D(@y{}@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
    improve_skill(g, ch, SKILL_ABSORB, 1);
    die(g, vict, nullptr);
  }

  /* A bio-android draining a mob's bio extract, leaving a husk */
  void bio_extract(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    int failthresh = rand_number(g, 1, 125);
    if (char_get_level(vict) > 99)
      failthresh += (char_get_level(vict) - 95) * 2;
    if (char_get_absorbby(g, vict)) {
      send_to_char(g, ch, "{} is already absorbing from them!", char_get_name(char_get_absorbby(g, vict)));
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0))
      return;
    const char *refusal = nullptr;
    if (getEffMaxPL(g, vict) >= getEffMaxPL(g, ch))
      refusal = "You are too weak to absorb them into your cellular structure!\r\n";
    else if (getEffMaxPL(g, vict) < getEffMaxPL(g, ch) / 5)
      refusal = "They would be worthless to you at your strength!\r\n";
    else if (!char_is_npc(vict))
      refusal = "You can't absorb their bio extract, you need to swallow them with your tail!\r\n";
    else if (is_soft_cap(ch, 0, 1.0))
      refusal = "You can not handle any more bio extract at your current level.\r\n";
    if (refusal) {
      send_to_char(g, ch, refusal);
      return;
    }
    if (char_get_skill(ch, SKILL_ABSORB) < failthresh) {
      act_triple(g, ABSORB_AVOIDED, ch, nullptr, vict);
      improve_skill(g, ch, SKILL_ABSORB, 0);
      fight_each_other(g, ch, vict);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    act_triple(g, {"@WYou rush at @c$N@W and stab them with your tail! You quickly suck out all the bio extract you need and leave the empty husk behind!", nullptr, "@C$n@w rushes at @c$N@W and stabs $M with $s tail! $e quickly sucks out all the bio extract and leaves the empty husk of @c$N@W behind!@n", true}, ch, nullptr, vict);
    int64_t stam = getBaseST(vict) / 2000, ki = getBaseKI(vict) / 2000, pl = getBasePL(vict) / 2000;
    stam += rand_number(g, char_get_level(ch), char_get_level(ch) * 2);
    pl += rand_number(g, char_get_level(ch), char_get_level(ch) * 2);
    ki += rand_number(g, char_get_level(ch), char_get_level(ch) * 2);
    stam = std::min<int64_t>(stam, 1500000);
    ki   = std::min<int64_t>(ki, 1500000);
    pl   = std::min<int64_t>(pl, 1500000);
    gainBasePL(ch, pl, true);
    gainBaseST(ch, stam, true);
    gainBaseKI(ch, ki, true);
    incCurLFPercent(g, ch, .05, true);
    send_to_char(g, ch, "@D[@gABSORB@D] @rPL@W: @D(@y{}@D) @cKi@W: @D(@y{}@D) @gSt@W: @D(@y{}@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
    improve_skill(g, ch, SKILL_ABSORB, 0);
    char_set_wait(ch, PULSE_4SEC);
    char_mob_set(vict, MOB_HUSK, true);
    die(g, vict, ch);
  }
} // namespace

ACMD(do_absorb)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (!check_skill(g, ch, SKILL_ABSORB))
    return;
  if (char_is_android(ch) && !limb_ok(g, ch, 0))
    return;
  if (!char_is_npc(ch) && char_is_bio(ch) && !char_plr_flagged(ch, PLR_TAIL)) {
    send_to_char(g, ch, "You have no tail!\r\n");
    return;
  }
  if (!char_is_android(ch) && !char_is_bio(ch)) {
    send_to_char(g, ch, "You shouldn't have this skill, you are incapable of absorbing.\r\n");
    send_to_imm(g, "ERROR: Absorb skill on {} when they are not a bio or android.", char_get_name(ch));
    return;
  }
  const char *refusal = nullptr;
  if (char_get_fighting(g, ch) && !char_is_android(ch))
    refusal = "You are too busy fighting!\r\n";
  else if (char_get_grappled(g, ch))
    refusal = "You are currently being grappled with! Try 'escape'!\r\n";
  else if (char_get_grappling(g, ch))
    refusal = "You are currently grappling with someone!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (struct char_data *drained = char_get_absorbing(g, ch)) {
    act_triple(g, {"@WYou stop absorbing from @c$N@W!@n", "$n stops absorbing from you!", "$n stops absorbing from $N!", true}, ch, nullptr, drained);
    if (char_is_npc(drained) && !char_get_fighting(g, drained))
      set_fighting(g, drained, ch);
    char_set_absorbby(drained, nullptr);
    char_set_absorbing(ch, nullptr);
  }

  /* an android names whom to absorb from; a bio-android how, then whom */
  bool android = char_is_android(ch);
  if (arg.empty()) {
    send_to_char(g, ch, android ? "Who do you want to absorb?\r\n" : "Syntax: absorb (swallow | extract) (target)\r\n");
    return;
  }
  auto found = find_char(g, ch, android ? arg : arg2, FIND_CHAR_ROOM, android ? "Absorb from who?\r\n" : "Syntax: absorb (swallow | extract) (target)\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_aff_flagged(vict, AFF_SANCTUARY)) {
    send_to_char(g, ch, "You can't absorb them, they have a barrier!\r\n");
    return;
  }
  if (android)
    android_absorb(g, ch, vict);
  else if (arg == "swallow")
    bio_swallow(g, ch, vict);
  else if (arg == "extract")
    bio_extract(g, ch, vict);
  else
    send_to_char(g, ch, "Syntax: absorb (extract | swallow) (target)\r\n");
}

namespace {
  /* How regeneration's healing looks, by how much it was: the first it's less than (in tenths of full) */
  constexpr struct {
    int64_t divisor;
    const char *to_char, *to_room;
  } REGENERATIONS[] = {
      {10, "You concentrate your ki and regenerate your body a little.", "$n concentrates and regenerates $s body a little."},
      {5, "You concentrate your ki and regenerate your body some.", "$n concentrates and regenerates $s body some."},
      {2, "You concentrate your ki and regenerate your body a great deal.", "$n concentrates and regenerates $s body a great deal."},
  };

  /* The limbs regeneration regrows or mends, in its order; quirk: some are set before they're told of, some after */
  constexpr struct {
    int limb;
    const char *name;
    bool regrown_first, mended_first; /* whether the limb is set before the messages */
  } REGROWN[] = {{0, "right arm", false, false}, {1, "left arm", true, false}, {3, "left leg", true, false}, {2, "right leg", true, true}};

  void regrow_limbs(GameWorld *g, struct char_data *ch)
  {
    for (const auto &r : REGROWN) {
      int condition = char_get_limb_condition(ch, r.limb);
      bool regrown = condition <= 0, mended = condition > 0 && condition < 50;
      if (!regrown && !mended)
        continue;
      bool set_first = regrown ? r.regrown_first : r.mended_first;
      if (set_first)
        char_set_limb_condition(ch, r.limb, 100);
      if (regrown) {
        act(g, str_format("You regrow your {}!", r.name).c_str(), true, ch, nullptr, nullptr, TO_CHAR);
        act(g, str_format("$n regrows $s {}!", r.name).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
      } else {
        act(g, str_format("Your broken {} mends itself!", r.name).c_str(), true, ch, nullptr, nullptr, TO_CHAR);
        act(g, str_format("$n regenerates $s broken {}!", r.name).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
      }
      if (!set_first)
        char_set_limb_condition(ch, r.limb, 100);
    }
  }
} // namespace

ACMD(do_regenerate)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  int skill = init_skill(g, ch, SKILL_REGENERATE);
  if (skill < 1) {
    send_to_char(g, ch, "You are incapable of regenerating.\r\n");
    return;
  }
  int suppression = char_get_suppression(ch);
  if (suppression > 0)
    skill = suppression;
  if (arg.empty()) {
    send_to_char(g, ch, "Regenerate how much PL?\r\nMax percent you can regen: {:d}\r\nSyntax: regenerate (1 - 100)\r\n", skill);
    return;
  }
  if (getCurPL(g, ch) >= getEffMaxPL(g, ch) || (suppression > 0 && getCurPL(g, ch) >= (getEffMaxPL(g, ch) / 100) * suppression)) {
    send_to_char(g, ch, "You do not need to regenerate, you are at full health.\r\n");
    return;
  }
  int num = parse_int(arg);
  if (num <= 0) {
    send_to_char(g, ch, "What is the point of that?\r\nSyntax: regenerate (1 - 100)\r\n");
    return;
  }
  if (num > 100 || num > skill || (suppression > 0 && num > suppression)) {
    send_to_char(g, ch, "You can't regenerate that much!\r\nMax you can regen: {:d}\r\n", skill);
    return;
  }

  int64_t amt = (getEffMaxPL(g, ch) * 0.01) * num;
  if (amt > 1)
    amt /= 2;
  if (char_is_bio(ch))
    amt = amt * 0.9;
  int64_t life = getCurLF(g, ch) - amt * 0.8, energy = getCurKI(g, ch) - amt * 0.2;
  if ((life <= 0 || energy <= 0) && !char_is_npc(ch)) {
    send_to_char(g, ch, "Your life force or ki are too low to regenerate that much.\r\n");
    send_to_char(g, ch, "@YLF Needed@D: @C{}@w, @YKi Needed@D: @C{}@w.@n\r\n", add_commas(amt * 0.8), add_commas(amt * 0.2));
    return;
  }
  if (char_is_npc(ch) && energy <= 0)
    return;

  incCurHealth(g, ch, amt * 2, true);
  if (!char_is_npc(ch))
    decCurLF(g, ch, amt * .8, 0);
  decCurKI(g, ch, amt * .2, 0);
  reveal_hiding(g, ch, 0);
  if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
    act(g, "You concentrate your ki and regenerate your body completely.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body completely.", true, ch, nullptr, nullptr, TO_ROOM);
  } else if (auto r = std::ranges::find_if(REGENERATIONS, [&](const auto &each) { return amt < getEffMaxPL(g, ch) / each.divisor; }); r != std::end(REGENERATIONS)) {
    act(g, r->to_char, true, ch, nullptr, nullptr, TO_CHAR);
    act(g, r->to_room, true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "You concentrate your ki and regenerate you nearly completely.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body nearly completely.", true, ch, nullptr, nullptr, TO_ROOM);
  }
  improve_skill(g, ch, SKILL_REGENERATE, 0);
  if (char_aff_flagged(ch, AFF_BURNED)) {
    send_to_char(g, ch, "Your burns are healed now.\r\n");
    act(g, "$n@w's burns are now healed.@n", true, ch, nullptr, nullptr, TO_ROOM);
    null_affect(g, ch, AFF_BURNED);
  }
  if (char_is_npc(ch))
    return;
  regrow_limbs(g, ch);
  if (!char_plr_flagged(ch, PLR_TAIL) && char_is_bio(ch)) {
    char_plr_set(ch, PLR_TAIL, true);
    act(g, "You regrow your tail!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n regrows $s tail!", true, ch, nullptr, nullptr, TO_ROOM);
  }
  improve_skill(g, ch, SKILL_REGENERATE, 0); /* quirk: a second time, for players */
}

namespace {
  /* The health each level of kaioken needs */
  constexpr int64_t KAIOKEN_LEVELS[21] = {0, 0, 0, 5000, 10000, 15000, 25000, 35000, 50000, 75000, 100000,
                                          150000, 200000, 250000, 300000, 400000, 500000, 600000, 700000, 800000, 1000000};
} // namespace

ACMD(do_kaioken)
{
  axion_dice(g, 0); /* quirk: rolled before anything, and unused */
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!check_skill(g, ch, SKILL_KAIOKEN))
    return;
  const char *refusal = nullptr;
  if (char_get_alignment(ch) <= -50)
    refusal = "Your heart is too corrupt to use that technique!\r\n";
  else if (!char_is_npc(ch) && char_plr_flagged(ch, PLR_HEALT))
    refusal = "You are inside a healing tank!\r\n";
  else if (arg.empty())
    refusal = "What level of kaioken do you want to try and achieve?\r\nSyntax: kaioken 1-20\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  int x = parse_int(arg);
  if (x < 0 || x > 20) {
    send_to_char(g, ch, "That level of kaioken dosn't exist...\r\nSyntax: kaioken 0-20\r\n");
    return;
  }
  if (x == 0) {
    if (char_get_kaioken(ch) > 0)
      remove_kaioken(g, ch, 1);
    else
      send_to_char(g, ch, "You are not in kaioken!\r\n");
    return;
  }
  if (x == char_get_kaioken(ch)) {
    send_to_char(g, ch, "You are already at that kaioken level! To release, try kaioken 0\r\n");
    return;
  }
  if (!char_is_npc(ch) && (char_is_transformed(ch) || (char_is_hoshijin(ch) && char_get_starphase(ch) > 0)) && x > 5) {
    send_to_char(g, ch, "You can not manage a kaioken level higher than 5 when transformed.\r\n");
    return;
  }
  /* it costs nothing to reduce your kaioken level */
  int64_t cost_unit = getMaxKI(g, ch) / 50;
  int64_t cost      = x < char_get_kaioken(ch) ? 0 : cost_unit * x - cost_unit * char_get_kaioken(ch);
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to focus into your body for that level.\r\n");
    return;
  }

  int roll = rand_number(g, 1, x * 5 + 1);
  reveal_hiding(g, ch, 0);
  decCurKI(g, ch, cost, 0);
  improve_skill(g, ch, SKILL_KAIOKEN, 1);
  if (init_skill(g, ch, SKILL_KAIOKEN) < roll) {
    send_to_char(g, ch, "You try to focus your ki into your body but mess up somehow.\r\n");
    act(g, "$n tries to use kaioken but messes up somehow.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  if (getMaxHealth(g, ch) < KAIOKEN_LEVELS[x]) {
    act(g, "@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  apply_kaioken(g, ch, x, true);
  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_zanzoken)
{
  if (!know_skill(g, ch, SKILL_ZANZOKEN) && !char_is_npc(ch))
    return;
  if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
    char_aff_set(ch, AFF_ZANZOKEN, false);
    send_to_char(g, ch, "You release the ki you had prepared for a zanzoken.\r\n");
    return;
  }
  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are busy in a grapple!\r\n");
    return;
  }
  int prob = char_is_npc(ch) ? rand_number(g, 80, 90) : char_get_skill(ch, SKILL_ZANZOKEN);
  int perc = axion_dice(g, 0);
  /* the less skilled, the more it costs */
  int64_t cost = getMaxKI(g, ch) / 50 * (prob > 75 ? 2 : prob > 50 ? 4 : prob >= 25 ? 8 : 10);
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }
  if (prob < perc) {
    send_to_char(g, ch, "You focus your ki in preparation of a zanzoken but mess up and waste your ki!\r\n");
    improve_skill(g, ch, SKILL_ZANZOKEN, 2);
    decCurKI(g, ch, cost, 0);
  } else {
    act(g, "@wYou focus your ki, preparing to move at super speeds if necessary.@n", true, ch, nullptr, nullptr, TO_CHAR);
    decCurKI(g, ch, cost, 0);
    char_aff_set(ch, AFF_ZANZOKEN, true);
    improve_skill(g, ch, SKILL_ZANZOKEN, 2);
  }
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_eyec)
{
  if (char_is_npc(ch))
    return;
  bool closed = char_plr_flagged(ch, PLR_EYEC);
  char_plr_set(ch, PLR_EYEC, !closed);
  act(g, closed ? "@wYou open your eyes.@n" : "@wYou close your eyes.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, closed ? "@C$n@w opens $s eyes.@n" : "@C$n@w closes $s eyes.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_solar)
{
  if (!know_skill(g, ch, SKILL_SOLARF) || !limb_ok(g, ch, 0))
    return;
  int prob = char_get_skill(ch, SKILL_SOLARF), perc = rand_number(g, 0, 101);
  int cost = getMaxKI(g, ch) / (prob >= 75 ? 50 : prob >= 50 ? 25 : prob >= 25 ? 20 : 15);
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }
  prob += char_get_aff_abils_intel(ch) / 3;
  if (prob < perc) {
    act(g, "@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' but nothing happens!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' but nothing happens!@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_SOLARF, 0);
    return;
  }
  act(g, "@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' as a blinding light fills the area!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' as a blinding light fills the area!@n", true, ch, nullptr, nullptr, TO_ROOM);
  /* everyone else who can see it is blinded */
  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch || char_plr_flagged(vict, PLR_EYEC) || char_aff_flagged(vict, AFF_BLIND) || char_get_position(vict) == POS_SLEEPING)
      continue;
    assign_affect(g, vict, AFF_BLIND, SKILL_SOLARF, 1, 0, 0, 0, 0, 0, 0);
    act_triple(g, {"@W$N@W is @YBLINDED@W!@n", "@RYou are @YBLINDED@R!@n", "@W$N@W is @YBLINDED@W!@n", true}, ch, nullptr, vict);
  }
  improve_skill(g, ch, SKILL_SOLARF, 0);
  decCurKI(g, ch, cost, 0);
  char_set_wait(ch, PULSE_3SEC);
}

namespace {
  /* What healing costs, of the healer's max ki, and heals, of the patient's max powerlevel: the first row skill reaches */
  constexpr struct {
    int skill, cost_divisor, heal_divisor;
  } HEALING[] = {{100, 20, 5}, {90, 16, 10}, {75, 14, 12}, {50, 12, 15}, {25, 10, 20}, {INT_MIN, 6, 20}};

  /* A healing's mending beyond powerlevel, by ch of vict */
  void mend(GameWorld *g, struct char_data *ch, struct char_data *vict, int64_t heal)
  {
    bool self = vict == ch;
    /* nail's students restore stamina too */
    if (char_is_nail(ch)) {
      int skill    = char_get_skill(ch, SKILL_HEAL);
      double share = skill >= 100 ? .4 : skill >= 60 ? .2 : skill >= 40 ? .1 : 0;
      if (share > 0) {
        incCurST(g, vict, heal * share, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      }
    }
    null_affect(g, vict, AFF_POISON);
    null_affect(g, vict, AFF_BLIND);
    if (!self) { /* quirk: healing yourself leaves these */
      if (char_aff_flagged(vict, AFF_BURNED)) {
        send_to_char(g, vict, "Your burns are healed now.\r\n");
        act(g, "$n@w's burns are now healed.@n", true, vict, nullptr, nullptr, TO_ROOM);
        char_aff_set(vict, AFF_BURNED, false);
      }
      if (is_affected(vict, AFF_HYDROZAP)) {
        send_to_char(g, vict, "You no longer feel a great thirst.\r\n");
        act(g, "$n@w no longer looks as if they could drink an ocean.@n", true, vict, nullptr, nullptr, TO_ROOM);
        remove_affect(g, vict, AFF_HYDROZAP);
      }
    }
    for (int limb = 0; limb < 4; limb++)
      char_set_limb_condition(vict, limb, 100);
    if (!char_plr_flagged(vict, PLR_TAIL) && (char_is_bio(vict) || char_is_icer(vict)) && !char_is_npc(vict))
      char_plr_set(vict, PLR_TAIL, true);
    if (!char_plr_flagged(vict, PLR_STAIL) && (char_is_saiyan(vict) || char_is_halfbreed(vict)) && !char_is_npc(vict))
      char_plr_set(vict, PLR_STAIL, true);
    if (!self && getCurLF(g, vict) <= getMaxLF(g, vict) * 0.5 && !char_is_android(vict)) {
      incCurLF(g, vict, getMaxLF(g, vict) * .35, true);
      send_to_char(g, vict, "You feel that your lifeforce has recovered some!\r\n");
    }
  }
} // namespace

ACMD(do_heal)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!check_skill(g, ch, SKILL_HEAL) || !limb_ok(g, ch, 0))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "You want to heal WHO?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "You do not see the target here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  bool self              = vict == ch;

  int64_t prob = init_skill(g, ch, SKILL_HEAL), perc = rand_number(g, 0, 110);
  const auto &tier = *std::ranges::find_if(HEALING, [&](const auto &h) { return prob >= h.skill; });
  int64_t cost = getMaxKI(g, ch) / tier.cost_divisor, heal = getEffMaxPL(g, vict) / tier.heal_divisor;
  if (char_get_bonuses(ch, BONUS_HEALER) > 0)
    heal += heal * .1;
  if (heal < getEffMaxPL(g, vict))
    heal += (heal / 100) * (char_get_aff_abils_wis(ch) / 4);
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }
  if (getCurPL(g, vict) >= getEffMaxPL(g, vict)) {
    send_to_char(g, ch, self ? "You are already at full health.\r\n" : "They are already at full health.\r\n");
    return;
  }
  if (char_get_suppression(vict) > 0 && getCurPL(g, vict) >= (getEffMaxPL(g, vict) / 100) * char_get_suppression(vict)) {
    send_to_char(g, ch, "They are already at full health.\r\n"); /* quirk: even of yourself */
    return;
  }
  prob += char_get_aff_abils_intel(ch) / 2 + char_get_aff_abils_wis(ch) / 3;
  if (prob < perc) {
    if (self)
      act_triple(g, {"@WYou place your hands on your body, but fail to concentrate to heal yourself!@n", nullptr, "@C$n@W places $s hands on $s body, but nothing happens.", true}, ch, nullptr, vict);
    else
      act_triple(g, {"@WYou place your hands near @c$N@W, but fail to concentrate enough to heal them!@n", "@C$n@W places $s hands near you, but nothing happens!@n", "@C$n@W places $s hands near @c$N@W, but nothing happens.", true}, ch, nullptr, vict);
    decCurKI(g, ch, cost, 0);
    improve_skill(g, ch, SKILL_HEAL, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (char_get_bonuses(ch, BONUS_HEALER) > 0)
    heal += heal * .25;
  if (self)
    act_triple(g, {"@WYou place your hands on your body and an orange glow surrounds you!@n", nullptr, "@C$n@W places $s hands on $s body and an orange glow surrounds $m.", true}, ch, nullptr, vict);
  else
    act_triple(g, {"@WYou place your hands near @c$N@W and an orange glow surrounds $M!@n", "@C$n@W places $s hands near you and an orange glow surrounds you!@n", "@C$n@W places $s hands near @c$N@W and an orange glow surrounds $M.", true}, ch, nullptr, vict);
  decCurKI(g, ch, cost, 0);
  incCurHealth(g, vict, heal, true);
  mend(g, ch, vict, heal);
  improve_skill(g, ch, SKILL_HEAL, 0);
  /* a namekian of nail's teaching learns a little from healing a companion who needed it */
  if (!self && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict)) &&
      char_is_nail(ch) && char_is_namek(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 &&
      getCurPL(g, vict) <= getEffMaxPL(g, vict) * 0.85 && rand_number(g, 1, 3) == 3)
    char_set_exp(ch, char_get_exp(ch) + level_exp(ch, char_get_level(ch) + 1) * 0.005);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_barrier)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!know_skill(g, ch, SKILL_BARRIER) && !char_get_skill(ch, SKILL_AQUA_BARRIER))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "[Syntax] barrier < 1-75 | release >\r\n");
    return;
  }
  bool shielded = char_aff_flagged(ch, AFF_SANCTUARY);
  if (str_eq_nocase("release", arg)) {
    if (!shielded) {
      send_to_char(g, ch, "You don't have a barrier.\r\n");
      return;
    }
    act(g, "@BYou dispel your barrier, releasing its energy.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@B$n@B dispels $s barrier, releasing its energy.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_barrier(ch, 0);
    char_aff_set(ch, AFF_SANCTUARY, false);
    return;
  }
  if (shielded) {
    send_to_char(g, ch, "You already have a barrier, try releasing it.\r\n");
    return;
  }
  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before concentrating again.\r\n");
    return;
  }

  /* a ki barrier, or a namekian's of water */
  bool ki_barrier = char_get_skill(ch, SKILL_BARRIER);
  int skill       = ki_barrier ? SKILL_BARRIER : SKILL_AQUA_BARRIER;
  int size        = parse_int(arg);
  int prob        = ki_barrier ? init_skill(g, ch, SKILL_BARRIER) : char_get_skill(ch, SKILL_AQUA_BARRIER);
  int perc        = axion_dice(g, 0);
  int64_t cost    = (getMaxKI(g, ch) * 0.01) * (size * 0.5);
  const char *refusal = nullptr;
  if (size > prob)
    refusal = "You can not create a barrier that is stronger than your skill in barrier.\r\n";
  else if (size < 1)
    refusal = "You have to put at least some ki into the barrier!\r\n";
  else if (size > 75)
    refusal = "You can't control a barrier with more than 75 percent!\r\n";
  else if (char_get_charge(ch) < cost)
    refusal = "You do not have enough ki charged up!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (prob < perc) {
    act(g, "@BYou shout as you form a barrier of ki around your body, but you imbalance it and it explodes outward!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@B$n@B shouts as $e forms a barrier of ki around $s body, but it becomes imbalanced and explodes outward!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_charge(ch, char_get_charge(ch) - cost);
    improve_skill(g, ch, skill, 2);
    char_set_con_cooldown(ch, 30);
    return;
  }
  if (ki_barrier) {
    act(g, "@BYou shout as you form a barrier of ki around your body!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@B$n@B shouts as $e forms a barrier of ki around $s body!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "@BYou shout as you form a barrier of ki and raging waters around your body!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@B$n@B shouts as $e forms a barrier of ki and raging waters around $s body!@n", true, ch, nullptr, nullptr, TO_ROOM);
  }
  char_set_barrier(ch, (getMaxKI(g, ch) / 100) * size);
  char_set_charge(ch, char_get_charge(ch) - cost);
  improve_skill(g, ch, skill, 2);
  char_aff_set(ch, AFF_SANCTUARY, true);
  char_set_con_cooldown(ch, 20);
}

namespace {
  /* Where instant transmission can take you without a target */
  constexpr struct {
    const char *name;
    room_vnum room;
  } PLANETS_BY_KI[] = {{"planet-earth", 300}, {"planet-namek", 10222}, {"planet-frigid", 4017}, {"planet-vegeta", 2200}, {"planet-konack", 8006}, {"planet-aether", 12024}};

  constexpr const char *SELF_TRANSMISSION = "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n";

  /* Why ch can't reach tar, if they can't */
  const char *unreachable(GameWorld *g, struct char_data *ch, struct char_data *tar)
  {
    if (tar == ch)
      return SELF_TRANSMISSION;
    if (char_get_grappling(g, ch) && char_get_grappling(g, ch) == tar)
      return "You are already in the same room with them and are grappling with them!\r\n";
    if (!read_sense_memory(g, ch, tar))
      return "You've never sensed them up close so you do not have a good bearing on their ki signal.\r\n";
    if (char_get_admlevel(tar) > 0 && char_get_admlevel(ch) < 1)
      return "That immortal prevents you from reaching them.\r\n";
    if (char_is_android(tar) || getCurPL(g, tar) < getCurPL(g, ch) * 0.001 + 1)
      return "You can't sense them well enough.\r\n";
    if ((!room_flagged(g, char_get_in_room(ch), ROOM_AL) && room_flagged(g, char_get_in_room(tar), ROOM_AL)) || (!room_flagged(g, char_get_in_room(ch), ROOM_RHELL) && room_flagged(g, char_get_in_room(tar), ROOM_RHELL)))
      return "They are dead and can't be reached.\r\n";
    if (room_flagged(g, char_get_in_room(tar), ROOM_NOINSTANT))
      return "You can not go there as it is a protected area!\r\n";
    return nullptr;
  }

  /* Why ch can't use instant transmission at all just now, if they can't */
  const char *cannot_transmit(GameWorld *g, struct char_data *ch)
  {
    room_vnum room = char_get_in_room(ch);
    if (!char_get_skill(ch, SKILL_SENSE) && !char_plr_flagged(ch, PLR_SENSEM))
      return "You can't sense them to go to there!\r\n";
    if (char_plr_flagged(ch, PLR_PILOTING))
      return "You are busy piloting a ship!\r\n";
    if (char_plr_flagged(ch, PLR_HEALT))
      return "You are inside a healing tank!\r\n";
    if (room >= 19800 && room <= 19899)
      return "@rYou are in a pocket dimension!@n\r\n";
    if (room_flagged(g, room, ROOM_RHELL) || room_flagged(g, room, ROOM_AL) || room_flagged(g, room, ROOM_HELL))
      return "You can not leave where you are at!\r\n";
    return nullptr;
  }
} // namespace

ACMD(do_instant)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
    char_prf_set(g, ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    send_to_char(g, ch, "You stop watching the arena action.\r\n");
  }
  if (!know_skill(g, ch, SKILL_INSTANTT))
    return;
  if (const char *why = cannot_transmit(g, ch)) {
    send_to_char(g, ch, why);
    return;
  }
  constexpr const char *SYNTAX = "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\nExample: instant goku\nExample 2: instant planet-earth\r\n";
  if (arg.empty()) {
    send_to_char(g, ch, SYNTAX);
    return;
  }
  int skill = char_get_skill(ch, SKILL_INSTANTT);
  /* quirk: at exactly 25 it costs nothing */
  int64_t cost = skill > 75 ? getMaxKI(g, ch) / 40 : skill > 50 ? getMaxKI(g, ch) / 20 : skill > 25 ? getMaxKI(g, ch) / 15 : skill < 25 ? getMaxKI(g, ch) / 10 : 0;
  if (getCurKI(g, ch) - cost < 0) {
    send_to_char(g, ch, "You do not have enough ki to instantaneously move.\r\n");
    return;
  }
  int perc = axion_dice(g, 0);

  room_vnum location     = 0;
  struct char_data *tar  = nullptr;
  auto planet = std::ranges::find_if(PLANETS_BY_KI, [&](const auto &p) { return str_eq_nocase(arg, p.name); });
  if (planet != std::end(PLANETS_BY_KI))
    location = planet->room;
  else if (!(tar = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "@RThat target was not found.@n\r\n{}", SYNTAX);
    return;
  }

  if (skill < perc || (char_get_fighting(g, ch) && rand_number(g, 1, 2) <= 1)) {
    if (tar == ch) {
      send_to_char(g, ch, SELF_TRANSMISSION);
      return;
    }
    send_to_char(g, ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
    decCurKI(g, ch, cost, 0);
    improve_skill(g, ch, SKILL_INSTANTT, 2);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  reveal_hiding(g, ch, 0);
  char_set_wait(ch, PULSE_2SEC);
  if (!tar) {
    decCurKI(g, ch, cost, 0);
    act(g, "@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move faster than light and arrive almost instantly at your destination. Having located the planet by its collective population's ki.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@w$n@w places two fingers on $s forehead and disappears in an instant!@n", true, ch, nullptr, nullptr, TO_NOTVICT);
    handle_teleport(g, ch, nullptr, location);
    improve_skill(g, ch, SKILL_INSTANTT, 2);
    return;
  }
  if (const char *why = unreachable(g, ch, tar)) {
    send_to_char(g, ch, why);
    return;
  }
  decCurKI(g, ch, cost, 0);
  act_triple(g, {"@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move through the molecules of the universe faster than the speed of light. You stop as you arrive at $N@w!@n", "@w$n@w appears in an instant out of nowhere right next to you!@n", "@w$n@w places two fingers on $s forehead and disappears in an instant!@n", true}, ch, nullptr, tar);
  char_plr_set(ch, PLR_TRANSMISSION, true);
  handle_teleport(g, ch, tar, 0);
  improve_skill(g, ch, SKILL_INSTANTT, 2);
}

ACMD(do_transform)
{
  if (!race_can_transform(char_get_race(ch))) {
    send_to_char(g, ch, "You do not have a transformation.\r\n");
    return;
  }
  bool npc = char_is_npc(ch);
  /*R: Hidden transformation stuff following this*/
  if (!npc && getBasePL(ch) < 50000) {
    send_to_char(g, ch, "@RYou are too weak to comprehend transforming!@n\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  /* Called with no argument - display transformation information */
  if (arg.empty()) {
    race_display_forms(g, ch);
    if (trans_req(ch, 1) > 0)
      race_display_trans_req(g, ch);
    return;
  }
  transform_bonus cur_form = race_current_form(g, ch);
  bool can_revert          = race_can_revert(char_get_race(ch));
  /* If we are in kaioken or something weird like that, prevent transforming. */
  if (!race_check_can_transform(g, ch))
    return;

  if (str_eq_nocase("revert", arg)) {
    if (!can_revert) {
      send_to_char(g, ch, "That would be unthinkable.\r\n");
      return;
    }
    if (!cur_form.flag) {
      send_to_char(g, ch, "You are not transformed.\r\n");
      return;
    }
    if (char_get_charge(ch) > 0)
      do_charge(g, ch, "release", 0, 0);
    race_echo_revert(g, ch, race_flag_to_tier(cur_form.flag));
    char_plr_set(ch, cur_form.flag, false);
    if (!arg2.empty()) /* revert and transform anew */
      do_transform(g, ch, arg2.data(), 0, 0);
    return;
  }

  transform_bonus trans;
  if (!race_find_form(ch, arg, &trans)) {
    send_to_char(g, ch, "You don't have that form.\r\n");
    return;
  }
  int to_tier = race_flag_to_tier(trans.flag);
  if (char_plr_flagged(ch, trans.flag)) {
    send_to_char(g, ch, "You are already in that form! Try 'revert'.\r\n");
    return;
  }
  if (!npc && getBasePL(ch) < trans_req(ch, to_tier)) {
    send_to_char(g, ch, "You are not strong enough to handle that transformation!\r\n");
    return;
  }
  if (!npc && getCurST(g, ch) <= getMaxST(g, ch) * trans.drain) {
    send_to_char(g, ch, "You do not have enough stamina!");
    return;
  }
  /* Pay the price to unlock form if necessary. */
  if (!npc && !race_check_trans_unlock(g, ch, to_tier))
    return;

  if (cur_form.flag)
    char_plr_set(ch, cur_form.flag, false);
  /* The stats are applied automatically just by having the flag. */
  char_plr_set(ch, trans.flag, true);
  race_echo_transform(g, ch, to_tier);
  reveal_hiding(g, ch, 0); /* No way is this a stealthy process... */
  if (race_has_noisy_transformations(char_get_race(ch)))
    if (struct zone_data *zone = find_zone_by_thing(g, char_get_in_room(ch)))
      send_to_zone(g, "An explosion of power ripples through the surrounding area!\r\n", zone);
  send_to_sense(g, 0, "You sense a nearby power grow unbelievably!", ch);
  send_to_scouter(g, str_format("@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y{}@D]", add_commas(getCurPL(g, ch))).c_str(), ch, 1, 0);
}

namespace {
  /* What a TCX-Medical Equipment Construction Kit can make, by aid's word for it */
  struct medical_product {
    const char *word;
    obj_vnum vnum;
    int skill;      /* the first aid it takes */
    int difficulty; /* added to the roll against it */
    const char *failed, *made, *made_seen;
  };
  constexpr medical_product MEDICAL_PRODUCTS[] = {
      {"adrenex", 381, 65, 15,
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you begin to construct an Andrenex Adreneline Injector you screw up and end up breaking the water tight seal. The adreneline leaks out and is wasted.@n",
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully construct an Adrenex Adreneline Injector@n",
       "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Adrenex Adreneline Injector!@n"},
      {"antitoxin", 383, 40, 5,
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete the Antitoxin Injector you notice that you didn't seal the syringe properly and it all leaks out.@n",
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble the Antitoxin Injector.@n",
       "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Antitoxin Injector!@n"},
      {"salve", 382, 50, 10,
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you go to put the salve ingredients into the kit's salve compartment and set the temperature you accidentally set it too high. The salve is burned and ruined. Yes you managed to burn a burn salve.@n",
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully boil a burn salve to perfection and it is automatically placed in a jar.@n",
       "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a jar of burn salve!@n"},
      {"formula-82", 385, 40, 15,
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete a vial of Formula 82 you notice that you read the mixture measurements wrong. You dispose of the vile vial immediately.@n",
       "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble a vial of Formula 82.@n",
       "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Vial of Formula 82!@n"},
  };

  /* aid (product): one made with a construction kit, which is used up either way */
  void construct_aid(GameWorld *g, struct char_data *ch, const medical_product &p, struct obj_data *kit)
  {
    if (char_get_skill(ch, SKILL_FIRST_AID) < p.skill) {
      send_to_char(g, ch, "You need at least a skill level of {:d} in First Aid.\r\n", p.skill);
      return;
    }
    if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(g, p.difficulty)) {
      act(g, p.failed, true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, ch, nullptr, nullptr, TO_ROOM);
      extract_obj(g, kit);
    } else {
      act(g, p.made, true, ch, nullptr, nullptr, TO_CHAR);
      act(g, p.made_seen, true, ch, nullptr, nullptr, TO_ROOM);
      struct obj_data *product = read_object(g, p.vnum);
      add_unique_id(g, product);
      obj_to_char(g, product, ch);
      extract_obj(g, kit);
      improve_skill(g, ch, SKILL_FIRST_AID, 1);
    }
    char_set_wait(ch, PULSE_3SEC);
  }

  /* aid (anything else) (target): bandaging their wounds */
  void bandage(GameWorld *g, struct char_data *ch, std::string &target, struct obj_data *bandages)
  {
    auto found = find_char(g, ch, target, FIND_CHAR_ROOM, "Apply first aid to who?\r\n");
    if (reject(g, ch, found))
      return;
    struct char_data *vict = *found;
    if (char_is_npc(vict)) {
      send_to_char(g, ch, "What ever for?\r\n");
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, "They are an android!\r\n");
      return;
    }
    if (char_plr_flagged(vict, PLR_BANDAGED) || char_aff_flagged(vict, AFF_SPIRIT)) {
      send_to_char(g, ch, char_plr_flagged(vict, PLR_BANDAGED) ? "They are already bandaged!\r\n" : "The dead don't need first aid.\r\n");
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (vict != ch)
      send_to_char(g, ch, "You attempt to lend first aid to {}.\r\n", char_get_name(vict));
    act(g, "$n attempts to bandage $N's wounds.", true, ch, nullptr, vict, TO_ROOM);
    if (char_get_skill(ch, SKILL_FIRST_AID) + 1 > axion_dice(g, 0)) {
      send_to_char(g, ch, "You bandage {}'s wounds.\r\n", char_get_name(vict));
      int64_t roll = (getEffMaxPL(g, vict) / 100) * (char_get_aff_abils_wis(ch) / 4) + getEffMaxPL(g, vict) * 0.25;
      if (char_get_bonuses(ch, BONUS_HEALER) > 0)
        roll += roll * .1;
      incCurHealth(g, vict, roll, true);
      send_to_char(g, vict, "Your wounds are bandaged by {}!\r\n", char_get_name(ch));
      act(g, "$n's wounds are stablized by $N!", true, vict, nullptr, ch, TO_NOTVICT);
      char_plr_set(vict, PLR_BANDAGED, true);
    } else if (vict != ch) {
      send_to_char(g, ch, "You fail to bandage their wounds properly, wasting the set of bandages...\r\n");
      act(g, "$N fails to bandage $n's wounds properly, wasting an entire set of bandages...", true, vict, nullptr, ch, TO_NOTVICT);
      act(g, "$N fails to bandage your wounds properly, wasting an entire set of bandages...", true, vict, nullptr, ch, TO_CHAR);
    } else {
      act(g, "$N fails to bandage $s wounds properly, wasting an entire set of bandages...", true, vict, nullptr, ch, TO_NOTVICT);
      act(g, "You fail to bandage your wounds properly, wasting an entire set of bandages...", true, vict, nullptr, ch, TO_VICT);
    }
    extract_obj(g, bandages);
    improve_skill(g, ch, SKILL_FIRST_AID, 1);
    char_set_wait(ch, PULSE_3SEC);
  }
} // namespace

ACMD(do_aid)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (char_is_npc(ch))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: aid heal (target)\r\n        aid adrenex\r\n        aid antitoxin\r\n        aid salve\r\n        aid formula-82\r\n");
    return;
  }
  /* a product made with a construction kit (380), or else bandages (47) */
  auto product      = std::ranges::find_if(MEDICAL_PRODUCTS, [&](const auto &p) { return str_eq_nocase(arg, p.word); });
  bool constructing = product != std::end(MEDICAL_PRODUCTS);
  struct obj_data *supplies = find_first(char_inventory(g, ch), [&](obj_data *obj) {
    return obj_get_vnum(g, obj) == (constructing ? 380 : 47) && !obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED);
  });
  if (!supplies) {
    send_to_char(g, ch, constructing ? "You need a TCX-Medical Equipment Construction Kit.\r\n" : "You need bandages to be able to use first aid.\r\n");
    return;
  }
  if (constructing)
    construct_aid(g, ch, *product, supplies);
  else
    bandage(g, ch, arg2, supplies);
}

ACMD(do_aura)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  const char *refusal = nullptr;
  if (arg.empty())
    refusal = "Syntax: aura light\r\n";
  else if (char_get_charge(ch))
    refusal = "You can't focus enough on this while charging.";
  else if (char_plr_flagged(ch, PLR_POWERUP))
    refusal = "You are busy powering up!\r\n";
  else if (!str_eq_nocase(arg, "light"))
    return;
  else if (char_get_skill(ch, SKILL_FOCUS) < 75 || char_get_skill(ch, SKILL_CONCENTRATION) < 75)
    refusal = "You need at least a skill level of 75 in Focus and Concentration to use this.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  if (char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(g, ch, "Your aura fades as you stop shining light.\r\n");
    act(g, "$n's aura fades as they stop shining light on the area.", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_AURALIGHT, false);
    room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) - 1);
    return;
  }
  if (getCurKI(g, ch) <= getMaxKI(g, ch) * 0.12) {
    send_to_char(g, ch, "You don't have enough KI to do that.\r\n");
    return;
  }
  if (char_get_in_room(ch) != NOWHERE)
    room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) + 1);
  reveal_hiding(g, ch, 0);
  decCurKIPercent(g, ch, .12, 0);
  send_to_char(g, ch, "A bright {} aura begins to burn around you as you provide light to the surrounding area!\r\n", aura_types[char_get_aura(ch)]);
  act(g, str_format("@wA {} aura flashes up brightly around $n@w as they provide light to the area.@n", aura_types[char_get_aura(ch)]).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_AURALIGHT, true);
}
