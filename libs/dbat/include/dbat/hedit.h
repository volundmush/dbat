#pragma once
#include "dbat/consts/types.h"

void hedit_parse(struct descriptor_data *, char *);
void hedit_string_cleanup(struct descriptor_data *, int);

ACMD(do_oasis_hedit);
ACMD(do_helpcheck);
ACMD(do_hindex);
