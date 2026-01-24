#include "dbat/game/AbstractGridArea.hpp"

ShapeBase& ShapeBase::operator=(const Shape& other) {
    *this = static_cast<const ShapeBase&>(other);
    return *this;
}