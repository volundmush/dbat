#pragma once
#include "consts/aligns.h"
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// variables

// functions
void log_imm_action(char *messg, ...);
int dball_count(struct char_data *ch);
void log_custom(struct descriptor_data *d, struct obj_data *obj);
void wishSYS(void);
void bring_to_cap(struct char_data *ch);
void char_bring_to_cap(struct char_data *ch);
void char_rp_save(struct char_data *ch);
void char_rpp_custom_equip_launch(struct char_data *ch);
void char_rpp_restring_launch(struct char_data *ch, struct obj_data *obj);
void base_update(void);
void load_shadow_dragons();

// commands
ACMD(do_skills);
ACMD(do_gen_comm);
ACMD(do_wear);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_hide);
// ACMD(do_steal);  // lua/characters/commands/misc/steal.lua
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
// ACMD(do_group);    // lua/characters/commands/misc/group.lua
// ACMD(do_ungroup);  // lua/characters/commands/misc/ungroup.lua
// ACMD(do_report);  // lua/characters/commands/misc/report.lua
// ACMD(do_split);   // lua/characters/commands/misc/split.lua
ACMD(do_use);
ACMD(do_value);
// ACMD(do_display);  // lua/characters/pcommands/misc/display.lua
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_file);
ACMD(do_scribe);

// ACMD(do_scouter);  // lua/characters/commands/misc/scouter.lua
ACMD(do_snet);
// ACMD(do_spar);  // lua/characters/commands/misc/spar.lua
ACMD(do_pushup);
ACMD(do_situp);
ACMD(do_summon);
ACMD(do_eavesdrop);
ACMD(do_disguise);
ACMD(do_appraise);
ACMD(do_forgery);
ACMD(do_plant);
ACMD(do_kaioken);
ACMD(do_focus);
ACMD(do_regenerate);
ACMD(do_absorb);
ACMD(do_ingest);
ACMD(do_upgrade);
ACMD(do_srepair);
ACMD(do_recharge);
/* do_form/create moved to lua/characters/commands/misc/create.lua */
// ACMD(do_spit);  // lua/characters/commands/misc/spit.lua
// ACMD(do_majinize);  // lua/characters/commands/misc/majinize.lua
// ACMD(do_potential);  // lua/characters/commands/misc/potential.lua
ACMD(do_telepathy);
// ACMD(do_fury);  // lua/characters/commands/misc/fury.lua
// ACMD(do_pose);  // lua/characters/commands/misc/pose.lua
// ACMD(do_hass);  // lua/characters/commands/misc/hass.lua
ACMD(do_implant);
// ACMD(do_hass);  // lua/characters/commands/misc/hass.lua (duplicate decl)
// ACMD(do_suppress);  // lua/characters/commands/misc/suppress.lua
// ACMD(do_drag);  // lua/characters/commands/misc/drag.lua
// ACMD(do_stop);  // lua/characters/commands/misc/stop.lua
// ACMD(do_future);  // lua/characters/commands/misc/future.lua
// ACMD(do_candy);  // lua/characters/commands/misc/candy.lua
// ACMD(do_kura);   // lua/characters/commands/misc/kura.lua
// ACMD(do_taisha);    // lua/characters/commands/misc/taisha.lua
// ACMD(do_paralyze);  // lua/characters/commands/misc/paralyze.lua
// ACMD(do_rip);  // lua/characters/commands/misc/rip.lua
// ACMD(do_train);  // lua/characters/commands/misc/train.lua
ACMD(do_trip);
ACMD(do_grapple);
ACMD(do_willpower);
ACMD(do_commune);
/* do_rpp moved to lua/characters/pcommands/misc/rpp.lua */
ACMD(do_meditate);
// ACMD(do_aura);  // lua/characters/commands/misc/aura.lua
ACMD(do_think);
ACMD(do_visible);
ACMD(do_compare);
// ACMD(do_compare);  // lua/characters/pcommands/misc/compare.lua
// ACMD(do_break);    // lua/characters/commands/misc/break.lua
// ACMD(do_fix);      // lua/characters/commands/misc/fix.lua
// ACMD(do_aid);      // lua/characters/commands/misc/aid.lua

#ifdef __cplusplus
}
#endif
