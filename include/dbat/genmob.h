/************************************************************************
 * Generic OLC Library - Mobiles / genmob.h			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once

#include "structs.h"

extern int free_mobile_strings(Character *mob);

extern int save_mobiles(zone_rnum rznum);

/* Handy macros. */
#define GET_NDD(mob)    ((mob)->mob_specials.damnodice)
#define GET_SDD(mob)    ((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob)    ((mob)->getName().c_str())
#define GET_SDESC(mob)    ((mob)->getShortDesc().c_str())
#define GET_LDESC(mob)    ((mob)->getRoomDesc().c_str())
#define GET_DDESC(mob)    ((mob)->getLookDesc().c_str())
#define GET_ATTACK(mob)    ((mob)->mob_specials.attack_type)
