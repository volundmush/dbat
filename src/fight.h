#pragma once
#include "consts/maximums.h"
#include "consts/types.h"
#include "spells.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables
extern struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES];

// functions
void death_cry(struct char_data *ch);
int group_bonus(struct char_data *ch, int type);
void die(struct char_data *ch, struct char_data *killer);
void remove_limb(struct char_data *vict, int num);
void impact_sound(struct char_data *ch, char *mssg);
void fight_stack(void);
void appear(struct char_data *ch);
void raw_kill(struct char_data *ch, struct char_data *killer);
void set_fighting(struct char_data *ch, struct char_data *victim);
void stop_fighting(struct char_data *ch);
void group_gain(struct char_data *ch, struct char_data *victim);
void mutant_limb_regen(struct char_data *ch);

// commands
// ACMD(do_trip);  // lua/characters/commands/misc/trip.lua
// ACMD(do_srepair);  // lua/characters/commands/misc/repair.lua
// ACMD(do_absorb);  // lua/characters/commands/misc/absorb.lua
// ACMD(do_kaioken);  // lua/characters/commands/misc/kaioken.lua
ACMD(do_hass);
ACMD(do_ensnare);
ACMD(do_get);
ACMD(do_split);
ACMD(do_sac);
// ACMD(do_kaioken);  // lua/characters/commands/misc/kaioken.lua

#ifdef __cplusplus
}
#endif
