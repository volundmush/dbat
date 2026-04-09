//
// Created by basti on 10/24/2021.
//
#include "dbat/game/structs.h"
#include "dbat/game/races.h"
#include "dbat/game/utils.h"
#include "dbat/game/spells.h"
#include "dbat/game/comm.h"
#include "dbat/game/class.h"
#include "dbat/game/fight.h"
#include "dbat/game/act.movement.h"
#include "dbat/game/character_utils.h"

const std::string& char_data::juggleRaceName(bool capitalized) {
    return ::juggleRaceName(this, capitalized);
}

void char_data::restore_by(char_data *ch) {
    ::restore_by(this, ch);
}

void char_data::restore(bool announce) {
    ::restore(this, announce);
}

void char_data::resurrect(ResurrectionMode mode) {
    ::resurrect(this, mode);
}

void char_data::ghostify() {
    ::ghostify(this);
}

void char_data::teleport_to(IDXTYPE rnum) {
    ::teleport_to(this, rnum);
}

bool char_data::in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum) {
    return ::in_room_range(this, low_rnum, high_rnum);
}

bool char_data::in_past() {
    return ::in_past(this);
}

bool char_data::is_newbie() {
    return ::is_newbie(this);
}

bool char_data::in_northran() {
    return ::in_northran(this);
}

static std::map<int, uint16_t> grav_threshold = {
        {10, 5000},
        {20, 20000},
        {30, 50000},
        {40, 100000},
        {50, 200000},
        {100, 400000},
        {200, 1000000},
        {300, 5000000},
        {400, 8000000},
        {500, 15000000},
        {1000, 25000000},
        {5000, 100000000},
        {10000, 200000000}
};

bool char_data::can_tolerate_gravity(int grav) {
    return ::can_tolerate_gravity(this, grav);
}


int char_data::calcTier() {
    return ::calcTier(this);
}

int64_t char_data::calc_soft_cap() {
    return ::calc_soft_cap(this);
}

bool char_data::is_soft_cap(int64_t type) {
    return ::is_soft_cap(this, type);
}

bool char_data::is_soft_cap(int64_t type, long double mult) {
    return ::is_soft_cap(this, type, mult);
}

int char_data::wearing_android_canister() {
    return ::wearing_android_canister(this);
}

int char_data::calcGravCost(int64_t num) {
    return ::calcGravCost(this, num);
}


int64_t char_data::getCurHealth() {
    return ::getCurHealth(this);
}

int64_t char_data::getMaxHealth() {
    return ::getMaxHealth(this);
}

double char_data::getCurHealthPercent() {
    return ::getCurHealthPercent(this);
}

int64_t char_data::getPercentOfCurHealth(double amt) {
    return ::getPercentOfCurHealth(this, amt);
}

int64_t char_data::getPercentOfMaxHealth(double amt) {
    return ::getPercentOfMaxHealth(this, amt);
}

bool char_data::isFullHealth() {
    return ::isFullHealth(this);
}

int64_t char_data::setCurHealth(int64_t amt) {
    return ::setCurHealth(this, amt);
}

int64_t char_data::setCurHealthPercent(double amt) {
    return ::setCurHealthPercent(this, amt);
}

int64_t char_data::incCurHealth(int64_t amt, bool limit_max) {
    return ::incCurHealth(this, amt, limit_max);
}

int64_t char_data::decCurHealth(int64_t amt, int64_t floor) {
    return ::decCurHealth(this, amt, floor);
}

int64_t char_data::incCurHealthPercent(double amt, bool limit_max) {
    return ::incCurHealthPercent(this, amt, limit_max);
}

int64_t char_data::decCurHealthPercent(double amt, int64_t floor) {
    return ::decCurHealthPercent(this, amt, floor);
}

void char_data::restoreHealth(bool announce) {
    ::restoreHealth(this, announce);
}

int64_t char_data::getMaxPLTrans() {
    return ::getMaxPLTrans(this);
}

int64_t char_data::getMaxPL() {
    return ::getMaxPL(this);
}

int64_t char_data::getCurPL() {
    return ::getCurPL(this);
}

int64_t char_data::getEffBasePL() {
    return ::getEffBasePL(this);
}

int64_t char_data::getBasePL() {
    return ::getBasePL(this);
}

double char_data::getCurPLPercent() {
    return ::getCurPLPercent(this);
}

int64_t char_data::getPercentOfCurPL(double amt) {
    return ::getPercentOfCurPL(this, amt);
}

int64_t char_data::getPercentOfMaxPL(double amt) {
    return ::getPercentOfMaxPL(this, amt);
}

