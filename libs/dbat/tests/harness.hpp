#pragma once

/* Runs a whole game inside a test: no sockets, no real data/ files. */
#include <ctime>
#include <string>

#include "dbat/consts/types.hpp"

/* Copies the fixture world to a temp directory, enters it, and boots a game there. */
GameWorld *th_boot(void);

/* The same, with the game's clock pinned to when from the start, so that the boot repeats exactly. */
GameWorld *th_boot_at(time_t when);

/* Where th_boot() copies its world from, instead of the fixture. The directory is only ever read. */
void th_use_world(const char *directory);

/* Frees the game and removes the temp directory. */
void th_shutdown(GameWorld *g);

/* Called once by the next boot, after copying the world and before reading any of it. */
void th_before_boot(void (*hook)(void));

/* Gets rid of the temp directory after a game that can't be freed, as it never finished booting. */
void th_abandon(void);

/* Frees the game, leaving the temp directory, from which th_start_again() can boot another. */
void th_stop(GameWorld *g);
GameWorld *th_start_again(time_t when);

/* Logs a new player in, the way character creation and the menus do. */
struct char_data *th_player(GameWorld *g, const char *name, int admlevel);

/* The same, with a chosen race and sensei. */
struct char_data *th_player_as(GameWorld *g, const char *name, int admlevel, int race, int sensei);

/*
 * Connects the way a real client does, returning the connection and setting *peer to the client's end
 * of its socket. Everything on it goes through the game's real input and output code. Returns nullptr
 * if the game refuses the connection.
 */
struct descriptor_data *th_connect(GameWorld *g, int *peer);

/* Sends bytes down a real connection and runs them. Returns false if the game closed the connection. */
bool th_send(GameWorld *g, struct descriptor_data *d, int peer, const char *bytes, size_t length);

/* Logs in a copy of a player who has a save file, the way the menus do. Returns nullptr if they can't load. */
struct char_data *th_existing_player(GameWorld *g, const char *name);

/* Queues a line of input for ch and runs it, as if it had just been typed. */
void th_type(GameWorld *g, struct char_data *ch, const char *line);

/* The same, for whoever is at this connection, whatever state it's in. */
void th_type_at(GameWorld *g, struct descriptor_data *d, const char *line);

/* Everything sent to ch since the last call to th_take_output. */
const char *th_output(struct char_data *ch);
void th_take_output(GameWorld *g, struct char_data *ch);
void th_take_output_at(GameWorld *g, struct descriptor_data *d);

/* Ends a forked child at once, keeping its coverage counts, which _exit() alone would lose. */
[[noreturn]] void th_exit(int status);

/* Runs n pulses of the game. */
void th_pulses(GameWorld *g, int n);

/* The whole of a file, or nothing if it can't be read. */
std::string th_file_text(const char *path);

/*
 * For a transcript of cases, to compare two builds: writes to stdout the generator's state, what each of chars was
 * sent, the files each saves (as a hash of each unless full), and who and what is in the fixture's rooms.
 */
void th_write_case_record(GameWorld *g, struct char_data *const *chars, size_t num_chars, bool full);
