#include "dbat/db/character_utils.h"
#include <stdlib.h>
#include <string.h>
#include "dbat/db/consts/types.h"
#include "dbat/db/consts/positions.h"
#include "dbat/db/consts/playerflags.h"
#include "dbat/db/consts/mobflags.h"
#include "dbat/db/consts/sex.h"
#include "dbat/db/consts/races.h"
#include "dbat/db/weather.h"


bool MOON_TIMECHECK() {
    switch(time_info.day) {
        case 20:
            return time_info.hours >= 21;
        case 21:
            return time_info.hours < 4 || time_info.hours >= 21;
        case 22:
            return time_info.hours < 4;
    }
    return false;
}
