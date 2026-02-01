#include <filesystem>
#include <fstream>
#include <sstream>
#include "dbat/portal/Client.hpp"
#include "boost/algorithm/string.hpp"
#include "volcano/circle/CircleAnsi.hpp"
#include "volcano/log/Log.hpp"

namespace dbat::portal {

    boost::asio::awaitable<std::optional<volcano::portal::JwtTokens>> refresh_jwt(volcano::portal::Client& client) {
        co_return std::optional<volcano::portal::JwtTokens>();
    }

    // CircleModeHandler implementation would go here
    boost::asio::awaitable<void> CircleModeHandler::sendCircleLine(std::string_view line) {
        auto color = static_cast<volcano::ansi::ColorMode>(client_.clientData().color);

        auto process = volcano::circle::processColors(line, color);

        co_await client_.sendLine(process);
        co_return;
    }

    boost::asio::awaitable<void> CircleModeHandler::sendCircleText(std::string_view text) {
        auto color = static_cast<volcano::ansi::ColorMode>(client_.clientData().color);

        auto process = volcano::circle::processColors(text, color);

        co_await client_.sendLine(process);
        co_return;
    }

    // AccountMode implementation would go here

    boost::asio::awaitable<void> AccountMode::enterMode() {

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
        co_return;
    }

    boost::asio::awaitable<std::vector<std::pair<std::string, std::string>>> AccountMode::getMSSPData() {
        std::vector<std::pair<std::string, std::string>> mssp_msg;

        auto req = client_.createBaseRequest(boost::beast::http::verb::get, "/v1/mssp");
            auto &http = client_.httpClient();
            auto res = co_await http.request(req);
            if(!res) {
                LERROR("Failed to fetch MSSP data: {}", res.error());
                co_return mssp_msg;
            }

            auto &response = *res;
            // if we got a 200, we should have a JSON body with MSSP data.
            if(response.result() == boost::beast::http::status::ok) {
                try {
                    auto j = nlohmann::json::parse(response.body());
                    if(j.is_array()) {
                        j.get_to(mssp_msg);
                    }
                } catch (const std::exception& e) {
                    LERROR("Failed to parse MSSP response JSON: {}", e.what());
                }   
            }

        co_return mssp_msg;
    }

    boost::asio::awaitable<void> AccountMode::runImpl() {
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

    boost::asio::awaitable<void> AccountMode::handleCommand(const std::string& data) {
        auto cmd = volcano::mud::CommandData(data);
        LINFO("AccountMode received command: {}", cmd);
        if(boost::iequals(cmd.cmd, "quit")) {
            co_await cmdQuit(cmd);
        } else if(boost::iequals(cmd.cmd, "mssp")) {
            co_await cmdMSSP(cmd);
        } else if(boost::iequals(cmd.cmd, "login")) {
            co_await cmdLogin(cmd);
        } else if(boost::iequals(cmd.cmd, "register")) {
            co_await cmdRegister(cmd);
        } else if(boost::equals(cmd.cmd, "IDLE")) {
            // do nothing...
        } else {
            co_await sendCircleLine("Unknown command: " + data);
        }
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdQuit(volcano::mud::CommandData& cmd) {
        co_await sendCircleLine("Goodbye!");
        requestCancel();
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdMSSP(volcano::mud::CommandData& cmd) {
        auto mssp_data = co_await getMSSPData();
        co_await sendCircleLine("MSSP Data:");
        for (const auto& [key, value] : mssp_data) {
            co_await sendCircleLine("  " + key + ": " + value);
        }
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdLogin(volcano::mud::CommandData& cmd) {
        if (cmd.rsargs.empty() | cmd.lsargs.empty()) {
            co_await sendCircleLine("Usage: login <username>=<password>");
            co_return;
        }

        auto username = boost::trim_copy(cmd.lsargs);
        auto password = boost::trim_copy(cmd.rsargs);

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
            bool error = false;
            try {
                auto j = nlohmann::json::parse(response.body());
                volcano::portal::JwtTokens tokens;
                tokens.jwt = j.at("access_token").get<std::string>();
                tokens.refresh = j.at("refresh_token").get<std::string>();
                tokens.expires_in = boost::asio::chrono::seconds(j.at("expires_in").get<int>());
                client_.tokens = tokens;
                //co_await sendCircleLine("Login successful. Welcome, " + j.at("username").get<std::string>() + "!");
            } catch (const std::exception& e) {
                LERROR("Failed to parse login response JSON: {}", e.what());
                error = true;
            }
            if(error) {
                co_await sendCircleLine("Login failed: invalid server response.");
            }
        } else {
            co_await sendCircleLine("Login failed: " + response.body());
        }

        co_return;
    }

    boost::asio::awaitable<void> AccountMode::cmdRegister(volcano::mud::CommandData& cmd) {
        if (cmd.rsargs.empty() || cmd.lsargs.empty()) {
            co_await sendCircleLine("Usage: register <username>=<password>");
            co_return;
        }

        auto username = boost::trim_copy(cmd.lsargs);
        auto password = boost::trim_copy(cmd.rsargs);

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

    boost::asio::awaitable<void> AccountMode::handleGMCP(const std::string& package, const nlohmann::json& data) {
        LINFO("AccountMode received GMCP package: {} with data: {}", package, data.dump());
        co_await sendCircleLine("Received GMCP package: " + package + " with data: " + data.dump());

        co_return;
    }

    // Other implementations below this

}