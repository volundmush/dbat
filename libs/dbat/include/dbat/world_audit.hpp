#pragma once
#include <functional>
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"

/*
 * Checks that the game's lists agree with each other: that everything is where it says it is,
 * and that everything anyone points at still exists. Changes nothing. Calls report for each
 * problem, up to a limit, and returns how many problems it found.
 */
int world_audit(GameWorld *g, const std::function<void(std::string_view problem)> &report);

/* The same, reporting each problem to a function given context and the problem as a C string, as the tests do */
template <class F> int world_audit(GameWorld *g, F *report, void *context)
{
  return world_audit(g, [=](std::string_view problem) { report(context, std::string(problem).c_str()); });
}
