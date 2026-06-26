#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void handle_multi_merge(struct char_data *form);
/* disp_rpp_store, handle_rpp_store, rpp_feature moved to lua/characters/pcommands/misc/rpp.lua */
/* ash_burn moved to lua/objects/scripts/ashcloud.lua */

// commands
ACMD(do_restring);
/* do_tailhide moved to lua/characters/commands/misc/tailhide.lua */
/* do_nogrow moved to lua/characters/commands/misc/nogrow.lua */
/* do_spoil moved to lua/characters/commands/misc/spoil.lua */
/* do_feed moved to lua/characters/commands/misc/feed.lua */
/* do_beacon moved to lua/characters/commands/misc/beacon.lua */
/* do_dimizu moved to lua/characters/commands/misc/dimizu.lua */
/* do_obstruct moved to lua/characters/commands/misc/obstruct.lua */
/* do_warppool moved to lua/characters/commands/misc/warppool.lua */
/* do_fireshield moved to lua/characters/commands/misc/fireshield.lua */
/* do_cook moved to lua/characters/commands/misc/cook.lua */
/* do_adrenaline moved to lua/characters/commands/misc/adrenaline.lua */
/* do_ensnare moved to lua/characters/commands/misc/ensnare.lua */
/* do_arena moved to lua/characters/commands/misc/arena.lua */
/* do_bury moved to lua/characters/commands/misc/bury.lua */
/* do_hayasa moved to lua/characters/commands/misc/hayasa.lua */
/* do_instill moved to lua/characters/commands/misc/instill.lua */
/* do_channel moved to lua/characters/commands/misc/channel.lua */
/* do_shimmer moved to lua/characters/commands/misc/shimmer.lua */
/* do_metamorph moved to lua/characters/commands/misc/metamorph.lua */
/* do_healglow moved to lua/characters/commands/misc/healglow.lua */
/* do_resize moved to lua/characters/commands/misc/resize.lua */
/* do_scry moved to lua/characters/commands/misc/scry.lua */
/* do_runic moved to lua/characters/commands/misc/runic.lua */
/* do_extract moved to lua/characters/commands/misc/extract.lua */
/* do_fish moved to lua/characters/commands/misc/fish.lua */
/* do_defend moved to lua/characters/commands/misc/defend.lua */
/* do_lifeforce moved to lua/characters/pcommands/info/lifeforce.lua */
/* do_liquefy → lua/characters/commands/misc/liquefy.lua */
/* do_shell moved to lua/characters/commands/misc/shell.lua */
/* do_moondust moved to lua/characters/commands/misc/moondust.lua */
/* do_preference moved to lua/characters/commands/misc/preference.lua */
/* do_song moved to lua/characters/commands/misc/song.lua */
/* do_multiform moved to lua/characters/commands/misc/multiform.lua */
/* do_spiritcontrol moved to lua/characters/commands/misc/spiritcontrol.lua */
/* do_ashcloud moved to lua/characters/commands/misc/ashcloud.lua */
/* do_silk moved to lua/characters/commands/misc/silk.lua */

#ifdef __cplusplus
}
#endif
