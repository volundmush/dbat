/************************************************************************
 * Generic OLC Library - General / genolc.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#ifndef __GENOLC_H__
#define __GENOLC_H__

#define __GENOLC_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "shop.h"
#include "oasis.h"
#include "genolc.h"
#include "genwld.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genobj.h"
#include "dg_olc.h"
#include "constants.h"
#include "guild.h"
#include "interpreter.h"

#define STRING_TERMINATOR       '~'

#define CONFIG_GENOLC_MOBPROG	0

/* from modify.c */
void smash_tilde(char *str);
int genolc_checkstring(struct descriptor_data *d, char *arg);

int remove_from_save_list(zone_vnum, int type);
int add_to_save_list(zone_vnum, int type);
int in_save_list(zone_vnum, int type);
void strip_cr(char *);
int save_all(void);
char *str_udup(const char *);
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);
void free_ex_descriptions(struct extra_descr_data *head);
int sprintascii(char *out, bitvector_t bits);

struct save_list_data {
  int zone;
  int type;
  struct save_list_data *next;
};

extern struct save_list_data *save_list;

/* save_list_data.type */
#define SL_MOB	0
#define SL_OBJ	1
#define SL_SHP	2
#define SL_WLD	3
#define SL_ZON	4
#define SL_CFG	5
#define SL_GLD	6
#define SL_MAX	SL_GLD
#define SL_ACT  SL_MAX + 1 /* must be above MAX */
#define SL_HLP  SL_MAX + 2 /* must be above MAX */

#define ZCMD(zon, cmds)	zone_table[(zon)].cmd[(cmds)]

#define LIMIT(var, low, high)	MIN(high, MAX(var, low))

room_vnum genolc_zone_bottom(zone_rnum rznum);
room_vnum genolc_zonep_bottom(struct zone_data *zone);
void create_world_index(int znum, const char *type);

#endif