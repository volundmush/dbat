#pragma once
#include "dbat/consts/types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* these factors should be unique integers */
constexpr int RENT_FACTOR = 1;
constexpr int CRYO_FACTOR = 4;

constexpr int LOC_INVENTORY = 0;
#define MAX_BAG_ROWS 5

// functions
void Crash_rentsave(GameWorld *g, struct char_data *ch, int cost);
bool Obj_to_store(GameWorld *g, struct obj_data *obj, FILE *fl, int location);
void update_obj_file(GameWorld *g);
void delete_inv_backup(struct char_data *ch);
bool Crash_delete_file(const char *name);
bool Crash_delete_crashfile(struct char_data *ch);
void Crash_listrent(GameWorld *g, struct char_data *ch, char *name);
int Crash_load(GameWorld *g, struct char_data *ch);
void Crash_crashsave(GameWorld *g, struct char_data *ch);
void Crash_save_all(GameWorld *g);

#ifdef __cplusplus
}
#endif
