/* ************************************************************************
*   File: chargen.c                                     Part of CircleMUD *
*  Usage: character creation                                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/config.h"
#include "dbat/dg_scripts.h"
#include "dbat/statedit.h"

#include <unistd.h>
#include "nanny_internal.h"

static void display_races_sub(descriptor_data *d)
{
  int i = 0;
  for (int r = 0; r < NUM_RACES; r++) {
    if (!race_is_playable(r) || !race_valid_sex(r, char_get_sex(d->character)))
      continue;
    send_to_char(d->character, "@C%-15s@D[@R%i RPP@D]@n%s", race_name(r), race_rpp_cost(r), !(++i % 2) ? "\r\n" : "   ");
  }
}

void display_races(struct descriptor_data *d)
{
  send_to_char(d->character, "\r\n@YRace SELECTION menu:\r\n@D---------------------------------------\r\n@n");
  display_races_sub(d);

  send_to_char(d->character, "\n @BR@W) @CRandom Race Selection!\r\n@n");
  send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(d->character, "\n@WRace: @n");
}

static void display_classes_sub(descriptor_data *d)
{
  int i = 0;
  for (int s = 0; s < NUM_CLASSES; s++) {
    if (!sensei_is_pc_ok(s) || !sensei_available_for_race(s, d->character->race))
      continue;
    send_to_char(d->character, "@C%s@n%s", sensei_name(s), !(++i % 2) ? "\r\n" : "	");
  }
}

static void display_classes(struct descriptor_data *d)
{
  send_to_char(d->character, "\r\n@YSensei SELECTION menu:\r\n@D--------------------------------------\r\n@n");
  display_classes_sub(d);

  send_to_char(d->character, "\n @BR@W) @CRandom Sensei Selection!\r\n@n");
  send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(d->character, "\n@WSensei: @n");
}

static void display_races_help(struct descriptor_data *d)
{
  send_to_char(d->character, "\r\n@YRace HELP menu:\r\n@G--------------------------------------------\r\n@n");
  display_races_sub(d);

  send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(d->character, "\n@WHelp on Race #: @n");
}

static void display_classes_help(struct descriptor_data *d)
{
  send_to_char(d->character, "\r\n@YClass HELP menu:\r\n@G-------------------------------------------\r\n@n");
  display_classes_sub(d);

  send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(d->character, "\n@WHelp on Class #: @n");
}

/* This is the bonus/negatives menu for Character Creation */
static void display_bonus_menu(struct char_data *ch, int type)
{
  int BonusCount = 26, NegCount = 52, x = 0, y = 0;
  const char *bonus[] = {
      "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost             @D[@G-2pts @D]",                                                                                                                                                              /* Bonus 0 */
      "Prodigy     - +25% Experience Gained Until Level 80                  @D[@G-5pts @D]",                                                                                                                                                              /* Bonus 1 */
      "Quick Study - Character auto-trains skills faster                    @D[@G-3pts @D]",                                                                                                                                                              /* Bonus 2 */
      "Die Hard    - Life Force's PL regen doubled, but cost is the same    @D[@G-6pts @D]",                                                                                                                                                              /* Bonus 3 */
      "Brawler     - Physical attacks do 20% more damage                    @D[@G-4pts @D]",                                                                                                                                                              /* Bonus 4 */
      "Destroyer   - Damaged Rooms act as regen rooms for you               @D[@G-3pts @D]",                                                                                                                                                              /* Bonus 5 */
      "Hard Worker - Physical rewards better + activity drains less stamina @D[@G-3pts @D]",                                                                                                                                                              /* Bonus 6 */
      "Healer      - Heal/First-aid/Vigor/Repair restore +10%               @D[@G-3pts @D]",                                                                                                                                                              /* Bonus 7 */
      "Loyal       - +20% Experience When Grouped As Follower               @D[@G-2pts @D]",                                                                                                                                                              /* Bonus 8 */
      "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%     @D[@G-5pts @D]",                                                                                                                                                              /* Bonus 9 */
      "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75% @D[@G-5pts @D]",                                                                                                                                                              /* Bonus 10 */
      "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%       @D[@G-5pts @D]",                                                                                                                                                              /* Bonus 11 */
      "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%      @D[@G-4pts @D]",                                                                                                                                                              /* Bonus 12 */
      "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%        @D[@G-6pts @D]",                                                                                                                                                              /* Bonus 13 */
      "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%       @D[@G-5pts @D]",                                                                                                                                                              /* Bonus 14 */
      "Thick Skin  - -20% Physical and -10% ki dmg received                 @D[@G-5pts @D]",                                                                                                                                                              /* Bonus 15 */
      "Recipe Int. - Food cooked by you lasts longer/heals better           @D[@G-2pts @D]",                                                                                                                                                              /* Bonus 16 */
      "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn         @D[@G-4pts @D]",                                                                                                                                                              /* Bonus 17 */
      "Powerhitter - 15% critical hits will be x4 instead of x2             @D[@G-4pts @D]",                                                                                                                                                              /* Bonus 18 */
      "Healthy     - 40% chance to recover from ill effects when sleeping   @D[@G-3pts @D]",                                                                                                                                                              /* Bonus  19 */
      "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis    @D[@G-2pts @D]",                                                                                                                                                              /* Bonus  20 */
      "Evasive     - +15% to dodge rolls                                    @D[@G-3pts @D]",                                                                                                                                                              /* Bonus  21 */
      "The Wall    - +20% chance to block                                   @D[@G-3pts @D]",                                                                                                                                                              /* Bonus  22 */
      "Accurate    - +20% chance to hit physical, +10% to hit with ki       @D[@G-4pts @D]",                                                                                                                                                              /* Bonus  23 */
      "Energy Leech- -2% ki damage received for every 5 character levels,   @D[@G-5pts @D]\n                  @cas long as you can take that ki to your charge pool.@D        ",                                                                          /* Bonus  24*/
      "Good Memory - +2 Skill Slots initially, +1 every 20 levels after     @D[@G-6pts @D]",                                                                                                                                                              /* Bonus 25 */
      "Soft Touch  - Half damage for all hit locations                      @D[@G+5pts @D]",                                                                                                                                                              /* Neg 26 */
      "Late Sleeper- Can only wake automatically. 33% every hour if maxed   @D[@G+5pts @D]",                                                                                                                                                              /* Neg 27 */
      "Impulse Shop- +25% shop costs                                        @D[@G+3pts @D]",                                                                                                                                                              /* Neg 28 */
      "Sickly      - Suffer from harmful effects longer                     @D[@G+5pts @D]",                                                                                                                                                              /* Neg 29 */
      "Punching Bag- -15% to dodge rolls                                    @D[@G+3pts @D]",                                                                                                                                                              /* Neg 30 */
      "Pushover    - -20% block chance                                      @D[@G+3pts @D]",                                                                                                                                                              /* Neg 31 */
      "Poor D. Perc- -20% chance to hit with physical, -10% with ki         @D[@G+4pts @D]",                                                                                                                                                              /* Neg 32 */
      "Thin Skin   - +20% physical and +10% ki damage received              @D[@G+4pts @D]",                                                                                                                                                              /* Neg 33 */
      "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned            @D[@G+5pts @D]",                                                                                                                                                              /* Neg 34 */
      "Energy Int. - +2% ki damage received for every 5 character levels,   @D[@G+6pts @D]\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
      "Coward      - Can't Attack Enemy With 150% Your Powerlevel           @D[@G+6pts @D]",                                                                                                                                                              /* Neg 36 */
      "Arrogant    - Cannot Suppress                                        @D[@G+1pt  @D]",                                                                                                                                                              /* Neg 37 */
      "Unfocused   - Charge concentration randomly breaks                   @D[@G+3pts @D]",                                                                                                                                                              /* Neg 38 */
      "Slacker     - Physical activity drains more stamina                  @D[@G+3pts @D]",                                                                                                                                                              /* Neg 39 */
      "Slow Learner- Character auto-trains skills slower                    @D[@G+3pts @D]",                                                                                                                                                              /* Neg 40 */
      "Masochistic - Defense Skills Cap At 75                               @D[@G+5pts @D]",                                                                                                                                                              /* Neg 41 */
      "Mute        - Can't use IC speech related commands                   @D[@G+4pts @D]",                                                                                                                                                              /* Neg 42 */
      "Wimp        - Strength is capped at 45                               @D[@G+6pts @D]",                                                                                                                                                              /* Neg 43 */
      "Dull        - Intelligence is capped at 45                           @D[@G+6pts @D]",                                                                                                                                                              /* Neg 44 */
      "Foolish     - Wisdom is capped at 45                                 @D[@G+6pts @D]",                                                                                                                                                              /* Neg 45 */
      "Clumsy      - Agility is capped at 45                                @D[@G+3pts @D]",                                                                                                                                                              /* Neg 46 */
      "Slow        - Speed is capped at 45                                  @D[@G+6pts @D]",                                                                                                                                                              /* Neg 47 */
      "Frail       - Constitution capped at 45                              @D[@G+4pts @D]",                                                                                                                                                              /* Neg 48 */
      "Sadistic    - Half Experience Gained For Quick Kills                 @D[@G+3pts @D]",                                                                                                                                                              /* Neg 49 */
      "Loner       - Can't Group, +5% Train gains, +10% to physical gains   @D[@G+2pts @D]",                                                                                                                                                              /* Neg 50 */
      "Bad Memory  - -5 Skill Slots                                         @D[@G+6pts @D]"                                                                                                                                                               /* Neg 51 */
  };

  if (type == 0) {
    send_to_char(ch, "\r\n@YBonus Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
    for (x = 0; x < BonusCount; x++) {
      send_to_char(ch, "@C%-2d@D)@c %s <@g%s@D>\n", x + 1, bonus[x], char_get_bonuses(ch, x) > 0 ? "X" : " ");
    }
    send_to_char(ch, "\n");
  }

  if (type == 1) {
    y = BonusCount;
    send_to_char(ch, "@YNegative Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
    while (y < NegCount) {
      send_to_char(ch, "@R%-2d@D)@r %s <@g%s@D>\n", y - 14, bonus[y], char_get_bonuses(ch, y) > 0 ? "X" : " ");
      y += 1;
    }
  }

  if (type == 0) {
    send_to_char(ch, "\n@CN@D)@c Show Negatives@n\n");
  } else {
    send_to_char(ch, "\n@CB@D)@c Show Bonuses@n\n");
  }
  send_to_char(ch, "@CX@D)@c Exit Traits Section and complete your character@n\n");
  send_to_char(ch, "@D---------------------------------------\n[@WCurrent Points Pool@W: @y%d@D] [@WPTS From Neg@W: @y%d@D]@w\n", char_get_ccpoints(ch), char_get_negcount(ch));
}

/* Return -1 if not an acceptable menu option *
 * Return 31 if selection is X                 *
 * Return other value if Bonus/Negative        */

static int parse_bonuses(const char *arg)
{
  int value = -1, ident = -1;

  switch (*arg) {
    case 'b':
    case 'B':
      value = 53;
      break;
    case 'n':
    case 'N':
      value = 54;
      break;
    case 'x':
    case 'X':
      value = 55;
      break;
  }

  if (value < 52) {
    ident = atoi(arg);
  }

  switch (ident) {
    /* Valid Selections */
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
      value = ident - 1;
      break;

    /* Invalid Selections */
    default:
      value = -1;
      break;
  }

  return (value);
}

/* List names of Bonus/Negative */
const char *list_bonus[] = {
    "Thrifty",               /* Bonus 0 */
    "Prodigy",               /* Bonus 1 */
    "Quick Study",           /* Bonus 2 */
    "Die Hard",              /* Bonus 3 */
    "Brawler",               /* Bonus 4 */
    "Destroyer",             /* Bonus 5 */
    "Hard Worker",           /* Bonus 6 */
    "Healer",                /* Bonus 7 */
    "Loyal",                 /* Bonus 8 */
    "Brawny",                /* Bonus 9 */
    "Scholarly",             /* Bonus 10 */
    "Sage",                  /* Bonus 11 */
    "Agile",                 /* Bonus 12 */
    "Quick",                 /* Bonus 13 */
    "Sturdy",                /* Bonus 14 */
    "Thick Skin",            /* Bonus 15 */
    "Recipe Int",            /* Bonus 16 */
    "Fireproof",             /* Bonus 17 */
    "Powerhitter",           /* Bonus 18 */
    "Healthy",               /* Bonus 19 */
    "Insomniac",             /* Bonus 20 */
    "Evasive",               /* Bonus 21 */
    "The Wall",              /* Bonus 22 */
    "Accurate",              /* Bonus 23 */
    "Energy Leech",          /* Bonus 24 */
    "Good Memory",           /* Bonus 25*/
    "Soft Touch",            /* Neg 26 */
    "Late Sleeper",          /* Neg 27 */
    "Impulse Shop",          /* Neg 28 */
    "Sickly",                /* Neg 29 */
    "Punching Bag",          /* Neg 30 */
    "Pushover",              /* Neg 31 */
    "Poor Depth Perception", /* Neg 32 */
    "Thin Skin",             /* Neg 33 */
    "Fireprone",             /* Neg 34 */
    "Energy Intollerant",    /* Neg 35 */
    "Coward",                /* Neg 36 */
    "Arrogant",              /* Neg 37 */
    "Unfocused",             /* Neg 38 */
    "Slacker",               /* Neg 39 */
    "Slow Learner",          /* Neg 40 */
    "Masochistic",           /* Neg 41 */
    "Mute",                  /* Neg 42 */
    "Wimp",                  /* Neg 43 */
    "Dull",                  /* Neg 44 */
    "Foolish",               /* Neg 45 */
    "Clumsy",                /* Neg 46 */
    "Slow",                  /* Neg 47 */
    "Frail",                 /* Neg 48 */
    "Sadistic",              /* Neg 49 */
    "Loner",                 /* Neg 50 */
    "Bad Memory"             /* Neg 51 */
};

/* List cost of bonus/negative */
static const int list_bonus_cost[] = {
    -2, /* Bonus 0 */
    -5, /* Bonus 1 */
    -3, /* Bonus 2 */
    -6, /* Bonus 3 */
    -4, /* Bonus 4 */
    -3, /* Bonus 5 */
    -3, /* Bonus 6 */
    -3, /* Bonus 7 */
    -2, /* Bonus 8 */
    -5, /* Bonus 9 */
    -5, /* Bonus 10 */
    -5, /* Bonus 11 */
    -4, /* Bonus 12 */
    -6, /* Bonus 13 */
    -5, /* Bonus 14 */
    -5, /* Bonus 15 */
    -2, /* Bonus 16 */
    -4, /* Bonus 17 */
    -4, /* Bonus 18 */
    -3, /* Bonus 19 */
    -2, /* Bonus 20 */
    -3, /* Bonus 21 */
    -3, /* Bonus 22 */
    -4, /* Bonus 23 */
    -5, /* Bonus 24 */
    -6, /* Bonus 25*/
    5,  /* Negative 26 */
    5,  /* Negative 27 */
    3,  /* Negative 28 */
    5,  /* Negative 29 */
    3,  /* Negative 30 */
    3,  /* Negative 31 */
    4,  /* Negative 32 */
    4,  /* Negative 33 */
    5,  /* Negative 34 */
    6,  /* Negative 35 */
    6,  /* Negative 36 */
    1,  /* Negative 37 */
    3,  /* Negative 38 */
    3,  /* Negative 39 */
    3,  /* Negative 40 */
    5,  /* Negative 41 */
    4,  /* Negative 42 */
    6,  /* Negative 43 */
    6,  /* Negative 44 */
    6,  /* Negative 45 */
    3,  /* Negative 46 */
    6,  /* Negative 47 */
    4,  /* Negative 48 */
    3,  /* Negative 49 */
    2,  /* Negative 50 */
    6,  /* Negative 51 */
};

static int opp_bonus(struct char_data *ch, int value, int type)
{
  int give = TRUE;

  switch (value) {
    case 0:
      if (char_get_bonuses(ch, BONUS_IMPULSE)) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[BONUS_IMPULSE], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 2:
      if (char_get_bonuses(ch, 40) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[40], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 3:
      if (char_is_android(ch)) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "You can not take %s as an android!@n\r\n", list_bonus[3]);
        give = FALSE;
      }
      break;
    case 6:
      if (char_get_bonuses(ch, 39) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[39], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 8:
      if (char_get_bonuses(ch, 50) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[50], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 9:
      if (char_get_bonuses(ch, 43) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[43], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 10:
      if (char_get_bonuses(ch, 44) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[44], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 11:
      if (char_get_bonuses(ch, 45) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[45], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 12:
      if (char_get_bonuses(ch, 46) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[46], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 13:
      if (char_get_bonuses(ch, 47) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[47], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 14:
      if (char_get_bonuses(ch, 48) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[48], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 15:
      if (char_get_bonuses(ch, 33) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[33], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 16:
      if (char_is_android(ch)) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "You are an android and can not suppress anyway.\n\n");
        give = FALSE;
      }
      break;
    case 17:
      if (char_is_demon(ch)) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "As a demon you are already fireproof.\r\n");
        give = FALSE;
      } else if (char_get_bonuses(ch, 34) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[34], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 18:
      if (char_get_bonuses(ch, 26) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[26], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 19:
      if (char_get_bonuses(ch, 29) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[29], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 20:
      if (char_get_bonuses(ch, 27) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[27], list_bonus[value]);
        give = FALSE;
      } else if (char_is_android(ch)) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "You can not take %s as an android!@n\r\n", list_bonus[value]);
        give = FALSE;
      }
      break;
    case 21:
      if (char_get_bonuses(ch, 30) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[30], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 22:
      if (char_get_bonuses(ch, 31) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[31], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 23:
      if (char_get_bonuses(ch, 32) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[32], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 24:
      if (char_get_bonuses(ch, 35) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[35], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 25:
      if (char_get_bonuses(ch, 51) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[51], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 26:
      if (char_get_bonuses(ch, 18) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[18], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 27:
      if (char_get_bonuses(ch, 20) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[27], list_bonus[value]);
        give = FALSE;
      } else if (char_is_android(ch)) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "You can not take %s as an android!@n\r\n", list_bonus[value]);
        give = FALSE;
      }
      break;
    case 29:
      if (char_get_bonuses(ch, 19) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[19], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 30:
      if (char_get_bonuses(ch, 21) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[21], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 31:
      if (char_get_bonuses(ch, 22) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[22], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 32:
      if (char_get_bonuses(ch, 23) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[23], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 33:
      if (char_get_bonuses(ch, 15) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[15], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 34:
      if (char_get_bonuses(ch, 17) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[17], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 35:
      if (char_get_bonuses(ch, 24) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[24], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 39:
      if (char_get_bonuses(ch, 6) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[6], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 40:
      if (char_get_bonuses(ch, 2) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[2], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 43:
      if (char_get_bonuses(ch, 9) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[9], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 44:
      if (char_get_bonuses(ch, 10) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[10], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 45:
      if (char_get_bonuses(ch, 11) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[11], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 46:
      if (char_get_bonuses(ch, 12) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[12], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 47:
      if (char_get_bonuses(ch, 13) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[13], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 48:
      if (char_get_bonuses(ch, 14) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[14], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 50:
      if (char_get_bonuses(ch, 8) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[8], list_bonus[value]);
        give = FALSE;
      }
      break;
    case 51:
      if (char_get_bonuses(ch, 25) > 0) {
        display_bonus_menu(ch, type);
        send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[25], list_bonus[value]);
        give = FALSE;
      }
      break;
  }

  return (give);
}

/* Handle CC point exchange for Bonus/negative */
static void exchange_ccpoints(struct char_data *ch, int value)
{
  int type = 0;

  if (desc_get_connected(ch->desc) == CON_BONUS) {
    type = 0;
  } else {
    type = 1;
  }

  if (char_get_bonuses(ch, value) > 0 && char_get_ccpoints(ch) - list_bonus_cost[value] < 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@RYou must unselect some bonus traits first.\r\n");
    return;
  } else if (char_get_bonuses(ch, value) > 0 && char_get_ccpoints(ch) - list_bonus_cost[value] >= 0) {
    char_set_ccpoints(ch, char_get_ccpoints(ch) - (list_bonus_cost[value]));
    if (list_bonus_cost[value] > 0) {
      char_set_negcount(ch, char_get_negcount(ch) - (list_bonus_cost[value]));
    }
    char_set_bonuses(ch, value, 0);
    display_bonus_menu(ch, type);
    send_to_char(ch, "@GYou cancel your selection of %s.\r\n", list_bonus[value]);
    return;
  }
  if (type == 0) {
    if (value > 25) {
      display_bonus_menu(ch, type);
      send_to_char(ch, "@RYou are not in the negatives menu, enter B to switch.\r\n");
      return;
    } else if (char_get_ccpoints(ch) + list_bonus_cost[value] < 0) {
      display_bonus_menu(ch, type);
      send_to_char(ch, "@RYou do not have enough points for %s.\r\n", list_bonus[value]);
      return;
    } else if (!opp_bonus(ch, value, type)) {
      return;
    } else if (list_bonus_cost[value] < 0) {
      char_set_ccpoints(ch, char_get_ccpoints(ch) + (list_bonus_cost[value]));
      char_set_bonuses(ch, value, 1);
      display_bonus_menu(ch, type);
      send_to_char(ch, "@GYou select the bonus %s\r\n", list_bonus[value]);
      return;
    }
  } else {

    if (value < 26) {
      display_bonus_menu(ch, type);
      send_to_char(ch, "@RYou are not in the bonuses menu, enter B to switch.\r\n");
      return;
    }

    int x, count = 0;

    for (x = 14; x < 52; x++) {
      if (char_get_bonuses(ch, x) > 1) {
        count += list_bonus_cost[x];
      }
    }

    if (list_bonus_cost[value] + count > 10) {
      display_bonus_menu(ch, type);
      send_to_char(ch, "@RYou can not have more than +10 points from negatives.\r\n");
      return;
    } else if (!opp_bonus(ch, value, type)) {
      return;
    } else {
      char_set_ccpoints(ch, char_get_ccpoints(ch) + (list_bonus_cost[value]));
      char_set_negcount(ch, char_get_negcount(ch) + (list_bonus_cost[value]));
      char_set_bonuses(ch, value, 2);
      display_bonus_menu(ch, type);
      send_to_char(ch, "@GYou select the negative %s\r\n", list_bonus[value]);
      return;
    }
  }
}

/* deal with newcomers and other non-playing sockets */
/* ---- Menu helpers ---------------------------------------------------- */

/* The position of the answer's first character in keys (letters in either case), or -1. */
static int menu_choice(const char *arg, const char *keys)
{
  const char *key;

  if (!*arg)
    return -1;
  key = strchr(keys, tolower((unsigned char)*arg));
  return key ? (int)(key - keys) : -1;
}

static void reject_option(struct descriptor_data *d)
{
  write_to_output(d, "That is not an acceptable option.\r\n");
}

/* Races with hair (rather than horns, a forelock, antennae or nothing). */
static bool has_hair(const struct char_data *ch)
{
  return char_is_human(ch) || char_is_saiyan(ch) || char_is_konatsu(ch) || char_is_mutant(ch) || char_is_android(ch) || char_is_kai(ch) || char_is_halfbreed(ch) || char_is_truffle(ch) || (char_is_hoshijin(ch) && char_is_female(ch));
}

static void show_sex_prompt(struct descriptor_data *d)
{
  write_to_output(d, "\r\n@wWhat is your sex @W(@BM@W/@MF@W/@GN@W)@w?@n");
}

/* Hair, horn, forelock or antenna length. */
static void show_length_menu(struct descriptor_data *d, const char *part, const char *shortest)
{
  write_to_output(d, "@Y%s Length SELECTION menu:\r\n", part);
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C %s  @B2@W)@C Short  @B3@W)@C Medium\r\n", shortest);
  write_to_output(d, "@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

/* Hair colour, or wing colour for female Arlians. */
static void show_color_menu(struct descriptor_data *d, const char *part)
{
  write_to_output(d, "@Y%s color SELECTION menu:\r\n", part);
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Black  @B2@W)@C Brown  @B3@W)@C Blonde\r\n");
  write_to_output(d, "@B4@W)@C Grey   @B5@W)@C Red    @B6@W)@C Orange@n\r\n");
  write_to_output(d, "@B7@W)@C Green  @B8@W)@C Blue   @B9@W)@C Pink\r\n");
  write_to_output(d, "@BA@W)@C Purple @BB@W)@C Silver @BC@W)@C Crimson@n\r\n");
  write_to_output(d, "@BD@W)@C White@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

static void show_hair_style_menu(struct descriptor_data *d)
{
  write_to_output(d, "@YHair style SELECTION menu:\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Plain     @B2@W)@C Mohawk    @B3@W)@C Spiky\r\n");
  write_to_output(d, "@B4@W)@C Curly     @B5@W)@C Uneven    @B6@W)@C Ponytail@n\r\n");
  write_to_output(d, "@B7@W)@C Afro      @B8@W)@C Fade      @B9@W)@C Crew Cut\r\n");
  write_to_output(d, "@BA@W)@C Feathered @BB@W)@C Dred Locks@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

static void show_skin_menu(struct descriptor_data *d)
{
  write_to_output(d, "@YSkin color SELECTION menu:\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
  write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
  write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
  write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

/* Starts the skin question: the step after hair, or instead of it. */
static void ask_skin(struct descriptor_data *d)
{
  show_skin_menu(d);
  desc_set_connected(d, CON_SKIN);
}

static void show_eye_menu(struct descriptor_data *d)
{
  write_to_output(d, "@YEye color SELECTION menu:\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Blue  @B2@W)@C Black  @B3@W)@C Green\r\n");
  write_to_output(d, "@B4@W)@C Brown @B5@W)@C Red    @B6@W)@C Aqua@n\r\n");
  write_to_output(d, "@B7@W)@C Pink  @B8@W)@C Purple @B9@W)@C Crimson\r\n");
  write_to_output(d, "@BA@W)@C Gold  @BB@W)@C Amber  @BC@W)@C Emerald@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

static void show_feature_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  write_to_output(d, "@YWhat do you want to be your most distinguishing feature:\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C My Eyes@n\r\n");
  if (char_is_majin(ch)) {
    write_to_output(d, "@B2@W)@C My Forelock@n\r\n");
  } else if (char_is_namek(ch) || char_is_arlian(ch)) {
    write_to_output(d, "@B2@W)@C My Antennae@n\r\n");
  } else if (char_is_icer(ch) || char_is_demon(ch)) {
    write_to_output(d, "@B2@W)@C My Horns@n\r\n");
  } else if (char_get_hairl(ch) == HAIRL_BALD) {
    write_to_output(d, "@B2@W)@C My Baldness@n\r\n");
  } else {
    write_to_output(d, "@B2@W)@C My Hair@n\r\n");
  }
  write_to_output(d, "@B3@W)@C My Skin@n\r\n");
  write_to_output(d, "@B4@W)@C My Height@n\r\n");
  write_to_output(d, "@B5@W)@C My Weight@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

/* Height (cm) and weight (kg) ranges offered at creation. */
struct size_range {
  int min_height, max_height, min_weight, max_weight;
};

static const struct size_range normal_sizes[] = {
    {100, 120, 25, 30 },
    {120, 140, 30, 35 },
    {140, 160, 35, 45 },
    {160, 180, 45, 60 },
    {180, 200, 60, 80 },
    {200, 220, 80, 100},
};
static const struct size_range truffle_sizes[] = {
    {20, 35, 5,  8 },
    {35, 40, 8,  10},
    {40, 50, 10, 12},
    {50, 60, 12, 15},
    {60, 70, 15, 18},
};

/* The ranges ch may choose from; Icers get the three smallest normal ones. */
static const struct size_range *size_ranges(const struct char_data *ch, int *count)
{
  if (char_is_truffle(ch)) {
    *count = sizeof(truffle_sizes) / sizeof(truffle_sizes[0]);
    return truffle_sizes;
  }
  *count = char_is_icer(ch) ? 3 : (int)(sizeof(normal_sizes) / sizeof(normal_sizes[0]));
  return normal_sizes;
}

static void show_size_menu(struct descriptor_data *d)
{
  int count;
  const struct size_range *sizes = size_ranges(d->character, &count);

  write_to_output(d, "@YWhat Height/Weight Range do you prefer:\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  for (int i = 0; i < count; i++)
    write_to_output(d, "@B%d@W)@C %d-%dcm, %d-%dkg@n\r\n", i + 1, sizes[i].min_height, sizes[i].max_height, sizes[i].min_weight, sizes[i].max_weight);
  write_to_output(d, "\n@WMake a selection:@n ");
}

static void show_aura_menu(struct descriptor_data *d)
{
  write_to_output(d, "@YAura color SELECTION menu:\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C White  @B2@W)@C Blue@n\r\n");
  write_to_output(d, "@B3@W)@C Red    @B4@W)@C Green@n\r\n");
  write_to_output(d, "@B5@W)@C Pink   @B6@W)@C Purple@n\r\n");
  write_to_output(d, "@B7@W)@C Yellow @B8@W)@C Black@n\r\n");
  write_to_output(d, "@B9@W)@C Orange@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
}

/* Question 1, after the caller's own introduction. */
static void ask_question1(struct descriptor_data *d)
{
  write_to_output(d, "@YAnswer the following question:\r\n");
  write_to_output(d, "@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Ask someone with more experience what to do.@n\r\n");
  write_to_output(d, "@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
  write_to_output(d, "@B3@W)@C Search for something magical to increase my strength.@n\r\n");
  write_to_output(d, "@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
  write_to_output(d, "@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q1);
}

/* rubbery_less picks between the two wordings the original menus used. */
static void show_mutation_menu(struct descriptor_data *d, const char *heading, bool rubbery_less)
{
  write_to_output(d, "%s", heading);
  write_to_output(d, "@D--------------------------------------------------------@n\n");
  write_to_output(d, "@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n", "%");
  write_to_output(d, "@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n", "%", "%");
  write_to_output(d, "@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
  write_to_output(d, "@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
  write_to_output(d, "@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
  write_to_output(d, "@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
  write_to_output(d, "@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
  if (rubbery_less)
    write_to_output(d, "@B 8@W) @CRubbery Body        @c-10%s less physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
  else
    write_to_output(d, "@B 8@W) @CRubbery Body        @c-10%s of physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
  write_to_output(d, "@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
  write_to_output(d, "@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n", "%");
}

static void show_genome_menu(struct descriptor_data *d, const char *heading)
{
  write_to_output(d, "%s", heading);
  write_to_output(d, "@D--------------------------------------------------------@n\n");
  write_to_output(d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
  write_to_output(d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
  write_to_output(d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
  write_to_output(d, "@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n", "%");
  write_to_output(d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
  write_to_output(d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
  write_to_output(d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
  write_to_output(d, "@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n", "%");
}

static const char *const mutation_names[] = {
    "Unselected",           /* 0 */
    "Extreme Speed",        /* 1 */
    "Increased Cell Regen", /* 2 */
    "Extreme Reflexes",     /* 3 */
    "Infravision",          /* 4 */
    "Natural Camo",         /* 5 */
    "Limb Regen",           /* 6 */
    "Poisonous",            /* 7 */
    "Rubbery Body",         /* 8 */
    "Innate Telepathy",     /* 9 */
    "Natural Energy"        /* 10 */
};

static const char *const genome_names[] = {
    "Unselected", /* 0 */
    "Human",      /* 1 */
    "Saiyan",     /* 2 */
    "Namek",      /* 3 */
    "Icer",       /* 4 */
    "Truffle",    /* 5 */
    "Arlian",     /* 6 */
    "Kai",        /* 7 */
    "Konatsu"     /* 8 */
};

static void show_chosen_genomes(struct descriptor_data *d)
{
  write_to_output(d, "@D----[@gGenome 1@W: @G%s@D]----[@gGenome 2@W: @G%s@D]----", genome_names[d->character->genome[0]], genome_names[d->character->genome[1]]);
}

/* The traits menu that follows skills, mutations or genomes. */
static void ask_bonuses(struct descriptor_data *d, const char *choose)
{
  display_bonus_menu(d->character, 0);
  write_to_output(d, "@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
  write_to_output(d, "%s", choose);
  desc_set_connected(d, CON_BONUS);
}

/* Leaving the traits menus: unspent creation points become practice sessions. */
static void finish_bonuses(struct descriptor_data *d)
{
  char_set_negcount(d->character, 0);
  write_to_output(d, "\r\n@wTo check the bonuses/negatives you have in game use the status command");
  if (char_get_ccpoints(d->character) > 0) {
    write_to_output(d, "\r\n@GYour left over points were spent on Practice Sessions@w");
    char_set_practice_points(d->character, char_get_practice_points(d->character) + ((100 * char_get_ccpoints(d->character))));
  }
  write_to_output(d, "\r\n*** PRESS RETURN: ");
  desc_set_connected(d, CON_QROLLSTATS);
}

static void show_rolled_stats(struct descriptor_data *d)
{
  write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                     "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                     "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
                  char_get_aff_abils_str(d->character), char_get_aff_abils_dex(d->character),
                  char_get_aff_abils_con(d->character), char_get_aff_abils_intel(d->character),
                  char_get_aff_abils_wis(d->character), char_get_aff_abils_cha(d->character));
}

/* Saves the finished character and shows the MOTD. */
static void finish_creation(struct descriptor_data *d)
{
  if (char_get_pfilepos(d->character) < 0)
    char_set_pfilepos(d->character, create_entry(char_get_pc_name(d->character)));
  /* Now GET_NAME() will work properly. */
  init_char(d->character);
  save_char(d->character);
  save_player_index();
  write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
  desc_set_connected(d, CON_RMOTD);
  mudlog(LOG_CMP, ADMLVL_GOD, TRUE, "New player: %s [%s %s]",
         char_get_name(d->character), char_true_race_name(d->character),
         char_sensei_name(d->character));
}

/* ---- Appearance ------------------------------------------------------- */

/* The hair length question, or its equivalent for the race (or straight to skin if there is none). */
static void ask_hair_length(struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  if (has_hair(ch)) {
    show_length_menu(d, "Hair", "Bald");
    desc_set_connected(d, CON_HAIRL);
  } else if (char_is_demon(ch) || char_is_icer(ch)) {
    show_length_menu(d, "Horn", "None");
    desc_set_connected(d, CON_HAIRL);
  } else if (char_is_majin(ch)) {
    show_length_menu(d, "Forelock", "Tiny");
    desc_set_connected(d, CON_HAIRL);
  } else if (char_is_namek(ch) || char_is_arlian(ch)) {
    show_length_menu(d, "Antenae", "Tiny");
    desc_set_connected(d, CON_HAIRL);
  } else {
    ask_skin(d);
  }
}

/* CON_QRACE / CON_RACE_HELP */
static void state_qrace(struct descriptor_data *d, char *arg)
{
  int chosen_race;

  if (!*arg || (strlen(arg) == 1 && toupper(*arg) != 'T')) {
    write_to_output(d, "\r\nThat's not a race.\r\nRace: ");
    return;
  }
  if (strlen(arg) == 1) { /* T: toggle between the selection and help menus */
    if (desc_get_connected(d) == CON_QRACE) {
      display_races_help(d);
      desc_set_connected(d, CON_RACE_HELP);
    } else {
      display_races(d);
      desc_set_connected(d, CON_QRACE);
    }
    return;
  }

  chosen_race = race_find(arg, char_get_sex(d->character), true);
  if (chosen_race == RACE_UNDEFINED) {
    write_to_output(d, "\r\nThat's not a race.\r\nRace: ");
    return;
  }
  if (desc_get_connected(d) == CON_RACE_HELP) {
    show_help(d, race_name(chosen_race));
    return;
  }
  if (race_rpp_cost(chosen_race)) {
    write_to_output(d, "\r\n%i RPP will be paid upon your first level up.\r\n", race_rpp_cost(chosen_race));
  }
  d->character->race = chosen_race;

  if (char_is_halfbreed(d->character)) {
    write_to_output(d, "@YWhat race do you prefer to by identified with?\r\n");
    write_to_output(d, "@cThis controls how others first view you and whether you start with\na tail and how fast it regrows when missing.\r\n");
    write_to_output(d, "@D---------------------------------------@n\r\n");
    write_to_output(d, "@B1@W)@C Human\n@B2@W)@C Saiyan\r\n");
    write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    desc_set_connected(d, CON_RACIAL);
  } else if (char_is_android(d->character)) {
    write_to_output(d, "@YWhat do you want to be identified as at first glance?\r\n");
    write_to_output(d, "@D---------------------------------------@n\r\n");
    write_to_output(d, "@B1@W)@C Android\n@B2@W)@C Human\n@B3@W)@C Robotic Humanoid\r\n");
    write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    desc_set_connected(d, CON_RACIAL);
  } else if (char_is_namek(d->character)) {
    char_set_sex(d->character, SEX_NEUTRAL);
    ask_hair_length(d);
  } else {
    show_sex_prompt(d);
    desc_set_connected(d, CON_QSEX);
  }
}

/* CON_RACIAL */
static void state_racial(struct descriptor_data *d, char *arg)
{
  int choice = menu_choice(arg, char_is_halfbreed(d->character) ? "12" : "123");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->player_specials->racial_pref = choice + 1;
  show_sex_prompt(d);
  desc_set_connected(d, CON_QSEX);
}

/* CON_QSEX: query sex of new user (Nameks have none) */
static void state_qsex(struct descriptor_data *d, char *arg)
{
  static const int sexes[] = {SEX_MALE, SEX_FEMALE, SEX_NEUTRAL};
  struct char_data *ch     = d->character;

  if (!char_is_namek(ch)) {
    int choice = menu_choice(arg, "mfn");

    if (choice < 0) {
      write_to_output(d, "That is not a sex..\r\n"
                         "What IS your sex? ");
      return;
    }
    ch->sex = sexes[choice];
  }
  ask_hair_length(d);
}

/* CON_HAIRL: query hair length (or horns, forelock, antennae) */
static void state_hairl(struct descriptor_data *d, char *arg)
{
  static const int lengths[] = {HAIRL_BALD, HAIRL_SHORT, HAIRL_MEDIUM, HAIRL_LONG, HAIRL_RLONG};
  struct char_data *ch       = d->character;
  int choice                 = menu_choice(arg, "12345");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  ch->hairl = lengths[choice];

  if (has_hair(ch)) {
    if (ch->hairl == HAIRL_BALD) {
      ch->hairc = HAIRC_NONE;
      ch->hairs = HAIRS_NONE;
      ask_skin(d);
    } else {
      show_color_menu(d, "Hair");
      desc_set_connected(d, CON_HAIRC);
    }
  } else if ((char_is_arlian(ch) && char_is_female(ch))) {
    show_color_menu(d, "Wing");
    desc_set_connected(d, CON_HAIRC);
  } else {
    ch->hairc = HAIRC_NONE;
    ch->hairs = HAIRS_NONE;
    if (char_is_demon(ch) || char_is_icer(ch) || char_is_majin(ch) || char_is_namek(ch) || char_is_arlian(ch))
      show_skin_menu(d);
    desc_set_connected(d, CON_SKIN);
  }
}

/* CON_HAIRC: query hair color (wing color for Arlians) */
static void state_hairc(struct descriptor_data *d, char *arg)
{
  static const int colors[] = {HAIRC_BLACK, HAIRC_BROWN, HAIRC_BLONDE, HAIRC_GREY, HAIRC_RED, HAIRC_ORANGE, HAIRC_GREEN,
                               HAIRC_BLUE, HAIRC_PINK, HAIRC_PURPLE, HAIRC_SILVER, HAIRC_CRIMSON, HAIRC_WHITE};
  int choice                = menu_choice(arg, "123456789abcd");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->hairc = colors[choice];
  if (char_is_arlian(d->character)) {
    d->character->hairs = HAIRS_NONE;
    ask_skin(d);
  } else {
    show_hair_style_menu(d);
    desc_set_connected(d, CON_HAIRS);
  }
}

/* CON_HAIRS */
static void state_hairs(struct descriptor_data *d, char *arg)
{
  static const int styles[] = {HAIRS_PLAIN, HAIRS_MOHAWK, HAIRS_SPIKY, HAIRS_CURLY, HAIRS_UNEVEN, HAIRS_PONYTAIL,
                               HAIRS_AFRO, HAIRS_FADE, HAIRS_CREW, HAIRS_FEATHERED, HAIRS_DRED};
  int choice                = menu_choice(arg, "123456789ab");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->hairs = styles[choice];
  ask_skin(d);
}

/* CON_SKIN */
static void state_skin(struct descriptor_data *d, char *arg)
{
  static const int skins[] = {SKIN_WHITE, SKIN_BLACK, SKIN_GREEN, SKIN_ORANGE, SKIN_YELLOW, SKIN_RED,
                              SKIN_GREY, SKIN_BLUE, SKIN_AQUA, SKIN_PINK, SKIN_PURPLE, SKIN_TAN};
  int choice               = menu_choice(arg, "123456789abc");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->skin = skins[choice];
  show_eye_menu(d);
  desc_set_connected(d, CON_EYE);
}

/* CON_EYE */
static void state_eye(struct descriptor_data *d, char *arg)
{
  static const int eyes[] = {EYE_BLUE, EYE_BLACK, EYE_GREEN, EYE_BROWN, EYE_RED, EYE_AQUA,
                             EYE_PINK, EYE_PURPLE, EYE_CRIMSON, EYE_GOLD, EYE_AMBER, EYE_EMERALD};
  int choice              = menu_choice(arg, "123456789abc");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->eye = eyes[choice];
  show_feature_menu(d);
  desc_set_connected(d, CON_DISTFEA);
}

/* CON_DISTFEA */
static void state_distfea(struct descriptor_data *d, char *arg)
{
  int choice = menu_choice(arg, "12345");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->distfea = choice;
  show_size_menu(d);
  desc_set_connected(d, CON_HW);
}

/* CON_HW */
static void state_hw(struct descriptor_data *d, char *arg)
{
  int count;
  const struct size_range *sizes = size_ranges(d->character, &count);
  int choice                     = menu_choice(arg, "123456");

  if (choice < 0 || choice >= count) {
    reject_option(d);
    return;
  }
  d->character->height = rand_number(sizes[choice].min_height, sizes[choice].max_height);
  d->character->weight = rand_number(sizes[choice].min_weight, sizes[choice].max_weight);
  show_aura_menu(d);
  desc_set_connected(d, CON_AURA);
}

/* CON_AURA */
static void state_aura(struct descriptor_data *d, char *arg)
{
  int choice = menu_choice(arg, "123456789");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->aura = choice;
  display_classes(d);
  desc_set_connected(d, CON_QCLASS);
}

/* CON_QCLASS / CON_CLASS_HELP */
static void state_qclass(struct descriptor_data *d, char *arg)
{
  int chosen_sensei;

  if (strlen(arg) == 1) {
    if (toupper(*arg) != 'T') {
      write_to_output(d, "\r\nThat's not a sensei.\r\nSensei: ");
    } else if (desc_get_connected(d) == CON_CLASS_HELP) { /* T: toggle between the selection and help menus */
      desc_set_connected(d, CON_QCLASS);
      display_classes(d);
    } else {
      display_classes_help(d);
      desc_set_connected(d, CON_CLASS_HELP);
    }
    return;
  }

  chosen_sensei = sensei_find(arg);
  if (chosen_sensei == CLASS_UNDEFINED) {
    write_to_output(d, "\r\nThat's not a sensei.\r\nSensei: ");
    return;
  }
  if (desc_get_connected(d) == CON_CLASS_HELP) {
    show_help(d, sensei_name(chosen_sensei));
    return;
  }
  /* only the senseis the menu offers this race */
  if (!sensei_is_pc_ok(chosen_sensei) || !sensei_available_for_race(chosen_sensei, d->character->race)) {
    write_to_output(d, "\r\nThat's not a sensei.\r\nSensei: ");
    return;
  }
  d->character->chclass = chosen_sensei;

  if (char_is_android(d->character)) {
    write_to_output(d, "\r\n@YChoose your model type.\r\n");
    write_to_output(d, "@D---------------------------------------@n\r\n");
    write_to_output(d, "@B1@W)@C Absorbtion Model@n\r\n");
    write_to_output(d, "@B2@W)@C Repair Model@n\r\n");
    write_to_output(d, "@B3@W)@C Sense, Powersense Model@n\r\n");
    write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    desc_set_connected(d, CON_ANDROID);
  } else {
    write_to_output(d, "\r\n@RAnswer The following questions carefully, they construct your alignment\r\nand affect your stats.\r\n\r\n");
    write_to_output(d, "@WQuestion (@G1@W out of @g10@W)\r\n");
    ask_question1(d);
  }
}

/* CON_ANDROID */
static void state_android(struct descriptor_data *d, char *arg)
{
  switch (*arg) {
    case '1':
      char_plr_set(d->character, PLR_ABSORB, true);
      write_to_output(d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your liking.\r\n\r\n");
      write_to_output(d, "\r\n@WQuestion (@G1@W out of @g10@W)");
      break;
    case '2':
      char_plr_set(d->character, PLR_REPAIR, true);
      write_to_output(d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your linking.\r\n\r\n");
      break;
    case '3':
      char_plr_set(d->character, PLR_SENSEM, true);
      write_to_output(d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer or your stats contrary to your liking.\r\n\r\n");
      break;
    default:
      write_to_output(d, "@wThat is not a correct selection, try again.@n\r\n");
      return;
  }
  ask_question1(d);
}

/* ---- Questionnaire ---------------------------------------------------- */

/* Starting ability ranges by race, in the order they are rolled. */
static const struct {
  bool (*is_race)(const struct char_data *ch);
  int str[2], con[2], wis[2], intel[2], cha[2], dex[2];
} starting_abilities[] = {
    {char_is_saiyan,    {12, 18}, {12, 18}, {8, 16},  {8, 14},  {8, 18},  {8, 16} },
    {char_is_halfbreed, {10, 18}, {10, 18}, {8, 18},  {8, 18},  {8, 18},  {8, 18} },
    {char_is_human,     {8, 18},  {8, 18},  {10, 18}, {12, 18}, {8, 18},  {8, 18} },
    {char_is_hoshijin,  {10, 18}, {9, 18},  {9, 18},  {9, 18},  {10, 18}, {9, 18} },
    {char_is_namek,     {9, 18},  {9, 18},  {12, 18}, {8, 18},  {8, 18},  {8, 18} },
    {char_is_arlian,    {15, 20}, {15, 20}, {8, 16},  {8, 16},  {8, 18},  {8, 18} },
    {char_is_android,   {12, 18}, {8, 18},  {8, 16},  {8, 16},  {8, 18},  {8, 18} },
    {char_is_bio,       {14, 18}, {8, 18},  {8, 18},  {8, 18},  {8, 18},  {8, 14} },
    {char_is_majin,     {11, 18}, {14, 18}, {8, 14},  {8, 14},  {8, 18},  {8, 17} },
    {char_is_truffle,   {8, 14},  {8, 14},  {8, 18},  {14, 18}, {8, 18},  {8, 18} },
    {char_is_kai,       {9, 18},  {8, 18},  {14, 18}, {10, 18}, {8, 18},  {8, 18} },
    {char_is_icer,      {10, 18}, {12, 18}, {8, 18},  {8, 18},  {8, 15},  {8, 18} },
    {char_is_mutant,    {9, 18},  {9, 18},  {9, 18},  {9, 18},  {9, 18},  {9, 18} },
    {char_is_kanassan,  {8, 16},  {8, 16},  {12, 18}, {12, 18}, {8, 18},  {8, 18} },
    {char_is_demon,     {11, 18}, {8, 18},  {10, 18}, {10, 18}, {8, 18},  {8, 18} },
    {char_is_konatsu,   {10, 14}, {10, 14}, {10, 16}, {10, 14}, {12, 18}, {14, 18}},
};

static void roll_starting_abilities(struct char_data *ch)
{
  for (size_t i = 0; i < sizeof(starting_abilities) / sizeof(starting_abilities[0]); i++) {
    if (!starting_abilities[i].is_race(ch))
      continue;
    ch->real_abils.str   = rand_number(starting_abilities[i].str[0], starting_abilities[i].str[1]);
    ch->real_abils.con   = rand_number(starting_abilities[i].con[0], starting_abilities[i].con[1]);
    ch->real_abils.wis   = rand_number(starting_abilities[i].wis[0], starting_abilities[i].wis[1]);
    ch->real_abils.intel = rand_number(starting_abilities[i].intel[0], starting_abilities[i].intel[1]);
    ch->real_abils.cha   = rand_number(starting_abilities[i].cha[0], starting_abilities[i].cha[1]);
    ch->real_abils.dex   = rand_number(starting_abilities[i].dex[0], starting_abilities[i].dex[1]);
    return;
  }
}

/* CON_Q1 */
static void state_q1(struct descriptor_data *d, char *arg)
{
  d->character->basepl = rand_number(30, 50);
  d->character->basest = rand_number(30, 50);
  d->character->baseki = rand_number(30, 50);
  roll_starting_abilities(d->character);

  switch (*arg) {
    case '1':
      d->character->basepl += roll_stats(d->character, 5, 25);
      d->character->basest += roll_stats(d->character, 8, 50);
      d->character->baseki += roll_stats(d->character, 6, 50);
      break;
    case '2':
      d->character->basepl += roll_stats(d->character, 5, 55);
      d->character->basest += roll_stats(d->character, 8, 40);
      d->character->baseki += roll_stats(d->character, 6, 40);
      break;
    case '3':
      d->character->basepl += roll_stats(d->character, 5, 125);
      d->character->basest += roll_stats(d->character, 8, 50);
      d->character->baseki += roll_stats(d->character, 6, 40);
      break;
    case '4':
      d->character->basepl += roll_stats(d->character, 5, 65);
      d->character->basest += roll_stats(d->character, 8, 65);
      d->character->baseki += roll_stats(d->character, 6, 65);
      char_plr_set(d->character, PLR_SKILLP, true);
      break;
    case '5':
      d->character->basepl += roll_stats(d->character, 5, 75);
      d->character->basest += roll_stats(d->character, 8, 100);
      d->character->baseki += roll_stats(d->character, 6, 75);
      break;
    default:
      reject_option(d);
      return;
  }
  write_to_output(d, "\r\n@WQuestion (@G2@W out of @g10@W)\r\n");
  write_to_output(d, "@YAnswer the following question:\r\n");
  write_to_output(d, "@wYou are faced with the strongest opponent you have ever\r\nfaced in your life. You both have beat each other to the\r\nlimits of both your strengths. A situation has presented \r\nan opportunity to win the fight, what do you do?\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Kill my opponent in a very brutal fashion!@n\r\n");
  write_to_output(d, "@B2@W)@C Disable my opponent, but spare their life.@n\r\n");
  write_to_output(d, "@B3@W)@C Kill my opponent, I have no other choice.@n\r\n");
  write_to_output(d, "@B4@W)@C Try to evade my opponent till I can get away.@n\r\n");
  write_to_output(d, "@B5@W)@C Take their head clean off and bathe in their blood!@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q2);
}

/* CON_Q2 */
static void state_q2(struct descriptor_data *d, char *arg)
{
  static const int alignment[] = {-200, 100, 10, 0, -400};
  int choice                   = menu_choice(arg, "12345");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->alignment += alignment[choice];
  write_to_output(d, "\r\n@WQuestion (@G3@W out of @g10@W)\r\n");
  write_to_output(d, "@YAnswer the following question:\r\n");
  write_to_output(d, "@wYou are one day offered a means to gain incredible strength\r\nby some extraordinary means. The only problem is it requires\r\nthe lives of innocents to obtain. What do you do?\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Hell yeah I take the opportunity to get stronger!@n\r\n");
  write_to_output(d, "@B2@W)@C I refuse to gain unnatural strength.@n\r\n");
  write_to_output(d, "@B3@W)@C I refuse to gain strength at the cost of the innocent.@n\r\n");
  write_to_output(d, "@B4@W)@C I kill that many innocents before breakfast anyway...@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q3);
}

/* CON_Q3 */
static void state_q3(struct descriptor_data *d, char *arg)
{
  static const struct {
    int alignment, pl, st, ki;
  } answers[] = {
      {-100, 100, 80,  10 },
      {10,   25,  25,  25 },
      {50,   20,  20,  20 },
      {-200, 100, 100, 100},
  };
  int choice = menu_choice(arg, "1234");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->alignment += answers[choice].alignment;
  d->character->basepl += answers[choice].pl;
  d->character->basest += answers[choice].st;
  d->character->baseki += answers[choice].ki;
  write_to_output(d, "\r\n@WQuestion (@G4@W out of @g10@W)\r\n");
  write_to_output(d, "@YAnswer the following question:\r\n");
  write_to_output(d, "@wOne day you are offered a way to make a lot of money, but in order\r\nto do so you will need to stop training for a whole month to\r\nhandle business. What do you do?\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C I take the opportunity, with more money I can train better later.@n\r\n");
  write_to_output(d, "@B2@W)@C I refuse to waste my time. What I need is some nice hard training.@n\r\n");
  write_to_output(d, "@B3@W)@C Hmm. With more money I can live better, certainly that is worth the time.@n\r\n");
  write_to_output(d, "@B4@W)@C I choose to earn a little money while still training instead.@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q4);
}

/* CON_Q4 */
static void state_q4(struct descriptor_data *d, char *arg)
{
  switch (*arg) {
    case '1':
      d->character->gold += 1000;
      d->character->basepl -= rand_number(10, 30);
      d->character->basest -= rand_number(10, 30);
      d->character->baseki -= rand_number(10, 30);
      break;
    case '2':
      d->character->gold = 0;
      d->character->basepl += rand_number(50, 165);
      d->character->basest += rand_number(50, 165);
      d->character->baseki += rand_number(50, 165);
      break;
    case '3':
      d->character->gold = 2500;
      d->character->basepl -= rand_number(15, 25);
      d->character->basest -= rand_number(15, 25);
      d->character->baseki -= rand_number(15, 25);
      break;
    case '4':
      d->character->gold = 150;
      d->character->basepl += rand_number(25, 80);
      d->character->basest += rand_number(25, 80);
      d->character->baseki += rand_number(25, 80);
      break;
    default:
      reject_option(d);
      return;
  }
  write_to_output(d, "\r\n@WQuestion (@G5@W out of @g10@W)\r\n");
  write_to_output(d, "@YAnswer the following question:\r\n");
  write_to_output(d, "@wYou are introduced to a new way of training one day, what do you do?\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C I prefer my way, it has worked so far.@n\r\n");
  write_to_output(d, "@B2@W)@C I am open to new possibilites, sure.@n\r\n");
  write_to_output(d, "@B3@W)@C I will at least try it, for a little while.@n\r\n");
  write_to_output(d, "@B4@W)@C No way is superior to eating spinach everyday...@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q5);
}

/* CON_Q5 */
static void state_q5(struct descriptor_data *d, char *arg)
{
  static const int pl_range[][2] = {
      {0,   40},
      {-30, 80},
      {-25, 60},
      {0,   50}
  };
  int choice = menu_choice(arg, "1234");

  if (choice < 0) {
    reject_option(d);
    return;
  }
  d->character->basepl += rand_number(pl_range[choice][0], pl_range[choice][1]);
  write_to_output(d, "\r\n@WQuestion (@G6@W out of @g10@W)\r\n");
  write_to_output(d, "\r\n@YAnswer the following question:\r\n");
  write_to_output(d, "@wYou have an enemy before you, what is your prefered method to attack him?@n\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C I prefer to defend rather than attack.@n\r\n");
  write_to_output(d, "@B2@W)@C I prefer to throw a strong punch at the enemy's throat.@n\r\n");
  write_to_output(d, "@B3@W)@C I prefer to send a devestating kick at the enemy's neck.@n\r\n");
  write_to_output(d, "@B4@W)@C I prefer to smash them with a two-handed slam!@n\r\n");
  write_to_output(d, "@B5@W)@C I prefer to throw one of my many energy attacks!@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q6);
}

/* CON_Q6 */
static void state_q6(struct descriptor_data *d, char *arg)
{
  switch (*arg) {
    case '1':
      d->character->basepl += rand_number(0, 15);
      d->character->basest += rand_number(0, 15);
      d->character->choice = 1;
      break;
    case '2':
      d->character->basepl += rand_number(0, 30);
      d->character->choice = 2;
      break;
    case '3':
      d->character->basepl += rand_number(0, 30);
      d->character->choice = 3;
      break;
    case '4':
      d->character->basepl += rand_number(0, 30);
      d->character->choice = 4;
      break;
    case '5':
      d->character->baseki += rand_number(0, 50);
      d->character->choice = 5;
      break;
    default:
      reject_option(d);
      return;
  }
  write_to_output(d, "\r\n@WQuestion (@G7@W out of @g10@W)\r\n");
  write_to_output(d, "\r\n@YAnswer the following question:\r\n");
  write_to_output(d, "@wYou are camped out one night in a field, the sky is clear and the\r\nstars visible. Looking at them, what thought crosses your mind?@n\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C One day, I am going to conquer every one of those.@n\r\n");
  write_to_output(d, "@B2@W)@C I really wish I had brought that special someone along as this is a great night for romance@n\r\n");
  write_to_output(d, "@B3@W)@C Those stars hold greater meaning to you than just planets, they dictate\r\nyour life or guide you and those arround you in one form or another.@n\r\n");
  write_to_output(d, "@B4@W)@C You'd very much like to travel into space to get to one of these stars to study all that it holds.@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q7);
}

/* CON_Q7 */
static void state_q7(struct descriptor_data *d, char *arg)
{
  switch (*arg) {
    case '1':
      d->character->alignment += -10;
      d->character->real_abils.str += 1;
      break;
    case '2':
      d->character->alignment += +10;
      d->character->real_abils.cha += 1;
      break;
    case '3':
      d->character->real_abils.wis += 1;
      break;
    case '4':
      d->character->real_abils.intel += 1;
      break;
    default:
      reject_option(d);
      return;
  }
  write_to_output(d, "\r\n@WQuestion (@G8@W out of @g10@W)\r\n");
  write_to_output(d, "\r\n@YAnswer the following question:\r\n");
  write_to_output(d, "@wOne day, you are on the way home. You happen to walk past two\r\nof your best friends about to lock horns. Do you@n\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C You try to talk them out of it. With a silver tongue, you can stop the fight before it has began.@n\r\n");
  write_to_output(d, "@B2@W)@C It's clear that they are quite chuffed about something, you could stop the fight now but they may just try again later. It's best to investigate and find out the source of the problem.@n\r\n");
  write_to_output(d, "@B3@W)@C Pick the one you like the most and help them to win.@n\r\n");
  write_to_output(d, "@B4@W)@C You try to sneak past them, reckoning no matter what you do there its a no win situation, so long as they don't see you.@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q8);
}

/* CON_Q8 */
static void state_q8(struct descriptor_data *d, char *arg)
{
  switch (*arg) {
    case '1':
      d->character->alignment += +10;
      d->character->real_abils.cha += 1;
      break;
    case '2':
      d->character->alignment += +20;
      d->character->real_abils.wis += 1;
      break;
    case '3':
      d->character->alignment += -10;
      d->character->real_abils.str += 1;
      break;
    case '4':
      d->character->alignment += -20;
      d->character->real_abils.dex += 1;
      break;
    default:
      reject_option(d);
      return;
  }
  write_to_output(d, "\r\n@WQuestion (@G9@W out of @g10@W)\r\n");
  write_to_output(d, "\r\n@YAnswer the following question:\r\n");
  write_to_output(d, "@wAs a kid you were confronted with this. On the way from the bakery,\r\na group of kids corner you in an alley and the leader demands that\r\nyou surrender your jam donut to him. Do you...@n\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C Throw the donut up in the air, hoping that the leader will pay enough attention to it so that you can get at least one good shot in.@n\r\n");
  write_to_output(d, "@B2@W)@C Surrender the donut for now but come back later with your friends and gain your revenge.@n\r\n");
  write_to_output(d, "@B3@W)@C Give him the donut, then return to the baker. You tell him a sob story and convince him to give you a replacement donut.@n\r\n");
  write_to_output(d, "@B4@W)@C It's better just to do as he says and leave it to that. After all its only a donut.@n\r\n");
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_Q9);
}

/* Races that live long enough to start very old. */
static bool long_lived(const struct char_data *ch)
{
  return char_is_kai(ch) || char_is_demon(ch) || char_is_majin(ch);
}

/* CON_Q9 */
static void state_q9(struct descriptor_data *d, char *arg)
{
  switch (*arg) {
    case '1':
      d->character->real_abils.str += 1;
      break;
    case '2':
      d->character->alignment += -30;
      d->character->real_abils.wis += 1;
      break;
    case '3':
      d->character->alignment += -10;
      d->character->real_abils.cha += 1;
      break;
    case '4':
      d->character->alignment += -5;
      d->character->real_abils.intel += 1;
      break;
    default:
      reject_option(d);
      return;
  }
  write_to_output(d, "\r\n@WQuestion (@G10@W out of @g10@W)\r\n");
  write_to_output(d, "\r\n@YAnswer the following question:\r\n");
  write_to_output(d, "@wWhat do you wish your starting age to be?@n\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@B1@W)@C  8@n    @B2@W)@C 10@n\r\n");
  write_to_output(d, "@B3@W)@C 12@n    @B4@W)@C 14@n\r\n");
  write_to_output(d, "@B5@W)@C 16@n    @B6@W)@C 18@n\r\n");
  write_to_output(d, "@B7@W)@C 20@n    @B8@W)@C 22@n\r\n");
  write_to_output(d, "@B9@W)@C 24@n    @BA@W)@C 26@n\r\n");
  write_to_output(d, "@BB@W)@C 28@n    @BC@W)@C 30@n\r\n");
  write_to_output(d, "@BD@W)@C 40@n    @BE@W)@C 50@n\r\n");
  write_to_output(d, "@BF@W)@C 60@n    @BG@W)@C 65@n\r\n");
  if (long_lived(d->character)) {
    write_to_output(d, "@BH@W)@C 500@n   @BI@W)@C 800@n\r\n");
  } else if (char_is_namek(d->character)) {
    write_to_output(d, "@BH@W)@C 250@n   @BI@W)@C 400@n\r\n");
  } else {
    write_to_output(d, "@BH@W)@C 70@n    @BI@W)@C 75@n\r\n");
  }
  write_to_output(d, "@w\r\nMake a selection:@n\r\n");
  desc_set_connected(d, CON_QX);
}

/* CON_QX: starting age */
static void state_qx(struct descriptor_data *d, char *arg)
{
  /* the last two depend on the race: long-lived, Namek, everyone else */
  static const int ages[]         = {8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 40, 50, 60, 65};
  static const int old_ages[3][2] = {
      {500, 800},
      {250, 400},
      {70,  75 }
  };
  int choice = menu_choice(arg, "123456789abcdefghi");
  int age;

  if (choice < 0) {
    reject_option(d);
    return;
  }
  if (choice < (int)(sizeof(ages) / sizeof(ages[0])))
    age = ages[choice];
  else
    age = old_ages[long_lived(d->character) ? 0 : char_is_namek(d->character) ? 1
                                                                              : 2][choice - 16];
  d->character->time.birth = time(0) - (age * SECS_PER_MUD_YEAR);

  char_set_ccpoints(d->character, char_is_hoshijin(d->character) ? 10 : 5);
  write_to_output(d, "@C             Alignment Menu@n\r\n");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@cCurrent Alignment@D: %s%s@n\r\n", char_get_alignment(d->character) < -50 ? "@R" : (char_get_alignment(d->character) > 50 ? "@C" : "@G"), disp_align(d->character));
  write_to_output(d, "@YThis is the alignment your character has based on your choices.\r\n");
  write_to_output(d, "Choose to keep this alignment with no penalty, or choose new\r\n");
  write_to_output(d, "alignment and suffer a -5%s PL and -1 stat (random) penalty.\r\n@n", "%");
  write_to_output(d, "@D---------------------------------------@n\r\n");
  write_to_output(d, "@BK@W) @wKeep This Alignment@n\r\n");
  write_to_output(d, "@B1@W) @wSaintly@n\r\n");
  write_to_output(d, "@B2@W) @wValiant@n\r\n");
  write_to_output(d, "@B3@W) @wHero@n\r\n");
  write_to_output(d, "@B4@W) @wDo-gooder@n\r\n");
  write_to_output(d, "@B5@W) @wNeutral\r\n");
  write_to_output(d, "@B6@W) @wCrook@n\r\n");
  write_to_output(d, "@B7@W) @wVillain@n\r\n");
  write_to_output(d, "@B8@W) @wTerrible@n\r\n");
  write_to_output(d, "@B9@W) @wHorribly Evil@n\r\n");
  write_to_output(d, "Choose: \r\n");
  desc_set_connected(d, CON_ALIGN);
}

/* CON_ALIGN: keep the questionnaire's alignment, or pick one at a cost */
static void state_align(struct descriptor_data *d, char *arg)
{
  static const int alignments[] = {1000, 799, 599, 299, 0, -299, -599, -799, -1000};
  int choice                    = menu_choice(arg, "k123456789");

  if (choice < 0) {
    write_to_output(d, "That is not an acceptable option! Choose again...\r\n");
    return;
  }
  if (choice == 0) {
    write_to_output(d, "@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you will receive 200 PS in exchange.@n\r\n");
  } else {
    char_set_alignment(d->character, alignments[choice - 1]);
    loseBasePLPercent(d->character, .2, false);
    switch (rand_number(1, 6)) { /* and lose a random point of one ability */
      case 1:
        d->character->real_abils.str -= 1;
        break;
      case 2:
        d->character->real_abils.con -= 1;
        break;
      case 3:
        d->character->real_abils.wis -= 1;
        break;
      case 4:
        d->character->real_abils.intel -= 1;
        break;
      case 5:
        d->character->real_abils.cha -= 1;
        break;
      case 6:
        d->character->real_abils.dex -= 1;
        break;
    }
    write_to_output(d, "@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you get 200 PS in exchange.@n\r\n");
  }
  write_to_output(d, "keep or forget: \r\n");
  desc_set_connected(d, CON_SKILLS);
}

/* CON_SKILLS: keep the sensei/race skills, or forget them for 200 practice sessions */
static void state_skills(struct descriptor_data *d, char *arg)
{
  bool forget = !strcasecmp(arg, "forget");

  if (!forget && strcasecmp(arg, "keep")) {
    write_to_output(d, "keep or forget: \r\n");
    return;
  }
  if (forget) {
    char_set_practice_points(d->character, char_get_practice_points(d->character) + (200));
    char_plr_set(d->character, PLR_FORGET, true);
  }

  if (!char_is_bio(d->character) && !char_is_mutant(d->character)) {
    ask_bonuses(d, "@wChoose: ");
    return;
  }
  if (char_is_mutant(d->character))
    show_mutation_menu(d, "\n@RSelect a mutation. A second will be chosen automatically..\n", forget);
  else
    show_genome_menu(d, "\n@RSelect two genomes to be your primary DNA strains.\n");
  write_to_output(d, "@wChoose: ");
  d->character->genome[0] = 0;
  d->character->genome[1] = 0;
  desc_set_connected(d, CON_GENOME);
}

/* Mutants pick one mutation; the next input rolls the second, and the one after moves on. */
static void mutant_genome(struct descriptor_data *d, char *arg)
{
  struct char_data *ch = d->character;

  show_mutation_menu(d, "\n@RSelect a mutation. A second will be chosen automatically..\n", true);
  write_to_output(d, "\n@wChoose: ");

  if (ch->genome[0] > 0 && ch->genome[1] <= 0) {
    int num = rand_number(1, 8);
    while (num == ch->genome[0])
      num = rand_number(1, 8);
    if (num == 3) {
      ch->real_abils.dex += 10;
    }
    write_to_output(d, "@CRolling second mutation... Your second mutation is @D[@Y%s@D]@n\r\n", mutation_names[num]);
    ch->genome[1] = num;
  } else if (ch->genome[1] > 0) {
    ask_bonuses(d, "\n@wChoose: ");
  } else if (!*arg) {
    write_to_output(d, "That is not an acceptable choice!\r\n");
    write_to_output(d, "\n@wChoose: ");
  } else {
    int choice = atoi(arg);
    if (choice < 1 || choice > 10) {
      write_to_output(d, "That is not an acceptable choice!\r\n");
      return;
    }
    write_to_output(d, "@CYou have chosen the mutation @D[@Y%s@D]@n\r\n", mutation_names[choice]);
    ch->genome[0] = choice;
    if (choice == 3) {
      ch->real_abils.dex += 10;
    } else if (choice == 9) {
      char_set_skills_base(ch, SKILL_TELEPATHY, 50);
    }
  }
}

/* Bios pick two different genomes; the input after the second moves on. */
static void bio_genome(struct descriptor_data *d, char *arg)
{
  struct char_data *ch = d->character;
  int select;

  if (ch->genome[1] > 0) {
    ask_bonuses(d, "@wChoose: ");
    return;
  }
  if (!*arg) {
    show_genome_menu(d, "@RSelect two genomes to be your primary DNA strains.\n");
    show_chosen_genomes(d);
    write_to_output(d, "\n@wChoose: ");
    return;
  }
  select = atoi(arg);
  if (select > 8 || select < 1) {
    show_genome_menu(d, "@RSelect two genomes to be your primary DNA strains.\n");
    write_to_output(d, "@RThat is not an acceptable selection. @WTry again:@n\n");
    return;
  }
  if (ch->genome[0] == 0) {
    ch->genome[0] = select;
  } else if (ch->genome[0] == select) {
    write_to_output(d, "You can't choose the same thing for both genomes!\r\n");
    write_to_output(d, "\n@wChoose: ");
    return;
  } else {
    ch->genome[1] = select;
  }
  if (select == 7) {
    char_set_skills_base(ch, SKILL_TELEPATHY, 30);
    char_set_skills_base(ch, SKILL_FOCUS, 30);
  }
  show_genome_menu(d, "@RSelect two genomes to be your primary DNA strains.\n");
  show_chosen_genomes(d);
}

/* CON_GENOME */
static void state_genome(struct descriptor_data *d, char *arg)
{
  if (char_is_mutant(d->character))
    mutant_genome(d, arg);
  else
    bio_genome(d, arg);
}

/* CON_BONUS / CON_NEGATIVE: buy bonus traits, or take negative ones for points */
static void state_traits(struct descriptor_data *d, char *arg)
{
  bool negatives = desc_get_connected(d) == CON_NEGATIVE;
  int menu       = negatives ? 1 : 0;
  int value;

  if (!*arg) {
    display_bonus_menu(d->character, menu);
    send_to_char(d->character, "@wChoose: ");
  } else if (!strcasecmp(arg, negatives ? "n" : "b")) {
    display_bonus_menu(d->character, menu);
    send_to_char(d->character, "@RYou are already in that menu.\r\n");
    send_to_char(d->character, "@wChoose: ");
  } else if (!strcasecmp(arg, negatives ? "b" : "n")) {
    display_bonus_menu(d->character, !menu);
    send_to_char(d->character, "@wChoose: ");
    desc_set_connected(d, negatives ? CON_BONUS : CON_NEGATIVE);
  } else if (!strcasecmp(arg, "x")) {
    if (negatives) { /* negatives can take base stats to zero or below */
      if (d->character->basepl <= 0) {
        d->character->basepl = 90;
      }
      if (d->character->baseki <= 0) {
        d->character->baseki = 90;
      }
      if (d->character->basest <= 0) {
        d->character->basest = 90;
      }
    }
    finish_bonuses(d);
  } else if ((value = parse_bonuses(arg)) == -1) {
    display_bonus_menu(d->character, menu);
    send_to_char(d->character, "@RThat is not an option.\r\n");
    send_to_char(d->character, "@wChoose: ");
  } else if (value != 1337) {
    exchange_ccpoints(d->character, negatives ? value + 15 : value);
    send_to_char(d->character, "@wChoose: ");
  } else {
    display_bonus_menu(d->character, menu);
    send_to_char(d->character, "@wChoose: ");
  }
}

/* CON_QROLLSTATS: the configured way of setting base stats, then the end of creation */
static void state_qrollstats(struct descriptor_data *d, char *arg)
{
  if (CONFIG_REROLL_PLAYER_CREATION &&
      (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_1)) {
    if (toupper(*arg) != 'Y') {
      cedit_creation(d->character);
      show_rolled_stats(d);
      write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
      return;
    }
  } else if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2 ||
             CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3) {
    if (CONFIG_REROLL_PLAYER_CREATION &&
        (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2)) {
      if (toupper(*arg) != 'Y') {
        cedit_creation(d->character);
        show_rolled_stats(d);
        write_to_output(d, "Initial statistics, you may reassign individual numbers\r\n");
        write_to_output(d, "between statistics after choosing yes.\r\n");
        write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
        return;
      }
    } else
      cedit_creation(d->character);
    if (!d->olc) {
      CREATE(d->olc, struct oasis_olc_data, 1);
    }
    if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3)
      OLC_VAL(d) = CONFIG_INITIAL_POINTS_POOL;
    else
      OLC_VAL(d) = 0;

    desc_set_connected(d, CON_QSTATS);
    stats_disp_menu(d);
    return;
  } else {
    cedit_creation(d->character);
  }
  finish_creation(d);
}

/* CON_QSTATS: assigning points to stats (creation methods 2 and 3) */
static void state_qstats(struct descriptor_data *d, char *arg)
{
  int total;

  if (!parse_stats(d, arg))
    return;
  if (d->olc) {
    free(d->olc);
    d->olc = NULL;
  }
  finish_creation(d);
  d->character->rp = d->rpp;
  total            = char_get_aff_abils_str(d->character) / 2 + char_get_aff_abils_con(d->character) / 2 +
          char_get_aff_abils_wis(d->character) / 2 + char_get_aff_abils_intel(d->character) / 2 +
          char_get_aff_abils_dex(d->character) / 2 + char_get_aff_abils_cha(d->character) / 2;
  total -= 30;
  mudlog(LOG_CMP, ADMLVL_GOD, TRUE, "Str: %2d Dex: %2d Con: %2d Int: %2d "
                                    "Wis:  %2d Cha: %2d mod total: %2d",
         char_get_aff_abils_str(d->character),
         char_get_aff_abils_dex(d->character), char_get_aff_abils_con(d->character), char_get_aff_abils_intel(d->character),
         char_get_aff_abils_wis(d->character), char_get_aff_abils_cha(d->character), total);
}

static const struct nanny_state chargen_states[] = {
    {CON_QSEX,       state_qsex      },
    {CON_QRACE,      state_qrace     },
    {CON_RACE_HELP,  state_qrace     },
    {CON_RACIAL,     state_racial    },
    {CON_HAIRL,      state_hairl     },
    {CON_HAIRC,      state_hairc     },
    {CON_HAIRS,      state_hairs     },
    {CON_SKIN,       state_skin      },
    {CON_EYE,        state_eye       },
    {CON_DISTFEA,    state_distfea   },
    {CON_HW,         state_hw        },
    {CON_AURA,       state_aura      },
    {CON_Q1,         state_q1        },
    {CON_Q2,         state_q2        },
    {CON_Q3,         state_q3        },
    {CON_Q4,         state_q4        },
    {CON_Q5,         state_q5        },
    {CON_Q6,         state_q6        },
    {CON_Q7,         state_q7        },
    {CON_Q8,         state_q8        },
    {CON_Q9,         state_q9        },
    {CON_QX,         state_qx        },
    {CON_ALIGN,      state_align     },
    {CON_SKILLS,     state_skills    },
    {CON_GENOME,     state_genome    },
    {CON_BONUS,      state_traits    },
    {CON_NEGATIVE,   state_traits    },
    {CON_QCLASS,     state_qclass    },
    {CON_CLASS_HELP, state_qclass    },
    {CON_ANDROID,    state_android   },
    {CON_QROLLSTATS, state_qrollstats},
    {CON_QSTATS,     state_qstats    },
};

bool chargen_parse(struct descriptor_data *d, char *arg)
{
  for (size_t i = 0; i < sizeof(chargen_states) / sizeof(chargen_states[0]); i++)
    if (desc_get_connected(d) == chargen_states[i].state) {
      chargen_states[i].handler(d, arg);
      return true;
    }
  return false;
}
