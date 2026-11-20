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

#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/resource.h>
#include <execinfo.h>
#include <sys/time.h>

#include "dbat/iterate.hpp"
#include "dbat/comm.hpp"

#include "comm_internal.hpp"

int tics_passed           = 0;         /* for extern checkpointing */
volatile sig_atomic_t reread_wizlist;  /* signal: SIGUSR1 */
volatile sig_atomic_t emergency_unban; /* signal: SIGUSR2 */
volatile sig_atomic_t shutdown_signal; /* signal: SIGHUP, SIGINT or SIGTERM (its number) */

/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

static void reread_wizlists(int sig)
{
  reread_wizlist = true;
}

static void unrestrict_game(int sig)
{
  emergency_unban = true;
}

/* clean up our zombie kids to avoid defunct processes */
static void reap(int sig)
{
  while (waitpid(-1, nullptr, WNOHANG) > 0);

  signal(SIGCHLD, reap);
}

/* Dying anyway... */
static void checkpointing(int sig)
{
#ifndef MEMORY_DEBUG
  if (!tics_passed) {
    mud_log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    abort();
  } else
    tics_passed = 0;
#endif
}

/* A fatal signal (crash, abort, or the deadlock checkpoint) is being delivered: write which one and a
 * backtrace to the log using only async-signal-safe calls, then re-raise it with the default action so
 * the process still dies and dumps core as before. */
static void fatal_signal(int sig)
{
  char msg[48] = "SYSERR: fatal signal ";
  size_t len   = strlen(msg);
  char digits[12];
  int nd = 0;

  for (int v = sig; v > 0 && nd < (int)sizeof(digits); v /= 10)
    digits[nd++] = '0' + v % 10;
  while (nd > 0 && len < sizeof(msg) - 2)
    msg[len++] = digits[--nd];
  msg[len++] = '\n';

  int fd = logfile ? fileno(logfile) : STDERR_FILENO;
  if (write(fd, msg, len) < 0) { /* nothing else we can do */
  }
  log_backtrace();

  signal(sig, SIG_DFL);
  raise(sig);
}

/* SIGHUP, SIGINT, SIGTERM: ask the game loop to shut down cleanly (saving everyone). Only sets a flag:
 * logging or exiting from inside a signal handler is not safe. */
static void hupsig(int sig)
{
  shutdown_signal = sig;
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

void signal_setup(void)
{
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec   = 180;
  interval.tv_usec  = 0;
  itime.it_interval = interval;
  itime.it_value    = interval;
  setitimer(ITIMER_VIRTUAL, &itime, nullptr);
  signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  signal(SIGHUP, hupsig);
  signal(SIGCHLD, reap);
  signal(SIGINT, hupsig);
  signal(SIGTERM, hupsig);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);

  /* crashes: log a backtrace, then die (and dump core) as before */
  {
    void *warm[1];
    backtrace(warm, 1); /* load the unwinder now, so the handler doesn't have to allocate */
  }
  signal(SIGSEGV, fatal_signal);
  signal(SIGBUS, fatal_signal);
  signal(SIGFPE, fatal_signal);
  signal(SIGILL, fatal_signal);
  signal(SIGABRT, fatal_signal);

  /* allow core dumps up to the hard limit */
  struct rlimit core;
  if (getrlimit(RLIMIT_CORE, &core) == 0) {
    core.rlim_cur = core.rlim_max;
    setrlimit(RLIMIT_CORE, &core);
  }
}
