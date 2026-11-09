#pragma once
#include "dbat/consts/types.hpp"
#include "dbat/descriptor.hpp"

int desc_get_connected(const struct descriptor_data *d);
void desc_set_connected(struct descriptor_data *d, int v);

bool desc_is_playing(const struct descriptor_data *d);

void customCreate(struct descriptor_data *d);
void customRead(GameWorld *g, struct descriptor_data *d, int type, const char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);

