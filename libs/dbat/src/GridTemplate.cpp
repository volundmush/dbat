#include "dbat/AbstractGridArea.h"

GridTemplate& GridTemplate::operator=(const AbstractGridArea& other) {
    this->strings = other.strings;
    this->shapes.clear();
    for(const auto& [name, shapePtr] : other.shapes) {
         shapes[name] = *shapePtr;
    }
    return *this;
}
