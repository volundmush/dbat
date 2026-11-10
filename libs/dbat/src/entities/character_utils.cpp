#include <cstdlib>
#include <cstring>

#include "dbat/iterate.hpp"
#include "dbat/characters.hpp"
#include "dbat/utils.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/races.hpp"
#include "dbat/spells.hpp"
#include "dbat/comm.hpp"
#include "dbat/class.hpp"
#include "dbat/fight.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/relocate.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/db.hpp"
#include "dbat/random.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/extract.hpp"
#include "dbat/affect.hpp"
#include "dbat/fileop.hpp"
#include "dbat/genzon.hpp"
#include "dbat/feats.hpp"
#include "dbat/time.hpp"
#include "dbat/weather.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/positions.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/sex.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

static void char_adm_set(struct char_data *ch, int flag, bool on);
static bool char_has_legs(GameWorld *g, const struct char_data *ch);
static bool char_is_fullpssj(const struct char_data *ch);
static bool char_is_grap(GameWorld *g, const struct char_data *ch);
static bool char_is_inferior(const struct char_data *ch);
static bool char_is_male(const struct char_data *ch);
static bool char_is_oozaru_race(const struct char_data *ch);
static bool char_not_hidden(const struct char_data *ch);
static bool char_oozaru_ok(const struct char_data *ch);
static bool char_outside_roomflag(GameWorld *g, const struct char_data *ch);
static bool char_outside_secttype(GameWorld *g, const struct char_data *ch);
static bool char_spar_train(GameWorld *g, const struct char_data *ch);
static void cureStatusBurn(GameWorld *g, char_data *ch, bool announce);
static void cureStatusPoison(GameWorld *g, char_data *ch, bool announce);
static double getCurPLPercent(GameWorld *g, char_data *ch);
static int64_t getPercentOfMaxPL(GameWorld *g, char_data *ch, double amt);
static bool isFullPL(char_data *ch);
static bool mob_respond(GameWorld *g, struct char_data *ch, struct char_data *vict, const char *speech);
static void restoreLimbs(GameWorld *g, char_data *ch, bool announce);
static void restoreStatus(GameWorld *g, char_data *ch, bool announce);
static int sensei_grav_tolerance(int s_id);
static double speednar(GameWorld *g, struct char_data *ch);

#define ABS(x) ((x) < 0 ? -(x) : (x))

const char *juggleRaceName(char_data *ch, bool capitalized)
{
  int apparent = ch->race;

  switch (apparent) {
    case RACE_HOSHIJIN:
      if (ch->mimic > -1) apparent = ch->mimic;
      break;
    case RACE_HALFBREED:
      switch (char_get_racial_pref(ch)) {
        case 1:
          apparent = RACE_HUMAN;
          break;
        case 2:
          apparent = RACE_SAIYAN;
          break;
      }
      break;
    case RACE_ANDROID:
      switch (char_get_racial_pref(ch)) {
        case 1:
          apparent = RACE_ANDROID;
          break;
        case 2:
          apparent = RACE_HUMAN;
          break;
        case 3:
          if (capitalized) {
            return "Robotic-Humanoid";
          }
          return "robotic-humanoid";
      }
      break;
    case RACE_SAIYAN:
      if (char_plr_flagged(ch, PLR_TAILHIDE)) {
        apparent = RACE_HUMAN;
      }
      break;
  }

  return capitalized ? pc_race_types[apparent] : race_names[apparent];
}

void restore_by(GameWorld *g, char_data *ch, char_data *healer)
{
  restore(g, ch, true);
  act(g, "You have been fully healed by $N!", false, ch, nullptr, healer, TO_CHAR | TO_SLEEP);
}

void restore(GameWorld *g, char_data *ch, bool announce)
{
  restoreVitals(ch, announce);
  restoreLimbs(g, ch, announce);
  restoreStatus(g, ch, announce);
  restoreLF(ch, announce);
}

void resurrect(GameWorld *g, char_data *ch, int mode)
{
  restore(g, ch, true);
  char_aff_set(ch, AFF_ETHEREAL, false);
  char_aff_set(ch, AFF_SPIRIT, false);
  char_plr_set(ch, PLR_PDEATH, false);
  char_from_room(g, ch);
  if (char_get_droom(ch) != NOWHERE && char_get_droom(ch) != 0 && char_get_droom(ch) != 1) {
    char_to_room(g, ch, char_get_droom(ch));
  } else {
    char_to_room(g, ch, sensei_start_room(ch->chclass));
  }
  look_at_room(g, char_get_in_room(ch), ch, 0);

  int dur = 100;
  switch (mode) {
    case 1:
      return;
    case 0:
      dur = 40;
      break;
    case 2:
      dur = 50;
      break;
  }

  if (char_get_level(ch) > 9) {
    int losschance = axion_dice(g, 0);
    send_to_char(g, ch, "@RThe the strain of this type of revival has caused you to be in a weakened state for a few hours (Game time)! Strength, constitution, wisdom, intelligence, speed, and agility have been reduced by a fifth for the duration.@n\r\n");
    int str = -8, intel = -8, wis = -8, spd = -8, con = -8, agl = -8;
    str   = -1 * (ch->real_abils.str / 5);
    intel = -1 * (ch->real_abils.intel / 5);
    wis   = -1 * (ch->real_abils.wis / 5);
    spd   = -1 * (ch->real_abils.cha / 5);
    con   = -1 * (ch->real_abils.con / 5);
    agl   = -1 * (ch->real_abils.dex / 5);
    assign_affect(g, ch, AFF_WEAKENED_STATE, SKILL_WARP, dur, str, con, intel, agl, wis, spd);
    if (losschance >= 100) {
      int psloss = rand_number(g, 100, 300);
      char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (psloss));
      send_to_char(g, ch, "@R...and a loss of @r{:d}@R PS!@n", psloss);
      if (char_get_practice_points(g, ch) < 0) {
        char_set_practice_points(g, ch, 0);
      }
    }
  }
  char_set_deathtime(ch, 0);
  act(g, "$n's body forms in a pool of @Bblue light@n.", true, ch, nullptr, nullptr, TO_ROOM);
}

void ghostify(GameWorld *g, char_data *ch)
{
  restore(g, ch, true);
  char_aff_set(ch, AFF_SPIRIT, true);
  char_aff_set(ch, AFF_ETHEREAL, true);

  if (!char_prf_flagged(g, ch, PRF_LKEEP)) {
    if (char_plr_flagged(ch, PLR_CLLEG)) {
      char_plr_set(ch, PLR_CLLEG, false);
    }
    if (char_plr_flagged(ch, PLR_CRLEG)) {
      char_plr_set(ch, PLR_CRLEG, false);
    }
    if (char_plr_flagged(ch, PLR_CRARM)) {
      char_plr_set(ch, PLR_CRARM, false);
    }
    if (char_plr_flagged(ch, PLR_CLARM)) {
      char_plr_set(ch, PLR_CLARM, false);
    }
  }

  char_aff_set(ch, AFF_KNOCKED, false);
  char_aff_set(ch, AFF_SLEEP, false);
  char_aff_set(ch, AFF_PARALYZE, false);
}

void teleport_to(GameWorld *g, char_data *ch, IDXTYPE rnum)
{
  char_from_room(g, ch);
  char_to_room(g, ch, rnum);
  look_at_room(g, char_get_in_room(ch), ch, 0);
  update_pos(g, ch);
}

static bool in_room_range(GameWorld *g, char_data *ch, IDXTYPE low_rnum, IDXTYPE high_rnum)
{
  return char_get_in_room(ch) >= low_rnum && char_get_in_room(ch) <= high_rnum;
}

bool in_past(GameWorld *g, char_data *ch)
{
  return room_flagged(g, char_get_in_room(ch), ROOM_PAST);
}

bool is_newbie(char_data *ch)
{
  return char_get_level(ch) < 9;
}

bool in_northran(GameWorld *g, char_data *ch)
{
  return in_room_range(g, ch, 17900, 17999);
}

static const struct {
  int grav;
  int64_t threshold;
} grav_threshold[] = {
    {10,    5000     },
    {20,    20000    },
    {30,    50000    },
    {40,    100000   },
    {50,    200000   },
    {100,   400000   },
    {200,   1000000  },
    {300,   5000000  },
    {400,   8000000  },
    {500,   15000000 },
    {1000,  25000000 },
    {5000,  100000000},
    {10000, 200000000}
};

/* NOTE: truncated to uint16_t to match the original std::map<int, uint16_t>;
 * unknown gravities return 0 like std::map::operator[] did. */
static uint16_t grav_threshold_for(int grav)
{
  for (size_t i = 0; i < countof(grav_threshold); i++)
    if (grav_threshold[i].grav == grav)
      return (uint16_t)grav_threshold[i].threshold;
  return 0;
}

static bool can_tolerate_gravity(GameWorld *g, char_data *ch, int grav)
{
  if (char_is_npc(ch)) return true;
  int tolerance = 0;
  tolerance     = MAX(tolerance, sensei_grav_tolerance(ch->chclass));
  if (tolerance >= grav)
    return true;
  return getMaxPL(g, ch) >= grav_threshold_for(grav);
}

static int calcTier(char_data *ch)
{
  int tier = ch->level / 10;
  if ((ch->level % 10) == 0)
    tier--;
  tier = MAX(tier, 0);
  tier = MIN(tier, 9);
  return tier;
}

int64_t calc_soft_cap(char_data *ch)
{
  int tier               = calcTier(ch);
  const int64_t *softmap = race_soft_cap_table(ch);
  return ch->level * softmap[tier];
}

bool is_soft_cap(char_data *ch, int64_t type, long double mult)
{
  if (char_is_npc(ch))
    return true;

  if (ch->level >= 100) {
    return false;
  }
  int64_t cur_cap = calc_soft_cap(ch) * mult;

  int64_t against = 0;

  switch (race_soft_cap_type(ch)) {
    case SOFTCAP_FIXED:
      switch (type) {
        case 0:
          against = (getBasePL(ch));
          break;
        case 1:
          against = (getBaseKI(ch));
          break;
        case 2:
          against = (getBaseST(ch));
          break;
      }
      break;
    case SOFTCAP_VARIABLE:
      against = (getBasePL(ch)) + (getBaseKI(ch)) + (getBaseST(ch));
      if (char_is_android(ch) && type > 0) {
        cur_cap += type;
      }
      break;
  }
  return against >= cur_cap;
}

bool calcGravCost(GameWorld *g, char_data *ch, int64_t num)
{
  int cost = 0;
  if (!can_tolerate_gravity(g, ch, room_get_gravity(g, char_get_in_room(ch))))
    cost = room_get_gravity(g, char_get_in_room(ch)) ^ 2;

  if (!num) {
    if (cost) {
      send_to_char(g, ch, "You sweat bullets straining against the current gravity.\r\n");
    }
    if ((getCurST(g, ch)) > cost) {
      decCurST(g, ch, cost, 0);
      return true;
    }
    decCurST(g, ch, cost, 0);
    return false;
  }
  return (getCurST(g, ch)) > (cost + num);
}

int64_t getCurHealth(GameWorld *g, char_data *ch)
{
  return getCurPL(g, ch);
}

int64_t getMaxHealth(GameWorld *g, char_data *ch)
{
  return getMaxPL(g, ch);
}

double getCurHealthPercent(GameWorld *g, char_data *ch)
{
  return getCurPLPercent(g, ch);
}

int64_t getPercentOfMaxHealth(GameWorld *g, char_data *ch, double amt)
{
  return getPercentOfMaxPL(g, ch, amt);
}

bool isFullHealth(char_data *ch)
{
  return isFullPL(ch);
}

int64_t setCurHealth(char_data *ch, int64_t amt)
{
  /* the legacy hit field this stored into no longer exists */
  return MAX(0L, ABS(amt));
}

int64_t setCurHealthPercent(GameWorld *g, char_data *ch, double amt)
{
  ch->health = clampHealth(amt);
  return getCurHealth(g, ch);
}

int64_t incCurHealth(GameWorld *g, char_data *ch, int64_t amt, bool limit_max)
{
  double newhealth = ch->health + safeDiv((double)ABS(amt), (double)getEffMaxPL(g, ch));
  newhealth        = fixnan(newhealth);
  if (limit_max)
    ch->health = MIN(1.0, newhealth);
  else
    ch->health = newhealth;
  ch->health = clampHealth(ch->health);
  return getCurHealth(g, ch);
}

int64_t decCurHealth(GameWorld *g, char_data *ch, int64_t amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = safeDiv((double)floor, (double)getEffMaxPL(g, ch));
  double newhealth = ch->health - safeDiv((double)ABS(amt), (double)getEffMaxPL(g, ch));
  newhealth        = fixnan(newhealth);
  ch->health       = MAX(fl, newhealth);
  ch->health       = clampHealth(ch->health);
  return getCurHealth(g, ch);
}

int64_t incCurHealthPercent(GameWorld *g, char_data *ch, double amt, bool limit_max)
{
  ch->health += ABS(amt);
  if (limit_max)
    ch->health = MIN(1.0, ch->health);
  ch->health = fixnan(ch->health);
  return getCurHealth(g, ch);
}

int64_t decCurHealthPercent(GameWorld *g, char_data *ch, double amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = safeDiv((double)floor, (double)getEffMaxPL(g, ch));
  ch->health = clampHealth(MAX(fl, fixnan(ch->health - ABS(amt))));
  return getCurHealth(g, ch);
}

void restoreHealth(char_data *ch, bool announce)
{
  if (!isFullHealth(ch)) ch->health = 1;
}

int64_t getMaxPLTrans(GameWorld *g, char_data *ch)
{
  transform_bonus form = race_current_form(g, ch);
  int64_t total        = 0;
  if (form.flag) {
    total = (form.bonus + getEffBasePL(g, ch)) * form.mult;
  } else {
    total = getEffBasePL(g, ch) * form.mult;
  }
  total += ch->max_hit;
  return total;
}

int64_t getMaxPL(GameWorld *g, char_data *ch)
{
  int64_t total = getMaxPLTrans(g, ch);
  if (char_get_kaioken(ch) > 0) {
    total += (total / 10) * char_get_kaioken(ch);
  }
  if (char_aff_flagged(ch, AFF_METAMORPH)) {
    total += (total * .6);
  }
  return total;
}

int64_t getCurPL(GameWorld *g, char_data *ch)
{
  double health = clampHealth(ch->health);
  if (ch->suppression > 0) {
    return (int64_t)(getEffMaxPL(g, ch) * MIN(health, (double)ch->suppression / 100));
  }
  return (int64_t)(getEffMaxPL(g, ch) * health);
}

int64_t getEffBasePL(GameWorld *g, char_data *ch)
{
  if (char_get_original(g, ch)) return getEffBasePL(g, char_get_original(g, ch));
  if (ch->clones) {
    return getBasePL(ch) / (ch->clones + 1);
  }
  return getBasePL(ch);
}

int64_t getBasePL(char_data *ch)
{
  return ch->basepl;
}

static double getCurPLPercent(GameWorld *g, char_data *ch)
{
  return (double)getCurPL(g, ch) / (double)getMaxPL(g, ch);
}

static int64_t getPercentOfMaxPL(GameWorld *g, char_data *ch, double amt)
{
  return getMaxPL(g, ch) * ABS(amt);
}

static bool isFullPL(char_data *ch)
{
  return ch->health >= 1.0;
}

int64_t getCurKI(GameWorld *g, char_data *ch)
{
  return (int64_t)(getMaxKI(g, ch) * clampHealth(ch->energy));
}

int64_t getMaxKI(GameWorld *g, char_data *ch)
{
  transform_bonus form = race_current_form(g, ch);
  int64_t total        = 0;
  if (form.flag) {
    total = (form.bonus + getEffBaseKI(g, ch)) * form.mult;
  } else {
    total = getEffBaseKI(g, ch);
  }
  total += ch->max_ki;
  return total;
}

int64_t getEffBaseKI(GameWorld *g, char_data *ch)
{
  if (char_get_original(g, ch)) return getEffBaseKI(g, char_get_original(g, ch));
  if (ch->clones) {
    return getBaseKI(ch) / (ch->clones + 1);
  }
  return getBaseKI(ch);
}

int64_t getBaseKI(char_data *ch)
{
  return ch->baseki;
}

double getCurKIPercent(GameWorld *g, char_data *ch)
{
  return (double)getCurKI(g, ch) / (double)getMaxKI(g, ch);
}

int64_t getPercentOfMaxKI(GameWorld *g, char_data *ch, double amt)
{
  return getMaxKI(g, ch) * ABS(amt);
}

bool isFullKI(char_data *ch)
{
  return ch->energy >= 1.0;
}

int64_t setCurKIPercent(GameWorld *g, char_data *ch, double amt)
{
  ch->energy = clampHealth(amt);
  return getCurKI(g, ch);
}

int64_t incCurKI(GameWorld *g, char_data *ch, int64_t amt, bool limit_max)
{
  double newenergy = ch->energy + safeDiv((double)ABS(amt), (double)getMaxKI(g, ch));
  newenergy        = fixnan(newenergy);
  if (limit_max)
    ch->energy = MIN(1.0, newenergy);
  else
    ch->energy = newenergy;
  ch->energy = clampHealth(ch->energy);
  return getCurKI(g, ch);
}

int64_t decCurKI(GameWorld *g, char_data *ch, int64_t amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = safeDiv((double)floor, (double)getMaxKI(g, ch));
  double newenergy = ch->energy - safeDiv((double)ABS(amt), (double)getMaxKI(g, ch));
  newenergy        = fixnan(newenergy);
  ch->energy       = MAX(fl, newenergy);
  ch->energy       = clampHealth(ch->energy);
  return getCurKI(g, ch);
}

int64_t incCurKIPercent(GameWorld *g, char_data *ch, double amt, bool limit_max)
{
  if (limit_max)
    ch->energy = MIN(1.0, ch->energy + ABS(amt));
  else
    ch->energy += ABS(amt);
  return getCurKI(g, ch);
}

int64_t decCurKIPercent(GameWorld *g, char_data *ch, double amt, int64_t floor)
{
  if (!strcasecmp(ch->name.c_str(), "Wayland")) {
    send_to_char(g, ch, "decCurKIPercent called with: {:f}\r\n", amt);
  }
  double fl = 0.0;
  if (floor > 0)
    fl = (double)floor / (double)getMaxKI(g, ch);
  ch->energy = MAX(fl, ch->energy - ABS(amt));
  return getCurKI(g, ch);
}

void restoreKI(char_data *ch, bool announce)
{
  if (!isFullKI(ch)) ch->energy = 1;
}

int64_t getCurST(GameWorld *g, char_data *ch)
{
  return (int64_t)(getMaxST(g, ch) * clampHealth(ch->stamina));
}

int64_t getMaxST(GameWorld *g, char_data *ch)
{
  transform_bonus form = race_current_form(g, ch);
  int64_t total        = 0;
  if (form.flag) {
    total = (form.bonus + getEffBaseST(g, ch)) * form.mult;
  } else {
    total = getEffBaseST(g, ch);
  }
  total += ch->max_move;
  return total;
}

int64_t getEffBaseST(GameWorld *g, char_data *ch)
{
  if (char_get_original(g, ch)) return getEffBaseST(g, char_get_original(g, ch));
  if (ch->clones) {
    return getBaseST(ch) / (ch->clones + 1);
  }
  return getBaseST(ch);
}

int64_t getBaseST(char_data *ch)
{
  return ch->basest;
}

double getCurSTPercent(GameWorld *g, char_data *ch)
{
  return (double)getCurST(g, ch) / (double)getMaxST(g, ch);
}

bool isFullST(char_data *ch)
{
  return ch->stamina >= 1;
}

int64_t setCurSTPercent(GameWorld *g, char_data *ch, double amt)
{
  /* the legacy move field this stored into no longer exists */
  return MAX(0L, (int64_t)(getMaxST(g, ch) * ABS(amt)));
}

int64_t incCurST(GameWorld *g, char_data *ch, int64_t amt, bool limit_max)
{
  double newstamina = ch->stamina + safeDiv((double)ABS(amt), (double)getMaxST(g, ch));
  newstamina        = fixnan(newstamina);
  if (limit_max)
    ch->stamina = MIN(1.0, newstamina);
  else
    ch->stamina = newstamina;
  ch->stamina = clampHealth(ch->stamina);
  return getCurST(g, ch);
}

int64_t decCurST(GameWorld *g, char_data *ch, int64_t amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = safeDiv((double)floor, (double)getMaxST(g, ch));
  double newstamina = ch->stamina - safeDiv((double)ABS(amt), (double)getMaxST(g, ch));
  newstamina        = fixnan(newstamina);
  ch->stamina       = MAX(fl, newstamina);
  ch->stamina       = clampHealth(ch->stamina);
  return getCurST(g, ch);
}

int64_t incCurSTPercent(GameWorld *g, char_data *ch, double amt, bool limit_max)
{
  if (limit_max)
    ch->stamina = MIN(1.0, ch->stamina + ABS(amt));
  else
    ch->stamina += ABS(amt);
  return getMaxST(g, ch);
}

int64_t decCurSTPercent(GameWorld *g, char_data *ch, double amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = (double)floor / (double)getMaxST(g, ch);
  ch->stamina = MAX(fl, ch->stamina - ABS(amt));
  return getCurST(g, ch);
}

void restoreST(char_data *ch, bool announce)
{
  if (!isFullST(ch)) ch->stamina = 1;
}

int64_t getCurLF(GameWorld *g, char_data *ch)
{
  return (int64_t)(getMaxLF(g, ch) * clampHealth(ch->life));
}

int64_t getMaxLF(GameWorld *g, char_data *ch)
{
  return (char_is_demon(ch) ? (((getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5)) * 0.75) + char_get_lifebonuses(g, ch) : (char_is_konatsu(ch) ? (((getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5)) * 0.85) + char_get_lifebonuses(g, ch) : (getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5) + char_get_lifebonuses(g, ch)));
}

double getCurLFPercent(char_data *ch)
{
  return ch->life;
}

bool isFullLF(char_data *ch)
{
  return ch->life >= 1.0;
}

int64_t incCurLF(GameWorld *g, char_data *ch, int64_t amt, bool limit_max)
{
  double newlife = ch->life + safeDiv((double)ABS(amt), (double)getMaxLF(g, ch));
  newlife        = fixnan(newlife);
  if (limit_max)
    ch->life = MIN(1.0, newlife);
  else
    ch->life = newlife;
  ch->life = clampHealth(ch->life);
  return getCurLF(g, ch);
}

int64_t decCurLF(GameWorld *g, char_data *ch, int64_t amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = safeDiv((double)floor, (double)getMaxLF(g, ch));
  double newlife = ch->life - safeDiv((double)ABS(amt), (double)getMaxLF(g, ch));
  newlife        = fixnan(newlife);
  ch->life       = MAX(fl, newlife);
  ch->life       = clampHealth(ch->life);
  return getCurLF(g, ch);
}

int64_t incCurLFPercent(GameWorld *g, char_data *ch, double amt, bool limit_max)
{
  if (limit_max)
    ch->life = MIN(1.0, ch->life + ABS(amt));
  else
    ch->life += ABS(amt);
  return getCurLF(g, ch);
}

int64_t decCurLFPercent(GameWorld *g, char_data *ch, double amt, int64_t floor)
{
  double fl = 0.0;
  if (floor > 0)
    fl = (double)floor / (double)getMaxLF(g, ch);
  ch->life = MAX(fl, ch->life - ABS(amt));
  return getCurLF(g, ch);
}

void restoreLF(char_data *ch, bool announce)
{
  if (!isFullLF(ch)) ch->life = 1;
}

bool isFullVitals(char_data *ch)
{
  return isFullHealth(ch) && isFullKI(ch) && isFullST(ch);
}

void restoreVitals(char_data *ch, bool announce)
{
  restoreHealth(ch, announce);
  restoreKI(ch, announce);
  restoreST(ch, announce);
}

static void restoreStatus(GameWorld *g, char_data *ch, bool announce)
{
  cureStatusKnockedOut(g, ch, announce);
  cureStatusBurn(g, ch, announce);
  cureStatusPoison(g, ch, announce);
}

void setStatusKnockedOut(char_data *ch)
{
  char_aff_set(ch, AFF_KNOCKED, true);
  if (char_aff_flagged(ch, AFF_FLYING)) {
    char_aff_set(ch, AFF_FLYING, false);
    char_set_altitude(ch, 0);
  }
  char_set_position(ch, POS_SLEEPING);
}

void cureStatusKnockedOut(GameWorld *g, char_data *ch, bool announce)
{
  if (char_aff_flagged(ch, AFF_KNOCKED)) {
    if (announce) {
      act(g, "@W$n@W is no longer senseless, and wakes up.@n", false, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "You are no longer knocked out, and wake up!@n\r\n");
    }

    if (char_get_carried_by(g, ch)) {
      if (char_get_alignment(char_get_carried_by(g, ch)) > 50) {
        carry_drop(g, char_get_carried_by(g, ch), 0);
      } else {
        carry_drop(g, char_get_carried_by(g, ch), 1);
      }
    }

    char_aff_set(ch, AFF_KNOCKED, false);
    char_set_position(ch, POS_SITTING);
  }
}

static void cureStatusBurn(GameWorld *g, char_data *ch, bool announce)
{
  if (char_aff_flagged(ch, AFF_BURNED)) {
    if (announce) {
      send_to_char(g, ch, "Your burns are healed now.\r\n");
      act(g, "$n@w's burns are now healed.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    char_aff_set(ch, AFF_BURNED, false);
  }
}

static void cureStatusPoison(GameWorld *g, char_data *ch, bool announce)
{
  act(g, "@C$n@W suddenly looks a lot better!@b", false, ch, nullptr, nullptr, TO_NOTVICT);
  affect_from_char(g, ch, SPELL_POISON);
}

static const char *const limb_names[] = {
    nullptr,
    "right arm",
    "left arm",
    "right leg",
    "left leg"};

static void restoreLimbs(GameWorld *g, char_data *ch, bool announce)
{

  for (int l = 1; l <= 4; l++) {
    if (announce) {
      if (char_get_limb_condition(ch, (l)-1) <= 0)
        send_to_char(g, ch, "Your {} grows back!\r\n", limb_names[l]);
      else if (char_get_limb_condition(ch, (l)-1) < 50)
        send_to_char(g, ch, "Your {} is no longer broken!\r\n", limb_names[l]);
    }
    char_set_limb_condition(ch, (l)-1, 100);
  }

  char_gain_tail(g, ch, announce);
}

int64_t gainBasePL(char_data *ch, int64_t amt, bool trans_mult)
{
  ch->basepl += amt;
  return ch->basepl;
}

int64_t gainBaseST(char_data *ch, int64_t amt, bool trans_mult)
{
  ch->basest += amt;
  return ch->basest;
}

int64_t gainBaseKI(char_data *ch, int64_t amt, bool trans_mult)
{
  ch->baseki += amt;
  return ch->baseki;
}

int64_t loseBasePL(char_data *ch, int64_t amt, bool trans_mult)
{
  ch->basepl = MAX(1L, ch->basepl - amt);
  return ch->basepl;
}

int64_t gainBasePLPercent(char_data *ch, double amt, bool trans_mult)
{
  return gainBasePL(ch, ch->basepl * amt, trans_mult);
}

static int64_t gainBaseKIPercent(char_data *ch, double amt, bool trans_mult)
{
  return gainBaseKI(ch, ch->baseki * amt, trans_mult);
}

static int64_t gainBaseSTPercent(char_data *ch, double amt, bool trans_mult)
{
  return gainBaseST(ch, ch->basest * amt, trans_mult);
}

int64_t loseBasePLPercent(char_data *ch, double amt, bool trans_mult)
{
  return loseBasePL(ch, ch->basepl * amt, trans_mult);
}

void gainBaseAllPercent(char_data *ch, double amt, bool trans_mult)
{
  gainBasePLPercent(ch, amt, trans_mult);
  gainBaseKIPercent(ch, amt, trans_mult);
  gainBaseSTPercent(ch, amt, trans_mult);
}

int64_t getMaxCarryWeight(GameWorld *g, char_data *ch)
{
  return MAX(1L, (getMaxPL(g, ch) / 200) + (char_get_aff_abils_str(ch) * 50));
}

static int64_t getCurGearWeight(char_data *ch)
{
  int64_t total_weight = 0;

  for (int i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i)) {
      total_weight += obj_get_weight(char_get_equipment(ch, i));
    }
  }
  return total_weight;
}

