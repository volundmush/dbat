#pragma once
#include <cstdint>

#include "dbat/consts/weather.hpp"
#include "dbat/consts/types.hpp"

struct weather_data {
  int pressure{}; /* How is the pressure ( Mb ) */
  int change{};   /* How fast and what way does it change. */
  int sky{};      /* How is the sky. */
  int sunlight{}; /* And how much sun. */
};

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
  int hours{}, day{}, month{};
  int16_t year{};
};

// commands
void star_phase(GameWorld *g, struct char_data *ch, int type);
void oozaru_transform(GameWorld *g, struct char_data *ch);
void oozaru_revert(GameWorld *g, struct char_data *ch);
void weather_and_time(GameWorld *g, int mode);

