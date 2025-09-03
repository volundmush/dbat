/************************************************************************
 * Generic OLC Library - Zones / genzon.h			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once
#include <vector>
#include "Typedefs.h"

struct Zone;

extern zone_rnum create_new_zone(zone_vnum vzone_num, const char **error);

extern std::vector<Zone*> getZoneChildren(zone_vnum parent = NOTHING);