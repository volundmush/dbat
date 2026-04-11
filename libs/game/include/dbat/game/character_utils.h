#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/character_utils.h"
#include "dbat/db/characters.h"
#include "dbat/game/log.h"
#include "dbat/game/races.h"
#include "dbat/game/class.h"
#include "dbat/db/weather.h"

int roll_stats(struct char_data *ch, int type, int bonus);
const char* juggleRaceName(char_data *ch, bool capitalized);
void restore_by(char_data *ch, char_data *healer);
void restore(char_data *ch, bool announce);
void resurrect(char_data *ch, int mode);
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
int64_t incCurHealth(char_data *ch, int64_t amt);
int64_t incCurHealthUnlimited(char_data *ch, int64_t amt);
int64_t decCurHealth(char_data *ch, int64_t amt, int64_t floor);
int64_t decCurHealthNoFloor(char_data *ch, int64_t amt);
int64_t incCurHealthPercent(char_data *ch, double amt, bool limit_max = true);
int64_t decCurHealthPercent(char_data *ch, double amt, int64_t floor = 0);
void restoreHealth(char_data *ch, bool announce);
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
void restoreKI(char_data *ch, bool announce);
void restoreKIAnnounce(char_data *ch);

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
void restoreST(char_data *ch, bool announce);
void restoreSTAnnounce(char_data *ch);

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
void restoreLF(char_data *ch, bool announce);
void restoreLFAnnounce(char_data *ch);

bool isFullVitals(char_data *ch);
void restoreVitals(char_data *ch, bool announce);
void restoreVitalsAnnounce(char_data *ch);
void restoreStatus(char_data *ch, bool announce);
void restoreStatusAnnounce(char_data *ch);
void restoreLimbs(char_data *ch, bool announce);
void restoreLimbsAnnounce(char_data *ch);

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

void dispel_ash(struct char_data *ch);
int has_group(struct char_data *ch);
const char *report_party_health(struct char_data *ch);
int sec_roll_check(struct char_data *ch);
int get_measure(struct char_data *ch, int height, int weight);
int trans_cost(struct char_data *ch, int trans);
int trans_req(struct char_data *ch, int trans);
const char *disp_align(struct char_data *ch);
void senseCreate(struct char_data *ch);
int read_sense_memory(struct char_data *ch, struct char_data *vict);
void sense_memory_write(struct char_data *ch, struct char_data *vict);
int roll_pursue(struct char_data *ch, struct char_data *vict);
void reveal_hiding(struct char_data *ch, int type);
int block_calc(struct char_data *ch);
int64_t molt_threshold(struct char_data *ch);
int armor_evolve(struct char_data *ch);
void handle_evolution(struct char_data *ch, int64_t dmg);
void demon_refill_lf(struct char_data *ch, int64_t num);
void mob_talk(struct char_data *ch, const char *speech);
int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech);
bool is_sparring(struct char_data *ch);
char *introd_calc(struct char_data *ch);
int64_t gear_exp(struct char_data *ch, int64_t exp);
void purge_homing(struct char_data *ch);
void improve_skill(struct char_data *ch, int skill, int num);
void stop_follower(struct char_data *ch);
int num_followers_charmed(struct char_data *ch);
void switch_leader(struct char_data *old, struct char_data *new_leader);
void die_follower(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
bool circle_follow(struct char_data *ch, struct char_data *victim);


void	advance_level(struct char_data *ch, int whichclass);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int64_t gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	update_pos(struct char_data *victim);

int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_remove(struct char_data *ch, int pos);


int can_grav(struct char_data *ch);
int check_skill(struct char_data *ch, int skill);
int check_points(struct char_data *ch, int64_t ki, int64_t st);
void pcost(struct char_data *ch, double ki, int64_t st);
int limb_ok(struct char_data *ch, int type);
int64_t physical_cost(struct char_data *ch, int skill);

int init_skill(struct char_data *ch, int snum);
int chance_to_hit(struct char_data *ch);
int handle_speed(struct char_data *ch, struct char_data *vict);
struct time_info_data *age(struct char_data *ch);
const char *get_i_name(struct char_data *ch, struct char_data *vict);

void assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd);
int know_skill(struct char_data *ch, int skill);

size_t send_to_char(struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void admin_set(struct char_data *ch, int value);
char *sense_location(struct char_data *ch);
void null_affect(struct char_data *ch, int aff_flag);
int planet_check(struct char_data *ch, struct char_data *vict);
/* Player autoexit levels: used as an index to exitlevels           */
#define EXIT_OFF        0       /* Autoexit off                     */
#define EXIT_NORMAL     1       /* Brief display (stock behaviour)  */
#define EXIT_NA         2       /* Not implemented - do not use     */
#define EXIT_COMPLETE   3       /* Full display                     */

#define _exitlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch),PRF_AUTOEXIT) ? 1 : 0 ) + (PRF_FLAGGED((ch),PRF_FULL_EXIT) ? 2 : 0 ) : 0 )
#define EXIT_LEV(ch) (_exitlevel(ch))

bool race_has_tail(int r_id);
void char_lose_tail(char_data *ch);
bool char_has_tail(char_data *ch);
void char_gain_tail(char_data *ch, bool announce);

room_vnum sensei_start_room(int s_id);
room_vnum sensei_location_id(int s_id);
int sensei_grav_tolerance(int s_id);
int sensei_rpp_cost(int s_id, int r_id);

int race_get_size(int r_id);
int race_is_playable(int r_id);
int race_is_people(int r_id);