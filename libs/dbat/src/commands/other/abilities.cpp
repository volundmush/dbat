/* abilities.c: racial and special abilities
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/act.other.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/shadowdragons.h"
#include "dbat/consts/auction.h"
#include "dbat/consts/aligns.h"
#include "dbat/utils.h"
#include "dbat/flags.h"
#include <sys/stat.h>
#include "dbat/config.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/zone_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/search.h"
#include "dbat/random.h"
#include "dbat/affect.h"
#include "dbat/relocate.h"
#include "dbat/extract.h"
#include "dbat/races.h"
#include "dbat/fileop.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/broadcast.h"
#include "dbat/combat.h"
#include "dbat/act.misc.h"
#include "dbat/weather.h"
#include "dbat/act.item.h"
#include "dbat/act.wizard.h"
#include "dbat/act.informative.h"
#include "dbat/act.movement.h"
#include "dbat/obj_edit.h"
#include "dbat/graph.h"
#include "dbat/alias.h"
#include "dbat/spells.h"
#include "dbat/interpreter.h"
#include "dbat/fight.h"
#include "dbat/class.h"
#include "dbat/shop.h"
#include "dbat/feats.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/objsave.h"
#include "dbat/mail.h"
#include "dbat/act.offensive.h"
#include "other_internal.h"
#include "dbat/game_world.h"

ACMD(do_willpower)
{

  int fail = false;

  if (char_is_npc(ch))
    return;

  if (char_get_majinize(ch) <= 0) {
    send_to_char(g, ch, "You are not majinized and have no need to reclaim full control of your own will.\r\n");
    return;
  }
  if (char_get_practice_points(g, ch) < 100 && char_get_level(ch) < 100) {
    send_to_char(g, ch, "You do not have enough PS to focus your attempt to break free.\r\n");
    fail = true;
  }
  if (char_get_practice_points(g, ch) < 200 && char_get_level(ch) >= 100) {
    send_to_char(g, ch, "You do not have enough PS to focus your attempt to break free.\r\n");
    fail = true;
  }
  if (char_get_exp(ch) < level_exp(ch, char_get_level(ch) + 1) && char_get_level(ch) < 100) {
    send_to_char(g, ch, "You need a full level's worth of experience stored up to try and break free.\r\n");
    fail = true;
  }
  if (fail == true) {
    return;
  }
  char_set_exp(ch, 0);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (100));
  if (char_get_level(ch) >= 100) {
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (100));
  }
  if (rand_number(g, 10, 100) - char_get_aff_abils_intel(ch) > 60) {
    reveal_hiding(g, ch, 0);
    act(g, "@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments you give up, having failed to overcome the majinization!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments $n seems to give up and the commotion dies down.@n", true, ch, 0, 0, TO_ROOM);
    return;
  }
  char_set_exp(ch, 0);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (100));
  if (char_get_level(ch) >= 100) {
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (100));
  }
  reveal_hiding(g, ch, 0);
  act(g, "@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments the ground splits beneath you and while letting out a piercing scream the M disappears from your forehead! You are free while still keeping the boost you had recieved from the majinization!@n", true, ch, 0, 0, TO_CHAR);
  act(g, "@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments the ground beneath $n splits and $e lets out a piercing scream. The M on $s forehead disappears!@n", true, ch, 0, 0, TO_ROOM);
  char_set_majinize(ch, 3);
  return;
}

ACMD(do_infuse)
{

  if (!know_skill(g, ch, SKILL_INFUSE)) {
    return;
  }

  if (char_aff_flagged(ch, AFF_INFUSE)) {
    act(g, "You stop infusing ki into your attacks.", true, ch, 0, 0, TO_CHAR);
    act(g, "$n stops infusing ki into $s attacks.", true, ch, 0, 0, TO_ROOM);
    char_aff_set(ch, AFF_INFUSE, false);
    return;
  }

  if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 100) {
    send_to_char(g, ch, "You don't have enough ki to infuse into your attacks!\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  act(g, "You start infusing ki into your attacks.", true, ch, 0, 0, TO_CHAR);
  act(g, "$n starts infusing ki into $s attacks.", true, ch, 0, 0, TO_ROOM);
  char_aff_set(ch, AFF_INFUSE, true);
  decCurKI(g, ch, getMaxKI(g, ch) / 100, 0);
}

ACMD(do_paralyze)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(g, ch, SKILL_PARALYZE)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Who are you wanting to paralyze?\r\n");
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "That target isn't here.\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0)) {
    return;
  }
  if (char_aff_flagged(vict, AFF_PARA)) {
    send_to_char(g, ch, "They are already partially paralyzed!\r\n");
    return;
  }

  if ((getCurKI(g, ch)) < getCurPL(g, vict) / 10 + (getMaxKI(g, ch) / 20)) {
    send_to_char(g, ch, "You realize you can't paralyze them. You don't have enough ki to restrain them!\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_PARALYZE), perc = axion_dice(g, 0);

  if (char_get_speedi(g, ch) * 2 < char_get_speedi(g, vict)) {
    prob -= 10;
  }
  if (char_get_speedi(g, ch) + (char_get_speedi(g, ch) / 2) < char_get_speedi(g, vict)) {
    prob -= 5;
  }

  if (char_get_bonuses(vict, BONUS_INSOMNIAC)) {
    decCurKI(g, ch, getCurPL(g, vict) / 6 + (getMaxKI(g, ch) / 20), 0);
    act(g, "@RYou focus ki and point both your arms at @r$N@R. However $N seems to shake off your paralysis attack!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@r$n @Rfocuses ki and points both $s arms at YOU! Your insomnia makes you immune to $s feeble paralysis attempt.@n", true, ch, 0, vict, TO_VICT);
    act(g, "@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $N seems to shake off $s paralysis attack!@n", true, ch, 0, vict, TO_NOTVICT);
    return;
  }
  if (prob < perc) {
    reveal_hiding(g, ch, 0);
    act(g, "@RYou focus ki and point both your arms at @r$N@R. However $E manages to avoid your attempt to paralyze $M!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@r$n @Rfocuses ki and points both $s arms at YOU! You manage to avoid $s technique though...@n", true, ch, 0, vict, TO_VICT);
    act(g, "@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $E manages to avoid @r$n's@R attempted technique...@n", true, ch, 0, vict, TO_NOTVICT);
    decCurKI(g, ch, getCurPL(g, vict) / 6 + (getMaxKI(g, ch) / 20), 0);
    improve_skill(g, ch, SKILL_PARALYZE, 0);
  } else {
    reveal_hiding(g, ch, 0);
    act(g, "@RYou focus ki and point both your arms at @r$N@R. Your ki flows into $S body and partially paralyzes $M!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@r$n @Rfocuses ki and points both $s arms at YOU! You are caught in $s paralysis technique and now can barely move!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@r$n @Rfocuses ki and points both $s arms at @r$N@R. @r$n's@R ki flows into @r$N@R body and partially paralyzes $M!@n", true, ch, 0, vict, TO_NOTVICT);
    int duration = char_get_aff_abils_intel(ch) / 15;
    assign_affect(g, vict, AFF_PARA, SKILL_PARALYZE, duration, 0, 0, 0, 0, 0, 0);
    decCurKI(g, ch, getCurPL(g, vict) / 6 + (getMaxKI(g, ch) / 20), 0);
    improve_skill(g, ch, SKILL_PARALYZE, 0);
  }
}

ACMD(do_taisha)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(g, ch, SKILL_TAISHA)) {
    return;
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_AURA)) {
    send_to_char(g, ch, "This area already has an aura of regeneration around it.\r\n");
    return;
  }

  if (rroom_get_gravity(g, char_get_in_room(ch)) > 0) {
    send_to_char(g, ch, "This area's gravity is too hostile to an aura.\r\n");
    return;
  }

  if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 3) {
    send_to_char(g, ch, "You don't have enough ki.\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_TAISHA), perc = axion_dice(g, 0);

  decCurKI(g, ch, getMaxKI(g, ch) / 3, 0);
  if (prob < perc) {
    reveal_hiding(g, ch, 0);
    act(g, "@WYou hold up your hands while channeling ki. Your technique fails to produce an aura though....@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@g$n@W holds up $s hands while channeling ki. $s technique fails to produce an aura though....", true, ch, 0, 0, TO_ROOM);
    improve_skill(g, ch, SKILL_TAISHA, 1);
    return;
  }
  reveal_hiding(g, ch, 0);
  act(g, "@WYou hold up your hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", true, ch, 0, 0, TO_CHAR);
  act(g, "@g$n holds up $s hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", true, ch, 0, 0, TO_ROOM);
  improve_skill(g, ch, SKILL_TAISHA, 1);
  rroom_set_flag(g, char_get_in_room(ch), ROOM_AURA, true);
  return;
}

ACMD(do_kura)
{
  if (!know_skill(g, ch, SKILL_KURA)) {
    return;
  }

  if ((getCurKI(g, ch)) >= getMaxKI(g, ch)) {
    send_to_char(g, ch, "Your ki is already maxed out!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: kuraiiro (1-100).\r\n");
    return;
  }

  int num      = atoi(arg);
  int skill    = char_get_skill(ch, SKILL_KURA);
  int64_t cost = 0, bonus = 0;

  if (num > skill) {
    send_to_char(g, ch, "The number can not be greater than your skill.\r\n");
    return;
  }
  if (num <= 0) {
    send_to_char(g, ch, "The number can not be less than 1.\r\n");
    return;
  }

  cost  = (getMaxKI(g, ch) / 100) * num;
  bonus = cost;

  if ((getCurST(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough stamina for that high a number.\r\n");
    return;
  }

  if (skill <= axion_dice(g, 0)) {
    decCurST(g, ch, cost, 0);
    reveal_hiding(g, ch, 0);
    act(g, "You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies but you fail!", true, ch, 0, 0, TO_CHAR);
    act(g, "@c$n@w crouches down and screams as $s eyes turn red and $e attempts to tap into dark energies but fails!", true, ch, 0, 0, TO_ROOM);
    improve_skill(g, ch, SKILL_KURA, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  decCurST(g, ch, cost, 0);
  incCurKI(g, ch, bonus, true);
  reveal_hiding(g, ch, 0);
  act(g, "You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies and succeed as a rush of energy explodes around you!", true, ch, 0, 0, TO_CHAR);
  act(g, "@c$n@w crouches down and screams as $s eyes turn red. Suddenly $e manages to tap into dark energies and a rush of energy explodes around $m!", true, ch, 0, 0, TO_ROOM);
  improve_skill(g, ch, SKILL_KURA, 0);
  char_set_wait(ch, PULSE_2SEC);
  return;
}

ACMD(do_candy)
{

  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_is_majin(ch)) {
    send_to_char(g, ch, "You are not a majin, how can you do that?\r\n");
    return;
  }
  //if (FIGHTING(ch)) {
  //send_to_char(ch, "You are too busy fighting!\r\n");
  //return;
  //}

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "Turn who into candy?\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0)) {
    return;
  }

  int64_t ch_max   = getMaxPLTrans(g, ch);
  int64_t vict_max = getMaxPLTrans(g, vict);

  if (!char_is_npc(vict)) {
    send_to_char(g, ch, "You can't turn them into candy.\r\n");
    return;
  }
  if (vict_max > ch_max * 2) {
    send_to_char(g, ch, "They are too powerful.\r\n");
    return;
  }
  if (vict_max < ch_max * .25 && char_get_level(ch) < 100) {
    send_to_char(g, ch, "They are too weak.\r\n");
    return;
  }
  if (vict_max < ch_max * .09 && char_get_level(ch) == 100) {
    send_to_char(g, ch, "They are too weak.\r\n");
    return;
  }
  if ((getCurKI(g, ch)) < ch_max / 15) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }
  if (rand_number(g, 1, 6) == 6) {
    decCurKI(g, ch, getMaxKI(g, ch) / 15, 0);
    reveal_hiding(g, ch, 0);
    act(g, "@cYou aim your forelock at @R$N@c and fire a beam of energy but it is dodged!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@c aims $s forelock at @R$N@c and fires a beam of energy but the beam is dodged!@n", true, ch, 0, vict, TO_NOTVICT);
    if (!char_get_fighting(ch)) {
      set_fighting(g, ch, vict);
    }
    if (!char_get_fighting(vict)) {
      set_fighting(g, vict, ch);
    }
    char_set_wait(ch, PULSE_3SEC);
    return;
  }
  decCurKI(g, ch, getMaxKI(g, ch) / 15, 0);
  reveal_hiding(g, ch, 0);
  act(g, "@cYou aim your forelock at @R$N@c and fire a beam of energy that envelopes $S entire body and changes $M into candy!@n", true, ch, 0, vict, TO_CHAR);
  act(g, "@C$n@c aims $s forelock at @R$N@c and fires a beam of energy that envelopes $S entire body and changes $M into candy!@n ", true, ch, 0, vict, TO_NOTVICT);
  if (vict_max >= ch_max * 1.5) {
    send_to_char(g, ch, "You grab the candy as it falls.\r\n");
    obj = read_object(g, 95, VIRTUAL);
    obj_to_char(g, obj, ch);
  } else if (vict_max >= ch_max) {
    send_to_char(g, ch, "You grab the candy as it falls.\r\n");
    obj = read_object(g, 94, VIRTUAL);
    obj_to_char(g, obj, ch);
  } else if (vict_max < ch_max && (((vict_max >= ch_max * .5) && char_get_level(ch) < 100) || ((vict_max >= ch_max * .1) && char_get_level(ch) >= 100))) {
    send_to_char(g, ch, "You grab the candy as it falls.\r\n");
    obj = read_object(g, 93, VIRTUAL);
    obj_to_char(g, obj, ch);
  } else if (vict_max < ch_max && (((vict_max < ch_max * .5) && char_get_level(ch) < 100) || ((vict_max < ch_max * .1) && char_get_level(ch) >= 100))) {
    send_to_char(g, ch, "You grab the candy as it falls.\r\n");
    obj = read_object(g, 53, VIRTUAL);
    obj_to_char(g, obj, ch);
  }
  char_mob_set(vict, MOB_HUSK, true);
  die(g, vict, ch);
}

ACMD(do_future)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = nullptr;
  one_argument(argument, arg);

  if (char_is_npc(ch) || !char_is_kanassan(ch)) {
    send_to_char(g, ch, "You are incapable of this ability.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Bestow advance future sight on who?\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "Bestow advance future sight on who?\r\n");
    return;
  }

  if (char_aff_flagged(vict, AFF_FUTURE)) {
    send_to_char(g, ch, "They already can see the future.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(g, ch, "You can't target them, there would be no point.\r\n");
    return;
  }

  if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 40) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }

  if (char_get_practice_points(g, ch) < 100) {
    send_to_char(g, ch, "You do not have enough PS to activate or pass on this ability.\r\n");
    return;
  }

  if (vict != ch) {
    if (char_get_real_abils_cha(vict) + 5 > 70 && char_get_bonuses(vict, BONUS_SLOW) > 0) {
      send_to_char(g, ch, "They can't handle having their speed increased beyond 70.\r\n");
      return;
    }
    if (char_get_real_abils_intel(vict) + 2 > 70 && char_get_bonuses(vict, BONUS_DULL) > 0) {
      send_to_char(g, ch, "They can't handle having their intelligence increased beyond 70.\r\n");
      return;
    }
    decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (100));
    reveal_hiding(g, ch, 0);
    act(g, "@CYou focus your energy into your fingers before stabbing your claws into $N and bestowing the power of Future Sight upon $M. Shortly after $E passes out.@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n focuses $s energy into $s fingers before stabbing $s claws into YOUR neck and bestowing the power of Future Sight upon you! Soon after you pass out!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@C$n focuses $s energy into $s fingers before stabbing $s claws into $N's neck and bestowing the power of Future Sight upon $M! Soon after $E passes out!@n", true, ch, 0, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_FUTURE, true);
    char_set_real_abils_cha(vict, char_get_real_abils_cha(vict) + 5);
    char_set_real_abils_intel(vict, char_get_real_abils_intel(vict) + 2);
    char_set_position(vict, POS_SLEEPING);
    save_char(g, vict);
  } else {
    if (char_get_real_abils_cha(ch) + 5 > 70 && char_get_bonuses(ch, BONUS_SLOW) > 0) {
      send_to_char(g, ch, "You can't handle having your speed increased beyond 70.\r\n");
      return;
    }
    if (char_get_real_abils_intel(ch) + 2 > 70 && char_get_bonuses(ch, BONUS_DULL) > 0) {
      send_to_char(g, ch, "You can't handle having your intelligence increased beyond 70.\r\n");
      return;
    }
    decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (100));
    reveal_hiding(g, ch, 0);
    act(g, "@CYou focus your energy into your mind and awaken your latent Future Sight powers!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n focuses $s energy while closing $s eyes for a moment.@n", true, ch, 0, vict, TO_VICT);
    act(g, "@C$n focuses $s energy while closing $s eyes for a moment.@n", true, ch, 0, vict, TO_NOTVICT);
    char_aff_set(ch, AFF_FUTURE, true);
    char_set_real_abils_cha(ch, char_get_real_abils_cha(ch) + 5);
    char_set_real_abils_intel(ch, char_get_real_abils_intel(ch) + 2);
    char_set_position(vict, POS_SLEEPING);
    save_char(g, ch);
  }
}

ACMD(do_suppress)
{
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  if (char_is_android(ch)) {
    send_to_char(g, ch, "You are unable to suppress your powerlevel.\r\n");
    return;
  }
  if (char_get_bonuses(ch, BONUS_ARROGANT) > 0) {
    send_to_char(g, ch, "You are far too arrogant to hide your strength.\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(g, ch, "You are currently powering up, can't suppress.\r\n");
    return;
  }
  if (char_get_kaioken(ch)) {
    send_to_char(g, ch, "You are currently concentrating on kaioken!\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Suppress to what percent?\r\nSyntax: suppress (1 - 99 | release)\r\n");
    return;
  }

  if (!strcasecmp(arg, "release")) {
    if (char_get_suppression(ch)) {
      reveal_hiding(g, ch, 0);
      act(g, "@GYou stop suppressing your current powerlevel!@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@G$n smiles as a rush of power erupts around $s body briefly.@n", true, ch, 0, 0, TO_ROOM);
      char_set_suppression(ch, 0);
      return;
    }
    send_to_char(g, ch, "You are not suppressing!\r\n");
    return;
  }

  int num = atoi(arg);

  if (num > 99 || num <= 0) {
    send_to_char(g, ch, "Out of suppression range.\r\nSyntax: suppress (1 - 99 | release)\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);

  if (char_get_suppression(ch) != 0) {
    act(g, "@GYou alter your suppression level!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@G$n seems to concentrate for a moment.@n", true, ch, 0, 0, TO_ROOM);

  } else {
    act(g, "@GYou suppress your current powerlevel!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@G$n seems to concentrate for a moment.@n", true, ch, 0, 0, TO_ROOM);
  }
  char_set_suppression(ch, num);
  return;
}

ACMD(do_hass)
{

  int perc = 0, prob = 0;

  if (!check_skill(g, ch, SKILL_HASSHUKEN)) {
    return;
  }
  if ((getCurST(g, ch)) < getMaxST(g, ch) / 30) {
    send_to_char(g, ch, "You do not have enough stamina.\r\n");
    return;
  }

  perc = init_skill(g, ch, SKILL_HASSHUKEN);
  prob = axion_dice(g, 0);

  if (perc < prob) {
    reveal_hiding(g, ch, 0);
    act(g, "@WYou try to move your arms at incredible speeds but screw up and waste some of your stamina.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@C$n@W tries to move $s arms at incredible speeds but screws up and wastes some of $s stamina.@n", true, ch, 0, 0, TO_ROOM);
    decCurST(g, ch, getMaxST(g, ch) / 30, 0);
    improve_skill(g, ch, SKILL_HASSHUKEN, 0);
    return;
  }
  reveal_hiding(g, ch, 0);
  act(g, "@WYou concentrate and start to move your arms at incredible speeds.@n", true, ch, 0, 0, TO_CHAR);
  act(g, "@C$n@W concentrates and starts to move $s arms at incredible speeds.@n", true, ch, 0, 0, TO_ROOM);
  int duration = perc / 15;
  assign_affect(g, ch, AFF_HASS, SKILL_HASSHUKEN, duration, 0, 0, 0, 0, 0, 0);
  decCurST(g, ch, getMaxST(g, ch) / 30, 0);
  improve_skill(g, ch, SKILL_HASSHUKEN, 0);
  return;
}

ACMD(do_implant)
{

  struct obj_data *limb = nullptr, *obj = nullptr, *next_obj;
  struct char_data *vict = nullptr;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int found = false;

  two_arguments(argument, arg, arg2);

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Syntax: implant (rarm | larm | rleg | lleg) (target)\r\n");
    return;
  }

  if (!*arg2) {
    vict = ch;
  } else if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "That person isn't here.\r\n");
    return;
  }

  for (obj = char_get_inventory(ch); obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
    if (found == false && obj_get_vnum(g, obj) == 66 && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found = true;
      limb  = obj;
    }
  }
  if (found == false) {
    send_to_char(g, ch, "You do not have a cybernetic limb to implant.\r\n");
    return;
  }

  if (!(strcmp(arg, "rarm"))) {
    if (char_get_limb_condition(vict, (1) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(g, ch, "They already have a right arm!\r\n");
      }
      if (vict == ch) {
        send_to_char(g, ch, "You already have a right arm!\r\n");
      }
      return;
    }
    if (vict != ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n", true, ch, limb, vict, TO_CHAR);
      act(g, "@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n", true, ch, limb, vict, TO_VICT);
      act(g, "@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n", true, ch, limb, vict, TO_NOTVICT);
    }
    if (vict == ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n", true, ch, limb, 0, TO_CHAR);
      act(g, "@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right arm!@n", true, ch, limb, 0, TO_ROOM);
    }
    char_plr_set(vict, PLR_CRARM, true);
    obj_from_char(g, limb);
    extract_obj(g, limb);
    return;
  }
  if (!(strcmp(arg, "larm"))) {
    if (char_get_limb_condition(vict, (2) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(g, ch, "They already have a left arm!\r\n");
      }
      if (vict == ch) {
        send_to_char(g, ch, "You already have a left arm!\r\n");
      }
      return;
    }
    if (vict && vict != ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n", true, ch, limb, vict, TO_CHAR);
      act(g, "@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n", true, ch, limb, vict, TO_VICT);
      act(g, "@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n", true, ch, limb, vict, TO_NOTVICT);
    }
    if (vict == ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n", true, ch, limb, 0, TO_CHAR);
      act(g, "@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left arm!@n", true, ch, limb, 0, TO_ROOM);
    }
    char_plr_set(vict, PLR_CLARM, true);
    obj_from_char(g, limb);
    extract_obj(g, limb);
    return;
  }
  if (!(strcmp(arg, "rleg"))) {
    if (char_get_limb_condition(vict, (3) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(g, ch, "They already have a right leg!\r\n");
      }
      if (vict == ch) {
        send_to_char(g, ch, "You already have a right leg!\r\n");
      }
      return;
    }
    if (vict && vict != ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n", true, ch, limb, vict, TO_CHAR);
      act(g, "@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n", true, ch, limb, vict, TO_VICT);
      act(g, "@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n", true, ch, limb, vict, TO_NOTVICT);
    }
    if (vict == ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n", true, ch, limb, 0, TO_CHAR);
      act(g, "@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right leg!@n", true, ch, limb, 0, TO_ROOM);
    }
    char_plr_set(vict, PLR_CRLEG, true);
    obj_from_char(g, limb);
    extract_obj(g, limb);
    return;
  }
  if (!(strcmp(arg, "lleg"))) {
    if (char_get_limb_condition(vict, (4) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(g, ch, "They already have a left leg!\r\n");
      }
      if (vict == ch) {
        send_to_char(g, ch, "You already have a left leg!\r\n");
      }
      return;
    }
    if (vict && vict != ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n", true, ch, limb, vict, TO_CHAR);
      act(g, "@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n", true, ch, limb, vict, TO_VICT);
      act(g, "@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n", true, ch, limb, vict, TO_NOTVICT);
    }
    if (!vict || vict == ch) {
      reveal_hiding(g, ch, 0);
      act(g, "@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n", true, ch, limb, 0, TO_CHAR);
      act(g, "@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left leg!@n", true, ch, limb, 0, TO_ROOM);
    }
    char_plr_set(vict, PLR_CLLEG, true);
    obj_from_char(g, limb);
    extract_obj(g, limb);
    return;
  }
  send_to_char(g, ch, "Syntax: implant (rarm | larm | rleg | rleg)\r\n");
  return;
}

/* do_fury for halfbreeds to release their raaaage, rawrg! */

