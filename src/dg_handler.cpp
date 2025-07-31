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

#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"


/* frees memory associated with var */
void free_var_el(struct trig_var_data *var) {
    if (var->name)
        free(var->name);
    if (var->value)
        free(var->value);
    free(var);
}

/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd) {
    struct trig_var_data *i, *j;

    for (i = vd; i;) {
        j = i;
        i = i->next;
        free_var_el(j);
    }
}

/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
int remove_var(struct trig_var_data **var_list, char *name) {
    struct trig_var_data *i, *j;

    for (j = nullptr, i = *var_list; i && strcasecmp(name, i->name);
         j = i, i = i->next);

    if (i) {
        if (j) {
            j->next = i->next;
            free_var_el(i);
        } else {
            *var_list = i->next;
            free_var_el(i);
        }

        return 1;
    }

    return 0;
}



trig_data::~trig_data() {
    free(name);
    name = nullptr;
    if (arglist) {
        free(arglist);
        arglist = nullptr;
    }
}


/* remove all triggers from a mob/obj/room */
void extract_script(unit_data *thing, int type) {
    
    for(auto &[vn, sc] : thing->scripts) {
        sc->deactivate();
    }

    thing->variables.clear();

}



