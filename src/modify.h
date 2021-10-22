//
// Created by basti on 10/21/2021.
//

#ifndef CIRCLE_MODIFY_H
#define CIRCLE_MODIFY_H

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "improved-edit.h"
#include "oasis.h"
#include "tedit.h"
#include "shop.h"
#include "guild.h"
#include "spell_parser.h"
#include "dg_olc.h"

// functions
void paginate_string(char *str, struct descriptor_data *d);
void smash_tilde(char *str);
void show_string(struct descriptor_data *d, char *input);

// commands
ACMD(do_skillset);

#endif //CIRCLE_MODIFY_H
