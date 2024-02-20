#include "dbat/structs.h"


void Messager::addVar(const std::string& key, MsgVar value) {
    variables[key] = value;
}

void Messager::deliver() {

}