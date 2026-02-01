#include <filesystem>
#include <fstream>
#include <sstream>
#include "dbat/portal/Client.hpp"
#include "boost/algorithm/string.hpp"
#include "volcano/circle/CircleAnsi.hpp"
#include "volcano/log/Log.hpp"
#include "volcano/util/PartialMatch.hpp"

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

    boost::asio::awaitable<void> CircleModeHandler::cmdQuit(volcano::mud::CommandData& cmd) {
        co_await sendCircleLine("Goodbye!");
        requestCancel();
        co_return;
    }

    boost::asio::awaitable<std::expected<std::vector<CharacterListEntry>, std::string>> CircleModeHandler::getCharacterList() {
        std::vector<CharacterListEntry> character_list;

        auto req = client_.createAuthenticatedRequest(boost::beast::http::verb::get, "/v1/character");
        auto &http = client_.httpClient();
        auto res = co_await http.request(req);
        if(!res) {
            co_return std::unexpected("Error fetching character list: " + res.error());
        }

        auto &response = *res;
        if(response.result() != boost::beast::http::status::ok) {
            co_return std::unexpected("Error fetching character list: " + response.body());
        }

        try {
            auto j = nlohmann::json::parse(response.body());
            if(j.is_array()) {
                for (const auto& item : j) {
                    CharacterListEntry entry;
                    entry.id = item.at("id").get<int64_t>();
                    entry.name = item.at("name").get<std::string>();
                    character_list.push_back(entry);
                }
            }
        } catch (const std::exception& e) {
            co_return std::unexpected("Failed to parse character list JSON: " + std::string(e.what()));
        }

        co_return character_list;
    }

    std::expected<nlohmann::json, std::string> CircleModeHandler::getJwtPayload() {
        if(!client_.tokens) {
            return std::unexpected("No JWT tokens available");
        }

        try {
            auto payload = volcano::jwt::extract_payload(client_.tokens->jwt);
            return payload;
        } catch (const std::exception& e) {
            return std::unexpected("Failed to decode JWT payload: " + std::string(e.what()));
        }
    }

    boost::asio::awaitable<std::vector<std::pair<std::string, std::string>>> CircleModeHandler::getMSSPData() {
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

    boost::asio::awaitable<void> CircleModeHandler::cmdLogout(volcano::mud::CommandData& cmd) {
        client_.tokens.reset();
        co_await requestMode(std::make_shared<LoginMode>(client_));
        co_return;
    }

    boost::asio::awaitable<std::expected<CharacterListEntry, std::string>> CircleModeHandler::getCharacterByName(std::string_view name) {
        auto character_list_res = co_await getCharacterList();
        if(!character_list_res) {
            co_return std::unexpected(character_list_res.error());
        }

        auto &character_list = *character_list_res;

        auto res = volcano::util::partialMatch(name, character_list, false, 
            [](const CharacterListEntry& entry) {
                return entry.name;
            }
        );
        if(!res) {
            co_return std::unexpected(res.error());
        }
        auto &matched = *res;
        co_return *matched;
    }

}