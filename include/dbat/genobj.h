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

extern int update_objects(struct obj_data *refobj);

extern int delete_object(obj_rnum);
