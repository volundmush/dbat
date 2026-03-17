#include "dbat/game/TimeInfo.hpp"
#include <nlohmann/json.hpp>

struct time_info_data old_time_info;/* the infomation about the time    */
struct time_info_data time_info;/* the infomation about the time    */
struct time_info_data era_uptime;/* the infomation about how long the server has been up    */

void to_json(nlohmann::json& j, const time_info_data& unit)
{
    if(unit.remainder != 0.0) {
        j[+"remainder"] = unit.remainder;
    }
    if(unit.seconds != 0) {
        j[+"seconds"] = unit.seconds;
    }
    if(unit.minutes != 0) {
        j[+"minutes"] = unit.minutes;
    }
    if(unit.hours != 0) {
        j[+"hours"] = unit.hours;
    }
    if(unit.day != 0) {
        j[+"day"] = unit.day;
    }
    if(unit.month != 0) {
        j[+"month"] = unit.month;
    }
    if(unit.year != 0) {
        j[+"year"] = unit.year;
    }
}

void from_json(const nlohmann::json& j, time_info_data& unit)
{
    if(j.contains(+"remainder")) {
        j.at(+"remainder").get_to(unit.remainder);
    }
    if(j.contains(+"seconds")) {
        j.at(+"seconds").get_to(unit.seconds);
    }
    if(j.contains(+"minutes")) {
        j.at(+"minutes").get_to(unit.minutes);
    }
    if(j.contains(+"hours")) {
        j.at(+"hours").get_to(unit.hours);
    }
    if(j.contains(+"day")) {
        j.at(+"day").get_to(unit.day);
    }
    if(j.contains(+"month")) {
        j.at(+"month").get_to(unit.month);
    }
    if(j.contains(+"year")) {
        j.at(+"year").get_to(unit.year);
    }
}
