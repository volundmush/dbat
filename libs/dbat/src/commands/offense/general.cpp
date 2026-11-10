/* general.c: charge, powerup, rescue, assist, kill, flee
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/consts/deathtype.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/db.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/fight.hpp"
#include "dbat/guild.hpp"
#include "dbat/class.hpp"
#include "dbat/techniques.hpp"
#include "dbat/game_world.hpp"

/* This handles charging for energy attacks */
ACMD(do_charge)
{

  std::string arg;
  int amt;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(g, ch, "@WYou are concentrating too much on your aura to be able to charge.");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_MBREAK)) {
    send_to_char(g, ch, "Your mind is still strained from psychic attacks...\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_POISON)) {
    send_to_char(g, ch, "You feel too sick from the poison to concentrate.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Charge, yes. How much percent though?\r\n");
    send_to_char(g, ch, "[ 1 - 100 | cancel | release]\r\n");
    return;
  }
  if (str_eq_nocase("release", arg) && (char_plr_flagged(ch, PLR_CHARGE) && char_get_charge(ch) <= 0)) {
    send_to_char(g, ch, "Try cancel instead, you have nothing charged up yet!\r\n");
    return;
  }
  if (str_eq_nocase("release", arg) && (char_plr_flagged(ch, PLR_CHARGE) && char_get_charge(ch) > 0)) {
    send_to_char(g, ch, "You stop charging and release your pent up energy.\r\n");
    switch (rand_number(g, 1, 3)) {
      case 1:
        act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      case 2:
        act(g, "$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      case 3:
        act(g, "$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      default:
        act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
    }
    incCurKI(g, ch, char_get_charge(ch), true);
    char_set_charge(ch, 0);
    char_set_chargeto(ch, 0);
    char_plr_set(ch, PLR_CHARGE, false);
    return;
  }
  if (str_eq_nocase("release", arg) && char_get_charge(ch) > 0) {
    send_to_char(g, ch, "You release your pent up energy.\r\n");
    switch (rand_number(g, 1, 3)) {
      case 1:
        act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      case 2:
        act(g, "$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      case 3:
        act(g, "$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      default:
        act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
    }
    incCurKI(g, ch, char_get_charge(ch), true);
    char_set_charge(ch, 0);
    char_set_chargeto(ch, 0);
    return;
  }
  if (str_eq_nocase("cancel", arg) && char_plr_flagged(ch, PLR_CHARGE)) {
    send_to_char(g, ch, "You stop charging.\r\n");
    switch (rand_number(g, 1, 3)) {
      case 1:
        act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      case 2:
        act(g, "$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      case 3:
        act(g, "$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
      default:
        act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
        break;
    }
    char_plr_set(ch, PLR_CHARGE, false);
    char_set_chargeto(ch, 0);
    return;
  }
  if (str_eq_nocase("cancel", arg) && !char_plr_flagged(ch, PLR_CHARGE)) {
    send_to_char(g, ch, "You are not even charging!\r\n");
    return;
  }
  if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 100) {
    send_to_char(g, ch, "You don't even have enough ki!\r\n");
    return;
  }
  if ((amt = parse_int(arg)) > 0) {
    if (amt >= 101) {
      send_to_char(g, ch, "You have set it too high!\r\n");
      return;
    }
    if (char_aff_flagged(ch, AFF_SPIRITCONTROL)) {
      int64_t diff = 0;
      if ((getCurKI(g, ch)) < ((getMaxKI(g, ch) * 0.01) * amt) + 1) {
        diff = (((getMaxKI(g, ch) * 0.01) * amt) + 1) - (getCurKI(g, ch));
      }
      int chance = 15;
      chance -= char_get_skill(ch, SKILL_SPIRITCONTROL) / 10;
      if (chance < 10)
        chance = 10;
      else if (chance > 15)
        chance = 15;

      if (chance > rand_number(g, 1, 100)) {
        send_to_char(g, ch, "The rush of ki that you try to pool temporarily overwhelms you and you lose control!\r\n");
        null_affect(g, ch, AFF_SPIRITCONTROL);
        return;
      }
      int64_t spiritcost = getMaxKI(g, ch) * 0.05;
      if (char_get_skill(ch, SKILL_SPIRITCONTROL) >= 100) {
        spiritcost = getMaxKI(g, ch) * 0.01;
      }
      reveal_hiding(g, ch, 0);
      send_to_char(g, ch, "Your {} colored aura flashes up around you as you instantly take control of the ki you needed!\r\n", aura_types[char_get_aura(ch)]);
      send_to_char(g, ch, "@D[@RCost@D: @r{}@D]@n\r\n", add_commas(spiritcost));
      char bloom[MAX_INPUT_LENGTH];
      snprintf(bloom, sizeof(bloom), "@wA %s aura flashes up brightly around $n@w!@n", aura_types[char_get_aura(ch)]);
      act(g, bloom, true, ch, nullptr, nullptr, TO_ROOM);
      char_set_charge(ch, (((getMaxKI(g, ch) * 0.01) * amt) + 1) - diff);
      decCurKI(g, ch, (((getMaxKI(g, ch) * 0.01) * amt) + 1) - diff + spiritcost, 0);
    } else {
      reveal_hiding(g, ch, 0);
      send_to_char(g, ch, "You begin to charge some energy, as a {} aura begins to burn around you!\r\n", aura_types[char_get_aura(ch)]);
      char bloom[MAX_INPUT_LENGTH];
      snprintf(bloom, sizeof(bloom), "@wA %s aura flashes up brightly around $n@w!@n", aura_types[char_get_aura(ch)]);
      act(g, bloom, true, ch, nullptr, nullptr, TO_ROOM);
      char_set_chargeto(ch, (((getMaxKI(g, ch) * 0.01) * amt) + 1));
      char_set_charge(ch, char_get_charge(ch) + (1));
      char_plr_set(ch, PLR_CHARGE, true);
    }
  } else if (amt < 1 && char_get_in_room(ch) != 1562) {
    send_to_char(g, ch, "You have set it too low!\r\n");
    return;
  } else {
    send_to_char(g, ch, "That is an invalid argument.\r\n");
  }
}

ACMD(do_powerup)
{
  if (char_is_npc(ch)) {
    char_mob_set(ch, MOB_POWERUP, true);
    if (getEffMaxPL(g, ch) < 50000) {
      act(g, "@RYou begin to powerup, and air billows outward around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and air billows outward around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (getEffMaxPL(g, ch) < 500000) {
      act(g, "@RYou begin to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (getEffMaxPL(g, ch) < 5000000) {
      act(g, "@RYou begin to powerup, and torrents of energy crackle around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and torrents of energy crackle around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (getEffMaxPL(g, ch) < 50000000) {
      act(g, "@RYou begin to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (getEffMaxPL(g, ch) < 100000000) {
      act(g, "@RYou begin to powerup, and massive cracks begin to form beneath you!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and massive cracks begin to form beneath $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (getEffMaxPL(g, ch) < 300000000) {
      act(g, "@RYou begin to powerup, and everything around you shudders from the power!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and everything around $m shudders from the power!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else {
      act(g, "@RYou begin to powerup, and the very air around you begins to burn!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n begins to powerup, and the very air around $m begins to burn!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    return;
  }
  if (char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(g, ch, "@WYou are concentrating too much on your aura to be able to power up.");
    return;
  }
  if (char_is_android(ch)) {
    send_to_char(g, ch, "@WYou are an android, you do not powerup.@n");
    return;
  }
  if (char_get_suppression(ch) > 0) {
    send_to_char(g, ch, "@WYou currently have your powerlevel suppressed to {:d} percent.@n", char_get_suppression(ch));
    return;
  }
  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(g, ch, "@WYou stop powering up.@n");
    char_plr_set(ch, PLR_POWERUP, false);
    return;
  }
  if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
    send_to_char(g, ch, "@WYou are already at max!@n");
    return;
  }
  if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 20) {
    send_to_char(g, ch, "@WYou do not have enough ki to powerup!@n");
    return;
  }
  reveal_hiding(g, ch, 0);
  if (getEffMaxPL(g, ch) < 50000) {
    act(g, "@RYou begin to powerup, and air billows outward around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and air billows outward around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else if (getEffMaxPL(g, ch) < 500000) {
    act(g, "@RYou begin to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and loose objects are lifted into the air!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else if (getEffMaxPL(g, ch) < 5000000) {
    act(g, "@RYou begin to powerup, and torrents of energy crackle around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and torrents of energy crackle around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else if (getEffMaxPL(g, ch) < 50000000) {
    act(g, "@RYou begin to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and the entire area begins to shudder!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else if (getEffMaxPL(g, ch) < 100000000) {
    act(g, "@RYou begin to powerup, and massive cracks begin to form beneath you!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and massive cracks begin to form beneath $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else if (getEffMaxPL(g, ch) < 300000000) {
    act(g, "@RYou begin to powerup, and everything around you shudders from the power!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and everything around $m shudders from the power!@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, "@RYou begin to powerup, and the very air around you begins to burn!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n begins to powerup, and the very air around $m begins to burn!@n", true, ch, nullptr, nullptr, TO_ROOM);
  }
  char_plr_set(ch, PLR_POWERUP, true);
  return;
}

ACMD(do_rescue)
{

  std::string arg;
  struct char_data *helpee, *opponent;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty())
    send_to_char(g, ch, "Whom do you wish to rescue?\r\n");
  else if (!(helpee = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM)))
    send_to_char(g, ch, "{}", CONFIG_NOPERSON.c_str());
  else if (helpee == ch)
    send_to_char(g, ch, "You can't help yourself any more than this!\r\n");
  else if (!char_get_fighting(g, helpee))
    send_to_char(g, ch, "They are not fighting anyone!\r\n");
  else if (char_get_fighting(g, ch) && !char_is_npc(ch))
    send_to_char(g, ch, "You are a little too busy fighting for yourself!\r\n");
  else {
    opponent     = char_get_fighting(g, helpee);
    int mobbonus = 0;

    if (char_is_npc(ch)) {
      mobbonus = char_get_speedi(g, ch) * 0.2;
    }
    if (char_get_speedi(g, ch) + mobbonus < char_get_speedi(g, opponent) && rand_number(g, 1, 3) != 3) {
      act(g, "@GYou leap towards @g$N@G and try to rescue $M but are too slow!@n", true, ch, nullptr, helpee, TO_CHAR);
      act(g, "@g$n@G leaps towards you! $n is too slow and fails to rescue you!@n", true, ch, nullptr, helpee, TO_VICT);
      act(g, "@g$n@G leaps towards @g$N@G and tries to rescue $M but is too slow!@n", true, ch, nullptr, helpee, TO_NOTVICT);
      return;
    }
    act(g, "@GYou leap in front of @g$N@G and rescue $M!@n", true, ch, nullptr, helpee, TO_CHAR);
    act(g, "@g$n@G leaps in front of you! You are rescued!@n", true, ch, nullptr, helpee, TO_VICT);
    act(g, "@g$n@G leaps in front of @g$N@G and rescues $M!@n", true, ch, nullptr, helpee, TO_NOTVICT);
    stop_fighting(g, opponent);
    hurt(g, 0, 0, ch, opponent, nullptr, rand_number(g, 1, char_get_level(ch)), 1);
    hurt(g, 0, 0, opponent, ch, nullptr, rand_number(g, 1, char_get_level(ch)), 1);
    return;
  }
}

ACMD(do_assist)
{
  std::string arg;
  struct char_data *helpee, *opponent;

  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty())
    send_to_char(g, ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM)))
    send_to_char(g, ch, "{}", CONFIG_NOPERSON.c_str());
  else if (helpee == ch)
    send_to_char(g, ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (char_get_fighting(g, helpee))
      opponent = char_get_fighting(g, helpee);
    else
      opponent = find_first(room_people(g, char_get_in_room(ch)), [&](char_data *c) { return char_get_fighting(g, c) == helpee; });

    if (!opponent)
      act(g, "But nobody is fighting $M!", true, ch, nullptr, helpee, TO_CHAR);
    else if (!char_can_see(g, ch, opponent))
      act(g, "You can't see who is fighting $M!", true, ch, nullptr, helpee, TO_CHAR);
    /* prevent accidental pkill */
    else {
      reveal_hiding(g, ch, 0);
      send_to_char(g, ch, "You join the fight!\r\n");
      act(g, "$N assists you!", 0, helpee, nullptr, ch, TO_CHAR);
      act(g, "$n assists $N.", true, ch, nullptr, helpee, TO_NOTVICT);
      if (!char_get_fighting(g, ch)) {
        set_fighting(g, ch, opponent);
      }
      if (!char_get_fighting(g, opponent)) {
        set_fighting(g, opponent, ch);
      }
    }
  }
}

ACMD(do_kill)
{
  std::string arg;
  struct char_data *vict;

  if (char_is_npc(ch) || !char_adm_flagged(ch, ADM_INSTANTKILL)) {
    return;
  }
  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM)))
      send_to_char(g, ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(g, ch, "Your mother would be so sad.. :(\r\n");
    else {
      act(g, "You chop $M to pieces!  Ah!  The blood!", true, ch, nullptr, vict, TO_CHAR);
      act(g, "$N chops you to pieces!", true, vict, nullptr, ch, TO_CHAR);
      act(g, "$n brutally slays $N!", true, ch, nullptr, vict, TO_NOTVICT);
      raw_kill(g, vict, ch);
    }
  }
}

ACMD(do_flee)
{
  int i, attempt;
  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (char_get_position(ch) < POS_RESTING) {
    send_to_char(g, ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (char_get_grappling(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return;
  }

  if (char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(g, ch)) {
    send_to_char(g, ch, "You are absorbing from someone!\r\n");
    return;
  }

  if (char_get_absorbby(g, ch)) {
    send_to_char(g, ch, "You are being absorbed from by someone!\r\n");
    return;
  }

  if (!char_is_npc(ch)) {
    int fail = false;
    for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
      if (obj_get_kicharge(obj) > 0 && obj_get_user(g, obj) == ch) {
        fail = true;
      }
    }
    if (fail == true) {
      send_to_char(g, ch, "You are too busy controlling your attack!\r\n");
      return;
    }
  }

  for (i = 0; i < 12; i++) {
    if (!arg.empty()) {
      if ((attempt = search_block(arg, dirs, false) > -1)) {
        attempt = search_block(arg, dirs, false);
      } else if ((attempt = search_block(arg, abbr_dirs, false) > -1)) {
        attempt = search_block(arg, abbr_dirs, false);
      } else {
        attempt = rand_number(g, 0, NUM_OF_DIRS - 1); /* Select a random direction */
      }
    } else {
      attempt = rand_number(g, 0, NUM_OF_DIRS - 1); /* Select a random direction */
    }
    if (char_can_go(g, ch, attempt) &&
        !room_flagged(g, char_get_exit(g, ch, attempt)->to_room, ROOM_DEATH)) {
      act(g, "$n panics, and attempts to flee!", true, ch, nullptr, nullptr, TO_ROOM);
      if (char_is_npc(ch) && room_flagged(g, char_get_exit(g, ch, attempt)->to_room, ROOM_NOMOB)) {
        return;
      }

      for (obj_data *wall : room_contents(g, char_get_in_room(ch))) {
        if (obj_get_vnum(g, wall) == 79) {
          if (obj_get_cost(wall) == attempt) {
            send_to_char(g, ch, "That direction has a glacial wall blocking it.\r\n");
            return;
          }
        }
      }

      if (!block_calc(g, ch)) {
        return;
      }

      if (char_get_absorbing(g, ch)) {
        send_to_char(g, ch, "You are busy absorbing from {}!\r\n", char_get_name(char_get_absorbing(g, ch)));
        return;
      }
      if (char_get_absorbby(g, ch)) {
        if (axion_dice(g, 0) < char_get_skill(char_get_absorbing(g, ch), SKILL_ABSORB)) {
          send_to_char(g, ch, "You are being held by {}, they are absorbing you!\r\n", char_get_name(char_get_absorbby(g, ch)));
          send_to_char(g, char_get_absorbby(g, ch), "{} struggles in your grasp!\r\n", char_get_name(ch));
          char_set_wait(ch, PULSE_2SEC);
          return;
        }
        act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_absorbby(g, ch), nullptr, ch, TO_NOTVICT);
        act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_absorbby(g, ch), nullptr, ch, TO_VICT);
        act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_absorbby(g, ch), nullptr, ch, TO_CHAR);
        char_set_absorbing(char_get_absorbby(g, ch), nullptr);
        char_set_absorbby(ch, nullptr);
      }
      if (do_simple_move(g, ch, attempt, true)) {
        send_to_char(g, ch, "You flee head over heels.\r\n");
        char_set_wait(ch, PULSE_2SEC);
      } else {
        act(g, "$n tries to flee, but can't!", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_wait(ch, PULSE_2SEC);
      }
      return;
    }
  }
  send_to_char(g, ch, "PANIC!  You couldn't escape!\r\n");
}
