#pragma once

#include "structs.h"

// defines
constexpr int MAX_PORTAL_TYPES = 6;
/* For show_obj_to_char 'mode'.	/-- arbitrary */
constexpr int SHOW_OBJ_LONG = 0;
constexpr int SHOW_OBJ_SHORT = 1;
constexpr int SHOW_OBJ_ACTION = 2;

constexpr int HIST_LENGTH = 100;

// global variables
extern int *cmd_sort_info;
extern char *default_color_choices[NUM_COLOR + 1];

// functions
extern int readIntro(Character *ch, Character *vict);

extern int check_disabled(const struct command_info *command);

extern void sort_commands();
extern void do_auto_exits(const Location& loc, Character *ch, int exit_mode);
extern void do_auto_exits2(const Location& loc, Character *ch);

extern char *find_exdesc(char *word, const std::vector<ExtraDescription> &list);
extern char *find_exdesc(char *word, struct extra_descr_data *list);

extern void add_history(Character *ch, char *str, int type);

extern void introWrite(Character *ch, Character *vict, char *name);

extern void list_one_char(Character *i, Character *ch);
extern void show_obj_to_char(Object *obj, Character *ch, int mode);

extern void look_at_room(Room *rm, Character *ch, int ignore_brief);
extern void look_at_room(room_rnum target_room, Character *ch, int ignore_brief);

extern int perf_skill(int skill);

extern int search_help(const char *argument, int level);

// commands
