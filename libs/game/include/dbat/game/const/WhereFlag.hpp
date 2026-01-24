#pragma once
#include <cstdint>

enum class WhereFlag : std::uint8_t
{
    planet_earth = 0,             // Room is on Earth
    earth_orbit = 1,              // Earth Orbit
    planet_vegeta = 2,            // Room is on Vegeta
    vegeta_orbit = 3,             // Vegeta Orbit
    planet_frigid = 4,            // Room is on Frigid
    frigid_orbit = 5,             // Frigid Orbit
    planet_konack = 6,            // Room is on Konack
    konack_orbit = 7,             // Konack Orbit
    planet_namek = 8,             // Room is on Namek
    namek_orbit = 9,              // Namek Orbit
    planet_aether = 10,           // Room is on Aether
    aether_orbit = 11,            // Aether Orbit
    planet_yardrat = 12,          // This room is on planet Yardrat
    yardrat_orbit = 13,           // Yardrat Orbit
    planet_kanassa = 14,          // This room is on planet Kanassa
    kanassa_orbit = 15,           // Kanassa Orbit
    planet_arlia = 16,            // This room is on planet Arlia
    arlia_orbit = 17,             // Arlia Orbit
    planet_cerria = 18,           // This room is on planet Cerria
    cerria_orbit = 19,            // This room is in Cerria's Orbit
    moon_zenith = 20,             // (unspecified)
    zenith_orbit = 21,            // zenith orbit
    neo_nirvana = 22,             // Room is on Neo
    afterlife = 23,               // Room is on AL
    afterlife_hell = 24,          // Room is HELLLLLLL
    hyperbolic_time_chamber = 25, // Room is extra special training area
    pendulum_past = 26,           // Inside the pendulum room
    space = 27,                   // Room is on Space
    nebula = 28,                  // Nebulae
    asteroid = 29,                // Asteroid
    wormhole = 30,                // Wormhole
    space_station = 31,           // Space Station
    star = 32                     // Is a star
};