int64_t getCurCarriedWeight(char_data *ch)
{
  return getCurGearWeight(ch) + ch->carry_weight;
}

// speednar is in utils.cpp

int64_t getEffMaxPL(GameWorld *g, char_data *ch)
{
  int64_t maxpl = getMaxPL(g, ch);
  if (char_is_npc(ch)) {
    return maxpl;
  }
  double snar = speednar(g, ch);
  return maxpl * (1.0 - snar);
}

bool isWeightedPL(GameWorld *g, char_data *ch)
{
  return getMaxPL(g, ch) > getEffMaxPL(g, ch);
}

void apply_kaioken(GameWorld *g, char_data *ch, int times, bool announce)
{
  char_set_kaioken(ch, times);
  char_plr_set(ch, PLR_POWERUP, false);

  if (announce) {
    send_to_char(g, ch, "@rA dark red aura bursts up around your body as you achieve Kaioken x {:d}!@n\r\n", times);
    act(g, "@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, ch, nullptr, nullptr, TO_ROOM);
  }
}

void remove_kaioken(GameWorld *g, char_data *ch, int8_t announce)
{
  int8_t kaio = char_get_kaioken(ch);
  if (!kaio) {
    return;
  }
  char_set_kaioken(ch, 0);

  switch (announce) {
    case 1:
      send_to_char(g, ch, "You drop out of kaioken.\r\n");
      act(g, "$n@w drops out of kaioken.@n", true, ch, nullptr, nullptr, TO_ROOM);
      break;
    case 2:
      send_to_char(g, ch, "You lose focus and your kaioken disappears.\r\n");
      act(g, "$n loses focus and $s kaioken aura disappears.", true, ch, nullptr, nullptr, TO_ROOM);
  }
}

void dispel_ash(GameWorld *g, struct char_data *ch)
{
  struct obj_data *ash = nullptr;

  for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_vnum(g, obj) == 1306) {
      ash = obj;
    }
  }

  if (ash) {
    int roll = axion_dice(g, 0);
    if (obj_get_cost(ash) == 3) {
      if (char_get_aff_abils_intel(ch) > roll) {
        act(g, "@GYou clear the air with the shockwaves of your power!@n", true, ch, ash, nullptr, TO_CHAR);
        act(g, "@C$n@G clears the air with the shockwaves of $s power!@n", true, ch, ash, nullptr, TO_ROOM);
        extract_obj(g, ash);
      }
    } else if (obj_get_cost(ash) == 2) {
      if (char_get_aff_abils_intel(ch) + 10 > roll) {
        act(g, "@GYou clear the air with the shockwaves of your power!@n", true, ch, ash, nullptr, TO_CHAR);
        act(g, "@C$n@G clears the air with the shockwaves of $s power!@n", true, ch, ash, nullptr, TO_ROOM);
        extract_obj(g, ash);
      }
    } else if (obj_get_cost(ash) == 1) {
      if (char_get_aff_abils_intel(ch) + 20 > roll) {
        act(g, "@GYou clear the air with the shockwaves of your power!@n", true, ch, ash, nullptr, TO_CHAR);
        act(g, "@C$n@G clears the air with the shockwaves of $s power!@n", true, ch, ash, nullptr, TO_ROOM);
        extract_obj(g, ash);
      }
    }
  }
}

bool has_group(GameWorld *g, struct char_data *ch)
{


  if (!char_aff_flagged(ch, AFF_GROUP))
    return (false);

  if (ch->followers) {
    for (char_data *fol : char_followers(g, ch)) {
      if (!char_aff_flagged(fol, AFF_GROUP)) {
        continue;
      }
      return (true);
    }
  } else if (char_get_master(g, ch)) {
    if (!char_aff_flagged(char_get_master(g, ch), AFF_GROUP))
      return (false);
    else
      return (true);
  }

  return (false);
}

const char *report_party_health(GameWorld *g, struct char_data *ch)
{

  if (!char_aff_flagged(ch, AFF_GROUP))
    return ("");

  if (!ch->followers && !char_get_master(g, ch))
    return ("");

  int count = 0, stam1 = 8, stam2 = 8, stam3 = 8, stam4 = 8, plc1 = 4, plc2 = 4, plc3 = 4, plc4 = 4;
  struct char_data *party1 = nullptr, *party2 = nullptr, *party3 = nullptr, *party4 = nullptr;
  int64_t plperc1 = 0, plperc2 = 0, plperc3 = 0, plperc4 = 0;
  int64_t kiperc1 = 0, kiperc2 = 0, kiperc3 = 0, kiperc4 = 0;
  std::string result1;
  std::string result2;
  std::string result3;
  std::string result4;
  std::string result5;

  const char *plcol[5] = {"@r",
                          "@y",
                          "@Y",
                          "@G",
                          ""};

  const char *exhaust[9] = {"Exhausted",   /* 0/7 */
                            "Strained",    /* 1/7 */
                            "Very Tired",  /* 2/7 */
                            "Tired",       /* 3/7 */
                            "Kinda Tired", /* 4/7 */
                            "Very Winded", /* 5/7 */
                            "Winded",      /* 6/7 */
                            "Energetic",   /* 7/7 */
                            "?????????"};

  const char *excol[9] = {"@r", /* 0/7 */
                          "@R", /* 1/7 */
                          "@R", /* 2/7 */
                          "@M", /* 3/7 */
                          "@M", /* 4/7 */
                          "@M", /* 5/7 */
                          "@G", /* 6/7 */
                          "@g", /* 7/7 */
                          "@w"};

  if (ch->followers) {
    for (char_data *fol : char_followers(g, ch)) {
      if (!char_aff_flagged(fol, AFF_GROUP))
        continue;
      if (fol != ch) {
        count += 1;
        if (count == 1) {
          party1  = fol;
          plperc1 = (getCurPL(g, party1) * 100) / getEffMaxPL(g, party1);
          kiperc1 = (char_get_charge(party1) * 100) / getMaxKI(g, party1);

          if (plperc1 >= 80)
            plc1 = 3;
          else if (plperc1 >= 50)
            plc1 = 2;
          else if (plperc1 >= 30)
            plc1 = 1;
          else
            plc1 = 0;

          if ((getCurST(g, party1)) >= getMaxST(g, party1)) {
            stam1 = 7;
          } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .9) {
            stam1 = 6;
          } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .8) {
            stam1 = 5;
          } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .7) {
            stam1 = 4;
          } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .5) {
            stam1 = 3;
          } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .4) {
            stam1 = 2;
          } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .2) {
            stam1 = 1;
          } else {
            stam1 = 0;
          }
        } else if (count == 2) {
          party2  = fol;
          plperc2 = (getCurPL(g, party2) * 100) / getEffMaxPL(g, party2);
          kiperc2 = (char_get_charge(party2) * 100) / getMaxKI(g, party2);

          if (plperc2 >= 80)
            plc2 = 3;
          else if (plperc2 >= 50)
            plc2 = 2;
          else if (plperc2 >= 30)
            plc2 = 1;
          else
            plc2 = 0;

          if ((getCurST(g, party2)) >= getMaxST(g, party2)) {
            stam2 = 7;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .9) {
            stam2 = 6;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .8) {
            stam2 = 5;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .7) {
            stam2 = 4;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .5) {
            stam2 = 3;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .4) {
            stam2 = 2;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .2) {
            stam2 = 1;
          } else {
            stam2 = 0;
          }
        } else if (count == 3) {
          party3  = fol;
          plperc3 = (getCurPL(g, party3) * 100) / getEffMaxPL(g, party3);
          kiperc3 = (char_get_charge(party3) * 100) / getMaxKI(g, party3);

          if (plperc3 >= 80)
            plc3 = 3;
          else if (plperc3 >= 50)
            plc3 = 2;
          else if (plperc3 >= 30)
            plc3 = 1;
          else
            plc3 = 0;

          if ((getCurST(g, party3)) >= getMaxST(g, party3)) {
            stam3 = 7;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .9) {
            stam3 = 6;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .8) {
            stam3 = 5;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .7) {
            stam3 = 4;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .5) {
            stam3 = 3;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .4) {
            stam3 = 2;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .2) {
            stam3 = 1;
          } else {
            stam3 = 0;
          }
        } else if (count == 4) {
          party4  = fol;
          plperc4 = (getCurPL(g, party4) * 100) / getEffMaxPL(g, party4);
          kiperc4 = (char_get_charge(party4) * 100) / getMaxKI(g, party4);

          if (plperc4 >= 80)
            plc4 = 3;
          else if (plperc4 >= 50)
            plc4 = 2;
          else if (plperc4 >= 30)
            plc4 = 1;
          else
            plc4 = 0;

          if ((getCurST(g, party4)) >= getMaxST(g, party4)) {
            stam4 = 7;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .9) {
            stam4 = 6;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .8) {
            stam4 = 5;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .7) {
            stam4 = 4;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .5) {
            stam4 = 3;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .4) {
            stam4 = 2;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .2) {
            stam4 = 1;
          } else {
            stam4 = 0;
          }
        }
      }
    }
    result1         = str_format("@D[@BG@D]-------@mF@D------- -------@mF@D------- -------@mF@D------- -------@mF@D-------[@BG@D] <@RV@Y{}@R>@n\n", add_commas(char_get_combatexpertise(ch)));
    result2         = str_format("@D[@BR@D]@C{:<15} {:<15} {:<15} {:<15}@D[@BR@D]@n\n", party1 ? get_i_name(ch, party1) : "Empty", party2 ? get_i_name(ch, party2) : "Empty", party3 ? get_i_name(ch, party3) : "Empty", party4 ? get_i_name(ch, party4) : "Empty");
    result3         = str_format("@D[@BO@D]@RPL@D:{}{:>11d}@w{} @RPL@D:{}{:>11d}@w{} @RPL@D:{}{:>11d}@w{} @RPL@D:{}{:>11d}@w{}@D[@BO@D]@n\n", plcol[plc1], plperc1, "%", plcol[plc2], plperc2, "%", plcol[plc3], plperc3, "%", plcol[plc4], plperc4, "%");
    result4         = str_format("@D[@BU@D]@cCharge@D:@B{:>7d}@w{} @cCharge@D:@B{:>7d}@w{} @cCharge@D:@B{:>7d}@w{} @cCharge@D:@B{:>7d}@w{}@D[@BU@D]@n\n", kiperc1, "%", kiperc2, "%", kiperc3, "%", kiperc4, "%");
    result5         = str_format("@D[@BP@D]@gSt@D:{}{:>12} @gSt@D:{}{:>12} @gSt@D:{}{:>12} @gSt@D:{}{:>12}@D[@BP@D]@n", excol[stam1], exhaust[stam1], excol[stam2], exhaust[stam2], excol[stam3], exhaust[stam3], excol[stam4], exhaust[stam4]);
    ch->temp_prompt = str_format("{}{}{}{}{}\n", result1, result2, result3, result4, result5);
    return ch->temp_prompt.c_str();
  }
  if (char_get_master(g, ch) && char_aff_flagged(char_get_master(g, ch), AFF_GROUP)) {
    party1  = char_get_master(g, ch);
    plperc1 = (getCurPL(g, party1) * 100) / getEffMaxPL(g, party1);
    kiperc1 = (char_get_charge(party1) * 100) / getMaxKI(g, party1);

    if (plperc1 >= 80)
      plc1 = 3;
    else if (plperc1 >= 50)
      plc1 = 2;
    else if (plperc1 >= 30)
      plc1 = 1;
    else
      plc1 = 0;

    if ((getCurST(g, party1)) >= getMaxST(g, party1)) {
      stam1 = 7;
    } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .9) {
      stam1 = 6;
    } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .8) {
      stam1 = 5;
    } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .7) {
      stam1 = 4;
    } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .5) {
      stam1 = 3;
    } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .4) {
      stam1 = 2;
    } else if ((getCurST(g, party1)) >= getMaxST(g, party1) * .2) {
      stam1 = 1;
    } else {
      stam1 = 0;
    }
    count = 1;

    for (char_data *foll : char_followers(g, party1)) {
      if (!char_aff_flagged(foll, AFF_GROUP))
        continue;
      if (foll != ch) {
        count += 1;
        if (count == 2) {
          party2  = foll;
          plperc2 = (getCurPL(g, party2) * 100) / getEffMaxPL(g, party2);
          kiperc2 = (char_get_charge(party2) * 100) / getMaxKI(g, party2);

          if (plperc2 >= 80)
            plc2 = 3;
          else if (plperc2 >= 50)
            plc2 = 2;
          else if (plperc2 >= 30)
            plc2 = 1;
          else
            plc2 = 0;

          if ((getCurST(g, party2)) >= getMaxST(g, party2)) {
            stam2 = 7;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .9) {
            stam2 = 6;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .8) {
            stam2 = 5;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .7) {
            stam2 = 4;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .5) {
            stam2 = 3;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .4) {
            stam2 = 2;
          } else if ((getCurST(g, party2)) >= getMaxST(g, party2) * .2) {
            stam2 = 1;
          } else {
            stam2 = 0;
          }
        } else if (count == 3) {
          party3  = foll;
          plperc3 = (getCurPL(g, party3) * 100) / getEffMaxPL(g, party3);
          kiperc3 = (char_get_charge(party3) * 100) / getMaxKI(g, party3);

          if (plperc3 >= 80)
            plc3 = 3;
          else if (plperc3 >= 50)
            plc3 = 2;
          else if (plperc3 >= 30)
            plc3 = 1;
          else
            plc3 = 0;

          if ((getCurST(g, party3)) >= getMaxST(g, party3)) {
            stam3 = 7;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .9) {
            stam3 = 6;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .8) {
            stam3 = 5;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .7) {
            stam3 = 4;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .5) {
            stam3 = 3;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .4) {
            stam3 = 2;
          } else if ((getCurST(g, party3)) >= getMaxST(g, party3) * .2) {
            stam3 = 1;
          } else {
            stam3 = 0;
          }
        } else if (count == 4) {
          party4  = foll;
          plperc4 = (getCurPL(g, party4) * 100) / getEffMaxPL(g, party4);
          kiperc4 = (char_get_charge(party4) * 100) / getMaxKI(g, party4);

          if (plperc4 >= 80)
            plc4 = 3;
          else if (plperc4 >= 50)
            plc4 = 2;
          else if (plperc4 >= 30)
            plc4 = 1;
          else
            plc4 = 0;

          if ((getCurST(g, party4)) >= getMaxST(g, party4)) {
            stam4 = 7;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .9) {
            stam4 = 6;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .8) {
            stam4 = 5;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .7) {
            stam4 = 4;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .5) {
            stam4 = 3;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .4) {
            stam4 = 2;
          } else if ((getCurST(g, party4)) >= getMaxST(g, party4) * .2) {
            stam4 = 1;
          } else {
            stam4 = 0;
          }
        }
      } /* Is follower */
    } /* End for */

    result1         = "@D[@BG@D]-------@YL@D------- -------@mF@D------- -------@mF@D------- -------@mF@D-------[@BG@D]@n\n";
    result2         = str_format("@D[@BR@D]@C{:<15} {:<15} {:<15} {:<15}@D[@BR@D]@n\n", party1 ? get_i_name(ch, party1) : "Empty", party2 ? get_i_name(ch, party2) : "Empty", party3 ? get_i_name(ch, party3) : "Empty", party4 ? get_i_name(ch, party4) : "Empty");
    result3         = str_format("@D[@BO@D]@RPL@D:{}{:>11d}@w{} @RPL@D:{}{:>11d}@w{} @RPL@D:{}{:>11d}@w{} @RPL@D:{}{:>11d}@w{}@D[@BO@D]@n\n", plcol[plc1], plperc1, "%", plcol[plc2], plperc2, "%", plcol[plc3], plperc3, "%", plcol[plc4], plperc4, "%");
    result4         = str_format("@D[@BU@D]@cCharge@D:@B{:>7d}@w{} @cCharge@D:@B{:>7d}@w{} @cCharge@D:@B{:>7d}@w{} @cCharge@D:@B{:>7d}@w{}@D[@BU@D]@n\n", kiperc1, "%", kiperc2, "%", kiperc3, "%", kiperc4, "%");
    result5         = str_format("@D[@BP@D]@gSt@D:{}{:>12} @gSt@D:{}{:>12} @gSt@D:{}{:>12} @gSt@D:{}{:>12}@D[@BP@D]@n", excol[stam1], exhaust[stam1], excol[stam2], exhaust[stam2], excol[stam3], exhaust[stam3], excol[stam4], exhaust[stam4]);
    ch->temp_prompt = str_format("{}{}{}{}{}\n", result1, result2, result3, result4, result5);
    return ch->temp_prompt.c_str();
  }
  return ("");
}

/* Check to see if anything interferes with their "knowing" the skill */
bool know_skill(GameWorld *g, struct char_data *ch, int skill)
{

  int know = 0;

  if (char_get_skill(ch, skill) > 0)
    know = 1;

  if (know == 0) {
    send_to_char(g, ch, "You do not know how to perform {}.\r\n", g->spell_info[skill].name);
    know = 0;
  }

  return (know);
}

bool is_affected(struct char_data *ch, int aff_flag)
{

  for (const struct affected_type &af : ch->affected) {
    if (af.location == APPLY_NONE && af.bitvector == (bitvector_t)aff_flag)
      return (true);
  }

  return (false);
}

void null_affect(GameWorld *g, struct char_data *ch, int aff_flag)
{

  for (auto it = ch->affected.begin(); it != ch->affected.end();) {
    struct affected_type *af = &*it++;
    if (af->location == APPLY_NONE && af->bitvector == (bitvector_t)aff_flag)
      affect_remove(g, ch, af);
  }
}

void remove_affect(GameWorld *g, struct char_data *ch, int aff_flag)
{

  for (auto it = ch->affected.begin(); it != ch->affected.end();) {
    struct affected_type *af = &*it++;
    if (af->bitvector == (bitvector_t)aff_flag)
      affect_remove(g, ch, af);
  }
}

void assign_affect(GameWorld *g, struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd)
{
  struct affected_type af[6] = {};
  int num = 0;

  if (str == 0 && con == 0 && wis == 0 && intel == 0 && agl == 0 && spd == 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = 0;
    af[num].location  = APPLY_NONE;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
  if (str != 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = str;
    af[num].location  = APPLY_STR;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
  if (con != 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = con;
    af[num].location  = APPLY_CON;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
  if (intel != 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = intel;
    af[num].location  = APPLY_INT;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
  if (agl != 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = agl;
    af[num].location  = APPLY_DEX;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
  if (spd != 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = spd;
    af[num].location  = APPLY_CHA;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
  if (wis != 0) {
    af[num].type      = skill;
    af[num].duration  = dur;
    af[num].modifier  = wis;
    af[num].location  = APPLY_WIS;
    af[num].bitvector = aff_flag;
    affect_join(g, ch, &af[num], false, false, false, false);
    num += 1;
  }
}

bool sec_roll_check(GameWorld *g, struct char_data *ch)
{

  int figure = 0, chance = 0, outcome = 0;

  figure = 10 + (char_get_level(ch) * 1.6);

  chance = axion_dice(g, 0) + axion_dice(g, 0) + rand_number(g, 0, 20);

  if (figure >= chance)
    outcome = 1;

  return (outcome);
}

int get_measure(struct char_data *ch, int height, int weight)
{
  int amt = 0;
  if (!char_plr_flagged(ch, PLR_OOZARU) && (!char_is_icer(ch) || !char_is_transformed(ch)) && char_get_genome(ch, 0) < 9) {
    if (height > 0) {
      amt = height;
    } else if (weight > 0) {
      amt = weight;
    }
  } else if (char_is_icer(ch) && char_plr_flagged(ch, PLR_TRANS1)) {
    if (height > 0) {
      amt = height * 3;
    } else if (weight > 0) {
      amt = weight * 4;
    }
  } else if (char_is_icer(ch) && char_plr_flagged(ch, PLR_TRANS2)) {
    if (height > 0) {
      amt = height * 3;
    } else if (weight > 0) {
      amt = weight * 5;
    }
  } else if (char_is_icer(ch) && char_plr_flagged(ch, PLR_TRANS3)) {
    if (height > 0) {
      amt = height * 1.5;
    } else if (weight > 0) {
      amt = weight * 2;
    }
  } else if (char_is_icer(ch) && char_plr_flagged(ch, PLR_TRANS4)) {
    if (height > 0) {
      amt = height * 2;
    } else if (weight > 0) {
      amt = weight * 3;
    }
  } else if (char_plr_flagged(ch, PLR_OOZARU) || char_get_genome(ch, 0) == 9) {
    if (height > 0) {
      amt = height * 10;
    } else if (weight > 0) {
      amt = weight * 50;
    }
  }

  return (amt);
}

int64_t physical_cost(GameWorld *g, struct char_data *ch, int skill)
{

  int64_t result = 0;

  if (skill == SKILL_PUNCH)
    result = getEffMaxPL(g, ch) / 500;
  else if (skill == SKILL_KICK)
    result = getEffMaxPL(g, ch) / 350;
  else if (skill == SKILL_ELBOW)
    result = getEffMaxPL(g, ch) / 400;
  else if (skill == SKILL_KNEE)
    result = getEffMaxPL(g, ch) / 300;
  else if (skill == SKILL_UPPERCUT)
    result = getEffMaxPL(g, ch) / 200;
  else if (skill == SKILL_ROUNDHOUSE)
    result = getEffMaxPL(g, ch) / 150;
  else if (skill == SKILL_HEELDROP)
    result = getEffMaxPL(g, ch) / 80;
  else if (skill == SKILL_SLAM)
    result = getEffMaxPL(g, ch) / 90;

  int cou1 = 1 + rand_number(g, 1, 20), cou2 = cou1 + rand_number(g, 1, 6);

  result += rand_number(g, cou1, cou2);

  if (char_get_skills_base(ch, SKILL_STYLE) >= 100) {
    result -= result * 0.4;
  } else if (char_get_skills_base(ch, SKILL_STYLE) >= 75) {
    if (char_is_tsuna(ch)) {
      result -= result * 0.40;
    } else if (char_is_tapion(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK)) {
      result -= result * 0.35;
    } else if (char_is_jinto(ch)) {
      if (char_get_skills_base(ch, skill) >= 100) {
        result -= result * 0.45;
      } else {
        result -= result * 0.25;
      }
    } else {
      result -= result * 0.25;
    }
  } else if (char_get_skills_base(ch, SKILL_STYLE) >= 50) {
    result -= result * 0.25;
  }

  if (char_is_android(ch)) {
    result *= 0.25;
  }

  return (result);
}

/* This returns the trans requirement for the player based on their trans class */
/* 3 = Great, 2 = Average, 1 = Terrible */
/* Rillao: transloc, add new transes here */
int trans_req(struct char_data *ch, int trans)
{

  int requirement = 0;

  if (char_is_human(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 1500000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1800000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 2100000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 37500000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 35000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 32500000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 200000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 190000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 185000000;
        }
        break;
      case 4:
        if (char_get_transclass(ch) == 1) {
          requirement = 1400000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1200000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1100000000;
        }
        break;
    }
  } /* End Human Requirement */

  if (char_is_halfbreed(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 1500000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1400000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1200000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 60000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 55000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 50000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 1200000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1050000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 950000000;
        }
        break;
    }
  } /* End Halfbreed Requirement */

  if (char_is_saiyan(ch)) {
    if (char_plr_flagged(ch, PLR_LSSJ)) {
      switch (trans) {
        case 1:
          if (char_get_transclass(ch) == 1) {
            requirement = 600000;
          } else if (char_get_transclass(ch) == 2) {
            requirement = 500000;
          } else if (char_get_transclass(ch) == 3) {
            requirement = 450000;
          }
          break;
        case 2:
          if (char_get_transclass(ch) == 1) {
            requirement = 300000000;
          } else if (char_get_transclass(ch) == 2) {
            requirement = 250000000;
          } else if (char_get_transclass(ch) == 3) {
            requirement = 225000000;
          }
          break;
      }
    } else {
      switch (trans) {
        case 1:
          if (char_get_transclass(ch) == 1) {
            requirement = 1400000;
          } else if (char_get_transclass(ch) == 2) {
            requirement = 1200000;
          } else if (char_get_transclass(ch) == 3) {
            requirement = 1100000;
          }
          break;
        case 2:
          if (char_get_transclass(ch) == 1) {
            requirement = 60000000;
          } else if (char_get_transclass(ch) == 2) {
            requirement = 55000000;
          } else if (char_get_transclass(ch) == 3) {
            requirement = 50000000;
          }
          break;
        case 3:
          if (char_get_transclass(ch) == 1) {
            requirement = 160000000;
          } else if (char_get_transclass(ch) == 2) {
            requirement = 150000000;
          } else if (char_get_transclass(ch) == 3) {
            requirement = 140000000;
          }
          break;
        case 4:
          if (char_get_transclass(ch) == 1) {
            requirement = 1800000000;
          } else if (char_get_transclass(ch) == 2) {
            requirement = 1625000000;
          } else if (char_get_transclass(ch) == 3) {
            requirement = 1400000000;
          }
          break;
      }
    }
  } /* End Saiyan Requirement */

  if (char_is_namek(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 400000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 360000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 335000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 10000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 9500000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 8000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 240000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 220000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 200000000;
        }
        break;
      case 4:
        if (char_get_transclass(ch) == 1) {
          requirement = 950000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 900000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 875000000;
        }
        break;
    }
  } /* End Namek Requirement */

  if (char_is_icer(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 550000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 500000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 450000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 20000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 17500000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 15000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 180000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 150000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 125000000;
        }
        break;
      case 4:
        if (char_get_transclass(ch) == 1) {
          requirement = 880000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 850000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 820000000;
        }
        break;
    }
  } /* End Icer Requirement */

  if (char_is_majin(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 2400000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 2200000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 2000000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 50000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 45000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 40000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 1800000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1550000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1300000000;
        }
        break;
    }
  } /* End Majin Requirement */

  if (char_is_truffle(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 3800000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 3600000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 3500000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 400000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 300000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 200000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 1550000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1450000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1250000000;
        }
        break;
    }
  } /* End Truffle Requirement */

  if (char_is_mutant(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 200000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 180000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 160000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 30000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 27500000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 25000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 750000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 700000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 675000000;
        }
        break;
    }
  } /* End Mutant Requirement */

  if (char_is_kai(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 3250000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 3000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 2850000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 700000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 650000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 625000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 1500000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1300000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1250000000;
        }
        break;
    }
  } /* End Kai Requirement */

  if (char_is_konatsu(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 2000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1800000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1600000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 250000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 225000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 200000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 1600000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1400000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1300000000;
        }
        break;
    }
  } /* End Konatsu Requirement */

  if (char_is_android(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 1200000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 850000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 8500000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 8000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 7750000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 55000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 50000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 40000000;
        }
        break;
      case 4:
        if (char_get_transclass(ch) == 1) {
          requirement = 325000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 300000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 275000000;
        }
        break;
      case 5:
        if (char_get_transclass(ch) == 1) {
          requirement = 900000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 800000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 750000000;
        }
        break;
      case 6:
        if (char_get_transclass(ch) == 1) {
          requirement = 1300000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1200000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1100000000;
        }
        break;
    }
  } /* End Android Requirement */

  if (char_is_bio(ch)) {
    switch (trans) {
      case 1:
        if (char_get_transclass(ch) == 1) {
          requirement = 2000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1800000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1700000;
        }
        break;
      case 2:
        if (char_get_transclass(ch) == 1) {
          requirement = 30000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 25000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 20000000;
        }
        break;
      case 3:
        if (char_get_transclass(ch) == 1) {
          requirement = 235000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 220000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 210000000;
        }
        break;
      case 4:
        if (char_get_transclass(ch) == 1) {
          requirement = 1500000000;
        } else if (char_get_transclass(ch) == 2) {
          requirement = 1300000000;
        } else if (char_get_transclass(ch) == 3) {
          requirement = 1150000000;
        }
        break;
    }
  } /* End Bioandroid Requirement */

  return (requirement);
}

