#pragma once
#include <stdint.h>

typedef struct disabled_data DISABLED_DATA;

extern DISABLED_DATA *disabled_first; /* interpreter.c */

/* one disabled command */
struct disabled_data {
       DISABLED_DATA *next;                /* pointer to next node          */
       struct command_info const *command; /* pointer to the command struct */
       char *disabled_by;                  /* name of disabler              */
       int16_t level;                       /* level of disabler             */
       int subcmd;                         /* the subcmd, if any            */
};