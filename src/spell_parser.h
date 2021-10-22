//
// Created by basti on 10/21/2021.
//

#ifndef CIRCLE_SPELL_PARSER_H
#define CIRCLE_SPELL_PARSER_H

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"

#define SINFO spell_info[spellnum]
extern const char *unused_spellname;

// Commands
ACMD(do_cast);

#endif //CIRCLE_SPELL_PARSER_H
