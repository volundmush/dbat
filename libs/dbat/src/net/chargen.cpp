/* ************************************************************************
*   File: chargen.c                                     Part of CircleMUD *
*  Usage: character creation                                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <array>
#include <algorithm>
#include <cstdlib>


#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/statedit.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/consts/create.hpp"

#include "nanny_internal.hpp"

static void display_races_sub(GameWorld *g, descriptor_data *d)
{
  int i = 0;
  for (int r = 0; r < NUM_RACES; r++) {
    if (!race_is_playable(r) || !race_valid_sex(r, char_get_sex(d->character)))
      continue;
    send_to_char(g, d->character, "@C{:<15}@D[@R{:d} RPP@D]@n{}", race_name(r), race_rpp_cost(r), !(++i % 2) ? "\r\n" : "   ");
  }
}

static void display_races(GameWorld *g, struct descriptor_data *d)
{
  send_to_char(g, d->character, "\r\n@YRace SELECTION menu:\r\n@D---------------------------------------\r\n@n");
  display_races_sub(g, d);

  send_to_char(g, d->character, "\n @BR@W) @CRandom Race Selection!\r\n@n");
  send_to_char(g, d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(g, d->character, "\n@WRace: @n");
}

static void display_classes_sub(GameWorld *g, descriptor_data *d)
{
  int i = 0;
  for (int s = 0; s < NUM_CLASSES; s++) {
    if (!sensei_is_pc_ok(s) || !sensei_available_for_race(s, char_get_race(d->character)))
      continue;
    send_to_char(g, d->character, "@C{}@n{}", sensei_name(s), !(++i % 2) ? "\r\n" : "	");
  }
}

static void display_classes(GameWorld *g, struct descriptor_data *d)
{
  send_to_char(g, d->character, "\r\n@YSensei SELECTION menu:\r\n@D--------------------------------------\r\n@n");
  display_classes_sub(g, d);

  send_to_char(g, d->character, "\n @BR@W) @CRandom Sensei Selection!\r\n@n");
  send_to_char(g, d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(g, d->character, "\n@WSensei: @n");
}

static void display_races_help(GameWorld *g, struct descriptor_data *d)
{
  send_to_char(g, d->character, "\r\n@YRace HELP menu:\r\n@G--------------------------------------------\r\n@n");
  display_races_sub(g, d);

  send_to_char(g, d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(g, d->character, "\n@WHelp on Race #: @n");
}

static void display_classes_help(GameWorld *g, struct descriptor_data *d)
{
  send_to_char(g, d->character, "\r\n@YClass HELP menu:\r\n@G-------------------------------------------\r\n@n");
  display_classes_sub(g, d);

  send_to_char(g, d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
  send_to_char(g, d->character, "\n@WHelp on Class #: @n");
}

/* ---- Traits: the bonuses and negatives -------------------------------- */

/* What stops a character taking a trait: another trait taken already, or being an android or a demon. */
enum class bar_kind { none, taken, android, demon };

struct trait_bar {
  bar_kind when{};
  int other{};          /* the trait, for taken */
  std::string_view says{}; /* what to say instead of the usual, if anything */
};

static constexpr trait_bar excludes(int other, std::string_view says = {}) { return {bar_kind::taken, other, says}; }
static constexpr trait_bar not_android(std::string_view says = {}) { return {bar_kind::android, 0, says}; }
static constexpr trait_bar not_demon(std::string_view says) { return {bar_kind::demon, 0, says}; }

struct trait {
  std::string_view name{}; /* as the status commands show it */
  std::string_view line{}; /* its line in the menu, the cost included */
  int cost{};          /* negative for a bonus (points spent), positive for a negative (points gained) */
  std::array<trait_bar, 2> bars{}; /* checked in order */
};

/* Indexed by BONUS_*. Most exclusions go both ways, each written at both ends. */
static const trait traits[] = {
    /* the bonuses: they cost points */
    {"Thrifty", "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost             @D[@G-2pts @D]",
     -2, {excludes(BONUS_IMPULSE)}}, /* BONUS_THRIFTY */
    {"Prodigy", "Prodigy     - +25% Experience Gained Until Level 80                  @D[@G-5pts @D]",
     -5}, /* BONUS_PRODIGY */
    {"Quick Study", "Quick Study - Character auto-trains skills faster                    @D[@G-3pts @D]",
     -3, {excludes(BONUS_SLOW_LEARNER)}}, /* BONUS_QUICK_STUDY */
    {"Die Hard", "Die Hard    - Life Force's PL regen doubled, but cost is the same    @D[@G-6pts @D]",
     -6, {not_android()}}, /* BONUS_DIEHARD */
    {"Brawler", "Brawler     - Physical attacks do 20% more damage                    @D[@G-4pts @D]",
     -4}, /* BONUS_BRAWLER */
    {"Destroyer", "Destroyer   - Damaged Rooms act as regen rooms for you               @D[@G-3pts @D]",
     -3}, /* BONUS_DESTROYER */
    {"Hard Worker", "Hard Worker - Physical rewards better + activity drains less stamina @D[@G-3pts @D]",
     -3, {excludes(BONUS_SLACKER)}}, /* BONUS_HARDWORKER */
    {"Healer", "Healer      - Heal/First-aid/Vigor/Repair restore +10%               @D[@G-3pts @D]",
     -3}, /* BONUS_HEALER */
    {"Loyal", "Loyal       - +20% Experience When Grouped As Follower               @D[@G-2pts @D]",
     -2, {excludes(BONUS_LONER)}}, /* BONUS_LOYAL */
    {"Brawny", "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%     @D[@G-5pts @D]",
     -5, {excludes(BONUS_WIMP)}}, /* BONUS_BRAWNY */
    {"Scholarly", "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75% @D[@G-5pts @D]",
     -5, {excludes(BONUS_DULL)}}, /* BONUS_SCHOLARLY */
    {"Sage", "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%       @D[@G-5pts @D]",
     -5, {excludes(BONUS_FOOLISH)}}, /* BONUS_SAGE */
    {"Agile", "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%      @D[@G-4pts @D]",
     -4, {excludes(BONUS_CLUMSY)}}, /* BONUS_AGILE */
    {"Quick", "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%        @D[@G-6pts @D]",
     -6, {excludes(BONUS_SLOW)}}, /* BONUS_QUICK */
    {"Sturdy", "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%       @D[@G-5pts @D]",
     -5, {excludes(BONUS_FRAIL)}}, /* BONUS_STURDY */
    {"Thick Skin", "Thick Skin  - -20% Physical and -10% ki dmg received                 @D[@G-5pts @D]",
     -5, {excludes(BONUS_THINSKIN)}}, /* BONUS_THICKSKIN */
    {"Recipe Int", "Recipe Int. - Food cooked by you lasts longer/heals better           @D[@G-2pts @D]",
     -2, {not_android("You are an android and can not suppress anyway.\n\n") /* quirk: Recipe Int. has nothing to suppress */}}, /* BONUS_RECIPE */
    {"Fireproof", "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn         @D[@G-4pts @D]",
     -4, {not_demon("As a demon you are already fireproof.\r\n"), excludes(BONUS_FIREPRONE)}}, /* BONUS_FIREPROOF */
    {"Powerhitter", "Powerhitter - 15% critical hits will be x4 instead of x2             @D[@G-4pts @D]",
     -4, {excludes(BONUS_SOFT)}}, /* BONUS_POWERHIT */
    {"Healthy", "Healthy     - 40% chance to recover from ill effects when sleeping   @D[@G-3pts @D]",
     -3, {excludes(BONUS_SICKLY)}}, /* BONUS_HEALTHY */
    {"Insomniac", "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis    @D[@G-2pts @D]",
     -2, {excludes(BONUS_LATE), not_android()}}, /* BONUS_INSOMNIAC */
    {"Evasive", "Evasive     - +15% to dodge rolls                                    @D[@G-3pts @D]",
     -3, {excludes(BONUS_PUNCHINGBAG)}}, /* BONUS_EVASIVE */
    {"The Wall", "The Wall    - +20% chance to block                                   @D[@G-3pts @D]",
     -3, {excludes(BONUS_PUSHOVER)}}, /* BONUS_WALL */
    {"Accurate", "Accurate    - +20% chance to hit physical, +10% to hit with ki       @D[@G-4pts @D]",
     -4, {excludes(BONUS_POORDEPTH)}}, /* BONUS_ACCURATE */
    {"Energy Leech", "Energy Leech- -2% ki damage received for every 5 character levels,   @D[@G-5pts @D]\n                  @cas long as you can take that ki to your charge pool.@D        ",
     -5, {excludes(BONUS_INTOLERANT)}}, /* BONUS_LEECH */
    {"Good Memory", "Good Memory - +2 Skill Slots initially, +1 every 20 levels after     @D[@G-6pts @D]",
     -6, {excludes(BONUS_BMEMORY)}}, /* BONUS_GMEMORY */
    /* the negatives: they give points */
    {"Soft Touch", "Soft Touch  - Half damage for all hit locations                      @D[@G+5pts @D]",
     5, {excludes(BONUS_POWERHIT)}}, /* BONUS_SOFT */
    {"Late Sleeper", "Late Sleeper- Can only wake automatically. 33% every hour if maxed   @D[@G+5pts @D]",
     5, {excludes(BONUS_INSOMNIAC, "@RLate Sleeper and Late Sleeper are mutually exclusive.\n\n") /* quirk: names itself twice */, not_android()}}, /* BONUS_LATE */
    {"Impulse Shop", "Impulse Shop- +25% shop costs                                        @D[@G+3pts @D]",
     3}, /* BONUS_IMPULSE */
    {"Sickly", "Sickly      - Suffer from harmful effects longer                     @D[@G+5pts @D]",
     5, {excludes(BONUS_HEALTHY)}}, /* BONUS_SICKLY */
    {"Punching Bag", "Punching Bag- -15% to dodge rolls                                    @D[@G+3pts @D]",
     3, {excludes(BONUS_EVASIVE)}}, /* BONUS_PUNCHINGBAG */
    {"Pushover", "Pushover    - -20% block chance                                      @D[@G+3pts @D]",
     3, {excludes(BONUS_WALL)}}, /* BONUS_PUSHOVER */
    {"Poor Depth Perception", "Poor D. Perc- -20% chance to hit with physical, -10% with ki         @D[@G+4pts @D]",
     4, {excludes(BONUS_ACCURATE)}}, /* BONUS_POORDEPTH */
    {"Thin Skin", "Thin Skin   - +20% physical and +10% ki damage received              @D[@G+4pts @D]",
     4, {excludes(BONUS_THICKSKIN)}}, /* BONUS_THINSKIN */
    {"Fireprone", "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned            @D[@G+5pts @D]",
     5, {excludes(BONUS_FIREPROOF)}}, /* BONUS_FIREPRONE */
    {"Energy Intollerant", "Energy Int. - +2% ki damage received for every 5 character levels,   @D[@G+6pts @D]\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ",
     6, {excludes(BONUS_LEECH)}}, /* BONUS_INTOLERANT */
    {"Coward", "Coward      - Can't Attack Enemy With 150% Your Powerlevel           @D[@G+6pts @D]",
     6}, /* BONUS_COWARD */
    {"Arrogant", "Arrogant    - Cannot Suppress                                        @D[@G+1pt  @D]",
     1}, /* BONUS_ARROGANT */
    {"Unfocused", "Unfocused   - Charge concentration randomly breaks                   @D[@G+3pts @D]",
     3}, /* BONUS_UNFOCUSED */
    {"Slacker", "Slacker     - Physical activity drains more stamina                  @D[@G+3pts @D]",
     3, {excludes(BONUS_HARDWORKER)}}, /* BONUS_SLACKER */
    {"Slow Learner", "Slow Learner- Character auto-trains skills slower                    @D[@G+3pts @D]",
     3, {excludes(BONUS_QUICK_STUDY)}}, /* BONUS_SLOW_LEARNER */
    {"Masochistic", "Masochistic - Defense Skills Cap At 75                               @D[@G+5pts @D]",
     5}, /* BONUS_MASOCHISTIC */
    {"Mute", "Mute        - Can't use IC speech related commands                   @D[@G+4pts @D]",
     4}, /* BONUS_MUTE */
    {"Wimp", "Wimp        - Strength is capped at 45                               @D[@G+6pts @D]",
     6, {excludes(BONUS_BRAWNY)}}, /* BONUS_WIMP */
    {"Dull", "Dull        - Intelligence is capped at 45                           @D[@G+6pts @D]",
     6, {excludes(BONUS_SCHOLARLY)}}, /* BONUS_DULL */
    {"Foolish", "Foolish     - Wisdom is capped at 45                                 @D[@G+6pts @D]",
     6, {excludes(BONUS_SAGE)}}, /* BONUS_FOOLISH */
    {"Clumsy", "Clumsy      - Agility is capped at 45                                @D[@G+3pts @D]",
     3, {excludes(BONUS_AGILE)}}, /* BONUS_CLUMSY */
    {"Slow", "Slow        - Speed is capped at 45                                  @D[@G+6pts @D]",
     6, {excludes(BONUS_QUICK)}}, /* BONUS_SLOW */
    {"Frail", "Frail       - Constitution capped at 45                              @D[@G+4pts @D]",
     4, {excludes(BONUS_STURDY)}}, /* BONUS_FRAIL */
    {"Sadistic", "Sadistic    - Half Experience Gained For Quick Kills                 @D[@G+3pts @D]",
     3}, /* BONUS_SADISTIC */
    {"Loner", "Loner       - Can't Group, +5% Train gains, +10% to physical gains   @D[@G+2pts @D]",
     2, {excludes(BONUS_LOYAL)}}, /* BONUS_LONER */
    {"Bad Memory", "Bad Memory  - -5 Skill Slots                                         @D[@G+6pts @D]",
     6, {excludes(BONUS_GMEMORY)}}, /* BONUS_BMEMORY */
};
static_assert(std::size(traits) == MAX_BONUSES);

