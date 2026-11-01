/* player.c: general player commands (quit, save, group, title, toggles...)
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
#include "dbat/local_limits.h"
#include "other_internal.h"

static int perform_group(struct char_data *ch, struct char_data *vict, int highlvl, int lowlvl, int64_t highpl, int64_t lowpl);
static void print_group(struct char_data *ch);

ACMD(do_spar)
{
  if (char_is_npc(ch)) {
    return;
  } else {
    if (char_plr_flagged(ch, PLR_SPAR)) {
      act("@wYou cease your sparring stance.@n", FALSE, ch, 0, 0, TO_CHAR);
      act("@C$n@w ceases $s sparring stance.@n", FALSE, ch, 0, 0, TO_ROOM);
    }
    if (!char_plr_flagged(ch, PLR_SPAR)) {
      act("@wYou move into your sparring stance.@n", FALSE, ch, 0, 0, TO_CHAR);
      act("@C$n@w moves into $s sparring stance.@n", FALSE, ch, 0, 0, TO_ROOM);
    }
    char_plr_toggle(ch, PLR_SPAR);
  }
}

ACMD(do_quit)
{
  if (char_is_npc(ch) || !ch->desc)
    return;

  if (rroom_flagged(char_get_in_room(ch), ROOM_PAST)) {
    send_to_char(ch, "This is the past, you can't quit here!\r\n");
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) >= 2002 && rroom_get_vnum(char_get_in_room(ch)) <= 2011) {
    send_to_char(ch, "You can't quit in the arena!\r\n");
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) >= 101 && rroom_get_vnum(char_get_in_room(ch)) <= 139) {
    send_to_char(ch, "You can't quit in the mud school!\r\n");
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
    send_to_char(ch, "You can't quit in a pocket dimension!\r\n");
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) == 2069) {
    send_to_char(ch, "You can't quit here!\r\n");
    return;
  }
  if (char_get_mindlink(ch) && char_get_linker(ch) == 0) {
    send_to_char(ch, "@RYou feel like the mind that is linked with yours is preventing you from quiting!@n\r\n");
    if (char_get_in_room(char_get_mindlink(ch)) != NOWHERE) {
      look_at_room(char_get_in_room(char_get_mindlink(ch)), ch, 0);
      send_to_char(ch, "You get an impression of where this interference is originating from.\r\n");
    }
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) == 2070) {
    send_to_char(ch, "You can't quit here!\r\n");
    return;
  }
  if (dball_count(ch)) {
    send_to_char(ch, "You can not quit while you have dragon balls! Place them somewhere first.");
    return;
  }

  if (subcmd != SCMD_QUIT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (char_get_position(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (char_get_position(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s has quit the game.", char_get_name(ch));
    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");
    if (ch->followers || ch->master)
      die_follower(ch);
    if (ch == ch_selling)
      stop_auction(AUC_QUIT_CANCEL, NULL);

    /*  We used to check here for duping attempts, but we may as well
     *  do it right in extract_char(), since there is no check if a
     *  player rents out and it can leave them in an equally screwy
     *  situation.
     */

    /* If someone is quitting in their house, let them load back here. */
    if (!rroom_flagged(char_get_in_room(ch), ROOM_PAST) && (rroom_get_vnum(char_get_in_room(ch)) < 19800 || rroom_get_vnum(char_get_in_room(ch)) > 19899)) {
      if (rroom_get_vnum(char_get_in_room(ch)) != NOWHERE && rroom_get_vnum(char_get_in_room(ch)) != 0 && rroom_get_vnum(char_get_in_room(ch)) != 1) {
        char_set_load_room(ch, rroom_get_vnum(char_get_in_room(ch)));
      }
    }
    if (rroom_flagged(char_get_in_room(ch), ROOM_PAST)) {
      if (rroom_get_vnum(char_get_in_room(ch)) != NOWHERE && rroom_get_vnum(char_get_in_room(ch)) != 0 && rroom_get_vnum(char_get_in_room(ch)) != 1) {
        char_set_load_room(ch, rroom_get_vnum(real_room(1561)));
      }
    }

    Crash_rentsave(ch, 0);

    extract_char(ch); /* Char is saved before extracting. */
  }
  /* Remove any snoopers */
  if (ch->desc->snoop_by) {
    write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by           = NULL;
  }
}

