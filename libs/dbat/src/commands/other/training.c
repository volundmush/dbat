/* training.c: training, exercises and growth
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

/* Used by do_rpp for soft-cap */
void bring_to_cap(struct char_data *ch)
{

  bool p_trans = (race_can_transform(ch->race) && !race_can_revert(ch->race));
  int64_t cap  = calc_soft_cap(ch);

  switch (race_soft_cap_type(ch)) {
    case SOFTCAP_FIXED:
      if (getBasePL(ch) < cap)
        gainBasePL(ch, cap - getBasePL(ch) - 1, p_trans);
      if (getBaseKI(ch) < cap)
        gainBaseKI(ch, cap - getBaseKI(ch) - 1, p_trans);
      if (getBaseST(ch) < cap)
        gainBaseST(ch, cap - getBaseST(ch) - 1, p_trans);
      break;
    default:
      break;
  }
}

ACMD(do_train)
{

  if (char_is_npc(ch)) {
    return;
  }

  if (char_get_carry_weight(ch) > CAN_CARRY_W(ch)) {
    send_to_char(ch, "You are weighted down too much!\r\n");
    return;
  }

  int plus      = 0;
  int64_t total = 0, weight = 0, bonus = 0, cost = 0;
  char arg[200];

  one_argument(argument, arg);

  weight = getCurCarriedWeight(ch);

  int strcap = 5000, spdcap = 5000, intcap = 5000, wiscap = 5000, concap = 5000, aglcap = 5000;

  strcap += 500 * ch->real_abils.str;
  intcap += 500 * ch->real_abils.intel;
  wiscap += 500 * ch->real_abils.wis;
  spdcap += 500 * ch->real_abils.cha;
  concap += 500 * ch->real_abils.con;
  aglcap += 500 * ch->real_abils.dex;

  if (char_is_human(ch)) {
    intcap = intcap * 0.75;
    wiscap = wiscap * 0.75;
  } else if (char_is_kanassan(ch)) {
    intcap = intcap * 0.4;
    wiscap = wiscap * 0.4;
    aglcap = aglcap * 0.4;
  } else if (char_is_halfbreed(ch)) {
    intcap = intcap * 0.75;
    strcap = strcap * 0.75;
  } else if (char_is_truffle(ch)) {
    strcap = strcap * 1.5;
    concap = concap * 1.5;
  }

  if (!*arg) {
    send_to_char(ch, "@D-------------[ @GTraining Status @D]-------------@n\r\n");
    send_to_char(ch, "  @mStrength Progress    @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINSTR(ch)), ch->real_abils.str >= 80 ? "@rCAPPED" : add_commas(strcap));
    send_to_char(ch, "  @mSpeed Progress       @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINSPD(ch)), ch->real_abils.cha >= 80 ? "@rCAPPED" : add_commas(spdcap));
    send_to_char(ch, "  @mConstitution Progress@D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINCON(ch)), ch->real_abils.con >= 80 ? "@rCAPPED" : add_commas(concap));
    send_to_char(ch, "  @mIntelligence Progress@D: @R%6s/%6s@n\r\n", add_commas(GET_TRAININT(ch)), ch->real_abils.intel >= 80 ? "@rCAPPED" : add_commas(intcap));
    send_to_char(ch, "  @mWisdom Progress      @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINWIS(ch)), ch->real_abils.wis >= 80 ? "@rCAPPED" : add_commas(wiscap));
    send_to_char(ch, "  @mAgility Progress     @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINAGL(ch)), ch->real_abils.dex >= 80 ? "@rCAPPED" : add_commas(aglcap));
    send_to_char(ch, "@D  -----------------------------------------  @n\r\n");
    send_to_char(ch, "  @CCurrent Weight Held  @D: @c%s@n\r\n", add_commas(weight));
    send_to_char(ch, "@D---------------------------------------------@n\r\n");
    send_to_char(ch, "Syntax: train (str | spd | agl | wis | int | con)\r\n");
    return;
  }

  /*
 if (weight < (GET_LEVEL(ch) * 100) && GET_LEVEL(ch) <= 19) {
  send_to_char(ch, "With so little weight on you like that it would be a joke to try and train.\r\n");
  return;
 } else if (weight < (GET_LEVEL(ch) * 110) && GET_LEVEL(ch) <= 45) {
  send_to_char(ch, "With so little weight on you like that it would be a joke to try and train.\r\n");
  return;
 } else if (weight < (GET_LEVEL(ch) * 125) && GET_LEVEL(ch) > 45) {
  send_to_char(ch, "With so little weight on you like that it would be a joke to try and train.\r\n");
  return;
 }
*/

  /* Figure up the weight bonus */
  total = weight * (rroom_get_gravity(char_get_in_room(ch)) + 1);
  total += (rroom_get_gravity(char_get_in_room(ch)) + 1) ^ 2;
  if (rroom_get_vnum(char_get_in_room(ch)) >= 6100 && rroom_get_vnum(char_get_in_room(ch)) <= 6135) {
    total += total * 0.15;
  }

  int sensei = -1;

  if (rroom_get_vnum(char_get_in_room(ch)) == sensei_location_id(ch->chclass)) {
    if (!(char_get_gold(ch) >= 8 && GET_PRACTICES(ch, char_get_class(ch)) >= 1)) {
      send_to_char(ch, "It costs 8 Zenni and 1 PS to train with your sensei.\r\n");
      return;
    }
    total += total * 0.85;
    if (char_get_level(ch) >= 100)
      total *= 15000;
    else if (char_get_level(ch) >= 80)
      total *= 1500;
    else if (char_get_level(ch) >= 40)
      total *= 600;
    else if (char_get_level(ch) >= 20)
      total *= 300;
    else if (char_get_level(ch) >= 10)
      total *= 150;
    sensei = ch->chclass;
    send_to_char(ch, "@G%s begins to instruct you in training technique.@n\r\n", SENSEI_NAME(ch));
  }

  if (total > GET_MAX_HIT(ch) * 2) {
    bonus = 5;
  } else if (total > GET_MAX_HIT(ch)) {
    bonus = 4;
  } else if (total > (GET_MAX_HIT(ch) / 2)) {
    bonus = 3;
  } else if (total > (GET_MAX_HIT(ch) / 4)) {
    bonus = 2;
  } else if (total > (GET_MAX_HIT(ch) / 8)) {
    bonus = 1;
  }

  if (sensei < 0)
    cost = ((total / 20) + (GET_MAX_MOVE(ch) / 50));
  else
    cost = ((total / 25) + (GET_MAX_MOVE(ch) / 60));

  if (char_get_bonuses(ch, BONUS_HARDWORKER)) {
    cost -= cost * 0.25;
  }

  if (char_get_relax_count(ch) >= 464) {
    cost *= 10;
  } else if (char_get_relax_count(ch) >= 232) {
    cost *= 5;
  } else if (char_get_relax_count(ch) >= 116) {
    cost *= 2;
  }

  int stat_id      = 0;
  int8_t *stat_val = nullptr;
  char *stat_name  = nullptr;
  int bonus_trait  = -1;
  int nega_trait   = -1;
  int *stat_train  = nullptr;
  int needed       = 0;

  if (!strcasecmp("str", arg)) {
    stat_id     = 1;
    stat_val    = &(ch->real_abils.str);
    stat_name   = "strength";
    bonus_trait = BONUS_BRAWNY;
    nega_trait  = BONUS_WIMP;
    stat_train  = &(ch->player_specials->trainstr);
    needed      = strcap;
  } else if (!strcasecmp("spd", arg)) {
    stat_id     = 2;
    stat_val    = &(ch->real_abils.cha);
    stat_name   = "speed";
    bonus_trait = BONUS_QUICK;
    nega_trait  = BONUS_SLOW;
    stat_train  = &(ch->player_specials->trainspd);
    needed      = spdcap;
  } else if (!strcasecmp("con", arg)) {
    stat_id     = 3;
    stat_val    = &(ch->real_abils.con);
    stat_name   = "constitution";
    bonus_trait = BONUS_STURDY;
    nega_trait  = BONUS_FRAIL;
    stat_train  = &(ch->player_specials->traincon);
    needed      = concap;
  } else if (!strcasecmp("agl", arg)) {
    stat_id     = 4;
    stat_val    = &(ch->real_abils.dex);
    stat_name   = "agility";
    bonus_trait = BONUS_AGILE;
    nega_trait  = BONUS_CLUMSY;
    stat_train  = &(ch->player_specials->trainagl);
    needed      = aglcap;
  } else if (!strcasecmp("int", arg)) {
    stat_id     = 5;
    stat_val    = &(ch->real_abils.intel);
    stat_name   = "intelligence";
    bonus_trait = BONUS_SCHOLARLY;
    nega_trait  = BONUS_DULL;
    stat_train  = &(ch->player_specials->trainint);
    needed      = intcap;
  } else if (!strcasecmp("wis", arg)) {
    stat_id     = 6;
    stat_val    = &(ch->real_abils.wis);
    stat_name   = "wisdom";
    bonus_trait = BONUS_SAGE;
    nega_trait  = BONUS_FOOLISH;
    stat_train  = &(ch->player_specials->trainwis);
    needed      = wiscap;
  } else {
    send_to_char(ch, "Syntax: train (str | spd | agl | wis | int | con)\r\n");
    return;
  }

  if (*stat_val == 80) {
    send_to_char(ch, "Your base %s is maxed!\r\n", stat_name);
    return;
  }

  if (*stat_val >= 45 && char_get_bonuses(ch, nega_trait) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your %s beyond 45.\r\n", stat_name);
    return;
  }

  int stat_cap = 20;
  if (char_get_level(ch) >= 61)
    stat_cap = 80;
  else if (char_get_level(ch) > 40)
    stat_cap = 60;
  else if (char_get_level(ch) > 20)
    stat_cap = 40;

  if (*stat_val >= stat_cap) {
    send_to_char(ch, "You have reached the stat cap for your level.\r\n");
    return;
  }

  switch (stat_id) {
    case 1:
    case 2:
    case 3:
    case 4:
      if ((getCurST(ch)) < cost) {
        send_to_char(ch, "You do not have enough stamina with the current weight worn and gravity!\r\n");
        return;
      }
      plus = (((total / 20) + (GET_MAX_MOVE(ch) / 50)) * 100) / GET_MAX_MOVE(ch);
      decCurST(ch, cost, 0);
      break;
    case 5:
    case 6:
      if ((getCurKI(ch)) < cost) {
        send_to_char(ch, "You do not have enough ki with the current weight worn and gravity!\r\n");
        return;
      }
      plus = (((total / 20) + (GET_MAX_MANA(ch) / 50)) * 100) / GET_MAX_MANA(ch);
      decCurKI(ch, cost, 0);
      break;
  }

  /* what training message is displayed? */
  reveal_hiding(ch, 0);

  int msg_case = rand_number(1, 3);

  switch (stat_id) {
    case 1:
      switch (msg_case) {
        case 1:
          act("@WYou throw a flurry of punches into the air at an invisible opponent.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n throws a flurry of punches into the air.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("@WYou leap into the air and throw a wild kick at an invisible opponent@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n leaps into the air and throws a wild kick at nothing.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("@WYou leap high into the air and unleash a flurry of punches and kicks at an invisible opponent@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n leaps high into the air and unleashes a flurry of punches and kicks at nothing.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      break;
    case 2:
      switch (msg_case) {
        case 1:
          act("@WYou dash quickly around the surrounding area as fast as you can!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n dashes quickly around the surrounding area as fast as $e can!@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("@WYou dodge to the side as fast as you can!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n dodges to the side as fast as $e can!@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("@WYou dash backwards as fast as you can!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n dashes backwards as fast as $e can!@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      break;
    case 3:
      switch (msg_case) {
        case 1:
          act("@WYou leap into the air and then slam into the ground with your feet outstretched!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n leaps into the air and then slams into the ground with $s feet outstretched!?@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("@WYou leap into the air and then slam into the ground with your fists!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n leaps into the air and then slams into the ground with $s fists!?@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("@WYou leap into the air and then slam into the ground with your body!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n leaps into the air and then slams into the ground with $s body!?@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      break;
    case 4:
      switch (msg_case) {
        case 1:
          act("@WYou do a series of backflips through the air, landing gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n does a series of backflips through the air, landing gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("@WYou flip forward and launch off your hands into the air. You land gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n flips forward and launches off $s hands into the air. Then $e lands gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("@WYou flip to the side off one hand and then land on your feet.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n flips to the side off one hand and then lands on $s feet.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      break;
    case 5:
      switch (msg_case) {
        case 1:
          act("@WConcentrating you fly high into the air as fast as you can before settling slowly back to the ground.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n flies high into the air as fast as $e can before settling slowly back to the ground.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("@WYou focus your ki at your outstretched hand and send a mild shockwave in that direction!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n focuses $s ki at $s outstretched hand and sends a mild shockwave in that direction!@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("@WYou concentrate on your ki and force torrents of it to rush out from your body randomly!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n seems to concentrate before torrents of ki randomly blasts out from $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      break;
    case 6:
      switch (msg_case) {
        case 1:
          act("@WYou close your eyes and wage a mental battle against an imaginary opponent.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n closes $s eyes for a moment and an expression of intensity forms on it.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("@WYou look around and contemplate battle tactics for an imaginary scenario.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n looks around and appears to be imagining things that aren't there.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("@WYou invent a battle plan for a battle that doesn't exist!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@W$n seems to have thought of something.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      break;
  }

  plus += 75;
  /*
    if (GET_LEVEL(ch) > 80) {
        plus += 50;
    } else if (GET_LEVEL(ch) > 60) {
        plus += 25;
    }
     */

  plus *= 2;

  if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
    plus *= 4;
  }
  if (rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
    plus *= 3;
  }
  if (char_get_bonuses(ch, BONUS_HARDWORKER)) {
    plus += plus * 0.25;
  }
  if (char_get_bonuses(ch, bonus_trait)) {
    plus += plus * 0.75;
  }
  if (char_get_bonuses(ch, BONUS_LONER)) {
    plus += plus * 0.05;
  }
  if (sensei > -1) {
    plus += plus * 0.2;
  }

  switch (bonus) {
    case 1:
      *stat_train += 5 + plus;
      send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
      WAIT_STATE(ch, PULSE_3SEC);
      break;
    case 2:
      *stat_train += 10 + plus;
      send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
      WAIT_STATE(ch, PULSE_3SEC);
      break;
    case 3:
      *stat_train += 25 + plus;
      send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
      WAIT_STATE(ch, PULSE_3SEC);
      break;
    case 4:
      *stat_train += 50 + plus;
      send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
      WAIT_STATE(ch, PULSE_5SEC);
      break;
    case 5:
      *stat_train += 100 + plus;
      send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
      WAIT_STATE(ch, PULSE_5SEC);
      break;
    default:
      *stat_train += 1;
      send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
      WAIT_STATE(ch, PULSE_3SEC);
      break;
  }

  if (sensei > -1) {
    char_set_gold(ch, char_get_gold(ch) - (8));
    GET_PRACTICES(ch, char_get_class(ch)) -= 1;
  }

  if (*stat_train >= needed) {
    *stat_train -= needed;
    send_to_char(ch, "You feel your %s improve!@n\r\n", stat_name);
    *stat_val += 1;
    if (char_is_piccolo(ch) && char_is_namek(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0) {
      char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.25));
      send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
    }
    save_char(ch);
  }
}

ACMD(do_potential)
{
  int boost = 0;

  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_POTENTIAL)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Who's potential do you want to release?\r\n");
    send_to_char(ch, "Potential Releases: %d\r\n", char_get_boosts(ch));
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That target isn't here.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(ch, "Why would you waste your time releasing their potential?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "You can't release your own potential.\r\n");
    return;
  }
  if (char_get_boosts(ch) == 0) {
    send_to_char(ch, "You have no potential releases to perform.\r\n");
    return;
  }
  if (char_plr_flagged(vict, PLR_PR)) {
    send_to_char(ch, "Their potential has already been released\r\n");
    return;
  }
  if (char_is_android(vict)) {
    send_to_char(ch, "They are a machine and have no potential to release.\r\n");
    return;
  }
  if (char_get_majinize(vict) > 0) {
    send_to_char(ch, "They are already majinized and have no potential to release.\r\n");
    return;
  }
  if (char_is_majin(vict)) {
    send_to_char(ch, "They have no potential to release...\r\n");
    return;
  }
  /* Rillao: transloc, add new transes here */
  else {
    boost = char_get_skill(ch, SKILL_POTENTIAL) / 2;

    char_plr_set(vict, PLR_PR, true);

    gainBasePL(vict, (getBasePL(vict) / 100) * boost, false);
    if (char_is_halfbreed(vict)) {
      gainBaseKI(vict, (getBaseKI(vict) / 100) * boost, false);
      gainBaseST(vict, (getBaseST(vict) / 100) * boost, false);
    }
    reveal_hiding(ch, 0);
    act("You place your hand on top of $N's head. After a moment of concentrating you release their hidden potential.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n places $s hand on top of your head. After a moment you feel a rush of power as your hidden potential is released!", TRUE, ch, 0, vict, TO_VICT);
    act("$n places $s hand on $N's head. After a moment a rush of power explodes off of $N's body!", TRUE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_POTENTIAL, 0);
    improve_skill(ch, SKILL_POTENTIAL, 0);
    improve_skill(ch, SKILL_POTENTIAL, 0);
    improve_skill(ch, SKILL_POTENTIAL, 0);
    char_set_boosts(ch, char_get_boosts(ch) - (1));
    return;
  }
}

ACMD(do_situp)
{

  int cost = 1, bonus = 0;

  if (char_is_npc(ch)) {
    send_to_char(ch, "You are a mob fool!\r\n");
    return;
  }
  if (rroom_flagged(char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(ch, "The fire makes it too hot!\r\n");
    return;
  }
  if (char_get_drag(ch)) {
    send_to_char(ch, "You are dragging someone!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_FISHING)) {
    send_to_char(ch, "Stop fishing first.\r\n");
    return;
  }
  if (CARRYING(ch)) {
    send_to_char(ch, "You are carrying someone!\r\n");
    return;
  }

  if ((rroom_get_gravity(char_get_in_room(ch)) != 10 && char_is_bardock(ch)) || !char_is_bardock(ch)) {
    cost += rroom_get_gravity(char_get_in_room(ch)) * ((rroom_get_gravity(char_get_in_room(ch)) * 2) + char_get_level(ch));
  } else if (char_is_bardock(ch)) {
    cost += rroom_get_gravity(char_get_in_room(ch)) * char_get_level(ch);
  }

  if (rroom_get_gravity(char_get_in_room(ch)) == 300) {
    cost += 1000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 400) {
    cost += 2000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 500) {
    cost += 7500000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 1000) {
    cost += 15000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 5000) {
    cost += 25000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 10000) {
    cost += 50000000;
  }

  if (cost == 1 || cost == 0) {
    cost = 25;
  }
  if (char_get_bonuses(ch, BONUS_HARDWORKER) > 0) {
    cost -= cost * .25;
  } else if (char_get_bonuses(ch, BONUS_SLACKER) > 0) {
    cost += cost * .25;
  }

  if (char_get_relax_count(ch) >= 464) {
    cost *= 50;
  } else if (char_get_relax_count(ch) >= 232) {
    cost *= 15;
  } else if (char_get_relax_count(ch) >= 116) {
    cost *= 4;
  }

  if (char_is_android(ch) || char_is_bio(ch) || char_is_majin(ch) || char_is_arlian(ch)) {
    send_to_char(ch, "You will gain nothing from exercising!\r\n");
    return;
  }

  if (!limb_ok(ch, 1)) {
    return;
  }

  if ((getCurST(ch)) < cost) {
    send_to_char(ch, "You are too tired!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_SPAR)) {
    send_to_char(ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
    return;
  }
  if (char_get_fighting(ch)) {
    send_to_char(ch, "You are fighting you moron!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_FLYING)) {
    send_to_char(ch, "You can't do situps in midair!\r\n");
    return;
  } else {
    if (rroom_get_gravity(char_get_in_room(ch)) == 0) {
      bonus = 1;
      act("@gYou do a situp.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 10) {
      bonus = rand_number(3, 7);
      act("@gYou do a situp, feeling the strain of gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 20) {
      bonus = rand_number(8, 14);
      act("@gYou do a situp, and feel gravity's pull.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 30) {
      bonus = rand_number(14, 20);
      act("@gYou do a situp, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 40) {
      bonus = rand_number(20, 35);
      act("@gYou do a situp, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 50) {
      bonus = rand_number(40, 60);
      act("@gYou do a situp, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 100) {
      bonus = rand_number(180, 250);
      act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 200) {
      bonus = rand_number(400, 600);
      act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 300) {
      bonus = rand_number(800, 1200);
      act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 400) {
      bonus = rand_number(2000, 3000);
      act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 500) {
      bonus = rand_number(4000, 6000);
      act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 1000) {
      bonus = rand_number(9000, 10000);
      act("@gYou do a situp, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 5000) {
      bonus = rand_number(15000, 20000);
      act("@gYou do a situp, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 10000) {
      bonus = rand_number(25000, 30000);
      act("@gYou do a situp, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a situp, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
    }
    if (cost >= GET_MAX_MOVE(ch) / 2) {
      send_to_char(ch, "This gravity is a great challenge for you!\r\n");
      bonus *= 10;
    } else if (cost >= GET_MAX_MOVE(ch) / 4) {
      send_to_char(ch, "This gravity is an awesome challenge for you!\r\n");
      bonus *= 8;
    } else if (cost >= GET_MAX_MOVE(ch) / 10) {
      send_to_char(ch, "This gravity is a good challenge for you!\r\n");
      bonus *= 6;
    } else if (cost < GET_MAX_MOVE(ch) / 1000) {
      send_to_char(ch, "This gravity is so easy to you, you could do it in your sleep...\r\n");
      bonus /= 8;
    } else if (cost < GET_MAX_MOVE(ch) / 100) {
      send_to_char(ch, "This gravity is the opposite of a challenge for you...\r\n");
      bonus /= 5;
    } else if (cost < GET_MAX_MOVE(ch) / 50) {
      send_to_char(ch, "This gravity is definitely not a challenge for you...\r\n");
      bonus /= 4;
    } else if (cost < GET_MAX_MOVE(ch) / 30) {
      send_to_char(ch, "This gravity is barely a challenge for you...\r\n");
      bonus /= 3;
    } else if (cost < GET_MAX_MOVE(ch) / 20) {
      send_to_char(ch, "This gravity is hardly a challenge for you...\r\n");
      bonus /= 2;
    } else {
      send_to_char(ch, "This gravity is just perfect for you...\r\n");
      bonus *= 4;
    }
    if (is_soft_cap(ch, 2, 1.0)) {
      bonus = 0;
    }
    if (rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      send_to_char(ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
      bonus *= 10;
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT)) {
      if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
        bonus *= 10;
      } else {
        bonus *= 5;
      }
    } else if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
      send_to_char(ch, "@rThis place feels like... Magic.@n\r\n");
      bonus *= 20;
    }
    if (bonus <= 0 && !rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      bonus = 1;
    }
    if (bonus <= 0 && rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      bonus = 15;
    }
    if (bonus <= 0 && rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT)) {
      if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
        bonus = 12;
      } else {
        bonus = 6;
      }
    }
    /* Rillao: transloc, add new transes here */
    bonus += char_get_level(ch) / 20;
    if (char_is_namek(ch)) {
      bonus -= bonus / 4;
    }
    if (char_get_bonuses(ch, BONUS_HARDWORKER)) {
      bonus += bonus * 0.5;
    }
    if (char_get_bonuses(ch, BONUS_LONER)) {
      bonus += bonus * 0.1;
    }
    send_to_char(ch, "You feel slightly more vigorous @D[@G+%s@D]@n.\r\n", add_commas(bonus));
    gainBaseST(ch, bonus, true);
    if (rroom_get_gravity(char_get_in_room(ch)) <= 50) {
      WAIT_STATE(ch, PULSE_2SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 200) {
      WAIT_STATE(ch, PULSE_3SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 500) {
      WAIT_STATE(ch, PULSE_4SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 1000) {
      WAIT_STATE(ch, PULSE_5SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 5000) {
      WAIT_STATE(ch, PULSE_6SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 10000) {
      WAIT_STATE(ch, PULSE_7SEC);
    }
    decCurST(ch, cost, 0);
  }
}

ACMD(do_meditate)
{

  int64_t bonus = 0, cost = 1, weight = 0;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (char_is_npc(ch)) {
    send_to_char(ch, "You are a mob fool!\r\n");
    return;
  }
  if (rroom_flagged(char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(ch, "The fire makes it too hot!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_FISHING)) {
    send_to_char(ch, "Stop fishing first.\r\n");
    return;
  }

  if (char_is_android(ch) || char_is_bio(ch) || char_is_majin(ch) || char_is_arlian(ch)) {
    send_to_char(ch, "You will gain nothing from exercising!\r\n");
    return;
  }

  if (CARRYING(ch)) {
    send_to_char(ch, "You are carrying someone!\r\n");
    return;
  }

  if (char_get_drag(ch)) {
    send_to_char(ch, "You are dragging someone!\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_SPAR)) {
    send_to_char(ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
    return;
  }

  if (char_get_fighting(ch)) {
    send_to_char(ch, "You are fighting you moron!\r\n");
    return;
  }

  if (char_get_position(ch) != POS_SITTING) {
    send_to_char(ch, "You need to be sitting to meditate.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Syntax: meditate (object)\nSyntax: meditate expand\nSyntax: meditate break\r\n");
    return;
  }

  if (!strcasecmp(arg, "expand")) {
    int cost = 3500;
    if (char_is_saiyan(ch)) {
      cost = 7000;
    }
    if (GET_PRACTICES(ch, char_get_class(ch)) < cost) {
      send_to_char(ch, "You do not have enough practice sessions to expand your mind and ability to remember skills.\r\n");
      send_to_char(ch, "%s needed.\r\n", add_commas(cost));
    } else if (char_get_skill_slots(ch) >= 60 && char_get_bonuses(ch, BONUS_GMEMORY) == 0) {
      send_to_char(ch, "You can not have any more slots through this process.\r\n");
    } else if (char_get_skill_slots(ch) >= 65 && char_get_bonuses(ch, BONUS_GMEMORY) == 1) {
      send_to_char(ch, "You can not have any more slots through this process.\r\n");
    } else {
      send_to_char(ch, "During your meditation you manage to expand your mind and get the feeling you could learn some new skills.\r\n");
      char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
      GET_PRACTICES(ch, char_get_class(ch)) -= cost;
      return;
    }
    return;
  } else if (!strcasecmp(arg, "break")) {
    if (char_get_mindlink(ch) == NULL) {
      send_to_char(ch, "You are not mind linked with anyone.\r\n");
      return;
    } else if (char_get_linker(ch) == 1) {
      send_to_char(ch, "This is not how you break YOUR mind link.\r\n");
      return;
    } else if ((getCurKI(ch)) < GET_MAX_MANA(char_get_mindlink(ch)) * 0.05) {
      send_to_char(ch, "You do not have enough ki to manage a break.\r\n");
      return;
    } else if (char_get_aff_abils_intel(ch) + rand_number(-5, 10) >= char_get_aff_abils_intel(char_get_mindlink(ch)) + (char_get_skill(char_get_mindlink(ch), SKILL_TELEPATHY) * 0.1)) {
      act("@rYou manage to break the mind link between you and @R$N@r!@n", FALSE, ch, 0, char_get_mindlink(ch), TO_CHAR);
      act("$n closes their eyes for a few seconds.", FALSE, ch, 0, char_get_mindlink(ch), TO_ROOM);
      send_to_char(char_get_mindlink(ch), "@rYour mind linked target manages to push you out!@n\r\n");
      if (char_get_aff_abils_intel(char_get_mindlink(ch)) < axion_dice(-10) && !char_aff_flagged(char_get_mindlink(ch), AFF_SHOCKED)) {
        send_to_char(char_get_mindlink(ch), "Your mind is shocked by the flood of mental energy that pushed it out!@n\r\n");
        char_aff_set(char_get_mindlink(ch), AFF_SHOCKED, false);
      }

      char_set_linker(char_get_mindlink(ch), 0);
      char_set_mindlink(char_get_mindlink(ch), NULL);
      char_set_mindlink(ch, NULL);
      return;
    } else {
      act("@rYou struggle to free your mind of @R$N's@r link, but fail!@n", FALSE, ch, 0, char_get_mindlink(ch), TO_CHAR);
      act("$n closes their eyes for a few seconds, and appears to struggle quite a bit.", FALSE, ch, 0, char_get_mindlink(ch), TO_ROOM);
      send_to_char(char_get_mindlink(ch), "@rYour mind linked target struggles to free their mind, but fails!@n\r\n");

      decCurKI(ch, GET_MAX_MANA(char_get_mindlink(ch)) * .05, 0);
      return;
    }
  } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[char_get_in_room(ch)].contents))) {
    send_to_char(ch, "Syntax: meditate (object)\nSyntax: meditate expand\nSyntax: meditate break\r\n");
    return;
  }

  if (obj_get_vnum(obj) == 79) {
    send_to_char(ch, "It's frozen to the surface.\r\n");
    return;
  }

  weight = obj_get_weight(obj);

  if (obj_get_sitting(obj)) {
    weight += char_get_weight(obj_get_sitting(obj));
  }

  if ((rroom_get_gravity(char_get_in_room(ch)) != 10 && char_is_bardock(ch)) || !char_is_bardock(ch)) {
    cost += rroom_get_gravity(char_get_in_room(ch)) * ((rroom_get_gravity(char_get_in_room(ch)) * 2) + char_get_level(ch));
  } else if (char_is_bardock(ch)) {
    cost += rroom_get_gravity(char_get_in_room(ch)) * char_get_level(ch);
  }
  cost += weight * ((rroom_get_gravity(char_get_in_room(ch)) + 1) / 5);

  if (rroom_get_gravity(char_get_in_room(ch)) == 300) {
    cost += 1000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 400) {
    cost += 2000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 500) {
    cost += 7500000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 1000) {
    cost += 15000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 5000) {
    cost += 25000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 10000) {
    cost += 50000000;
  }

  if (cost < weight) {
    cost = weight;
  }

  if (cost == 1 || cost == 0) {
    cost = 25;
  }
  if (char_get_bonuses(ch, BONUS_HARDWORKER) > 0) {
    cost -= cost * .25;
  } else if (char_get_bonuses(ch, BONUS_SLACKER) > 0) {
    cost += cost * .25;
  }

  if (char_get_relax_count(ch) >= 464) {
    cost *= 50;
  } else if (char_get_relax_count(ch) >= 232) {
    cost *= 15;
  } else if (char_get_relax_count(ch) >= 116) {
    cost *= 4;
  }

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You don't have enough ki!\r\n");
    return;
  }

  else {
    /* Meditate Message */
    act("@cYou close your eyes and concentrate, lifting $p@c with your ki.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@c$n closes $s eyes and lifts $p@c with $s ki.@n", TRUE, ch, obj, 0, TO_ROOM);

    if (rroom_get_gravity(char_get_in_room(ch)) == 0) {
      bonus = 1;
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 10) {
      bonus = rand_number(2, 4);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 20) {
      bonus = rand_number(5, 10);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 30) {
      bonus = rand_number(10, 15);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 40) {
      bonus = rand_number(15, 20);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 50) {
      bonus = rand_number(40, 60);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 100) {
      bonus = rand_number(180, 250);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 200) {
      bonus = rand_number(400, 600);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 300) {
      bonus = rand_number(800, 1200);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 400) {
      bonus = rand_number(2000, 3000);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 500) {
      bonus = rand_number(4000, 6000);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 1000) {
      bonus = rand_number(9000, 10000);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 5000) {
      bonus = rand_number(15000, 20000);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 10000) {
      bonus = rand_number(25000, 30000);
    }

    bonus += ((weight + 1) / 500) + 1;

    if (cost >= GET_MAX_MANA(ch) / 2) {
      send_to_char(ch, "The object weight and gravity are a great challenge for you!\r\n");
      bonus *= 10;
    } else if (cost >= GET_MAX_MANA(ch) / 4) {
      send_to_char(ch, "The object weight and gravity are an awesome challenge for you!\r\n");
      bonus *= 8;
    } else if (cost >= GET_MAX_MANA(ch) / 10) {
      send_to_char(ch, "The object weight and gravity are a good challenge for you!\r\n");
      bonus *= 6;
    } else if (cost < GET_MAX_MANA(ch) / 1000) {
      send_to_char(ch, "The object weight and gravity are so easy to you, you could do it in your sleep....\r\n");
      bonus /= 8;
    } else if (cost < GET_MAX_MANA(ch) / 100) {
      send_to_char(ch, "The object weight and gravity are the opposite of a challenge for you....\r\n");
      bonus /= 5;
    } else if (cost < GET_MAX_MANA(ch) / 50) {
      send_to_char(ch, "The object weight and gravity are definitely not a challenge for you....\r\n");
      bonus /= 4;
    } else if (cost < GET_MAX_MANA(ch) / 30) {
      send_to_char(ch, "The object weight and gravity are barely a challenge for you....\r\n");
      bonus /= 3;
    } else if (cost < GET_MAX_MANA(ch) / 20) {
      send_to_char(ch, "The object weight and gravity are hardly a challenge for you....\r\n");
      bonus /= 2;
    } else {
      send_to_char(ch, "This gravity is just perfect for you....\r\n");
      bonus *= 4;
    }
    if (is_soft_cap(ch, 1, 1.0)) {
      bonus = 0;
    }
    if (rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      send_to_char(ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
      bonus *= 10;
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT)) {
      if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
        bonus *= 10;
      } else {
        bonus *= 5;
      }
    } else if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
      send_to_char(ch, "@rThis place feels like... Magic.@n\r\n");
      bonus *= 20;
    }
    if (bonus <= 0 && !rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      bonus = 1;
    }
    if (bonus <= 0 && rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      bonus = 15;
    }
    if (bonus <= 1 && rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT)) {
      if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
        bonus = 12;
      } else {
        bonus = 6;
      }
    }
    if (bonus != 1 && char_is_demon(ch) && rand_number(1, 100) >= 80) {
      send_to_char(ch, "Your spirit magnifies the strength of your body! @D[@G+%s@D]@n\r\n", add_commas(bonus / 2));
      gainBasePL(ch, bonus / 2, false);
    }

    bonus += char_get_level(ch) / 20;
    if (char_is_namek(ch)) {
      bonus += bonus / 2;
    }
    if (char_get_bonuses(ch, BONUS_LONER)) {
      bonus += bonus * 0.1;
    }
    /* Rillao: transloc, add new transes here */
    send_to_char(ch, "You feel your spirit grow stronger @D[@G+%s@D]@n.\r\n", add_commas(bonus));
    gainBaseKI(ch, bonus, true);
    if (rroom_get_gravity(char_get_in_room(ch)) <= 50) {
      WAIT_STATE(ch, PULSE_2SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 200) {
      WAIT_STATE(ch, PULSE_3SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 500) {
      WAIT_STATE(ch, PULSE_4SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 1000) {
      WAIT_STATE(ch, PULSE_5SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 5000) {
      WAIT_STATE(ch, PULSE_6SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 10000) {
      WAIT_STATE(ch, PULSE_7SEC);
    }

    decCurKI(ch, cost, 0);
  }
}

ACMD(do_pushup)
{

  int cost = 1, bonus = 0;

  if (char_is_npc(ch)) {
    send_to_char(ch, "You are a mob fool!\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_FISHING)) {
    send_to_char(ch, "Stop fishing first.\r\n");
    return;
  }
  if (char_get_drag(ch)) {
    send_to_char(ch, "You are dragging someone!\r\n");
    return;
  }
  if (CARRYING(ch)) {
    send_to_char(ch, "You are carrying someone!\r\n");
    return;
  }

  if ((rroom_get_gravity(char_get_in_room(ch)) != 10 && char_is_bardock(ch)) || !char_is_bardock(ch)) {
    cost += rroom_get_gravity(char_get_in_room(ch)) * ((rroom_get_gravity(char_get_in_room(ch)) * 2) + char_get_level(ch));
  } else if (char_is_bardock(ch)) {
    cost += rroom_get_gravity(char_get_in_room(ch)) * char_get_level(ch);
  }

  if (rroom_get_gravity(char_get_in_room(ch)) == 300) {
    cost += 1000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 400) {
    cost += 2000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 500) {
    cost += 7500000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 1000) {
    cost += 15000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 5000) {
    cost += 25000000;
  } else if (rroom_get_gravity(char_get_in_room(ch)) == 10000) {
    cost += 50000000;
  }

  if (cost == 1 || cost == 0) {
    cost = 25;
  }

  if (char_get_bonuses(ch, BONUS_HARDWORKER) > 0) {
    cost -= cost * .25;
  } else if (char_get_bonuses(ch, BONUS_SLACKER) > 0) {
    cost += cost * .25;
  }

  if (char_get_relax_count(ch) >= 464) {
    cost *= 50;
  } else if (char_get_relax_count(ch) >= 232) {
    cost *= 15;
  } else if (char_get_relax_count(ch) >= 116) {
    cost *= 4;
  }

  if (char_is_android(ch) || char_is_bio(ch) || char_is_majin(ch) || char_is_arlian(ch)) {
    send_to_char(ch, "You will gain nothing from exercising!\r\n");
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if ((getCurST(ch)) < cost) {
    send_to_char(ch, "You are too tired!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_SPAR)) {
    send_to_char(ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
    return;
  }
  if (char_get_fighting(ch)) {
    send_to_char(ch, "You are fighting you moron!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_FLYING)) {
    send_to_char(ch, "You can't do pushups in midair!\r\n");
    return;
  }

  else {
    if (rroom_get_gravity(char_get_in_room(ch)) == 0) {
      bonus = 1;
      act("@gYou do a pushup.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 10) {
      bonus = rand_number(3, 7);
      act("@gYou do a pushup, feeling the strain of gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 20) {
      bonus = rand_number(8, 14);
      act("@gYou do a pushup, and feel gravity's pull.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 30) {
      bonus = rand_number(14, 20);
      act("@gYou do a pushup, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 40) {
      bonus = rand_number(20, 35);
      act("@gYou do a pushup, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 50) {
      bonus = rand_number(40, 60);
      act("@gYou do a pushup, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 100) {
      bonus = rand_number(180, 250);
      act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 200) {
      bonus = rand_number(400, 600);
      act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 300) {
      bonus = rand_number(800, 1200);
      act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 400) {
      bonus = rand_number(2000, 3000);
      act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 500) {
      bonus = rand_number(4000, 6000);
      act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 1000) {
      bonus = rand_number(9000, 10000);
      act("@gYou do a pushup, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 5000) {
      bonus = rand_number(15000, 20000);
      act("@gYou do a pushup, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (rroom_get_gravity(char_get_in_room(ch)) == 10000) {
      bonus = rand_number(25000, 30000);
      act("@gYou do a pushup, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@g$n does a pushup, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
    }
    if (cost >= getMaxPLTrans(ch) / 2) {
      send_to_char(ch, "This gravity is a great challenge for you!\r\n");
      bonus *= 10;
    } else if (cost >= getMaxPLTrans(ch) / 4) {
      send_to_char(ch, "This gravity is an awesome challenge for you!\r\n");
      bonus *= 8;
    } else if (cost >= getMaxPLTrans(ch) / 10) {
      send_to_char(ch, "This gravity is a good challenge for you!\r\n");
      bonus *= 6;
    } else if (cost < getMaxPLTrans(ch) / 1000) {
      send_to_char(ch, "This gravity is so easy to you, you could do it in your sleep....\r\n");
      bonus /= 8;
    } else if (cost < getMaxPLTrans(ch) / 100) {
      send_to_char(ch, "This gravity is the opposite of a challenge for you....\r\n");
      bonus /= 5;
    } else if (cost < getMaxPLTrans(ch) / 50) {
      send_to_char(ch, "This gravity is definitely not a challenge for you....\r\n");
      bonus /= 4;
    } else if (cost < getMaxPLTrans(ch) / 30) {
      send_to_char(ch, "This gravity is barely a challenge for you....\r\n");
      bonus /= 3;
    } else if (cost < getMaxPLTrans(ch) / 20) {
      send_to_char(ch, "This gravity is hardly a challenge for you....\r\n");
      bonus /= 2;
    } else {
      send_to_char(ch, "This gravity is just perfect for you....\r\n");
      bonus *= 4;
    }
    if (is_soft_cap(ch, 0, 1.0)) {
      bonus = 0;
    }
    if (rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      send_to_char(ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
      bonus *= 10;
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT)) {
      if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
        bonus *= 10;
      } else {
        bonus *= 5;
      }
    } else if (rroom_get_vnum(char_get_in_room(ch)) >= 19800 && rroom_get_vnum(char_get_in_room(ch)) <= 19899) {
      send_to_char(ch, "@rThis place feels like... Magic.@n\r\n");
      bonus *= 20;
    }
    if (bonus <= 0 && !rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      bonus = 1;
    }
    if (bonus <= 0 && rroom_flagged(char_get_in_room(ch), ROOM_HBTC)) {
      bonus = 15;
    }
    if (bonus <= 1 && rroom_flagged(char_get_in_room(ch), ROOM_WORKOUT)) {
      if (rroom_get_vnum(char_get_in_room(ch)) >= 19100 && rroom_get_vnum(char_get_in_room(ch)) <= 19199) {
        bonus = 12;
      } else {
        bonus = 6;
      }
    }
    bonus += char_get_level(ch) / 20;
    if (char_is_namek(ch)) {
      bonus -= bonus / 4;
    }
    if (char_get_bonuses(ch, BONUS_HARDWORKER)) {
      bonus += bonus * 0.5;
    }
    if (char_get_bonuses(ch, BONUS_LONER)) {
      bonus += bonus * 0.1;
    }
    /* Rillao: transloc, add new transes here */
    send_to_char(ch, "You feel slightly stronger @D[@G+%s@D]@n.\r\n", add_commas(bonus));

    if (char_is_human(ch)) {
      bonus = bonus * 0.8;
    }
    gainBasePL(ch, bonus, true);
    if (rroom_get_gravity(char_get_in_room(ch)) <= 50) {
      WAIT_STATE(ch, PULSE_2SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 200) {
      WAIT_STATE(ch, PULSE_3SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 500) {
      WAIT_STATE(ch, PULSE_4SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 1000) {
      WAIT_STATE(ch, PULSE_5SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 5000) {
      WAIT_STATE(ch, PULSE_6SEC);
    } else if (rroom_get_gravity(char_get_in_room(ch)) <= 10000) {
      WAIT_STATE(ch, PULSE_7SEC);
    }
    decCurST(ch, cost, 0);
  }
}

/* This handles many player specific routines. It may be a bit too bloated though. */
void base_update(void)
{
  struct descriptor_data *d;
  int cash = FALSE, inc = 0;
  int countch = FALSE, pcoun = 0;

  if (INTERESTTIME != 0 && INTERESTTIME <= time(0) && time(0) != 0) {
    INTERESTTIME = time(0) + 86400;
    LASTINTEREST = time(0);
    save_mud_time(&time_info);
    cash    = TRUE;
    countch = TRUE;
  }

  if (TOPCOUNTDOWN > 0) {
    TOPCOUNTDOWN -= 4;
  }

  for (d = descriptor_list; d; d = d->next) {
    if (!desc_is_playing(d))
      continue;
    if (char_is_npc(d->character)) {
      if (char_get_absorbing(d->character) && char_get_in_room(d->character) != char_get_in_room(char_get_absorbing(d->character))) {
        send_to_char(d->character, "You stop absorbing %s!\r\n", char_get_name(char_get_absorbing(d->character)));
        char_set_absorbby(char_get_absorbing(d->character), NULL);
        char_set_absorbing(d->character, NULL);
      }
      if (char_is_android(d->character) && char_get_absorbing(d->character) && rand_number(1, 10) >= 7) {
        int64_t drain1 = GET_MAX_MANA(d->character) * 0.01, drain2 = GET_MAX_MOVE(d->character) * 0.01;
        struct char_data *drained = char_get_absorbing(d->character);
        if ((getCurST(drained)) - drain2 < 0) {
          drain2 = (getCurST(drained));
        }
        if ((getCurKI(drained)) - drain1 < 0) {
          drain1 = (getCurKI(drained));
        }
        incCurST(d->character, drain2, true);
        incCurKI(d->character, drain1, true);
        incCurHealth(d->character, drain1 * .5, true);

        if (isFullKI(d->character) && isFullST(d->character)) {
          do_absorb(d->character, NULL, 0, 0);
        }
      }
      continue;
    }
    if (countch == TRUE) {
      pcoun += 1;
    }
    if (!char_is_npc(d->character) && rand_number(1, 15) >= 14) {
      ash_burn(d->character);
    }
    int64_t forty_lf = getMaxLF(d->character) * 0.4;
    if (char_aff_flagged(d->character, AFF_CURSE) && getCurLF(d->character) > forty_lf) {
      decCurLFPercent(d->character, .01, 0);
      demon_refill_lf(d->character, getMaxLF(d->character) * 0.01);

      if (getCurLF(d->character) < forty_lf) {
        incCurLF(d->character, forty_lf - getCurLF(d->character), true);
      }
    }
    if (char_get_backstabcool(d->character) > 0) {
      char_set_backstabcool(d->character, char_get_backstabcool(d->character) - (1));
    }
    if (char_plr_flagged(d->character, PLR_GOOP) && d->character->gooptime == 60) {
      if (char_is_bio(d->character)) {
        act("@GConciousness slowly returns to you. You realize quickly that some of your cells have survived. You take control of your regenerative processes and focus on growing a new body!@n", TRUE, d->character, 0, 0, TO_CHAR);
      } else {
        act("@MSlowly you regain conciousness. The various split off chunks of your body begin to likewise stir.@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@MYou think you notice the chunks of @m$n@M's moving slightly.@n", TRUE, d->character, 0, 0, TO_ROOM);
      }
      d->character->gooptime -= 1;
    } else if (char_plr_flagged(d->character, PLR_GOOP) && d->character->gooptime == 30) {
      if (char_is_bio(d->character)) {
        act("@GFrom the collection of cells growing a crude form of your body starts to take shape!@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@GYou start to notice a large mass of pulsing flesh growing before you!@n", TRUE, d->character, 0, 0, TO_ROOM);
      } else {
        act("@MYou will the various chunks of your body to return and slowly more and more of them begin to fly into you. Your body begins to grow larger and larger as this process unfolds!@n ", TRUE, d->character, 0, 0, TO_CHAR);
        act("@MThe various chunks of @m$n@M's body start to fly into the largest chunk! As the chunks collide they begin to form a larger and still growing blob of goo!@n", TRUE, d->character, 0, 0, TO_ROOM);
      }
      d->character->gooptime -= 1;
    } else if (char_plr_flagged(d->character, PLR_GOOP) && d->character->gooptime == 15) {
      if (char_is_bio(d->character)) {
        act("@GYour body has almost reached its previous form! Only a little more regenerating is needed!@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@GThe lump of flesh has now grown to the size where the likeness of @g$n@G can be seen of it! It appears that $e is regenerating $s body from what was only a few cells!@n", TRUE, d->character, 0, 0, TO_ROOM);
      } else {
        act("@MYour body has reached half its previous size as your limbs ooze slowly out into their proper shape!@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@m$n@M's body has regenerated to half its previous size! Slowly $s limbs ooze out into their proper shape! It won't be long now till $e has fully regenerated!@n", TRUE, d->character, 0, 0, TO_ROOM);
      }
      d->character->gooptime -= 1;
    } else if (char_plr_flagged(d->character, PLR_GOOP) && d->character->gooptime == 0) {
      if (char_is_bio(d->character)) {
        restoreHealth(d->character, true);
        act("@GYour body has fully regenerated! You flex your arms and legs outward with a rush of renewed strength!@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@g$n@G's body has fully regenerated! Suddenly $e flexes $s arms and legs and a rush of power erupts from off of $s body!@n", TRUE, d->character, 0, 0, TO_ROOM);
      }
      //Zenkai Boost
      else if (char_is_saiyan(d->character)) {

        setCurHealthPercent(d->character, 0.5);
        double curKiPerc = getCurKIPercent(d->character);
        // if less than 25%, restore up to 25%
        if (curKiPerc < 0.25) {
          setCurKIPercent(d->character, 0.25);
        }
        double curSTPerc = getCurSTPercent(d->character);
        // if less than 25%, restore up to 25%
        if (curSTPerc < 0.25) {
          setCurSTPercent(d->character, 0.25);
        }

        if (!char_in_arena(d->character)) {
          int64_t zenkaiPL, zenkaiKi, zenkaiSt;
          zenkaiPL = getBasePL(d->character) * 0.03;
          zenkaiKi = getBaseKI(d->character) * 0.015;
          zenkaiSt = getBaseST(d->character) * 0.015;

          gainBasePL(d->character, zenkaiPL, false);
          gainBaseKI(d->character, zenkaiKi, false);
          gainBaseST(d->character, zenkaiSt, false);

          send_to_char(d->character, "@D[@YZ@ye@wn@Wk@Ya@yi @YB@yo@wo@Ws@Yt@D] @WYou feel much stronger!\r\n");
          send_to_char(d->character, "@D[@RPL@Y:@n+%s@D] @D[@CKI@Y:@n+%s@D] @D[@GSTA@Y:@n+%s@D]@n\r\n", add_commas(zenkaiPL), add_commas(zenkaiKi), add_commas(zenkaiSt));
        }
        act("@RYou collapse to the ground, body pushed beyond the typical limits of exhaustion. The passage of time distorts and an indescribable amount of time passes as raw emotions pass through your very being. Your eyes open and focus with a newfound clarity as your unadulterated emotions and feelings revive you for a second wind!@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@r$n@R collapses to the ground, seemingly dead. After a brief moment, their eyes flash open with a determined look on their face!", TRUE, d->character, 0, 0, TO_ROOM);
      } else {
        restoreHealth(d->character, true);
        act("@MYour body has fully regenerated! You scream out in triumph and a short gust of steam erupts from your pores!@n", TRUE, d->character, 0, 0, TO_CHAR);
        act("@m$n@M's body has fully regenerated! Suddenly $e screams out in gleeful triumph and short gust of steam erupts from $s skin pores!", TRUE, d->character, 0, 0, TO_ROOM);
      }
      char_plr_set(d->character, PLR_GOOP, false);
    } else {
      d->character->gooptime -= 1;
    }
    if (char_get_con_cooldown(d->character) > 0) {
      char_set_con_cooldown(d->character, char_get_con_cooldown(d->character) - (2));
      if (char_get_con_cooldown(d->character) <= 0) {
        char_set_con_cooldown(d->character, 0);
        send_to_char(d->character, "You can concentrate again.\r\n");
      }
    }
    /* Andros Start */
    if (char_get_con_sdcooldown(d->character) > 0) {
      char_set_con_sdcooldown(d->character, char_get_con_sdcooldown(d->character) - (10));
      if (char_get_con_sdcooldown(d->character) <= 0) {
        char_set_con_sdcooldown(d->character, 0);
        send_to_char(d->character, "Your body has recovered from your last selfdestruct.\r\n");
      }
    } /* Andros End */
    if (CARRYING(d->character)) {
      if (char_get_in_room(CARRYING(d->character)) != char_get_in_room(d->character)) {
        carry_drop(d->character, 3);
      }
    }
    if (char_get_defender(d->character)) {
      if (char_get_in_room(d->character) != char_get_in_room(char_get_defender(d->character))) {
        char_set_defending(char_get_defender(d->character), NULL);
        char_set_defender(d->character, NULL);
      }
    }
    if (char_get_defending(d->character)) {
      if (char_get_in_room(d->character) != char_get_in_room(char_get_defending(d->character))) {
        char_set_defender(char_get_defending(d->character), NULL);
        char_set_defending(d->character, NULL);
      }
    }
    if (char_plr_flagged(d->character, PLR_TRANSMISSION)) {
      char_plr_set(d->character, PLR_TRANSMISSION, false);
    }
    if (!char_get_fighting(d->character) && char_aff_flagged(d->character, AFF_POSITION)) {
      char_aff_set(d->character, AFF_POSITION, false);
    }
    if (char_get_sits(d->character)) {
      if (char_get_in_room(d->character) != obj_get_in_room(char_get_sits(d->character))) {
        struct obj_data *chair = char_get_sits(d->character);
        obj_set_sitting(chair, NULL);
        char_set_sits(d->character, NULL);
      }
    }
    if (char_get_ping(d->character) >= 1) {
      char_set_ping(d->character, char_get_ping(d->character) - (1));
      if (char_plr_flagged(d->character, PLR_PILOTING) && char_get_ping(d->character) == 0) {
        send_to_char(d->character, "Your radar is ready to calculate the direction of another destination.\r\n");
      }
    }
    if (char_get_admlevel(d->character) < 1 && TOPCOUNTDOWN <= 0 && char_get_level(d->character) > 0) {
      topWrite(d->character);
    }
    if (char_plr_flagged(d->character, PLR_SELFD) && !char_plr_flagged(d->character, PLR_SELFD2)) {
      if (rand_number(4, 100) < char_get_skill(d->character, SKILL_SELFD)) {
        send_to_char(d->character, "You feel you are ready to self destruct!\r\n");
        char_plr_set(d->character, PLR_SELFD2, true);
      }
    }
    if (!char_get_fighting(d->character) && char_get_combo(d->character) > -1) {
      char_set_combo(d->character, -1);
      char_set_combhits(d->character, 0);
    }
    if (char_moon_ok(d->character)) {
      oozaru_transform(d->character);
    }
    if (cash == TRUE && char_get_bank_gold(d->character) > 0) {
      inc = char_get_bank_interest(d->character);
      char_set_lastint(d->character, LASTINTEREST);
      char_set_bank_gold(d->character, char_get_bank_gold(d->character) + (inc));
      send_to_char(d->character, "@cBank Interest@D: @Y%s@n\r\n", add_commas(inc));
    }
    if (!char_is_npc(d->character)) {
      check_eq(d->character);
    }
    if (!char_is_npc(d->character) && rroom_get_geffect(char_get_in_room(d->character)) >= 1 && rand_number(1, 100) >= 96) {
      if (rroom_get_geffect(char_get_in_room(d->character)) <= 4) {
        switch (rand_number(1, 4)) {
          case 1:
            act("@RLava spews up violently from the cracks in the ground!@n", FALSE, d->character, 0, 0, TO_ROOM);
            act("@RLava spews up violently from the cracks in the ground!@n", FALSE, d->character, 0, 0, TO_CHAR);
            break;
          case 2:
            act("@RThe lava bubbles and gives off tremendous heat!@n", FALSE, d->character, 0, 0, TO_ROOM);
            act("@RThe lava bubbles and gives off tremendous heat!@n", FALSE, d->character, 0, 0, TO_CHAR);
            break;
          case 3:
            act("@RNoxious fumes rise from the bubbling lava!@n", FALSE, d->character, 0, 0, TO_ROOM);
            act("@RNoxious fumes rise from the bubbling lava!@n", FALSE, d->character, 0, 0, TO_CHAR);
            break;
          case 4:
            act("@RSome of the lava cools as it spreads further from the source!@n", FALSE, d->character, 0, 0, TO_ROOM);
            act("@RSome of the lava cools as it spreads further from the source!@n", FALSE, d->character, 0, 0, TO_CHAR);
            break;
        }
        rroom_set_geffect(char_get_in_room(d->character), rroom_get_geffect(char_get_in_room(d->character)) + (1));
      } else if (rroom_get_geffect(char_get_in_room(d->character)) == 5) {
        act("@RLava covers the entire area now!@n", FALSE, d->character, 0, 0, TO_ROOM);
        act("@RLava covers the entire area now!@n", FALSE, d->character, 0, 0, TO_CHAR);
        rroom_set_geffect(char_get_in_room(d->character), rroom_get_geffect(char_get_in_room(d->character)) + (1));
      }
    }
    if (char_get_absorbing(d->character) && char_get_in_room(d->character) != char_get_in_room(char_get_absorbing(d->character))) {
      send_to_char(d->character, "You stop absorbing %s!\r\n", char_get_name(char_get_absorbing(d->character)));
      char_set_absorbby(char_get_absorbing(d->character), NULL);
      char_set_absorbing(d->character, NULL);
    }
    if (char_is_android(d->character) && char_get_absorbing(d->character)) {
      if (getCurST(char_get_absorbing(d->character)) < (GET_MAX_MOVE(d->character) / 15) &&
          getCurKI(char_get_absorbing(d->character)) < (GET_MAX_MANA(d->character) / 15)) {
        act("@WYou stop absorbing stamina and ki from @c$N as they don't have enough for you to take@W!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_CHAR);
        act("@C$n@W stops absorbing stamina and ki from you!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_VICT);
        act("@C$n@W stops absorbing stamina and ki from @c$N@w!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_NOTVICT);
        if (!char_get_fighting(d->character) || char_get_fighting(d->character) != char_get_absorbing(d->character)) {
          set_fighting(d->character, char_get_absorbby(char_get_absorbing(d->character)));
        }
        if (!char_get_fighting(char_get_absorbby(char_get_absorbing(d->character))) || char_get_fighting(char_get_absorbby(char_get_absorbing(d->character))) != d->character) {
          set_fighting(char_get_absorbby(char_get_absorbing(d->character)), d->character);
        }
        char_set_absorbby(char_get_absorbing(d->character), NULL);
        char_set_absorbing(d->character, NULL);
      }
    }
    if (char_is_android(d->character) && char_get_absorbing(d->character) && rand_number(1, 9) >= 6) {
      if (getCurST(char_get_absorbing(d->character)) > (GET_MAX_MOVE(d->character) / 15) ||
          getCurKI(char_get_absorbing(d->character)) > (GET_MAX_MANA(d->character) / 15)) {

        incCurKI(d->character, getMaxKI(d->character) * .08, true);
        incCurST(d->character, getMaxST(d->character) * .08, true);

        decCurKI(char_get_absorbing(d->character), getMaxKI(d->character) / 20, 1);
        decCurST(char_get_absorbing(d->character), getMaxST(d->character) / 20, 1);

        act("@WYou absorb stamina and ki from @c$N@W!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_CHAR);
        act("@C$n@W absorbs stamina and ki from you!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_VICT);
        send_to_char(char_get_absorbing(d->character), "@wTry 'escape'!@n\r\n");
        act("@C$n@W absorbs stamina and ki from @c$N@w!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_NOTVICT);
        if (GET_HIT(d->character) < getEffMaxPL(d->character)) {
          incCurHealth(d->character, getMaxKI(d->character) * .04, true);
          send_to_char(d->character, "@CYou convert a portion of the absorbed energy into refilling your powerlevel.@n\r\n");
        }

        if (isFullST(d->character) && isFullKI(d->character)) {

          act("@WYou stop absorbing stamina and ki from @c$N as you are full@W!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_CHAR);
          act("@C$n@W stops absorbing stamina and ki from you!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_VICT);
          act("@C$n@W stops absorbing stamina and ki from @c$N@w!@n", TRUE, d->character, 0, char_get_absorbing(d->character), TO_NOTVICT);
          if (!char_get_fighting(d->character) || char_get_fighting(d->character) != char_get_absorbing(d->character)) {
            set_fighting(d->character, char_get_absorbby(char_get_absorbing(d->character)));
          }
          if (!char_get_fighting(char_get_absorbby(char_get_absorbing(d->character))) || char_get_fighting(char_get_absorbby(char_get_absorbing(d->character))) != d->character) {
            set_fighting(char_get_absorbby(char_get_absorbing(d->character)), d->character);
          }
          char_set_absorbby(char_get_absorbing(d->character), NULL);
          char_set_absorbing(d->character, NULL);
        }
        bool sum                 = !is_soft_cap(d->character, 0, 1.0);
        bool mum                 = !is_soft_cap(d->character, 2, 1.0);
        bool ium                 = !is_soft_cap(d->character, 1, 1.0);
        struct char_data *leader = d->character->master ? d->character->master : d->character;
        if (sum) {
          if (rand_number(1, 8) >= 6) {
            int gain = rand_number(char_get_level(d->character) / 2, char_get_level(d->character) * 3) + (char_get_level(d->character) * 18);
            if (char_get_level(d->character) > 30) {
              gain += rand_number(char_get_level(d->character) * 2, char_get_level(d->character) * 4) + (char_get_level(d->character) * 50);
            }
            if (char_get_level(d->character) > 60) {
              gain *= 2;
            }
            if (char_get_level(d->character) > 80) {
              gain *= 3;
            }
            if (char_get_level(d->character) > 90) {
              gain *= 4;
            }
            send_to_char(d->character, "@gYou gain +@G%d@g permanent powerlevel!@n\r\n", gain);
            if (group_bonus(d->character, 2) == 7) {
              if (char_plr_flagged(leader, PLR_SENSEM)) {
                int gbonus = gain * 0.15;
                gain += gbonus;
                send_to_char(d->character, "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n", add_commas(gbonus));
              }
            }
            gainBasePL(d->character, gain, false);
          }
        }
        if (mum) {
          if (rand_number(1, 8) >= 6) {
            int gain = rand_number(char_get_level(d->character) / 2, char_get_level(d->character) * 3) + (char_get_level(d->character) * 18);
            if (char_get_level(d->character) > 30) {
              gain += rand_number(char_get_level(d->character) * 2, char_get_level(d->character) * 4) + (char_get_level(d->character) * 50);
            }
            if (char_get_level(d->character) > 60) {
              gain *= 2;
            }
            if (char_get_level(d->character) > 80) {
              gain *= 3;
            }
            if (char_get_level(d->character) > 90) {
              gain *= 4;
            }
            send_to_char(d->character, "@gYou gain +@G%d@g permanent stamina!@n\r\n", gain);
            if (group_bonus(d->character, 2) == 7) {
              if (char_plr_flagged(leader, PLR_SENSEM)) {
                int gbonus = gain * 0.15;
                gain += gbonus;
                send_to_char(d->character, "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n", add_commas(gbonus));
              }
            }
            gainBaseST(d->character, gain, false);
          }
        }
        if (ium) {
          if (rand_number(1, 8) >= 6) {
            int gain = rand_number(char_get_level(d->character) / 2, char_get_level(d->character) * 3) + (char_get_level(d->character) * 18);
            if (char_get_level(d->character) > 30) {
              gain += rand_number(char_get_level(d->character) * 2, char_get_level(d->character) * 4) + (char_get_level(d->character) * 50);
            }
            if (char_get_level(d->character) > 60) {
              gain *= 2;
            }
            if (char_get_level(d->character) > 80) {
              gain *= 3;
            }
            if (char_get_level(d->character) > 90) {
              gain *= 4;
            }
            send_to_char(d->character, "@gYou gain +@G%d@g permanent ki!@n\r\n", gain);
            if (d->character->master && group_bonus(d->character, 2) == 7) {
              if (char_plr_flagged(leader, PLR_SENSEM)) {
                int gbonus = gain * 0.15;
                gain += gbonus;
                send_to_char(d->character, "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n", add_commas(gbonus));
              }
            }
            gainBaseKI(d->character, gain, false);
          }
        }
        if (!sum) {
          if (rand_number(1, 8) >= 6) {
            int gain = 1;
            send_to_char(d->character, "@gYou gain +@G%d@g permanent powerlevel. You may need to level.@n\r\n", gain);
            gainBasePL(d->character, gain, false);
          }
        }
        if (!mum) {
          if (rand_number(1, 8) >= 6) {
            int gain = 1;
            send_to_char(d->character, "@gYou gain +@G%d@g permanent stamina. You may need to level.@n\r\n", gain);
            gainBaseST(d->character, gain, false);
          }
        }
        if (!ium) {
          if (rand_number(1, 8) >= 6) {
            int gain = 1;
            send_to_char(d->character, "@gYou gain +@G%d@g permanent ki. You may need to level.@n\r\n", gain);
            gainBaseKI(d->character, gain, false);
          }
        }
      }
    }
    if (char_get_blocks(d->character)) {
      struct char_data *vict = char_get_blocks(d->character);
      if (char_get_in_room(vict) != char_get_in_room(d->character)) {
        char_set_blocked(vict, NULL);
        char_set_blocks(d->character, NULL);
      }
    }
    if (char_get_overf(d->character) == TRUE) {
      mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "OVERFLOW: %s has caused an overflow, check for illegal activity.", char_get_name(d->character));
      char_set_overf(d->character, FALSE);
    }
    if (char_get_spam(d->character) > 0) {
      char_set_spam(d->character, 0);
    } else
      continue;
  }

  if (countch == TRUE) {
    PCOUNT    = pcoun;
    PCOUNTDAY = time(0);
  }

  if (TOPCOUNTDOWN <= 0) {
    TOPCOUNTDOWN = 60;
  }
}
