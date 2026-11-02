#pragma once
/*
 * Functions shared between the DG Scripts source files in src/dg.
 * Not part of the public interface (that is dbat/dg_scripts.h).
 */
#include "dbat/dg_scripts.h"

/* dg_handler.c */
struct script_data **script_slot(void *go, int type);

/* dg_scripts.c */
void dg_context_str(char *buf, size_t len);

/* dg_lookup.c */
obj_data *find_obj(long n);
room_data *find_room(long n);

/* dg_eval.c */
void eval_expr(char *line, char *result, void *go, struct script_data *sc,
               trig_data *trig, int type);
bool process_if(char *cond, void *go, struct script_data *sc,
                trig_data *trig, int type);
struct cmdlist_element *find_end(trig_data *trig, struct cmdlist_element *cl);
struct cmdlist_element *find_else_end(trig_data *trig,
                                      struct cmdlist_element *cl, void *go,
                                      struct script_data *sc, int type);
struct cmdlist_element *
find_case(struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, char *cond);
struct cmdlist_element *find_done(struct cmdlist_element *cl);

/* dg_wizcmd.c */
bool remove_trigger(struct script_data *sc, char *name);
