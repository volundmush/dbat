//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_FIGHT_H
#define CIRCLE_FIGHT_H

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"
#include "combat.h"

// functions
void death_cry(struct char_data *ch);
int group_bonus(struct char_data *ch, int type);

#endif //CIRCLE_FIGHT_H
