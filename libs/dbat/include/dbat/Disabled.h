#pragma once
#include <cstdint>

typedef struct disabled_data DISABLED_DATA;

extern DISABLED_DATA *disabled_first;

struct disabled_data {
    DISABLED_DATA *next;                /* pointer to next node          */
    struct command_info const *command; /* pointer to the command struct */
    char *disabled_by;                  /* name of disabler              */
    int16_t level;                       /* level of disabler             */
    int subcmd;                         /* the subcmd, if any            */
};

extern void load_disabled();

extern void save_disabled();

extern void free_disabled();