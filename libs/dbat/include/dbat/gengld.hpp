#pragma once
#include "dbat/guild_utils.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/guild.hpp"

void copy_guild(struct guild_data *tguild, struct guild_data *fguild);
void free_guild(struct guild_data *guild);
guild_rnum add_guild(GameWorld *g, struct guild_data *guild);
bool save_guilds(GameWorld *g, zone_rnum zone_num);
guild_rnum real_guild(GameWorld *g, guild_vnum vnum);

/*
 * Handy macros.
 */

#define MSG_TRAINER_NOT_OPEN      "I'm busy! Come back later!"
#define MSG_TRAINER_NO_SEE_CH     writable{"I don't train someone I can't see!"}
#define MSG_TRAINER_DISLIKE_ALIGN "Get out of here before I get angry, you are not aligned with me!"
#define MSG_TRAINER_DISLIKE_CLASS "I won't train those of your discipline!"
#define MSG_TRAINER_DISLIKE_RACE  "Get out of here, I don't help your kind!"
#define MSG_TRAINER_MINLVL        "You are not of a skilled enough level to recieve my training."

constexpr int LEARNED_LEVEL = 0; /* % known which is considered "learned" */
constexpr int MAX_PER_PRAC  = 1; /* max percent gain in skill per practice */
constexpr int MIN_PER_PRAC  = 2; /* min percent gain in skill per practice */
constexpr int PRAC_TYPE     = 3; /* should it say 'spell' or 'skill'?	 */
const char *gedit_modify_string(char **str, char *new_g);

