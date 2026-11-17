#pragma once
/*
 * Loading the world's files: the zones, triggers, rooms, mobs, objects, shops and guilds that each kind's index file
 * lists. Each kind's reader sits beside its writer, in src/db/world/<kind>.cpp.
 */
#include <functional>

#include "dbat/io/reader.hpp"

struct GameWorld;

/* Loads every file of one kind (DB_BOOT_*) that its index lists, logging how many records there are as it goes */
io::loaded<void> boot_index(GameWorld *g, int mode);

/* One zone file; the zone is added to g->zones */
io::loaded<void> read_zone_file(GameWorld *g, io::text_reader &in);

/* One room file; its rooms are added to g->rooms */
io::loaded<void> read_room_file(GameWorld *g, io::text_reader &in);

/* One mob file; its mobs are added to g->mob_index and g->mob_proto */
io::loaded<void> read_mob_file(GameWorld *g, io::text_reader &in);

/* One object file; its objects are added to g->obj_index and g->obj_proto */
io::loaded<void> read_object_file(GameWorld *g, io::text_reader &in);

/* One shop file; its shops are added to g->shops */
io::loaded<void> read_shop_file(GameWorld *g, io::text_reader &in);

/* One guild file; its guilds are added to g->guilds */
io::loaded<void> read_guild_file(GameWorld *g, io::text_reader &in);

/* One trigger file; its triggers are added to g->trig_index */
io::loaded<void> read_trigger_file(GameWorld *g, io::text_reader &in);

/* Reads the "#<vnum>" records of a file up to its "$" (or a #99999), calling read for each, with the reader just
   past the "#<vnum>" line. label names the kind in errors, as "trg". */
io::loaded<void> read_records(io::text_reader &in, const char *label,
                              const std::function<io::loaded<void>(io::text_reader &, int)> &read);
