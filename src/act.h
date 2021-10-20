//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_ACT_H
#define CIRCLE_ACT_H

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "clan.h"
#include "combat.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "boards.h"

/* act.attack.c */

// Commands
ACMD(do_get);
ACMD(do_spike);
ACMD(do_selfd);
ACMD(do_spiral);
ACMD(do_breaker);
ACMD(do_throw);
ACMD(do_razor);
ACMD(do_koteiru);
ACMD(do_hspiral);
ACMD(do_seishou);
ACMD(do_bash);
ACMD(do_head);
ACMD(do_nova);
ACMD(do_malice);
ACMD(do_zen);
ACMD(do_sunder);
ACMD(do_combine);
ACMD(do_energize);
ACMD(do_lightgrenade);
ACMD(do_strike);
ACMD(do_ram);
ACMD(do_breath);


/* act.comm.c */

// commands
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_respond);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);


#endif //CIRCLE_ACT_H
