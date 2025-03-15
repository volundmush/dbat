#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>
#include <regex>

#include <cppcodec/base64_rfc4648.hpp>
#include <boost/algorithm/string.hpp>

#include "dbat/net.h"
#include "dbat/utils.h"
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"
#include "dbat/account.h"
#include "dbat/accmenu.h"
#include "dbat/charmenu.h"
#include "dbat/chargen.h"
#include "dbat/puppet.h"

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {
    std::unordered_map<int, std::shared_ptr<Connection>> connections;
    std::unordered_map<int, DisconnectReason> deadConnections;

    static int nextConnID() {
        int checking = 1;
        while(connections.contains(checking)) {
            checking++;
        }
        return checking;
    };

    int server_fd = -1;

    using base64 = cppcodec::base64_rfc4648;

    // Overload for std::vector<uint8_t>
    std::string encodeBase64(const std::vector<uint8_t>& data) {
        return base64::encode(data);
    }

    // Overload for std::string
    std::string encodeBase64(const std::string& data) {
        return base64::encode(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    std::vector<uint8_t> decodeBase64(const std::string& base64Str) {
        return base64::decode<std::vector<uint8_t>>(base64Str);
    }

    // Optionally, you can also provide a decode overload for std::string if needed
    std::string decodeBase64ToString(const std::string& base64Str) {
        auto decodedData = base64::decode<std::vector<uint8_t>>(base64Str);
        return std::string(decodedData.begin(), decodedData.end());
    }

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

        if(j.contains("colorType")) colorType = static_cast<ColorType>(j.at("colorType").get<int>());

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
        if(j.contains("mslp")) mslp = j["mslp"];
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
        if(mslp) j["mslp"] = mslp;

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

    nlohmann::json SendBuffer::serialize() {
        nlohmann::json j;
        j["data"] = encodeBase64(data);
        j["sent"] = sent;
        j["bitflags"] = bitflags;
        return j;
    }

    SendBuffer::SendBuffer(const nlohmann::json& j) {
        std::string dataStr = j["data"];
        if(j.contains("data")) data = decodeBase64ToString(dataStr);
        if(j.contains("sent")) sent = j["sent"];
        if(j.contains("bitflags")) bitflags = j["bitflags"];
    }

    void ConnectionParser::close() {

    }

    void ConnectionParser::sendText(const std::string &txt, int bitflags) {
        conn->sendText(txt, bitflags);
    }

    void ConnectionParser::sendGMCP(const std::string &cmd, const nlohmann::json& j, int bitflags) {
        conn->sendGMCP(cmd, j, bitflags);
    }

    void ConnectionParser::start() {

    }

    void ConnectionParser::handleGMCP(const std::string &txt, const nlohmann::json &j) {

    }

    nlohmann::json ConnectionParser::serialize() {
        nlohmann::json j;
        j["parserName"] = getName();
        return j;
    }

    void ConnectionParser::deserialize(const nlohmann::json& j) {

    }

    bool ConnectionParser::canCopyover() {
        return false;
    }

    void ConnectionParser::update(double deltaTime) {

    }



    void Connection::sendText(const std::string &text, int bitflags) {
        auto colored = processColors(text, true, nullptr);
        telnet_send_text(teldata, colored.c_str(), colored.size());
        // telnet_send_text will use outbuf.emplace_back() so to apply the
        // bitflags we need to do it immediately afterwards.
        auto &latest = outbuf.back();
        latest.bitflags = bitflags;
    }

    void Connection::sendGMCP(const std::string &cmd, const nlohmann::json &j, int bitflags) {

        // telnet_send_text will use outbuf.emplace_back() so to apply the
        // bitflags we need to do it immediately afterwards.
        auto &latest = outbuf.back();
        latest.bitflags = bitflags;
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
        if(!fdClosed) {
            ::close(socket);
            fdClosed = true;
        }
    }

    static const telnet_telopt_t my_telopts[] = {
            { TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DO },
            { TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DONT   },
            { TELNET_TELOPT_COMPRESS3, TELNET_WILL, TELNET_DONT   },
            { TELNET_TELOPT_MSSP,      TELNET_WILL, TELNET_DONT   },
            { TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DO },
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
        if(parser) {
            parser->update(deltaTime);
        }
        while(parser && !pendingCommands.empty()) {
            parser->parse(pendingCommands.front());
            pendingCommands.pop_front();
        }
    }

    void Connection::handleGMCP(char* cmd, char *data) {

    }

    void Connection::handleAppData() {
        lastReceivedBytes = std::chrono::steady_clock::now();

        // Find the first occurrence of "\r\n" in the buffer
        auto it = appbuf.find("\r\n");

        while (it != std::string::npos) {
            // Extract the command up to the "\r\n"
            std::string command = appbuf.substr(0, it);

            // Append the command to pendingCommands
            pendingCommands.push_back(std::move(command));

            // Remove the processed command and "\r\n" from appbuf
            appbuf.erase(0, it + 2);

            // Look for the next "\r\n" in the remaining buffer
            it = appbuf.find("\r\n");
        }

        // If no more complete commands are found, the remaining data in appbuf is left as-is
    }

    void Connection::handleTelnetCommand(char cmd) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetWill(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();

        switch(telopt) {
            case TELNET_TELOPT_NAWS:
                capabilities.naws = true;
                break;
            case TELNET_TELOPT_TTYPE:
                capabilities.ttype = true;
                telnet_ttype_send(teldata);
                break;
        }

    }

    void Connection::handleTelnetDo(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();

        switch(telopt) {
            default:
                break;
        }

    }

    void Connection::handleTelnetDont(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();

        switch(telopt) {
            default:
                break;
        }
    }

    void Connection::handleTelnetWont(char telopt) {
        lastReceivedBytes = std::chrono::steady_clock::now();
    }

    void Connection::handleTelnetSubNegotiate(char telopt, const char *buffer, size_t size) {
        lastReceivedBytes = std::chrono::steady_clock::now();

        // just ignore anything coming from these, they have special handlers.
        switch(telopt) {
            case TELNET_TELOPT_ZMP:
            case TELNET_TELOPT_NAWS:
            case TELNET_TELOPT_GMCP:
            case TELNET_TELOPT_TTYPE:
            case TELNET_TELOPT_NEW_ENVIRON:
                return;
        }

    }

    static const std::unordered_set<std::string> clientsWithXterm256Support = {
            "ATLANTIS", "CMUD", "KILDCLIENT", "MUDLET", "MUSHCLIENT", "PUTTY", "BEIP", "POTATO", "TINYFUGUE"
    };

    void Connection::handleTelnetTTYPE(unsigned char cmd, const char *data) {
        std::string newData(data);

        // Check if the received TTYPE is the same as the last one
        if (newData == lastTTYPE) {
            return;
        }

        // Update last TTYPE
        lastTTYPE = newData;

        // Process the TTYPE based on the current state
        switch (ttypeState) {
            case 0:
                // Some clients respond with <name> <version>
                // Split the data into name and version if applicable
            {
                size_t spacePos = newData.find(' ');
                if (spacePos != std::string::npos) {
                    capabilities.clientName = newData.substr(0, spacePos);
                    capabilities.clientVersion = newData.substr(spacePos + 1);
                } else {
                    capabilities.clientName = newData;
                }

                // Anything that answers TTYPE always at least supports ANSI.
                if(capabilities.colorType < ColorType::Standard)
                    capabilities.colorType = ColorType::Standard;

                std::string upperStr = capabilities.clientName;
                std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

                if(clientsWithXterm256Support.contains(upperStr)) {
                    if(capabilities.colorType < ColorType::Xterm256)
                        capabilities.colorType = ColorType::Xterm256;
                }
            }
                break;

            case 1:
                // Terminal type information
                // The client should report one of the following: DUMB, ANSI, VT100, XTERM
                if (newData == "DUMB") {
                    if(capabilities.colorType < ColorType::NoColor)
                        capabilities.colorType = ColorType::NoColor;
                } else if (newData == "ANSI") {
                    if(capabilities.colorType < ColorType::Standard)
                        capabilities.colorType = ColorType::Standard;
                } else if (newData == "VT100") {
                    if(capabilities.colorType < ColorType::Standard)
                        capabilities.colorType = ColorType::Standard;
                    capabilities.vt100 = true;
                } else if (newData == "XTERM") {
                    if(capabilities.colorType < ColorType::Xterm256)
                        capabilities.colorType = ColorType::Xterm256;
                    capabilities.vt100 = true;
                    capabilities.mouse_tracking = true;
                } else if (newData == "ANSI-256COLOR" || newData == "VT100-256COLOR" || newData == "XTERM-256COLOR") {
                    if(capabilities.colorType < ColorType::Xterm256)
                        capabilities.colorType = ColorType::Xterm256;
                } else if (newData.find("-TRUECOLOR") != std::string::npos) {
                    if(capabilities.colorType < ColorType::TrueColor)
                        capabilities.colorType = ColorType::TrueColor;
                    capabilities.osc_color_palette = true;
                }
                break;
            case 2:
                // MTTS information
                if (newData.find("MTTS") == 0) {
                    int mttsCapabilities = std::stoi(newData.substr(5));
                    capabilities.gmcp = mttsCapabilities & 1;
                    capabilities.vt100 = mttsCapabilities & 2;
                    capabilities.utf8 = mttsCapabilities & 4;
                    if(mttsCapabilities & 8 && capabilities.colorType < ColorType::Xterm256)
                        capabilities.colorType = ColorType::Xterm256;
                    capabilities.mouse_tracking = mttsCapabilities & 16;
                    capabilities.osc_color_palette = mttsCapabilities & 32;
                    capabilities.screen_reader = mttsCapabilities & 64;
                    capabilities.proxy = mttsCapabilities & 128;
                    if(mttsCapabilities & 256 && capabilities.colorType < ColorType::TrueColor)
                        capabilities.colorType = ColorType::TrueColor;
                    capabilities.mnes = mttsCapabilities & 512;
                    capabilities.mslp = mttsCapabilities & 1024;
                    capabilities.tls = mttsCapabilities & 2048;
                }
                break;

                // Add more cases as needed to handle further TTYPE negotiations
        }

        // Move to the next state
        ttypeState++;
        if(ttypeState < 3)
            telnet_ttype_send(teldata);
    }

    void Connection::handleTelnet(telnet_event_t *event) {
        switch(event->type) {
            case TELNET_EV_DATA: {
                appbuf += std::string(event->data.buffer, event->data.size);
                handleAppData();
            }
                break;
            case TELNET_EV_SEND: {
                outbuf.emplace_back(std::string(event->data.buffer, event->data.size));
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
                handleTelnetTTYPE(event->ttype.cmd, event->ttype.name);
                break;
            case TELNET_EV_ENVIRON:
                break;
            case TELNET_EV_MSSP:
                break;
            case TELNET_EV_GMCP:
                handleGMCP(event->gmcp.cmd, event->gmcp.data);
                break;
            case TELNET_EV_NAWS:
                capabilities.width = event->naws.width;
                capabilities.height = event->naws.height;
                break;
            case TELNET_EV_WARNING:
                break;
            case TELNET_EV_ERROR:
                break;
        }
    }

    Connection::Connection(int connId, int socket) : connId(connId), socket(socket) {
        teldata = telnet_init(my_telopts, &telnet_handler, TELNET_FLAG_ROLE_SERVER, this);
    }

    nlohmann::json Connection::serialize() {
        nlohmann::json j;

        j["socket"] = socket;
        j["connId"] = connId;
        j["capabilities"] = capabilities.serialize();
        if(account) j["account"] = account->vn;
        if(adminLevel) j["adminLevel"] = adminLevel;
        // We don't serialize descriptors, since the descriptor will rebuild that.

        j["connected"] = std::chrono::duration_cast<std::chrono::nanoseconds>(connected.time_since_epoch()).count();
        j["lastActivity"] = std::chrono::duration_cast<std::chrono::nanoseconds>(lastActivity.time_since_epoch()).count();
        j["lastReceivedBytes"] = std::chrono::duration_cast<std::chrono::nanoseconds>(lastReceivedBytes.time_since_epoch()).count();

        for(auto &cmd : pendingCommands) {
            j["pendingCommands"].push_back(cmd);
        }

        if(parser) {
            j["parser"] = parser->serialize();
        }

        j["state"] = static_cast<int>(state);

        // Serialize the deque<SendBuffer>
        j["outbuf"] = nlohmann::json::array();
        for (auto& buffer : outbuf) {
            j["outbuf"].push_back(buffer.serialize());
        }

        if(!appbuf.empty()) j["appbuf"] = encodeBase64(appbuf);
        if(!lastTTYPE.empty()) j["lastTTYPE"] = lastTTYPE;
        if(ttypeState) j["ttypeState"] = ttypeState;

        if(teldata) {
            nlohmann::json t;

            std::string tbuf(teldata->buffer, teldata->buffer_pos);
            if(teldata->buffer_pos > 0) {
                t["buffer"] = encodeBase64(tbuf);
                t["buffer_pos"] = teldata->buffer_pos;
                t["buffer_size"] = teldata->buffer_size;
            }

            for(auto i = 0; i < teldata->q_cnt; i++) {
                t["q"].push_back(std::make_pair(teldata->q[i].telopt, teldata->q[i].state));
            }

            t["state"] = teldata->state;
            if(teldata->flags) t["flags"] = teldata->flags;
            t["sb_telopt"] = teldata->sb_telopt;

            j["teldata"] = t;
        }

        return j;
    }

    void Connection::deserialize(const nlohmann::json &j) {
        capabilities.deserialize(j.at("capabilities"));
        if (j.contains("account")) {
            auto &acc = accounts[j.at("account").get<int>()];
            account = &acc;
        }
        if (j.contains("adminLevel")) adminLevel = j.at("adminLevel").get<int>();

        connected = std::chrono::system_clock::time_point(std::chrono::nanoseconds(j.at("connected").get<int64_t>()));
        lastActivity = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(j.at("lastActivity").get<int64_t>()));
        lastReceivedBytes = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(j.at("lastReceivedBytes").get<int64_t>()));

        if(j.contains("pendingCommands"))
            for (const auto& cmd : j.at("pendingCommands"))
                pendingCommands.emplace_back(cmd.get<std::string>());

        state = static_cast<ConnectionState>(j.at("state").get<int>());

        if(j.contains("outbuf"))
            for (const auto& bufferJson : j.at("outbuf"))
                outbuf.emplace_back(bufferJson);

        if(j.contains("appbuf")) {
            std::string appbufData = j.at("appbuf").get<std::string>();
            appbuf = decodeBase64ToString(appbufData);
        }

        if(j.contains("parser")) {
            auto p = j["parser"];
            auto pname = p.at("parserName").get<std::string>();
            auto sh = shared_from_this();

            if(pname == "AccountMenu") {
                parser = std::make_unique<AccountMenu>(sh);
            } else if(pname == "ChargenParser") {
                parser = std::make_unique<ChargenParser>(sh);
            } else if(pname == "CharacterMenu") {
                parser = std::make_unique<CharacterMenu>(sh, nullptr);
            } else if(pname == "LoginParser") {
                parser = std::make_unique<LoginParser>(sh);
            } else if(pname == "PuppetParser") {
                parser = std::make_unique<PuppetParser>(sh, nullptr);
            }
            parser->deserialize(p);
            //parser->parse("");

        } else {
            // This really, really should not happen.
            // But if it did, let's set them up with a login.
            onWelcome();
        }

        if(j.contains("teldata")) {
            auto t = j["teldata"];
            if(t.contains("buffer")) {
                std::string tbuf = decodeBase64ToString(t.at("buffer").get<std::string>());
                if(teldata->buffer) {
                    free(teldata->buffer);
                    teldata->buffer = (char*)calloc(1, tbuf.size());
                    memcpy(teldata->buffer, tbuf.c_str(), tbuf.size());
                    teldata->buffer_size = tbuf.size();
                    teldata->buffer_pos = t.at("buffer_pos").get<size_t>();
                }
            }

            if(t.contains("q")) {
                auto q = t["q"];
                int i = 0;
                if(teldata->q) free(teldata->q);
                teldata->q = (telnet_rfc1143_t*)calloc(sizeof(telnet_rfc1143_t), q.size());
                teldata->q_cnt = q.size();
                teldata->q_size = sizeof(telnet_rfc1143_t) * q.size();
                for(const auto &qop : q) {
                    teldata->q[i].telopt = qop[0].get<unsigned char>();
                    teldata->q[i].state = qop[1].get<unsigned char>();
                    i++;
                }
            }

            if(t.contains("flags")) {
                teldata->flags = t.at("flags").get<int>();
            }

            if(t.contains("sb_telopt")) {
                teldata->sb_telopt = t.at("sb_telopt").get<unsigned char>();
            }

            if(t.contains("state")) {
                teldata->state = static_cast<telnet_state_t>(t.at("state").get<int>());
            }
        }
    }

    Connection::~Connection() {
        // Free the telnet data.
        telnet_free(teldata);
        // Close the descriptor.
        if(!fdClosed)
            ::close(socket);
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
        for (p = res; p; p = p->ai_next) {
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


    void init_listeners() {
        // server_fd might already be set to a proper port by the copyover process.
        if(server_fd == -1) {
            // This is a fresh start. We need to create and bind a server to config::hostAddress config::port and set
            // the fd to server_fd
            server_fd = create_listener(config::hostAddress, config::port);
        }
    }

    void Connection::readFromSocket() {
        if(state == ConnectionState::Dead || fdClosed) {
            return;
        }

        std::vector<char> buffer(4096);

        while (true) {
            ssize_t bytesRead = read(socket, buffer.data(), buffer.size());
            if (bytesRead > 0) {
                // Successfully read data, process it
                telnet_recv(teldata, buffer.data(), bytesRead);
            } else if (bytesRead == 0) {
                // Connection closed by the client
                basic_mud_log("Connection %d fd %d closed by client.", connId, socket);
                onNetworkDisconnected();
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more data available right now, would block
                    break;
                } else if(errno == ETIMEDOUT) {
                    // Connection timed out
                    basic_mud_log("Connection %d fd %d timed out.", connId, socket);
                    onNetworkDisconnected();
                    break;
                } else {
                    // An error occurred, log or handle it
                    basic_mud_log("Connection %d fd %d read error: %s", connId, socket, strerror(errno));
                    onNetworkDisconnected();
                    break;
                }
            }
        }
    }


    void Connection::writeToSocket() {
        if (state == ConnectionState::Dead || fdClosed) {
            return;
        }

        // Iterate over the buffer's data sequence and attempt to send it
        while (!outbuf.empty()) {
            // Access the front data sequence in the buffer.
            auto &sbuf = outbuf.front();
            int remaining = sbuf.data.size() - sbuf.sent;

            if (remaining > 0) {
                // Attempt to send the data
                ssize_t bytesSent = send(socket, sbuf.data.c_str() + sbuf.sent, remaining, 0);

                if (bytesSent == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Socket is not ready to send more data; stop and wait for EPOLLOUT
                        break;
                    } else {
                        // An error occurred, log or handle it
                        std::cerr << "Send error: " << strerror(errno) << std::endl;
                        onNetworkDisconnected();
                        break;
                    }
                }

                // Consume the sent data from the buffer
                sbuf.sent += bytesSent;
                remaining -= bytesSent;
            }

            if (remaining == 0) {
                if (sbuf.bitflags & SendBuffer::BF_CLOSE_AFTER_SEND) {
                    outbuf.clear();
                    close();
                } else {
                    outbuf.pop_front();
                }
            }
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
        auto id = nextConnID();
        auto conn = std::make_shared<Connection>(id, conn_fd);
        connections[id] = conn;

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
        
    }

    void prepareForCopyover() {
        // This must iterate through all connections,
        // kick any connections that can't be safely serialized, and ensure that all outgoing buffers are
        // fully flushed to the OS.

        std::unordered_set<int> toClose;

        for(auto &[connId, conn] : connections) {
            if(conn->state == net::ConnectionState::Pending || conn->parser && !conn->parser->canCopyover()) {
                conn->sendText("\r\nSorry, we are rebooting. This may take a minute or two. Please reconnect in a bit.\r\n", SendBuffer::BF_CLOSE_AFTER_SEND);
                toClose.insert(connId);
            }
            // There shouldn't be any dead connections here, but we'll check anyway.
            if(conn->state == net::ConnectionState::Dead) {
                toClose.insert(connId);
            }
        }

        // Ensure all pending data is sent out.
        auto start = std::chrono::steady_clock::now();
    
        while(true) {
            for(auto &[id, conn] : connections) {
                conn->writeToSocket();
            }

            // Check if 200 milliseconds have passed
            if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(200)) {
                break;
            }
        }

        // Close the pending and any remaining connections.
        for(auto &id : toClose) {
            net::connections.erase(id);
        }
        for(auto const &[id, reason] : deadConnections) {
            net::connections.erase(id);
        }

    }

}