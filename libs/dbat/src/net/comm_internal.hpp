#pragma once

/* Shared by the files that were comm.c only: connection, input, output, colour, prompt, the game loop,
 * signals and copyover. */
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/net/platform.hpp"

#define COLOR_ON(g, ch) (COLOR_LEV(g, ch) > 0)

/* connection.cpp */
void check_idle_menu(GameWorld *g);
void check_idle_passwords(GameWorld *g);
int get_max_players(GameWorld *g);
void init_descriptor(GameWorld *g, struct descriptor_data *newd, net::socket sock);
/* Accepts the next connection waiting on the game's listener */
int new_descriptor(GameWorld *g);
void set_empty_account(struct descriptor_data *d);

/* input.cpp */
void flush_input_queue(struct descriptor_data *d);
/* The next line queued, if there is one */
std::optional<txt_line> get_from_q(struct txt_q *queue);
int process_input(GameWorld *g, struct descriptor_data *t);

/* output.cpp */
/* Empties out, back to the small size (returning a large buffer to the pool) */
void reset_output(GameWorld *g, struct output_buffer &out);
int process_output(GameWorld *g, struct descriptor_data *t);

/* colour.cpp */
/* text with its colour codes made ANSI escapes (or taken out, unless parse), as much as fits in maxlen - 1, and how
   long it would have been with room for all of it */
struct colourized {
  std::string text;
  size_t wanted;
};
colourized colourize(GameWorld *g, std::string_view text, bool parse, const std::string *choices, size_t maxlen);

/* prompt.cpp */
std::string make_prompt(GameWorld *g, struct descriptor_data *d);

/* copyover.cpp */
void copyover_recover(GameWorld *g);

/* signals.cpp */
void signal_setup(void);
extern int tics_passed;                       /* for extern checkpointing */
extern volatile sig_atomic_t reread_wizlist;  /* signal: SIGUSR1 */
extern volatile sig_atomic_t emergency_unban; /* signal: SIGUSR2 */
extern volatile sig_atomic_t shutdown_signal; /* signal: SIGHUP, SIGINT or SIGTERM (its number) */
