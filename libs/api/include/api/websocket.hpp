#pragma once
#include "api/channel.hpp"
#include "logging/Log.hpp"

#include <boost/asio/experimental/awaitable_operators.hpp>

namespace dbat::api
{
    template <typename T>
    struct WebSocketConnection
    {
        boost::beast::websocket::stream<T> &ws;
        dbat::web::RouteContext &data;
        std::shared_ptr<GameConnectionInfo> game_info;

        boost::asio::awaitable<void> handle_read_message(const nlohmann::json &msg)
        {
            // we need to determine the "type" field.
            if (!msg.contains("type") || !msg["type"].is_string())
            {
                LINFO("Received message without valid 'type' field from connection {}.", game_info->connection_id);
                co_return;
            }
            std::string type = msg["type"].get<std::string>();

            boost::system::error_code ec;
            if (boost::iequals(type, "text"))
            {
                if (!msg.contains("text") || !msg["text"].is_string())
                {
                    LINFO("Received 'text' message without valid 'text' field from connection {}.", game_info->connection_id);
                    co_return;
                }
                std::string text = msg["text"].get<std::string>();
                GameMessageText gmsg{text};
                co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
                if (ec)
                {
                    LERROR("Failed to send text message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
                }
            }
            else if (boost::iequals(type, "json"))
            {
                if (!msg.contains("data") || !msg["data"].is_object())
                {
                    LINFO("Received 'json' message without valid 'data' field from connection {}.", game_info->connection_id);
                    co_return;
                }
                nlohmann::json data = msg["data"];
                GameMessageJson gmsg{data};
                co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
                if (ec)
                {
                    LERROR("Failed to send json message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
                }
            }
            else if (boost::iequals(type, "disconnect"))
            {
                std::string reason = "Client requested disconnect.";
                if (msg.contains("reason") && msg["reason"].is_string())
                {
                    reason = msg["reason"].get<std::string>();
                }
                GameMessageDisconnect gmsg{reason};
                co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
                if (ec)
                {
                    LERROR("Failed to send disconnect message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
                }
            }
            else
            {
                LINFO("Received unknown message type '{}' from connection {}.", type, game_info->connection_id);
            }

            co_return;
        }

        boost::asio::awaitable<void> run_reader()
        {
            boost::beast::flat_buffer buffer;

            boost::system::error_code ec;
            boost::system::error_code ws_ec;
            try
            {
                while (true)
                {
                    // read a message
                    std::size_t n = co_await ws.async_read(buffer, boost::asio::use_awaitable);
                    if (ws.text())
                    {
                        auto data_ptr = buffer.data().data();
                        auto data_str = std::string_view(static_cast<const char *>(data_ptr), n);
                        nlohmann::json msg = nlohmann::json::parse(data_str, nullptr, false);
                        if (msg.is_discarded())
                        {
                            LINFO("Received invalid JSON message from connection {}.", game_info->connection_id);
                        }
                        else
                        {
                            co_await handle_read_message(msg);
                        }
                    }
                    else
                    {
                        LINFO("Received non-text WebSocket message from connection {}. Ignoring.", game_info->connection_id);
                    }
                    buffer.consume(n);
                }
            }
            catch (const boost::system::error_code &e)
            {
                ws_ec = e;
                LINFO("WebSocket closed for connection {}: {}", game_info->connection_id, e.what());
            }
            catch (const std::exception &e)
            {
                ws_ec = boost::asio::error::operation_aborted;
                LINFO("WebSocket reader error for connection {}: {}", game_info->connection_id, e.what());
            }

            if (ws_ec)
            {
                co_await game_disconnection_channel().async_send(ec, std::make_pair(game_info->connection_id, ws_ec.message()), boost::asio::use_awaitable);
            }

            co_return;
        }

        boost::asio::awaitable<void> run_writer()
        {
            ws.text(true);

            try
            {
                while (true)
                {
                    // Step 1: wait for a message from the game server
                    boost::system::error_code ec;
                    auto gmsg = co_await game_info->to_websocket.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                    if (ec)
                    {
                        LINFO("Game to WebSocket channel closed for connection {}: {}", game_info->connection_id, ec.message().c_str());
                        break;
                    }

                    // ready outgoing message...
                    nlohmann::json jmsg;

                    if (std::holds_alternative<GameMessageText>(gmsg))
                    {
                        auto &msg = std::get<GameMessageText>(gmsg);
                        jmsg["type"] = "text";
                        jmsg["text"] = msg.text;
                    }
                    else if (std::holds_alternative<GameMessageJson>(gmsg))
                    {
                        auto &msg = std::get<GameMessageJson>(gmsg);
                        jmsg["type"] = "json";
                        jmsg["data"] = msg.data;
                    }
                    else if (std::holds_alternative<GameMessageDisconnect>(gmsg))
                    {
                        auto &msg = std::get<GameMessageDisconnect>(gmsg);
                        jmsg["type"] = "disconnect";
                        jmsg["reason"] = msg.reason;
                    }
                    else
                    {
                        LERROR("Unknown message type received from game server for connection {}.", game_info->connection_id);
                        continue;
                    }
                    auto to_send = jmsg.dump();

                    // Step 2: send the message over the WebSocket
                    boost::system::error_code ws_ec;
                    co_await ws.async_write(boost::asio::buffer(to_send), boost::asio::redirect_error(boost::asio::use_awaitable, ws_ec));
                    if (ws_ec)
                    {
                        LINFO("WebSocket write error for connection {}: {}", game_info->connection_id, ws_ec.message().c_str());
                        break;
                    }
                }
            }
            catch (const std::exception &e)
            {
                LINFO("WebSocket writer error for connection {}: {}", game_info->connection_id, e.what());
            }

            co_return;
        }

        boost::asio::awaitable<void> run()
        {
            // first we announce the new connection to the game server...
            boost::system::error_code ec;
            co_await game_connection_channel().async_send(ec, game_info, boost::asio::use_awaitable);
            if (ec)
            {
                LERROR("Failed to send new game connection info: {}", ec.message().c_str());
                co_return;
            }

            using namespace boost::asio::experimental::awaitable_operators;

            co_await (run_reader() || run_writer());

            co_return;
        }
    };

    template <typename T>
    boost::asio::awaitable<dbat::web::Result> handle_play_helper(dbat::net::AnyConnection &conn, dbat::web::RouteContext &data, boost::beast::websocket::stream<T> ws, int64_t account_id, int64_t character_id)
    {
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::response_type &res)
            {
                res.set(http::field::server, "dbat");
            }));

        try
        {
            co_await ws.async_accept(data.req, boost::asio::use_awaitable);
        }
        catch (const std::exception &e)
        {
            co_return std::unexpected(HttpError{http::status::bad_request, std::string("WebSocket upgrade failed: ") + e.what() + "\n"});
        }

        auto exec = co_await boost::asio::this_coro::executor;

        auto gameinfo = std::make_shared<GameConnectionInfo>(conn.isSecure, conn.hostname, conn.address, account_id,
                                                             character_id, std::move(Channel<ToGameMessage>(exec, 32)), std::move(Channel<ToGameMessage>(exec, 32)), -1);

        WebSocketConnection<T> ws_conn{ws, data, gameinfo};

        co_await ws_conn.run();

        // This is included to satisfy the return type; the handler will ignore it.
        co_return HttpAnswer{http::status::switching_protocols, ""};
    }

    boost::asio::awaitable<dbat::web::Result> handle_play(dbat::net::AnyConnection &conn, dbat::web::RouteContext &data);
}