const char *disp_align(struct char_data *ch)
{
  int align;

  if (char_get_alignment(ch) < -800) { // Horrifically Evil
    align = 8;
  } else if (char_get_alignment(ch) < -600) { // Terrible
    align = 7;
  } else if (char_get_alignment(ch) < -300) { // Villain
    align = 6;
  } else if (char_get_alignment(ch) < -50) { // Crook
    align = 5;
  } else if (char_get_alignment(ch) < 51) { // Neutral
    align = 4;
  } else if (char_get_alignment(ch) < 300) { // Do-gooder
    align = 3;
  } else if (char_get_alignment(ch) < 600) { // Hero
    align = 2;
  } else if (char_get_alignment(ch) < 800) { // Valiant
    align = 1;
  } else { // Saintly
    align = 0;
  }

  return (alignments[align]);
}

/* This creates a player's sense memory file for the first time. */
void senseCreate(struct char_data *ch)
{
  struct atomic_file af;
  char fname[40];
  FILE *fl;
  /* Write Sense File */
  if (!get_filename(fname, sizeof(fname), SENSE_FILE, char_get_name(ch)))
    return;

  if (!(fl = atomic_open(&af, fname, true))) {
    mud_log("ERROR: could not save sense memory of, {}, to filename, {}.", char_get_name(ch), fname);
    return;
  }

  fprintf(fl, "0\n");

  atomic_commit(&af);
  return;
}

int read_sense_memory(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  char fname[40], line[256];
  int known       = false;
  mob_vnum idnums = -1337;
  FILE *fl;

  /* Read Sense File */
  if (vict == nullptr) {
    mud_log("Noone.");
    return 0;
  }

  if (!get_filename(fname, sizeof(fname), SENSE_FILE, char_get_name(ch))) {
    senseCreate(ch);
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
    sscanf(line, "%d\n", &idnums);
    if (char_is_npc(vict)) {
      if (idnums == char_get_mob_vnum(g, vict)) {
        known = true;
      }
    } else {
      if (idnums == char_get_id(vict)) {
        known = true;
      }
    }
  }
  fclose(fl);

  if (known == true)
    return 1;
  else
    return 0;
}

/* This writes a player's sense memory to file. */
void sense_memory_write(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  struct atomic_file af;
  FILE *file;
  char fname[40], line[256];
  int idnums[500] = {0};
  FILE *fl;
  int count = 0, x = 0, id_sample;

  /* Read Sense File */
  if (!get_filename(fname, sizeof(fname), SENSE_FILE, char_get_name(ch))) {
    senseCreate(ch);
  }
  if (!(file = fopen(fname, "r"))) {
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%d\n", &id_sample);
    idnums[count] = id_sample;
    count++;
  }
  fclose(file);

  /* Write Sense File */

  if (!get_filename(fname, sizeof(fname), SENSE_FILE, char_get_name(ch)))
    return;

  if (!(fl = atomic_open(&af, fname, true))) {
    mud_log("ERROR: could not save sense memory file, {}, to filename, {}.", char_get_name(ch), fname);
    return;
  }

  while (x < count) {
    if (x == 0 || idnums[x - 1] != idnums[x]) {
      if (!char_is_npc(vict)) {
        if (idnums[x] != char_get_id(vict)) {
          fprintf(fl, "%d\n", idnums[x]);
        }
      } else {
        if (idnums[x] != char_get_mob_vnum(g, vict)) {
          fprintf(fl, "%d\n", idnums[x]);
        }
      }
    }
    x++;
  }

  if (!char_is_npc(vict)) {
    fprintf(fl, "%d\n", char_get_id(vict));
  } else {
    fprintf(fl, "%d\n", char_get_mob_vnum(g, vict));
  }

  atomic_commit(&af);
  return;
}

/* Will they manage to pursue a fleeing enemy? */
bool roll_pursue(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  int skill, perc = axion_dice(g, 0);

  if (ch == nullptr || vict == nullptr)
    return (false);

  if (!char_is_npc(ch)) {
    skill = char_get_skill(ch, SKILL_PURSUIT);
  } else if (char_is_npc(ch) && !char_mob_flagged(ch, MOB_SENTINEL)) {
    skill = char_get_level(ch);
    if (room_flagged(g, char_get_in_room(vict), ROOM_NOMOB))
      skill = -1;
  } else {
    skill = -1;
  }

  if (!char_is_npc(vict)) {
    if (char_is_npc(ch) && !(vict->desc)) {
      skill = -1;
    }
  }

  if (skill > perc) {
    room_vnum inroom = char_get_in_room(ch);
    act(g, "@C$n@R pursues after the fleeing @c$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
    char_from_room(g, ch);
    char_to_room(g, ch, char_get_in_room(vict));
    act(g, "@GYou pursue right after @c$N@G!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@C$n@R pursues after you!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@C$n@R pursues after the fleeing @c$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);


    if (ch->followers) {
      for (char_data *fol : char_followers(g, ch)) {
        if ((char_get_in_room(fol) == inroom) && (char_get_position(fol) >= POS_STANDING) && (!char_aff_flagged(ch, AFF_ZANZOKEN) || (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(fol, AFF_GROUP)))) {
          act(g, "You follow $N.", true, fol, nullptr, ch, TO_CHAR);
          act(g, "$n follows after $N.", true, fol, nullptr, ch, TO_NOTVICT);
          act(g, "$n follows after you.", true, fol, nullptr, ch, TO_VICT);
          char_from_room(g, fol);
          char_to_room(g, fol, char_get_in_room(ch));
        }
      }
    }
    char_aff_set(vict, AFF_PURSUIT, false);
    return (true);
  }
  send_to_char(g, ch, "@RYou fail to pursue after them!@n\r\n");
  if (char_get_fighting(g, ch)) {
    stop_fighting(g, ch);
  }
  if (char_get_fighting(g, vict)) {
    stop_fighting(g, vict);
  }
  return (false);
}

const char *sense_location(GameWorld *g, struct char_data *ch)
{
  const char *message = ""; /* a place none of the cases name */
  room_vnum roomnum = char_get_in_room(ch);
  int num           = zone_place(g, find_zone_by_thing(g, roomnum)); /* by place, as when zones were an array */

  switch (num) {
    case 2:
      message = "East of Nexus City";
      break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      if (roomnum < 795)
        message = "Nexus City";
      else
        message = "South Ocean";
      break;
    case 8:
    case 9:
    case 10:
    case 11:
      if (roomnum < 1133)
        message = "South Ocean";
      else if (roomnum < 1179)
        message = "Nexus Field";
      else
        message = "Cherry Blossom Mountain";
      break;
    case 12:
    case 13:
      if (roomnum < 1287)
        message = "Cherry Blossom Mountain";
      else
        message = "Sandy Desert";
      break;
    case 14:
      if (roomnum < 1428)
        message = "Sandy Desert";
      else if (roomnum < 1484)
        message = "Northern Plains";
      else if (roomnum < 1496)
        message = "Korin's Tower";
      else
        message = "Kami's Lookout";
      break;
    case 15:
      if (roomnum < 1577)
        message = "Kami's Lookout";
      else if (roomnum < 1580)
        message = "Northern Plains";
      else if (roomnum < 1589)
        message = "Kami's Lookout";
      else
        message = "Shadow Forest";
      break;
    case 16:
      message = "Shadow Forest";
      break;
    case 17:
    case 18:
      if (roomnum < 1715)
        message = "Decrepit Area";
      else
        message = "Inside Cherry Blossom Mountain";
      break;
    case 19:
      message = "West City";
      break;
    case 20:
      if (roomnum < 2012)
        message = "West City";
      else if (roomnum > 2070)
        message = "West City";
      else
        message = "Silver Mine";
      break;
    case 21:
      if (roomnum < 2141)
        message = "West City";
      else
        message = "Hercule Beach";
      break;
    case 22:
      message = "Vegetos City";
      break;
    case 23:
    case 24:
      if (roomnum < 2334)
        message = "Vegetos City";
      else if (roomnum > 2462)
        message = "Vegetos City";
      else
        message = "Vegetos Palace";
      break;
    case 25:
    case 26:
      if (roomnum < 2616)
        message = "Blood Dunes";
      else
        message = "Ancestral Mountains";
      break;
    case 27:
      if (roomnum < 2709)
        message = "Ancestral Mountains";
      else if (roomnum < 2736)
        message = "Destopa Swamp";
      else
        message = "Swamp Base";
      break;
    case 28:
      message = "Pride Forest";
      break;
    case 29:
    case 30:
    case 31:
      message = "Pride Tower";
      break;
    case 32:
      message = "Ruby Cave";
      break;
    case 34:
      message = "Utatlan City";
      break;
    case 35:
      message = "Zenith Jungle";
      break;
    case 40:
    case 41:
    case 42:
      message = "Ice Crown City";
      break;
    case 43:
      if (roomnum < 4351)
        message = "Ice Highway";
      else
        message = "Topica Snowfield";
      break;
    case 44:
    case 45:
      message = "Glug's Volcano";
      break;
    case 46:
    case 47:
      message = "Platonic Sea";
      break;
    case 48:
      message = "Slave City";
      break;
    case 49:
      if (roomnum < 4915)
        message = "Descent Down Icecrown";
      else if (roomnum != 4915 && roomnum < 4994)
        message = "Topica Snowfield";
      else
        message = "Ice Highway";
      break;
    case 50:
      message = "Mirror Shard Maze";
      break;
    case 51:
      if (roomnum < 5150)
        message = "Acturian Woods";
      else if (roomnum < 5165)
        message = "Desolate Demesne";
      else
        message = "Chateau Ishran";
      break;
    case 52:
      message = "Wyrm Spine Mountain";
      break;
    case 53:
    case 54:
      message = "Aromina Hunting Preserve";
      break;
    case 55:
      message = "Cloud Ruler Temple";
      break;
    case 56:
      message = "Koltoan Mine";
      break;
    case 78:
      message = "Orium Cave";
      break;
    case 79:
      message = "Crystalline Forest";
      break;
    case 80:
    case 81:
    case 82:
      message = "Tiranoc City";
      break;
    case 83:
      message = "Great Oroist Temple";
      break;
    case 84:
      if (roomnum < 8447)
        message = "Elsthuan Forest";
      else
        message = "Mazori Farm";
      break;
    case 85:
      message = "Dres";
      break;
    case 86:
      message = "Colvian Farm";
      break;
    case 87:
      message = "Saint Alucia";
      break;
    case 88:
      if (roomnum < 8847)
        message = "Meridius Memorial";
      else
        message = "Battlefields";
      break;
    case 89:
      if (roomnum < 8954)
        message = "Desert of Illusion";
      else
        message = "Plains of Confusion";
      break;
    case 90:
      message = "Shadowlas Temple";
      break;
    case 92:
      message = "Turlon Fair";
      break;
    case 97:
      message = "Wetlands";
      break;
    case 98:
      if (roomnum < 9855)
        message = "Wetlands";
      else if (roomnum < 9866)
        message = "Kerberos";
      else
        message = "Shaeras Mansion";
      break;
    case 99:
      if (roomnum < 9907)
        message = "Slavinos Ravine";
      else if (roomnum < 9960)
        message = "Kerberos";
      else
        message = "Furian Citadel";
      break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
      message = "Namekian Wilderness";
      break;
    case 116:
      if (roomnum < 11672)
        message = "Senzu Village";
      else if (roomnum > 11672 && roomnum < 11698)
        message = "Senzu Village";
      else
        message = "Guru's House";
      break;
    case 117:
    case 118:
    case 119:
      message = "Crystalline Cave";
      break;
    case 120:
      message = "Haven City";
      break;
    case 121:
      if (roomnum < 12103)
        message = "Haven City";
      else
        message = "Serenity Lake";
      break;
    case 122:
      message = "Serenity Lake";
      break;
    case 123:
      message = "Kaiju Forest";
      break;
    case 124:
      if (roomnum < 12480)
        message = "Ortusian Temple";
      else
        message = "Silent Glade";
      break;
    case 125:
      message = "Near Serenity Lake";
      break;
    case 130:
    case 131:
      if (roomnum < 13153)
        message = "Satan City";
      else if (roomnum == 13153)
        message = "West City";
      else if (roomnum == 13154)
        message = "Nexus City";
      else
        message = "South Ocean";
      break;
    case 132:
      if (roomnum < 13232)
        message = "Frieza's Ship";
      else
        message = "Namekian Wilderness";
      break;
    case 133:
      message = "Elder Village";
      break;
    case 134:
      message = "Satan City";
      break;
    case 140:
      message = "Yardra City";
      break;
    case 141:
      message = "Jade Forest";
      break;
    case 142:
      message = "Jade Cliff";
      break;
    case 143:
      message = "Mount Valaria";
      break;
    case 149:
    case 150:
      message = "Aquis City";
      break;
    case 151:
    case 152:
    case 153:
      message = "Kanassan Ocean";
      break;
    case 154:
      message = "Kakureta Village";
      break;
    case 155:
      message = "Captured Aether City";
      break;
    case 156:
      message = "Yunkai Pirate Base";
      break;
    case 160:
    case 161:
      message = "Janacre";
      break;
    case 165:
      message = "Arlian Wasteland";
      break;
    case 166:
      message = "Arlian Mine";
      break;
    case 167:
      message = "Kilnak Caverns";
      break;
    case 168:
      message = "Kemabra Wastes";
      break;
    case 169:
      message = "Dark of Arlia";
      break;
    case 174:
      message = "Fistarl Volcano";
      break;
    case 175:
    case 176:
      message = "Cerria Colony";
      break;
    case 182:
      message = "Below Tiranoc";
      break;
    case 196:
      message = "Ancient Castle";
      break;
    default:
      message = "Unknown.";
      break;
  }

  return message;
}

void reveal_hiding(GameWorld *g, struct char_data *ch, int type)
{
  if (char_is_npc(ch) || !char_aff_flagged(ch, AFF_HIDE))
    return;

  int rand1 = rand_number(g, -5, 5), rand2 = rand_number(g, -5, 5), bonus = 0;

  if (char_aff_flagged(ch, AFF_LIQUEFIED)) {
    bonus = 10;
  }

  if (type == 0) { /* Automatic reveal. */
    act(g, "@MYou feel as though what you just did may have revealed your hiding spot...@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n moves a little and you notice them!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_aff_set(ch, AFF_HIDE, false);
  } else if (type == 1) { /* Their skill at hiding failed, reveal */
    if (char_get_skill(ch, SKILL_HIDE) + bonus < axion_dice(g, 0)) {
      act(g, "@MYou feel as though what you just did may have revealed your hiding spot...@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@M$n moves a little and you notice them!@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_aff_set(ch, AFF_HIDE, false);
    }
  } else if (type == 2) { /* They were spotted */
    struct char_data *tch = nullptr;
    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING)
        continue;
      tch = d->character;

      if (tch == ch)
        continue;

      if (char_get_in_room(tch) != char_get_in_room(ch))
        continue;

      if (char_get_skill(tch, SKILL_SPOT) + rand1 >= char_get_skill(ch, SKILL_HIDE) + rand2) {
        char_aff_set(ch, AFF_HIDE, false);
        act(g, "@M$N seems to notice you!@n", true, ch, nullptr, tch, TO_CHAR);
        act(g, "@MYou notice $n's movements reveal $s hiding spot!@n", true, ch, nullptr, tch, TO_VICT);
        act(g, "@MYou notice $N look keenly somewhere nearby. At that spot you see $n hiding!@n", true, ch, nullptr, tch, TO_NOTVICT);
        return;
      }
    }
  } else if (type == 3) { /* They were heard, reveal with different messages. */
    struct char_data *tch = nullptr;

    act(g, "@MThe scouter makes some beeping sounds as you tinker with its buttons.@n", true, ch, nullptr, nullptr, TO_CHAR);
    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING)
        continue;
      tch = d->character;

      if (tch == ch)
        continue;

      if (char_get_in_room(tch) != char_get_in_room(ch))
        continue;

      if (char_get_skill(tch, SKILL_LISTEN) > axion_dice(g, 0)) {
        switch (type) {
          case 3:
            act(g, "@MYou notice some beeping sounds that sound really close by.@n", true, ch, nullptr, tch, TO_VICT);
            break;
          default:
            act(g, "@MYou notice some sounds coming from this room but can't seem to locate the source...@n", true, ch, nullptr, tch, TO_VICT);
            break;
        }
      }
    }
  } else if (type == 4) { /* You were found from search! */
    char_aff_set(ch, AFF_HIDE, false);
  }
}

bool block_calc(GameWorld *g, struct char_data *ch)
{
  struct char_data *blocker = nullptr;

  if (char_get_blocked(g, ch)) {
    blocker = char_get_blocked(g, ch);
  } else {
    return true;
  }

  if (char_get_speedi(g, ch) < char_get_speedi(g, blocker) && char_get_position(blocker) > POS_SITTING) {
    if (!char_aff_flagged(blocker, AFF_BLIND) && !char_plr_flagged(blocker, PLR_EYEC)) {
      int minimum = char_get_aff_abils_cha(blocker) + rand_number(g, 5, 20);
      if (minimum > 100) {
        minimum = 100;
      }
      if (!char_get_skill(ch, SKILL_ESCAPE_ARTIST) || (char_get_skill(ch, SKILL_ESCAPE_ARTIST) && char_get_skill(ch, SKILL_ESCAPE_ARTIST) < rand_number(g, minimum, 120))) {
        act(g, "$n tries to leave, but can't outrun $N!", true, ch, nullptr, blocker, TO_NOTVICT);
        act(g, "$n tries to leave, but can't outrun you!", true, ch, nullptr, blocker, TO_VICT);
        act(g, "You try to leave, but can't outrun $N!", true, ch, nullptr, blocker, TO_CHAR);
        if (char_aff_flagged(ch, AFF_FLYING) && !char_aff_flagged(blocker, AFF_FLYING) && char_get_altitude(ch) == 1) {
          send_to_char(g, blocker, "You're now floating in the air.\r\n");
          char_aff_set(blocker, AFF_FLYING, true);
          char_set_altitude(blocker, char_get_altitude(ch));
        } else if (char_aff_flagged(ch, AFF_FLYING) && !char_aff_flagged(blocker, AFF_FLYING) && char_get_altitude(ch) == 2) {
          send_to_char(g, blocker, "You're now floating high in the sky.\r\n");
          char_aff_set(blocker, AFF_FLYING, true);
          char_set_altitude(blocker, char_get_altitude(ch));
        }
        return false;
      }
      act(g, "$n proves $s great skill and escapes from $N's attempted block!", true, ch, nullptr, blocker, TO_NOTVICT);
      act(g, "$n proves $s great skill and escapes from your attempted block!", true, ch, nullptr, blocker, TO_VICT);
      act(g, "Using your great skill you manage to escape from $N's attempted block!", true, ch, nullptr, blocker, TO_CHAR);
      char_set_blocked(ch, nullptr);
      char_set_blocks(blocker, nullptr);
    } else {
      act(g, "$n proves $s great skill and escapes from $N's attempted block!", true, ch, nullptr, blocker, TO_NOTVICT);
      act(g, "$n proves $s great skill and escapes from your attempted block!", true, ch, nullptr, blocker, TO_VICT);
      act(g, "Using your great skill you manage to escape from $N's attempted block!", true, ch, nullptr, blocker, TO_CHAR);
      char_set_blocked(ch, nullptr);
      char_set_blocks(blocker, nullptr);
    }
  } else if (char_get_position(blocker) <= POS_SITTING) {
    act(g, "$n proves $s great skill and escapes from $N!", true, ch, nullptr, blocker, TO_NOTVICT);
    act(g, "$n proves $s great skill and escapes from you!", true, ch, nullptr, blocker, TO_VICT);
    act(g, "Using your great skill you manage to escape from $N!", true, ch, nullptr, blocker, TO_CHAR);
    char_set_blocked(ch, nullptr);
    char_set_blocks(blocker, nullptr);
  } else if (char_get_position(blocker) > POS_SITTING) {
    act(g, "$n proves $s great skill and escapes from $N's attempted block!", true, ch, nullptr, blocker, TO_NOTVICT);
    act(g, "$n proves $s great skill and escapes from your attempted block!", true, ch, nullptr, blocker, TO_VICT);
    act(g, "Using your great skill you manage to escape from $N's attempted block!", true, ch, nullptr, blocker, TO_CHAR);
    char_set_blocked(ch, nullptr);
    char_set_blocks(blocker, nullptr);
  }

  return true;
}

int64_t molt_threshold(struct char_data *ch)
{

  int64_t threshold = 0, max = 2000000000;

  max *= 250;

  if (!char_is_arlian(ch))
    return (0);
  else if (char_get_moltlevel(ch) < 100) {
    threshold = (((char_get_moltlevel(ch) + 1) * (getBasePL(ch) * 0.02)) * char_get_aff_abils_con(ch)) / 4;
    threshold = threshold * 0.25;
  } else if (char_get_moltlevel(ch) < 200) {
    threshold = (((char_get_moltlevel(ch) + 1) * (getBasePL(ch) * 0.02)) * char_get_aff_abils_con(ch)) / 2;
    threshold = threshold * 0.20;
  } else if (char_get_moltlevel(ch) < 400) {
    threshold = (((char_get_moltlevel(ch) + 1) * (getBasePL(ch) * 0.02)) * char_get_aff_abils_con(ch));
    threshold = threshold * 0.17;
  } else if (char_get_moltlevel(ch) < 800) {
    threshold = (((char_get_moltlevel(ch) + 1) * (getBasePL(ch) * 0.02)) * char_get_aff_abils_con(ch)) * 2;
    threshold = threshold * 0.15;
  } else {
    threshold = (((char_get_moltlevel(ch) + 1) * (getBasePL(ch) * 0.02)) * char_get_aff_abils_con(ch)) * 4;
    threshold = threshold * 0.12;
  }

  return MIN(threshold, max);
}

static int armor_evolve(struct char_data *ch)
{
  int value = 0;

  if (char_get_moltlevel(ch) <= 5) {
    value = 8;
  } else if (char_get_moltlevel(ch) <= 10) {
    value = 12;
  } else if (char_get_moltlevel(ch) <= 20) {
    value = 15;
  } else if (char_get_moltlevel(ch) <= 30) {
    value = 20;
  } else if (char_get_moltlevel(ch) <= 40) {
    value = 30;
  } else if (char_get_moltlevel(ch) <= 50) {
    value = 50;
  } else if (char_get_moltlevel(ch) <= 75) {
    value = 100;
  } else if (char_get_moltlevel(ch) <= 100) {
    value = 150;
  } else if (char_get_moltlevel(ch) <= 500) {
    value = 200;
  } else {
    value = 220;
  }

  return (value);
}

/* This handles arlian exoskeleton molting */
void handle_evolution(GameWorld *g, struct char_data *ch, int64_t dmg)
{

  /* Reject NPCs and non-arlians */
  if (char_is_npc(ch) || !char_is_arlian(ch)) {
    return;
  }

  int64_t moltgain = 0;

  moltgain = dmg * 0.5;
  if (char_get_level(ch) == 100)
    moltgain += 100000;
  else if (char_get_level(ch) >= 90)
    moltgain += char_get_level(ch) * 1000;
  else if (char_get_level(ch) >= 75)
    moltgain += char_get_level(ch) * 500;
  else if (char_get_level(ch) >= 50)
    moltgain += char_get_level(ch) * 250;
  else if (char_get_level(ch) >= 10)
    moltgain += char_get_level(ch) * 50;

  char_set_moltexp(ch, char_get_moltexp(ch) + (moltgain));

  if (char_aff_flagged(ch, AFF_SPIRIT)) {
    send_to_char(g, ch, "You are dead and all evolution experience is reduced. Gains are divided by 100 or reduced to a minimum of 1.\r\n");
    moltgain /= 100;
  }

  if (char_get_moltexp(ch) > molt_threshold(ch)) {
    if (char_get_moltlevel(ch) <= char_get_level(ch) * 2 || char_get_level(ch) >= 100) {
      char_set_moltexp(ch, 0);
      char_set_moltlevel(ch, char_get_moltlevel(ch) + (1));
      double rand1 = 0.02;
      double rand2 = 0.03;
      if (rand_number(g, 1, 4) == 3) {
        rand1 += 0.02;
        rand2 += 0.02;
      } else if (rand_number(g, 1, 4) >= 3) {
        rand1 += 0.01;
        rand2 += 0.01;
      }
      int armorgain  = 0;
      int64_t plgain = getBasePL(ch) * rand1, stamgain = getBaseST(ch) * rand2;
      armorgain = armor_evolve(ch);
      gainBasePL(ch, plgain, false);
      gainBaseST(ch, stamgain, false);
      char_set_armor(ch, char_get_armor(ch) + (armorgain));
      if (char_get_armor(ch) > 50000) {
        char_set_armor(ch, 50000);
      }
      act(g, "@gYour @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. You quickly shed it and reveal a stronger version that was growing beneath it! At the same time you feel your adrenal sacs to be more efficient@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@G$n's@g @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. Suddenly $e sheds the damaged @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn and reveals a stronger version that had been growing underneath!@n", true, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "@D[@RPL@W: @G+{}@D] [@gStamina@W: @G+{}@D] [@wArmor Index@W: @G+{}@D]@n\r\n", add_commas(plgain), add_commas(stamgain), char_get_armor(ch) >= 50000 ? "50k CAP" : add_commas(armorgain));
    } else {
      send_to_char(g, ch, "@gYou are unable to evolve while your evolution level is higher than twice your character level.@n\r\n");
    }
  }
}

