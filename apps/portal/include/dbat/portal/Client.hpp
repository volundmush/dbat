#pragma once
#include "volcano/portal/portal.hpp"
#include "volcano/portal/Client.hpp"


namespace dbat::portal {

    boost::asio::awaitable<std::optional<volcano::portal::JwtTokens>> refresh_jwt(volcano::portal::Client& client);

    class AccountMode : public volcano::portal::ModeHandler {
        public:
        using ModeHandler::ModeHandler;
        protected:
        boost::asio::awaitable<void> enterMode() override;
        boost::asio::awaitable<void> handleCommand(const std::string& data) override;
        boost::asio::awaitable<void> handleGMCP(const std::string& package, const nlohmann::json& data) override;
    };
}