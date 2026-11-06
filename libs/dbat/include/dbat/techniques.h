#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool tech_handle_android_absorb(GameWorld *g, char_data *ch, char_data *vict);
bool tech_handle_zanzoken(GameWorld *g, char_data *ch, char_data *vict, const char *name);
void tech_handle_posmodifier(char_data *vict, int *pry, int *blk, int *dge, int *prob);
void tech_handle_fireshield(GameWorld *g, char_data *ch, char_data *vict, const char *part);
bool tech_handle_targeting(GameWorld *g, char_data *ch, char *arg, char_data **vict, obj_data **obj);
bool tech_handle_charge(GameWorld *g, char_data *ch, char *arg, double minimum, double *attperc);
void tech_handle_crashdown(GameWorld *g, char_data *ch, char_data *vict);

#ifdef __cplusplus
}
#endif
