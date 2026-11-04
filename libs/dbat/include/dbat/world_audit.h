#pragma once
#include "dbat/consts/types.h"

typedef void world_audit_report(void *context, const char *problem);

/*
 * Checks that the game's lists agree with each other: that everything is where it says it is,
 * and that everything anyone points at still exists. Changes nothing. Calls report for each
 * problem, up to a limit, and returns how many problems it found.
 */
int world_audit(GameWorld *g, world_audit_report *report, void *context);
