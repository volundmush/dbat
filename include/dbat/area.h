#pragma once
#include "structs.h"

constexpr int PLANET_EARTH = 1;
constexpr int PLANET_VEGETA = 2;
constexpr int PLANET_FRIGID = 3;
constexpr int PLANET_NAMEK = 4;
constexpr int PLANET_KONACK = 5;
constexpr int PLANET_AETHER = 6;
constexpr int PLANET_YARDRAT = 7;
constexpr int PLANET_KANASSA = 8;
constexpr int PLANET_CERRIA = 9;
constexpr int PLANET_ARLIA = 10;
constexpr int PLANET_ZENITH = 11;

constexpr int ORBIT_EARTH = 50;
constexpr int ORBIT_VEGETA = 53;
constexpr int ORBIT_FRIGID = 51;
constexpr int ORBIT_NAMEK = 54;
constexpr int ORBIT_KONACK = 52;
constexpr int ORBIT_AETHER = 55;
constexpr int ORBIT_YARDRAT = 56;
constexpr int ORBIT_KANASSA = 58;
constexpr int ORBIT_CERRIA = 198;
constexpr int ORBIT_ARLIA = 59;
constexpr int ORBIT_ZENITH = 57;

extern int getPlanet(const room_vnum room);
extern std::string getPlanetName(const int planet);
extern std::string getPlanetColorName(const int planet);
extern std::optional<double> getPlanetEnvironment(const int planet, const int environment);
extern room_vnum getPlanetOrbit(const int planet);
extern std::vector<std::pair<std::string, room_vnum>> getPlanetLandspots(const room_vnum orbit);
extern std::pair<int, std::vector<std::pair<std::string, room_vnum>>> getPlanetSpacepads(const room_vnum orbit);
extern void displayLandSpots(struct char_data *ch, const std::string& planet_name, const std::vector<std::pair<std::string, room_vnum>>& locations);