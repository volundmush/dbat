//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_DG_COMM_H
#define CIRCLE_DG_COMM_H

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "db.h"
#include "constants.h"
#include "feats.h"

// functions
void send_to_imm(char *messg, ...);
void fly_zone(zone_rnum zone, char *messg, struct char_data *ch);
void send_to_scouter(char *messg, struct char_data *ch, int num, int type);
void send_to_sense(int type, char *messg, struct char_data *ch);


#endif //CIRCLE_DG_COMM_H
