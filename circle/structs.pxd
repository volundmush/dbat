from libcpp cimport bool
from libc.stdint cimport int64_t, int16_t, int8_t
from libc.time cimport time_t
from libcpp.string cimport string
from libcpp.list cimport list
from libcpp.set cimport set
from libcpp.map cimport map
from libcpp.unordered_map cimport unordered_map
from libcpp.vector cimport vector
from libcpp.memory cimport shared_ptr
cimport accounts
cimport utils

cdef extern from "dbat/structs.h":
    cdef cppclass reset_com:
        pass

    cdef cppclass zone_data:
        pass

    cdef cppclass area_data:
        area_data(const utils.json& j)
        utils.json serialize()

    cdef cppclass player_data:
        player_data(const utils.json& j)
        int64_t id
        string name
        accounts.account_data* account

        utils.json serialize()

    cdef cppclass extra_descr_data:
        char* keyword
        char* description
        extra_descr_data* next

    cdef cppclass GameEntity:
        int vn
        int zone
        char* name
        char* room_description
        char* look_description
        char* short_description
        bool exists
        extra_descr_data *ex_description

        vector[int] proto_script
        Object* contents

        int64_t id
        time_t generation

    cdef cppclass Object(GameEntity):
        Object(const utils.json& j)
        utils.json serializeInstance()
        utils.json serializeProto()
        string serializeLocation()
        utils.json serializeRelations()

        void deserializeLocation(const string& txt, int16_t slot)
        void deserializeRelations(const utils.json& j)

        void deserializeProto(const utils.json& j)
        void deserializeInstance(const utils.json& j, bool isActive)
        void deserializeContents(const utils.json& j, bool isActive)

        int worn_on

    cdef cppclass Room(GameEntity):
        Room(const utils.json& j)

        utils.json serialize()

        utils.json serializeDgVars()

    cdef cppclass time_info_data:
        void deserialize(const utils.json& j)
        utils.json serialize()

    cdef cppclass BaseCharacter(GameEntity):
        BaseCharacter(const utils.json& j)
        utils.json serializeInstance()
        utils.json serializeProto()
        utils.json serializePlayer()
        utils.json serializeLocation()
        utils.json serializeRelations()

        void deserializeProto(const utils.json& j)
        void deserializeInstance(const utils.json& j, bool isActive)
        void deserializeMobile(const utils.json& j)
        void deserializePlayer(const utils.json& j)
        void deserializeLocation(const utils.json& j)
        void deserializeRelations(const utils.json& j)

    cdef cppclass weather_data:
        utils.json serialize()
        void deserialize(const utils.json& j)

    cdef cppclass index_data:
        int vn

    cdef cppclass descriptor_data:
        int64_t id
        int connected
        BaseCharacter *character
        BaseCharacter *original
    
    cdef cppclass HasVars:
        unordered_map[string, string] vars
        utils.json serializeVars()

    cdef cppclass trig_proto:
        int64_t vn
        int8_t attach_type
        int8_t data_type
        string name
        int trigger_type
        int narg
        string arglist
        vector[string] lines
        utils.json serialize()
    
    cdef cppclass trig_data(HasVars):
        shared_ptr[trig_proto] parent

        int loops
        int totalLoops
        double waiting
        bool purged
        bool active

        utils.json serializeProto()
        utils.json serializeInstance()
        string serializeLocation()

        void deserializeInstance(const utils.json& j)
        void deserializeLocation(const string& txt)

        int order

cdef extern from "dbat/guild.h":
    cdef cppclass guild_data:
        guild_data(const utils.json& j)
        utils.json serialize()


cdef extern from "dbat/shop.h":
    cdef cppclass shop_data:
        shop_data(const utils.json& j)
        utils.json serialize()



cdef extern from "dbat/db.h":
    cdef cppclass zone_data:
        zone_data(const utils.json& j)
        utils.json serialize()