#include "character_utils.h"
#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "object_impl.h"
#include "object_macros.h"
#include "room_api.h"
#include "util_macros.h"

#include "act.informative.h"
#include "act.movement.h"
#include "class.h"
#include "guild.h"
#include "combat.h"
#include "comm.h"
#include "consts/affflags.h"
#include "consts/aligns.h"
#include "consts/applies.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/prefflags.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "db.h"
#include "extract.h"
#include "feats.h"
#include "fight.h"
#include "fileop.h"
#include "flags.h"
#include "genzon.h"
#include "local_limits.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_db.h"
#include "skills.h"
#include "spells.h"
#include "stringutils.h"
#include "time.h"
#include "weather.h"
#include "zone_impl.h"

#include "consts/admlevel.h"
#include "consts/constates.h"
#include "consts/exitflags.h"
#include "consts/fightprefs.h"
#include "consts/pulse.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "consts/sizes.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "log.h"
#include "room_macros.h"
#include "room_utils.h"
#include "time_info.h"
#include "weather_db.h"

#include "iterate.hpp"

#include <string>

#define ABS(x) ((x) < 0 ? -(x) : (x))

const char *juggleRaceName(char_data *ch, bool capitalized) {
  static char buf[256];

  int apparent = ch->race;

  switch (apparent) {
  case RACE_HOSHIJIN:
    if (ch->mimic > -1)
      apparent = ch->mimic;
    break;
  case RACE_HALFBREED:
    switch (RACIAL_PREF(ch)) {
    case 1:
      apparent = RACE_HUMAN;
      break;
    case 2:
      apparent = RACE_SAIYAN;
      break;
    }
    break;
  case RACE_ANDROID:
    switch (RACIAL_PREF(ch)) {
    case 1:
      apparent = RACE_ANDROID;
      break;
    case 2:
      apparent = RACE_HUMAN;
      break;
    case 3:
      if (capitalized) {
        return "Robotic-Humanoid";
      } else {
        return "robotic-humanoid";
      }
    }
    break;
  case RACE_SAIYAN:
    if (PLR_FLAGGED(ch, PLR_TAILHIDE)) {
      apparent = RACE_HUMAN;
    }
    break;
  }

  if (capitalized) {
    snprintf(buf, sizeof(buf), "%s", pc_race_types[apparent]);
    return buf;
  } else {
    snprintf(buf, sizeof(buf), "%s", race_names[apparent]);
    return buf;
  }
}

void restore_by(char_data *ch, char_data *healer) {
  restore(ch, true);
  act("You have been fully healed by $N!", FALSE, ch, 0, healer,
      TO_CHAR | TO_SLEEP);
}

void restore(char_data *ch, bool announce) {
  restoreVitalsAnnounced(ch, announce);
  restoreLimbsAnnounced(ch, announce);
  restoreStatusAnnounced(ch, announce);
  restoreLFAnnounced(ch, announce);
}

void resurrect(char_data *ch, int mode) {
  restore(ch, true);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_PDEATH);
  char_from_room(ch);
  if (GET_DROOM(ch) != NOWHERE && GET_DROOM(ch) != 0 && GET_DROOM(ch) != 1) {
    char_to_room(ch, room_by_id(GET_DROOM(ch)));
  } else {
    char_to_room(ch, room_by_id(sensei_start_room(ch->chclass)));
  }
  look_at_room(char_room_get(ch), ch, 0);

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

  if (GET_LEVEL(ch) > 9) {
    int losschance = axion_dice(0);
    send_to_char(
        ch, "@RThe the strain of this type of revival has caused you to be in "
            "a weakened state for a few hours (Game time)! Strength, "
            "constitution, wisdom, intelligence, speed, and agility have been "
            "reduced by a fifth for the duration.@n\r\n");
    char_condition_apply_with_duration(ch, "resurrection_weakness", "resurrection", "normal", dur * SECS_PER_MUD_HOUR);
    if (losschance >= 100) {
      int psloss = rand_number(100, 300);
      char_stat_mod(ch, "practices", -psloss);
      send_to_char(ch, "@R...and a loss of @r%d@R PS!@n", psloss);
      if (GET_PRACTICES(ch, GET_CLASS(ch)) < 0) {
        char_stat_set(ch, "practices", 0);
      }
    }
  }
  GET_DTIME(ch) = 0;
  act("$n's body forms in a pool of @Bblue light@n.", TRUE, ch, nullptr,
      nullptr, TO_ROOM);
}

void ghostify(char_data *ch) {
  restore(ch, true);
  SET_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
  SET_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);

  if (!PRF_FLAGGED(ch, PRF_LKEEP)) {
    if (PLR_FLAGGED(ch, PLR_CLLEG)) {
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CLLEG);
    }
    if (PLR_FLAGGED(ch, PLR_CRLEG)) {
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRLEG);
    }
    if (PLR_FLAGGED(ch, PLR_CRARM)) {
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRARM);
    }
    if (PLR_FLAGGED(ch, PLR_CLARM)) {
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CLARM);
    }
  }

  char_condition_remove_tag(ch, "sleep_aff", "ghostify");

  char_condition_remove(ch, "knocked_out", "restored");
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PARALYZE);
}

void teleport_to(char_data *ch, IDXTYPE rnum) {
  char_from_room(ch);
  char_to_room(ch, room_by_id(rnum));
  look_at_room(char_room_get(ch), ch, 0);
  update_pos(ch);
}

bool in_room_range(char_data *ch, IDXTYPE low_rnum, IDXTYPE high_rnum) {
  return char_room_vnum_get(ch) >= low_rnum &&
         char_room_vnum_get(ch) <= high_rnum;
}

bool in_past(char_data *ch) {
  return room_flagged(char_room_get(ch), ROOM_PAST);
}

bool is_newbie(char_data *ch) { return GET_LEVEL(ch) < 9; }

bool in_northran(char_data *ch) { return in_room_range(ch, 17900, 17999); }

static std::map<int, uint16_t> grav_threshold = {
    {10, 5000},        {20, 20000},     {30, 50000},      {40, 100000},
    {50, 200000},      {100, 400000},   {200, 1000000},   {300, 5000000},
    {400, 8000000},    {500, 15000000}, {1000, 25000000}, {5000, 100000000},
    {10000, 200000000}};

bool can_tolerate_gravity(char_data *ch, int grav) {
  if (IS_NPC(ch))
    return true;
  int tolerance = 0;
  tolerance = MAX(tolerance, sensei_grav_tolerance(ch->chclass));
  if (tolerance >= grav)
    return true;
  return getMaxPL(ch) >= grav_threshold[grav];
}

int calcTier(char_data *ch) {
  int64_t level = char_stat_get(ch, "level");
  int tier = level / 10;
  if ((level % 10) == 0)
    tier--;
  tier = MAX(tier, 0);
  tier = MIN(tier, 9);
  return tier;
}

int64_t calc_soft_cap(char_data *ch) {
  int tier = calcTier(ch);
  auto softmap = get_race(ch->race)->getSoftMap(ch);
  return char_stat_get(ch, "level") * softmap[tier];
}

bool is_soft_cap(char_data *ch, int64_t type) {
  return is_soft_cap_mult(ch, type, 1.0);
}

bool is_soft_cap_mult(char_data *ch, int64_t type, long double mult) {
  if (IS_NPC(ch))
    return true;

  if (char_stat_get(ch, "level") >= 100) {
    return false;
  }
  int64_t cur_cap = calc_soft_cap(ch) * mult;

  int64_t against = 0;

  switch (get_race(ch->race)->getSoftType(ch)) {
  case dbat::race::Fixed:
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
  case dbat::race::Variable:
    against = (getBasePL(ch)) + (getBaseKI(ch)) + (getBaseST(ch));
    if (IS_ANDROID(ch) && type > 0) {
      cur_cap += type;
    }
    break;
  }
  return against >= cur_cap;
}

int wearing_android_canister(char_data *ch) {
  if (!IS_ANDROID(ch))
    return 0;
  struct obj_data *obj = GET_EQ(ch, WEAR_BACKPACK);
  if (!obj)
    return 0;
  switch (GET_OBJ_VNUM(obj)) {
  case 1806:
    return 1;
  case 1807:
    return 2;
  default:
    return 0;
  }
}

int calcGravCost(char_data *ch, int64_t num) {
  int cost = 0;
  int gravity = room_gravity_get(char_room_get(ch));
  if (!can_tolerate_gravity(ch, gravity))
    cost = gravity ^ 2;

  if (!num) {
    if (cost) {
      send_to_char(
          ch, "You sweat bullets straining against the current gravity.\r\n");
    }
    if ((getCurST(ch)) > cost) {
      decCurST(ch, cost);
      return 1;
    } else {
      decCurST(ch, cost);
      return 0;
    }
  } else {
    return (getCurST(ch)) > (cost + num);
  }
}

int64_t getCurHealth(char_data *ch) { return getCurPL(ch); }

int64_t getMaxHealth(char_data *ch) { return getMaxPL(ch); }

double getCurHealthPercent(char_data *ch) { return getCurPLPercent(ch); }

int64_t getPercentOfCurHealth(char_data *ch, double amt) {
  return getPercentOfCurPL(ch, amt);
}

int64_t getPercentOfMaxHealth(char_data *ch, double amt) {
  return getPercentOfMaxPL(ch, amt);
}

bool isFullHealth(char_data *ch) { return isFullPL(ch); }

int64_t setCurHealth(char_data *ch, int64_t amt) {
  return char_meter_set_int(ch, "powerlevel", amt);
}

int64_t setCurHealthPercent(char_data *ch, double amt) {
  return char_meter_set(ch, "powerlevel", 1000000 * ABS(amt));
}

int64_t incCurHealthLimited(char_data *ch, int64_t amt, bool limit_max) {
  return char_meter_mod_int(ch, "powerlevel", amt);
};

int64_t incCurHealth(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "powerlevel", amt);
}

int64_t decCurHealthFloored(char_data *ch, int64_t amt, int64_t floor) {
  return char_meter_mod_int(ch, "powerlevel", -amt);
}

int64_t decCurHealth(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "powerlevel", -amt);
}

int64_t incCurHealthPercentLimited(char_data *ch, double amt, bool limit_max) {
  return char_meter_mod(ch, "powerlevel", 1000000 * ABS(amt));
}

int64_t incCurHealthPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "powerlevel", 1000000 * ABS(amt));
}

int64_t decCurHealthPercentFloored(char_data *ch, double amt, int64_t floor) {
  return char_meter_mod(ch, "powerlevel", 1000000 * -ABS(amt));
}

int64_t decCurHealthPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "powerlevel", 1000000 * -ABS(amt));
}

void restoreHealthAnnounced(char_data *ch, bool announce) {
  if (!isFullHealth(ch))
    char_meter_set(ch, "powerlevel", 1000000);
}

void restoreHealth(char_data *ch) { restoreHealthAnnounced(ch, true); }

int64_t healCurHealth(char_data *ch, int64_t amt) {
  return incCurHealth(ch, amt);
}

int64_t harmCurHealth(char_data *ch, int64_t amt) {
  return decCurHealth(ch, amt);
}

int64_t getCurPL(char_data *ch) {
  return char_meter_current(ch, "powerlevel");
}

int64_t getBasePL(char_data *ch) { return char_stat_get(ch, "powerlevel"); }

double getCurPLPercent(char_data *ch) {
  return (double)getCurPL(ch) / (double)getMaxPL(ch);
}

int64_t getPercentOfCurPL(char_data *ch, double amt) {
  return getCurPL(ch) * ABS(amt);
}

int64_t getPercentOfMaxPL(char_data *ch, double amt) {
  return getMaxPL(ch) * ABS(amt);
}

bool isFullPL(char_data *ch) { return char_meter_full(ch, "powerlevel"); }

int64_t getCurKI(char_data *ch) {
  return char_meter_current(ch, "ki");
}

int64_t getMaxKI(char_data *ch) {
  auto total = char_der_total_get(ch, "ki");
  return total;
}

int64_t getBaseKI(char_data *ch) { return char_stat_get(ch, "ki"); }

double getCurKIPercent(char_data *ch) {
  return (double)getCurKI(ch) / (double)getMaxKI(ch);
}

int64_t getPercentOfCurKI(char_data *ch, double amt) {
  return getCurKI(ch) * ABS(amt);
}

int64_t getPercentOfMaxKI(char_data *ch, double amt) {
  return getMaxKI(ch) * ABS(amt);
}

bool isFullKI(char_data *ch) { return char_meter_full(ch, "ki"); }

int64_t setCurKI(char_data *ch, int64_t amt) {
  return char_meter_set_int(ch, "ki", amt);
}

int64_t setCurKIPercent(char_data *ch, double amt) {
  return char_meter_set(ch, "ki", 1000000 * ABS(amt));
}

int64_t incCurKILimited(char_data *ch, int64_t amt, bool limit_max) {
  return char_meter_mod_int(ch, "ki", amt);
};

int64_t incCurKI(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "ki", amt);
}

int64_t decCurKIFloored(char_data *ch, int64_t amt, int64_t floor) {
  return char_meter_mod_int(ch, "ki", -amt);
}

int64_t decCurKI(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "ki", -amt);
}

int64_t incCurKIPercentLimited(char_data *ch, double amt, bool limit_max) {
  return char_meter_mod(ch, "ki", 1000000 * ABS(amt));
}

int64_t incCurKIPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "ki", 1000000 * ABS(amt));
}

int64_t decCurKIPercentFloored(char_data *ch, double amt, int64_t floor) {
  return char_meter_mod(ch, "ki", 1000000 * -ABS(amt));
}

int64_t decCurKIPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "ki", 1000000 * -ABS(amt));
}

void restoreKIAnnounced(char_data *ch, bool announce) {
  char_meter_set(ch, "ki", 1000000);
}

void restoreKI(char_data *ch) { restoreKIAnnounced(ch, true); }

int64_t getCurST(char_data *ch) {
  return char_meter_current(ch, "stamina");
}

int64_t getMaxST(char_data *ch) {
  auto total = char_der_total_get(ch, "stamina");
  return total;
}

int64_t getBaseST(char_data *ch) { return char_stat_get(ch, "stamina"); }

double getCurSTPercent(char_data *ch) {
  return (double)getCurST(ch) / (double)getMaxST(ch);
}

int64_t getPercentOfCurST(char_data *ch, double amt) {
  return getCurST(ch) * ABS(amt);
}

int64_t getPercentOfMaxST(char_data *ch, double amt) {
  return getMaxST(ch) * ABS(amt);
}

bool isFullST(char_data *ch) { return char_meter_full(ch, "stamina"); }

int64_t setCurST(char_data *ch, int64_t amt) {
  return char_meter_set_int(ch, "stamina", amt);
}

int64_t setCurSTPercent(char_data *ch, double amt) {
  return char_meter_set(ch, "stamina", 1000000 * ABS(amt));
}

int64_t incCurSTLimited(char_data *ch, int64_t amt, bool limit_max) {
  return char_meter_mod_int(ch, "stamina", amt);
};

int64_t incCurST(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "stamina", amt);
}

int64_t decCurSTFloored(char_data *ch, int64_t amt, int64_t floor) {
  return char_meter_mod_int(ch, "stamina", -amt);
}

int64_t decCurST(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "stamina", -amt);
}

int64_t incCurSTPercentLimited(char_data *ch, double amt, bool limit_max) {
  return char_meter_mod(ch, "stamina", 1000000 * ABS(amt));
}

int64_t incCurSTPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "stamina", 1000000 * ABS(amt));
}

int64_t decCurSTPercentFloored(char_data *ch, double amt, int64_t floor) {
  return char_meter_mod(ch, "stamina", 1000000 * -ABS(amt));
}

