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

cimport db

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

    db.PlayerData* create_player_character(int account_id, const json& j)

    void to_json(json& j, const db.help_index_element& h)
    void from_json(const json& j, db.help_index_element& m)

    void to_json(json& j, const db.ObjectPrototype& h)
    void from_json(const json& j, db.ObjectPrototype& m)

    void to_json(json& j, const db.CharacterPrototype& h)
    void from_json(const json& j, db.CharacterPrototype& m)

    void to_json(json& j, const db.Zone& z)
    void from_json(const json& j, db.Zone& z)

    void to_json(json& j, const db.Account& a)
    void from_json(const json& j, db.Account& a)

    void to_json(json& j, const db.DgScriptPrototype& t)
    void from_json(const json& j, db.DgScriptPrototype& t)

    void to_json(json& j, const db.DgScript& t)
    void from_json(const json& j, db.DgScript& t)

    void to_json(json&j, const db.Shop& s)
    void from_json(const json& j, db.Shop& s)

    void to_json(json& j, const db.Guild& g)
    void from_json(const json& j, db.Guild& g)

    void to_json(json& j, const db.Destination &e)
    void from_json(const json& j, db.Destination &e)

    void to_json(json& j, const db.Room& r)
    void from_json(const json& j, db.Room& r)

    void to_json(json& j, const db.Object& o)
    void from_json(const json& j, db.Object& o)

    void to_json(json& j, const db.Character& c)
    void from_json(const json& j, db.Character& c)

    void to_json(json& j, const db.PlayerData& p)
    void from_json(const json& j, db.PlayerData& p)