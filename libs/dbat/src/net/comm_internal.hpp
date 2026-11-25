#pragma once

/*
 * What the networking's files share, which nothing else needs. How they fit together:
 *
 *   game_loop.cpp   init_game(), and the loop: one pass polls the listener and every connection (net::poll_set),
 *                   accepts, reads, runs a command for each, sends, closes the finished, then runs the heartbeat
 *                   table for each pulse due, and sleeps until the next (pulse_clock)
 *   connection.cpp  a connection's start (game_add_connection(): full, too many from one site, banned) and end
 *                   (close_socket()), and the idle time-outs
 *   input.cpp       raw bytes to typed lines (process_input()), and the line rules: !, ^old^new, --, flooding
 *   output.cpp      a connection's pending text (write_to_output(), output_buffer) and sending it with the prompt
 *                   (process_output())
 *   colour.cpp      @ codes to ANSI (colourize())
 *   prompt.cpp      the prompt; messages.cpp: act() and the send_to_*() broadcasts
 *   signals.cpp     what the signals and the deadlock timer do; copyover.cpp: the exec, and coming back from it
 *   platform_posix.cpp, platform_win32.cpp
 *                   everything that talks to the system (dbat/net/platform.hpp); nothing else here does
 *
 * nanny.cpp, chargen.cpp and accounts.cpp, the login and character creation, have nanny_internal.hpp.
 */
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
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
colourized colourize(GameWorld *g, std::string_view text, bool parse, std::span<const std::string> choices, size_t maxlen);

/* prompt.cpp */
std::string make_prompt(GameWorld *g, struct descriptor_data *d);

/* copyover.cpp */
void copyover_recover(GameWorld *g);

/* signals.cpp */
void signal_setup();
extern int tics_passed;                                /* passes of the game loop, for the deadlock timer */
extern net::process::signal_flags signals_received; /* what the signals have asked for */
