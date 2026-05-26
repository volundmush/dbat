#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void lua_repl_launch(struct descriptor_data *d);
void lua_repl_close(struct descriptor_data *d);
void lua_repl_parse(struct descriptor_data *d, const char *arg);

#ifdef __cplusplus
}
#endif
