/* scouter.c: scouters, SNET and eavesdropping
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <cstdlib>

#include <sys/stat.h>

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

/* local functions */

static bool has_scanner(GameWorld *g, struct char_data *ch);

ACMD(do_telepathy)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg, arg2);

  if (!know_skill(g, ch, SKILL_TELEPATHY)) {
    return;
  }

  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "Lol, no.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Syntax: telepathy [ read ] (target)\r\n"
                     "        telepathy [ link ] (target)\r\n"
                     "        telepathy [  far ] (target)\r\n"
                     "        telepathy (target) (message)\r\n");
    return;
  }

  else if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 40) {
    send_to_char(g, ch, "You do not have enough ki to focus your mental abilities.\r\n");
    return;
  }
  if (!(strcmp(arg, "far"))) {
    if (char_is_npc(ch)) {
      return;
    }
    if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_WORLD))) {
      send_to_char(g, ch, "Look through who's eyes?\r\n");
      return;
    }
    if (vict == ch) {
      send_to_char(g, ch, "Oh that makes a lot of sense...\r\n");
      return;
    }
    if (char_is_npc(vict)) {
      send_to_char(g, ch, "You can't touch the mind of such a thing.\r\n");
      return;
    }
    if (char_get_admlevel(vict) > char_get_admlevel(ch)) {
      send_to_char(g, ch, "Their mental power oustrips your's by unfathomable measurements!\r\n");
      return;
    }
    if (char_aff_flagged(ch, AFF_SHOCKED)) {
      send_to_char(g, ch, "Your mind has been shocked by telepathic feedback! You are not able to use telepathy right now.\r\n");
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, "You can't touch the mind of such an artificial being.\r\n");
      return;
    }
    if (char_get_skill(vict, SKILL_TELEPATHY) + char_get_aff_abils_intel(vict) > char_get_skill(ch, SKILL_TELEPATHY) + char_get_aff_abils_intel(ch)) {
      send_to_char(g, ch, "They throw off your attempt with their own telepathic abilities!\r\n");
      return;
    }
    if (char_get_in_room(ch) == char_get_in_room(vict)) {
      send_to_char(g, ch, "They are in the same room as you!\r\n");
      return;
    }
    if (char_aff_flagged(vict, AFF_BLIND)) {
      send_to_char(g, ch, "They are blind!\r\n");
      return;
    }
    if (char_plr_flagged(vict, PLR_EYEC)) {
      send_to_char(g, ch, "Their eyes are closed!\r\n");
      return;
    }
    look_at_room(g, char_get_in_room(vict), ch, 0);
    send_to_char(g, ch, "You see all this through their eyes!\r\n");
    if (char_get_aff_abils_intel(vict) > char_get_aff_abils_intel(ch)) {
      send_to_char(g, ch, "You feel like someone was using your mind for something...\r\n");
    }
    decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
    return;
  }
  if (!(strcmp(arg, "link"))) {
    if (char_is_npc(ch)) {
      return;
    }
    if (char_get_mindlink(g, ch)) {
      act(g, "@CYou remove the link your mind had with @w$N.@n", true, ch, nullptr, char_get_mindlink(g, ch), TO_CHAR);
      act(g, "@w$n@C removes the link $s mind had with yours.@n", true, ch, nullptr, char_get_mindlink(g, ch), TO_VICT);
      char_set_mindlink(char_get_mindlink(g, ch), nullptr);
      char_set_mindlink(ch, nullptr);
      char_set_linker(ch, 0);
      return;
    }
    if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_WORLD))) {
      send_to_char(g, ch, "Link with the mind of who?\r\n");
      return;
    }
    if (vict == ch) {
      send_to_char(g, ch, "Oh that makes a lot of sense...\r\n");
      return;
    }
    if (char_is_npc(vict)) {
      send_to_char(g, ch, "You can't touch the mind of such a thing.\r\n");
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, "You can't touch the mind of such an artificial being.\r\n");
      return;
    }
    if (char_get_skill(vict, SKILL_TELEPATHY)) {
      send_to_char(g, ch, "Kinda pointless when you are both telepathic huh?\r\n");
      return;
    }
    if (char_get_mindlink(g, vict)) {
      send_to_char(g, ch, "Someone else is already telepathically linked with them.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_TELEPATHY) < axion_dice(g, char_get_aff_abils_intel(vict) * 0.1)) {
      act(g, "@R$n@r tried to link $s mind with yours, but you manage to force a break in the link!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@R$N@r manages to sense the intrusion and with $S intelligence push you out!@n", false, ch, nullptr, vict, TO_CHAR);
      return;
    }
    act(g, "@CYou link your mind with @w$N.@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@w$n@C links $s mind with yours. You can speak your thoughts to $m with 'think'.@n", true, ch, nullptr, vict, TO_VICT);
    send_to_char(g, vict, "@wIf this is undesirable, Try: meditate break@n\r\n");
    char_set_mindlink(vict, ch);
    char_set_mindlink(ch, vict);
    char_set_linker(ch, 1);
    return;
  }
  if (!(strcmp(arg, "read"))) {
    if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Read the mind of who?\r\n");
      return;
    }
    if (vict == ch) {
      send_to_char(g, ch, "Oh that makes a lot of sense...\r\n");
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, "You can't touch the mind of such an artificial being.\r\n");
      return;
    } else {
      if (axion_dice(g, 0) > char_get_skill(ch, SKILL_TELEPATHY)) {
        decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
        act(g, "@wYou attempt to read $N's@w mind, but fail to see it clearly.@n", true, ch, nullptr, vict, TO_CHAR);
        if (rand_number(g, 1, 15) >= 14 && !char_aff_flagged(ch, AFF_SHOCKED)) {
          act(g, "@MYour mind has been shocked!@n", true, ch, nullptr, nullptr, TO_CHAR);
          char_aff_set(ch, AFF_SHOCKED, true);
        } else {
          improve_skill(g, ch, SKILL_TELEPATHY, 0);
        }
        return;
      }
      if (char_get_skill(vict, SKILL_TELEPATHY) >= char_get_skill(ch, SKILL_TELEPATHY) && rand_number(g, 1, 2) == 2) {
        decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
        act(g, "@wYou fail to read @c$N's@w mind and they seemed to have noticed the attempt!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@w attempts to read your mind, but you resist and force $m out!@n", true, ch, nullptr, vict, TO_VICT);
        improve_skill(g, ch, SKILL_TELEPATHY, 0);
        return;
      } else {
        send_to_char(g, ch, "@wYou peer into their mind:\r\n");
        decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
        send_to_char(g, ch, "@GName      @D: @W%s@n\r\n", char_get_name(vict));
        send_to_char(g, ch, "@GRace      @D: @W%s@n\r\n", char_true_race_name(vict));
        send_to_char(g, ch, "@GSensei    @D: @W%s@n\r\n", char_sensei_name(vict));
        send_to_char(g, ch, "@GStr       @D: @W%d@n\r\n", char_get_aff_abils_str(vict));
        send_to_char(g, ch, "@GCon       @D: @W%d@n\r\n", char_get_aff_abils_con(vict));
        send_to_char(g, ch, "@GInt       @D: @W%d@n\r\n", char_get_aff_abils_intel(vict));
        send_to_char(g, ch, "@GWis       @D: @W%d@n\r\n", char_get_aff_abils_wis(vict));
        send_to_char(g, ch, "@GSpd       @D: @W%d@n\r\n", char_get_aff_abils_cha(vict));
        send_to_char(g, ch, "@GAgi       @D: @W%d@n\r\n", char_get_aff_abils_dex(vict));
        send_to_char(g, ch, "@GZenni     @D: @W%s@n\r\n", add_commas(char_get_gold(vict)));
        send_to_char(g, ch, "@GBank Zenni@D: @W%s@n\r\n", add_commas(char_get_bank_gold(vict)));
        if (char_get_alignment(vict) >= 1000) {
          send_to_char(g, ch, "@GAlignment @D: @wSaint         @n\r\n");
        } else if (char_get_alignment(vict) > 750) {
          send_to_char(g, ch, "@GAlignment @D: @wExtremely Good@n\r\n");
        } else if (char_get_alignment(vict) > 500) {
          send_to_char(g, ch, "@GAlignment @D: @wReally Good   @n\r\n");
        } else if (char_get_alignment(vict) > 250) {
          send_to_char(g, ch, "@GAlignment @D: @wGood          @n\r\n");
        } else if (char_get_alignment(vict) > 100) {
          send_to_char(g, ch, "@GAlignment @D: @wPretty Good   @n\r\n");
        } else if (char_get_alignment(vict) > 50) {
          send_to_char(g, ch, "@GAlignment @D: @wSorta Good    @n\r\n");
        } else if (char_get_alignment(vict) > -50) {
          send_to_char(g, ch, "@GAlignment @D: @wNeutral       @n\r\n");
        } else if (char_get_alignment(vict) > -100) {
          send_to_char(g, ch, "@GAlignment @D: @wSorta Evil    @n\r\n");
        } else if (char_get_alignment(vict) > -500) {
          send_to_char(g, ch, "@GAlignment @D: @wPretty Evil   @n\r\n");
        } else if (char_get_alignment(vict) >= -750) {
          send_to_char(g, ch, "@GAlignment @D: @wEvil          @n\r\n");
        } else if (char_get_alignment(vict) < -750) {
          send_to_char(g, ch, "@GAlignment @D: @wExtremely Evil@n\r\n");
        } else if (char_get_alignment(vict) <= -1000) {
          send_to_char(g, ch, "@GAlignment @D: @wDevil         @n\r\n");
        } else {
          send_to_char(g, ch, "@GAlignment @D: @wUnknown       @n\r\n");
        }
      } // End of read success
    } // End of vict is there
    improve_skill(g, ch, SKILL_TELEPATHY, 0);
    return;
  } // End of read argument

  else {
    if (char_get_mindlink(g, ch)) {
      vict = char_get_mindlink(g, ch);
    } else if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      send_to_char(g, ch, "Send your thoughts to who?\r\n");
      return;
    }
    if (vict == ch) {
      send_to_char(g, ch, "Oh that makes a lot of sense...\r\n");
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, "You can't touch the mind of such an artificial being.\r\n");
      return;
    } else {
      if (!char_get_mindlink(g, ch)) {
        send_to_char(g, ch, "@WYou tell @c%s@W telepathically, @w'@C%s@w'@n\r\n", char_get_name(vict), arg2);
        send_to_char(g, vict, "@c%s@W talks to you telepathically, @w'@C%s@w'@n\r\n", char_get_name(ch), arg2);
        send_to_imm(g, "@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), char_get_admlevel(vict) > 0 ? char_get_name(vict) : char_get_user(vict), arg2);
      } else {
        send_to_char(g, ch, "@WYou tell @c%s@W telepathically, @w'@C%s@w'@n\r\n", char_get_name(vict), argument);
        send_to_char(g, vict, "@c%s@W talks to you telepathically, @w'@C%s@w'@n\r\n", char_get_name(ch), argument);
        send_to_imm(g, "@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), char_get_admlevel(vict) > 0 ? char_get_name(vict) : char_get_user(vict), argument);
      }
      decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
    } // End of vict is here
    return;
  } // End of send argument
}

