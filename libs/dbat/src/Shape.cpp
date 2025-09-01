#include "dbat/AbstractGridArea.h"


static inline AABB computeAabb(const std::variant<BoxDim, RoundDim>& g) {
    return std::visit([](auto const& v) -> AABB {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, BoxDim>)
            return v.box;
        else
            return v.bounds();
    }, g);
}

Shape::Shape(const ShapeBase& b) : ShapeBase(b), seq(0), cachedAabb(computeAabb(geom)) {

}

bool Shape::contains(const Coordinates& c) const noexcept {
    if (!cachedAabb.contains(c)) return false;
    return std::visit([&](auto const& g){ return g.contains(c); }, geom);
}

void Shape::reComputeAABB() {
    cachedAabb = computeAabb(geom);
}
