#pragma once
/*
 * The fix-ups the loaders make to what the world's files say: each is a rule with a name, called from its kind's
 * reader as the record loads.
 */
#include <string>

#include "dbat/consts/types.hpp"

struct GameWorld;
struct extra_descr_data;
struct char_data;
struct obj_data;

namespace rules {

/* Rooms flagged Vegeta or gravity x10 have 10 times gravity */
void gravity_by_flags(GameWorld *g, room_vnum room);
/* The gravity-training rooms have the gravity of their range of vnums, whatever their flags say */
void gravity_by_vnum(GameWorld *g, room_vnum room);
/* An extra description ends in a line end, as the editors' formatting assumes (Welcor 09/03) */
void extra_description_ends_a_line(extra_descr_data &descr);

/* A short description that starts with "a", "an" or "the" starts in lowercase, so that it reads well mid-sentence */
void article_in_lowercase(std::string &text);

/* A mob's class is one there is, or a commoner's */
void mob_class_in_range(char_data *mob, mob_vnum vnum);
/* Every mob but a human sees in the dark */
void infravision_unless_human(char_data *mob);
/* A mob flagged to autobalance starts with no experience, armor or damage dice, which autobalancing gives it */
void autobalance_starts_bare(char_data *mob);
/* MOB_NOTDEADYET marks a mob being extracted, so no prototype has it */
void not_dead_yet_cleared(char_data *mob, mob_vnum vnum);

/* A portal or hatch with no lock or hide DC has 20 for both */
void portal_and_hatch_dcs(obj_data *obj);
/* Weapons with CWG-SunTzu's weapon types (170-182) get today's */
void suntzu_weapon_types(obj_data *obj);
/* A drink container or fountain weighs more than what it holds */
void drink_container_weight(obj_data *obj);
/* A portal never times out */
void portal_timer(obj_data *obj);
/* No object charms who wears it */
void object_charm_cleared(obj_data *obj, obj_vnum vnum);

} // namespace rules
