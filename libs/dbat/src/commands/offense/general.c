/* general.c: charge, powerup, rescue, assist, kill, flee
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/act.offensive.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/consts/deathtype.h"
#include "dbat/config.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/db.h"
#include "dbat/extract.h"
#include "dbat/act.movement.h"
#include "dbat/interpreter.h"
#include "dbat/combat.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/fight.h"
#include "dbat/guild.h"
#include "dbat/class.h"
#include "dbat/techniques.h"

/* This handles charging for energy attacks */
ACMD(do_charge)
{

  char arg[MAX_INPUT_LENGTH];
  int amt;

  one_argument(argument, arg);

  if (char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "@WYou are concentrating too much on your aura to be able to charge.");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_MBREAK)) {
    send_to_char(ch, "Your mind is still strained from psychic attacks...\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_POISON)) {
    send_to_char(ch, "You feel too sick from the poison to concentrate.\r\n");
    return;
  }
  if (!*arg) {
    send_to_char(ch, "Charge, yes. How much percent though?\r\n");
    send_to_char(ch, "[ 1 - 100 | cancel | release]\r\n");
    return;
  } else if (!strcasecmp("release", arg) && (char_plr_flagged(ch, PLR_CHARGE) && char_get_charge(ch) <= 0)) {
    send_to_char(ch, "Try cancel instead, you have nothing charged up yet!\r\n");
    return;
  } else if (!strcasecmp("release", arg) && (char_plr_flagged(ch, PLR_CHARGE) && char_get_charge(ch) > 0)) {
    send_to_char(ch, "You stop charging and release your pent up energy.\r\n");
    switch (rand_number(1, 3)) {
      case 1:
        act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n@w's aura fades.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n@w's aura flickers brightly before disappearing.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
    }
    incCurKI(ch, char_get_charge(ch), true);
    char_set_charge(ch, 0);
    char_set_chargeto(ch, 0);
    char_plr_set(ch, PLR_CHARGE, false);
    return;
  } else if (!strcasecmp("release", arg) && char_get_charge(ch) > 0) {
    send_to_char(ch, "You release your pent up energy.\r\n");
    switch (rand_number(1, 3)) {
      case 1:
        act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n@w's aura fades.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n@w's aura flickers brightly before disappearing.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
    }
    incCurKI(ch, char_get_charge(ch), true);
    char_set_charge(ch, 0);
    char_set_chargeto(ch, 0);
    return;
  } else if (!strcasecmp("cancel", arg) && char_plr_flagged(ch, PLR_CHARGE)) {
    send_to_char(ch, "You stop charging.\r\n");
    switch (rand_number(1, 3)) {
      case 1:
        act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n@w's aura fades.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n@w's aura flickers brightly before disappearing.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
    }
    char_plr_set(ch, PLR_CHARGE, false);
    char_set_chargeto(ch, 0);
    return;
  } else if (!strcasecmp("cancel", arg) && !char_plr_flagged(ch, PLR_CHARGE)) {
    send_to_char(ch, "You are not even charging!\r\n");
    return;
  } else if ((getCurKI(ch)) < GET_MAX_MANA(ch) / 100) {
    send_to_char(ch, "You don't even have enough ki!\r\n");
    return;
  } else if ((amt = atoi(arg)) > 0) {
    if (amt >= 101) {
      send_to_char(ch, "You have set it too high!\r\n");
      return;
    } else if (char_aff_flagged(ch, AFF_SPIRITCONTROL)) {
      int64_t diff = 0;
      if ((getCurKI(ch)) < ((GET_MAX_MANA(ch) * 0.01) * amt) + 1) {
        diff = (((GET_MAX_MANA(ch) * 0.01) * amt) + 1) - (getCurKI(ch));
      }
      int chance = 15;
      chance -= char_get_skill(ch, SKILL_SPIRITCONTROL) / 10;
      if (chance < 10)
        chance = 10;
      else if (chance > 15)
        chance = 15;

      if (chance > rand_number(1, 100)) {
        send_to_char(ch, "The rush of ki that you try to pool temporarily overwhelms you and you lose control!\r\n");
        null_affect(ch, AFF_SPIRITCONTROL);
        return;
      } else {
        int64_t spiritcost = GET_MAX_MANA(ch) * 0.05;
        if (char_get_skill(ch, SKILL_SPIRITCONTROL) >= 100) {
          spiritcost = GET_MAX_MANA(ch) * 0.01;
        }
        reveal_hiding(ch, 0);
        send_to_char(ch, "Your %s colored aura flashes up around you as you instantly take control of the ki you needed!\r\n", aura_types[char_get_aura(ch)]);
        send_to_char(ch, "@D[@RCost@D: @r%s@D]@n\r\n", add_commas(spiritcost));
        char bloom[MAX_INPUT_LENGTH];
        sprintf(bloom, "@wA %s aura flashes up brightly around $n@w!@n", aura_types[char_get_aura(ch)]);
        act(bloom, TRUE, ch, 0, 0, TO_ROOM);
        char_set_charge(ch, (((GET_MAX_MANA(ch) * 0.01) * amt) + 1) - diff);
        decCurKI(ch, (((GET_MAX_MANA(ch) * 0.01) * amt) + 1) - diff + spiritcost, 0);
      }
    } else {
      reveal_hiding(ch, 0);
      send_to_char(ch, "You begin to charge some energy, as a %s aura begins to burn around you!\r\n", aura_types[char_get_aura(ch)]);
      char bloom[MAX_INPUT_LENGTH];
      sprintf(bloom, "@wA %s aura flashes up brightly around $n@w!@n", aura_types[char_get_aura(ch)]);
      act(bloom, TRUE, ch, 0, 0, TO_ROOM);
      char_set_chargeto(ch, (((GET_MAX_MANA(ch) * 0.01) * amt) + 1));
      char_set_charge(ch, char_get_charge(ch) + (1));
      char_plr_set(ch, PLR_CHARGE, true);
    }
  } else if (amt < 1 && rroom_get_vnum(char_get_in_room(ch)) != 1562) {
    send_to_char(ch, "You have set it too low!\r\n");
    return;
  } else {
    send_to_char(ch, "That is an invalid argument.\r\n");
  }
}

