/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"

#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"


#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"

#include "dbat/boards.h"

#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"

/* local functions */
static void gen_map(struct char_data *ch, int num);
static void bringdesc(struct char_data *ch, struct char_data *tch);
static void see_plant(struct obj_data *obj, struct char_data *ch);
static double terrain_bonus(struct char_data *ch);
static void search_room(struct char_data *ch);
static void bonus_status(struct char_data *ch);
static int sort_commands_helper(const void *a, const void *b);
static void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
static void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
static void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
static void trans_check(struct char_data *ch, struct char_data *vict);
static int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
static void perform_mortal_where(struct char_data *ch, char *arg);
static void perform_immort_where(struct char_data *ch, char *arg);
static void diag_char_to_char(struct char_data *i, struct char_data *ch);
static void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);
static void look_at_char(struct char_data *i, struct char_data *ch);
static void list_one_char(struct char_data *i, struct char_data *ch);
static void list_char_to_char(struct char_data *list, struct char_data *ch);
static void look_in_direction(struct char_data *ch, int dir);
static void look_in_obj(struct char_data *ch, char *arg);
static void look_out_window(struct char_data *ch, char *arg);
static void look_at_target(struct char_data *ch, char *arg, int read);
static void search_in_direction(struct char_data * ch, int dir);
static void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode);
static void do_auto_exits2(room_rnum target_room, struct char_data *ch);
static void display_spells(struct char_data *ch, struct obj_data *obj);
static void display_scroll(struct char_data *ch, struct obj_data *obj);
static void space_to_minus(char *str);
static void free_history(struct char_data *ch, int type);
static int yesrace(int num);
static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
                          struct char_data *ch);
// definitions
ACMD(do_evolve)
{

 if (!char_is_arlian(ch) || char_is_npc(ch)) {
  send_to_char(ch, "You are not an arlian!\r\n");
  return;
 }

 char arg[MAX_INPUT_LENGTH];
 one_argument(argument, arg);

 int64_t plcost = char_get_level(ch), stcost = char_get_level(ch), kicost = char_get_level(ch);

 plcost += molt_threshold(ch) * 0.65 + (getBasePL(ch) * 0.15);
 kicost += (molt_threshold(ch) * 0.50) + (getBaseKI(ch) * 0.22);
 stcost += (molt_threshold(ch) * 0.50) + (getBaseST(ch) * 0.15);

 if (!*arg) {
  send_to_char(ch, "@D-=@YConvert Evolution Points To What?@D=-@n\r\n");
  send_to_char(ch, "@D-------------------------------------@n\r\n");
  send_to_char(ch, "@CPowerlevel  @D: @Y%s @Wpts\r\n", add_commas(plcost));
  send_to_char(ch, "@CKi          @D: @Y%s @Wpts\r\n", add_commas(kicost));
  send_to_char(ch, "@CStamina     @D: @Y%s @Wpts\r\n", add_commas(stcost));
  send_to_char(ch, "@D[@Y%s @Wpts currently@D]@n\r\n", add_commas(char_get_moltexp(ch)));
  return;
 } else if (!strcasecmp(arg, "powerlevel") || !strcasecmp(arg, "pl")) {
  if (plcost > molt_threshold(ch)) {
   send_to_char(ch, "You need a few more evolution levels before you can start upgrading powerlevel.\r\n");
   return;
  } else if (char_get_moltexp(ch) < plcost) {
   send_to_char(ch, "You do not have enough evolution experience.\r\n");
   return;
  } else {
   int64_t plgain = (getBasePL(ch)) * 0.01;

   if (plgain <= 0) {
    plgain = rand_number(1, 5);
   } else {
    plgain = rand_number(plgain, plgain * .5);
   }
   gainBasePL(ch, plgain, false);
   char_set_moltexp(ch, char_get_moltexp(ch) - (plcost));
   send_to_char(ch, "Your body evolves to make better use of the way it is now, and you feel that your body has strengthened. @D[@RPL@D: @Y+%s@D]@n\r\n", add_commas(plgain));
  }
 } else if (!strcasecmp(arg, "ki")) {
  if (kicost > molt_threshold(ch)) {
   send_to_char(ch, "You need a few more evolution levels before you can start upgrading ki.\r\n");
   return;
  } else if (char_get_moltexp(ch) < kicost) {
   send_to_char(ch, "You do not have enough evolution experience.\r\n");
   return;
  } else {
   int64_t kigain = (getBaseKI(ch)) * 0.01;

   if (kigain <= 0) {
    kigain = rand_number(1, 5);
   } else {
    kigain = rand_number(kigain, kigain * .5);
   }
   gainBaseKI(ch, kigain, false);
   char_set_moltexp(ch, char_get_moltexp(ch) - (kicost));
   send_to_char(ch, "Your body evolves to make better use of the way it is now, and you feel that your spirit has strengthened. @D[@CKi@D: @Y+%s@D]@n\r\n", add_commas(kigain));
  }
 } else if (!strcasecmp(arg, "stamina") || !strcasecmp(arg, "st")) {
  if (stcost > molt_threshold(ch)) {
   send_to_char(ch, "You need a few more evolution levels before you can start upgrading stamina.\r\n");
   return;
  } else if (char_get_moltexp(ch) < stcost) {
   send_to_char(ch, "You do not have enough evolution experience.\r\n");
   return;
  } else {
   int64_t stgain = (getBaseST(ch)) * 0.01;

   if (stgain <= 0) {
    stgain = rand_number(1, 5);
   } else {
    stgain = rand_number(stgain, stgain * .5);
   }
   gainBaseST(ch, stgain, false);
   char_set_moltexp(ch, char_get_moltexp(ch) - (stcost));
   send_to_char(ch, "Your body evolves to make better use of the way it is now, and you feel that your body has more stamina. @D[@GST@D: @Y+%s@D]@n\r\n", add_commas(stgain));
  }
 }
}

static void see_plant(struct obj_data *obj, struct char_data *ch)
{

 int water = obj_get_value(obj, VAL_WATERLEVEL);

 if (water >= 0) {
  switch (obj_get_value(obj, VAL_MATURITY)) {
   case 0:
    send_to_char(ch, "@wA @G%s@y seed@w has been planted here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   case 1:
    send_to_char(ch, "@wA very young @G%s@w has sprouted from a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   case 2:
    send_to_char(ch, "@wA half grown @G%s@w is in a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   case 3:
    send_to_char(ch, "@wA mature @G%s@w is growing in a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   case 4:
    send_to_char(ch, "@wA mature @G%s@w is flowering in a planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   case 5:
    send_to_char(ch, "@wA mature @G%s@w that is close to harvestable is here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   case 6:
    send_to_char(ch, "@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d Water Hours@D)@n\r\n", obj->short_description, water);
    break;
   default:
    break;
  }
 } else {
  if (water > -4) {
   send_to_char(ch, "@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n", obj->short_description);
  } else if (water > -10) {
   send_to_char(ch, "@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n", obj->short_description);
  } else if (water <= -10) {
   send_to_char(ch, "@yA @G%s@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n", obj->short_description);
  }
 }

}

/* This is used to determine the terrain bonus for search_room - Iovan 12/16/2012*/
static double terrain_bonus(struct char_data *ch)
{

 double bonus = 0.0;

 switch (rroom_sect(char_get_in_room(ch))) {
  case SECT_FOREST:
   bonus += 0.5;
   break;
  case SECT_SPACE:
   bonus += -0.5;
   break;
  case SECT_WATER_NOSWIM:
   bonus += 0.25;
   break;
  case SECT_MOUNTAIN:
   bonus += 0.1;
   break;
  default:
   bonus += 0.0;
   break;
 }

 if (rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) {
  bonus += -0.5;
 }

 return (bonus);

}


/* This is used to find hidden people in a room with search - Iovan 12/16/2012 */
static void search_room(struct char_data *ch)
{

 struct char_data *vict, *next_v;
 int perc = (char_get_aff_abils_intel(ch) * 0.6) + char_get_skill(ch, SKILL_SPOT) + char_get_skill(ch, SKILL_SEARCH) + char_get_skill(ch, SKILL_LISTEN);
 int prob = 0, found = 0;
 double bonus = 1.0, terrain = 1.0;

 if ((getCurST(ch)) < GET_MAX_MOVE(ch) * 0.001) {
  send_to_char(ch, "You do not have enough stamina.\r\n");
  return;
 }

 if (char_get_skill(ch, SKILL_SENSE)) {
   bonus += (char_get_skill(ch, SKILL_SENSE) * 0.01);
 }

 reveal_hiding(ch, 0);
 act("@y$n@Y begins searching the room carefully.@n", TRUE, ch, 0, 0, TO_ROOM);
 WAIT_STATE(ch, PULSE_1SEC);

 for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
  next_v = vict->next_in_room;
  if (char_aff_flagged(vict, AFF_HIDE) && vict != ch) {
   if (char_get_suppression(vict) >= 1) {
    perc *= (char_get_suppression(vict) * 0.01);
   }
   prob = char_get_aff_abils_dex(vict) + (char_get_aff_abils_intel(vict) * 0.6) + char_get_skill(vict, SKILL_HIDE) + char_get_skill(vict, SKILL_MOVE_SILENTLY);

   if (char_aff_flagged(vict, AFF_LIQUEFIED)) {
    prob *= 1.5;
   }
   if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4)) {
    perc += 5;
   }
   if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 5 || char_get_genome(vict, 1) == 5)) {
    prob += 10;
   }
   terrain += terrain_bonus(vict);
   if (perc * bonus >= prob * terrain) { /* Found them. */
    act("@YYou find @y$N@Y hiding nearby!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@y$n@Y has found your hiding spot!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@y$n@Y has found @y$N's@Y hiding spot!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    reveal_hiding(vict, 4);
    found++;
   }
  }
 }

 struct obj_data *obj = NULL;

 for (obj = world[char_get_in_room(ch)].contents; obj;obj=obj->next_content) {
  if (obj_extra_flagged(obj, ITEM_BURIED) && perc * bonus > rand_number(50, 200)) {
   act("@YYou uncover @y$p@Y, which had been burried here.@n", TRUE, ch, obj, 0, TO_CHAR);
   act("@y$n@Y uncovers @y$p@Y, which had burried here.@n", TRUE, ch, obj, 0, TO_ROOM);
   obj_extra_set(obj, ITEM_BURIED, false);
   found++;
  }
 }
 decCurSTPercent(ch, .001, 0);


 if (found == 0) {
  send_to_char(ch, "You find nothing hidden.\r\n");
  return;
 }

}

static const char *weapon_disp[6] = {
 "Sword",
 "Dagger",
 "Spear",
 "Club",
 "Gun",
 "Brawling"
};

ACMD(do_mimic)
{

 if (char_is_npc(ch))
  return;

 char arg[MAX_INPUT_LENGTH];
 
 one_argument(argument, arg);
 
 if (!char_get_skill(ch, SKILL_MIMIC)) {
  send_to_char(ch, "You do not know how to mimic the appearance of other races.\r\n");
  return;
 }

 int count = 0, x = 0;

 // generate a list of mimic'able races.
 bool can_mimic[NUM_RACES];
    for(int r = 0; r < NUM_RACES; r++) {
        can_mimic[r] = race_can_be_mimiced(r) && race_valid_sex(r, char_get_sex(ch));
    }

 if (!*arg) {
   send_to_char(ch, "@CMimic Menu\n@c--------------------@W\r\n");
   for(int r = 0; r < NUM_RACES; r++) {
       if (!can_mimic[r])
           continue;
       if (count == 2) {
           send_to_char(ch, "%s\n", race_name(r));
           count = 0;
       } else {
           send_to_char(ch, "%s\n", race_name(r));
           count++;
       }
   }
   send_to_char(ch, "Stop@n\r\n");
   if(ch->mimic) {
       send_to_char(ch, "You currently Mimic a %s", pc_race_types[ch->mimic]);
   }

   return;
 }

  if(!strcasecmp(arg, "stop")) {
      if(!ch->mimic) {
          send_to_char(ch, "You are not imitating another race.\r\n");
          return;
      }
      act("@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", TRUE, ch, 0, 0, TO_ROOM);
      ch->mimic = 0;
  }

  int race = RACE_UNDEFINED;
  for(int r = 0; r < NUM_RACES; r++) {
      if (can_mimic[r] && !strcasecmp(arg, race_name(r))) {
          race = r;
          break;
      }
  }
  if(race == RACE_UNDEFINED) {
      send_to_char(ch, "That is not a race you can change into. Enter mimic without arugments for the mimic menu.\r\n");
      return;
  }

  int prob = char_get_skill(ch, SKILL_MIMIC), perc = axion_dice(0);
  double mult = 1 / prob;
  int64_t cost = GET_MAX_MANA(ch) * mult;

 if (race == ch->mimic) {
	send_to_char(ch, "You are already mimicing that race. To stop enter 'mimic stop'\r\n");
	return;
 } else if ((getCurKI(ch)) < cost) {
   send_to_char(ch, "You do not have enough ki to perform the technique.\r\n");
   return;
 } else if (prob < perc) {
     decCurKI(ch, cost, 0);
   act("@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n", TRUE, ch, 0, 0, TO_ROOM);
   return;
 } else {
	char buf[MAX_STRING_LENGTH];
	ch->mimic = race;
     decCurKI(ch, cost, 0);
	sprintf(buf, "@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be %s @M%s!@n", AN(RACE(ch)), LRACE(ch));
	send_to_char(ch, "@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be %s %s.@n\r\n", AN(RACE(ch)), LRACE(ch));
	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	return;
 }
}

ACMD(do_kyodaika)
{

 if (char_is_npc(ch)) {
  return;
 }

 if (!char_is_namek(ch)) {
  send_to_char(ch, "You are not a namek!\r\n");
  return;
 }

 if (!is_affected(ch, AFF_KYODAIKA)) {
  act("@GYou growl as your body grows to ten times its normal size!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@g$n@G growls as $s body grows to ten times its normal size!@n", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "@cStrength@D: @C+5\r\n@cSpeed@D: @c-2@n\r\n");
  assign_affect(ch, AFF_KYODAIKA, 0, -1, 5, 0, 0, 0, 0, -2);
  save_char(ch);
  return;
 } else {
  act("@GYou growl as your body shrinks to its normal size!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@g$n@G growls as $s body shrinks to its normal size!@n", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "@cStrength@D: @C-5\r\n@cSpeed@D: @c+2@n\r\n");
  remove_affect(ch, AFF_KYODAIKA);
  save_char(ch);
  return;
 }

}


ACMD(do_table)
{
 struct obj_data *obj = NULL, *obj2 = NULL;
 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 
 two_arguments(argument, arg, arg2);

 if (!*arg || !*arg2) {
  send_to_char(ch, "Syntax: table (red | blue | green | yellow) (card name)");
  return;
 }

 if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[char_get_in_room(ch)].contents))) {
  send_to_char(ch, "You don't see that table here.\r\n");
  return;
 }

 if (!(obj2 = get_obj_in_list_vis(ch, arg2, NULL, obj->contains))) {
  send_to_char(ch, "That card doesn't seem to be on that table.\r\n");
  return;
 }

 char buf[200];
 sprintf(buf, "$n looks at %s on %s.\r\n", obj2->short_description, obj->short_description);
 act(buf, TRUE, ch, 0, 0, TO_ROOM);
 send_to_char(ch, "%s", obj2->action_description);
}

ACMD(do_draw)
{
 if (!char_get_sits(ch)) {
  send_to_char(ch, "You are not sitting at a duel table.\r\n");
  return;
 }

 if (obj_get_vnum(char_get_sits(ch)) < 604 || obj_get_vnum(char_get_sits(ch)) > 607) {
  send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
  return;
 }

 struct obj_data *obj = NULL, *obj2 = NULL, *obj3 = NULL, *next_obj = NULL;
 int drawn = FALSE;

 if (!(obj = get_obj_in_list_vis(ch, "case", NULL, ch->carrying))) {
  send_to_char(ch, "You don't have a case.\r\n");
  return;
 }
 for (obj2 = obj->contains; obj2; obj2 = next_obj) {
  next_obj = obj2->next_content;
  if (drawn == FALSE) {
   obj_from_obj(obj2);
   obj_to_char(obj2, ch);
   obj3 = obj2;
   drawn = TRUE;
  }
 }
 if (drawn == FALSE) {
  send_to_char(ch, "You don't have any cards in the case!\r\n");
  return;
 }
 else {
  act("$n draws a card from $s $p.\r\n", TRUE, ch, obj, 0, TO_ROOM);
  send_to_char(ch, "You draw a card.\r\n%s\r\n", obj3->action_description);
  return;
 }

}

ACMD(do_shuffle)
{

 if (!char_get_sits(ch)) {
  send_to_char(ch, "You are not sitting at a duel table.\r\n");
  return;
 }

 if (obj_get_vnum(char_get_sits(ch)) < 604 || obj_get_vnum(char_get_sits(ch)) > 607) {
  send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
  return;
 }

 struct obj_data *obj = NULL, *obj2 = NULL, *next_obj = NULL;
 int count = 0;

 if (!(obj = get_obj_in_list_vis(ch, "case", NULL, ch->carrying))) {
  send_to_char(ch, "You don't have a case.\r\n");
  return;
 }

 for (obj2 = obj->contains; obj2; obj2 = next_obj) {
  next_obj = obj2->next_content;
  if (!obj_extra_flagged(obj2, ITEM_ANTI_HIEROPHANT)) {
   continue;
  }
  count += 1;
 }
 if (count <= 0) {
  send_to_char(ch, "You don't have any cards in the case!\r\n");
  return;
 }
 int total = count;
 for (obj2 = obj->contains; obj2; obj2 = next_obj) {
  next_obj = obj2->next_content;
  obj_from_obj(obj2);
  obj_to_room(obj2, real_room(48));
 }
 while (count > 0) {
 for (obj2 = world[real_room(48)].contents; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
   if (!obj_extra_flagged(obj2, ITEM_ANTI_HIEROPHANT)) {
    continue;
   }
   if (obj2 && count > 1 && rand_number(1, 4) == 3) {
    count -= 1;
    obj_from_room(obj2);
    obj_to_obj(obj2, obj);
   } else if (obj2 && count == 1) {
    count -= 1;
    obj_from_room(obj2);
    obj_to_obj(obj2, obj);
   }
  }
 }
 send_to_char(ch, "You shuffle the cards carefully.\r\n");
 act("$n shuffles their deck.", TRUE, ch, 0, 0, TO_ROOM);
 send_to_room(char_get_in_room(ch), "There were %d cards in the deck.\r\n", total);
}

ACMD(do_hand)
{

  struct obj_data *obj, *next_obj;
  char arg[MAX_INPUT_LENGTH];
  int count = 0;

  one_argument(argument, arg);

  if (!*arg) {
   send_to_char(ch, "Syntax: hand (look | show)\r\n");
   return;
  }

 if (!strcasecmp("look", arg)) {
   send_to_char(ch, "@CYour hand contains:\r\n@D---------------------------@n\r\n");
   for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
     if (obj && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
      continue;
     }
     if (obj) {
      count += 1;
      send_to_char(ch, "%s\r\n", obj->short_description);
     }
   }
   act("$n looks at $s hand.", TRUE, ch, 0, 0, TO_ROOM);
   if (count == 0) {
    send_to_char(ch, "No cards.");
    act("There were no cards.", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (count > 7) {
    act("You have more than seven cards in your hand.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n has more than seven cards in $s hand.", TRUE, ch, 0, 0, TO_ROOM);
   } else {
    char buf[200];
    sprintf(buf, "There are %d cards in the hand.", count);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
   }
 }
 else if (!strcasecmp("show", arg)) {
   send_to_char(ch, "You show off your hand to the room.\r\n");
   act("@C$n's hand contains:\r\n@D---------------------------@n", TRUE, ch, 0, 0, TO_ROOM);
   for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
     if (obj && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
      continue;
     }
     if (obj) {
      count += 1;
      act("$p", TRUE, ch, obj, 0, TO_ROOM);
     }
   }
   if (count == 0) {
      act("No cards.", TRUE, ch, 0, 0, TO_ROOM);
   }
   if (count > 7) {
    act("You have more than seven cards in your hand.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n has more than seven cards in $s hand.", TRUE, ch, 0, 0, TO_ROOM);
   }
 }
 else {
  send_to_char(ch, "Syntax: hand (look | show)\r\n");
  return;
 }
}

ACMD(do_post)
{

 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 struct obj_data *obj;
 struct obj_data *obj2;
 
 two_arguments(argument, arg, arg2);

 if (!*arg) {
  send_to_char(ch, "Syntax: post (obj name)\n"
                   "        post (obj name) (target obj name)\r\n");
  return;
 }

 if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
  send_to_char(ch, "You don't seem to have that.\r\n");
  return;
 }

 if (obj_get_type_flag(obj) != ITEM_NOTE) {
  send_to_char(ch, "You can only post notepaper.\r\n");
  return;
 }

 if (rroom_flagged(char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(char_get_in_room(ch), ROOM_GARDEN2)) {
  send_to_char(ch, "You can not post on things in a garden.\r\n");
  return;
 }

 if (!*arg2) {
  if (rroom_sect(char_get_in_room(ch)) != SECT_INSIDE && rroom_sect(char_get_in_room(ch)) != SECT_CITY) {
   send_to_char(ch, "You are not near any general structure you can post it on.\r\n");
   return;
  }
  act("@WYou post $p@W on a nearby structure.@n", TRUE, ch, obj, 0, TO_CHAR);
  act("@C$n@W posts $p@W on a nearby structure.@n", TRUE, ch, obj, 0, TO_ROOM);
  obj_from_char(obj);
  obj_to_room(obj, char_get_in_room(ch));
  obj_set_posttype(obj, 1);
  return;
 } else {
   if (!(obj2 = get_obj_in_list_vis(ch, arg2, NULL, world[char_get_in_room(ch)].contents))) {
    send_to_char(ch, "You can't seem to find the thing you want to post it on.\r\n");
    return;
   } else if (obj_get_posted_to(obj2)) {
    send_to_char(ch, "It already has something posted on it. Get that first if you want to post.\r\n");
    return;
   } else if (obj_get_type_flag(obj2) == ITEM_BOARD) {
    send_to_char(ch, "Boards come with their own means of posting messages.\r\n");
    return;
   } else {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "@C$n@W posts %s@W on %s@W.@n", obj->short_description, obj2->short_description);
    send_to_char(ch, "@WYou post %s@W on %s@W.@n\r\n", obj->short_description, obj2->short_description);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);   
    obj_from_char(obj);
    obj_to_room(obj, char_get_in_room(ch));
    obj_set_posttype(obj, 2);
    obj_set_posted_to(obj, obj2);
    obj_set_posted_to(obj2, obj);
    return;
   }
 }

}

ACMD(do_play)
{

 if (char_get_position(ch) != POS_SITTING) {
  send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
  return;
 }

 if (!char_get_sits(ch)) {
  send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
  return;
 }

 if (obj_get_vnum(char_get_sits(ch)) < 604 || obj_get_vnum(char_get_sits(ch)) > 607) {
  send_to_char(ch, "You need to be sitting at an official table to play.\r\n");
  return;
 }

 struct obj_data *obj = NULL, *obj2 = NULL, *obj3 = NULL, *next_obj = NULL;
 char arg[MAX_INPUT_LENGTH];
 one_argument(argument, arg);

 if (!*arg) {
  send_to_char(ch, "Syntax: play (card name)");
  return;
 }

 if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
  send_to_char(ch, "You don't have that card to play.\r\n");
  return;
 }

 for (obj3 = world[char_get_in_room(ch)].contents; obj3; obj3 = next_obj) {
     next_obj = obj3->next_content;
  if (obj_get_vnum(obj3) == obj_get_vnum(char_get_sits(ch)) - 4) {
   obj2 = obj3;
  }
 }

 if (obj2 == NULL) {
  send_to_char(ch, "Your table is missing. Inform an immortal of this problem.\r\n");
  return;
 }

 act("You play $p on your table.", TRUE, ch, obj, 0, TO_CHAR);
 act("$n plays $p on $s table.", TRUE, ch, obj, 0, TO_ROOM);
 obj_from_char(obj);
 obj_to_obj(obj, obj2);
}

/* Nickname an object */
ACMD(do_nickname)
{
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
   send_to_char(ch, "Syntax: nickname (object) (nickname)\n");
   send_to_char(ch, "Syntax: nickname ship (nickname)\n");
   return;
  }

  if (strcasecmp(arg, "ship")) {
   if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't have that item to nickname.\r\n");
    return;
   }
  }
  if (strlen(arg2) > 20) {
   send_to_char(ch, "You can't nickname items with any name longer than 20 characters.\r\n");
   return;
  }

  if (!strcasecmp(arg, "ship")) {
   struct obj_data *ship = NULL, *next_obj = NULL, *ship2 = NULL; 
   int found = FALSE;
   for (ship = world[char_get_in_room(ch)].contents; ship; ship = next_obj) {
    next_obj = ship->next_content;
    if (obj_get_vnum(ship) >= 45000 && obj_get_vnum(ship) <= 45999 && found == FALSE) {
     found = TRUE;
     ship2 = ship;
    }
   }
   if (found == TRUE) {
    if (strstr(arg2, "@")) {
     send_to_char(ch, "You can't nickname a ship and use color codes. Sorry.\r\n");
     return;
    } else {
     char nick[MAX_INPUT_LENGTH];
     sprintf(nick, "%s", CAP(arg2));
     ship2->action_description = strdup(nick);
     struct obj_data *k;
     for (k = object_list; k; k = k->next) {
      if (obj_get_vnum(k) == obj_get_vnum(ship2) + 1000) {
       extract_obj(k);
       int was_in = rroom_get_vnum(obj_get_in_room(ship2));
       obj_from_room(ship2);
       obj_to_room(ship2, real_room(was_in));
      }
     }
    }
   }
   return;
  }
  
  if (strstr(obj->short_description, "nicknamed")) {
   send_to_char(ch, "%s@w has already been nicknamed.@n\r\n", obj->short_description);
   return;
  } else if (strstr(obj->name, "corpse")) {
    send_to_char(ch, "%s@w is a corpse!@n\r\n", obj->short_description);
    return;
  } else {
   send_to_char(ch, "@wYou nickname %s@w as '@C%s@w'.@n\r\n", obj->short_description, arg2);
   char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH];
   sprintf(nick, "%s @wnicknamed @D(@C%s@D)@n", obj->short_description, CAP(arg2));
   sprintf(nick2, "%s %s", obj->name, arg2);
   obj->short_description = strdup(nick);
   obj->name = strdup(nick2);
   return;
  } 
}


/* local globals */
int *cmd_sort_info;

/* Portal appearance types */
static const char *portal_appearance[] = {
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
    "\n"
};


ACMD(do_showoff)
{
  struct obj_data *obj = NULL;
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  *arg = '\0';
  *arg2 = '\0';

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
   send_to_char(ch, "You want to show what item to what character?\r\n");
   return;
  }

 if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
  send_to_char(ch, "You don't seem to have that.\r\n");
  return;
 } else if (!(vict = get_player_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "There is no such person around.\r\n");
   return;
 } else { /* Ok show that target the object! */
  act("@WYou hold up $p@W for @C$N@W to see:@n", TRUE, ch, obj, vict, TO_CHAR);
  act("@C$n@W holds up $p@W for you to see:@n", TRUE, ch, obj, vict, TO_VICT);
  act("@C$n@W holds up $p@W for @c$N@W to see.@n", TRUE, ch, obj, vict, TO_NOTVICT);
  show_obj_to_char(obj, vict, SHOW_OBJ_ACTION);
  return;
 }
}

/* Used for checking if you know the character in question. */
void introCreate(struct char_data *ch)
{
  char fname[40];
  FILE *fl;
  /* Write Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch)))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save user, %s, to filename, %s.", char_get_name(ch), fname);
    return;
  }

  fprintf(fl, "Gibbles Gibbles\n");

  fclose(fl);
  return;
}

int readIntro(struct char_data *ch, struct char_data *vict) {  
  char fname[40], filler[50], scrap[100], line[256];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */
  if (vict == NULL) {
    return 0;
  }

  if (char_is_npc(ch) || char_is_npc(vict)) {
   return 1;
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
    introCreate(ch);
  }
  if (!(fl = fopen(fname, "r"))) {
    return 2;
  }
  if (vict == ch) {
    fclose(fl);
    return 0;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%s %s\n", filler, scrap);
    if (!strcasecmp(char_get_name(vict), filler)) {
     known = TRUE;
    }
  }
   fclose(fl);

   if (known == TRUE)
    return 1;
   else
    return 0;
}

