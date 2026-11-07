#pragma once
#include <cstdint>

#include "dbat/consts/types.hpp"

typedef struct disabled_data DISABLED_DATA;

/* one disabled command */
struct disabled_data {
  DISABLED_DATA *next;                /* pointer to next node          */
  struct command_info const *command; /* pointer to the command struct */
  char *disabled_by;                  /* name of disabler              */
  int16_t level;                      /* level of disabler             */
  int subcmd;                         /* the subcmd, if any            */
};

ACMD(do_disable);
bool check_disabled(GameWorld *g, const struct command_info *command);

