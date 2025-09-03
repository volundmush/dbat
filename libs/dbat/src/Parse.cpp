#include "dbat/Parse.h"

std::regex parseRangeRegex(R"(^(\d+)(-(\d+))?$)", std::regex::icase);