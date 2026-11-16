#pragma once

/*
 * Functions shared between the DG Scripts source files in src/dg.
 * Not part of the public interface (that is dbat/dg_scripts.hpp).
 */
#include "dbat/dg_scripts.hpp"

/* dg_handler.c */
struct script_data **script_slot(owner_ptr go);

/* dg_scripts.c */
/* What the run does after a command line */
enum class command_result {
  next, /* goes on to the next line */
  halt, /* ends, as at the end of the script */
  stop, /* returns at once, keeping its place: it waits, or its owner was purged */
};

/* A command line being run, after substitution */
struct command_call {
  GameWorld *g;
  owner_ptr go;
  struct script_data *sc;
  trig_data *trig;
  char *cmd;
  size_t at; /* its line, in trig->run.program */
  int &ret_val;
  bool &owner_gone;
};

/* Runs a DG command, or else the owner's own */
command_result run_command(command_call &c);
/* Has trig's run carry on when pulses from now */
void schedule_wait(GameWorld *g, owner_ptr go, trig_data *trig, long when);

/* errors.cpp */
/* " [trig ... ]" describing the running trigger, or "" when none is */
std::string dg_context(GameWorld *g);
/* Starts g->dg_ctx describing trig's run on go */
void dg_ctx_begin(GameWorld *g, owner_ptr go, trig_data *trig);
/* The line the run is at, for the context */
void dg_ctx_set_line(GameWorld *g, const dg::line &l);

/* dg_lookup.c */
room_data *find_room(GameWorld *g, long n);

/* dg_eval.c */
std::string eval_expr(GameWorld *g, const char *line, owner_ptr go, struct script_data *sc, trig_data *trig);
bool process_if(GameWorld *g, const char *cond, owner_ptr go, struct script_data *sc, trig_data *trig);
/* Whether a case label's value, as written, matches the switch's subject */
bool case_matches(const std::string &subject, const char *value);

/* dg_wizcmd.c */
bool remove_trigger(GameWorld *g, struct script_data *sc, char *name);

