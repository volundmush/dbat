#include <filesystem>
#include <fstream>
#include <sstream>
#include "dbat/portal/Client.hpp"
#include "boost/algorithm/string.hpp"
#include "volcano/circle/CircleAnsi.hpp"
#include "volcano/log/Log.hpp"

namespace dbat::portal {

    boost::asio::awaitable<void> LoginMode::enterMode() {

        static std::string cached_screen;
        static bool cached_screen_loaded = false;

        if (!cached_screen_loaded) {
            auto greet_path = std::filesystem::current_path() / "data" / "text" / "greetansi";
            if (std::filesystem::exists(greet_path) && std::filesystem::is_regular_file(greet_path)) {
                std::ifstream greet_file(greet_path, std::ios::in | std::ios::binary);
                std::ostringstream buffer;
                buffer << greet_file.rdbuf();
                cached_screen = buffer.str();
            }
            cached_screen_loaded = true;
        }

        // now we get what color mode to use...
        co_await sendCircleLine(cached_screen);

        std::ostringstream menu;
        menu << " - @Wregister <username>=<password>@n : Register a new account\r\n";
        menu << " - @Wlogin <username>=<password>@n : Login to an existing account\r\n";
        co_await sendCircleText(menu.str());
        
        co_return;
    }

    boost::asio::awaitable<void> LoginMode::runImpl() {
        auto &cd = client_.clientData();
        if(cd.mssp) { // the connected client supports and requested MSSP, so we'll send it.
            auto mssp_msg = co_await getMSSPData();
            co_await client_.sendMSSP(mssp_msg);
        }
        // and then we continue with the normal CircleModeHandler runImpl which by default
        // just waits forever.
        co_await volcano::portal::ModeHandler::runImpl();
        co_return;
    }

    boost::asio::awaitable<void> LoginMode::handleCommand(const std::string& data) {
        auto cmd = volcano::mud::CommandData(data);
        if(boost::equals(cmd.cmd, "IDLE")) {
            co_return;
        }
        
        if(boost::iequals(cmd.cmd, "quit")) {
            co_await cmdQuit(cmd);
        } else if(boost::iequals(cmd.cmd, "mssp")) {
            co_await cmdMSSP(cmd);
        } else if(boost::iequals(cmd.cmd, "login")) {
            co_await cmdLogin(cmd);
        } else if(boost::iequals(cmd.cmd, "register")) {
            co_await cmdRegister(cmd);
        } else {
            co_await sendCircleLine("Unknown command: " + data);
        }
        co_return;
    }

    boost::asio::awaitable<void> LoginMode::cmdMSSP(volcano::mud::CommandData& cmd) {
        auto mssp_data = co_await getMSSPData();
        co_await sendCircleLine("MSSP Data:");
        for (const auto& [key, value] : mssp_data) {
            co_await sendCircleLine("  " + key + ": " + value);
        }
        co_return;
    }

    boost::asio::awaitable<void> LoginMode::cmdLogin(volcano::mud::CommandData& cmd) {
        if (cmd.rsargs.empty() || cmd.lsargs.empty()) {
            co_await sendCircleLine("Usage: login <username>=<password>");
            co_return;
        }

        auto username = boost::trim_copy(cmd.lsargs);
        auto password = boost::trim_copy(cmd.rsargs);

        if(username.empty() || password.empty()) {
            co_await sendCircleLine("Username and password cannot be empty.");
            co_return;
        }

        nlohmann::json login_json;
        login_json["username"] = username;
        login_json["password"] = password;

        auto req = client_.createJsonRequest(boost::beast::http::verb::post, "/v1/auth/login", login_json);
        auto &http = client_.httpClient();
        auto res = co_await http.request(req);
        if(!res) {
            co_await sendCircleLine("Login failed: " + res.error());
            co_return;
        }

        auto &response = *res;
        if(response.result() == boost::beast::http::status::ok) {
            std::optional<std::string> error;
            try {
                auto j = nlohmann::json::parse(response.body());
                volcano::portal::JwtTokens tokens;
                tokens.jwt = j.at("access_token").get<std::string>();
                tokens.refresh = j.at("refresh_token").get<std::string>();
                tokens.expires_in = boost::asio::chrono::seconds(j.at("expires_in").get<int>());
                client_.tokens = tokens;
            } catch (const std::exception& e) {
                error = e.what();
            }
            if(error) {
                co_await sendCircleLine("Login failed: " + *error);
            } else {
                // no error, so login was successful!
                co_await requestMode(std::make_shared<AccountMode>(client_));
            }
        } else {
            co_await sendCircleLine("Login failed: " + response.body());
        }

        co_return;
    }

    boost::asio::awaitable<void> LoginMode::cmdRegister(volcano::mud::CommandData& cmd) {
        if (cmd.rsargs.empty() || cmd.lsargs.empty()) {
            co_await sendCircleLine("Usage: register <username>=<password>");
            co_return;
        }

        auto username = boost::trim_copy(cmd.lsargs);
        auto password = boost::trim_copy(cmd.rsargs);

        if(username.empty() || password.empty()) {
            co_await sendCircleLine("Username and password cannot be empty.");
            co_return;
        }

        nlohmann::json register_json;
        register_json["username"] = username;
        register_json["password"] = password;

        auto req = client_.createJsonRequest(boost::beast::http::verb::post, "/v1/auth/register", register_json);
        auto &http = client_.httpClient();
        auto res = co_await http.request(req);
        if(!res) {
            co_await sendCircleLine("Registration failed: " + res.error());
            co_return;
        }

        auto &response = *res;
        if(response.result() == boost::beast::http::status::created) {
            co_await sendCircleLine("Registration successful. You can now log in with your credentials.");
        } else {
            co_await sendCircleLine("Registration failed: " + response.body());
        }

        co_return;
    }
}