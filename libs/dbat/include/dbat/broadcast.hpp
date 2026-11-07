#pragma once
#include "dbat/consts/types.hpp"

// functions
void send_to_imm(GameWorld *g, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void fly_zone(GameWorld *g, zone_rnum zone, const char *messg, struct char_data *ch);
void send_to_scouter(GameWorld *g, const char *messg, struct char_data *ch, int num, int type);
void send_to_sense(GameWorld *g, int type, const char *messg, struct char_data *ch);
void send_to_worlds(GameWorld *g, struct char_data *ch);

