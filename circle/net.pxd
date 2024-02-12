from libcpp cimport bool
from libc.stdint cimport int64_t, int16_t, uint8_t
from libcpp.string cimport string
from libcpp.list cimport list
from libcpp.set cimport set
from libcpp.unordered_map cimport unordered_map
from libcpp.vector cimport vector
from libcpp.memory cimport shared_ptr
from libcpp.pair cimport pair
cimport accounts
cimport structs

cdef extern from "dbat/net.h" namespace "net":

    cdef enum class ConnectionState:
        Negotiating = 0
        Pending = 1
        Connected = 2
        Dead = 3

    cdef cppclass ProtocolCapabilities:
        string protocolName
        string clientName
        string clientVersion
        string hostAddress
        vector[string] hostNames
        int16_t hostPort

    cdef cppclass Connection:
        Connection(const string& connID, const string& host)
        int64_t connId
        void queueMessage(const string& event, const string& data)
        list[pair[string, string]] outQueue
        ProtocolCapabilities capabilities
        bool running
        ConnectionState state
        void onHeartbeat(double deltaTime)
        shared_ptr[accounts.account_data] account
        structs.descriptor_data* desc;

    cdef unordered_map[string, shared_ptr[Connection]] connections


    cdef shared_ptr[Connection] newConnection(const string& connID, const string& ip, int64_t account)