#pragma once
#include "volcano/portal/portal.hpp"
#include "volcano/portal/Client.hpp"
#include "volcano/mud/Command.hpp"

namespace dbat::portal {

    boost::asio::awaitable<std::optional<volcano::portal::JwtTokens>> refresh_jwt(volcano::portal::Client& client);

    class CircleModeHandler : public volcano::portal::ModeHandler {
        public:
        using ModeHandler::ModeHandler;
        protected:
        boost::asio::awaitable<void> sendCircleLine(std::string_view line);
        boost::asio::awaitable<void> sendCircleText(std::string_view text);
    };

    class AccountMode : public CircleModeHandler {
        public:
        using CircleModeHandler::CircleModeHandler;
        protected:
        boost::asio::awaitable<std::vector<std::pair<std::string, std::string>>> getMSSPData();
        boost::asio::awaitable<void> enterMode() override;
        boost::asio::awaitable<void> runImpl() override;
        boost::asio::awaitable<void> handleCommand(const std::string& data) override;
        boost::asio::awaitable<void> handleGMCP(const std::string& package, const nlohmann::json& data) override;
    
        boost::asio::awaitable<void> cmdQuit(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdMSSP(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdLogin(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdRegister(volcano::mud::CommandData& cmd);

    };
}