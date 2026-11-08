#pragma once
#include "dbat/consts/types.hpp"

void extract_char(GameWorld *g, struct char_data *ch);
void extract_char_final(GameWorld *g, struct char_data *ch);
/* Frees what was extracted since the last call: characters, then objects. Call it between pulses, when nothing can
 * hold a pointer to them. */
void finish_extractions(GameWorld *g);
/* Takes obj, and what it contains, out of the game at once; they are freed by finish_extractions(). */
void extract_obj(GameWorld *g, struct obj_data *obj);
/* Frees the extracted objects made from prototype vnum now, or all of them for NOTHING: for code about to change or
 * delete a prototype whose strings they share. */
void free_extracted_objects(GameWorld *g, obj_vnum vnum);
