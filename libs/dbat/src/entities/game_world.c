#include "dbat/game_world.h"
#include "dbat/utils.h"

GameWorld *game_world_create(void)
{
  GameWorld *g;

  CREATE(g, GameWorld, 1);
  g->top_shop       = -1;
  g->top_guild      = -1;
  g->top_of_socialt = -1;
  g->TOPCOUNTDOWN   = 60;
  g->exp_multiplier = 1.0;
  g->max_mob_id     = MOB_ID_BASE;
  g->max_obj_id     = OBJ_ID_BASE;
  g->SHADOW_DRAGON1 = -1;
  g->SHADOW_DRAGON2 = -1;
  g->SHADOW_DRAGON3 = -1;
  g->SHADOW_DRAGON4 = -1;
  g->SHADOW_DRAGON5 = -1;
  g->SHADOW_DRAGON6 = -1;
  g->SHADOW_DRAGON7 = -1;
  return g;
}

void game_world_destroy(GameWorld *g)
{
  free(g);
}
