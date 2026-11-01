#pragma once
#include "dbat/consts/types.h"

// functions
void send_to_imm(const char *messg, ...) __attribute__((format(printf, 1, 2)));
void fly_zone(zone_rnum zone, const char *messg, struct char_data *ch);
void send_to_scouter(const char *messg, struct char_data *ch, int num, int type);
void send_to_sense(int type, const char *messg, struct char_data *ch);
void send_to_worlds(struct char_data *ch);
