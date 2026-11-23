#pragma once
#include <span>
#include <string_view>

#include "dbat/consts/types.hpp"

/*
 * What a connection that isn't playing does with a line of input: the handler for the state it's in. The states are
 * the CON_* numbers, set wherever a connection moves on, so a state's handler is found by its number. nanny() looks
 * them up in turn: the editors' (each of which context help may answer first), the login menus', then character
 * creation's.
 */
struct con_state {
  int state{};
  void (*parse)(GameWorld *g, struct descriptor_data *d, std::string_view arg){}; /* arg: the line, its leading spaces gone */
  void (*prompt)(GameWorld *g, struct descriptor_data *d){};           /* what's shown on entering it, if always the same */
  bool olc{};                                                           /* an editor's */
};

/* Runs a line of input from a connection that isn't playing, by its state's handler. */
void nanny(GameWorld *g, struct descriptor_data *d, std::string_view line);

/* Moves d to the state, and shows its prompt if it has one. */
void enter_state(GameWorld *g, struct descriptor_data *d, int state);