bool char_data::isFullPL() {
    return ::isFullPL(this);
}

int64_t char_data::getCurKI() {
    return ::getCurKI(this);
}

int64_t char_data::getMaxKI() {
    return ::getMaxKI(this);
}

int64_t char_data::getEffBaseKI() {
    return ::getEffBaseKI(this);
}

int64_t char_data::getBaseKI() {
    return ::getBaseKI(this);
}

double char_data::getCurKIPercent() {
    return ::getCurKIPercent(this);
}

int64_t char_data::getPercentOfCurKI(double amt) {
    return ::getPercentOfCurKI(this, amt);
}

int64_t char_data::getPercentOfMaxKI(double amt) {
    return ::getPercentOfMaxKI(this, amt);
}

bool char_data::isFullKI() {
    return ::isFullKI(this);
}

int64_t char_data::setCurKI(int64_t amt) {
    return ::setCurKI(this, amt);
}

int64_t char_data::setCurKIPercent(double amt) {
    return ::setCurKIPercent(this, amt);
}

int64_t char_data::incCurKI(int64_t amt, bool limit_max) {
    return ::incCurKI(this, amt, limit_max);
}

int64_t char_data::decCurKI(int64_t amt, int64_t floor) {
    return ::decCurKI(this, amt, floor);
}

int64_t char_data::incCurKIPercent(double amt, bool limit_max) {
    return ::incCurKIPercent(this, amt, limit_max);
}

int64_t char_data::decCurKIPercent(double amt, int64_t floor) {
    return ::decCurKIPercent(this, amt, floor);
}


void char_data::restoreKI(bool announce) {
    ::restoreKI(this, announce);
}

int64_t char_data::getCurST() {
    return ::getCurST(this);
}

int64_t char_data::getMaxST() {
    return ::getMaxST(this);
}

int64_t char_data::getEffBaseST() {
    return ::getEffBaseST(this);
}

int64_t char_data::getBaseST() {
    return ::getBaseST(this);
}

double char_data::getCurSTPercent() {
    return ::getCurSTPercent(this);
}

int64_t char_data::getPercentOfCurST(double amt) {
    return ::getPercentOfCurST(this, amt);
}

int64_t char_data::getPercentOfMaxST(double amt) {
    return ::getPercentOfMaxST(this, amt);
}

bool char_data::isFullST() {
    return ::isFullST(this);
}

int64_t char_data::setCurST(int64_t amt) {
    return ::setCurST(this, amt);
}

int64_t char_data::setCurSTPercent(double amt) {
    return ::setCurSTPercent(this, amt);
}

int64_t char_data::incCurST(int64_t amt, bool limit_max) {
    return ::incCurST(this, amt, limit_max);
}

int64_t char_data::decCurST(int64_t amt, int64_t floor) {
    return ::decCurST(this, amt, floor);
}

int64_t char_data::incCurSTPercent(double amt, bool limit_max) {
    return ::incCurSTPercent(this, amt, limit_max);
}

int64_t char_data::decCurSTPercent(double amt, int64_t floor) {
    return ::decCurSTPercent(this, amt, floor);
}


void char_data::restoreST(bool announce) {
    ::restoreST(this, announce);
}


int64_t char_data::getCurLF() {
    return ::getCurLF(this);
}

int64_t char_data::getMaxLF() {
    return ::getMaxLF(this);
}

double char_data::getCurLFPercent() {
    return ::getCurLFPercent(this);
}

int64_t char_data::getPercentOfCurLF(double amt) {
    return ::getPercentOfCurLF(this, amt);
}

int64_t char_data::getPercentOfMaxLF(double amt) {
    return ::getPercentOfMaxLF(this, amt);
}

bool char_data::isFullLF() {
    return ::isFullLF(this);
}

int64_t char_data::setCurLF(int64_t amt) {
    return ::setCurLF(this, amt);
}

int64_t char_data::setCurLFPercent(double amt) {
    return ::setCurLFPercent(this, amt);
}

int64_t char_data::incCurLF(int64_t amt, bool limit_max) {
    return ::incCurLF(this, amt, limit_max);
}

int64_t char_data::decCurLF(int64_t amt, int64_t floor) {
    return ::decCurLF(this, amt, floor);
}

int64_t char_data::incCurLFPercent(double amt, bool limit_max) {
    return ::incCurLFPercent(this, amt, limit_max);
}

int64_t char_data::decCurLFPercent(double amt, int64_t floor) {
    return ::decCurLFPercent(this, amt, floor);
}


