#pragma once

/*
 * Functions shared between the DG Scripts source files in src/dg.
 * Not part of the public interface (that is dbat/dg_scripts.hpp).
 */
#include "dbat/dg_scripts.hpp"

/* dg_handler.c */
struct script_data **script_slot(void *go, int type);

/* dg_scripts.c */
void dg_context_str(GameWorld *g, char *buf, size_t len);

/* dg_lookup.c */
room_data *find_room(GameWorld *g, long n);

/* dg_eval.c */
std::string eval_expr(GameWorld *g, const char *line, void *go, struct script_data *sc, trig_data *trig, int type);
bool process_if(GameWorld *g, const char *cond, void *go, struct script_data *sc,
                trig_data *trig, int type);
struct cmdlist_element *find_end(GameWorld *g, trig_data *trig, struct cmdlist_element *cl);
struct cmdlist_element *find_else_end(GameWorld *g, trig_data *trig,
                                      struct cmdlist_element *cl, void *go,
                                      struct script_data *sc, int type);
struct cmdlist_element *
find_case(GameWorld *g, struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, const char *cond);
struct cmdlist_element *find_done(struct cmdlist_element *cl);

/* dg_wizcmd.c */
bool remove_trigger(GameWorld *g, struct script_data *sc, char *name);

