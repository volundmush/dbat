#pragma once
#include "dbat/consts/types.h"

int desc_get_connected(const struct descriptor_data *d);
void desc_set_connected(struct descriptor_data *d, int v);

bool desc_is_playing(const struct descriptor_data *d);

#include "dbat/descriptor.h"
void customCreate(struct descriptor_data *d);
void customRead(struct descriptor_data *d, int type, char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);
