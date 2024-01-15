#include "dbat/net.h"
#include "dbat/utils.h"
#include <regex>
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {

    std::mutex connectionsMutex, pendingConnectionsMutex;


    std::map<int64_t, std::shared_ptr<Connection>> connections;
    std::set<int64_t> pendingConnections;

    std::unordered_map<int64_t, DisconnectReason> deadConnections;

    void Connection::sendGMCP(const std::string &cmd, const nlohmann::json &j) {
        GameMessage msg;
        msg.type = GameMessageType::GMCP;
        msg.data.push_back(cmd);
        msg.data.push_back(j);

    }

    void Connection::sendText(const std::string &text) {
        if(text.empty()) return;
        GameMessage msg;
        msg.type = GameMessageType::Command;

        if(desc && desc->character) {
            auto &p = players[desc->character->id];
            msg.data = processColors(text, COLOR_ON(desc->character), p.color_choices);
        } else {
            msg.data = processColors(text, true, nullptr);
        }

    }


    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        deadConnections[this->connId] = DisconnectReason::GameLogoff;
    }

    void Connection::onNetworkDisconnected() {
        deadConnections[this->connId] = DisconnectReason::ConnectionLost;
    }


    Connection::Connection(int64_t connId)
    : connId(connId) {

    }

    void Connection::cleanup(DisconnectReason reason) {
        if (account) {
            // Remove ourselves from the account's connections list.
            account->connections.erase(this);
            account = nullptr;
        }
        switch (reason) {
            case DisconnectReason::ConnectionLost:
                // The connection to remote host was lost for unknown reasons.
                // The portal is aware of this and already cleaned them up.
                if (desc) {
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
                if (desc) {
                    // If we have a desc then we can attempt a 'quit' on the
                    // connected character. OR however else it wants to handle it.
                    // onConnectionClosed must remove the connection from the descriptor.
                    desc->onConnectionClosed(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::GameLogoff: {
                // The game server is initiating disconnection for some reason or another.
                // The portal must be informed so it can boot the remote host.

                break;
            }
                // We do not remove ourselves from net::connections; that's done by the main loop.
        }
    }

    void Connection::onHeartbeat(double deltaTime) {

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

    void Connection::setParser(ConnectionParser *p) {
        if(parser) parser->close();
        parser.reset(p);
        p->start();
    }

    void Connection::handleMessage(const GameMessage &msg) {
        switch(msg.type) {
            case GameMessageType::Update:
                capabilities.deserialize(msg.data);
                break;
            case GameMessageType::Command: {
                auto cmd = msg.data.get<std::string>();
                if(parser) parser->parse(cmd);
            }
                break;
            case GameMessageType::GMCP:
                if(msg.data.is_array() && msg.data.size() == 2) {
                    auto cmd = msg.data[0].get<std::string>();
                    auto data = msg.data[1];
                    if(parser) parser->handleGMCP(cmd, data);
                }
                break;
            case GameMessageType::Disconnect:
                break;
            case GameMessageType::Timeout:
                break;
        }
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

        if(j.contains("colorType")) colorType = j["colorType"].get<ColorType>();

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

    std::string ProtocolCapabilities::protocolName() {
        switch(protocol) {
            case Protocol::Telnet: return "telnet";
            case Protocol::WebSocket: return "websocket";
            default:
                return "unknown";
        }
    }

    nlohmann::json GameMessage::serialize() const {
        nlohmann::json j;
        j["type"] = type;
        if(!data.empty()) j["data"] = data;

        return j;
    }

    GameMessage::GameMessage(const nlohmann::json& j) : GameMessage() {
        if(j.contains("type")) type = j["type"].get<GameMessageType>();
        if(j.contains("data")) data = j["data"];
    }
}