int64_t decCurSTPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "stamina", 1000000 * -ABS(amt));
}

void restoreSTAnnounced(char_data *ch, bool announce) {
  if (!isFullST(ch))
    char_meter_set(ch, "stamina", 1000000);
}

void restoreST(char_data *ch) { restoreSTAnnounced(ch, true); }

int64_t getCurLF(char_data *ch) {
  return char_meter_current(ch, "lifeforce");
}

int64_t getMaxLF(char_data *ch) { return char_der_total_get(ch, "lifeforce"); }

double getCurLFPercent(char_data *ch) { return char_meter_get(ch, "lifeforce") / 1000000.0; }

int64_t getPercentOfCurLF(char_data *ch, double amt) {
  return getCurLF(ch) * ABS(amt);
}

int64_t getPercentOfMaxLF(char_data *ch, double amt) {
  return getMaxLF(ch) * ABS(amt);
}

bool isFullLF(char_data *ch) { return char_meter_full(ch, "lifeforce"); }

int64_t setCurLF(char_data *ch, int64_t amt) {
  char_meter_set_int(ch, "lifeforce", amt);
  return getCurLF(ch);
}

int64_t setCurLFPercent(char_data *ch, double amt) {
  char_meter_set(ch, "lifeforce", 1000000 * ABS(amt));
  return getCurLF(ch);
}

int64_t incCurLF(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "lifeforce", amt);
}

int64_t decCurLFFloored(char_data *ch, int64_t amt, int64_t floor) {
  return char_meter_mod_int(ch, "lifeforce", -amt);
}

int64_t decCurLF(char_data *ch, int64_t amt) {
  return char_meter_mod_int(ch, "lifeforce", -amt);
}

int64_t incCurLFPercentLimited(char_data *ch, double amt, bool limit_max) {
  return char_meter_mod(ch, "lifeforce", 1000000 * ABS(amt));
}

int64_t incCurLFPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "lifeforce", 1000000 * ABS(amt));
}

int64_t decCurLFPercentFloored(char_data *ch, double amt, int64_t floor) {
  return char_meter_mod(ch, "lifeforce", 1000000 * -ABS(amt));
}

int64_t decCurLFPercent(char_data *ch, double amt) {
  return char_meter_mod(ch, "lifeforce", 1000000 * -ABS(amt));
}

void restoreLFAnnounced(char_data *ch, bool announce) {
  if (!isFullLF(ch))
    char_meter_set(ch, "lifeforce", 1000000);
}

void restoreLF(char_data *ch) { restoreLFAnnounced(ch, true); }

bool isFullVitals(char_data *ch) {
  return isFullHealth(ch) && isFullKI(ch) && isFullST(ch);
}

void restoreVitalsAnnounced(char_data *ch, bool announce) {
  restoreHealthAnnounced(ch, announce);
  restoreKIAnnounced(ch, announce);
  restoreSTAnnounced(ch, announce);
}

void restoreVitals(char_data *ch) { restoreVitalsAnnounced(ch, true); }

void restoreStatusAnnounced(char_data *ch, bool announce) {
  cureStatusKnockedOutAnnounced(ch, announce);
  cureStatusBurnAnnounced(ch, announce);
  cureStatusPoisonAnnounced(ch, announce);
}

void restoreStatus(char_data *ch) { restoreStatusAnnounced(ch, true); }

void setStatusKnockedOut(char_data *ch) {
  char_condition_apply(ch, "knocked_out", "combat", "knocked_out");
  if (char_condition_has(ch, "flying")) {
    char_condition_remove(ch, "flying", "stop_flying");
  }
  char_position_set(ch, POS_SLEEPING);
}

void cureStatusKnockedOutAnnounced(char_data *ch, bool announce) {
  if (char_condition_has(ch, "knocked_out")) {
    if (auto cby = CARRIED_BY(ch)) {
      if (GET_ALIGNMENT(cby) > 50) {
        carry_drop(cby, 0);
      } else {
        carry_drop(cby, 1);
      }
    }

    char_condition_remove(ch, "knocked_out", announce ? "recovered" : "recovered_silent");
  }
}

void cureStatusKnockedOut(char_data *ch) {
  cureStatusKnockedOutAnnounced(ch, true);
}

void cureStatusBurnAnnounced(char_data *ch, bool announce) {
  if (char_condition_has(ch, "burned")) {
    if (announce) {
      send_to_char(ch, "Your burns are healed now.\r\n");
      act("$n@w's burns are now healed.@n", TRUE, ch, 0, 0, TO_ROOM);
    }
    char_condition_remove(ch, "burned", "healing_burned");
  }
}

void cureStatusBurn(char_data *ch) { cureStatusBurnAnnounced(ch, true); }

void cureStatusPoisonAnnounced(char_data *ch, bool announce) {
  act("@C$n@W suddenly looks a lot better!@b", FALSE, ch, 0, 0, TO_NOTVICT);
  char_condition_remove(ch, "poison", "healing_poisoned");
}

void cureStatusPoison(char_data *ch) { cureStatusPoisonAnnounced(ch, true); }

static const std::map<int, std::string> limb_names = {
    {1, "right arm"}, {2, "left arm"}, {3, "right leg"}, {4, "left leg"}};

void restoreLimbsAnnounced(char_data *ch, bool announce) {

  for (const auto &l : limb_names) {
    if (announce) {
      if (GET_LIMBCOND(ch, l.first) <= 0)
        send_to_char(ch, "Your %s grows back!\r\n", l.second.c_str());
      else if (GET_LIMBCOND(ch, l.first) < 50)
        send_to_char(ch, "Your %s is no longer broken!\r\n", l.second.c_str());
    }
    SET_LIMBCOND(ch, l.first, 100);
  }

  char_gain_tail(ch, announce);
}

void restoreLimbs(char_data *ch) { restoreLimbsAnnounced(ch, true); }

int64_t gainBasePLTransformed(char_data *ch, int64_t amt, bool trans_mult) {
  char_stat_mod(ch, "powerlevel", amt);
  return char_stat_get(ch, "powerlevel");
}

int64_t gainBasePL(char_data *ch, int64_t amt) {
  return gainBasePLTransformed(ch, amt, false);
}

int64_t gainBaseSTTransformed(char_data *ch, int64_t amt, bool trans_mult) {
  char_stat_mod(ch, "stamina", amt);
  return char_stat_get(ch, "stamina");
}

int64_t gainBaseST(char_data *ch, int64_t amt) {
  return gainBaseSTTransformed(ch, amt, false);
}

int64_t gainBaseKITransformed(char_data *ch, int64_t amt, bool trans_mult) {
  char_stat_mod(ch, "ki", amt);
  return char_stat_get(ch, "ki");
}

int64_t gainBaseKI(char_data *ch, int64_t amt) {
  return gainBaseKITransformed(ch, amt, false);
}

void gainBaseAllTransformed(char_data *ch, int64_t amt, bool trans_mult) {
  gainBasePLTransformed(ch, amt, trans_mult);
  gainBaseKITransformed(ch, amt, trans_mult);
  gainBaseSTTransformed(ch, amt, trans_mult);
}

void gainBaseAll(char_data *ch, int64_t amt) {
  gainBaseAllTransformed(ch, amt, false);
}

int64_t loseBasePLTransformed(char_data *ch, int64_t amt, bool trans_mult) {
  char_stat_set(ch, "powerlevel",
                MAX(1L, char_stat_get(ch, "powerlevel") - amt));
  return char_stat_get(ch, "powerlevel");
}

int64_t loseBasePL(char_data *ch, int64_t amt) {
  return loseBasePLTransformed(ch, amt, false);
}

int64_t loseBaseSTTransformed(char_data *ch, int64_t amt, bool trans_mult) {
  char_stat_set(ch, "stamina", MAX(1L, char_stat_get(ch, "stamina") - amt));
  return char_stat_get(ch, "stamina");
}

int64_t loseBaseST(char_data *ch, int64_t amt) {
  return loseBaseSTTransformed(ch, amt, false);
}

int64_t loseBaseKITransformed(char_data *ch, int64_t amt, bool trans_mult) {
  char_stat_set(ch, "ki", MAX(1L, char_stat_get(ch, "ki") - amt));
  return char_stat_get(ch, "ki");
}

int64_t loseBaseKI(char_data *ch, int64_t amt) {
  return loseBaseKITransformed(ch, amt, false);
}

void loseBaseAllTransformed(char_data *ch, int64_t amt, bool trans_mult) {
  loseBasePLTransformed(ch, amt, trans_mult);
  loseBaseKITransformed(ch, amt, trans_mult);
  loseBaseSTTransformed(ch, amt, trans_mult);
}

void loseBaseAll(char_data *ch, int64_t amt) {
  loseBaseAllTransformed(ch, amt, false);
}

int64_t gainBasePLPercentTransformed(char_data *ch, double amt,
                                     bool trans_mult) {
  return gainBasePLTransformed(ch, char_stat_get(ch, "powerlevel") * amt,
                               trans_mult);
}

int64_t gainBasePLPercent(char_data *ch, double amt) {
  return gainBasePLPercentTransformed(ch, amt, false);
}

int64_t gainBaseKIPercentTransformed(char_data *ch, double amt,
                                     bool trans_mult) {
  return gainBaseKITransformed(ch, char_stat_get(ch, "ki") * amt, trans_mult);
}

int64_t gainBaseKIPercent(char_data *ch, double amt) {
  return gainBaseKIPercentTransformed(ch, amt, false);
}

int64_t gainBaseSTPercentTransformed(char_data *ch, double amt,
                                     bool trans_mult) {
  return gainBaseSTTransformed(ch, char_stat_get(ch, "stamina") * amt,
                               trans_mult);
}

int64_t gainBaseSTPercent(char_data *ch, double amt) {
  return gainBaseSTPercentTransformed(ch, amt, false);
}

int64_t loseBasePLPercentTransformed(char_data *ch, double amt,
                                     bool trans_mult) {
  return loseBasePLTransformed(ch, char_stat_get(ch, "powerlevel") * amt,
                               trans_mult);
}

int64_t loseBasePLPercent(char_data *ch, double amt) {
  return loseBasePLPercentTransformed(ch, amt, false);
}

int64_t loseBaseKIPercentTransformed(char_data *ch, double amt,
                                     bool trans_mult) {
  return loseBaseKITransformed(ch, char_stat_get(ch, "ki") * amt, trans_mult);
}

int64_t loseBaseKIPercent(char_data *ch, double amt) {
  return loseBaseKIPercentTransformed(ch, amt, false);
}

int64_t loseBaseSTPercentTransformed(char_data *ch, double amt,
                                     bool trans_mult) {
  return loseBaseSTTransformed(ch, char_stat_get(ch, "stamina") * amt,
                               trans_mult);
}

int64_t loseBaseSTPercent(char_data *ch, double amt) {
  return loseBaseSTPercentTransformed(ch, amt, false);
}

void gainBaseAllPercentTransformed(char_data *ch, double amt, bool trans_mult) {
  gainBasePLPercentTransformed(ch, amt, trans_mult);
  gainBaseKIPercentTransformed(ch, amt, trans_mult);
  gainBaseSTPercentTransformed(ch, amt, trans_mult);
}

void gainBaseAllPercent(char_data *ch, double amt) {
  gainBaseAllPercentTransformed(ch, amt, false);
}

void loseBaseAllPercentTransformed(char_data *ch, double amt, bool trans_mult) {
  loseBasePLPercentTransformed(ch, amt, trans_mult);
  loseBaseKIPercentTransformed(ch, amt, trans_mult);
  loseBaseSTPercentTransformed(ch, amt, trans_mult);
}

void loseBaseAllPercent(char_data *ch, double amt) {
  loseBaseAllPercentTransformed(ch, amt, false);
}

int64_t getMaxCarryWeight(char_data *ch) {
  return char_der_total_get(ch, "weight_carry_capacity");
}

int64_t getCurGearWeight(char_data *ch) {
  return char_der_total_get(ch, "weight_equipment");
}

int64_t getCurCarriedWeight(char_data *ch) {
  return char_der_total_get(ch, "weight_carried");
}

int64_t getAvailableCarryWeight(char_data *ch) {
  return getMaxCarryWeight(ch) - getCurCarriedWeight(ch);
}

// speednar is in utils.cpp

int64_t getMaxPL(char_data *ch) {
  auto total = char_der_total_get(ch, "powerlevel");
  return total;
}

bool isWeightedPL(char_data *ch) { return false; }

void apply_kaioken(char_data *ch, int times, bool announce) {
  char_stat_set(ch, "kaioken", times);
  char_condition_apply_with_number(ch, "kaioken", "command", "kaioken", "level",
                                   times);
  char_condition_remove(ch, "powering_up", "kaioken");

  if (announce) {
    send_to_char(ch,
                 "@rA dark red aura bursts up around your body as you achieve "
                 "Kaioken x %d!@n\r\n",
                 times);
    act("@rA dark red aura bursts up around @R$n@r as they achieve a level of "
        "Kaioken!@n",
        TRUE, ch, 0, 0, TO_ROOM);
  }
}

