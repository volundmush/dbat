#include <filesystem>
#include <fstream>
#include <sstream>
#include "dbat/portal/Client.hpp"
#include "boost/algorithm/string.hpp"
#include "volcano/log/Log.hpp"

namespace dbat::portal {

    boost::asio::awaitable<void> PlayMode::sendToWebSocket(const nlohmann::json& j, const std::string& msg_type) {
        ws_.text(true);
        auto payload = j.dump();
        auto buffer = boost::asio::buffer(payload);
        boost::system::error_code ec;
        LINFO("Sending {} over WebSocket: {}", msg_type, payload);
        co_await ws_.async_write(
            buffer,
            boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if(ec) {
            LERROR("Failed to send {} over WebSocket: {}", msg_type, ec.message());
        }
        LINFO("successfully sent {} over WebSocket.", msg_type);
        co_return;
    }

    boost::asio::awaitable<void> PlayMode::handleCommand(const std::string& data) {

        nlohmann::json msg;
        msg["type"] = "text";
        msg["text"] = data;
        co_await sendToWebSocket(msg, "command");

        co_return;
    }

    boost::asio::awaitable<void> PlayMode::handleGMCP(const std::string& package, const nlohmann::json& data) {
        nlohmann::json msg;
        msg["type"] = "gmcp";
        msg["package"] = package;
        msg["data"] = data;
        co_await sendToWebSocket(msg, "GMCP");

        co_return;
    }

    boost::asio::awaitable<void> PlayMode::handleDisconnect() {
        LINFO("PlayMode::handleDisconnect called");
        nlohmann::json msg;
        msg["type"] = "disconnect";
        msg["reason"] = "Client disconnected";
        co_await sendToWebSocket(msg, "disconnect");
        co_return;
    }

    boost::asio::awaitable<void> PlayMode::handleChangeCapabilities(const nlohmann::json& j) {
        nlohmann::json msg;
        msg["type"] = "change_capabilities";
        msg["capabilities"] = j;
        co_await sendToWebSocket(msg, "change_capabilities");
        co_return;
    }



    boost::asio::awaitable<void> PlayMode::runImpl() {
        boost::system::error_code ec;
        boost::beast::flat_buffer buffer;

        while(true) {
            if(cancellation_state_.cancelled() != boost::asio::cancellation_type::none) {
                co_return;
            }

            // now we'll read from websocket...
            
            auto n = co_await ws_.async_read(
                buffer,
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            if(ec) {
                LINFO("WebSocket read error: {}", ec.message());
                co_return;
            }
            LINFO("Received {} bytes from WebSocket.", n);

            if (ws_.got_text())
            {
                auto data_ptr = buffer.data().data();
                auto data_str = std::string_view(static_cast<const char *>(data_ptr), n);
                LINFO("WebSocket text message: {}", data_str);
                nlohmann::json msg = nlohmann::json::parse(data_str, nullptr, false);
                if (msg.is_discarded())
                {
                    LINFO("{} received invalid JSON message from server.", client_);
                }
                else
                {
                    co_await processMessage(msg);
                }
            }
            else
            {
                LINFO("{} received non-text WebSocket message from server. Ignoring.", client_);
            }
            buffer.consume(n);

        }
        co_return;
    }

    boost::asio::awaitable<void> PlayMode::processMessage(const nlohmann::json& j) {
        if(!j.contains("type") || !j["type"].is_string()) {
            LINFO("{} received JSON message without valid type field.", client_);
            co_return;
        }

        std::string type = j["type"].get<std::string>();
        if(boost::iequals(type, "text")) {
            if(!j.contains("text") || !j["text"].is_string()) {
                LINFO("{} received text message without valid text field.", client_);
                co_return;
            }
            std::string text = j["text"].get<std::string>();
            co_await sendCircleText(text);
        } else if(boost::iequals(type, "gmcp")) {
            if(!j.contains("package") || !j["package"].is_string() ||
               !j.contains("data") || !j["data"].is_object()) {
                LINFO("{} received GMCP message without valid package or data field.", client_);
                co_return;
            }
            std::string package = j["package"].get<std::string>();
            nlohmann::json data = j["data"];
            co_await client_.sendGMCP(package, data);
        } else if(boost::iequals(type, "disconnect")) {
            LINFO("PlayMode received disconnect message from server");
            co_await handleDisconnect();
        } else {
            LINFO("{} received unknown message type '{}'.", client_, type);
        }

        co_return;
    }

}