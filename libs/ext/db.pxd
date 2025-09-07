from libc.stdint cimport int64_t, int32_t, int16_t, int8_t, uint32_t
from libc.time cimport time_t
from libc.stddef cimport size_t

from libcpp.list cimport list
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.string_view cimport string_view
from libcpp.vector cimport vector
from libcpp.map cimport map
from libcpp.unordered_set cimport unordered_set
from libcpp.unordered_map cimport unordered_map
from libcpp.memory cimport shared_ptr, weak_ptr
from libcpp.optional cimport optional

cdef extern from "dbat/db.h":
    void load_config()

cdef extern from "dbat/Shop.h":
    cdef cppclass Shop:
        pass
    
    map[int, shared_ptr[Shop]] shop_index

cdef extern from "dbat/Guild.h":
    cdef cppclass Guild:
        pass

    map[int, shared_ptr[Guild]] guild_index

cdef extern from "dbat/DgScriptPrototype.h":
    cdef cppclass DgScriptPrototype:
        pass
    map[int, shared_ptr[DgScriptPrototype]] trig_index

cdef extern from "dbat/DgScript.h":
    cdef cppclass DgScript:
        pass

cdef extern from "dbat/players.h":
    map[int64_t, shared_ptr[PlayerData]] players

cdef extern from "dbat/Descriptor.h":
    cdef cppclass descriptor_data:
        int64_t id
        string processed_output
        list[string] raw_input_queue
        void onConnectionLost(int64_t connID)
        void onConnectionClosed(int64_t connID)

    map[int64_t, descriptor_data*] sessions

cdef extern from "dbat/Account.h":
    cdef cppclass Account:
        int id
        string name
        string password
        string email
        time_t created
        time_t lastLogin
        time_t lastLogout
        time_t lastPasswordChanged
        double totalPlayTime
        string disabledReason
        time_t disabledUntil
        int adminLevel
        int rpp
        int slots
        vector[string] customs
        vector[int] characters

        bool checkPassword(const string& password)
        bool setPassword(const string& password)
    
    map[int, shared_ptr[Account]] accounts

    Account* findAccount(const string& name)
    Account* createAccount(const string& name, const string& password)


cdef extern from "dbat/CharacterPrototype.h":
    cdef cppclass CharacterPrototype:
        pass

    map[int, shared_ptr[CharacterPrototype]] mob_proto

cdef extern from "dbat/ObjectPrototype.h":
    cdef cppclass ObjectPrototype:
        pass

    map[int, shared_ptr[ObjectPrototype]] obj_proto

cdef extern from "dbat/Zone.h":
    cdef cppclass Zone:
        string name

    map[int, shared_ptr[Zone]] zone_index

cdef extern from "dbat/Coordinates.h":
    cdef cppclass Coordinates:
        int32_t x
        int32_t y
        int32_t z

cdef extern from "dbat/Location.h":
    cdef cppclass Location:
        pass

cdef extern from "dbat/Destination.h":
    cdef cppclass Destination(Location):
        pass

cdef extern from "dbat/Character.h":
    cdef struct PlayerData:
        int id
        string name
        Account* account
        Character* character
        unordered_set[int] sensePlayer
        unordered_set[int] senseMemory
        map[int, string] dubNames

    cdef cppclass Character:
        pass
    
    cdef unordered_map[int64_t, shared_ptr[Character]] Character_registry "Character::registry"


cdef extern from "dbat/Object.h":
    cdef cppclass Object:
        pass
    
    cdef unordered_map[int64_t, shared_ptr[Object]] Object_registry "Object::registry"

cdef extern from "dbat/Room.h":
    cdef cppclass Room:
        pass
    cdef unordered_map[int, shared_ptr[Room]] Room_registry "Room::registry"

cdef extern from "dbat/RoomUtils.h":
    Room* get_room(int vn)

cdef extern from "dbat/db.h":
    int getNextUnitID()
    int getNextAccountID()
    
    vector[weak_ptr[Character]] getAllCharacters()

    vector[weak_ptr[Object]] getAllObjects()

    void boot_db_new()

    int create_join_session(int account_id, int character_id, int64_t connection_id, const string& ip)

cdef extern from "dbat/Help.h":
    cdef struct help_index_element:
        string index
        string keywords
        string entry
        int duplicate
        int min_level
    
    vector[help_index_element] help_table
    help_index_element* get_help(string_view name, int)

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

cdef extern from "dbat/ansi.h":
    string processColors(const string &txt, int parse, char **choices)

cdef extern from "dbat/API.h":
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
    vector[string] getAndroidModelNames()