void remove_kaioken(char_data *ch, int8_t announce) {
  int8_t kaio = GET_KAIOKEN(ch);
  if (!kaio) {
    return;
  }
  char_stat_set(ch, "kaioken", 0);
  char_condition_remove(ch, "kaioken", "kaioken_removed");

  switch (announce) {
  case 1:
    send_to_char(ch, "You drop out of kaioken.\r\n");
    act("$n@w drops out of kaioken.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  case 2:
    send_to_char(ch, "You lose focus and your kaioken disappears.\r\n");
    act("$n loses focus and $s kaioken aura disappears.", TRUE, ch, 0, 0,
        TO_ROOM);
  }
}

void dispel_ash(struct char_data *ch) {
  struct obj_data *obj, *next_obj, *ash = nullptr;
  int there = FALSE;

  room_contents_iterate(char_room_get(ch), [&](auto obj) {
    if (GET_OBJ_VNUM(obj) == 1306) {
      there = TRUE;
      ash = obj;
    }
    return true;
  });

  if (ash) {
    int roll = axion_dice(0);
    if (GET_OBJ_COST(ash) == 3) {
      if (GET_INT(ch) > roll) {
        act("@GYou clear the air with the shockwaves of your power!@n", TRUE,
            ch, ash, 0, TO_CHAR);
        act("@C$n@G clears the air with the shockwaves of $s power!@n", TRUE,
            ch, ash, 0, TO_ROOM);
        extract_obj(ash);
      }
    } else if (GET_OBJ_COST(ash) == 2) {
      if (GET_INT(ch) + 10 > roll) {
        act("@GYou clear the air with the shockwaves of your power!@n", TRUE,
            ch, ash, 0, TO_CHAR);
        act("@C$n@G clears the air with the shockwaves of $s power!@n", TRUE,
            ch, ash, 0, TO_ROOM);
        extract_obj(ash);
      }
    } else if (GET_OBJ_COST(ash) == 1) {
      if (GET_INT(ch) + 20 > roll) {
        act("@GYou clear the air with the shockwaves of your power!@n", TRUE,
            ch, ash, 0, TO_CHAR);
        act("@C$n@G clears the air with the shockwaves of $s power!@n", TRUE,
            ch, ash, 0, TO_ROOM);
        extract_obj(ash);
      }
    }
  }
}

int has_group(struct char_data *ch) {

  if (!char_condition_has(ch, "group"))
    return (FALSE);

  if (char_follower_count(ch)) {
    bool found = false;
    char_followers_iterate(ch, [&](struct char_data *fol) {
      if (char_condition_has(fol, "group")) { found = true; return false; }
      return true;
    });
    if (found) return (TRUE);
  } else if (MASTER(ch)) {
    if (!char_condition_has(MASTER(ch), "group"))
      return (FALSE);
    else
      return (TRUE);
  }

  return (FALSE);
}

const char *report_party_health(struct char_data *ch) {
  if (!char_condition_has(ch, "group"))
    return "";
  if (!char_follower_count(ch) && !MASTER(ch))
    return "";

  static const char *plcol[] = {"@r", "@y", "@Y", "@G", ""};
  static const char *exhaust[] = {
    "Exhausted", "Strained", "Very Tired", "Tired",
    "Kinda Tired", "Very Winded", "Winded", "Energetic", "?????????"
  };
  static const char *excol[] = {"@r", "@R", "@R", "@M", "@M", "@M", "@G", "@g", "@w"};

  auto member_stam = [](char_data *p) -> int {
    int64_t cur = getCurST(p), max = GET_MAX_MOVE(p);
    if (cur >= max)       return 7;
    if (cur >= max * 0.9) return 6;
    if (cur >= max * 0.8) return 5;
    if (cur >= max * 0.7) return 4;
    if (cur >= max * 0.5) return 3;
    if (cur >= max * 0.4) return 2;
    if (cur >= max * 0.2) return 1;
    return 0;
  };

  /* collect up to 4 grouped party members (excluding ch) */
  char_data *party[4] = {};
  int n = 0;
  bool is_leader = char_follower_count(ch) > 0;

  if (is_leader) {
    char_followers_iterate(ch, [&](struct char_data *fol) {
      if (n >= 4) return false;
      if (fol != ch && char_condition_has(fol, "group"))
        party[n++] = fol;
      return true;
    });
  } else if (MASTER(ch) && char_condition_has(MASTER(ch), "group")) {
    party[n++] = MASTER(ch);
    char_followers_iterate(MASTER(ch), [&](struct char_data *fol) {
      if (n >= 4) return false;
      if (fol != ch && char_condition_has(fol, "group"))
        party[n++] = fol;
      return true;
    });
  } else {
    return "";
  }

  int64_t plperc[4] = {}, kiperc[4] = {};
  int plc[4] = {4, 4, 4, 4}, stam[4] = {8, 8, 8, 8};
  for (int i = 0; i < 4; ++i) {
    if (!party[i]) continue;
    plperc[i] = (GET_HIT(party[i]) * 100) / GET_MAX_HIT(party[i]);
    kiperc[i] = (GET_CHARGE(party[i]) * 100) / GET_MAX_MANA(party[i]);
    int64_t p  = plperc[i];
    plc[i]     = p >= 80 ? 3 : p >= 50 ? 2 : p >= 30 ? 1 : 0;
    stam[i]    = member_stam(party[i]);
  }

  char buf[MAX_STRING_LENGTH];
  if (is_leader) {
    snprintf(buf, sizeof(buf),
      "@D[@BG@D]-------@mF@D------- -------@mF@D------- "
      "-------@mF@D------- -------@mF@D-------[@BG@D] <@RV@Y%s@R>@n\n"
      "@D[@BR@D]@C%-15s %-15s %-15s %-15s@D[@BR@D]@n\n"
      "@D[@BO@D]@RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T
      "@w%s @RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s@D[@BO@D]@n\n"
      "@D[@BU@D]@cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T
      "@w%s @cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s@D[@BU@D]@n\n"
      "@D[@BP@D]@gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s "
      "@gSt@D:%s%12s@D[@BP@D]@n\n",
      add_commas(GET_GROUPKILLS(ch)),
      party[0] ? get_i_name(ch, party[0]) : "Empty",
      party[1] ? get_i_name(ch, party[1]) : "Empty",
      party[2] ? get_i_name(ch, party[2]) : "Empty",
      party[3] ? get_i_name(ch, party[3]) : "Empty",
      plcol[plc[0]], plperc[0], "%", plcol[plc[1]], plperc[1], "%",
      plcol[plc[2]], plperc[2], "%", plcol[plc[3]], plperc[3], "%",
      kiperc[0], "%", kiperc[1], "%", kiperc[2], "%", kiperc[3], "%",
      excol[stam[0]], exhaust[stam[0]], excol[stam[1]], exhaust[stam[1]],
      excol[stam[2]], exhaust[stam[2]], excol[stam[3]], exhaust[stam[3]]);
  } else {
    snprintf(buf, sizeof(buf),
      "@D[@BG@D]-------@YL@D------- -------@mF@D------- "
      "-------@mF@D------- -------@mF@D-------[@BG@D]@n\n"
      "@D[@BR@D]@C%-15s %-15s %-15s %-15s@D[@BR@D]@n\n"
      "@D[@BO@D]@RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T
      "@w%s @RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s@D[@BO@D]@n\n"
      "@D[@BU@D]@cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T
      "@w%s @cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s@D[@BU@D]@n\n"
      "@D[@BP@D]@gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s "
      "@gSt@D:%s%12s@D[@BP@D]@n\n",
      party[0] ? get_i_name(ch, party[0]) : "Empty",
      party[1] ? get_i_name(ch, party[1]) : "Empty",
      party[2] ? get_i_name(ch, party[2]) : "Empty",
      party[3] ? get_i_name(ch, party[3]) : "Empty",
      plcol[plc[0]], plperc[0], "%", plcol[plc[1]], plperc[1], "%",
      plcol[plc[2]], plperc[2], "%", plcol[plc[3]], plperc[3], "%",
      kiperc[0], "%", kiperc[1], "%", kiperc[2], "%", kiperc[3], "%",
      excol[stam[0]], exhaust[stam[0]], excol[stam[1]], exhaust[stam[1]],
      excol[stam[2]], exhaust[stam[2]], excol[stam[3]], exhaust[stam[3]]);
  }

  ch->temp_prompt = strdup(buf);
  return ch->temp_prompt;
}

/* Check to see if anything interferes with their "knowing" the skill */
int know_skill(struct char_data *ch, int skill) {

  int know = 0;

  if (GET_SKILL(ch, skill) > 0)
    know = 1;

  if (know == 0) {
    send_to_char(ch, "You do not know how to perform %s.\r\n",
                 spell_info[skill].name);
    know = 0;
  } else if (know == 2) {
    send_to_char(ch,
                 "@WYou try to use @M%s@W but lingering thoughts of a certain "
                 "kiss distracts you!@n\r\n",
                 spell_info[skill].name);
    send_to_char(ch, "You must sleep in order to cure this.\r\n");
    know = 0;
  }

  return (know);
}

bool is_affected(struct char_data *ch, int aff_flag) {
  if (AFF_FLAGGED(ch, aff_flag)) return true;

  if (char_condition_check_legacy_affect(ch, aff_flag)) return true;

  bool found = false;

  char_equipment_iterate(ch, [&](auto i, auto eq) {
    if(obj_aff_flagged(eq, aff_flag)) {
      found = true;
      return false;
    }
    return true;
  });

  return found;
}

int sec_roll_check(struct char_data *ch) {

  int figure = 0, chance = 0, outcome = 0;

  figure = 10 + (GET_LEVEL(ch) * 1.6);

  chance = axion_dice(0) + axion_dice(0) + rand_number(0, 20);

  if (figure >= chance)
    outcome = 1;

  return (outcome);
}

int get_measure(struct char_data *ch, int height, int weight) {
  int amt = 0;
  if (!PLR_FLAGGED(ch, PLR_OOZARU) && (!IS_ICER(ch) || !IS_TRANSFORMED(ch)) &&
      GET_GENOME(ch, 0) < 9) {
    if (height > 0) {
      amt = height;
    } else if (weight > 0) {
      amt = weight;
    }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
    if (height > 0) {
      amt = height * 3;
    } else if (weight > 0) {
      amt = weight * 4;
    }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
    if (height > 0) {
      amt = height * 3;
    } else if (weight > 0) {
      amt = weight * 5;
    }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
    if (height > 0) {
      amt = height * 1.5;
    } else if (weight > 0) {
      amt = weight * 2;
    }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS4)) {
    if (height > 0) {
      amt = height * 2;
    } else if (weight > 0) {
      amt = weight * 3;
    }
  } else if (PLR_FLAGGED(ch, PLR_OOZARU) || GET_GENOME(ch, 0) == 9) {
    if (height > 0) {
      amt = height * 10;
    } else if (weight > 0) {
      amt = weight * 50;
    }
  }

  return (amt);
}

int64_t physical_cost(struct char_data *ch, int skill) {

  int64_t result = 0;

  if (skill == SKILL_PUNCH)
    result = GET_MAX_HIT(ch) / 500;
  else if (skill == SKILL_KICK)
    result = GET_MAX_HIT(ch) / 350;
  else if (skill == SKILL_ELBOW)
    result = GET_MAX_HIT(ch) / 400;
  else if (skill == SKILL_KNEE)
    result = GET_MAX_HIT(ch) / 300;
  else if (skill == SKILL_UPPERCUT)
    result = GET_MAX_HIT(ch) / 200;
  else if (skill == SKILL_ROUNDHOUSE)
    result = GET_MAX_HIT(ch) / 150;
  else if (skill == SKILL_HEELDROP)
    result = GET_MAX_HIT(ch) / 80;
  else if (skill == SKILL_SLAM)
    result = GET_MAX_HIT(ch) / 90;

  int cou1 = 1 + rand_number(1, 20), cou2 = cou1 + rand_number(1, 6);

  result += rand_number(cou1, cou2);

  if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100) {
    result -= result * 0.4;
  } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75) {
    if (IS_TSUNA(ch)) {
      result -= result * 0.40;
    } else if (IS_TAPION(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK)) {
      result -= result * 0.35;
    } else if (IS_JINTO(ch)) {
      if (GET_SKILL_BASE(ch, skill) >= 100) {
        result -= result * 0.45;
      } else {
        result -= result * 0.25;
      }
    } else {
      result -= result * 0.25;
    }
  } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 50) {
    result -= result * 0.25;
  }

  if (IS_ANDROID(ch)) {
    result *= 0.25;
  }

  return (result);
}

int trans_cost(struct char_data *ch, int trans) {

  if (GET_TRANSCOST(ch, trans) == 0) {
    return (50);
  } else {
    return (0);
  }
}

/* This returns the trans requirement for the player based on their trans class
 */
/* 3 = Great, 2 = Average, 1 = Terrible */
/* Rillao: transloc, add new transes here */
int trans_req(struct char_data *ch, int trans) {

  int requirement = 0;

  if (IS_HUMAN(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1500000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1800000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 2100000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 37500000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 35000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 32500000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 200000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 190000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 185000000;
      }
      break;
    case 4:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1400000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1200000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1100000000;
      }
      break;
    }
  } /* End Human Requirement */

  if (IS_HALFBREED(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1500000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1400000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1200000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 60000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 55000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 50000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1200000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1050000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 950000000;
      }
      break;
    }
  } /* End Halfbreed Requirement */

  if (IS_SAIYAN(ch)) {
    if (PLR_FLAGGED(ch, PLR_LSSJ)) {
      switch (trans) {
      case 1:
        if (GET_TRANSCLASS(ch) == 1) {
          requirement = 600000;
        } else if (GET_TRANSCLASS(ch) == 2) {
          requirement = 500000;
        } else if (GET_TRANSCLASS(ch) == 3) {
          requirement = 450000;
        }
        break;
      case 2:
        if (GET_TRANSCLASS(ch) == 1) {
          requirement = 300000000;
        } else if (GET_TRANSCLASS(ch) == 2) {
          requirement = 250000000;
        } else if (GET_TRANSCLASS(ch) == 3) {
          requirement = 225000000;
        }
        break;
      }
    } else {
      switch (trans) {
      case 1:
        if (GET_TRANSCLASS(ch) == 1) {
          requirement = 1400000;
        } else if (GET_TRANSCLASS(ch) == 2) {
          requirement = 1200000;
        } else if (GET_TRANSCLASS(ch) == 3) {
          requirement = 1100000;
        }
        break;
      case 2:
        if (GET_TRANSCLASS(ch) == 1) {
          requirement = 60000000;
        } else if (GET_TRANSCLASS(ch) == 2) {
          requirement = 55000000;
        } else if (GET_TRANSCLASS(ch) == 3) {
          requirement = 50000000;
        }
        break;
      case 3:
        if (GET_TRANSCLASS(ch) == 1) {
          requirement = 160000000;
        } else if (GET_TRANSCLASS(ch) == 2) {
          requirement = 150000000;
        } else if (GET_TRANSCLASS(ch) == 3) {
          requirement = 140000000;
        }
        break;
      case 4:
        if (GET_TRANSCLASS(ch) == 1) {
          requirement = 1800000000;
        } else if (GET_TRANSCLASS(ch) == 2) {
          requirement = 1625000000;
        } else if (GET_TRANSCLASS(ch) == 3) {
          requirement = 1400000000;
        }
        break;
      }
    }
  } /* End Saiyan Requirement */

  if (IS_NAMEK(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 400000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 360000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 335000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 10000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 9500000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 8000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 240000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 220000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 200000000;
      }
      break;
    case 4:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 950000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 900000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 875000000;
      }
      break;
    }
  } /* End Namek Requirement */

  if (IS_ICER(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 550000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 500000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 450000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 20000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 17500000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 15000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 180000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 150000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 125000000;
      }
      break;
    case 4:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 880000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 850000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 820000000;
      }
      break;
    }
  } /* End Icer Requirement */

  if (IS_MAJIN(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 2400000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 2200000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 2000000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 50000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 45000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 40000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1800000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1550000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1300000000;
      }
      break;
    }
  } /* End Majin Requirement */

  if (IS_TRUFFLE(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 3800000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 3600000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 3500000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 400000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 300000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 200000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1550000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1450000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1250000000;
      }
      break;
    }
  } /* End Truffle Requirement */

  if (IS_MUTANT(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 200000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 180000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 160000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 30000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 27500000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 25000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 750000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 700000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 675000000;
      }
      break;
    }
  } /* End Mutant Requirement */

  if (IS_KAI(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 3250000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 3000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 2850000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 700000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 650000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 625000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1500000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1300000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1250000000;
      }
      break;
    }
  } /* End Kai Requirement */

  if (IS_KONATSU(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 2000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1800000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1600000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 250000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 225000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 200000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1600000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1400000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1300000000;
      }
      break;
    }
  } /* End Konatsu Requirement */

  if (IS_ANDROID(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1200000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 850000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 8500000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 8000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 7750000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 55000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 50000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 40000000;
      }
      break;
    case 4:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 325000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 300000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 275000000;
      }
      break;
    case 5:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 900000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 800000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 750000000;
      }
      break;
    case 6:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1300000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1200000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1100000000;
      }
      break;
    }
  } /* End Android Requirement */

  if (IS_BIO(ch)) {
    switch (trans) {
    case 1:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 2000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1800000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1700000;
      }
      break;
    case 2:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 30000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 25000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 20000000;
      }
      break;
    case 3:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 235000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 220000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 210000000;
      }
      break;
    case 4:
      if (GET_TRANSCLASS(ch) == 1) {
        requirement = 1500000000;
      } else if (GET_TRANSCLASS(ch) == 2) {
        requirement = 1300000000;
      } else if (GET_TRANSCLASS(ch) == 3) {
        requirement = 1150000000;
      }
      break;
    }
  } /* End Bioandroid Requirement */

  return (requirement);
}

const char *disp_align(struct char_data *ch) {
  int align;

  if (GET_ALIGNMENT(ch) < -800) { // Horrifically Evil
    align = 8;
  } else if (GET_ALIGNMENT(ch) < -600) { // Terrible
    align = 7;
  } else if (GET_ALIGNMENT(ch) < -300) { // Villain
    align = 6;
  } else if (GET_ALIGNMENT(ch) < -50) { // Crook
    align = 5;
  } else if (GET_ALIGNMENT(ch) < 51) { // Neutral
    align = 4;
  } else if (GET_ALIGNMENT(ch) < 300) { // Do-gooder
    align = 3;
  } else if (GET_ALIGNMENT(ch) < 600) { // Hero
    align = 2;
  } else if (GET_ALIGNMENT(ch) < 800) { // Valiant
    align = 1;
  } else { // Saintly
    align = 0;
  }

  return (alignments[align]);
}

