from libcpp cimport bool
from libc.stdint cimport int64_t, int16_t, int8_t
from libcpp.pair cimport pair
from libc.time cimport time_t
from libcpp.string cimport string
from libcpp.list cimport list
from libcpp.set cimport set
from libcpp.map cimport map
from libcpp.unordered_map cimport unordered_map
from libcpp.vector cimport vector
from libcpp.memory cimport shared_ptr
cimport structs
cimport utils

ctypedef structs.BaseCharacter* BaseCharacter_ptr
ctypedef structs.Object* Object_ptr

cdef extern from "dbat/db.h":
    structs.time_info_data time_info
    structs.weather_data weather_info

    void load_config()
    void migrate_db()

    unordered_map[int, structs.Room] world
    unordered_map[int, structs.zone_data] zone_table
    unordered_map[int, structs.area_data] areas

    unordered_map[int, structs.index_data] mob_index
    unordered_map[int, structs.BaseCharacter] mob_proto

    unordered_map[int64_t, pair[time_t, BaseCharacter_ptr]] uniqueCharacters

    unordered_map[int, structs.index_data] obj_index
    unordered_map[int, structs.Object] obj_proto

    unordered_map[int64_t, pair[time_t, Object_ptr]] uniqueObjects

    unordered_map[int, shared_ptr[structs.trig_proto]] trig_index

cdef extern from "dbat/guild.h":
    unordered_map[int, structs.guild_data] guild_index


cdef extern from "dbat/shop.h":
    unordered_map[int, structs.shop_data] shop_index

cdef extern from "dbat/players.h":
    unordered_map[int64_t, shared_ptr[structs.player_data]] players