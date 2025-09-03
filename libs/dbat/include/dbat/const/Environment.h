#pragma once

// I'm thinking that rooms with lava might automatically have a considerable heat value.
// They might also generate light. Molten lava glows.
// The below are queried by thing_data::getMyEnvironment(int type).
constexpr int ENV_GRAVITY = 0;       // For rooms with special gravity. val is x Gs. 1 is Earth-like.
constexpr int ENV_LAVA = 1;          // For rooms with lava. val is how much? how does one measure lava? 0 means no lava.
constexpr int ENV_WATER = 2;         // For rooms with water. val is how much? 0 means no water. but what should I measure water by?
constexpr int ENV_TEMPERATURE = 3;   // For rooms with heat. val is how much in C. Can be negative, obviously.
constexpr int ENV_LIGHT = 4;         // For rooms with light. val is how much?
constexpr int ENV_PRESSURE = 5;      // Atmospheric pressure. it's in atmospheres. 1 is normal. Kami's Lookout... maybe 0.5? Space is 0.0.
constexpr int ENV_RADIATION = 6;     // Radiation level. measured in rads. 0 is normal.
constexpr int ENV_SUNLIGHT = 7;      // how much sunlight is available. This should be 0 during the night, 100 at zenith, and in-between otherwise.
constexpr int ENV_MOONLIGHT = 8;     // how much moonlight is available. 100 is a full moon. For those Oozaru freaks. And Lycanthropes. And whatever else.
constexpr int ENV_WIND = 9;          // Wind speed in... kmh maybe? 0 is no wind.
constexpr int ENV_HUMIDITY = 10;     // Humidity level. 0 is bone dry, 100 is a rainforest.
constexpr int ENV_OXYGEN = 11;       // Oxygen level. 0 is no oxygen, 100 is normal. Multiply by pressure to get breathability.
constexpr int ENV_TOXICITY = 12;     // The environment is toxic. This can be used for all kinds of systems. 0.0 is no toxicity.
constexpr int ENV_CORROSIVITY = 13;  // The environment is corrosive. This might inflict injury or damage equipment over time. 0.0 is no corrosivity.
constexpr int ENV_ETHER_STREAM = 14; // Ether Stream value for Hoshijin. 0 is no ether stream. 100 is maximum.
