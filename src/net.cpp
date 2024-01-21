#include "dbat/net.h"
#include "dbat/utils.h"
#include <regex>
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {

    std::mutex connectionMutex;
    std::map<int64_t, std::shared_ptr<Connection>> connections;

    void Connection::sendGMCP(const std::string &cmd, const nlohmann::json &j) {
        // default implementation does nothing.
    }

    void Connection::sendText(const std::string &text) {
        // default implementation does nothing.
    }


    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        state = net::ConnectionState::Dead;
    }

    void Connection::onNetworkDisconnected() {
        state = net::ConnectionState::Dead;
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
        // default implementation does nothing.
    }

    void Connection::executeGMCP(const std::string &cmd, const nlohmann::json &j) {
        if(parser) parser->handleGMCP(cmd, j);
    }

    void Connection::executeCommand(const std::string &cmd) {
        if(parser) parser->parse(cmd);
    }

}