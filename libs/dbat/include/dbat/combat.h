#pragma once
#include "dbat/consts/types.h"

/* combat.c functions */
void homing_update(GameWorld *g);
void damage_weapon(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct char_data *vict);
int roll_accuracy(GameWorld *g, struct char_data *ch, int skill, bool kiatt);
int handle_parry(GameWorld *g, struct char_data *ch);
void handle_spiral(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill, int first);
void handle_cooldown(GameWorld *g, struct char_data *ch, int cooldown);
void parry_ki(GameWorld *g, double attperc, struct char_data *ch, struct char_data *vict, const char *sname, int prob, int perc, int skill, int type);
void dodge_ki(GameWorld *g, struct char_data *ch, struct char_data *vict, int type, int type2, int skill, int skill2);

void hurt(GameWorld *g, int limb, int chance, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int64_t dmg, int type);
int64_t damtype(GameWorld *g, struct char_data *ch, int type, int skill, double percent);
void huge_update(GameWorld *g);

int check_def(GameWorld *g, struct char_data *vict);
void dam_eq_loc(GameWorld *g, struct char_data *vict, int area);
void handle_defense(GameWorld *g, struct char_data *vict, int *pry, int *blk, int *dge);

int roll_hitloc(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill);
long double calc_critical(GameWorld *g, struct char_data *ch, int loc);
bool backstab(GameWorld *g, struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg);
int64_t gun_dam(GameWorld *g, struct char_data *ch, int wlvl);
int handle_dodge(GameWorld *g, struct char_data *ch);
void cut_limb(GameWorld *g, struct char_data *ch, struct char_data *vict, int wlvl, int hitspot);
void club_stamina(GameWorld *g, struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg);
bool boom_headshot(GameWorld *g, struct char_data *ch);
void handle_knockdown(GameWorld *g, struct char_data *ch);
int roll_balance(GameWorld *g, struct char_data *ch);
bool check_ruby(GameWorld *g, struct char_data *ch);
void combine_attacks(GameWorld *g, struct char_data *ch, struct char_data *vict);
void handle_disarm(GameWorld *g, struct char_data *ch, struct char_data *vict);
bool handle_defender(GameWorld *g, struct char_data *vict, struct char_data *ch);
void handle_multihit(GameWorld *g, struct char_data *ch, struct char_data *vict);
