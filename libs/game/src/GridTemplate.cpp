#include "dbat/game/AbstractGridArea.hpp"

GridTemplate& GridTemplate::operator=(const AbstractGridArea& other) {
    HasMudStrings::operator=(static_cast<const HasMudStrings&>(other));
    this->shapes.clear();
    for(const auto& [name, shapePtr] : other.shapes) {
         shapes[name] = *shapePtr;
    }
    return *this;
}
