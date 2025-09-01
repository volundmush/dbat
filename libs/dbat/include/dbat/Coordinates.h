#pragma once
#include "sysdep.h"
#include "defs.h"

struct Coordinates {
    int32_t x{0}, y{0}, z{0};

    bool operator==(const Coordinates& other) const;
    explicit operator bool() const;

    void apply(Direction dir);
    Coordinates get_direction_offset(Direction dir);
};

template <>
struct fmt::formatter<Coordinates> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Coordinates& z, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}:{}:{}", z.x, z.y, z.z);
    }
};

namespace std {
    template<>
    struct hash<Coordinates> {
        std::size_t operator()(const Coordinates& coord) const noexcept;
    };
}