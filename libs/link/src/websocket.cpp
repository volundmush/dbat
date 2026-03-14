#include "dbat/api/websocket.hpp"
#include "dbat/api/channel.hpp"
#include "dbat/game/Parse.hpp"
#include "dbat/log/Log.hpp"
#include "dbat/mud/ClientDataSave.hpp"
#include "dbat/api/guards.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

namespace dbat::link
{
    namespace
    {
        struct WebSocketConnection
        {
            dbat::web::WebSocketStream &ws;
            dbat::web::RequestContext &data;
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
                else if (boost::iequals(type, "gmcp"))
                {
                    if(!msg.contains("package") || !msg["package"].is_string() ||
                       !msg.contains("data") || !msg["data"].is_object())
                    {
                        LINFO("Received 'gmcp' message without valid 'package' or 'data' field from connection {}.", game_info->connection_id);
                        co_return;
                    }
                    std::string package = msg["package"].get<std::string>();
                    nlohmann::json data = msg["data"];
                    GameMessageGMCP gmsg{package, data};
                    co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
                    if (ec)
                    {
                        LERROR("Failed to send gmcp message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
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
                else if (boost::iequals(type, "change_capabilities"))
                {
                    if(!msg.contains("capabilities") || !msg["capabilities"].is_object())
                    {
                        LINFO("Received 'change_capabilities' message without valid 'capabilities' field from connection {}.", game_info->connection_id);
                        co_return;
                    }
                    nlohmann::json capabilities = msg["capabilities"];
                    GameMessageChangeCapabilities gmsg{capabilities};
                    co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
                    if (ec)
                    {
                        LERROR("Failed to send change_capabilities message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
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
                        if (ws.got_text())
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
                        else if (std::holds_alternative<GameMessageGMCP>(gmsg))
                        {
                            auto &msg = std::get<GameMessageGMCP>(gmsg);
                            jmsg["type"] = "gmcp";
                            jmsg["package"] = msg.package;
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
    }

    boost::asio::awaitable<void> handle_play(dbat::web::WebSocketStream &ws, dbat::web::RequestContext &data)
    {
        auto account_id = data.user_data["account_id"].get<int64_t>();
        auto character_id = data.user_data["character_id"].get<int64_t>();

        // retrieve X-Client-Info header
        auto exec = co_await boost::asio::this_coro::executor;
        auto &net_stream = ws.next_layer();

        auto gameinfo = std::make_shared<GameConnectionInfo>(
            net_stream.is_tls(),
            net_stream.hostname(),
            net_stream.endpoint().address().to_string(),
            account_id,
            character_id,
            Channel<ToGameMessage>(exec, 32),
            Channel<ToGameMessage>(exec, 32),
            -1
        );
        from_json(data.user_data["client_info"], gameinfo->client_data);

        WebSocketConnection ws_conn{ws, data, gameinfo};
        co_await ws_conn.run();
    }
}