void demon_refill_lf(GameWorld *g, struct char_data *ch, int64_t num)
{
  for (char_data *tch : room_people(g, char_get_in_room(ch))) {
    if (!char_is_demon(tch))
      continue;
    if ((getCurLF(g, tch)) >= (getMaxLF(g, tch)))
      continue;
    else {
      incCurLF(g, ch, num, true);
      act(g, "@CYou feel the life energy from @c$N@C's cursed body flow out and you draw it into yourself!@n", true, tch, nullptr, ch, TO_CHAR);
    }
  }
}

void mob_talk(GameWorld *g, struct char_data *ch, const char *speech)
{

  struct char_data *vict = nullptr;
  int stop = 1;

  if (char_is_npc(ch)) {
    return;
  }

  for (char_data *tch : room_people(g, char_get_in_room(ch))) {
    if (!char_is_npc(tch))
      continue;
    if (!char_is_humanoid(tch))
      continue;
    if (stop == 0)
      continue;
    else {
      vict = tch;
      stop = mob_respond(g, ch, vict, speech);
      if (rand_number(g, 1, 2) == 2) {
        stop = 0;
      }
    }
  } /* End for loop */
} /* End Mob Talk */

static bool mob_respond(GameWorld *g, struct char_data *ch, struct char_data *vict, const char *speech)
{
  if (ch != nullptr && vict != nullptr) {
    if (!char_is_npc(ch) && char_is_npc(vict)) {
      if ((strstr(speech, "hello") || strstr(speech, "greet") || strstr(speech, "Hello") || strstr(speech, "Greet")) && !char_get_fighting(g, vict)) {
        send_to_room(g, char_get_in_room(vict), "\r\n");
        if (char_is_human(vict) || char_is_halfbreed(vict)) {
          switch (rand_number(g, 1, 4)) {
            case 1:
              act(g, "@w$n@W says, '@CYes, hello to you as well $N.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CHello!@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 3:
              act(g, "@w$n@W says, '@CHi, $N, how are you doing?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 4:
              act(g, "@w$n@W says, '@CGreetings, $N. What are you up to?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End Human Section */
        else if (char_is_saiyan(vict)) {
          switch (rand_number(g, 1, 4)) {
            case 1:
              act(g, "@w$n@W says, '@CHmph, hi.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CHello weakling.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 3:
              act(g, "@w$n@W says, '@C$N do all weaklings like you waste time in idle talk?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 4:
              act(g, "@w$n@W says, '@C$N, you are not welcome around me.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End Saiyan Section */
        else if (char_is_icer(vict)) {
          switch (rand_number(g, 1, 4)) {
            case 1:
              act(g, "@w$n@W says, '@CHa ha... Yes, hello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CAh a polite greeting. It's good to know your kind isn't totally worthless.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 3:
              act(g, "@w$n@W says, '@C$N, hello. Now leave me be.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 4:
              act(g, "@w$n@W says, '@C$N, you are below me. Now begone.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End Icer Section */
        else if (char_is_konatsu(vict)) {
          switch (rand_number(g, 1, 4)) {
            case 1:
              act(g, "@w$n@W says, '@CGreetings, $N, may your travels be well.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CHello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 3:
              act(g, "@w$n@W says, '@C$N, hello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 4:
              act(g, "@w$n@W says, '@C$N, it is nice to meet you.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End Konatsu Section */
        else if (char_is_namek(vict)) {
          switch (rand_number(g, 1, 4)) {
            case 1:
              act(g, "@w$n@W says, '@CHello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CA peaceful greeting to you, $N.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 3:
              act(g, "@w$n@W says, '@C$N, hello. What is your business here?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 4:
              act(g, "@w$n@W says, '@C$N, greetings.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End Namek Section */
        else if (char_is_arlian(vict)) {
          switch (rand_number(g, 1, 4)) {
            case 1:
              act(g, "@w$n@W says, '@CPeace, stranger.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CStay out of my way.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 3:
              act(g, "@w$n@W says, '@C$N, what is your business here?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 4:
              act(g, "@w$n@W says, '@C...Hello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End Arlian Section */
        else if (char_is_android(vict)) {
          act(g, "@w$n@W says, '@C...@W'@n", true, vict, nullptr, ch, TO_ROOM);
        } /* End Android Section */
        else if (char_is_majin(vict)) {
          switch (rand_number(g, 1, 2)) {
            case 1:
              act(g, "@w$n@W says, '@CHa ha...@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
            case 2:
              act(g, "@w$n@W says, '@CHello. What candy you want to be?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              break;
          }
        } /* End MAJIN Section */
        else if (char_is_truffle(vict)) {
          switch (rand_number(g, 1, 3)) {
            case 1:
              if (char_is_saiyan(ch)) {
                act(g, "@w$n@W says, '@CEwww, dirty monkey...@W'@n", true, vict, nullptr, ch, TO_ROOM);
              } else {
                act(g, "@w$n@W says, '@CHello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              }
              break;
            case 2:
              if (char_is_saiyan(ch)) {
                act(g, "@w$n@W says, '@CEwww, dirty monkey...@W'@n", true, vict, nullptr, ch, TO_ROOM);
              } else {
                act(g, "@w$n@W says, '@C$N, hello. You are a curious individual.@W'@n", true, vict, nullptr, ch, TO_ROOM);
              }
              break;
            case 3:
              if (char_is_saiyan(ch)) {
                act(g, "@w$n@W says, '@CEwww, dirty monkey...@W'@n", true, vict, nullptr, ch, TO_ROOM);
              } else {
                act(g, "@w$n@W says, '@C$N, hello. What's your IQ?@W'@n", true, vict, nullptr, ch, TO_ROOM);
              }
              break;
          }
        } /* End Truffle Section */
        else {
          act(g, "Hmph, yeah hi.", true, vict, nullptr, ch, TO_ROOM);
        }
      } /* End Hello Section */

      if ((strstr(speech, "spar") || strstr(speech, "Spar")) && !char_get_fighting(g, vict)) {
        send_to_room(g, char_get_in_room(vict), "\r\n");

        if (char_get_original(g, vict) == ch) {
          act(g, "@w$n@W says, '@C$N, sure. I'll spar with you.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          char_mob_set(vict, MOB_SPAR, true);
          return false;
        }

        if (char_get_level(vict) > 4 && char_get_alignment(vict) >= 0) {
          int remember = char_get_mob_specials_memory(vict).contains(char_get_idnum(ch));

          if (remember == true) {
            act(g, "@w$n@W says, '@C$N you will die by my hand!@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (char_mob_flagged(vict, MOB_NOKILL)) {
            act(g, "@w$n@W says, '@C$N, I have no need to spar with you.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (char_mob_flagged(vict, MOB_AGGRESSIVE)) {
            act(g, "@w$n@W says, '@C$N, I will kill you instead.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (char_mob_flagged(vict, MOB_DUMMY)) {
            act(g, "@w$n@W says, '@C...@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (getEffMaxPL(g, ch) > getEffMaxPL(g, vict) * 2) {
            act(g, "@w$n@W says, '@C$N, no way will I spar. I already know I would lose badly.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (getEffMaxPL(g, vict) > getEffMaxPL(g, ch) * 2) {
            act(g, "@w$n@W says, '@C$N, you wouldn't last very long.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (getCurPL(g, vict) < getEffMaxPL(g, vict) * .8) {
            act(g, "@w$n@W says, '@C$N, I need to recover first.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (rand_number(g, 1, 50) >= 40 && !char_mob_flagged(vict, MOB_SPAR)) {
            act(g, "@w$n@W says, '@C$N, maybe in a bit.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            return true;
          }
          if (char_mob_flagged(vict, MOB_SPAR)) {
            act(g, "@w$n@W says, '@C$N, fine our match will wait till later then.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            char_mob_set(vict, MOB_SPAR, false);
          } else {
            act(g, "@w$n@W says, '@C$N, sure. I'll spar with you.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            char_mob_set(vict, MOB_SPAR, true);
          }
          return false;
        }
        if (char_get_level(vict) > 4 && char_get_alignment(vict) < 0) {
          act(g, "@w$n@W says, '@CSpar? I don't play games, I play for blood...@W'@n", true, vict, nullptr, ch, TO_ROOM);
          return true;
        }
        act(g, "@w$n@W says, '@CSpar? I prefer not to thank you...@W'@n", true, vict, nullptr, ch, TO_ROOM);
        return true;
      } /* End challenge section */
      if (strstr(speech, "goodbye") || strstr(speech, "Goodbye") || strstr(speech, "bye") || strstr(speech, "Bye")) {
        send_to_room(g, char_get_in_room(vict), "\r\n");
        if (char_get_alignment(vict) >= 0) {
          if (char_get_sex(vict) == SEX_MALE) {
            if (char_get_sex(ch) == SEX_FEMALE) {
              act(g, "@w$n@W says, '@C$N, goodbye babe.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            } else {
              act(g, "@w$n@W says, '@C$N, goodbye.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            }
          } else if (char_get_sex(vict) == SEX_FEMALE) {
            if (char_get_sex(ch) == SEX_MALE) {
              act(g, "@w$n@W says, '@C$N, goodbye...@W'@n", true, vict, nullptr, ch, TO_ROOM);
            } else {
              act(g, "@w$n@W says, '@C$N, bye.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            }
          } else {
            act(g, "@w$n@W says, '@C$N, goodbye.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
        }
        if (char_get_alignment(vict) < 0) {
          if (char_get_sex(vict) == SEX_MALE) {
            if (char_get_sex(ch) == SEX_FEMALE) {
              act(g, "@w$n@W says, '@CGoodbye. Eh heh heh.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            } else {
              act(g, "@w$n@W says, '@CSo long and good ridance.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            }
          } else if (char_get_sex(vict) == SEX_FEMALE) {
            if (char_get_sex(ch) == SEX_MALE) {
              act(g, "@w$n@W says, '@CGoodbye then...@W'@n", true, vict, nullptr, ch, TO_ROOM);
            } else {
              act(g, "@w$n@W says, '@C$N, no one wanted you around anyway.@W'@n", true, vict, nullptr, ch, TO_ROOM);
            }
          } else {
            act(g, "@w$n@W says, '@CFine get lost.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
        }
      } /* End goodbye If */
      if (strstr(speech, "train") || strstr(speech, "Train") || strstr(speech, "exercise") || strstr(speech, "Exercise")) {
        send_to_room(g, char_get_in_room(vict), "\r\n");
        if (char_get_alignment(vict) >= 0 && !char_mob_flagged(vict, MOB_NOKILL)) {
          if (char_get_level(vict) > 4 && char_get_level(vict) < 10) {
            act(g, "@w$n@W says, '@CTraining is good for the body. I think I may need to go workout myself.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) >= 10 && char_get_level(vict) < 30) {
            act(g, "@w$n@W says, '@CI think I might need a little more training...@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) >= 30 && char_get_level(vict) < 60) {
            act(g, "@w$n@W says, '@CI'm pretty tough already. Though I should probably work on my skills.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) >= 60) {
            act(g, "@w$n@W says, '@CI'm on top of my game.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) < 5) {
            act(g, "@w$n@W says, '@CI really need to bust ass and train.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
        }
        if (char_get_alignment(vict) < 0 && !char_mob_flagged(vict, MOB_NOKILL)) {
          if (char_get_level(vict) > 4 && char_get_level(vict) < 10) {
            act(g, "@w$n@W says, '@CWell maybe I could use some more training.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) >= 10 && char_get_level(vict) < 30) {
            act(g, "@w$n@W says, '@CTrain? Yeah it has become harder to take what I want....@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) >= 30 && char_get_level(vict) < 60) {
            act(g, "@w$n@W says, '@CTrain? I don't need to train to take you!@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) >= 60) {
            act(g, "@w$n@W says, '@CTraining won't save you when I tire of your continued life.@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
          if (char_get_level(vict) < 5) {
            act(g, "@w$n@W says, '@CYes. I need to train so I can reach the top. Then everyone will have to listen to me!@W'@n", true, vict, nullptr, ch, TO_ROOM);
          }
        }
      }
      return true;
    }
  } /* End Valid targets Loop. */
  return true;
}

bool is_sparring(struct char_data *ch)
{

  if (char_is_npc(ch) && char_mob_flagged(ch, MOB_SPAR)) {
    return true;
  }
  if (!char_is_npc(ch) && char_plr_flagged(ch, PLR_SPAR)) {
    return true;
  }
  return false;
}

const char *introd_calc(struct char_data *ch)
{
  static std::string intro; /* what is returned, until the next call */
  const char *sex, *race;

  if (char_is_npc(ch)) { /* How the hell... */
    return ("IAMERROR");
  }

  sex  = char_mafe(ch);
  race = juggleRaceName(ch, true);
  if (char_is_halfbreed(ch)) {
    if (char_get_racial_pref(ch) == 1)
      race = "human";
    else if (char_get_racial_pref(ch) == 2)
      race = "saiyan";
  } else if (char_is_android(ch)) {
    if (char_get_racial_pref(ch) == 1)
      race = "android";
    else if (char_get_racial_pref(ch) == 2)
      race = "human";
    else if (char_get_racial_pref(ch) == 3)
      race = "robotic-humanoid";
  }
  intro = str_format("{} {} {}", AN(sex), sex, race);
  return intro.c_str();
}

static double speednar(GameWorld *g, struct char_data *ch)
{

  double result     = 0;
  int64_t carried   = getCurCarriedWeight(ch);
  int64_t can_carry = getMaxCarryWeight(g, ch);

  if (carried >= can_carry) {
    result = 1.0;
  } else if (carried >= can_carry * 0.95) {
    result = 0.95;
  } else if (carried >= can_carry * 0.9) {
    result = 0.90;
  } else if (carried >= can_carry * 0.85) {
    result = 0.85;
  } else if (carried >= can_carry * 0.80) {
    result = 0.80;
  } else if (carried >= can_carry * 0.75) {
    result = 0.75;
  } else if (carried >= can_carry * 0.70) {
    result = 0.70;
  } else if (carried >= can_carry * 0.65) {
    result = 0.65;
  } else if (carried >= can_carry * 0.60) {
    result = 0.60;
  } else if (carried >= can_carry * 0.55) {
    result = 0.55;
  } else if (carried >= can_carry * 0.50) {
    result = 0.50;
  } else if (carried >= can_carry * 0.45) {
    result = 0.45;
  } else if (carried >= can_carry * 0.40) {
    result = 0.40;
  } else if (carried >= can_carry * 0.35) {
    result = 0.35;
  } else if (carried >= can_carry * 0.30) {
    result = 0.30;
  } else if (carried >= can_carry * 0.25) {
    result = 0.25;
  } else if (carried >= can_carry * 0.20) {
    result = 0.20;
  } else if (carried >= can_carry * 0.15) {
    result = 0.15;
  } else if (carried >= can_carry * 0.10) {
    result = 0.10;
  } else if (carried >= can_carry * 0.05) {
    result = 0.05;
  } else if (carried >= can_carry * 0.01) {
    result = 0.01;
  }

  return (result);
}

int64_t gear_exp(GameWorld *g, struct char_data *ch, int64_t exp)
{

  if (char_is_npc(ch)) {
    return 0;
  }

  double ratio = 2 * speednar(g, ch);
  ratio += 1.0;
  return exp * ratio;
}

bool planet_check(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  if (ch == nullptr) {
    mud_log("ERROR: planet_check called without ch!");
    return false;
  }
  if (vict == nullptr) {
    mud_log("ERROR: planet_check called without vict!");
    return false;
  }
  int success = 0;
  if (char_get_admlevel(vict) <= 0) {
    if (room_flagged(g, char_get_in_room(ch), ROOM_EARTH) && room_flagged(g, char_get_in_room(vict), ROOM_EARTH)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_FRIGID) && room_flagged(g, char_get_in_room(vict), ROOM_FRIGID)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_NAMEK) && room_flagged(g, char_get_in_room(vict), ROOM_NAMEK)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_VEGETA) && room_flagged(g, char_get_in_room(vict), ROOM_VEGETA)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_AETHER) && room_flagged(g, char_get_in_room(vict), ROOM_AETHER)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_KONACK) && room_flagged(g, char_get_in_room(vict), ROOM_KONACK)) {
      success = 1;
    } else if (room_planet_zenith(g, char_get_in_room(ch)) && room_planet_zenith(g, char_get_in_room(vict))) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_KANASSA) && room_flagged(g, char_get_in_room(vict), ROOM_KANASSA)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_YARDRAT) && room_flagged(g, char_get_in_room(vict), ROOM_YARDRAT)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_AL) && room_flagged(g, char_get_in_room(vict), ROOM_AL)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_HELL) && room_flagged(g, char_get_in_room(vict), ROOM_HELL)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_ARLIA) && room_flagged(g, char_get_in_room(vict), ROOM_ARLIA)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_NEO) && room_flagged(g, char_get_in_room(vict), ROOM_NEO)) {
      success = 1;
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_CERRIA) && room_flagged(g, char_get_in_room(vict), ROOM_CERRIA)) {
      success = 1;
    }
  }
  return (success);
}

void purge_homing(GameWorld *g, struct char_data *ch)
{

  for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_vnum(g, obj) == 80 || obj_get_vnum(g, obj) == 81) {
      if (obj_get_target(g, obj) == ch || obj_get_user(g, obj) == ch) {
        act(g, "$p @wloses its target and flies off into the distance.@n", true, nullptr, obj, nullptr, TO_ROOM);
        extract_obj(g, obj);
        continue;
      }
    }
  }
}

void improve_skill(GameWorld *g, struct char_data *ch, int skill, int num)
{
  int percent = char_get_skill(ch, skill);
  int newpercent, roll = 1200;
  std::string skillbuf;

  if (char_is_npc(ch))
    return;

  if (!num) {
    num = 2;
  }

  if (char_aff_flagged(ch, AFF_SHOCKED))
    return;

  if (char_get_forgeting(ch) == skill)
    return;

  if (char_get_skills_base(ch, skill) >= 90) {
    roll += 800;
  } else if (char_get_skills_base(ch, skill) >= 75) {
    roll += 600;
  } else if (char_get_skills_base(ch, skill) >= 50) {
    roll += 400;
  }

  if (skill == SKILL_PARRY || skill == SKILL_DODGE || skill == SKILL_BARRIER || skill == SKILL_BLOCK || skill == SKILL_ZANZOKEN || skill == SKILL_TSKIN) {
    if (char_get_bonuses(ch, BONUS_MASOCHISTIC) > 0) {
      return;
    }
  }

  if (!char_spar_train(g, ch)) {
    if (num == 0) {
      roll -= 400;
    } else if (num == 1) {
      roll -= 200;
    }
  } else {
    if (num == 0) {
      roll -= 500;
    } else if (num == 1) {
      roll -= 400;
    } else {
      roll -= 200;
    }
  }

  if (char_get_fighting(g, ch) != nullptr && char_is_npc(char_get_fighting(g, ch)) && char_mob_flagged(char_get_fighting(g, ch), MOB_DUMMY)) {
    roll -= 100;
  }

  if (char_is_truffle(ch) || (char_is_bio(ch) && (char_get_genome(ch, 0) == 6 || char_get_genome(ch, 1) == 6))) {
    roll *= 0.5;
  } else if (char_is_majin(ch)) {
    roll += roll * .3;
  } else if (char_is_bio(ch) && skill == SKILL_ABSORB) {
    roll -= roll * .15;
  } else if (char_is_hoshijin(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK || skill == SKILL_KNEE || skill == SKILL_ELBOW || skill == SKILL_UPPERCUT || skill == SKILL_ROUNDHOUSE || skill == SKILL_SLAM || skill == SKILL_HEELDROP || skill == SKILL_DAGGER || skill == SKILL_SWORD || skill == SKILL_CLUB || skill == SKILL_GUN || skill == SKILL_SPEAR || skill == SKILL_BRAWL)) {
    roll = roll * 0.30;
  }

  if (char_get_fighting(g, ch) != nullptr && char_is_npc(char_get_fighting(g, ch)) && char_mob_flagged(char_get_fighting(g, ch), MOB_DUMMY)) {
    roll -= 100;
  }

  if (char_get_bonuses(ch, BONUS_QUICK_STUDY) > 0) {
    roll -= roll * .25;
  } else if (char_get_bonuses(ch, BONUS_SLOW_LEARNER) > 0) {
    roll += roll * .25;
  }

  if (char_get_asb(ch) > 0) {
    roll -= (roll * 0.01) * char_get_asb(ch);
  }

  roll = MAX(roll, 300);

  if (rand_number(g, 1, roll) > ((char_get_aff_abils_intel(ch) * 2) + char_get_aff_abils_wis(ch))) {
    return;
  }
  if ((percent > 99 || percent <= 0)) {
    return;
  }
  if (char_is_majin(ch) && char_get_skill(ch, skill) >= 75) {
    switch (skill) {
      case 407:
      case 408:
      case 409:
      case 425:
      case 431:
      case 449:
      case 450:
      case 451:
      case 452:
      case 453:
      case 454:
      case 455:
      case 456:
      case 465:
      case 466:
      case 467:
      case 468:
      case 469:
      case 470:
      case 499:
      case 501:
      case 530:
      case 531:
      case 538:
        /* Do nothing. */
        break;
      default:
        return;
    }
  } else if (char_is_majin(ch) && skill == 425) {
    roll += 250;
  }

  /*
  if ((IS_JINTO(ch) || IS_TSUNA(ch) || IS_DABURA(ch) || IS_TAPION(ch) && GET_SKILL(ch, SKILL_SENSE) >= 75) && skill == SKILL_SENSE) {
    return;
  }
  
  if ((IS_BARDOCK(ch) || IS_KURZAK(ch) || IS_FRIEZA(ch) || IS_GINYU(ch) || IS_ANDSIX(ch) && GET_SKILL(ch, SKILL_SENSE) >= 50) && skill == SKILL_SENSE) {
    return;
  }
   */

  newpercent = 1;
  percent += newpercent;
  char_set_skills_base(ch, skill, percent);
  if (newpercent >= 1) {
    skillbuf = str_format("@WYou feel you have learned something new about @G{}@W.@n\r\n", g->spell_info[skill].name);
    send_to_char(g, ch, skillbuf);
    if (char_get_skills_base(ch, skill) >= 100) {
      send_to_char(g, ch, "You learned a lot by mastering that skill.\r\n");
      if (perf_skill(skill)) {
        send_to_char(g, ch, "You can now choose a perfection for this skill (help perfection).\r\n");
      }
      if (char_is_konatsu(ch) && skill == SKILL_PARRY) {
        char_set_skills_base(ch, skill, char_get_skills_base(ch, skill) + 5);
      }
      if (char_get_level(ch) < 100) {
        char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) / 20));
      } else {
        gain_exp(g, ch, 5000000);
      }
    }
  }
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(GameWorld *g, struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = char_get_master(g, k)) {
    if (k == ch)
      return (true);
  }

  return (false);
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(GameWorld *g, struct char_data *ch)
{
  if (char_get_master(g, ch) == nullptr) {
    core_dump();
    return;
  }

  act(g, "You stop following $N.", false, ch, nullptr, char_get_master(g, ch), TO_CHAR);
  act(g, "$n stops following $N.", true, ch, nullptr, char_get_master(g, ch), TO_NOTVICT);
  if (!(char_is_dead(char_get_master(g, ch)) || (char_get_master(g, ch)->desc && desc_get_connected(char_get_master(g, ch)->desc) == CON_MENU)))
    act(g, "$n stops following you.", true, ch, nullptr, char_get_master(g, ch), TO_VICT);

  if (has_group(g, ch))
    char_set_combatexpertise(ch, 0);

  char_followers_slot(char_get_master(g, ch))->remove(char_get_id(ch));
  char_set_master(ch, nullptr);
}

/* Called when a character that follows/is followed dies */
void die_follower(GameWorld *g, struct char_data *ch)
{

  if (char_get_master(g, ch))
    stop_follower(g, ch);

  for (char_data *fol : char_followers(g, ch)) {
    stop_follower(g, fol);
  }
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(GameWorld *g, struct char_data *ch, struct char_data *leader)
{
  if (char_get_master(g, ch)) {
    core_dump();
    return;
  }

  char_set_master(ch, leader);
  char_followers_slot(leader)->add(char_get_id(ch));

  act(g, "You now follow $N.", false, ch, nullptr, leader, TO_CHAR);
  if (char_get_in_room(ch) != NOWHERE && char_get_in_room(leader) != NOWHERE && char_can_see(g, leader, ch)) {
    act(g, "$n starts following you.", true, ch, nullptr, leader, TO_VICT);
    act(g, "\r\n$n starts to follow $N.", true, ch, nullptr, leader, TO_NOTVICT);
  }
}

static const int default_admin_flags_mortal[] =
    {-1};

static const int default_admin_flags_immortal[] =
    {ADM_SEEINV, ADM_SEESECRET, ADM_FULLWHERE, ADM_NOPOISON, ADM_WALKANYWHERE,
     ADM_NODAMAGE, ADM_NOSTEAL, -1};

static const int default_admin_flags_builder[] =
    {-1};

static const int default_admin_flags_god[] =
    {ADM_ALLSHOPS, ADM_TELLALL, ADM_KNOWWEATHER, ADM_MONEY, ADM_EATANYTHING,
     ADM_NOKEYS, -1};

static const int default_admin_flags_vice[] =
    {-1};

static const int default_admin_flags_grgod[] =
    {ADM_TRANSALL, ADM_FORCEMASS, ADM_ALLHOUSES, -1};

static const int default_admin_flags_impl[] =
    {ADM_SWITCHMORTAL, ADM_INSTANTKILL, ADM_CEDIT, -1};

static const int *const default_admin_flags[ADMLVL_IMPL + 1] = {
    default_admin_flags_mortal,
    default_admin_flags_immortal,
    default_admin_flags_builder,
    default_admin_flags_god,
    default_admin_flags_vice,
    default_admin_flags_grgod,
    default_admin_flags_impl};

void admin_set(GameWorld *g, struct char_data *ch, int value)
{
  int i;
  int orig = char_get_admlevel(ch);

  if (char_get_admlevel(ch) == value)
    return;
  if (char_get_admlevel(ch) < value) { /* Promotion */
    mudlog(g, LOG_BRF, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true,
           "{} promoted from {} to {}", char_get_name(ch), admin_level_names[char_get_admlevel(ch)],
           admin_level_names[value]);
    while (char_get_admlevel(ch) < value) {
      char_set_admlevel(ch, char_get_admlevel(ch) + 1);
      for (i = 0; default_admin_flags[char_get_admlevel(ch)][i] != -1; i++)
        char_adm_set(ch, default_admin_flags[char_get_admlevel(ch)][i], true);
    }
    run_autowiz(g);
    if (orig < ADMLVL_IMMORT && value >= ADMLVL_IMMORT) {
      char_prf_set(g, ch, PRF_LOG2, true);
      char_prf_set(g, ch, PRF_HOLYLIGHT, true);
      char_prf_set(g, ch, PRF_ROOMFLAGS, true);
      char_prf_set(g, ch, PRF_AUTOEXIT, true);
    }
    if (char_get_admlevel(ch) >= ADMLVL_IMMORT) {
      for (i = 0; i < 3; i++)
        char_set_conditions(g, ch, i, (char)-1);
      char_prf_set(g, ch, PRF_HOLYLIGHT, true);
    }
    return;
  }
  if (char_get_admlevel(ch) > value) { /* Demotion */
    mudlog(g, LOG_BRF, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true,
           "{} demoted from {} to {}", char_get_name(ch), admin_level_names[char_get_admlevel(ch)],
           admin_level_names[value]);
    while (char_get_admlevel(ch) > value) {
      for (i = 0; default_admin_flags[char_get_admlevel(ch)][i] != -1; i++)
        char_adm_set(ch, default_admin_flags[char_get_admlevel(ch)][i], false);
      char_set_admlevel(ch, char_get_admlevel(ch) - 1);
    }
    run_autowiz(g);
    if (orig >= ADMLVL_IMMORT && value < ADMLVL_IMMORT) {
      char_prf_set(g, ch, PRF_LOG1, false);
      char_prf_set(g, ch, PRF_LOG2, false);
      char_prf_set(g, ch, PRF_NOHASSLE, false);
      char_prf_set(g, ch, PRF_HOLYLIGHT, false);
      char_prf_set(g, ch, PRF_ROOMFLAGS, false);
    }
    return;
  }
}

struct time_info_data *age(GameWorld *g, struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(game_now(g), ch->time.birth);

  return (&player_age);
}

/* Used to roll starting PL/KI/ST in character creation */
int roll_stats(GameWorld *g, struct char_data *ch, int type, int bonus)
{

  int pool = 0, base_num = bonus, max_num = bonus;
  int powerlevel = 0, ki = 1, stamina = 2;

  if (type == powerlevel) {
    base_num = ch->real_abils.str * 3;
    max_num  = ch->real_abils.str * 5;
  } else if (type == ki) {
    base_num = ch->real_abils.intel * 3;
    max_num  = ch->real_abils.intel * 5;
  } else if (type == stamina) {
    base_num = ch->real_abils.con * 3;
    max_num  = ch->real_abils.con * 5;
  }

  pool = rand_number(g, base_num, max_num) + bonus;

  return (pool);
}

/* For Getting An Intro Name */
const char *get_i_name(struct char_data *ch, struct char_data *vict)
{
  char fname[40], filler[50], scrap[100], line[256];
  static char name[50];
  int known = false;
  FILE *fl;

  /* Read Introduction File */
  if (vict == nullptr) {
    return ("");
  }

  if (char_is_npc(ch) || char_is_npc(vict)) {
    return (juggleRaceName(vict, true));
  }

  if (vict == ch) {
    return ("");
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
    return (juggleRaceName(vict, true));
  }
  if (!(fl = fopen(fname, "r"))) {
    return (juggleRaceName(vict, true));
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%49s %99s\n", filler, scrap);
    if (!strcasecmp(char_get_name(vict), filler)) {
      snprintf(name, sizeof(name), "%s", scrap);
      known = true;
    }
  }
  fclose(fl);

  if (known == true)
    return (name);
  else
    return (juggleRaceName(vict, true));
}

bool can_grav(GameWorld *g, struct char_data *ch)
{
  /* Gravity Related */
  if (room_get_gravity(g, char_get_in_room(ch)) == 10 && getEffMaxPL(g, ch) < 5000 && !char_is_bardock(ch) && !char_is_npc(ch)) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 20 && getEffMaxPL(g, ch) < 20000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 30 && getEffMaxPL(g, ch) < 50000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 40 && getEffMaxPL(g, ch) < 100000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 50 && getEffMaxPL(g, ch) < 200000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 100 && getEffMaxPL(g, ch) < 400000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 200 && getEffMaxPL(g, ch) < 1000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 300 && getEffMaxPL(g, ch) < 5000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 400 && getEffMaxPL(g, ch) < 8000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 500 && getEffMaxPL(g, ch) < 15000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 1000 && getEffMaxPL(g, ch) < 25000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 5000 && getEffMaxPL(g, ch) < 100000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  if (room_get_gravity(g, char_get_in_room(ch)) == 10000 && getEffMaxPL(g, ch) < 200000000) {
    send_to_char(g, ch, "You are hardly able to move in this gravity!\r\n");
    return false;
  }
  return true;
}

/* If they can preform the attack or perform the attack on target. */
bool can_kill(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int num)
{
  /* Target Related */
  if (!char_is_npc(ch) && char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return false;
  }
  if (char_get_carry_weight(ch) > getMaxCarryWeight(g, ch)) {
    send_to_char(g, ch, "You are weighted down too much!\r\n");
    return false;
  }
  if (vict) {
    if (getCurPL(g, vict) <= 0 && char_get_fighting(g, vict)) {
      return false;
    }
    if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
      send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
      return false;
    }
    if (vict == ch) {
      send_to_char(g, ch, "That's insane, don't hurt yourself. Hurt others! That's the key to life ^_^\r\n");
      return false;
    }
    if (vict->gooptime > 0) {
      send_to_char(g, ch, "It seems like it'll be hard to kill them right now...\r\n");
      return false;
    }
    if (char_get_carrying(g, ch)) {
      send_to_char(g, ch, "You are too busy protecting the person on your shoulder!\r\n");
      return false;
    }
    if (char_get_carried_by(g, vict)) {
      send_to_char(g, ch, "They are being protected by someone else!\r\n");
      return false;
    }
    if (char_aff_flagged(vict, AFF_PARALYZE)) {
      send_to_char(g, ch, "They are a statue, just leave them alone...\r\n");
      return false;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      send_to_char(g, ch, "But they are not to be killed!\r\n");
      return false;
    }
    if (char_get_majinize(ch) == char_get_id(vict)) {
      send_to_char(g, ch, "You can not harm your master!\r\n");
      return false;
    }
    if (char_get_bonuses(ch, BONUS_COWARD) > 0 && getEffMaxPL(g, vict) > getEffMaxPL(g, ch) + (getEffMaxPL(g, ch) * .5) && !char_get_fighting(g, ch)) {
      send_to_char(g, ch, "You are too cowardly to start anything with someone so much stronger than yourself!\r\n");
      return false;
    }
    if (char_get_majinize(vict) == char_get_id(ch)) {
      send_to_char(g, ch, "You can not harm your servant.\r\n");
      return false;
    }
    if ((char_get_grappling(g, ch) && char_get_grap(ch) != 3) || (char_get_grappled(g, ch) && (char_get_grap(ch) == 1 || char_get_grap(ch) == 4))) {
      send_to_char(g, ch, "You are too busy grappling!{}\r\n", char_get_grappled(g, ch) != nullptr ? " Try 'escape'!" : "");
      return false;
    }
    if (char_get_grappling(g, ch) && char_get_grappling(g, ch) != vict) {
      send_to_char(g, ch, "You can't reach that far in your current position!\r\n");
      return false;
    }
    if (char_get_grappled(g, ch) && char_get_grappled(g, ch) != vict) {
      send_to_char(g, ch, "You can't reach that far in your current position!\r\n");
      return false;
    }
    if (!char_is_npc(ch) && !char_is_npc(vict) && char_aff_flagged(ch, AFF_SPIRIT) && (!is_sparring(ch) || !is_sparring(vict)) && num != 2) {
      send_to_char(g, ch, "You can not fight other players in AL/Hell.\r\n");
      return false;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(ch) && !char_is_npc(vict) && (!is_sparring(ch) || !is_sparring(vict))) {
      send_to_char(g, ch, "Newbie Shield Protects them!\r\n");
      return false;
    }
    if (char_get_level(ch) <= 8 && !char_is_npc(ch) && !char_is_npc(vict) && (!is_sparring(ch) || !is_sparring(vict))) {
      send_to_char(g, ch, "Newbie Shield Protects you until level 8.\r\n");
      return false;
    }
    if (char_plr_flagged(vict, PLR_SPIRAL) && num != 3) {
      send_to_char(g, ch, "Due to the nature of their current technique anything less than a Tier 4 or AOE attack will not work on them.\r\n");
      return false;
    }
    if (char_get_absorbing(g, ch)) {
      send_to_char(g, ch, "You are too busy absorbing {}!\r\n", char_get_name(char_get_absorbing(g, ch)));
      return false;
    }
    if (char_get_absorbby(g, ch)) {
      send_to_char(g, ch, "You are too busy being absorbed by {}!\r\n", char_get_name(char_get_absorbby(g, ch)));
      return false;
    }
    if ((char_get_altitude(vict) > char_get_altitude(ch) || char_get_altitude(vict) < char_get_altitude(ch)) && char_is_namek(ch)) {
      act(g, "@GYou stretch your limbs toward @g$N@G in an attempt to hit $M!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@g$n@G stretches $s limbs toward @RYOU@G in an attempt to land a hit!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@g$n@G stretches $s limbs toward @g$N@G in an attempt to hit $M!@n", true, ch, nullptr, vict, TO_NOTVICT);
      return true;
    }
    if (char_aff_flagged(ch, AFF_FLYING) && !char_aff_flagged(vict, AFF_FLYING) && num == 0) {
      send_to_char(g, ch, "You are too far above them.\r\n");
      return false;
    }
    if (!char_aff_flagged(ch, AFF_FLYING) && char_aff_flagged(vict, AFF_FLYING) && num == 0) {
      send_to_char(g, ch, "They are too far above you.\r\n");
      return false;
    }
    if (!char_is_npc(ch) && char_get_altitude(ch) > char_get_altitude(vict) && !char_is_npc(vict) && num == 0) {
      if (char_get_altitude(vict) < 0) {
        char_set_altitude(vict, char_get_altitude(ch));
        return true;
      }
      send_to_char(g, ch, "You are too far above them.\r\n");
      return false;
    }
    if (!char_is_npc(ch) && char_get_altitude(ch) < char_get_altitude(vict) && !char_is_npc(vict) && num == 0) {
      if (char_get_altitude(vict) > 2) {
        char_set_altitude(vict, char_get_altitude(ch));
        return true;
      }
      send_to_char(g, ch, "They are too far above you.\r\n");
      return false;
    }
    return true;
  }
  if (obj) {
    if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
      send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
      return false;
    }
    if (obj_extra_flagged(obj, ITEM_UNBREAKABLE) && obj_get_vnum(g, obj) != 87 && obj_get_vnum(g, obj) != 80 && obj_get_vnum(g, obj) != 81 && obj_get_vnum(g, obj) != 82 && obj_get_vnum(g, obj) != 83) {
      send_to_char(g, ch, "You can't hit that, it is protected by the immortals!\r\n");
      return false;
    }
    if (char_aff_flagged(ch, AFF_FLYING)) {
      send_to_char(g, ch, "You are too far above it.\r\n");
      return false;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is already broken!\r\n");
      return false;
    }
    return true;
  }
  send_to_char(g, ch, "Error: Report to imm.");
  return false;
}

/* Whether they know the skill they are trying to use */
bool check_skill(GameWorld *g, struct char_data *ch, int skill)
{
  if (!know_skill(g, ch, skill) && !char_is_npc(ch)) {
    return false;
  }
  return true;
}

/* Whether they have enough stamina or charged ki to preform the skill */
bool check_points(GameWorld *g, struct char_data *ch, int64_t ki, int64_t st)
{

  if (char_get_preference(ch) == PREFERENCE_H2H && char_get_charge(ch) >= getMaxKI(g, ch) * 0.1) {
    st -= st * 0.5;
  }

  int fail = false;
  if (char_is_npc(ch)) {
    if ((getCurKI(g, ch)) < ki) {
      send_to_char(g, ch, "You do not have enough ki!\r\n");
      fail = true;
    }
    if ((getCurST(g, ch)) < st) {
      send_to_char(g, ch, "You do not have enough stamina!\r\n");
      fail = true;
    }
  } else {
    if (!calcGravCost(g, ch, st) && ki <= 0) {
      send_to_char(g, ch, "You do not have enough stamina to perform it in this gravity!\r\n");
      return false;
    }
    if (char_get_charge(ch) < ki) {
      send_to_char(g, ch, "You do not have enough ki charged.\r\n");
      int64_t perc = getMaxKI(g, ch) * 0.01;
      if (ki >= perc * 49) {
        send_to_char(g, ch, "You need at least 50 percent charged.\r\n");
      } else if (ki >= perc * 44) {
        send_to_char(g, ch, "You need at least 45 percent charged.\r\n");
      } else if (ki >= perc * 39) {
        send_to_char(g, ch, "You need at least 40 percent charged.\r\n");
      } else if (ki >= perc * 34) {
        send_to_char(g, ch, "You need at least 35 percent charged.\r\n");
      } else if (ki >= perc * 29) {
        send_to_char(g, ch, "You need at least 30 percent charged.\r\n");
      } else if (ki >= perc * 24) {
        send_to_char(g, ch, "You need at least 25 percent charged.\r\n");
      } else if (ki >= perc * 19) {
        send_to_char(g, ch, "You need at least 20 percent charged.\r\n");
      } else if (ki >= perc * 14) {
        send_to_char(g, ch, "You need at least 15 percent charged.\r\n");
      } else if (ki >= perc * 9) {
        send_to_char(g, ch, "You need at least 10 percent charged.\r\n");
      } else if (ki >= perc * 4) {
        send_to_char(g, ch, "You need at least 5 percent charged.\r\n");
      } else if (ki >= 1) {
        send_to_char(g, ch, "You need at least 1 percent charged.\r\n");
      }
      fail = true;
    }
    if (char_is_nonptrans(ch)) {
      if (char_plr_flagged(ch, PLR_TRANS1)) {
        st -= st * 0.2;
      } else if (char_plr_flagged(ch, PLR_TRANS2)) {
        st -= st * 0.4;
      } else if (char_plr_flagged(ch, PLR_TRANS3)) {
        st -= st * 0.6;
      } else if (char_plr_flagged(ch, PLR_TRANS4)) {
        st -= st * 0.8;
      }
    }
    if ((getCurST(g, ch)) < st) {
      send_to_char(g, ch, "You do not have enough stamina.\r\n@C{}@n needed.\r\n", add_commas(st));
      fail = true;
    }
  }
  if (fail == true) {
    return false;
  }
  return true;
}

/* Subtract the stamina or ki required */
void pcost(GameWorld *g, struct char_data *ch, double ki, int64_t st)
{
  int before = 0;
  if (char_get_level(ch) > 1 && !char_is_npc(ch)) {
    if (ki == 0) {
      before = (getCurST(g, ch));
      if (calcGravCost(g, ch, 0)) {
        if (before > (getCurST(g, ch))) {
          send_to_char(g, ch, "You exert more stamina in this gravity.\r\n");
        }
      }
    }
    if (char_get_charge(ch) <= (getMaxKI(g, ch) * ki)) {
      char_set_charge(ch, 0);
    }
    if (char_get_charge(ch) > (getMaxKI(g, ch) * ki)) {
      char_set_charge(ch, char_get_charge(ch) - ((getMaxKI(g, ch) * ki)));
    }
    if (char_get_charge(ch) < 0) {
      char_set_charge(ch, 0);
    }
    if (char_get_kaioken(ch) > 0) {
      st += (st / 20) * char_get_kaioken(ch);
    }
    if (char_aff_flagged(ch, AFF_HASS)) {
      st += st * .3;
    }
    if (!char_is_npc(ch) && char_get_bonuses(ch, BONUS_HARDWORKER) > 0) {
      st -= st * .25;
    } else if (!char_is_npc(ch) && char_get_bonuses(ch, BONUS_SLACKER) > 0) {
      st += st * .25;
    }
    if (char_is_icer(ch)) {
      if (char_plr_flagged(ch, PLR_TRANS1)) {
        st = st * 1.05;
      } else if (char_plr_flagged(ch, PLR_TRANS2)) {
        st = st * 1.1;
      } else if (char_plr_flagged(ch, PLR_TRANS3)) {
        st = st * 1.15;
      } else if (char_plr_flagged(ch, PLR_TRANS4)) {
        st = st * 1.20;
      }
    }
    if (char_get_preference(ch) == PREFERENCE_H2H && char_get_charge(ch) >= getMaxKI(g, ch) * 0.1) {
      st -= st * 0.5;
      char_set_charge(ch, char_get_charge(ch) - (st));
      if (char_get_charge(ch) < 0)
        char_set_charge(ch, 0);
    }
    if (char_is_nonptrans(ch)) {
      if (char_plr_flagged(ch, PLR_TRANS1)) {
        st -= st * 0.2;
      } else if (char_plr_flagged(ch, PLR_TRANS2)) {
        st -= st * 0.4;
      } else if (char_plr_flagged(ch, PLR_TRANS3)) {
        st -= st * 0.6;
      } else if (char_plr_flagged(ch, PLR_TRANS4)) {
        st -= st * 0.8;
      }
    }
    decCurST(g, ch, st, 0);
  }
  if (char_is_npc(ch)) {
    decCurKI(g, ch, ki, 0);
    decCurST(g, ch, st, 0);
  }
}

/* For checking if they have enough free limbs to preform the technique. */
/* Tells them so, if they're too busy with a song to do anything else. */
bool playing_song(GameWorld *g, struct char_data *ch)
{
  if (char_get_powerattack(ch) > 0) {
    send_to_char(g, ch, "You are currently playing a song! Enter the song command in order to stop!\r\n");
    return true;
  }
  return false;
}

bool limb_ok(GameWorld *g, struct char_data *ch, int type)
{
  if (char_is_npc(ch)) {
    if (char_aff_flagged(ch, AFF_ENSNARED) && rand_number(g, 1, 100) <= 90) {
      return false;
    }
    return true;
  }
  if (char_get_grappling(g, ch) && char_get_grap(ch) != 3) {
    send_to_char(g, ch, "You are too busy grappling!\r\n");
    return false;
  }
  if (char_get_grappled(g, ch) && (char_get_grap(ch) == 1 || char_get_grap(ch) == 4)) {
    send_to_char(g, ch, "You are unable to move while in this hold! Try using 'escape' to get out of it!\r\n");
    return false;
  }
  if (playing_song(g, ch)) {
    return false;
  }
  if (type == 0) {
    if (!char_has_arms(g, ch)) {
      send_to_char(g, ch, "You have no available arms!\r\n");
      return false;
    }
    if (char_aff_flagged(ch, AFF_ENSNARED) && rand_number(g, 1, 100) <= 90) {
      send_to_char(g, ch, "You are unable to move your arms while bound by this strong silk!\r\n");
      char_set_wait(ch, PULSE_1SEC);
      return false;
    }
    if (char_aff_flagged(ch, AFF_ENSNARED)) {
      act(g, "You manage to break the silk ensnaring your arms!", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n manages to break the silk ensnaring $s arms!", true, ch, nullptr, nullptr, TO_ROOM);
      char_aff_set(ch, AFF_ENSNARED, false);
    }
    if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(g, ch, "Your hands are full!\r\n");
      return false;
    }
  } // Arms
  else if (type > 0) {
    if (!char_has_legs(g, ch)) {
      send_to_char(g, ch, "You have no working legs!\r\n");
      return false;
    }
  } // Legs
  return true;
}

size_t send_to_char(GameWorld *g, struct char_data *ch, std::string_view text)
{
  if (ch->desc && !text.empty())
    return write_to_output(g, ch->desc, text);
  return 0;
}

int init_skill(GameWorld *g, struct char_data *ch, int snum)
{
  int skill = 0;

  if (!char_is_npc(ch)) {
    skill = char_get_skill(ch, snum);

    if (char_plr_flagged(ch, PLR_TRANSMISSION)) {
      skill += 4;
    }

    if (skill > 118)
      skill = 118;

    return (skill);
  }

  if (char_is_npc(ch) && char_get_level(ch) <= 10) {
    skill = rand_number(g, 30, 50);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 20) {
    skill = rand_number(g, 45, 65);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 30) {
    skill = rand_number(g, 55, 70);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 50) {
    skill = rand_number(g, 65, 80);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 70) {
    skill = rand_number(g, 75, 90);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 80) {
    skill = rand_number(g, 85, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 90) {
    skill = rand_number(g, 90, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 100) {
    skill = rand_number(g, 95, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 110) {
    skill = rand_number(g, 95, 105);
  } else {
    skill = rand_number(g, 100, 110);
  }

  return (skill);
}

/* For calculating the difficulty the player has to hit with the skill. */
int chance_to_hit(GameWorld *g, struct char_data *ch)
{

  int num = axion_dice(g, 0);

  if (char_is_npc(ch))
    return (num);

  if (char_get_conditions(g, ch, DRUNK) > 4) {
    num += char_get_conditions(g, ch, DRUNK);
  }

  return (num);
}

/* For calculating the speed of the attacker and defender */
int handle_speed(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  if (ch == nullptr || vict == nullptr) { /* Ruh roh*/
    return (0);
  }

  if (char_get_speedi(g, ch) > char_get_speedi(g, vict) * 4) {
    return (15);
  }
  if (char_get_speedi(g, ch) > char_get_speedi(g, vict) * 2) {
    return (10);
  }
  if (char_get_speedi(g, ch) > char_get_speedi(g, vict)) {
    return (5);
  }
  if (char_get_speedi(g, ch) * 4 < char_get_speedi(g, vict)) {
    return (-15);
  }
  if (char_get_speedi(g, ch) * 2 < char_get_speedi(g, vict)) {
    return (-10);
  }
  if (char_get_speedi(g, ch) < char_get_speedi(g, vict)) {
    return (-5);
  }

  return (0);
}

static bool race_has_tail(int r_id)
{
  switch (r_id) {
    case RACE_ICER:
    case RACE_BIO:
    case RACE_SAIYAN:
    case RACE_HALFBREED:
      return true;
    default:
      return false;
  }
}

void char_lose_tail(GameWorld *g, char_data *ch)
{
  if (!char_has_tail(ch)) return;
  switch (ch->race) {
    case RACE_ICER:
    case RACE_BIO:
      char_plr_set(ch, PLR_TAIL, false);
      remove_limb(g, ch, 6);
      char_set_tail_growth(ch, 0);
      break;
    case RACE_SAIYAN:
    case RACE_HALFBREED:
      char_plr_set(ch, PLR_STAIL, false);
      remove_limb(g, ch, 5);
      if (char_plr_flagged(ch, PLR_OOZARU)) {
        oozaru_revert(g, ch);
      }
      char_set_tail_growth(ch, 0);
      break;
  }
}

bool char_has_tail(char_data *ch)
{
  if (!race_has_tail(ch->race))
    return false;
  switch (ch->race) {
    case RACE_ICER:
    case RACE_BIO:
      return char_plr_flagged(ch, PLR_TAIL);
    case RACE_SAIYAN:
    case RACE_HALFBREED:
      return char_plr_flagged(ch, PLR_STAIL);
    default:
      return false;
  }
}

void char_gain_tail(GameWorld *g, char_data *ch, bool announce)
{
  if (char_has_tail(ch)) return;
  switch (ch->race) {
    case RACE_ICER:
    case RACE_BIO:
      char_plr_set(ch, PLR_TAIL, true);
      break;
    case RACE_SAIYAN:
    case RACE_HALFBREED:
      char_plr_set(ch, PLR_STAIL, true);
      if (char_moon_ok(g, ch)) {
        oozaru_transform(g, ch);
      }
      break;
  }
}

room_vnum sensei_location_id(int s_id)
{
  switch (s_id) {
    case CLASS_ROSHI:
      return 1131;
    case CLASS_KABITO:
      return 12098;
    case CLASS_NAIL:
      return 11683;
    case CLASS_BARDOCK:
      return 2267;
    case CLASS_KRANE:
      return 13012;
    case CLASS_TAPION:
      return 8233;
    case CLASS_PICCOLO:
      return 1662;
    case CLASS_ANDSIX:
      return 1714;
    case CLASS_DABURA:
      return 6487;
    case CLASS_FRIEZA:
      return 4283;
    case CLASS_GINYU:
      return 4290;
    case CLASS_JINTO:
      return 3499;
    case CLASS_KURZAK:
      return 16100;
    case CLASS_TSUNA:
      return 15009;
    default:
      return 300;
  }
}

room_vnum sensei_start_room(int s_id)
{
  switch (s_id) {
    case CLASS_ROSHI:
      return 1130;
    case CLASS_KABITO:
      return 12098;
    case CLASS_NAIL:
      return 11683;
    case CLASS_BARDOCK:
      return 2268;
    case CLASS_KRANE:
      return 13009;
    case CLASS_TAPION:
      return 8231;
    case CLASS_PICCOLO:
      return 1659;
    case CLASS_ANDSIX:
      return 1713;
    case CLASS_DABURA:
      return 6486;
    case CLASS_FRIEZA:
      return 4282;
    case CLASS_GINYU:
      return 4289;
    case CLASS_JINTO:
      return 3499;
    case CLASS_KURZAK:
      return 16100;
    case CLASS_TSUNA:
      return 15009;
    default:
      return 300;
  }
}

static int sensei_grav_tolerance(int s_id)
{
  switch (s_id) {
    case CLASS_BARDOCK:
      return 10;
    default:
      return 0;
  }
}

int race_get_size(int r_id)
{
  switch (r_id) {
    case RACE_ANIMAL:
      return SIZE_FINE;
    case RACE_SAIBA:
    case RACE_OGRE:
      return SIZE_LARGE;
    case RACE_SPIRIT:
      return SIZE_TINY;
    case RACE_TRUFFLE:
      return SIZE_SMALL;
    default:
      return SIZE_MEDIUM;
  }
}

bool race_is_playable(int r_id)
{
  switch (r_id) {
    case RACE_ANIMAL:
    case RACE_SAIBA:
    case RACE_SERPENT:
    case RACE_OGRE:
    case RACE_YARDRATIAN:
    case RACE_DRAGON:
    case RACE_MECHANICAL:
    case RACE_SPIRIT:
      return false;
    default:
      return true;
  }
}

bool MOON_TIMECHECK(GameWorld *g)
{
  switch (g->time_info.day) {
    case 20:
      return g->time_info.hours >= 21;
    case 21:
      return g->time_info.hours < 4 || g->time_info.hours >= 21;
    case 22:
      return g->time_info.hours < 4;
  }
  return false;
}

/* Formerly macros in character_utils.hpp. */

/* NPCs are exactly the characters without player_specials. (MOB_ISNPC is still set on mobs so mob files and
 * OLC stay compatible, but it is not consulted.) */
bool char_is_npc(const struct char_data *ch)
{
  return !ch->player_specials;
}

bool char_is_mob(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_nr(ch) != NOBODY); /* a prototype goes only after every copy of it */
}

bool char_mob_flagged(const struct char_data *ch, int flag)
{
  return (char_is_npc(ch) && IS_SET_AR(char_get_act(ch), (flag)));
}

bool char_plr_flagged(const struct char_data *ch, int flag)
{
  return (!char_is_npc(ch) && IS_SET_AR(char_get_act(ch), (flag)));
}

bool char_aff_flagged(const struct char_data *ch, int flag)
{
  return (IS_SET_AR(char_get_affected_by(ch), (flag)));
}

bool char_prf_flagged(GameWorld *g, const struct char_data *ch, int flag)
{
  return (IS_SET_AR(char_get_pref(g, ch), (flag)));
}

bool char_adm_flagged(const struct char_data *ch, int flag)
{
  return (IS_SET_AR(char_get_admflags(ch), (flag)));
}

bool char_body_flagged(const struct char_data *ch, int flag)
{
  return (IS_SET_AR(char_get_bodyparts(ch), (flag)));
}

bool char_is_affected(const struct char_data *ch, int skill)
{
  return (char_aff_flagged((ch), (skill)));
}

bool char_plr_toggle(struct char_data *ch, int flag)
{
  if (char_is_npc(ch))
    return false;
  return ((TOGGLE_BIT_AR(char_get_act(ch), (flag))) & Q_BIT(flag));
}

bool char_prf_toggle(GameWorld *g, struct char_data *ch, int flag)
{
  return ((TOGGLE_BIT_AR(char_get_pref(g, ch), (flag))) & Q_BIT(flag));
}

bool char_aff_toggle(struct char_data *ch, int flag)
{
  return ((TOGGLE_BIT_AR(char_get_affected_by(ch), (flag))) & Q_BIT(flag));
}

bool char_is_dead(const struct char_data *ch)
{
  return (char_plr_flagged((ch), PLR_NOTDEADYET) || char_mob_flagged((ch), MOB_NOTDEADYET));
}

zone_vnum char_in_zone(GameWorld *g, const struct char_data *ch)
{
  return (((room_get_zone(g, (char_get_in_room(ch)))))->number);
}

const char *char_get_name(const struct char_data *ch)
{
  return char_is_npc(ch) ? c_str_or_null(ch->short_descr) : char_get_pc_name(ch);
}

const char *char_get_title(const struct char_data *ch)
{
  return ((ch)->desc ? (!(ch)->desc->title.empty() ? (ch)->desc->title.c_str() : "[Unset Title]") : "@D[@GNew User@D]");
}

const char *char_get_user(const struct char_data *ch)
{
  return ((ch)->desc ? (!(ch)->desc->user.empty() ? (ch)->desc->user.c_str() : "NOUSER") : "NOUSER");
}

int char_get_level(const struct char_data *ch)
{
  return (char_get_class_level(ch) + char_get_level_adj(ch) + char_get_race_level(ch));
}

int char_get_class(const struct char_data *ch)
{
  return ((ch)->chclass ? (ch)->chclass : 0);
}

int char_get_class_ranks(const struct char_data *ch, int whichclass)
{
  return char_get_level(ch);
}

int char_get_pc_height(GameWorld *g, struct char_data *ch)
{
  return (!char_is_npc(ch) ? age(g, ch)->year <= 10 ? (int)((ch)->height * 0.68) : age(g, ch)->year <= 12 ? (int)((ch)->height * 0.72)
                                                                            : age(g, ch)->year <= 14   ? (int)((ch)->height * 0.85)
                                                                            : age(g, ch)->year <= 16   ? (int)((ch)->height * 0.92)
                                                                                                    : (ch)->height
                           : (ch)->height);
}

int char_get_pc_weight(GameWorld *g, struct char_data *ch)
{
  return (!char_is_npc(ch) ? age(g, ch)->year <= 10 ? (int)((ch)->weight * 0.48) : age(g, ch)->year <= 12 ? (int)((ch)->weight * 0.55)
                                                                            : age(g, ch)->year <= 14   ? (int)((ch)->weight * 0.7)
                                                                            : age(g, ch)->year <= 16   ? (int)((ch)->weight * 0.85)
                                                                                                    : (ch)->weight
                           : (ch)->weight);
}

double char_get_mutboost(GameWorld *g, struct char_data *ch)
{
  return (char_is_mutant(ch) ? ((char_get_genome(ch, 0) == 1 || char_get_genome(ch, 1) == 1) ? (char_get_speedcalc(g, ch) + char_get_speedbonus(g, ch) + char_get_speedboost(ch)) * 0.3 : 0) : 0);
}

double char_get_speedi(GameWorld *g, struct char_data *ch)
{
  return (char_get_speedcalc(g, ch) + char_get_speedbonus(g, ch) + char_get_speedboost(ch) + char_get_mutboost(g, ch));
}

double char_get_speedcalc(GameWorld *g, struct char_data *ch)
{
  return (char_is_grap(g, ch) ? char_get_aff_abils_cha(ch) : (char_is_inferior(ch) ? (char_aff_flagged(ch, AFF_FLYING) ? (char_get_speedvar(g, ch) * 1.25) : char_get_speedvar(g, ch)) : char_get_speedvar(g, ch)));
}

double char_get_speedbonus(GameWorld *g, struct char_data *ch)
{
  return (char_is_arlian(ch) ? char_aff_flagged(ch, AFF_SHELL) ? char_get_speedvar(g, ch) * -0.5 : (char_is_male(ch) ? (char_aff_flagged(ch, AFF_FLYING) ? (char_get_speedvar(g, ch) * 0.5) : 0) : 0) : 0);
}

double char_get_speedvar(GameWorld *g, struct char_data *ch)
{
  return (char_get_speedvem(g, ch) > char_get_aff_abils_cha(ch) ? char_get_speedvem(g, ch) : char_get_aff_abils_cha(ch));
}

double char_get_speedvem(GameWorld *g, struct char_data *ch)
{
  return (char_get_speedint(g, ch) - (char_get_speedint(g, ch) * speednar(g, ch)));
}

static bool char_is_grap(GameWorld *g, const struct char_data *ch)
{
  return (char_get_grappling(g, ch) || char_get_grappled(g, ch));
}

long char_get_speedint(GameWorld *g, struct char_data *ch)
{
  return (char_is_bio(ch) ? ((char_get_aff_abils_cha(ch) * char_get_aff_abils_dex(ch)) * (getEffMaxPL(g, ch) / 1200) / 1200) + (char_get_aff_abils_cha(ch) * (char_get_kaioken(ch) * 100)) : ((char_get_aff_abils_cha(ch) * char_get_aff_abils_dex(ch)) * (getEffMaxPL(g, ch) / 1000) / 1000) + (char_get_aff_abils_cha(ch) * (char_get_kaioken(ch) * 100)));
}

static bool char_is_inferior(const struct char_data *ch)
{
  return (char_is_konatsu(ch) || char_is_demon(ch));
}

bool char_is_weighted(GameWorld *g, struct char_data *ch)
{
  return (getEffMaxPL(g, ch) < getEffMaxPL(g, ch));
}

bool char_is_spoiled(const struct char_data *ch)
{
  return ((ch)->time.played > 86400);
}

bool char_has_genome(const struct char_data *ch, int i)
{
  return ((ch)->genome[0] == (i) || (ch)->genome[1] == (i));
}

double char_get_blessbonus(GameWorld *g, struct char_data *ch)
{
  return (char_aff_flagged(ch, AFF_BLESS) ? (char_get_blesslvl(ch) >= 100 ? ((getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5)) * 0.1 : char_get_blesslvl(ch) >= 60 ? ((getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5)) * 0.05
                                                                                                                              : char_get_blesslvl(ch) >= 40   ? ((getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5)) * 0.02
                                                                                                                                                              : 0)
                                          : 0);
}

double char_get_poself(struct char_data *ch)
{
  return (!char_is_npc(ch) ? is_affected(ch, AFF_SPECIAL_POSE) ? char_get_skill(ch, SKILL_POSE) >= 100 ? 0.15 : char_get_skill(ch, SKILL_POSE) >= 60 ? 0.1
                                                                                                            : char_get_skill(ch, SKILL_POSE) >= 40   ? 0.05
                                                                                                                                                     : 0
                                                               : 0
                           : 0);
}

double char_get_posebonus(GameWorld *g, struct char_data *ch)
{
  return (((getMaxKI(g, ch) * 0.5) + (getMaxST(g, ch) * 0.5)) * char_get_poself(ch));
}

double char_get_lifebonus(GameWorld *g, struct char_data *ch)
{
  return (char_is_arlian(ch) ? ((getMaxKI(g, ch) * 0.01) * (char_get_moltlevel(ch) / 100)) + ((getMaxST(g, ch) * 0.01) * (char_get_moltlevel(ch) / 100)) : 0);
}

double char_get_lifebonuses(GameWorld *g, struct char_data *ch)
{
  return ((ch)->lifebonus > 0 ? (char_get_lifebonus(g, ch) + char_get_blessbonus(g, ch) + char_get_posebonus(g, ch)) * (((ch)->lifebonus + 100) * 0.01) : (char_get_lifebonus(g, ch) + char_get_blessbonus(g, ch) + char_get_posebonus(g, ch)));
}

int char_get_skill(const struct char_data *ch, int i)
{
  return char_get_skills_base(ch, i) + char_get_skills_mod(ch, i);
}

spec_proc_func char_get_mob_spec(GameWorld *g, const struct char_data *ch)
{
  return (char_is_mob(g, ch) ? mob_index_of(g, (ch)->nr)->func : nullptr);
}

mob_vnum char_get_mob_vnum(GameWorld *g, const struct char_data *mob)
{
  return (char_is_mob(g, mob) ? char_get_nr(mob) : NOBODY); /* nr is the prototype's vnum */
}

bool char_is_awake(const struct char_data *ch)
{
  return (char_get_position(ch) > POS_SLEEPING);
}

bool char_can_see_in_dark(GameWorld *g, const struct char_data *ch)
{
  return (char_aff_flagged(ch, AFF_INFRAVISION) || (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_HOLYLIGHT)) || (char_is_mutant(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4)) || char_plr_flagged(ch, PLR_AURALIGHT));
}

bool char_is_good(const struct char_data *ch)
{
  return (char_get_alignment(ch) >= 50);
}

bool char_is_evil(const struct char_data *ch)
{
  return (char_get_alignment(ch) <= -50);
}

bool char_is_neutral(const struct char_data *ch)
{
  return (!char_is_good(ch) && !char_is_evil(ch));
}

bool char_in_arena(GameWorld *g, const struct char_data *ch)
{
  return (char_get_in_room(ch) >= 17800 && char_get_in_room(ch) <= 17874);
}

static bool char_light_ok(GameWorld *g, const struct char_data *sub)
{
  return (!char_aff_flagged(sub, AFF_BLIND) && !char_plr_flagged(sub, PLR_EYEC) && (room_is_light(g, char_get_in_room(sub)) || char_aff_flagged((sub), AFF_INFRAVISION) || (char_is_mutant(sub) && (char_get_genome(sub, 0) == 4 || char_get_genome(sub, 1) == 4)) || char_plr_flagged(sub, PLR_AURALIGHT)));
}

static bool char_invis_ok(const struct char_data *sub, const struct char_data *obj)
{
  return (!char_aff_flagged((obj), AFF_INVISIBLE) || char_aff_flagged(sub, AFF_DETECT_INVIS));
}

static bool char_mort_can_see(GameWorld *g, const struct char_data *sub, const struct char_data *obj)
{
  return (char_light_ok(g, sub) && char_invis_ok(sub, obj));
}

static bool char_imm_can_see(GameWorld *g, const struct char_data *sub, const struct char_data *obj)
{
  return (char_mort_can_see(g, sub, obj) || (!char_is_npc(sub) && char_prf_flagged(g, sub, PRF_HOLYLIGHT)));
}

static bool char_is_self(const struct char_data *sub, const struct char_data *obj)
{
  return ((sub) == (obj));
}

bool char_can_see(GameWorld *g, const struct char_data *sub, const struct char_data *obj)
{
  return (char_is_self(sub, obj) || ((char_get_admlevel(sub) >= (char_is_npc(obj) ? 0 : char_get_invis_level(g, obj))) && char_imm_can_see(g, sub, obj) && (char_not_hidden(obj) || char_get_admlevel(sub) > 0)));
}

static bool char_not_hidden(const struct char_data *ch)
{
  return (!char_aff_flagged(ch, AFF_HIDE));
}

static bool char_invis_ok_obj(const struct char_data *sub, const struct obj_data *obj)
{
  return (!obj_extra_flagged((obj), ITEM_INVISIBLE) || char_aff_flagged((sub), AFF_DETECT_INVIS));
}

static bool char_can_see_obj_carrier(GameWorld *g, const struct char_data *sub, const struct obj_data *obj)
{
  return ((!obj_get_carried_by(obj) || char_can_see(g, sub, obj_get_carried_by(obj))) && (!obj_get_worn_by(obj) || char_can_see(g, sub, obj_get_worn_by(obj))));
}

static bool char_mort_can_see_obj(GameWorld *g, const struct char_data *sub, const struct obj_data *obj)
{
  return ((char_light_ok(g, sub) || obj_get_carried_by(obj) == sub || obj_get_worn_by(obj)) && char_invis_ok_obj(sub, obj) && char_can_see_obj_carrier(g, sub, obj));
}

bool char_can_see_obj(GameWorld *g, const struct char_data *sub, const struct obj_data *obj)
{
  return (char_mort_can_see_obj(g, sub, obj) || (!char_is_npc(sub) && char_prf_flagged(g, (sub), PRF_HOLYLIGHT)));
}

static bool char_can_carry_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  return (((char_get_carry_weight(ch) + obj_get_weight(obj)) <= getMaxCarryWeight(g, ch)) && ((char_get_carry_items(ch) + 1) <= char_can_carry_n(ch)));
}

bool char_can_get_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  return (obj_wear_flagged((obj), ITEM_WEAR_TAKE) && !obj_get_sitting(g, obj) && char_can_carry_obj(g, (ch), (obj)) && char_can_see_obj(g, (ch), (obj)));
}

static bool char_disg(const struct char_data *ch, const struct char_data *vict)
{
  return ((!char_plr_flagged(ch, PLR_DISGUISED)) || (char_plr_flagged(ch, PLR_DISGUISED) && (char_get_admlevel(vict) > 0 || char_is_npc(vict))));
}

static bool char_introd(struct char_data *ch, struct char_data *vict)
{
  return (ch == vict || readIntro(ch, vict) == 1 || (char_is_npc(vict) || char_is_npc(ch) || (char_get_admlevel(ch) > 0 || char_get_admlevel(vict) > 0)));
}

static bool char_iswiz(const struct char_data *ch, const struct char_data *vict)
{
  return (ch == vict || char_get_admlevel(ch) > 0 || char_get_admlevel(vict) > 0 || char_is_npc(vict) || char_is_npc(ch));
}

const char *char_pers(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  return ((char_disg(ch, vict) ? (char_can_see(g, vict, ch) ? (char_introd(vict, ch) ? (char_iswiz(ch, vict) ? char_get_name(ch) : get_i_name(vict, ch)) : introd_calc(ch)) : "Someone") : char_true_race_name(ch)));
}

const char *obj_shortdesc_for(GameWorld *g, const struct obj_data *obj, const struct char_data *vict)
{
  return (char_can_see_obj(g, (vict), (obj)) ? obj_get_short_description(obj) : "something");
}

const char *obj_fname_for(GameWorld *g, const struct obj_data *obj, const struct char_data *vict)
{
  return (char_can_see_obj(g, (vict), (obj)) ? fname(obj_get_name(obj)) : "something");
}

bool char_can_go(GameWorld *g, const struct char_data *ch, int door)
{
  return (char_get_exit(g, ch, door) && (char_get_exit(g, ch, door)->to_room != NOWHERE) && !IS_SET(char_get_exit(g, ch, door)->exit_info, EX_CLOSED));
}

bool char_is_roshi(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_ROSHI);
}

bool char_is_piccolo(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_PICCOLO);
}

bool char_is_krane(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_KRANE);
}

bool char_is_nail(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_NAIL);
}

bool char_is_bardock(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_BARDOCK);
}

bool char_is_ginyu(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_GINYU);
}

bool char_is_frieza(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_FRIEZA);
}

bool char_is_tapion(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_TAPION);
}

bool char_is_andsix(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_ANDSIX);
}

bool char_is_dabura(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_DABURA);
}

bool char_is_kabito(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_KABITO);
}

bool char_is_jinto(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_JINTO);
}

bool char_is_tsuna(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_TSUNA);
}

bool char_is_kurzak(const struct char_data *ch)
{
  return (char_get_class(ch) == CLASS_KURZAK);
}

bool char_is_assassin(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_ASSASSIN) > 0);
}

bool char_is_blackguard(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_BLACKGUARD) > 0);
}

bool char_is_dragon_disciple(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_DRAGON_DISCIPLE) > 0);
}

bool char_is_duelist(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_DUELIST) > 0);
}

bool char_is_dwarven_defender(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_DWARVEN_DEFENDER) > 0);
}

bool char_is_eldritch_knight(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_ELDRITCH_KNIGHT) > 0);
}

bool char_is_hierophant(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_HIEROPHANT) > 0);
}

bool char_is_horizon_walker(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_HORIZON_WALKER) > 0);
}

bool char_is_loremaster(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_LOREMASTER) > 0);
}

bool char_is_mystic_theurge(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_MYSTIC_THEURGE) > 0);
}

bool char_is_shadowdancer(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_SHADOWDANCER) > 0);
}

bool char_is_thaumaturgist(const struct char_data *ch)
{
  return (char_get_class_ranks(ch, CLASS_THAUMATURGIST) > 0);
}

int char_gold_carry(const struct char_data *ch)
{
  return (char_get_level(ch) < 100 ? (char_get_level(ch) < 50 ? char_get_level(ch) * 10000 : 500000) : 50000000);
}

bool char_is_shadow_dragon1(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON1_VNUM);
}

bool char_is_shadow_dragon2(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON2_VNUM);
}

bool char_is_shadow_dragon3(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON3_VNUM);
}

bool char_is_shadow_dragon4(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON4_VNUM);
}

bool char_is_shadow_dragon5(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON5_VNUM);
}

bool char_is_shadow_dragon6(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON6_VNUM);
}

bool char_is_shadow_dragon7(GameWorld *g, const struct char_data *ch)
{
  return (char_is_npc(ch) && char_get_mob_vnum(g, ch) == SHADOW_DRAGON7_VNUM);
}

bool char_can_grand_master(const struct char_data *ch)
{
  return (char_is_human(ch));
}

bool char_is_humanoid(const struct char_data *ch)
{
  return (!char_is_serpent(ch) && !char_is_animal(ch));
}

bool char_is_restricted_race(const struct char_data *ch)
{
  return (char_is_majin(ch) || char_is_saiyan(ch) || char_is_bio(ch) || char_is_hoshijin(ch));
}

bool char_is_cheap_race(const struct char_data *ch)
{
  return (char_is_truffle(ch) || char_is_mutant(ch) || char_is_konatsu(ch) || char_is_demon(ch) || char_is_kanassan(ch));
}

static bool char_spar_train(GameWorld *g, const struct char_data *ch)
{
  return (char_get_fighting(g, ch) && !char_is_npc(ch) && char_plr_flagged(ch, PLR_SPAR) && !char_is_npc(char_get_fighting(g, ch)) && char_plr_flagged(char_get_fighting(g, ch), PLR_SPAR));
}

bool char_is_nonptrans(const struct char_data *ch)
{
  return (char_is_human(ch) || ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && !char_is_fullpssj(ch) && !char_plr_flagged(ch, PLR_LSSJ) && !char_plr_flagged(ch, PLR_OOZARU)) || char_is_namek(ch) || char_is_mutant(ch) || char_is_icer(ch) || char_is_kai(ch) || char_is_konatsu(ch) || char_is_demon(ch) || char_is_kanassan(ch));
}

static bool char_is_fullpssj(const struct char_data *ch)
{
  return ((char_is_saiyan(ch) && char_plr_flagged(ch, PLR_FPSSJ) && char_plr_flagged(ch, PLR_TRANS1)) || (char_is_halfbreed(ch) && char_plr_flagged(ch, PLR_FPSSJ) && char_plr_flagged(ch, PLR_TRANS1)));
}

bool char_is_transformed(const struct char_data *ch)
{
  return (char_plr_flagged(ch, PLR_TRANS1) || char_plr_flagged(ch, PLR_TRANS2) || char_plr_flagged(ch, PLR_TRANS3) || char_plr_flagged(ch, PLR_TRANS4) || char_plr_flagged(ch, PLR_TRANS5) || char_plr_flagged(ch, PLR_TRANS6) || char_plr_flagged(ch, PLR_OOZARU));
}

bool char_moon_ok(GameWorld *g, const struct char_data *ch)
{
  return (char_has_moon(g, ch) && MOON_TIMECHECK(g) && char_oozaru_ok(ch));
}

static bool char_oozaru_ok(const struct char_data *ch)
{
  return (char_is_oozaru_race(ch) && char_plr_flagged(ch, PLR_STAIL) && !char_is_transformed(ch));
}

static bool char_is_oozaru_race(const struct char_data *ch)
{
  return (char_is_saiyan(ch) || char_is_halfbreed(ch));
}

bool char_ether_stream(GameWorld *g, const struct char_data *ch)
{
  /* race_current_form() gets here for characters not in a room, such as saving during logout */
  if (char_get_in_room(ch) == NOWHERE)
    return false;

  return (room_flagged(g, char_get_in_room(ch), ROOM_EARTH) || room_flagged(g, char_get_in_room(ch), ROOM_AETHER) || room_flagged(g, char_get_in_room(ch), ROOM_NAMEK) || room_planet_zenith(g, char_get_in_room(ch)));
}

bool char_has_moon(GameWorld *g, const struct char_data *ch)
{
  return (room_flagged(g, char_get_in_room(ch), ROOM_VEGETA) || room_flagged(g, char_get_in_room(ch), ROOM_EARTH) || room_flagged(g, char_get_in_room(ch), ROOM_FRIGID) || room_flagged(g, char_get_in_room(ch), ROOM_AETHER));
}

bool char_has_arms(GameWorld *g, const struct char_data *ch)
{
  return (((char_is_npc(ch) && (char_mob_flagged(ch, MOB_LARM) || char_mob_flagged(ch, MOB_RARM))) || char_get_limb_condition(ch, (1) - 1) > 0 || char_get_limb_condition(ch, (2) - 1) > 0 || char_plr_flagged(ch, PLR_CRARM) || char_plr_flagged(ch, PLR_CLARM)) && ((!char_get_grappling(g, ch) && !char_get_grappled(g, ch)) || (char_get_grappling(g, ch) && char_get_grap(ch) == 3) || (char_get_grappled(g, ch) && char_get_grap(ch) != 1 && char_get_grap(ch) != 4)));
}

static bool char_has_legs(GameWorld *g, const struct char_data *ch)
{
  return (((char_is_npc(ch) && (char_mob_flagged(ch, MOB_LLEG) || char_mob_flagged(ch, MOB_RLEG))) || char_get_limb_condition(ch, (3) - 1) > 0 || char_get_limb_condition(ch, (4) - 1) > 0 || char_plr_flagged(ch, PLR_CRLEG) || char_plr_flagged(ch, PLR_CLLEG)) && ((!char_get_grappling(g, ch) && !char_get_grappled(g, ch)) || (char_get_grappling(g, ch) && char_get_grap(ch) == 3) || (char_get_grappled(g, ch) && char_get_grap(ch) != 1)));
}

bool char_is_human(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_HUMAN);
}

bool char_is_saiyan(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_SAIYAN);
}

bool char_is_icer(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_ICER);
}

bool char_is_konatsu(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_KONATSU);
}

bool char_is_namek(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_NAMEK);
}

bool char_is_mutant(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_MUTANT);
}

bool char_is_kanassan(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_KANASSAN);
}

