#include "dbat/AbstractGridArea.h"

ShapeBase& ShapeBase::operator=(const Shape& other) {
    *this = static_cast<const ShapeBase&>(other);
    return *this;
}