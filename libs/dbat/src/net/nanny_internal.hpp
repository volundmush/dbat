#pragma once

/* Shared by nanny.c, chargen.c and accounts.c only. */
#include <span>
#include <string>
#include "dbat/con_states.hpp"
#include "dbat/consts/types.hpp"

/* chargen.c */
extern const std::span<const con_state> CHARGEN_STATES; /* character creation's states, for nanny() */

/* accounts.cpp */
constexpr int MAX_EMAIL_LENGTH = 64; /* as typed; stored with each @ as <AT>, read back into 100 bytes */
void empty_account_slots_named(struct descriptor_data *d, const char *name);
void userCreate(struct descriptor_data *d);
void userDelete(GameWorld *g, struct descriptor_data *d);
void userRead(GameWorld *g, struct descriptor_data *d);