bool char_is_halfbreed(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_HALFBREED);
}

bool char_is_bio(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_BIO);
}

bool char_is_android(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_ANDROID);
}

bool char_is_demon(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_DEMON);
}

bool char_is_majin(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_MAJIN);
}

bool char_is_kai(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_KAI);
}

bool char_is_truffle(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_TRUFFLE);
}

bool char_is_hoshijin(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_HOSHIJIN);
}

bool char_is_animal(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_ANIMAL);
}

bool char_is_saiba(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_SAIBA);
}

bool char_is_serpent(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_SERPENT);
}

bool char_is_ogre(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_OGRE);
}

bool char_is_yardratian(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_YARDRATIAN);
}

bool char_is_arlian(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_ARLIAN);
}

bool char_is_dragon(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_DRAGON);
}

bool char_is_mechanical(const struct char_data *ch)
{
  return (char_get_race(ch) == RACE_MECHANICAL);
}

static bool char_is_male(const struct char_data *ch)
{
  return (char_get_sex(ch) == SEX_MALE);
}

bool char_is_female(const struct char_data *ch)
{
  return (char_get_sex(ch) == SEX_FEMALE);
}

bool char_is_outside(GameWorld *g, const struct char_data *ch)
{
  return (char_outside_roomflag(g, ch) && char_outside_secttype(g, ch));
}

