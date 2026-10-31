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
#include "dbat/dg_comm.h"
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
#include "other_internal.h"

ACMD(do_willpower)
{

  int fail = FALSE;

  if (char_is_npc(ch))
    return;

  if (char_get_majinize(ch) <= 0) {
    send_to_char(ch, "You are not majinized and have no need to reclaim full control of your own will.\r\n");
    return;
  } else {
    if (char_get_practice_points(ch) < 100 && char_get_level(ch) < 100) {
      send_to_char(ch, "You do not have enough PS to focus your attempt to break free.\r\n");
      fail = TRUE;
    }
    if (char_get_practice_points(ch) < 200 && char_get_level(ch) >= 100) {
      send_to_char(ch, "You do not have enough PS to focus your attempt to break free.\r\n");
      fail = TRUE;
    }
    if (char_get_exp(ch) < level_exp(ch, char_get_level(ch) + 1) && char_get_level(ch) < 100) {
      send_to_char(ch, "You need a full level's worth of experience stored up to try and break free.\r\n");
      fail = TRUE;
    }
    if (fail == TRUE) {
      return;
    } else {
      char_set_exp(ch, 0);
      char_set_practice_points(ch, char_get_practice_points(ch) - (100));
      if (char_get_level(ch) >= 100) {
        char_set_practice_points(ch, char_get_practice_points(ch) - (100));
      }
      if (rand_number(10, 100) - char_get_aff_abils_intel(ch) > 60) {
        reveal_hiding(ch, 0);
        act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments you give up, having failed to overcome the majinization!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments $n seems to give up and the commotion dies down.@n", TRUE, ch, 0, 0, TO_ROOM);
        return;
      } else {
        char_set_exp(ch, 0);
        char_set_practice_points(ch, char_get_practice_points(ch) - (100));
        if (char_get_level(ch) >= 100) {
          char_set_practice_points(ch, char_get_practice_points(ch) - (100));
        }
        reveal_hiding(ch, 0);
        act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments the ground splits beneath you and while letting out a piercing scream the M disappears from your forehead! You are free while still keeping the boost you had recieved from the majinization!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments the ground beneath $n splits and $e lets out a piercing scream. The M on $s forehead disappears!@n", TRUE, ch, 0, 0, TO_ROOM);
        char_set_majinize(ch, 3);
        return;
      }
    }
  }
}

ACMD(do_infuse)
{

  if (!know_skill(ch, SKILL_INFUSE)) {
    return;
  }

  if (char_aff_flagged(ch, AFF_INFUSE)) {
    act("You stop infusing ki into your attacks.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n stops infusing ki into $s attacks.", TRUE, ch, 0, 0, TO_ROOM);
    char_aff_set(ch, AFF_INFUSE, false);
    return;
  }

  if ((getCurKI(ch)) < getMaxKI(ch) / 100) {
    send_to_char(ch, "You don't have enough ki to infuse into your attacks!\r\n");
    return;
  }
  reveal_hiding(ch, 0);
  act("You start infusing ki into your attacks.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n starts infusing ki into $s attacks.", TRUE, ch, 0, 0, TO_ROOM);
  char_aff_set(ch, AFF_INFUSE, true);
  decCurKI(ch, getMaxKI(ch) / 100, 0);
}

