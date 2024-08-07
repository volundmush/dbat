#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>
#include <regex>
#include <sys/epoll.h>

#include "dbat/net.h"
#include "dbat/utils.h"
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"
#include "dbat/account.h"
#include "dbat/accmenu.h"

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {
    std::unordered_map<int, std::shared_ptr<Connection>> connections;
    std::unordered_map<int, DisconnectReason> deadConnections;
    std::set<int> pendingConnections, pendingOutData, pendingReads, pendingWrites;

    int server_fd = -1;
    int epoll_fd = -1;

    void ProtocolCapabilities::deserialize(const nlohmann::json& j) {
        if(j.contains("protocol")) {
            auto s = j["protocol"].get<std::string>();
            if(boost::iequals(s, "Telnet")) protocol = Protocol::Telnet;
            else if(boost::iequals(s, "WebSocket")) protocol = Protocol::WebSocket;
        }
        if(j.contains("encryption")) encryption = j["encryption"];
        if(j.contains("client_name")) clientName = j["client_name"];
        if(j.contains("client_version")) clientVersion = j["client_version"];
		if(j.contains("host_address")) hostAddress = j["host_address"];
        if(j.contains("host_port")) hostPort = j["host_port"];
        if(j.contains("host_names")) for(auto &hn : j["host_names"]) {
            hostNames.emplace_back(hn.get<std::string>());
        }
        if(j.contains("encoding")) encoding = j["encoding"];
        if(j.contains("utf8")) utf8 = j["utf8"];

        if(j.contains("colorType")) colorType = j["color"].get<ColorType>();

        if(j.contains("width")) width = j["width"];
        if(j.contains("height")) height = j["height"];
        if(j.contains("gmcp")) gmcp = j["gmcp"];
        if(j.contains("msdp")) msdp = j["msdp"];
        if(j.contains("mssp")) mssp = j["mssp"];
        if(j.contains("mxp")) mxp = j["mxp"];
        if(j.contains("mccp2")) mccp2 = j["mccp2"];
        if(j.contains("mccp3")) mccp3 = j["mccp3"];
        if(j.contains("ttype")) ttype = j["ttype"];
        if(j.contains("naws")) naws = j["naws"];
        if(j.contains("sga")) sga = j["sga"];
        if(j.contains("linemode")) linemode = j["linemode"];
        if(j.contains("force_endline")) force_endline = j["force_endline"];
        if(j.contains("oob")) oob = j["oob"];
        if(j.contains("tls")) tls = j["tls"];
        if(j.contains("screen_reader")) screen_reader = j["screen_reader"];
        if(j.contains("mouse_tracking")) mouse_tracking = j["mouse_tracking"];
        if(j.contains("vt100")) vt100 = j["vt100"];
        if(j.contains("osc_color_palette")) osc_color_palette = j["osc_color_palette"];
        if(j.contains("proxy")) proxy = j["proxy"];
        if(j.contains("mnes")) mnes = j["mnes"];
    }

    nlohmann::json ProtocolCapabilities::serialize() {
        nlohmann::json j;

        if(encryption) j["encryption"] = encryption;
        j["client_name"] = clientName;
        j["client_version"] = clientVersion;
        j["host_address"] = hostAddress;
        if(hostPort) j["host_port"] = hostPort;
        for(auto &hn : hostNames) {
            j["host_names"].push_back(hn);
        }
        if(!encoding.empty()) j["encoding"] = encoding;
        if(utf8) j["utf8"] = utf8;
        if(colorType != ColorType::NoColor) j["colorType"] = colorType;
        if(width != 80) j["width"] = width;
        if(height != 52) j["height"] = height;
        if(gmcp) j["gmcp"] = gmcp;
        if(msdp) j["msdp"] = msdp;
        if(mssp) j["mssp"] = mssp;
        if(mxp) j["mxp"] = mxp;
        if(mccp2) j["mccp2"] = mccp2;
        if(mccp3) j["mccp3"] = mccp3;
        if(ttype) j["ttype"] = ttype;
        if(naws) j["naws"] = naws;
        if(sga) j["sga"] = sga;
        if(linemode) j["linemode"] = linemode;
        if(force_endline) j["force_endline"] = force_endline;
        if(oob) j["oob"] = oob;
        if(tls) j["tls"] = tls;
        if(screen_reader) j["screen_reader"] = screen_reader;
        if(mouse_tracking) j["mouse_tracking"] = mouse_tracking;
        if(vt100) j["vt100"] = vt100;
        if(osc_color_palette) j["osc_color_palette"] = osc_color_palette;
        if(proxy) j["proxy"] = proxy;
        if(mnes) j["mnes"] = mnes;

        return j;
    }

    std::string ProtocolCapabilities::protocolName() const {
        switch(protocol) {
            case Protocol::Telnet: return "telnet";
            case Protocol::WebSocket: return "websocket";
            default:
                return "unknown";
        }
    }

    void ConnectionParser::close() {

    }

    void ConnectionParser::sendText(const std::string &txt) {
        conn->sendText(txt);
    }

    void ConnectionParser::start() {

    }

    void ConnectionParser::handleGMCP(const std::string &txt, const nlohmann::json &j) {

    }

    void Connection::sendText(const std::string &text) {
        auto colored = processColors(text, true, nullptr);

        telnet_send_text(teldata, colored.c_str(), colored.size());
    }

    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        state = ConnectionState::Connected;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        state = ConnectionState::Dead;
        deadConnections[connId] = DisconnectReason::GameLogoff;
        if(parser) parser->close();
        parser.reset();
    }

    void Connection::onNetworkDisconnected() {
        state = ConnectionState::Dead;
        deadConnections[connId] = DisconnectReason::ConnectionLost;
    }

    static const telnet_telopt_t my_telopts[] = {
            { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
            //{ TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   },
            { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
            { TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT },
            { -1, 0, 0 }
    };

    void telnet_handler(telnet_t *telnet, telnet_event_t *event, void *user_data) {
        auto conn = (Connection*)user_data;
        conn->handleTelnet(event);
    }

    void Connection::sendTelnetNegotiations() {
        for (int i = 0; teldata->telopts[i].telopt != -1; ++i) {
            auto &opt = teldata->telopts[i];
            if(opt.us == TELNET_WILL) {
                telnet_negotiate(teldata, TELNET_WILL, opt.telopt);
            }
            if(opt.him == TELNET_DO) {
                telnet_negotiate(teldata, TELNET_DO, opt.telopt);
            }
        }
    }

    void Connection::update(double deltaTime) {
        while(parser && !pendingCommands.empty()) {
            parser->parse(pendingCommands.front());
            pendingCommands.pop_front();
        }
    }

    void Connection::handleAppData() {
        lastReceivedBytes = std::chrono::steady_clock::now();

        // Find the first occurrence of "\r\n" in the buffer
        auto it = std::search(appbuf.begin(), appbuf.end(), "\r\n", "\r\n" + 2);

        while (it != appbuf.end()) {
            // Extract the command up to the "\r\n"
            std::string command(appbuf.begin(), it);

            // Append the command to pendingCommands
            pendingCommands.push_back(std::move(command));

            // Remove the processed command and "\r\n" from appbuf
            it += 2; // Move past the "\r\n"
            appbuf.erase(appbuf.begin(), it);

            // Look for the next "\r\n" in the remaining buffer
            it = std::search(appbuf.begin(), appbuf.end(), "\r\n", "\r\n" + 2);
        }

        // If no more complete commands are found, the remaining data in appbuf is left as-is
    }

    void Connection::handleTelnetCommand(char cmd) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetWill(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetDo(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetDont(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetWont(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetSubNegotiate(char telopt, const char *buffer, size_t size) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnet(telnet_event_t *event) {
        switch(event->type) {
            case TELNET_EV_DATA: {
                appbuf.insert(appbuf.end(), event->data.buffer, event->data.buffer + event->data.size);
                handleAppData();
            }
                break;
            case TELNET_EV_SEND: {
                outbuf.insert(outbuf.end(), event->data.buffer, event->data.buffer + event->data.size);
                pendingOutData.insert(connId);
            }
                break;
            case TELNET_EV_IAC:
                handleTelnetCommand(event->iac.cmd);
                break;
            case TELNET_EV_WILL:
                handleTelnetWill(event->neg.telopt);
                break;
            case TELNET_EV_DO:
                handleTelnetDo(event->neg.telopt);
                break;
            case TELNET_EV_WONT:
                handleTelnetWont(event->neg.telopt);
                break;
            case TELNET_EV_DONT:
                handleTelnetDont(event->neg.telopt);
                break;
            case TELNET_EV_SUBNEGOTIATION:
                handleTelnetSubNegotiate(event->sub.telopt, event->sub.buffer, event->sub.size);
                break;
            case TELNET_EV_COMPRESS:
                break;
            case TELNET_EV_ZMP:
                break;
            case TELNET_EV_TTYPE:
                break;
            case TELNET_EV_ENVIRON:
                break;
            case TELNET_EV_MSSP:
                break;
            case TELNET_EV_WARNING:
                break;
            case TELNET_EV_ERROR:
                break;
        }
    }

    Connection::Connection(int connId) : connId(connId) {
        teldata = telnet_init(my_telopts, &telnet_handler, 0, this);
    }

    Connection::Connection(int connId, const nlohmann::json &j) : Connection(connId) {
        deserialize(j);
    }

    nlohmann::json Connection::serialize() {

    }

    void Connection::deserialize(const nlohmann::json &j) {

    }

    Connection::~Connection() {
        epollUnregister();
        // Free the telnet data.
        telnet_free(teldata);
        // Close the descriptor.
        ::close(connId);
    }

    void Connection::cleanup(DisconnectReason reason) {
        if(account) {
            // Remove ourselves from the account's connections list.
            account->connections.erase(this);
            account = nullptr;
        }
        switch(reason) {
            case DisconnectReason::ConnectionLost:
                // The connection to remote host was lost for unknown reasons.
                if(desc) {
                    // If we have a desc then we inform it of the unexpected disconnect.
                    // It can handle this as it sees fit.
                    // onConnectionLost must remove the connection from the descriptor.
                    desc->onConnectionLost(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::ConnectionClosed:
                // The remote socket gracefully/voluntarily closed by the remote host.
                // The portal is aware of this and already cleaned them up.
                if(desc) {
                    // If we have a desc then we can attempt a 'quit' on the
                    // connected character. OR however else it wants to handle it.
                    // onConnectionClosed must remove the connection from the descriptor.
                    desc->onConnectionClosed(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::GameLogoff: {
                // The game server is initiating disconnection for some reason or another.
                }
                break;
        }
    }


    void Connection::setParser(ConnectionParser *p) {
        if(parser) parser->close();
        parser.reset(p);
        p->start();
    }

    int create_listener(const std::string& host, uint16_t port) {
        struct addrinfo hints{}, *res, *p;
        int status;
        char port_str[6];
        int listen_fd;
        snprintf(port_str, sizeof(port_str), "%u", config::port);

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;     // Use AF_INET6 to force IPv6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;     // Use the IP address provided

        // Convert hostAddress and port to a suitable format for getaddrinfo
        if ((status = getaddrinfo(host.c_str(), port_str, &hints, &res)) != 0) {
            std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
            exit(EXIT_FAILURE);
        }

        // Loop through the results and bind to the first valid address
        for (p = res; p != nullptr; p = p->ai_next) {
            // Create a socket
            if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("Socket creation failed");
                continue;
            }

            // Allow reusing the address
            int optval = 1;
            if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
                perror("setsockopt");
                close(listen_fd);
                continue;
            }

            // Bind the socket to the address
            if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
                perror("Bind failed");
                close(listen_fd);
                continue;
            }

            break; // Successfully bound
        }

        // Free the address info linked list
        freeaddrinfo(res);

        // Listen for incoming connections
        if (listen(listen_fd, 50) < 0) {
            perror("Listen failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }

        int flags = fcntl(listen_fd, F_GETFD);
        if (flags == -1) {
            perror("fcntl F_GETFD failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        flags &= ~FD_CLOEXEC;  // Clear the FD_CLOEXEC flag
        if (fcntl(listen_fd, F_SETFD, flags) == -1) {
            perror("fcntl F_SETFD failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        // Set it non-blocking.
        flags = fcntl(listen_fd, F_GETFL, 0);
        fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

        return listen_fd;
    }

    void init() {
        // Let's initialize the epoll
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            perror("epoll_create1");
            exit(EXIT_FAILURE);
        }

        // server_fd might already be set to a proper port by the copyover process.
        if(server_fd == -1) {
            // This is a fresh start. We need to create and bind a server to config::hostAddress config::port and set
            // the fd to server_fd
            server_fd = create_listener(config::hostAddress, config::port);
        }
    }

    void Connection::epollRegister() {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev.data.fd = connId;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connId, &ev) == -1) {
            perror("epoll_ctl: connection fd");
            exit(EXIT_FAILURE);
        }
    }

    void Connection::epollUnregister() {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connId, NULL) == -1) {
            perror("epoll_ctl: EPOLL_CTL_DEL");
        }
    }

    void Connection::disableCompression() {

    }

    void Connection::readFromSocket() {
        if(state == ConnectionState::Dead) {
            return;
        }

        std::vector<char> buffer(4096);

        while (true) {
            ssize_t bytesRead = read(connId, buffer.data(), buffer.size());
            if (bytesRead > 0) {
                // Successfully read data, process it
                telnet_recv(teldata, buffer.data(), bytesRead);
            } else if (bytesRead == 0) {
                // Connection closed by the client
                void onNetworkDisconnected();
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more data available right now, would block
                    break;
                } else if(errno == ETIMEDOUT) {
                    void onNetworkTimedOut();
                } else {
                    // An error occurred, log or handle it
                    std::cerr << "Read error: " << strerror(errno) << std::endl;
                    // Handle error if needed
                    break;
                }
            }
        }
    }

    void Connection::writeToSocket() {
        // Iterate over the buffer's data sequence and attempt to send it
        while (!outbuf.empty()) {
            // Access the first data sequence in the buffer
            // Attempt to send the data
            ssize_t bytesSent = send(connId, outbuf.data(), outbuf.size(), 0);

            if (bytesSent == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket is not ready to send more data; stop and wait for EPOLLOUT
                    pendingWrites.erase(connId);
                    break;
                } else {
                    onNetworkDisconnected();
                    break;
                }
            }

            // Consume the sent data from the buffer
            outbuf.erase(outbuf.begin(), outbuf.begin() + bytesSent);
        }

        if (outbuf.empty()) {
            pendingOutData.erase(connId);
        }
    }

    void acceptConnection(int conn_fd, sockaddr_in& client_addr) {

        // Set the new connection to non-blocking
        int flags = fcntl(conn_fd, F_GETFL, 0);
        fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);

        int optval = 1;
        if (setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
            perror("setsockopt(SO_KEEPALIVE) failed");
            exit(EXIT_FAILURE);
        }

        optval = 1;
        if (setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
            perror("setsockopt(SO_KEEPALIVE) failed");
            exit(EXIT_FAILURE);
        }

        // Set TCP_KEEPIDLE to 60 seconds (start sending keepalive probes after 60 seconds of idle time)
        optval = 60;
        if (setsockopt(conn_fd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0) {
            perror("setsockopt(TCP_KEEPIDLE) failed");
            exit(EXIT_FAILURE);
        }

        // Set TCP_KEEPINTVL to 10 seconds (interval between keepalive probes)
        optval = 10;
        if (setsockopt(conn_fd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) < 0) {
            perror("setsockopt(TCP_KEEPINTVL) failed");
            exit(EXIT_FAILURE);
        }

        // Set TCP_KEEPCNT to 5 (number of unacknowledged probes before considering the connection dead)
        optval = 5;
        if (setsockopt(conn_fd, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) < 0) {
            perror("setsockopt(TCP_KEEPCNT) failed");
            exit(EXIT_FAILURE);
        }

        // Create a new Connection object and add it to the map
        auto conn = std::make_shared<Connection>(conn_fd);
        connections[conn_fd] = conn;

        char ip_str[INET6_ADDRSTRLEN]; // Enough space to hold both IPv4 and IPv6 addresses

        if (client_addr.sin_family == AF_INET) {
            auto s = (struct sockaddr_in *)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
        } else if (client_addr.sin_family == AF_INET6) {
            auto s = (struct sockaddr_in6 *)&client_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
        } else {
            fprintf(stderr, "Unknown address family\n");
            exit(EXIT_FAILURE);
        }
        conn->capabilities.hostAddress = ip_str;

        conn->epollRegister();
        conn->state = ConnectionState::Pending;
        conn->sendTelnetNegotiations();
    }

    void acceptAllIncomingConnections() {
        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int conn_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (conn_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more incoming connections
                    break;
                } else {
                    perror("accept");
                    break;
                }
            }
            acceptConnection(conn_fd, client_addr);
        }
    }

    void update(double deltaTime) {
        if(connections.empty()) return;

        std::vector<epoll_event> events(connections.size());

        int nfds = epoll_wait(epoll_fd, events.data(), events.size(), 0);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < nfds; i++) {
            auto &ev = events[i];

            if(ev.events & EPOLLIN) {
                pendingReads.insert(ev.data.fd);
            }

            if(ev.events & EPOLLOUT) {
                pendingWrites.insert(ev.data.fd);
            }
        }
    }

    void prepareForCopyover() {
        // This must iterate through all connections,
        // force them to disable MCCP2 compression, and ensure that all outgoing buffers are
        // fully flushed to the OS.

        std::set<int> toClose;

        for(auto &[connId, conn] : connections) {
            conn->disableCompression();
            if(conn->state == net::ConnectionState::Pending) {
                conn->sendText("\r\nSorry, we are rebooting. Please wait warmly for a few seconds.\r\n");
                toClose.insert(connId);
            }
            if(conn->state == net::ConnectionState::Dead) {
                toClose.insert(connId);
            }
        }

        // Ensure all pending data is sent out.
        while(!pendingOutData.empty()) {
            for(auto &connId : pendingOutData) {
                auto conn = connections.at(connId);
                conn->writeToSocket();
            }
            if(!pendingOutData.empty())
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // Close the pending connections.
        for(auto &id : toClose) {
            net::connections.erase(id);
        }

    }

}