ACMD(do_eavesdrop)
{
  int dir;
  char buf[MAX_INPUT_LENGTH];
  one_argument(argument, buf);

  if (char_get_listenroom(ch) > 0) {
    send_to_char(g, ch, "You stop eavesdropping.\r\n");
    char_set_listenroom(ch, real_room(g, 0));
    char_set_eavesdir(ch, -1);
    return;
  }

  if (!*buf) {
    send_to_char(g, ch, "In which direction would you like to eavesdrop?\r\n");
    return;
  }
  if ((dir = search_block(buf, dirs, false)) < 0) {
    send_to_char(g, ch, "Which directions is that?\r\n");
    return;
  }
  if (!know_skill(g, ch, SKILL_EAVESDROP)) {
    return;
  }
  if (char_get_exit(g, ch, dir)) {
    if (IS_SET(char_get_exit(g, ch, dir)->exit_info, EX_CLOSED) && char_get_exit(g, ch, dir)->keyword) {
      snprintf(buf, sizeof(buf), "The %s is closed.\r\n", fname(char_get_exit(g, ch, dir)->keyword));
      send_to_char(g, ch, "%s", buf);
    } else {
      char_set_listenroom(ch, rroom_get_vnum(g, char_get_exit(g, ch, dir)->to_room));
      char_set_eavesdir(ch, dir);
      send_to_char(g, ch, "Okay.\r\n");
    }
  } else
    send_to_char(g, ch, "There is not a room there...\r\n");
}