ACMD(do_paralyze)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_PARALYZE)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Who are you wanting to paralyze?\r\n");
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That target isn't here.\r\n");
    return;
  }
  if (!can_kill(ch, vict, NULL, 0)) {
    return;
  }
  if (char_aff_flagged(vict, AFF_PARA)) {
    send_to_char(ch, "They are already partially paralyzed!\r\n");
    return;
  }

  if ((getCurKI(ch)) < getCurPL(vict) / 10 + (getMaxKI(ch) / 20)) {
    send_to_char(ch, "You realize you can't paralyze them. You don't have enough ki to restrain them!\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_PARALYZE), perc = axion_dice(0);

  if (char_get_speedi(ch) * 2 < char_get_speedi(vict)) {
    prob -= 10;
  }
  if (char_get_speedi(ch) + (char_get_speedi(ch) / 2) < char_get_speedi(vict)) {
    prob -= 5;
  }

  if (char_get_bonuses(vict, BONUS_INSOMNIAC)) {
    decCurKI(ch, getCurPL(vict) / 6 + (getMaxKI(ch) / 20), 0);
    act("@RYou focus ki and point both your arms at @r$N@R. However $N seems to shake off your paralysis attack!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@r$n @Rfocuses ki and points both $s arms at YOU! Your insomnia makes you immune to $s feeble paralysis attempt.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $N seems to shake off $s paralysis attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    return;
  } else if (prob < perc) {
    reveal_hiding(ch, 0);
    act("@RYou focus ki and point both your arms at @r$N@R. However $E manages to avoid your attempt to paralyze $M!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@r$n @Rfocuses ki and points both $s arms at YOU! You manage to avoid $s technique though...@n", TRUE, ch, 0, vict, TO_VICT);
    act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $E manages to avoid @r$n's@R attempted technique...@n", TRUE, ch, 0, vict, TO_NOTVICT);
    decCurKI(ch, getCurPL(vict) / 6 + (getMaxKI(ch) / 20), 0);
    improve_skill(ch, SKILL_PARALYZE, 0);
  } else {
    reveal_hiding(ch, 0);
    act("@RYou focus ki and point both your arms at @r$N@R. Your ki flows into $S body and partially paralyzes $M!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@r$n @Rfocuses ki and points both $s arms at YOU! You are caught in $s paralysis technique and now can barely move!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. @r$n's@R ki flows into @r$N@R body and partially paralyzes $M!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    int duration = char_get_aff_abils_intel(ch) / 15;
    assign_affect(vict, AFF_PARA, SKILL_PARALYZE, duration, 0, 0, 0, 0, 0, 0);
    decCurKI(ch, getCurPL(vict) / 6 + (getMaxKI(ch) / 20), 0);
    improve_skill(ch, SKILL_PARALYZE, 0);
  }
}

ACMD(do_taisha)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(ch, SKILL_TAISHA)) {
    return;
  }

  if (rroom_flagged(char_get_in_room(ch), ROOM_AURA)) {
    send_to_char(ch, "This area already has an aura of regeneration around it.\r\n");
    return;
  }

  if (rroom_get_gravity(char_get_in_room(ch)) > 0) {
    send_to_char(ch, "This area's gravity is too hostile to an aura.\r\n");
    return;
  }

  if ((getCurKI(ch)) < getMaxKI(ch) / 3) {
    send_to_char(ch, "You don't have enough ki.\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_TAISHA), perc = axion_dice(0);

  decCurKI(ch, getMaxKI(ch) / 3, 0);
  if (prob < perc) {
    reveal_hiding(ch, 0);
    act("@WYou hold up your hands while channeling ki. Your technique fails to produce an aura though....@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n@W holds up $s hands while channeling ki. $s technique fails to produce an aura though....", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_TAISHA, 1);
    return;
  } else {
    reveal_hiding(ch, 0);
    act("@WYou hold up your hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n holds up $s hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_TAISHA, 1);
    rroom_set_flag(char_get_in_room(ch), ROOM_AURA, true);
    return;
  }
}

ACMD(do_kura)
{
  if (!know_skill(ch, SKILL_KURA)) {
    return;
  }

  if ((getCurKI(ch)) >= getMaxKI(ch)) {
    send_to_char(ch, "Your ki is already maxed out!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: kuraiiro (1-100).\r\n");
    return;
  }

  int num      = atoi(arg);
  int skill    = char_get_skill(ch, SKILL_KURA);
  int64_t cost = 0, bonus = 0;

  if (num > skill) {
    send_to_char(ch, "The number can not be greater than your skill.\r\n");
    return;
  }
  if (num <= 0) {
    send_to_char(ch, "The number can not be less than 1.\r\n");
    return;
  }

  cost  = (getMaxKI(ch) / 100) * num;
  bonus = cost;

  if ((getCurST(ch)) < cost) {
    send_to_char(ch, "You do not have enough stamina for that high a number.\r\n");
    return;
  }

  if (skill <= axion_dice(0)) {
    decCurST(ch, cost, 0);
    reveal_hiding(ch, 0);
    act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies but you fail!", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@w crouches down and screams as $s eyes turn red and $e attempts to tap into dark energies but fails!", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_KURA, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else {
    decCurST(ch, cost, 0);
    incCurKI(ch, bonus, true);
    reveal_hiding(ch, 0);
    act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies and succeed as a rush of energy explodes around you!", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@w crouches down and screams as $s eyes turn red. Suddenly $e manages to tap into dark energies and a rush of energy explodes around $m!", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_KURA, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
}

ACMD(do_candy)
{

  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_is_majin(ch)) {
    send_to_char(ch, "You are not a majin, how can you do that?\r\n");
    return;
  }
  //if (FIGHTING(ch)) {
  //send_to_char(ch, "You are too busy fighting!\r\n");
  //return;
  //}

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Turn who into candy?\r\n");
    return;
  }
  if (!can_kill(ch, vict, NULL, 0)) {
    return;
  }

  int64_t ch_max   = getMaxPLTrans(ch);
  int64_t vict_max = getMaxPLTrans(vict);

  if (!char_is_npc(vict)) {
    send_to_char(ch, "You can't turn them into candy.\r\n");
    return;
  }
  if (vict_max > ch_max * 2) {
    send_to_char(ch, "They are too powerful.\r\n");
    return;
  }
  if (vict_max < ch_max * .25 && char_get_level(ch) < 100) {
    send_to_char(ch, "They are too weak.\r\n");
    return;
  }
  if (vict_max < ch_max * .09 && char_get_level(ch) == 100) {
    send_to_char(ch, "They are too weak.\r\n");
    return;
  }
  if ((getCurKI(ch)) < ch_max / 15) {
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }
  if (rand_number(1, 6) == 6) {
    decCurKI(ch, getMaxKI(ch) / 15, 0);
    reveal_hiding(ch, 0);
    act("@cYou aim your forelock at @R$N@c and fire a beam of energy but it is dodged!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy but the beam is dodged!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    if (!char_get_fighting(ch)) {
      set_fighting(ch, vict);
    }
    if (!char_get_fighting(vict)) {
      set_fighting(vict, ch);
    }
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else {
    decCurKI(ch, getMaxKI(ch) / 15, 0);
    reveal_hiding(ch, 0);
    act("@cYou aim your forelock at @R$N@c and fire a beam of energy that envelopes $S entire body and changes $M into candy!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy that envelopes $S entire body and changes $M into candy!@n ", TRUE, ch, 0, vict, TO_NOTVICT);
    if (vict_max >= ch_max * 1.5) {
      send_to_char(ch, "You grab the candy as it falls.\r\n");
      obj = read_object(95, VIRTUAL);
      obj_to_char(obj, ch);
    } else if (vict_max >= ch_max) {
      send_to_char(ch, "You grab the candy as it falls.\r\n");
      obj = read_object(94, VIRTUAL);
      obj_to_char(obj, ch);
    } else if (vict_max < ch_max && (((vict_max >= ch_max * .5) && char_get_level(ch) < 100) || ((vict_max >= ch_max * .1) && char_get_level(ch) >= 100))) {
      send_to_char(ch, "You grab the candy as it falls.\r\n");
      obj = read_object(93, VIRTUAL);
      obj_to_char(obj, ch);
    } else if (vict_max < ch_max && (((vict_max < ch_max * .5) && char_get_level(ch) < 100) || ((vict_max < ch_max * .1) && char_get_level(ch) >= 100))) {
      send_to_char(ch, "You grab the candy as it falls.\r\n");
      obj = read_object(53, VIRTUAL);
      obj_to_char(obj, ch);
    }
    char_mob_set(vict, MOB_HUSK, true);
    die(vict, ch);
  }
}

ACMD(do_future)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = NULL;
  one_argument(argument, arg);

  if (char_is_npc(ch) || !char_is_kanassan(ch)) {
    send_to_char(ch, "You are incapable of this ability.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Bestow advance future sight on who?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Bestow advance future sight on who?\r\n");
    return;
  }

  if (char_aff_flagged(vict, AFF_FUTURE)) {
    send_to_char(ch, "They already can see the future.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(ch, "You can't target them, there would be no point.\r\n");
    return;
  }

  if ((getCurKI(ch)) < getMaxKI(ch) / 40) {
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }

  if (char_get_practice_points(ch) < 100) {
    send_to_char(ch, "You do not have enough PS to activate or pass on this ability.\r\n");
    return;
  }

  if (vict != ch) {
    if (vict->real_abils.cha + 5 > 70 && char_get_bonuses(vict, BONUS_SLOW) > 0) {
      send_to_char(ch, "They can't handle having their speed increased beyond 70.\r\n");
      return;
    }
    if (vict->real_abils.intel + 2 > 70 && char_get_bonuses(vict, BONUS_DULL) > 0) {
      send_to_char(ch, "They can't handle having their intelligence increased beyond 70.\r\n");
      return;
    }
    decCurKI(ch, getMaxKI(ch) / 40, 0);
    char_set_practice_points(ch, char_get_practice_points(ch) - (100));
    reveal_hiding(ch, 0);
    act("@CYou focus your energy into your fingers before stabbing your claws into $N and bestowing the power of Future Sight upon $M. Shortly after $E passes out.@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n focuses $s energy into $s fingers before stabbing $s claws into YOUR neck and bestowing the power of Future Sight upon you! Soon after you pass out!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n focuses $s energy into $s fingers before stabbing $s claws into $N's neck and bestowing the power of Future Sight upon $M! Soon after $E passes out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_FUTURE, true);
    vict->real_abils.cha += 5;
    vict->real_abils.intel += 2;
    char_set_position(vict, POS_SLEEPING);
    save_char(vict);
  } else {
    if (ch->real_abils.cha + 5 > 70 && char_get_bonuses(ch, BONUS_SLOW) > 0) {
      send_to_char(ch, "You can't handle having your speed increased beyond 70.\r\n");
      return;
    }
    if (ch->real_abils.intel + 2 > 70 && char_get_bonuses(ch, BONUS_DULL) > 0) {
      send_to_char(ch, "You can't handle having your intelligence increased beyond 70.\r\n");
      return;
    }
    decCurKI(ch, getMaxKI(ch) / 40, 0);
    char_set_practice_points(ch, char_get_practice_points(ch) - (100));
    reveal_hiding(ch, 0);
    act("@CYou focus your energy into your mind and awaken your latent Future Sight powers!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n focuses $s energy while closing $s eyes for a moment.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n focuses $s energy while closing $s eyes for a moment.@n", TRUE, ch, 0, vict, TO_NOTVICT);
    char_aff_set(ch, AFF_FUTURE, true);
    ch->real_abils.cha += 5;
    ch->real_abils.intel += 2;
    char_set_position(vict, POS_SLEEPING);
    save_char(ch);
  }
}

ACMD(do_suppress)
{
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  if (char_is_android(ch)) {
    send_to_char(ch, "You are unable to suppress your powerlevel.\r\n");
    return;
  }
  if (char_get_bonuses(ch, BONUS_ARROGANT) > 0) {
    send_to_char(ch, "You are far too arrogant to hide your strength.\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(ch, "You are currently powering up, can't suppress.\r\n");
    return;
  }
  if (char_get_kaioken(ch)) {
    send_to_char(ch, "You are currently concentrating on kaioken!\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Suppress to what percent?\r\nSyntax: suppress (1 - 99 | release)\r\n");
    return;
  }

  if (!strcasecmp(arg, "release")) {
    if (char_get_suppression(ch)) {
      reveal_hiding(ch, 0);
      act("@GYou stop suppressing your current powerlevel!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@G$n smiles as a rush of power erupts around $s body briefly.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_suppression(ch, 0);
      return;
    } else {
      send_to_char(ch, "You are not suppressing!\r\n");
      return;
    }
  }

  int num = atoi(arg);

  if (num > 99 || num <= 0) {
    send_to_char(ch, "Out of suppression range.\r\nSyntax: suppress (1 - 99 | release)\r\n");
    return;
  }

  reveal_hiding(ch, 0);

  if (char_get_suppression(ch) != 0) {
    act("@GYou alter your suppression level!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@G$n seems to concentrate for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);

  } else {
    act("@GYou suppress your current powerlevel!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@G$n seems to concentrate for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);
  }
  char_set_suppression(ch, num);
  return;
}

ACMD(do_hass)
{

  int perc = 0, prob = 0;

  if (!check_skill(ch, SKILL_HASSHUKEN)) {
    return;
  }
  if ((getCurST(ch)) < getMaxST(ch) / 30) {
    send_to_char(ch, "You do not have enough stamina.\r\n");
    return;
  }

  perc = init_skill(ch, SKILL_HASSHUKEN);
  prob = axion_dice(0);

  if (perc < prob) {
    reveal_hiding(ch, 0);
    act("@WYou try to move your arms at incredible speeds but screw up and waste some of your stamina.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W tries to move $s arms at incredible speeds but screws up and wastes some of $s stamina.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurST(ch, getMaxST(ch) / 30, 0);
    improve_skill(ch, SKILL_HASSHUKEN, 0);
    return;
  } else {
    reveal_hiding(ch, 0);
    act("@WYou concentrate and start to move your arms at incredible speeds.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W concentrates and starts to move $s arms at incredible speeds.@n", TRUE, ch, 0, 0, TO_ROOM);
    int duration = perc / 15;
    assign_affect(ch, AFF_HASS, SKILL_HASSHUKEN, duration, 0, 0, 0, 0, 0, 0);
    decCurST(ch, getMaxST(ch) / 30, 0);
    improve_skill(ch, SKILL_HASSHUKEN, 0);
    return;
  }
}

ACMD(do_implant)
{

  struct obj_data *limb = NULL, *obj = NULL, *next_obj;
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int found = FALSE;

  two_arguments(argument, arg, arg2);

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Syntax: implant (rarm | larm | rleg | lleg) (target)\r\n");
    return;
  }

  if (!*arg2) {
    vict = ch;
  } else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That person isn't here.\r\n");
    return;
  }

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (found == FALSE && obj_get_vnum(obj) == 66 && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found = TRUE;
      limb  = obj;
    }
  }
  if (found == FALSE) {
    send_to_char(ch, "You do not have a cybernetic limb to implant.\r\n");
    return;
  }

  if (!(strcmp(arg, "rarm"))) {
    if (char_get_limb_condition(vict, (1) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(ch, "They already have a right arm!\r\n");
      }
      if (vict == ch) {
        send_to_char(ch, "You already have a right arm!\r\n");
      }
      return;
    } else {
      if (vict != ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, vict, TO_CHAR);
        act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, vict, TO_VICT);
        act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, vict, TO_NOTVICT);
      }
      if (vict == ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, 0, TO_CHAR);
        act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, 0, TO_ROOM);
      }
      char_plr_set(vict, PLR_CRARM, true);
      obj_from_char(limb);
      extract_obj(limb);
      return;
    }
  } else if (!(strcmp(arg, "larm"))) {
    if (char_get_limb_condition(vict, (2) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(ch, "They already have a left arm!\r\n");
      }
      if (vict == ch) {
        send_to_char(ch, "You already have a left arm!\r\n");
      }
      return;
    } else {
      if (vict && vict != ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, vict, TO_CHAR);
        act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, vict, TO_VICT);
        act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, vict, TO_NOTVICT);
      }
      if (vict == ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, 0, TO_CHAR);
        act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, 0, TO_ROOM);
      }
      char_plr_set(vict, PLR_CLARM, true);
      obj_from_char(limb);
      extract_obj(limb);
      return;
    }
  } else if (!(strcmp(arg, "rleg"))) {
    if (char_get_limb_condition(vict, (3) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(ch, "They already have a right leg!\r\n");
      }
      if (vict == ch) {
        send_to_char(ch, "You already have a right leg!\r\n");
      }
      return;
    } else {
      if (vict && vict != ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, vict, TO_CHAR);
        act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, vict, TO_VICT);
        act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, vict, TO_NOTVICT);
      }
      if (vict == ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, 0, TO_CHAR);
        act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, 0, TO_ROOM);
      }
      char_plr_set(vict, PLR_CRLEG, true);
      obj_from_char(limb);
      extract_obj(limb);
      return;
    }
  } else if (!(strcmp(arg, "lleg"))) {
    if (char_get_limb_condition(vict, (4) - 1) >= 1) {
      if (vict != ch) {
        send_to_char(ch, "They already have a left leg!\r\n");
      }
      if (vict == ch) {
        send_to_char(ch, "You already have a left leg!\r\n");
      }
      return;
    } else {
      if (vict && vict != ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, vict, TO_CHAR);
        act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, vict, TO_VICT);
        act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, vict, TO_NOTVICT);
      }
      if (!vict || vict == ch) {
        reveal_hiding(ch, 0);
        act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, 0, TO_CHAR);
        act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, 0, TO_ROOM);
      }
      char_plr_set(vict, PLR_CLLEG, true);
      obj_from_char(limb);
      extract_obj(limb);
      return;
    }
  } else {
    send_to_char(ch, "Syntax: implant (rarm | larm | rleg | rleg)\r\n");
    return;
  }
}

