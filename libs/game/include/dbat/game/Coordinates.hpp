#pragma once
#include <cstdint>
#include <functional>
#include <fmt/format.h>

#include "const/Direction.hpp"

struct Coordinates {
    int32_t x{0}, y{0}, z{0};

    bool operator==(const Coordinates& other) const;
    explicit operator bool() const;

    void apply(Direction dir);
    Coordinates get_direction_offset(Direction dir);
};

inline std::string format_as(const Coordinates& coor) {
    return fmt::format("{}:{}:{}", coor.x, coor.y, coor.z);
}

namespace std {
    template<>
    struct hash<Coordinates> {
        std::size_t operator()(const Coordinates& coord) const noexcept;
    };
}