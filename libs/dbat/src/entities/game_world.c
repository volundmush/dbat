#include "dbat/game_world.h"
#include "dbat/act.informative.h"
#include "dbat/act.social.h"
#include "dbat/boards.h"
#include "dbat/comm.h"
#include "dbat/genolc.h"
#include "dbat/mail.h"
#include "dbat/random.h"
#include "dbat/utils.h"

/* Any non-zero value does: a generator seeded with 0 repeats one value forever. */
constexpr unsigned long DEFAULT_RNG_SEED = 12345;

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
  circle_srandom(g, DEFAULT_RNG_SEED);
  return g;
}

void game_world_destroy(GameWorld *g)
{
  free(g);
}

void game_world_free_contents(GameWorld *g)
{
  mud_log("Clearing game world.");
  destroy_db(g);

  if (!g->scheck) {
    mud_log("Clearing other memory.");
    free_bufpool(g);                        /* comm.c */
    free_player_index(g);                   /* players.c */
    clear_free_list(g);                     /* mail.c */
    free_mail_index(g);                     /* mail.c */
    free_text_files(g);                     /* db.c */
    clear_boards(g);                        /* boards.c */
    free(g->cmd_sort_info);                   /* act.informative.c */
    free_command_list(g);                   /* act.informative.c */
    free_social_messages(g);                /* act.social.c */
    free_help_table(g);                     /* db.c */
    Free_Invalid_List(g);                   /* ban.c */
    free_ban_list(g);                       /* ban.c */
    free_strings(g, &g->config_info, OASIS_CFG); /* oasis_delete.c */
    free_disabled(g);                       /* interpreter.c */
    free_save_list(g);                      /* genolc.c */
  }

  if (g->last_act_message)
    free(g->last_act_message);
}

time_t game_now(const GameWorld *g)
{
  return g->pinned_time ? g->pinned_time : time(nullptr);
}
