#include <bitset>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include "magic_enum/magic_enum_all.hpp"

template <typename T>
struct fmt::formatter<T, char, std::enable_if_t<std::is_enum_v<T>, void>> {
    // No special format specifiers.
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const T& val, FormatContext& ctx) {
        // Get the enum's name using magic_enum.
        std::string name = std::string(magic_enum::enum_name(val));
        // Explicitly use fmt::string_view in both branches.
        fmt::string_view sv = name.empty() ? fmt::string_view("<unknown>") : fmt::string_view(name);
        return fmt::format_to(ctx.out(), "{}", sv);
    }
};

template<typename Container>
requires std::is_enum_v<typename Container::value_type>
void sprintbitarray(const Container& container, const char *names[], int maxar, char *result) {

    auto joined = fmt::format("{}", fmt::join(container, " "));
    std::strcpy(result, joined.c_str());
}
