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

constexpr room_vnum ORBIT_EARTH = 50;
constexpr room_vnum ORBIT_VEGETA = 53;
constexpr room_vnum ORBIT_FRIGID = 51;
constexpr room_vnum ORBIT_NAMEK = 54;
constexpr room_vnum ORBIT_KONACK = 52;
constexpr room_vnum ORBIT_AETHER = 55;
constexpr room_vnum ORBIT_YARDRAT = 56;
constexpr room_vnum ORBIT_KANASSA = 58;
constexpr room_vnum ORBIT_CERRIA = 198;
constexpr room_vnum ORBIT_ARLIA = 59;
constexpr room_vnum ORBIT_ZENITH = 57;

extern std::optional<WhereFlag> checkOrbit(const room_vnum room);
extern std::optional<WhereFlag> getPlanet(const room_vnum room);
extern std::string getPlanetName(const WhereFlag planet);
extern std::string getPlanetColorName(const WhereFlag planet);
extern std::optional<double> getPlanetEnvironment(const WhereFlag planet, const int environment);
extern room_vnum getPlanetOrbit(const WhereFlag planet);
extern std::vector<std::pair<std::string, room_vnum>> getPlanetLandspots(const WhereFlag orbit);
extern std::vector<std::pair<std::string, room_vnum>> getPlanetSpacepads(const WhereFlag orbit);
extern void displayLandSpots(struct char_data *ch, const std::string& planet_name, const std::vector<std::pair<std::string, room_vnum>>& locations);