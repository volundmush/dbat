from libc.time cimport time_t
from libc.stdint cimport int64_t, int16_t, int8_t
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.unordered_map cimport unordered_map
from libcpp.memory cimport shared_ptr
cimport utils

cdef extern from "dbat/structs.h":
    cdef cppclass account_data:
        account_data()
        account_data(const utils.json& j)
        int vn
        string name
        string email
        string passHash
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
        vector[int64_t] characters

        void modRPP(int amt)

        utils.json serialize()
        void deserialize(const utils.json& j)
        @staticmethod
        int getNextID()

cdef extern from "dbat/account.h":
    shared_ptr[account_data] createAccount(const string& name, const string& password) except+
    shared_ptr[account_data] findAccount(const string& name)

    unordered_map[int, shared_ptr[account_data]] accounts