#pragma once
#include "dbat/db/consts/types.h"

ACMD(do_alias);

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
void write_aliases(struct char_data *ch);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);
