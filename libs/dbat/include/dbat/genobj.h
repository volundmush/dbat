/************************************************************************
 * Generic OLC Library - Objects / genobj.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once
#include "Typedefs.h"
struct Character;
struct Object;
struct ObjectPrototype;

extern void auto_equip(Character *ch, Object *obj, int location);

extern int update_objects(ObjectPrototype *refobj);

extern obj_rnum add_object(ObjectPrototype *, obj_vnum ovnum);

extern int delete_object(obj_rnum);
