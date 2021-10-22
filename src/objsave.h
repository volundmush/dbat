//
// Created by basti on 10/21/2021.
//

#ifndef CIRCLE_OBJSAVE_H
#define CIRCLE_OBJSAVE_H

#include "fcntl.h"
#include "unistd.h"
#include "errno.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "act.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5

// functions
void Crash_extract_norent_eq(struct char_data *ch);
void Crash_rentsave(struct char_data *ch, int cost);
int Obj_to_store(struct obj_data *obj, FILE *fl, int location);
void update_obj_file(void);
int cp(struct char_data *ch);
void delete_inv_backup(struct char_data *ch);

#endif //CIRCLE_OBJSAVE_H
