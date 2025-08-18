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
    int getNextUnitID()
    int getNextAccountID()

    void load_config()
    structs.Room* get_room(int vn)

    map[int, structs.Zone] zone_table
    map[int, structs.CharacterPrototype] mob_proto
    map[int, structs.ObjectPrototype] obj_proto

    vector[weak_ptr[structs.Character]] getAllCharacters()

    vector[weak_ptr[structs.Object]] getAllObjects()

    map[int, structs.Account] accounts

    void boot_db_new()

    structs.help_index_element* get_help(const string& name, int)

    map[int, structs.shop_data] shop_index
    map[int, structs.guild_data] guild_index
    map[int, structs.DgScriptPrototype] trig_index
    map[int64_t, structs.PlayerData] players
    map[int64_t, structs.descriptor_data*] sessions

    int create_join_session(int account_id, int character_id, int64_t connection_id, const string& ip)

cdef extern from "dbat/comm.h" namespace "game":
    void init()
    void init_locale()
    void init_database()
    void init_zones()
    void init_copyover()

cdef extern from "dbat/comm.h":
    void runOneLoop(double deltaTime)

    ctypedef void (*distribute_output_t)()
    ctypedef void (*send_close_t)(int)

    cdef distribute_output_t g_distribute_output
    cdef send_close_t g_send_close

cdef extern from "dbat/utils.h":
    string processColors(const string &txt, int parse, char **choices)

cdef extern from "dbat/constants.h":
    vector[string] getRaceNames()
    vector[string] getSenseiNames()
    vector[string] getFormNames()
    vector[string] getSkillNames()
    vector[string] getRoomFlagNames()
    vector[string] getSectorTypeNames()
    vector[string] getSizeNames()
    vector[string] getPlayerFlagNames()
    vector[string] getMobFlagNames()
    vector[string] getPrefFlagNames()
    vector[string] getAffectFlagNames()
    vector[string] getItemTypeNames()
    vector[string] getWearFlagNames()
    vector[string] getItemFlagNames()
    vector[string] getAdminFlagNames()
    vector[string] getDirectionNames()
    vector[string] getAttributeNames()
    vector[string] getAttributeTrainNames()
    vector[string] getAppearanceNames()
    vector[string] getAlignNames()
    vector[string] getMoneyNames()
    vector[string] getVitalNames()
    vector[string] getStatNames()
    vector[string] getDimNames()
    vector[string] getComStatNames()
    vector[string] getShopFlagNames()
    vector[string] getCharacterFlagNames()
    vector[string] getZoneFlagNames()
    vector[string] getBioGenomeNames()
    vector[string] getMutationNames()
    vector[string] getSexNames()
    vector[string] getSubRaceNames()