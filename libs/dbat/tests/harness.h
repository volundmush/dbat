#pragma once
/* Runs a whole game inside a test: no sockets, no real data/ files. */
#include <time.h>
#include "dbat/consts/types.h"

/* Copies the fixture world to a temp directory, enters it, and boots a game there. */
GameWorld *th_boot(void);

/* The same, with the game's clock pinned to when from the start, so that the boot repeats exactly. */
GameWorld *th_boot_at(time_t when);

/* Frees the game and removes the temp directory. */
void th_shutdown(GameWorld *g);

/* Logs a new player in, the way character creation and the menus do. */
struct char_data *th_player(GameWorld *g, const char *name, int admlevel);

/* Queues a line of input for ch and runs it, as if it had just been typed. */
void th_type(GameWorld *g, struct char_data *ch, const char *line);

/* The same, for whoever is at this connection, whatever state it's in. */
void th_type_at(GameWorld *g, struct descriptor_data *d, const char *line);

/* Everything sent to ch since the last call to th_take_output. */
const char *th_output(struct char_data *ch);
void th_take_output(GameWorld *g, struct char_data *ch);
void th_take_output_at(GameWorld *g, struct descriptor_data *d);

/* Runs n pulses of the game. */
void th_pulses(GameWorld *g, int n);
