#pragma once
#include "base.hpp"

#include <boost/asio/experimental/concurrent_channel.hpp>
#include <nlohmann/json.hpp>

namespace dbat::api
{
    template <typename T>
    using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

    struct UserRegisterReq
    {
        std::string username;
        std::string password;
    };

    struct UserLoginReq
    {
        std::string username;
        std::string password;
    };

    struct UserRefreshReq
    {
        std::string refresh_token;
    };

    struct CharacterListReq
    {
        int64_t account_id;
    };

    struct CharacterDeleteReq
    {
        int64_t account_id;
        int64_t character_id;
    };

    struct CharacterCreateReq
    {
        int64_t account_id;
        nlohmann::json character_data;
    };

    struct PlayReq
    {
        int64_t account_id;
        int64_t character_id;
    };

    struct MsspReq {};

    using NetRequest = std::variant<UserRegisterReq, UserLoginReq, UserRefreshReq,
                                    CharacterListReq, CharacterDeleteReq, CharacterCreateReq, PlayReq, MsspReq>;

    using JsonChannel = Channel<nlohmann::json>;
    using ResponseChannel = Channel<volcano::web::HttpAnswer>;

    using ResponseChannelPtr = std::shared_ptr<ResponseChannel>;

    

    Channel<std::pair<int64_t, std::string>> &game_disconnection_channel();

    Channel<std::string> &broadcast_channel();

    boost::asio::awaitable<volcano::web::HttpAnswer> send_request_and_reply(volcano::web::RequestContext &ctx, NetRequest &&request);

    struct GameMessageText
    {
        std::string text;
    };

    struct GameMessageJson
    {
        nlohmann::json data;
    };

    struct GameMessageDisconnect
    {
        std::string reason;
    };

    using ToGameMessage = std::variant<GameMessageText, GameMessageJson, GameMessageDisconnect>;

    struct GameConnectionInfo
    {
        bool isSecure{false};
        std::string hostname, address;
        int64_t account_id;
        int64_t character_id;
        Channel<ToGameMessage> to_game, to_websocket;
        int64_t connection_id;
    };

    extern std::unordered_map<int64_t, std::shared_ptr<GameConnectionInfo>> active_game_connections;

    struct RequestMessage {
    NetRequest            request;
    ResponseChannelPtr    response_channel;

    RequestMessage()
        : response_channel(std::make_shared<ResponseChannel>(volcano::net::context().get_executor(), 1)) {}

    RequestMessage(NetRequest req)
        : request(std::move(req)), response_channel(std::make_shared<ResponseChannel>(volcano::net::context().get_executor(), 1)) {}

    RequestMessage(NetRequest req, ResponseChannel response)
        : request(std::move(req)), response_channel(std::make_shared<ResponseChannel>(std::move(response))) {}

    RequestMessage(NetRequest req, ResponseChannelPtr response)
        : request(std::move(req)), response_channel(std::move(response)) {}
};

using RequestChannel = Channel<RequestMessage>;

RequestChannel &request_channel();

    Channel<std::shared_ptr<GameConnectionInfo>> &game_connection_channel();
    Channel<std::pair<int64_t, std::string>> &game_disconnection_channel();


}