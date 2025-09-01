/**************************************************************************
*  File: dg_handler.c                                                     *
*                                                                         *
*  Usage: contains functions to handle memory for scripts.                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
***************************************************************************/
#include "dbat/HasDgScripts.h"


/* remove all triggers from a mob/obj/room */
void extract_script(HasDgScripts *thing, UnitType type) {
    
    for(auto &[vn, sc] : thing->scripts) {
        sc->deactivate();
    }

    thing->variables.clear();

}



