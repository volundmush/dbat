/************************************************************************
 * Generic OLC Library - Zones / genzon.h			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once

#include "structs.h"
#include "db.h"

extern zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);

extern int new_command(struct Zone *zone, int pos);

extern void delete_zone_command(struct Zone *zone, int pos);

extern zone_rnum real_zone_by_thing(room_vnum vznum);