void introWrite(struct char_data *ch, struct char_data *vict, char *name)
{
  FILE *file;
  char fname[40], filler[50], scrap[100], line[256];
  char *names[500] = {""}, *alias[500] = {""};
  FILE *fl;
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
    introCreate(ch);
  }
  if (!(file = fopen(fname, "r"))) {
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%s %s\n", filler, scrap);
    names[count] = strdup(filler);
    alias[count] = strdup(scrap);
    count++;
     *filler = '\0';
     *scrap = '\0';
  }
  fclose(file);

  /* Write Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch)))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save intro file, %s, to filename, %s.", char_get_name(ch), fname);
    return;
  }

  while(x < count) {
    if (x == 0 || strcasecmp(names[x-1], names[x])) {
     if (strcasecmp(names[x], char_get_name(vict))) {
      fprintf(fl, "%s %s\n", names[x], alias[x]);
     }
    }
    x++;
  }

  x = 0;
  while (x < count) {
   if (names[x] != NULL) {
    free(names[x]);
   } if (alias[x] != NULL) {
    free(alias[x]);
   }
   x++;
  }

  fprintf(fl, "%s %s\n", char_get_name(vict), CAP(name));

  fclose(fl);
  return;
}

ACMD(do_intro)
{

 if (char_is_npc(ch))
  return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct char_data *vict;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
   send_to_char(ch, "Syntax: dub (target) (name)\r\nWho do you want to dub and what do you want to name them?\r\n");
   return;
  }

  if (!*arg2) {
   send_to_char(ch, "Syntax: dub (target) (name)\r\nWhat name do you wish to know them by?\r\n");
   return;
  }

  if (strlen(arg2) > 20) {
   send_to_char(ch, "Limit the name to 20 characters.\r\n");
   return;
  }
  if (strlen(arg2) < 3) {
   send_to_char(ch, "Limit the name to at least 3 characters.\r\n");
   return;
  }

  if (strstr(arg2, "$") || strstr(arg2, "@") || strstr(arg2, "%")) {
   send_to_char(ch, "Illegal character. No symbols.\r\n");
   return;
  }

  if (!(vict = get_player_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "There is no such person.\r\n");
   return;
  }

  if (vict == ch) {
   send_to_char(ch, "That seems rather odd.\r\n");
   return;
  }

  if (char_is_npc(vict)) {
   send_to_char(ch, "That seems rather unwise.\r\n");
   return;
  }

  if (readIntro(vict, ch) == 2) {
   send_to_char(ch, "There seems to have been an error, report this to Iovan.\r\n");
   return;
  }
  else if (readIntro(ch, vict) == 1 && strstr(RACE(vict), arg)) {
   send_to_char(ch, "You have already dubbed them a name. If you want to redub them target the name you know them by.\r\n");
   return;
  }
  else {
   introWrite(ch, vict, arg2);
   act("You decide to call $M, $N.", TRUE, ch, 0, vict, TO_CHAR);
   act("$n seems to decide something about you.", TRUE, ch, 0, vict, TO_VICT);
   act("$n seems to decide something about $N.", TRUE, ch, 0, vict, TO_NOTVICT);
   return;
  }
}

/* Used when checking status or looking at a character */
static void bringdesc(struct char_data *ch, struct char_data *tch)
{

  if (ch != NULL &&  tch != NULL && char_is_humanoid(tch)) {

  if (ch != tch && char_plr_flagged(tch, PLR_DISGUISED)) {
   send_to_char(ch, "            @D[@cHair Length @D: @WHidden.         @D]@n\r\n");
   send_to_char(ch, "            @D[@cHair Color  @D: @WHidden.         @D]@n\r\n");
   send_to_char(ch, "            @D[@cHair Style  @D: @WHidden.         @D]@n\r\n");
   send_to_char(ch, "            @D[@cEye Color   @D: @WHidden.         @D]@n\r\n");
   if (char_get_skin(tch) == SKIN_WHITE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_TAN) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_BLACK) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_GREEN) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_ORANGE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_YELLOW) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_RED) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_GREY) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_BLUE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_AQUA) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_PINK) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_PURPLE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
   }
   return;
  }

  if (char_is_human(tch) || char_is_saiyan(tch) || char_is_konatsu(tch) || char_is_mutant(tch) || char_is_android(tch) || char_is_kai(tch) || char_is_halfbreed(tch) || char_is_truffle(tch) || char_is_hoshijin(tch)) {
  if ((!char_is_saiyan(tch) && !char_is_halfbreed(tch)) || ((char_is_saiyan(tch) || char_is_halfbreed(tch)) && !char_is_transformed(tch))) {
   if (char_get_hairl(tch) == HAIRL_LONG) {
    send_to_char(ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
   }
   else if (char_get_hairl(tch) == HAIRL_BALD) {
    send_to_char(ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
   }
   else if (char_get_hairl(tch) == HAIRL_SHORT) {
    send_to_char(ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
   }
   else if (char_get_hairl(tch) == HAIRL_MEDIUM) {
    send_to_char(ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
   }
   else if (char_get_hairl(tch) == HAIRL_RLONG) {
    send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
   }
   if (char_get_hairs(tch) == HAIRS_PLAIN) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WPlain.        @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_MOHAWK) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WMohawk.       @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_SPIKY) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_CURLY) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WCurly.        @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_UNEVEN) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WUneven.       @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_PONYTAIL) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WPony Tail.    @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_AFRO) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WAfro.         @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_FADE) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WFade.         @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_CREW) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WCrew Cut.     @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_FEATHERED) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WFeathered.    @D]@n\r\n");
   }
   else if (char_get_hairs(tch) == HAIRS_DRED) {
    send_to_char(ch, "            @D[@cHair Style  @D: @WDread Locks.  @D]@n\r\n");
   }
   if (char_get_hairc(tch) == HAIRC_BLACK) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WBlack.        @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_BROWN) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WBrown.        @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_BLONDE) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WBlonde.       @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_GREY) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WGrey.         @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_RED) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WRed.          @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_ORANGE) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WOrange.       @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_GREEN) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WGreen.        @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_BLUE) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WBlue.         @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_PINK) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WPink.         @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_PURPLE) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WPurple.       @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_SILVER) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WSilver.       @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_CRIMSON) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WCrimson.      @D]@n\r\n");
   }
   else if (char_get_hairc(tch) == HAIRC_WHITE) {
    send_to_char(ch, "            @D[@cHair Color  @D: @WWhite.        @D]@n\r\n");
    }
   }
    else if (char_is_saiyan(tch) || char_is_halfbreed(tch)) {
     if (char_plr_flagged(tch, PLR_TRANS1)) {
      if (char_get_hairl(tch) == HAIRL_LONG) {
       send_to_char(ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_BALD) {
       send_to_char(ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_SHORT) {
       send_to_char(ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_MEDIUM) {
       send_to_char(ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_RLONG) {
       send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
      }
      send_to_char(ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
      send_to_char(ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
     }
     else if (char_plr_flagged(tch, PLR_TRANS2)) {
      if (char_get_hairl(tch) == HAIRL_LONG) {
       send_to_char(ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_BALD) {
       send_to_char(ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_SHORT) {
       send_to_char(ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_MEDIUM) {
       send_to_char(ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
      }
      else if (char_get_hairl(tch) == HAIRL_RLONG) {
       send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
      }
      send_to_char(ch, "            @D[@cHair Style  @D: @WSharp Spikes. @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
      send_to_char(ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
     }
     else if (char_plr_flagged(tch, PLR_TRANS3)) {
      send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
      send_to_char(ch, "            @D[@cEye Color   @D: @WAqua Green.   @D]@n\r\n");
     }
     else if (char_plr_flagged(tch, PLR_TRANS4)) {
      send_to_char(ch, "            @D[@cHair Length @D: @WLong.        @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Style  @D: @WSoft Spikes. @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Color  @D: @WBlack.       @D]@n\r\n");
      send_to_char(ch, "            @D[@cEye Color   @D: @WAmber.       @D]@n\r\n");
     }
    }
   }
   if (char_is_demon(tch) || char_is_icer(tch)) {
    if (char_get_hairl(tch) == HAIRL_BALD) {
     send_to_char(ch, "            @D[@cHorn Length @D: @WNone.         @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_SHORT) {
     send_to_char(ch, "            @D[@cHorn Length @D: @WShort.        @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_MEDIUM) {
     send_to_char(ch, "            @D[@cHorn Length @D: @WMedium.       @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_LONG) {
     send_to_char(ch, "            @D[@cHorn Length @D: @WLong.         @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_RLONG) {
     send_to_char(ch, "            @D[@cHorn Length @D: @WReally Long.  @D]@n\r\n");
    }
   }
   if (char_is_namek(tch) || char_is_arlian(tch)) {
    if (char_get_hairl(tch) == HAIRL_BALD) {
     send_to_char(ch, "            @D[@cAnt. Length @D: @WTiny.        @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_SHORT) {
     send_to_char(ch, "            @D[@cAnt. Length @D: @WShort.       @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_MEDIUM) {
     send_to_char(ch, "            @D[@cAnt. Length @D: @WMedium.      @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_LONG) {
     send_to_char(ch, "            @D[@cAnt. Length @D: @WLong.        @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_RLONG) {
     send_to_char(ch, "            @D[@cAnt. Length @D: @WR. Long.     @D]@n\r\n");
    }
   }
   if (char_is_arlian(tch) && char_is_female(tch)) {
    if (char_get_hairc(tch) == HAIRC_BLACK) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WBlack.        @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_BROWN) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WBrown.        @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_BLONDE) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WBlonde.       @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_GREY) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WGrey.         @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_RED) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WRed.          @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_ORANGE) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WOrange.       @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_GREEN) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WGreen.        @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_BLUE) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WBlue.         @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_PINK) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WPink.         @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_PURPLE) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WPurple.       @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_SILVER) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WSilver.       @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_CRIMSON) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WCrimson.      @D]@n\r\n");
    }
    else if (char_get_hairc(tch) == HAIRC_WHITE) {
     send_to_char(ch, "            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
    }    
   } else if (char_is_arlian(tch) && !char_is_female(tch)) {
    send_to_char(ch, "            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
   }
   if (char_is_majin(tch)) {
    if (char_get_hairl(tch) == HAIRL_BALD) {
     send_to_char(ch, "            @D[@cFor. Length @D: @WTiny.         @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_SHORT) {
     send_to_char(ch, "            @D[@cFor. Length @D: @WShort.        @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_MEDIUM) {
     send_to_char(ch, "            @D[@cFor. Length @D: @WMedium.       @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_LONG) {
     send_to_char(ch, "            @D[@cFor. Length @D: @WLong.         @D]@n\r\n");
    }
    if (char_get_hairl(tch) == HAIRL_RLONG) {
     send_to_char(ch, "            @D[@cFor. Length @D: @WR. Long.      @D]@n\r\n");
    }
   }
  if ((!char_is_saiyan(tch) && !char_is_halfbreed(tch)) || ((char_is_saiyan(tch) || char_is_halfbreed(tch)) && !char_is_transformed(tch))) {
   if (char_get_eye(tch) == EYE_BLUE) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WBlue.         @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_BLACK) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WBlack.        @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_GREEN) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WGreen.        @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_BROWN) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WBrown.        @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_RED) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WRed.          @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_AQUA) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WAqua.         @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_PINK) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WPink.         @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_PURPLE) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WPurple.       @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_CRIMSON) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WCrimson.      @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_GOLD) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WGold.         @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_AMBER) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WAmber.        @D]@n\r\n");
   }
   else if (char_get_eye(tch) == EYE_EMERALD) {
    send_to_char(ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
   }
  }
   if (char_get_skin(tch) == SKIN_WHITE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_TAN) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_BLACK) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_GREEN) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_ORANGE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_YELLOW) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_RED) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_GREY) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_BLUE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_AQUA) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_PINK) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
   }
   else if (char_get_skin(tch) == SKIN_PURPLE) {
    send_to_char(ch, "            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
   }
  if (char_get_majinize(tch) != 0 && char_get_majinize(tch) != 3) {
    send_to_char(ch, "            @D[@cForehead    @D: @mMajin Symbol  @D]@n\r\n");
  }
  }
  else if (!char_is_humanoid(tch)) {
    /* Display nothing */
    return;
   }
  else {
   send_to_char(ch, "Error in bring-desc, please report.\r\n");
  }
}

static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
struct char_data *ch)
{
  int door;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (world[rnum].dir_option[door] && world[rnum].dir_option[door]->to_room != NOWHERE && (room_exit_flagged(world[rnum].dir_option[door], EX_CLOSED) && !room_exit_flagged(world[rnum].dir_option[door], EX_SECRET))) {
     switch (door) {
      case NORTH:
       map[y-1][x] = '8';
      break;
      case EAST:
       map[y][x+1] = '8';
      break;
      case SOUTH:
       map[y+1][x] = '8';
      break;
      case WEST:
       map[y][x-1] = '8';
      break;
      case NORTHEAST:
       map[y-1][x+1] = '8';
      break;
      case NORTHWEST:
       map[y-1][x-1] = '8';
      break;
      case SOUTHEAST:
       map[y+1][x+1] = '8';
      break;
      case SOUTHWEST:
       map[y+1][x-1] = '8';
      break;
     }
    } else if (world[rnum].dir_option[door] && world[rnum].dir_option[door]->to_room != NOWHERE && !room_exit_flagged(world[rnum].dir_option[door], EX_CLOSED)) {
      switch (door) {
        case NORTH:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y-1][x] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '2';
           } else {
            map[y-1][x] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '2';
           } else {
            map[y-1][x] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '7';
           } else {
            map[y-1][x] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '1';
           } else {
            map[y-1][x] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '6';
           } else {
            map[y-1][x] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '5';
           } else {
            map[y-1][x] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x] = '3';
           } else {
            map[y-1][x] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y-1][x] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y-1][x] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y-1][x] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y-1][x] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y-1][x] = '*';
          }
          else {
           map[y-1][x] = '-';
          }
         break;
        case EAST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y][x+1] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '2';
           } else {
            map[y][x+1] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '2';
           } else {
            map[y][x+1] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '7';
           } else {
            map[y][x+1] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '1';
           } else {
            map[y][x+1] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '6';
           } else {
            map[y][x+1] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '5';
           } else {
            map[y][x+1] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x+1] = '3';
           } else {
            map[y][x+1] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y][x+1] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y][x+1] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y][x+1] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y][x+1] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y][x+1] = '*';
          }
          else {
           map[y][x+1] = '-';
          }
         break;
        case SOUTH:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y+1][x] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '2';
           } else {
            map[y+1][x] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '2';
           } else {
            map[y+1][x] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '7';
           } else {
            map[y+1][x] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '1';
           } else {
            map[y+1][x] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '6';
           } else {
            map[y+1][x] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '5';
           } else {
            map[y+1][x] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '3';
           } else {
            map[y+1][x] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y+1][x] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y+1][x] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y+1][x] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y+1][x] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y+1][x] = '*';
          }
          else {
           map[y+1][x] = '-';
          }
         break;
        case WEST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y][x-1] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x] = '2';
           } else {
            map[y][x-1] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x-1] = '2';
           } else {
            map[y][x-1] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x-1] = '7';
           } else {
            map[y][x-1] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x-1] = '1';
           } else {
            map[y][x-1] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x-1] = '6';
           } else {
            map[y][x-1] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x-1] = '5';
           } else {
            map[y][x-1] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y][x-1] = '3';
           } else {
            map[y][x-1] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y][x-1] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y][x-1] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y][x-1] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y][x-1] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y][x-1] = '*';
          }
          else {
           map[y][x-1] = '-';
          }
         break;
        case NORTHEAST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y-1][x+1] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '2';
           } else {
            map[y-1][x+1] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '2';
           } else {
            map[y-1][x+1] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '7';
           } else {
            map[y-1][x+1] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '1';
           } else {
            map[y-1][x+1] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '6';
           } else {
            map[y-1][x+1] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '5';
           } else {
            map[y-1][x+1] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x+1] = '3';
           } else {
            map[y-1][x+1] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y-1][x+1] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y-1][x+1] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y-1][x+1] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y-1][x+1] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y-1][x+1] = '*';
          }
          else {
           map[y-1][x+1] = '-';
          }
         break;
        case NORTHWEST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y-1][x-1] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '2';
           } else {
            map[y-1][x-1] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '2';
           } else {
            map[y-1][x-1] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '7';
           } else {
            map[y-1][x-1] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '1';
           } else {
            map[y-1][x-1] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '6';
           } else {
            map[y-1][x-1] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '5';
           } else {
            map[y-1][x-1] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y-1][x-1] = '3';
           } else {
            map[y-1][x-1] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y-1][x-1] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y-1][x-1] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y-1][x-1] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y-1][x-1] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y-1][x-1] = '*';
          }
          else {
           map[y-1][x-1] = '-';
          }
         break;
        case SOUTHEAST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y+1][x+1] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '2';
           } else {
            map[y+1][x+1] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '2';
           } else {
            map[y+1][x+1] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '7';
           } else {
            map[y+1][x+1] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '1';
           } else {
            map[y+1][x+1] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '6';
           } else {
            map[y+1][x+1] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '5';
           } else {
            map[y+1][x+1] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x+1] = '3';
           } else {
            map[y+1][x+1] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y+1][x+1] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y+1][x+1] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y+1][x+1] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y+1][x+1] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y+1][x+1] = '*';
          }
          else {
           map[y+1][x+1] = '-';
          }
         break;
        case SOUTHWEST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
           map[y+1][x-1] = '=';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '2';
           } else {
            map[y+1][x-1] = 'i';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '2';
           } else {
            map[y+1][x-1] = 'p';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '7';
           } else {
            map[y+1][x-1] = '!';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '1';
           } else {
            map[y+1][x-1] = '(';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '6';
           } else {
            map[y+1][x-1] = 'f';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '5';
           } else {
            map[y+1][x-1] = '^';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
           if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
            map[y+1][x-1] = '3';
           } else {
            map[y+1][x-1] = 'h';
           }
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
           map[y+1][x-1] = 's';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
           map[y+1][x-1] = '`';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
           map[y+1][x-1] = '+';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
           map[y+1][x-1] = '&';
          }
          else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
           map[y+1][x-1] = '*';
          }
          else {
           map[y+1][x-1] = '-';
          }
         break;
      }
    }
  }
}

ACMD(do_map)
{
 gen_map(ch, 1);
}

static void gen_map(struct char_data *ch, int num)
{
  int door, i;
  char map[9][10] = {{'-'}, {'-'}};
  char buf2[MAX_INPUT_LENGTH];

  if (num == 1) {
  /* Map Key */
   send_to_char(ch, "@W               @D-[@CArea Map@D]-\r\n");
   send_to_char(ch, "@D-------------------------------------------@w\r\n");
   send_to_char(ch, "@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
   send_to_char(ch, "@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
   send_to_char(ch, "@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
   send_to_char(ch, "@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
   send_to_char(ch, "@WLastly @RX@W = You.\r\n");
   send_to_char(ch, "@D-------------------------------------------\r\n");
   send_to_char(ch, "@D                  @CNorth@w\r\n");
   send_to_char(ch, "@D                    @c^@w\r\n");
   send_to_char(ch, "@D             @CWest @c< O > @CEast@w\r\n");
   send_to_char(ch, "@D                    @cv@w\r\n");
   send_to_char(ch, "@D                  @CSouth@w\r\n");
   send_to_char(ch, "@D                ---------@w\r\n");
  }

  /* blank the map */
  for(i=0;i<9;i++) {
    strcpy(map[i], "         ");
  }

  /* print out exits */
  map_draw_room(map, 4, 4, ch->in_room, ch);
  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (char_get_exit(ch, door) && char_get_exit(ch, door)->to_room != NOWHERE && !room_exit_flagged(char_get_exit(ch, door), EX_CLOSED)) {
      switch (door) {
        case NORTH:
          map_draw_room(map, 4, 3,char_get_exit(ch, door)->to_room, ch);
         break;
        case EAST:
          map_draw_room(map, 5, 4,char_get_exit(ch, door)->to_room, ch);
         break;
        case SOUTH:
          map_draw_room(map, 4, 5,char_get_exit(ch, door)->to_room, ch);
         break;
        case WEST:
          map_draw_room(map, 3, 4,char_get_exit(ch, door)->to_room, ch);
         break;
        case NORTHEAST:
          map_draw_room(map, 5, 3,char_get_exit(ch, door)->to_room, ch);
         break;
        case NORTHWEST:
          map_draw_room(map, 3, 3,char_get_exit(ch, door)->to_room, ch);
         break;
        case SOUTHEAST:
          map_draw_room(map, 5, 5,char_get_exit(ch, door)->to_room, ch);
         break;
        case SOUTHWEST:
          map_draw_room(map, 3, 5,char_get_exit(ch, door)->to_room, ch);
         break;
      }
    }
  }

  /* make it obvious what room they are in */
  map[4][4] = 'x';

  /* print out the map */
  int key = 0;
  *buf2 = '\0';
  for(i=2;i<9;i++) {
     if (i > 6) {
      continue;
     }
    if (num == 1) {
     sprintf(buf2, "@w                %s\r\n", map[i]);
    }
    else {
     if (i == 2) {
      sprintf(buf2, "@w       @w|%s@w|           %s", (char_get_exit(ch, 0) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 0), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 0), EX_CLOSED) ? " @rN " : " @CN ") : "   ", map[i]);
     }
     if (i == 3) {
      sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s", (char_get_exit(ch, 6) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 6), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 6), EX_CLOSED) ? " @rNW" : " @CNW") : "   ", (char_get_exit(ch, 4) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 4), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 4), EX_CLOSED) ? " @yU " : " @YU ") : "   ", (char_get_exit(ch, 7) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 7), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 7), EX_SECRET) ? "@rNE " : "@CNE ") : "   ", map[i]);
     }
     if (i == 4) {
      sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s", (char_get_exit(ch, 3) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 3), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 3), EX_CLOSED) ? "  @rW" : "  @CW") : "   ", (char_get_exit(ch, 10) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 10), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 10), EX_CLOSED) ? " @rI " : " @mI ") : ((char_get_exit(ch, 11) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 11), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 11), EX_CLOSED) ? "@rOUT" : "@mOUT") : "@r{ }"), (char_get_exit(ch, 1) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 1), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 1), EX_CLOSED) ? "@rE  " : "@CE  ") : "   ", map[i]);
     }
     if (i == 5) {
      sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s", (char_get_exit(ch, 9) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 9), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 9), EX_CLOSED) ? " @rSW" : " @CSW") : "   ", (char_get_exit(ch, 5) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 5), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 5), EX_CLOSED) ? " @yD " : " @YD ") : "   ", (char_get_exit(ch, 8) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 8), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 8), EX_SECRET) ? "@rSE " : "@CSE ") : "   ", map[i]);
     }
     if (i == 6) {
      sprintf(buf2, "@w       @w|%s@w|           %s", (char_get_exit(ch, 2) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 2), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 2), EX_CLOSED) ? " @rS " : " @CS ") : "   ", map[i]);
     }
    }
     search_replace(buf2, "x", "@RX");
     search_replace(buf2, "&", "@m$");
     search_replace(buf2, "*", "@m#");
     search_replace(buf2, "+", "@c~");
     search_replace(buf2, "s", "@CS");
     search_replace(buf2, "i", "@wI");
     search_replace(buf2, "(", "@WC");
     search_replace(buf2, "^", "@DM");
     search_replace(buf2, "h", "@yH");
     search_replace(buf2, "`", "@BW");
     search_replace(buf2, "=", "@bU");
     search_replace(buf2, "p", "@GP");
     search_replace(buf2, "f", "@gF");
     search_replace(buf2, "!", "@YD");
     search_replace(buf2, "-", "@w:");
     /* ------- Do Lava Rooms ------- */
     search_replace(buf2, "1", "@4@YC@n");
     search_replace(buf2, "2", "@4@YP@n");
     search_replace(buf2, "3", "@4@YH@n");
     search_replace(buf2, "7", "@4@YD@n");
     search_replace(buf2, "5", "@4@YM@n");
     search_replace(buf2, "6", "@4@YF@n");
     /* ------- Do Closed Rooms------- */
     search_replace(buf2, "8", "@1 @n");

    if (num != 1) {
     if (key == 0) {
      send_to_char(ch, "%s    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n", buf2);
     }
     if (key == 1) {
      send_to_char(ch, "%s    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n", buf2);
     }
     if (key == 2) {
      send_to_char(ch, "%s    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n", buf2);
     }
     if (key == 3) {
      send_to_char(ch, "%s    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n", buf2);
     }
     if (key == 4) {
      send_to_char(ch, "%s    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n", buf2);
     }
     key += 1;
    }
    else {
      send_to_char(ch, buf2);
    }
  }
  if (num == 1) {
   send_to_char(ch, "@D                ---------@w\r\n");
  }
}


static void display_spells(struct char_data *ch, struct obj_data *obj)
{
  int i;

  send_to_char(ch, "The spellbook contains the following spells:\r\n");
  send_to_char(ch, "@c---@wSpell Name@c------------------------------------@w# of pages@c-----@n\r\n");

  if (!obj->sbinfo) {
    return;
  }
  for (i=0; i < SPELLBOOK_SIZE; i++) {
    if (obj->sbinfo[i].spellname != 0) {
      if (obj->sbinfo[i].spellname > MAX_SPELLS) {
        continue;
      }
      send_to_char(ch, "@y%-20s@n					[@R%2d@n]\r\n", spell_info[obj->sbinfo[i].spellname].name ,obj->sbinfo[i].pages);
    }
  }
  return;
}

static void display_scroll(struct char_data *ch, struct obj_data *obj)
{
  send_to_char(ch, "The scroll contains the following spell:\r\n");
  send_to_char(ch, "@c---@wSpell Name@c---------------------------------------------------@n\r\n");
  send_to_char(ch, "@y%-20s@n\r\n", skill_name(obj_get_value(obj, VAL_SCROLL_SPELL1)));
  return;
}

static void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    /*  SYSERR_DESC:
     *  Somehow a NULL pointer was sent to show_obj_to_char() in either the
     *  'obj' or the 'ch' variable.  The error will indicate which was NULL
     *  be listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger.
     */
    return;
  }

  int spotted = FALSE;

  if (char_get_skill(ch, SKILL_SPOT) > rand_number(20, 110)) {
   spotted = TRUE;
  }

  switch (mode) {
  case SHOW_OBJ_LONG:
    /*
     * hide objects starting with . from non-holylighted people 
     * Idea from Elaseth of TBA 
     */
    if (*obj->description == '.' && (char_is_npc(ch) || !char_prf_flagged(ch, PRF_HOLYLIGHT))) 
      return;
    if (obj_get_type_flag(obj) == ITEM_VEHICLE && rroom_get_vnum(char_get_in_room(ch)) == obj_get_value(obj, 0)) {
      return;
    }
    if (obj_get_sitting(obj) && char_get_admlevel(ch) < 1) {
      return;
    }
    if (obj_get_sitting(obj) && char_get_admlevel(ch) >= 1) {
      send_to_char(ch, "@D(@YBeing Used@D)@w");
    }
    if (obj_get_type_flag(obj) == ITEM_PLANT && (rroom_flagged(obj_get_in_room(obj), ROOM_GARDEN1) || rroom_flagged(obj_get_in_room(obj), ROOM_GARDEN2))) {
      see_plant(obj, ch);
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BURIED)) {
     char bury[MAX_INPUT_LENGTH];
     if (!obj_is_corpse(obj)) {
      if (obj_get_weight(obj) < 10) {
       sprintf(bury, "small mound of");
      } else if (obj_get_weight(obj) < 50) {
       sprintf(bury, "medium sized mound of");
      } else if (obj_get_weight(obj) < 1000) {
       sprintf(bury, "large mound of");
      } else {
       sprintf(bury, "gigantic mound of");
      }
     } else {
      sprintf(bury, "recent grave covered by");
     }
     if (spotted == TRUE && rroom_sect(obj_get_in_room(obj)) != SECT_DESERT) {
      send_to_char(ch, "@yA %s soft dirt is here.@n\r\n", bury);
      return;
     } else if (spotted == TRUE && rroom_sect(obj_get_in_room(obj)) == SECT_DESERT) {
      send_to_char(ch, "@YA %s soft sand is here.@n\r\n", bury);
      return;
     } else {
      return;
     }
    }
    if (obj_get_vnum(obj) == 11) {
     send_to_char(ch, "@wA gravity generator, set to %sx gravity, is built here", add_commas(obj_get_weight(obj)));
    } else if (obj_get_vnum(obj) == 79) {
     send_to_char(ch, "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction", add_commas(obj_get_weight(obj)), dirs[obj_get_cost(obj)]);
    }
    else {
    send_to_char(ch, "@w");
    if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS))  {
     if (obj_get_posted_to(obj) == NULL) {
      send_to_char(ch, "@D[@G%d@D]@w ", obj_get_vnum(obj));
      if (SCRIPT(obj))
        send_to_char(ch, "@D[@wT%d@D]@w ", obj->proto_script->vnum);
     } else {
      if (obj_get_posttype(obj) <= 0) {
       send_to_char(ch, "@D[@G%d@D]@w ", obj_get_vnum(obj));
       if (SCRIPT(obj))
         send_to_char(ch, "@D[@wT%d@D]@w ", obj->proto_script->vnum);
      }
     }
    }

    if (obj_get_posttype(obj) > 0) {
     if (obj_get_posted_to(obj)) {
      return;
     } else {
      send_to_char(ch, "%s@w, has been posted here.@n", obj->short_description);
     }
    } else {
     if (!obj_extra_flagged(obj, ITEM_BURIED)) {
      send_to_char(ch, "%s@n", obj->description);
     }
    }

     if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
      if (!obj_val_flagged(obj, CONT_CLOSED) && obj_get_vnum(obj) > 19199)
       send_to_char(ch, "\r\n@c...its outer hatch is open@n");
      else if (!obj_val_flagged(obj, CONT_CLOSED) && obj_get_vnum(obj) <= 19199)
       send_to_char(ch, "\r\n@c...its door is open@n");
     }
     if (obj_get_type_flag(obj) == ITEM_CONTAINER && !obj_is_corpse(obj)) {
      if (!obj_val_flagged(obj, CONT_CLOSED) && !obj_extra_flagged(obj, ITEM_SHEATH))
       send_to_char(ch, ". @D[@G-open-@D]@n");
      else if (!obj_extra_flagged(obj, ITEM_SHEATH))
       send_to_char(ch, ". @D[@rclosed@D]@n");
     }
     if (obj_get_type_flag(obj) == ITEM_HATCH) {
      if (!obj_val_flagged(obj, CONT_CLOSED))
       send_to_char(ch, ", it is open");
      else if (obj_val_flagged(obj, CONT_CLOSED))
       send_to_char(ch, ", it is closed");
      if (obj_val_flagged(obj, CONT_LOCKED))
       send_to_char(ch, " and locked@n");
      else
       send_to_char(ch, "@n");
     }
     if (obj_get_type_flag(obj) == ITEM_FOOD) {
      if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
       send_to_char(ch, ", and it has been ate on@n");
      }
     }
    }
    break;

  case SHOW_OBJ_SHORT:
    if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS))  {
      send_to_char(ch, "[%d] ", obj_get_vnum(obj));
      if (SCRIPT(obj))
        send_to_char(ch, "[T%d] ", obj->proto_script->vnum);
    }
 
    if (char_prf_flagged(ch, PRF_IHEALTH)) {   
     send_to_char(ch, "@D<@gH@D: @C%d@D>@w %s", obj_get_value(obj, VAL_ALL_HEALTH), obj->short_description);
    } else {
     send_to_char(ch, "%s", obj->short_description);
    }
     if (obj_get_type_flag(obj) == ITEM_FOOD) {
      if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
       send_to_char(ch, ", and it has been ate on.@n");
      }
     }
     if (obj_get_vnum(obj) == 255) {
      switch (obj_get_value(obj, 0)) {
       case 0:
       case 1:
        send_to_char(ch, " @D[@wQuality @RC@D]@n");
        break;
       case 2:
        send_to_char(ch, " @D[@wQuality @RC+@D]@n");
        break;
       case 3:
        send_to_char(ch, " @D[@wQuality @yC++@D]@n");
        break;
       case 4:
        send_to_char(ch, " @D[@wQuality @yB@D]@n");
        break;
       case 5:
        send_to_char(ch, " @D[@wQuality @CB+@D]@n");
        break;
       case 6:
        send_to_char(ch, " @D[@wQuality @CB++@D]@n");
        break;
       case 7:
        send_to_char(ch, " @D[@wQuality @CA@D]@n");
        break;
       case 8:
        send_to_char(ch, " @D[@wQuality @GA+@D]@n");
        break;
      }
     }
     
     if (obj_get_vnum(obj) == 3424) {
      send_to_char(ch, " @D[@bInk Remaining@D: @w%d@D]@n", obj_get_value(obj, 6));
     }
     if (obj_get_vnum(obj) == 3423) {
      send_to_char(ch, " @D[@B%d@D/@B24 Inks@D]@n", obj_get_value(obj, 6));
     }
     if (obj_extra_flagged(obj, ITEM_THROW)) {
       send_to_char(ch, " @D[@RThrow Only@D]@n");
     }
     if (obj_get_type_flag(obj) == ITEM_PLANT && !obj_extra_flagged(obj, ITEM_MATURE)) {
      if (obj_get_value(obj, VAL_WATERLEVEL) < -9) {
       send_to_char(ch, "@D[@RDead@D]@n");
      } else {
       switch (obj_get_value(obj, VAL_MATURITY)) {
        case 0:
         send_to_char(ch, " @D[@ySeed@D]@n");
         break;
        case 1:
         send_to_char(ch, " @D[@GSprout@D]@n");
         break;
        case 2:
         send_to_char(ch, " @D[@GYoung@D]@n");
         break;
        case 3:
         send_to_char(ch, " @D[@GMature@D]@n");
         break;
        case 4:
         send_to_char(ch, " @D[@GBudding@D]@n");
         break;
        case 5:
         send_to_char(ch, "@D[@GClose Harvest@D]@n");
         break;
        case 6:
         send_to_char(ch, "@D[@gHarvest@D]@n");
         break;
       }
      }
     }
     if (obj_get_type_flag(obj) == ITEM_CONTAINER && !obj_is_corpse(obj)) {
      if (!obj_val_flagged(obj, CONT_CLOSED) && !obj_extra_flagged(obj, ITEM_SHEATH))
       send_to_char(ch, " @D[@G-open-@D]@n");
      else if (!obj_extra_flagged(obj, ITEM_SHEATH))
       send_to_char(ch, " @D[@rclosed@D]@n");
     }
     if (obj_extra_flagged(obj, ITEM_DUPLICATE)) {
      send_to_char(ch, " @D[@YDuplicate@D]@n");
     }
    break;

  case SHOW_OBJ_ACTION:
    switch (obj_get_type_flag(obj)) {
    case ITEM_NOTE:
      if (obj->action_description) {
        char notebuf[MAX_NOTE_LENGTH];

        snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
        page_string(ch->desc, notebuf, TRUE);
      } else
	send_to_char(ch, "There appears to be nothing written on it.\r\n");
      return;

    case ITEM_BOARD:
      show_board(obj_get_vnum(obj),ch);
      break;
  
    case ITEM_CONTROL:
      send_to_char(ch, "@RFUEL@D: %s%s@n\r\n", obj_get_value(obj, 2) >= 200 ? "@G" : obj_get_value(obj, 2) >= 100 ? "@Y" : "@r", add_commas(obj_get_value(obj, 2)));
      break;
      
    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.\r\n");
      break;

    case ITEM_LIGHT:
      if (obj_get_value(obj, VAL_LIGHT_HOURS) == -1)
        send_to_char(ch, "Light Cycles left: Infinite\r\n");
      else
        send_to_char(ch, "Light Cycles left: [%d]\r\n", obj_get_value(obj, VAL_LIGHT_HOURS));
      break;

    case ITEM_FOOD:
      if (obj_get_foob(obj) >= 4) {
       if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj) / 4) {
        send_to_char(ch, "Condition of the food: Almost gone.\r\n");
       }
       else if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj) / 2) {
        send_to_char(ch, "Condition of the food: Half Eaten.");
       }
       else if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
        send_to_char(ch, "Condition of the food: Partially Eaten.");
       }
       else if (obj_get_value(obj, VAL_FOOD_FOODVAL) == obj_get_foob(obj)) {
        send_to_char(ch, "Condition of the food: Whole.");
       }
      }
      else if (obj_get_foob(obj) > 0) {
       if (obj_get_value(obj, VAL_FOOD_FOODVAL) < obj_get_foob(obj)) {
        send_to_char(ch, "Condition of the food: Almost gone.");
       }
       else if (obj_get_value(obj, VAL_FOOD_FOODVAL) == obj_get_foob(obj)) {
        send_to_char(ch, "Condition of the food: Whole.");
       }
      }
      else {
        send_to_char(ch, "Condition of the food: Insignificant.");
      }
      break;

    case ITEM_SPELLBOOK:
      send_to_char(ch, "It looks like an arcane tome.\r\n");
      display_spells(ch, obj);
      break;

    case ITEM_SCROLL:
      send_to_char(ch, "It looks like an arcane scroll.\r\n");
      display_scroll(ch, obj);
      break;

    case ITEM_VEHICLE:
     if (obj_get_vnum(obj) > 19199) {
      send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
      send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
      send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
      send_to_char(ch, "@YSyntax@D: @CEnter hatch\r\n");
     } else {
      send_to_char(ch, "@YSyntax@D: @CUnlock door\r\n");
      send_to_char(ch, "@YSyntax@D: @COpen door\r\n");
      send_to_char(ch, "@YSyntax@D: @CClose door\r\n");
      send_to_char(ch, "@YSyntax@D: @CEnter door\r\n");
     }
     break;

    case ITEM_HATCH:
     if (obj_get_vnum(obj) > 19199) {
      send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
      send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
      send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
      send_to_char(ch, "@YSyntax@D: @CLeave@n\r\n");
     } else {
      send_to_char(ch, "@YSyntax@D: @CUnlock door\r\n");
      send_to_char(ch, "@YSyntax@D: @COpen door\r\n");
      send_to_char(ch, "@YSyntax@D: @CClose door\r\n");
      send_to_char(ch, "@YSyntax@D: @CEnter door\r\n");
     }
     break;

    case ITEM_WINDOW:
     look_out_window(ch, obj->name);
     return;
     break;

    default:
      if (!obj_is_corpse(obj)) {
       send_to_char(ch, "You see nothing special..\r\n");
      } else {
       int mention = FALSE;
       send_to_char(ch, "This corpse has ");
      
       if (obj_get_value(obj, VAL_CORPSE_HEAD) == 0) {
        send_to_char(ch, "no head,");
        mention = TRUE;
       }

       if (obj_get_value(obj, VAL_CORPSE_RARM) == 0) {
        send_to_char(ch, "no right arm, ");
        mention = TRUE;
       } else if (obj_get_value(obj, VAL_CORPSE_RARM) == 2) {
        send_to_char(ch, "a broken right arm, ");
        mention = TRUE;
       }

       if (obj_get_value(obj, VAL_CORPSE_LARM) == 0) {
        send_to_char(ch, "no left arm, ");
        mention = TRUE;
       } else if (obj_get_value(obj, VAL_CORPSE_LARM) == 2) {
        send_to_char(ch, "a broken left arm, ");
        mention = TRUE;
       }

       if (obj_get_value(obj, VAL_CORPSE_RLEG) == 0) {
        send_to_char(ch, "no right leg, ");
        mention = TRUE;
       } else if (obj_get_value(obj, VAL_CORPSE_RLEG) == 2) {
        send_to_char(ch, "a broken right leg, ");
        mention = TRUE;
       }

       if (obj_get_value(obj, VAL_CORPSE_LLEG) == 0) {
        send_to_char(ch, "no left leg, ");
        mention = TRUE;
       } else if (obj_get_value(obj, VAL_CORPSE_LLEG) == 2) {
        send_to_char(ch, "a broken left leg, ");
        mention = TRUE;
       }

       if (mention == FALSE) {
        send_to_char(ch, "nothing missing from it but life.");
       } else {
        send_to_char(ch, "and is dead.");
       }
         
       send_to_char(ch, "\r\n");
      }
      break;
    }

    if (obj_get_type_flag(obj) == ITEM_WEAPON) {
     int num = 0;
     if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      num = 1;
     } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      num = 0;
     } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      num = 3;
     } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      num = 2;
     } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      num = 4;
     } else {
      num = 5;
     }
      send_to_char(ch, "The weapon type of %s@n is '%s'.\r\n", obj_get_short_description(obj), weapon_disp[num]);
      send_to_char(ch, "You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), obj)]);
    }
    diag_obj_to_char(obj, ch);
    send_to_char(ch, "It appears to be made of %s, and weighs %s", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
    break;

  default:
    log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    /*  SYSERR_DESC:
     *  show_obj_to_char() has some predefined 'mode's (argument #3) to tell
     *  it what to display to the character when it is called.  If the mode
     *  is not one of these, it will output this error, and indicate what
     *  mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode.
     */
    return;
  }

  if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
  send_to_char(ch, "\r\n");
}

