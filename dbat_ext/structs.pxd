from libc.stdint cimport int64_t, int32_t, int16_t, int8_t
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


DEF NUM_OF_DIRS = 12
DEF NUM_ROOM_FLAGS = 69


cdef extern from "dbat/structs.h":
    cdef struct account_data:
        int vn
        string name
        string passHash
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

    
    cdef struct player_data:
        int id
        string name
        account_data* account
        char_data* character
        unordered_set[int] sensePlayer
        unordered_set[int] senseMemory
        map[int, string] dubNames

    cdef struct extra_descr_data:
        char* keyword
        char* description
        extra_descr_data* next

    cdef cppclass unit_data:
        int vn
        int zone
        int getType() const
        unit_data* proto

        # univeral strings.
        char* name
        char* room_description
        char* look_description
        char* short_description

        extra_descr_data *ex_description
        # dgscripts data
        vector[int] proto_script
        long trigger_types
        long script_context

        double getInventoryWeight()
        int64_t getInventoryCount()

        vector[weak_ptr[obj_data]] getObjects()

        int id
        time_t generation

        string scriptString()

        string getUID(bool active)
        bool isActive()

    cdef cppclass room_direction_data:
        char* general_description
        char* keyword
        int16_t exit_info
        int key
        int to_room
        int dclock
        int dchide
        int dcskill
        int dcmove
        int failsavetype
        int dcfailsave
        int failroom
        int totalfailroom

        room_data* getDestination()

    cdef cppclass room_data(unit_data):
        int sector_type
        list[weak_ptr[char_data]] characters
        int timed
        int dmg
        int geffect

        shared_ptr[room_data] shared()
        optional[int] getLaunchDestination()

        vector[weak_ptr[char_data]] getPeople()

        double getEnvironment(int type)
        double setEnvironment(int type, double value)
        double modEnvironment(int type, double value)
        void clearEnvironment(int type)
        unordered_map[int, double] environment

    cdef cppclass thing_data(unit_data):
        pass

    cdef cppclass obj_data(thing_data):
        pass

    cdef cppclass char_data(thing_data):
        pass


cdef extern from "dbat/db.h":
    map[int, shared_ptr[room_data]] world
    room_data* get_room(int vn)


cdef extern from "dbat/account.h":
    account_data* findAccount(const string& name)
    account_data* createAccount(const string& name, const string& password)