ACMD(do_save)
{
  if (char_is_npc(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (CONFIG_AUTO_SAVE && char_get_admlevel(ch) < 1) {
      send_to_char(ch, "Saving.\r\n");
      write_aliases(ch);
      save_char(ch);
      Crash_crashsave(ch);
      if (rroom_get_vnum(char_get_in_room(ch)) < 19800 || rroom_get_vnum(char_get_in_room(ch)) > 19899) {
        if (rroom_get_vnum(char_get_in_room(ch)) != NOWHERE && rroom_get_vnum(char_get_in_room(ch)) != 0 && rroom_get_vnum(char_get_in_room(ch)) != 1) {
          char_set_load_room(ch, rroom_get_vnum(char_get_in_room(ch)));
        }
      }
      return;
    }
    send_to_char(ch, "Saving.\r\n");
  }

  write_aliases(ch);
  if (rroom_get_vnum(char_get_in_room(ch)) < 19800 || rroom_get_vnum(char_get_in_room(ch)) > 19899) {
    if (rroom_get_vnum(char_get_in_room(ch)) != NOWHERE && rroom_get_vnum(char_get_in_room(ch)) != 0 && rroom_get_vnum(char_get_in_room(ch)) != 1) {
      char_set_load_room(ch, rroom_get_vnum(char_get_in_room(ch)));
    }
  }
  save_char(ch);
  Crash_crashsave(ch);
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_practice)
{
  char arg[200];

  /* if (IS_NPC(ch))
    return; */

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills with your trainer.\r\n");
  else
    send_to_char(ch, "Use the skills command unless you are at your trainer.\r\n");
  /*list_skills(ch);*/
}

ACMD(do_skills)
{
  char arg[1000];
  one_argument(argument, arg);
  if (char_is_npc(ch)) {
    return;
  }
  list_skills(ch, arg);
}

ACMD(do_visible)
{
  int appeared = 0;

  if (char_get_admlevel(ch)) {
    perform_immort_vis(ch);
    return;
  }

  if (char_aff_flagged(ch, AFF_INVISIBLE)) {
    appear(ch);
    appeared = 1;
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  }

  if (!appeared)
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (char_is_npc(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (char_plr_flagged(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s %s.\r\n", char_get_name(ch), char_get_title(ch));
  }
}

static int perform_group(struct char_data *ch, struct char_data *vict, int highlvl, int lowlvl, int64_t highpl, int64_t lowpl)
{
  if (char_aff_flagged(vict, AFF_GROUP) || !char_can_see(ch, vict))
    return (0);

  if (char_get_bonuses(vict, BONUS_LONER) > 0) {
    act("$n is the loner type and refuses to be in your group.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
  }

  /*
  if (GET_LEVEL(vict) + 12 < highlvl) {
   act("$n isn't experienced enough to be in your group with its current members.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (GET_LEVEL(vict) > lowlvl + 12) {
   act("$n is too experienced to be in your group with its current members.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (highlvl >= 100) {
   if ((getEffMaxPL(vict))) > highpl * 1.5) {
    act("$n is too powerful right now to be in a level 100 group with you.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
   }

   if ((getEffMaxPL(vict))) < lowpl * 0.5) {
    act("$n is too weak right now to be in a level 100 group with you.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
   }
  }
  */

  char_aff_set(vict, AFF_GROUP, true);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}

static void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!char_aff_flagged(ch, AFF_GROUP))
    send_to_char(ch, "But you are not the member of a group!\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "Your group consists of:\r\n");

    k = (ch->master ? ch->master : ch);

    if (char_aff_flagged(k, AFF_GROUP)) {
      send_to_char(ch, "@D----------------@n\r\n");
      if (getCurPL(k) > getEffMaxPL(k) / 10) {
        snprintf(buf, sizeof(buf), "@gL@D: @w$N @W- @D[@RPL@Y: @c%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
                 add_commas(getCurPL(k)), add_commas((getCurKI(k))), add_commas((getCurST(k))), char_get_level(k),
                 char_class_abbr(k), char_race_abbr(k));
      }
      if (getCurPL(k) <= (getEffMaxPL(k) - (getCurCarriedWeight(k))) / 10) {
        snprintf(buf, sizeof(buf), "@gL@D: @w$N @W- @D[@RPL@Y: @r%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
                 add_commas(getCurPL(k)), add_commas((getCurKI(k))), add_commas((getCurST(k))), char_get_level(k),
                 char_class_abbr(k), char_race_abbr(k));
      }
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!char_aff_flagged(f->follower, AFF_GROUP))
        continue;
      send_to_char(ch, "@D----------------@n\r\n");
      if (getCurPL(f->follower) > (getEffMaxPL(f->follower) - (getCurCarriedWeight(f->follower))) / 10) {
        snprintf(buf, sizeof(buf), "@gF@D: @w$N @W- @D[@RPL@Y: @c%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]",
                 add_commas(getCurPL(f->follower)), add_commas((getCurKI(f->follower))), add_commas((getCurST(f->follower))),
                 char_get_level(f->follower), char_class_abbr(f->follower), char_race_abbr(f->follower));
      }
      if (getCurPL(f->follower) <= (getEffMaxPL(f->follower) - (getCurCarriedWeight(f->follower))) / 10) {
        snprintf(buf, sizeof(buf), "@gF@D: @w$N @W- @D[@RPL@Y: @r%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]",
                 add_commas(getCurPL(f->follower)), add_commas((getCurKI(f->follower))), add_commas((getCurST(f->follower))),
                 char_get_level(f->follower), char_class_abbr(f->follower), char_race_abbr(f->follower));
      }
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
    send_to_char(ch, "@D----------------@n\r\n");
  }
}

ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found, highlvl = 0, lowlvl = 0;
  int64_t highpl = 0, lowpl = 0;

  one_argument(argument, buf);

  if (char_get_bonuses(ch, BONUS_LONER) > 0) {
    send_to_char(ch, "You can not group as you prefer to be alone.\r\n");
    return;
  }

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You cannot enroll group members without being head of a group.",
        FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  highlvl = char_get_level(ch);
  lowlvl  = char_get_level(ch);
  highpl  = getEffMaxPL(ch);
  lowpl   = getEffMaxPL(ch);

  for (found = 0, f = ch->followers; f; f = f->next) {
    if (char_aff_flagged(f->follower, AFF_GROUP)) {
      if (char_get_level(f->follower) > highlvl) {
        highlvl = char_get_level(f->follower);
      }
      if (char_get_level(f->follower) < lowlvl) {
        lowlvl = char_get_level(f->follower);
      }
    }
  }

  int foundwas = 0;

  if (!strcasecmp(buf, "all")) {
    perform_group(ch, ch, char_get_level(ch), char_get_level(ch), highpl, lowpl);
    for (found = 0, f = ch->followers; f; f = f->next) {
      foundwas = found;
      found += perform_group(ch, f->follower, highlvl, lowlvl, highpl, lowpl);
      if (found > foundwas) {
        if (char_get_level(f->follower) > highlvl)
          highlvl = char_get_level(f->follower);
        else if (char_get_level(f->follower) < lowlvl)
          lowlvl = char_get_level(f->follower);
      }
    }
    if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!char_aff_flagged(vict, AFF_GROUP)) {
      if (!char_aff_flagged(ch, AFF_GROUP)) {
        send_to_char(ch, "You form a group, with you as leader.\r\n");
        char_aff_set(ch, AFF_GROUP, true);
      }
      perform_group(ch, vict, highlvl, lowlvl, highpl, lowpl);
    } else {
      if (ch != vict)
        act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      char_aff_set(vict, AFF_GROUP, false);
    }
  }
}

ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(char_aff_flagged(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (char_aff_flagged(f->follower, AFF_GROUP)) {
        char_aff_set(f->follower, AFF_GROUP, false);
        act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
        char_set_combatexpertise(f->follower, 0);
        if (!char_aff_flagged(f->follower, AFF_CHARM))
          stop_follower(f->follower);
      }
    }

    char_aff_set(ch, AFF_GROUP, false);
    char_set_combatexpertise(ch, 0);
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }

  if (!char_aff_flagged(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  char_aff_set(tch, AFF_GROUP, false);
  char_set_combatexpertise(tch, 0);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

  if (!char_aff_flagged(tch, AFF_CHARM))
    stop_follower(tch);
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (char_is_npc(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > char_get_gold(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    char_set_gold(ch, char_get_gold(ch) - (amount));
    k = (ch->master ? ch->master : ch);

    if (char_aff_flagged(k, AFF_GROUP) && (char_get_in_room(k) == char_get_in_room(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (char_aff_flagged(f->follower, AFF_GROUP) &&
          (!char_is_npc(f->follower)) && f->follower != ch &&
          (char_get_in_room(f->follower) == char_get_in_room(ch)))
        num++;

    if (num > 0 && char_aff_flagged(ch, AFF_GROUP)) {
      share = amount / num;
      rest  = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    char_set_gold(ch, char_get_gold(ch) + (share));

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d zenni; you receive %d.\r\n",
                   char_get_name(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
               "%d zenni %s not splitable, so %s keeps the money.\r\n", rest, (rest == 1) ? "was" : "were", char_get_name(ch));
    }
    if (char_aff_flagged(k, AFF_GROUP) && char_get_in_room(k) == char_get_in_room(ch) &&
        !char_is_npc(k) && k != ch) {
      char_set_gold(k, char_get_gold(k) + (share));
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (char_aff_flagged(f->follower, AFF_GROUP) &&
          (!char_is_npc(f->follower)) &&
          (char_get_in_room(f->follower) == char_get_in_room(ch)) &&
          f->follower != ch) {

        char_set_gold(f->follower, char_get_gold(f->follower) + (share));
        send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d zenni among %d members -- %d zenni each.\r\n",
                 amount, num, share);

    if (rest) {
      send_to_char(ch, "%d zenni %s not splitable, so you keep the money.\r\n",
                   rest, (rest == 1) ? "was" : "were");
      char_set_gold(ch, char_get_gold(ch) + (rest));
    }
  } else {
    send_to_char(ch, "How much zenni do you wish to split with your group?\r\n");
    return;
  }
}

ACMD(do_display)
{
  size_t i;

  if (char_is_npc(ch)) {
    send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { P | K | T | S | F | H | G | L | C | M | all/on | none/off }\r\n");
    return;
  }

  if (!strcasecmp(argument, "on") || !strcasecmp(argument, "all")) {
    char_prf_set(ch, PRF_DISPHP, true);
    char_prf_set(ch, PRF_DISPMOVE, true);
    char_prf_set(ch, PRF_DISPKI, true);
    char_prf_set(ch, PRF_DISPTNL, true);
    char_prf_set(ch, PRF_FURY, true);
    char_prf_set(ch, PRF_DISTIME, true);
    char_prf_set(ch, PRF_DISGOLD, true);
    char_prf_set(ch, PRF_DISPRAC, true);
    char_prf_set(ch, PRF_DISHUTH, true);
    char_prf_set(ch, PRF_DISPERC, true);
  } else if (!strcasecmp(argument, "off") || !strcasecmp(argument, "none")) {
    char_prf_set(ch, PRF_DISPHP, false);
    char_prf_set(ch, PRF_DISPKI, false);
    char_prf_set(ch, PRF_DISPMOVE, false);
    char_prf_set(ch, PRF_DISPTNL, false);
    char_prf_set(ch, PRF_FURY, false);
    char_prf_set(ch, PRF_DISTIME, false);
    char_prf_set(ch, PRF_DISGOLD, false);
    char_prf_set(ch, PRF_DISPRAC, false);
    char_prf_set(ch, PRF_DISHUTH, false);
    char_prf_set(ch, PRF_DISPERC, false);
  } else {
    /*char_prf_set(ch, PRF_DISPHP, false);
    char_prf_set(ch, PRF_DISPMOVE, false);
    char_prf_set(ch, PRF_DISPTNL, false);
    char_prf_set(ch, PRF_DISPKI, false);*/

    for (i = 0; i < strlen(argument); i++) {
      switch (tolower(argument[i])) {
        case 'p':
          char_prf_toggle(ch, PRF_DISPHP);
          break;
        case 's':
          char_prf_toggle(ch, PRF_DISPMOVE);
          break;
        case 'k':
          char_prf_toggle(ch, PRF_DISPKI);
          break;
        case 't':
          char_prf_toggle(ch, PRF_DISPTNL);
          break;
        case 'h':
          char_prf_toggle(ch, PRF_DISTIME);
          break;
        case 'g':
          char_prf_toggle(ch, PRF_DISGOLD);
          break;
        case 'l':
          char_prf_toggle(ch, PRF_DISPRAC);
          break;
        case 'c':
          char_prf_toggle(ch, PRF_DISPERC);
          break;
        case 'm':
          char_prf_toggle(ch, PRF_DISHUTH);
          break;
        case 'f':
          if (!char_is_halfbreed(ch)) {
            send_to_char(ch, "Only halfbreeds use fury.\r\n");
          }
          char_prf_toggle(ch, PRF_FURY);
          break;
        default:
          send_to_char(ch, "Usage: prompt { P | K | T | S | F | H | G | L | all/on | none/off }\r\n");
          return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp;
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
    case SCMD_BUG:
      filename = BUG_FILE;
      break;
    case SCMD_TYPO:
      filename = TYPO_FILE;
      break;
    case SCMD_IDEA:
      filename = IDEA_FILE;
      break;
    default:
      return;
  }

  ct  = time(0);
  tmp = asctime(localtime(&ct));

  if (char_is_npc(ch)) {
    send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "That must be a mistake...\r\n");
    return;
  }
  send_to_imm("[A new %s has been filed by: %s]\r\n", CMD_NAME, char_get_name(ch));

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
     *  This is from do_gen_write() and indicates that it cannot call the
     *  stat() system call on the file required.  The error string at the
     *  end of the line should explain what the problem is.
     */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
    send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    /*  SYSERR_DESC:
     *  This is from do_gen_write(), and will be output if the file in
     *  question cannot be opened for appending to.  The error string
     *  at the end of the line should explain what the problem is.
     */

    send_to_char(ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  fprintf(fl, "@D[@WUser: @c%-10s@D] [@WChar: @C%-10s@D] [@WRoom: @G%-4d@D] [@WDate: @Y%6.6s@D]@b \n-----------@w\n%s\n", char_get_user(ch) ? char_get_user(ch) : "ERR", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)), (tmp + 4), argument);
  fprintf(fl, "@D-------------------------------@n\n");
  fclose(fl);
  send_to_char(ch, "Okay.  Thanks!\r\n");
}

#define TOG_OFF 0
#define TOG_ON  1

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
      {"You are now safe from summoning by other players.\r\n",
       "You may now be summoned by other players.\r\n"                               },
      {"Nohassle disabled.\r\n",
       "Nohassle enabled.\r\n"                                                       },
      {"Brief mode off.\r\n",
       "Brief mode on.\r\n"                                                          },
      {"Compact mode off.\r\n",
       "Compact mode on.\r\n"                                                        },
      {"You can now hear tells.\r\n",
       "You are now deaf to tells.\r\n"                                              },
      {"You can now hear newbie.\r\n",
       "You are now deaf to newbie.\r\n"                                             },
      {"You can now hear shouts.\r\n",
       "You are now deaf to shouts.\r\n"                                             },
      {"You can now hear ooc.\r\n",
       "You are now deaf to ooc.\r\n"                                                },
      {"You can now hear the congratulation messages.\r\n",
       "You are now deaf to the congratulation messages.\r\n"                        },
      {"You can now hear the Wiz-channel.\r\n",
       "You are now deaf to the Wiz-channel.\r\n"                                    },
      {"You are no longer part of the Quest.\r\n",
       "Okay, you are part of the Quest!\r\n"                                        },
      {"You will no longer see the room flags.\r\n",
       "You will now see the room flags.\r\n"                                        },
      {"You will now have your communication repeated.\r\n",
       "You will no longer have your communication repeated.\r\n"                    },
      {"HolyLight mode off.\r\n",
       "HolyLight mode on.\r\n"                                                      },
      {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
       "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
      {"Autoexits disabled.\r\n",
       "Autoexits enabled.\r\n"                                                      },
      {"Will no longer track through doors.\r\n",
       "Will now track through doors.\r\n"                                           },
      {"Buildwalk Off.\r\n",
       "Buildwalk On.\r\n"                                                           },
      {"AFK flag is now off.\r\n",
       "AFK flag is now on.\r\n"                                                     },
      {"You will no longer Auto-Assist.\r\n",
       "You will now Auto-Assist.\r\n"                                               },
      {"Autoloot disabled.\r\n",
       "Autoloot enabled.\r\n"                                                       },
      {"Autogold disabled.\r\n",
       "Autogold enabled.\r\n"                                                       },
      {"Will no longer clear screen in OLC.\r\n",
       "Will now clear screen in OLC.\r\n"                                           },
      {"Autosplit disabled.\r\n",
       "Autosplit enabled.\r\n"                                                      },
      {"Autosac disabled.\r\n",
       "Autosac enabled.\r\n"                                                        },
      {"You will no longer attempt to be sneaky.\r\n",
       "You will try to move as silently as you can.\r\n"                            },
      {"You will no longer attempt to stay hidden.\r\n",
       "You will try to stay hidden.\r\n"                                            },
      {"You will no longer automatically memorize spells in your list.\r\n",
       "You will automatically memorize spells in your list.\r\n"                    },
      {"Viewing newest board messages at top of list.\r\n",
       "Viewing newest board messages at bottom of list.\r\n"                        },
      {"Compression will be used if your client supports it.\r\n",
       "Compression will not be used even if your client supports it.\r\n"           },
      {"",
       ""                                                                            },
      {"You are no longer hidden from view on the who list and public channels.\r\n",
       "You are now hidden from view on the who list and public channels.\r\n"       },
      {"You will now be told that you have mail on prompt.\r\n",
       "You will no longer be told that you have mail on prompt.\r\n"                },
      {"You will no longer receive automatic hints.\r\n",
       "You will now receive automatic hints.\r\n"                                   },
      {"Screen Reader Friendly Mode Deactivated.\r\n",
       "Screen Reader Friendly Mode Activated..\r\n"                                 },
      {"You will now see equipment when looking at someone.\r\n",
       "You will no longer see equipment when looking at someone.\r\n"               },
      {"You will now listen to the music channel.\r\n",
       "You will no longer listen to the music channel.\r\n"                         },
      {"You will now parry attacks.\r\n",
       "You will no longer parry attacks.\r\n"                                       },
      {"You will no longer keep cybernetic limbs with death.\r\n",
       "You will now keep cybernetic limbs with death.\r\n"                          },
      {"You will no longer worry about acquiring steaks from animals.\r\n",
       "You will now acquire steaks from animal if you can.\r\n"                     },
      {"You will now accept things being given to you.\r\n",
       "You will no longer accept things being given to you.\r\n"                    },
      {"You will no longer instruct those you spar with.\r\n",
       "You will now instruct those you spar with.\r\n"                              },
      {"You will no longer view group health.\r\n",
       "You will now view group health.\r\n"                                         },
      {"You will no longer view item health.\r\n",
       "You will now view item health.\r\n"                                          }
  };

  if (char_is_npc(ch))
    return;

  switch (subcmd) {
    case SCMD_NOSUMMON:
      result = char_prf_toggle(ch, PRF_SUMMONABLE);
      break;
    case SCMD_NOHASSLE:
      result = char_prf_toggle(ch, PRF_NOHASSLE);
      break;
    case SCMD_BRIEF:
      result = char_prf_toggle(ch, PRF_BRIEF);
      break;
    case SCMD_COMPACT:
      result = char_prf_toggle(ch, PRF_COMPACT);
      break;
    case SCMD_NOTELL:
      result = char_prf_toggle(ch, PRF_NOTELL);
      break;
    case SCMD_NOAUCTION:
      result = char_prf_toggle(ch, PRF_NOAUCT);
      break;
    case SCMD_DEAF:
      result = char_prf_toggle(ch, PRF_DEAF);
      break;
    case SCMD_NOGOSSIP:
      result = char_prf_toggle(ch, PRF_NOGOSS);
      break;
    case SCMD_NOGRATZ:
      result = char_prf_toggle(ch, PRF_NOGRATZ);
      break;
    case SCMD_NOWIZ:
      result = char_prf_toggle(ch, PRF_NOWIZ);
      break;
    case SCMD_QUEST:
      result = char_prf_toggle(ch, PRF_QUEST);
      break;
    case SCMD_ROOMFLAGS:
      result = char_prf_toggle(ch, PRF_ROOMFLAGS);
      break;
    case SCMD_NOREPEAT:
      result = char_prf_toggle(ch, PRF_NOREPEAT);
      break;
    case SCMD_HOLYLIGHT:
      result = char_prf_toggle(ch, PRF_HOLYLIGHT);
      break;
    case SCMD_SLOWNS:
      result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
      break;
    case SCMD_AUTOEXIT:
      result = char_prf_toggle(ch, PRF_AUTOEXIT);
      break;
    case SCMD_TRACK:
      result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
      break;
    case SCMD_AFK:
      result = char_prf_toggle(ch, PRF_AFK);
      if (char_prf_flagged(ch, PRF_AFK))
        act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
      else {
        act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
        if (has_mail(char_get_idnum(ch)))
          send_to_char(ch, "You have mail waiting.\r\n");
      }
      break;
    case SCMD_AUTOLOOT:
      result = char_prf_toggle(ch, PRF_AUTOLOOT);
      break;
    case SCMD_AUTOGOLD:
      result = char_prf_toggle(ch, PRF_AUTOGOLD);
      break;
    case SCMD_CLS:
      result = char_prf_toggle(ch, PRF_CLS);
      break;
    case SCMD_BUILDWALK:
      if (char_get_admlevel(ch) < ADMLVL_IMMORT) {
        send_to_char(ch, "Immortals only, sorry.\r\n");
        return;
      }
      result = char_prf_toggle(ch, PRF_BUILDWALK);
      if (char_prf_flagged(ch, PRF_BUILDWALK))
        mudlog(LOG_CMP, char_get_level(ch), TRUE,
               "OLC: %s turned buildwalk on. Allowed zone %d", char_get_name(ch), char_get_olc_zone(ch));
      else
        mudlog(LOG_CMP, char_get_level(ch), TRUE,
               "OLC: %s turned buildwalk off. Allowed zone %d", char_get_name(ch), char_get_olc_zone(ch));
      break;
    case SCMD_AUTOSPLIT:
      result = char_prf_toggle(ch, PRF_AUTOSPLIT);
      break;
    case SCMD_AUTOSAC:
      result = char_prf_toggle(ch, PRF_AUTOSAC);
      break;
    case SCMD_SNEAK:
      result = char_aff_toggle(ch, AFF_SNEAK);
      break;
    case SCMD_HIDE:
      if ((char_get_charge(ch) > 0 && char_get_preference(ch) != PREFERENCE_KI) || (char_get_charge(ch) > getMaxKI(ch) * 0.1 && char_get_preference(ch) == PREFERENCE_KI) || char_plr_flagged(ch, PLR_POWERUP) || char_aff_flagged(ch, AFF_FLYING)) {
        send_to_char(ch, "You stand out too much to hide right now!\r\n");
        return;
      } else if (char_plr_flagged(ch, PLR_HEALT)) {
        send_to_char(ch, "You are inside a healing tank!\r\n");
        return;
      }
      if (!char_get_skill(ch, SKILL_HIDE) && slot_count(ch) + 1 <= char_get_skill_slots(ch)) {
        send_to_char(ch, "@GYou learn the very minimal basics to hiding.@n\r\n");
        char_set_skills_base(ch, SKILL_HIDE, rand_number(1, 5));
      } else if (!char_get_skill(ch, SKILL_HIDE) && slot_count(ch) + 1 > char_get_skill_slots(ch)) {
        send_to_char(ch, "@RYou need more skill slots in order to learn this skill.@n\r\n");
        return;
      }
      result = char_aff_toggle(ch, AFF_HIDE);
      break;
    case SCMD_AUTOMEM:
      result = char_prf_toggle(ch, PRF_AUTOMEM);
      break;
    case SCMD_VIEWORDER:
      result = char_prf_toggle(ch, PRF_VIEWORDER);
      break;
    case SCMD_TEST:
      if (char_get_admlevel(ch) >= 1) {
        char_prf_toggle(ch, PRF_TEST);
        send_to_char(ch, "Okay. Testing is now: %s\r\n", char_prf_flagged(ch, PRF_TEST) ? "On" : "Off");
        if (char_prf_flagged(ch, PRF_TEST)) {
          send_to_char(ch, "Make sure to remove nohassle as well.\r\n");
        }
        return;
      } else {
        send_to_char(ch, "You are not an immortal.\r\n");
        return;
      }
      break;
    case SCMD_NOCOMPRESS:
      if (CONFIG_ENABLE_COMPRESSION) {
        result = char_prf_toggle(ch, PRF_NOCOMPRESS);
        break;
      } else {
        send_to_char(ch, "Sorry, compression is globally disabled.\r\n");
        return;
      }
    case SCMD_AUTOASSIST:
      result = char_prf_toggle(ch, PRF_AUTOASSIST);
      break;
    case SCMD_WHOHIDE:
      result = char_prf_toggle(ch, PRF_HIDE);
      break;
    case SCMD_NMWARN:
      result = char_prf_toggle(ch, PRF_NMWARN);
      break;
    case SCMD_HINTS:
      result = char_prf_toggle(ch, PRF_HINTS);
      break;
    case SCMD_NODEC:
      result = char_prf_toggle(ch, PRF_NODEC);
      break;
    case SCMD_NOEQSEE:
      result = char_prf_toggle(ch, PRF_NOEQSEE);
      break;
    case SCMD_NOMUSIC:
      result = char_prf_toggle(ch, PRF_NOMUSIC);
      break;
    case SCMD_NOPARRY:
      result = char_prf_toggle(ch, PRF_NOPARRY);
      break;
    case SCMD_LKEEP:
      result = char_prf_toggle(ch, PRF_LKEEP);
      break;
    case SCMD_CARVE:
      result = char_prf_toggle(ch, PRF_CARVE);
      break;
    case SCMD_NOGIVE:
      result = char_prf_toggle(ch, PRF_NOGIVE);
      break;
    case SCMD_INSTRUCT:
      result = char_prf_toggle(ch, PRF_INSTRUCT);
      break;
    case SCMD_GHEALTH:
      result = char_prf_toggle(ch, PRF_GHEALTH);
      break;
    case SCMD_IHEALTH:
      result = char_prf_toggle(ch, PRF_IHEALTH);
      break;
    default:
      mud_log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
      /*  SYSERR_DESC:
     *  This is the same as the unhandled case in do_gen_ps(), but in the
     *  function which handles 'compact', 'brief', and so forth.
     */
      return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line  = 0,
      num_lines = 0,
      req_lines = 0,
      i;
  int l;
  char field[100], value[100], line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
  } fields[] = {
      {"none",     6,             "Does Nothing"        },
      {"bug",      ADMLVL_IMMORT, "../data/misc/bugs"   },
      {"typo",     ADMLVL_IMMORT, "../data/misc/typos"  },
      {"report",   ADMLVL_IMMORT, "../data/misc/ideas"  },
      {"xnames",   4,             "../data/misc/xnames" },
      {"levels",   4,             "../log/levels"       },
      {"rip",      4,             "../log/rip"          },
      {"players",  4,             "../log/newplayers"   },
      {"rentgone", 4,             "../log/rentgone"     },
      {"errors",   4,             "../log/errors"       },
      {"godcmds",  4,             "../log/godcmds"      },
      {"syslog",   ADMLVL_IMMORT, "../syslog"           },
      {"crash",    ADMLVL_IMMORT, "../syslog.CRASH"     },
      {"immlog",   ADMLVL_IMMORT, "../data/misc/request"},
      {"customs",  ADMLVL_IMMORT, "../data/misc/customs"},
      {"todo",     5,             "../todo"             },
      {"\n",       0,             "\n"                  }
  };

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
    for (i = 1; fields[i].level; i++)
      if (fields[i].level <= char_get_level(ch))
        sprintf(buf + strlen(buf), "%-15s%s\r\n", fields[i].cmd, fields[i].file);
    send_to_char(ch, "%s", buf);
    return;
  }

  two_arguments(argument, field, value);

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (*(fields[l].cmd) == '\n') {
    send_to_char(ch, "That is not a valid option!\r\n");
    return;
  }

  if (char_get_admlevel(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough to view that file!\r\n");
    return;
  }

  if (!strcasecmp(field, "request")) {
    char_set_lboard(ch, 2, time(0));
  }

  if (!*value)
    req_lines = 15; /* default is the last 15 lines */
  else
    req_lines = atoi(value);

  if (!(req_file = fopen(fields[l].file, "r"))) {
    mudlog(LOG_BRF, ADMLVL_IMPL, TRUE,
           "SYSERR: Error opening file %s using 'file' command.",
           fields[l].file);
    return;
  }

  get_line(req_file, line);
  while (!feof(req_file)) {
    num_lines++;
    get_line(req_file, line);
  }
  rewind(req_file);

  req_lines = MIN(MIN(req_lines, num_lines), 5000);

  buf[0] = '\0';

  get_line(req_file, line);
  while (!feof(req_file)) {
    cur_line++;
    if (cur_line > (num_lines - req_lines))
      sprintf(buf + strlen(buf), "%s\r\n", line);

    get_line(req_file, line);
  }
  fclose(req_file);

  page_string(ch->desc, buf, 1);
}

/* Returns true if the spell/skillnum is innate to the character
   (as opposed to just practiced).
 */
/* returns FALSE if the spell is found in the innate linked list. 
  This just means the ability is ready IF they can even get the
  ability. (see is_innate). 
*/
/* Adds a node to the linked list with a timer which indicates
   that the innate skill/spell is NOT ready to be used.

   is_innate_ready(...) should be called first to determine if
   there is already a timer set for the spellnum.
*/

/* called when a player enters the game to set permanent affects.
   Usually these will stay set, but certain circumstances will
   cause them to wear off (ie. removing eq with a perm affect).
*/
/* Called to update the innate timers */
void update_innate(struct char_data *ch)
{
}

ACMD(do_pagelength)
{
  char arg[MAX_INPUT_LENGTH];

  if (char_is_npc(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You current page length is set to %d lines.\r\n",
                 char_get_page_length(ch));
  } else if (is_number(arg)) {
    char_set_page_length(ch, MIN(MAX(atoi(arg), 5), 50));
    send_to_char(ch, "Okay, your page length is now set to %d lines.\r\n",
                 char_get_page_length(ch));
  } else {
    send_to_char(ch, "Please specify a number of lines (5 - 50).\r\n");
  }
}
