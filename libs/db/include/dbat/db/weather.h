#pragma once
#include <stdint.h>

struct weather_data {
   int	pressure;	/* How is the pressure ( Mb ) */
   int	change;	/* How fast and what way does it change. */
   int	sky;	/* How is the sky. */
   int	sunlight;	/* And how much sun. */
};

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   int16_t year;
};

extern struct time_info_data time_info;/* the infomation about the time    */
extern struct weather_data weather_info;	/* the infomation about the weather */