/* focus.c: the focus command (ki techniques)
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <sys/stat.h>

#include "dbat/args.hpp"
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

ACMD(do_focus)
{
  struct char_data *vict = nullptr;
  std::string arg;
  std::string name;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  name                  = take_arg(rest);

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
  if (!(arg != "tough")) {
    if (!know_skill(g, ch, SKILL_TSKIN)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_STONESKIN)) {
        send_to_char(g, ch, "You already have tough skin!\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into your skin.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_TSKIN) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into your skin, but fail in making it tough!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki into $s skin, but fails in making it tough!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      int duration = char_get_aff_abils_intel(ch) / 20;
      assign_affect(g, ch, AFF_STONESKIN, SKILL_TSKIN, duration, 0, 0, 0, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into your skin, making it tough!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki into $s skin, making it tough!", true, ch, nullptr, nullptr, TO_ROOM);
      return;
    } // End of no vict tough skin

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Focus your ki into who's skin?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_STONESKIN)) {
        send_to_char(g, ch, "They already have tough skin!\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into their skin.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_TSKIN) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into $N's skin, but fail in making it tough!", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki into your skin, but fails in making it tough!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki into $N's skin, but fails in making it tough!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      assign_affect(g, vict, AFF_STONESKIN, SKILL_TSKIN, duration, 0, 0, 0, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's skin, making it tough!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your skin, making it tough!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki into $N's skin, making it tough!", true, ch, nullptr, vict, TO_NOTVICT);
      return;

    } // End of victim of tough skin
  } // End of tough skin

  else if (!(arg != "might")) {
    if (!know_skill(g, ch, SKILL_MIGHT)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_MIGHT)) {
        send_to_char(g, ch, "You already have mighty muscles!\r\n");
        return;
      }
      if (char_get_bonuses(ch, BONUS_WIMP) > 0 && char_get_aff_abils_str(ch) + 10 > 70) {
        send_to_char(g, ch, "Your body is not able to withstand increasing its strength beyond 70.\r\n");
        return;
      }
      if (char_get_bonuses(ch, BONUS_FRAIL) > 0 && char_get_aff_abils_str(ch) + 2 > 70) {
        send_to_char(g, ch, "Your body is not able to withstand increasing its strength beyond 70.\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into your muscles.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_MIGHT) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into your muscles, but fail in making them mighty!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki into $s muscles, but fails in making them mighty!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      char_aff_set(ch, AFF_MIGHT, true);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, ch, AFF_MIGHT, SKILL_MIGHT, duration, 10, 2, 0, 0, 0, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into your muscles, making them mighty!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki into $s muscles, making them mighty!", true, ch, nullptr, nullptr, TO_ROOM);
      return;
    } // End of no vict might

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Focus your ki into who's muscles?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_MIGHT)) {
        send_to_char(g, ch, "They already have mighty muscles!\r\n");
        return;
      }
      if (char_get_bonuses(vict, BONUS_WIMP) > 0 && char_get_aff_abils_str(vict) + 10 > 70) {
        send_to_char(g, ch, "Their body is not able to withstand increasing its strength beyond 70.\r\n");
        return;
      }
      if (char_get_bonuses(vict, BONUS_FRAIL) > 0 && char_get_aff_abils_con(vict) + 2 > 70) {
        send_to_char(g, ch, "Their body is not able to withstand increasing its constitution beyond 70.\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into their muscles.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_MIGHT) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into $N's muscles, but fail in making them mighty!", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki into your muscles, but fails in making them mighty!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki into $N's muscles, but fails in making them mighty!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_MIGHT, SKILL_MIGHT, duration, 10, 2, 0, 0, 0, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's muscles, making them mighty!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your muscles, making them mighty!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki into $N's muscles, making them mighty!", true, ch, nullptr, vict, TO_NOTVICT);
      return;

    } // End of victim of might
  } // End of might

  else if (!(arg != "wither")) {
    if (!know_skill(g, ch, SKILL_WITHER)) {
      return;
    }
    if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Focus your ki into who's muscles?\r\n");
      return;
    }
    if (ch == vict) {
      send_to_char(g, ch, "You don't want to wither your own body!\r\n");
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 2)) {
      return;
    }
    if (is_affected(vict, AFF_WITHER)) {
      send_to_char(g, ch, "They already have been withered!\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
      send_to_char(g, ch, "You do not have enough ki to wither them.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_WITHER) < axion_dice(g, 0)) {
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's body, but fail in withering it!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your body, but fails in withering it!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki into $N's body, but fails in withering it!", true, ch, nullptr, vict, TO_NOTVICT);
      return;
    }
    decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
    assign_affect(g, vict, AFF_WITHER, SKILL_WITHER, -1, -3, 0, 0, 0, 0, -3);
    save_char(g, vict);
    reveal_hiding(g, ch, 0);
    act(g, "You focus ki into $N's body, and succeed in withering it!", true, ch, nullptr, vict, TO_CHAR);
    act(g, "$n focuses ki into your body, and succeeds in withering it!", true, ch, nullptr, vict, TO_VICT);
    act(g, "$n focuses ki into $N's body, and succeeds in withering it!", true, ch, nullptr, vict, TO_NOTVICT);
    return;
  } // End of wither

  else if (!(arg != "enlighten")) {
    if (!know_skill(g, ch, SKILL_ENLIGHTEN)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_ENLIGHTEN)) {
        send_to_char(g, ch, "You already have superior wisdom!\r\n");
        return;
      }
      if (char_get_bonuses(ch, BONUS_FOOLISH) > 0 && char_get_aff_abils_wis(ch) + 10 > 70) {
        send_to_char(g, ch, "You're not able to withstand increasing your wisdom beyond 70.\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to use this skill.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_ENLIGHTEN) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into your mind, but fail in awakening it to cosmic wisdom!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki into $s mind, but fails in awakening it to cosmic wisdom!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, ch, AFF_ENLIGHTEN, SKILL_ENLIGHTEN, duration, 0, 0, 0, 0, 10, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into your mind, awakening it to cosmic wisdom!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki into $s mind, awakening it to cosmic wisdom!", true, ch, nullptr, nullptr, TO_ROOM);
      if (char_is_jinto(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && char_get_practice_points(g, ch) >= 15 && rand_number(g, 1, 4) >= 3) {
        int64_t gain = 0;
        char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (15));
        if (char_get_skill(ch, SKILL_ENLIGHTEN) >= 100) {
          gain = level_exp(ch, char_get_level(ch) + 1) * 0.15;
          char_set_exp(ch, char_get_exp(ch) + (gain));
          send_to_char(g, ch, "@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
        } else if (char_get_skill(ch, SKILL_ENLIGHTEN) >= 60) {
          gain = level_exp(ch, char_get_level(ch) + 1) * 0.10;
          char_set_exp(ch, char_get_exp(ch) + (gain));
          send_to_char(g, ch, "@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
        } else if (char_get_skill(ch, SKILL_ENLIGHTEN) >= 40) {
          gain = level_exp(ch, char_get_level(ch) + 1) * 0.05;
          char_set_exp(ch, char_get_exp(ch) + (gain));
          send_to_char(g, ch, "@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
        }
      }
      return;
    } // End of no vict enlighten

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Focus your ki into who's mind?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_ENLIGHTEN)) {
        send_to_char(g, ch, "They already have superior wisdom!\r\n");
        return;
      }
      if (char_get_bonuses(vict, BONUS_FOOLISH) > 0 && char_get_aff_abils_wis(vict) + 10 > 70) {
        send_to_char(g, ch, "They're not able to withstand increasing their wisdom beyond 70.\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to use this skill.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_ENLIGHTEN) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into $N's mind, but fail in awakening it to cosmic wisdom!", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki into your mind, but fails in awakening it to cosmic wisdom!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki into $N's mind, but fails in awakening it to cosmic wisdom!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_ENLIGHTEN, SKILL_ENLIGHTEN, duration, 0, 0, 0, 0, 10, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's mind, awakening it to cosmic wisdom!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your mind, awakening it to cosmic wisdom!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki into $N's mind, awakening it to cosmic wisdom!", true, ch, nullptr, vict, TO_NOTVICT);
      if (char_is_jinto(ch) && level_exp(vict, char_get_level(vict) + 1) - char_get_exp(vict) > 0 && char_get_practice_points(g, ch) >= 15 && rand_number(g, 1, 4) >= 3) {
        int64_t gain = 0;
        char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (15));
        if (char_get_skill(ch, SKILL_ENLIGHTEN) >= 100) {
          gain = level_exp(vict, char_get_level(vict) + 1) * 0.15;
          char_set_exp(vict, char_get_exp(vict) + (gain));
          send_to_char(g, vict, "@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
        } else if (char_get_skill(ch, SKILL_ENLIGHTEN) >= 60) {
          gain = level_exp(vict, char_get_level(vict) + 1) * 0.10;
          char_set_exp(vict, char_get_exp(vict) + (gain));
          send_to_char(g, vict, "@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
        } else if (char_get_skill(ch, SKILL_ENLIGHTEN) >= 40) {
          gain = level_exp(vict, char_get_level(vict) + 1) * 0.05;
          char_set_exp(vict, char_get_exp(vict) + (gain));
          send_to_char(g, vict, "@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
        }
      }
      return;

    } // End of victim of enlighten
  } // End of enlighten

  else if (!(arg != "genius")) {
    if (!know_skill(g, ch, SKILL_GENIUS)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_GENIUS)) {
        send_to_char(g, ch, "You already have superior intelligence!\r\n");
        return;
      }
      if (char_get_bonuses(ch, BONUS_DULL) > 0 && char_get_aff_abils_intel(ch) + 10 > 70) {
        send_to_char(g, ch, "You're not able to withstand increasing your intelligence beyond 70.\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into your mind.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_GENIUS) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into your mind, but fail in making it work faster!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki into $s muscles, but fails in making it work faster!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, ch, AFF_GENIUS, SKILL_GENIUS, duration, 0, 0, 10, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into your mind, making it work faster!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki into $s mind, making it work faster!", true, ch, nullptr, nullptr, TO_ROOM);
      return;
    } // End of no vict genius

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Focus your ki into who's mind?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_GENIUS)) {
        send_to_char(g, ch, "They already have superior intelligence!\r\n");
        return;
      }
      if (char_get_bonuses(vict, BONUS_DULL) > 0 && char_get_aff_abils_intel(vict) + 10 > 70) {
        send_to_char(g, ch, "They're not able to withstand increasing their intelligence beyond 70.\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into their mind.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_GENIUS) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into $N's mind, but fail in making it work faster!", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki into your mind, but fails in making it work faster!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki into $N's mind, but fails in making it work faster!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_GENIUS, SKILL_GENIUS, duration, 0, 0, 10, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's mind, making it work faster!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your mind, making it work faster!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki into $N's mind, making it work faster!", true, ch, nullptr, vict, TO_NOTVICT);
      if ((char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict)) && char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
        if (char_is_kai(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && rand_number(g, 1, 3) == 3) {
          char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.05));
        }
      }
      return;

    } // End of victim of genius
  } // End of genius

  else if (!(arg != "flex")) {
    if (!know_skill(g, ch, SKILL_FLEX)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_FLEX)) {
        send_to_char(g, ch, "You already have superior agility!\r\n");
        return;
      }
      if (char_get_bonuses(ch, BONUS_CLUMSY) > 0 && char_get_aff_abils_dex(ch) + 10 > 70) {
        send_to_char(g, ch, "You're not able to withstand increasing your agility beyond 70.\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into your limbs.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_FLEX) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into your limbs, but fail in making them more flexible!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki into $s muscles, but fails in making them more flexible!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, ch, AFF_FLEX, SKILL_FLEX, duration, 0, 0, 0, 10, 0, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into your limbs, making them more flexible!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki into $s limbs, making them more flexible!", true, ch, nullptr, nullptr, TO_ROOM);
      return;
    } // End of no vict FLEX

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Focus your ki into who's limbs?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_FLEX)) {
        send_to_char(g, ch, "They already have superior agility!\r\n");
        return;
      }
      if (char_get_bonuses(vict, BONUS_CLUMSY) > 0 && char_get_aff_abils_dex(vict) + 3 > 70) {
        send_to_char(g, ch, "They're not able to withstand increasing their agility beyond 70.\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to infuse into their limbs.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_FLEX) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into $N's limbs, but fail in making them more flexible!", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki into your limbs, but fails in making them more flexible!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki into $N's limbs, but fails in making them more flexible!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 2);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_FLEX, SKILL_FLEX, duration, 0, 0, 0, 10, 0, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's limbs, making them more flexible!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your limbs, making them more flexible!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki into $N's limbs, making them more flexible!", true, ch, nullptr, vict, TO_NOTVICT);
      if ((char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict)) && char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
        if (char_is_kai(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && rand_number(g, 1, 3) == 3) {
          char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.05));
        }
      }
      return;

    } // End of victim of FLEX
  } // End of FLEX

  else if (!(arg != "bless")) {
    if (!know_skill(g, ch, SKILL_BLESS)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_BLESS)) {
        send_to_char(g, ch, "You already are blessed!\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to bless.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_BLESS) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki while chanting spiritual words. Your blessing does nothing though, you must have messed up!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki while chanting spiritual words. $n seems disappointed.", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 3);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, ch, AFF_BLESS, SKILL_BLESS, duration, 0, 0, 0, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      if (char_is_kabito(ch)) {
        char_set_blesslvl(ch, char_get_skill(ch, SKILL_BLESS));
      } else {
        char_set_blesslvl(ch, 0);
      }
      act(g, "You focus ki while chanting spiritual words. You feel your body recovering at above normal speed!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki while chanting spiritual words. $n smiles after finishing $s chant.", true, ch, nullptr, nullptr, TO_ROOM);
      if (char_aff_flagged(ch, AFF_CURSE)) {
        send_to_char(g, ch, "Your cursing was nullified!\r\n");
        null_affect(g, ch, AFF_CURSE);
      }
      return;
    } // End of no vict BLESS

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Bless who?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_BLESS)) {
        send_to_char(g, ch, "They already have been blessed!\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to bless.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_BLESS) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki while chanting spiritual words. Your blessing fails!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki while chanting spiritual words. $n places a hand on your head, but nothing happens!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki while chanting spiritual words. $n places a hand on $N's head, but nothing happens!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 3);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_BLESS, SKILL_BLESS, duration, 0, 0, 0, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      if (char_is_kai(ch)) {
        char_set_blesslvl(vict, char_get_skill(ch, SKILL_BLESS));
      } else {
        char_set_blesslvl(vict, 0);
      }
      act(g, "You focus ki while chanting spiritual words. Blessing $N with faster regeneration!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki while chanting spiritual words. $n then places a hand on your head, blessing you!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki while chanting spiritual words. $n then places a hand on $N's head, blessing them!", true, ch, nullptr, vict, TO_NOTVICT);
      if ((char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict)) && char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
        if (char_is_kai(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 && rand_number(g, 1, 3) == 3) {
          char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.05));
        }
      }
      if (char_aff_flagged(vict, AFF_CURSE)) {
        send_to_char(g, vict, "Your cursing was nullified!\r\n");
        null_affect(g, vict, AFF_CURSE);
      }
      return;

    } // End of victim of BLESS
  } // End of BLESS

  else if (!(arg != "curse")) {
    if (!know_skill(g, ch, SKILL_CURSE)) {
      return;
    }
    if (name.empty()) {
      if (char_aff_flagged(ch, AFF_CURSE)) {
        send_to_char(g, ch, "You already are cursed!\r\n");
        return;
      }
      if (char_is_demon(ch)) {
        send_to_char(g, ch, "You are immune to curses!\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to CURSE.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_CURSE) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki while chanting demonic words. Your cursing does nothing though, you must have messed up!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki while chanting demonic words. $n seems disappointed.", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 3);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_CURSE, SKILL_CURSE, duration, 0, 0, 0, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki while chanting demonic words. You feel your body recovering at below normal speed!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki while chanting demonic words. $n grins after finishing $s chant.", true, ch, nullptr, nullptr, TO_ROOM);
      if (char_aff_flagged(ch, AFF_BLESS)) {
        send_to_char(g, ch, "Your blessing was nullified!\r\n");
        null_affect(g, ch, AFF_BLESS);
      }
      return;
    } // End of no vict CURSE

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Curse who?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 0)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (char_aff_flagged(vict, AFF_CURSE)) {
        send_to_char(g, ch, "They already have been cursed!\r\n");
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if (char_is_demon(vict)) {
        send_to_char(g, ch, "They are immune to curses!\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to CURSE.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_CURSE) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki while chanting demonic words. Your cursing fails!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki while chanting demonic words. $n places a hand on your head, but nothing happens!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki while chanting demonic words. $n places a hand on $N's head, but nothing happens!", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      int duration = roll_aff_duration(g, char_get_aff_abils_intel(ch), 3);
      ;
      /* Str , Con, Int, Agl, Wis, Spd */
      assign_affect(g, vict, AFF_CURSE, SKILL_CURSE, duration, 0, 0, 0, 0, 0, 0);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki while chanting demonic words. cursing $N with slower regeneration!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki while chanting demonic words. $n then places a hand on your head, cursing you!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki while chanting demonic words. $n then places a hand on $N's head, cursing them!", true, ch, nullptr, vict, TO_NOTVICT);
      if (char_aff_flagged(vict, AFF_BLESS)) {
        send_to_char(g, vict, "Your blessing was nullified!\r\n");
        null_affect(g, vict, AFF_BLESS);
      }
      return;

    } // End of victim of CURSE
  } // End of CURSE

  else if (!(arg != "yoikominminken") || !(arg != "yoik")) {
    if (!know_skill(g, ch, SKILL_YOIK)) {
      return;
    }
    if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Use Yoikominminken on who?\r\n");
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
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
    if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
      send_to_char(g, ch, "You do not have enough ki to use Yoikominminken.\r\n");
      return;
    }
    if (char_get_bonuses(vict, BONUS_INSOMNIAC)) {
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki while moving your hands in lulling patterns, but $N doesn't look the least bit sleepy!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki while moving $s hands in a lulling pattern, but you just don't feel tired.", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki while moving $s hands in a lulling pattern, but $N doesn't look the least bit sleepy!", true, ch, nullptr, vict, TO_NOTVICT);
      return;
    }
    if (char_get_skill(ch, SKILL_YOIK) < axion_dice(g, 0) || (char_get_aff_abils_intel(ch) + rand_number(g, 1, 3) < char_get_aff_abils_intel(vict) + rand_number(g, 1, 5))) {
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki while moving your hands in lulling patterns, but fail to put $N to sleep!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki while moving $s hands in a lulling pattern, but you resist the technique!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki while moving $s hands in a lulling pattern, but $N resists the technique!", true, ch, nullptr, vict, TO_NOTVICT);
      return;
    }
    int duration = rand_number(g, 1, 2);
    /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(g, vict, AFF_SLEEP, SKILL_YOIK, duration, 0, 0, 0, 0, 0, 0);
    decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
    reveal_hiding(g, ch, 0);
    act(g, "You focus ki while moving your hands in lulling patterns, putting $N to sleep!", true, ch, nullptr, vict, TO_CHAR);
    act(g, "$n focuses ki while moving $s hands in a lulling pattern, before you realise it you are asleep!", true, ch, nullptr, vict, TO_VICT);
    act(g, "$n focuses ki while moving $s hands in a lulling pattern, putting $N to sleep!", true, ch, nullptr, vict, TO_NOTVICT);
    char_set_position(vict, POS_SLEEPING);
    if (char_aff_flagged(vict, AFF_FLYING)) {
      char_aff_set(vict, AFF_FLYING, false);
      char_set_altitude(vict, 0);
    }
    return;
  } // End of Yoik

  else if (!(arg != "vigor")) {
    if (!know_skill(g, ch, SKILL_VIGOR)) {
      return;
    }
    if (name.empty()) {
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 10) {
        send_to_char(g, ch, "You do not have enough ki to use vigor.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_VIGOR) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into your very cells, but fail at re-engerizing them!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki and glows green for a moment, $e then frowns.", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_wait(ch, PULSE_2SEC);
        return;
      }
      if ((getCurST(g, ch)) >= getMaxST(g, ch)) {
        send_to_char(g, ch, "You already have full stamina.\r\n");
        return;
      }
      if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
        incCurST(g, ch, getMaxKI(g, ch) / 8, true);
        decCurKI(g, ch, getMaxKI(g, ch) / 8, 0);
      } else {
        incCurST(g, ch, getMaxKI(g, ch) / 10, true);
        decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
      }

      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into your very cells, and manage to re-energize them!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki and glows green for a moment, $e then smiles.", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_wait(ch, PULSE_2SEC);
      return;
    } // End of no vict VIGOR

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "VIGOR who?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (char_is_npc(vict)) {
        send_to_char(g, ch, "Whatever would you waste your ki on them for?\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 10) {
        send_to_char(g, ch, "You do not have enough ki to use vigor.\r\n");
        return;
      }
      if ((getCurST(g, vict)) >= getMaxST(g, vict)) {
        send_to_char(g, ch, "They already have full stamina.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_VIGOR) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki into $N's very cells, and fail at re-energizing them!", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki into your very cells, but nothing happens!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki and $N glows green for a moment, $N frowns.", true, ch, nullptr, vict, TO_NOTVICT);
        char_set_wait(ch, PULSE_2SEC);
        return;
      }
      if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
        incCurST(g, vict, getMaxKI(g, vict) / 8, true);
        decCurKI(g, ch, getMaxKI(g, ch) / 8, 0);
      } else {
        incCurST(g, vict, getMaxKI(g, vict) / 10, true);
        decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
      }
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki into $N's very cells, and manage to re-energize them!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki into your very cells, and manages to re-energize them!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki and $N glows green for a moment, $N smiles.", true, ch, nullptr, vict, TO_NOTVICT);
      char_set_wait(ch, PULSE_2SEC);
      return;

    } // End of victim of VIGOR
  } // End of VIGOR

  else if (!(arg != "cure")) {
    if (!know_skill(g, ch, SKILL_CURE)) {
      return;
    }
    if (name.empty()) {
      if (!char_aff_flagged(ch, AFF_POISON)) {
        send_to_char(g, ch, "You are not poisoned!\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to cure.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_CURE) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki and aim a pulsing light at your body. Nothing happens!", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "$n focuses ki and aims a pulsing light at $s body. Nothing seems to happen.", true, ch, nullptr, nullptr, TO_ROOM);
        return;
      }
      affect_from_char(g, ch, SPELL_POISON);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki and aim a pulsing light at your body. You feel the poison in your blood disappear!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n focuses ki and aims a pulsing light at $s body. $n smiles.", true, ch, nullptr, nullptr, TO_ROOM);
      null_affect(g, ch, AFF_POISON);
      return;
    } // End of no vict cure

    else {
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "cure who?\r\n");
        return;
      }
      if (!can_kill(g, ch, vict, nullptr, 2)) {
        return;
      }
      if (ch == vict) {
        send_to_char(g, ch, "Use focus %s, not focus %s %s.\r\n", arg.c_str(), arg.c_str(), char_get_name(vict));
        return;
      }
      if (!char_aff_flagged(vict, AFF_POISON)) {
        send_to_char(g, ch, "They are not poisoned!\r\n");
        return;
      }
      if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
        send_to_char(g, ch, "You do not have enough ki to cure.\r\n");
        return;
      }
      if (char_get_skill(ch, SKILL_CURE) < axion_dice(g, 0)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        reveal_hiding(g, ch, 0);
        act(g, "You focus ki and aim a pulsing light at $N's body. Nothing happens.", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n focuses ki and aims a pulsing light at your body. You are STILL poisoned!", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n focuses ki and aims a pulsing light at $N's body. $N looks disappointed.", true, ch, nullptr, vict, TO_NOTVICT);
        return;
      }
      affect_from_char(g, vict, SPELL_POISON);
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki and aim a pulsing light at $N's body. $e is cured.", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki and aims a pulsing light at your body. You have been cured of your poison!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki and aims a pulsing light at $N's body. $N smiles.", true, ch, nullptr, vict, TO_NOTVICT);
      null_affect(g, vict, AFF_POISON);
      return;

    } // End of victim of cure
  } // End of cure

  else if (!(arg != "poison")) {

    if (!know_skill(g, ch, SKILL_POISON)) {
      return;
    }
    if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Poison who?\r\n");
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (ch == vict) {
      send_to_char(g, ch, "Why poison yourself?\r\n");
      return;
    }
    if (char_is_npc(vict)) {
      if (char_mob_flagged(vict, MOB_NOPOISON)) {
        send_to_char(g, ch, "You get the feeling that this being is immune to poison.\r\n");
        return;
      }
    }
    if (char_aff_flagged(vict, AFF_POISON)) {
      send_to_char(g, ch, "They already have been poisoned!\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
      send_to_char(g, ch, "You do not have enough ki to poison.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_POISON) < axion_dice(g, 0)) {
      decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
      reveal_hiding(g, ch, 0);
      act(g, "You focus ki and fling poison at $N. You missed!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$n focuses ki and flings poison at you, but misses!", true, ch, nullptr, vict, TO_VICT);
      act(g, "$n focuses ki and flings poison at $N, but misses!", true, ch, nullptr, vict, TO_NOTVICT);
      return;
    }
    decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
    reveal_hiding(g, ch, 0);
    act(g, "You focus ki and fling poison at $N! The poison burns into $s skin!", true, ch, nullptr, vict, TO_CHAR);
    act(g, "$n focuses ki and flings poison at you! The poison burns into your skin!", true, ch, nullptr, vict, TO_VICT);
    act(g, "$n focuses ki and flings poison at $N! The poison burns into $s skin!", true, ch, nullptr, vict, TO_NOTVICT);
    if (char_is_npc(vict)) {
      set_fighting(g, vict, ch);
    }
    if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 7 || char_get_genome(vict, 1) == 7)) {
      act(g, "However $N seems unaffected by the poison.", true, ch, nullptr, vict, TO_CHAR);
      act(g, "Your natural immunity to poison prevents it from affecting you.", true, ch, nullptr, vict, TO_VICT);
      act(g, "However $N seems unaffected by the poison.", true, ch, nullptr, vict, TO_NOTVICT);
    } else {
      char_set_poisonby(vict, ch);
      if (char_get_charge(vict) > 0) {
        send_to_char(g, vict, "You lose your concentration and release your charged ki!\r\n");
        do_charge(g, vict, "release", 0, 0);
      }
      int duration = char_get_aff_abils_intel(ch) / 20;
      assign_affect(g, vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
    }
    return;
  } // End of POISON

  else {
    send_to_char(g, ch, "What do you want to focus?\r\n");
    return;
  }
}
