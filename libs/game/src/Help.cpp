#include "dbat/game/Typedefs.hpp"
#include "dbat/game/Help.hpp"
#include "dbat/game/Database.hpp"

#include <boost/algorithm/string.hpp>

std::expected<int, std::string> search_help(std::string_view argument, int level)
{
    if (argument.empty()) {
        return std::unexpected("No search term provided");
    }

    auto arg_lower = boost::algorithm::to_lower_copy(std::string{argument});

    auto exact_rows = dbat::db::txn->exec(
        "SELECT h.id FROM dbat.help h "
        "JOIN dbat.help_keywords k ON h.id = k.help_id "
        "WHERE LOWER(k.keyword) = LOWER($1) AND h.min_level <= $2 "
        "LIMIT 1",
        pqxx::params{std::string{argument}, level}
    );

    if (!exact_rows.empty()) {
        return exact_rows[0][0].as<int>();
    }

    auto partial_rows = dbat::db::txn->exec(
        "SELECT DISTINCT k.keyword FROM dbat.help h "
        "JOIN dbat.help_keywords k ON h.id = k.help_id "
        "WHERE k.keyword ILIKE ($1 || '%') AND h.min_level <= $2 "
        "ORDER BY k.keyword "
        "LIMIT 10",
        pqxx::params{arg_lower, level}
    );

    if (!partial_rows.empty()) {
        std::string matches = "No exact match. Perhaps you meant: ";
        for (size_t i = 0; i < partial_rows.size(); ++i) {
            if (i > 0) matches += ", ";
            matches += partial_rows[i][0].as<std::string>();
        }
        return std::unexpected(matches);
    }

    return std::unexpected("No help found for '" + std::string{argument} + "'");
}
