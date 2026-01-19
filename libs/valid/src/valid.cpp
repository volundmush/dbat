#include "valid/valid.hpp"

#include <cctype>

namespace dbat::valid {

    std::string trim_ascii(std::string_view input) {
        auto in = std::string(input);
        return boost::algorithm::trim_copy_if(
            in,
            [](unsigned char ch) {
                return std::isspace(ch);
            }
        );
    }

    std::string to_lower_ascii(std::string_view input) {
        return boost::algorithm::to_lower_copy(std::string(input));
    }

    static bool is_valid_username_char(char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
    }

    Result<std::string> username(std::string_view input) {
        auto trimmed = trim_ascii(input);
        if (trimmed.empty()) {
            return std::unexpected("username is required");
        }

        auto normalized = to_lower_ascii(trimmed);

        if (trimmed.size() < 3) {
            return std::unexpected("username must be at least 3 characters");
        }
        if (trimmed.size() > 32) {
            return std::unexpected("username must be at most 32 characters");
        }

        auto starts = tolower(trimmed[0]);

        if (!(starts >= 'a' && starts <= 'z')) {
            return std::unexpected("username must start with a letter");
        }

        for (char ch : trimmed) {
            if (!is_valid_username_char(tolower(ch))) {
                return std::unexpected("username may only contain letters, digits, and underscore");
            }
        }

        return trimmed;
    }

    static bool is_valid_charname_char(char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
    }

    Result<std::string> character_name(std::string_view input) {
        auto trimmed = trim_ascii(input);
        if (trimmed.empty()) {
            return std::unexpected("character name is required");
        }

        if (trimmed.size() < 1) {
            return std::unexpected("character name must be at least 1 characters");
        }
        if (trimmed.size() > 32) {
            return std::unexpected("character name must be at most 32 characters");
        }

        for (char ch : trimmed) {
            if (!is_valid_charname_char(tolower(ch))) {
                return std::unexpected("character name may only contain letters and digits");
            }
        }

        return trimmed;
    }

} // namespace dbat::valid
