#include "dbat/structs.h"

Vehicle::Vehicle(const nlohmann::json& j) {
    deserialize(j);
}

std::string Vehicle::getUnitClass() {
    return "Vehicle";
}


void Vehicle::executeCommand(const std::string& command) {

}

void Vehicle::sendText(const std::string& text) {

}