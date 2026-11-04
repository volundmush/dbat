#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"
#include "spells.h"

// global variables
extern const struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES];

// functions
void death_cry(GameWorld *g, struct char_data *ch);
int group_bonus(GameWorld *g, struct char_data *ch, int type);
void die(GameWorld *g, struct char_data *ch, struct char_data *killer);
void remove_limb(GameWorld *g, struct char_data *vict, int num);
void impact_sound(GameWorld *g, struct char_data *ch, const char *mssg);
void fight_stack(GameWorld *g);
void appear(GameWorld *g, struct char_data *ch);
void raw_kill(GameWorld *g, struct char_data *ch, struct char_data *killer);
void set_fighting(GameWorld *g, struct char_data *ch, struct char_data *victim);
void stop_fighting(GameWorld *g, struct char_data *ch);
void group_gain(GameWorld *g, struct char_data *ch, struct char_data *victim);
void solo_gain(GameWorld *g, struct char_data *ch, struct char_data *victim);
void mutant_limb_regen(GameWorld *g, struct char_data *ch);

// commands
