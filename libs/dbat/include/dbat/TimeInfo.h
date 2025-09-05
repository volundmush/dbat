#pragma once
#include <cstdint>

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
    time_info_data() = default;
    time_info_data(std::int64_t timestamp);
    double remainder{};
    int seconds{}, minutes{}, hours{}, day{}, month{};
    std::int64_t year{};
    // The number of seconds since year 0. Can be negative.
    std::int64_t current();
};

extern struct time_info_data time_info;  /* the infomation about the time    */
extern struct time_info_data era_uptime; /* the infomation about the time    */
extern struct time_info_data old_time_info; /* UNUSED (to be removed) the infomation about the time    */