void check_eq(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj;
  int i;
  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i)) {
      obj = char_get_equipment(ch, i);
      if (obj_extra_flagged(obj, ITEM_BROKEN)) {
        act(g, "@W$p@W falls apart and you remove it.@n", false, ch, obj, nullptr, TO_CHAR);
        act(g, "@W$p@W falls apart and @C$n@W remove it.@n", false, ch, obj, nullptr, TO_ROOM);
        perform_remove(g, ch, i);
        return;
      }
      if (obj == char_get_equipment(ch, WEAR_WIELD1) && char_get_limb_condition(ch, (1) - 1) <= 0) {
        act(g, "@WWithout your right arm you let go of @c$p@W!@n", false, ch, obj, nullptr, TO_CHAR);
        act(g, "@C$n@W lets go of @c$p@W!@n", false, ch, obj, nullptr, TO_ROOM);
        perform_remove(g, ch, i);
        return;
      }
      if (obj == char_get_equipment(ch, WEAR_WIELD2) && char_get_limb_condition(ch, (2) - 1) <= 0) {
        act(g, "@WWithout your left arm you let go of @c$p@W!@n", false, ch, obj, nullptr, TO_CHAR);
        act(g, "@C$n@W lets go of @c$p@W!@n", false, ch, obj, nullptr, TO_ROOM);
        perform_remove(g, ch, i);
        return;
      }
    }
  }
}