ACMD(do_fury)
{

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_is_halfbreed(ch) || char_is_npc(ch)) {
    send_to_char(g, ch, "You are furious, but you'll get over it.\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_FURY)) {
    send_to_char(g, ch, "You are already furious, your next attack will devestate, hurry use it!\r\n");
    return;
  }

  if (char_get_fury(ch) < 100) {
    send_to_char(g, ch, "You do not have enough anger to release your fury upon your foes!\r\n");
    return;
  }

  if (!*arg) {
    if (getCurPL(g, ch) < getEffMaxPL(g, ch)) {
      if ((getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.2) {
        restoreHealth(ch, false);
        decCurLFPercent(g, ch, .2, 0);
      } else {
        incCurHealth(g, ch, (getCurLF(g, ch)), true);
        decCurLFPercent(g, ch, 2, -1);
      }
    }
    char_set_fury(ch, 0);
  } else if (!strcasecmp(arg, "attack")) {
    char_set_fury(ch, 50);
  } else {
    send_to_char(g, ch, "Syntax: fury (attack) <--- this will not use up your LF to restore PL.\n        fury <--- fury by itself will do both LF to PL restore and attack boost.\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);
  act(g, "You release your fury! Your very next attack is guaranteed to rip your foes a new one!", true, ch, 0, 0, TO_CHAR);
  act(g, "$n screams furiously as a look of anger appears on $s face!", true, ch, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_FURY, true);
}

ACMD(do_majinize)
{

  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_is_majin(ch)) {
    send_to_char(g, ch, "You are not a majin and can not majinize anyone.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Who do you want to majinize?\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "That target isn't here.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(g, ch, "Why would you waste your time majinizing them?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(g, ch, "You can't majinize yourself.\r\n");
    return;
  }
  if (char_plr_flagged(vict, PLR_PR)) {
    send_to_char(g, ch, "You can't majinize them their potential has been released!\r\n");
    return;
  }
  int alignmentTotal = char_get_alignment(ch) - char_get_alignment(vict);
  if (char_get_majinize(vict) > 0 && char_get_majinize(vict) != char_get_id(ch)) {
    send_to_char(g, ch, "They are already majinized before by someone else.\r\n");
    return;
  }
  if ((char_get_master(vict) != ch)) {
    send_to_char(g, ch, "They must be following you in order for you to majinize them.\r\n");
    return;
  }

  else if (!((alignmentTotal >= -1500) && (alignmentTotal <= 1500))) {
    send_to_char(g, ch, "Their alignment is so opposed to your's that they resist your attempts to enslave them!\r\n");
    return;
  } else if (getEffMaxPL(g, vict) > getEffMaxPL(g, ch) * 4) {
    send_to_char(g, ch, "Their powerlevel is so much higher than yours they resist your attempts to enslave them!\r\n");
    return;
  }
  /* Rillao: transloc, add new transes here */
  else if (char_get_majinize(vict) > 0 && char_get_majinize(vict) == char_get_id(ch)) {
    reveal_hiding(g, ch, 0);
    act(g, "You remove $N's majinization, freeing them from your influence, but also weakening them.", true, ch, 0, vict, TO_CHAR);
    act(g, "$n removes your majinization, freeing you from their influence, and weakening you!", true, ch, 0, vict, TO_VICT);
    act(g, "$n waves a hand at $N, and instantly the glowing M on $S forehead disappears!", true, ch, 0, vict, TO_NOTVICT);
    char_set_majinize(vict, 0);
    char_set_boosts(ch, char_get_boosts(ch) + (1));

    if (char_get_majinizer(vict) == 0) {
      char_set_majinizer(vict, ((getBasePL(vict)) * .4));
    }
    loseBasePL(vict, char_get_majinizer(vict), false);
    return;
  } else if (char_get_boosts(ch) == 0) {
    send_to_char(g, ch, "You are incapable of majinizing%s.\r\n", char_get_level(ch) < 100 ? " right now" : " anymore");
    if (char_get_level(ch) < 25) {
      send_to_char(g, ch, "Your next available majinize will be at level 25\r\n");
    } else if (char_get_level(ch) < 50) {
      send_to_char(g, ch, "Your next available majinize will be at level 50\r\n");
    } else if (char_get_level(ch) < 75) {
      send_to_char(g, ch, "Your next available majinize will be at level 75\r\n");
    } else if (char_get_level(ch) < 100) {
      send_to_char(g, ch, "Your next available majinize will be at level 100\r\n");
    }
    return;
  } else {
    reveal_hiding(g, ch, 0);
    act(g, "You focus your power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", true, ch, 0, vict, TO_CHAR);
    act(g, "$n focuses power into you, influencing your mind and increasing your strength! After the struggle in your mind ends a glowing purple M forms on your forehead.", true, ch, 0, vict, TO_VICT);
    act(g, "$n focuses power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", true, ch, 0, vict, TO_NOTVICT);
    char_set_majinize(vict, char_get_id(ch));
    char_set_boosts(ch, char_get_boosts(ch) - (1));

    char_set_majinizer(vict, (getBasePL(vict)) * .4);
    gainBasePLPercent(vict, .4, true);
    return;
  }
}

ACMD(do_spit)
{
  int cost = 0;
  struct char_data *vict;
  struct affected_type af = {};
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(g, ch, SKILL_SPIT)) {
    return;
  }
  if (char_get_fighting(ch)) {
    send_to_char(g, ch, "You can't manage to spit in this fight!\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Yes but who do you want to petrify?\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "That target isn't here.\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0)) {
    return;
  }
  if (char_aff_flagged(vict, AFF_PARALYZE)) {
    act(g, "$N has already been turned to stone.", true, ch, 0, vict, TO_CHAR);
    return;
  }
  if (char_get_fighting(vict)) {
    send_to_char(g, vict, "You can't manage to spit on them, they are moving around too much!\r\n");
    return;
  }

  cost = ((getMaxKI(g, ch) / MAX(char_get_skill(ch, SKILL_SPIT) / 4, 1)) + getMaxKI(g, ch) / 100);

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki to petrifiy with your spit!\r\n");
    return;
  }

  if (char_get_skill(ch, SKILL_SPIT) < axion_dice(g, 0)) {
    decCurKI(g, ch, cost, 0);
    reveal_hiding(g, ch, 0);
    act(g, "@WGathering spit you concentrate ki into a wicked loogie and let it loose, but it falls short of hitting @c$N@W!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W seems to focus ki before hawking a loogie at you! Fortunatly the loogie falls short.@n", true, ch, 0, vict, TO_VICT);
    act(g, "@C$n@W seems to focus ki before hawking a loogie at @c$N@W! Fortunatly for @c$N@W the loogie falls short.@n", true, ch, 0, vict, TO_NOTVICT);
    improve_skill(g, ch, SKILL_SPIT, 1);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  if (char_aff_flagged(vict, AFF_ZANZOKEN) && (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
    decCurKI(g, ch, cost, 0);
    reveal_hiding(g, ch, 0);
    act(g, "@C$N@c disappears, avoiding your spit before reappearing!@n", false, ch, 0, vict, TO_CHAR);
    act(g, "@cYou disappear, avoiding @C$n's@c @rstone spit@c before reappearing!@n", false, ch, 0, vict, TO_VICT);
    act(g, "@C$N@c disappears, avoiding @C$n's@c @rstone spit@c before reappearing!@n", false, ch, 0, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    char_set_wait(ch, PULSE_2SEC);
    improve_skill(g, ch, SKILL_SPIT, 1);
    return;
  }
  af.type      = SPELL_PARALYZE;
  af.duration  = rand_number(g, 1, 2);
  af.modifier  = 0;
  af.location  = APPLY_NONE;
  af.bitvector = AFF_PARALYZE;
  affect_join(g, vict, &af, false, false, false, false);

  decCurKI(g, ch, cost, 0);
  reveal_hiding(g, ch, 0);
  act(g, "@WGathering spit you concentrate ki into a wicked loogie and let it loose, and it smacks into @c$N@W turning $M into stone!@n", true, ch, 0, vict, TO_CHAR);
  act(g, "@C$n@W seems to focus ki before hawking a loogie at you! It manages to hit and you instantly turn to stone!@n", true, ch, 0, vict, TO_VICT);
  act(g, "@C$n@W seems to focus ki before hawking a loogie at @c$N@W! It manages to hit and $E instantly turns to stone!@n", true, ch, 0, vict, TO_NOTVICT);
  improve_skill(g, ch, SKILL_SPIT, 1);
  char_set_wait(ch, PULSE_2SEC);
  return;
}

ACMD(do_absorb)
{
  struct char_data *vict = nullptr;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!check_skill(g, ch, SKILL_ABSORB)) {
    return;
  }

  if (char_is_android(ch)) {
    if (!limb_ok(g, ch, 0)) {
      return;
    }
  }
  if (!char_is_npc(ch)) {
    if (char_is_bio(ch) && !char_plr_flagged(ch, PLR_TAIL)) {
      send_to_char(g, ch, "You have no tail!\r\n");
      return;
    }
  }

  if (!char_is_android(ch) && !char_is_bio(ch)) {
    send_to_char(g, ch, "You shouldn't have this skill, you are incapable of absorbing.\r\n");
    send_to_imm(g, "ERROR: Absorb skill on %s when they are not a bio or android.", char_get_name(ch));
    return;
  }

  if (char_get_fighting(ch) && !char_is_android(ch)) {
    send_to_char(g, ch, "You are too busy fighting!\r\n");
    return;
  }

  if (char_get_grappled(ch)) {
    send_to_char(g, ch, "You are currently being grappled with! Try 'escape'!\r\n");
    return;
  }
  if (char_get_grappling(ch)) {
    send_to_char(g, ch, "You are currently grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(ch)) {
    act(g, "@WYou stop absorbing from @c$N@W!@n", true, ch, 0, char_get_absorbing(ch), TO_CHAR);
    act(g, "$n stops absorbing from you!", true, ch, 0, char_get_absorbing(ch), TO_VICT);
    act(g, "$n stops absorbing from $N!", true, ch, 0, char_get_absorbing(ch), TO_NOTVICT);
    if (char_is_npc(char_get_absorbing(ch)) && !char_get_fighting(char_get_absorbing(ch))) {
      set_fighting(g, char_get_absorbing(ch), ch);
    }
    char_set_absorbby(char_get_absorbing(ch), nullptr);
    char_set_absorbing(ch, nullptr);
  }

  if (!*arg && char_is_android(ch)) {
    send_to_char(g, ch, "Who do you want to absorb?\r\n");
    return;
  }

  if (char_is_android(ch)) {
    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Absorb %s?\r\n", char_is_android(ch) ? "from who" : "who");
      return;
    }
  }
  if (char_is_bio(ch)) {
    if (!*arg) {
      send_to_char(g, ch, "Syntax: absorb (swallow | extract) (target)\r\n");
      return;
    }
    if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Syntax: absorb (swallow | extract) (target)\r\n");
      return;
    }
  }
  if (char_aff_flagged(vict, AFF_SANCTUARY)) {
    send_to_char(g, ch, "You can't absorb them, they have a barrier!\r\n");
    return;
  }

  if (char_is_android(ch)) {
    if (!char_is_npc(ch)) {
      if (!char_plr_flagged(ch, PLR_ABSORB)) {
        send_to_char(g, ch, "You are not an absorbtion model.\r\n");
        return;
      }
    }
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (char_get_absorbby(vict)) {
      send_to_char(g, ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (getEffMaxPL(g, vict) > getEffMaxPL(g, ch) * 2) {
      send_to_char(g, ch, "They are too strong for you to absorb from.\r\n");
      return;
    }
    if (getEffMaxPL(g, vict) * 20 < getEffMaxPL(g, ch)) {
      send_to_char(g, ch, "They are too weak for you to bother absorbing from.\r\n");
      return;
    }
    if ((getCurST(g, vict)) < (getMaxST(g, vict) / 20) && (getCurKI(g, vict)) < (getMaxKI(g, vict) / 20)) {
      send_to_char(g, ch, "They have nothing to absorb right now, they are drained...\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    if (init_skill(g, ch, SKILL_ABSORB) < axion_dice(g, 0)) {
      act(g, "@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, ch, 0, vict, TO_NOTVICT);
      improve_skill(g, ch, SKILL_ABSORB, 1);
      if (char_is_npc(vict) && char_is_humanoid(vict) && rand_number(g, 1, 3) == 3) {
        if (char_get_fighting(ch) == nullptr) {
          set_fighting(g, ch, vict);
        }
        if (char_get_fighting(vict) == nullptr) {
          set_fighting(g, vict, ch);
        }
      }
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    act(g, "@WYou rush at @c$N@W and try to absorb from them, and manage to grab on!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W rushes at you and tries to grab you, and manages to grab on!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@C$n@w rushes at @c$N@W and tries to grab $M, and manages to grab on!@n", true, ch, 0, vict, TO_NOTVICT);
    improve_skill(g, ch, SKILL_ABSORB, 1);
    char_set_absorbing(ch, vict);
    char_set_absorbby(vict, ch);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } // End of android absorb

  else if (char_is_bio(ch) && !(strcmp(arg, "swallow"))) {
    if (char_get_absorbby(vict)) {
      send_to_char(g, ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (char_get_absorbs(ch) < 1) {
      send_to_char(g, ch, "You already have already absorbed 3 people.\r\n");
      return;
    }
    if (getEffMaxPL(g, vict) >= (getBasePL(ch)) * 3) {
      send_to_char(g, ch, "You are too weak to absorb them into your cellular structure!\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    if (char_get_skill(ch, SKILL_ABSORB) < axion_dice(g, 0)) {
      act(g, "@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, ch, 0, vict, TO_NOTVICT);
      improve_skill(g, ch, SKILL_ABSORB, 1);
      if (!char_get_fighting(ch)) {
        set_fighting(g, ch, vict);
      }
      if (!char_get_fighting(vict)) {
        set_fighting(g, vict, ch);
      }
      char_set_wait(ch, PULSE_3SEC);
    } else {
      act(g, "@WYou rush at @c$N@W and your tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing $m!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W rushes at you and $s tail engulfs you! $e quickly sucks your squirming body into $s tail, absorbing you!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n@w rushes at @c$N@W and $s tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
      char_set_absorbs(ch, char_get_absorbs(ch) - (1));

      int64_t stam = (getBaseST(vict)) / 5;
      int64_t ki   = (getBaseKI(vict)) / 5;
      int64_t pl   = (getBasePL(vict)) / 5;

      gainBasePL(ch, pl, true);
      gainBaseST(ch, stam, true);
      gainBaseKI(ch, ki, true);

      if (!char_is_npc(vict) && !char_is_npc(ch)) {
        send_to_imm(g, "[PK] %s killed %s at room [%d]\r\n", char_get_name(ch), char_get_name(vict), rroom_get_vnum(g, char_get_in_room(vict)));
        char_plr_set(vict, PLR_ABSORBED, true);
      }

      send_to_char(g, ch, "@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
      improve_skill(g, ch, SKILL_ABSORB, 1);
      die(g, vict, nullptr);
    }
  } // End of bio absorb
  else if (char_is_bio(ch) && !(strcmp(arg, "extract"))) {
    int failthresh = rand_number(g, 1, 125);
    if (char_get_level(vict) > 99) {
      failthresh += (char_get_level(vict) - 95) * 2;
    }
    if (char_get_absorbby(vict)) {
      send_to_char(g, ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (getEffMaxPL(g, vict) >= getEffMaxPL(g, ch)) {
      send_to_char(g, ch, "You are too weak to absorb them into your cellular structure!\r\n");
      return;
    }
    if (getEffMaxPL(g, vict) < getEffMaxPL(g, ch) / 5) {
      send_to_char(g, ch, "They would be worthless to you at your strength!\r\n");
      return;
    }
    if (!char_is_npc(vict)) {
      send_to_char(g, ch, "You can't absorb their bio extract, you need to swallow them with your tail!\r\n");
      return;
    }
    if (is_soft_cap(ch, 0, 1.0)) {
      send_to_char(g, ch, "You can not handle any more bio extract at your current level.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_ABSORB) < failthresh) {
      act(g, "@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, ch, 0, vict, TO_NOTVICT);
      improve_skill(g, ch, SKILL_ABSORB, 0);
      if (!char_get_fighting(ch)) {
        set_fighting(g, ch, vict);
      }
      if (!char_get_fighting(vict)) {
        set_fighting(g, vict, ch);
      }
      char_set_wait(ch, PULSE_4SEC);
    }
    /* Rillao: transloc, add new transes here */
    else {
      act(g, "@WYou rush at @c$N@W and stab them with your tail! You quickly suck out all the bio extract you need and leave the empty husk behind!", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@w rushes at @c$N@W and stabs $M with $s tail! $e quickly sucks out all the bio extract and leaves the empty husk of @c$N@W behind!@n", true, ch, 0, vict, TO_NOTVICT);
      int64_t stam = (getBaseST(vict)) / 2000;
      int64_t ki   = (getBaseKI(vict)) / 2000;
      int64_t pl   = (getBasePL(vict)) / 2000;
      stam += rand_number(g, char_get_level(ch), char_get_level(ch) * 2);
      pl += rand_number(g, char_get_level(ch), char_get_level(ch) * 2);
      ki += rand_number(g, char_get_level(ch), char_get_level(ch) * 2);
      stam = MIN(stam, 1500000L);
      ki   = MIN(ki, 1500000L);
      pl   = MIN(pl, 1500000L);

      gainBasePL(ch, pl, true);
      gainBaseST(ch, stam, true);
      gainBaseKI(ch, ki, true);
      incCurLFPercent(g, ch, .05, true);
      send_to_char(g, ch, "@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
      improve_skill(g, ch, SKILL_ABSORB, 0);
      char_set_wait(ch, PULSE_4SEC);
      char_mob_set(vict, MOB_HUSK, true);
      die(g, vict, ch);
    }
  }

  else {
    if (!char_is_bio(ch) && !char_is_android(ch)) {
      send_to_char(g, ch, "You have the absorb skill but are incapable of absorbing. This error has been reported.\r\n");
      send_to_imm(g, "ERROR: Absorb attempted by %s even though they are not bio or android.", char_get_name(ch));
    } else {
      send_to_char(g, ch, "Syntax: absorb (extract | swallow) (target)\r\n");
    }
    return;
  } // Error
}

ACMD(do_regenerate)
{

  int64_t amt = 0;
  int skill   = 0;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  skill = init_skill(g, ch, SKILL_REGENERATE);

  if (skill < 1) {
    send_to_char(g, ch, "You are incapable of regenerating.\r\n");
    return;
  }
  if (char_get_suppression(ch) > 0) {
    skill = char_get_suppression(ch);
  }

  if (!*arg) {
    send_to_char(g, ch, "Regenerate how much PL?\r\nMax percent you can regen: %d\r\nSyntax: regenerate (1 - 100)\r\n", skill);
    return;
  }

  if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
    send_to_char(g, ch, "You do not need to regenerate, you are at full health.\r\n");
    return;
  }

  if (char_get_suppression(ch) > 0 && getCurPL(g, ch) >= ((getEffMaxPL(g, ch) / 100) * char_get_suppression(ch))) {
    send_to_char(g, ch, "You do not need to regenerate, you are at full health.\r\n");
    return;
  }

  int num = atoi(arg);

  if (num <= 0) {
    send_to_char(g, ch, "What is the point of that?\r\nSyntax: regenerate (1 - 100)\r\n");
    return;
  }

  if (num > 100) {
    send_to_char(g, ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
    return;
  }

  if (num > skill) {
    send_to_char(g, ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
    return;
  }

  if (char_get_suppression(ch) > 0 && num > char_get_suppression(ch)) {
    send_to_char(g, ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
    return;
  }

  amt = (getEffMaxPL(g, ch) * 0.01) * num;
  if (amt > 1)
    amt /= 2;

  if (char_is_bio(ch)) {
    amt = amt * 0.9;
  }

  int64_t life = ((getCurLF(g, ch)) - amt * 0.8), energy = ((getCurKI(g, ch)) - amt * 0.2);

  if ((life <= 0 || energy <= 0) && !char_is_npc(ch)) {
    send_to_char(g, ch, "Your life force or ki are too low to regenerate that much.\r\n");
    send_to_char(g, ch, "@YLF Needed@D: @C%s@w, @YKi Needed@D: @C%s@w.@n\r\n", add_commas(amt * 0.8), add_commas(amt * 0.2));
    return;
  }
  if (char_is_npc(ch) && energy <= 0) {
    return;
  }

  incCurHealth(g, ch, amt * 2, true);

  if (!char_is_npc(ch))
    decCurLF(g, ch, amt * .8, 0);

  decCurKI(g, ch, amt * .2, 0);

  reveal_hiding(g, ch, 0);

  if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
    act(g, "You concentrate your ki and regenerate your body completely.", true, ch, 0, 0, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body completely.", true, ch, 0, 0, TO_ROOM);
  } else if (amt < getEffMaxPL(g, ch) / 10) {
    act(g, "You concentrate your ki and regenerate your body a little.", true, ch, 0, 0, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body a little.", true, ch, 0, 0, TO_ROOM);
  } else if (amt < getEffMaxPL(g, ch) / 5) {
    act(g, "You concentrate your ki and regenerate your body some.", true, ch, 0, 0, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body some.", true, ch, 0, 0, TO_ROOM);
  } else if (amt < getEffMaxPL(g, ch) / 2) {
    act(g, "You concentrate your ki and regenerate your body a great deal.", true, ch, 0, 0, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body a great deal.", true, ch, 0, 0, TO_ROOM);
  } else if (getCurPL(g, ch) < getEffMaxPL(g, ch)) {
    act(g, "You concentrate your ki and regenerate you nearly completely.", true, ch, 0, 0, TO_CHAR);
    act(g, "$n concentrates and regenerates $s body nearly completely.", true, ch, 0, 0, TO_ROOM);
  }
  improve_skill(g, ch, SKILL_REGENERATE, 0);
  if (char_aff_flagged(ch, AFF_BURNED)) {
    send_to_char(g, ch, "Your burns are healed now.\r\n");
    act(g, "$n@w's burns are now healed.@n", true, ch, 0, 0, TO_ROOM);
    null_affect(g, ch, AFF_BURNED);
  }

  if (!char_is_npc(ch)) {
    if (char_get_limb_condition(ch, (1) - 1) <= 0) {
      act(g, "You regrow your right arm!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regrows $s right arm!", true, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (1) - 1, 100);
    } else if (char_get_limb_condition(ch, (1) - 1) >= 0 && char_get_limb_condition(ch, (1) - 1) < 50) {
      act(g, "Your broken right arm mends itself!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regenerates $s broken right arm!", true, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (1) - 1, 100);
    }
    if (char_get_limb_condition(ch, (2) - 1) <= 0) {
      char_set_limb_condition(ch, (2) - 1, 100);
      act(g, "You regrow your left arm!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regrows $s left arm!", true, ch, 0, 0, TO_ROOM);
    } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50) {
      act(g, "Your broken left arm mends itself!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regenerates $s broken left arm!", true, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (2) - 1, 100);
    }
    if (char_get_limb_condition(ch, (4) - 1) <= 0) {
      char_set_limb_condition(ch, (4) - 1, 100);
      act(g, "You regrow your left leg!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regrows $s left leg!", true, ch, 0, 0, TO_ROOM);
    } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50) {
      act(g, "Your broken left leg mends itself!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regenerates $s broken left leg!", true, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (4) - 1, 100);
    }
    if (char_get_limb_condition(ch, (3) - 1) <= 0) {
      char_set_limb_condition(ch, (3) - 1, 100);
      act(g, "You regrow your right leg!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regrows $s right leg!", true, ch, 0, 0, TO_ROOM);
    } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50) {
      char_set_limb_condition(ch, (3) - 1, 100);
      act(g, "Your broken right leg mends itself!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regenerates $s broken right leg!", true, ch, 0, 0, TO_ROOM);
    }
    if (!char_plr_flagged(ch, PLR_TAIL) && char_is_bio(ch)) {
      char_plr_set(ch, PLR_TAIL, true);
      act(g, "You regrow your tail!", true, ch, 0, 0, TO_CHAR);
      act(g, "$n regrows $s tail!", true, ch, 0, 0, TO_ROOM);
    }
    improve_skill(g, ch, SKILL_REGENERATE, 0);
  }
}

static const int64_t kaioken_levels[21] = {
    0,      // 0 - unused
    0,      // 1
    0,      // 2
    5000,   // 3
    10000,  // 4
    15000,  // 5
    25000,  // 6
    35000,  // 7
    50000,  // 8
    75000,  // 9
    100000, // 10
    150000, // 11
    200000, // 12
    250000, // 13
    300000, // 14
    400000, // 15
    500000, // 16
    600000, // 17
    700000, // 18
    800000, // 19
    1000000 // 20
};

ACMD(do_kaioken)
{
  char arg[MAX_INPUT_LENGTH];
  int roll = axion_dice(g, 0), x = 0;
  one_argument(argument, arg);

  if (!check_skill(g, ch, SKILL_KAIOKEN)) {
    return;
  }
  if (char_get_alignment(ch) <= -50) {
    send_to_char(g, ch, "Your heart is too corrupt to use that technique!\r\n");
    return;
  }
  if (!char_is_npc(ch)) {
    if (char_plr_flagged(ch, PLR_HEALT)) {
      send_to_char(g, ch, "You are inside a healing tank!\r\n");
      return;
    }
  }

  if (!*arg) {
    send_to_char(g, ch, "What level of kaioken do you want to try and achieve?\r\n"
                     "Syntax: kaioken 1-20\r\n");
    return;
  }

  x = atoi(arg);

  if (x < 0 || x > 20) {
    send_to_char(g, ch, "That level of kaioken dosn't exist...\r\n"
                     "Syntax: kaioken 0-20\r\n");
    return;
  }

  if (x == 0) {
    if (char_get_kaioken(ch) > 0) {
      remove_kaioken(g, ch, 1);
      return;
    }
    send_to_char(g, ch, "You are not in kaioken!\r\n");
    return;
  }

  if (x == char_get_kaioken(ch)) {
    send_to_char(g, ch, "You are already at that kaioken level! To release, try kaioken 0\r\n");
    return;
  }

  if (!char_is_npc(ch)) {
    if ((char_is_transformed(ch) || (char_is_hoshijin(ch) && char_get_starphase(ch) > 0)) && x > 5) {
      send_to_char(g, ch, "You can not manage a kaioken level higher than 5 when transformed.\r\n");
      return;
    }
  }

  int64_t cost_unit = getMaxKI(g, ch) / 50;
  int64_t cost_diff = cost_unit * char_get_kaioken(ch);
  int64_t cost      = (cost_unit * x) - cost_diff;

  // it costs nothing to reduce your kaioken level.
  if (x < char_get_kaioken(ch)) {
    cost = 0;
  }

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki to focus into your body for that level.\r\n");
    return;
  }

  int xnum = (x * 5) + 1;
  roll     = rand_number(g, 1, xnum);
  reveal_hiding(g, ch, 0);

  decCurKI(g, ch, cost, 0);
  improve_skill(g, ch, SKILL_KAIOKEN, 1);

  if (init_skill(g, ch, SKILL_KAIOKEN) < roll) {
    send_to_char(g, ch, "You try to focus your ki into your body but mess up somehow.\r\n");
    act(g, "$n tries to use kaioken but messes up somehow.", true, ch, 0, 0, TO_ROOM);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }

  if (getMaxHealth(g, ch) < kaioken_levels[x]) {
    act(g, "@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", true, ch, 0, 0, TO_ROOM);
    return;
  }

  apply_kaioken(g, ch, x, true);

  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_zanzoken)
{

  int prob = 0, perc = 0;
  int64_t cost = 0;

  if (!know_skill(g, ch, SKILL_ZANZOKEN) && !char_is_npc(ch)) {
    return;
  }

  if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
    char_aff_set(ch, AFF_ZANZOKEN, false);
    send_to_char(g, ch, "You release the ki you had prepared for a zanzoken.\r\n");
    return;
  }

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(g, ch, "You are busy in a grapple!\r\n");
    return;
  }

  if (!char_is_npc(ch)) {
    prob = char_get_skill(ch, SKILL_ZANZOKEN);
  } else {
    prob = rand_number(g, 80, 90);
  }
  perc = axion_dice(g, 0);
  cost = getMaxKI(g, ch) / 50;

  if (prob > 75) {
    cost *= 2;
  } else if (prob > 50) {
    cost *= 4;
  } else if (prob >= 25) {
    cost *= 8;
  } else if (prob < 25) {
    cost *= 10;
  }

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }

  if (prob < perc) {
    send_to_char(g, ch, "You focus your ki in preparation of a zanzoken but mess up and waste your ki!\r\n");
    improve_skill(g, ch, SKILL_ZANZOKEN, 2);
    decCurKI(g, ch, cost, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  act(g, "@wYou focus your ki, preparing to move at super speeds if necessary.@n", true, ch, 0, 0, TO_CHAR);
  decCurKI(g, ch, cost, 0);
  char_aff_set(ch, AFF_ZANZOKEN, true);
  improve_skill(g, ch, SKILL_ZANZOKEN, 2);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_eyec)
{

  if (char_is_npc(ch))
    return;

  if (char_plr_flagged(ch, PLR_EYEC)) {
    char_plr_set(ch, PLR_EYEC, false);
    act(g, "@wYou open your eyes.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@C$n@w opens $s eyes.@n", true, ch, 0, 0, TO_ROOM);
  }

  else if (!char_plr_flagged(ch, PLR_EYEC)) {
    char_plr_set(ch, PLR_EYEC, true);
    act(g, "@wYou close your eyes.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@C$n@w closes $s eyes.@n", true, ch, 0, 0, TO_ROOM);
  }

  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_solar)
{
  struct char_data *vict = nullptr, *next_v = nullptr;

  int prob = 0, perc = 0, cost = 0, bonus = 0;

  if (!know_skill(g, ch, SKILL_SOLARF)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  prob = char_get_skill(ch, SKILL_SOLARF);
  perc = rand_number(g, 0, 101);

  if (prob >= 75) {
    cost = getMaxKI(g, ch) / 50;
  } else if (prob >= 50) {
    cost = getMaxKI(g, ch) / 25;
  } else if (prob >= 25) {
    cost = getMaxKI(g, ch) / 20;
  } else if (prob < 25) {
    cost = getMaxKI(g, ch) / 15;
  }

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }

  bonus = char_get_aff_abils_intel(ch) / 3;
  prob += bonus;

  if (prob < perc) {
    act(g, "@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' but nothing happens!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' but nothing happens!@n", true, ch, 0, 0, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_SOLARF, 0);
    return;
  }

  act(g, "@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' as a blinding light fills the area!@n", true, ch, 0, 0, TO_CHAR);
  act(g, "@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' as a blinding light fills the area!@n", true, ch, 0, 0, TO_ROOM);

  for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = next_v) {
    next_v = char_get_next_in_room(vict);

    if (vict == ch)
      continue;
    else if (char_plr_flagged(vict, PLR_EYEC))
      continue;
    else if (char_aff_flagged(vict, AFF_BLIND))
      continue;
    else if (char_get_position(vict) == POS_SLEEPING)
      continue;
    else {
      int duration = 1;
      assign_affect(g, vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
      act(g, "@W$N@W is @YBLINDED@W!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@RYou are @YBLINDED@R!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@W$N@W is @YBLINDED@W!@n", true, ch, 0, vict, TO_NOTVICT);
    }
  }
  improve_skill(g, ch, SKILL_SOLARF, 0);
  decCurKI(g, ch, cost, 0);
  char_set_wait(ch, PULSE_3SEC);
}

ACMD(do_heal)
{
  int64_t cost = 0, prob = 0, perc = 0, heal = 0, bonus = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!check_skill(g, ch, SKILL_HEAL)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "You want to heal WHO?\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "You do not see the target here.\r\n");
    return;
  }

  prob = init_skill(g, ch, SKILL_HEAL);
  perc = rand_number(g, 0, 110);

  if (prob >= 100) {
    cost = getMaxKI(g, ch) / 20;
    heal = getEffMaxPL(g, vict) / 5;
  } else if (prob >= 90) {
    cost = getMaxKI(g, ch) / 16;
    heal = getEffMaxPL(g, vict) / 10;
  } else if (prob >= 75) {
    cost = getMaxKI(g, ch) / 14;
    heal = getEffMaxPL(g, vict) / 12;
  } else if (prob >= 50) {
    cost = getMaxKI(g, ch) / 12;
    heal = getEffMaxPL(g, vict) / 15;
  } else if (prob >= 25) {
    cost = getMaxKI(g, ch) / 10;
    heal = getEffMaxPL(g, vict) / 20;
  } else if (prob < 25) {
    cost = getMaxKI(g, ch) / 6;
    heal = getEffMaxPL(g, vict) / 20;
  }

  if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
    heal += heal * .1;
  }

  if (heal < getEffMaxPL(g, vict)) {
    heal += (heal / 100) * (char_get_aff_abils_wis(ch) / 4);
  }

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }

  if (getCurPL(g, vict) >= getEffMaxPL(g, vict)) {
    if (vict != ch) {
      send_to_char(g, ch, "They are already at full health.\r\n");
    } else {
      send_to_char(g, ch, "You are already at full health.\r\n");
    }
    return;
  }

  if (char_get_suppression(vict) > 0 && getCurPL(g, vict) >= ((getEffMaxPL(g, vict) / 100) * char_get_suppression(vict))) {
    send_to_char(g, ch, "They are already at full health.\r\n");
    return;
  }

  bonus = (char_get_aff_abils_intel(ch) / 2) + (char_get_aff_abils_wis(ch) / 3);
  prob += bonus;

  if (prob < perc) {
    if (vict != ch) {
      act(g, "@WYou place your hands near @c$N@W, but fail to concentrate enough to heal them!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W places $s hands near you, but nothing happens!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n@W places $s hands near @c$N@W, but nothing happens.", true, ch, 0, vict, TO_NOTVICT);
      decCurKI(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_HEAL, 0);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    if (vict == ch) {
      act(g, "@WYou place your hands on your body, but fail to concentrate to heal yourself!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W places $s hands on $s body, but nothing happens.", true, ch, 0, vict, TO_NOTVICT);
      decCurKI(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_HEAL, 0);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
  }

  if (vict != ch) {
    if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
      heal += heal * .25;
    }
    act(g, "@WYou place your hands near @c$N@W and an orange glow surrounds $M!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W places $s hands near you and an orange glow surrounds you!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@C$n@W places $s hands near @c$N@W and an orange glow surrounds $M.", true, ch, 0, vict, TO_NOTVICT);
    decCurKI(g, ch, cost, 0);
    incCurHealth(g, vict, heal, true);

    if (char_is_nail(ch)) {
      if (char_get_skill(ch, SKILL_HEAL) >= 100) {
        incCurST(g, vict, heal * .4, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 60) {
        incCurST(g, vict, heal * .2, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 40) {
        incCurST(g, vict, heal * .1, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      }
    }

    null_affect(g, vict, AFF_POISON);
    null_affect(g, vict, AFF_BLIND);
    if (char_aff_flagged(vict, AFF_BURNED)) {
      send_to_char(g, vict, "Your burns are healed now.\r\n");
      act(g, "$n@w's burns are now healed.@n", true, vict, 0, 0, TO_ROOM);
      char_aff_set(vict, AFF_BURNED, false);
    }
    if (is_affected(vict, AFF_HYDROZAP)) {
      send_to_char(g, vict, "You no longer feel a great thirst.\r\n");
      act(g, "$n@w no longer looks as if they could drink an ocean.@n", true, vict, 0, 0, TO_ROOM);
      remove_affect(g, vict, AFF_HYDROZAP);
    }
    char_set_limb_condition(vict, (1) - 1, 100);
    char_set_limb_condition(vict, (2) - 1, 100);
    char_set_limb_condition(vict, (3) - 1, 100);
    char_set_limb_condition(vict, (4) - 1, 100);
    if (!char_plr_flagged(vict, PLR_TAIL) && (char_is_bio(vict) || char_is_icer(vict)) && !char_is_npc(vict)) {
      char_plr_set(vict, PLR_TAIL, true);
    }
    if (!char_plr_flagged(vict, PLR_STAIL) && (char_is_saiyan(vict) || char_is_halfbreed(vict)) && !char_is_npc(vict)) {
      char_plr_set(vict, PLR_STAIL, true);
    }
    if (getCurLF(g, vict) <= getMaxLF(g, vict) * 0.5 && !char_is_android(vict)) {
      incCurLF(g, vict, (getMaxLF(g, vict)) * .35, true);
      send_to_char(g, vict, "You feel that your lifeforce has recovered some!\r\n");
    }
    improve_skill(g, ch, SKILL_HEAL, 0);
    if (char_get_master(vict) == ch || char_get_master(ch) == vict || char_get_master(ch) == char_get_master(vict)) {
      if (char_is_nail(ch) && char_is_namek(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && getCurPL(g, vict) <= getEffMaxPL(g, vict) * 0.85 && rand_number(g, 1, 3) == 3) {
        char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.005));
      }
    }

    char_set_wait(ch, PULSE_2SEC);
  }

  if (vict == ch) {
    if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
      heal += heal * .25;
    }
    act(g, "@WYou place your hands on your body and an orange glow surrounds you!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W places $s hands on $s body and an orange glow surrounds $m.", true, ch, 0, vict, TO_NOTVICT);
    decCurKI(g, ch, cost, 0);
    incCurHealth(g, vict, heal, true);

    if (char_is_nail(ch)) {
      if (char_get_skill(ch, SKILL_HEAL) >= 100) {
        incCurST(g, vict, heal * .4, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 60) {
        incCurST(g, vict, heal * .2, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 40) {
        incCurST(g, vict, heal * .1, true);
        send_to_char(g, vict, "@GYou feel some of your stamina return as well!@n\r\n");
      }
    }
    null_affect(g, vict, AFF_POISON);
    null_affect(g, vict, AFF_BLIND);
    char_set_limb_condition(vict, (1) - 1, 100);
    char_set_limb_condition(vict, (2) - 1, 100);
    char_set_limb_condition(vict, (3) - 1, 100);
    char_set_limb_condition(vict, (4) - 1, 100);
    if (!char_plr_flagged(vict, PLR_TAIL) && (char_is_bio(vict) || char_is_icer(vict)) && !char_is_npc(vict)) {
      char_plr_set(vict, PLR_TAIL, true);
    }
    if (!char_plr_flagged(vict, PLR_STAIL) && (char_is_saiyan(vict) || char_is_halfbreed(vict)) && !char_is_npc(vict)) {
      char_plr_set(vict, PLR_STAIL, true);
    }
    improve_skill(g, ch, SKILL_HEAL, 0);
    char_set_wait(ch, PULSE_2SEC);
  }

  return;
}

ACMD(do_barrier)
{
  int prob = 0, perc = 0, size = 0;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(g, ch, SKILL_BARRIER) && !char_get_skill(ch, SKILL_AQUA_BARRIER)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "[Syntax] barrier < 1-75 | release >\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SANCTUARY) && !strcasecmp("release", arg)) {
    act(g, "@BYou dispel your barrier, releasing its energy.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@B$n@B dispels $s barrier, releasing its energy.@n", true, ch, 0, 0, TO_ROOM);
    char_set_barrier(ch, 0);
    char_aff_set(ch, AFF_SANCTUARY, false);
    return;
  }
  if (!strcasecmp("release", arg)) {
    send_to_char(g, ch, "You don't have a barrier.\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SANCTUARY)) {
    send_to_char(g, ch, "You already have a barrier, try releasing it.\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before concentrating again.\r\n");
    return;
  }

  size         = atoi(arg);
  int64_t cost = 0;
  prob         = 0;
  if (char_get_skill(ch, SKILL_BARRIER)) {
    prob = init_skill(g, ch, SKILL_BARRIER);
  } else {
    prob = char_get_skill(ch, SKILL_AQUA_BARRIER);
  }
  perc = axion_dice(g, 0);

  cost = (getMaxKI(g, ch) * 0.01) * (size * 0.5);

  if (size > prob) {
    send_to_char(g, ch, "You can not create a barrier that is stronger than your skill in barrier.\r\n");
    return;
  }
  if (size < 1) {
    send_to_char(g, ch, "You have to put at least some ki into the barrier!\r\n");
    return;
  }
  if (size > 75) {
    send_to_char(g, ch, "You can't control a barrier with more than 75 percent!\r\n");
    return;
  }
  if (char_get_charge(ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki charged up!\r\n");
    return;
  }
  if (prob < perc) {
    act(g, "@BYou shout as you form a barrier of ki around your body, but you imbalance it and it explodes outward!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@B$n@B shouts as $e forms a barrier of ki around $s body, but it becomes imbalanced and explodes outward!@n", true, ch, 0, 0, TO_ROOM);
    char_set_charge(ch, char_get_charge(ch) - (cost));
    if (char_get_skill(ch, SKILL_BARRIER)) {
      improve_skill(g, ch, SKILL_BARRIER, 2);
    } else {
      improve_skill(g, ch, SKILL_AQUA_BARRIER, 2);
    }
    char_set_con_cooldown(ch, 30);
    return;
  }
  if (char_get_skill(ch, SKILL_BARRIER)) {
    act(g, "@BYou shout as you form a barrier of ki around your body!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@B$n@B shouts as $e forms a barrier of ki around $s body!@n", true, ch, 0, 0, TO_ROOM);
  } else {
    act(g, "@BYou shout as you form a barrier of ki and raging waters around your body!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@B$n@B shouts as $e forms a barrier of ki and raging waters around $s body!@n", true, ch, 0, 0, TO_ROOM);
  }
  char_set_barrier(ch, (getMaxKI(g, ch) / 100) * size);
  char_set_charge(ch, char_get_charge(ch) - (cost));
  if (char_get_skill(ch, SKILL_BARRIER)) {
    improve_skill(g, ch, SKILL_BARRIER, 2);
  } else {
    improve_skill(g, ch, SKILL_AQUA_BARRIER, 2);
  }
  char_aff_set(ch, AFF_SANCTUARY, true);
  char_set_con_cooldown(ch, 20);
  return;
}

ACMD(do_instant)
{

  int skill = 0, perc = 0, skill_num = 0;
  room_vnum location = 0;
  int64_t cost          = 0;
  struct char_data *tar = nullptr;

  char arg[MAX_INPUT_LENGTH] = "";

  one_argument(argument, arg);

  if (!char_is_npc(ch)) {
    if (char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
      char_prf_set(g, ch, PRF_ARENAWATCH, false);
      char_set_arenawatch(ch, -1);
      send_to_char(g, ch, "You stop watching the arena action.\r\n");
    }
  }
  if (!know_skill(g, ch, SKILL_INSTANTT)) {
    return;
  }
  if (!char_get_skill(ch, SKILL_SENSE) && !char_plr_flagged(ch, PLR_SENSEM)) {
    send_to_char(g, ch, "You can't sense them to go to there!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (rroom_get_vnum(g, char_get_in_room(ch)) >= 19800 && rroom_get_vnum(g, char_get_in_room(ch)) <= 19899) {
    send_to_char(g, ch, "@rYou are in a pocket dimension!@n\r\n");
    return;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_RHELL) || rroom_flagged(g, char_get_in_room(ch), ROOM_AL) || rroom_flagged(g, char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(g, ch, "You can not leave where you are at!\r\n");
    return;
  }
  if (!*arg) {
    send_to_char(g, ch, "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
    send_to_char(g, ch, "Example: instant goku\nExample 2: instant planet-earth\r\n");
    return;
  }

  if (char_get_skill(ch, SKILL_INSTANTT) > 75) {
    cost = getMaxKI(g, ch) / 40;
  } else if (char_get_skill(ch, SKILL_INSTANTT) > 50) {
    cost = getMaxKI(g, ch) / 20;
  } else if (char_get_skill(ch, SKILL_INSTANTT) > 25) {
    cost = getMaxKI(g, ch) / 15;
  } else if (char_get_skill(ch, SKILL_INSTANTT) < 25) {
    cost = getMaxKI(g, ch) / 10;
  }

  if ((getCurKI(g, ch)) - cost < 0) {
    send_to_char(g, ch, "You do not have enough ki to instantaneously move.\r\n");
    return;
  }

  perc      = axion_dice(g, 0);
  skill     = char_get_skill(ch, SKILL_INSTANTT);
  skill_num = SKILL_INSTANTT;

  if (!strcasecmp(arg, "planet-earth")) {
    location = 300;
  } else if (!strcasecmp(arg, "planet-namek")) {
    location = 10222;
  } else if (!strcasecmp(arg, "planet-frigid")) {
    location = 4017;
  } else if (!strcasecmp(arg, "planet-vegeta")) {
    location = 2200;
  } else if (!strcasecmp(arg, "planet-konack")) {
    location = 8006;
  } else if (!strcasecmp(arg, "planet-aether")) {
    location = 12024;
  } else if (!(tar = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "@RThat target was not found.@n\r\n");
    send_to_char(g, ch, "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
    send_to_char(g, ch, "Example: instant goku\nExample 2: instant planet-earth\r\n");
    return;
  }

  if (skill < perc || (char_get_fighting(ch) && rand_number(g, 1, 2) <= 1)) {
    if (tar != nullptr) {
      if (tar != ch) {
        send_to_char(g, ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
        decCurKI(g, ch, cost, 0);
        improve_skill(g, ch, skill_num, 2);
        char_set_wait(ch, PULSE_2SEC);
        return;
      }
      send_to_char(g, ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
      return;
    }
    send_to_char(g, ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
    decCurKI(g, ch, cost, 0);
    improve_skill(g, ch, skill_num, 2);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  reveal_hiding(g, ch, 0);
  char_set_wait(ch, PULSE_2SEC);
  if (tar != nullptr) {
    if (tar == ch) {
      send_to_char(g, ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
      return;
    }
    if (char_get_grappling(ch) && char_get_grappling(ch) == tar) {
      send_to_char(g, ch, "You are already in the same room with them and are grappling with them!\r\n");
      return;
    }
    if (!read_sense_memory(g, ch, tar)) {
      send_to_char(g, ch, "You've never sensed them up close so you do not have a good bearing on their ki signal.\r\n");
      return;
    }
    if (char_get_admlevel(tar) > 0 && char_get_admlevel(ch) < 1) {
      send_to_char(g, ch, "That immortal prevents you from reaching them.\r\n");
      return;
    }
    if (char_is_android(tar) || getCurPL(g, tar) < (getCurPL(g, ch) * 0.001) + 1) {
      send_to_char(g, ch, "You can't sense them well enough.\r\n");
      return;
    }
    if (!rroom_flagged(g, char_get_in_room(ch), ROOM_AL) && rroom_flagged(g, char_get_in_room(tar), ROOM_AL)) {
      send_to_char(g, ch, "They are dead and can't be reached.\r\n");
      return;
    }
    if (!rroom_flagged(g, char_get_in_room(ch), ROOM_RHELL) && rroom_flagged(g, char_get_in_room(tar), ROOM_RHELL)) {
      send_to_char(g, ch, "They are dead and can't be reached.\r\n");
      return;
    }
    if (rroom_flagged(g, char_get_in_room(tar), ROOM_NOINSTANT)) {
      send_to_char(g, ch, "You can not go there as it is a protected area!\r\n");
      return;
    }

    decCurKI(g, ch, cost, 0);
    act(g, "@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move through the molecules of the universe faster than the speed of light. You stop as you arrive at $N@w!@n", true, ch, 0, tar, TO_CHAR);
    act(g, "@w$n@w appears in an instant out of nowhere right next to you!@n", true, ch, 0, tar, TO_VICT);
    act(g, "@w$n@w places two fingers on $s forehead and disappears in an instant!@n", true, ch, 0, tar, TO_NOTVICT);
    char_plr_set(ch, PLR_TRANSMISSION, true);
    handle_teleport(g, ch, tar, 0);
    improve_skill(g, ch, skill_num, 2);
  } else {
    decCurKI(g, ch, cost, 0);
    act(g, "@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move faster than light and arrive almost instantly at your destination. Having located the planet by its collective population's ki.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@w$n@w places two fingers on $s forehead and disappears in an instant!@n", true, ch, 0, 0, TO_NOTVICT);
    handle_teleport(g, ch, nullptr, location);
    improve_skill(g, ch, skill_num, 2);
  }
}

ACMD(do_transform)
{

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf3[MAX_INPUT_LENGTH];

  if (!race_can_transform(char_get_race(ch))) {
    send_to_char(g, ch, "You do not have a transformation.\r\n");
    return;
  }

  bool npc = char_is_npc(ch);

  /*R: Hidden transformation stuff following this*/
  if (!npc && (getBasePL(ch)) < 50000) {
    send_to_char(g, ch, "@RYou are too weak to comprehend transforming!@n\r\n");
    return;
  }

  // Moved this further down. No need to parse the entry if we quit early. -Volund
  two_arguments(argument, arg, arg2);

  /* Called with no argument - display transformation information */
  if (!*arg) {
    race_display_forms(g, ch);
    if (trans_req(ch, 1) > 0) {
      race_display_trans_req(g, ch);
    }
    return;
  } /* End of No Argument */

  transform_bonus cur_form = race_current_form(g, ch);
  bool can_revert          = race_can_revert(char_get_race(ch));

  // If we are in kaioken or something weird like that, prevent transforming.
  if (!race_check_can_transform(g, ch)) {
    return;
  }

  // check for revert.
  if (!strcasecmp("revert", arg)) {
    if (!can_revert) {
      send_to_char(g, ch, "That would be unthinkable.\r\n");
      return;
    }
    if (!cur_form.flag) {
      send_to_char(g, ch, "You are not transformed.\r\n");
      return;
    }
    // We are all clear to revert.
    if ((char_get_charge(ch) > 0)) {
      do_charge(g, ch, writable{"release"}, 0, 0);
    }
    race_echo_revert(g, ch, race_flag_to_tier(cur_form.flag));
    char_plr_set(ch, cur_form.flag, false);

    if (*arg2) {
      do_transform(g, ch, arg2, 0, 0);
    }
    return;
  }

  // Search for available transformations. Error out if we can't find one.
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

  if (!npc && (getBasePL(ch)) < trans_req(ch, to_tier)) {
    send_to_char(g, ch, "You are not strong enough to handle that transformation!\r\n");
    return;
  }

  if (!npc && (getCurST(g, ch)) <= getMaxST(g, ch) * trans.drain) {
    send_to_char(g, ch, "You do not have enough stamina!");
    return;
  }

  if (!npc) {
    // Pay the price to unlock form if necessary.
    if (!race_check_trans_unlock(g, ch, to_tier)) {
      return;
    }
  }

  // revert current form's flag.
  if (cur_form.flag) {
    char_plr_set(ch, cur_form.flag, false);
  }

  // The stats are applied automatically in the new system just by having the flag.
  char_plr_set(ch, trans.flag, true);

  // Custom racial messages displayed.
  race_echo_transform(g, ch, to_tier);

  // No way is this a stealthy process...
  reveal_hiding(g, ch, 0);

  // Announce noisy transformations in the zone.
  zone_rnum zone = 0;
  if (race_has_noisy_transformations(char_get_race(ch))) {
    if ((zone = real_zone_by_thing(g, char_get_in_room(ch))) != NOWHERE) {
      send_to_zone(g, "An explosion of power ripples through the surrounding area!\r\n", zone);
    };
  }

  send_to_sense(g, 0, "You sense a nearby power grow unbelievably!", ch);
  snprintf(buf3, sizeof(buf3), "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(getCurPL(g, ch)));
  send_to_scouter(g, buf3, ch, 1, 0);
}

ACMD(do_aid)
{
  struct char_data *vict;
  struct obj_data *obj = nullptr, *aid_obj = nullptr, *aid_prod = nullptr, *next_obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int dc = 0, found = false;
  obj_vnum num = 47, num2 = 0;

  if (char_is_npc(ch))
    return;

  if (char_get_skill(ch, SKILL_SURVIVAL)) {
  }

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: aid heal (target)\r\n");
    send_to_char(g, ch, "        aid adrenex\r\n");
    send_to_char(g, ch, "        aid antitoxin\r\n");
    send_to_char(g, ch, "        aid salve\r\n");
    send_to_char(g, ch, "        aid formula-82\r\n");
    return;
  }

  if (!strcasecmp(arg, "adrenex")) {
    num  = 380;
    num2 = 381;
  } else if (!strcasecmp(arg, "antitoxin")) {
    num  = 380;
    num2 = 383;
  } else if (!strcasecmp(arg, "salve")) {
    num  = 380;
    num2 = 382;
  } else if (!strcasecmp(arg, "formula-82")) {
    num  = 380;
    num2 = 385;
  }

  for (obj = char_get_inventory(ch); obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
    if (found == false && obj_get_vnum(g, obj) == num && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found   = true;
      aid_obj = obj;
    }
  }

  if (found == false || aid_obj == nullptr) {
    if (num == 47) {
      send_to_char(g, ch, "You need bandages to be able to use first aid.\r\n");
    } else {
      send_to_char(g, ch, "You need a TCX-Medical Equipment Construction Kit.\r\n");
    }
    return;
  }
  if (num == 47) {
    if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Apply first aid to who?\r\n");
      return;
    }
    if (char_is_npc(vict)) {
      send_to_char(g, ch, "What ever for?\r\n");
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, "They are an android!\r\n");
      return;
    }

    if (!char_aff_flagged(vict, AFF_SPIRIT) && !char_plr_flagged(vict, PLR_BANDAGED)) {
      if (vict != ch) {
        send_to_char(g, ch, "You attempt to lend first aid to %s.\r\n", char_get_name(vict));
      }
      act(g, "$n attempts to bandage $N's wounds.", true, ch, 0, vict, TO_ROOM);
      dc = axion_dice(g, 0);
      if ((char_get_skill(ch, SKILL_FIRST_AID) + 1) > dc) {
        send_to_char(g, ch, "You bandage %s's wounds.\r\n", char_get_name(vict));
        int64_t roll = (getEffMaxPL(g, vict) / 100) * (char_get_aff_abils_wis(ch) / 4) + getEffMaxPL(g, vict) * 0.25;
        if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
          roll += roll * .1;
        }
        incCurHealth(g, vict, roll, true);

        send_to_char(g, vict, "Your wounds are bandaged by %s!\r\n", char_get_name(ch));
        act(g, "$n's wounds are stablized by $N!", true, vict, 0, ch, TO_NOTVICT);
        char_plr_set(vict, PLR_BANDAGED, true);
        extract_obj(g, aid_obj);
      } else {
        if (vict != ch) {
          send_to_char(g, ch, "You fail to bandage their wounds properly, wasting the set of bandages...\r\n");
          act(g, "$N fails to bandage $n's wounds properly, wasting an entire set of bandages...", true, vict, 0, ch, TO_NOTVICT);
          act(g, "$N fails to bandage your wounds properly, wasting an entire set of bandages...", true, vict, 0, ch, TO_CHAR);
        } else {
          act(g, "$N fails to bandage $s wounds properly, wasting an entire set of bandages...", true, vict, 0, ch, TO_NOTVICT);
          act(g, "You fail to bandage your wounds properly, wasting an entire set of bandages...", true, vict, 0, ch, TO_VICT);
        }
        extract_obj(g, aid_obj);
      }
      improve_skill(g, ch, SKILL_FIRST_AID, 1);
    } else if (char_plr_flagged(vict, PLR_BANDAGED)) {
      send_to_char(g, ch, "They are already bandaged!\r\n");
    } else if (char_aff_flagged(vict, AFF_SPIRIT)) {
      send_to_char(g, ch, "The dead don't need first aid.\r\n");
    } else {
      send_to_char(g, ch, "They apparently do not need bandaging.\r\n");
    }
  } else if (num2 == 381) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 65) {
      send_to_char(g, ch, "You need at least a skill level of 65 in First Aid.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(g, 15)) {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you begin to construct an Andrenex Adreneline Injector you screw up and end up breaking the water tight seal. The adreneline leaks out and is wasted.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, ch, 0, 0, TO_ROOM);
      extract_obj(g, aid_obj);
    } else {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully construct an Adrenex Adreneline Injector@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Adrenex Adreneline Injector!@n", true, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(g, num2, VIRTUAL);
      add_unique_id(g, aid_prod);
      obj_to_char(g, aid_prod, ch);
      extract_obj(g, aid_obj);
      improve_skill(g, ch, SKILL_FIRST_AID, 1);
    }
  } else if (num2 == 382) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 50) {
      send_to_char(g, ch, "You need at least a skill level of 50 in First Aid.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(g, 10)) {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you go to put the salve ingredients into the kit's salve compartment and set the temperature you accidentally set it too high. The salve is burned and ruined. Yes you managed to burn a burn salve.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, ch, 0, 0, TO_ROOM);
      extract_obj(g, aid_obj);
    } else {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully boil a burn salve to perfection and it is automatically placed in a jar.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a jar of burn salve!@n", true, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(g, num2, VIRTUAL);
      add_unique_id(g, aid_prod);
      obj_to_char(g, aid_prod, ch);
      extract_obj(g, aid_obj);
      improve_skill(g, ch, SKILL_FIRST_AID, 1);
    }
  } else if (num2 == 383) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 40) {
      send_to_char(g, ch, "You need at least a skill level of 40 in First Aid.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(g, 5)) {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete the Antitoxin Injector you notice that you didn't seal the syringe properly and it all leaks out.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, ch, 0, 0, TO_ROOM);
      extract_obj(g, aid_obj);
    } else {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble the Antitoxin Injector.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Antitoxin Injector!@n", true, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(g, num2, VIRTUAL);
      add_unique_id(g, aid_prod);
      obj_to_char(g, aid_prod, ch);
      extract_obj(g, aid_obj);
      improve_skill(g, ch, SKILL_FIRST_AID, 1);
    }
  } else if (num2 == 385) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 40) {
      send_to_char(g, ch, "You need at least a skill level of 40 in First Aid.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(g, 15)) {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete a vial of Formula 82 you notice that you read the mixture measurements wrong. You dispose of the vile vial immediately.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, ch, 0, 0, TO_ROOM);
      extract_obj(g, aid_obj);
    } else {
      act(g, "@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble a vial of Formula 82.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Vial of Formula 82!@n", true, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(g, num2, VIRTUAL);
      add_unique_id(g, aid_prod);
      obj_to_char(g, aid_prod, ch);
      extract_obj(g, aid_obj);
      improve_skill(g, ch, SKILL_FIRST_AID, 1);
    }
  }

  char_set_wait(ch, PULSE_3SEC);
}

ACMD(do_aura)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: aura light\r\n");
    return;
  }

  if (char_get_charge(ch)) {
    send_to_char(g, ch, "You can't focus enough on this while charging.");
    return;
  }

  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(g, ch, "You are busy powering up!\r\n");
    return;
  }

  if (!strcasecmp(arg, "light")) {
    if (char_get_skill(ch, SKILL_FOCUS) < 75 || char_get_skill(ch, SKILL_CONCENTRATION) < 75) {
      send_to_char(g, ch, "You need at least a skill level of 75 in Focus and Concentration to use this.\r\n");
      return;
    }
    if (char_plr_flagged(ch, PLR_AURALIGHT)) {
      send_to_char(g, ch, "Your aura fades as you stop shining light.\r\n");
      act(g, "$n's aura fades as they stop shining light on the area.", true, ch, 0, 0, TO_ROOM);
      char_plr_set(ch, PLR_AURALIGHT, false);
      rroom_set_light(g, char_get_in_room(ch), rroom_get_light(g, char_get_in_room(ch)) - 1);

    } else if ((getCurKI(g, ch)) > getMaxKI(g, ch) * 0.12) {
      if (char_get_in_room(ch) != NOWHERE) {
        rroom_set_light(g, char_get_in_room(ch), rroom_get_light(g, char_get_in_room(ch)) + 1);
      }
      reveal_hiding(g, ch, 0);
      decCurKIPercent(g, ch, .12, 0);
      send_to_char(g, ch, "A bright %s aura begins to burn around you as you provide light to the surrounding area!\r\n", aura_types[char_get_aura(ch)]);
      char bloom[MAX_INPUT_LENGTH];
      snprintf(bloom, sizeof(bloom), "@wA %s aura flashes up brightly around $n@w as they provide light to the area.@n", aura_types[char_get_aura(ch)]);
      act(g, bloom, true, ch, 0, 0, TO_ROOM);
      char_plr_set(ch, PLR_AURALIGHT, true);

    } else {
      send_to_char(g, ch, "You don't have enough KI to do that.\r\n");
      return;
    }
  }
}
