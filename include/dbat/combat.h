/* ************************************************************************
*  File: combat.h                    Part of Dragonball Advent Truth      *
*  Usage: Combat utilities and common functions for act.offensive.c and   *
*  act.attack.c                                                           *
*                                                                         *
*  All rights reserved to Iovan that are not due to anyone else.          *
*                                                                         *
*  This file was first written on 2011 and aside for a few instances only *
*  contains code written by Iovan for use with the Real Dragonball Battle *
*  System (RDBS) of the MUD Dragonball Advent Truth.                      *
************************************************************************ */
#pragma once

#include "structs.h"

/* combat.c functions */
extern void homing_update(uint64_t heartPulse, double deltaTime);

extern void damage_weapon(BaseCharacter *ch, Object *obj, BaseCharacter *vict);

extern int64_t advanced_energy(BaseCharacter *ch, int64_t dmg);

extern int roll_accuracy(BaseCharacter *ch, int skill, bool kiatt);

extern void handle_death_msg(BaseCharacter *ch, BaseCharacter *vict, int type);

extern int handle_combo(BaseCharacter *ch, BaseCharacter *vict);

extern int handle_parry(BaseCharacter *ch);

extern void handle_spiral(BaseCharacter *ch, BaseCharacter *vict, int skill, int first);

extern void handle_cooldown(BaseCharacter *ch, int cooldown);

extern void
parry_ki(double attperc, BaseCharacter *ch, BaseCharacter *vict, char sname[1000], int prob, int perc, int skill,
         int type);

extern void dodge_ki(BaseCharacter *ch, BaseCharacter *vict, int type, int type2, int skill, int skill2);

extern void damage_eq(BaseCharacter *vict, int location);

extern void remove_limb(BaseCharacter *vict, int num);

extern int check_skill(BaseCharacter *ch, int skill);

extern bool check_points(BaseCharacter *ch, int64_t ki, int64_t st);

extern void pcost(BaseCharacter *ch, double ki, int64_t st);

extern void
hurt(int limb, int chance, BaseCharacter *ch, BaseCharacter *vict, Object *obj, int64_t dmg, int type);

extern int64_t damtype(BaseCharacter *ch, int type, int skill, double percent);

extern int can_kill(BaseCharacter *ch, BaseCharacter *vict, Object *obj, int num);

extern void huge_update(uint64_t heartPulse, double deltaTime);

extern int init_skill(BaseCharacter *ch, int snum);

extern bool can_grav(BaseCharacter *ch);

extern int limb_ok(BaseCharacter *ch, int type);

extern int check_def(BaseCharacter *vict);

extern void dam_eq_loc(BaseCharacter *vict, int area);

extern void hurt_limb(BaseCharacter *ch, BaseCharacter *vict, int chance, int area, int64_t power);

extern int handle_speed(BaseCharacter *ch, BaseCharacter *vict);

extern void handle_defense(BaseCharacter *vict, int *pry, int *blk, int *dge);

extern void spar_gain(BaseCharacter *ch, BaseCharacter *vict, int type, int64_t dmg);

extern int chance_to_hit(BaseCharacter *ch);

extern int roll_hitloc(BaseCharacter *ch, BaseCharacter *vict, int skill);

long double calc_critical(BaseCharacter *ch, int loc);

extern int backstab(BaseCharacter *ch, BaseCharacter *vict, int wlvl, int64_t dmg);

extern int64_t gun_dam(BaseCharacter *ch, int wlvl);

extern int physical_mastery(BaseCharacter *ch);

extern int count_physical(BaseCharacter *ch);

extern int handle_dodge(BaseCharacter *ch);

extern int handle_block(BaseCharacter *ch);

extern void cut_limb(BaseCharacter *ch, BaseCharacter *vict, int wlvl, int hitspot);

extern void club_stamina(BaseCharacter *ch, BaseCharacter *vict, int wlvl, int64_t dmg);

extern int boom_headshot(BaseCharacter *ch);

extern void handle_knockdown(BaseCharacter *ch);

extern int roll_balance(BaseCharacter *ch);

extern int64_t combo_damage(BaseCharacter *ch, int64_t damage, int type);

extern int check_ruby(BaseCharacter *ch);

extern void combine_attacks(BaseCharacter *ch, BaseCharacter *vict);

extern void handle_disarm(BaseCharacter *ch, BaseCharacter *vict);

extern int handle_defender(BaseCharacter *vict, BaseCharacter *ch);

extern void handle_multihit(BaseCharacter *ch, BaseCharacter *vict);

extern int64_t armor_calc(BaseCharacter *ch, int64_t dmg, int type);
