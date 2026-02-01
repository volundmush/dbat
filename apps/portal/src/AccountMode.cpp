#include <filesystem>
#include <fstream>
#include <sstream>
#include "dbat/portal/Client.hpp"
#include "boost/algorithm/string.hpp"
#include "volcano/jwt/jwt.hpp"
#include "volcano/log/Log.hpp"
#include "volcano/net/net.hpp"
#include "volcano/mud/ClientDataSave.hpp"

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace dbat::portal {
    boost::asio::awaitable<void> AccountMode::enterMode() {
        co_await displayMenu();
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::displayMenu() {
        auto jwt_payload_res = getJwtPayload();
        if(!jwt_payload_res) {
            co_await sendCircleLine("Error: Invalid session token. Please log in again.");
            requestCancel();
            co_return;
        }

        auto &j = *jwt_payload_res;
        auto username = j.at(+"username").get<std::string>();
        
        auto character_res = co_await getCharacterList();
        if(!character_res) {
            co_await sendCircleLine("Error fetching character list: " + character_res.error());
            requestCancel();
            co_return;
        }

        auto &characters = *character_res;
        
        std::ostringstream menu;
        menu << "Welcome, @W" << username << "@n!\r\n";
        menu << "Account Menu:\r\n";

        if(!characters.empty()) {
            menu << "@WYour Characters:@n\r\n";
            for(const auto& character : characters) {
                menu << " - " << character.name << "\r\n";
            }
        } else {
            menu << "You have no characters. Enter @Wchargen@n to create one.\r\n";
        }
        menu << "\r\n";
        menu << "Available Commands:\r\n";
        menu << " - @Wchargen@n : Create a new character\r\n";
        menu << " - @Wdelete <character name>@n : Delete an existing character\r\n";
        menu << " - @Wplay <character name>@n : Enter play as an existing character\r\n";
        menu << " - @Wlogout@n : Log out of your account\r\n";
        menu << " - @WQUIT@n : Disconnect from the server\r\n";

        co_await sendCircleText(menu.str());
        co_return;
    }


    boost::asio::awaitable<void> AccountMode::handleCommand(const std::string& data) {
        auto cmd = volcano::mud::CommandData(data);
        if(boost::iequals(cmd.cmd, "chargen")) {
            co_await cmdChargen(cmd);
        } else if(boost::iequals(cmd.cmd, "delete")) {
            co_await cmdDelete(cmd);
        } else if(boost::iequals(cmd.cmd, "play")) {
            co_await cmdPlay(cmd);
        } else if(boost::iequals(cmd.cmd, "logout")) {
            co_await cmdLogout(cmd);
        } else if(boost::iequals(cmd.cmd, "quit")) {
            co_await cmdQuit(cmd);
        } else if(boost::iequals(cmd.cmd, "look")) {
            co_await displayMenu();
        } else {
            co_await sendCircleLine("Unknown command. Please choose from the available commands.");
        }
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdChargen(volcano::mud::CommandData& cmd) {
        co_await sendCircleLine("Character creation is not yet implemented.");
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdDelete(volcano::mud::CommandData& cmd) {
        auto character_name = boost::trim_copy(cmd.lsargs);
        auto character_res = co_await getCharacterByName(character_name);
        if(!character_res) {
            co_await sendCircleLine("Error: " + character_res.error());
            co_return;
        }
        auto& character = *character_res;

        co_await sendCircleLine("Character deletion is not yet implemented.");
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdPlay(volcano::mud::CommandData& cmd) {
        auto character_name = boost::trim_copy(cmd.argument);
        auto character_res = co_await getCharacterByName(character_name);
        if(!character_res) {
            co_await sendCircleLine("Error: " + character_res.error());
            co_return;
        }
        auto& character = *character_res;

        co_await sendCircleLine("Starting play session for character: " + character.name);

        auto character_id = character.id;

        auto target_path = "/v1/play?character_id=" + std::to_string(character_id);
        auto req = client_.createAuthenticatedRequest(boost::beast::http::verb::get, target_path);

        // Transition to PlayMode with the selected character
        auto address = volcano::portal::target.address;
        auto port = volcano::portal::target.port;

        volcano::net::ConnectOptions options;
        if (volcano::portal::target.scheme == volcano::web::HttpScheme::https) {
            options.transport = volcano::net::TransportMode::tls;
            options.verify_peer = false;
        } else {
            options.transport = volcano::net::TransportMode::tcp;
        }

        auto stream_res = co_await volcano::net::connect_any(address, port, options);
        if(!stream_res) {
            co_await sendCircleLine("Error connecting to play server: " + stream_res.error().message());
            co_return;
        }

        nlohmann::json client_info_json;
        to_json(client_info_json, client_.clientData());

        auto client_64 = volcano::jwt::base64UrlEncode(client_info_json.dump());

        volcano::web::WebSocketStream ws(std::move(*stream_res));

        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [&](boost::beast::websocket::request_type& ws_req) {
                ws_req.set(boost::beast::http::field::host, volcano::portal::target.host());
                ws_req.set(boost::beast::http::field::user_agent, "dbat-portal/1.0");
                ws_req.set(boost::beast::http::field::x_forwarded_for, client_.link().address.to_string());
                if (client_.tokens) {
                    ws_req.set(boost::beast::http::field::authorization, "Bearer " + client_.tokens->jwt);
                }
                ws_req.set("X-Client-Info", client_64);
            }));

        boost::system::error_code ws_ec;
        co_await ws.async_handshake(volcano::portal::target.host(), target_path,
            boost::asio::redirect_error(boost::asio::use_awaitable, ws_ec));
        if (ws_ec) {
            co_await sendCircleLine("Error establishing play websocket: " + ws_ec.message());
            co_return;
        }

        co_await requestMode(std::make_shared<PlayMode>(client_, std::move(ws)));

        co_return;
    }
}