#pragma once

/*
 * Functions shared between the DG Scripts source files in src/dg.
 * Not part of the public interface (that is dbat/dg_scripts.hpp).
 */
#include "dbat/dg_scripts.hpp"

/* dg_handler.c */
struct script_data **script_slot(owner_ptr go);

/* dg_scripts.c */
/* " [trig ... ]" describing the running trigger, or "" when none is */
std::string dg_context(GameWorld *g);

/* dg_lookup.c */
room_data *find_room(GameWorld *g, long n);

/* dg_eval.c */
std::string eval_expr(GameWorld *g, const char *line, owner_ptr go, struct script_data *sc, trig_data *trig);
bool process_if(GameWorld *g, const char *cond, owner_ptr go, struct script_data *sc, trig_data *trig);
struct cmdlist_element *find_end(GameWorld *g, trig_data *trig, struct cmdlist_element *cl);
struct cmdlist_element *find_else_end(GameWorld *g, trig_data *trig, struct cmdlist_element *cl, owner_ptr go,
                                      struct script_data *sc);
struct cmdlist_element *find_case(GameWorld *g, struct trig_data *trig, struct cmdlist_element *cl, owner_ptr go,
                                  struct script_data *sc, const char *cond);
struct cmdlist_element *find_done(struct cmdlist_element *cl);

/* dg_wizcmd.c */
bool remove_trigger(GameWorld *g, struct script_data *sc, char *name);

