#pragma once
#include <string_view>
#include "dbat/consts/types.hpp"

/* these factors should be unique integers */
constexpr int RENT_FACTOR = 1;
constexpr int CRYO_FACTOR = 4;

constexpr int LOC_INVENTORY = 0;
#define MAX_BAG_ROWS 5

// functions
void Crash_rentsave(GameWorld *g, struct char_data *ch, int cost);
void update_obj_file(GameWorld *g);
void delete_inv_backup(struct char_data *ch);
bool Crash_delete_file(std::string_view name);
bool Crash_delete_crashfile(struct char_data *ch);
void Crash_listrent(GameWorld *g, struct char_data *ch, std::string_view name);
int Crash_load(GameWorld *g, struct char_data *ch);
void Crash_crashsave(GameWorld *g, struct char_data *ch);
void Crash_save_all(GameWorld *g);

