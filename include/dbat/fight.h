//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"
#include "spells.h"

// global variables
extern struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES];

// functions
extern void death_cry(Character *ch);

extern int group_bonus(Character *ch, int type);

extern void die(Character *ch, Character *killer);

extern void remove_limb(Character *vict, int num);

extern void impact_sound(Character *ch, const char *mssg);

extern void fight_stack(uint64_t heartPulse, double deltaTime);

extern void powerupService(uint64_t heartPulse, double deltaTime);

extern void lifeforceSystem(uint64_t heartPulse, double deltaTime);

extern void kiChargeSystem(uint64_t heartPulse, double deltaTime);

extern void appear(Character *ch);

extern void raw_kill(Character *ch, Character *killer);

extern void set_fighting(Character *ch, Character *victim);

extern void stop_fighting(Character *ch);

extern void group_gain(Character *ch, Character *victim);

extern void solo_gain(Character *ch, Character *victim);

extern void mutant_limb_regen(Character *ch);

// commands

