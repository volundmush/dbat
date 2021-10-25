//
// Created by basti on 10/24/2021.
//
#include "structs.h"
#include "races.h"
#include "utils.h"

static std::string robot = "Robotic-Humanoid", robot_lower = "robotic-humanoid", unknown = "UNKNOWN";


const std::string& char_data::juggleRaceName(bool capitalized) const {
    if(!race) return unknown;

    dbat::race::Race *apparent = race;

    switch(apparent->getID()) {
        case dbat::race::hoshijin:
            if(mimic) apparent = mimic;
            break;
        case dbat::race::halfbreed:
            switch(RACIAL_PREF(this)) {
                case 1:
                    apparent = dbat::race::race_map[dbat::race::human];
                    break;
                case 2:
                    apparent = dbat::race::race_map[dbat::race::saiyan];
                    break;
            }
            break;
        case dbat::race::android:
            switch(RACIAL_PREF(this)) {
                case 1:
                    apparent = dbat::race::race_map[dbat::race::android];
                    break;
                case 2:
                    apparent = dbat::race::race_map[dbat::race::human];
                    break;
                case 3:
                    if(capitalized) {
                        return robot;
                    } else {
                        return robot_lower;
                    }
            }
            break;
        case dbat::race::saiyan:
            if(PLR_FLAGGED(this, PLR_TAILHIDE)) {
                apparent = dbat::race::race_map[dbat::race::human];
            }
            break;
    }

    if(capitalized) {
        return apparent->getName();
    } else {
        return apparent->getNameLower();
    }
}