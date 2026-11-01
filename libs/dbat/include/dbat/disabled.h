#pragma once
#include <stdint.h>

typedef struct disabled_data DISABLED_DATA;

/* one disabled command */
struct disabled_data {
  DISABLED_DATA *next;                /* pointer to next node          */
  struct command_info const *command; /* pointer to the command struct */
  char *disabled_by;                  /* name of disabler              */
  int16_t level;                      /* level of disabler             */
  int subcmd;                         /* the subcmd, if any            */
};

#include "dbat/consts/types.h"
ACMD(do_disable);
int check_disabled(const struct command_info *command);
