/************************************************************************
 * Generic OLC Library - Rooms / genwld.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once

#include "structs.h"

extern room_rnum add_room(Room *);

extern int delete_room(room_rnum);

extern int save_rooms(zone_rnum);

extern int copy_room(Room *to, Room *from);

extern room_rnum duplicate_room(room_vnum to, room_rnum from);

extern int copy_room_strings(Room *dest, Room *source);

extern int free_room_strings(Room *);
