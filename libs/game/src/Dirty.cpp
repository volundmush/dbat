#include "dbat/game/Dirty.hpp"
#include "dbat/game/Database.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/Room.hpp"
#include "dbat/game/Area.hpp"
#include "dbat/game/Structure.hpp"
#include "dbat/game/Character.hpp"
#include "dbat/game/players.hpp"
#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/Object.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/assemblies.hpp"
#include "dbat/game/weather.hpp"
#include "dbat/game/TimeInfo.hpp"
#include "dbat/game/ID.hpp"
#include <nlohmann/json.hpp>

using ::to_json;
using ::from_json;

namespace dbat::dirty {

    std::unordered_set<std::string> players;
    std::unordered_set<vnum> zones, dgproto, shops, guilds, rooms, areas, structures, nproto, oproto, assemblies;
    
    void saveDirty() {
        auto txn = dbat::db::txn.get();

        if(!players.empty()) {
            for(const auto& id : players) {
                if(auto find = ::players.find(id); find != ::players.end()) {
                    auto &p = *find->second;
                    nlohmann::json j;
                    to_json(j, p);

                    txn->exec(
                        "INSERT INTO dbat.pc_components (pc_id, component_name, data, created_at, updated_at) "
                        "VALUES ($1, 'dbat_player', $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (pc_id, component_name) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );

                    if(p.character) {
                        nlohmann::json jc;
                        to_json(jc, *p.character);

                        txn->exec(
                            "INSERT INTO dbat.pc_components (pc_id, component_name, data, created_at, updated_at) "
                            "VALUES ($1, 'dbat_character', $2::jsonb, NOW(), NOW()) "
                            "ON CONFLICT (pc_id, component_name) DO UPDATE SET "
                            "data = EXCLUDED.data, "
                            "updated_at = NOW()",
                            {id, jc.dump()}
                        );
                    }
                } else {
                    // was deleted!
                }
            }
            players.clear();
        }

        if(!zones.empty()) {
            for(const auto& id : zones) {
                if(auto find = zone_table.find(id); find != zone_table.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.zones_blob (id, name, data, created_at, updated_at) "
                        "VALUES ($1, $2, $3::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "name = EXCLUDED.name, "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.value("name", ""), j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.zones_blob WHERE id=$1", {id});
                }
            }
            zones.clear();
        }

        if(!dgproto.empty()) {
            for(const auto& id : dgproto) {
                if(auto find = trig_index.find(id); find != trig_index.end()) {
                    auto &dg = *find->second;
                    txn->exec(
                        "INSERT INTO dbat.dgproto (id, name, attach_type, trigger_type, narg, arglist, body, created_at, updated_at) "
                        "VALUES ($1, $2, $3, $4, $5, $6, $7, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "name = EXCLUDED.name, "
                        "attach_type = EXCLUDED.attach_type, "
                        "trigger_type = EXCLUDED.trigger_type, "
                        "narg = EXCLUDED.narg, "
                        "arglist = EXCLUDED.arglist, "
                        "body = EXCLUDED.body, "
                        "updated_at = NOW()",
                        {dg.vn, dg.name, dg.attach_type, dg.trigger_type, dg.narg, dg.arglist, dg.scriptString()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.dgproto WHERE id=$1", {id});
                }
            }
            dgproto.clear();
        }

        if(!shops.empty()) {
            for(const auto& id : shops) {
                if(auto find = shop_index.find(id); find != shop_index.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.shops_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.shops_blob WHERE id=$1", {id});
                }
            }
            shops.clear();
        }

        if(!guilds.empty()) {
            for(const auto& id : guilds) {
                if(auto find = guild_index.find(id); find != guild_index.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.guilds_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.guilds_blob WHERE id=$1", {id});
                }
            }
            guilds.clear();
        }

        if(!rooms.empty()) {
            for(const auto& id : rooms) {
                if(auto find = Room::registry.find(id); find != Room::registry.end()) {
                    nlohmann::json j, je;
                    to_json(j, *find->second);
                    to_json(je, find->second->getDirections());
                    txn->exec(
                        "INSERT INTO dbat.rooms_blob (id, data, exits, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, $3::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "exits = EXCLUDED.exits, "
                        "updated_at = NOW()",
                        {id, j.dump(), je.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.rooms_blob WHERE id=$1", {id});
                }
            }
            rooms.clear();
        }

        if(!areas.empty()) {
            for(const auto& id : areas) {
                if(auto find = ::areas.find(id); find != ::areas.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.areas_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.areas_blob WHERE id=$1", {id});
                }
            }
            areas.clear();
        }

        if(!structures.empty()) {
            for(const auto& id : structures) {
                if(auto find = Structure::registry.find(id); find != Structure::registry.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.structures_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.structures_blob WHERE id=$1", {id});
                }
            }
            structures.clear();
        }

        if(!oproto.empty()) {
            for(const auto& id : oproto) {
                if(auto find = obj_proto.find(id); find != obj_proto.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.oproto_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.oproto_blob WHERE id=$1", {id});
                }
            }
            oproto.clear();
        }

        if(!nproto.empty()) {
            for(const auto& id : nproto) {
                if(auto find = mob_proto.find(id); find != mob_proto.end()) {
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.nproto_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.nproto_blob WHERE id=$1", {id});
                }
            }
            nproto.clear();
        }

        if(!assemblies.empty()) {
            for(const auto& id : assemblies) {
                if(auto find = g_mAssemblyTable.find(id); find != g_mAssemblyTable.end()) {
                    nlohmann::json j;
                    to_json(j, find->second);
                    txn->exec(
                        "INSERT INTO dbat.assemblies_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.assemblies_blob WHERE id=$1", {id});
                }
            }
            assemblies.clear();
        }

        {
            nlohmann::json j;
            j["time"] = time_info;
            j["era_uptime"] = era_uptime;
            j["weather"] = weather_info;
            j["lastCharacterID"] = Character::lastID;
            j["lastObjectID"] = Object::lastID;
            j["lastStructureID"] = lastStructureID;
            j["lastGridTemplateID"] = lastGridTemplateID;
            j["lastAreaID"] = lastAreaID;
            j["lastRoomID"] = Room::lastID;
            j["lastShopID"] = lastShopID;
            j["lastGuildID"] = lastGuildID;
            j["lastScriptID"] = lastScriptID;

            for(const auto& [key, value] : j.items()) {
                txn->exec(
                    "INSERT INTO dbat.globals (key, data) VALUES ($1, $2::jsonb) "
                    "ON CONFLICT (key) DO UPDATE SET data = EXCLUDED.data",
                    {key, value.dump()}
                );
            }
        }
    }

    void dirtyAll() {
        for(const auto& [id, p] : ::players) {
            players.insert(id);
        }
        for(const auto& [id, z] : zone_table) {
            zones.insert(id);
        }
        for(const auto& [id, dg] : trig_index) {
            dgproto.insert(id);
        }
        for(const auto& [id, s] : shop_index) {
            shops.insert(id);
        }
        for(const auto& [id, g] : guild_index) {
            guilds.insert(id);
        }
        for(const auto& [id, r] : Room::registry) {
            rooms.insert(id);
        }
        for(const auto& [id, a] : ::areas) {
            areas.insert(id);
        }
        for(const auto& [id, s] : Structure::registry) {
            structures.insert(id);
        }
        for(const auto& [id, o] : obj_proto) {
            oproto.insert(id);
        }
        for(const auto& [id, n] : mob_proto) {
            nproto.insert(id);
        }
        for(const auto& [vn, a] : g_mAssemblyTable) {
            assemblies.insert(vn);
        }
    }
};