static bool char_outside_roomflag(GameWorld *g, const struct char_data *ch)
{
  return (!room_flagged(g, char_get_in_room(ch), ROOM_INDOORS) && !room_flagged(g, char_get_in_room(ch), ROOM_UNDERGROUND) && !room_flagged(g, char_get_in_room(ch), ROOM_SPACE));
}

static bool char_outside_secttype(GameWorld *g, const struct char_data *ch)
{
  return ((room_sect(g, char_get_in_room(ch)) != SECT_INSIDE) && (room_sect(g, char_get_in_room(ch)) != SECT_UNDERWATER) && (room_sect(g, char_get_in_room(ch)) != SECT_IMPORTANT) && (room_sect(g, char_get_in_room(ch)) != SECT_SHOP) && (room_sect(g, char_get_in_room(ch)) != SECT_SPACE));
}

const char *char_hshr(const struct char_data *ch)
{
  return (char_get_sex(ch) ? (char_get_sex(ch) == SEX_MALE ? "his" : "her") : "its");
}

const char *char_hssh(const struct char_data *ch)
{
  return (char_get_sex(ch) ? (char_get_sex(ch) == SEX_MALE ? "he" : "she") : "it");
}

const char *char_hmhr(const struct char_data *ch)
{
  return (char_get_sex(ch) ? (char_get_sex(ch) == SEX_MALE ? "him" : "her") : "it");
}

const char *char_mafe(const struct char_data *ch)
{
  return (char_get_sex(ch) ? (char_get_sex(ch) == SEX_MALE ? "male" : "female") : "questionably gendered");
}

int char_exit_level(GameWorld *g, const struct char_data *ch)
{
  return (!char_is_npc(ch) ? (char_prf_flagged(g, ch, PRF_AUTOEXIT) ? 1 : 0) + (char_prf_flagged(g, ch, PRF_FULL_EXIT) ? 2 : 0) : 0);
}

int char_get_bank_interest(const struct char_data *ch)
{
  return MIN(25000, (int)(((double)char_get_bank_gold((ch)) * 0.04)));
}

/* Field accessors (formerly accessor macros). */

bitvector_t *char_get_act(const struct char_data *ch)
{
  return const_cast<bitvector_t *>(ch->act); /* writable flag array, as the macro allowed */
}

bitvector_t *char_get_affected_by(const struct char_data *ch)
{
  return const_cast<bitvector_t *>(ch->affected_by); /* writable flag array, as the macro allowed */
}

bitvector_t *char_get_admflags(const struct char_data *ch)
{
  return const_cast<bitvector_t *>(ch->admflags); /* writable flag array, as the macro allowed */
}

room_vnum char_get_in_room(const struct char_data *ch)
{
  return ch->in_room;
}

void char_set_in_room(struct char_data *ch, room_vnum v)
{
  ch->in_room = v;
}

room_vnum char_get_was_in_room(const struct char_data *ch)
{
  return ch->was_in_room;
}

void char_set_was_in_room(struct char_data *ch, room_vnum v)
{
  ch->was_in_room = v;
}

