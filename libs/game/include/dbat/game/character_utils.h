#pragma once

#include "dbat/game/structs.h"

const std::string &juggleRaceName(char_data *ch, bool capitalized);
void restore_by(char_data *ch, char_data *healer);
void restore(char_data *ch, bool announce);
void resurrect(char_data *ch, ResurrectionMode mode);
void ghostify(char_data *ch);
void teleport_to(char_data *ch, IDXTYPE rnum);
bool in_room_range(char_data *ch, IDXTYPE low_rnum, IDXTYPE high_rnum);
bool in_past(char_data *ch);
bool is_newbie(char_data *ch);
bool in_northran(char_data *ch);
bool can_tolerate_gravity(char_data *ch, int grav);
int calcTier(char_data *ch);
int64_t calc_soft_cap(char_data *ch);
bool is_soft_cap(char_data *ch, int64_t type, long double mult);
bool is_soft_cap(char_data *ch, int64_t type);
int wearing_android_canister(char_data *ch);
int calcGravCost(char_data *ch, int64_t num);

int64_t getCurHealth(char_data *ch);
int64_t getMaxHealth(char_data *ch);
double getCurHealthPercent(char_data *ch);
int64_t getPercentOfCurHealth(char_data *ch, double amt);
int64_t getPercentOfMaxHealth(char_data *ch, double amt);
bool isFullHealth(char_data *ch);
int64_t setCurHealth(char_data *ch, int64_t amt);
int64_t setCurHealthPercent(char_data *ch, double amt);
int64_t incCurHealth(char_data *ch, int64_t amt, bool limit_max = true);
int64_t decCurHealth(char_data *ch, int64_t amt, int64_t floor = 0);
int64_t incCurHealthPercent(char_data *ch, double amt, bool limit_max = true);
int64_t decCurHealthPercent(char_data *ch, double amt, int64_t floor = 0);
void restoreHealth(char_data *ch, bool announce = true);
int64_t healCurHealth(char_data *ch, int64_t amt);
int64_t harmCurHealth(char_data *ch, int64_t amt);

int64_t getMaxPL(char_data *ch);
int64_t getMaxPLTrans(char_data *ch);
int64_t getCurPL(char_data *ch);
int64_t getBasePL(char_data *ch);
int64_t getEffBasePL(char_data *ch);
double getCurPLPercent(char_data *ch);
int64_t getPercentOfCurPL(char_data *ch, double amt);
int64_t getPercentOfMaxPL(char_data *ch, double amt);
bool isFullPL(char_data *ch);

int64_t getCurKI(char_data *ch);
int64_t getMaxKI(char_data *ch);
int64_t getBaseKI(char_data *ch);
int64_t getEffBaseKI(char_data *ch);
double getCurKIPercent(char_data *ch);
int64_t getPercentOfCurKI(char_data *ch, double amt);
int64_t getPercentOfMaxKI(char_data *ch, double amt);
bool isFullKI(char_data *ch);
int64_t setCurKI(char_data *ch, int64_t amt);
int64_t setCurKIPercent(char_data *ch, double amt);
int64_t incCurKI(char_data *ch, int64_t amt, bool limit_max = true);
int64_t decCurKI(char_data *ch, int64_t amt, int64_t floor = 0);
int64_t incCurKIPercent(char_data *ch, double amt, bool limit_max = true);
int64_t decCurKIPercent(char_data *ch, double amt, int64_t floor = 0);
void restoreKI(char_data *ch, bool announce = true);

int64_t getCurST(char_data *ch);
int64_t getMaxST(char_data *ch);
int64_t getBaseST(char_data *ch);
int64_t getEffBaseST(char_data *ch);
double getCurSTPercent(char_data *ch);
int64_t getPercentOfCurST(char_data *ch, double amt);
int64_t getPercentOfMaxST(char_data *ch, double amt);
bool isFullST(char_data *ch);
int64_t setCurST(char_data *ch, int64_t amt);
int64_t setCurSTPercent(char_data *ch, double amt);
int64_t incCurST(char_data *ch, int64_t amt, bool limit_max = true);
int64_t decCurST(char_data *ch, int64_t amt, int64_t floor = 0);
int64_t incCurSTPercent(char_data *ch, double amt, bool limit_max = true);
int64_t decCurSTPercent(char_data *ch, double amt, int64_t floor = 0);
void restoreST(char_data *ch, bool announce = true);

int64_t getCurLF(char_data *ch);
int64_t getMaxLF(char_data *ch);
double getCurLFPercent(char_data *ch);
int64_t getPercentOfCurLF(char_data *ch, double amt);
int64_t getPercentOfMaxLF(char_data *ch, double amt);
bool isFullLF(char_data *ch);
int64_t setCurLF(char_data *ch, int64_t amt);
int64_t setCurLFPercent(char_data *ch, double amt);
int64_t incCurLF(char_data *ch, int64_t amt, bool limit_max = true);
int64_t decCurLF(char_data *ch, int64_t amt, int64_t floor = 0);
int64_t incCurLFPercent(char_data *ch, double amt, bool limit_max = true);
int64_t decCurLFPercent(char_data *ch, double amt, int64_t floor = 0);
void restoreLF(char_data *ch, bool announce = true);

bool isFullVitals(char_data *ch);
void restoreVitals(char_data *ch, bool announce = true);
void restoreStatus(char_data *ch, bool announce = true);
void restoreLimbs(char_data *ch, bool announce = true);

int64_t gainBasePL(char_data *ch, int64_t amt, bool trans_mult = false);
int64_t gainBaseKI(char_data *ch, int64_t amt, bool trans_mult = false);
int64_t gainBaseST(char_data *ch, int64_t amt, bool trans_mult = false);
void gainBaseAll(char_data *ch, int64_t amt, bool trans_mult = false);

int64_t loseBasePL(char_data *ch, int64_t amt, bool trans_mult = false);
int64_t loseBaseKI(char_data *ch, int64_t amt, bool trans_mult = false);
int64_t loseBaseST(char_data *ch, int64_t amt, bool trans_mult = false);
void loseBaseAll(char_data *ch, int64_t amt, bool trans_mult = false);

int64_t gainBasePLPercent(char_data *ch, double amt, bool trans_mult = false);
int64_t gainBaseKIPercent(char_data *ch, double amt, bool trans_mult = false);
int64_t gainBaseSTPercent(char_data *ch, double amt, bool trans_mult = false);
void gainBaseAllPercent(char_data *ch, double amt, bool trans_mult = false);

int64_t loseBasePLPercent(char_data *ch, double amt, bool trans_mult = false);
int64_t loseBaseKIPercent(char_data *ch, double amt, bool trans_mult = false);
int64_t loseBaseSTPercent(char_data *ch, double amt, bool trans_mult = false);
void loseBaseAllPercent(char_data *ch, double amt, bool trans_mult = false);

void cureStatusKnockedOut(char_data *ch, bool announce = true);
void cureStatusBurn(char_data *ch, bool announce = true);
void cureStatusPoison(char_data *ch, bool announce = true);
void setStatusKnockedOut(char_data *ch);

int64_t getMaxCarryWeight(char_data *ch);
int64_t getCurGearWeight(char_data *ch);
int64_t getCurCarriedWeight(char_data *ch);
int64_t getAvailableCarryWeight(char_data *ch);

double speednar(char_data *ch);
int64_t getEffMaxPL(char_data *ch);
bool isWeightedPL(char_data *ch);

void apply_kaioken(char_data *ch, int times, bool announce);
void remove_kaioken(char_data *ch, int8_t announce);