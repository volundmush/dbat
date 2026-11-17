#pragma once
/*
 * The loaders still in db.cpp that read through a FILE, for the boot table in world/boot.cpp. Each goes as its kind
 * moves onto io::text_reader: help (F6). mob_stats() is game logic the mob loader calls, until it moves to the
 * mobs' code (F9).
 */
#include <cstdio>

struct GameWorld;
struct help_index_element;
struct char_data;

/* An enhanced (E) mob's abilities, rolled at random by its level and race as it loads */
void mob_stats(GameWorld *g, char_data *mob);

void load_help(GameWorld *g, std::FILE *fl, const char *name);
int count_alias_records(std::FILE *fl);
bool help_keyword_order(const help_index_element &a, const help_index_element &b);
