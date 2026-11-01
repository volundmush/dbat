/*************************************************************************
*   File: olc.c                                         Part of CircleMUD *
*  Usage: online creation                                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * PLEASE, FOR THE LOVE OF GOD, DON'T TRY TO USE THIS YET!!!
 *  *** DO *** NOT *** SEND ME MAIL ASKING WHY IT DOESN'T WORK -- IT'S
 *  NOT DONE!!
 */
#include "dbat/config.h"
#include "dbat/olc.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"

/* OLC command format:
 *
 * olc {"." | {<"room"|"mobile"|"object"> <number>}} <arguments>
 * olc {"set"|"show"} <attribute> <arguments>
 */

#define OLC_USAGE "Usage: olc { . | set | show | obj | mob | room} [args]\r\n"

/* local globals */

/* local functions */

/* The actual do_olc command for the interpreter.  Determines the target
   entity, checks permissions, and passes control to olc_interpreter */

/* OLC interpreter command; called by do_olc */

/* can_modify: determine if a particular char can modify a vnum */

/* generic fn for modifying a string */

/* generic fn for modifying a bitvector */