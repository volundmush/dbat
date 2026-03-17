#pragma once
#include <cstdint>
#include <nlohmann/json_fwd.hpp>

/* Sun state for weather_data */
constexpr int SUN_DARK = 0;
constexpr int SUN_RISE = 1;
constexpr int SUN_LIGHT = 2;
constexpr int SUN_SET = 3;

/* Sky conditions for weather_data */
constexpr int SKY_CLOUDLESS = 0;
constexpr int SKY_CLOUDY = 1;
constexpr int SKY_RAINING = 2;
constexpr int SKY_LIGHTNING = 3;


constexpr double MUD_TIME_ACCELERATION = 12.0;  // 12 MUD seconds pass per real second.

constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr double MINUTES_PER_HOUR = 60.0;
constexpr double HOURS_PER_DAY = 24.0;
constexpr double DAYS_PER_WEEK = 7.0;
constexpr double DAYS_PER_MONTH = 30.0;
constexpr double MONTHS_PER_YEAR = 12.0;
constexpr double DAYS_PER_YEAR = 365.0;

constexpr double SECS_PER_MINUTE = SECONDS_PER_MINUTE;
constexpr double SECS_PER_HOUR =  (SECONDS_PER_MINUTE*MINUTES_PER_HOUR);
constexpr double SECS_PER_DAY =   (SECS_PER_HOUR*HOURS_PER_DAY);
constexpr double SECS_PER_WEEK =  (SECS_PER_DAY*DAYS_PER_WEEK);
constexpr double SECS_PER_MONTH = (SECS_PER_DAY*DAYS_PER_MONTH);
constexpr double SECS_PER_YEAR =  (SECS_PER_DAY*DAYS_PER_YEAR);
constexpr double SECS_PER_GAME_YEAR = (SECS_PER_MONTH*MONTHS_PER_YEAR);


struct weather_data {
    int pressure{};    /* How is the pressure ( Mb ) */
    int change{};    /* How fast and what way does it change. */
    int sky{};    /* How is the sky. */
    int sunlight{};    /* And how much sun. */
};

void to_json(nlohmann::json& j, const weather_data& unit);
void from_json(const nlohmann::json& j, weather_data& unit);

struct Character;

// world data...

extern struct weather_data weather_info; /* the infomation about the weather */


// commands
extern void star_phase(Character *ch, int type);

extern void oozaru_revert(Character *ch);

void advanceClock(uint64_t heartPulse, double deltaTime);

void moonrise();

void moondown();