/* This creates a player's sense memory file for the first time. */
void senseCreate(struct char_data *ch) {
  char fname[40];
  FILE *fl;
  /* Write Sense File */
  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mud_log("ERROR: could not save sense memory of, %s, to filename, %s.",
        GET_NAME(ch), fname);
    return;
  }

  fprintf(fl, "0\n");

  fclose(fl);
  return;
}

int read_sense_memory(struct char_data *ch, struct char_data *vict) {
  char fname[40], line[256];
  int known = FALSE, idnums = -1337;
  FILE *fl;

  /* Read Sense File */
  if (vict == NULL) {
    mud_log("Noone.");
    return 0;
  }

  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch))) {
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
    if (IS_NPC(vict)) {
      if (idnums == GET_MOB_VNUM(vict)) {
        known = TRUE;
      }
    } else {
      if (idnums == GET_ID(vict)) {
        known = TRUE;
      }
    }
  }
  fclose(fl);

  if (known == TRUE)
    return 1;
  else
    return 0;
}

/* This writes a player's sense memory to file. */
void sense_memory_write(struct char_data *ch, struct char_data *vict) {
  FILE *file;
  char fname[40], line[256];
  int idnums[500] = {0};
  FILE *fl;
  int count = 0, x = 0, id_sample;

  /* Read Sense File */
  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch))) {
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

  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mud_log("ERROR: could not save sense memory file, %s, to filename, %s.",
        GET_NAME(ch), fname);
    return;
  }

  while (x < count) {
    if (x == 0 || idnums[x - 1] != idnums[x]) {
      if (!IS_NPC(vict)) {
        if (idnums[x] != GET_ID(vict)) {
          fprintf(fl, "%d\n", idnums[x]);
        }
      } else {
        if (idnums[x] != GET_MOB_VNUM(vict)) {
          fprintf(fl, "%d\n", idnums[x]);
        }
      }
    }
    x++;
  }

  if (!IS_NPC(vict)) {
    fprintf(fl, "%d\n", GET_ID(vict));
  } else {
    fprintf(fl, "%d\n", GET_MOB_VNUM(vict));
  }

  fclose(fl);
  return;
}

/* Will they manage to pursue a fleeing enemy? */
int roll_pursue(struct char_data *ch, struct char_data *vict) {

  int skill, perc = axion_dice(0);

  if (ch == NULL || vict == NULL)
    return (FALSE);

  if (!IS_NPC(ch)) {
    skill = GET_SKILL(ch, SKILL_PURSUIT);
  } else if (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_SENTINEL)) {
    skill = GET_LEVEL(ch);
    if ((char_room_get(vict) && room_flagged(char_room_get(vict), ROOM_NOMOB)))
      skill = -1;
  } else {
    skill = -1;
  }

  if (!IS_NPC(vict)) {
    if (IS_NPC(ch) && !(vict->desc)) {
      skill = -1;
    }
  }

  if (skill > perc) {
    int inroom = char_room_vnum_get(ch);
    act("@C$n@R pursues after the fleeing @c$N@R!@n", TRUE, ch, 0, vict,
        TO_NOTVICT);
    char_from_room(ch);
    char_to_room(ch, char_room_get(vict));
    act("@GYou pursue right after @c$N@G!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@R pursues after you!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@R pursues after the fleeing @c$N@R!@n", TRUE, ch, 0, vict,
        TO_NOTVICT);

    if (char_follower_count(ch)) {
      char_followers_iterate(ch, [&](struct char_data *fol) {
        if ((char_room_vnum_get(fol) == inroom) &&
            (GET_POS(fol) >= POS_STANDING) &&
            (!char_condition_has(ch, "zanzoken") ||
             (char_condition_has(ch, "group") && char_condition_has(fol, "group")))) {
          act("You follow $N.", TRUE, fol, 0, ch, TO_CHAR);
          act("$n follows after $N.", TRUE, fol, 0, ch, TO_NOTVICT);
          act("$n follows after you.", TRUE, fol, 0, ch, TO_VICT);
          char_from_room(fol);
          char_to_room(fol, char_room_get(ch));
        }
        return true;
      });
    }
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_PURSUIT);
    return (TRUE);
  } else {
    send_to_char(ch, "@RYou fail to pursue after them!@n\r\n");
    if (FIGHTING(ch)) {
      stop_fighting(ch);
    }
    if (FIGHTING(vict)) {
      stop_fighting(vict);
    }
    return (FALSE);
  }
}

char *sense_location(struct char_data *ch) {

  char *message;
  CREATE(message, char, MAX_INPUT_LENGTH);
  int roomnum = char_room_vnum_get(ch), num = 0;
  auto zone = char_zone_get(ch);
  num = zone->id;

  switch (num) {
  case 2:
    sprintf(message, "East of Nexus City");
    break;
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
    if (roomnum < 795)
      sprintf(message, "Nexus City");
    else
      sprintf(message, "South Ocean");
    break;
  case 8:
  case 9:
  case 10:
  case 11:
    if (roomnum < 1133)
      sprintf(message, "South Ocean");
    else if (roomnum < 1179)
      sprintf(message, "Nexus Field");
    else
      sprintf(message, "Cherry Blossom Mountain");
    break;
  case 12:
  case 13:
    if (roomnum < 1287)
      sprintf(message, "Cherry Blossom Mountain");
    else
      sprintf(message, "Sandy Desert");
    break;
  case 14:
    if (roomnum < 1428)
      sprintf(message, "Sandy Desert");
    else if (roomnum < 1484)
      sprintf(message, "Northern Plains");
    else if (roomnum < 1496)
      sprintf(message, "Korin's Tower");
    else
      sprintf(message, "Kami's Lookout");
    break;
  case 15:
    if (roomnum < 1577)
      sprintf(message, "Kami's Lookout");
    else if (roomnum < 1580)
      sprintf(message, "Northern Plains");
    else if (roomnum < 1589)
      sprintf(message, "Kami's Lookout");
    else
      sprintf(message, "Shadow Forest");
    break;
  case 16:
    sprintf(message, "Shadow Forest");
    break;
  case 17:
  case 18:
    if (roomnum < 1715)
      sprintf(message, "Decrepit Area");
    else
      sprintf(message, "Inside Cherry Blossom Mountain");
    break;
  case 19:
    sprintf(message, "West City");
    break;
  case 20:
    if (roomnum < 2012)
      sprintf(message, "West City");
    else if (roomnum > 2070)
      sprintf(message, "West City");
    else
      sprintf(message, "Silver Mine");
    break;
  case 21:
    if (roomnum < 2141)
      sprintf(message, "West City");
    else
      sprintf(message, "Hercule Beach");
    break;
  case 22:
    sprintf(message, "Vegetos City");
    break;
  case 23:
  case 24:
    if (roomnum < 2334)
      sprintf(message, "Vegetos City");
    else if (roomnum > 2462)
      sprintf(message, "Vegetos City");
    else
      sprintf(message, "Vegetos Palace");
    break;
  case 25:
  case 26:
    if (roomnum < 2616)
      sprintf(message, "Blood Dunes");
    else
      sprintf(message, "Ancestral Mountains");
    break;
  case 27:
    if (roomnum < 2709)
      sprintf(message, "Ancestral Mountains");
    else if (roomnum < 2736)
      sprintf(message, "Destopa Swamp");
    else
      sprintf(message, "Swamp Base");
    break;
  case 28:
    sprintf(message, "Pride Forest");
    break;
  case 29:
  case 30:
  case 31:
    sprintf(message, "Pride Tower");
    break;
  case 32:
    sprintf(message, "Ruby Cave");
    break;
  case 34:
    sprintf(message, "Utatlan City");
    break;
  case 35:
    sprintf(message, "Zenith Jungle");
    break;
  case 40:
  case 41:
  case 42:
    sprintf(message, "Ice Crown City");
    break;
  case 43:
    if (roomnum < 4351)
      sprintf(message, "Ice Highway");
    else
      sprintf(message, "Topica Snowfield");
    break;
  case 44:
  case 45:
    sprintf(message, "Glug's Volcano");
    break;
  case 46:
  case 47:
    sprintf(message, "Platonic Sea");
    break;
  case 48:
    sprintf(message, "Slave City");
    break;
  case 49:
    if (roomnum < 4915)
      sprintf(message, "Descent Down Icecrown");
    else if (roomnum != 4915 && roomnum < 4994)
      sprintf(message, "Topica Snowfield");
    else
      sprintf(message, "Ice Highway");
    break;
  case 50:
    sprintf(message, "Mirror Shard Maze");
    break;
  case 51:
    if (roomnum < 5150)
      sprintf(message, "Acturian Woods");
    else if (roomnum < 5165)
      sprintf(message, "Desolate Demesne");
    else
      sprintf(message, "Chateau Ishran");
    break;
  case 52:
    sprintf(message, "Wyrm Spine Mountain");
    break;
  case 53:
  case 54:
    sprintf(message, "Aromina Hunting Preserve");
    break;
  case 55:
    sprintf(message, "Cloud Ruler Temple");
    break;
  case 56:
    sprintf(message, "Koltoan Mine");
    break;
  case 78:
    sprintf(message, "Orium Cave");
    break;
  case 79:
    sprintf(message, "Crystalline Forest");
    break;
  case 80:
  case 81:
  case 82:
    sprintf(message, "Tiranoc City");
    break;
  case 83:
    sprintf(message, "Great Oroist Temple");
    break;
  case 84:
    if (roomnum < 8447)
      sprintf(message, "Elsthuan Forest");
    else
      sprintf(message, "Mazori Farm");
    break;
  case 85:
    sprintf(message, "Dres");
    break;
  case 86:
    sprintf(message, "Colvian Farm");
    break;
  case 87:
    sprintf(message, "Saint Alucia");
    break;
  case 88:
    if (roomnum < 8847)
      sprintf(message, "Meridius Memorial");
    else
      sprintf(message, "Battlefields");
    break;
  case 89:
    if (roomnum < 8954)
      sprintf(message, "Desert of Illusion");
    else
      sprintf(message, "Plains of Confusion");
    break;
  case 90:
    sprintf(message, "Shadowlas Temple");
    break;
  case 92:
    sprintf(message, "Turlon Fair");
    break;
  case 97:
    sprintf(message, "Wetlands");
    break;
  case 98:
    if (roomnum < 9855)
      sprintf(message, "Wetlands");
    else if (roomnum < 9866)
      sprintf(message, "Kerberos");
    else
      sprintf(message, "Shaeras Mansion");
    break;
  case 99:
    if (roomnum < 9907)
      sprintf(message, "Slavinos Ravine");
    else if (roomnum < 9960)
      sprintf(message, "Kerberos");
    else
      sprintf(message, "Furian Citadel");
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
    sprintf(message, "Namekian Wilderness");
    break;
  case 116:
    if (roomnum < 11672)
      sprintf(message, "Senzu Village");
    else if (roomnum > 11672 && roomnum < 11698)
      sprintf(message, "Senzu Village");
    else
      sprintf(message, "Guru's House");
    break;
  case 117:
  case 118:
  case 119:
    sprintf(message, "Crystalline Cave");
    break;
  case 120:
    sprintf(message, "Haven City");
    break;
  case 121:
    if (roomnum < 12103)
      sprintf(message, "Haven City");
    else
      sprintf(message, "Serenity Lake");
    break;
  case 122:
    sprintf(message, "Serenity Lake");
    break;
  case 123:
    sprintf(message, "Kaiju Forest");
    break;
  case 124:
    if (roomnum < 12480)
      sprintf(message, "Ortusian Temple");
    else
      sprintf(message, "Silent Glade");
    break;
  case 125:
    sprintf(message, "Near Serenity Lake");
    break;
  case 130:
  case 131:
    if (roomnum < 13153)
      sprintf(message, "Satan City");
    else if (roomnum == 13153)
      sprintf(message, "West City");
    else if (roomnum == 13154)
      sprintf(message, "Nexus City");
    else
      sprintf(message, "South Ocean");
    break;
  case 132:
    if (roomnum < 13232)
      sprintf(message, "Frieza's Ship");
    else
      sprintf(message, "Namekian Wilderness");
    break;
  case 133:
    sprintf(message, "Elder Village");
    break;
  case 134:
    sprintf(message, "Satan City");
    break;
  case 140:
    sprintf(message, "Yardra City");
    break;
  case 141:
    sprintf(message, "Jade Forest");
    break;
  case 142:
    sprintf(message, "Jade Cliff");
    break;
  case 143:
    sprintf(message, "Mount Valaria");
    break;
  case 149:
  case 150:
    sprintf(message, "Aquis City");
    break;
  case 151:
  case 152:
  case 153:
    sprintf(message, "Kanassan Ocean");
    break;
  case 154:
    sprintf(message, "Kakureta Village");
    break;
  case 155:
    sprintf(message, "Captured Aether City");
    break;
  case 156:
    sprintf(message, "Yunkai Pirate Base");
    break;
  case 160:
  case 161:
    sprintf(message, "Janacre");
    break;
  case 165:
    sprintf(message, "Arlian Wasteland");
    break;
  case 166:
    sprintf(message, "Arlian Mine");
    break;
  case 167:
    sprintf(message, "Kilnak Caverns");
    break;
  case 168:
    sprintf(message, "Kemabra Wastes");
    break;
  case 169:
    sprintf(message, "Dark of Arlia");
    break;
  case 174:
    sprintf(message, "Fistarl Volcano");
    break;
  case 175:
  case 176:
    sprintf(message, "Cerria Colony");
    break;
  case 182:
    sprintf(message, "Below Tiranoc");
    break;
  case 196:
    sprintf(message, "Ancient Castle");
    break;
  default:
    sprintf(message, "Unknown.");
    break;
  }

  return (message);
}

void reveal_hiding(struct char_data *ch, int type) {
  if (IS_NPC(ch) || !AFF_FLAGGED(ch, AFF_HIDE))
    return;

  int rand1 = rand_number(-5, 5), rand2 = rand_number(-5, 5), bonus = 0;

  if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
    bonus = 10;
  }

  if (type == 0) { /* Automatic reveal. */
    act("@MYou feel as though what you just did may have revealed your hiding "
        "spot...@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@M$n moves a little and you notice them!@n", TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  } else if (type == 1) { /* Their skill at hiding failed, reveal */
    if (GET_SKILL(ch, SKILL_HIDE) + bonus < axion_dice(0)) {
      act("@MYou feel as though what you just did may have revealed your "
          "hiding spot...@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@M$n moves a little and you notice them!@n", TRUE, ch, 0, 0,
          TO_ROOM);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    }
  } else if (type == 2) { /* They were spotted */
    struct descriptor_data *d;
    struct char_data *tch = NULL;
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING)
        continue;
      tch = d->character;

      if (tch == ch)
        continue;

      if (char_room_get(tch) != char_room_get(ch))
        continue;

      if (GET_SKILL(tch, SKILL_SPOT) + rand1 >=
          GET_SKILL(ch, SKILL_HIDE) + rand2) {
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
        act("@M$N seems to notice you!@n", TRUE, ch, 0, tch, TO_CHAR);
        act("@MYou notice $n's movements reveal $s hiding spot!@n", TRUE, ch, 0,
            tch, TO_VICT);
        act("@MYou notice $N look keenly somewhere nearby. At that spot you "
            "see $n hiding!@n",
            TRUE, ch, 0, tch, TO_NOTVICT);
        return;
      }
    }
  } else if (type == 3) { /* They were heard, reveal with different messages. */
    struct descriptor_data *d;
    struct char_data *tch = NULL;

    act("@MThe scouter makes some beeping sounds as you tinker with its "
        "buttons.@n",
        TRUE, ch, 0, 0, TO_CHAR);
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING)
        continue;
      tch = d->character;

      if (tch == ch)
        continue;

      if (char_room_get(tch) != char_room_get(ch))
        continue;

      if (GET_SKILL(tch, SKILL_LISTEN) > axion_dice(0)) {
        switch (type) {
        case 3:
          act("@MYou notice some beeping sounds that sound really close by.@n",
              TRUE, ch, 0, tch, TO_VICT);
          break;
        default:
          act("@MYou notice some sounds coming from this room but can't seem "
              "to locate the source...@n",
              TRUE, ch, 0, tch, TO_VICT);
          break;
        }
      }
    }
  } else if (type == 4) { /* You were found from search! */
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  }
}

