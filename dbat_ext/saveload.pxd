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

    structs.PlayerData* create_player_character(int account_id, const json& j)

    void to_json(json& j, const structs.help_index_element& h)
    void from_json(const json& j, structs.help_index_element& m)

    void to_json(json& j, const structs.ThingPrototype& h)
    void from_json(const json& j, structs.ThingPrototype& m)

    void to_json(json& j, const structs.ObjectPrototype& h)
    void from_json(const json& j, structs.ObjectPrototype& m)

    void to_json(json& j, const structs.CharacterPrototype& h)
    void from_json(const json& j, structs.CharacterPrototype& m)

    void to_json(json& j, const structs.Zone& z)
    void from_json(const json& j, structs.Zone& z)

    void to_json(json& j, const structs.Account& a)
    void from_json(const json& j, structs.Account& a)

    void to_json(json& j, const structs.DgScriptPrototype& t)
    void from_json(const json& j, structs.DgScriptPrototype& t)

    void to_json(json& j, const structs.DgScript& t)
    void from_json(const json& j, structs.DgScript& t)

    void to_json(json&j, const structs.Shop& s)
    void from_json(const json& j, structs.Shop& s)

    void to_json(json& j, const structs.Guild& g)
    void from_json(const json& j, structs.Guild& g)

    void to_json(json& j, const structs.Destination &e)
    void from_json(const json& j, structs.Destination &e)

    void to_json(json& j, const structs.Room& r)
    void from_json(const json& j, structs.Room& r)

    void to_json(json& j, const structs.Object& o)
    void from_json(const json& j, structs.Object& o)

    void to_json(json& j, const structs.Character& c)
    void from_json(const json& j, structs.Character& c)

    void to_json(json& j, const structs.PlayerData& p)
    void from_json(const json& j, structs.PlayerData& p)