static int show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  int found = FALSE;

  if (obj_extra_flagged(obj, ITEM_INVISIBLE)) {
    send_to_char(ch, " (invisible)");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_BLESS) && char_aff_flagged(ch, AFF_DETECT_ALIGN)) {
    send_to_char(ch, " ..It glows blue!");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_MAGIC) && char_aff_flagged(ch, AFF_DETECT_MAGIC)) {
    send_to_char(ch, " ..It glows yellow!");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_GLOW)) {
    send_to_char(ch, " @D(@GGlowing@D)@n");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_HOT)) {
   send_to_char(ch, " @D(@RHOT@D)@n");
   found++;
  }
  if (obj_extra_flagged(obj, ITEM_HUM)) {
    send_to_char(ch, " @D(@RHumming@D)@n");
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_SLOT2)) {
   if (obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED))
    send_to_char(ch, " @D[@m1/2 Tokens@D]@n");
   else if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED))
    send_to_char(ch, " @D[@m2/2 Tokens@D]@n");
   else
    send_to_char(ch, " @D[@m0/2 Tokens@D]@n");
   found++;
  }
  if (obj_extra_flagged(obj, ITEM_SLOT1)) {
   if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED))
    send_to_char(ch, " @D[@m1/1 Tokens@D]@n");
   else
    send_to_char(ch, " @D[@m0/1 Tokens@D]@n");
   found++;
  }
  if (obj_get_kicharge(obj) > 0) {
    int num = (obj_get_distance(obj) * 20) + rand_number(1, 5);
    send_to_char(ch, " %d meters away", num);
    found++;
  }
  if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
   send_to_char(ch, " @D(@YCUSTOM@D)@n");
  }
  if (obj_extra_flagged(obj, ITEM_RESTRING)) {
   send_to_char(ch, " @D(@R%s@D)@n", char_get_admlevel(ch) > 0 ? obj->name : "*");
  }
  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
   if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL || obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL || obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
    send_to_char(ch, ", and appears to be twisted and broken.");
   }
   else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
    send_to_char(ch, ", and is broken into hundreds of splinters.");
   }
   else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
    send_to_char(ch, ", and is shattered on the ground.");
   }
   else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
    send_to_char(ch, ", and is a pile of rubble.");
   }
   else {
    send_to_char(ch, ", and is broken.");
   }
    found++;
  }
  else {
    if (obj_get_type_flag(obj) != ITEM_BOARD) {
     if (obj_get_type_flag(obj) != ITEM_CONTAINER) {
      send_to_char(ch, ".");
     }
     if (!char_is_npc(ch) && obj_get_posted_to(obj) && obj_get_posttype(obj) <= 0) {
      struct obj_data *obj2 = obj_get_posted_to(obj);
      char dvnum[200];
      *dvnum = '\0';
      sprintf(dvnum, "@D[@G%d@D] @w", obj_get_vnum(obj2));
      send_to_char(ch, "\n...%s%s has been posted to it.", char_prf_flagged(ch, PRF_ROOMFLAGS) ? dvnum : "", obj2->short_description);
     }
    }
    found++;
  }
  return(found);
}

static void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show)
{
  struct obj_data *i, *j, *d;
  bool found = FALSE;
  int num;

  /* Loop through all objects in the list */
  for (i = list; i; i = i->next_content) {
    if (i->description == NULL)
      continue;
    if (strcasecmp(i->description, "undefined") == 0)
      continue;
    num = 0;
    d = i;
    if (CONFIG_STACK_OBJS) {
      for (j = list; j != i; j = j->next_content)
       if ((!strcasecmp(j->short_description, i->short_description) && !strcasecmp(j->description, i->description)) &&
            (j->item_number == i->item_number) && ((obj_extra_flagged(j, ITEM_BROKEN) && obj_extra_flagged(i, ITEM_BROKEN)) || (!obj_extra_flagged(j, ITEM_BROKEN) && !obj_extra_flagged(i, ITEM_BROKEN))))
         if ((!obj_get_sitting(j) && !obj_get_sitting(i)))
          if (obj_get_value(j, 6) == obj_get_value(i, 6))
            if ((obj_get_type_flag(j) != ITEM_PLANT && obj_get_type_flag(i) != ITEM_PLANT) || (obj_get_type_flag(j) == ITEM_PLANT && obj_get_type_flag(i) == ITEM_PLANT && obj_get_value(j, VAL_MATURITY) == obj_get_value(i, VAL_MATURITY) && obj_get_value(j, VAL_WATERLEVEL) == obj_get_value(i, VAL_WATERLEVEL)))
            if ((!obj_extra_flagged(j, ITEM_DUPLICATE) && !obj_extra_flagged(i, ITEM_DUPLICATE)) || (obj_extra_flagged(j, ITEM_DUPLICATE) && obj_extra_flagged(i, ITEM_DUPLICATE)))
             if (obj_get_posttype(j) == 0 && obj_get_posttype(i) == 0)
              if (!obj_get_fellow_wall(j) && !obj_get_fellow_wall(i))
               if ((obj_get_value(j, 0) == obj_get_value(i, 0) && obj_get_vnum(j) == 255 && obj_get_vnum(i) == 255) || (obj_get_vnum(j) != 255 && obj_get_vnum(i) != 255))
               break;
      if (j!=i)
        continue;
      for (d = j = i; j; j = j->next_content)
       if ((!strcasecmp(j->short_description, i->short_description) && !strcasecmp(j->description, i->description)) &&
            (j->item_number == i->item_number) && ((obj_extra_flagged(j, ITEM_BROKEN) && obj_extra_flagged(i, ITEM_BROKEN)) || (!obj_extra_flagged(j, ITEM_BROKEN) && !obj_extra_flagged(i, ITEM_BROKEN))))
         if ((!obj_get_sitting(j) && !obj_get_sitting(i)))
          if (obj_get_posttype(j) == 0 && obj_get_posttype(i) == 0)
           if (obj_get_value(j, 6) == obj_get_value(i, 6))
            if ((obj_get_type_flag(j) != ITEM_PLANT && obj_get_type_flag(i) != ITEM_PLANT) || (obj_get_type_flag(j) == ITEM_PLANT && obj_get_type_flag(i) == ITEM_PLANT && obj_get_value(j, VAL_MATURITY) == obj_get_value(i, VAL_MATURITY) && obj_get_value(j, VAL_WATERLEVEL) == obj_get_value(i, VAL_WATERLEVEL)))
             if ((!obj_extra_flagged(j, ITEM_DUPLICATE) && !obj_extra_flagged(i, ITEM_DUPLICATE)) || (obj_extra_flagged(j, ITEM_DUPLICATE) && obj_extra_flagged(i, ITEM_DUPLICATE)))
              if (!obj_get_fellow_wall(j) && !obj_get_fellow_wall(i))
               if ((obj_get_value(j, 0) == obj_get_value(i, 0) && obj_get_vnum(i) == 255 && obj_get_vnum(j) == 255) || (obj_get_vnum(j) != 255 && obj_get_vnum(i) != 255))
               if (char_can_see_obj(ch, j)) {
                num++;
          if (d == i && !char_can_see_obj(ch, d))
            d = j;
          }
    }
    if ((char_can_see_obj(ch, d) && ((*d->description != '.' && *d->short_description != '.' )|| char_prf_flagged(ch, PRF_HOLYLIGHT))) || (obj_get_type_flag(d) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(ch, "@D(@Rx@Y%2i@D)@n ", num);
      show_obj_to_char(d, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
  send_to_char(ch, " Nothing.\r\n");
}

static void diag_obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."                 },
    {  90, "has a few scuffs."                          },
    {  75, "has some small scuffs and scratches."       },
    {  50, "has quite a few scratches."                 },
    {  30, "has some big nasty scrapes and scratches."  },
    {  15, "looks pretty damaged."                      },
    {   0, "is in awful condition."                     },
    {  -1, "is in need of repair."                      },
  };
  int percent, ar_index;
  const char *objs = obj_shortdesc_for(obj, ch);

  if (obj_get_value(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * obj_get_value(obj, VAL_ALL_HEALTH)) / obj_get_value(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0;               /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "\r\n%c%s %s\r\n", toupper(*objs), objs + 1, diagnosis[ar_index].text);
}

static void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "@wis in @Gexcellent@w condition."			},
    {  90, "@whas a few @Rscratches@w."			},
    {  80, "@whas some small @Rwounds@w and @Rbruises@w."		},
    {  70, "@whas quite a few @Rwounds@w."			},
    {  60, "@whas some big @rnasty wounds@w and @Rscratches@w."	},
    {  50, "@wlooks pretty @rhurt@w."				},
    {  40, "@wis mainly @rinjured@w."                         },
    {  30, "@wis a @rmess@w of @rinjuries@w."                     },
    {  20, "@wis @rstruggling@w to @msurvive@w."                  },
    {  10, "@wis in @mawful condition@w."                     },
    {   0, "@Ris barely alive.@w"		        	},
    {  -1, "@ris nearly dead.@w"	                        },
  };
  int percent, ar_index;

  percent = (int)(i->health * 100.0);

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent) {
        send_to_char(ch, "%s\r\n", diagnosis[ar_index].text);
        return;
    }
}

