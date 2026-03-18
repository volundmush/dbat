#pragma once
#include <filesystem>

#include "dbat/game/Character.hpp"
#include "dbat/game/Object.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/AbstractGridArea.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/TimeInfo.hpp"
#include "dbat/game/Help.hpp"

void load_zones();
//void load_accounts();
void load_dgscript_prototypes();
void load_dgscripts();
void load_globaldata();

void load_shops();
void load_guilds();
void load_rooms();
void load_exits();
void load_areas_initial();
void load_areas_finish();
void load_grid_templates();
void load_structures_initial();
void load_structures_finish();

void load_item_prototypes();
void load_items_initial();
void load_items_finish();
void load_npc_prototypes();
void load_characters_finish();
void load_characters_initial();

void load_players();

void load_assemblies();
