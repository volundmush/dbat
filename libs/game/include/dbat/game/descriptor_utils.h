#pragma once
#include "dbat/db/descriptor.h"
#include "dbat/db/descriptor_utils.h"

void customCreate(struct descriptor_data *d);
void customRead(struct descriptor_data *d, int type, char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);