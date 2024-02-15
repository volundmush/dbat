/* ************************************************************************
*  File: dg_db_scripts.c                         Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */

#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/comm.h"
#include "dbat/constants.h"

extern bitvector_t asciiflag_conv(char *flag);

/* local functions */
void trig_data_init(trig_data *this_data);

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
std::shared_ptr<trig_data> read_trigger(int nr) {

    auto proto = trig_index.find(nr);
    if(proto == trig_index.end()) return nullptr;

    auto trig = std::make_shared<trig_data>(proto->second);

    return trig;
}


void trig_data_init(trig_data *this_data) {

}


void trig_data_copy(trig_data *this_data, const trig_data *trg) {

}