void char_data::restoreLF(bool announce) {
    ::restoreLF(this, announce);
}


bool char_data::isFullVitals() {
    return ::isFullVitals(this);
}

void char_data::restoreVitals(bool announce) {
    ::restoreVitals(this, announce);
}

void char_data::restoreStatus(bool announce) {
    ::restoreStatus(this, announce);
}

void char_data::setStatusKnockedOut() {
    ::setStatusKnockedOut(this);
}

void char_data::cureStatusKnockedOut(bool announce) {
    ::cureStatusKnockedOut(this, announce);
}

void char_data::cureStatusBurn(bool announce) {
    ::cureStatusBurn(this, announce);
}

void char_data::cureStatusPoison(bool announce) {
    ::cureStatusPoison(this, announce);
}

void char_data::restoreLimbs(bool announce) {
    ::restoreLimbs(this, announce);
}

int64_t char_data::gainBasePL(int64_t amt, bool trans_mult) {
    return ::gainBasePL(this, amt, trans_mult);
}

int64_t char_data::gainBaseST(int64_t amt, bool trans_mult) {
    return ::gainBaseST(this, amt, trans_mult);
}

int64_t char_data::gainBaseKI(int64_t amt, bool trans_mult) {
    return ::gainBaseKI(this, amt, trans_mult);
}

void char_data::gainBaseAll(int64_t amt, bool trans_mult) {
    ::gainBaseAll(this, amt, trans_mult);
}

int64_t char_data::loseBasePL(int64_t amt, bool trans_mult) {
    return ::loseBasePL(this, amt, trans_mult);
}

int64_t char_data::loseBaseST(int64_t amt, bool trans_mult) {
    return ::loseBaseST(this, amt, trans_mult);
}

int64_t char_data::loseBaseKI(int64_t amt, bool trans_mult) {
    return ::loseBaseKI(this, amt, trans_mult);
}

void char_data::loseBaseAll(int64_t amt, bool trans_mult) {
    ::loseBaseAll(this, amt, trans_mult);
}

int64_t char_data::gainBasePLPercent(double amt, bool trans_mult) {
    return ::gainBasePLPercent(this, amt, trans_mult);
}

int64_t char_data::gainBaseKIPercent(double amt, bool trans_mult) {
    return ::gainBaseKIPercent(this, amt, trans_mult);
}

int64_t char_data::gainBaseSTPercent(double amt, bool trans_mult) {
    return ::gainBaseSTPercent(this, amt, trans_mult);
}

int64_t char_data::loseBasePLPercent(double amt, bool trans_mult) {
    return ::loseBasePLPercent(this, amt, trans_mult);
}

int64_t char_data::loseBaseKIPercent(double amt, bool trans_mult) {
    return ::loseBaseKIPercent(this, amt, trans_mult);
}

int64_t char_data::loseBaseSTPercent(double amt, bool trans_mult) {
    return ::loseBaseSTPercent(this, amt, trans_mult);
}

void char_data::gainBaseAllPercent(double amt, bool trans_mult) {
    ::gainBaseAllPercent(this, amt, trans_mult);
}

void char_data::loseBaseAllPercent(double amt, bool trans_mult) {
    ::loseBaseAllPercent(this, amt, trans_mult);
}


int64_t char_data::getMaxCarryWeight() {
    return ::getMaxCarryWeight(this);
}

int64_t char_data::getCurGearWeight() {
    return ::getCurGearWeight(this);
}

int64_t char_data::getCurCarriedWeight() {
    return ::getCurCarriedWeight(this);
}

int64_t char_data::getAvailableCarryWeight() {
    return ::getAvailableCarryWeight(this);
}

double char_data::speednar() {
    return ::speednar(this);
}

int64_t char_data::getEffMaxPL() {
    return ::getEffMaxPL(this);
}

bool char_data::isWeightedPL() {
    return ::isWeightedPL(this);
}

void char_data::apply_kaioken(int times, bool announce) {
    ::apply_kaioken(this, times, announce);
}

void char_data::remove_kaioken(int8_t announce) {
    ::remove_kaioken(this, announce);

    switch(announce) {
        case 1:
            send_to_char(this, "You drop out of kaioken.\r\n");
            ::act("$n@w drops out of kaioken.@n", TRUE, this, 0, 0, TO_ROOM);
            break;
        case 2:
            send_to_char(this, "You lose focus and your kaioken disappears.\r\n");
            ::act("$n loses focus and $s kaioken aura disappears.", TRUE, this, 0, 0, TO_ROOM);
    }
}