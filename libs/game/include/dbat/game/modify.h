#pragma once
#include "dbat/db/consts/types.h"


#ifdef __cplusplus
extern "C" {
#endif

// functions
void paginate_string(char *str, struct descriptor_data *d);
void smash_tilde(char *str);
void show_string(struct descriptor_data *d, char *input);

// commands
ACMD(do_skillset);


#ifdef __cplusplus
}
#endif
