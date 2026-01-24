#include "dbat/game/Parse.hpp"

std::regex parseRangeRegex(R"(^(\d+)(-(\d+))?$)", std::regex::icase);