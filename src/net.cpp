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

    std::map<int64_t, std::shared_ptr<Connection>> connections;
    std::set<int64_t> pendingConnections;

    std::set<int64_t> deadConnections;

    void Connection::sendGMCP(const std::string &cmd, const nlohmann::json &j) {

        nlohmann::json out;
        out["cmd"] = cmd;
        out["data"] = j;
        out["type"] = "Game.GMCP";

        outQueue.emplace_back(jdump(out));
    }

    void Connection::sendText(const std::string &text) {
        if(text.empty()) return;

        nlohmann::json out;
        out["data"] = text;
        out["Type"] = "Game.LegacyText";

        outQueue.emplace_back(jdump(out));

    }


    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        deadConnections.insert(connId);
    }

    void Connection::onNetworkDisconnected() {
        deadConnections.insert(connId);
    }


    Connection::Connection(int64_t connId)
    : connId(connId) {

    }

    void Connection::cleanup() {
        if (account) {
            // Remove ourselves from the account's connections list.
            account->connections.erase(this);
            account = nullptr;
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

    void Connection::setParser(ConnectionParser *p) {
        if(parser) parser->close();
        parser.reset(p);
        p->start();
    }

    void Connection::onHeartbeat(double deltaTime) {
        if(inQueue.empty()) return;

        std::string msg = inQueue.front();
        inQueue.pop_front();

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(msg);
        }
        catch(std::exception &err) {
            logger->error("Connection {} received un-parseable input: {}",
                          connId, msg);
            return;
        }
        handleMessage(j);

    }

    void Connection::handleGMCP(const std::string &cmd, const nlohmann::json &j) {
        if(parser) parser->handleGMCP(cmd, j);
    }

    void Connection::handleCommand(const std::string &cmd) {
        if(parser) parser->parse(cmd);
    }

    void Connection::handleMessage(const nlohmann::json &j) {
        if(!j.contains("type")) {
            logger->error("Connection {} received malformed JSON: {}",
                          connId, jdump(j));
            return;
        }

        auto msgType = j["type"].get<std::string>();
        if(boost::equals(msgType, "Game.Command")) {
            if(!j.contains("data")) {
                logger->error("Connection {} received malformed Game.Command JSON: {}",
                              connId, jdump(j));
                return;
            }
            auto cmd = j["data"].get<std::string>();
            handleCommand(cmd);
        } else if(boost::equals(msgType, "Game.GMCP")) {
            if(!(j.contains("cmd") && j.contains("data"))) {
                logger->error("Connection {} received malformed Game.Command JSON: {}",
                              connId, jdump(j));
                return;
            }
            auto cmd = j["cmd"].get<std::string>();
            auto j2 = j["data"];
            handleGMCP(cmd, j2);
        }
    }

    std::shared_ptr<Connection> newConnection() {
        int64_t connId = 0;
        while(connections.contains(connId)) connId++;
        auto out = std::make_shared<Connection>(connId);
        connections[connId] = out;
        pendingConnections.insert(connId);
        return out;
    }

}