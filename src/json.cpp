#include "dbat/json.h"

std::string jdumps(const json &j)
{
    return j.dump(-1, ' ', false, json::error_handler_t::ignore);
}

std::string jdumps_pretty(const json &j)
{
    return j.dump(4, ' ', false, json::error_handler_t::ignore);
}

json jloads(const std::string &s)
{
    return json::parse(s);
}

json jobject()
{
    return json::object();
}