const char *char_get_pc_name(const struct char_data *ch)
{
  return c_str_or_null(ch->name); /* null for none, as the files have it */
}

void char_set_pc_name(struct char_data *ch, const char *v)
{
  ch->name = v ? v : "";
}

void char_set_pc_name(struct char_data *ch, const std::string &v)
{
  ch->name = v;
}

int char_get_starphase(const struct char_data *ch)
{
  return ch->starphase;
}

void char_set_starphase(struct char_data *ch, int v)
{
  ch->starphase = v;
}

int char_get_mimic(const struct char_data *ch)
{
  return ch->mimic;
}

const char *char_get_voice(const struct char_data *ch)
{
  return c_str_or_null(ch->voice); /* null for none, as the files have it */
}

void char_set_voice(struct char_data *ch, const char *v)
{
  ch->voice = v ? v : "";
}

void char_set_voice(struct char_data *ch, const std::string &v)
{
  ch->voice = v;
}

int char_get_transclass(const struct char_data *ch)
{
  return ch->transclass;
}

void char_set_transclass(struct char_data *ch, int v)
{
  ch->transclass = v;
}

const char *char_get_feature(const struct char_data *ch)
{
  return c_str_or_null(ch->feature); /* null for none, as the files have it */
}

void char_set_feature(struct char_data *ch, const char *v)
{
  ch->feature = v ? v : "";
}

void char_set_feature(struct char_data *ch, const std::string &v)
{
  ch->feature = v;
}

const char *char_get_loguser(const struct char_data *ch)
{
  return c_str_or_null(ch->loguser); /* null for none, as the files have it */
}

void char_set_loguser(struct char_data *ch, const char *v)
{
  ch->loguser = v ? v : "";
}

void char_set_loguser(struct char_data *ch, const std::string &v)
{
  ch->loguser = v;
}

int char_get_admlevel(const struct char_data *ch)
{
  return ch->admlevel;
}

void char_set_admlevel(struct char_data *ch, int v)
{
  ch->admlevel = v;
}

int char_get_class_level(const struct char_data *ch)
{
  return ch->level;
}

void char_set_class_level(struct char_data *ch, int v)
{
  ch->level = v;
}

int char_get_level_adj(const struct char_data *ch)
{
  return ch->level_adj;
}

void char_set_level_adj(struct char_data *ch, int v)
{
  ch->level_adj = v;
}

int char_get_race_level(const struct char_data *ch)
{
  return ch->race_level;
}

void char_set_race_level(struct char_data *ch, int v)
{
  ch->race_level = v;
}

int char_get_pfilepos(const struct char_data *ch)
{
  return ch->pfilepos;
}

void char_set_pfilepos(struct char_data *ch, int v)
{
  ch->pfilepos = v;
}

int char_get_race(const struct char_data *ch)
{
  return ch->race;
}

int8_t char_get_hairl(const struct char_data *ch)
{
  return ch->hairl;
}

void char_set_hairl(struct char_data *ch, int8_t v)
{
  ch->hairl = v;
}

int8_t char_get_hairc(const struct char_data *ch)
{
  return ch->hairc;
}

void char_set_hairc(struct char_data *ch, int8_t v)
{
  ch->hairc = v;
}

int8_t char_get_hairs(const struct char_data *ch)
{
  return ch->hairs;
}

void char_set_hairs(struct char_data *ch, int8_t v)
{
  ch->hairs = v;
}

int8_t char_get_skin(const struct char_data *ch)
{
  return ch->skin;
}

void char_set_skin(struct char_data *ch, int8_t v)
{
  ch->skin = v;
}

int8_t char_get_eye(const struct char_data *ch)
{
  return ch->eye;
}

void char_set_eye(struct char_data *ch, int8_t v)
{
  ch->eye = v;
}

int8_t char_get_distfea(const struct char_data *ch)
{
  return ch->distfea;
}

void char_set_distfea(struct char_data *ch, int8_t v)
{
  ch->distfea = v;
}

room_vnum char_get_hometown(const struct char_data *ch)
{
  return ch->hometown;
}

void char_set_hometown(struct char_data *ch, room_vnum v)
{
  ch->hometown = v;
}

uint8_t char_get_weight(const struct char_data *ch)
{
  return ch->weight;
}

void char_set_weight(struct char_data *ch, uint8_t v)
{
  ch->weight = v;
}

uint8_t char_get_height(const struct char_data *ch)
{
  return ch->height;
}

void char_set_height(struct char_data *ch, uint8_t v)
{
  ch->height = v;
}

int8_t char_get_sex(const struct char_data *ch)
{
  return ch->sex;
}

void char_set_sex(struct char_data *ch, int8_t v)
{
  ch->sex = v;
}

int char_get_rp(const struct char_data *ch)
{
  return ch->rp;
}

void char_set_rp(struct char_data *ch, int v)
{
  ch->rp = v;
}

int char_get_rbank(const struct char_data *ch)
{
  return ch->rbank;
}

void char_set_rbank(struct char_data *ch, int v)
{
  ch->rbank = v;
}

int char_get_trp(const struct char_data *ch)
{
  return ch->trp;
}

void char_set_trp(struct char_data *ch, int v)
{
  ch->trp = v;
}

int64_t char_get_suppression(const struct char_data *ch)
{
  return ch->suppression;
}

void char_set_suppression(struct char_data *ch, int64_t v)
{
  ch->suppression = v;
}

int64_t char_get_suppressed(const struct char_data *ch)
{
  return 0;
}

void char_set_suppressed(struct char_data *ch, int64_t v)
{
}

const char *char_get_rdisplay(const struct char_data *ch)
{
  return c_str_or_null(ch->rdisplay); /* null for none, as the files have it */
}

void char_set_rdisplay(struct char_data *ch, const char *v)
{
  ch->rdisplay = v ? v : "";
}

void char_set_rdisplay(struct char_data *ch, const std::string &v)
{
  ch->rdisplay = v;
}

int8_t char_get_aff_abils_str(const struct char_data *ch)
{
  return ch->aff_abils.str;
}

void char_set_aff_abils_str(struct char_data *ch, int8_t v)
{
  ch->aff_abils.str = v;
}

int8_t char_get_aff_abils_dex(const struct char_data *ch)
{
  return ch->aff_abils.dex;
}

void char_set_aff_abils_dex(struct char_data *ch, int8_t v)
{
  ch->aff_abils.dex = v;
}

int8_t char_get_aff_abils_intel(const struct char_data *ch)
{
  return ch->aff_abils.intel;
}

void char_set_aff_abils_intel(struct char_data *ch, int8_t v)
{
  ch->aff_abils.intel = v;
}

int8_t char_get_aff_abils_wis(const struct char_data *ch)
{
  return ch->aff_abils.wis;
}

void char_set_aff_abils_wis(struct char_data *ch, int8_t v)
{
  ch->aff_abils.wis = v;
}

int8_t char_get_aff_abils_con(const struct char_data *ch)
{
  return ch->aff_abils.con;
}

void char_set_aff_abils_con(struct char_data *ch, int8_t v)
{
  ch->aff_abils.con = v;
}

int8_t char_get_aff_abils_cha(const struct char_data *ch)
{
  return ch->aff_abils.cha;
}

void char_set_aff_abils_cha(struct char_data *ch, int8_t v)
{
  ch->aff_abils.cha = v;
}

int64_t char_get_exp(const struct char_data *ch)
{
  return ch->exp;
}

void char_set_exp(struct char_data *ch, int64_t v)
{
  ch->exp = v;
}

int char_get_death_type(const struct char_data *ch)
{
  return ch->death_type;
}

void char_set_death_type(struct char_data *ch, int v)
{
  ch->death_type = v;
}

int char_get_sleeptime(const struct char_data *ch)
{
  return ch->sleeptime;
}

void char_set_sleeptime(struct char_data *ch, int v)
{
  ch->sleeptime = v;
}

int char_get_foodr(const struct char_data *ch)
{
  return ch->foodr;
}

void char_set_foodr(struct char_data *ch, int v)
{
  ch->foodr = v;
}

int char_get_altitude(const struct char_data *ch)
{
  return ch->altitude;
}

void char_set_altitude(struct char_data *ch, int v)
{
  ch->altitude = v;
}

int64_t char_get_charge(const struct char_data *ch)
{
  return ch->charge;
}

void char_set_charge(struct char_data *ch, int64_t v)
{
  ch->charge = v;
}

int64_t char_get_chargeto(const struct char_data *ch)
{
  return ch->chargeto;
}

void char_set_chargeto(struct char_data *ch, int64_t v)
{
  ch->chargeto = v;
}

int char_get_armor(const struct char_data *ch)
{
  return ch->armor;
}

void char_set_armor(struct char_data *ch, int v)
{
  ch->armor = v;
}

int64_t char_get_ki(const struct char_data *ch)
{
  return 0;
}

void char_set_ki(struct char_data *ch, int64_t v)
{
}

int64_t char_get_max_ki(const struct char_data *ch)
{
  return ch->max_ki;
}

void char_set_max_ki(struct char_data *ch, int64_t v)
{
  ch->max_ki = v;
}

room_vnum char_get_droom(const struct char_data *ch)
{
  return ch->droom;
}

void char_set_droom(struct char_data *ch, room_vnum v)
{
  ch->droom = v;
}

int char_get_overf(const struct char_data *ch)
{
  return ch->overf;
}

void char_set_overf(struct char_data *ch, int v)
{
  ch->overf = v;
}

int char_get_spam(const struct char_data *ch)
{
  return ch->spam;
}

void char_set_spam(struct char_data *ch, int v)
{
  ch->spam = v;
}

time_t char_get_lastpl(const struct char_data *ch)
{
  return ch->lastpl;
}

void char_set_lastpl(struct char_data *ch, time_t v)
{
  ch->lastpl = v;
}

time_t char_get_deathtime(const struct char_data *ch)
{
  return ch->deathtime;
}

void char_set_deathtime(struct char_data *ch, time_t v)
{
  ch->deathtime = v;
}

time_t char_get_rewtime(const struct char_data *ch)
{
  return ch->rewtime;
}

void char_set_rewtime(struct char_data *ch, time_t v)
{
  ch->rewtime = v;
}

int char_get_dcount(const struct char_data *ch)
{
  return ch->dcount;
}

void char_set_dcount(struct char_data *ch, int v)
{
  ch->dcount = v;
}

time_t char_get_lboard(const struct char_data *ch, int i)
{
  return ch->lboard[i];
}

void char_set_lboard(struct char_data *ch, int i, time_t v)
{
  ch->lboard[i] = v;
}

int char_get_limb_condition(const struct char_data *ch, int i)
{
  return ch->limb_condition[i];
}

void char_set_limb_condition(struct char_data *ch, int i, int v)
{
  ch->limb_condition[i] = v;
}

int char_get_powerattack(const struct char_data *ch)
{
  return ch->powerattack;
}

void char_set_powerattack(struct char_data *ch, int v)
{
  ch->powerattack = v;
}

int char_get_bonuses(const struct char_data *ch, int i)
{
  return ch->bonuses[i];
}

void char_set_bonuses(struct char_data *ch, int i, int v)
{
  ch->bonuses[i] = v;
}

int char_get_transcost(const struct char_data *ch, int i)
{
  return ch->transcost[i];
}

void char_set_transcost(struct char_data *ch, int i, int v)
{
  ch->transcost[i] = v;
}

int char_get_ccpoints(const struct char_data *ch)
{
  return ch->ccpoints;
}

void char_set_ccpoints(struct char_data *ch, int v)
{
  ch->ccpoints = v;
}

int char_get_negcount(const struct char_data *ch)
{
  return ch->negcount;
}

void char_set_negcount(struct char_data *ch, int v)
{
  ch->negcount = v;
}

int char_get_genome(const struct char_data *ch, int i)
{
  return ch->genome[i];
}

void char_set_genome(struct char_data *ch, int i, int v)
{
  ch->genome[i] = v;
}

int char_get_combo(const struct char_data *ch)
{
  return ch->combo;
}

void char_set_combo(struct char_data *ch, int v)
{
  ch->combo = v;
}

int char_get_lastattack(const struct char_data *ch)
{
  return ch->lastattack;
}

void char_set_lastattack(struct char_data *ch, int v)
{
  ch->lastattack = v;
}

int char_get_combhits(const struct char_data *ch)
{
  return ch->combhits;
}

void char_set_combhits(struct char_data *ch, int v)
{
  ch->combhits = v;
}

int char_get_aura(const struct char_data *ch)
{
  return ch->aura;
}

void char_set_aura(struct char_data *ch, int v)
{
  ch->aura = v;
}

room_vnum char_get_radar1(const struct char_data *ch)
{
  return ch->radar1;
}

void char_set_radar1(struct char_data *ch, room_vnum v)
{
  ch->radar1 = v;
}

room_vnum char_get_radar2(const struct char_data *ch)
{
  return ch->radar2;
}

void char_set_radar2(struct char_data *ch, room_vnum v)
{
  ch->radar2 = v;
}

room_vnum char_get_radar3(const struct char_data *ch)
{
  return ch->radar3;
}

void char_set_radar3(struct char_data *ch, room_vnum v)
{
  ch->radar3 = v;
}

int char_get_ping(const struct char_data *ch)
{
  return ch->ping;
}

void char_set_ping(struct char_data *ch, int v)
{
  ch->ping = v;
}

int char_get_skill_slots(const struct char_data *ch)
{
  return ch->skill_slots;
}

void char_set_skill_slots(struct char_data *ch, int v)
{
  ch->skill_slots = v;
}

int char_get_tail_growth(const struct char_data *ch)
{
  return ch->tail_growth;
}

void char_set_tail_growth(struct char_data *ch, int v)
{
  ch->tail_growth = v;
}

int char_get_rage_meter(const struct char_data *ch)
{
  return ch->rage_meter;
}

void char_set_rage_meter(struct char_data *ch, int v)
{
  ch->rage_meter = v;
}

int char_get_personality(const struct char_data *ch)
{
  return ch->personality;
}

void char_set_personality(struct char_data *ch, int v)
{
  ch->personality = v;
}

int char_get_combine(const struct char_data *ch)
{
  return ch->combine;
}

void char_set_combine(struct char_data *ch, int v)
{
  ch->combine = v;
}

int char_get_preference(const struct char_data *ch)
{
  return ch->preference;
}

void char_set_preference(struct char_data *ch, int v)
{
  ch->preference = v;
}

int char_get_relax_count(const struct char_data *ch)
{
  return ch->relax_count;
}

void char_set_relax_count(struct char_data *ch, int v)
{
  ch->relax_count = v;
}

int char_get_blesslvl(const struct char_data *ch)
{
  return ch->blesslvl;
}

void char_set_blesslvl(struct char_data *ch, int v)
{
  ch->blesslvl = v;
}

int char_get_asb(const struct char_data *ch)
{
  return ch->asb;
}

void char_set_asb(struct char_data *ch, int v)
{
  ch->asb = v;
}

int char_get_regen(const struct char_data *ch)
{
  return ch->regen;
}

void char_set_regen(struct char_data *ch, int v)
{
  ch->regen = v;
}

int char_get_lifeperc(const struct char_data *ch)
{
  return ch->lifeperc;
}

void char_set_lifeperc(struct char_data *ch, int v)
{
  ch->lifeperc = v;
}

int char_get_speedboost(const struct char_data *ch)
{
  return ch->speedboost;
}

void char_set_speedboost(struct char_data *ch, int v)
{
  ch->speedboost = v;
}

int char_get_backstabcool(const struct char_data *ch)
{
  return ch->backstabcool;
}

void char_set_backstabcool(struct char_data *ch, int v)
{
  ch->backstabcool = v;
}

int char_get_con_cooldown(const struct char_data *ch)
{
  return ch->con_cooldown;
}

void char_set_con_cooldown(struct char_data *ch, int v)
{
  ch->con_cooldown = v;
}

int64_t char_get_barrier(const struct char_data *ch)
{
  return ch->barrier;
}

void char_set_barrier(struct char_data *ch, int64_t v)
{
  ch->barrier = v;
}

int char_get_gold(const struct char_data *ch)
{
  return ch->gold;
}

void char_set_gold(struct char_data *ch, int v)
{
  ch->gold = v;
}

int char_get_kaioken(const struct char_data *ch)
{
  return ch->kaioken;
}

void char_set_kaioken(struct char_data *ch, int v)
{
  ch->kaioken = v;
}

int char_get_boosts(const struct char_data *ch)
{
  return ch->boosts;
}

void char_set_boosts(struct char_data *ch, int v)
{
  ch->boosts = v;
}

int char_get_majinize(const struct char_data *ch)
{
  return ch->majinize;
}

void char_set_majinize(struct char_data *ch, int v)
{
  ch->majinize = v;
}

int64_t char_get_majinizer(const struct char_data *ch)
{
  return ch->majinizer;
}

void char_set_majinizer(struct char_data *ch, int64_t v)
{
  ch->majinizer = v;
}

short char_get_fury(const struct char_data *ch)
{
  return ch->fury;
}

void char_set_fury(struct char_data *ch, short v)
{
  ch->fury = v;
}

int char_get_upgrade(const struct char_data *ch)
{
  return ch->upgrade;
}

void char_set_upgrade(struct char_data *ch, int v)
{
  ch->upgrade = v;
}

int char_get_forgeting(const struct char_data *ch)
{
  return ch->forgeting;
}

void char_set_forgeting(struct char_data *ch, int v)
{
  ch->forgeting = v;
}

int char_get_forgetcount(const struct char_data *ch)
{
  return ch->forgetcount;
}

void char_set_forgetcount(struct char_data *ch, int v)
{
  ch->forgetcount = v;
}

int char_get_bank_gold(const struct char_data *ch)
{
  return ch->bank_gold;
}

void char_set_bank_gold(struct char_data *ch, int v)
{
  ch->bank_gold = v;
}

int char_get_accuracy(const struct char_data *ch)
{
  return ch->accuracy;
}

void char_set_accuracy(struct char_data *ch, int v)
{
  ch->accuracy = v;
}

int char_get_fishstate(const struct char_data *ch)
{
  return ch->fishstate;
}

void char_set_fishstate(struct char_data *ch, int v)
{
  ch->fishstate = v;
}

int char_get_accuracy_mod(const struct char_data *ch)
{
  return ch->accuracy_mod;
}

void char_set_accuracy_mod(struct char_data *ch, int v)
{
  ch->accuracy_mod = v;
}

int64_t char_get_moltexp(const struct char_data *ch)
{
  return ch->moltexp;
}

void char_set_moltexp(struct char_data *ch, int64_t v)
{
  ch->moltexp = v;
}

int char_get_moltlevel(const struct char_data *ch)
{
  return ch->moltlevel;
}

void char_set_moltlevel(struct char_data *ch, int v)
{
  ch->moltlevel = v;
}

int char_get_con_sdcooldown(const struct char_data *ch)
{
  return ch->con_sdcooldown;
}

void char_set_con_sdcooldown(struct char_data *ch, int v)
{
  ch->con_sdcooldown = v;
}

int char_get_ingestLearned(const struct char_data *ch)
{
  return ch->ingestLearned;
}

void char_set_ingestLearned(struct char_data *ch, int v)
{
  ch->ingestLearned = v;
}

int8_t char_get_position(const struct char_data *ch)
{
  return ch->position;
}

void char_set_position(struct char_data *ch, int8_t v)
{
  ch->position = v;
}

int32_t char_get_idnum(const struct char_data *ch)
{
  return ch->idnum;
}

void char_set_idnum(struct char_data *ch, int32_t v)
{
  ch->idnum = v;
}

int32_t char_get_id(const struct char_data *ch)
{
  return ch->id;
}

void char_set_id(struct char_data *ch, int32_t v)
{
  ch->id = v;
}

int char_get_carry_weight(const struct char_data *ch)
{
  return ch->carry_weight;
}

void char_set_carry_weight(struct char_data *ch, int v)
{
  ch->carry_weight = v;
}

int char_get_carry_items(const struct char_data *ch)
{
  return ch->carry_items;
}

void char_set_carry_items(struct char_data *ch, int v)
{
  ch->carry_items = v;
}

struct char_data *char_get_fighting(GameWorld *g, const struct char_data *ch)
{
  return ch->fighting_uid ? char_by_id(g, ch->fighting_uid) : nullptr;
}

void char_set_fighting(struct char_data *ch, struct char_data *v)
{
  ch->fighting_uid = v ? char_get_id(v) : 0;
}

int char_get_combatexpertise(const struct char_data *ch)
{
  return ch->combatexpertise;
}

void char_set_combatexpertise(struct char_data *ch, int v)
{
  ch->combatexpertise = v;
}

int char_get_alignment(const struct char_data *ch)
{
  return ch->alignment;
}

void char_set_alignment(struct char_data *ch, int v)
{
  ch->alignment = v;
}

struct obj_data *char_get_sits(GameWorld *g, const struct char_data *ch)
{
  return ch->sits_uid ? obj_by_id(g, ch->sits_uid) : nullptr;
}

void char_set_sits(struct char_data *ch, struct obj_data *v)
{
  ch->sits_uid = v ? obj_get_id(v) : 0;
}

struct char_data *char_get_mindlink(GameWorld *g, const struct char_data *ch)
{
  return ch->mindlink_uid ? char_by_id(g, ch->mindlink_uid) : nullptr;
}

void char_set_mindlink(struct char_data *ch, struct char_data *v)
{
  ch->mindlink_uid = v ? char_get_id(v) : 0;
}

int char_get_linker(const struct char_data *ch)
{
  return ch->linker;
}

void char_set_linker(struct char_data *ch, int v)
{
  ch->linker = v;
}

int char_get_lasthit(const struct char_data *ch)
{
  return ch->lasthit;
}

void char_set_lasthit(struct char_data *ch, int v)
{
  ch->lasthit = v;
}

struct char_data *char_get_drag(GameWorld *g, const struct char_data *ch)
{
  return ch->drag_uid ? char_by_id(g, ch->drag_uid) : nullptr;
}