/* do_fury for halfbreeds to release their raaaage, rawrg! */

ACMD(do_fury)
{

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_is_halfbreed(ch) || char_is_npc(ch)) {
    send_to_char(ch, "You are furious, but you'll get over it.\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_FURY)) {
    send_to_char(ch, "You are already furious, your next attack will devestate, hurry use it!\r\n");
    return;
  }

  if (char_get_fury(ch) < 100) {
    send_to_char(ch, "You do not have enough anger to release your fury upon your foes!\r\n");
    return;
  }

  if (!*arg) {
    if (getCurPL(ch) < getEffMaxPL(ch)) {
      if ((getCurLF(ch)) >= (getMaxLF(ch)) * 0.2) {
        restoreHealth(ch, false);
        decCurLFPercent(ch, .2, 0);
      } else {
        incCurHealth(ch, (getCurLF(ch)), true);
        decCurLFPercent(ch, 2, -1);
      }
    }
    char_set_fury(ch, 0);
  } else if (!strcasecmp(arg, "attack")) {
    char_set_fury(ch, 50);
  } else {
    send_to_char(ch, "Syntax: fury (attack) <--- this will not use up your LF to restore PL.\n        fury <--- fury by itself will do both LF to PL restore and attack boost.\r\n");
    return;
  }

  reveal_hiding(ch, 0);
  act("You release your fury! Your very next attack is guaranteed to rip your foes a new one!", TRUE, ch, 0, 0, TO_CHAR);
  act("$n screams furiously as a look of anger appears on $s face!", TRUE, ch, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_FURY, true);
}

ACMD(do_majinize)
{

  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_is_majin(ch)) {
    send_to_char(ch, "You are not a majin and can not majinize anyone.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Who do you want to majinize?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That target isn't here.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(ch, "Why would you waste your time majinizing them?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "You can't majinize yourself.\r\n");
    return;
  }
  if (char_plr_flagged(vict, PLR_PR)) {
    send_to_char(ch, "You can't majinize them their potential has been released!\r\n");
    return;
  }
  int alignmentTotal = char_get_alignment(ch) - char_get_alignment(vict);
  if (char_get_majinize(vict) > 0 && char_get_majinize(vict) != char_get_id(ch)) {
    send_to_char(ch, "They are already majinized before by someone else.\r\n");
    return;
  } else if ((vict->master != ch)) {
    send_to_char(ch, "They must be following you in order for you to majinize them.\r\n");
    return;
  }

  else if (!((alignmentTotal >= -1500) && (alignmentTotal <= 1500))) {
    send_to_char(ch, "Their alignment is so opposed to your's that they resist your attempts to enslave them!\r\n");
    return;
  } else if (getEffMaxPL(vict) > getEffMaxPL(ch) * 4) {
    send_to_char(ch, "Their powerlevel is so much higher than yours they resist your attempts to enslave them!\r\n");
    return;
  }
  /* Rillao: transloc, add new transes here */
  else if (char_get_majinize(vict) > 0 && char_get_majinize(vict) == char_get_id(ch)) {
    reveal_hiding(ch, 0);
    act("You remove $N's majinization, freeing them from your influence, but also weakening them.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n removes your majinization, freeing you from their influence, and weakening you!", TRUE, ch, 0, vict, TO_VICT);
    act("$n waves a hand at $N, and instantly the glowing M on $S forehead disappears!", TRUE, ch, 0, vict, TO_NOTVICT);
    char_set_majinize(vict, 0);
    char_set_boosts(ch, char_get_boosts(ch) + (1));

    if (char_get_majinizer(vict) == 0) {
      char_set_majinizer(vict, ((getBasePL(vict)) * .4));
    }
    loseBasePL(vict, char_get_majinizer(vict), false);
    return;
  } else if (char_get_boosts(ch) == 0) {
    send_to_char(ch, "You are incapable of majinizing%s.\r\n", char_get_level(ch) < 100 ? " right now" : " anymore");
    if (char_get_level(ch) < 25) {
      send_to_char(ch, "Your next available majinize will be at level 25\r\n");
    } else if (char_get_level(ch) < 50) {
      send_to_char(ch, "Your next available majinize will be at level 50\r\n");
    } else if (char_get_level(ch) < 75) {
      send_to_char(ch, "Your next available majinize will be at level 75\r\n");
    } else if (char_get_level(ch) < 100) {
      send_to_char(ch, "Your next available majinize will be at level 100\r\n");
    }
    return;
  } else {
    reveal_hiding(ch, 0);
    act("You focus your power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n focuses power into you, influencing your mind and increasing your strength! After the struggle in your mind ends a glowing purple M forms on your forehead.", TRUE, ch, 0, vict, TO_VICT);
    act("$n focuses power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", TRUE, ch, 0, vict, TO_NOTVICT);
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
  struct affected_type af;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_SPIT)) {
    return;
  }
  if (char_get_fighting(ch)) {
    send_to_char(ch, "You can't manage to spit in this fight!\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Yes but who do you want to petrify?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That target isn't here.\r\n");
    return;
  }
  if (!can_kill(ch, vict, NULL, 0)) {
    return;
  }
  if (char_aff_flagged(vict, AFF_PARALYZE)) {
    act("$N has already been turned to stone.", TRUE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (char_get_fighting(vict)) {
    send_to_char(vict, "You can't manage to spit on them, they are moving around too much!\r\n");
    return;
  }

  cost = ((getMaxKI(ch) / (char_get_skill(ch, SKILL_SPIT) / 4)) + getMaxKI(ch) / 100);

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to petrifiy with your spit!\r\n");
    return;
  }

  if (char_get_skill(ch, SKILL_SPIT) < axion_dice(0)) {
    decCurKI(ch, cost, 0);
    reveal_hiding(ch, 0);
    act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, but it falls short of hitting @c$N@W!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W seems to focus ki before hawking a loogie at you! Fortunatly the loogie falls short.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! Fortunatly for @c$N@W the loogie falls short.@n", TRUE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_SPIT, 1);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else if (char_aff_flagged(vict, AFF_ZANZOKEN) && (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
    decCurKI(ch, cost, 0);
    reveal_hiding(ch, 0);
    act("@C$N@c disappears, avoiding your spit before reappearing!@n", FALSE, ch, 0, vict, TO_CHAR);
    act("@cYou disappear, avoiding @C$n's@c @rstone spit@c before reappearing!@n", FALSE, ch, 0, vict, TO_VICT);
    act("@C$N@c disappears, avoiding @C$n's@c @rstone spit@c before reappearing!@n", FALSE, ch, 0, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    char_set_wait(ch, PULSE_2SEC);
    improve_skill(ch, SKILL_SPIT, 1);
    return;
  } else {
    af.type      = SPELL_PARALYZE;
    af.duration  = rand_number(1, 2);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_PARALYZE;
    affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);

    decCurKI(ch, cost, 0);
    reveal_hiding(ch, 0);
    act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, and it smacks into @c$N@W turning $M into stone!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W seems to focus ki before hawking a loogie at you! It manages to hit and you instantly turn to stone!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! It manages to hit and $E instantly turns to stone!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_SPIT, 1);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
}

