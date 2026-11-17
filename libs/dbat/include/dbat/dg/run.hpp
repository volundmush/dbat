#pragma once
/*
 * Where a trigger instance's run is in its program: the blocks it is inside, innermost last, each with the next node
 * to run in it. A wait keeps them, and the run carries on from there when the wait is over, so a script can pause at
 * any line, in any block.
 */
#include <memory>
#include <vector>

#include "dbat/dg/program.hpp"

namespace dg {

/* What an open block is */
enum class block_role {
  script, /* the script's own body */
  branch, /* an if's branch */
  loop,   /* a while's body: at its end, the condition is tested again */
  cases,  /* a switch's body, entered at a label */
};

struct frame {
  block_role role{};
  const block *body{};
  size_t next{};       /* the body's next node to run */
  const node *of{};    /* the if, while or switch; null for the script */
};

struct run_state {
  std::shared_ptr<const program> prog{}; /* what the frames point into, kept whatever happens to the trigger */
  std::vector<frame> frames{};              /* none when not running */
  int passes{};                             /* loop passes since the run started, for the 5,000 limit */

  bool running() const { return !frames.empty(); }
  void stop()
  {
    frames.clear();
    prog.reset();
  }
  /* The line the run carries on at, or null at the end of the script */
  const line *next_line() const;
  /* The line that opened f's block: its if, elseif, else, while or switch, or null for the script's own body */
  const line *opened_at(const frame &f) const;
};

} // namespace dg
