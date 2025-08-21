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


cdef extern from "dbat/Descriptor.h":
    cdef cppclass descriptor_data:
        int64_t id
        string processed_output
        list[string] raw_input_queue
        void onConnectionLost(int64_t connID)
        void onConnectionClosed(int64_t connID)

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

cdef extern from "dbat/DgScriptPrototype":
    cdef cppclass DgScriptPrototype:
        pass

cdef extern from "dbat/DgScript.h":
    cdef cppclass DgScript:
        pass

cdef extern from "dbat/ThingPrototype.h":
    cdef cppclass ThingPrototype:
        pass

cdef extern from "dbat/Coordinates.h":
    cdef cppclass Coordinates:
        int32_t x
        int32_t y
        int32_t z

cdef extern from "dbat/Location.h":
    cdef cppclass Location:
        pass

cdef extern from "dbat/ObjectPrototype.h":
    cdef cppclass ObjectPrototype(ThingPrototype):
        pass

cdef extern from "dbat/Object.h":
    cdef cppclass Object:
        pass
    
    cdef unordered_map[int64_t, shared_ptr[Object]] Object_registry "Object::registry"

cdef extern from "dbat/Destination.h":
    cdef cppclass Destination(Location):
        pass

    
cdef extern from "dbat/CharacterPrototype.h":
    cdef cppclass CharacterPrototype(ThingPrototype):
        pass

cdef extern from "dbat/structs.h":
    cdef struct help_index_element:
        char *index
        char *keywords
        char *entry
        int duplicate
        int min_level

cdef extern from "dbat/Guild.h":
    cdef cppclass Guild:
        pass

cdef extern from "dbat/Shop.h":
    cdef cppclass Shop:
        pass

cdef extern from "dbat/Room.h":
    cdef cppclass Room:
        pass
    cdef unordered_map[int, shared_ptr[Room]] Room_registry "Room::registry"

cdef extern from "dbat/Zone.h":
    cdef cppclass Zone:
        string name

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


cdef extern from "dbat/Account.h":
    Account* findAccount(const string& name)
    Account* createAccount(const string& name, const string& password)