static void look_at_char(struct char_data *i, struct char_data *ch)
{
	int j, found, clan = FALSE;
	char buf[100];
	struct obj_data *tmp_obj;

	if (!ch->desc) {
		return;
	}
	if (i->description) {
		send_to_char(ch, "%s", i->description);
	}
	if (!char_mob_flagged(i, MOB_JUSTDESC)) {
		bringdesc(ch, i);
	}
	send_to_char(ch, "\r\n");
	if (!char_is_npc(i)) {
		if (char_get_limb_condition(i, (1)-1) >= 50 && !char_plr_flagged(i, PLR_CRARM)) {
			send_to_char(ch, "            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (1)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (1)-1) > 0 && !char_plr_flagged(i, PLR_CRARM)) {
			send_to_char(ch, "            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (1)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (1)-1) > 0 && char_plr_flagged(i, PLR_CRARM)) {
			send_to_char(ch, "            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (1)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (1)-1) <= 0) {
			send_to_char(ch, "            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
		}
		if (char_get_limb_condition(i, (2)-1) >= 50 && !char_plr_flagged(i, PLR_CLARM)) {
			send_to_char(ch, "            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (2)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (2)-1) > 0 && !char_plr_flagged(i, PLR_CLARM)) {
			send_to_char(ch, "            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (2)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (2)-1) > 0 && char_plr_flagged(i, PLR_CLARM)) {
			send_to_char(ch, "            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (2)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (2)-1) <= 0) {
			send_to_char(ch, "            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
		}
		if (char_get_limb_condition(i, (3)-1) >= 50 && !char_plr_flagged(i, PLR_CLARM)) {
			send_to_char(ch, "            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (3)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (3)-1) > 0 && !char_plr_flagged(i, PLR_CRLEG)) {
			send_to_char(ch, "            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (3)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (3)-1) > 0 && char_plr_flagged(i, PLR_CRLEG)) {
			send_to_char(ch, "            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (3)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (3)-1) <= 0) {
			send_to_char(ch, "            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
		}
		if (char_get_limb_condition(i, (4)-1) >= 50 && !char_plr_flagged(i, PLR_CLLEG)) {
			send_to_char(ch, "            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(i, (4)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (4)-1) > 0 && !char_plr_flagged(i, PLR_CLLEG)) {
			send_to_char(ch, "            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(i, (4)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (4)-1) > 0 && char_plr_flagged(i, PLR_CLLEG)) {
			send_to_char(ch, "            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(i, (4)-1), "%", "%");
		}
		else if (char_get_limb_condition(i, (4)-1) <= 0) {
			send_to_char(ch, "            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
		}
		if (char_plr_flagged(i, PLR_HEAD)) {
			send_to_char(ch, "            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
		}
		if (!char_plr_flagged(i, PLR_HEAD)) {
			send_to_char(ch, "            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
		}
		if (((char_is_saiyan(i) || char_is_halfbreed(i)) && char_plr_flagged(i, PLR_STAIL)) && !char_plr_flagged(i, PLR_TAILHIDE)) {
			send_to_char(ch, "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
		}
		if ((char_is_saiyan(i) || char_is_halfbreed(i)) && (!char_plr_flagged(i, PLR_STAIL)) && (!char_plr_flagged(i, PLR_TAILHIDE))) {
			send_to_char(ch, "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
		}
		if ((char_is_icer(i) || char_is_bio(i)) && char_plr_flagged(i, PLR_TAIL)) {
			send_to_char(ch, "            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
		}
		if ((char_is_icer(i) || char_is_bio(i)) && !char_plr_flagged(i, PLR_TAIL)) {
			send_to_char(ch, "            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
		}
	}
	send_to_char(ch, "\r\n");
	clan = FALSE;
	if (!char_is_npc(i)) {
		send_to_char(ch, "            @D[@mClan        @D: @W%-20s@D]@n\r\n", clan ? buf : "None.");
	}
	if (!char_is_npc(i)) {
		send_to_char(ch, "\r\n         @D----------------------------------------@n\r\n");
		trans_check(ch, i);
		send_to_char(ch, "         @D----------------------------------------@n\r\n");
	}
	send_to_char(ch, "\r\n");

	if ((!char_plr_flagged(i, PLR_DISGUISED) && (readIntro(ch, i) == 1 && !char_is_npc(i)))) {
		if (char_get_sex(i) == SEX_NEUTRAL)
			send_to_char(ch, "%s appears to be %s %s, ", get_i_name(ch, i), AN(RACE(i)), LRACE(i));
		else
			send_to_char(ch, "%s appears to be %s %s %s, ", get_i_name(ch, i), AN(char_mafe(i)), char_mafe(i), LRACE(i));
	}
	else if (ch == i || char_is_npc(i)) {
		if (char_get_sex(i) == SEX_NEUTRAL)
			send_to_char(ch, "%c%s appears to be %s %s, ", toupper(*char_get_name(i)), char_get_name(i) + 1, AN(RACE(i)), LRACE(i));
		else
			send_to_char(ch, "%c%s appears to be %s %s %s, ", toupper(*char_get_name(i)), char_get_name(i) + 1, AN(char_mafe(i)), char_mafe(i), LRACE(i));
	}
	else {
		if (char_get_sex(i) == SEX_NEUTRAL)
			send_to_char(ch, "Appears to be %s %s, ", AN(RACE(i)), LRACE(i));
		else
			send_to_char(ch, "Appears to be %s %s %s, ", AN(char_mafe(i)), char_mafe(i), LRACE(i));
	}

	if (char_is_npc(i)) {
		send_to_char(ch, "is %s sized, and\r\n", size_names[get_size(i)]);
	}
	if (!char_is_npc(i)) {
		if (!char_plr_flagged(i, PLR_OOZARU) && (!char_is_icer(i) || !char_is_transformed(i)) && char_get_genome(i, 0) < 11) {
			send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], char_get_pc_height(i), char_get_pc_weight(i));
		}
		else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS1)) {
			int num1 = char_get_pc_height(i) * 3;
			int num2 = char_get_pc_weight(i) * 4;
			send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
		}
		else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS2)) {
			int num1 = char_get_pc_height(i) * 3;
			int num2 = char_get_pc_weight(i) * 4;
			send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
		}
		else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS3)) {
			int num1 = char_get_pc_height(i) * 1.5;
			int num2 = char_get_pc_weight(i) * 2;
			send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
		}
		else if (char_is_icer(i) && char_plr_flagged(i, PLR_TRANS4)) {
			int num1 = char_get_pc_height(i) * 2;
			int num2 = char_get_pc_weight(i) * 3;
			send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
		}
		else if (char_plr_flagged(i, PLR_OOZARU) || char_get_genome(i, 0) == 11) {
			int num1 = char_get_pc_height(i) * 10;
			int num2 = char_get_pc_weight(i) * 50;
			send_to_char(ch, "is %s sized, about %dcm tall,\r\nabout %dkg heavy,", size_names[get_size(i)], num1, num2);
		}
		if (i == ch) {
			send_to_char(ch, " and ");
		}
		else if (GET_AGE(ch) >= GET_AGE(i) + 30) {
			send_to_char(ch, " appears to be very much younger than you, and ");
		}
		else if (GET_AGE(ch) >= GET_AGE(i) + 25) {
			send_to_char(ch, " appears to be much younger than you, and ");
		}
		else if (GET_AGE(ch) >= GET_AGE(i) + 15) {
			send_to_char(ch, " appears to be a good amount younger than you, and ");
		}
		else if (GET_AGE(ch) >= GET_AGE(i) + 10) {
			send_to_char(ch, " appears to be about a decade younger than you, and ");
		}
		else if (GET_AGE(ch) >= GET_AGE(i) + 5) {
			send_to_char(ch, " appears to be several years younger than you, and ");
		}
		else if (GET_AGE(ch) >= GET_AGE(i) + 2) {
			send_to_char(ch, " appears to be a bit younger than you, and ");
		}
		else if (GET_AGE(ch) > GET_AGE(i)) {
			send_to_char(ch, " appears to be slightly younger than you, and ");
		}
		else if (GET_AGE(ch) == GET_AGE(i)) {
			send_to_char(ch, " appears to be the same age as you, and ");
		}
		if (GET_AGE(i) >= GET_AGE(ch) + 30) {
			send_to_char(ch, " appears to be very much older than you, and ");
		}
		else if (GET_AGE(i) >= GET_AGE(ch) + 25) {
			send_to_char(ch, " appears to be much older than you, and ");
		}
		else if (GET_AGE(i) >= GET_AGE(ch) + 15) {
			send_to_char(ch, " appears to be a good amount older than you, and ");
		}
		else if (GET_AGE(i) >= GET_AGE(ch) + 10) {
			send_to_char(ch, " appears to be about a decade older than you, and ");
		}
		else if (GET_AGE(i) >= GET_AGE(ch) + 5) {
			send_to_char(ch, " appears to be several years older than you, and ");
		}
		else if (GET_AGE(i) >= GET_AGE(ch) + 2) {
			send_to_char(ch, " appears to be a bit older than you, and ");
		}
		else if (GET_AGE(i) > GET_AGE(ch)) {
			send_to_char(ch, " appears to be slightly older than you, and ");
		}
	}
	diag_char_to_char(i, ch);
	found = FALSE;
	for (j = 0; !found && j < NUM_WEARS; j++)
		if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)))
			found = TRUE;

	if (found && (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NOEQSEE))) {
		send_to_char(ch, "\r\n");	/* act() does capitalization. */
		if (!char_plr_flagged(i, PLR_DISGUISED)) {
			act("$n is using:", FALSE, i, 0, ch, TO_VICT);
		}
		else {
			act("The disguised person is using:", FALSE, i, 0, ch, TO_VICT);
		}
		for (j = 0; j < NUM_WEARS; j++)
			if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)) && (j != WEAR_WIELD1 && j != WEAR_WIELD2)) {
				send_to_char(ch, "%s", wear_where[j]);
				show_obj_to_char(char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
				if (obj_extra_flagged(char_get_equipment(i, j), ITEM_SHEATH)) {
					struct obj_data *obj2 = NULL, *next_obj = NULL, *sheath = char_get_equipment(i, j);
					for (obj2 = sheath->contains; obj2; obj2 = next_obj) {
						next_obj = obj2->next_content;
						if (obj2) {
							send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
							show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
						}
					}
					obj2 = NULL;
				}
			}
			else if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)) && (!char_plr_flagged(i, PLR_THANDW))) {
				send_to_char(ch, "%s", wear_where[j]);
				show_obj_to_char(char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
				if (obj_extra_flagged(char_get_equipment(i, j), ITEM_SHEATH)) {
					struct obj_data *obj2 = NULL, *next_obj = NULL, *sheath = char_get_equipment(i, j);
					for (obj2 = sheath->contains; obj2; obj2 = next_obj) {
						next_obj = obj2->next_content;
						if (obj2) {
							send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
							show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
						}

					}
					obj2 = NULL;
				}
			}
			else if (char_get_equipment(i, j) && char_can_see_obj(ch, char_get_equipment(i, j)) && (char_plr_flagged(i, PLR_THANDW))) {
				send_to_char(ch, "@c<@CWielded by B. Hands@c>@n ");
				show_obj_to_char(char_get_equipment(i, j), ch, SHOW_OBJ_SHORT);
			}
	}
	if (ch != i && ((char_get_skill(ch, SKILL_KEEN) && char_aff_flagged(ch, AFF_SNEAK)) || char_get_admlevel(ch))) {
		found = FALSE;
		act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
		if (char_can_see(i, ch))
			act("$n tries to evaluate what you have in your inventory.", TRUE, ch, 0, i, TO_VICT);
		if (char_get_skill(ch, SKILL_KEEN) > axion_dice(0) && (!char_is_npc(i) || char_get_admlevel(ch) > 1)) {
			for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
				if (char_can_see_obj(ch, tmp_obj) &&
					(char_adm_flagged(ch, ADM_SEEINV) || (rand_number(0, 20) < char_get_level(ch)))) {
					show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
					found = TRUE;
				}
			}
			improve_skill(ch, SKILL_KEEN, 1);
		}
		else if (char_is_npc(i) && char_get_admlevel(ch) < 2) {
			return;
		}
		else {
			act("You are unsure about $s inventory.", FALSE, i, 0, ch, TO_VICT);
			if (char_can_see(i, ch))
				act("$n didn't seem to get a good enough look.", TRUE, ch, 0, i, TO_VICT);
			improve_skill(ch, SKILL_KEEN, 1);
			return;
		}
		if (!found) {
			send_to_char(ch, "You can't see anything.\r\n");
			improve_skill(ch, SKILL_KEEN, 1);
		}
	}
}

static void list_one_char(struct char_data *i, struct char_data *ch)
{
  struct obj_data *chair = NULL;
  int count = FALSE;
  const char *positions[] = {
    " is dead",
    " is mortally wounded",
    " is lying here, incapacitated",
    " is lying here, stunned",
    " is sleeping here",
    " is resting here",
    " is sitting here",
    "!FIGHTING!",
    " is standing here"
  };

  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS) && char_is_npc(i)) 
     send_to_char(ch, "@D[@G%d@D]@w %s", char_get_mob_vnum(i), SCRIPT(i) ? "[TRIG] " : "");
  
  if (char_is_npc(i) && i->long_descr && char_get_position(i) == char_get_mob_specials_default_pos(i) && !char_get_fighting(i)) {
    send_to_char(ch, "%s", i->long_descr);

    if(char_is_npc(i)) {
      double health = i->health;
      if(health <= 0.1) {
        act("@R...Should be DEAD soon.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.2) {
        act("@R...Is on $s last leg.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.3) {
        act("@R...Is absolutely covered in wounds.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.4) {
        act("@R...$s body is in terrible shape.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.5) {
        act("@R...Blood is seeping from the wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.6) {
        act("@R...Horrible wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.7) {
        act("@R...Quite a few wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.8) {
        act("@R...Many wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health <= 0.9) {
        act("@R...A few wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      } else if(health < 1.0) {
        act("@R...Some slight wounds on $s body.@w", TRUE, i, 0, ch, TO_VICT);
      }
    }


    if (char_get_listenroom(i) > 0) {
     char eaves[300];
     sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.", dirs[char_get_eavesdir(i)]);
     act(eaves, TRUE, i, 0, ch, TO_VICT);
    }
    if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 1)
      act("...$e is in the air!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 2)
      act("...$e is high in the air!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_SANCTUARY) && !char_get_skill(i, SKILL_AQUA_BARRIER))
      act("...$e has a barrier around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_FIRESHIELD))
      act("...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_SANCTUARY) && char_get_skill(i, SKILL_AQUA_BARRIER))
      act("...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (!char_is_npc(i) && char_plr_flagged(i, PLR_SPIRAL))
      act("...$e is spinning in a vortex!", FALSE, i, 0, ch, TO_VICT);
    if (char_get_charge(i))
      act("...$e has a bright %s aura around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_METAMORPH))
      act("@w...$e has a dark, @rred@w aura and menacing presence.", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_HAYASA))
      act("@w...$e has a soft @cblue@w glow around $s body!", FALSE, i, 0, ch, TO_VICT);
    if (char_aff_flagged(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_FAERIE_FIRE))
      act("@m...$e @mis outlined with purple fire!@m", FALSE, i, 0, ch, TO_VICT);
    if (char_get_feature(i)) {
      char woo[MAX_STRING_LENGTH];
      sprintf(woo, "@C%s@n", char_get_feature(i));
      act(woo, FALSE, i, 0, ch, TO_VICT);
    }

    return;
  }

  if (char_is_npc(i) && !char_get_fighting(i) && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_grappled(i) && char_get_grappled(i) == ch)
    send_to_char(ch, "@w%c%s is being grappled with by YOU!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_grappled(i) && char_get_grappled(i) != ch)
    send_to_char(ch, "@w%c%s is being absorbed from by %s!", toupper(*i->short_descr), i->short_descr + 1, readIntro(ch, char_get_grappled(i)) == 1 ? get_i_name(ch, char_get_grappled(i)) : AN(RACE(char_get_grappled(i))));
  else if (char_is_npc(i) && char_get_absorbby(i) && char_get_absorbby(i) == ch)
    send_to_char(ch, "@w%c%s is being absorbed from by YOU!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_absorbby(i) && char_get_absorbby(i) != ch)
    send_to_char(ch, "@w%c%s is being absorbed from by %s!", toupper(*i->short_descr), i->short_descr + 1, readIntro(ch, char_get_absorbby(i)) == 1 ? get_i_name(ch, char_get_absorbby(i)) : AN(RACE(char_get_absorbby(i))));
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_fighting(i) != ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING && is_sparring(i))
    send_to_char(ch, "@w%c%s is sparring with %s!", toupper(*i->short_descr), i->short_descr + 1, char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : LRACE(char_get_fighting(i))));
  else if (char_is_npc(i) && char_get_fighting(i) && is_sparring(i) && char_get_fighting(i) == ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is sparring with you!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_fighting(i) != ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is fighting %s!", toupper(*i->short_descr), i->short_descr + 1, char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : LRACE(char_get_fighting(i))));
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_fighting(i) == ch && char_get_position(i) != POS_SITTING && char_get_position(i) != POS_SLEEPING)
    send_to_char(ch, "@w%c%s is fighting YOU!", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_position(i) == POS_SITTING)
    send_to_char(ch, "@w%c%s is sitting here.", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i) && char_get_fighting(i) && char_get_position(i) == POS_SLEEPING)
    send_to_char(ch, "@w%c%s is sleeping here.", toupper(*i->short_descr), i->short_descr + 1);
  else if (char_is_npc(i))
    send_to_char(ch, "@w%c%s", toupper(*i->short_descr), i->short_descr + 1);
  else if (!char_is_npc(i)) {
   if (char_is_majin(i) && char_aff_flagged(i, AFF_LIQUEFIED)) {
    send_to_char(ch, "@wSeveral blobs of %s colored goo spread out here.@n\n", skin_types[(int)char_get_skin(i)]);
    return;
   }
   if ((char_get_admlevel(ch) > 0 || char_get_admlevel(i) > 0) || char_is_npc(ch)) {
     send_to_char(ch, "@w%s", i->name);
   }
   else if ((!char_plr_flagged(i, PLR_DISGUISED) && readIntro(ch, i) == 1)) {
    send_to_char(ch, "@w%s", get_i_name(ch, i));
   }
   else if (!char_plr_flagged(i, PLR_DISGUISED) && readIntro(ch, i) != 1) {
    if (char_get_distfea(i) == DISTFEA_EYE) {
      send_to_char(ch, "@wA %s eyed %s %s", eye_types[(int)char_get_eye(i)], char_mafe(i), LRACE(i));
    }
    else if (char_get_distfea(i) == DISTFEA_HAIR) {
     if (char_is_majin(i)) {
       send_to_char(ch, "@wA %s majin, with a %s forelock,", char_mafe(i), FHA_types[(int)char_get_hairl(i)]);
     }
     else if (char_is_namek(i)) {
       send_to_char(ch, "@wA namek, with %s antennae,", FHA_types[(int)char_get_hairl(i)]);
     }
     else if (char_is_arlian(i)) {
       send_to_char(ch, "@wA arlian, with %s antennae,", FHA_types[(int)char_get_hairl(i)]);
     }
     else if (char_is_icer(i) || char_is_demon(i)) {
       send_to_char(ch, "@wA %s %s, with %s horns", char_mafe(i), LRACE(i), FHA_types[(int)char_get_hairl(i)]);
     }
     else {
       char blarg[MAX_INPUT_LENGTH];
       sprintf(blarg, "%s %s hair %s", hairl_types[(int)char_get_hairl(i)], hairc_types[(int)char_get_hairc(i)], hairs_types[(int)char_get_hairs(i)]);
       send_to_char(ch, "@wA %s %s, with %s", char_mafe(i), LRACE(i), char_get_hairl(i) == 0 ? "a bald head" : (blarg));
     }
    }
    else if (char_get_distfea(i) == DISTFEA_SKIN) {
      send_to_char(ch, "@wA %s skinned %s %s", skin_types[(int)char_get_skin(i)], char_mafe(i), LRACE(i));
    }
    else if (char_get_distfea(i) == DISTFEA_HEIGHT) {
      char *height;
      if (char_is_truffle(i)) {
       if (char_get_pc_height(i) > 70) {
        height = strdup("very tall");
       }
       else if (char_get_pc_height(i) > 55) {
        height = strdup("tall");
       }
       else if (char_get_pc_height(i) > 35) {
        height = strdup("average height");
       }
       else {
        height = strdup("short");
       }
      }
      else if (char_plr_flagged(i, PLR_OOZARU) || char_get_genome(i, 0) == 11) {
       if (char_get_pc_height(i) * 10 > 2000) {
        height = strdup("very tall");
       }
       else if (char_get_pc_height(i) * 10 > 1800) {
        height = strdup("tall");
       }
       else if (char_get_pc_height(i) * 10 > 1500) {
        height = strdup("average height");
       }
       else {
        height = strdup("short");
       }
      }
      else {
       if (char_get_pc_height(i) > 200) {
        height = strdup("very tall");
       }
       else if (char_get_pc_height(i) > 180) {
        height = strdup("tall");
       }
       else if (char_get_pc_height(i) > 150) {
        height = strdup("average height");
       }
       else if (char_get_pc_height(i) > 120) {
        height = strdup("short");
       }
       else {
        height = strdup("very short");
       }
      }
      send_to_char(ch, "@wA %s %s %s", height, char_mafe(i), LRACE(i));
      if (height) {
       free(height);
      }
    }
    else if (char_get_distfea(i) == DISTFEA_WEIGHT) {
      char *height;
      if (char_is_truffle(i)) {
       if (char_get_pc_weight(i) > 35) {
        height = strdup("very heavy");
       }
       else if (char_get_pc_weight(i) > 25) {
        height = strdup("heavy");
       }
       else if (char_get_pc_weight(i) > 15) {
        height = strdup("average weight");
       }
       else {
        height = strdup("welterweight");
       }
      }
      else if (char_plr_flagged(i, PLR_OOZARU) || char_get_genome(i, 0) == 11) {
       if (char_get_pc_weight(i) * 50 > 6000) {
        height = strdup("very heavy");
       }
       else if (char_get_pc_weight(i) * 50 > 5000) {
        height = strdup("heavy");
       }
       else if (char_get_pc_weight(i) * 50 > 4000) {
        height = strdup("average weight");
       }
       else if (char_get_pc_weight(i) * 50 > 3000) {
        height = strdup("lightweight");
       }
       else {
        height = strdup("welterweight");
       }
      }
      else {
       if (char_get_pc_weight(i) > 120) {
        height = strdup("very heavy");
       }
       else if (char_get_pc_weight(i) > 100) {
        height = strdup("heavy");
       }
       else if (char_get_pc_weight(i) > 80) {
        height = strdup("average weight");
       }
       else if (char_get_pc_weight(i) > 60) {
        height = strdup("lightweight");
       }
       else {
        height = strdup("welterweight");
       }
      }
      send_to_char(ch, "@wA %s %s %s", height, char_mafe(i), LRACE(i));
      if (height) {
       free(height);
      }
    }
   }
   else {
    send_to_char(ch, "@wA disguised %s %s", char_mafe(i), LRACE(i));
   }
  }

 if (!char_is_npc(i) || !char_get_fighting(i)) {
  if (char_aff_flagged(i, AFF_INVISIBLE)) {
    send_to_char(ch, ", is invisible");
    count = TRUE;
  }
  if (char_aff_flagged(i, AFF_ETHEREAL)) {
    send_to_char(ch, ", has a halo");
    count = TRUE;
  }
  if (char_aff_flagged(i, AFF_HIDE) && i != ch) {
    send_to_char(ch, ", is hiding");
    if (char_get_skill(i, SKILL_HIDE) && !char_is_npc(ch) && i != ch) {
     improve_skill(i, SKILL_HIDE, 1);
    }
    count = TRUE;
  }
  if (!char_is_npc(i) && !i->desc) {
    send_to_char(ch, ", has a blank stare");
    count = TRUE;
  }
  if (!char_is_npc(i) && char_plr_flagged(i, PLR_WRITING)) {
    send_to_char(ch, ", is writing");
    count = TRUE;
  }
  if (!char_is_npc(i) && char_prf_flagged(i, PRF_BUILDWALK)) {
    send_to_char(ch, ", is buildwalking");
    count = TRUE;
  }
  if (!char_is_npc(i) && char_get_absorbing(i) && char_get_absorbing(i) != ch) {
    send_to_char(ch, ", is absorbing from %s", char_get_name(char_get_absorbing(i)));
    count = TRUE;
  }
  if (!char_is_npc(i) && char_get_grappling(i) && char_get_grappling(i) != ch) {
    send_to_char(ch, ", is grappling with %s", readIntro(ch, char_get_grappling(i)) == 1 ? get_i_name(ch, char_get_grappling(i)) : introd_calc(char_get_grappling(i)));
    count = TRUE;
  }
  if (!char_is_npc(i) && CARRYING(i) && CARRYING(i) != ch) {
    send_to_char(ch, ", is carrying %s", readIntro(ch, CARRYING(i)) == 1 ? get_i_name(ch, CARRYING(i)) : introd_calc(CARRYING(i)));
    count = TRUE;
  }
  if (!char_is_npc(i) && CARRIED_BY(i) && CARRIED_BY(i) != ch) {
    send_to_char(ch, ", is being carried by %s", readIntro(ch, CARRIED_BY(i)) == 1 ? get_i_name(ch, CARRIED_BY(i)) : introd_calc(CARRIED_BY(i)));
    count = TRUE;
  }
  if (!char_is_npc(i) && char_get_grappling(i) && char_get_grappling(i) == ch) {
    send_to_char(ch, ", is grappling with YOU");
    count = TRUE;
  }
  if (!char_is_npc(i) && char_get_absorbing(i) && char_get_absorbing(i) == ch) {
    send_to_char(ch, ", is absorbing from YOU");
    count = TRUE;
  }
  if (!char_is_npc(i) && char_get_absorbing(ch) && char_get_absorbing(ch) == i) {
    send_to_char(ch, ", is being absorbed from by YOU");
    count = TRUE;
  }
  if (!char_is_npc(i) && char_get_grappling(ch) && char_get_grappling(ch) == i) {
    send_to_char(ch, ", is being grappled with by YOU");
    count = TRUE;
  }
  if (!char_is_npc(i) && CARRYING(ch) && CARRYING(ch) == i) {
    send_to_char(ch, ", is being carried by you");
    count = TRUE;
  }
  if (!char_is_npc(ch) && !char_is_npc(i) && char_get_fighting(i)) {
   if (!char_plr_flagged(i, PLR_SPAR) || (char_plr_flagged(i, PLR_SPAR) && (!char_plr_flagged(char_get_fighting(i), PLR_SPAR) || char_is_npc(char_get_fighting(i))))) {
   send_to_char(ch, ", is here fighting ");
   }
   if (char_plr_flagged(i, PLR_SPAR) && char_plr_flagged(char_get_fighting(i), PLR_SPAR)) {
   send_to_char(ch, ", is here sparring ");
   }
     if (char_get_fighting(i) == ch) {
        send_to_char(ch, "@rYOU@w");
        count = TRUE;
     }
     else {
      if (char_get_in_room(i) == char_get_in_room(char_get_fighting(i))) {
       send_to_char(ch, "%s", char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : LRACE(char_get_fighting(i))));
       count = TRUE;
      }
      else {
       send_to_char(ch,  "someone who has already left!");
      }
     }
  }
 }
  if (char_get_sits(i)) {
       chair = char_get_sits(i);
       if (char_plr_flagged(i, PLR_HEALT)) {
        send_to_char(ch, "@w is floating inside a healing tank.");
       }
       else if (count == TRUE) {
       send_to_char(ch, ",@w and%s on %s.", positions[(int) char_get_position(i)], chair->short_description);
       }
       else if (count == FALSE) {
       send_to_char(ch, "@w%s on %s.", positions[(int) char_get_position(i)], chair->short_description);
       }
  }
  else if (!char_plr_flagged(i, PLR_PILOTING) && !char_get_sits(i) && (!char_is_npc(i) || !char_get_fighting(i))) {
   if (count == TRUE) {
    send_to_char(ch, "@w, and%s.", positions[(int) char_get_position(i)]);
   }
   if (count == FALSE) {
    send_to_char(ch, "@w%s.", positions[(int) char_get_position(i)]);
   }
  }
   else if (char_plr_flagged(i, PLR_PILOTING)) {
   send_to_char(ch, "@w, is sitting in the pilot's chair.\r\n");
   }
    else {
   
    if (char_get_fighting(i) && !char_is_npc(ch) && !char_is_npc(i)) {
      if (!char_plr_flagged(i, PLR_SPAR)) {
       send_to_char(ch, ", is here fighting ");
      }
      if (char_plr_flagged(i, PLR_SPAR)) {
       send_to_char(ch, ", is here sparring ");
      }
      if (char_get_fighting(i) == ch)
	send_to_char(ch, "@rYOU@w!");
      else {
	if (char_get_in_room(i) == char_get_in_room(char_get_fighting(i)))
	  send_to_char(ch, "%s!", char_get_admlevel(ch) ? char_get_name(char_get_fighting(i)) : (readIntro(ch, char_get_fighting(i)) == 1 ? get_i_name(ch, char_get_fighting(i)) : LRACE(char_get_fighting(i))));
	else
	  send_to_char(ch,  "someone who has already left!");
      }
    } else if (!char_is_npc(i)) {			/* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
    }
  }
  

  if (char_aff_flagged(ch, AFF_DETECT_ALIGN)) {
    if (char_is_evil(i))
      send_to_char(ch, " (@rRed@[3] Aura)");
    else if (char_is_good(i))
      send_to_char(ch, " (@bBlue@[3] Aura)");
  }
  if (!char_is_npc(i) && char_prf_flagged(i, PRF_AFK))
   send_to_char(ch, " @D(@RAFK@D)");
  else if (!char_is_npc(i) && i->timer > 3)
   send_to_char(ch, " @D(@RIDLE@D)");
  send_to_char(ch, "@n\r\n");

  if (char_get_listenroom(i) > 0) {
    char eaves[300];
    sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.", dirs[char_get_eavesdir(i)]);
    act(eaves, TRUE, i, 0, ch, TO_VICT);
  }
  if (!char_is_npc(i)) {
   if (char_plr_flagged(i, PLR_FISHING)) {
    act("@w...$e is @Cfishing@w.@n", TRUE, i, 0, ch, TO_VICT);
   }
  }
  if (char_plr_flagged(i, PLR_AURALIGHT)) {
	  char bloom[MAX_INPUT_LENGTH];
      sprintf(bloom, "...is surrounded by a bright %s aura.@n", aura_types[char_get_aura(i)]);
      act(bloom, TRUE, i, 0, ch, TO_VICT);
	}
  if (char_aff_flagged(i, AFF_SANCTUARY) && !char_get_skill(i, SKILL_AQUA_BARRIER))
    act("@w...$e has a @bbarrier@w around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_FIRESHIELD))
      act("@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", FALSE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_HEALGLOW))
      act("@w...$e has a serene @Cblue@Y glow@w around $s body.", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_EARMOR))
      act("@w...$e has ghostly @Ggreen@w ethereal armor around $s body.", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_SANCTUARY) && char_get_skill(i, SKILL_AQUA_BARRIER))
    act("@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 1)
    act("@w...$e is in the air!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_FLYING) && char_get_altitude(i) == 2)
    act("@w...$e is high in the air!", TRUE, i, 0, ch, TO_VICT);
  if (char_get_kaioken(i) > 0)
    act("@w...@r$e has a red aura around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (!char_is_npc(i) && char_plr_flagged(i, PLR_SPIRAL))
      act("@w...$e is spinning in a vortex!", FALSE, i, 0, ch, TO_VICT);
  if (char_is_transformed(i) && !char_is_android(i) && !char_is_saiyan(i) && !char_is_halfbreed(i))
    act("@w...$e has energy crackling around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_get_charge(i) && !char_is_saiyan(i) && !char_is_halfbreed(i)) {
    char aura[MAX_INPUT_LENGTH];
    sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!", aura_types[char_get_aura(i)]);
    act(aura, TRUE, i, 0, ch, TO_VICT);
  }
  if (!char_plr_flagged(i, PLR_OOZARU) && char_get_charge(i) && char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (!char_plr_flagged(i, PLR_OOZARU) && char_get_charge(i) && !char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i))) {
    char aura[MAX_INPUT_LENGTH];
    sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!", aura_types[char_get_aura(i)]);
    act(aura, TRUE, i, 0, ch, TO_VICT);
  }
  if (!char_plr_flagged(i, PLR_OOZARU) && !char_get_charge(i) && char_is_transformed(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e has energy crackling around $s body!", TRUE, i, 0, ch, TO_VICT);
  if (char_plr_flagged(i, PLR_OOZARU) && char_get_charge(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e is in the form of a @rgreat ape@w!", TRUE, i, 0, ch, TO_VICT);
  if (is_affected(i, AFF_KYODAIKA))
    act("@w...$e has expanded $s body size@w!", TRUE, i, 0, ch, TO_VICT);
  if (char_aff_flagged(i, AFF_HAYASA))
      act("@w...$e has a soft @cblue@w glow around $s body!", FALSE, i, 0, ch, TO_VICT);
  if (char_plr_flagged(i, PLR_OOZARU) && !char_get_charge(i) && (char_is_saiyan(i) || char_is_halfbreed(i)))
    act("@w...$e has energy crackling around $s @rgreat ape@w body!", TRUE, i, 0, ch, TO_VICT);
  if (char_get_feature(i)) {
   char woo[MAX_STRING_LENGTH];
   sprintf(woo, "@C%s@n", char_get_feature(i));
   act(woo, FALSE, i, 0, ch, TO_VICT);
  }
  
    if (char_get_rdisplay(i)) {
	 if (char_get_rdisplay(i) != "Empty") {
		char rdis[MAX_STRING_LENGTH];
		sprintf(rdis, "...%s", char_get_rdisplay(i));
		act(rdis, FALSE, i, 0, ch, TO_VICT);
	}
  }

}

static void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i, *j;
  struct hide_node {
    struct hide_node *next;
    struct char_data *hidden;
  } *hideinfo, *lasthide, *tmphide;
  int num;
  
  hideinfo = lasthide = NULL;
  
  for (i = list; i; i = i->next_in_room) {
    if (char_aff_flagged(i, AFF_HIDE) && roll_resisted(i, SKILL_HIDE, ch, SKILL_SPOT)) {
     if (char_get_skill(i, SKILL_HIDE) && !char_is_npc(ch) && i != ch) {
      improve_skill(i, SKILL_HIDE, 1);
     }
      CREATE(tmphide, struct hide_node, 1);
      tmphide->next = NULL;
      tmphide->hidden = i;
      if (!lasthide) {
        hideinfo = lasthide = tmphide;
      } else {
        lasthide->next = tmphide;
        lasthide = tmphide;
      }
      continue;
    }
  }

  for (i = list; i; i = i->next_in_room) {
    /* hide npcs whose description starts with a '.' from non-holylighted people
    - Idea from Elaseth of TBA */
    if ((ch == i) || (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_HOLYLIGHT) && char_is_npc(i)
    && i->long_descr && *i->long_descr == '.')) 
      continue;

    for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
      if (tmphide->hidden == i)
        break;
    if (tmphide)
      continue;

    if (char_can_see(ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurences of this mob are there? */
        for (j = list; j != i; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (char_get_position(i)      == char_get_position(j)       ) &&
               (char_get_affected_by(i)[0]    == char_get_affected_by(j)[0]     ) &&
               (char_get_affected_by(i)[1]    == char_get_affected_by(j)[1]     ) &&
               (char_get_affected_by(i)[2]    == char_get_affected_by(j)[2]     ) &&
               (char_get_affected_by(i)[3]    == char_get_affected_by(j)[3]     ) &&
               (!char_get_fighting(i) && !char_get_fighting(j)) &&
               (GET_HIT(i) == getEffMaxPL(i) && GET_HIT(j) == getEffMaxPL(j)) &&
               !strcmp(char_get_name(i), char_get_name(j))         ) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              break;
          }
     	if (j!=i)
          /* This will be true where we have already found this
	   * mob for an earlier "i".  The continue pops us out of
	   * the main "i" for loop.
	   */
          continue;
 	for (j = i; j; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (char_get_position(i)      == char_get_position(j)       ) &&
               (char_get_affected_by(i)[0]    == char_get_affected_by(j)[0]     ) &&
               (char_get_affected_by(i)[1]    == char_get_affected_by(j)[1]     ) &&
               (char_get_affected_by(i)[2]    == char_get_affected_by(j)[2]     ) &&
               (char_get_affected_by(i)[3]    == char_get_affected_by(j)[3]     ) &&
               (!char_get_fighting(i) && !char_get_fighting(j)) &&
               (GET_HIT(i) == GET_MAX_HIT(i) && GET_HIT(j) == GET_MAX_HIT(j)) &&
               !strcmp(char_get_name(i), char_get_name(j))         ) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              num++;
        }
      }
      /* Now show this mob's name and other stuff */
      send_to_char(ch, "@w");
      if (num > 1)
        send_to_char(ch, "@D(@Rx@Y%2i@D)@n ", num);
      list_one_char(i, ch);
      send_to_char(ch, "@n");
    } /* processed a character we can see */
    else if (room_is_dark(char_get_in_room(ch)) && !char_can_see_in_dark(ch) &&
           char_aff_flagged(i, AFF_INFRAVISION))
      send_to_char(ch, "@wYou see a pair of glowing red eyes looking your way.@n\r\n");
  } /* loop through all characters in room */
}

static void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode)
{
  int door, door_found = 0, has_light = FALSE, i;
  char dlist1[500];
  char dlist2[500];
  char dlist3[500];
  char dlist4[500];
  char dlist5[500];
  char dlist6[500];
  char dlist7[500];
  char dlist8[500];
  char dlist9[500];
  char dlist10[500];
  char dlist11[500];
  char dlist12[500];

   *dlist1 = '\0';
   *dlist2 = '\0';
   *dlist3 = '\0';
   *dlist4 = '\0';
   *dlist5 = '\0';
   *dlist6 = '\0';
   *dlist7 = '\0';
   *dlist8 = '\0';
   *dlist9 = '\0';
   *dlist10 = '\0';
   *dlist11 = '\0';
   *dlist12 = '\0';

  if (exit_mode == EXIT_OFF) {
   send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  int space = FALSE;
  if (rroom_sect(target_room) == SECT_SPACE && rroom_get_vnum(target_room) >= 20000) {
   space = TRUE;
  }
  if (exit_mode == EXIT_NORMAL && space == FALSE && char_get_in_room(ch) == target_room) {
    /* Compass and Auto-map - Iovan 9-11-10 */
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
    send_to_char(ch, "@w      Compass           Auto-Map            Map Key\r\n");
    send_to_char(ch, "@R     ---------         ----------   -----------------------------\r\n");
    gen_map(ch, 0);
   send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  if (exit_mode == EXIT_NORMAL && space == TRUE) {
    /* printmap */
    send_to_char(ch, "@D------------------------------[@CRadar@D]---------------------------------@n\r\n");
    printmap(target_room, ch, 1, -1);
    send_to_char(ch, "     @D[@wTurn autoexit complete on for directions instead of radar@D]@n\r\n");
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  if (exit_mode == EXIT_COMPLETE || (exit_mode == EXIT_NORMAL && space == FALSE && char_get_in_room(ch) != target_room)) {
    send_to_char(ch, "@D----------------------------[@gObvious Exits@D]-----------------------------@n\r\n");
    if (char_is_affected(ch, AFF_BLIND)) {
      send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
      return;
    }
    if (char_plr_flagged(ch, PLR_EYEC)) {
      send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
      return;
    }
    if (room_is_dark(char_get_in_room(ch)) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
      send_to_char(ch, "It is pitch black...\r\n");
      return;
    }

    /* Is the character using a working light source? */
    for (i = 0; i < NUM_WEARS; i++)
      if (char_get_equipment(ch, i))
        if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT)
          if (obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS))
            has_light = TRUE;
			
	  if (char_plr_flagged(ch, PLR_AURALIGHT)) {
	    has_light = TRUE;
	  }

    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (rroom_get_dir_option(target_room, door) &&
          rroom_get_dir_option(target_room, door)->to_room != NOWHERE) {
        /* We have a door that leads somewhere */
        if (char_adm_flagged(ch, ADM_SEESECRET) || char_get_admlevel(ch) > 4) {
          /* Immortals see everything */
          door_found++;
          char blam[9];
          sprintf(blam, "%s", dirs[door]);
          *blam = toupper(*blam);
          if (door == 6) {
           sprintf(dlist1, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            char argh[100];
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION NORTHWEST@n\r\n");
             log("ERROR: %s found error direction NORTHWEST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist1+strlen(dlist1), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 0) {
           sprintf(dlist2, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION NORTH@n\r\n");
             log("ERROR: %s found error direction NORTH at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[200];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist2+strlen(dlist2), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 7) {
           sprintf(dlist3, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION NORTHEAST@n\r\n");
             log("ERROR: %s found error direction NORTHEAST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist3+strlen(dlist3), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 1) {
           sprintf(dlist4, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION EAST@n\r\n");
             log("ERROR: %s found error direction EAST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist4+strlen(dlist4), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 8) {
           sprintf(dlist5, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION SOUTHEAST@n\r\n");
             log("ERROR: %s found error direction SOUTHEAST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist5+strlen(dlist5), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                  strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 2) {
           sprintf(dlist6, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION SOUTH@n\r\n");
             log("ERROR: %s found error direction SOUTH at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist6+strlen(dlist6), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 9) {
           sprintf(dlist7, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION SOUTHWEST@n\r\n");
             log("ERROR: %s found error direction SOUTHWEST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist7+strlen(dlist7), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 3) {
           sprintf(dlist8, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION WEST@n\r\n");
             log("ERROR: %s found error direction WEST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist8+strlen(dlist8), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 4) {
           sprintf(dlist9, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION UP@n\r\n");
             log("ERROR: %s found error direction UP at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist9+strlen(dlist9), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 5) {
           sprintf(dlist10, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION DOWN@n\r\n");
             log("ERROR: %s found error direction DOWN at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist10+strlen(dlist10), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 10) {
           sprintf(dlist11, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION INSIDE@n\r\n");
             log("ERROR: %s found error direction INSIDE at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist11+strlen(dlist11), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
          if (door == 11) {
           sprintf(dlist12, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
          if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
             send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION OUTSIDE@n\r\n");
             log("ERROR: %s found error direction OUTSIDE at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
             return;
            }
            char argh[100];
            sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            sprintf(dlist12+strlen(dlist12), "                    The %s%s %s %s %s%s.\r\n",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ?
                    "@rsecret@w " : "",
                (rroom_get_dir_option(target_room, door)->keyword &&
                 strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined")) ?
                    fname(rroom_get_dir_option(target_room, door)->keyword) : "opening",
                 strstr(argh, "s ") != NULL ? "are" : "is",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "and unlocked",
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
          }
        }
        else { /* This is what mortal characters see */
          if (!IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED)) {
            /* And the door is open */
            door_found++;
            char blam[9];
            sprintf(blam, "%s", dirs[door]);
            *blam = toupper(*blam);
          if (door == 6) {
           sprintf(dlist1, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 0) {
           sprintf(dlist2, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 7) {
           sprintf(dlist3, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 1) {
           sprintf(dlist4, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 8) {
           sprintf(dlist5, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 2) {
           sprintf(dlist6, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 9) {
           sprintf(dlist7, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 3) {
           sprintf(dlist8, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 4) {
           sprintf(dlist9, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 5) {
           sprintf(dlist10, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 10) {
           sprintf(dlist11, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }
          if (door == 11) {
           sprintf(dlist12, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
          }

          } else if (CONFIG_DISP_CLOSED_DOORS &&
              !IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* But we tell them the door is closed */
              door_found++;
              char blam[9];
              sprintf(blam, "%s", dirs[door]);
              *blam = toupper(*blam);
          if (door == 6) {
           sprintf(dlist1, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 0) {
           sprintf(dlist2, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
          }
          if (door == 7) {
           sprintf(dlist3, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 1) {
           sprintf(dlist4, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 8) {
           sprintf(dlist5, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 2) {
           sprintf(dlist6, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 9) {
           sprintf(dlist7, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 3) {
           sprintf(dlist8, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 4) {
           sprintf(dlist9, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 5) {
           sprintf(dlist10, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 10) {
           sprintf(dlist11, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
          if (door == 11) {
           sprintf(dlist12, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening" );
          }
            }
        }
      }
    }
   if (!door_found)
    send_to_char(ch, " None.\r\n");
   if (strstr(dlist1, "Northwest")) {
    send_to_char(ch, "%s", dlist1);
    *dlist1 = '\0';
   }
   if (strstr(dlist2, "North")) {
    send_to_char(ch, "%s", dlist2);
    *dlist2 = '\0';
   }
   if (strstr(dlist3, "Northeast")) {
    send_to_char(ch, "%s", dlist3);
    *dlist3 = '\0';
   }
   if (strstr(dlist4, "East")) {
    send_to_char(ch, "%s", dlist4);
    *dlist4 = '\0';
   }
   if (strstr(dlist5, "Southeast")) {
    send_to_char(ch, "%s", dlist5);
    *dlist5 = '\0';
   }
   if (strstr(dlist6, "South")) {
    send_to_char(ch, "%s", dlist6);
    *dlist6 = '\0';
   }
   if (strstr(dlist7, "Southwest")) {
    send_to_char(ch, "%s", dlist7);
    *dlist7 = '\0';
   }
   if (strstr(dlist8, "West")) {
    send_to_char(ch, "%s", dlist8);
    *dlist8 = '\0';
   }
   if (strstr(dlist9, "Up")) {
    send_to_char(ch, "%s", dlist9);
    *dlist9 = '\0';
   }
   if (strstr(dlist10, "Down")) {
    send_to_char(ch, "%s", dlist10);
    *dlist10 = '\0';
   }
   if (strstr(dlist11, "Inside")) {
    send_to_char(ch, "%s", dlist11);
    *dlist11 = '\0';
   }
   if (strstr(dlist12, "Outside")) {
    send_to_char(ch, "%s", dlist12);
    *dlist12 = '\0';
   }
   send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
   if (rroom_flagged(target_room, ROOM_HOUSE) && !rroom_flagged(target_room, ROOM_GARDEN1) && !rroom_flagged(target_room, ROOM_GARDEN2)) {
    send_to_char(ch, "@D[@GItems Stored@D: @g%d@D]@n\r\n", check_saveroom_count(ch, NULL));
   }
   if (rroom_flagged(target_room, ROOM_HOUSE) && rroom_flagged(target_room, ROOM_GARDEN1) && !rroom_flagged(target_room, ROOM_GARDEN2)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(ch, NULL));
   }
   if (rroom_flagged(target_room, ROOM_HOUSE) && !rroom_flagged(target_room, ROOM_GARDEN1) && rroom_flagged(target_room, ROOM_GARDEN2)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(ch, NULL));
   }
   if (char_get_radar1(ch) == rroom_get_vnum(target_room) && char_get_radar2(ch) == rroom_get_vnum(target_room) && char_get_radar3(ch) !=  rroom_get_vnum(target_room)) {
    send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
   }
   else if (char_get_radar1(ch) == rroom_get_vnum(target_room) && char_get_radar2(ch) != rroom_get_vnum(target_room) && char_get_radar3(ch) == rroom_get_vnum(target_room)) {
    send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
   }
   else if (char_get_radar1(ch) != rroom_get_vnum(target_room) && char_get_radar2(ch) == rroom_get_vnum(target_room) && char_get_radar3(ch) == rroom_get_vnum(target_room)) {
    send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
   }
   else if (char_get_radar1(ch) == rroom_get_vnum(target_room) && char_get_radar2(ch) == rroom_get_vnum(target_room) && char_get_radar3(ch) == rroom_get_vnum(target_room) && target_room != 0) {
    send_to_char(ch, "@CAll three of your buoys are floating here. Why?@n\r\n");
   }
   else if (char_get_radar1(ch) == rroom_get_vnum(target_room)) {
    send_to_char(ch, "@CYour @cBuoy #1@C is floating here.@n\r\n");
   }
   else if (char_get_radar2(ch) == rroom_get_vnum(target_room)) {
    send_to_char(ch, "@CYour @cBuoy #2@C is floating here.@n\r\n");
   }
   else if (char_get_radar3(ch) == rroom_get_vnum(target_room)) {
    send_to_char(ch, "@CYour @cBuoy #3@C is floating here.@n\r\n");
   }
  }
}

static void do_auto_exits2(room_rnum target_room, struct char_data *ch)
{
  int door, slen = 0;

  send_to_char(ch, "\nExits: ");

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (!rroom_get_dir_option(target_room, door) || rroom_get_dir_option(target_room, door)->to_room == NOWHERE)
      continue;
    if (room_exit_flagged(rroom_get_dir_option(target_room, door), EX_CLOSED))
      continue;

    send_to_char(ch, "%s ", abbr_dirs[door]);
    slen++;
  }

  send_to_char(ch, "%s\r\n", slen ? "" : "None!");
}

ACMD(do_exits)
{
  /* Why duplicate code? */
  if (!char_prf_flagged(ch, PRF_NODEC)) {
   do_auto_exits(char_get_in_room(ch), ch, EXIT_COMPLETE);
  } else {
   do_auto_exits2(char_get_in_room(ch), ch);
  }
}

static const char *exitlevels[] = {
  "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (char_is_npc(ch))
    return;

  one_argument(argument, arg);


  if (!*arg) {
    send_to_char(ch, "Your current autoexit level is %s.\r\n", exitlevels[char_exit_level(ch)]);
    return;
  }
  if (((tp = search_block(arg, exitlevels, FALSE)) == -1)) {
    send_to_char(ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
    case EXIT_OFF:
      char_prf_set(ch, PRF_AUTOEXIT, false);
      char_prf_set(ch, PRF_FULL_EXIT, false);
      break;
    case EXIT_NORMAL:
      char_prf_set(ch, PRF_AUTOEXIT, true);
      char_prf_set(ch, PRF_FULL_EXIT, false);
      break;
    case EXIT_COMPLETE:
      char_prf_set(ch, PRF_AUTOEXIT, true);
      char_prf_set(ch, PRF_FULL_EXIT, true);
      break;
  }
  send_to_char(ch, "Your @rautoexit level@n is now %s.\r\n", exitlevels[char_exit_level(ch)]);
}

void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief)
{
  struct room_data *rm = &world[char_get_in_room(ch)];
  trig_data *t;

  if (!ch->desc)
    return;

  if (room_is_dark(target_room) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  } else if (char_plr_flagged(ch, PLR_EYEC)) {
      send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
      return;
    }
  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];

    sprintbitarray(rroom_get_room_flags(target_room), room_bits, RF_ARRAY_MAX, buf, sizeof(buf));
    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "\r\n@wO----------------------------------------------------------------------O@n\r\n");
    }

    send_to_char(ch, "@wLocation: @G%-70s@w\r\n", world[target_room].name);
    if (SCRIPT(rm)) {
      send_to_char(ch, "@D[@GTriggers");
      for (t = TRIGGERS(SCRIPT(rm)); t; t = t->next)
        send_to_char(ch, " %d", GET_TRIG_VNUM(t));
      send_to_char(ch, "@D] ");
    }
    sprintf(buf3, "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n", buf, buf2, rroom_get_vnum(target_room));
    send_to_char(ch, "@wFlags: %-70s@w\r\n", buf3);
   if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
    send_to_char(ch, "@wO----------------------------------------------------------------------O@n\r\n");
   }
  } else {
     if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------------------------O@n\r\n");
     }
     send_to_char(ch, "@wLocation: %-70s@n\r\n", world[target_room].name);
     if (rroom_flagged(target_room, ROOM_EARTH)) {
      send_to_char(ch, "@wPlanet: @GEarth@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_CERRIA)) {
      send_to_char(ch, "@wPlanet: @RCerria@n\r\n");
     }
     else if (rroom_planet_zenith(target_room)) {
      send_to_char(ch, "@wPlanet: @BZenith@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_AETHER)) {
      send_to_char(ch, "@wPlanet: @MAether@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_FRIGID)) {
      send_to_char(ch, "@wPlanet: @CFrigid@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_SPACE)) {
      send_to_char(ch, "@wPlanet: @DNone@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_VEGETA)) {
      send_to_char(ch, "@wPlanet: @YVegeta@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_NAMEK)) {
      send_to_char(ch, "@wPlanet: @gNamek@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_KONACK)) {
      send_to_char(ch, "@wPlanet: @MKonack@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_NEO)) {
      send_to_char(ch, "@wPlanet: @WNeo Nirvana@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_AL)) {
      send_to_char(ch, "@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_HELL)) {
      send_to_char(ch, "@wDimension: @RPunishment Hell@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_RHELL)) {
      send_to_char(ch, "@wDimension: @RH@re@Dl@Rl@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_YARDRAT)) {
      send_to_char(ch, "@wPlanet: @mYardrat@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_KANASSA)) {
      send_to_char(ch, "@wPlanet: @BKanassa@n\r\n");
     }
     else if (rroom_flagged(target_room, ROOM_ARLIA)) {
      send_to_char(ch, "@wPlanet: @GArlia@n\r\n");
     }

     else {
      send_to_char(ch, "@wPlanet: @WUNKNOWN@n\r\n");
     }
     if (rroom_get_gravity(target_room) <= 0) {
      send_to_char(ch, "@wGravity: @WNormal@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 10) {
      send_to_char(ch, "@wGravity: @W10x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 20) {
      send_to_char(ch, "@wGravity: @W20x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 30) {
      send_to_char(ch, "@wGravity: @W30x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 40) {
      send_to_char(ch, "@wGravity: @W40x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 50) {
      send_to_char(ch, "@wGravity: @W50x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 100) {
      send_to_char(ch, "@wGravity: @W100x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 200) {
      send_to_char(ch, "@wGravity: @W200x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 300) {
      send_to_char(ch, "@wGravity: @W300x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 400) {
      send_to_char(ch, "@wGravity: @W400x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 500) {
      send_to_char(ch, "@wGravity: @W500x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 1000) {
      send_to_char(ch, "@wGravity: @W1,000x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 5000) {
      send_to_char(ch, "@wGravity: @W5,000x@n\r\n");
     }
     else if (rroom_get_gravity(target_room) == 10000) {
      send_to_char(ch, "@wGravity: @W10,000x@n\r\n");
     }
     if (rroom_flagged(target_room, ROOM_REGEN)) {
      send_to_char(ch, "@CA feeling of calm and relaxation fills this room.@n\r\n");
     }
     if (rroom_flagged(target_room, ROOM_AURA)) {
      send_to_char(ch, "@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
     }
     if (rroom_flagged(target_room, ROOM_HBTC)) {
      send_to_char(ch, "@rThis room feels like it opperates in a different time frame.@n\r\n");
     }
     if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------------------------O@n\r\n");
     }
    }

  if ((!char_is_npc(ch) && !char_prf_flagged(ch, PRF_BRIEF)) || rroom_flagged(target_room, ROOM_DEATH)) {
    if (rroom_get_dmg(target_room) <= 99) {
    send_to_char(ch, "@w%s@n", world[target_room].description);
    }
    if (rroom_get_dmg(target_room) == 100 && (rroom_sect(target_room) == SECT_WATER_SWIM || rroom_sunken(target_room) || rroom_sect(target_room) == SECT_FLYING || rroom_sect(target_room) == SECT_SHOP || rroom_sect(target_room) == SECT_IMPORTANT)) {
    send_to_char(ch, "@w%s@n", world[target_room].description);
    }
    if (rroom_sect(target_room) == SECT_INSIDE && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
     if (rroom_get_dmg(target_room) <= 2) {
      send_to_char(ch, "@wA small hole with chunks of debris that can be seen scarring the floor.@n");
     }
     else if (rroom_get_dmg(target_room) <= 4) {
      send_to_char(ch, "@wA couple small holes with chunks of debris that can be seen scarring the floor.@n");
     }
     else if (rroom_get_dmg(target_room) <= 6) {
      send_to_char(ch, "@wA few small holes with chunks of debris that can be seen scarring the floor.@n");
     }
     else if (rroom_get_dmg(target_room) <= 10) {
      send_to_char(ch, "@wThere are several small holes with chunks of debris that can be seen scarring the floor.@n");
     }
     else if (rroom_get_dmg(target_room) <= 20) {
      send_to_char(ch, "@wMany holes fill the floor of this area, many of which have burn marks.@n");
     }
     else if (rroom_get_dmg(target_room) <= 30) {
      send_to_char(ch, "@wThe floor is severely damaged with many large holes.@n");
     }
     else if (rroom_get_dmg(target_room) <= 50) {
      send_to_char(ch, "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 75) {
      send_to_char(ch, "@wThis entire area is falling apart, it has been damaged so badly.@n");
     }
     else if (rroom_get_dmg(target_room) <= 99) {
      send_to_char(ch, "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
     }
     else if (rroom_get_dmg(target_room) >= 100) {
      send_to_char(ch, "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up holes, and overflowing onto what is left of the\r\nfloor. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
     }
      send_to_char(ch, "\r\n");
    }
    else if ((rroom_sect(target_room) == SECT_CITY || rroom_sect(target_room) == SECT_FIELD || rroom_sect(target_room) == SECT_HILLS || rroom_sect(target_room) == SECT_IMPORTANT) && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
     if (rroom_get_dmg(target_room) <= 2) {
      send_to_char(ch, "@wA small hole with chunks of debris that can be seen scarring the ground.@n");
     }
     else if (rroom_get_dmg(target_room) <= 4) {
      send_to_char(ch, "@wA couple small craters with chunks of debris that can be seen scarring the ground.@n");
     }
     else if (rroom_get_dmg(target_room) <= 6) {
      send_to_char(ch, "@wA few small craters with chunks of debris that can be seen scarring the ground.@n");
     }
     else if (rroom_get_dmg(target_room) <= 10) {
      send_to_char(ch, "@wThere are several small craters with chunks of debris that can be seen scarring the ground.@n");
     }
     else if (rroom_get_dmg(target_room) <= 20) {
      send_to_char(ch, "@wMany craters fill the ground of this area, many of which have burn marks.@n");
     }
     else if (rroom_get_dmg(target_room) <= 30) {
      send_to_char(ch, "@wThe ground is severely damaged with many large craters.@n");
     }
     else if (rroom_get_dmg(target_room) <= 50) {
      send_to_char(ch, "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 75) {
      send_to_char(ch, "@wThis entire area is falling apart, it has been damaged so badly.@n");
     }
     else if (rroom_get_dmg(target_room) <= 99) {
      send_to_char(ch, "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
     }
     else if (rroom_get_dmg(target_room) >= 100) {
      send_to_char(ch, "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up craters, and overflowing onto what is left of the\r\nground. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
     }
      send_to_char(ch, "\r\n");
    }
    else if (rroom_sect(target_room) == SECT_FOREST && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
     if (rroom_get_dmg(target_room) <= 2) {
      send_to_char(ch, "@wA small tree sits in a little crater here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 4) {
      send_to_char(ch, "@wTrees have been uprooted by craters in the ground.@n");
     }
     else if (rroom_get_dmg(target_room) <= 6) {
      send_to_char(ch, "@wSeveral trees have been reduced to chunks of debris and are\r\nlaying in a few craters here. @n");
     }
     else if (rroom_get_dmg(target_room) <= 10) {
      send_to_char(ch, "@wA large patch of trees have been destroyed and are laying in craters here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 20) {
      send_to_char(ch, "@wSeveral craters have merged into one large crater in one part of this forest.@n");
     }
     else if (rroom_get_dmg(target_room) <= 30) {
      send_to_char(ch, "@wThe open sky can easily be seen through a hole of trees destroyed\r\nand resting at the bottom of several craters here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 50) {
      send_to_char(ch, "@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 75) {
      send_to_char(ch, "@wVery few trees are left standing in this area, replaced instead by large craters.@n");
     }
     else if (rroom_get_dmg(target_room) <= 99) {
      send_to_char(ch, "@wSingle solitary trees can be found still standing here or there in the area.\r\nThe rest have been almost completely obliterated in recent conflicts.@n");
     }
     else if (rroom_get_dmg(target_room) >= 100) {
      send_to_char(ch, "@w  One massive crater fills this area. This desolate crater leaves no\r\nevidence of what used to be found in the area. Smoke slowly wafts into\r\nthe sky from the central point of the crater, creating an oppressive\r\natmosphere.@n");
     }
      send_to_char(ch, "\r\n");
    }
    else if (rroom_sect(target_room) == SECT_MOUNTAIN && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
     if (rroom_get_dmg(target_room) <= 2) {
      send_to_char(ch, "@wA small crater has been burned into the side of this mountain.@n");
     }
     else if (rroom_get_dmg(target_room) <= 4) {
      send_to_char(ch, "@wA couple craters have been burned into the side of this mountain.@n");
     }
     else if (rroom_get_dmg(target_room) <= 6) {
      send_to_char(ch, "@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
     }
     else if (rroom_get_dmg(target_room) <= 10) {
      send_to_char(ch, "@wSeveral bad craters can be seen in the side of the mountain here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 20) {
      send_to_char(ch, "@wLarge boulders have rolled down the mountain side and collected in many nearby craters.@n");
     }
     else if (rroom_get_dmg(target_room) <= 30) {
      send_to_char(ch, "@wMany craters are covering the mountainside here.@n");
     }
     else if (rroom_get_dmg(target_room) <= 50) {
      send_to_char(ch, "@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
     }
     else if (rroom_get_dmg(target_room) <= 75) {
      send_to_char(ch, "@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
     }
     else if (rroom_get_dmg(target_room) <= 99) {
      send_to_char(ch, "@wThe mountain side here has completely collapsed, shedding dangerous rubble down to its base.@n");
     }
     else if (rroom_get_dmg(target_room) >= 100) {
      send_to_char(ch, "@w  Half the mountain has been blown away, leaving a scarred and jagged\r\nrock in its place. Billowing smoke wafts up from several parts of the\r\nmountain, filling the nearby skies and blotting out the sun.@n");
     }
      send_to_char(ch, "\r\n");
    }
    if (rroom_get_geffect(target_room) >= 1 && rroom_get_geffect(target_room) <= 5) {
      send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
    }
    if (rroom_get_geffect(target_room) >= 6) {
      send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
    }
    if (rroom_get_geffect(target_room) < 0) {
      send_to_char(ch, "@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
    }
  }
  /* autoexits */
  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NODEC))
    do_auto_exits2(target_room, ch);
  if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC))
   do_auto_exits(target_room, ch, char_exit_level(ch));

  /* now list characters & objects */
   if (rroom_flagged(target_room, ROOM_GARDEN1)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(ch, NULL));
   } else if (rroom_flagged(target_room, ROOM_GARDEN2)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(ch, NULL));
   } else if (rroom_flagged(target_room, ROOM_HOUSE)) {
    send_to_char(ch, "@D[@GItems Stored@D: @g%d@D]@n\r\n", check_saveroom_count(ch, NULL));
   }
  list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_LONG, FALSE);
  list_char_to_char(world[target_room].people, ch);
}

static void look_in_direction(struct char_data *ch, int dir)
{
  if (char_get_exit(ch, dir)) {
    if (char_get_exit(ch, dir)->general_description)
      send_to_char(ch, "%s", char_get_exit(ch, dir)->general_description);
    else {
     struct obj_data *obj, *next_obj;
     int founded = FALSE;
     for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
       if (obj_get_vnum(obj) == 17) {
        founded = TRUE;
       }
     }
     if (founded == FALSE) {
      send_to_char(ch, "You were unable to discern anything about that direction. Try looking again...\r\n");
      struct obj_data *obj;
      obj = read_object(17, VIRTUAL);
      obj_to_char(obj, ch);
     }
    }

    if (room_exit_flagged(char_get_exit(ch, dir), EX_ISDOOR) && char_get_exit(ch, dir)->keyword) {
      if (!room_exit_flagged(char_get_exit(ch, dir), EX_SECRET) &&
           room_exit_flagged(char_get_exit(ch, dir), EX_CLOSED) )
        send_to_char(ch, "The %s is closed.\r\n", fname(char_get_exit(ch, dir)->keyword));
      else if (!room_exit_flagged(char_get_exit(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is open.\r\n", fname(char_get_exit(ch, dir)->keyword));
    }
  } else
    send_to_char(ch, "Nothing special there...\r\n");
}

static void look_in_obj(struct char_data *ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if (find_exdesc(arg, obj->ex_description) != NULL && !bits)
      send_to_char(ch, "There's nothing inside that!\r\n");
    else if ((obj_get_type_flag(obj) == ITEM_PORTAL) && !obj_val_flagged(obj, CONT_CLOSEABLE)) { 
    if (obj_get_value(obj, VAL_PORTAL_APPEAR) < 0) {
      /* You can look through the portal to the destination */
      /* where does this lead to? */
      room_rnum portal_dest = real_room(obj_get_value(obj, VAL_PORTAL_DEST)); 
      if (portal_dest == NOWHERE) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else if (room_is_dark(portal_dest) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else {
       send_to_char(ch, "After seconds of concentration you see the image of %s.\r\n", world[portal_dest].name);
      }
    } else if (obj_get_value(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
     /* display the appropriate description from the list of descriptions
*/
      send_to_char(ch, "%s\r\n", portal_appearance[obj_get_value(obj, VAL_PORTAL_APPEAR)]);
    } else {
      /* We shouldn't really get here, so give a default message */
      send_to_char(ch, "All you can see is the glow of the portal.\r\n");
    }
  } else if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
    if (obj_val_flagged(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else if (obj_get_value(obj, VAL_VEHICLE_APPEAR) < 0) {
      /* You can look inside the vehicle */
      /* where does this lead to? */
      room_rnum vehicle_inside = real_room(obj_get_value(obj, VAL_VEHICLE_ROOM)); 
      if (vehicle_inside == NOWHERE) {
        send_to_char(ch, "You cannot see inside that.\r\n");
      } else if (room_is_dark(vehicle_inside) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "It is pitch black...\r\n");
      } else {
        send_to_char(ch, "You look inside and see:\r\n");
        look_at_room(vehicle_inside, ch, 0);
      }
    } else {
      send_to_char(ch, "You cannot see inside that.\r\n");
    }
  } else if (obj_get_type_flag(obj) == ITEM_WINDOW) {
    look_out_window(ch, arg);
  } else if ((obj_get_type_flag(obj) != ITEM_DRINKCON) &&
	     (obj_get_type_flag(obj) != ITEM_FOUNTAIN) &&
	     (obj_get_type_flag(obj) != ITEM_CONTAINER)&&
             (obj_get_type_flag(obj) != ITEM_PORTAL))     {
    send_to_char(ch, "There's nothing inside that!\r\n");
  } else if ((obj_get_type_flag(obj) == ITEM_CONTAINER) || 
             (obj_get_type_flag(obj) == ITEM_PORTAL)) {
      if (obj_val_flagged(obj, CONT_CLOSED))
	send_to_char(ch, "It is closed.\r\n");
      else {
	send_to_char(ch, "%s", obj->short_description);
        if (obj_get_type_flag(obj) == ITEM_CONTAINER && (obj_get_vnum(obj) == 697 || obj_get_vnum(obj) == 698 || obj_get_vnum(obj) == 682 || obj_get_vnum(obj) == 683 || obj_get_vnum(obj) == 684)) {
         act("$n looks in $p.", TRUE, ch, obj, 0, TO_ROOM);
        }
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(ch, " (carried): \r\n");
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(ch, " (here): \r\n");
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(ch, " (used): \r\n");
	  break;
	}

	list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (obj_get_value(obj, VAL_DRINKCON_HOWFULL) <= 0 && (!obj_get_value(obj, VAL_DRINKCON_CAPACITY) == 1))
	send_to_char(ch, "It is empty.\r\n");
      else {
        if (obj_get_value(obj, VAL_DRINKCON_CAPACITY) < 0)
        {
          char buf2[MAX_STRING_LENGTH];
	  sprinttype(obj_get_value(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
	  send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        }
       else if (obj_get_value(obj, VAL_DRINKCON_HOWFULL)>obj_get_value(obj, VAL_DRINKCON_CAPACITY)) {
	  send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
          char buf2[MAX_STRING_LENGTH];
	  amt = obj_get_value(obj, VAL_DRINKCON_CAPACITY);
          int leftin = obj_get_value(obj, VAL_DRINKCON_HOWFULL);
	  sprinttype(obj_get_value(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
          if (leftin == amt) {
           send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
          } else if (leftin >= amt * .8) {
           send_to_char(ch, "It's almost full of a %s liquid.\r\n", buf2);
          } else if (leftin >= amt * .5) {
           send_to_char(ch, "It's about half full of a %s liquid.\r\n", buf2);
          } else if (leftin >= amt * .2) {
           send_to_char(ch, "It's less than half full of a %s liquid.\r\n", buf2);
          } else if (leftin > 0) {
           send_to_char(ch, "It's barely filled with a %s liquid.\r\n", buf2);
          } else {
           send_to_char(ch, "It's empty.\r\n");
          }
	}
      }
    }
}

char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    /*if (isname(word, i->keyword))*/
    if (*i->keyword == '.' ? isname(word, i->keyword + 1) : isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
static void look_at_target(struct char_data *ch, char *arg, int cmread)
{
  int bits, found = FALSE, j, fnum, i = 0, msg = 1;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  char number[MAX_STRING_LENGTH];

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  if (cmread) {
    for (obj = ch->carrying; obj;obj=obj->next_content) {
      if(obj_get_type_flag(obj) == ITEM_BOARD) {
	found = TRUE;
	break;
      }
    }
    if(!obj) {
      for (obj = world[char_get_in_room(ch)].contents; obj;obj=obj->next_content) {
	if(obj_get_type_flag(obj) == ITEM_BOARD) {
	  found = TRUE;
	  break;
	}
      }
    }
    if (obj) {
      arg = one_argument(arg, number);
      if (!*number) {
	send_to_char(ch,"Read what?\r\n");
	return;
      }
      
      /* Okay, here i'm faced with the fact that the person could be
	 entering in something like 'read 5' or 'read 4.mail' .. so, whats the
	 difference between the two?  Well, there's a period in the second,
	 so, we'll just stick with that basic difference */
      
      if (isname(number, obj->name)) {
	show_board(obj_get_vnum(obj), ch);
      } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
		 (strchr(number,'.'))) {
	sprintf(arg,"%s %s", number,arg);
	look_at_target(ch, arg, 0);
      } else {
	board_display_msg(obj_get_vnum(obj), ch, msg);
      }
    }
  } else {
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (!char_aff_flagged(ch, AFF_HIDE)) {
	    act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
        act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
      }
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg))) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[char_get_in_room(ch)].ex_description)) != NULL && ++i == fnum) {
    page_string(ch->desc, desc, FALSE);
    return;
  } 

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (char_get_equipment(ch, j) && char_can_see_obj(ch, char_get_equipment(ch, j)))
      if ((desc = find_exdesc(arg, char_get_equipment(ch, j)->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
        if (isname(arg, char_get_equipment(ch, j)->name)) {
          if (obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_WEAPON) {
            send_to_char(ch, "The weapon type of %s is a %s.\r\n", 
              obj_get_short_description(char_get_equipment(ch, j)), 
              weapon_type[(int) obj_get_value(char_get_equipment(ch, j), VAL_WEAPON_SKILL)]);
          }
          if (obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_SPELLBOOK) {
            display_spells(ch, char_get_equipment(ch, j));
          }
          if (obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_SCROLL) {
            display_scroll(ch, char_get_equipment(ch, j));
          }
          diag_obj_to_char(char_get_equipment(ch, j), ch);
          send_to_char(ch, "It appears to be made of %s", material_names[obj_get_value(char_get_equipment(ch, j), 7)]);
        }
	found = TRUE;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (char_can_see_obj(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if (obj_get_type_flag(obj) == ITEM_BOARD) {
	  show_board(obj_get_vnum(obj), ch);
	} else {
	  send_to_char(ch, "%s", desc);
          if (isname(arg, obj->name)) {
            if (obj_get_type_flag(obj) == ITEM_WEAPON) {
              send_to_char(ch, "The weapon type of %s is a %s.\r\n", 
                obj_get_short_description(obj), weapon_type[(int) obj_get_value(obj, VAL_WEAPON_SKILL)]);
            }
            if (obj_get_type_flag(obj) == ITEM_SPELLBOOK) {
              display_spells(ch, obj);
            }
            if (obj_get_type_flag(obj) == ITEM_SCROLL) {
              display_scroll(ch, obj);
            }
            diag_obj_to_char(obj, ch);
            send_to_char(ch, "It appears to be made of %s, and weights %s", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
          }
	}
	found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[char_get_in_room(ch)].contents; obj && !found; obj = obj->next_content)
    if (char_can_see_obj(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if(obj_get_type_flag(obj) == ITEM_BOARD) {
	  show_board(obj_get_vnum(obj), ch);
	} else {
	send_to_char(ch, "%s", desc);
        if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
         send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @CEnter hatch\r\n");
        } else if (obj_get_type_flag(obj) == ITEM_HATCH) {
         send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
         send_to_char(ch, "@YSyntax@D: @CLeave@n\r\n");
        } else if (obj_get_type_flag(obj) == ITEM_WINDOW) {
         look_out_window(ch, obj->name);
        }

         if (obj_get_type_flag(obj) == ITEM_CONTROL) {
         send_to_char(ch, "@RFUEL@D: %s%s@n\r\n", obj_get_value(obj, 2) >= 200 ? "@G" : obj_get_value(obj, 2) >= 100 ? "@Y" : "@r", add_commas(obj_get_value(obj, 2)));
        }
        if (obj_get_type_flag(obj) == ITEM_WEAPON) {
          send_to_char(ch, "The weapon type of %s is a %s.\r\n", obj_get_short_description(obj), weapon_type[(int) obj_get_value(obj, VAL_WEAPON_SKILL)]);
        }
        diag_obj_to_char(obj, ch);
        send_to_char(ch, "It appears to be made of %s, and weights %s", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
	}
	found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
    else {
      if (show_obj_modifiers(found_obj, ch))
      send_to_char(ch, "\r\n");
    }
  } else if (!found)
    send_to_char(ch, "You do not see that here.\r\n");
  }
}

static void look_out_window(struct char_data *ch, char *arg)
{
  struct obj_data *i, *viewport = NULL, *vehicle = NULL;
  struct char_data *dummy = NULL;
  room_rnum target_room = NOWHERE;
  int bits, door;

  /* First, lets find something to look out of or through. */
  if (*arg) {
    /* Find this object and see if it is a window */
    if (!(bits = generic_find(arg,
              FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
              ch, &dummy, &viewport))) {
      send_to_char(ch, "You don't see that here.\r\n");
      return;
    } else if (obj_get_type_flag(viewport) != ITEM_WINDOW) {
      send_to_char(ch, "You can't look out that!\r\n");
    return;
  }
  } else if (char_is_outside(ch)) {
      /* yeah, sure stupid */
      send_to_char(ch, "But you are already outside.\r\n");
    return;
  } else {
    /* Look for any old window in the room */
    for (i = world[char_get_in_room(ch)].contents; i; i = i->next_content)
      if ((obj_get_type_flag(i) == ITEM_WINDOW) &&
           isname("window", i->name)) {
        viewport = i;
      continue;
      }
  }
  if (!viewport) {
    /* Nothing suitable to look through */
    send_to_char(ch, "You don't seem to be able to see outside.\r\n");
  } else if (obj_val_flagged(viewport, CONT_CLOSEABLE) &&
             obj_val_flagged(viewport, CONT_CLOSED)) {
    /* The window is closed */
    send_to_char(ch, "It is closed.\r\n");
  } else {
    if (obj_get_value(viewport, VAL_WINDOW_UNUSED1) < 0) {
      /* We are looking out of the room */
      if (obj_get_value(viewport, VAL_WINDOW_UNUSED4) < 0) {
        /* Look for the default "outside" room */
        for (door = 0; door < NUM_OF_DIRS; door++)
          if (char_get_exit(ch, door))
            if (char_get_exit(ch, door)->to_room != NOWHERE)
              if (!rroom_flagged(char_get_exit(ch, door)->to_room, ROOM_INDOORS)) {
                target_room = char_get_exit(ch, door)->to_room;
                continue;
              }
      } else {
        target_room = real_room(obj_get_value(viewport, VAL_WINDOW_UNUSED4));
      }
    } else {
      /* We are looking out of a vehicle */
      if ( (vehicle = find_vehicle_by_vnum(obj_get_value(viewport, VAL_WINDOW_UNUSED1))) )
        target_room = obj_get_in_room(vehicle);
    }
    if (target_room == NOWHERE) {
      send_to_char(ch, "You don't seem to be able to see outside.\r\n");
    } else {
      if (viewport->action_description)
        act(viewport->action_description, TRUE, ch, viewport, 0, TO_CHAR);
      else
        act("$n looks out the window.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "You look outside and see:\r\n");
      look_at_room(target_room, ch, 0);
    }
  }
}

ACMD(do_finger)
{

 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);
 
 if (!*arg) {
  send_to_char(ch, "What user are you wanting to look at?\r\n");
  return;
 }
 if (!readUserIndex(arg)) {
  send_to_char(ch, "That user does not exist\r\n");
  return;
 }
 else {
  fingerUser(ch, arg);
  return;
 }
}

ACMD(do_rptrans)
{
    struct char_data *vict = NULL;
    struct descriptor_data *k;
    int amt = 0;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);
 
    if (!*arg || !*arg2) {
      send_to_char(ch, "Syntax: exchange (target) (amount)\r\n");
      return;
    }

    amt = atoi(arg2);

    if (amt <= 0) {
     send_to_char(ch, "Are you being funny?\r\n");
     return; 
    }

    if (amt > char_get_rp(ch)) {
     send_to_char(ch, "@WYou only have @C%d@W RPP!@n\r\n", char_get_rp(ch));
     return;
    }

    if(!readUserIndex(arg)) {
     send_to_char(ch, "That is not a recognised user file.\r\n");
     return;
    }

    for (k = descriptor_list; k; k = k->next) {
     if (char_is_npc(k->character))
      continue;
     if (desc_get_connected(k) != CON_PLAYING)
      continue;
     if (!strcasecmp(k->user, arg))
      vict = k->character;
    }
    if (vict == NULL) {
     userWrite(NULL, 0, amt, 0, arg);
    }
    else {
     char_set_rp(vict, char_get_rp(vict) + (amt));
     vict->desc->rpp += amt;
     userWrite(vict->desc, 0, 0, 0, "index");
     char_set_trp(vict, char_get_trp(vict) + (amt));
     save_char(vict);
     send_to_char(vict, "@W%s gives @C%d@W of their RPP to you. How nice!\r\n", char_get_name(ch), amt);
    }
    char_set_rp(ch, char_get_rp(ch) - (amt));
    ch->desc->rpp -= amt;
    userWrite(ch->desc, 0, 0, 0, "index");          
    send_to_char(ch, "@WYou exchange @C%d@W RPP to user @c%s@W for a warm fuzzy feeling.\r\n", amt, CAP(arg));
    mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "EXCHANGE: %s gave %d RPP to user %s", char_get_name(ch), amt, arg);
    save_char(ch);
}

/*By Andros*/
ACMD(do_rpbank) 
{
    int amt = 0;
    char arg[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg);
	
	if (!*arg) {
	  send_to_char(ch, "Syntax: rpbank <amount>\r\n");
	  send_to_char(ch, "Current RPP Bank: %d\r\n", char_get_rbank(ch));
	  return;
	}
	
	amt = atoi(arg);
	
	if (amt <= 0) {
	  send_to_char(ch, "You cannot withdraw from the RPP Bank.\r\n");
	  return;
	}
	
	if (amt > char_get_rp(ch)) {
	  send_to_char(ch, "You do not have that much RPP to send to the Bank!\r\n");
	  return;
	}
	
	char_set_rp(ch, char_get_rp(ch) - (amt));
	ch->desc->rpp -= amt;
	char_set_rbank(ch, char_get_rbank(ch) + (amt));
	ch->desc->rbank += amt;
	send_to_char(ch, "You send %d to your RPP Bank. Your total is now %d.\r\n", amt, char_get_rbank(ch));
	mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "RPP Bank: %s has put %d RPP into their bank", char_get_name(ch), amt);
	
}

/* Rillao: An attempt was made. */
ACMD(do_rbanktrans)
{
    struct char_data *vict = NULL;
    struct descriptor_data *k;
    int amt = 0;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);
 
    if (!*arg || !*arg2) {
      send_to_char(ch, "Syntax: bexchange (target) (amount)\r\n");
      return;
    }

    amt = atoi(arg2);

    if (amt <= 0) {
     send_to_char(ch, "Are you being funny?\r\n");
     return; 
    }

    if (amt > char_get_rbank(ch)) {
     send_to_char(ch, "@WYou only have @C%d@W Banked RPP!@n\r\n", char_get_rbank(ch));
     return;
    }

    if(!readUserIndex(arg)) {
     send_to_char(ch, "That is not a recognised user file.\r\n");
     return;
    }

    for (k = descriptor_list; k; k = k->next) {
     if (char_is_npc(k->character))
      continue;
     if (desc_get_connected(k) != CON_PLAYING)
      continue;
     if (!strcasecmp(k->user, arg))
      vict = k->character;
    }
    if (vict == NULL) {
     userWrite(NULL, 0, amt, 0, arg);
    }
    else {
     char_set_rbank(vict, char_get_rbank(vict) + (amt));
     vict->desc->rbank += amt;
     userWrite(vict->desc, 0, 0, 0, "index");
     save_char(vict);
     send_to_char(vict, "@W%s gives @C%d@W of their Banked RPP to you. How nice!\r\n", char_get_name(ch), amt);
    }
    char_set_rbank(ch, char_get_rbank(ch) - (amt));
    ch->desc->rbank -= amt;
    userWrite(ch->desc, 0, 0, 0, "index");          
    send_to_char(ch, "@wWELL @xGOLLY @xGEE @xWILLICKERS@x! @wYOU @RZ@YIM @RZ@YAMMED @C%d @wRIPROOZLES TO @C%s!@n\r\n", amt, CAP(arg));
    mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "EXCHANGE: %s gave %d Banked RPP to user %s", char_get_name(ch), amt, arg);
    save_char(ch);
}

ACMD(do_rdisplay)
{
	skip_spaces(&argument);
	
	if (char_is_npc(ch)) {
		return;
	}
	
	if (!*argument) {
		send_to_char(ch, "Clearing room display.\r\n");
		char_set_rdisplay(ch, "Empty");
	}
	else {
		char derp[MAX_STRING_LENGTH];
	
		strcpy(derp, argument);
		
		send_to_char(ch, "You set your display to; %s\r\n", derp);
		char_set_rdisplay(ch, strdup(derp));
	}
}

int perf_skill(int skill)
{

 if (!skill) {
  return 0;
 }

 switch (skill) {
  case 464:
    return 1;
   break;
  case 441:
    return 1;
   break;
  case 444:
    return 1;
   break;
  case 475:
    return 1;
   break;
  case 474:
    return 1;
   break;
  case 476:
    return 1;
   break;
  case 488:
    return 1;
   break;
  case 472:
    return 1;
   break;
  case 485:
    return 1;
   break;
  case 442:
    return 1;
   break;
  case 510:
    return 1;
   break;
  case 533:
    return 1;
    break;
  default:
    return 0;
   break;
 }
}

ACMD(do_perf)
{
 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 int i, skill = 1, found = FALSE, type = 0;

 two_arguments(argument, arg, arg2);

 if (char_is_npc(ch) || char_get_admlevel(ch) > 0) {
  send_to_char(ch, "I don't think so.\r\n");
  return;
 }

 if (!*arg || !*arg2) {
   send_to_char(ch, "@WType @G1@D: @wOver Charged@n\r\n");
   send_to_char(ch, "@WType @G2@D: @wAccurate@n\r\n");
   send_to_char(ch, "@WType @G3@D: @wEfficient@n\r\n");
   send_to_char(ch, "Syntax: perfect (skillname) (type 1/2/or 3)\r\n");
   return;
 } 
 if (strlen(arg) < 4) {
  send_to_char(ch, "The skill name should be longer than 3 characters...\r\n");
  return;
 }
 for (i = 0; i < SKILL_TABLE_SIZE; i++) {
  if (spell_info[i].skilltype != SKTYPE_SKILL)
   continue;
  
  if (found == TRUE)
   continue;

  if (strstr(spell_info[i].name, arg)) {
   skill = i;
   found = TRUE;
  }
 }
 if (found == FALSE) {
  send_to_char(ch, "The skill %s doesn't exist.\r\n", arg);
  return;
 }
 if (!char_get_skill(ch, skill)) {
  send_to_char(ch, "You don't know %s.\r\n", arg);
  return;
 }
 if (char_get_skill(ch, skill) < 100) {
  send_to_char(ch, "You have not mastered the skill %s and thus can't perfect it.\r\n", arg);
  return;
 }
 if (char_get_skills_perf(ch, skill) > 0) {
  send_to_char(ch, "You have already mastered the skill %s and chosen how to perfect it.\r\n", arg);
  return;
 }
 if (!perf_skill(skill)) {
  send_to_char(ch, "You can't perfect that type of skill.\r\n");
  return;
 }
 if (atoi(arg2) < 1 || atoi(arg2) > 3) {
  send_to_char(ch, "@WType @G1@D: @wOver Charged@n\r\n");
  send_to_char(ch, "@WType @G2@D: @wAccurate@n\r\n");
  send_to_char(ch, "@WType @G3@D: @wEfficient@n\r\n");
  send_to_char(ch, "@RType must be a number between 1 and 3.@n\r\n");
  return;
 }
 else {
  type = atoi(arg2);
  switch (type) {
   case 1:
    send_to_char(ch, "You perfect the skill %s so that you can over charge it!\r\n", spell_info[skill].name);
    char_set_skills_perf(ch, skill, 1);
    break;
   case 2:
    send_to_char(ch, "You perfect the skill %s so that you have supreme accuracy with it!\r\n", spell_info[skill].name);
    char_set_skills_perf(ch, skill, 2);
    break;
   case 3:
    send_to_char(ch, "You perfect the skill %s so that you require a lower minimum charge for it!\r\n", spell_info[skill].name);
    char_set_skills_perf(ch, skill, 3);
    break;
  }
 }

}

ACMD(do_look)
{
  int look_type;

  if (!ch->desc)
    return;
  if (char_get_position(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (char_aff_flagged(ch, AFF_BLIND))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (char_plr_flagged(ch, PLR_EYEC))
      send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
  else if (room_is_dark(char_get_in_room(ch)) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[char_get_in_room(ch)].people, ch);	/* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[200];

    if (subcmd == SCMD_READ) {
      one_argument(argument, arg);
      if (!*arg)
	send_to_char(ch, "Read what?\r\n");
      else
	look_at_target(ch, arg, 1);
      return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
      if (subcmd == SCMD_SEARCH) {
       search_room(ch);
      } else {
       look_at_room(char_get_in_room(ch), ch, 1);
       if (char_get_admlevel(ch) < 1 && !char_aff_flagged(ch, AFF_HIDE)) {
        //act("@w$n@w looks around the room.@n", TRUE, ch, 0, 0, TO_ROOM);
       }
      }
    } else if (is_abbrev(arg, "inside")   && char_get_exit(ch, INDIR) && !*arg2) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, INDIR);
      else
        look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
      search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") ||
               is_abbrev(arg, "into") || is_abbrev(arg, "onto"))  { 
      look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") || 
                is_abbrev(arg, "through") ||
	        is_abbrev(arg, "thru")      ) && 
               (subcmd == SCMD_LOOK) && *arg2) {
      look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") && 
               (subcmd == SCMD_LOOK) && !char_get_exit(ch, OUTDIR)) {
      look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, FALSE)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, FALSE)) >= 0) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, FALSE)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, FALSE)) >= 0 )) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
      look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
       look_at_target(ch, arg2, 0);
    } else if (is_abbrev(arg, "around")) { 
      struct extra_descr_data *i; 
      int found = 0; 
 
      for (i = world[char_get_in_room(ch)].ex_description; i; i = i->next) { 
        if (*i->keyword != '.') { 
          send_to_char(ch, "%s%s:\r\n%s",                  
                (found ? "\r\n" : ""), i->keyword, i->description);                
          found = 1; 
        } 
      } 
      if (!found) 
        send_to_char(ch, "You couldn't find anything noticeable.\r\n");
    } else if (find_exdesc(arg, world[char_get_in_room(ch)].ex_description) != NULL) {
      look_at_target(ch, arg, 0);
    } else {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
    else
      look_at_target(ch, arg, 0);
  }
  }
}

ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg),0);	/* strcpy: OK */

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((obj_get_type_flag(tmp_object) == ITEM_DRINKCON) ||
	(obj_get_type_flag(tmp_object) == ITEM_FOUNTAIN) ||
	(obj_get_type_flag(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}

ACMD(do_gold)
{
  if (char_get_gold(ch) == 0)
    send_to_char(ch, "You're broke!\r\n");
  else if (char_get_gold(ch) == 1)
    send_to_char(ch, "You have one little zenni.\r\n");
  else
    send_to_char(ch, "You have %d zenni.\r\n", char_get_gold(ch));
}

ACMD(do_score)
{

  if (char_is_npc(ch))
    return;

 int view = 0, full = 5, personal = 1, health = 2, stats = 3, other = 4;
 
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (!*arg) {
  view = full;
 } else if (strstr("personal", arg) || strstr("Personal", arg)) {
  view = personal;
 } else if (strstr("health", arg) || strstr("Health", arg)) {
  view = health;
 } else if (strstr("statistics", arg) || strstr("Statistics", arg)) {
  view = stats;
 } else if (strstr("other", arg) || strstr("Other", arg)) {
  view = other;
 } else {
  send_to_char(ch, "Syntax: score, or... score (personal, health, statistics, other)\r\n");
  return;
 }

 if (view == full || view == personal) {
 send_to_char(ch, "  @cO@D-----------------------------[  @cPersonal  @D]-----------------------------@cO@n\n");
 send_to_char(ch, "  @D|  @CName@D: @W%15s@D,   @CTitle@D: @W%-38s@D|@n\n", char_get_name(ch), char_get_title(ch));
  if (char_is_android(ch)) {
   char model[100], version[100];
   int absorb = 0;
    if (char_plr_flagged(ch, PLR_ABSORB)) {
     sprintf(model, "@CAbsorption");
    } else if (char_plr_flagged(ch, PLR_REPAIR)) {
     sprintf(model, "@GSelf Repairing");
    } else if (char_plr_flagged(ch, PLR_SENSEM)) {
     sprintf(model, "@RSensor Equiped");
    }
   if (char_plr_flagged(ch, PLR_TRANS1)) {
     sprintf(version, "Beta 1.0");
   } else if (char_plr_flagged(ch, PLR_TRANS2)) {
     sprintf(version, "ANS 2.0");
   } else if (char_plr_flagged(ch, PLR_TRANS3)) {
     sprintf(version, "ANS 3.0");
   } else if (char_plr_flagged(ch, PLR_TRANS4)) {
     sprintf(version, "ANS 4.0");
   } else if (char_plr_flagged(ch, PLR_TRANS5)) {
     sprintf(version, "ANS 5.0");
   } else if (char_plr_flagged(ch, PLR_TRANS6)) {
     sprintf(version, "ANS 6.0");
   } else {
     sprintf(version, "Alpha 0.5");
   }
   send_to_char(ch, "  @D| @CModel@D: %15s@D,    @CUGP@D: @G%15s@D,  @CVersion@D: @r%-12s@D|@n\n", model, absorb > 0 ? "@RN/A" : add_commas(char_get_upgrade(ch)), version);
  }
 send_to_char(ch, "  @D|  @CRace@D: @W%10s@D,  @CSensei@D: @W%15s@D,     @CArt@D: @W%-17s@D|@n\n", TRUE_RACE(ch), SENSEI_NAME(ch), SENSEI_STYLE(ch));
  char hei[300], wei[300];
  sprintf(hei, "%dcm", get_measure(ch, char_get_pc_height(ch), 0));
  sprintf(wei, "%dkg", get_measure(ch, 0, char_get_pc_weight(ch)));
 send_to_char(ch, "  @D|   @CAge@D: @W%10s@D,  @CHeight@D: @W%15s@D,  @CWeight@D: @W%-17s@D|@n\n", add_commas(GET_AGE(ch)), hei, wei);
 send_to_char(ch, "  @D|@CGender@D: @W%10s@D,  @C  Size@D: @W%15s@D,  @C Align@D: @W%-17s@D|@n\n", genders[(int)char_get_sex(ch)], size_names[get_size(ch)], disp_align(ch));
 }
 if (view == full || view == health) {
 send_to_char(ch, "  @cO@D-----------------------------@D[   @cHealth   @D]-----------------------------@cO@n\n");
 send_to_char(ch, "                 @D<@rPowerlevel@D>          <@BKi@D>             <@GStamina@D>@n\n");
 send_to_char(ch, "    @wCurrent   @D-[@R%-16s@D]-[@R%-16s@D]-[@R%-16s@D]@n\n", add_commas(getCurPL(ch)), add_commas(
         (getCurKI(ch))), add_commas((getCurST(ch))));
 send_to_char(ch, "    @wMaximum   @D-[@r%-16s@D]-[@r%-16s@D]-[@r%-16s@D]@n\n", add_commas(getEffMaxPL(ch)), add_commas(GET_MAX_MANA(ch)), add_commas(GET_MAX_MOVE(ch)));
 send_to_char(ch, "    @wBase      @D-[@m%-16s@D]-[@m%-16s@D]-[@m%-16s@D]@n\n", add_commas((getEffBasePL(ch))), add_commas(
         (getEffBaseKI(ch))), add_commas((getEffBaseST(ch))));
  if (!char_is_android(ch) && (getCurLF(ch)) > 0) {
   send_to_char(ch, "    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(
           (getCurLF(ch))), "/", add_commas((getMaxLF(ch))), char_get_lifeperc(ch), "%");
  } else if (!char_is_android(ch)) {
   send_to_char(ch, "    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(0), "/", add_commas(
           (getMaxLF(ch))), char_get_lifeperc(ch), "%");
  }
 }
 if (view == full || view == stats) {
 send_to_char(ch, "  @cO@D-----------------------------@D[ @cStatistics @D]-----------------------------@cO@n\n");
 send_to_char(ch, "      @D<@wCharacter Level@D: @w%-3d@D> <@wRPP@D: @w%-3d@D> <@wRPP Bank@D: @w%-3d@D>@n\n", char_get_level(ch), char_get_rp(ch), char_get_rbank(ch));
 send_to_char(ch, "      @D<@wSpeed Index@D: @w%-15s@D> <@wArmor Index@D: @w%-15s@D>@n\n", add_commas(char_get_speedi(ch)), add_commas(char_get_armor(ch)));
 send_to_char(ch, "    @D[    @RStrength@D|@G%2d (%3d)@D] [     @YAgility@D|@G%2d (%3d)@D] [      @BSpeed@D|@G%2d (%3d)@D]@n\n", ch->real_abils.str, char_get_aff_abils_str(ch), ch->real_abils.dex, char_get_aff_abils_dex(ch), ch->real_abils.cha, char_get_aff_abils_cha(ch));
 send_to_char(ch, "    @D[@gConstitution@D|@G%2d (%3d)@D] [@CIntelligence@D|@G%2d (%3d)@D] [     @MWisdom@D|@G%2d (%3d)@D]@n\n", ch->real_abils.con, char_get_aff_abils_con(ch), ch->real_abils.intel, char_get_aff_abils_intel(ch), ch->real_abils.wis, char_get_aff_abils_wis(ch));
 }
 if (view == full || view == other) {
 send_to_char(ch, "  @cO@D-----------------------------@D[   @cOther    @D]-----------------------------@cO@n\n");
 send_to_char(ch, "                @D<@YZenni@D>                 <@rInventory Weight@D>@n\n");
 send_to_char(ch, "      @D[   @CCarried@D| @W%-15s@D] [   @CCarried@D| @W%-15s@D]@n\n", add_commas(char_get_gold(ch)), add_commas(
         (getCurCarriedWeight(ch))));
 send_to_char(ch, "      @D[      @CBank@D| @W%-15s@D] [ @CMax Carry@D| @W%-15s@D]@n\n", add_commas(char_get_bank_gold(ch)), add_commas(CAN_CARRY_W(ch)));
 send_to_char(ch, "      @D[ @CMax Carry@D| @W%-15s@D]@n\n", add_commas(char_gold_carry(ch)));
  int numb = char_get_bank_interest(ch);
 send_to_char(ch, "      @D[  @CInterest@D| @W%-15s@D]\n", add_commas(numb));
  if (char_is_arlian(ch)) {
   send_to_char(ch, "                             @D<@GEvolution @D>@n\n");
   send_to_char(ch, "      @D[ @CEvo Level@D| @W%-15d@D] [   @CEvo Exp@D| @W%-15s@D]\n",char_get_moltlevel(ch), add_commas(char_get_moltexp(ch)));
   send_to_char(ch, "      @D[ @CThreshold@D| @W%-15s@D]@n\n", add_commas(molt_threshold(ch)));
  }
  if (char_get_level(ch) < 100) {
   send_to_char(ch, "                             @D<@gAdvancement@D>@n\n");
  }
  if (char_get_level(ch) < 100) {
   send_to_char(ch, "      @D[@CExperience@D| @W%-15s@D] [@CNext Level@D| @W%-15s@D]@n\n", add_commas(char_get_exp(ch)), add_commas(level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch)));
   send_to_char(ch, "      @D[  @CRpp Cost@D| @W%-15d@D]@n\n", rpp_to_level(ch));
  }

 send_to_char(ch, "\n     @D<@wPlayed@D: @yYears @D(@W%2d@D) @yWeeks @D(@W%2d@D) @yDays @D(@W%2d@D) @yHours @D(@W%2d@D) @yMinutes @D(@W%2d@D)>@n\n", (int)ch->time.played / 31536000, (int)((ch->time.played % 31536000) / 604800), (int)((ch->time.played % 604800) / 86400), (int)((ch->time.played % 86400) / 3600), (int)((ch->time.played % 3600) / 60));
 }
 send_to_char(ch, "  @cO@D------------------------------------------------------------------------@cO@n\n");

}

static void trans_check(struct char_data *ch, struct char_data *vict)
{
/* Rillao: transloc, add new transes here */
 if (char_is_human(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Human First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Human Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Human Third@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Human Fourth@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_hoshijin(vict)) {
  if (char_get_mimic(vict) == 0 || vict == ch) {
   if (char_get_starphase(vict) == 1) {
    send_to_char(ch, "         @cCurrent Transformation@D: @CBirth Phase@n\r\n");
   }
   else if (char_get_starphase(vict) == 2) {
    send_to_char(ch, "         @cCurrent Transformation@D: @CLife Phase@n\r\n");
   } else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
   }
  } else {
    send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if ((char_is_saiyan(vict) || char_is_halfbreed(vict)) && !char_plr_flagged(vict, PLR_LSSJ)) {
  if (char_plr_flagged(vict, PLR_TRANS1) && !char_plr_flagged(vict, PLR_FPSSJ)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Saiyan First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS1) && char_plr_flagged(vict, PLR_FPSSJ)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @YFull Powered @CSuper Saiyan@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Saiyan Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Saiyan Third@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Saiyan Fourth@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_OOZARU)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @COozaru@n\r\n");
   }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_saiyan(vict) && char_plr_flagged(vict, PLR_LSSJ)) {
     if (char_plr_flagged(vict, PLR_TRANS1) && !char_plr_flagged(vict, PLR_FPSSJ)) {
         send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Saiyan First@n\r\n");
     }
     else if (char_plr_flagged(vict, PLR_TRANS1) && char_plr_flagged(vict, PLR_FPSSJ)) {
         send_to_char(ch, "         @cCurrent Transformation@D: @YFull Powered @CSuper Saiyan@n\r\n");
     }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @YLegendary @CSuper Saiyan@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_OOZARU)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @COozaru@n\r\n");
   }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_namek(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Namek First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Namek Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Namek Third@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Namek Fourth@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_icer(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CTransform First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CTransform Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CTransform Third@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CTransform Fourth@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_konatsu(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CShadow First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CShadow Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CShadow Third@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CShadow Fourth@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_mutant(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMutate First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMutate Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMutate Third@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_bio(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMature@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSemi-perfect@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CPerfect@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper Perfect@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_android(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSeries 1.0@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSeries 2.0@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSeries 3.0@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS4)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSeries 4.0@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS5)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSeries 5.0@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS6)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSeries 6.0@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_majin(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CAffinity@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CSuper@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CTrue@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_truffle(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CAscend First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CAscend Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CAscend Third@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else if (char_is_kai(vict)) {
  if (char_plr_flagged(vict, PLR_TRANS1)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMystic First@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS2)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMystic Second@n\r\n");
  }
  else if (char_plr_flagged(vict, PLR_TRANS3)) {
   send_to_char(ch, "         @cCurrent Transformation@D: @CMystic Third@n\r\n");
  }
  else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }
 }
 else {
   send_to_char(ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
 }

} // End trans check

ACMD(do_status)
{
    char arg[MAX_INPUT_LENGTH];
    struct affected_type *aff;

    const char *forget_level[7] = {
            "@GRemembered Well@n",
            "@GRemembered Well Enough@n",
            "@RGetting Foggy@n",
            "@RHalf Forgotten@n",
            "@rAlmost Forgotten@n",
            "@rForgotten@n",
            "\n"
    };

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "@D<@b------------------------@D[@YYour Status@D]@b-------------------------@D>@n\r\n\r\n");
        send_to_char(ch, "            @D---------------@CAppearance@D---------------\n");
        bringdesc(ch, ch);
        send_to_char(ch, "            @D---------------@RAppendages@D---------------\n");

        if (char_plr_flagged(ch, PLR_HEAD)) {
            send_to_char(ch, "            @D[@cHead        @D: @GHave.          @D]@n\r\n");
        }
        if (!char_plr_flagged(ch, PLR_HEAD)) {
            send_to_char(ch, "            @D[@cHead        @D: @rMissing.         @D]@n\r\n");
        }
        if (char_get_limb_condition(ch, (1)-1) >= 50 && !char_plr_flagged(ch, PLR_CRARM)) {
            send_to_char(ch, "            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (1)-1) > 0 && !char_plr_flagged(ch, PLR_CRARM)) {
            send_to_char(ch, "            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (1)-1) > 0 && char_plr_flagged(ch, PLR_CRARM)) {
            send_to_char(ch, "            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (1)-1) <= 0) {
            send_to_char(ch, "            @D[@cRight Arm   @D: @rMissing.         @D]@n\r\n");
        }
        if (char_get_limb_condition(ch, (2)-1) >= 50 && !char_plr_flagged(ch, PLR_CLARM)) {
            send_to_char(ch, "            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (2)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (2)-1) > 0 && !char_plr_flagged(ch, PLR_CLARM)) {
            send_to_char(ch, "            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (2)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (2)-1) > 0 && char_plr_flagged(ch, PLR_CLARM)) {
            send_to_char(ch, "            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (2)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (2)-1) <= 0) {
            send_to_char(ch, "            @D[@cLeft Arm    @D: @rMissing.         @D]@n\r\n");
        }
        if (char_get_limb_condition(ch, (3)-1) >= 50 && !char_plr_flagged(ch, PLR_CLARM)) {
            send_to_char(ch, "            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (3)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (3)-1) > 0 && !char_plr_flagged(ch, PLR_CRLEG)) {
            send_to_char(ch, "            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (3)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (3)-1) > 0 && char_plr_flagged(ch, PLR_CRLEG)) {
            send_to_char(ch, "            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (3)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (3)-1) <= 0) {
            send_to_char(ch, "            @D[@cRight Leg   @D: @rMissing.         @D]@n\r\n");
        }
        if (char_get_limb_condition(ch, (4)-1) >= 50 && !char_plr_flagged(ch, PLR_CLLEG)) {
            send_to_char(ch, "            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (4)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (4)-1) > 0 && !char_plr_flagged(ch, PLR_CLLEG)) {
            send_to_char(ch, "            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (4)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (4)-1) > 0 && char_plr_flagged(ch, PLR_CLLEG)) {
            send_to_char(ch, "            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (4)-1), "%", "%");
        }
        else if (char_get_limb_condition(ch, (4)-1) <= 0) {
            send_to_char(ch, "            @D[@cLeft Leg    @D: @rMissing.         @D]@n\r\n");
        }

        if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && char_plr_flagged(ch, PLR_STAIL)) {
            send_to_char(ch, "            @D[@cTail        @D: @GHave.            @D]@n\r\n");
        }
        if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && !char_plr_flagged(ch, PLR_STAIL)) {
            send_to_char(ch, "            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
        }
        if ((char_is_icer(ch) || char_is_bio(ch)) && char_plr_flagged(ch, PLR_TAIL)) {
            send_to_char(ch, "            @D[@cTail        @D: @GHave.            @D]@n\r\n");
        }
        if ((char_is_icer(ch) || char_is_bio(ch)) && !char_plr_flagged(ch, PLR_TAIL)) {
            send_to_char(ch, "            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
        }
        send_to_char(ch, "\r\n");

        send_to_char(ch, "         @D-----------------@YHunger@D/@yThirst@D-----------------@n\r\n");
        if (GET_COND(ch, HUNGER) >= 48) {
            send_to_char(ch, "         You are full.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 40) {
            send_to_char(ch, "         You are nearly full.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 30) {
            send_to_char(ch, "         You are not hungry.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 21) {
            send_to_char(ch, "         You wouldn't mind a snack.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 15) {
            send_to_char(ch, "         You are slightly hungry.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 10) {
            send_to_char(ch, "         You are partially hungry.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 5) {
            send_to_char(ch, "         You are really hungry.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 2) {
            send_to_char(ch, "         You are extremely hungry.\r\n");
        }
        else if (GET_COND(ch, HUNGER) >= 0) {
            send_to_char(ch, "         You are starving!\r\n");
        }
        else if (GET_COND(ch, HUNGER) < 0) {
            send_to_char(ch, "         You need not eat.\r\n");
        }
        if (GET_COND(ch, THIRST) >= 48) {
            send_to_char(ch, "         You are not thirsty.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 40) {
            send_to_char(ch, "         You are nearly quenched.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 30) {
            send_to_char(ch, "         You are not thirsty.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 21) {
            send_to_char(ch, "         You wouldn't mind a drink.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 15) {
            send_to_char(ch, "         You are slightly thirsty.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 10) {
            send_to_char(ch, "         You are partially thirsty.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 5) {
            send_to_char(ch, "         You are really thirsty.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 2) {
            send_to_char(ch, "         You are extremely thirsty.\r\n");
        }
        else if (GET_COND(ch, THIRST) >= 0) {
            send_to_char(ch, "         You are dehydrated!\r\n");
        }
        else if (GET_COND(ch, THIRST) < 0) {
            send_to_char(ch, "         You need not drink.\r\n");
        }
        send_to_char(ch, "         @D--------------------@D[@GInfo@D]---------------------@n\r\n");
        trans_check(ch, ch);
        send_to_char(ch, "         You have died %d times.\r\n", char_get_dcount(ch));
        if (char_plr_flagged(ch, PLR_NOSHOUT)) {
            send_to_char(ch, "         You have been @rmuted@n on public channels.\r\n");
        }
        if (char_get_in_room(ch) == real_room(9)) {
            send_to_char(ch, "         You are in punishment hell, so sad....\r\n");
        }
        if (!char_prf_flagged(ch, PRF_HINTS)) {
            send_to_char(ch, "         You have hints turned off.\r\n");
        }
        if (NEWSUPDATE > char_get_lastpl(ch)) {
            send_to_char(ch, "         Check the 'news', it has been updated recently.\r\n");
        }
        if (has_mail(char_get_idnum(ch))) {
            send_to_char(ch, "         Check your mail at the nearest postmaster.\r\n");
        }
        if (char_prf_flagged(ch, PRF_HIDE)) {
            send_to_char(ch, "         You are hidden from who and ooc.\r\n");
        }
        if (char_get_voice(ch) != NULL) {
            send_to_char(ch, "         Your voice desc: '%s'\r\n", char_get_voice(ch));
        }
        if (char_get_distfea(ch) == DISTFEA_EYE) {
            send_to_char(ch, "         Your eyes are your most distinctive feature.\r\n");
        }
        if (char_get_preference(ch) == 0) {
            send_to_char(ch, "         You preferred a balanced form of fighting.\r\n");
        } else if (char_get_preference(ch) == PREFERENCE_KI) {
            send_to_char(ch, "         You preferred a ki dominate form of fighting.\r\n");
        } else if (char_get_preference(ch) == PREFERENCE_WEAPON) {
            send_to_char(ch, "         You preferred a weapon dominate form of fighting.\r\n");
        } else if (char_get_preference(ch) == PREFERENCE_H2H) {
            send_to_char(ch, "         You preferred a body dominate form of fighting.\r\n");
        } else if (char_get_preference(ch) == PREFERENCE_THROWING) {
            send_to_char(ch, "         You preferred a throwing dominate form of fighting.\r\n");
        }
        if (char_get_distfea(ch) == DISTFEA_HAIR && !char_is_demon(ch) && !char_is_majin(ch) && !char_is_icer(ch) && !char_is_namek(ch)) {
            send_to_char(ch, "         Your hair is your most distinctive feature.\r\n");
        } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_demon(ch)) {
            send_to_char(ch, "         Your horns are your most distinctive feature.\r\n");
        } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_majin(ch)) {
            send_to_char(ch, "         Your forelock is your most distinctive feature.\r\n");
        } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_icer(ch)) {
            send_to_char(ch, "         Your horns are your most distinctive feature.\r\n");
        } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_namek(ch)) {
            send_to_char(ch, "         Your antennae are your most distinctive feature.\r\n");
        }

        if (char_get_distfea(ch) == DISTFEA_SKIN) {
            send_to_char(ch, "         Your skin is your most distinctive feature.\r\n");
        }
        if (char_get_distfea(ch) == DISTFEA_HEIGHT) {
            send_to_char(ch, "         Your height is your most distinctive feature.\r\n");
        }
        if (char_get_distfea(ch) == DISTFEA_WEIGHT) {
            send_to_char(ch, "         Your weight is your most distinctive feature.\r\n");
        }
        if (char_get_equipment(ch, WEAR_EYE)) {
            struct obj_data *obj = char_get_equipment(ch, WEAR_EYE);
            if (obj_get_scoutfreq(obj) == 0) {
                obj_set_scoutfreq(obj, 1);
            }
            send_to_char(ch, "         Your scouter is on frequency @G%d@n\r\n", obj_get_scoutfreq(obj));
            obj = NULL;
        }
        if (char_get_charge(ch) > 0) {
            send_to_char(ch, "         You have @C%s@n ki charged.\r\n", add_commas(char_get_charge(ch)));
        }
        if (char_get_kaioken(ch) > 0) {
            send_to_char(ch, "         You are focusing Kaioken x %d.\r\n", char_get_kaioken(ch));
        }
        if (char_aff_flagged(ch, AFF_SANCTUARY)) {
            send_to_char(ch, "         You are surrounded by a barrier @D(@Y%s@D)@n\r\n", add_commas(char_get_barrier(ch)));
        }
        if (char_aff_flagged(ch, AFF_FIRESHIELD)) {
            send_to_char(ch, "         You are surrounded by flames!@n\r\n");
        }
        if (char_get_suppression(ch) > 0) {
            send_to_char(ch, "         You are suppressing current PL to %" I64T ".\r\n", char_get_suppression(ch));
        }
        if (char_is_majin(ch)) {
            send_to_char(ch, "         You have ingested %d people.\r\n", char_get_absorbs(ch));
        }
        if (char_is_bio(ch)) {
            send_to_char(ch, "         You have %d absorbs left.\r\n", char_get_absorbs(ch));
        }
        send_to_char(ch, "         You have %s colored aura.\r\n", aura_types[char_get_aura(ch)]);

        if (char_get_level(ch) < 100) {
            if ((char_is_android(ch) && char_plr_flagged(ch, PLR_ABSORB)) || (!char_is_android(ch) && !char_is_bio(ch) && !char_is_majin(ch))) {
                send_to_char(ch, "         @R%s@n to SC a stat this level.\r\n", add_commas(calc_soft_cap(ch)));
            } else {
                send_to_char(ch, "         @R%s@n in PL/KI/ST combined to SC this level.\r\n", add_commas(calc_soft_cap(ch)));
            }
        } else {
            send_to_char(ch, "         Your strengths are potentially limitless.\r\n");
        }

        if (char_get_forgeting(ch) != 0) {
            send_to_char(ch, "         @MForgetting @D[@m%s - %s@D]@n\r\n", spell_info[char_get_forgeting(ch)].name, forget_level[char_get_forgetcount(ch)]);
        } else {
            send_to_char(ch, "         @MForgetting @D[@mNothing.@D]@n\r\n");
        }

        if (char_get_skill(ch, SKILL_DAGGER) > 0) {
            if (char_get_backstabcool(ch) > 0) {
                send_to_char(ch, "         @yYou can't preform a backstab yet.@n\r\n");
            } else {
                send_to_char(ch, "         @YYou can backstab.@n\r\n");
            }
        }

        if (char_get_feature(ch)) {
            send_to_char(ch, "         Extra Feature: @C%s@n\r\n", char_get_feature(ch));
        }

        if (char_get_rdisplay(ch)) {
            if (char_get_rdisplay(ch) != "Empty") {
                send_to_char(ch, "         Room Display: @C...%s@n\r\n", char_get_rdisplay(ch));
            }
        }

        send_to_char(ch, "\r\n@D<@b-------------------------@D[@BCondition@D]@b--------------------------@D>@n\r\n");

        if (char_get_bonuses(ch, BONUS_INSOMNIAC)) {
            send_to_char(ch, "You can not sleep.\r\n");
        } else {
            if (char_get_sleeptime(ch) > 6 && char_get_position(ch) != POS_SLEEPING) {
                send_to_char(ch, "You are well rested.\r\n");
            }
            else if (char_get_sleeptime(ch) > 6 && char_get_position(ch) == POS_SLEEPING) {
                send_to_char(ch, "You are getting the rest you need.\r\n");
            }
            else if (char_get_sleeptime(ch) > 4) {
                send_to_char(ch, "You are rested.\r\n");
            }
            else if (char_get_sleeptime(ch) > 2) {
                send_to_char(ch, "You are not sleepy.\r\n");
            }
            else if (char_get_sleeptime(ch) >= 1) {
                send_to_char(ch, "You are getting a little sleepy.\r\n");
            }
            else if (char_get_sleeptime(ch) == 0) {
                send_to_char(ch, "You could sleep at any time.\r\n");
            }
        }


        if (char_get_relax_count(ch) > 464) {
            send_to_char(ch, "You are far too at ease to train hard like you should. Get out of the house more often.\r\n");
        } else if (char_get_relax_count(ch) > 232) {
            send_to_char(ch, "You are too at ease to train hard like you should. Get out of the house more often.\r\n");
        } else if (char_get_relax_count(ch) > 116) {
            send_to_char(ch, "You are a bit at ease and your training suffers. Get out of the house more often.\r\n");
        }

        if (char_get_mimic(ch) > 0) {
            send_to_char(ch, "You are mimicing the general appearance of %s %s\r\n", AN(LRACE(ch)), LRACE(ch));
        }
        if (char_is_mutant(ch)) {
            send_to_char(ch, "Your Mutations:\r\n");
            if (char_get_genome(ch, 0) == 1) {
                send_to_char(ch, "  Extreme Speed.\r\n");
            }
            if (char_get_genome(ch, 0) == 2) {
                send_to_char(ch, "  Increased Cell Regeneration.\r\n");
            }
            if (char_get_genome(ch, 0) == 3) {
                send_to_char(ch, "  Extreme Reflexes.\r\n");
            }
            if (char_get_genome(ch, 0) == 4) {
                send_to_char(ch, "  Infravision.\r\n");
            }
            if (char_get_genome(ch, 0) == 5) {
                send_to_char(ch, "  Natural Camo.\r\n");
            }
            if (char_get_genome(ch, 0) == 6) {
                send_to_char(ch, "  Limb Regen.\r\n");
            }
            if (char_get_genome(ch, 0) == 7) {
                send_to_char(ch, "  Poisonous (you can use the bite attack).\r\n");
            }
            if (char_get_genome(ch, 0) == 8) {
                send_to_char(ch, "  Rubbery Body.\r\n");
            }
            if (char_get_genome(ch, 0) == 9) {
                send_to_char(ch, "  Innate Telepathy.\r\n");
            }
            if (char_get_genome(ch, 0) == 10) {
                send_to_char(ch, "  Natural Energy.\r\n");
            }
            if (char_get_genome(ch, 1) == 1) {
                send_to_char(ch, "  Extreme Speed.\r\n");
            }
            if (char_get_genome(ch, 1) == 2) {
                send_to_char(ch, "  Increased Cell Regeneration.\r\n");
            }
            if (char_get_genome(ch, 1) == 3) {
                send_to_char(ch, "  Extreme Reflexes.\r\n");
            }
            if (char_get_genome(ch, 1) == 4) {
                send_to_char(ch, "  Infravision.\r\n");
            }
            if (char_get_genome(ch, 1) == 5) {
                send_to_char(ch, "  Natural Camo.\r\n");
            }
            if (char_get_genome(ch, 1) == 6) {
                send_to_char(ch, "  Limb Regen.\r\n");
            }
            if (char_get_genome(ch, 1) == 7) {
                send_to_char(ch, "  Poisonous (you can use the bite attack).\r\n");
            }
            if (char_get_genome(ch, 1) == 8) {
                send_to_char(ch, "  Rubbery Body.\r\n");
            }
            if (char_get_genome(ch, 1) == 9) {
                send_to_char(ch, "  Innate Telepathy.\r\n");
            }
            if (char_get_genome(ch, 1) == 10) {
                send_to_char(ch, "  Natural Energy.\r\n");
            }
        }
        if (char_is_bio(ch)) {
            send_to_char(ch, "Your genes carry:\r\n");
            if (char_get_genome(ch, 0) == 1) {
                send_to_char(ch, "  Human DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 2) {
                send_to_char(ch, "  Saiyan DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 3) {
                send_to_char(ch, "  Namek DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 4) {
                send_to_char(ch, "  Icer DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 5) {
                send_to_char(ch, "  Truffle DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 6) {
                send_to_char(ch, "  Arlian DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 7) {
                send_to_char(ch, "  Kai DNA.\r\n");
            }
            if (char_get_genome(ch, 0) == 8) {
                send_to_char(ch, "  Konatsu DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 1) {
                send_to_char(ch, "  Human DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 2) {
                send_to_char(ch, "  Saiyan DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 3) {
                send_to_char(ch, "  Namek DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 4) {
                send_to_char(ch, "  Icer DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 5) {
                send_to_char(ch, "  Truffle DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 6) {
                send_to_char(ch, "  Arlian DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 7) {
                send_to_char(ch, "  Kai DNA.\r\n");
            }
            if (char_get_genome(ch, 1) == 8) {
                send_to_char(ch, "  Konatsu DNA.\r\n");
            }
        }
        if (char_get_genome(ch, 0) == 11) {
            send_to_char(ch, "You have used kyodaika.\r\n");
        }
        if (char_prf_flagged(ch, PRF_NOPARRY)) {
            send_to_char(ch, "You have decided not to parry attacks.\r\n");
        }
        switch (char_get_position(ch)) {
            case POS_DEAD:
                send_to_char(ch, "You are DEAD!\r\n");
                break;
            case POS_MORTALLYW:
                send_to_char(ch, "You are mortally wounded! You should seek help!\r\n");
                break;
            case POS_INCAP:
                send_to_char(ch, "You are incapacitated, slowly fading away...\r\n");
                break;
            case POS_STUNNED:
                send_to_char(ch, "You are stunned! You can't move!\r\n");
                break;
            case POS_SLEEPING:
                send_to_char(ch, "You are sleeping.\r\n");
                break;
            case POS_RESTING:
                send_to_char(ch, "You are resting.\r\n");
                break;
            case POS_SITTING:
                send_to_char(ch, "You are sitting.\r\n");
                break;
            case POS_FIGHTING:
                send_to_char(ch, "You are fighting %s.\r\n", char_get_fighting(ch) ? char_pers(char_get_fighting(ch), ch) : "thin air");
                break;
            case POS_STANDING:
                send_to_char(ch, "You are standing.\r\n");
                break;
            default:
                send_to_char(ch, "You are floating.\r\n");
                break;
        }

        if (has_group(ch)) {
            send_to_char(ch, "@GGroup Victories@D: @w%s@n\r\n", add_commas(char_get_combatexpertise(ch)));
        }

        if (char_plr_flagged(ch, PLR_EYEC)) {
            send_to_char(ch, "Your eyes are closed.\r\n");
        }
        if (char_aff_flagged(ch, AFF_SNEAK)) {
            send_to_char(ch, "You are prepared to sneak where ever you go.\r\n");
        }
        if (char_plr_flagged(ch, PLR_DISGUISED)) {
            send_to_char(ch, "You have disguised your facial features.\r\n");
        }
        if (char_aff_flagged(ch, AFF_FLYING)) {
            send_to_char(ch, "You are flying.\r\n");
        }
        if (char_plr_flagged(ch, PLR_PILOTING)) {
            send_to_char(ch, "You are busy piloting a ship.\r\n");
        }
        if (char_get_powerattack(ch) > 0) {
            send_to_char(ch, "You are playing @y'@Y%s@y'@n.\r\n", song_types[char_get_powerattack(ch)]);
        }

        if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
            send_to_char(ch, "You are prepared to zanzoken.\r\n");
        }
        if (char_aff_flagged(ch, AFF_HASS)) {
            send_to_char(ch, "Your arms are moving fast.\r\n");
        }
        if (char_aff_flagged(ch, AFF_INFUSE)) {
            send_to_char(ch, "Your ki will be infused in your next physical attack.\r\n");
        }
        if (char_plr_flagged(ch, PLR_TAILHIDE)) {
            send_to_char(ch, "Your tail is hidden!\r\n");
        }
        if (char_plr_flagged(ch, PLR_NOGROW)) {
            send_to_char(ch, "Your tail is no longer regrowing!\r\n");
        }
        if (is_affected(ch, AFF_SPECIAL_POSE)) {
            send_to_char(ch, "You are feeling confident from your pose earlier.\r\n");
        }
        if (is_affected(ch, AFF_HYDROZAP)) {
            send_to_char(ch, "You are effected by Kanso Suru.\r\n");
        }
        if (GET_COND(ch, DRUNK) > 15)
            send_to_char(ch, "You are extremely drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 10)
            send_to_char(ch, "You are pretty drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 4)
            send_to_char(ch, "You are drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 0)
            send_to_char(ch, "You have an alcoholic buzz.\r\n");

        if (ch->affected) {
            int lasttype = 0;
            for (aff = ch->affected; aff; aff = aff->next) {
                if (!strcasecmp(skill_name(aff->type), "runic") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Kenaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "punch") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Algiz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "knee") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Oagaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "slam") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Wunjo rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "heeldrop") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Purisaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "special beam cannon") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Laguz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "might") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your muscles are pumped! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "flex") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You are more agile right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "bless") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have been blessed! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "curse") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have been cursed! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "healing glow") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have a healing glow enveloping your body! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "genius") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You are smarter right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "enlighten") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You are wiser right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "yoikominminken") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have been lulled to sleep! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "solar flare") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have been blinded! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "spirit control") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have full control of your spirit! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "!UNUSED!") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You feel poison burning through your blood! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "tough skin") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have toughened skin right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "poison") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "You have been poisoned! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "warp pool") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Weakened State! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "dark metamorphosis") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your Dark Metamorphosis is still in effect. (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "hayasa") && aff->type != lasttype) {
                    lasttype = aff->type;
                    send_to_char(ch, "Your body has been infused to move faster! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
            }
        }

        if (char_aff_flagged(ch, AFF_KNOCKED))
            send_to_char(ch, "You have been knocked unconcious!\r\n");

        if (char_aff_flagged(ch, AFF_INVISIBLE))
            send_to_char(ch, "You are invisible.\r\n");

        if (char_aff_flagged(ch, AFF_DETECT_INVIS))
            send_to_char(ch, "You are sensitive to the presence of invisible things.\r\n");

        if (char_aff_flagged(ch, AFF_MBREAK))
            send_to_char(ch, "Your mind has been broken!\r\n");

        if (char_aff_flagged(ch, AFF_WITHER))
            send_to_char(ch, "You've been withered! You feel so weak...\r\n");

        if (char_aff_flagged(ch, AFF_SHOCKED))
            send_to_char(ch, "Your mind has been shocked!\r\n");

        if (char_aff_flagged(ch, AFF_CHARM))
            send_to_char(ch, "You have been charmed!\r\n");

        if (affected_by_spell(ch, SPELL_MAGE_ARMOR))
            send_to_char(ch, "You feel protected.\r\n");

        if (char_aff_flagged(ch, AFF_INFRAVISION))
            send_to_char(ch, "You can see in darkness with infravision.\r\n");

        if (char_prf_flagged(ch, PRF_SUMMONABLE))
            send_to_char(ch, "You are summonable by other players.\r\n");

        if (char_aff_flagged(ch, AFF_DETECT_ALIGN))
            send_to_char(ch, "You see into the hearts of others.\r\n");

        if (char_aff_flagged(ch, AFF_DETECT_MAGIC))
            send_to_char(ch, "You are sensitive to the magical nature of things.\r\n");

        if (char_aff_flagged(ch, AFF_SPIRIT))
            send_to_char(ch, "You have died and are part of the SPIRIT world!\r\n");

        if (char_prf_flagged(ch, PRF_NOGIVE))
            send_to_char(ch, "You are not accepting items being handed to you right now.\r\n");

        if (char_aff_flagged(ch, AFF_ETHEREAL))
            send_to_char(ch, "You are ethereal and cannot interact with normal space!\r\n");

        if (char_get_regen(ch) > 0) {
            send_to_char(ch, "Something is augmenting your regen rate by %s%d%s!\r\n", char_get_regen(ch) > 0 ? "+" : "-", char_get_regen(ch), "%");
        }

        if (char_get_asb(ch) > 0) {
            send_to_char(ch, "Something is augmenting your auto-skill training rate by %s%d%s!\r\n", char_get_asb(ch) > 0 ? "+" : "-", char_get_asb(ch), "%");
        }

        if (ch->lifebonus > 0) {
            send_to_char(ch, "Something is augmenting your Life Force Max by %s%d%s!\r\n", ch->lifebonus > 0 ? "+" : "-", ch->lifebonus, "%");
        }

        if (char_plr_flagged(ch, PLR_FISHING))
            send_to_char(ch, "Current Fishing Pole Bonus @D[@C%d@D]@n\r\n", char_get_accuracy(ch));

        if (char_plr_flagged(ch, PLR_AURALIGHT))
            send_to_char(ch, "Aura Light is active.\r\n");
        send_to_char(ch, "@D<@b--------------------------------------------------------------@D>@n\r\n");
        send_to_char(ch, "To view your bonus/negative traits enter: status traits\r\n");
    } else if (!strcasecmp(arg, "traits")) {
        bonus_status(ch);
    } else {
        send_to_char(ch, "The only argument status takes is 'traits'. If you just want your status do not use an argument.\r\n");
    }
}
const char *list_bonuses[] = {
                   "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost          ", /* Bonus 0 */
                   "Prodigy     - +25% Experience Gained Until Level 80               ", /* Bonus 1 */
                   "Quick Study - Character auto-trains skills faster                 ", /* Bonus 2 */
                   "Die Hard    - Life Force's PL regen doubled, but cost is the same ", /* Bonus 3 */
                   "Brawler     - Physical attacks do 20% more damage                 ", /* Bonus 4 */
                   "Destroyer   - Damaged Rooms act as regen rooms for you            ", /* Bonus 5 */
                   "Hard Worker - Physical activity bonuses + drains less stamina     ", /* Bonus 6 */
                   "Healer      - Heal/First-aid/Vigor/Repair restore +10%            ", /* Bonus 7 */
                   "Loyal       - +20% Experience When Grouped As Follower            ", /* Bonus 8 */
                   "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%  ", /* Bonus 9 */
                   "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75%", /* Bonus 10 */
                   "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%    ", /* Bonus 11 */
                   "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%   ", /* Bonus 12 */
                   "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%     ", /* Bonus 13 */
                   "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%    ", /* Bonus 14 */
                   "Thick Skin  - -20% Physical and -10% ki dmg received              ", /* Bonus 15 */
                   "Recipe Int. - Food cooked by you lasts longer/heals better        ", /* Bonus 16 */
                   "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn      ", /* Bonus 17 */
                   "Powerhitter - 15% critical hits will be x4 instead of x2          ", /* Bonus 18 */
                   "Healthy     - 40% chance to recover from ill effects when sleeping", /* Bonus  19 */
                   "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis ", /* Bonus  20 */
                   "Evasive     - +15% to dodge rolls                                 ", /* Bonus  21 */
                   "The Wall    - +20% chance to block                                ", /* Bonus  22 */
                   "Accurate    - +20% chance to hit physical, +10% to hit with ki     ", /* Bonus  23 */
                   "Energy Leech- -2% ki damage received for every 5 character levels,\n                  @cas long as you can take that ki to your charge pool.@D        ", /* Bonus  24*/
                   "Good Memory - +2 Skill Slots initially, +1 every 20 levels after  ", /* Bonus 25 */
                   "Soft Touch  - Half damage for all hit locations                   ", /* Neg 26 */
                   "Late Sleeper- Can only wake automatically. 33% every hour if maxed", /* Neg 27 */
                   "Impulse Shop- +25% shop costs                                     ", /* Neg 28 */
                   "Sickly      - Suffer from harmful effects longer                  ", /* Neg 29 */
                   "Punching Bag- -15% to dodge rolls                                 ", /* Neg 30 */
                   "Pushover    - -20% block chance                                   ", /* Neg 31 */
                   "Poor D. Perc- -20% chance to hit with physical, -10% with ki       ", /* Neg 32 */
                   "Thin Skin   - +20% physical and +10% ki damage received           ", /* Neg 33 */
                   "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned         ", /* Neg 34 */
                   "Energy Int. - +2% ki damage received for every 5 character levels,\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
                   "Coward      - Can't Attack Enemy With 150% Your Powerlevel        ", /* Neg 36 */
                   "Arrogant    - Cannot Suppress                                     ", /* Neg 37 */
                   "Unfocused   - Charge concentration randomly breaks                ", /* Neg 38 */
                   "Slacker     - Physical activity drains more stamina               ", /* Neg 39 */
                   "Slow Learner- Character auto-trains skills slower                 ", /* Neg 40 */
                   "Masochistic - Defense Skills Cap At 75                            ", /* Neg 41 */
                   "Mute        - Can't use IC speech related commands                ", /* Neg 42 */
                   "Wimp        - Strength is capped at 45                            ", /* Neg 43 */
                   "Dull        - Intelligence is capped at 45                        ", /* Neg 44 */
                   "Foolish     - Wisdom is capped at 45                              ", /* Neg 45 */
                   "Clumsy      - Agility is capped at 45                             ", /* Neg 46 */
                   "Slow        - Speed is capped at 45                               ", /* Neg 47 */
                   "Frail       - Constitution capped at 45                           ", /* Neg 48 */
                   "Sadistic    - Half Experience Gained For Quick Kills              ", /* Neg 49 */
                   "Loner       - Can't Group with anyone, +5% train and +10% Phys    ", /* Neg 50 */
                   "Bad Memory  - -5 Skill Slots                                      "  /* Neg 51 */
};

/* Display What Bonuses/Negatives Player Has */
static void bonus_status(struct char_data *ch)
{
 int i, max = 52, count = 0;

 if (char_is_npc(ch))
  return;
 
 send_to_char(ch, "@CYour Traits@n\n@D-----------------------------@w\n");
 for (i = 0; i < max; i++) {
  if (i < 26) {
   if (char_get_bonuses(ch, i)) {
	send_to_char(ch, "@c%s@n\n", list_bonuses[i]);
    count++;
   }
  } else {
   if (i == 26) {
    send_to_char(ch, "\r\n");
   }
   if (char_get_bonuses(ch, i)) {
    send_to_char(ch, "@r%s@n\n", list_bonuses[i]);
    count++;
   }
  }
 }
 if (count <= 0) {
  send_to_char(ch, "@wNone.\r\n");
 }
 send_to_char(ch, "@D-----------------------------@n\r\n");
 return;
}

ACMD(do_inventory)
{
  send_to_char(ch, "@w              @YInventory\r\n@D-------------------------------------@w\r\n");
  if (!char_is_npc(ch)) {
   if (char_plr_flagged(ch, PLR_STOLEN)) {
    char_plr_set(ch, PLR_STOLEN, false);
    send_to_char(ch, "@r   --------------------------------------------------@n\n");
    send_to_char(ch, "@R    You notice that you have been robbed sometime recently!\n");
    send_to_char(ch, "@r   --------------------------------------------------@n\n");
    return;
   }
  }
  list_obj_to_char(ch->carrying, ch, SHOW_OBJ_SHORT, TRUE);
  send_to_char(ch, "\n");
}

ACMD(do_equipment)
{
  int i;

  send_to_char(ch, "        @YEquipment Being Worn\r\n@D-------------------------------------@w\r\n");
  for (i = 1; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i)) {
      if (char_can_see_obj(ch, char_get_equipment(ch, i)) && (i != WEAR_WIELD1 && i != WEAR_WIELD2)) {
	send_to_char(ch, "%s", wear_where[i]);
	show_obj_to_char(char_get_equipment(ch, i), ch, SHOW_OBJ_SHORT);

       if (obj_extra_flagged(char_get_equipment(ch, i), ITEM_SHEATH)) {
         struct obj_data *obj2 = NULL, *next_obj = NULL, *sheath = char_get_equipment(ch, i);
         for (obj2 = sheath->contains; obj2; obj2 = next_obj) {
         next_obj = obj2->next_content;
         if (obj2) {
          send_to_char(ch, "@D  ---- @YSheathed@D ----@c> @n");
          show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
         }

        }
        obj2 = NULL;
       }
      } else if (char_can_see_obj(ch, char_get_equipment(ch, i)) && (!char_plr_flagged(ch, PLR_THANDW))) {
        send_to_char(ch, "%s", wear_where[i]);
        show_obj_to_char(char_get_equipment(ch, i), ch, SHOW_OBJ_SHORT);
        if (obj_extra_flagged(char_get_equipment(ch, i), ITEM_SHEATH)) {
         struct obj_data *obj2 = NULL, *next_obj = NULL, *sheath = char_get_equipment(ch, i);
         for (obj2 = sheath->contains; obj2; obj2 = next_obj) {
         next_obj = obj2->next_content;
         if (obj2) {
          send_to_char(ch, "@D  ---- @YSheathed@D ----> @n");
          show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
         }

        }
        obj2 = NULL;
       }
      } else if (char_can_see_obj(ch, char_get_equipment(ch, i)) && (char_plr_flagged(ch, PLR_THANDW))) {
        send_to_char(ch, "@c<@CWielded by B. Hands@c>@n ");
        show_obj_to_char(char_get_equipment(ch, i), ch, SHOW_OBJ_SHORT);
      } else {
	send_to_char(ch, "%s", wear_where[i]);
	send_to_char(ch, "Something.\r\n");
      }
    } else {
      if (char_body_flagged(ch, i) && (i != WEAR_WIELD2)) {
        send_to_char(ch, "%s@wNothing.@n\r\n", wear_where[i]);
      }
      else if (char_body_flagged(ch, i) && (i == WEAR_WIELD2 && !char_plr_flagged(ch, PLR_THANDW))) {
        send_to_char(ch, "%s@wNothing.@n\r\n", wear_where[i]);
      }
    }
  }
}

ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..30] */
  day = time_info.day + 1;

  /* 30 days in a month, 6 days a week */
  weekday = day % 6;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
	  (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
	  time_info.hours >= 12 ? "PM" : "AM", weekdays[weekday]);

  /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

  suf = "th";

  if (((day % 100) / 10) != 1) {
    switch (day % 10) {
    case 1:
      suf = "st";
      break;
    case 2:
      suf = "nd";
      break;
    case 3:
      suf = "rd";
      break;
    }
  }

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[time_info.month], time_info.year);
}

ACMD(do_weather)
{
  const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (char_is_outside(ch))
    {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due");
    if (char_adm_flagged(ch, ADM_KNOWWEATHER))
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                 weather_info.pressure,
                 weather_info.change,
                 weather_info.sky,
                 sky_look[weather_info.sky]);
    }
  else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}

/* puts -'s instead of spaces */
static void space_to_minus(char *str)
{
  while ((str = strchr(str, ' ')) != NULL)
    *str = '-';
}

int search_help(const char *argument, int level)
{
  int chk, bot, top, mid, minlen;

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  while (bot <= top) {
    mid = (bot + top) / 2;

    if (!(chk = strncasecmp(argument, help_table[mid].keywords, minlen)))  {
      while ((mid > 0) && !strncasecmp(argument, help_table[mid - 1].keywords, minlen))
        mid--;

      while (level < help_table[mid].min_level && mid < (bot + top) / 2)
        mid++;

      if (strncasecmp(argument, help_table[mid].keywords, minlen))
        break;

      return mid;
    }
    else if (chk > 0)
      bot = mid + 1;
    else
      top = mid - 1;
  }
  return NOWHERE;
}

ACMD(do_help)
{
  char buf[MAX_STRING_LENGTH * 4];
  int mid = 0;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  if (!*argument) {
    if (char_get_admlevel(ch) < ADMLVL_IMMORT)
      page_string(ch->desc, help, 0);
    else
      page_string(ch->desc, ihelp, 0);
    return;
  }

  space_to_minus(argument);

  if ((mid = search_help(argument, char_get_admlevel(ch))) == NOWHERE) {
    int i, found = 0;
    send_to_char(ch, "There is no help on that word.\r\n");
    if (char_get_admlevel(ch) < 3) {
    mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "%s tried to get help on %s", char_get_name(ch), argument);
    }
    for (i = 0; i <= top_of_helpt; i++)  {
      if (help_table[i].min_level > char_get_admlevel(ch))
        continue;
      /* To help narrow down results, if they don't start with the same letters, move on */
      if (*argument != *help_table[i].keywords)
        continue;
      if (levenshtein_distance(argument, help_table[i].keywords) <= 2) {
        if (!found) {
          send_to_char(ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(ch, "  %s\r\n", help_table[i].keywords);
      }
    }
    return;
  }
  if (help_table[mid].min_level > char_get_admlevel(ch)) {
   send_to_char(ch, "There is no help on that word.\r\n");
   return;
  }
  sprintf(buf, "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  sprintf(buf+strlen(buf), "%s", help_table[mid].entry);
  sprintf(buf+strlen(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  if (char_get_admlevel(ch) > 0) {
   sprintf(buf+strlen(buf), "@WHelp File Level@w: @D(@R%d@D)@n\n", help_table[mid].min_level);
  }
  page_string(ch->desc, buf, 0);
}

#define WHO_FORMAT \
"Usage: who [minlev[-maxlev]] [-k] [-n name] [-q] [-r] [-s] [-z]\r\n"

/* Written by Rhade */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  int i, num_can_see = 0;
  char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int low = 0, high = CONFIG_LEVEL_CAP, localwho = 0, questwho = 0, hide = 0;
  int showclass = 0, short_list = 0, outlaws = 0;
  int who_room = 0, showgroup = 0, showleader = 0;
  char *line_color ="@n";

  skip_spaces(&argument);
  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  struct {
    char *disp;
    int min_level;
    int max_level;
    int count; /* must always start as 0 */
  } rank[] = {
    { "\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs   @D]  @c------------@n\r\n", ADMLVL_IMMORT, ADMLVL_IMPL, 0},
    { "\r\n@D[@wx@D]@yxxxxxxxxxx@W  [    @GImmortals   @W]  @yxxxxxxxxxx@D[@wx@D]@n\r\n", ADMLVL_IMMORT + 8, ADMLVL_GRGOD + 8, 0},
    { "\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  @c------------@n\r\n", 0, ADMLVL_IMMORT - 1, 0}
    /*{ "\r\n@GAdministrators@n\r\n\r\n", ADMLVL_GRGOD, ADMLVL_IMPL, 0},
    { "\r\n@GImmortals@n\r\n\r\n"     , ADMLVL_IMMORT, ADMLVL_GRGOD - 1, 0},
    { "\r\n@GMortal@n\r\n\r\n"        , 0, ADMLVL_IMMORT - 1, 0 }*/
  };
  char *tmstr;
  tmstr = (char *) asctime(localtime(&PCOUNTDATE));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  int num_ranks = sizeof(rank)/sizeof(rank[0]);
  send_to_char(ch, "\r\n      @r{@b===============  @D[  @DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt @DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");
  for (d = descriptor_list; d && !short_list; d = d->next) {
    if (!desc_is_playing(d))
     continue;
    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (char_get_admlevel(tch) >= ADMLVL_IMMORT)
      line_color = "@w";
    else
      line_color = "@w";

    if (char_can_see(ch, tch) && desc_is_playing(d)) {
    if (*name_search && strcasecmp(char_get_name(tch), name_search) &&
	!strstr(char_get_title(tch), name_search))
      continue;
    if (!char_can_see(ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
      continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
      continue;
    if (questwho && !char_prf_flagged(tch, PRF_QUEST))
      continue;
    if (localwho && world[char_get_in_room(ch)].zone != world[char_get_in_room(tch)].zone)
      continue;
    if (char_prf_flagged(tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT) {
       hide += 1;
       continue;
    }
    if (who_room && (char_get_in_room(tch) != char_get_in_room(ch)))
      continue;
    if (showclass && !(showclass & (1U << char_get_class(tch))))
      continue;
      if (showgroup && (!tch->master || !char_aff_flagged(tch, AFF_GROUP)))
        continue;
      for (i = 0; i < num_ranks; i++)
        if (char_get_admlevel(tch) >= rank[i].min_level && char_get_admlevel(tch) <= rank[i].max_level)
          rank[i].count++;
    }
  }

  for (i = 0; i < num_ranks; i++) {
    if (!rank[i].count && !short_list)
      continue;

    if (short_list)
      send_to_char(ch, "Players\r\n-------\r\n");
    else
      send_to_char(ch, rank[i].disp);

    for (d = descriptor_list; d; d = d->next) {
      if (!desc_is_playing(d))
       continue;
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if ((char_get_admlevel(tch) < rank[i].min_level || char_get_admlevel(tch) > rank[i].max_level) && !short_list)
        continue;
      if (!desc_is_playing(d))
        continue;
      if (*name_search && strcasecmp(char_get_name(tch), name_search) &&
          !strstr(char_get_title(tch), name_search))
       continue;
      if (!char_can_see(ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (questwho && !char_prf_flagged(tch, PRF_QUEST))
        continue;
      if (localwho && world[char_get_in_room(ch)].zone != world[char_get_in_room(tch)].zone)
        continue;
      if (who_room && (char_get_in_room(tch) != char_get_in_room(ch)))
        continue;
      if (char_prf_flagged(tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT)
        continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
        continue;
      if (showgroup && (!tch->master || !char_aff_flagged(tch, AFF_GROUP)))
        continue;
      if (showleader && (!tch->followers || !char_aff_flagged(tch, AFF_GROUP)))
        continue;

    if (short_list) {
        send_to_char(ch, "               @B[@W%3d @Y%s @C%s@B]@W %-12.12s@n%s@n",
          char_get_level(tch), RACE_ABBR(tch), CLASS_ABBR(tch), char_get_name(tch),
	  ((!(++num_can_see % 4)) ? "\r\n" : ""));
    } else {
       num_can_see++;

       char usr[100];
       sprintf(usr, "@W(@R%s@W)%s", tch->desc->user, char_plr_flagged(tch, PLR_BIOGR) ? "" : (char_is_spoiled(tch) ? " @R*@n" : ""));
       send_to_char(ch, "%s               @D<@C%-12s@D> %s@w%s", line_color, char_get_admlevel(ch) > 0 ? char_get_name(tch) : (char_get_admlevel(tch) > 0 ? char_get_name(tch) : (char_get_user(tch) ? char_get_user(tch) : "NULL")), char_get_admlevel(ch) > 0 ? usr : "", line_color);

      if (char_get_admlevel(tch)) {
        send_to_char(ch, " (%s)", admin_level_names[char_get_admlevel(tch)]);
      }

      if (d->snooping && d->snooping->character != ch && char_get_admlevel(ch) >= 3)
        send_to_char(ch, " (Snoop: %s)", char_get_name(d->snooping->character));
      if (GET_INVIS_LEV(tch))
        send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
      else if (char_aff_flagged(tch, AFF_INVISIBLE))
	send_to_char(ch, " (invis)");

      if (char_plr_flagged(tch, PLR_MAILING))
	send_to_char(ch, " (mailing)");
      else if (d->olc)
        send_to_char(ch, " (OLC)");
      else if (char_plr_flagged(tch, PLR_WRITING))
	send_to_char(ch, " (writing)");

      if (d->original)
        send_to_char(ch, " (out of body)");

      if (d->connected == CON_OEDIT)
        send_to_char(ch, " (O Edit)");
      if (d->connected == CON_MEDIT)
        send_to_char(ch, " (M Edit)");
      if (d->connected == CON_ZEDIT)
        send_to_char(ch, " (Z Edit)");
      if (d->connected == CON_SEDIT)
        send_to_char(ch, " (S Edit)");
      if (d->connected == CON_REDIT)
        send_to_char(ch, " (R Edit)");
      if (d->connected == CON_TEDIT)
        send_to_char(ch, " (T Edit)");
      if (d->connected == CON_TRIGEDIT)
        send_to_char(ch, " (T Edit)");
      if (d->connected == CON_AEDIT)
        send_to_char(ch, " (S Edit)");
      if (d->connected == CON_CEDIT)
        send_to_char(ch, " (C Edit)");
      if (d->connected == CON_HEDIT)
          send_to_char(ch, " (H Edit)");
      if (char_prf_flagged(tch, PRF_DEAF))
	send_to_char(ch, " (DEAF)");
      if (char_prf_flagged(tch, PRF_NOTELL))
	send_to_char(ch, " (NO TELL)");
      if (char_prf_flagged(tch, PRF_NOGOSS))
        send_to_char(ch, " (NO OOC)");
      if (char_plr_flagged(tch, PLR_NOSHOUT))
        send_to_char(ch, " (MUTED)");
      if (char_prf_flagged(tch, PRF_HIDE))
        send_to_char(ch, " (WH)");
      if (char_prf_flagged(tch, PRF_BUILDWALK))
	send_to_char(ch, " (Buildwalking)");
      if (char_prf_flagged(tch, PRF_AFK))
        send_to_char(ch, " (AFK)");
      if (char_plr_flagged(tch, PLR_FISHING) && char_get_admlevel(ch) >= ADMLVL_IMMORT)
        send_to_char(ch, " (@BFISHING@n)");
      if (char_prf_flagged(tch, PRF_NOWIZ))
        send_to_char(ch, " (NO WIZ)");
      send_to_char(ch, "@n\r\n");
      }
    }
    send_to_char(ch, "\r\n");
    if (short_list)
      break;
  }

  if (!num_can_see)
    send_to_char(ch, "                            Nobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(ch, "                         One lonely character displayed.\r\n");
  else {
    send_to_char(ch, "                           @Y%d@w characters displayed.\r\n", num_can_see);
   if (hide > 0) {
    int bam = FALSE;
    if (hide > 1) {
     bam = TRUE;
    }
    send_to_char(ch, "                           and @Y%d@w character%s hidden.\r\n", hide, bam ? "s" : "");
   }
  }
  if (circle_restrict > 0 && circle_restrict <= 100) {
    send_to_char(ch, "                      @rThe mud has been wizlocked to lvl %d@n\r\n", circle_restrict);
  }
  if (circle_restrict == 101) {
    send_to_char(ch, "                      @rThe mud has been wizlocked to IMMs only.@n\r\n");
  }
 send_to_char(ch, "      @r{@b=================================================================@r}@n\r\n");
 send_to_char(ch, "           @cHighest Logon Count Ever@D: @Y%d@w, on %s\r\n", HIGHPCOUNT, tmstr);
 send_to_char(ch, "                        @cHighest Logon Count Today@D: @Y%d@n\r\n", PCOUNT);
}

#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
  char line[200], line2[220], idletime[10];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  while (*buf) {
    char buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      default:
	send_to_char(ch, "%s", USERS_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  }				/* end while (parser) */
  send_to_char(ch,
         "Num Name                 User-name            State          Idl Login    C\r\n"
         "--- -------------------- -------------------- -------------- --- -------- -\r\n");

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (desc_get_connected(d) != CON_PLAYING && playing)
      continue;
    if (desc_get_connected(d) == CON_PLAYING && deadweight)
      continue;
    if (desc_is_playing(d)) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && strcasecmp(char_get_name(tch), name_search))
	continue;
      if (!char_can_see(ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
	continue;
      if (char_prf_flagged(tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT) {
        continue;
      }
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) &&
	  !char_plr_flagged(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1U << char_get_class(tch))))
	continue;
      if (showrace && !(showrace & (1U << char_get_race(tch))))
        continue;
      if (GET_INVIS_LEV(tch) > char_get_admlevel(ch))
	continue;
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (desc_get_connected(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[desc_get_connected(d)]);

    if (d->character && desc_get_connected(d) == CON_PLAYING && char_get_admlevel(d->character) <= char_get_admlevel(ch))
      sprintf(idletime, "%3d", d->character->timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    sprintf(line, "%3d %-20s %-20s %-14s %-3s %-8s %1s ", d->desc_num,
	d->original && d->original->name ? d->original->name :
	d->character && d->character->name ? d->character->name :
	"UNDEFINED", d->user ? d->user : "UNKNOWN", state, idletime, timeptr,
        "N");
    if (d->host && *d->host)
      sprintf(line + strlen(line), "\n%3d [%s Site: %s]\r\n", d->desc_num, d->user ? d->user : "UNKNOWN", d->host);
    else
      sprintf(line + strlen(line), "\n%3d [%s Site: Hostname unknown]\r\n", d->desc_num, d->user ? d->user : "UNKNOWN");

    if (desc_get_connected(d) != CON_PLAYING) {
      sprintf(line2, "@g%s@n", line);
      strcpy(line, line2);
    }
    if (desc_get_connected(d) != CON_PLAYING ||
		(desc_get_connected(d) == CON_PLAYING && char_can_see(ch, d->character))) {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  char arg[MAX_INPUT_LENGTH];
  char bum[10000];
  one_argument(argument, arg);
  
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    char_set_lastpl(ch, time(0));
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    sprintf(bum, "--------------------\r\n%s\r\n--------------------\r\n", policies);
    page_string(ch->desc, bum, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", char_get_name(ch));
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    /*  SYSERR_DESC:
     *  General page string function for such things as 'credits', 'news',
     *  'wizlist', 'clear', 'version'.  This occurs when a call is made to
     *  this routine that is not one of the predefined calls.  To correct
     *  it, either a case needs to be added into the function to account for
     *  the subcmd that is being passed to it, or the call to the function
     *  needs to have the correct subcmd put into place.
     */
    return;
  }
}

static void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (desc_get_connected(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (char_get_in_room(i) == NOWHERE || !char_can_see(ch, i))
	continue;
      if (world[char_get_in_room(ch)].zone != world[char_get_in_room(i)].zone)
	continue;
      send_to_char(ch, "%-20s - %s\r\n", char_get_name(i), world[char_get_in_room(i)].name);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (char_get_in_room(i) == NOWHERE || i == ch)
	continue;
      if (!char_can_see(ch, i) || world[char_get_in_room(i)].zone != world[char_get_in_room(ch)].zone)
	continue;
      if (!isname(arg, i->name))
	continue;
      send_to_char(ch, "%-25s - %s\r\n", char_get_name(i), world[char_get_in_room(i)].name);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
			        int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (SCRIPT(obj))
    send_to_char(ch, "[T%d]", obj->proto_script->vnum);

  if (obj_get_in_room(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s\r\n", rroom_get_vnum(obj_get_in_room(obj)), world[obj_get_in_room(obj)].name);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s in room [%d]\r\n", char_pers(obj->carried_by, ch), rroom_get_vnum(char_get_in_room(obj->carried_by)));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s in room [%d]\r\n", char_pers(obj->worn_by, ch), rroom_get_vnum(char_get_in_room(obj->worn_by)));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}

static void perform_immort_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, num2 = 0, found = 0;
  
  const char *planet[11] = {"@GEarth@n",
                    "@CFrigid@n",
                    "@YVegeta@n",
                    "@MKonack@n",
                    "@gNamek@n",
                    "@mAether@n",
                    "@mArlia@n",
                    "@CZenith@n",
                    "@YYardrat@n",
                    "@cKanassa@n",
                    "@RUNKOWN@n"
          };


  if (!*arg) {
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "GODCMD: %s has checked where to check player locations", char_get_name(ch));
    send_to_char(ch, "Players                  Vnum    Planet        Location\r\n-------                 ------   ----------    ----------------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (desc_is_playing(d)) {
        if (char_get_in_room(d->character) != NOWHERE) {
         if (rroom_flagged(char_get_in_room(d->character), ROOM_EARTH)) {
          num2 = 0;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_FRIGID)) {
          num2 = 1;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_VEGETA)) {
          num2 = 2;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_KONACK)) {
          num2 = 3;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_NAMEK)) {
          num2 = 4;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_AETHER)) {
          num2 = 5;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_ARLIA)) {
          num2 = 6;
         } else if (rroom_planet_zenith(char_get_in_room(d->character))) {
          num2 = 7;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_YARDRAT)) {
          num2 = 8;
         } else if (rroom_flagged(char_get_in_room(d->character), ROOM_KANASSA)) {
          num2 = 9;
         } else {
          num2 = 10;
         }
        }
	i = (d->original ? d->original : d->character);
	if (i && char_can_see(ch, i) && (char_get_in_room(i) != NOWHERE)) {
	  if (d->original)
	    send_to_char(ch, "%-20s - [%5d]   %s (in %s)\r\n",
		char_get_name(i), rroom_get_vnum(char_get_in_room(d->character)),
		world[char_get_in_room(d->character)].name, char_get_name(d->character));
	  else {
	    send_to_char(ch, "%-20s - [%5d]   %-14s %s\r\n", char_get_name(i), rroom_get_vnum(char_get_in_room(i)), planet[num2], world[char_get_in_room(i)].name);
          }
           
	}
      }
  } else {
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "GODCMD: %s has checked where for the location of %s", char_get_name(ch), arg);
    for (i = character_list; i; i = i->next) {
      if (char_can_see(ch, i) && char_get_in_room(i) != NOWHERE && isname(arg, i->name)) {
	found = 1;
	send_to_char(ch, "M%3d. %-25s - [%5d] %-25s", ++num, char_get_name(i),
		rroom_get_vnum(char_get_in_room(i)), world[char_get_in_room(i)].name);
        if (char_is_npc(i) && SCRIPT(i)) { 
          if (!TRIGGERS(SCRIPT(i))->next) 
            send_to_char(ch, "[T%5d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(i)))); 
          else 
            send_to_char(ch, "[TRIGS] ");
        }
        send_to_char(ch, "\r\n");
      }
    }
    for (k = object_list; k; k = k->next)
      if (char_can_see_obj(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found) {
      send_to_char(ch, "Couldn't find any such thing.\r\n");
    } else {
      send_to_char(ch, "\r\nFound %d matches.\r\n", num);
    }
  }
}

ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_adm_flagged(ch, ADM_FULLWHERE) || char_get_admlevel(ch) > 4)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}

ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH];
  size_t i, len = 0, nlen;

  if (char_is_npc(ch)) {
    send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  for (i = 1; i < 101; i++) {
    if (i == 100)
      nlen = snprintf(buf + len, sizeof(buf) - len, "[100] %8s          : \r\n", add_commas(level_exp(ch, 100)));
    else
    nlen = snprintf(buf + len, sizeof(buf) - len, "[%2" SZT "] %8s-%-8s : \r\n", i,
		add_commas(level_exp(ch, i)), add_commas(level_exp(ch, i + 1) - 1));
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;
  }

  page_string(ch->desc, buf, TRUE);
}

ACMD(do_consider)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  diff = (char_get_level(victim) - char_get_level(ch));

  if (diff <= -10)
    send_to_char(ch, "Now where did that chicken go?\r\n");
  else if (diff <= -5)
    send_to_char(ch, "You could do it with a needle!\r\n");
  else if (diff <= -2)
    send_to_char(ch, "Easy.\r\n");
  else if (diff <= -1)
    send_to_char(ch, "Fairly easy.\r\n");
  else if (diff == 0)
    send_to_char(ch, "The perfect match!\r\n");
  else if (diff <= 1)
    send_to_char(ch, "You could probably manage it.\r\n");
  else if (diff <= 2)
    send_to_char(ch, "You might take a beating.\r\n");
  else if (diff <= 3)
    send_to_char(ch, "You MIGHT win, maybe.\r\n");
  else if (diff <= 5)
    send_to_char(ch, "Do you feel lucky? You better.\r\n");
  else if (diff <= 10)
    send_to_char(ch, "Better bring some tough backup!\r\n");
  else if (diff <= 25)
    send_to_char(ch, "Maybe if they are allergic to you, otherwise your last words will be 'Oh shit.'\r\n");
  else
    send_to_char(ch, "No chance.\r\n");
}

ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else {
      send_to_char(ch, "%s", char_get_sex(vict) == SEX_MALE ? "He " : (char_get_sex(vict) == SEX_FEMALE ? "She " : "It "));
      diag_char_to_char(vict, ch);
    }
  } else {
    if (char_get_fighting(ch)) {
      send_to_char(ch, "%s", char_get_sex(char_get_fighting(ch)) == SEX_MALE ? "He " : (char_get_sex(char_get_fighting(ch)) == SEX_FEMALE ? "She " : "It "));
      diag_char_to_char(char_get_fighting(ch), ch);
    }
    else {
      send_to_char(ch, "Diagnose who?\r\n");
    }
  }
}

static const char *ctypes[] = {
  "off", "on", "\n"
};

char *cchoice_to_str(char *col)
{
  static char buf[READ_SIZE];
  char *s = NULL;
  int i = 0;
  int fg = 0;
  int needfg = 0;
  int bold = 0;

  if (!col) {
    buf[0] = 0;
    return buf;
  }
  while (*col) {
    if (strchr(ANSISTART, *col)) {
      col++;
    } else {
      switch (*col) {
      case ANSISEP:
      case ANSIEND:
        s = NULL;
        break;
      case '0':
        s = NULL;
        break;
      case '1':
        bold = 1;
        s = NULL;
        break;
      case '5':
        s = "blinking";
        break;
      case '7':
        s = "reverse";
        break;
      case '8':
        s = "invisible";
        break;
      case '3':
        col++;
        fg = 1;
        switch (*col) {
        case '0':
          s = bold ? (char*)"grey" : (char*)"black";
          bold = 0;
          fg = 1;
          break;
        case '1':
          s = "red";
          fg = 1;
          break;
        case '2':
          s = "green";
          fg = 1;
          break;
        case '3':
          s = "yellow";
          fg = 1;
          break;
        case '4':
          s = "blue";
          fg = 1;
          break;
        case '5':
          s = "magenta";
          fg = 1;
          break;
        case '6':
          s = "cyan";
          fg = 1;
          break;
        case '7':
          s = "white";
          fg = 1;
          break;
        case 0:
          s = NULL;
          break;
        }
        break;
      case '4':
        col++;
        switch (*col) {
        case '0':
          s = "on black";
          needfg = 1;
          bold = 0;
        case '1':
          s = "on red";
          needfg = 1;
          bold = 0;
        case '2':
          s = "on green";
          needfg = 1;
          bold = 0;
        case '3':
          s = "on yellow";
          needfg = 1;
          bold = 0;
        case '4':
          s = "on blue";
          needfg = 1;
          bold = 0;
        case '5':
          s = "on magenta";
          needfg = 1;
          bold = 0;
        case '6':
          s = "on cyan";
          needfg = 1;
          bold = 0;
        case '7':
          s = "on white";
          needfg = 1;
          bold = 0;
        default:
          s = "underlined";
          break;
        }
        break;
      default:
        s = NULL;
        break;
      }
      if (s) {
        if (needfg && !fg) {
          i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
          fg = 1;
        }
        if (i)
          i += snprintf(buf + i, sizeof(buf) - i, " ");
        if (bold) {
          i += snprintf(buf + i, sizeof(buf) - i, "bright ");
          bold = 0;
        }
        i += snprintf(buf + i, sizeof(buf) - i, "%s", s ? s : "null 1");
        s = NULL;
      }
      col++;
    }
  }
  if (!fg)
    i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
  return buf;
}

int str_to_cchoice(char *str, char *choice)
{
  char buf[MAX_STRING_LENGTH];
  int bold = 0, blink = 0, uline = 0, rev = 0, invis = 0, fg = 0, bg = 0, error = 0;
  int i, len = MAX_INPUT_LENGTH;
  struct {
    char *name; 
    int *ptr;
  } attribs[] = {
    { "bright", &bold },
    { "bold", &bold },
    { "underlined", &uline },
    { "reverse", &rev },
    { "blinking", &blink },
    { "invisible", &invis },
    { NULL, NULL }
  };
  struct {
    char *name;
    int val;
    int bold;
  } colors[] = {
    { "default", -1, 0 },
    { "normal", -1, 0 },
    { "black", 0, 0 },
    { "red", 1, 0 },
    { "green", 2, 0 },
    { "yellow", 3, 0 },
    { "blue", 4, 0 },
    { "magenta", 5, 0 },
    { "cyan", 6, 0 },
    { "white", 7, 0 },
    { "grey", 0, 1 },
    { "gray", 0, 1 },
    { NULL, 0, 0 }
  };
  skip_spaces(&str);
  if (isdigit(*str)) { /* Accept a raw code */
    strcpy(choice, str);
    for (i = 0; choice[i] && (isdigit(choice[i]) || choice[i] == ';'); i++);
    error = choice[i] != 0;
    choice[i] = 0;
    return error;
  }
  while (*str) {
    str = any_one_arg(str, buf);
    if (!strcmp(buf, "on")) {
      bg = 1;
      continue;
    }
    if (!fg) {
      for (i = 0; attribs[i].name; i++)
        if (!strncmp(attribs[i].name, buf, strlen(buf)))
          break;
      if (attribs[i].name) {
        *(attribs[i].ptr) = 1;
        continue;
      }
    }
    for (i = 0; colors[i].name; i++)
      if (!strncmp(colors[i].name, buf, strlen(buf)))
        break;
    if (!colors[i].name) {
      error = 1;
      continue;
    }
    if (colors[i].val != -1) {
      if (bg == 1) {
        bg = 40 + colors[i].val;
      } else {
        fg = 30 + colors[i].val;
        if (colors[i].bold)
          bold = 1;
      }
    }
  }
  choice[0] = i = 0;
  if (bold)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_BOLD);
  if (uline)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_UNDERLINE);
  if (blink)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_BLINK);
  if (rev)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_REVERSE);
  if (invis)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_INVIS);
  if (!i)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_NORMAL);
  if (fg && fg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "" , fg);
  if (bg && bg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "" , bg);

  return error;
}

char *default_color_choices[NUM_COLOR+1] = {
/* COLOR_NORMAL */	AA_NORMAL,
/* COLOR_ROOMNAME */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMOBJS */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_ROOMPEOPLE */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_HITYOU */	AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUHIT */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_OTHERHIT */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_CRITICAL */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_HOLLER */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_SHOUT */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_GOSSIP */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_AUCTION */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_CONGRAT */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_TELL */	AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUSAY */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMSAY */	AA_NORMAL ANSISEPSTR AF_WHITE,
NULL
};

ACMD(do_color)
{
  char arg[MAX_INPUT_LENGTH];
  char *p;
  int tp;

  /*if (IS_NPC(ch))
    return;*/

  p = any_one_arg(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  switch (tp) {
    case C_OFF:
      char_prf_set(ch, PRF_COLOR, false);
      break;
    case C_ON:
      char_prf_set(ch, PRF_COLOR, true);
      break;
  }
  send_to_char(ch, "Your color is now @o%s@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle)
{
  char buf2[4];

  if (char_is_npc(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");	/* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch));	/* sprintf: OK */

  if (char_get_admlevel(ch)) {
    send_to_char(ch,
          "      Buildwalk: %-3s    "
          "Clear Screen in OLC: %-3s\r\n",
        ONOFF(char_prf_flagged(ch, PRF_BUILDWALK)),
        ONOFF(char_prf_flagged(ch, PRF_CLS)));

    send_to_char(ch,
	  "      No Hassle: %-3s    "
	  "      Holylight: %-3s    "
	  "     Room Flags: %-3s\r\n",
	ONOFF(char_prf_flagged(ch, PRF_NOHASSLE)),
	ONOFF(char_prf_flagged(ch, PRF_HOLYLIGHT)),
	ONOFF(char_prf_flagged(ch, PRF_ROOMFLAGS))
    );
  }

  send_to_char(ch,
	  "Hit Pnt Display: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Move Display: %-3s    "
	  "   Compact Mode: %-3s    "
	  "       On Quest: %-3s\r\n"

	  "    Exp Display: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

	  "     Ki Display: %-3s    "
	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

          "      Auto Loot: %-3s    "
          "      Auto Gold: %-3s    "
	  "    Color Level: %s\r\n"

          "     Auto Split: %-3s    "
          "       Auto Sac: %-3s    "
          "       Auto Mem: %-3s\r\n"

          "     View Order: %-3s    "
          "    Auto Assist: %-3s    "
	  " Auto Show Exit: %-3s\r\n"

	  "    TNL Display: %-3s    ",

	  ONOFF(char_prf_flagged(ch, PRF_DISPHP)),
	  ONOFF(char_prf_flagged(ch, PRF_BRIEF)),
	  ONOFF(!char_prf_flagged(ch, PRF_SUMMONABLE)),

	  ONOFF(char_prf_flagged(ch, PRF_DISPMOVE)),
	  ONOFF(char_prf_flagged(ch, PRF_COMPACT)),
	  YESNO(char_prf_flagged(ch, PRF_QUEST)),

	  ONOFF(char_prf_flagged(ch, PRF_DISPEXP)),
	  ONOFF(char_prf_flagged(ch, PRF_NOTELL)),
	  YESNO(!char_prf_flagged(ch, PRF_NOREPEAT)),

	  ONOFF(char_prf_flagged(ch, PRF_DISPKI)),
	  YESNO(char_prf_flagged(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!char_prf_flagged(ch, PRF_NOGOSS)),
	  ONOFF(!char_prf_flagged(ch, PRF_NOAUCT)),
	  ONOFF(!char_prf_flagged(ch, PRF_NOGRATZ)),

          ONOFF(char_prf_flagged(ch, PRF_AUTOLOOT)),
          ONOFF(char_prf_flagged(ch, PRF_AUTOGOLD)),
	  ctypes[COLOR_LEV(ch)],

          ONOFF(char_prf_flagged(ch, PRF_AUTOSPLIT)),
          ONOFF(char_prf_flagged(ch, PRF_AUTOSAC)),
          ONOFF(char_prf_flagged(ch, PRF_AUTOMEM)),

          ONOFF(char_prf_flagged(ch, PRF_VIEWORDER)),
          ONOFF(char_prf_flagged(ch, PRF_AUTOASSIST)),
	  ONOFF(char_prf_flagged(ch, PRF_AUTOEXIT)),

	  ONOFF(char_prf_flagged(ch, PRF_DISPTNL)));

          if (CONFIG_ENABLE_COMPRESSION) {
            send_to_char(ch, "    Compression: %-3s\r\n", ONOFF(!char_prf_flagged(ch, PRF_NOCOMPRESS)));
          }
}

static int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;	/* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || char_is_npc(vict)) {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
    if (char_get_level(ch) < char_get_level(vict)) {
      send_to_char(ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : char_get_name(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    i = cmd_sort_info[cmd_num];

    if (complete_cmd_info[i].minimum_level < 0 ||
        char_get_level(vict) < complete_cmd_info[i].minimum_level)
      continue;

    if (complete_cmd_info[i].minimum_admlevel < 0 ||
        char_get_admlevel(vict) < complete_cmd_info[i].minimum_admlevel)
      continue;

    if ((complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action ||
                                complete_cmd_info[i].command_pointer == do_insult))
      continue;

     if (check_disabled(&complete_cmd_info[i]))
      sprintf(arg, "(%s)", complete_cmd_info[i].command);
    else  
      sprintf(arg, "%s", complete_cmd_info[i].command);

    send_to_char(ch, "%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");

  }

  if (no % 7 != 1)
    send_to_char(ch, "\r\n");
}

static void free_history(struct char_data *ch, int type)
{ 
  struct txt_block *tmp = GET_HISTORY(ch, type), *ftmp;      
 
  while ((ftmp = tmp)) { 
    tmp = tmp->next; 
    if (ftmp->text)    
      free(ftmp->text); 
    free(ftmp); 
  } 
  GET_HISTORY(ch, type) = NULL;        
} 

ACMD(do_history) 
{ 
  char arg[MAX_INPUT_LENGTH]; 
  int type;
 
  one_argument(argument, arg); 
 
  type = search_block(arg, history_types, FALSE); 
  if (!*arg || type < 0) { 
    int i; 
 
    send_to_char(ch, "Usage: history <"); 
    for (i = 0; *history_types[i] != '\n'; i++) { 
      if ((i != 3 && char_get_admlevel(ch) <= 0) || char_get_admlevel(ch) >= 1) {
      send_to_char(ch, " %s ", history_types[i]); 
      }
      if (*history_types[i + 1] == '\n') {
        send_to_char(ch, ">\r\n"); 
      }
      else {
       if ((i != 3 && char_get_admlevel(ch) <= 0) || char_get_admlevel(ch) >= 1) {
        send_to_char(ch, "|"); 
       }
      }
    } 
    return; 
  } 
 
  if (GET_HISTORY(ch, type) && GET_HISTORY(ch, type)->text && *GET_HISTORY(ch, type)->text) { 
    struct txt_block *tmp; 
    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next) 
      send_to_char(ch, "%s", tmp->text); 
/* Make this a 1 if you want history to cear after viewing */ 
#if 0 
      free_history(ch, type); 
#endif 
  } else 
    send_to_char(ch, "You have no history in that channel.\r\n"); 
} 

void add_history(struct char_data *ch, char *str, int type) 
{ 
  int i = 0; 
  char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH]; 
  struct txt_block *tmp;
  time_t ct;

  if (char_is_npc(ch))
    return;

  tmp = GET_HISTORY(ch, type);      
  ct = time(0);
  strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct)); 
 
  sprintf(buf, "%s%s", time_str, str); 
 
  if (!tmp) { 
    CREATE(GET_HISTORY(ch, type), struct txt_block, 1); 
    GET_HISTORY(ch, type)->text = strdup(buf); 
  }  
  else {    
    while (tmp->next) 
      tmp = tmp->next; 
    CREATE(tmp->next, struct txt_block, 1); 
    tmp->next->text = strdup(buf); 
 
    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next, i++); 
 
    for (; i > HIST_LENGTH && GET_HISTORY(ch, type); i--) { 
      tmp = GET_HISTORY(ch, type); 
      GET_HISTORY(ch, type) = tmp->next; 
      if (tmp->text) 
        free(tmp->text); 
      free(tmp); 
    } 
  } 
  /* add this history message to ALL */ 
  if (type != HIST_ALL) 
    add_history(ch, str, HIST_ALL); 
} 

ACMD(do_scan) 
{ 
  int i,newroom;
  char *dirnames[]= { 
    "North", "East", "South", "West", "Up", "Down", "Northwest", "Northeast", "Southeast", "Southwest", "Inside", "Outside"}; 

  if(char_get_position(ch) < POS_SLEEPING){ 
    send_to_char(ch, "You can't see anything but stars!\n\r"); 
    return; 
  } 
  if(!char_is_awake(ch)){ 
    send_to_char(ch, "You must be dreaming.\n\r"); 
    return; 
  } 
  if(char_aff_flagged(ch, AFF_BLIND)){ 
    send_to_char(ch, "You can't see a damn thing, you're blind!\n\r"); 
    return; 
  } 
    if (char_plr_flagged(ch, PLR_EYEC)) {
      send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
      return;
    }
  for(i=0;i<10;i++){ 
    if(char_get_exit(ch, i)) { 
      if(room_is_dark(ch->in_room) && (char_get_admlevel(ch) < ADMLVL_IMMORT) && 
          (!char_aff_flagged(ch, AFF_INFRAVISION))){ 
        send_to_char(ch,"%s: DARK\n\r",dirnames[i]); 
        continue; 
      } 
      if(char_can_go(ch,i)) { 
        newroom=world[ch->in_room].dir_option[i]->to_room; 
        send_to_char(ch, "@w-----------------------------------------@n\r\n");
        send_to_char(ch,"          %s%s: %s %s\n\r", CCCYN(ch, C_NRM), dirnames[i], world[newroom].name ? world[newroom].name : "You don't think you saw what you just saw.", CCNRM(ch, C_NRM)); 
        send_to_char(ch, "@W          -----------------          @n\r\n");

        list_obj_to_char(world[newroom].contents,ch,SHOW_OBJ_LONG, FALSE); 
        list_char_to_char(world[newroom].people,ch); 
         if (rroom_get_geffect(newroom) >= 1 && rroom_get_geffect(newroom) <= 5) {
           send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
         }
         if (rroom_get_geffect(newroom) >= 6) {
           send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
         }
        /* Check 2nd room away */ 
        if(char_get_exit2(ch, i) && char_get_exit2(ch, i)->to_room) { 
          newroom = char_get_exit2(ch, i)->to_room; 

          if ((newroom != NOWHERE) && (!IS_SET(char_get_exit2(ch, i)->exit_info, EX_CLOSED)) ) { 
            if (!room_is_dark(char_get_exit2(ch, i)->to_room)) { 
              send_to_char(ch, "@w-----------------------------------------@n\r\n");
              send_to_char(ch,"          %sFar %s: %s %s\n\r", CCCYN(ch, C_NRM), dirnames[i], world[newroom].name ? world[newroom].name : "You don't think you saw what you just saw.", CCNRM(ch, C_NRM) ); 
              send_to_char(ch, "@W          -----------------          @n\r\n");

             list_obj_to_char(world[newroom].contents,ch,SHOW_OBJ_LONG, FALSE); 
             list_char_to_char(world[newroom].people,ch); 
             if (rroom_get_geffect(newroom) >= 1 && rroom_get_geffect(newroom) <= 5) {
              send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
             }
             if (rroom_get_geffect(newroom) >= 6) {
              send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
             }
            } 
            else { 
              send_to_char(ch, "%s<-> %sFar %s: Too dark to tell! %s<->%s\r\n", QMAG, QCYN, dirnames[i], QMAG, QNRM); 
            } 
          } 
        } 
      } 
    } 
  } 
              send_to_char(ch, "@w-----------------------------------------@n\r\n");
}

ACMD(do_toplist)
{
  if (char_is_npc(ch))
   return;

  FILE *file;
  char fname[40], filler[50], line[256];
  int64_t points[25] = {0}, stats;
  char *title[25] = {""};
  int count = 0, x = 0;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
    send_to_char(ch, "The toplist file does not exist.");
    return;
  }
  else if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The toplist file does not exist.");
    return;
  }
  while (!feof(file) || count < 25) {
    get_line(file, line);
    switch (count) {
     default:
      sscanf(line, "%s %" I64T "\n", filler, &stats);
      break;

    }
     title[count] = strdup(filler);
     points[count] = stats;
     count++;
     *filler = '\0';
  }
  send_to_char(ch, "@D-=[@BDBAT Top Lists for @REra@C %d@D]=-@n\r\n", CURRENT_ERA);
  while (x <= count) {
   switch (x) {
    /* Powerlevel Area */
    case 0:
     send_to_char(ch, "       @D-@RPowerlevel@D-@n\r\n");
     send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 1:
     send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 2:
     send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 3:
     send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 4:
     send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    /* Ki Area */
    case 5:
     send_to_char(ch, "       @D-@BKi        @D-@n\r\n");
     send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 6:
     send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 7:
     send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 8:
     send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 9:
     send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    /* Stamina Area */
    case 10:
     send_to_char(ch, "       @D-@GStamina   @D-@n\r\n");
     send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 11:
     send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 12:
     send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 13:
     send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 14:
     send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    /* Stamina Area */
    case 15:
     send_to_char(ch, "       @D-@gZenni     @D-@n\r\n");
     send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 16:
     send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 17:
     send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 18:
     send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    case 19:
     send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
     free(title[x]);
     break;
    /* Rpp Area */
    case 20:
     /*send_to_char(ch, "       @D-@mRPP       @D-@n\r\n");
     send_to_char(ch, "    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);*/
     free(title[x]);
     break;
    case 21:
     /*send_to_char(ch, "    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);*/
     free(title[x]);
     break;
    case 22:
     /*send_to_char(ch, "    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);*/
     free(title[x]);
     break;
    case 23:
     /*send_to_char(ch, "    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);*/
     free(title[x]);
     break;
    case 24:
     /*send_to_char(ch, "    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);*/
     free(title[x]);
     break;
   }
   x++;
  }
  fclose(file);
}

ACMD(do_whois)
{
  char buf[MAX_INPUT_LENGTH];
  int clan = FALSE;
  const char *immlevels[ADMLVL_IMPL + 2] = {
  "[Mortal]",          /* lowest admin level */
  "[Enforcer]",        /* lowest admin level +1 */
  "[First Class Enforcer]",         /* lowest admin level +2 */
  "[High Enforcer]",             /* lowest admin level +3 */
  "[Vice Admin]",     /* lowest admin level +4 */
  "[Administrator]",     /* lowest admin level +5 */
  "[Implementor]",
  };

  struct char_data *victim = 0;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Who?\r\n");
  } else {
  CREATE(victim, struct char_data, 1);
  clear_char(victim);
  CREATE(victim->player_specials, struct player_special_data, 1);
  if (load_char(argument, victim) >= 0) {
    clan = FALSE;
    send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
    if (char_get_admlevel(victim) >= ADMLVL_IMMORT) {
      send_to_char(ch, "@cName     @D: @G%s\r\n", char_get_name(victim));
      send_to_char(ch, "@cImm Level@D: @G%s\r\n", immlevels[char_get_admlevel(victim)]);
      send_to_char(ch, "@cTitle    @D: @G%s\r\n", char_get_title(victim));
    }
    else {
      send_to_char(ch, "@cName  @D: @w%s\r\n@cSensei@D: @w%s\r\n@cRace  @D: @w%s\r\n@cTitle @D: @w%s@n\r\n@cClan  @D: @w%s@n\r\n", char_get_name(victim), SENSEI_NAME(victim), TRUE_RACE(victim), char_get_title(victim), clan ? buf : "None.");
     }
    send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
    } else {
    send_to_char(ch, "There is no such player.\r\n"); 
    }
    free(victim); 
  }
}

#define DOOR_DCHIDE(ch, door)           (char_get_exit(ch, door)->dchide)

static void search_in_direction(struct char_data * ch, int dir)
{
  int check=FALSE, skill_lvl, dchide=20;

  send_to_char(ch, "You search for secret doors.\r\n");
  act("$n searches the area intently.", TRUE, ch, 0, 0, TO_ROOM);

  /* SEARCHING is allowed untrained */
  skill_lvl = char_get_skill(ch, SKILL_SEARCH);
  if (char_is_truffle(ch) || char_is_human(ch)) 
    skill_lvl = skill_lvl + 2;
  if (char_is_halfbreed(ch))
    skill_lvl = skill_lvl + 1;

  if (char_get_exit(ch, dir))
    dchide = DOOR_DCHIDE(ch, dir);

  if (skill_lvl > dchide)
    check = TRUE;

  if (char_get_exit(ch, dir)) {
    if (char_get_exit(ch, dir)->general_description &&
        !room_exit_flagged(char_get_exit(ch, dir), EX_SECRET))
      send_to_char(ch, char_get_exit(ch, dir)->general_description);
    else if (!room_exit_flagged(char_get_exit(ch, dir), EX_SECRET))
      send_to_char(ch, "There is a normal exit there.\r\n");
    else if (room_exit_flagged(char_get_exit(ch, dir), EX_ISDOOR) &&
             room_exit_flagged(char_get_exit(ch, dir), EX_SECRET) &&
             char_get_exit(ch, dir)->keyword && (check == TRUE)  )
      send_to_char(ch, "There is a hidden door keyword: '%s' %sthere.\r\n",
                   fname (char_get_exit(ch, dir)->keyword),
                   (room_exit_flagged(char_get_exit(ch, dir), EX_CLOSED)) ? "" : "open ");
    else
      send_to_char(ch, "There is no exit there.\r\n");
  } else
    send_to_char(ch, "There is no exit there.\r\n");
}
