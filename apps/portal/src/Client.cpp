#include <filesystem>
#include <fstream>
#include <sstream>
#include "dbat/portal/Client.hpp"
#include "volcano/mud/Command.hpp"
#include "volcano/circle/CircleAnsi.hpp"
#include "volcano/log/Log.hpp"

namespace dbat::portal {

    boost::asio::awaitable<std::optional<volcano::portal::JwtTokens>> refresh_jwt(volcano::portal::Client& client) {
        co_return std::optional<volcano::portal::JwtTokens>();
    }

    // AccountMode implementation would go here

    boost::asio::awaitable<void> AccountMode::enterMode() {
        LINFO("Entering AccountMode");
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
        auto color = client_.clientData().color;

        auto process = volcano::circle::processColors(cached_screen, color, nullptr);
        co_await client_.sendLine(process);
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::handleCommand(const std::string& data) {
        auto color = client_.clientData().color;

        auto process = volcano::circle::processColors(data, color, nullptr);

        co_await client_.sendLine("ECHO: " + process);
        co_return;
    }

    boost::asio::awaitable<void> AccountMode::handleGMCP(const std::string& package, const nlohmann::json& data) {
        LINFO("AccountMode received GMCP package: {} with data: {}", package, data.dump());
        co_await client_.sendLine("Received GMCP package: " + package + " with data: " + data.dump());

        co_return;
    }

    // Other implementations below this

}