from libc.stdint cimport int64_t, int32_t, int16_t, int8_t, uint32_t
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

cdef extern from "dbat/db.h":
    map[int, shared_ptr[structs.room_data]] world
    structs.room_data* get_room(int vn)

    map[int, structs.zone_data] zone_table
    map[int, structs.char_data] mob_proto
    map[int, structs.obj_data] obj_proto

    unordered_map[int, shared_ptr[structs.char_data]] uniqueCharacters
    vector[weak_ptr[structs.char_data]] getAllCharacters()

    map[int, shared_ptr[structs.obj_data]] uniqueObjects
    vector[weak_ptr[structs.obj_data]] getAllObjects()

    map[int, structs.account_data] accounts

    void boot_db_new()

    map[int, structs.shop_data] shop_index
    map[int, structs.guild_data] guild_index
    map[int, structs.index_data] trig_index
    map[int64_t, structs.player_data] players