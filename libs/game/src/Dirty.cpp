#include "dbat/game/Dirty.hpp"
#include "dbat/game/Database.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/Room.hpp"
#include "dbat/game/Area.hpp"
#include "dbat/game/Structure.hpp"
#include "dbat/game/Character.hpp"
#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/Object.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/assemblies.hpp"
#include <nlohmann/json.hpp>

namespace dbat::dirty {

    std::unordered_set<std::string> players;
    std::unordered_set<vnum> zones, dgproto, shops, guilds, rooms, areas, structures, nproto, oproto, assemblies;
    
    void saveDirty() {
        auto txn = dbat::db::txn.get();

        if(!players.empty()) {
            for(const auto& id : players) {
                if(auto find = ::players.find(id); find != ::players.end()) {
                    // TODO: handle.
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
                    nlohmann::json j;
                    to_json(j, *find->second);
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
                        {id, j.value("name", ""), j.value("attach_type", ""), j.value("trigger_type", 0), j.value("narg", 0), j.value("arglist", ""), j.value("body", "")}
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
                    nlohmann::json j;
                    to_json(j, *find->second);
                    txn->exec(
                        "INSERT INTO dbat.rooms_blob (id, data, created_at, updated_at) "
                        "VALUES ($1, $2::jsonb, NOW(), NOW()) "
                        "ON CONFLICT (id) DO UPDATE SET "
                        "data = EXCLUDED.data, "
                        "updated_at = NOW()",
                        {id, j.dump()}
                    );
                } else {
                    txn->exec("DELETE FROM dbat.rooms_blob WHERE id=$1", {id});
                }
            }
            rooms.clear();
        }

        if(!areas.empty()) {
            for(const auto& id : areas) {
                if(auto find = Area::registry.find(id); find != Area::registry.end()) {
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
                if(auto find = assemblies.find(id); find != assemblies.end()) {
                    nlohmann::json j;
                    to_json(j, *find);
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
    }
};
