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

extern void damage_weapon(Character *ch, Object *obj, Character *vict);

extern int64_t advanced_energy(Character *ch, int64_t dmg);

extern int roll_accuracy(Character *ch, int skill, bool kiatt);

extern void handle_death_msg(Character *ch, Character *vict, int type);

extern int handle_combo(Character *ch, Character *vict);

extern int handle_parry(Character *ch);

extern void handle_spiral(Character *ch, Character *vict, int skill, int first);

extern void handle_cooldown(Character *ch, int cooldown);

extern void
parry_ki(double attperc, Character *ch, Character *vict, char sname[1000], int prob, int perc, int skill,
         int type);

extern void dodge_ki(Character *ch, Character *vict, int type, int type2, int skill, int skill2);

extern void damage_eq(Character *vict, int location);

extern void remove_limb(Character *vict, int num);

extern int check_skill(Character *ch, int skill);

extern bool check_points(Character *ch, int64_t ki, int64_t st);

extern void pcost(Character *ch, double ki, int64_t st);

extern void
hurt(int limb, int chance, Character *ch, Character *vict, Object *obj, int64_t dmg, int type);

extern int64_t damtype(Character *ch, int type, int skill, double percent);

extern int can_kill(Character *ch, Character *vict, Object *obj, int num);

extern void huge_update(uint64_t heartPulse, double deltaTime);

extern int init_skill(Character *ch, int snum);

extern bool can_grav(Character *ch);

extern int limb_ok(Character *ch, int type);

extern int check_def(Character *vict);

extern void dam_eq_loc(Character *vict, int area);

extern void hurt_limb(Character *ch, Character *vict, int chance, int area, int64_t power);

extern int handle_speed(Character *ch, Character *vict);

extern void handle_defense(Character *vict, int *pry, int *blk, int *dge);

extern void spar_gain(Character *ch, Character *vict, int type, int64_t dmg);

extern int chance_to_hit(Character *ch);

extern int roll_hitloc(Character *ch, Character *vict, int skill);

long double calc_critical(Character *ch, int loc);

extern int backstab(Character *ch, Character *vict, int wlvl, int64_t dmg);

extern int64_t gun_dam(Character *ch, int wlvl);

extern int physical_mastery(Character *ch);

extern int count_physical(Character *ch);

extern int handle_dodge(Character *ch);

extern int handle_block(Character *ch);

extern void cut_limb(Character *ch, Character *vict, int wlvl, int hitspot);

extern void club_stamina(Character *ch, Character *vict, int wlvl, int64_t dmg);

extern int boom_headshot(Character *ch);

extern void handle_knockdown(Character *ch);

extern int roll_balance(Character *ch);

extern int64_t combo_damage(Character *ch, int64_t damage, int type);

extern int check_ruby(Character *ch);

extern void combine_attacks(Character *ch, Character *vict);

extern void handle_disarm(Character *ch, Character *vict);

extern int handle_defender(Character *vict, Character *ch);

extern void handle_multihit(Character *ch, Character *vict);

extern int64_t armor_calc(Character *ch, int64_t dmg, int type);
