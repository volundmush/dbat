#pragma once
#include "consts/aligns.h"
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// variables

// functions

int dball_count(struct char_data *ch);

void wishSYS(void);
void bring_to_cap(struct char_data *ch);
void char_bring_to_cap(struct char_data *ch);
void char_rp_save(struct char_data *ch);
void char_rpp_custom_equip_launch(struct char_data *ch);
void char_rpp_restring_launch(struct char_data *ch, struct obj_data *obj);
void load_shadow_dragons();

// commands
ACMD(do_skills);
ACMD(do_gen_comm);
ACMD(do_wear);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_hide);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
ACMD(do_use);
ACMD(do_gen_write);
ACMD(do_file);
ACMD(do_scribe);
ACMD(do_snet);
ACMD(do_summon);
ACMD(do_visible);
ACMD(do_compare);

#ifdef __cplusplus
}
#endif
