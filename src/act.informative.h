#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// defines
#define MAX_PORTAL_TYPES 6
/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG 0
#define SHOW_OBJ_SHORT 1
#define SHOW_OBJ_ACTION 2

#define HIST_LENGTH 100

// global variables
extern int *cmd_sort_info;

// functions
int readIntro(struct char_data *ch, struct char_data *vict);
void introCreate(struct char_data *ch);
int check_disabled(const struct command_info *command);
void sort_commands(void);
char *find_exdesc(char *word, struct extra_descr_data *list);
void add_history(struct char_data *ch, char *str, int type);
void introWrite(struct char_data *ch, struct char_data *vict, char *name);
void look_at_room(struct room_data *target_room, struct char_data *ch,
                  int ignore_brief);
int perf_skill(int skill);
int search_help(const char *argument, int level);

// commands
ACMD(do_look);
ACMD(do_examine);
/* do_status moved to lua/characters/pcommands/info/status.lua */
/* do_inventory moved to lua/characters/pcommands/info/inventory.lua */
/* do_equipment moved to lua/characters/pcommands/info/equipment.lua */
/* do_time moved to lua/characters/pcommands/info/time.lua */
/* do_weather moved to lua/characters/pcommands/info/weather.lua */
/* do_levels moved to lua/characters/pcommands/info/levels.lua */
/* do_consider moved to lua/characters/pcommands/info/consider.lua */
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
ACMD(do_commands);
ACMD(do_exits);
ACMD(do_autoexit);
ACMD(do_history);
ACMD(do_map);
ACMD(do_rptrans);
ACMD(do_finger);
ACMD(do_perf);
ACMD(do_nickname);
/* do_table moved to lua/characters/commands/cardgame/table.lua */
/* do_play moved to lua/characters/commands/cardgame/play.lua */
ACMD(do_post);
/* do_hand moved to lua/characters/commands/cardgame/hand.lua */
/* do_shuffle moved to lua/characters/commands/cardgame/shuffle.lua */
/* do_draw moved to lua/characters/commands/cardgame/draw.lua */
/* do_kyodaika moved to lua/characters/commands/namek/kyodaika.lua */
/* do_mimic moved to lua/characters/commands/misc/mimic.lua */
ACMD(do_rdisplay);
/* do_evolve moved to lua/characters/commands/advancement/evolve.lua */
/* do_showoff moved to lua/characters/commands/items/show.lua */
ACMD(do_intro);
ACMD(do_scan);
ACMD(do_toplist);
ACMD(do_whois);

#ifdef __cplusplus
}
#endif
