/************************************************************************
 * Generic OLC Library - Objects / genobj.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once

#include "structs.h"
extern void auto_equip(struct char_data *ch, struct obj_data *obj, int location);

extern int save_objects(zone_rnum vznum);

extern int update_objects(struct item_proto_data* refobj);

extern obj_rnum add_object(struct item_proto_data*, obj_vnum ovnum);

extern int delete_object(obj_rnum);
