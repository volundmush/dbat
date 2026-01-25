#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace dbat::valid {

    template<typename T>
    using Result = std::expected<T, std::string>;

    Result<std::string> username(std::string_view input);
    Result<std::string> character_name(std::string_view input);

} // namespace dbat::valid