ACMD(do_powerup)
{
  if (char_is_npc(ch)) {
    char_mob_set(ch, MOB_POWERUP, true);
    if (GET_MAX_HIT(ch) < 50000) {
      act("@RYou begin to powerup, and air billows outward around you!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and air billows outward around $m!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 500000) {
      act("@RYou begin to powerup, and loose objects are lifted into the air!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and loose objects are lifted into the air!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 5000000) {
      act("@RYou begin to powerup, and torrents of energy crackle around you!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and torrents of energy crackle around $m!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 50000000) {
      act("@RYou begin to powerup, and the entire area begins to shudder!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the entire area begins to shudder!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 100000000) {
      act("@RYou begin to powerup, and massive cracks begin to form beneath you!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and massive cracks begin to form beneath $m!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 300000000) {
      act("@RYou begin to powerup, and everything around you shudders from the power!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and everything around $m shudders from the power!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else {
      act("@RYou begin to powerup, and the very air around you begins to burn!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the very air around $m begins to burn!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
    return;
  }
  if (char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "@WYou are concentrating too much on your aura to be able to power up.");
    return;
  }
  if (char_is_android(ch)) {
    send_to_char(ch, "@WYou are an android, you do not powerup.@n");
    return;
  }
  if (char_get_suppression(ch) > 0) {
    send_to_char(ch, "@WYou currently have your powerlevel suppressed to %" I64T " percent.@n", char_get_suppression(ch));
    return;
  }
  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(ch, "@WYou stop powering up.@n");
    char_plr_set(ch, PLR_POWERUP, false);
    return;
  }
  if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
    send_to_char(ch, "@WYou are already at max!@n");
    return;
  }
  if ((getCurKI(ch)) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "@WYou do not have enough ki to powerup!@n");
    return;
  } else {
    reveal_hiding(ch, 0);
    if (GET_MAX_HIT(ch) < 50000) {
      act("@RYou begin to powerup, and air billows outward around you!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and air billows outward around $m!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 500000) {
      act("@RYou begin to powerup, and loose objects are lifted into the air!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and loose objects are lifted into the air!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 5000000) {
      act("@RYou begin to powerup, and torrents of energy crackle around you!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and torrents of energy crackle around $m!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 50000000) {
      act("@RYou begin to powerup, and the entire area begins to shudder!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the entire area begins to shudder!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 100000000) {
      act("@RYou begin to powerup, and massive cracks begin to form beneath you!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and massive cracks begin to form beneath $m!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 300000000) {
      act("@RYou begin to powerup, and everything around you shudders from the power!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and everything around $m shudders from the power!@n", TRUE, ch, 0, 0, TO_ROOM);
    } else {
      act("@RYou begin to powerup, and the very air around you begins to burn!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the very air around $m begins to burn!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
    char_plr_set(ch, PLR_POWERUP, true);
    return;
  }
}

ACMD(do_rescue)
{

  char arg[100];
  struct char_data *helpee, *opponent;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to rescue?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else if (!char_get_fighting(helpee))
    send_to_char(ch, "They are not fighting anyone!\r\n");
  else if (char_get_fighting(ch) && !char_is_npc(ch))
    send_to_char(ch, "You are a little too busy fighting for yourself!\r\n");
  else {
    opponent     = char_get_fighting(helpee);
    int mobbonus = 0;

    if (char_is_npc(ch)) {
      mobbonus = char_get_speedi(ch) * 0.2;
    }
    if (char_get_speedi(ch) + mobbonus < char_get_speedi(opponent) && rand_number(1, 3) != 3) {
      act("@GYou leap towards @g$N@G and try to rescue $M but are too slow!@n", TRUE, ch, 0, helpee, TO_CHAR);
      act("@g$n@G leaps towards you! $n is too slow and fails to rescue you!@n", TRUE, ch, 0, helpee, TO_VICT);
      act("@g$n@G leaps towards @g$N@G and tries to rescue $M but is too slow!@n", TRUE, ch, 0, helpee, TO_NOTVICT);
      return;
    }
    act("@GYou leap in front of @g$N@G and rescue $M!@n", TRUE, ch, 0, helpee, TO_CHAR);
    act("@g$n@G leaps in front of you! You are rescued!@n", TRUE, ch, 0, helpee, TO_VICT);
    act("@g$n@G leaps in front of @g$N@G and rescues $M!@n", TRUE, ch, 0, helpee, TO_NOTVICT);
    stop_fighting(opponent);
    hurt(0, 0, ch, opponent, NULL, rand_number(1, char_get_level(ch)), 1);
    hurt(0, 0, opponent, ch, NULL, rand_number(1, char_get_level(ch)), 1);
    return;
  }
}

ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (char_get_fighting(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (char_get_fighting(helpee))
      opponent = char_get_fighting(helpee);
    else
      for (opponent = world[char_get_in_room(ch)].people;
           opponent && (char_get_fighting(opponent) != helpee);
           opponent = opponent->next_in_room);

    if (!opponent)
      act("But nobody is fighting $M!", TRUE, ch, 0, helpee, TO_CHAR);
    else if (!char_can_see(ch, opponent))
      act("You can't see who is fighting $M!", TRUE, ch, 0, helpee, TO_CHAR);
    /* prevent accidental pkill */
    else {
      reveal_hiding(ch, 0);
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", TRUE, ch, 0, helpee, TO_NOTVICT);
      if (!char_get_fighting(ch)) {
        set_fighting(ch, opponent);
      }
      if (!char_get_fighting(opponent)) {
        set_fighting(opponent, ch);
      }
    }
  }
}

ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (char_is_npc(ch) || !char_adm_flagged(ch, ADM_INSTANTKILL)) {
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
    else {
      act("You chop $M to pieces!  Ah!  The blood!", TRUE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", TRUE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", TRUE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict, ch);
    }
  }
}

ACMD(do_flee)
{
  int i, attempt;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_get_position(ch) < POS_RESTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (char_get_grappling(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return;
  }

  if (char_get_grappled(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(ch)) {
    send_to_char(ch, "You are absorbing from someone!\r\n");
    return;
  }

  if (char_get_absorbby(ch)) {
    send_to_char(ch, "You are being absorbed from by someone!\r\n");
    return;
  }

  if (!char_is_npc(ch)) {
    int fail = FALSE;
    struct obj_data *obj, *next_obj;
    for (obj = world[char_get_in_room(ch)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (obj_get_kicharge(obj) > 0 && obj_get_user(obj) == ch) {
        fail = TRUE;
      }
    }
    if (fail == TRUE) {
      send_to_char(ch, "You are too busy controlling your attack!\r\n");
      return;
    }
  }

  for (i = 0; i < 12; i++) {
    if (*arg) {
      if ((attempt = search_block(arg, dirs, FALSE) > -1)) {
        attempt = search_block(arg, dirs, FALSE);
      } else if ((attempt = search_block(arg, abbr_dirs, FALSE) > -1)) {
        attempt = search_block(arg, abbr_dirs, FALSE);
      } else {
        attempt = rand_number(0, NUM_OF_DIRS - 1); /* Select a random direction */
      }
    }
    if (!*arg) {
      attempt = rand_number(0, NUM_OF_DIRS - 1); /* Select a random direction */
    }
    if (char_can_go(ch, attempt) &&
        !rroom_flagged(char_get_exit(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (char_is_npc(ch) && rroom_flagged(char_get_exit(ch, attempt)->to_room, ROOM_NOMOB)) {
        return;
      }

      struct obj_data *wall;
      for (wall = world[char_get_in_room(ch)].contents; wall; wall = wall->next_content) {
        if (obj_get_vnum(wall) == 79) {
          if (obj_get_cost(wall) == attempt) {
            send_to_char(ch, "That direction has a glacial wall blocking it.\r\n");
            return;
          }
        }
      }

      if (!block_calc(ch)) {
        return;
      }

      if (char_get_absorbing(ch)) {
        send_to_char(ch, "You are busy absorbing from %s!\r\n", char_get_name(char_get_absorbing(ch)));
        return;
      }
      if (char_get_absorbby(ch)) {
        if (axion_dice(0) < char_get_skill(char_get_absorbing(ch), SKILL_ABSORB)) {
          send_to_char(ch, "You are being held by %s, they are absorbing you!\r\n", char_get_name(char_get_absorbby(ch)));
          send_to_char(char_get_absorbby(ch), "%s struggles in your grasp!\r\n", char_get_name(ch));
          WAIT_STATE(ch, PULSE_2SEC);
          return;
        } else {
          act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE, char_get_absorbby(ch), 0, ch, TO_NOTVICT);
          act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE, char_get_absorbby(ch), 0, ch, TO_VICT);
          act("@c$N@W manages to break loose of your hold!@n", TRUE, char_get_absorbby(ch), 0, ch, TO_CHAR);
          char_set_absorbing(char_get_absorbby(ch), NULL);
          char_set_absorbby(ch, NULL);
        }
      }
      if (do_simple_move(ch, attempt, TRUE)) {
        send_to_char(ch, "You flee head over heels.\r\n");
        WAIT_STATE(ch, PULSE_2SEC);
      } else {
        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}
