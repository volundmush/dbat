#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"
#include "spells.h"

// global variables
extern struct char_data *combat_list;
extern struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES];

// functions
void death_cry(struct char_data *ch);
int group_bonus(struct char_data *ch, int type);
void die(struct char_data *ch, struct char_data *killer);
void remove_limb(struct char_data *vict, int num);
void impact_sound(struct char_data *ch, const char *mssg);
void fight_stack(void);
void appear(struct char_data *ch);
void raw_kill(struct char_data *ch, struct char_data *killer);
void set_fighting(struct char_data *ch, struct char_data *victim);
void stop_fighting(struct char_data *ch);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
void mutant_limb_regen(struct char_data *ch);

// commands
