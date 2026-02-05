#pragma once
#include "volcano/portal/portal.hpp"
#include "volcano/portal/Client.hpp"
#include "volcano/mud/Command.hpp"
#include "volcano/web/Base.hpp"

namespace dbat::portal {

    boost::asio::awaitable<std::optional<volcano::portal::JwtTokens>> refresh_jwt(volcano::portal::Client& client);

    struct CharacterListEntry {
        int64_t id;
        std::string name;
    };

    class CircleModeHandler : public volcano::portal::ModeHandler {
        public:
        using ModeHandler::ModeHandler;
        protected:
        virtual boost::asio::awaitable<void> handleServerDisconnect();
        boost::asio::awaitable<void> handleDisconnect() override;
        boost::asio::awaitable<void> sendCircleLine(std::string_view line);
        boost::asio::awaitable<void> sendCircleText(std::string_view text);
        boost::asio::awaitable<void> cmdQuit(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdLogout(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<std::vector<std::pair<std::string, std::string>>> getMSSPData();
        boost::asio::awaitable<std::expected<std::vector<CharacterListEntry>, std::string>> getCharacterList();
        std::expected<nlohmann::json, std::string> getJwtPayload();
        boost::asio::awaitable<std::expected<CharacterListEntry, std::string>> getCharacterByName(std::string_view name);
    };

    class LoginMode : public CircleModeHandler {
        public:
        using CircleModeHandler::CircleModeHandler;
        protected:
        boost::asio::awaitable<void> enterMode() override;
        boost::asio::awaitable<void> runImpl() override;
        boost::asio::awaitable<void> handleCommand(const std::string& data) override;
        boost::asio::awaitable<void> cmdMSSP(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdLogin(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdRegister(volcano::mud::CommandData& cmd);

    };

    class AccountMode : public CircleModeHandler {
        public:
        using CircleModeHandler::CircleModeHandler;
        protected:
        boost::asio::awaitable<void> enterMode() override;

        boost::asio::awaitable<void> displayMenu();

        boost::asio::awaitable<void> handleCommand(const std::string& data) override;
    
        boost::asio::awaitable<void> cmdChargen(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdDelete(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdPlay(volcano::mud::CommandData& cmd);
        
    };

    class ChargenMode : public CircleModeHandler {
        public:
        using CircleModeHandler::CircleModeHandler;
        protected:
        boost::asio::awaitable<void> enterMode() override;

        boost::asio::awaitable<void> handleCommand(const std::string& data) override;

        boost::asio::awaitable<void> cmdSetName(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdSetClass(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdSetRace(volcano::mud::CommandData& cmd);
        boost::asio::awaitable<void> cmdFinalize(volcano::mud::CommandData& cmd);
    };

    class PlayMode : public CircleModeHandler {
        public:
        PlayMode(volcano::portal::Client& client, volcano::web::WebSocketStream&& ws) : CircleModeHandler(client), ws_(std::move(ws)) {}
        protected:

        volcano::web::WebSocketStream ws_;

        boost::asio::awaitable<void> runImpl() override;
        boost::asio::awaitable<void> processMessage(const nlohmann::json& j);

        boost::asio::awaitable<void> sendToWebSocket(const nlohmann::json& j, const std::string& msg_type);

        boost::asio::awaitable<void> handleCommand(const std::string& data) override;
        boost::asio::awaitable<void> handleDisconnect() override;
        boost::asio::awaitable<void> handleChangeCapabilities(const nlohmann::json& j) override;
        boost::asio::awaitable<void> handleGMCP(const std::string& package, const nlohmann::json& data) override;
    };
}