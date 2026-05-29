#pragma once
#include "dbat/db/consts/types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int delete_mobile(mob_vnum vnum);
int copy_mobile(struct char_data *to, struct char_data *from);
int add_mobile(struct char_data *, mob_vnum);
int copy_mob_strings(struct char_data *to, struct char_data *from);
int free_mobile_strings(struct char_data *mob);
int free_mobile(struct char_data *mob);
int save_mobiles(struct zone_data *zone);
int write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd);
int copy_mobile_strings(struct char_data *t, struct char_data *f);

/* Handy macros. */
#define GET_NDD(mob)	((mob)->mob_specials.damnodice)
#define GET_SDD(mob)	((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob)	((mob)->name)
#define GET_SDESC(mob)	((mob)->short_descr)
#define GET_LDESC(mob)	((mob)->long_descr)
#define GET_DDESC(mob)	((mob)->description)
#define GET_ATTACK(mob)	((mob)->mob_specials.attack_type)


#ifdef __cplusplus
}
#endif
