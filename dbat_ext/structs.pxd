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


cdef extern from "dbat/structs.h":
    cdef cppclass descriptor_data:
        int64_t id
        string processed_output
        list[string] raw_input_queue
        void onConnectionLost(int64_t connID)
        void onConnectionClosed(int64_t connID)

    cdef struct extra_descr_data:
        char* keyword
        char* description
        extra_descr_data* next
    
    cdef cppclass affect_t:
        uint64_t location
        double modifier
        uint64_t specific
        vector[string] specificNames()
        string locName()
        bool isBitwise()
        bool match(int loc, int spec)
        bool isPercent()
    
    cdef cppclass affected_type(affect_t):
        int16_t type
        int16_t duration
        uint32_t bitvector
        affected_type* next
    
    cdef struct obj_spellbook_spell:
        int spellname
        int pages
    
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

    
    cdef struct PlayerData:
        int id
        string name
        Account* account
        Character* character
        unordered_set[int] sensePlayer
        unordered_set[int] senseMemory
        map[int, string] dubNames

    cdef struct cmdlist_element:
        char *cmd
        cmdlist_element* original
        cmdlist_element* next
    
    cdef cppclass DgScriptPrototype:
        int vn
        int8_t attach_type
        int8_t data_type
        char* name
        long trigger_type
        cmdlist_element* cmdlist
        int narg
        char *arglist

    cdef cppclass DgScript(DgScriptPrototype):
        cmdlist_element* curr_state
        int depth
        int loops
        double waiting
        bool purged
        unordered_map[string, string] variables
        Entity* owner
        int order
        int countLine(cmdlist_element* c) const
        bool active
        shared_ptr[DgScript] shared()

    cdef cppclass ThingPrototype:
        int vn

        char* name
        char* room_description
        char* look_description
        char* short_description
        extra_descr_data *ex_description
        vector[int] proto_script
        unordered_map[string, double] stats

    cdef cppclass Coordinates:
        int32_t x
        int32_t y
        int32_t z
    
    cdef cppclass Location:
        Entity* unit
        Coordinates position

    cdef cppclass Entity:
        int getVnum() const
        int getType() const
        Entity* proto

        # univeral strings.
        char* name
        char* room_description
        char* look_description
        char* short_description

        extra_descr_data *ex_description
        # dgscripts data
        
        long trigger_types
        long script_context

        double getInventoryWeight()
        int64_t getInventoryCount()

        vector[weak_ptr[Object]] getObjects()

        int id
        time_t generation

        string scriptString()

        string getUID(bool active)
        bool isActive()

        Location location
    
    cdef cppclass AbstractThing(Entity):
        Room* getRoom() const
        int getRoomVnum() const

        string getLocationName() const
        optional[Destination] getLocationExit(int dir) const
        map[int, Destination] getLocationExits() const

        double getLocationEnvironment(int type) const
        double setLocationEnvironment(int type, double value) const
        double modLocationEnvironment(int type, double value) const
        void clearLocationEnvironment(int type) const

        void setRoomFlag(int flag, bool value) const
        bool toogleRoomFlag(int flag) const
        bool getRoomFlag(int flag) const

        void broadcastAtLocation(const string& message) const
        vector[weak_ptr[Object]] getLocationObjects() const
        vector[weak_ptr[Character]] getLocationPeople() const

        int getLocationDamage() const
        int setLocationDamage(int amount) const
        int modLocationDamage(int amount) const

        int getLocationTileType() const

        int getLocationGroundEffect() const
        int setLocationGroundEffect(int val) const
        int modLocationGroundEffect(int val)
    
    cdef cppclass ObjectPrototype(ThingPrototype):
        pass

    cdef cppclass Object(AbstractThing):
        bool active
        
        Room* getAbsoluteRoom()
        bool isWorking()
        void clearLocation()
        shared_ptr[Object] shared()
        int room_loaded

        # can't convert the std::array value, skipping...

        unordered_map[string, double] dvalue
        int8_t type_flag
        int level
        # can't convert any std::bitsets, skipping...

        double weight
        double getWeight()
        double getTotalWeight()
        int cost
        int cost_per_day
        int timer
        #skipping another bitset....
        int size
        #skipping affected bitset...

        Object* in_obj
        Character* carried_by
        Character* worn_by
        int16_t worn_on
        Entity* holder

        weak_ptr[Character] sitting
        int scoutfreq
        time_t lload
        int64_t kicharge
        int kitype
        Character* user
        Character* target
        int distance
        int foob
        int64_t aucter
        int64_t curBidder
        time_t aucTime
        int bid
        int startbid
        char *auctname
        int posttype
        Object* posted_to
        Object* fellow_wall
        optional[double] gravity

        bool isProvidingLight()
        double currentGravity()
    

    cdef cppclass Destination(Location):
        string general_description
        string keyword
        int16_t exit_info
        int key
        int dclock
        int dchide

        optional[Destination] getReverse() const

    cdef cppclass AbstractLocation(Entity):
        pass

    cdef cppclass Room(AbstractLocation):
        int zone
        vector[int] proto_script
        int sector_type
        list[weak_ptr[Character]] characters
        int timed
        int dmg
        int geffect

        shared_ptr[Room] shared()
        optional[int] getLaunchDestination()

        vector[weak_ptr[Character]] getPeople()

        double getEnvironment(int type)
        double setEnvironment(int type, double value)
        double modEnvironment(int type, double value)
        void clearEnvironment(int type)
        unordered_map[int, double] environment

    cdef cppclass time_info_data:
        double remainder
        int seconds
        int minutes
        int hours
        int day
        int month
        int64_t year
        int64_t current()
    
    cdef cppclass time_data:
        int64_t birth
        time_t created
        int64_t maxage
        time_t logon
        double played
        double secondsAged
        int currentAge()
    
    cdef cppclass alias_data:
        string name
        string replacement
        int type
    
    cdef cppclass mob_special_data:
        list[weak_ptr[Character]] memory
        int attack_type
        int default_pos
        int damnodice
        int damsizedice
        bool newitem
    
    cdef struct follow_type:
        Character* follower
        follow_type* next
    
    cdef struct skill_data:
        int16_t level
        int16_t perfs
    
    cdef cppclass trans_data:
        string description
        double timeSpentInForm
        int grade
        bool visible
        bool limitBroken
        bool unLocked
        double vars[5]
        double blutz

    cdef cppclass CharacterPrototype(ThingPrototype):
        pass

    cdef cppclass Character(AbstractThing):
        shared_ptr[Character] shared()
        char* title

    cdef struct weather_data:
        pass
    
    cdef struct index_data:
        int vn

        char* farg
        DgScript* proto
    
    cdef cppclass shop_buy_data:
        int type
        string keywords
    
    cdef cppclass shop_data:
        pass
    
    cdef cppclass guild_data:
        pass
    
    cdef struct help_index_element:
        char *index
        char *keywords
        char *entry
        int duplicate
        int min_level

    cdef cppclass reset_com:
        char command
        bool if_flag
        int arg1
        int arg2
        int arg3
        int arg4
        int arg5
        int line
        string sarg1
        string sarg2
    
    cdef cppclass Zone:
        string name
        string builders
        int lifespan
        double age
        int bot
        int top
        int reset_mode
        int number
        vector[reset_com] cmd
        int min_level
        int max_level
        uint32_t zone_flags[4]

        list[weak_ptr[Room]] rooms
        unordered_set[int] mobiles
        unordered_set[int] objects
        unordered_set[int] shops
        unordered_set[int] triggers
        unordered_set[int] guilds

        list[weak_ptr[Character]] npcsInZone
        list[weak_ptr[Character]] playersInZone
        list[weak_ptr[Object]] objectsInZone


cdef extern from "dbat/account.h":
    account_data* findAccount(const string& name)
    account_data* createAccount(const string& name, const string& password)