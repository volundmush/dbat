/**************************************************************************
*  File: dg_olc.hpp                                                         *
*                                                                         *
*  Usage: this source file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#pragma once
#include "dbat/dg/owner.hpp"
#include <cstdio>

#include "dbat/consts/types.hpp"

constexpr int NUM_TRIG_TYPE_FLAGS = 20;

/* prototype exported functions from dg_olc.c */
bool save_triggers(GameWorld *g, struct zone_data *zone);
bool format_script(GameWorld *g, struct descriptor_data *d);

ACMD(do_oasis_trigedit);

