/*************************************************************************
*   File: signals.cpp                                   Part of CircleMUD *
*  Usage: Signal handling                                                *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <string_view>

#include "dbat/log.hpp"

#include "comm_internal.hpp"

int tics_passed = 0; /* for extern checkpointing */
net::process::signal_flags signals_received;

/* The deadlock timer ran out: unless the game loop has finished a pass since last time, it is stuck. Dying anyway... */
static void checkpointing()
{
#ifndef MEMORY_DEBUG
  if (!tics_passed) {
    mud_log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    std::abort();
  } else
    tics_passed = 0;
#endif
}

/* A fatal signal (crash, abort, or the deadlock checkpoint) is being delivered: write which one and a backtrace to
 * the log using only async-signal-safe calls. The process then dies of it, dumping core, as before. */
static void crash_report(int sig)
{
  constexpr std::string_view says = "SYSERR: fatal signal ";
  std::array<char, 48> msg;
  char *end = std::ranges::copy(says, msg.begin()).out;

  end    = std::to_chars(end, msg.end() - 1, sig).ptr;
  *end++ = '\n';
  net::process::write_raw(log_descriptor(), std::string_view(msg.data(), end));
  log_backtrace();
}

void signal_setup()
{
  net::process::install_signals(signals_received, crash_report);
  /* the MUD aborts itself if it gets caught in an infinite loop for more than 3 minutes */
  net::process::start_deadlock_timer(180, checkpointing);
}