void char_set_drag(struct char_data *ch, struct char_data *v)
{
  ch->drag_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_dragged(GameWorld *g, const struct char_data *ch)
{
  return ch->dragged_uid ? char_by_id(g, ch->dragged_uid) : nullptr;
}

void char_set_dragged(struct char_data *ch, struct char_data *v)
{
  ch->dragged_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_grappling(GameWorld *g, const struct char_data *ch)
{
  return ch->grappling_uid ? char_by_id(g, ch->grappling_uid) : nullptr;
}

void char_set_grappling(struct char_data *ch, struct char_data *v)
{
  ch->grappling_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_grappled(GameWorld *g, const struct char_data *ch)
{
  return ch->grappled_uid ? char_by_id(g, ch->grappled_uid) : nullptr;
}

void char_set_grappled(struct char_data *ch, struct char_data *v)
{
  ch->grappled_uid = v ? char_get_id(v) : 0;
}

int char_get_grap(const struct char_data *ch)
{
  return ch->grap;
}

void char_set_grap(struct char_data *ch, int v)
{
  ch->grap = v;
}

struct char_data *char_get_original(GameWorld *g, const struct char_data *ch)
{
  return ch->original_uid ? char_by_id(g, ch->original_uid) : nullptr;
}

void char_set_original(struct char_data *ch, struct char_data *v)
{
  ch->original_uid = v ? char_get_id(v) : 0;
}

short char_get_clones(const struct char_data *ch)
{
  return ch->clones;
}

void char_set_clones(struct char_data *ch, short v)
{
  ch->clones = v;
}

struct char_data *char_get_defender(GameWorld *g, const struct char_data *ch)
{
  return ch->defender_uid ? char_by_id(g, ch->defender_uid) : nullptr;
}

void char_set_defender(struct char_data *ch, struct char_data *v)
{
  ch->defender_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_defending(GameWorld *g, const struct char_data *ch)
{
  return ch->defending_uid ? char_by_id(g, ch->defending_uid) : nullptr;
}

void char_set_defending(struct char_data *ch, struct char_data *v)
{
  ch->defending_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_blocks(GameWorld *g, const struct char_data *ch)
{
  return ch->blocks_uid ? char_by_id(g, ch->blocks_uid) : nullptr;
}

void char_set_blocks(struct char_data *ch, struct char_data *v)
{
  ch->blocks_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_blocked(GameWorld *g, const struct char_data *ch)
{
  return ch->blocked_uid ? char_by_id(g, ch->blocked_uid) : nullptr;
}

void char_set_blocked(struct char_data *ch, struct char_data *v)
{
  ch->blocked_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_absorbing(GameWorld *g, const struct char_data *ch)
{
  return ch->absorbing_uid ? char_by_id(g, ch->absorbing_uid) : nullptr;
}

void char_set_absorbing(struct char_data *ch, struct char_data *v)
{
  ch->absorbing_uid = v ? char_get_id(v) : 0;
}

struct char_data *char_get_absorbby(GameWorld *g, const struct char_data *ch)
{
  return ch->absorbby_uid ? char_by_id(g, ch->absorbby_uid) : nullptr;
}

void char_set_absorbby(struct char_data *ch, struct char_data *v)
{
  ch->absorbby_uid = v ? char_get_id(v) : 0;
}

room_vnum char_get_listenroom(const struct char_data *ch)
{
  return ch->listenroom;
}

void char_set_listenroom(struct char_data *ch, room_vnum v)
{
  ch->listenroom = v;
}

int char_get_eavesdir(const struct char_data *ch)
{
  return ch->eavesdir;
}

void char_set_eavesdir(struct char_data *ch, int v)
{
  ch->eavesdir = v;
}

int char_get_absorbs(const struct char_data *ch)
{
  return ch->absorbs;
}

void char_set_absorbs(struct char_data *ch, int v)
{
  ch->absorbs = v;
}

time_t char_get_lastint(const struct char_data *ch)
{
  return ch->lastint;
}

void char_set_lastint(struct char_data *ch, time_t v)
{
  ch->lastint = v;
}

/* Skills live in player_specials, by id from SKILL_FIRST to SKILL_TABLE_SIZE-1, and only those with something set
 * are there: one that isn't reads as 0. NPCs have no player_specials and so no skills: reads give 0 and writes are
 * ignored. */
static bool has_skills(const struct char_data *ch, int i)
{
  return !char_is_npc(ch) && ch->player_specials && i >= SKILL_FIRST && i < SKILL_TABLE_SIZE;
}

static struct skill_data skill_of(const struct char_data *ch, int i)
{
  if (!has_skills(ch, i))
    return {};
  auto it = ch->player_specials->skills.find(static_cast<uint16_t>(i));
  return it == ch->player_specials->skills.end() ? skill_data{} : it->second;
}

/* Changes skill i with change(skill), dropping it once nothing is set. */
template <typename F>
static void change_skill(struct char_data *ch, int i, F &&change)
{
  if (!has_skills(ch, i))
    return;
  auto &skills         = ch->player_specials->skills;
  struct skill_data &s = skills[static_cast<uint16_t>(i)];
  change(s);
  if (!s.base && !s.mod && !s.perf)
    skills.erase(static_cast<uint16_t>(i));
}

int8_t char_get_skills_mod(const struct char_data *ch, int i)
{
  return skill_of(ch, i).mod;
}

void char_set_skills_mod(struct char_data *ch, int i, int8_t v)
{
  change_skill(ch, i, [v](struct skill_data &s) { s.mod = v; });
}

int8_t char_get_skills_perf(const struct char_data *ch, int i)
{
  return skill_of(ch, i).perf;
}

void char_set_skills_perf(struct char_data *ch, int i, int8_t v)
{
  change_skill(ch, i, [v](struct skill_data &s) { s.perf = v; });
}

int8_t char_get_skills_base(const struct char_data *ch, int i)
{
  return skill_of(ch, i).base;
}

void char_set_skills_base(struct char_data *ch, int i, int8_t v)
{
  change_skill(ch, i, [v](struct skill_data &s) { s.base = v; });
}

bitvector_t *char_get_bodyparts(const struct char_data *ch)
{
  return const_cast<bitvector_t *>(ch->bodyparts); /* writable flag array, as the macro allowed */
}

struct obj_data *char_get_equipment(const struct char_data *ch, int i)
{
  return ch->equipment[i];
}

void char_set_equipment(struct char_data *ch, int i, struct obj_data *v)
{
  ch->equipment[i] = v;
}

mob_vnum char_get_nr(const struct char_data *ch)
{
  return ch->nr;
}

void char_set_nr(struct char_data *ch, mob_vnum v)
{
  ch->nr = v;
}

int8_t char_get_mob_specials_default_pos(const struct char_data *ch)
{
  return ch->mob_specials.default_pos;
}

void char_set_mob_specials_default_pos(struct char_data *ch, int8_t v)
{
  ch->mob_specials.default_pos = v;
}

const id_list &char_get_mob_specials_memory(const struct char_data *ch)
{
  return ch->mob_specials.memory;
}

id_list *char_mob_memory_slot(struct char_data *ch)
{
  return &ch->mob_specials.memory;
}

int char_get_cooldown(const struct char_data *ch)
{
  return ch->cooldown;
}

void char_set_cooldown(struct char_data *ch, int v)
{
  ch->cooldown = v;
}

int char_get_arenawatch(const struct char_data *ch)
{
  return ch->arenawatch;
}

void char_set_arenawatch(struct char_data *ch, int v)
{
  ch->arenawatch = v;
}

int char_get_wait(const struct char_data *ch)
{
  return ch->wait;
}

void char_set_wait(struct char_data *ch, int v)
{
  ch->wait = v;
}

struct room_direction_data *char_get_exit(GameWorld *g, const struct char_data *ch, int door)
{
  return room_get_dir_option(g, char_get_in_room(ch), door);
}

void char_set_exit(GameWorld *g, struct char_data *ch, int door, struct room_direction_data *v)
{
  room_set_dir_option(g, char_get_in_room(ch), door, v);
}

struct room_direction_data *char_get_exit2(GameWorld *g, const struct char_data *ch, int door)
{
  return room_get_dir_option(g, char_get_exit(g, ch, door)->to_room, door);
}

/* Flag setters and toggles. */

static void flag_array_set(bitvector_t *array, int flag, bool on)
{
  if (on)
    SET_BIT_AR(array, flag);
  else
    REMOVE_BIT_AR(array, flag);
}

static bool flag_array_toggle(bitvector_t *array, int flag)
{
  return (TOGGLE_BIT_AR(array, flag) & Q_BIT(flag)) != 0;
}

/* MOB bits only apply to NPCs; MOB_ISNPC is what makes a character one. */
static bool mob_flag_applies(const struct char_data *ch, int flag)
{
  return flag == MOB_ISNPC || char_is_npc(ch);
}

void char_plr_set(struct char_data *ch, int flag, bool on)
{
  if (char_is_npc(ch))
    return;
  flag_array_set(char_get_act(ch), flag, on);
}

void char_mob_set(struct char_data *ch, int flag, bool on)
{
  if (!mob_flag_applies(ch, flag))
    return;
  flag_array_set(char_get_act(ch), flag, on);
}

bool char_mob_toggle(struct char_data *ch, int flag)
{
  if (!mob_flag_applies(ch, flag))
    return false;
  return flag_array_toggle(char_get_act(ch), flag);
}

void char_aff_set(struct char_data *ch, int flag, bool on)
{
  flag_array_set(char_get_affected_by(ch), flag, on);
}

void char_prf_set(GameWorld *g, struct char_data *ch, int flag, bool on)
{
  flag_array_set(char_get_pref(g, ch), flag, on);
}

static void char_adm_set(struct char_data *ch, int flag, bool on)
{
  flag_array_set(char_get_admflags(ch), flag, on);
}

void char_body_set(struct char_data *ch, int flag, bool on)
{
  flag_array_set(char_get_bodyparts(ch), flag, on);
}

bool race_is_people(int r_id)
{
  switch (r_id) {
    case RACE_ANIMAL:
    case RACE_SAIBA:
    case RACE_MECHANICAL:
    case RACE_SPIRIT:
      return false;
    default:
      return true;
  }
}

/* player_specials of a PC; for an NPC, log which field was wanted (with a backtrace) and return NULL. */
static struct player_special_data *ps_of(GameWorld *g, const struct char_data *ch, const char *field)
{
  if (ch->player_specials)
    return ch->player_specials.get();
  mud_log("SYSERR: player-only field '{}' used on NPC {} (#{:d})", field, char_get_name(ch), char_get_mob_vnum(g, ch));
  log_backtrace();
  return nullptr;
}

bitvector_t *char_get_pref(GameWorld *g, const struct char_data *ch)
{
  static bitvector_t none[PR_ARRAY_MAX]; /* NPCs: always reads as "no preferences" */
  struct player_special_data *ps = ps_of(g, ch, "pref");
  if (!ps) {
    memset(none, 0, sizeof(none));
    return none;
  }
  return ps->pref;
}

int char_get_gauntlet(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "gauntlet");
  return ps ? ps->gauntlet : 0;
}

void char_set_gauntlet(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "gauntlet");
  if (ps)
    ps->gauntlet = v;
}

int char_get_tlevel(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "tlevel");
  return ps ? ps->tlevel : 0;
}

void char_set_tlevel(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "tlevel");
  if (ps)
    ps->tlevel = v;
}

/* Position and movement code asks this of any character; NPCs never carry or get carried (do_carry
 * refuses them), so for them it is simply NULL. Setting it on an NPC is still logged. */
struct char_data *char_get_carrying(GameWorld *g, const struct char_data *ch)
{
  return ch->player_specials && ch->player_specials->carrying_uid ? char_by_id(g, ch->player_specials->carrying_uid) : nullptr;
}

void char_set_carrying(GameWorld *g, struct char_data *ch, struct char_data *v)
{
  struct player_special_data *ps = ps_of(g, ch, "carrying");
  if (ps)
    ps->carrying_uid = v ? char_get_id(v) : 0;
}

/* Position and movement code asks this of any character; NPCs never carry or get carried (do_carry
 * refuses them), so for them it is simply NULL. Setting it on an NPC is still logged. */
struct char_data *char_get_carried_by(GameWorld *g, const struct char_data *ch)
{
  return ch->player_specials && ch->player_specials->carried_by_uid ? char_by_id(g, ch->player_specials->carried_by_uid) : nullptr;
}

void char_set_carried_by(GameWorld *g, struct char_data *ch, struct char_data *v)
{
  struct player_special_data *ps = ps_of(g, ch, "carried_by");
  if (ps)
    ps->carried_by_uid = v ? char_get_id(v) : 0;
}

/* Asked of any character when naming its race (juggleRaceName); NPCs have no preference (0). */
int char_get_racial_pref(const struct char_data *ch)
{
  return ch->player_specials ? ch->player_specials->racial_pref : 0;
}

void char_set_racial_pref(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "racial_pref");
  if (ps)
    ps->racial_pref = v;
}

int8_t char_get_conditions(GameWorld *g, const struct char_data *ch, int i)
{
  struct player_special_data *ps = ps_of(g, ch, "conditions");
  return ps ? ps->conditions[i] : 0;
}

void char_set_conditions(GameWorld *g, struct char_data *ch, int i, int8_t v)
{
  struct player_special_data *ps = ps_of(g, ch, "conditions");
  if (ps)
    ps->conditions[i] = v;
}

room_vnum char_get_load_room(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "load_room");
  return ps ? ps->load_room : 0;
}

void char_set_load_room(GameWorld *g, struct char_data *ch, room_vnum v)
{
  struct player_special_data *ps = ps_of(g, ch, "load_room");
  if (ps)
    ps->load_room = v;
}

int char_get_practice_points(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "practice_points");
  return ps ? ps->practice_points : 0;
}

void char_set_practice_points(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "practice_points");
  if (ps)
    ps->practice_points = v;
}

int char_get_ability_trains(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "ability_trains");
  return ps ? ps->ability_trains : 0;
}

void char_set_ability_trains(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "ability_trains");
  if (ps)
    ps->ability_trains = v;
}

int char_get_trainstr(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "trainstr");
  return ps ? ps->trainstr : 0;
}

void char_set_trainstr(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "trainstr");
  if (ps)
    ps->trainstr = v;
}

int char_get_trainint(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "trainint");
  return ps ? ps->trainint : 0;
}

void char_set_trainint(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "trainint");
  if (ps)
    ps->trainint = v;
}

int char_get_traincon(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "traincon");
  return ps ? ps->traincon : 0;
}

void char_set_traincon(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "traincon");
  if (ps)
    ps->traincon = v;
}

int char_get_trainwis(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "trainwis");
  return ps ? ps->trainwis : 0;
}

void char_set_trainwis(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "trainwis");
  if (ps)
    ps->trainwis = v;
}

int char_get_trainagl(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "trainagl");
  return ps ? ps->trainagl : 0;
}

void char_set_trainagl(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "trainagl");
  if (ps)
    ps->trainagl = v;
}

int char_get_trainspd(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "trainspd");
  return ps ? ps->trainspd : 0;
}

void char_set_trainspd(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "trainspd");
  if (ps)
    ps->trainspd = v;
}

int16_t char_get_invis_level(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "invis_level");
  return ps ? ps->invis_level : 0;
}

void char_set_invis_level(GameWorld *g, struct char_data *ch, int16_t v)
{
  struct player_special_data *ps = ps_of(g, ch, "invis_level");
  if (ps)
    ps->invis_level = v;
}

int char_get_wimp_level(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "wimp_level");
  return ps ? ps->wimp_level : 0;
}

void char_set_wimp_level(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "wimp_level");
  if (ps)
    ps->wimp_level = v;
}

int8_t char_get_freeze_level(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "freeze_level");
  return ps ? ps->freeze_level : 0;
}

void char_set_freeze_level(GameWorld *g, struct char_data *ch, int8_t v)
{
  struct player_special_data *ps = ps_of(g, ch, "freeze_level");
  if (ps)
    ps->freeze_level = v;
}

uint8_t char_get_bad_pws(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "bad_pws");
  return ps ? ps->bad_pws : 0;
}

void char_set_bad_pws(GameWorld *g, struct char_data *ch, uint8_t v)
{
  struct player_special_data *ps = ps_of(g, ch, "bad_pws");
  if (ps)
    ps->bad_pws = v;
}

const char *char_get_poofin(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "poofin");
  return ps ? c_str_or_null(ps->poofin) : nullptr;
}

void char_set_poofin(GameWorld *g, struct char_data *ch, const char *v)
{
  char_set_poofin(g, ch, std::string(v ? v : ""));
}

void char_set_poofin(GameWorld *g, struct char_data *ch, const std::string &v)
{
  struct player_special_data *ps = ps_of(g, ch, "poofin");
  if (ps)
    ps->poofin = v;
}

const char *char_get_poofout(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "poofout");
  return ps ? c_str_or_null(ps->poofout) : nullptr;
}

void char_set_poofout(GameWorld *g, struct char_data *ch, const char *v)
{
  char_set_poofout(g, ch, std::string(v ? v : ""));
}

void char_set_poofout(GameWorld *g, struct char_data *ch, const std::string &v)
{
  struct player_special_data *ps = ps_of(g, ch, "poofout");
  if (ps)
    ps->poofout = v;
}

int char_get_olc_zone(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "olc_zone");
  return ps ? ps->olc_zone : 0;
}

void char_set_olc_zone(GameWorld *g, struct char_data *ch, zone_vnum v)
{
  struct player_special_data *ps = ps_of(g, ch, "olc_zone");
  if (ps)
    ps->olc_zone = v;
}

std::list<alias_data> *char_aliases(GameWorld *g, struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "aliases");
  return ps ? &ps->aliases : nullptr;
}

int32_t char_get_last_tell(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "last_tell");
  return ps ? ps->last_tell : 0;
}

void char_set_last_tell(GameWorld *g, struct char_data *ch, int32_t v)
{
  struct player_special_data *ps = ps_of(g, ch, "last_tell");
  if (ps)
    ps->last_tell = v;
}

const char *char_get_host(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "host");
  return ps ? c_str_or_null(ps->host) : nullptr;
}

void char_set_host(GameWorld *g, struct char_data *ch, const char *v)
{
  char_set_host(g, ch, std::string(v ? v : ""));
}

void char_set_host(GameWorld *g, struct char_data *ch, const std::string &v)
{
  struct player_special_data *ps = ps_of(g, ch, "host");
  if (ps)
    ps->host = v;
}

std::deque<std::string> *char_comm_hist(GameWorld *g, const struct char_data *ch, int i)
{
  struct player_special_data *ps = ps_of(g, ch, "comm_hist");
  return ps ? &ps->comm_hist[i] : nullptr;
}

/* NPCs always speak the common tongue (mob loading sets it, and mob speech reads it), so this is not a
 * wrong-target use: NPCs read SKILL_LANG_COMMON and writes to them are ignored without logging. */
int char_get_speaking(const struct char_data *ch)
{
  return ch->player_specials ? ch->player_specials->speaking : SKILL_LANG_COMMON;
}

void char_set_speaking(struct char_data *ch, int v)
{
  if (ch->player_specials)
    ch->player_specials->speaking = v;
}

int char_get_feat_points(GameWorld *g, const struct char_data *ch)
{
  struct player_special_data *ps = ps_of(g, ch, "feat_points");
  return ps ? ps->feat_points : 0;
}

void char_set_feat_points(GameWorld *g, struct char_data *ch, int v)
{
  struct player_special_data *ps = ps_of(g, ch, "feat_points");
  if (ps)
    ps->feat_points = v;
}

int char_get_age_years(GameWorld *g, struct char_data *ch)
{
  return age(g, ch)->year;
}

int char_can_carry_n(const struct char_data *ch)
{
  return 50;
}

const char *char_true_race_name(const struct char_data *ch)
{
  return pc_race_types[ch->race];
}

const char *char_sensei_name(const struct char_data *ch)
{
  return pc_class_types[ch->chclass];
}

const char *char_sensei_name_lower(const struct char_data *ch)
{
  return class_names[ch->chclass];
}

const char *char_sensei_style(const struct char_data *ch)
{
  return sensei_style[ch->chclass];
}

const char *char_class_abbr(const struct char_data *ch)
{
  return class_abbrevs[ch->chclass];
}

const char *char_race_abbr(const struct char_data *ch)
{
  return race_abbrevs[ch->race];
}

const char *char_get_alias(const struct char_data *ch)
{
  return c_str_or_null(ch->name); /* null for none, as the files have it */
}

void char_set_alias(struct char_data *ch, const char *v)
{
  ch->name = v ? v : "";
}

void char_set_alias(struct char_data *ch, const std::string &v)
{
  ch->name = v;
}

const char *char_get_sdesc(const struct char_data *ch)
{
  return c_str_or_null(ch->short_descr); /* null for none, as the files have it */
}

void char_set_sdesc(struct char_data *ch, const char *v)
{
  ch->short_descr = v ? v : "";
}

void char_set_sdesc(struct char_data *ch, const std::string &v)
{
  ch->short_descr = v;
}

const char *char_get_ldesc(const struct char_data *ch)
{
  return c_str_or_null(ch->long_descr); /* null for none, as the files have it */
}

void char_set_ldesc(struct char_data *ch, const char *v)
{
  ch->long_descr = v ? v : "";
}

void char_set_ldesc(struct char_data *ch, const std::string &v)
{
  ch->long_descr = v;
}

const char *char_get_ddesc(const struct char_data *ch)
{
  return c_str_or_null(ch->description); /* null for none, as the files have it */
}

void char_set_ddesc(struct char_data *ch, const char *v)
{
  ch->description = v ? v : "";
}

void char_set_ddesc(struct char_data *ch, const std::string &v)
{
  ch->description = v;
}

int8_t char_get_mob_specials_damnodice(const struct char_data *ch)
{
  return ch->mob_specials.damnodice;
}

void char_set_mob_specials_damnodice(struct char_data *ch, int8_t v)
{
  ch->mob_specials.damnodice = v;
}

int8_t char_get_mob_specials_damsizedice(const struct char_data *ch)
{
  return ch->mob_specials.damsizedice;
}

void char_set_mob_specials_damsizedice(struct char_data *ch, int8_t v)
{
  ch->mob_specials.damsizedice = v;
}

int8_t char_get_mob_specials_attack_type(const struct char_data *ch)
{
  return ch->mob_specials.attack_type;
}

void char_set_mob_specials_attack_type(struct char_data *ch, int8_t v)
{
  ch->mob_specials.attack_type = v;
}

std::list<affected_type> &char_affects(struct char_data *ch)
{
  return ch->affected;
}

const std::list<affected_type> &char_affects(const struct char_data *ch)
{
  return ch->affected;
}

int char_get_aggtimer(const struct char_data *ch)
{
  return ch->aggtimer;
}

void char_set_aggtimer(struct char_data *ch, int v)
{
  ch->aggtimer = v;
}

void char_set_baseki(struct char_data *ch, int64_t v)
{
  ch->baseki = v;
}

void char_set_basepl(struct char_data *ch, int64_t v)
{
  ch->basepl = v;
}

void char_set_basest(struct char_data *ch, int64_t v)
{
  ch->basest = v;
}

const id_list &char_get_inventory(const struct char_data *ch)
{
  return ch->carrying;
}

void char_set_class(struct char_data *ch, int v)
{
  ch->chclass = v;
}

int char_get_choice(const struct char_data *ch)
{
  return ch->choice;
}

void char_set_choice(struct char_data *ch, int v)
{
  ch->choice = v;
}

struct descriptor_data *char_get_desc(const struct char_data *ch)
{
  return ch->desc;
}

void char_set_desc(struct char_data *ch, struct descriptor_data *v)
{
  ch->desc = v;
}

void char_set_energy(struct char_data *ch, double v)
{
  ch->energy = v;
}

const id_list &char_get_followers(const struct char_data *ch)
{
  return ch->followers;
}

id_list *char_followers_slot(struct char_data *ch)
{
  return &ch->followers;
}

int char_get_gooptime(const struct char_data *ch)
{
  return ch->gooptime;
}

void char_set_gooptime(struct char_data *ch, int v)
{
  ch->gooptime = v;
}

double char_get_health(const struct char_data *ch)
{
  return ch->health;
}

void char_set_health(struct char_data *ch, double v)
{
  ch->health = v;
}

void char_set_life(struct char_data *ch, double v)
{
  ch->life = v;
}

int char_get_raw_lifebonus(const struct char_data *ch)
{
  return ch->lifebonus;
}

void char_set_raw_lifebonus(struct char_data *ch, int v)
{
  ch->lifebonus = v;
}

int *char_get_limbs(struct char_data *ch)
{
  return ch->limbs;
}

struct char_data *char_get_master(GameWorld *g, const struct char_data *ch)
{
  return ch->master_uid ? char_by_id(g, ch->master_uid) : nullptr;
}

void char_set_master(struct char_data *ch, struct char_data *v)
{
  ch->master_uid = v ? char_get_id(v) : 0;
}

int32_t char_get_master_id(const struct char_data *ch)
{
  return ch->master_id;
}

void char_set_master_id(struct char_data *ch, int32_t v)
{
  ch->master_id = v;
}

int64_t char_get_max_hit(const struct char_data *ch)
{
  return ch->max_hit;
}

void char_set_max_hit(struct char_data *ch, int64_t v)
{
  ch->max_hit = v;
}

void char_set_max_mana(struct char_data *ch, int64_t v)
{
  ch->max_mana = v;
}

int64_t char_get_max_move(const struct char_data *ch)
{
  return ch->max_move;
}

void char_set_max_move(struct char_data *ch, int64_t v)
{
  ch->max_move = v;
}

struct script_memory *char_get_memory(const struct char_data *ch)
{
  return ch->memory;
}

void char_set_memory(struct char_data *ch, struct script_memory *v)
{
  ch->memory = v;
}

void char_set_mimic(struct char_data *ch, int v)
{
  ch->mimic = v;
}

int char_get_mobcharge(const struct char_data *ch)
{
  return ch->mobcharge;
}

void char_set_mobcharge(struct char_data *ch, int v)
{
  ch->mobcharge = v;
}



struct player_special_data *char_get_player_specials(const struct char_data *ch)
{
  return ch->player_specials.get();
}

struct player_special_data *char_make_player_specials(struct char_data *ch)
{
  return ch->player_specials.make();
}

void char_drop_player_specials(struct char_data *ch)
{
  ch->player_specials.reset();
}

struct char_data *char_get_poisonby(GameWorld *g, const struct char_data *ch)
{
  return ch->poisonby_uid ? char_by_id(g, ch->poisonby_uid) : nullptr;
}

void char_set_poisonby(struct char_data *ch, struct char_data *v)
{
  ch->poisonby_uid = v ? char_get_id(v) : 0;
}

struct trig_proto_list *char_get_proto_script(const struct char_data *ch)
{
  return ch->proto_script;
}

void char_set_proto_script(struct char_data *ch, struct trig_proto_list *v)
{
  ch->proto_script = v;
}

void char_set_race(struct char_data *ch, int v)
{
  ch->race = v;
}

int8_t char_get_real_abils_cha(const struct char_data *ch)
{
  return ch->real_abils.cha;
}

void char_set_real_abils_cha(struct char_data *ch, int8_t v)
{
  ch->real_abils.cha = v;
}

int8_t char_get_real_abils_con(const struct char_data *ch)
{
  return ch->real_abils.con;
}

void char_set_real_abils_con(struct char_data *ch, int8_t v)
{
  ch->real_abils.con = v;
}

int8_t char_get_real_abils_dex(const struct char_data *ch)
{
  return ch->real_abils.dex;
}

void char_set_real_abils_dex(struct char_data *ch, int8_t v)
{
  ch->real_abils.dex = v;
}

int8_t char_get_real_abils_intel(const struct char_data *ch)
{
  return ch->real_abils.intel;
}

void char_set_real_abils_intel(struct char_data *ch, int8_t v)
{
  ch->real_abils.intel = v;
}

int8_t char_get_real_abils_str(const struct char_data *ch)
{
  return ch->real_abils.str;
}

void char_set_real_abils_str(struct char_data *ch, int8_t v)
{
  ch->real_abils.str = v;
}

int8_t char_get_real_abils_wis(const struct char_data *ch)
{
  return ch->real_abils.wis;
}

void char_set_real_abils_wis(struct char_data *ch, int8_t v)
{
  ch->real_abils.wis = v;
}

struct script_data *char_get_script(const struct char_data *ch)
{
  return ch->script;
}

void char_set_script(struct char_data *ch, struct script_data *v)
{
  ch->script = v;
}

int char_get_size(const struct char_data *ch)
{
  return ch->size;
}

void char_set_size(struct char_data *ch, int v)
{
  ch->size = v;
}

void char_set_stamina(struct char_data *ch, double v)
{
  ch->stamina = v;
}

const char *char_get_temp_prompt(const struct char_data *ch)
{
  return c_str_or_null(ch->temp_prompt);
}

int char_get_throws(const struct char_data *ch)
{
  return ch->throws;
}

void char_set_throws(struct char_data *ch, int v)
{
  ch->throws = v;
}

time_t char_get_time_birth(const struct char_data *ch)
{
  return ch->time.birth;
}

void char_set_time_birth(struct char_data *ch, time_t v)
{
  ch->time.birth = v;
}

time_t char_get_time_created(const struct char_data *ch)
{
  return ch->time.created;
}

void char_set_time_created(struct char_data *ch, time_t v)
{
  ch->time.created = v;
}

time_t char_get_time_logon(const struct char_data *ch)
{
  return ch->time.logon;
}

void char_set_time_logon(struct char_data *ch, time_t v)
{
  ch->time.logon = v;
}

time_t char_get_time_maxage(const struct char_data *ch)
{
  return ch->time.maxage;
}

void char_set_time_maxage(struct char_data *ch, time_t v)
{
  ch->time.maxage = v;
}

time_t char_get_time_played(const struct char_data *ch)
{
  return ch->time.played;
}

void char_set_time_played(struct char_data *ch, time_t v)
{
  ch->time.played = v;
}

int char_get_timer(const struct char_data *ch)
{
  return ch->timer;
}

void char_set_timer(struct char_data *ch, int v)
{
  ch->timer = v;
}

const char *char_get_raw_title(const struct char_data *ch)
{
  return c_str_or_null(ch->title); /* null for none, as the files have it */
}

void char_set_raw_title(struct char_data *ch, const char *v)
{
  ch->title = v ? v : "";
}

void char_set_raw_title(struct char_data *ch, const std::string &v)
{
  ch->title = v;
}

struct script_data **char_script_slot(struct char_data *ch)
{
  return &ch->script;
}

struct script_memory **char_memory_slot(struct char_data *ch)
{
  return &ch->memory;
}

id_list *char_inventory_slot(struct char_data *ch)
{
  return &ch->carrying;
}

std::string *char_description_slot(struct char_data *ch)
{
  return &ch->description;
}

std::string *char_long_descr_slot(struct char_data *ch)
{
  return &ch->long_descr;
}

std::string *char_short_descr_slot(struct char_data *ch)
{
  return &ch->short_descr;
}

std::string *char_name_slot(struct char_data *ch)
{
  return &ch->name;
}

int8_t *char_real_abils_str_slot(struct char_data *ch)
{
  return &ch->real_abils.str;
}

int8_t *char_real_abils_intel_slot(struct char_data *ch)
{
  return &ch->real_abils.intel;
}

int8_t *char_real_abils_wis_slot(struct char_data *ch)
{
  return &ch->real_abils.wis;
}

int8_t *char_real_abils_dex_slot(struct char_data *ch)
{
  return &ch->real_abils.dex;
}

int8_t *char_real_abils_con_slot(struct char_data *ch)
{
  return &ch->real_abils.con;
}

int8_t *char_real_abils_cha_slot(struct char_data *ch)
{
  return &ch->real_abils.cha;
}

struct obj_data **char_get_equipment_array(struct char_data *ch)
{
  return ch->equipment;
}

void char_unlink_from_affect_list(GameWorld *g, struct char_data *ch)
{
  g->affect_list.remove(char_get_id(ch), ch);
}

void char_unlink_from_combat_list(GameWorld *g, struct char_data *ch)
{
  g->combat_list.remove(char_get_id(ch), ch);
}

void char_reset_aff_abils(struct char_data *ch)
{
  ch->aff_abils = ch->real_abils;
}

void char_copy_time(struct char_data *to, const struct char_data *from)
{
  to->time = from->time;
}

struct char_data *mob_proto_of(GameWorld *g, mob_vnum vnum)
{
  return g->mob_proto.find(vnum);
}

struct index_data *mob_index_of(GameWorld *g, mob_vnum vnum)
{
  return g->mob_index.find(vnum);
}
