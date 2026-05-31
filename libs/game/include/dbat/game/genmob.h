#pragma once
#include "dbat/db/consts/types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int delete_mobile(mob_vnum vnum);
int copy_mobile(struct char_data *to, struct char_data *from);
int copy_mobile_from_proto(struct char_data *to, struct mob_proto_data *from);
int copy_mobile_to_proto(struct mob_proto_data *to, struct char_data *from);
void mob_proto_free(struct mob_proto_data *mob);
void mob_proto_free_script(struct mob_proto_data *mob);
void mob_proto_copy_script_to_mobile(struct mob_proto_data *source, struct char_data *dest);
int add_mobile(struct char_data *, mob_vnum);
int copy_mob_strings(struct char_data *to, struct char_data *from);
int free_mobile_strings(struct char_data *mob);
int mobile_free_editor(struct char_data *mob);
int free_mobile(struct char_data *mob);
int save_mobiles(struct zone_data *zone);
int write_mobile_record(mob_vnum mvnum, struct mob_proto_data *mob, FILE *fd);
int copy_mobile_strings(struct char_data *t, struct char_data *f);

/* Handy macros. */
#define GET_ALIAS(mob)	((mob)->name)
#define GET_SDESC(mob)	((mob)->short_descr)
#define GET_LDESC(mob)	((mob)->long_descr)
#define GET_DDESC(mob)	((mob)->description)


#ifdef __cplusplus
}
#endif