ACMD(do_absorb)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!check_skill(ch, SKILL_ABSORB)) {
    return;
  }

  if (char_is_android(ch)) {
    if (!limb_ok(ch, 0)) {
      return;
    }
  }
  if (!char_is_npc(ch)) {
    if (char_is_bio(ch) && !char_plr_flagged(ch, PLR_TAIL)) {
      send_to_char(ch, "You have no tail!\r\n");
      return;
    }
  }

  if (!char_is_android(ch) && !char_is_bio(ch)) {
    send_to_char(ch, "You shouldn't have this skill, you are incapable of absorbing.\r\n");
    send_to_imm("ERROR: Absorb skill on %s when they are not a bio or android.", char_get_name(ch));
    return;
  }

  if (char_get_fighting(ch) && !char_is_android(ch)) {
    send_to_char(ch, "You are too busy fighting!\r\n");
    return;
  }

  if (char_get_grappled(ch)) {
    send_to_char(ch, "You are currently being grappled with! Try 'escape'!\r\n");
    return;
  }
  if (char_get_grappling(ch)) {
    send_to_char(ch, "You are currently grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(ch)) {
    act("@WYou stop absorbing from @c$N@W!@n", TRUE, ch, 0, char_get_absorbing(ch), TO_CHAR);
    act("$n stops absorbing from you!", TRUE, ch, 0, char_get_absorbing(ch), TO_VICT);
    act("$n stops absorbing from $N!", TRUE, ch, 0, char_get_absorbing(ch), TO_NOTVICT);
    if (char_is_npc(char_get_absorbing(ch)) && !char_get_fighting(char_get_absorbing(ch))) {
      set_fighting(char_get_absorbing(ch), ch);
    }
    char_set_absorbby(char_get_absorbing(ch), NULL);
    char_set_absorbing(ch, NULL);
  }

  if (!*arg && char_is_android(ch)) {
    send_to_char(ch, "Who do you want to absorb?\r\n");
    return;
  }

  if (char_is_android(ch)) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Absorb %s?\r\n", char_is_android(ch) ? "from who" : "who");
      return;
    }
  }
  if (char_is_bio(ch)) {
    if (!*arg) {
      send_to_char(ch, "Syntax: absorb (swallow | extract) (target)\r\n");
      return;
    } else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Syntax: absorb (swallow | extract) (target)\r\n");
      return;
    }
  }
  if (char_aff_flagged(vict, AFF_SANCTUARY)) {
    send_to_char(ch, "You can't absorb them, they have a barrier!\r\n");
    return;
  }

  if (char_is_android(ch)) {
    if (!char_is_npc(ch)) {
      if (!char_plr_flagged(ch, PLR_ABSORB)) {
        send_to_char(ch, "You are not an absorbtion model.\r\n");
        return;
      }
    }
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (char_get_absorbby(vict)) {
      send_to_char(ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (getEffMaxPL(vict) > getEffMaxPL(ch) * 2) {
      send_to_char(ch, "They are too strong for you to absorb from.\r\n");
      return;
    }
    if (getEffMaxPL(vict) * 20 < getEffMaxPL(ch)) {
      send_to_char(ch, "They are too weak for you to bother absorbing from.\r\n");
      return;
    }
    if ((getCurST(vict)) < (getMaxST(vict) / 20) && (getCurKI(vict)) < (getMaxKI(vict) / 20)) {
      send_to_char(ch, "They have nothing to absorb right now, they are drained...\r\n");
      return;
    }
    reveal_hiding(ch, 0);
    if (init_skill(ch, SKILL_ABSORB) < axion_dice(0)) {
      act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      improve_skill(ch, SKILL_ABSORB, 1);
      if (char_is_npc(vict) && char_is_humanoid(vict) && rand_number(1, 3) == 3) {
        if (char_get_fighting(ch) == NULL) {
          set_fighting(ch, vict);
        }
        if (char_get_fighting(vict) == NULL) {
          set_fighting(vict, ch);
        }
      }
      char_set_wait(ch, PULSE_3SEC);
      return;
    } else {
      act("@WYou rush at @c$N@W and try to absorb from them, and manage to grab on!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W rushes at you and tries to grab you, and manages to grab on!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w rushes at @c$N@W and tries to grab $M, and manages to grab on!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      improve_skill(ch, SKILL_ABSORB, 1);
      char_set_absorbing(ch, vict);
      char_set_absorbby(vict, ch);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
  } // End of android absorb

  else if (char_is_bio(ch) && !(strcmp(arg, "swallow"))) {
    if (char_get_absorbby(vict)) {
      send_to_char(ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (char_get_absorbs(ch) < 1) {
      send_to_char(ch, "You already have already absorbed 3 people.\r\n");
      return;
    }
    if (getEffMaxPL(vict) >= (getBasePL(ch)) * 3) {
      send_to_char(ch, "You are too weak to absorb them into your cellular structure!\r\n");
      return;
    }
    reveal_hiding(ch, 0);
    if (char_get_skill(ch, SKILL_ABSORB) < axion_dice(0)) {
      act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      improve_skill(ch, SKILL_ABSORB, 1);
      if (!char_get_fighting(ch)) {
        set_fighting(ch, vict);
      }
      if (!char_get_fighting(vict)) {
        set_fighting(vict, ch);
      }
      char_set_wait(ch, PULSE_3SEC);
    } else {
      act("@WYou rush at @c$N@W and your tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing $m!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W rushes at you and $s tail engulfs you! $e quickly sucks your squirming body into $s tail, absorbing you!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w rushes at @c$N@W and $s tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_absorbs(ch, char_get_absorbs(ch) - (1));

      int64_t stam = (getBaseST(vict)) / 5;
      int64_t ki   = (getBaseKI(vict)) / 5;
      int64_t pl   = (getBasePL(vict)) / 5;

      gainBasePL(ch, pl, true);
      gainBaseST(ch, stam, true);
      gainBaseKI(ch, ki, true);

      if (!char_is_npc(vict) && !char_is_npc(ch)) {
        send_to_imm("[PK] %s killed %s at room [%d]\r\n", char_get_name(ch), char_get_name(vict), rroom_get_vnum(char_get_in_room(vict)));
        char_plr_set(vict, PLR_ABSORBED, true);
      }

      send_to_char(ch, "@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
      improve_skill(ch, SKILL_ABSORB, 1);
      die(vict, NULL);
    }
  } // End of bio absorb
  else if (char_is_bio(ch) && !(strcmp(arg, "extract"))) {
    int failthresh = rand_number(1, 125);
    if (char_get_level(vict) > 99) {
      failthresh += (char_get_level(vict) - 95) * 2;
    }
    if (char_get_absorbby(vict)) {
      send_to_char(ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (getEffMaxPL(vict) >= getEffMaxPL(ch)) {
      send_to_char(ch, "You are too weak to absorb them into your cellular structure!\r\n");
      return;
    }
    if (getEffMaxPL(vict) < getEffMaxPL(ch) / 5) {
      send_to_char(ch, "They would be worthless to you at your strength!\r\n");
      return;
    }
    if (!char_is_npc(vict)) {
      send_to_char(ch, "You can't absorb their bio extract, you need to swallow them with your tail!\r\n");
      return;
    }
    if (is_soft_cap(ch, 0, 1.0)) {
      send_to_char(ch, "You can not handle any more bio extract at your current level.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_ABSORB) < failthresh) {
      act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      improve_skill(ch, SKILL_ABSORB, 0);
      if (!char_get_fighting(ch)) {
        set_fighting(ch, vict);
      }
      if (!char_get_fighting(vict)) {
        set_fighting(vict, ch);
      }
      char_set_wait(ch, PULSE_4SEC);
    }
    /* Rillao: transloc, add new transes here */
    else {
      act("@WYou rush at @c$N@W and stab them with your tail! You quickly suck out all the bio extract you need and leave the empty husk behind!", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@w rushes at @c$N@W and stabs $M with $s tail! $e quickly sucks out all the bio extract and leaves the empty husk of @c$N@W behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      int64_t stam = (getBaseST(vict)) / 2000;
      int64_t ki   = (getBaseKI(vict)) / 2000;
      int64_t pl   = (getBasePL(vict)) / 2000;
      stam += rand_number(char_get_level(ch), char_get_level(ch) * 2);
      pl += rand_number(char_get_level(ch), char_get_level(ch) * 2);
      ki += rand_number(char_get_level(ch), char_get_level(ch) * 2);
      stam = MIN(stam, 1500000L);
      ki   = MIN(ki, 1500000L);
      pl   = MIN(pl, 1500000L);

      gainBasePL(ch, pl, true);
      gainBaseST(ch, stam, true);
      gainBaseKI(ch, ki, true);
      incCurLFPercent(ch, .05, true);
      send_to_char(ch, "@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
      improve_skill(ch, SKILL_ABSORB, 0);
      char_set_wait(ch, PULSE_4SEC);
      char_mob_set(vict, MOB_HUSK, true);
      die(vict, ch);
    }
  }

  else {
    if (!char_is_bio(ch) && !char_is_android(ch)) {
      send_to_char(ch, "You have the absorb skill but are incapable of absorbing. This error has been reported.\r\n");
      send_to_imm("ERROR: Absorb attempted by %s even though they are not bio or android.", char_get_name(ch));
    } else {
      send_to_char(ch, "Syntax: absorb (extract | swallow) (target)\r\n");
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

  skill = init_skill(ch, SKILL_REGENERATE);

  if (skill < 1) {
    send_to_char(ch, "You are incapable of regenerating.\r\n");
    return;
  }
  if (char_get_suppression(ch) > 0) {
    skill = char_get_suppression(ch);
  }

  if (!*arg) {
    send_to_char(ch, "Regenerate how much PL?\r\nMax percent you can regen: %d\r\nSyntax: regenerate (1 - 100)\r\n", skill);
    return;
  }

  if (getCurPL(ch) >= getEffMaxPL(ch)) {
    send_to_char(ch, "You do not need to regenerate, you are at full health.\r\n");
    return;
  }

  if (char_get_suppression(ch) > 0 && getCurPL(ch) >= ((getEffMaxPL(ch) / 100) * char_get_suppression(ch))) {
    send_to_char(ch, "You do not need to regenerate, you are at full health.\r\n");
    return;
  }

  int num = atoi(arg);

  if (num <= 0) {
    send_to_char(ch, "What is the point of that?\r\nSyntax: regenerate (1 - 100)\r\n");
    return;
  }

  if (num > 100) {
    send_to_char(ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
    return;
  }

  if (num > skill) {
    send_to_char(ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
    return;
  }

  if (char_get_suppression(ch) > 0 && num > char_get_suppression(ch)) {
    send_to_char(ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
    return;
  }

  amt = (getEffMaxPL(ch) * 0.01) * num;
  if (amt > 1)
    amt /= 2;

  if (char_is_bio(ch)) {
    amt = amt * 0.9;
  }

  int64_t life = ((getCurLF(ch)) - amt * 0.8), energy = ((getCurKI(ch)) - amt * 0.2);

  if ((life <= 0 || energy <= 0) && !char_is_npc(ch)) {
    send_to_char(ch, "Your life force or ki are too low to regenerate that much.\r\n");
    send_to_char(ch, "@YLF Needed@D: @C%s@w, @YKi Needed@D: @C%s@w.@n\r\n", add_commas(amt * 0.8), add_commas(amt * 0.2));
    return;
  } else if (char_is_npc(ch) && energy <= 0) {
    return;
  }

  incCurHealth(ch, amt * 2, true);

  if (!char_is_npc(ch))
    decCurLF(ch, amt * .8, 0);

  decCurKI(ch, amt * .2, 0);

  reveal_hiding(ch, 0);

  if (getCurPL(ch) >= getEffMaxPL(ch)) {
    act("You concentrate your ki and regenerate your body completely.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n concentrates and regenerates $s body completely.", TRUE, ch, 0, 0, TO_ROOM);
  } else if (amt < getEffMaxPL(ch) / 10) {
    act("You concentrate your ki and regenerate your body a little.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n concentrates and regenerates $s body a little.", TRUE, ch, 0, 0, TO_ROOM);
  } else if (amt < getEffMaxPL(ch) / 5) {
    act("You concentrate your ki and regenerate your body some.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n concentrates and regenerates $s body some.", TRUE, ch, 0, 0, TO_ROOM);
  } else if (amt < getEffMaxPL(ch) / 2) {
    act("You concentrate your ki and regenerate your body a great deal.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n concentrates and regenerates $s body a great deal.", TRUE, ch, 0, 0, TO_ROOM);
  } else if (getCurPL(ch) < getEffMaxPL(ch)) {
    act("You concentrate your ki and regenerate you nearly completely.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n concentrates and regenerates $s body nearly completely.", TRUE, ch, 0, 0, TO_ROOM);
  }
  improve_skill(ch, SKILL_REGENERATE, 0);
  if (char_aff_flagged(ch, AFF_BURNED)) {
    send_to_char(ch, "Your burns are healed now.\r\n");
    act("$n@w's burns are now healed.@n", TRUE, ch, 0, 0, TO_ROOM);
    null_affect(ch, AFF_BURNED);
  }

  if (!char_is_npc(ch)) {
    if (char_get_limb_condition(ch, (1) - 1) <= 0) {
      act("You regrow your right arm!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regrows $s right arm!", TRUE, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (1) - 1, 100);
    } else if (char_get_limb_condition(ch, (1) - 1) >= 0 && char_get_limb_condition(ch, (1) - 1) < 50) {
      act("Your broken right arm mends itself!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regenerates $s broken right arm!", TRUE, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (1) - 1, 100);
    }
    if (char_get_limb_condition(ch, (2) - 1) <= 0) {
      char_set_limb_condition(ch, (2) - 1, 100);
      act("You regrow your left arm!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regrows $s left arm!", TRUE, ch, 0, 0, TO_ROOM);
    } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50) {
      act("Your broken left arm mends itself!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regenerates $s broken left arm!", TRUE, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (2) - 1, 100);
    }
    if (char_get_limb_condition(ch, (4) - 1) <= 0) {
      char_set_limb_condition(ch, (4) - 1, 100);
      act("You regrow your left leg!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regrows $s left leg!", TRUE, ch, 0, 0, TO_ROOM);
    } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50) {
      act("Your broken left leg mends itself!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regenerates $s broken left leg!", TRUE, ch, 0, 0, TO_ROOM);
      char_set_limb_condition(ch, (4) - 1, 100);
    }
    if (char_get_limb_condition(ch, (3) - 1) <= 0) {
      char_set_limb_condition(ch, (3) - 1, 100);
      act("You regrow your right leg!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regrows $s right leg!", TRUE, ch, 0, 0, TO_ROOM);
    } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50) {
      char_set_limb_condition(ch, (3) - 1, 100);
      act("Your broken right leg mends itself!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regenerates $s broken right leg!", TRUE, ch, 0, 0, TO_ROOM);
    }
    if (!char_plr_flagged(ch, PLR_TAIL) && char_is_bio(ch)) {
      char_plr_set(ch, PLR_TAIL, true);
      act("You regrow your tail!", TRUE, ch, 0, 0, TO_CHAR);
      act("$n regrows $s tail!", TRUE, ch, 0, 0, TO_ROOM);
    }
    improve_skill(ch, SKILL_REGENERATE, 0);
  }
}

static int64_t kaioken_levels[21] = {
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
  int roll = axion_dice(0), x = 0;
  one_argument(argument, arg);

  if (!check_skill(ch, SKILL_KAIOKEN)) {
    return;
  }
  if (char_get_alignment(ch) <= -50) {
    send_to_char(ch, "Your heart is too corrupt to use that technique!\r\n");
    return;
  }
  if (!char_is_npc(ch)) {
    if (char_plr_flagged(ch, PLR_HEALT)) {
      send_to_char(ch, "You are inside a healing tank!\r\n");
      return;
    }
  }

  if (!*arg) {
    send_to_char(ch, "What level of kaioken do you want to try and achieve?\r\n"
                     "Syntax: kaioken 1-20\r\n");
    return;
  }

  x = atoi(arg);

  if (x < 0 || x > 20) {
    send_to_char(ch, "That level of kaioken dosn't exist...\r\n"
                     "Syntax: kaioken 0-20\r\n");
    return;
  }

  if (x == 0) {
    if (char_get_kaioken(ch) > 0) {
      remove_kaioken(ch, 1);
      return;
    } else {
      send_to_char(ch, "You are not in kaioken!\r\n");
      return;
    }
  }

  if (x == char_get_kaioken(ch)) {
    send_to_char(ch, "You are already at that kaioken level! To release, try kaioken 0\r\n");
    return;
  }

  if (!char_is_npc(ch)) {
    if ((char_is_transformed(ch) || (char_is_hoshijin(ch) && char_get_starphase(ch) > 0)) && x > 5) {
      send_to_char(ch, "You can not manage a kaioken level higher than 5 when transformed.\r\n");
      return;
    }
  }

  int64_t cost_unit = getMaxKI(ch) / 50;
  int64_t cost_diff = cost_unit * char_get_kaioken(ch);
  int64_t cost      = (cost_unit * x) - cost_diff;

  // it costs nothing to reduce your kaioken level.
  if (x < char_get_kaioken(ch)) {
    cost = 0;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to focus into your body for that level.\r\n");
    return;
  }

  int xnum = (x * 5) + 1;
  roll     = rand_number(1, xnum);
  reveal_hiding(ch, 0);

  decCurKI(ch, cost, 0);
  improve_skill(ch, SKILL_KAIOKEN, 1);

  if (init_skill(ch, SKILL_KAIOKEN) < roll) {
    send_to_char(ch, "You try to focus your ki into your body but mess up somehow.\r\n");
    act("$n tries to use kaioken but messes up somehow.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }

  if (getMaxHealth(ch) < kaioken_levels[x]) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  apply_kaioken(ch, x, true);

  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_zanzoken)
{

  int prob = 0, perc = 0;
  int64_t cost = 0;

  if (!know_skill(ch, SKILL_ZANZOKEN) && !char_is_npc(ch)) {
    return;
  }

  if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
    char_aff_set(ch, AFF_ZANZOKEN, false);
    send_to_char(ch, "You release the ki you had prepared for a zanzoken.\r\n");
    return;
  }

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(ch, "You are busy in a grapple!\r\n");
    return;
  }

  if (!char_is_npc(ch)) {
    prob = char_get_skill(ch, SKILL_ZANZOKEN);
  } else {
    prob = rand_number(80, 90);
  }
  perc = axion_dice(0);
  cost = getMaxKI(ch) / 50;

  if (prob > 75) {
    cost *= 2;
  } else if (prob > 50) {
    cost *= 4;
  } else if (prob >= 25) {
    cost *= 8;
  } else if (prob < 25) {
    cost *= 10;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }

  if (prob < perc) {
    send_to_char(ch, "You focus your ki in preparation of a zanzoken but mess up and waste your ki!\r\n");
    improve_skill(ch, SKILL_ZANZOKEN, 2);
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  act("@wYou focus your ki, preparing to move at super speeds if necessary.@n", TRUE, ch, 0, 0, TO_CHAR);
  decCurKI(ch, cost, 0);
  char_aff_set(ch, AFF_ZANZOKEN, true);
  improve_skill(ch, SKILL_ZANZOKEN, 2);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_eyec)
{

  if (char_is_npc(ch))
    return;

  if (char_plr_flagged(ch, PLR_EYEC)) {
    char_plr_set(ch, PLR_EYEC, false);
    act("@wYou open your eyes.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@w opens $s eyes.@n", TRUE, ch, 0, 0, TO_ROOM);
  }

  else if (!char_plr_flagged(ch, PLR_EYEC)) {
    char_plr_set(ch, PLR_EYEC, true);
    act("@wYou close your eyes.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@w closes $s eyes.@n", TRUE, ch, 0, 0, TO_ROOM);
  }

  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_solar)
{
  struct char_data *vict = NULL, *next_v = NULL;

  int prob = 0, perc = 0, cost = 0, bonus = 0;

  if (!know_skill(ch, SKILL_SOLARF)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  prob = char_get_skill(ch, SKILL_SOLARF);
  perc = rand_number(0, 101);

  if (prob >= 75) {
    cost = getMaxKI(ch) / 50;
  } else if (prob >= 50) {
    cost = getMaxKI(ch) / 25;
  } else if (prob >= 25) {
    cost = getMaxKI(ch) / 20;
  } else if (prob < 25) {
    cost = getMaxKI(ch) / 15;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }

  bonus = char_get_aff_abils_intel(ch) / 3;
  prob += bonus;

  if (prob < perc) {
    act("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' but nothing happens!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' but nothing happens!@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(ch, SKILL_SOLARF, 0);
    return;
  }

  act("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' as a blinding light fills the area!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' as a blinding light fills the area!@n", TRUE, ch, 0, 0, TO_ROOM);

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;

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
      assign_affect(vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
      act("@W$N@W is @YBLINDED@W!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@RYou are @YBLINDED@R!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@W$N@W is @YBLINDED@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    }
  }
  improve_skill(ch, SKILL_SOLARF, 0);
  decCurKI(ch, cost, 0);
  char_set_wait(ch, PULSE_3SEC);
}

ACMD(do_heal)
{
  int64_t cost = 0, prob = 0, perc = 0, heal = 0, bonus = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!check_skill(ch, SKILL_HEAL)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "You want to heal WHO?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "You do not see the target here.\r\n");
    return;
  }

  prob = init_skill(ch, SKILL_HEAL);
  perc = rand_number(0, 110);

  if (prob >= 100) {
    cost = getMaxKI(ch) / 20;
    heal = getEffMaxPL(vict) / 5;
  } else if (prob >= 90) {
    cost = getMaxKI(ch) / 16;
    heal = getEffMaxPL(vict) / 10;
  } else if (prob >= 75) {
    cost = getMaxKI(ch) / 14;
    heal = getEffMaxPL(vict) / 12;
  } else if (prob >= 50) {
    cost = getMaxKI(ch) / 12;
    heal = getEffMaxPL(vict) / 15;
  } else if (prob >= 25) {
    cost = getMaxKI(ch) / 10;
    heal = getEffMaxPL(vict) / 20;
  } else if (prob < 25) {
    cost = getMaxKI(ch) / 6;
    heal = getEffMaxPL(vict) / 20;
  }

  if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
    heal += heal * .1;
  }

  if (heal < getEffMaxPL(vict)) {
    heal += (heal / 100) * (char_get_aff_abils_wis(ch) / 4);
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki.\r\n");
    return;
  }

  if (getCurPL(vict) >= getEffMaxPL(vict)) {
    if (vict != ch) {
      send_to_char(ch, "They are already at full health.\r\n");
    } else {
      send_to_char(ch, "You are already at full health.\r\n");
    }
    return;
  }

  if (char_get_suppression(vict) > 0 && getCurPL(vict) >= ((getEffMaxPL(vict) / 100) * char_get_suppression(vict))) {
    send_to_char(ch, "They are already at full health.\r\n");
    return;
  }

  bonus = (char_get_aff_abils_intel(ch) / 2) + (char_get_aff_abils_wis(ch) / 3);
  prob += bonus;

  if (prob < perc) {
    if (vict != ch) {
      act("@WYou place your hands near @c$N@W, but fail to concentrate enough to heal them!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W places $s hands near you, but nothing happens!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@W places $s hands near @c$N@W, but nothing happens.", TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_HEAL, 0);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    if (vict == ch) {
      act("@WYou place your hands on your body, but fail to concentrate to heal yourself!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W places $s hands on $s body, but nothing happens.", TRUE, ch, 0, vict, TO_NOTVICT);
      decCurKI(ch, cost, 0);
      improve_skill(ch, SKILL_HEAL, 0);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
  }

  if (vict != ch) {
    if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
      heal += heal * .25;
    }
    act("@WYou place your hands near @c$N@W and an orange glow surrounds $M!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W places $s hands near you and an orange glow surrounds you!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@W places $s hands near @c$N@W and an orange glow surrounds $M.", TRUE, ch, 0, vict, TO_NOTVICT);
    decCurKI(ch, cost, 0);
    incCurHealth(vict, heal, true);

    if (char_is_nail(ch)) {
      if (char_get_skill(ch, SKILL_HEAL) >= 100) {
        incCurST(ch, heal * .4, true);
        send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 60) {
        incCurST(ch, heal * .2, true);
        send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 40) {
        incCurST(ch, heal * .1, true);
        send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
      }
    }

    null_affect(ch, AFF_POISON);
    null_affect(ch, AFF_BLIND);
    if (char_aff_flagged(vict, AFF_BURNED)) {
      send_to_char(vict, "Your burns are healed now.\r\n");
      act("$n@w's burns are now healed.@n", TRUE, vict, 0, 0, TO_ROOM);
      char_aff_set(vict, AFF_BURNED, false);
    }
    if (is_affected(vict, AFF_HYDROZAP)) {
      send_to_char(vict, "You no longer feel a great thirst.\r\n");
      act("$n@w no longer looks as if they could drink an ocean.@n", TRUE, vict, 0, 0, TO_ROOM);
      remove_affect(vict, AFF_HYDROZAP);
    }
    char_set_limb_condition(vict, (1) - 1, 100);
    char_set_limb_condition(vict, (2) - 1, 100);
    char_set_limb_condition(vict, (3) - 1, 100);
    char_set_limb_condition(vict, (4) - 1, 100);
    if (getCurLF(vict) <= getMaxLF(vict) * 0.5 && !char_is_android(vict)) {
      incCurLF(vict, (getMaxLF(ch)) * .35, true);
      send_to_char(vict, "You feel that your lifeforce has recovered some!\r\n");
    }
    improve_skill(ch, SKILL_HEAL, 0);
    if (vict->master == ch || ch->master == vict || ch->master == vict->master) {
      if (char_is_nail(ch) && char_is_namek(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && getCurPL(vict) <= getEffMaxPL(vict) * 0.85 && rand_number(1, 3) == 3) {
        char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.005));
      }
    }

    char_set_wait(ch, PULSE_2SEC);
  }

  if (vict == ch) {
    if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
      heal += heal * .25;
    }
    act("@WYou place your hands on your body and an orange glow surrounds you!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W places $s hands on $s body and an orange glow surrounds $m.", TRUE, ch, 0, vict, TO_NOTVICT);
    decCurKI(ch, cost, 0);
    incCurHealth(vict, heal, true);

    if (char_is_nail(ch)) {
      if (char_get_skill(ch, SKILL_HEAL) >= 100) {
        incCurST(ch, heal * .4, true);
        send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 60) {
        incCurST(ch, heal * .2, true);
        send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
      } else if (char_get_skill(ch, SKILL_HEAL) >= 40) {
        incCurST(ch, heal * .1, true);
        send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
      }
    }
    char_aff_set(vict, AFF_BLIND, false);
    char_set_limb_condition(vict, (1) - 1, 100);
    char_set_limb_condition(vict, (2) - 1, 100);
    char_set_limb_condition(vict, (3) - 1, 100);
    char_set_limb_condition(vict, (4) - 1, 100);
    if (!char_plr_flagged(vict, PLR_TAIL) && (char_is_bio(vict) || char_is_icer(vict))) {
      char_plr_set(vict, PLR_TAIL, true);
    }
    if (!char_plr_flagged(vict, PLR_STAIL) && (char_is_saiyan(vict) || char_is_halfbreed(vict))) {
      char_plr_set(vict, PLR_STAIL, true);
    }
    improve_skill(ch, SKILL_HEAL, 0);
    char_set_wait(ch, PULSE_2SEC);
  }

  return;
}

ACMD(do_barrier)
{
  int prob = 0, perc = 0, size = 0;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_BARRIER) && !char_get_skill(ch, SKILL_AQUA_BARRIER)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "[Syntax] barrier < 1-75 | release >\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SANCTUARY) && !strcasecmp("release", arg)) {
    act("@BYou dispel your barrier, releasing its energy.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@B$n@B dispels $s barrier, releasing its energy.@n", TRUE, ch, 0, 0, TO_ROOM);
    char_set_barrier(ch, 0);
    char_aff_set(ch, AFF_SANCTUARY, false);
    return;
  } else if (!strcasecmp("release", arg)) {
    send_to_char(ch, "You don't have a barrier.\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SANCTUARY)) {
    send_to_char(ch, "You already have a barrier, try releasing it.\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(ch, "You must wait a short period before concentrating again.\r\n");
    return;
  }

  size         = atoi(arg);
  int64_t cost = 0;
  prob         = 0;
  if (char_get_skill(ch, SKILL_BARRIER)) {
    prob = init_skill(ch, SKILL_BARRIER);
  } else {
    prob = char_get_skill(ch, SKILL_AQUA_BARRIER);
  }
  perc = axion_dice(0);

  cost = (getMaxKI(ch) * 0.01) * (size * 0.5);

  if (size > prob) {
    send_to_char(ch, "You can not create a barrier that is stronger than your skill in barrier.\r\n");
    return;
  } else if (size < 1) {
    send_to_char(ch, "You have to put at least some ki into the barrier!\r\n");
    return;
  } else if (size > 75) {
    send_to_char(ch, "You can't control a barrier with more than 75 percent!\r\n");
    return;
  } else if (char_get_charge(ch) < cost) {
    send_to_char(ch, "You do not have enough ki charged up!\r\n");
    return;
  } else if (prob < perc) {
    act("@BYou shout as you form a barrier of ki around your body, but you imbalance it and it explodes outward!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@B$n@B shouts as $e forms a barrier of ki around $s body, but it becomes imbalanced and explodes outward!@n", TRUE, ch, 0, 0, TO_ROOM);
    char_set_charge(ch, char_get_charge(ch) - (cost));
    if (char_get_skill(ch, SKILL_BARRIER)) {
      improve_skill(ch, SKILL_BARRIER, 2);
    } else {
      improve_skill(ch, SKILL_AQUA_BARRIER, 2);
    }
    char_set_con_cooldown(ch, 30);
    return;
  } else {
    if (char_get_skill(ch, SKILL_BARRIER)) {
      act("@BYou shout as you form a barrier of ki around your body!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@B$n@B shouts as $e forms a barrier of ki around $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else {
      act("@BYou shout as you form a barrier of ki and raging waters around your body!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@B$n@B shouts as $e forms a barrier of ki and raging waters around $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
    char_set_barrier(ch, (getMaxKI(ch) / 100) * size);
    char_set_charge(ch, char_get_charge(ch) - (cost));
    if (char_get_skill(ch, SKILL_BARRIER)) {
      improve_skill(ch, SKILL_BARRIER, 2);
    } else {
      improve_skill(ch, SKILL_AQUA_BARRIER, 2);
    }
    char_aff_set(ch, AFF_SANCTUARY, true);
    char_set_con_cooldown(ch, 20);
    return;
  }
}

ACMD(do_instant)
{

  int skill = 0, perc = 0, skill_num = 0, location = 0;
  int64_t cost          = 0;
  struct char_data *tar = NULL;

  char arg[MAX_INPUT_LENGTH] = "";

  one_argument(argument, arg);

  if (!char_is_npc(ch)) {
    if (char_prf_flagged(ch, PRF_ARENAWATCH)) {
      char_prf_set(ch, PRF_ARENAWATCH, false);
      char_set_arenawatch(ch, -1);
      send_to_char(ch, "You stop watching the arena action.\r\n");
    }
  }
  if (!know_skill(ch, SKILL_INSTANTT)) {
    return;
  } else if (!char_get_skill(ch, SKILL_SENSE) && !char_plr_flagged(ch, PLR_SENSEM)) {
    send_to_char(ch, "You can't sense them to go to there!\r\n");
    return;
  } else if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(ch, "You are busy piloting a ship!\r\n");
    return;
  } else if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return;
  } else if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
    send_to_char(ch, "@rYou are in a pocket dimension!@n\r\n");
    return;
  } else if (rroom_flagged(char_get_in_room(ch), ROOM_RHELL) || rroom_flagged(char_get_in_room(ch), ROOM_AL) || rroom_flagged(char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(ch, "You can not leave where you are at!\r\n");
    return;
  } else if (!*arg) {
    send_to_char(ch, "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
    send_to_char(ch, "Example: instant goku\nExample 2: instant planet-earth\r\n");
    return;
  }

  if (char_get_skill(ch, SKILL_INSTANTT) > 75) {
    cost = getMaxKI(ch) / 40;
  } else if (char_get_skill(ch, SKILL_INSTANTT) > 50) {
    cost = getMaxKI(ch) / 20;
  } else if (char_get_skill(ch, SKILL_INSTANTT) > 25) {
    cost = getMaxKI(ch) / 15;
  } else if (char_get_skill(ch, SKILL_INSTANTT) < 25) {
    cost = getMaxKI(ch) / 10;
  }

  if ((getCurKI(ch)) - cost < 0) {
    send_to_char(ch, "You do not have enough ki to instantaneously move.\r\n");
    return;
  }

  perc      = axion_dice(0);
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
  } else if (!(tar = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "@RThat target was not found.@n\r\n");
    send_to_char(ch, "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
    send_to_char(ch, "Example: instant goku\nExample 2: instant planet-earth\r\n");
    return;
  }

  if (skill < perc || (char_get_fighting(ch) && rand_number(1, 2) <= 1)) {
    if (tar != NULL) {
      if (tar != ch) {
        send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
        decCurKI(ch, cost, 0);
        improve_skill(ch, skill_num, 2);
        char_set_wait(ch, PULSE_2SEC);
        return;
      } else {
        send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
        return;
      }
    } else {
      send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
      decCurKI(ch, cost, 0);
      improve_skill(ch, skill_num, 2);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
  }

  reveal_hiding(ch, 0);
  char_set_wait(ch, PULSE_2SEC);
  if (tar != NULL) {
    if (tar == ch) {
      send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
      return;
    } else if (char_get_grappling(ch) && char_get_grappling(ch) == tar) {
      send_to_char(ch, "You are already in the same room with them and are grappling with them!\r\n");
      return;
    } else if (!read_sense_memory(ch, tar)) {
      send_to_char(ch, "You've never sensed them up close so you do not have a good bearing on their ki signal.\r\n");
      return;
    } else if (char_get_admlevel(tar) > 0 && char_get_admlevel(ch) < 1) {
      send_to_char(ch, "That immortal prevents you from reaching them.\r\n");
      return;
    } else if (char_is_android(tar) || getCurPL(tar) < (getCurPL(ch) * 0.001) + 1) {
      send_to_char(ch, "You can't sense them well enough.\r\n");
      return;
    } else if (!rroom_flagged(char_get_in_room(ch), ROOM_AL) && rroom_flagged(char_get_in_room(tar), ROOM_AL)) {
      send_to_char(ch, "They are dead and can't be reached.\r\n");
      return;
    } else if (!rroom_flagged(char_get_in_room(ch), ROOM_RHELL) && rroom_flagged(char_get_in_room(tar), ROOM_RHELL)) {
      send_to_char(ch, "They are dead and can't be reached.\r\n");
      return;
    } else if (rroom_flagged(char_get_in_room(tar), ROOM_NOINSTANT)) {
      send_to_char(ch, "You can not go there as it is a protected area!\r\n");
      return;
    }

    decCurKI(ch, cost, 0);
    act("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move through the molecules of the universe faster than the speed of light. You stop as you arrive at $N@w!@n", TRUE, ch, 0, tar, TO_CHAR);
    act("@w$n@w appears in an instant out of nowhere right next to you!@n", TRUE, ch, 0, tar, TO_VICT);
    act("@w$n@w places two fingers on $s forehead and disappears in an instant!@n", TRUE, ch, 0, tar, TO_NOTVICT);
    char_plr_set(ch, PLR_TRANSMISSION, true);
    handle_teleport(ch, tar, 0);
    improve_skill(ch, skill_num, 2);
  } else {
    decCurKI(ch, cost, 0);
    act("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move faster than light and arrive almost instantly at your destination. Having located the planet by its collective population's ki.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@w$n@w places two fingers on $s forehead and disappears in an instant!@n", TRUE, ch, 0, 0, TO_NOTVICT);
    handle_teleport(ch, NULL, location);
    improve_skill(ch, skill_num, 2);
  }
}

ACMD(do_transform)
{

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf3[MAX_INPUT_LENGTH];

  if (!race_can_transform(ch->race)) {
    send_to_char(ch, "You do not have a transformation.\r\n");
    return;
  }

  bool npc = char_is_npc(ch);

  /*R: Hidden transformation stuff following this*/
  if (!npc && (getBasePL(ch)) < 50000) {
    send_to_char(ch, "@RYou are too weak to comprehend transforming!@n\r\n");
    return;
  }

  // Moved this further down. No need to parse the entry if we quit early. -Volund
  two_arguments(argument, arg, arg2);

  /* Called with no argument - display transformation information */
  if (!*arg) {
    race_display_forms(ch);
    if (trans_req(ch, 1) > 0) {
      race_display_trans_req(ch);
    }
    return;
  } /* End of No Argument */

  transform_bonus cur_form = race_current_form(ch);
  bool can_revert          = race_can_revert(ch->race);

  // If we are in kaioken or something weird like that, prevent transforming.
  if (!race_check_can_transform(ch)) {
    return;
  }

  // check for revert.
  if (!strcasecmp("revert", arg)) {
    if (!can_revert) {
      send_to_char(ch, "That would be unthinkable.\r\n");
      return;
    }
    if (!cur_form.flag) {
      send_to_char(ch, "You are not transformed.\r\n");
      return;
    }
    // We are all clear to revert.
    if ((char_get_charge(ch) > 0)) {
      do_charge(ch, "release", 0, 0);
    }
    race_echo_revert(ch, race_flag_to_tier(cur_form.flag));
    char_plr_set(ch, cur_form.flag, false);

    if (*arg2) {
      do_transform(ch, arg2, 0, 0);
    }
    return;
  }

  // Search for available transformations. Error out if we can't find one.
  transform_bonus trans;
  if (!race_find_form(ch, arg, &trans)) {
    send_to_char(ch, "You don't have that form.\r\n");
    return;
  }

  int to_tier = race_flag_to_tier(trans.flag);

  if (char_plr_flagged(ch, trans.flag)) {
    send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
    return;
  }

  if (!npc && (getBasePL(ch)) < trans_req(ch, to_tier)) {
    send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
    return;
  }

  if (!npc && (getCurST(ch)) <= getMaxST(ch) * trans.drain) {
    send_to_char(ch, "You do not have enough stamina!");
    return;
  }

  if (!npc) {
    // Pay the price to unlock form if necessary.
    if (!race_check_trans_unlock(ch, to_tier)) {
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
  race_echo_transform(ch, to_tier);

  // No way is this a stealthy process...
  reveal_hiding(ch, 0);

  // Announce noisy transformations in the zone.
  int zone = 0;
  if (race_has_noisy_transformations(ch->race)) {
    if ((zone = real_zone_by_thing(char_get_in_room(ch))) != NOWHERE) {
      send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
    };
  }

  send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
  sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(getCurPL(ch)));
  send_to_scouter(buf3, ch, 1, 0);
}

ACMD(do_aid)
{
  struct char_data *vict;
  struct obj_data *obj = NULL, *aid_obj = NULL, *aid_prod = NULL, *next_obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int dc = 0, found = FALSE, num = 47, num2 = 0;

  if (char_is_npc(ch))
    return;

  if (char_get_skill(ch, SKILL_SURVIVAL)) {
  }

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: aid heal (target)\r\n");
    send_to_char(ch, "        aid adrenex\r\n");
    send_to_char(ch, "        aid antitoxin\r\n");
    send_to_char(ch, "        aid salve\r\n");
    send_to_char(ch, "        aid formula-82\r\n");
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

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (found == FALSE && obj_get_vnum(obj) == num && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found   = TRUE;
      aid_obj = obj;
    }
  }

  if (found == FALSE || aid_obj == NULL) {
    if (num == 47) {
      send_to_char(ch, "You need bandages to be able to use first aid.\r\n");
    } else {
      send_to_char(ch, "You need a TCX-Medical Equipment Construction Kit.\r\n");
    }
    return;
  }
  if (num == 47) {
    if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Apply first aid to who?\r\n");
      return;
    } else if (char_is_npc(vict)) {
      send_to_char(ch, "What ever for?\r\n");
      return;
    } else if (char_is_android(vict)) {
      send_to_char(ch, "They are an android!\r\n");
      return;
    }

    if (!char_aff_flagged(vict, AFF_SPIRIT) && !char_plr_flagged(vict, PLR_BANDAGED)) {
      if (vict != ch) {
        send_to_char(ch, "You attempt to lend first aid to %s.\r\n", char_get_name(vict));
      }
      act("$n attempts to bandage $N's wounds.", TRUE, ch, 0, vict, TO_ROOM);
      dc = axion_dice(0);
      if ((char_get_skill(ch, SKILL_FIRST_AID) + 1) > dc) {
        send_to_char(ch, "You bandage %s's wounds.\r\n", char_get_name(vict));
        int64_t roll = (getEffMaxPL(vict) / 100) * (char_get_aff_abils_wis(ch) / 4) + getEffMaxPL(vict) * 0.25;
        if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
          roll += roll * .1;
        }
        incCurHealth(vict, roll, true);

        send_to_char(vict, "Your wounds are bandaged by %s!\r\n", char_get_name(ch));
        act("$n's wounds are stablized by $N!", TRUE, vict, 0, ch, TO_NOTVICT);
        char_plr_set(vict, PLR_BANDAGED, true);
        extract_obj(aid_obj);
      } else {
        if (vict != ch) {
          send_to_char(ch, "You fail to bandage their wounds properly, wasting the set of bandages...\r\n");
          act("$N fails to bandage $n's wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_NOTVICT);
          act("$N fails to bandage your wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_CHAR);
        } else {
          act("$N fails to bandage $s wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_NOTVICT);
          act("You fail to bandage your wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_VICT);
        }
        extract_obj(aid_obj);
      }
      improve_skill(ch, SKILL_FIRST_AID, 1);
    } else if (char_plr_flagged(vict, PLR_BANDAGED)) {
      send_to_char(ch, "They are already bandaged!\r\n");
    } else if (char_aff_flagged(vict, AFF_SPIRIT)) {
      send_to_char(ch, "The dead don't need first aid.\r\n");
    } else {
      send_to_char(ch, "They apparently do not need bandaging.\r\n");
    }
  } else if (num2 == 381) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 65) {
      send_to_char(ch, "You need at least a skill level of 65 in First Aid.\r\n");
      return;
    } else {
      if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(15)) {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you begin to construct an Andrenex Adreneline Injector you screw up and end up breaking the water tight seal. The adreneline leaks out and is wasted.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
        extract_obj(aid_obj);
      } else {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully construct an Adrenex Adreneline Injector@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Adrenex Adreneline Injector!@n", TRUE, ch, 0, 0, TO_ROOM);
        aid_prod = read_object(num2, VIRTUAL);
        add_unique_id(aid_prod);
        obj_to_char(aid_prod, ch);
        extract_obj(aid_obj);
        improve_skill(ch, SKILL_FIRST_AID, 1);
      }
    }
  } else if (num2 == 382) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 50) {
      send_to_char(ch, "You need at least a skill level of 50 in First Aid.\r\n");
      return;
    } else {
      if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(10)) {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you go to put the salve ingredients into the kit's salve compartment and set the temperature you accidentally set it too high. The salve is burned and ruined. Yes you managed to burn a burn salve.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
        extract_obj(aid_obj);
      } else {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully boil a burn salve to perfection and it is automatically placed in a jar.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a jar of burn salve!@n", TRUE, ch, 0, 0, TO_ROOM);
        aid_prod = read_object(num2, VIRTUAL);
        add_unique_id(aid_prod);
        obj_to_char(aid_prod, ch);
        extract_obj(aid_obj);
        improve_skill(ch, SKILL_FIRST_AID, 1);
      }
    }
  } else if (num2 == 383) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 40) {
      send_to_char(ch, "You need at least a skill level of 40 in First Aid.\r\n");
      return;
    } else {
      if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(5)) {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete the Antitoxin Injector you notice that you didn't seal the syringe properly and it all leaks out.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
        extract_obj(aid_obj);
      } else {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble the Antitoxin Injector.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Antitoxin Injector!@n", TRUE, ch, 0, 0, TO_ROOM);
        aid_prod = read_object(num2, VIRTUAL);
        add_unique_id(aid_prod);
        obj_to_char(aid_prod, ch);
        extract_obj(aid_obj);
        improve_skill(ch, SKILL_FIRST_AID, 1);
      }
    }
  } else if (num2 == 385) {
    if (char_get_skill(ch, SKILL_FIRST_AID) < 40) {
      send_to_char(ch, "You need at least a skill level of 40 in First Aid.\r\n");
      return;
    } else {
      if (char_get_skill(ch, SKILL_FIRST_AID) < axion_dice(15)) {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete a vial of Formula 82 you notice that you read the mixture measurements wrong. You dispose of the vile vial immediately.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
        extract_obj(aid_obj);
      } else {
        act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble a vial of Formula 82.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Vial of Formula 82!@n", TRUE, ch, 0, 0, TO_ROOM);
        aid_prod = read_object(num2, VIRTUAL);
        add_unique_id(aid_prod);
        obj_to_char(aid_prod, ch);
        extract_obj(aid_obj);
        improve_skill(ch, SKILL_FIRST_AID, 1);
      }
    }
  }

  char_set_wait(ch, PULSE_3SEC);
}

ACMD(do_aura)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: aura light\r\n");
    return;
  }

  if (char_get_charge(ch)) {
    send_to_char(ch, "You can't focus enough on this while charging.");
    return;
  }

  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(ch, "You are busy powering up!\r\n");
    return;
  }

  if (!strcasecmp(arg, "light")) {
    if (char_get_skill(ch, SKILL_FOCUS) < 75 || char_get_skill(ch, SKILL_CONCENTRATION) < 75) {
      send_to_char(ch, "You need at least a skill level of 75 in Focus and Concentration to use this.\r\n");
      return;
    } else {
      if (char_plr_flagged(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "Your aura fades as you stop shining light.\r\n");
        act("$n's aura fades as they stop shining light on the area.", TRUE, ch, 0, 0, TO_ROOM);
        char_plr_set(ch, PLR_AURALIGHT, false);
        world[char_get_in_room(ch)].light--;

      } else if ((getCurKI(ch)) > getMaxKI(ch) * 0.12) {
        if (char_get_in_room(ch) != NOWHERE) {
          world[char_get_in_room(ch)].light++;
        }
        reveal_hiding(ch, 0);
        decCurKIPercent(ch, .12, 0);
        send_to_char(ch, "A bright %s aura begins to burn around you as you provide light to the surrounding area!\r\n", aura_types[char_get_aura(ch)]);
        char bloom[MAX_INPUT_LENGTH];
        sprintf(bloom, "@wA %s aura flashes up brightly around $n@w as they provide light to the area.@n", aura_types[char_get_aura(ch)]);
        act(bloom, TRUE, ch, 0, 0, TO_ROOM);
        char_plr_set(ch, PLR_AURALIGHT, true);

      } else {
        send_to_char(ch, "You don't have enough KI to do that.\r\n");
        return;
      }
    }
  }
}
