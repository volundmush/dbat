#pragma once
/*
 * The loaders still in db.cpp that read through a FILE, for the boot table in world/boot.cpp. Each goes as its kind
 * moves onto io::text_reader: rooms (F3), mobs and objects (F4), help (F6).
 */
#include <cstdio>

struct GameWorld;
struct help_index_element;

/* The "#<vnum>" records of a room, mob or object file (DB_BOOT_WLD, _MOB or _OBJ) */
void discrete_load(GameWorld *g, std::FILE *fl, int mode, const char *filename);

void load_help(GameWorld *g, std::FILE *fl, const char *name);
int count_alias_records(std::FILE *fl);
bool help_keyword_order(const help_index_element &a, const help_index_element &b);
