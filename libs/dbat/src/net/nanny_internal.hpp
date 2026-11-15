#pragma once

/* Shared by nanny.c, chargen.c and accounts.c only. */
#include <span>
#include <string>
#include "dbat/con_states.hpp"
#include "dbat/consts/types.hpp"

/* chargen.c */
extern const std::span<const con_state> CHARGEN_STATES; /* character creation's states, for nanny() */

/* accounts.c */
constexpr int MAX_ACCOUNT_SLOTS = 5;  /* character slots per account: d->tmp1 .. d->tmp5 */
constexpr int MAX_EMAIL_LENGTH  = 64; /* as typed; stored with each @ as <AT>, read back into 100 bytes */
std::string *account_slot(struct descriptor_data *d, int n);
void set_account_slot(struct descriptor_data *d, int n, const char *name);
void empty_account_slots_named(struct descriptor_data *d, const char *name);
bool lockRead(const char *name);
void userCreate(struct descriptor_data *d);
void userDelete(GameWorld *g, struct descriptor_data *d);
void userRead(GameWorld *g, struct descriptor_data *d);

