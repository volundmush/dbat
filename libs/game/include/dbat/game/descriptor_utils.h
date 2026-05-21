#pragma once
#include "dbat/db/descriptor.h"
#include "dbat/db/descriptor_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void customCreate(struct descriptor_data *d);
void customRead(struct descriptor_data *d, int type, char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);

#ifdef __cplusplus
}
#endif
