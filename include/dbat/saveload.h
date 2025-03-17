#pragma once
#include <filesystem>
#include "dbat/structs.h"
#include "dbat/json.h"

void runSave();
void load_zones(const std::filesystem::path& loc);
void load_accounts(const std::filesystem::path& loc);
void load_dgscript_prototypes(const std::filesystem::path& loc);

void load_dgscripts_initial(const std::filesystem::path& loc);
void load_dgscripts_finish(const std::filesystem::path& loc);

void load_globaldata(const std::filesystem::path& loc);

void load_shops(const std::filesystem::path& loc);
void load_guilds(const std::filesystem::path& loc);
void load_rooms(const std::filesystem::path& loc);
void load_exits(const std::filesystem::path& loc);

void load_item_prototypes(const std::filesystem::path& loc);
void load_items_initial(const std::filesystem::path& loc);
void load_items_finish(const std::filesystem::path& loc);
void load_npc_prototypes(const std::filesystem::path& loc);
void load_characters_finish(const std::filesystem::path& loc);
void load_characters_initial(const std::filesystem::path& loc);

void load_players(const std::filesystem::path& loc);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(mob_special_data, attack_type, default_pos, damnodice, damsizedice)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(time_data, birth, created, maxage, logon, played, secondsAged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(time_info_data, remainder, seconds, minutes, hours, day, month, year)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(weather_data, pressure, change, sky, sunlight)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(shop_buy_data, type, keywords)

void to_json(json& j, const reset_com& r);
void from_json(const json& j, reset_com& r);

void to_json(json& j, const zone_data& z);
void from_json(const json& j, zone_data& z);

void to_json(json& j, const affect_t& a);
void from_json(const json& j, affect_t& a);

void to_json(json& j, const account_data& a);
void from_json(const json& j, account_data& a);

void to_json(json& j, const trig_var_data& t);
void from_json(const json& j, trig_var_data& t);

void to_json(json& j, const trig_data& t);
void from_json(const json& j, trig_data& t);

void to_json(json&j, const shop_data& s);
void from_json(const json& j, shop_data& s);

void to_json(json& j, const guild_data& g);
void from_json(const json& j, guild_data& g);

void to_json(json& j, const unit_data& u);
void from_json(const json& j, unit_data& u);

void to_json(json& j, const room_direction_data &e);
void from_json(const json& j, room_direction_data &e);

void to_json(json& j, const room_data& r);
void from_json(const json& j, room_data& r);

void to_json(json& j, const obj_data& o);
void from_json(const json& j, obj_data& o);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(skill_data, level, perfs)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(alias_data, name, replacement, type)

void to_json(json& j, const trans_data& t);
void from_json(const json& j, trans_data& t);

void to_json(json& j, const affected_type& a);
void from_json(const json& j, affected_type& a);

void to_json(json& j, const char_data& c);
void from_json(const json& j, char_data& c);

void from_json(const json& j, char_data& c);
void to_json(json& j, const char_data& c);

void to_json(json& j, const player_data& p);
void from_json(const json& j, player_data& p);