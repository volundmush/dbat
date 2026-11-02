#pragma once
/* Shared by nanny.c, chargen.c and accounts.c only. */
#include "dbat/consts/types.h"

struct nanny_state {
  int state;
  void (*handler)(struct descriptor_data *d, char *arg);
};

/* chargen.c */
bool chargen_parse(struct descriptor_data *d, char *arg);
void display_races(struct descriptor_data *d);

/* accounts.c */
constexpr int MAX_ACCOUNT_SLOTS = 5;  /* character slots per account: d->tmp1 .. d->tmp5 */
constexpr int MAX_EMAIL_LENGTH  = 64; /* as typed; stored with each @ as <AT>, read back into 100 bytes */
char **account_slot(struct descriptor_data *d, int n);
void set_account_slot(struct descriptor_data *d, int n, const char *name);
void empty_account_slots_named(struct descriptor_data *d, const char *name);
bool lockRead(const char *name);
void userCreate(struct descriptor_data *d);
void userDelete(struct descriptor_data *d);
void userRead(struct descriptor_data *d);