constexpr int FIRST_NEGATIVE = BONUS_SOFT;

std::string_view trait_name(int i)
{
  return traits[i].name;
}

/* The bonuses menu (numbered from 1) or the negatives menu (numbered from 12), with the points left. */
static void display_bonus_menu(GameWorld *g, struct char_data *ch, bool negatives)
{
  if (!negatives) {
    send_to_char(g, ch, "\r\n@YBonus Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
    for (int x = 0; x < FIRST_NEGATIVE; x++)
      send_to_char(g, ch, "@C{:<2d}@D)@c {} <@g{}@D>\n", x + 1, traits[x].line, char_get_bonuses(ch, x) > 0 ? "X" : " ");
    send_to_char(g, ch, "\n");
    send_to_char(g, ch, "\n@CN@D)@c Show Negatives@n\n");
  } else {
    send_to_char(g, ch, "@YNegative Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
    for (int y = FIRST_NEGATIVE; y < MAX_BONUSES; y++)
      send_to_char(g, ch, "@R{:<2d}@D)@r {} <@g{}@D>\n", y - 14, traits[y].line, char_get_bonuses(ch, y) > 0 ? "X" : " ");
    send_to_char(g, ch, "\n@CB@D)@c Show Bonuses@n\n");
  }
  send_to_char(g, ch, "@CX@D)@c Exit Traits Section and complete your character@n\n");
  send_to_char(g, ch, "@D---------------------------------------\n[@WCurrent Points Pool@W: @y{:d}@D] [@WPTS From Neg@W: @y{:d}@D]@w\n", char_get_ccpoints(ch), char_get_negcount(ch));
}

/* The trait a menu number names, or -1. A number in the negatives menu below 12 names a bonus. */
static int trait_choice(std::string_view arg, bool negatives)
{
  int n = static_cast<int>(io::c_atol(arg));

  if (n < 1 || n > 51)
    return -1;
  n = negatives ? n + 14 : n - 1;
  return n < MAX_BONUSES ? n : -1;
}

/* Whether something stops ch taking the trait; if so, shows the menu again and says what. */
static bool trait_barred(GameWorld *g, struct char_data *ch, int value, bool negatives)
{
  for (const trait_bar &bar : traits[value].bars) {
    bool applies = (bar.when == bar_kind::taken && char_get_bonuses(ch, bar.other) > 0) ||
                   (bar.when == bar_kind::android && char_is_android(ch)) ||
                   (bar.when == bar_kind::demon && char_is_demon(ch));
    if (!applies)
      continue;
    display_bonus_menu(g, ch, negatives);
    if (!bar.says.empty())
      send_to_char(g, ch, "{}", bar.says);
    else if (bar.when == bar_kind::taken)
      send_to_char(g, ch, "@R{} and {} are mutually exclusive.\n\n", traits[bar.other].name, traits[value].name);
    else
      send_to_char(g, ch, "You can not take {} as an android!@n\r\n", traits[value].name);
    return true;
  }
  return false;
}

/* Takes the trait, or gives it back if taken already, paying or refunding its points. */
static void exchange_ccpoints(GameWorld *g, struct char_data *ch, int value)
{
  bool negatives = desc_get_connected(char_get_desc(ch)) != CON_BONUS;
  const trait &t = traits[value];

  if (char_get_bonuses(ch, value) > 0) {
    if (char_get_ccpoints(ch) - t.cost < 0) {
      display_bonus_menu(g, ch, negatives);
      send_to_char(g, ch, "@RYou must unselect some bonus traits first.\r\n");
      return;
    }
    char_set_ccpoints(ch, char_get_ccpoints(ch) - t.cost);
    if (t.cost > 0)
      char_set_negcount(ch, char_get_negcount(ch) - t.cost);
    char_set_bonuses(ch, value, 0);
    display_bonus_menu(g, ch, negatives);
    send_to_char(g, ch, "@GYou cancel your selection of {}.\r\n", t.name);
    return;
  }

  if (!negatives) {
    if (value >= FIRST_NEGATIVE) {
      display_bonus_menu(g, ch, negatives);
      send_to_char(g, ch, "@RYou are not in the negatives menu, enter B to switch.\r\n"); /* quirk: N switches */
      return;
    }
    if (char_get_ccpoints(ch) + t.cost < 0) {
      display_bonus_menu(g, ch, negatives);
      send_to_char(g, ch, "@RYou do not have enough points for {}.\r\n", t.name);
      return;
    }
    if (trait_barred(g, ch, value, negatives))
      return;
    char_set_ccpoints(ch, char_get_ccpoints(ch) + t.cost);
    char_set_bonuses(ch, value, 1);
    display_bonus_menu(g, ch, negatives);
    send_to_char(g, ch, "@GYou select the bonus {}\r\n", t.name);
    return;
  }

  if (value < FIRST_NEGATIVE) {
    display_bonus_menu(g, ch, negatives);
    send_to_char(g, ch, "@RYou are not in the bonuses menu, enter B to switch.\r\n");
    return;
  }
  int count = 0; /* the points the negatives taken give */
  for (int x = 14; x < MAX_BONUSES; x++)
    if (char_get_bonuses(ch, x) > 1)
      count += traits[x].cost;
  if (t.cost + count > 10) {
    display_bonus_menu(g, ch, negatives);
    send_to_char(g, ch, "@RYou can not have more than +10 points from negatives.\r\n");
    return;
  }
  if (trait_barred(g, ch, value, negatives))
    return;
  char_set_ccpoints(ch, char_get_ccpoints(ch) + t.cost);
  char_set_negcount(ch, char_get_negcount(ch) + t.cost);
  char_set_bonuses(ch, value, 2);
  display_bonus_menu(g, ch, negatives);
  send_to_char(g, ch, "@GYou select the negative {}\r\n", t.name);
}

/* ---- Menu helpers ---------------------------------------------------- */

/* The position of the answer's first character in keys (letters in either case), or -1. */
static int menu_choice(std::string_view arg, std::string_view keys)
{
  if (arg.empty())
    return -1;
  size_t key = keys.find(static_cast<char>(tolower(static_cast<unsigned char>(arg.front()))));
  return key == std::string_view::npos ? -1 : static_cast<int>(key);
}

static void reject_option(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "That is not an acceptable option.\r\n");
}

/* ---- Prompts: what each state shows on entering it ------------------- */

/* Races with hair (rather than horns, a forelock, antennae or nothing). */
static bool has_hair(const struct char_data *ch)
{
  return char_is_human(ch) || char_is_saiyan(ch) || char_is_konatsu(ch) || char_is_mutant(ch) || char_is_android(ch) || char_is_kai(ch) || char_is_halfbreed(ch) || char_is_truffle(ch) || (char_is_hoshijin(ch) && char_is_female(ch));
}

/* Races asked a length: of hair, or of horns, a forelock or antennae. */
static bool has_length(const struct char_data *ch)
{
  return has_hair(ch) || char_is_demon(ch) || char_is_icer(ch) || char_is_majin(ch) || char_is_namek(ch) || char_is_arlian(ch);
}

/* CON_RACIAL: how a halfbreed or an android is first seen */
static void show_racial_menu(GameWorld *g, struct descriptor_data *d)
{
  if (char_is_halfbreed(d->character)) {
    write_to_output(g, d, "@YWhat race do you prefer to by identified with?\r\n");
    write_to_output(g, d, "@cThis controls how others first view you and whether you start with\na tail and how fast it regrows when missing.\r\n");
    write_to_output(g, d, "@D---------------------------------------@n\r\n");
    write_to_output(g, d, "@B1@W)@C Human\n@B2@W)@C Saiyan\r\n");
  } else {
    write_to_output(g, d, "@YWhat do you want to be identified as at first glance?\r\n");
    write_to_output(g, d, "@D---------------------------------------@n\r\n");
    write_to_output(g, d, "@B1@W)@C Android\n@B2@W)@C Human\n@B3@W)@C Robotic Humanoid\r\n");
  }
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

static void show_sex_prompt(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "\r\n@wWhat is your sex @W(@BM@W/@MF@W/@GN@W)@w?@n");
}

/* CON_HAIRL: hair, horn, forelock or antenna length */
static void show_length_menu(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  std::string_view part, shortest;

  if (has_hair(ch)) {
    part = "Hair", shortest = "Bald";
  } else if (char_is_demon(ch) || char_is_icer(ch)) {
    part = "Horn", shortest = "None";
  } else if (char_is_majin(ch)) {
    part = "Forelock", shortest = "Tiny";
  } else {
    part = "Antenae", shortest = "Tiny";
  }
  write_to_output(g, d, "@Y{} Length SELECTION menu:\r\n", part);
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C {}  @B2@W)@C Short  @B3@W)@C Medium\r\n", shortest);
  write_to_output(g, d, "@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

/* CON_HAIRC: hair colour, or wing colour for female Arlians */
static void show_color_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@Y{} color SELECTION menu:\r\n", char_is_arlian(d->character) ? "Wing" : "Hair");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C Black  @B2@W)@C Brown  @B3@W)@C Blonde\r\n");
  write_to_output(g, d, "@B4@W)@C Grey   @B5@W)@C Red    @B6@W)@C Orange@n\r\n");
  write_to_output(g, d, "@B7@W)@C Green  @B8@W)@C Blue   @B9@W)@C Pink\r\n");
  write_to_output(g, d, "@BA@W)@C Purple @BB@W)@C Silver @BC@W)@C Crimson@n\r\n");
  write_to_output(g, d, "@BD@W)@C White@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

static void show_hair_style_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@YHair style SELECTION menu:\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C Plain     @B2@W)@C Mohawk    @B3@W)@C Spiky\r\n");
  write_to_output(g, d, "@B4@W)@C Curly     @B5@W)@C Uneven    @B6@W)@C Ponytail@n\r\n");
  write_to_output(g, d, "@B7@W)@C Afro      @B8@W)@C Fade      @B9@W)@C Crew Cut\r\n");
  write_to_output(g, d, "@BA@W)@C Feathered @BB@W)@C Dred Locks@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

static void show_skin_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@YSkin color SELECTION menu:\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
  write_to_output(g, d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
  write_to_output(g, d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
  write_to_output(g, d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

static void show_eye_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@YEye color SELECTION menu:\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C Blue  @B2@W)@C Black  @B3@W)@C Green\r\n");
  write_to_output(g, d, "@B4@W)@C Brown @B5@W)@C Red    @B6@W)@C Aqua@n\r\n");
  write_to_output(g, d, "@B7@W)@C Pink  @B8@W)@C Purple @B9@W)@C Crimson\r\n");
  write_to_output(g, d, "@BA@W)@C Gold  @BB@W)@C Amber  @BC@W)@C Emerald@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

static void show_feature_menu(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  write_to_output(g, d, "@YWhat do you want to be your most distinguishing feature:\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C My Eyes@n\r\n");
  if (char_is_majin(ch)) {
    write_to_output(g, d, "@B2@W)@C My Forelock@n\r\n");
  } else if (char_is_namek(ch) || char_is_arlian(ch)) {
    write_to_output(g, d, "@B2@W)@C My Antennae@n\r\n");
  } else if (char_is_icer(ch) || char_is_demon(ch)) {
    write_to_output(g, d, "@B2@W)@C My Horns@n\r\n");
  } else if (char_get_hairl(ch) == HAIRL_BALD) {
    write_to_output(g, d, "@B2@W)@C My Baldness@n\r\n");
  } else {
    write_to_output(g, d, "@B2@W)@C My Hair@n\r\n");
  }
  write_to_output(g, d, "@B3@W)@C My Skin@n\r\n");
  write_to_output(g, d, "@B4@W)@C My Height@n\r\n");
  write_to_output(g, d, "@B5@W)@C My Weight@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

/* Height (cm) and weight (kg) ranges offered at creation. */
struct size_range {
  int min_height{}, max_height{}, min_weight{}, max_weight{};
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
static std::span<const size_range> size_ranges(const struct char_data *ch)
{
  if (char_is_truffle(ch))
    return truffle_sizes;
  return std::span<const size_range>(normal_sizes).first(char_is_icer(ch) ? 3 : std::size(normal_sizes));
}

static void show_size_menu(GameWorld *g, struct descriptor_data *d)
{
  std::span<const size_range> sizes = size_ranges(d->character);

  write_to_output(g, d, "@YWhat Height/Weight Range do you prefer:\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  for (size_t i = 0; i < sizes.size(); i++)
    write_to_output(g, d, "@B{:d}@W)@C {:d}-{:d}cm, {:d}-{:d}kg@n\r\n", (int)i + 1, sizes[i].min_height, sizes[i].max_height, sizes[i].min_weight, sizes[i].max_weight);
  write_to_output(g, d, "\n@WMake a selection:@n ");
}

static void show_aura_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@YAura color SELECTION menu:\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C White  @B2@W)@C Blue@n\r\n");
  write_to_output(g, d, "@B3@W)@C Red    @B4@W)@C Green@n\r\n");
  write_to_output(g, d, "@B5@W)@C Pink   @B6@W)@C Purple@n\r\n");
  write_to_output(g, d, "@B7@W)@C Yellow @B8@W)@C Black@n\r\n");
  write_to_output(g, d, "@B9@W)@C Orange@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

/* CON_ANDROID */
static void show_model_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "\r\n@YChoose your model type.\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@B1@W)@C Absorbtion Model@n\r\n");
  write_to_output(g, d, "@B2@W)@C Repair Model@n\r\n");
  write_to_output(g, d, "@B3@W)@C Sense, Powersense Model@n\r\n");
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

/* CON_ALIGN: keep the questionnaire's alignment, or pick one */
static int answers_alignment(const struct descriptor_data *d);

static void show_alignment_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@C             Alignment Menu@n\r\n");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  int alignment = char_get_alignment(d->character) + answers_alignment(d); /* the answers aren't applied yet */

  write_to_output(g, d, "@cCurrent Alignment@D: {}{}@n\r\n", alignment < -50 ? "@R" : (alignment > 50 ? "@C" : "@G"), align_name(alignment));
  write_to_output(g, d, "@YThis is the alignment your character has based on your choices.\r\n");
  write_to_output(g, d, "Choose to keep this alignment with no penalty, or choose new\r\n");
  write_to_output(g, d, "alignment and suffer a -5{} PL and -1 stat (random) penalty.\r\n@n", "%");
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  write_to_output(g, d, "@BK@W) @wKeep This Alignment@n\r\n");
  write_to_output(g, d, "@B1@W) @wSaintly@n\r\n");
  write_to_output(g, d, "@B2@W) @wValiant@n\r\n");
  write_to_output(g, d, "@B3@W) @wHero@n\r\n");
  write_to_output(g, d, "@B4@W) @wDo-gooder@n\r\n");
  write_to_output(g, d, "@B5@W) @wNeutral\r\n");
  write_to_output(g, d, "@B6@W) @wCrook@n\r\n");
  write_to_output(g, d, "@B7@W) @wVillain@n\r\n");
  write_to_output(g, d, "@B8@W) @wTerrible@n\r\n");
  write_to_output(g, d, "@B9@W) @wHorribly Evil@n\r\n");
  write_to_output(g, d, "Choose: \r\n");
}

/* CON_SKILLS */
static void show_skills_prompt(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "keep or forget: \r\n");
}

/* rubbery_less picks between the two wordings the original menus used. */
static void show_mutation_menu(GameWorld *g, struct descriptor_data *d, std::string_view heading, bool rubbery_less)
{
  write_to_output(g, d, "{}", heading);
  write_to_output(g, d, "@D--------------------------------------------------------@n\n");
  write_to_output(g, d, "@B 1@W) @CExtreme Speed       @c-+30{} to Speed Index @C@n\n", "%");
  write_to_output(g, d, "@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12{} instead of 5{}@C@n\n", "%", "%");
  write_to_output(g, d, "@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
  write_to_output(g, d, "@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
  write_to_output(g, d, "@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
  write_to_output(g, d, "@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
  write_to_output(g, d, "@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
  if (rubbery_less)
    write_to_output(g, d, "@B 8@W) @CRubbery Body        @c-10{} less physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
  else
    write_to_output(g, d, "@B 8@W) @CRubbery Body        @c-10{} of physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
  write_to_output(g, d, "@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
  write_to_output(g, d, "@B10@w) @CNatural Energy      @c-Get 5{} of your ki damage refunded back into your current ki total.@n\n", "%");
}

static void show_genome_menu(GameWorld *g, struct descriptor_data *d, std::string_view heading)
{
  write_to_output(g, d, "{}", heading);
  write_to_output(g, d, "@D--------------------------------------------------------@n\n");
  write_to_output(g, d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
  write_to_output(g, d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
  write_to_output(g, d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
  write_to_output(g, d, "@B4@W) @CIcer    @c- @C+20{} damage for Tier 4 attacks@n\n", "%");
  write_to_output(g, d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
  write_to_output(g, d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
  write_to_output(g, d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
  write_to_output(g, d, "@B8@w) @CKonatsu @c- @C40{} higher chance to multihit on physical attacks.\r\n", "%");
}

static constexpr std::string_view mutation_names[] = {
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

static constexpr std::string_view genome_names[] = {
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

static void show_chosen_genomes(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@D----[@gGenome 1@W: @G{}@D]----[@gGenome 2@W: @G{}@D]----", genome_names[char_get_genome(d->character, 0)], genome_names[char_get_genome(d->character, 1)]);
}

/* The traits menu that follows skills, mutations or genomes. */
static void ask_bonuses(GameWorld *g, struct descriptor_data *d, std::string_view choose)
{
  display_bonus_menu(g, d->character, false);
  write_to_output(g, d, "@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
  write_to_output(g, d, "{}", choose);
  desc_set_connected(d, CON_BONUS);
}

/* Leaving the traits menus: unspent creation points become practice sessions. */
static void finish_bonuses(GameWorld *g, struct descriptor_data *d)
{
  char_set_negcount(d->character, 0);
  write_to_output(g, d, "\r\n@wTo check the bonuses/negatives you have in game use the status command");
  if (char_get_ccpoints(d->character) > 0) {
    write_to_output(g, d, "\r\n@GYour left over points were spent on Practice Sessions@w");
    char_set_practice_points(g, d->character, char_get_practice_points(g, d->character) + ((100 * char_get_ccpoints(d->character))));
  }
  write_to_output(g, d, "\r\n*** PRESS RETURN: ");
  desc_set_connected(d, CON_QROLLSTATS);
}

static void show_rolled_stats(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "\r\n@rStr@w: [@m{:>2d}@w] @rDex@w: [@m{:>2d}@w]\r\n@rCon@w: [@m{:>2d}@w] @rInt@w: [@m{:>2d}@w]\r\n@rWis@w: [@m{:>2d}@w] @rCha@w: [@m{:>2d}@w]@n",
                  char_get_aff_abils_str(d->character), char_get_aff_abils_dex(d->character),
                  char_get_aff_abils_con(d->character), char_get_aff_abils_intel(d->character),
                  char_get_aff_abils_wis(d->character), char_get_aff_abils_cha(d->character));
}

/* Saves the finished character and shows the MOTD. */
static void finish_creation(GameWorld *g, struct descriptor_data *d)
{
  if (char_get_pfilepos(d->character) < 0)
    char_set_pfilepos(d->character, create_entry(g, char_get_pc_name(d->character)));
  /* Now GET_NAME() will work properly. */
  init_char(g, d->character);
  save_char(g, d->character);
  save_player_index(g);
  write_to_output(g, d, "{}\r\n*** PRESS RETURN: ", g->motd);
  desc_set_connected(d, CON_RMOTD);
  mudlog(g, LOG_CMP, ADMLVL_GOD, true, "New player: {} [{} {}]",
         char_get_name(d->character), char_true_race_name(d->character),
         char_sensei_name(d->character));
}

/* ---- Race and sex ----------------------------------------------------- */

static void next_step(GameWorld *g, struct descriptor_data *d);

/* CON_QRACE / CON_RACE_HELP */
static void state_qrace(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int chosen_race;

  if (arg.empty() || (arg.size() == 1 && toupper(arg.front()) != 'T')) {
    write_to_output(g, d, "\r\nThat's not a race.\r\nRace: ");
    return;
  }
  if (arg.size() == 1) { /* T: toggle between the selection and help menus */
    enter_state(g, d, desc_get_connected(d) == CON_QRACE ? CON_RACE_HELP : CON_QRACE);
    return;
  }

  chosen_race = race_find(arg, char_get_sex(d->character), true);
  if (chosen_race == RACE_UNDEFINED) {
    write_to_output(g, d, "\r\nThat's not a race.\r\nRace: ");
    return;
  }
  if (desc_get_connected(d) == CON_RACE_HELP) {
    show_help(g, d, race_name(chosen_race));
    return;
  }
  if (race_rpp_cost(chosen_race)) {
    write_to_output(g, d, "\r\n{:d} RPP will be paid upon your first level up.\r\n", race_rpp_cost(chosen_race));
  }
  char_set_race(d->character, chosen_race);
  next_step(g, d);
}

/* CON_RACIAL */
static void state_racial(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int choice = menu_choice(arg, char_is_halfbreed(d->character) ? "12" : "123");

  if (choice < 0) {
    reject_option(g, d);
    return;
  }
  char_get_player_specials(d->character)->racial_pref = choice + 1;
  next_step(g, d);
}

/* CON_QSEX: query sex of new user (Nameks aren't asked) */
static void state_qsex(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  static const int sexes[] = {SEX_MALE, SEX_FEMALE, SEX_NEUTRAL};
  int choice               = menu_choice(arg, "mfn");

  if (choice < 0) {
    write_to_output(g, d, "That is not a sex..\r\nWhat IS your sex? ");
    return;
  }
  char_set_sex(d->character, sexes[choice]);
  next_step(g, d);
}

/* ---- Appearance ------------------------------------------------------- */

static const int hair_lengths[] = {HAIRL_BALD, HAIRL_SHORT, HAIRL_MEDIUM, HAIRL_LONG, HAIRL_RLONG};
static const int hair_colors[]  = {HAIRC_BLACK, HAIRC_BROWN, HAIRC_BLONDE, HAIRC_GREY, HAIRC_RED, HAIRC_ORANGE, HAIRC_GREEN,
                                   HAIRC_BLUE, HAIRC_PINK, HAIRC_PURPLE, HAIRC_SILVER, HAIRC_CRIMSON, HAIRC_WHITE};
static const int hair_styles[]  = {HAIRS_PLAIN, HAIRS_MOHAWK, HAIRS_SPIKY, HAIRS_CURLY, HAIRS_UNEVEN, HAIRS_PONYTAIL,
                                   HAIRS_AFRO, HAIRS_FADE, HAIRS_CREW, HAIRS_FEATHERED, HAIRS_DRED};
static const int skins[]        = {SKIN_WHITE, SKIN_BLACK, SKIN_GREEN, SKIN_ORANGE, SKIN_YELLOW, SKIN_RED,
                                   SKIN_GREY, SKIN_BLUE, SKIN_AQUA, SKIN_PINK, SKIN_PURPLE, SKIN_TAN};
static const int eyes[]         = {EYE_BLUE, EYE_BLACK, EYE_GREEN, EYE_BROWN, EYE_RED, EYE_AQUA,
                                   EYE_PINK, EYE_PURPLE, EYE_CRIMSON, EYE_GOLD, EYE_AMBER, EYE_EMERALD};

/* An appearance question: the keys answering it, and what each sets (or, with no values, the key's position). */
struct appearance {
  int state{};
  std::string_view keys{};
  std::span<const int> values{};
  void (*set)(struct char_data *ch, int value){};
};

static const appearance appearances[] = {
    {CON_HAIRL,   "12345",         hair_lengths, [](char_data *ch, int v) { char_set_hairl(ch, v); }  },
    {CON_HAIRC,   "123456789abcd", hair_colors,  [](char_data *ch, int v) { char_set_hairc(ch, v); }  },
    {CON_HAIRS,   "123456789ab",   hair_styles,  [](char_data *ch, int v) { char_set_hairs(ch, v); }  },
    {CON_SKIN,    "123456789abc",  skins,        [](char_data *ch, int v) { char_set_skin(ch, v); }   },
    {CON_EYE,     "123456789abc",  eyes,         [](char_data *ch, int v) { char_set_eye(ch, v); }    },
    {CON_DISTFEA, "12345",         {},           [](char_data *ch, int v) { char_set_distfea(ch, v); }},
    {CON_AURA,    "123456789",     {},           [](char_data *ch, int v) { char_set_aura(ch, v); }   },
};

/* CON_HAIRL, CON_HAIRC, CON_HAIRS, CON_SKIN, CON_EYE, CON_DISTFEA, CON_AURA */
static void state_appearance(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  const appearance *a = std::ranges::find(appearances, desc_get_connected(d), &appearance::state);
  int choice          = menu_choice(arg, a->keys);

  if (choice < 0) {
    reject_option(g, d);
    return;
  }
  a->set(d->character, a->values.empty() ? choice : a->values[choice]);
  next_step(g, d);
}

/* CON_HW */
static void state_hw(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  std::span<const size_range> sizes = size_ranges(d->character);
  int choice                        = menu_choice(arg, "123456");

  if (choice < 0 || choice >= (int)sizes.size()) {
    reject_option(g, d);
    return;
  }
  d->chargen.size = choice;
  next_step(g, d);
}

/* ---- Sensei ----------------------------------------------------------- */

/* CON_QCLASS / CON_CLASS_HELP */
static void state_qclass(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int chosen_sensei;

  if (arg.size() == 1) {
    if (toupper(arg.front()) != 'T') /* T: toggle between the selection and help menus */
      write_to_output(g, d, "\r\nThat's not a sensei.\r\nSensei: ");
    else
      enter_state(g, d, desc_get_connected(d) == CON_CLASS_HELP ? CON_QCLASS : CON_CLASS_HELP);
    return;
  }

  chosen_sensei = sensei_find(arg);
  if (chosen_sensei == CLASS_UNDEFINED) {
    write_to_output(g, d, "\r\nThat's not a sensei.\r\nSensei: ");
    return;
  }
  if (desc_get_connected(d) == CON_CLASS_HELP) {
    show_help(g, d, sensei_name(chosen_sensei));
    return;
  }
  /* only the senseis the menu offers this race */
  if (!sensei_is_pc_ok(chosen_sensei) || !sensei_available_for_race(chosen_sensei, char_get_race(d->character))) {
    write_to_output(g, d, "\r\nThat's not a sensei.\r\nSensei: ");
    return;
  }
  char_set_class(d->character, chosen_sensei);
  if (!char_is_android(d->character)) { /* an android's model, asked next, says its own */
    write_to_output(g, d, "\r\n@RAnswer The following questions carefully, they construct your alignment\r\nand affect your stats.\r\n\r\n");
    write_to_output(g, d, "@WQuestion (@G1@W out of @g10@W)\r\n");
  }
  next_step(g, d);
}

/* CON_ANDROID */
static void state_android(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  switch (first_char(arg)) {
    case '1':
      char_plr_set(d->character, player_flag::absorb, true);
      write_to_output(g, d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your liking.\r\n\r\n");
      write_to_output(g, d, "\r\n@WQuestion (@G1@W out of @g10@W)");
      break;
    case '2':
      char_plr_set(d->character, player_flag::repair, true);
      write_to_output(g, d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your linking.\r\n\r\n");
      break;
    case '3':
      char_plr_set(d->character, player_flag::sensem, true);
      write_to_output(g, d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer or your stats contrary to your liking.\r\n\r\n");
      break;
    default:
      write_to_output(g, d, "@wThat is not a correct selection, try again.@n\r\n");
      return;
  }
  next_step(g, d);
}

/* ---- Questionnaire ---------------------------------------------------- */

/* Starting ability ranges by race, in the order they are rolled. */
static const struct {
  bool (*is_race)(const struct char_data *ch){};
  int str[2]{}, con[2]{}, wis[2]{}, intel[2]{}, cha[2]{}, dex[2]{};
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

static void roll_starting_abilities(GameWorld *g, struct char_data *ch)
{
  for (size_t i = 0; i < countof(starting_abilities); i++) {
    if (!starting_abilities[i].is_race(ch))
      continue;
    char_set_real_abils_str(ch, rand_number(g, starting_abilities[i].str[0], starting_abilities[i].str[1]));
    char_set_real_abils_con(ch, rand_number(g, starting_abilities[i].con[0], starting_abilities[i].con[1]));
    char_set_real_abils_wis(ch, rand_number(g, starting_abilities[i].wis[0], starting_abilities[i].wis[1]));
    char_set_real_abils_intel(ch, rand_number(g, starting_abilities[i].intel[0], starting_abilities[i].intel[1]));
    char_set_real_abils_cha(ch, rand_number(g, starting_abilities[i].cha[0], starting_abilities[i].cha[1]));
    char_set_real_abils_dex(ch, rand_number(g, starting_abilities[i].dex[0], starting_abilities[i].dex[1]));
    return;
  }
}

/* The base stats and abilities, rolled before the answers are applied. */
static void roll_starting_stats(GameWorld *g, struct char_data *ch)
{
  char_set_basepl(ch, rand_number(g, 30, 50));
  char_set_basest(ch, rand_number(g, 30, 50));
  char_set_baseki(ch, rand_number(g, 30, 50));
  roll_starting_abilities(g, ch);
}

/* In the order of the ability penalty's roll. */
enum class ability { none, str, con, wis, intel, cha, dex };

static void add_ability(struct char_data *ch, ability which, int n)
{
  switch (which) {
    case ability::none:
      break;
    case ability::str:
      char_set_real_abils_str(ch, char_get_real_abils_str(ch) + n);
      break;
    case ability::con:
      char_set_real_abils_con(ch, char_get_real_abils_con(ch) + n);
      break;
    case ability::wis:
      char_set_real_abils_wis(ch, char_get_real_abils_wis(ch) + n);
      break;
    case ability::intel:
      char_set_real_abils_intel(ch, char_get_real_abils_intel(ch) + n);
      break;
    case ability::cha:
      char_set_real_abils_cha(ch, char_get_real_abils_cha(ch) + n);
      break;
    case ability::dex:
      char_set_real_abils_dex(ch, char_get_real_abils_dex(ch) + n);
      break;
  }
}

/* What an answer does to a base stat (PL, ST or KI): a fixed amount, a rand_number range added or taken away, or
   roll_stats' roll. */
enum class gain_kind { none, fixed, add, take, dice };

struct gain {
  gain_kind how{};
  int lo{}, hi{}; /* for dice, roll_stats' type and bonus */
};

static constexpr gain plus(int n) { return {gain_kind::fixed, n, 0}; }
static constexpr gain add(int lo, int hi) { return {gain_kind::add, lo, hi}; }
static constexpr gain take(int lo, int hi) { return {gain_kind::take, lo, hi}; }
static constexpr gain dice(int type, int bonus) { return {gain_kind::dice, type, bonus}; }

static int64_t gained(GameWorld *g, struct char_data *ch, gain how)
{
  switch (how.how) {
    case gain_kind::none:
      break;
    case gain_kind::fixed:
      return how.lo;
    case gain_kind::add:
      return rand_number(g, how.lo, how.hi);
    case gain_kind::take:
      return -(int64_t)rand_number(g, how.lo, how.hi);
    case gain_kind::dice:
      return roll_stats(g, ch, how.lo, how.hi);
  }
  return 0;
}

/* An answer, and what it does, in this order. */
struct answer {
  std::string_view text{}; /* empty past the last */
  int alignment{};    /* added */
  int gold_add{};
  int gold_set{-1};   /* or left alone */
  gain pl{}, st{}, ki{};
  ability raise{};    /* by one */
  int choice{};       /* or left alone */
  std::optional<player_flag> flag{}; /* a PLR_* set, or none */
};

struct question {
  int state{};
  bool gap{};          /* a blank line before "Answer the following question" */
  std::string_view ask{};
  std::array<answer, 5> answers{}; /* or none, and choices shows them */
  void (*choices)(GameWorld *g, struct descriptor_data *d){};
};

/* Question 10's choices: the last two ages depend on the race. */
static bool long_lived(const struct char_data *ch)
{
  return char_is_kai(ch) || char_is_demon(ch) || char_is_majin(ch);
}

static void show_ages(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "@B1@W)@C  8@n    @B2@W)@C 10@n\r\n");
  write_to_output(g, d, "@B3@W)@C 12@n    @B4@W)@C 14@n\r\n");
  write_to_output(g, d, "@B5@W)@C 16@n    @B6@W)@C 18@n\r\n");
  write_to_output(g, d, "@B7@W)@C 20@n    @B8@W)@C 22@n\r\n");
  write_to_output(g, d, "@B9@W)@C 24@n    @BA@W)@C 26@n\r\n");
  write_to_output(g, d, "@BB@W)@C 28@n    @BC@W)@C 30@n\r\n");
  write_to_output(g, d, "@BD@W)@C 40@n    @BE@W)@C 50@n\r\n");
  write_to_output(g, d, "@BF@W)@C 60@n    @BG@W)@C 65@n\r\n");
  if (long_lived(d->character)) {
    write_to_output(g, d, "@BH@W)@C 500@n   @BI@W)@C 800@n\r\n");
  } else if (char_is_namek(d->character)) {
    write_to_output(g, d, "@BH@W)@C 250@n   @BI@W)@C 400@n\r\n");
  } else {
    write_to_output(g, d, "@BH@W)@C 70@n    @BI@W)@C 75@n\r\n");
  }
}

/* The ten questions, in order; question 10 (the age) has its own handler. */
static const question questions[] = {
    {CON_Q1, false, "@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n",
     {{{.text = "Ask someone with more experience what to do.", .pl = dice(5, 25), .st = dice(8, 50), .ki = dice(6, 50)},
      {.text = "Jump in with some nice classic pushups!", .pl = dice(5, 55), .st = dice(8, 40), .ki = dice(6, 40)},
      {.text = "Search for something magical to increase my strength.", .pl = dice(5, 125), .st = dice(8, 50), .ki = dice(6, 40)},
      {.text = "Practice my favorite skills instead of working just on my body.", .pl = dice(5, 65), .st = dice(8, 65), .ki = dice(6, 65), .flag = player_flag::skillp},
      {.text = "Spar with a friend so we can both improve our abilities.", .pl = dice(5, 75), .st = dice(8, 100), .ki = dice(6, 75)}}}},
    {CON_Q2, false, "@wYou are faced with the strongest opponent you have ever\r\nfaced in your life. You both have beat each other to the\r\nlimits of both your strengths. A situation has presented \r\nan opportunity to win the fight, what do you do?\r\n",
     {{{.text = "Kill my opponent in a very brutal fashion!", .alignment = -200},
      {.text = "Disable my opponent, but spare their life.", .alignment = 100},
      {.text = "Kill my opponent, I have no other choice.", .alignment = 10},
      {.text = "Try to evade my opponent till I can get away.", .alignment = 0},
      {.text = "Take their head clean off and bathe in their blood!", .alignment = -400}}}},
    {CON_Q3, false, "@wYou are one day offered a means to gain incredible strength\r\nby some extraordinary means. The only problem is it requires\r\nthe lives of innocents to obtain. What do you do?\r\n",
     {{{.text = "Hell yeah I take the opportunity to get stronger!", .alignment = -100, .pl = plus(100), .st = plus(80), .ki = plus(10)},
      {.text = "I refuse to gain unnatural strength.", .alignment = 10, .pl = plus(25), .st = plus(25), .ki = plus(25)},
      {.text = "I refuse to gain strength at the cost of the innocent.", .alignment = 50, .pl = plus(20), .st = plus(20), .ki = plus(20)},
      {.text = "I kill that many innocents before breakfast anyway...", .alignment = -200, .pl = plus(100), .st = plus(100), .ki = plus(100)}}}},
    {CON_Q4, false, "@wOne day you are offered a way to make a lot of money, but in order\r\nto do so you will need to stop training for a whole month to\r\nhandle business. What do you do?\r\n",
     {{{.text = "I take the opportunity, with more money I can train better later.", .gold_add = 1000, .pl = take(10, 30), .st = take(10, 30), .ki = take(10, 30)},
      {.text = "I refuse to waste my time. What I need is some nice hard training.", .gold_set = 0, .pl = add(50, 165), .st = add(50, 165), .ki = add(50, 165)},
      {.text = "Hmm. With more money I can live better, certainly that is worth the time.", .gold_set = 2500, .pl = take(15, 25), .st = take(15, 25), .ki = take(15, 25)},
      {.text = "I choose to earn a little money while still training instead.", .gold_set = 150, .pl = add(25, 80), .st = add(25, 80), .ki = add(25, 80)}}}},
    {CON_Q5, false, "@wYou are introduced to a new way of training one day, what do you do?\r\n",
     {{{.text = "I prefer my way, it has worked so far.", .pl = add(0, 40)},
      {.text = "I am open to new possibilites, sure.", .pl = add(-30, 80)},
      {.text = "I will at least try it, for a little while.", .pl = add(-25, 60)},
      {.text = "No way is superior to eating spinach everyday...", .pl = add(0, 50)}}}},
    {CON_Q6, true, "@wYou have an enemy before you, what is your prefered method to attack him?@n\r\n",
     {{{.text = "I prefer to defend rather than attack.", .pl = add(0, 15), .st = add(0, 15), .choice = 1},
      {.text = "I prefer to throw a strong punch at the enemy's throat.", .pl = add(0, 30), .choice = 2},
      {.text = "I prefer to send a devestating kick at the enemy's neck.", .pl = add(0, 30), .choice = 3},
      {.text = "I prefer to smash them with a two-handed slam!", .pl = add(0, 30), .choice = 4},
      {.text = "I prefer to throw one of my many energy attacks!", .ki = add(0, 50), .choice = 5}}}},
    {CON_Q7, true, "@wYou are camped out one night in a field, the sky is clear and the\r\nstars visible. Looking at them, what thought crosses your mind?@n\r\n",
     {{{.text = "One day, I am going to conquer every one of those.", .alignment = -10, .raise = ability::str},
      {.text = "I really wish I had brought that special someone along as this is a great night for romance", .alignment = 10, .raise = ability::cha},
      {.text = "Those stars hold greater meaning to you than just planets, they dictate\r\nyour life or guide you and those arround you in one form or another.", .raise = ability::wis},
      {.text = "You'd very much like to travel into space to get to one of these stars to study all that it holds.", .raise = ability::intel}}}},
    {CON_Q8, true, "@wOne day, you are on the way home. You happen to walk past two\r\nof your best friends about to lock horns. Do you@n\r\n",
     {{{.text = "You try to talk them out of it. With a silver tongue, you can stop the fight before it has began.", .alignment = 10, .raise = ability::cha},
      {.text = "It's clear that they are quite chuffed about something, you could stop the fight now but they may just try again later. It's best to investigate and find out the source of the problem.", .alignment = 20, .raise = ability::wis},
      {.text = "Pick the one you like the most and help them to win.", .alignment = -10, .raise = ability::str},
      {.text = "You try to sneak past them, reckoning no matter what you do there its a no win situation, so long as they don't see you.", .alignment = -20, .raise = ability::dex}}}},
    {CON_Q9, true, "@wAs a kid you were confronted with this. On the way from the bakery,\r\na group of kids corner you in an alley and the leader demands that\r\nyou surrender your jam donut to him. Do you...@n\r\n",
     {{{.text = "Throw the donut up in the air, hoping that the leader will pay enough attention to it so that you can get at least one good shot in.", .raise = ability::str},
      {.text = "Surrender the donut for now but come back later with your friends and gain your revenge.", .alignment = -30, .raise = ability::wis},
      {.text = "Give him the donut, then return to the baker. You tell him a sob story and convince him to give you a replacement donut.", .alignment = -10, .raise = ability::cha},
      {.text = "It's better just to do as he says and leave it to that. After all its only a donut.", .alignment = -5, .raise = ability::intel}}}},
    {CON_QX, true, "@wWhat do you wish your starting age to be?@n\r\n", {}, show_ages},
};

static const question &question_for(int state)
{
  return *std::ranges::find(questions, state, &question::state);
}

static int answer_count(const question &q)
{
  return (int)std::ranges::count_if(q.answers, [](const answer &a) { return !a.text.empty(); });
}

/* CON_Q1 to CON_QX. Question 1's heading is its caller's. */
static void ask_question(GameWorld *g, struct descriptor_data *d)
{
  const question &q = question_for(desc_get_connected(d));
  int number        = (int)(&q - questions) + 1;

  if (number > 1)
    write_to_output(g, d, "\r\n@WQuestion (@G{:d}@W out of @g10@W)\r\n", number);
  if (q.gap)
    write_to_output(g, d, "\r\n");
  write_to_output(g, d, "@YAnswer the following question:\r\n");
  write_to_output(g, d, "{}", q.ask);
  write_to_output(g, d, "@D---------------------------------------@n\r\n");
  if (q.choices)
    q.choices(g, d);
  for (int i = 0; i < answer_count(q); i++)
    write_to_output(g, d, "@B{:d}@W)@C {}@n\r\n", i + 1, q.answers[i].text);
  write_to_output(g, d, "@w\r\nMake a selection:@n\r\n");
}

static void apply_answer(GameWorld *g, struct char_data *ch, const answer &a)
{
  if (a.alignment)
    char_set_alignment(ch, char_get_alignment(ch) + a.alignment);
  if (a.gold_add)
    char_set_gold(ch, char_get_gold(ch) + a.gold_add);
  if (a.gold_set >= 0)
    char_set_gold(ch, a.gold_set);
  if (a.pl.how != gain_kind::none)
    char_set_basepl(ch, getBasePL(ch) + gained(g, ch, a.pl));
  if (a.st.how != gain_kind::none)
    char_set_basest(ch, getBaseST(ch) + gained(g, ch, a.st));
  if (a.ki.how != gain_kind::none)
    char_set_baseki(ch, getBaseKI(ch) + gained(g, ch, a.ki));
  add_ability(ch, a.raise, 1);
  if (a.choice)
    char_set_choice(ch, a.choice);
  if (a.flag)
    char_plr_set(ch, *a.flag, true);
}

/* The alignment the answers picked give, added to the character's when they're applied. */
static int answers_alignment(const struct descriptor_data *d)
{
  int sum = 0;

  for (size_t i = 0; i < d->chargen.answers.size(); i++)
    sum += questions[i].answers[d->chargen.answers[i]].alignment;
  return sum;
}

/* CON_Q1 to CON_Q9 */
static void state_question(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  const question &q = question_for(desc_get_connected(d));
  char c            = first_char(arg);

  if (c < '1' || c >= '1' + answer_count(q)) {
    reject_option(g, d);
    return;
  }
  d->chargen.answers[&q - questions] = c - '1';
  next_step(g, d);
}

/* Question 10's age, by the choice's position */
static int age_chosen(const struct char_data *ch, int choice)
{
  /* the last two depend on the race: long-lived, Namek, everyone else */
  static const int ages[]         = {8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 40, 50, 60, 65};
  static const int old_ages[3][2] = {
      {500, 800},
      {250, 400},
      {70,  75 }
  };

  if (choice < (int)(countof(ages)))
    return ages[choice];
  return old_ages[long_lived(ch) ? 0 : char_is_namek(ch) ? 1
                                                         : 2][choice - 16];
}

/* CON_QX: starting age */
static void state_qx(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int choice = menu_choice(arg, "123456789abcdefghi");

  if (choice < 0) {
    reject_option(g, d);
    return;
  }
  d->chargen.age = choice;
  char_set_ccpoints(d->character, char_is_hoshijin(d->character) ? 10 : 5);
  next_step(g, d);
}

/* CON_ALIGN: keep the questionnaire's alignment, or pick one at a cost */
static const int alignments[] = {1000, 799, 599, 299, 0, -299, -599, -799, -1000}; /* CON_ALIGN's, from 1 */

static void state_align(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int choice = menu_choice(arg, "k123456789");

  if (choice < 0) {
    write_to_output(g, d, "That is not an acceptable option! Choose again...\r\n");
    return;
  }
  d->chargen.alignment = choice;
  if (choice == 0)
    write_to_output(g, d, "@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you will receive 200 PS in exchange.@n\r\n");
  else
    write_to_output(g, d, "@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you get 200 PS in exchange.@n\r\n");
  next_step(g, d);
}

/* CON_SKILLS: keep the sensei/race skills, or forget them for 200 practice sessions */
static void state_skills(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  bool forget = str_eq_nocase(arg, "forget");

  if (!forget && !str_eq_nocase(arg, "keep")) {
    show_skills_prompt(g, d);
    return;
  }
  d->chargen.forget = forget;
  next_step(g, d);
}

/* Entering CON_GENOME: a mutant's mutation, or a bio's two genomes. */
static void ask_genomes(GameWorld *g, struct descriptor_data *d)
{
  if (char_is_mutant(d->character))
    show_mutation_menu(g, d, "\n@RSelect a mutation. A second will be chosen automatically..\n", d->chargen.forget);
  else
    show_genome_menu(g, d, "\n@RSelect two genomes to be your primary DNA strains.\n");
  write_to_output(g, d, "@wChoose: ");
  char_set_genome(d->character, 0, 0);
  char_set_genome(d->character, 1, 0);
  desc_set_connected(d, CON_GENOME);
}

/* ---- Genomes, traits and stats --------------------------------------- */

/* Mutants pick one mutation; the next input rolls the second, and the one after moves on. */
static void mutant_genome(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  struct char_data *ch = d->character;

  show_mutation_menu(g, d, "\n@RSelect a mutation. A second will be chosen automatically..\n", true);
  write_to_output(g, d, "\n@wChoose: ");

  if (char_get_genome(ch, 0) > 0 && char_get_genome(ch, 1) <= 0) {
    int num = rand_number(g, 1, 8);
    while (num == char_get_genome(ch, 0))
      num = rand_number(g, 1, 8);
    write_to_output(g, d, "@CRolling second mutation... Your second mutation is @D[@Y{}@D]@n\r\n", mutation_names[num]);
    char_set_genome(ch, 1, num);
  } else if (char_get_genome(ch, 1) > 0) {
    next_step(g, d);
  } else if (arg.empty()) {
    write_to_output(g, d, "That is not an acceptable choice!\r\n");
    write_to_output(g, d, "\n@wChoose: ");
  } else {
    int choice = static_cast<int>(io::c_atol(arg));
    if (choice < 1 || choice > 10) {
      write_to_output(g, d, "That is not an acceptable choice!\r\n");
      return;
    }
    write_to_output(g, d, "@CYou have chosen the mutation @D[@Y{}@D]@n\r\n", mutation_names[choice]);
    char_set_genome(ch, 0, choice);
  }
}

/* Bios pick two different genomes; the input after the second moves on. */
static void bio_genome(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  struct char_data *ch = d->character;
  int select;

  if (char_get_genome(ch, 1) > 0) {
    next_step(g, d);
    return;
  }
  if (arg.empty()) {
    show_genome_menu(g, d, "@RSelect two genomes to be your primary DNA strains.\n");
    show_chosen_genomes(g, d);
    write_to_output(g, d, "\n@wChoose: ");
    return;
  }
  select = static_cast<int>(io::c_atol(arg));
  if (select > 8 || select < 1) {
    show_genome_menu(g, d, "@RSelect two genomes to be your primary DNA strains.\n");
    write_to_output(g, d, "@RThat is not an acceptable selection. @WTry again:@n\n");
    return;
  }
  if (char_get_genome(ch, 0) == 0) {
    char_set_genome(ch, 0, select);
  } else if (char_get_genome(ch, 0) == select) {
    write_to_output(g, d, "You can't choose the same thing for both genomes!\r\n");
    write_to_output(g, d, "\n@wChoose: ");
    return;
  } else {
    char_set_genome(ch, 1, select);
  }
  show_genome_menu(g, d, "@RSelect two genomes to be your primary DNA strains.\n");
  show_chosen_genomes(g, d);
}

/* CON_GENOME */
static void state_genome(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (char_is_mutant(d->character))
    mutant_genome(g, d, arg);
  else
    bio_genome(g, d, arg);
}

/* ---- The order of creation, and the effects made at its end ---------- */

static bool asked_racial(const struct char_data *ch) { return char_is_halfbreed(ch) || char_is_android(ch); }
static bool asked_sex(const struct char_data *ch) { return !char_is_namek(ch); }

/* the colour of hair, or of a female Arlian's wings */
static bool asked_hair_color(const struct char_data *ch)
{
  return has_length(ch) && (has_hair(ch) ? char_get_hairl(ch) != HAIRL_BALD : char_is_arlian(ch) && char_is_female(ch));
}

static bool asked_hair_style(const struct char_data *ch) { return asked_hair_color(ch) && !char_is_arlian(ch); }
static bool asked_genomes(const struct char_data *ch) { return char_is_bio(ch) || char_is_mutant(ch); }

/* A step of creation: who is asked it (everyone, without), what not being asked sets, and what entering it shows,
   where that isn't its state's prompt. */
struct step {
  int state{};
  bool (*asked)(const struct char_data *ch){};
  void (*skipped)(struct char_data *ch){};
  void (*enter)(GameWorld *g, struct descriptor_data *d){};
};

static const step steps[] = {
    {CON_QRACE},
    {CON_RACIAL, asked_racial},
    {CON_QSEX, asked_sex, [](char_data *ch) { char_set_sex(ch, SEX_NEUTRAL); }},
    {CON_HAIRL, has_length},
    {CON_HAIRC, asked_hair_color, [](char_data *ch) { char_set_hairc(ch, HAIRC_NONE); char_set_hairs(ch, HAIRS_NONE); }},
    {CON_HAIRS, asked_hair_style, [](char_data *ch) { char_set_hairs(ch, HAIRS_NONE); }},
    {CON_SKIN},
    {CON_EYE},
    {CON_DISTFEA},
    {CON_HW},
    {CON_AURA},
    {CON_QCLASS},
    {CON_ANDROID, char_is_android},
    {CON_Q1}, {CON_Q2}, {CON_Q3}, {CON_Q4}, {CON_Q5}, {CON_Q6}, {CON_Q7}, {CON_Q8}, {CON_Q9}, {CON_QX},
    {CON_ALIGN},
    {CON_SKILLS},
    {CON_GENOME, asked_genomes, nullptr, ask_genomes},
    {CON_BONUS, nullptr, nullptr, [](GameWorld *g, descriptor_data *d) { ask_bonuses(g, d, "@wChoose: "); }},
};

/* On from the step d is at to the next it's asked. */
static void next_step(GameWorld *g, struct descriptor_data *d)
{
  int at = desc_get_connected(d) == CON_RACE_HELP ? CON_QRACE : desc_get_connected(d) == CON_CLASS_HELP ? CON_QCLASS
                                                                                                         : desc_get_connected(d);
  const step *s = std::ranges::find(steps, at, &step::state);

  while (++s != std::end(steps)) {
    if (!s->asked || s->asked(d->character)) {
      if (s->enter)
        s->enter(g, d);
      else
        enter_state(g, d, s->state);
      return;
    }
    if (s->skipped)
      s->skipped(d->character);
  }
}

/* What the draft's choices do, in the order they were made: the size, the base stats and abilities rolled and then
   each answer, the age, the alignment chosen over the answers', the skills forgotten, the genomes'. */
static void apply_draft(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *ch    = d->character;
  const chargen_draft &dr = d->chargen;
  const size_range &size  = size_ranges(ch)[dr.size];

  char_set_height(ch, rand_number(g, size.min_height, size.max_height));
  char_set_weight(ch, rand_number(g, size.min_weight, size.max_weight));
  roll_starting_stats(g, ch);
  for (size_t i = 0; i < dr.answers.size(); i++)
    apply_answer(g, ch, questions[i].answers[dr.answers[i]]);
  char_set_time_birth(ch, game_now(g) - (age_chosen(ch, dr.age) * SECS_PER_MUD_YEAR));
  if (dr.alignment > 0) { /* at a cost */
    char_set_alignment(ch, alignments[dr.alignment - 1]);
    loseBasePLPercent(ch, .2, false);
    add_ability(ch, (ability)rand_number(g, 1, 6), -1); /* and lose a random point of one ability */
  }
  if (dr.forget) {
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) + (200));
    char_plr_set(ch, player_flag::forget, true);
  }
  if (char_is_mutant(ch)) {
    if (char_get_genome(ch, 0) == 3)
      char_set_real_abils_dex(ch, char_get_real_abils_dex(ch) + 10);
    else if (char_get_genome(ch, 0) == 9)
      char_set_skills_base(ch, SKILL_TELEPATHY, 50);
    if (char_get_genome(ch, 1) == 3) /* quirk: a second Innate Telepathy gives none */
      char_set_real_abils_dex(ch, char_get_real_abils_dex(ch) + 10);
  } else if (char_is_bio(ch) && (char_get_genome(ch, 0) == 7 || char_get_genome(ch, 1) == 7)) {
    char_set_skills_base(ch, SKILL_TELEPATHY, 30);
    char_set_skills_base(ch, SKILL_FOCUS, 30);
  }
}

/* CON_BONUS / CON_NEGATIVE: buy bonus traits, or take negative ones for points */
static void state_traits(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  bool negatives = desc_get_connected(d) == CON_NEGATIVE;
  int value;

  if (arg.empty()) {
    display_bonus_menu(g, d->character, negatives);
    send_to_char(g, d->character, "@wChoose: ");
  } else if (str_eq_nocase(arg, negatives ? "n" : "b")) {
    display_bonus_menu(g, d->character, negatives);
    send_to_char(g, d->character, "@RYou are already in that menu.\r\n");
    send_to_char(g, d->character, "@wChoose: ");
  } else if (str_eq_nocase(arg, negatives ? "b" : "n")) {
    display_bonus_menu(g, d->character, !negatives);
    send_to_char(g, d->character, "@wChoose: ");
    desc_set_connected(d, negatives ? CON_BONUS : CON_NEGATIVE);
  } else if (str_eq_nocase(arg, "x")) {
    apply_draft(g, d);
    if (negatives) { /* negatives can take base stats to zero or below */
      if (getBasePL(d->character) <= 0) {
        char_set_basepl(d->character, 90);
      }
      if (getBaseKI(d->character) <= 0) {
        char_set_baseki(d->character, 90);
      }
      if (getBaseST(d->character) <= 0) {
        char_set_basest(d->character, 90);
      }
    }
    finish_bonuses(g, d);
  } else if ((value = trait_choice(arg, negatives)) == -1) {
    display_bonus_menu(g, d->character, negatives);
    send_to_char(g, d->character, "@RThat is not an option.\r\n");
    send_to_char(g, d->character, "@wChoose: ");
  } else {
    exchange_ccpoints(g, d->character, value);
    send_to_char(g, d->character, "@wChoose: ");
  }
}

/* CON_QROLLSTATS: the configured way of setting base stats, then the end of creation */
static void state_qrollstats(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (CONFIG_REROLL_PLAYER_CREATION &&
      (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_1)) {
    if (toupper(first_char(arg)) != 'Y') {
      cedit_creation(g, d->character);
      show_rolled_stats(g, d);
      write_to_output(g, d, "\r\n\r\nKeep these stats? (y/N)");
      return;
    }
  } else if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2 ||
             CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3) {
    if (CONFIG_REROLL_PLAYER_CREATION &&
        (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2)) {
      if (toupper(first_char(arg)) != 'Y') {
        cedit_creation(g, d->character);
        show_rolled_stats(g, d);
        write_to_output(g, d, "Initial statistics, you may reassign individual numbers\r\n");
        write_to_output(g, d, "between statistics after choosing yes.\r\n");
        write_to_output(g, d, "\r\n\r\nKeep these stats? (y/N)");
        return;
      }
    } else
      cedit_creation(g, d->character);
    start_stat_points(g, d, CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3 ? CONFIG_INITIAL_POINTS_POOL : 0);

    desc_set_connected(d, CON_QSTATS);
    stats_disp_menu(g, d);
    return;
  } else {
    cedit_creation(g, d->character);
  }
  finish_creation(g, d);
}

/* CON_QSTATS: assigning points to stats (creation methods 2 and 3) */
static void state_qstats(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int total;

  if (!parse_stats(g, d, arg))
    return;
  if (d->olc) {
    d->olc.reset();
  }
  finish_creation(g, d);
  char_set_rp(d->character, d->acct.rpp);
  total            = char_get_aff_abils_str(d->character) / 2 + char_get_aff_abils_con(d->character) / 2 +
          char_get_aff_abils_wis(d->character) / 2 + char_get_aff_abils_intel(d->character) / 2 +
          char_get_aff_abils_dex(d->character) / 2 + char_get_aff_abils_cha(d->character) / 2;
  total -= 30;
  mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Str: {:>2d} Dex: {:>2d} Con: {:>2d} Int: {:>2d} Wis:  {:>2d} Cha: {:>2d} mod total: {:>2d}",
         char_get_aff_abils_str(d->character),
         char_get_aff_abils_dex(d->character), char_get_aff_abils_con(d->character), char_get_aff_abils_intel(d->character),
         char_get_aff_abils_wis(d->character), char_get_aff_abils_cha(d->character), total);
}

static const con_state chargen_states[] = {
    {CON_QRACE,      state_qrace,      display_races        },
    {CON_RACE_HELP,  state_qrace,      display_races_help   },
    {CON_RACIAL,     state_racial,     show_racial_menu     },
    {CON_QSEX,       state_qsex,       show_sex_prompt      },
    {CON_HAIRL,      state_appearance, show_length_menu     },
    {CON_HAIRC,      state_appearance, show_color_menu      },
    {CON_HAIRS,      state_appearance, show_hair_style_menu },
    {CON_SKIN,       state_appearance, show_skin_menu       },
    {CON_EYE,        state_appearance, show_eye_menu        },
    {CON_DISTFEA,    state_appearance, show_feature_menu    },
    {CON_HW,         state_hw,         show_size_menu       },
    {CON_AURA,       state_appearance, show_aura_menu       },
    {CON_QCLASS,     state_qclass,     display_classes      },
    {CON_CLASS_HELP, state_qclass,     display_classes_help },
    {CON_ANDROID,    state_android,    show_model_menu      },
    {CON_Q1,         state_question,   ask_question         },
    {CON_Q2,         state_question,   ask_question         },
    {CON_Q3,         state_question,   ask_question         },
    {CON_Q4,         state_question,   ask_question         },
    {CON_Q5,         state_question,   ask_question         },
    {CON_Q6,         state_question,   ask_question         },
    {CON_Q7,         state_question,   ask_question         },
    {CON_Q8,         state_question,   ask_question         },
    {CON_Q9,         state_question,   ask_question         },
    {CON_QX,         state_qx,         ask_question         },
    {CON_ALIGN,      state_align,      show_alignment_menu  },
    {CON_SKILLS,     state_skills,     show_skills_prompt   },
    {CON_GENOME,     state_genome                           },
    {CON_BONUS,      state_traits                           },
    {CON_NEGATIVE,   state_traits                           },
    {CON_QROLLSTATS, state_qrollstats                       },
    {CON_QSTATS,     state_qstats                           },
};

const std::span<const con_state> CHARGEN_STATES = chargen_states;
