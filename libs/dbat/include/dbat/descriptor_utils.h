#pragma once
#include "dbat/consts/types.h"

#define STATE(d)	((d)->connected)

bool desc_is_playing(const struct descriptor_data *d);

#include "dbat/descriptor.h"
void customCreate(struct descriptor_data *d);
void customRead(struct descriptor_data *d, int type, char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);