int block_calc(struct char_data *ch) {
  auto *blocker = BLOCKED(ch);
  if (!blocker)
    return 1;

  auto escape_acts = [&](bool attempted) {
    if (attempted) {
      act("$n proves $s great skill and escapes from $N's attempted block!",
          TRUE, ch, 0, blocker, TO_NOTVICT);
      act("$n proves $s great skill and escapes from your attempted block!",
          TRUE, ch, 0, blocker, TO_VICT);
      act("Using your great skill you manage to escape from $N's attempted block!",
          TRUE, ch, 0, blocker, TO_CHAR);
    } else {
      act("$n proves $s great skill and escapes from $N!", TRUE, ch, 0, blocker, TO_NOTVICT);
      act("$n proves $s great skill and escapes from you!", TRUE, ch, 0, blocker, TO_VICT);
      act("Using your great skill you manage to escape from $N!", TRUE, ch, 0, blocker, TO_CHAR);
    }
    char_blocked_by_set(ch, NULL);
    char_blocking_set(blocker, NULL);
  };

  if (GET_SPEEDI(ch) < GET_SPEEDI(blocker) && GET_POS(blocker) > POS_SITTING) {
    if (!AFF_FLAGGED(blocker, AFF_BLIND) && !PLR_FLAGGED(blocker, PLR_EYEC)) {
      int minimum = GET_CHA(blocker) + rand_number(5, 20);
      if (minimum > 100) minimum = 100;
      int ea = GET_SKILL(ch, SKILL_ESCAPE_ARTIST);
      if (!ea || ea < rand_number(minimum, 120)) {
        act("$n tries to leave, but can't outrun $N!", TRUE, ch, 0, blocker, TO_NOTVICT);
        act("$n tries to leave, but can't outrun you!", TRUE, ch, 0, blocker, TO_VICT);
        act("You try to leave, but can't outrun $N!", TRUE, ch, 0, blocker, TO_CHAR);
        if (char_condition_has(ch, "flying") && !char_condition_has(blocker, "flying")) {
          int alt = GET_ALT(ch);
          if (alt == 1 || alt == 2) {
            send_to_char(blocker, alt == 1 ? "You're now floating in the air.\r\n"
                                           : "You're now floating high in the sky.\r\n");
            char_condition_add(blocker, "flying", "skill", "fly");
            char_condition_number_set(blocker, "flying", "altitude", alt);
          }
        }
        return 0;
      }
    }
    escape_acts(true);
  } else if (GET_POS(blocker) <= POS_SITTING) {
    escape_acts(false);
  } else {
    escape_acts(true);
  }

  return 1;
}

int64_t molt_threshold(struct char_data *ch) {

  int64_t threshold = 0, max = 2000000000;

  max *= 250;

  if (!IS_ARLIAN(ch))
    return (0);
  else if (GET_MOLT_LEVEL(ch) < 100) {
    threshold =
        (((GET_MOLT_LEVEL(ch) + 1) * (getBasePL(ch) * 0.02)) * GET_CON(ch)) / 4;
    threshold = threshold * 0.25;
  } else if (GET_MOLT_LEVEL(ch) < 200) {
    threshold =
        (((GET_MOLT_LEVEL(ch) + 1) * (getBasePL(ch) * 0.02)) * GET_CON(ch)) / 2;
    threshold = threshold * 0.20;
  } else if (GET_MOLT_LEVEL(ch) < 400) {
    threshold =
        (((GET_MOLT_LEVEL(ch) + 1) * (getBasePL(ch) * 0.02)) * GET_CON(ch));
    threshold = threshold * 0.17;
  } else if (GET_MOLT_LEVEL(ch) < 800) {
    threshold =
        (((GET_MOLT_LEVEL(ch) + 1) * (getBasePL(ch) * 0.02)) * GET_CON(ch)) * 2;
    threshold = threshold * 0.15;
  } else {
    threshold =
        (((GET_MOLT_LEVEL(ch) + 1) * (getBasePL(ch) * 0.02)) * GET_CON(ch)) * 4;
    threshold = threshold * 0.12;
  }

  return MIN(threshold, max);
}

int armor_evolve(struct char_data *ch) {
  int value = 0;

  if (GET_MOLT_LEVEL(ch) <= 5) {
    value = 8;
  } else if (GET_MOLT_LEVEL(ch) <= 10) {
    value = 12;
  } else if (GET_MOLT_LEVEL(ch) <= 20) {
    value = 15;
  } else if (GET_MOLT_LEVEL(ch) <= 30) {
    value = 20;
  } else if (GET_MOLT_LEVEL(ch) <= 40) {
    value = 30;
  } else if (GET_MOLT_LEVEL(ch) <= 50) {
    value = 50;
  } else if (GET_MOLT_LEVEL(ch) <= 75) {
    value = 100;
  } else if (GET_MOLT_LEVEL(ch) <= 100) {
    value = 150;
  } else if (GET_MOLT_LEVEL(ch) <= 500) {
    value = 200;
  } else {
    value = 220;
  }

  return (value);
}

/* This handles arlian exoskeleton molting */
void handle_evolution(struct char_data *ch, int64_t dmg) {

  /* Reject NPCs and non-arlians */
  if (IS_NPC(ch) || !IS_ARLIAN(ch)) {
    return;
  }

  int64_t moltgain = 0;

  moltgain = dmg * 0.5;
  if (GET_LEVEL(ch) == 100)
    moltgain += 100000;
  else if (GET_LEVEL(ch) >= 90)
    moltgain += GET_LEVEL(ch) * 1000;
  else if (GET_LEVEL(ch) >= 75)
    moltgain += GET_LEVEL(ch) * 500;
  else if (GET_LEVEL(ch) >= 50)
    moltgain += GET_LEVEL(ch) * 250;
  else if (GET_LEVEL(ch) >= 10)
    moltgain += GET_LEVEL(ch) * 50;

  char_stat_mod(ch, "molt_experience", moltgain);

  if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
    send_to_char(ch,
                 "You are dead and all evolution experience is reduced. Gains "
                 "are divided by 100 or reduced to a minimum of 1.\r\n");
    moltgain /= 100;
  }

  if (GET_MOLT_EXP(ch) > molt_threshold(ch)) {
    if (GET_MOLT_LEVEL(ch) <= GET_LEVEL(ch) * 2 || GET_LEVEL(ch) >= 100) {
      char_stat_set(ch, "molt_experience", 0);
      char_stat_mod(ch, "molt_level", 1);
      double rand1 = 0.02;
      double rand2 = 0.03;
      if (rand_number(1, 4) == 3) {
        rand1 += 0.02;
        rand2 += 0.02;
      } else if (rand_number(1, 4) >= 3) {
        rand1 += 0.01;
        rand2 += 0.01;
      }
      int armorgain = 0;
      int64_t plgain = getBasePL(ch) * rand1, stamgain = getBaseST(ch) * rand2;
      armorgain = armor_evolve(ch);
      gainBasePL(ch, plgain);
      gainBaseST(ch, stamgain);
      char_stat_mod(ch, "armor", armorgain);
      if (GET_ARMOR(ch) > 50000) {
        char_stat_set(ch, "armor", 50000);
      }
      act("@gYour @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. You "
          "quickly shed it and reveal a stronger version that was growing "
          "beneath it! At the same time you feel your adrenal sacs to be more "
          "efficient@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@G$n's@g @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. "
          "Suddenly $e sheds the damaged @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn and "
          "reveals a stronger version that had been growing underneath!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch,
                   "@D[@RPL@W: @G+%s@D] [@gStamina@W: @G+%s@D] [@wArmor "
                   "Index@W: @G+%s@D]@n\r\n",
                   add_commas(plgain), add_commas(stamgain),
                   GET_ARMOR(ch) >= 50000 ? "50k CAP" : add_commas(armorgain));
    } else {
      send_to_char(ch, "@gYou are unable to evolve while your evolution level "
                       "is higher than twice your character level.@n\r\n");
    }
  }
}

void demon_refill_lf(struct char_data *ch, int64_t num) {
  struct char_data *tch = NULL;

  room_people_iterate(char_room_get(ch), [&](auto tch) {
    if (!IS_DEMON(tch))
      return true;
    if ((getCurLF(tch)) >= (getMaxLF(tch)))
      return true;
    else {
      incCurLF(ch, num);
      act("@CYou feel the life energy from @c$N@C's cursed body flow out and "
          "you draw it into yourself!@n",
          TRUE, tch, 0, ch, TO_CHAR);
    }
    return true;
  });
}

void mob_talk(struct char_data *ch, const char *speech) {

  struct char_data *tch = NULL, *vict = NULL;
  int stop = 1;

  if (IS_NPC(ch)) {
    return;
  }

  room_people_iterate(char_room_get(ch), [&](auto tch) {
    if (!IS_NPC(tch))
      return true;
    if (!IS_HUMANOID(tch))
      return true;
    if (stop == 0)
      return true;
    else {
      vict = tch;
      stop = mob_respond(ch, vict, speech);
      if (rand_number(1, 2) == 2) {
        stop = 0;
      }
    }
    return true;
  });
} /* End Mob Talk */

int mob_respond(struct char_data *ch, struct char_data *vict,
                const char *speech) {
  if (!ch || !vict)
    return 1;
  if (IS_NPC(ch) || !IS_NPC(vict))
    return 1;

  auto hears = [speech](const char *word) { return strstr(speech, word) != nullptr; };
  auto say   = [&](const char *line) { act(line, TRUE, vict, 0, ch, TO_ROOM); };
  auto say_one_of = [&](std::initializer_list<const char *> lines) {
    say(*(lines.begin() + rand_number(1, (int)lines.size()) - 1));
  };

  if ((hears("hello") || hears("Hello") || hears("greet") || hears("Greet")) && !FIGHTING(vict)) {
    send_to_room(char_room_get(vict), "\r\n");
    if (IS_HUMAN(vict) || IS_HALFBREED(vict)) {
      say_one_of({
        "@w$n@W says, '@CYes, hello to you as well $N.@W'@n",
        "@w$n@W says, '@CHello!@W'@n",
        "@w$n@W says, '@CHi, $N, how are you doing?@W'@n",
        "@w$n@W says, '@CGreetings, $N. What are you up to?@W'@n",
      });
    } else if (IS_SAIYAN(vict)) {
      say_one_of({
        "@w$n@W says, '@CHmph, hi.@W'@n",
        "@w$n@W says, '@CHello weakling.@W'@n",
        "@w$n@W says, '@C$N do all weaklings like you waste time in idle talk?@W'@n",
        "@w$n@W says, '@C$N, you are not welcome around me.@W'@n",
      });
    } else if (IS_ICER(vict)) {
      say_one_of({
        "@w$n@W says, '@CHa ha... Yes, hello.@W'@n",
        "@w$n@W says, '@CAh a polite greeting. It's good to know your kind isn't totally worthless.@W'@n",
        "@w$n@W says, '@C$N, hello. Now leave me be.@W'@n",
        "@w$n@W says, '@C$N, you are below me. Now begone.@W'@n",
      });
    } else if (IS_KONATSU(vict)) {
      say_one_of({
        "@w$n@W says, '@CGreetings, $N, may your travels be well.@W'@n",
        "@w$n@W says, '@CHello.@W'@n",
        "@w$n@W says, '@C$N, hello.@W'@n",
        "@w$n@W says, '@C$N, it is nice to meet you.@W'@n",
      });
    } else if (IS_NAMEK(vict)) {
      say_one_of({
        "@w$n@W says, '@CHello.@W'@n",
        "@w$n@W says, '@CA peaceful greeting to you, $N.@W'@n",
        "@w$n@W says, '@C$N, hello. What is your business here?@W'@n",
        "@w$n@W says, '@C$N, greetings.@W'@n",
      });
    } else if (IS_ARLIAN(vict)) {
      say_one_of({
        "@w$n@W says, '@CPeace, stranger.@W'@n",
        "@w$n@W says, '@CStay out of my way.@W'@n",
        "@w$n@W says, '@C$N, what is your business here?@W'@n",
        "@w$n@W says, '@C...Hello.@W'@n",
      });
    } else if (IS_ANDROID(vict)) {
      say("@w$n@W says, '@C...@W'@n");
    } else if (IS_MAJIN(vict)) {
      say_one_of({
        "@w$n@W says, '@CHa ha...@W'@n",
        "@w$n@W says, '@CHello. What candy you want to be?@W'@n",
      });
    } else if (IS_TRUFFLE(vict)) {
      if (IS_SAIYAN(ch))
        say("@w$n@W says, '@CEwww, dirty monkey...@W'@n");
      else
        say_one_of({
          "@w$n@W says, '@CHello.@W'@n",
          "@w$n@W says, '@C$N, hello. You are a curious individual.@W'@n",
          "@w$n@W says, '@C$N, hello. What's your IQ?@W'@n",
        });
    } else {
      say("Hmph, yeah hi.");
    }
  }

  if ((hears("spar") || hears("Spar")) && !FIGHTING(vict)) {
    send_to_room(char_room_get(vict), "\r\n");
    if (GET_ORIGINAL(vict) == ch) {
      say("@w$n@W says, '@C$N, sure. I'll spar with you.@W'@n");
      SET_BIT_AR(MOB_FLAGS(vict), MOB_SPAR);
      return 0;
    }
    if (GET_LEVEL(vict) <= 4) {
      say("@w$n@W says, '@CSpar? I prefer not to thank you...@W'@n");
      return 1;
    }
    if (GET_ALIGNMENT(vict) < 0) {
      say("@w$n@W says, '@CSpar? I don't play games, I play for blood...@W'@n");
      return 1;
    }
    for (memory_rec *names = MEMORY(vict); names; names = names->next) {
      if (names->id == GET_IDNUM(ch)) {
        say("@w$n@W says, '@C$N you will die by my hand!@W'@n");
        return 1;
      }
    }
    if (MOB_FLAGGED(vict, MOB_NOKILL)) {
      say("@w$n@W says, '@C$N, I have no need to spar with you.@W'@n");
      return 1;
    }
    if (MOB_FLAGGED(vict, MOB_AGGRESSIVE)) {
      say("@w$n@W says, '@C$N, I will kill you instead.@W'@n");
      return 1;
    }
    if (MOB_FLAGGED(vict, MOB_DUMMY)) {
      say("@w$n@W says, '@C...@W'@n");
      return 1;
    }
    if (GET_MAX_HIT(ch) > GET_MAX_HIT(vict) * 2) {
      say("@w$n@W says, '@C$N, no way will I spar. I already know I would lose badly.@W'@n");
      return 1;
    }
    if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 2) {
      say("@w$n@W says, '@C$N, you wouldn't last very long.@W'@n");
      return 1;
    }
    if (GET_HIT(vict) < GET_MAX_HIT(vict) * .8) {
      say("@w$n@W says, '@C$N, I need to recover first.@W'@n");
      return 1;
    }
    if (rand_number(1, 50) >= 40 && !MOB_FLAGGED(vict, MOB_SPAR)) {
      say("@w$n@W says, '@C$N, maybe in a bit.@W'@n");
      return 1;
    }
    if (MOB_FLAGGED(vict, MOB_SPAR)) {
      say("@w$n@W says, '@C$N, fine our match will wait till later then.@W'@n");
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_SPAR);
    } else {
      say("@w$n@W says, '@C$N, sure. I'll spar with you.@W'@n");
      SET_BIT_AR(MOB_FLAGS(vict), MOB_SPAR);
    }
    return 0;
  }

  if (hears("goodbye") || hears("Goodbye") || hears("bye") || hears("Bye")) {
    send_to_room(char_room_get(vict), "\r\n");
    int vs = GET_SEX(vict), cs = GET_SEX(ch);
    if (GET_ALIGNMENT(vict) >= 0) {
      if      (vs == SEX_MALE   && cs == SEX_FEMALE) say("@w$n@W says, '@C$N, goodbye babe.@W'@n");
      else if (vs == SEX_MALE)                        say("@w$n@W says, '@C$N, goodbye.@W'@n");
      else if (vs == SEX_FEMALE && cs == SEX_MALE)    say("@w$n@W says, '@C$N, goodbye...@W'@n");
      else if (vs == SEX_FEMALE)                      say("@w$n@W says, '@C$N, bye.@W'@n");
      else                                            say("@w$n@W says, '@C$N, goodbye.@W'@n");
    } else {
      if      (vs == SEX_MALE   && cs == SEX_FEMALE) say("@w$n@W says, '@CGoodbye. Eh heh heh.@W'@n");
      else if (vs == SEX_MALE)                        say("@w$n@W says, '@CSo long and good ridance.@W'@n");
      else if (vs == SEX_FEMALE && cs == SEX_MALE)    say("@w$n@W says, '@CGoodbye then...@W'@n");
      else if (vs == SEX_FEMALE)                      say("@w$n@W says, '@C$N, no one wanted you around anyway.@W'@n");
      else                                            say("@w$n@W says, '@CFine get lost.@W'@n");
    }
  }

  if ((hears("train") || hears("Train") || hears("exercise") || hears("Exercise")) &&
      !MOB_FLAGGED(vict, MOB_NOKILL)) {
    static const char *train_lines[2][5] = {
      { /* good alignment, tiers: <5, 5-9, 10-29, 30-59, 60+ */
        "@w$n@W says, '@CI really need to bust ass and train.@W'@n",
        "@w$n@W says, '@CTraining is good for the body. I think I may need to go workout myself.@W'@n",
        "@w$n@W says, '@CI think I might need a little more training...@W'@n",
        "@w$n@W says, '@CI'm pretty tough already. Though I should probably work on my skills.@W'@n",
        "@w$n@W says, '@CI'm on top of my game.@W'@n",
      },
      { /* evil alignment */
        "@w$n@W says, '@CYes. I need to train so I can reach the top. Then everyone will have to listen to me!@W'@n",
        "@w$n@W says, '@CWell maybe I could use some more training.@W'@n",
        "@w$n@W says, '@CTrain? Yeah it has become harder to take what I want....@W'@n",
        "@w$n@W says, '@CTrain? I don't need to train to take you!@W'@n",
        "@w$n@W says, '@CTraining won't save you when I tire of your continued life.@W'@n",
      },
    };
    send_to_room(char_room_get(vict), "\r\n");
    int lv   = GET_LEVEL(vict);
    int tier = lv < 5 ? 0 : lv < 10 ? 1 : lv < 30 ? 2 : lv < 60 ? 3 : 4;
    say(train_lines[GET_ALIGNMENT(vict) < 0 ? 1 : 0][tier]);
  }

  return 1;
}

