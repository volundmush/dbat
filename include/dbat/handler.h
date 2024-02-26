/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once

#include "structs.h"

/* handling the affected-structures */
extern void update_char_objects(Character *ch);    /* handler.c */
extern void item_check(Object *object, Character *ch);

extern void affect_total(Character *ch);

extern void affect_modify(Character *ch, int loc, int mod, int spec, long bitv, bool add);

extern void affect_to_char(Character *ch, struct affected_type *af);

extern void affect_remove(Character *ch, struct affected_type *af);

extern void affect_from_char(Character *ch, int type);

extern bool affected_by_spell(Character *ch, int type);

extern bool affectedv_by_spell(Character *ch, int type);

extern void affect_join(Character *ch, struct affected_type *af,
                        bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);

extern void affectv_join(Character *ch, struct affected_type *af,
                         bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);

extern void affectv_remove(Character *ch, struct affected_type *af);

extern void affectv_to_char(Character *ch, struct affected_type *af);

extern void affectv_from_char(Character *ch, int type);


/* utility */
const char *money_desc(int amount);

Object *create_money(int amount);

extern int isname(const char *str, const char *namelist);

extern int is_name(const char *str, const char *namelist);

extern char *fname(const char *namelist);

extern int get_number(char **name);

const char *get_i_name(Character *ch, Character *vict);

/* ******** objects *********** */

extern void equip_char(Character *ch, Object *obj, int pos);

Object *unequip_char(Character *ch, int pos);

extern int invalid_align(Character *ch, Object *obj);

extern void extract_obj(Object *obj);

/* ******* characters ********* */

Character *get_char_room(char *name, int *num, room_rnum room);

Character *get_char_num(mob_rnum nr);

extern void extract_char(Character *ch);

extern void extract_char_final(Character *ch);

extern void extract_pending_chars(uint64_t heartBeat, double deltaTime);

/* find if character can see */
Character *get_player_vis(Character *ch, char *name, int *number, int inroom);

Character *get_char_vis(Character *ch, char *name, int *number, int where);

Character *get_char_room_vis(Character *ch, char *name, int *number);

Character *get_char_world_vis(Character *ch, char *name, int *number);

Object *get_obj_num(obj_rnum nr);

Object *get_obj_in_list_vis(Character *ch, char *name, int *number, std::vector<Object*> list);

Object *get_obj_vis(Character *ch, char *name, int *num);

Object *get_obj_in_equip_vis(Character *ch, char *arg, int *number, std::map<int, Object*> equipment);

extern int get_obj_pos_in_equip_vis(Character *ch, char *arg, int *num, std::map<int, Object*> equipment);

extern int find_eq_pos(Character *ch, Object *obj, char *arg);


/* find all dots */

extern int find_all_dots(char *arg);

#define FIND_INDIV    0
#define FIND_ALL    1
#define FIND_ALLDOT    2


/* Generic Find */

extern int generic_find(char *arg, bitvector_t bitvector, Character *ch,
                        Character **tar_ch, Object **tar_obj);

#define FIND_CHAR_ROOM     (1 << 0)
#define FIND_CHAR_WORLD    (1 << 1)
#define FIND_OBJ_INV       (1 << 2)
#define FIND_OBJ_ROOM      (1 << 3)
#define FIND_OBJ_WORLD     (1 << 4)
#define FIND_OBJ_EQUIP     (1 << 5)
