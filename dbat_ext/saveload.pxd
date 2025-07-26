from libc.stdint cimport int64_t, int32_t, int16_t, int8_t, uint32_t, uint64_t
from libc.time cimport time_t
from libc.stddef cimport size_t

from libcpp.list cimport list
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.map cimport map
from libcpp.unordered_set cimport unordered_set
from libcpp.unordered_map cimport unordered_map
from libcpp.memory cimport shared_ptr, weak_ptr
from libcpp.optional cimport optional

cimport structs

cdef extern from "nlohmann/json.hpp" namespace "nlohmann":
    cdef cppclass json:
        pass

cdef extern from "dbat/json.h":
    string jdumps(const json& j)
    json jloads(const string& s)
    string jdumps_pretty(const json& j)
    json jobject()

cdef extern from "dbat/saveload.h":
    void runSave()

    structs.player_data* create_player_character(int account_id, const json& j)

    void to_json(json& j, const structs.help_index_element& h)
    void from_json(const json& j, structs.help_index_element& m)

    void to_json(json& j, const structs.proto_data& h)
    void from_json(const json& j, structs.proto_data& m)

    void to_json(json& j, const structs.item_proto_data& h)
    void from_json(const json& j, structs.item_proto_data& m)

    void to_json(json& j, const structs.npc_proto_data& h)
    void from_json(const json& j, structs.npc_proto_data& m)
    
    void to_json(json& j, const structs.mob_special_data& m)
    void from_json(const json& j, structs.mob_special_data& m)

    void to_json(json& j, const structs.time_data &t)
    void from_json(const json& j, structs.time_data &t)

    void to_json(json& j, const structs.time_info_data &t)
    void from_json(const json& j, structs.time_info_data &t)

    void to_json(json& j, const structs.weather_data &w)
    void from_json(const json& j, structs.weather_data &w)

    void to_json(json& j, const structs.reset_com& r)
    void from_json(const json& j, structs.reset_com& r)

    void to_json(json& j, const structs.zone_data& z)
    void from_json(const json& j, structs.zone_data& z)

    void to_json(json& j, const structs.affect_t& a)
    void from_json(const json& j, structs.affect_t& a)

    void to_json(json& j, const structs.account_data& a)
    void from_json(const json& j, structs.account_data& a)

    void to_json(json& j, const structs.trig_var_data& t)
    void from_json(const json& j, structs.trig_var_data& t)

    void to_json(json& j, const structs.trig_data& t)
    void from_json(const json& j, structs.trig_data& t)

    void to_json(json& j, const structs.shop_buy_data& b)
    void from_json(const json& j, structs.shop_buy_data& b)

    void to_json(json&j, const structs.shop_data& s)
    void from_json(const json& j, structs.shop_data& s)

    void to_json(json& j, const structs.guild_data& g)
    void from_json(const json& j, structs.guild_data& g)

    void to_json(json& j, const structs.unit_data& u)
    void from_json(const json& j, structs.unit_data& u)

    void to_json(json& j, const structs.room_direction_data &e)
    void from_json(const json& j, structs.room_direction_data &e)

    void to_json(json& j, const structs.room_data& r)
    void from_json(const json& j, structs.room_data& r)

    void to_json(json& j, const structs.obj_data& o)
    void from_json(const json& j, structs.obj_data& o)

    void to_json(json& j, const structs.skill_data& s)
    void from_json(const json& j, structs.skill_data& s)

    void to_json(json& j, const structs.alias_data& a)
    void from_json(const json& j, structs.alias_data& a)

    void to_json(json& j, const structs.trans_data& t)
    void from_json(const json& j, structs.trans_data& t)

    void to_json(json& j, const structs.affected_type& a)
    void from_json(const json& j, structs.affected_type& a)

    void to_json(json& j, const structs.char_data& c)
    void from_json(const json& j, structs.char_data& c)

    void to_json(json& j, const structs.player_data& p)
    void from_json(const json& j, structs.player_data& p)