bool is_sparring(struct char_data *ch) {

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SPAR)) {
    return TRUE;
  }
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPAR)) {
    return TRUE;
  }
  return FALSE;
}

char *introd_calc(struct char_data *ch) {
  char *sex, *race;
  static char intro[100];

  *intro = '\0';

  if (IS_NPC(ch)) { /* How the hell... */
    return ("IAMERROR");
  }

  if (IS_HALFBREED(ch)) {
    if (RACIAL_PREF(ch) == 1) {
      race = strdup("human");
    } else if (RACIAL_PREF(ch) == 2) {
      race = strdup("saiyan");
    } else {
      race = strdup(RACE(ch));
    }
    sex = strdup(MAFE(ch));
  } else if (IS_ANDROID(ch)) {
    if (RACIAL_PREF(ch) == 1) {
      race = strdup("android");
    } else if (RACIAL_PREF(ch) == 2) {
      race = strdup("human");
    } else if (RACIAL_PREF(ch) == 3) {
      race = strdup("robotic-humanoid");
    } else {
      race = strdup(RACE(ch));
    }
    sex = strdup(MAFE(ch));
  } else {
    sex = strdup(MAFE(ch));
    race = strdup(RACE(ch));
  }
  sprintf(intro, "%s %s %s", AN(sex), sex, race);
  if (sex) {
    free(sex);
  }
  if (race) {
    free(race);
  }

  return (intro);
}

