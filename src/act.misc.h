#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void handle_multi_merge(struct char_data *form);
void handle_songs(void);
void disp_rpp_store(struct char_data *ch);
void handle_rpp_store(struct char_data *ch, int choice);
void rpp_feature(struct char_data *ch, const char *arg);
void ash_burn(struct char_data *ch);

// commands
ACMD(do_restring);
/* do_tailhide moved to lua/characters/commands/misc/tailhide.lua */
/* do_nogrow moved to lua/characters/commands/misc/nogrow.lua */
ACMD(do_spoil);
ACMD(do_feed);
ACMD(do_beacon);
/* do_dimizu moved to lua/characters/commands/misc/dimizu.lua */
ACMD(do_obstruct);
ACMD(do_warppool);
ACMD(do_fireshield);
ACMD(do_cook);
/* do_adrenaline moved to lua/characters/commands/misc/adrenaline.lua */
ACMD(do_ensnare);
ACMD(do_arena);
ACMD(do_bury);
/* do_hayasa moved to lua/characters/commands/misc/hayasa.lua */
ACMD(do_instill);
ACMD(do_channel);
ACMD(do_shimmer);
/* do_metamorph moved to lua/characters/commands/misc/metamorph.lua */
/* do_healglow moved to lua/characters/commands/misc/healglow.lua */
ACMD(do_resize);
ACMD(do_scry);
/* do_runic moved to lua/characters/commands/misc/runic.lua */
/* do_extract moved to lua/characters/commands/misc/extract.lua */
/* do_fish moved to lua/characters/commands/misc/fish.lua */
/* do_defend moved to lua/characters/commands/misc/defend.lua */
/* do_lifeforce moved to lua/characters/pcommands/info/lifeforce.lua */
/* do_liquefy → lua/characters/commands/misc/liquefy.lua */
/* do_shell moved to lua/characters/commands/misc/shell.lua */
ACMD(do_moondust);
/* do_preference moved to lua/characters/commands/misc/preference.lua */
/* do_song moved to lua/characters/commands/misc/song.lua */
ACMD(do_multiform);
/* do_spiritcontrol moved to lua/characters/commands/misc/spiritcontrol.lua */
ACMD(do_ashcloud);
ACMD(do_silk);

#ifdef __cplusplus
}
#endif
