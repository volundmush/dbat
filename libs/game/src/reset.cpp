/************************************************************************* 
*  File: reset.c                             Designed for CircleMUD 3.5  * 
*  Usage: implementation of pre-zone resets and post-zone resets         * 
*         functions called before and after a normal zone reset          * 
*                                                                        * 
*  All rights reserved.                                                  * 
*                                                                        * 
*  Copyright (C) 2007 Stefan Cole (a.k.a. Jamdog)                        * 
*  To see this in action, check out AderonMUD                            * 
************************************************************************ */ 

#include "dbat/game/utils.h" 
#include "dbat/game/db.h"
#include "dbat/game/handler.h"
#include "dbat/game/reset.h" 
#include "dbat/game/spec_procs.h"


/* pre_reset is called before a zone is reset - returns TRUE to prevent a normal reset of the zone */ 
bool pre_reset(struct zone_data *zone) 
{ 
  /* By default, a normal zone reset follows this function */ 
  bool ret_value = PERFORM_NORMAL_RESET; 

  switch(zone->number) 
  { 
    /* Gauntlet zone reset type determined by players in the zone */ 
    case RESET_GAUNTLET:      ret_value = prereset_gauntlet_zone(); 
                              break; 

    default: /* No special zone reset to perform for this zone number */ 
             ret_value = PERFORM_NORMAL_RESET; 
             break; 
  } 
  return ret_value; 
} 


/* post_reset is called after a normal zone reset */ 
void post_reset(struct zone_data *zone) 
{ 
  switch(zone->number) 
  { 
    default: 
      break; 
  } 
} 

bool prereset_gauntlet_zone(void) 
{
  return PERFORM_NORMAL_RESET;
}