static bool has_scanner(GameWorld *g, struct char_data *ch)
{
  int success = 0;

  for (obj_data *obj : char_inventory(g, ch)) {
    if (obj && obj_get_vnum(g, obj) == 13600) {
      success = 1;
    }
  }

  return (success);
}

ACMD(do_snet)
{
  int channel = 0, global = false, call = -1, reached = false;
  char voice[150], arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char hist[MAX_INPUT_LENGTH];

  half_chop(argument, arg, arg2);

  struct obj_data *obj  = nullptr;
  struct obj_data *obj2 = nullptr;

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_HBTC)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "Lol, no.\r\n");
    return;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_PAST)) {
    send_to_char(g, ch, "This is the past, you can't talk on scouter net!\r\n");
    return;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(g, ch, "The fire eats your transmission!\r\n");
    return;
  }
  if (rroom_get_vnum(g, char_get_in_room(ch)) >= 19800 && rroom_get_vnum(g, char_get_in_room(ch)) <= 19899) {
    send_to_char(g, ch, "Your signal will not be able to escape the walls of the pocket dimension.\r\n");
    return;
  }

  if (char_get_equipment(ch, WEAR_EYE)) {
    obj = char_get_equipment(ch, WEAR_EYE);
  } else {
    send_to_char(g, ch, "You do not have a scouter on.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "[Syntax] snet < [1-999] | check | #(scouter number) | * message | message>\r\n");
    return;
  }

  if (strstr(arg, "#")) {
    search_replace(arg, "#", "");
    call = atoi(arg);
    if (call <= -1) {
      send_to_char(g, ch, "Call what personal scouter number?\r\n");
      return;
    }
  }

  if (!strcasecmp(arg, "check")) {
    send_to_char(g, ch, "Your personal scouter number is: %d\r\n", char_get_id(ch));
    return;
  }

  if (call <= -1) {
    channel = atoi(arg);
  }

  if (channel > 0) {
    obj_set_scoutfreq(obj, channel);
    if (channel > 999) {
      obj_set_scoutfreq(obj, 999);
    }
    act(g, "@wYou push some buttons on $p@w and change its channel.", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@w pushes some buttons on $p@w and changes its channel.", true, ch, obj, nullptr, TO_ROOM);
    return;
  } else {
    if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
      send_to_char(g, ch, "You are unable to speak though.\r\n");
      return;
    }
    if (obj_get_scoutfreq(obj) == 0) {
      obj_set_scoutfreq(obj, 1);
    }
    if (!strcasecmp(arg, "*") && call <= -1) {
      global = true;
    }
    if (char_get_voice(ch) != nullptr) {
      snprintf(voice, sizeof(voice), "%s", char_get_voice(ch));
    }
    if (char_get_voice(ch) == nullptr) {
      snprintf(voice, sizeof(voice), "A generic voice");
    }
    for (descriptor_data *i : each_desc(g)) {
      if (!i->character) {
        continue;
      }
      if (i->character == ch) {
        continue;
      }
      /* It will always be heard and logged for admin. */
      if (char_get_admlevel(i->character) > 0) {
        if (call <= -1 && global == false) {
          snprintf(hist, sizeof(hist), "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, char_get_name(ch), obj_get_scoutfreq(obj), CAP(arg), !*arg2 ? "" : arg2);
        } else if (call <= -1) {
          snprintf(hist, sizeof(hist), "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice, char_get_name(ch), obj_get_scoutfreq(obj), !*arg2 ? "" : CAP(arg2));
        } else {
          snprintf(hist, sizeof(hist), "@C%s (%s) is heard, @D[@WCall to @R#@Y%d@D] @G%s@n\r\n", voice, char_get_name(ch), call, !*arg2 ? "" : CAP(arg2));
        }
        send_to_char(g, i->character, "%s", hist);
        add_history(g, i->character, hist, HIST_SNET);
        continue;
      }
      if (desc_get_connected(i) != CON_PLAYING) {
        continue;
      }
      if (char_get_in_room(i->character) == char_get_in_room(ch)) {
        continue;
      }
      if (rroom_flagged(g, char_get_in_room(i->character), ROOM_HBTC)) {
        continue;
      }
      if (rroom_flagged(g, char_get_in_room(i->character), ROOM_PAST)) {
        continue;
      }
      if ((rroom_flagged(g, char_get_in_room(i->character), ROOM_RHELL) && !rroom_flagged(g, char_get_in_room(ch), ROOM_RHELL)) || (rroom_flagged(g, char_get_in_room(i->character), ROOM_AL) && !rroom_flagged(g, char_get_in_room(ch), ROOM_AL))) {
        continue;
      }
      if ((!rroom_flagged(g, char_get_in_room(i->character), ROOM_RHELL) && rroom_flagged(g, char_get_in_room(ch), ROOM_RHELL)) || (!rroom_flagged(g, char_get_in_room(i->character), ROOM_AL) && rroom_flagged(g, char_get_in_room(ch), ROOM_AL))) {
        continue;
      }
      if (char_get_position(i->character) == POS_SLEEPING) {
        continue;
      }
      if (rroom_get_vnum(g, char_get_in_room(i->character)) >= 19800 && rroom_get_vnum(g, char_get_in_room(i->character)) <= 19899) {
        continue;
      }
      if (char_get_equipment(i->character, WEAR_EYE)) {
        obj2 = char_get_equipment(i->character, WEAR_EYE);
        if (obj_get_scoutfreq(obj2) == 0) {
          obj_set_scoutfreq(obj2, 1);
        }
        if (global == false && call <= -1 && obj_get_scoutfreq(obj2) == obj_get_scoutfreq(obj) && char_get_admlevel(i->character) < 1) {
          send_to_char(g, i->character, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", obj_get_scoutfreq(obj), CAP(arg), !*arg2 ? "" : arg2);
          *hist = '\0';
          snprintf(hist, sizeof(hist), "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", obj_get_scoutfreq(obj), CAP(arg), !*arg2 ? "" : arg2);
          add_history(g, i->character, hist, HIST_SNET);
          if (has_scanner(g, i->character)) {
            char *blah = sense_location(g, ch);
            send_to_char(g, i->character, "@WScanner@D: @Y%s@n\r\n", blah);
            free(blah);
          }
          continue;
        } /* It is the right freq */
        else if (global == true && call <= -1 && char_get_admlevel(i->character) < 1) {
          send_to_char(g, i->character, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", obj_get_scoutfreq(obj), CAP(arg2));
          *hist = '\0';
          snprintf(hist, sizeof(hist), "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", obj_get_scoutfreq(obj), CAP(arg2));
          add_history(g, i->character, hist, HIST_SNET);
          if (has_scanner(g, i->character)) {
            char *blah = sense_location(g, ch);
            send_to_char(g, i->character, "@WScanner@D: @Y%s@n\r\n", blah);
            free(blah);
          }
          continue;
        } else if (call > -1 && char_get_id(i->character) == call) {
          send_to_char(g, i->character, "@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", char_get_id(ch), !*arg2 ? "" : CAP(arg2));
          *hist = '\0';
          snprintf(hist, sizeof(hist), "@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", char_get_id(ch), !*arg2 ? "" : CAP(arg2));
          add_history(g, i->character, hist, HIST_SNET);
          if (has_scanner(g, i->character)) {
            char *blah = sense_location(g, ch);
            send_to_char(g, i->character, "@WScanner@D: @Y%s@n\r\n", blah);
            free(blah);
          }
          reached = true;
        }
      } /* They have a scouter */
    } /* End switch */
    if (call <= -1) {
      if (global == false) {
        reveal_hiding(g, ch, 3);
        send_to_char(g, ch, "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", obj_get_scoutfreq(obj), arg, !*arg2 ? "" : arg2);
        *hist = '\0';
        snprintf(hist, sizeof(hist), "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", obj_get_scoutfreq(obj), arg, !*arg2 ? "" : arg2);
        add_history(g, ch, hist, HIST_SNET);
        char over[MAX_STRING_LENGTH];
        snprintf(over, sizeof(over), "@C$n@W says into $s scouter, '@G@G%s %s@W'@n\r\n", CAP(arg), !*arg2 ? "" : arg2);
        act(g, over, true, ch, nullptr, nullptr, TO_ROOM);
        if (rroom_flagged(g, char_get_in_room(ch), ROOM_RHELL) || rroom_flagged(g, char_get_in_room(ch), ROOM_AL)) {
          send_to_char(g, ch, "@mThe transmission only reaches those who are in the afterlife.@n\r\n");
        }
      }
      if (global == true) {
        reveal_hiding(g, ch, 3);
        send_to_char(g, ch, "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", obj_get_scoutfreq(obj), !*arg2 ? "" : CAP(arg2));
        *hist = '\0';
        snprintf(hist, sizeof(hist), "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", obj_get_scoutfreq(obj), !*arg2 ? "" : CAP(arg2));
        add_history(g, ch, hist, HIST_SNET);
        char over[MAX_STRING_LENGTH];
        snprintf(over, sizeof(over), "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n", !*arg2 ? "" : CAP(arg2));
        act(g, over, true, ch, nullptr, nullptr, TO_ROOM);
        if (rroom_flagged(g, char_get_in_room(ch), ROOM_RHELL) || rroom_flagged(g, char_get_in_room(ch), ROOM_AL)) {
          send_to_char(g, ch, "@mThe transmission only reaches those who are in the afterlife.@n\r\n");
        }
      }
    } else {
      reveal_hiding(g, ch, 3);
      send_to_char(g, ch, "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call, !*arg2 ? "" : CAP(arg2));
      *hist = '\0';
      snprintf(hist, sizeof(hist), "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call, !*arg2 ? "" : CAP(arg2));
      add_history(g, ch, hist, HIST_SNET);
      char over[MAX_STRING_LENGTH];
      snprintf(over, sizeof(over), "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n", !*arg2 ? "" : CAP(arg2));
      act(g, over, true, ch, nullptr, nullptr, TO_ROOM);
      if (reached == false) {
        send_to_char(g, ch, "@mThe transmission didn't reach them.@n\r\n");
      }
    }
  } /* end if statement */
} /*end snet command */

ACMD(do_scouter)
{
  struct char_data *vict = nullptr;
  int count = 0;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
    return;
  }

  struct obj_data *obj = char_get_equipment(ch, WEAR_EYE);
  if (!obj) {
    send_to_char(g, ch, "You do not even have a scouter!");
    obj = nullptr;
    return;
  }
  if (!*arg) {
    send_to_char(g, ch, "[Syntax] scouter < target | scan>\r\n");
    return;
  }
  reveal_hiding(g, ch, 3);
  if (!strcasecmp("scan", arg)) {
    for (descriptor_data *i : each_desc(g)) {
      if (desc_get_connected(i) != CON_PLAYING) {
        continue;
      }
      if (i->character == ch) {
        continue;
      }
      if (char_is_android(i->character)) {
        continue;
      }
      if (planet_check(g, ch, i->character)) {
        int dir  = find_first_step(g, char_get_in_room(ch), char_get_in_room(i->character));
        int same = false;
        char pathway[MAX_STRING_LENGTH];

        if (char_in_zone(g, ch) == char_in_zone(g, i->character))
          same = true;

        switch (dir) {
          case BFS_ERROR:
            snprintf(pathway, sizeof(pathway), "@rERROR");
            break;
          case BFS_ALREADY_THERE:
            snprintf(pathway, sizeof(pathway), "@RHERE");
            break;
          case BFS_NO_PATH:
            send_to_char(g, ch, "@MUNKNOWN");
            break;
          default:
            send_to_char(g, ch, "@G%s\r\n", dirs[dir]);
            break;
        }

        char *blah = sense_location(g, i->character);
        if (obj_extra_flagged(obj, ITEM_BSCOUTER) && getCurPL(g, i->character) >= 150000) {
          send_to_char(g, ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == true ? pathway : blah);
        } else if (obj_extra_flagged(obj, ITEM_MSCOUTER) && getCurPL(g, i->character) >= 5000000) {
          send_to_char(g, ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == true ? pathway : blah);
        } else if (obj_extra_flagged(obj, ITEM_ASCOUTER) && getCurPL(g, i->character) >= 15000000) {
          send_to_char(g, ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == true ? pathway : blah);
        } else {
          send_to_char(g, ch, "@D<@GPowerlevel Detected@D: [@Y%s@D]@w ---> @C%s@n\r\n", add_commas(getCurPL(g, i->character)), same == true ? pathway : blah);
        }
        ++count;
        free(blah);
      }
    }
    if (count == 0) {
      send_to_char(g, ch, "You didn't detect anyone of notice.\r\n");
      return;
    }
    if (count >= 1) {
      send_to_char(g, ch, "%d powerlevels detected.\r\n", count);
      return;
    }
  }
  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "They don't seem to be here.\r\n");
    return;
  }
  if (char_is_android(vict)) {
    act(g, "$n points $s scouter at you.", false, ch, nullptr, vict, TO_VICT);
    act(g, "$n points $s scouter at $N.", false, ch, nullptr, vict, TO_NOTVICT);
    send_to_char(g, ch, "@D,==================================|@n\r\n");
    send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1@RReading target...                 @n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D:                 @RERROR@n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D:                 @RERROR@n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D:                 @RERROR@n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D:                 @RERROR@n@D|@n\r\n");
    send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
    send_to_char(g, ch, "@D`==================================|@n\r\n");
    return;
  }
  if (obj_extra_flagged(obj, ITEM_BSCOUTER) && getCurPL(g, vict) >= 150000) {
    act(g, "$n points $s scouter at you.", false, ch, nullptr, vict, TO_VICT);
    act(g, "$n points $s scouter at $N.", false, ch, nullptr, vict, TO_NOTVICT);
    perform_remove(g, ch, WEAR_EYE);
    send_to_char(g, ch, "Your scouter overloads and explodes!\r\n");
    act(g, "$n's scouter explodes!", false, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, obj);
    save_char(g, ch);
    return;
  }
  if (obj_extra_flagged(obj, ITEM_MSCOUTER) && getCurPL(g, vict) >= 5000000) {
    act(g, "$n points $s scouter at you.", false, ch, nullptr, vict, TO_VICT);
    act(g, "$n points $s scouter at $N.", false, ch, nullptr, vict, TO_NOTVICT);
    perform_remove(g, ch, WEAR_EYE);
    send_to_char(g, ch, "Your scouter overloads and explodes!\r\n");
    act(g, "$n's scouter explodes!", false, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, obj);
    save_char(g, ch);
    return;
  }
  if (obj_extra_flagged(obj, ITEM_ASCOUTER) && getCurPL(g, vict) >= 15000000) {
    act(g, "$n points $s scouter at you.", false, ch, nullptr, vict, TO_VICT);
    act(g, "$n points $s scouter at $N.", false, ch, nullptr, vict, TO_NOTVICT);
    perform_remove(g, ch, WEAR_EYE);
    send_to_char(g, ch, "Your scouter overloads and explodes!\r\n");
    act(g, "$n's scouter explodes!", false, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, obj);
    save_char(g, ch);
    return;
  }
  long double percent = 0.0, cur = 0.0, max = 0.0;
  int64_t stam = (getCurST(g, vict)), mstam = getMaxST(g, vict);
  if (stam <= 0)
    stam = 1;
  if (mstam <= 0)
    mstam = 1;

  cur = (long double)(stam);
  max = (long double)(mstam);

  percent = (cur / max) * 100;
  act(g, "$n points $s scouter at you.", false, ch, nullptr, vict, TO_VICT);
  act(g, "$n points $s scouter at $N.", false, ch, nullptr, vict, TO_NOTVICT);
  send_to_char(g, ch, "@D,==================================|@n\r\n");
  send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
  send_to_char(g, ch, "@D|@1@RReading target...                 @n@D|@n\r\n");
  send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
  send_to_char(g, ch, "@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D: @Y%21s@n@D|@n\r\n", add_commas(getCurPL(g, vict)));
  if (!char_is_npc(vict)) {
    send_to_char(g, ch, "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n\r\n", add_commas(char_get_charge(vict)));
  } else if (char_is_npc(vict)) {
    send_to_char(g, ch, "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n\r\n", add_commas(char_get_mobcharge(vict) * rand_number(g, char_get_level(ch) * 50, char_get_level(ch) * 200)));
  }
  if (percent < 10)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Exhausted");
  else if (percent < 25)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Extremely Tired");
  else if (percent < 50)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Very Tired");
  else if (percent < 75)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Tired");
  else if (percent < 90)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Winded");
  else if (percent < 100)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Untired");
  else if (percent >= 100)
    send_to_char(g, ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Energetic");
  send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
  int check = false;
  send_to_char(g, ch, "@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D: ");
  if (char_aff_flagged(vict, AFF_ZANZOKEN)) {
    send_to_char(g, ch, "@Y%21s@n@D|@n\n", "Zanzoken Prepared");
    check = true;
  }
  if (char_aff_flagged(vict, AFF_HASS)) {
    send_to_char(g, ch, "%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Accelerated Arms");
    check = true;
  }
  if (char_aff_flagged(vict, AFF_HEALGLOW)) {
    send_to_char(g, ch, "%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Healing Glow Prepared");
    check = true;
  }
  if (char_aff_flagged(vict, AFF_POISON)) {
    send_to_char(g, ch, "%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Poisoned");
    check = true;
  }
  if (char_plr_flagged(vict, PLR_SELFD)) {
    send_to_char(g, ch, "%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Explosive Energy");
    check = true;
  }
  if (check == false) {
    send_to_char(g, ch, "%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "None Detected.");
  }
  send_to_char(g, ch, "@D|@1                                  @n@D|@n\r\n");
  send_to_char(g, ch, "@D`==================================|@n\r\n");
}