double speednar(struct char_data *ch) {

  double result = 0;
  int64_t carried = getCurCarriedWeight(ch);
  int64_t can_carry = CAN_CARRY_W(ch);

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

int64_t gear_exp(struct char_data *ch, int64_t exp) {

  if (IS_NPC(ch)) {
    return 0;
  }

  double ratio = 2 * speednar(ch);
  ratio += 1.0;
  return exp * ratio;
}

int planet_check(struct char_data *ch, struct char_data *vict) {

  if (ch == NULL) {
    mud_log("ERROR: planet_check called without ch!");
    return 0;
  }
  if (vict == NULL) {
    mud_log("ERROR: planet_check called without vict!");
    return 0;
  }
  int success = 0;
  struct room_data *room = char_room_get(ch);
  struct room_data *vict_room = char_room_get(vict);
  if (GET_ADMLEVEL(vict) <= 0) {
    for (auto flag : {ROOM_FRIGID, ROOM_EARTH, ROOM_NAMEK, ROOM_VEGETA,
                      ROOM_AETHER, ROOM_KONACK, ROOM_KANASSA, ROOM_YARDRAT,
                      ROOM_AL, ROOM_HELL, ROOM_ARLIA, ROOM_NEO, ROOM_CERRIA}) {
      if (room_flagged(room, flag) && room_flagged(vict_room, flag)) {
        success = 1;
        break;
      }
    }
    if (!success) {
      success = char_planet_zenith(ch) && char_planet_zenith(vict);
    }
  }
  return (success);
}

void purge_homing(struct char_data *ch) {

  struct obj_data *obj = NULL, *next_obj = NULL;
  room_contents_iterate(char_room_get(ch), [&](auto obj) {
    if (GET_OBJ_VNUM(obj) == 80 || GET_OBJ_VNUM(obj) == 81) {
      if (TARGET(obj) == ch || USER(obj) == ch) {
        act("$p @wloses its target and flies off into the distance.@n", TRUE, 0,
            obj, 0, TO_ROOM);
        extract_obj(obj);
      }
    }
    return true;
  });
}

void improve_skill(struct char_data *ch, int skill, int num) {
  int percent = GET_SKILL(ch, skill);
  int newpercent, roll = 1200;
  char skillbuf[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;

  if (!num) {
    num = 2;
  }

  if (AFF_FLAGGED(ch, AFF_SHOCKED))
    return;

  if (char_condition_has(ch, "forget_skill") &&
      strcmp(char_condition_string_get(ch, "forget_skill", "skill_name"), spell_info[skill].name) == 0)
    return;

  if (GET_SKILL_BASE(ch, skill) >= 90) {
    roll += 800;
  } else if (GET_SKILL_BASE(ch, skill) >= 75) {
    roll += 600;
  } else if (GET_SKILL_BASE(ch, skill) >= 50) {
    roll += 400;
  }

  if (skill == SKILL_PARRY || skill == SKILL_DODGE || skill == SKILL_BARRIER ||
      skill == SKILL_BLOCK || skill == SKILL_ZANZOKEN || skill == SKILL_TSKIN) {
    if (GET_BONUS(ch, BONUS_MASOCHISTIC) > 0) {
      return;
    }
  }

  if (!SPAR_TRAIN(ch)) {
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

  if (FIGHTING(ch) != NULL && IS_NPC(FIGHTING(ch)) &&
      MOB_FLAGGED(FIGHTING(ch), MOB_DUMMY)) {
    roll -= 100;
  }

  if (IS_TRUFFLE(ch) ||
      (IS_BIO(ch) && (GET_GENOME(ch, 0) == 6 || GET_GENOME(ch, 1) == 6))) {
    roll *= 0.5;
  } else if (IS_MAJIN(ch)) {
    roll += roll * .3;
  } else if (IS_BIO(ch) && skill == SKILL_ABSORB) {
    roll -= roll * .15;
  } else if (IS_HOSHIJIN(ch) &&
             (skill == SKILL_PUNCH || skill == SKILL_KICK ||
              skill == SKILL_KNEE || skill == SKILL_ELBOW ||
              skill == SKILL_UPPERCUT || skill == SKILL_ROUNDHOUSE ||
              skill == SKILL_SLAM || skill == SKILL_HEELDROP ||
              skill == SKILL_DAGGER || skill == SKILL_SWORD ||
              skill == SKILL_CLUB || skill == SKILL_GUN ||
              skill == SKILL_SPEAR || skill == SKILL_BRAWL)) {
    roll = roll * 0.30;
  }

  if (FIGHTING(ch) != NULL && IS_NPC(FIGHTING(ch)) &&
      MOB_FLAGGED(FIGHTING(ch), MOB_DUMMY)) {
    roll -= 100;
  }

  if (GET_BONUS(ch, BONUS_QUICK_STUDY) > 0) {
    roll -= roll * .25;
  } else if (GET_BONUS(ch, BONUS_SLOW_LEARNER) > 0) {
    roll += roll * .25;
  }

  if (GET_ASB(ch) > 0) {
    roll -= (roll * 0.01) * GET_ASB(ch);
  }

  roll = MAX(roll, 300);

  if (rand_number(1, roll) > ((GET_INT(ch) * 2) + GET_WIS(ch))) {
    return;
  }
  if ((percent > 99 || percent <= 0)) {
    return;
  }
  if (IS_MAJIN(ch) && GET_SKILL(ch, skill) >= 75) {
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
  } else if (IS_MAJIN(ch) && skill == 425) {
    roll += 250;
  }

  /*
  if ((IS_JINTO(ch) || IS_TSUNA(ch) || IS_DABURA(ch) || IS_TAPION(ch) &&
  GET_SKILL(ch, SKILL_SENSE) >= 75) && skill == SKILL_SENSE) { return;
  }

  if ((IS_BARDOCK(ch) || IS_KURZAK(ch) || IS_FRIEZA(ch) || IS_GINYU(ch) ||
  IS_ANDSIX(ch) && GET_SKILL(ch, SKILL_SENSE) >= 50) && skill == SKILL_SENSE) {
    return;
  }
   */

  newpercent = 1;
  percent += newpercent;
  SET_SKILL(ch, skill, percent);
  if (newpercent >= 1) {
    sprintf(skillbuf,
            "@WYou feel you have learned something new about @G%s@W.@n\r\n",
            spell_info[skill].name);
    send_to_char(ch, "%s", skillbuf);
    if (GET_SKILL_BASE(ch, skill) >= 100) {
      send_to_char(ch, "You learned a lot by mastering that skill.\r\n");
      if (perf_skill(skill)) {
        send_to_char(ch, "You can now choose a perfection for this skill (help "
                         "perfection).\r\n");
      }
      if (IS_KONATSU(ch) && skill == SKILL_PARRY) {
        SET_SKILL(ch, skill, GET_SKILL_BASE(ch, skill) + 5);
      }
      if (GET_LEVEL(ch) < 100) {
        char_stat_mod(ch, "experience", level_exp(ch, GET_LEVEL(ch) + 1) / 20);
      } else {
        gain_exp(ch, 5000000);
      }
    }
  }
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim) {
  struct char_data *k;

  for (k = victim; k; k = MASTER(k)) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch) {
  if (MASTER(ch) == NULL) {
    core_dump();
    return;
  }

  act("You stop following $N.", FALSE, ch, 0, MASTER(ch), TO_CHAR);
  act("$n stops following $N.", TRUE, ch, 0, MASTER(ch), TO_NOTVICT);
  if (!(DEAD(MASTER(ch)) ||
        (MASTER(ch)->desc && STATE(MASTER(ch)->desc) == CON_MENU)))
    act("$n stops following you.", TRUE, ch, 0, MASTER(ch), TO_VICT);

  char_follower_remove(MASTER(ch), ch);
  char_following_set(ch, NULL);
}

int num_followers_charmed(struct char_data *ch) {
  int total = 0;

  /* Summoned creatures don't count against total */
  char_followers_iterate(ch, [&](struct char_data *lackey) {
    if (AFF_FLAGGED(lackey, AFF_CHARM) &&
        !AFF_FLAGGED(lackey, AFF_SUMMONED) &&
        MASTER(lackey) == ch)
      total++;
    return true;
  });

  return (total);
}

void switch_leader(struct char_data *old, struct char_data *new_leader) {
  char_followers_iterate(old, [&](struct char_data *fol) {
    if (fol == new_leader) {
      stop_follower(fol);
    } else {
      stop_follower(fol);
      add_follower(fol, new_leader);
    }
    return true;
  });
}
/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch) {
  if (MASTER(ch))
    stop_follower(ch);

  char_followers_iterate(ch, [&](struct char_data *fol) {
    stop_follower(fol);
    return true;
  });
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader) {
  if (MASTER(ch)) {
    core_dump();
    return;
  }

  char_following_set(ch, leader);
  char_follower_add(leader, ch);

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (char_room_get(ch) != NULL && char_room_get(leader) != NULL &&
      CAN_SEE(leader, ch)) {
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
    act("\r\n$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
  }
}

int count_metamagic_feats(struct char_data *ch) {
  int count = 0; /* Number of Metamagic Feats Known */

  return count;
}

int default_admin_flags_mortal[] = {-1};

int default_admin_flags_immortal[] = {
    ADM_SEEINV,       ADM_SEESECRET, ADM_FULLWHERE, ADM_NOPOISON,
    ADM_WALKANYWHERE, ADM_NODAMAGE,  ADM_NOSTEAL,   -1};

int default_admin_flags_builder[] = {-1};

int default_admin_flags_god[] = {
    ADM_ALLSHOPS, ADM_TELLALL, ADM_KNOWWEATHER, ADM_MONEY, ADM_EATANYTHING,
    ADM_NOKEYS,   -1};

int default_admin_flags_grgod[] = {ADM_TRANSALL, ADM_FORCEMASS, ADM_ALLHOUSES,
                                   -1};

int default_admin_flags_impl[] = {ADM_SWITCHMORTAL, ADM_INSTANTKILL, ADM_CEDIT,
                                  -1};

int *default_admin_flags[ADMLVL_IMPL + 1] = {
    default_admin_flags_mortal,  default_admin_flags_immortal,
    default_admin_flags_builder, default_admin_flags_god,
    default_admin_flags_grgod,   default_admin_flags_impl};

void admin_set(struct char_data *ch, int value) {
  int i;
  int orig = GET_ADMLEVEL(ch);

  if (GET_ADMLEVEL(ch) == value)
    return;
  if (GET_ADMLEVEL(ch) < value) { /* Promotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s promoted from %s to %s", GET_NAME(ch),
           admin_level_names[GET_ADMLEVEL(ch)], admin_level_names[value]);
    while (GET_ADMLEVEL(ch) < value) {
      GET_ADMLEVEL(ch)++;
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        SET_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
    }
    run_autowiz();
    if (orig < ADMLVL_IMMORT && value >= ADMLVL_IMMORT) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    }
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
      char_stat_set(ch, "drunk", -1);
      char_stat_set(ch, "hunger", -1);
      char_stat_set(ch, "thirst", -1);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }
    return;
  }
  if (GET_ADMLEVEL(ch) > value) { /* Demotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s demoted from %s to %s", GET_NAME(ch),
           admin_level_names[GET_ADMLEVEL(ch)], admin_level_names[value]);
    while (GET_ADMLEVEL(ch) > value) {
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        REMOVE_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
      GET_ADMLEVEL(ch)--;
    }
    run_autowiz();
    if (orig >= ADMLVL_IMMORT && value < ADMLVL_IMMORT) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
    }
    return;
  }
}

struct time_info_data *age(struct char_data *ch) {
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->time.birth);

  return (&player_age);
}

/* Used to roll starting PL/KI/ST in character creation */
int roll_stats(struct char_data *ch, int type, int bonus) {

  int pool = 0, base_num = bonus, max_num = bonus;
  int powerlevel = 0, ki = 1, stamina = 2;

  if (type == powerlevel) {
    base_num = char_stat_get(ch, "strength") * 3;
    max_num = char_stat_get(ch, "strength") * 5;
  } else if (type == ki) {
    base_num = char_stat_get(ch, "intelligence") * 3;
    max_num = char_stat_get(ch, "intelligence") * 5;
  } else if (type == stamina) {
    base_num = char_stat_get(ch, "constitution") * 3;
    max_num = char_stat_get(ch, "constitution") * 5;
  }

  pool = rand_number(base_num, max_num) + bonus;

  return (pool);
}

/* For Getting An Intro Name */
const char *get_i_name(struct char_data *ch, struct char_data *vict) {
  char fname[40], filler[50], scrap[100], line[256];
  static char name[50];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */
  if (vict == NULL) {
    return ("");
  }

  if (IS_NPC(ch) || IS_NPC(vict)) {
    return (RACE(vict));
  }

  if (vict == ch) {
    return ("");
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch))) {
    return (RACE(vict));
  } else if (!(fl = fopen(fname, "r"))) {
    return (RACE(vict));
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%s %s\n", filler, scrap);
    if (!strcasecmp(GET_NAME(vict), filler)) {
      sprintf(name, "%s", scrap);
      known = TRUE;
    }
  }
  fclose(fl);

  if (known == TRUE)
    return (name);
  else
    return (RACE(vict));
}

int can_grav(struct char_data *ch) {
  /* Gravity Related */
  int gravity = room_gravity_get(char_room_get(ch));
  auto pl = GET_MAX_HIT(ch);
  if (gravity == 10 && pl < 5000 && !IS_BARDOCK(ch) &&
      !IS_NPC(ch)) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 20 && pl < 20000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 30 && pl < 50000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 40 && pl < 100000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 50 && pl < 200000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 100 && pl < 400000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 200 && pl < 1000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 300 && pl < 5000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 400 && pl < 8000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 500 && pl < 15000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 1000 && pl < 25000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 5000 && pl < 100000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else if (gravity == 10000 && pl < 200000000) {
    send_to_char(ch, "You are hardly able to move in this gravity!\r\n");
    return 0;
  } else {
    return 1;
  }
}

/* If they can preform the attack or perform the attack on target. */
int can_kill(struct char_data *ch, struct char_data *vict, struct obj_data *obj,
             int num) {
  /* Target Related */
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return 0;
  }
  if (IS_CARRYING_W(ch) > CAN_CARRY_W(ch)) {
    send_to_char(ch, "You are weighted down too much!\r\n");
    return 0;
  }
  if (vict) {
    if (GET_HIT(vict) <= 0 && FIGHTING(vict)) {
      return 0;
    }
    if (room_flagged(char_room_get(ch), ROOM_PEACEFUL)) {
      send_to_char(ch,
                   "This room just has such a peaceful, easy feeling...\r\n");
      return 0;
    } else if (vict == ch) {
      send_to_char(ch, "That's insane, don't hurt yourself. Hurt others! "
                       "That's the key to life ^_^\r\n");
      return 0;
    } else if (vict->gooptime > 0) {
      send_to_char(ch,
                   "It seems like it'll be hard to kill them right now...\r\n");
      return 0;
    } else if (CARRYING(ch)) {
      send_to_char(
          ch, "You are too busy protecting the person on your shoulder!\r\n");
      return 0;
    } else if (CARRIED_BY(vict)) {
      send_to_char(ch, "They are being protected by someone else!\r\n");
      return 0;
    } else if (AFF_FLAGGED(vict, AFF_PARALYZE)) {
      send_to_char(ch, "They are a statue, just leave them alone...\r\n");
      return 0;
    } else if (MOB_FLAGGED(vict, MOB_NOKILL)) {
      send_to_char(ch, "But they are not to be killed!\r\n");
      return 0;
    } else if (char_condition_has(ch, "majinized") && char_condition_number_get(ch, "majinized", "lord") == GET_IDNUM(vict)) {
      send_to_char(ch, "You can not harm your master!\r\n");
      return 0;
    } else if (GET_BONUS(ch, BONUS_COWARD) > 0 &&
               GET_MAX_HIT(vict) > GET_MAX_HIT(ch) + (GET_MAX_HIT(ch) * .5) &&
               !FIGHTING(ch)) {
      send_to_char(ch, "You are too cowardly to start anything with someone so "
                       "much stronger than yourself!\r\n");
      return 0;
    } else if (char_condition_has(vict, "majinized") && char_condition_number_get(vict, "majinized", "lord") == GET_IDNUM(ch)) {
      send_to_char(ch, "You can not harm your servant.\r\n");
      return 0;
    } else if ((GRAPPLING(ch) && GRAPTYPE(ch) != 3) ||
               (GRAPPLED(ch) && (GRAPTYPE(ch) == 1 || GRAPTYPE(ch) == 4))) {
      send_to_char(ch, "You are too busy grappling!%s\r\n",
                   GRAPPLED(ch) != NULL ? " Try 'escape'!" : "");
      return 0;
    } else if (GRAPPLING(ch) && GRAPPLING(ch) != vict) {
      send_to_char(ch,
                   "You can't reach that far in your current position!\r\n");
      return 0;
    } else if (GRAPPLED(ch) && GRAPPLED(ch) != vict) {
      send_to_char(ch,
                   "You can't reach that far in your current position!\r\n");
      return 0;
    } else if (!IS_NPC(ch) && !IS_NPC(vict) && AFF_FLAGGED(ch, AFF_SPIRIT) &&
               (!is_sparring(ch) || !is_sparring(vict)) && num != 2) {
      send_to_char(ch, "You can not fight other players in AL/Hell.\r\n");
      return 0;
    } else if (GET_LEVEL(vict) <= 8 && !IS_NPC(ch) && !IS_NPC(vict) &&
               (!is_sparring(ch) || !is_sparring(vict))) {
      send_to_char(ch, "Newbie Shield Protects them!\r\n");
      return 0;
    } else if (GET_LEVEL(ch) <= 8 && !IS_NPC(ch) && !IS_NPC(vict) &&
               (!is_sparring(ch) || !is_sparring(vict))) {
      send_to_char(ch, "Newbie Shield Protects you until level 8.\r\n");
      return 0;
    } else if (char_condition_has(vict, "spiral") && num != 3) {
      send_to_char(ch,
                   "Due to the nature of their current technique anything less "
                   "than a Tier 4 or AOE attack will not work on them.\r\n");
      return 0;
    } else if (ABSORBING(ch)) {
      send_to_char(ch, "You are too busy absorbing %s!\r\n",
                   GET_NAME(ABSORBING(ch)));
      return 0;
    } else if (ABSORBBY(ch)) {
      send_to_char(ch, "You are too busy being absorbed by %s!\r\n",
                   GET_NAME(ABSORBBY(ch)));
      return 0;
    } else if ((GET_ALT(vict) > GET_ALT(ch) || GET_ALT(vict) < GET_ALT(ch)) &&
               IS_NAMEK(ch)) {
      act("@GYou stretch your limbs toward @g$N@G in an attempt to hit $M!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@g$n@G stretches $s limbs toward @RYOU@G in an attempt to land a "
          "hit!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@g$n@G stretches $s limbs toward @g$N@G in an attempt to hit $M!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      return 1;
    } else if (char_condition_has(ch, "flying") && !char_condition_has(vict, "flying") &&
               num == 0) {
      send_to_char(ch, "You are too far above them.\r\n");
      return 0;
    } else if (!char_condition_has(ch, "flying") && char_condition_has(vict, "flying") &&
               num == 0) {
      send_to_char(ch, "They are too far above you.\r\n");
      return 0;
    } else if (!IS_NPC(ch) && GET_ALT(ch) > GET_ALT(vict) && !IS_NPC(vict) &&
               num == 0) {
      if (GET_ALT(vict) < 0) {
        char_condition_number_set(vict, "flying", "altitude", GET_ALT(ch));
        return 1;
      } else {
        send_to_char(ch, "You are too far above them.\r\n");
        return 0;
      }
    } else if (!IS_NPC(ch) && GET_ALT(ch) < GET_ALT(vict) && !IS_NPC(vict) &&
               num == 0) {
      if (GET_ALT(vict) > 2) {
        char_condition_number_set(vict, "flying", "altitude", GET_ALT(ch));
        return 1;
      } else {
        send_to_char(ch, "They are too far above you.\r\n");
        return 0;
      }
    } else {
      return 1;
    }
  }
  if (obj) {
    if (room_flagged(char_room_get(ch), ROOM_PEACEFUL)) {
      send_to_char(ch,
                   "This room just has such a peaceful, easy feeling...\r\n");
      return 0;
    } else if (OBJ_FLAGGED(obj, ITEM_UNBREAKABLE) && GET_OBJ_VNUM(obj) != 87 &&
               GET_OBJ_VNUM(obj) != 80 && GET_OBJ_VNUM(obj) != 81 &&
               GET_OBJ_VNUM(obj) != 82 && GET_OBJ_VNUM(obj) != 83) {
      send_to_char(ch,
                   "You can't hit that, it is protected by the immortals!\r\n");
      return 0;
    } else if (char_condition_has(ch, "flying")) {
      send_to_char(ch, "You are too far above it.\r\n");
      return 0;
    } else if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is already broken!\r\n");
      return 0;
    } else {
      return 1;
    }
  } else {
    send_to_char(ch, "Error: Report to imm.");
    return 0;
  }
}

/* Whether they know the skill they are trying to use */
int check_skill(struct char_data *ch, int skill) {
  if (!know_skill(ch, skill) && !IS_NPC(ch)) {
    return 0;
  } else {
    return 1;
  }
}

/* Whether they have enough stamina or charged ki to preform the skill */
int check_points(struct char_data *ch, int64_t ki, int64_t st) {

  if (GET_PREFERENCE(ch) == PREFERENCE_H2H &&
      GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.1) {
    st -= st * 0.5;
  }

  int fail = FALSE;
  if (IS_NPC(ch)) {
    if ((getCurKI(ch)) < ki) {
      send_to_char(ch, "You do not have enough ki!\r\n");
      fail = TRUE;
    }
    if ((getCurST(ch)) < st) {
      send_to_char(ch, "You do not have enough stamina!\r\n");
      fail = TRUE;
    }
  } else {
    if (!calcGravCost(ch, st) && ki <= 0) {
      send_to_char(
          ch,
          "You do not have enough stamina to perform it in this gravity!\r\n");
      return 0;
    }
    if (GET_CHARGE(ch) < ki) {
      send_to_char(ch, "You do not have enough ki charged.\r\n");
      int64_t perc = GET_MAX_MANA(ch) * 0.01;
      if (ki >= perc * 49) {
        send_to_char(ch, "You need at least 50 percent charged.\r\n");
      } else if (ki >= perc * 44) {
        send_to_char(ch, "You need at least 45 percent charged.\r\n");
      } else if (ki >= perc * 39) {
        send_to_char(ch, "You need at least 40 percent charged.\r\n");
      } else if (ki >= perc * 34) {
        send_to_char(ch, "You need at least 35 percent charged.\r\n");
      } else if (ki >= perc * 29) {
        send_to_char(ch, "You need at least 30 percent charged.\r\n");
      } else if (ki >= perc * 24) {
        send_to_char(ch, "You need at least 25 percent charged.\r\n");
      } else if (ki >= perc * 19) {
        send_to_char(ch, "You need at least 20 percent charged.\r\n");
      } else if (ki >= perc * 14) {
        send_to_char(ch, "You need at least 15 percent charged.\r\n");
      } else if (ki >= perc * 9) {
        send_to_char(ch, "You need at least 10 percent charged.\r\n");
      } else if (ki >= perc * 4) {
        send_to_char(ch, "You need at least 5 percent charged.\r\n");
      } else if (ki >= 1) {
        send_to_char(ch, "You need at least 1 percent charged.\r\n");
      }
      fail = TRUE;
    }
    if (IS_NONPTRANS(ch)) {
      if (PLR_FLAGGED(ch, PLR_TRANS1)) {
        st -= st * 0.2;
      } else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
        st -= st * 0.4;
      } else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
        st -= st * 0.6;
      } else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
        st -= st * 0.8;
      }
    }
    if ((getCurST(ch)) < st) {
      send_to_char(ch, "You do not have enough stamina.\r\n@C%s@n needed.\r\n",
                   add_commas(st));
      fail = TRUE;
    }
  }
  if (fail == TRUE) {
    return 0;
  } else {
    return 1;
  }
}

/* Subtract the stamina or ki required */
void pcost(struct char_data *ch, double ki, int64_t st) {
  int before = 0;
  if (GET_LEVEL(ch) > 1 && !IS_NPC(ch)) {
    if (ki == 0) {
      before = (getCurST(ch));
      if (calcGravCost(ch, 0)) {
        if (before > (getCurST(ch))) {
          send_to_char(ch, "You exert more stamina in this gravity.\r\n");
        }
      }
    }
    if (GET_CHARGE(ch) <= (GET_MAX_MANA(ch) * ki)) {
      char_charge_set(ch, 0);
    }
    if (GET_CHARGE(ch) > (GET_MAX_MANA(ch) * ki)) {
      char_charge_set(ch, GET_CHARGE(ch) - (int64_t)(GET_MAX_MANA(ch) * ki));
    }
    if (GET_CHARGE(ch) < 0) {
      char_charge_set(ch, 0);
    }
    if (GET_KAIOKEN(ch) > 0) {
      st += (st / 20) * GET_KAIOKEN(ch);
    }
    if (char_condition_has(ch, "hasshuken")) {
      st += st * .3;
    }
    if (!IS_NPC(ch) && GET_BONUS(ch, BONUS_HARDWORKER) > 0) {
      st -= st * .25;
    } else if (!IS_NPC(ch) && GET_BONUS(ch, BONUS_SLACKER) > 0) {
      st += st * .25;
    }
    if (IS_ICER(ch)) {
      if (PLR_FLAGGED(ch, PLR_TRANS1)) {
        st = st * 1.05;
      } else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
        st = st * 1.1;
      } else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
        st = st * 1.15;
      } else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
        st = st * 1.20;
      }
    }
    if (GET_PREFERENCE(ch) == PREFERENCE_H2H &&
        GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.1) {
      st -= st * 0.5;
      char_charge_set(ch, GET_CHARGE(ch) - (int64_t)st);
    }
    if (IS_NONPTRANS(ch)) {
      if (PLR_FLAGGED(ch, PLR_TRANS1)) {
        st -= st * 0.2;
      } else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
        st -= st * 0.4;
      } else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
        st -= st * 0.6;
      } else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
        st -= st * 0.8;
      }
    }
    decCurST(ch, st);
  }
  if (IS_NPC(ch)) {
    decCurKI(ch, ki);
    decCurST(ch, st);
  }
}

