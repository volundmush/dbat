#include "api/channel.hpp"

namespace dbat::api {
    RequestChannel& request_channel() {
        static RequestChannel channel(dbat::net::context().get_executor(), 1024);
        return channel;
    }

    Channel<std::shared_ptr<GameConnectionInfo>>& game_connection_channel() {
        static Channel<std::shared_ptr<GameConnectionInfo>> channel(dbat::net::context().get_executor(), 1024);
        return channel;
    }

    Channel<std::pair<int64_t, std::string>>& game_disconnection_channel() {
        static Channel<std::pair<int64_t, std::string>> channel(dbat::net::context().get_executor(), 1024);
        return channel;
    }

    Channel<std::string>& broadcast_channel() {
        static Channel<std::string> channel(dbat::net::context().get_executor(), 1024);
        return channel;
    }

    boost::asio::awaitable<dbat::web::Result> send_request_and_reply(dbat::web::RouteContext& data, NetRequest&& request) {
        RequestMessage msg;
        msg.request = std::move(request);

        auto response_channel = msg.response_channel;
        boost::system::error_code send_ec;
        co_await request_channel().async_send(send_ec, std::move(msg), boost::asio::use_awaitable);
        if (send_ec) {
            co_return std::unexpected(HttpError{http::status::internal_server_error, "Failed to send request: " + std::string(send_ec.message()) + "\n"});
        }

        NetResponse response;
        try {
            response = co_await response_channel->async_receive(boost::asio::use_awaitable);
        } catch (const boost::system::system_error& e) {
            co_return std::unexpected(HttpError{http::status::internal_server_error, "Failed to receive response: " + std::string(e.code().message()) + "\n"});
        }

        if (std::holds_alternative<HttpAnswer>(response)) {
            auto& answer = std::get<HttpAnswer>(response);
            co_return answer;
        }

        auto& error = std::get<HttpError>(response);
        co_return std::unexpected(error);
    }
}