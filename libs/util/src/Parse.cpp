#include "dbat/util/Parse.hpp"

namespace dbat::util
{
    std::regex parseRangeRegex(R"(^(\d+)(-(\d+))?$)", std::regex::icase);
}