/* For checking if they have enough free limbs to preform the technique. */
int limb_ok(struct char_data *ch, int type) {
  if (IS_NPC(ch)) {
    if (AFF_FLAGGED(ch, AFF_ENSNARED) && rand_number(1, 100) <= 90) {
      return FALSE;
    }
    return TRUE;
  }
  if (GRAPPLING(ch) && GRAPTYPE(ch) != 3) {
    send_to_char(ch, "You are too busy grappling!\r\n");
    return FALSE;
  }
  if (GRAPPLED(ch) && (GRAPTYPE(ch) == 1 || GRAPTYPE(ch) == 4)) {
    send_to_char(ch, "You are unable to move while in this hold! Try using "
                     "'escape' to get out of it!\r\n");
    return FALSE;
  }
  if (char_condition_has(ch, "mystic_melody")) {
    send_to_char(ch, "You are currently playing a song! Enter the song command "
                     "in order to stop!\r\n");
    return FALSE;
  }
  if (type == 0) {
    if (!HAS_ARMS(ch)) {
      send_to_char(ch, "You have no available arms!\r\n");
      return FALSE;
    }
    if (AFF_FLAGGED(ch, AFF_ENSNARED) && rand_number(1, 100) <= 90) {
      send_to_char(ch, "You are unable to move your arms while bound by this "
                       "strong silk!\r\n");
      WAIT_STATE(ch, PULSE_1SEC);
      return FALSE;
    } else if (AFF_FLAGGED(ch, AFF_ENSNARED)) {
      act("You manage to break the silk ensnaring your arms!", TRUE, ch, 0, 0,
          TO_CHAR);
      act("$n manages to break the silk ensnaring $s arms!", TRUE, ch, 0, 0,
          TO_ROOM);
      char_condition_remove(ch, "ensnared", "equipped");
    }
    if (GET_EQ(ch, WEAR_WIELD1) && GET_EQ(ch, WEAR_WIELD2)) {
      send_to_char(ch, "Your hands are full!\r\n");
      return FALSE;
    }
  } // Arms
  else if (type > 0) {
    if (!HAS_LEGS(ch)) {
      send_to_char(ch, "You have no working legs!\r\n");
      return FALSE;
    }
  } // Legs
  return TRUE;
}

size_t send_to_char(struct char_data *ch, const char *messg, ...) {
  if (ch->desc && messg && *messg) {
    size_t left;
    va_list args;

    va_start(args, messg);
    left = vwrite_to_output(ch->desc, messg, args);
    va_end(args);
    return left;
  }
  return 0;
}

int init_skill(struct char_data *ch, int snum) {
  int skill = 0;

  if (!IS_NPC(ch)) {
    skill = GET_SKILL(ch, snum);

    if (PLR_FLAGGED(ch, PLR_TRANSMISSION)) {
      skill += 4;
    }

    if (skill > 118)
      skill = 118;

    return (skill);
  }

  if (IS_NPC(ch) && GET_LEVEL(ch) <= 10) {
    skill = rand_number(30, 50);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 20) {
    skill = rand_number(45, 65);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 30) {
    skill = rand_number(55, 70);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 50) {
    skill = rand_number(65, 80);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 70) {
    skill = rand_number(75, 90);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 80) {
    skill = rand_number(85, 100);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 90) {
    skill = rand_number(90, 100);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 100) {
    skill = rand_number(95, 100);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 110) {
    skill = rand_number(95, 105);
  } else {
    skill = rand_number(100, 110);
  }

  return (skill);
}

/* For calculating the difficulty the player has to hit with the skill. */
int chance_to_hit(struct char_data *ch) {

  int num = axion_dice(0);

  if (IS_NPC(ch))
    return (num);

  if (char_stat_get(ch, "drunk") > 4) {
    num += char_stat_get(ch, "drunk");
  }

  return (num);
}

/* For calculating the speed of the attacker and defender */
int handle_speed(struct char_data *ch, struct char_data *vict) {

  if (ch == NULL || vict == NULL) { /* Ruh roh*/
    return (0);
  }

  if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 4) {
    return (15);
  } else if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 2) {
    return (10);
  } else if (GET_SPEEDI(ch) > GET_SPEEDI(vict)) {
    return (5);
  } else if (GET_SPEEDI(ch) * 4 < GET_SPEEDI(vict)) {
    return (-15);
  } else if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict)) {
    return (-10);
  } else if (GET_SPEEDI(ch) < GET_SPEEDI(vict)) {
    return (-5);
  }

  return (0);
}

bool race_has_tail(int r_id) {
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

void char_lose_tail(char_data *ch) {
  if (!char_has_tail(ch))
    return;
  switch (ch->race) {
  case RACE_ICER:
  case RACE_BIO:
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TAIL);
    remove_limb(ch, 6);
    break;
  case RACE_SAIYAN:
  case RACE_HALFBREED:
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_STAIL);
    remove_limb(ch, 5);
    if (PLR_FLAGGED(ch, PLR_OOZARU)) {
      oozaru_revert(ch);
    }
    break;
  }
  if (!IS_NPC(ch) && !PLR_FLAGGED(ch, PLR_NOGROW)) {
    char_condition_apply(ch, "regrow_tail", "natural", "tail_lost");
  }
}

bool char_has_tail(char_data *ch) {
  if (!race_has_tail(ch->race))
    return false;
  switch (ch->race) {
  case RACE_ICER:
  case RACE_BIO:
    return PLR_FLAGGED(ch, PLR_TAIL);
  case RACE_SAIYAN:
  case RACE_HALFBREED:
    return PLR_FLAGGED(ch, PLR_STAIL);
  default:
    return false;
  }
}

void char_gain_tail(char_data *ch, bool announce) {
  if (char_has_tail(ch))
    return;
  switch (ch->race) {
  case RACE_ICER:
  case RACE_BIO:
    SET_BIT_AR(PLR_FLAGS(ch), PLR_TAIL);
    break;
  case RACE_SAIYAN:
  case RACE_HALFBREED:
    SET_BIT_AR(PLR_FLAGS(ch), PLR_STAIL);
    if (MOON_OK(ch)) {
      oozaru_transform(ch);
    }
    break;
  }
}

room_vnum sensei_location_id(int s_id) {
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

room_vnum sensei_start_room(int s_id) {
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

int sensei_grav_tolerance(int s_id) {
  switch (s_id) {
  case CLASS_BARDOCK:
    return 10;
  default:
    return 0;
  }
}

int sensei_rpp_cost(int s_id, int r_id) {
  switch (s_id) {
  case CLASS_KABITO:
    if (r_id != RACE_KAI) {
      return 10;
    } else {
      return 0;
    }
  default:
    return 0;
  }
}

int race_get_size(int r_id) {
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

int race_is_playable(int r_id) {
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

int race_is_people(int r_id) {
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

struct room_direction_data *char_exit_dir(struct char_data *ch, int dir) {
  struct room_data *room = char_room_get(ch);
  if (!room) {
    return NULL;
  }

  if (dir < 0 || dir >= NUM_OF_DIRS) {
    return NULL;
  }

  return room_dir_option_get(room, dir);
}

struct room_direction_data *char_exit_dir_2nd(struct char_data *ch, int dir) {
  struct room_direction_data *exit = char_exit_dir(ch, dir);
  if (!exit) {
    return NULL;
  }

  struct room_data *dest = exit_dest_get(exit);
  if (!dest) {
    return NULL;
  }

  if (dir < 0 || dir >= NUM_OF_DIRS) {
    return NULL;
  }

  return room_dir_option_get(dest, dir);
}

struct room_direction_data *char_exit_dir_3rd(struct char_data *ch, int dir) {
  struct room_direction_data *exit = char_exit_dir_2nd(ch, dir);
  if (!exit) {
    return NULL;
  }

  struct room_data *dest = exit_dest_get(exit);
  if (!dest) {
    return NULL;
  }

  if (dir < 0 || dir >= NUM_OF_DIRS) {
    return NULL;
  }

  return room_dir_option_get(dest, dir);
}

struct room_data* char_can_go_exit(struct char_data *ch, struct room_direction_data *exit) {
  if (!exit)
    return NULL;

  if (exit_flagged(exit, EX_CLOSED)) {
    return NULL;
  }

  struct room_data *dest = exit_dest_get(exit);
  if (!dest) {
    return NULL;
  }

  return dest;
}

struct room_data* char_can_go_dir(struct char_data *ch, int dir) {

  struct room_direction_data *exit = char_exit_dir(ch, dir);
  return char_can_go_exit(ch, exit);
}

#define SELF(sub, obj) ((sub) == (obj))

#define LIGHT_OK(sub)                                                          \
  (!AFF_FLAGGED(sub, AFF_BLIND) && !PLR_FLAGGED(sub, PLR_EYEC) &&              \
   (IS_LIGHT(char_room_get(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION) ||     \
    (IS_MUTANT(sub) && HAS_GENOME(sub, 4)) ||                                  \
    PLR_FLAGGED(sub, PLR_AURALIGHT)))

#define INVIS_OK(sub, obj)                                                     \
  (!AFF_FLAGGED((obj), AFF_INVISIBLE) || AFF_FLAGGED(sub, AFF_DETECT_INVIS))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj)                                                  \
  (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define NOT_HIDDEN(ch) (!AFF_FLAGGED(ch, AFF_HIDE))
/* End of CAN_SEE */

#define INVIS_OK_OBJ(sub, obj)                                                 \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj)                                          \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&                      \
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj)                                             \
  ((LIGHT_OK(sub) || obj->carried_by == sub || obj->worn_by) &&                \
   INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

bool char_can_see_in_dark(struct char_data *ch) {
  return (AFF_FLAGGED(ch, AFF_INFRAVISION) ||
          (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) ||
          (IS_MUTANT(ch) && HAS_GENOME(ch, 4)) ||
          PLR_FLAGGED(ch, PLR_AURALIGHT));
}

bool char_can_see_char(struct char_data *ch, struct char_data *vict) {
  return (SELF(ch, vict) ||
          ((GET_ADMLEVEL(ch) >= (IS_NPC(vict) ? 0 : GET_INVIS_LEV(vict))) &&
           IMM_CAN_SEE(ch, vict) &&
           (NOT_HIDDEN(vict) || GET_ADMLEVEL(ch) > 0)));
}

bool char_can_see_obj(struct char_data *ch, struct obj_data *obj) {
  return (MORT_CAN_SEE_OBJ(ch, obj) ||
          (!IS_NPC(ch) && PRF_FLAGGED((ch), PRF_HOLYLIGHT)));
}

bool char_has_arms(struct char_data *ch) {
  return (((IS_NPC(ch) &&
            (MOB_FLAGGED(ch, MOB_LARM) || MOB_FLAGGED(ch, MOB_RARM))) ||
           GET_LIMBCOND(ch, 1) > 0 || GET_LIMBCOND(ch, 2) > 0 ||
           PLR_FLAGGED(ch, PLR_CRARM) || PLR_FLAGGED(ch, PLR_CLARM)) &&
          ((!GRAPPLING(ch) && !GRAPPLED(ch)) ||
           (GRAPPLING(ch) && GRAPTYPE(ch) == 3) ||
           (GRAPPLED(ch) && GRAPTYPE(ch) != 1 && GRAPTYPE(ch) != 4)));
}

bool char_has_legs(struct char_data *ch) {
  return (((IS_NPC(ch) &&
            (MOB_FLAGGED(ch, MOB_LLEG) || MOB_FLAGGED(ch, MOB_RLEG))) ||
           GET_LIMBCOND(ch, 3) > 0 || GET_LIMBCOND(ch, 4) > 0 ||
           PLR_FLAGGED(ch, PLR_CRLEG) || PLR_FLAGGED(ch, PLR_CLLEG)) &&
          ((!GRAPPLING(ch) && !GRAPPLED(ch)) ||
           (GRAPPLING(ch) && GRAPTYPE(ch) == 3) ||
           (GRAPPLED(ch) && GRAPTYPE(ch) != 1)));
}

bool char_outside_sector_type(struct char_data *ch) {
  struct room_data *room = char_room_get(ch);
  if (!room) {
    return false;
  }
  switch (room_sector_type_get(room)) {
  case SECT_INSIDE:
  case SECT_UNDERWATER:
  case SECT_IMPORTANT:
  case SECT_SHOP:
  case SECT_SPACE:
    return false;
  default:
    return true;
  }
}

bool char_outside_roomflag(struct char_data *ch) {
  struct room_data *room = char_room_get(ch);
  if (!room) {
    return false;
  }
  if (room_flagged(room, ROOM_INDOORS))
    return false;
  if (room_flagged(room, ROOM_UNDERGROUND))
    return false;
  if (room_flagged(room, ROOM_SPACE))
    return false;
  return true;
}

bool char_ether_stream(struct char_data *ch) {
  struct room_data *room = char_room_get(ch);
  if (!room) {
    return false;
  }
  for (auto flag : {ROOM_EARTH, ROOM_AETHER, ROOM_NAMEK}) {
    if (room_flagged(room, flag))
      return true;
  }
  if (char_planet_zenith(ch))
    return true;
  return false;
}

bool char_has_moon(struct char_data *ch) {
  struct room_data *room = char_room_get(ch);
  if (!room) {
    return false;
  }
  for (auto flag : {ROOM_VEGETA, ROOM_EARTH, ROOM_AETHER}) {
    if (room_flagged(room, flag))
      return true;
  }
  return false;
}

bool char_planet_zenith(struct char_data *ch) {
  room_vnum v = char_room_vnum_get(ch);
  if (v == NOWHERE)
    return false;
  return ((v >= 3400 && v <= 3599) || (v >= 62900 && v <= 62999) ||
          (v == 19600));
}

/* This handles charging for energy attacks */
extern "C" bool release_charge(struct char_data *ch) {
  if (GET_CHARGE(ch) <= 0) {
    return false;
  }

  send_to_char(ch,
               PLR_FLAGGED(ch, PLR_CHARGE)
                   ? "You stop charging and release your pent up energy.\r\n"
                   : "You release your pent up energy.\r\n");
  switch (rand_number(1, 3)) {
  case 1:
    act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  case 2:
    act("$n@w's aura fades.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  case 3:
    act("$n@w's aura flickers brightly before disappearing.@n", TRUE, ch, 0, 0,
        TO_ROOM);
    break;
  default:
    act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  incCurKI(ch, GET_CHARGE(ch));
  char_charge_set(ch, 0);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CHARGE);
  char_condition_remove(ch, "charge", "released");
  return true;
}

bool char_is_extracted(struct char_data *ch) {
  if(IS_NPC(ch)) return IS_SET_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
  else return PLR_FLAGGED(ch, PLR_NOTDEADYET);
}

extern "C" int64_t char_level_exp(struct char_data *ch, int level) {
  return level_exp(ch, level);
}

extern "C" int char_rpp_to_level(struct char_data *ch) {
  return rpp_to_level(ch);
}

#include "mail.h"
#include "config.h"

extern "C" bool char_has_mail(struct char_data *ch) {
  return has_mail(GET_IDNUM(ch)) != 0;
}

extern "C" bool char_news_pending(struct char_data *ch) {
  return NEWSUPDATE > GET_LPLAY(ch);
}

extern "C" int char_slot_count(struct char_data *ch) {
  return slot_count(ch);
}
