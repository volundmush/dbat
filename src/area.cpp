#include "dbat/area.h"
#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/send.h"

static const std::unordered_set<WhereFlag> planetFlags = {
    {WhereFlag::planet_earth, WhereFlag::planet_vegeta, WhereFlag::planet_frigid, 
        WhereFlag::planet_namek, WhereFlag::planet_konack, WhereFlag::planet_aether,
         WhereFlag::planet_yardrat, WhereFlag::planet_kanassa, WhereFlag::planet_cerria,
          WhereFlag::planet_arlia},
};

static const std::unordered_map<room_vnum, WhereFlag> planetOrbits = {
    {ORBIT_EARTH, WhereFlag::planet_earth},
    {ORBIT_VEGETA, WhereFlag::planet_vegeta},
    {ORBIT_FRIGID, WhereFlag::planet_frigid},
    {ORBIT_NAMEK, WhereFlag::planet_namek},
    {ORBIT_KONACK, WhereFlag::planet_konack},
    {ORBIT_AETHER, WhereFlag::planet_aether},
    {ORBIT_YARDRAT, WhereFlag::planet_yardrat},
    {ORBIT_KANASSA, WhereFlag::planet_kanassa},
    {ORBIT_CERRIA, WhereFlag::planet_cerria},
    {ORBIT_ARLIA, WhereFlag::planet_arlia},
    {ORBIT_ZENITH, WhereFlag::moon_zenith},
};

std::optional<WhereFlag> checkOrbit(const room_vnum room) {
    if(auto find = planetOrbits.find(room); find != planetOrbits.end()) {
        return find->second;
    }
    return {};
}

std::optional<WhereFlag> getPlanet(const room_vnum room) {
    auto r = get_room(room);
    if(!r) return {};

    // this approach covers most, but not all, planets.
    for(const auto& planet : planetFlags) {
        if(r->where_flags.get(planet)) return planet;
    }

    if((room >= 3400 && room <= 3599) || (room >= 62900 && room <= 62999) || room == 19600)
        return WhereFlag::moon_zenith;

    return {};

};

std::string getPlanetName(const WhereFlag planet) {
    switch(planet) {
        case WhereFlag::planet_earth: return "Earth";
        case WhereFlag::planet_vegeta: return "Vegeta";
        case WhereFlag::planet_frigid: return "Frigid";
        case WhereFlag::planet_namek: return "Namek";
        case WhereFlag::planet_konack: return "Konack";
        case WhereFlag::planet_aether: return "Aether";
        case WhereFlag::planet_yardrat: return "Yardrat";
        case WhereFlag::planet_kanassa: return "Kanassa";
        case WhereFlag::planet_cerria: return "Cerria";
        case WhereFlag::planet_arlia: return "Arlia";
        case WhereFlag::moon_zenith: return "Zenith";
        default: return "Unknown";
    }
}

std::string getPlanetColorName(const WhereFlag planet) {
    switch(planet) {
        case WhereFlag::planet_earth: return "@GEarth@n";
        case WhereFlag::planet_vegeta: return "@YVegeta@n";
        case WhereFlag::planet_frigid: return "@CFrigid@n";
        case WhereFlag::planet_namek: return "@gNamek@n";
        case WhereFlag::planet_konack: return "@MKonack@n";
        case WhereFlag::planet_aether: return "@MAether@n";
        case WhereFlag::planet_yardrat: return "@mYardrat@n";
        case WhereFlag::planet_kanassa: return "@BKanassa@n";
        case WhereFlag::planet_cerria: return "@RCerria@n";
        case WhereFlag::planet_arlia: return "@GArlia@n";
        case WhereFlag::moon_zenith: return "@BZenith@n";
        default: return "Unknown";
    }
}

room_vnum getPlanetOrbit(const WhereFlag planet) {
    switch(planet) {
        case WhereFlag::planet_earth: return ORBIT_EARTH;
        case WhereFlag::planet_vegeta: return ORBIT_VEGETA;
        case WhereFlag::planet_frigid: return ORBIT_FRIGID;
        case WhereFlag::planet_namek: return ORBIT_NAMEK;
        case WhereFlag::planet_konack: return ORBIT_KONACK;
        case WhereFlag::planet_aether: return ORBIT_AETHER;
        case WhereFlag::planet_yardrat: return ORBIT_YARDRAT;
        case WhereFlag::planet_kanassa: return ORBIT_KANASSA;
        case WhereFlag::planet_cerria: return ORBIT_CERRIA;
        case WhereFlag::planet_arlia: return ORBIT_ARLIA;
        case WhereFlag::moon_zenith: return ORBIT_ZENITH;
        default: return NOWHERE;
    }
}

std::optional<double> getPlanetEnvironment(const WhereFlag planet, const int environment) {
    switch(environment) {
        case ENV_GRAVITY:
            switch(planet) {
                case WhereFlag::planet_vegeta:
                    return 10.0;
            }
            break;
        case ENV_MOONLIGHT:
            switch(planet) {
                case WhereFlag::planet_earth:
                case WhereFlag::planet_aether:
                case WhereFlag::planet_vegeta:
                case WhereFlag::planet_frigid:
                    return MOON_TIMECHECK() ? 100.0 : 0.0;
                default:
                    return -1;
            }
        case ENV_ETHER_STREAM:
            switch(planet) {
                case WhereFlag::planet_earth:
                case WhereFlag::planet_aether:
                case WhereFlag::planet_namek:
                case WhereFlag::moon_zenith:
                    return 100.0;
                default: return 0.0;
            }
            break;
    };

    return {};
};

using land_spots = std::vector<std::pair<std::string, room_vnum>>;


static const land_spots land_earth = {
    {"Nexus City", 300},
    {"South Ocean", 800},
    {"Nexus Field", 1150},
    {"Cherry Blossom Mountain", 1180},
    {"Sandy Desert", 1287},
    {"Northern Plains", 1428},
    {"Korin's Tower", 1456},
    {"Kami's Lookout", 1506},
    {"Shadow Forest", 1636},
    {"Decrepit Area", 1710},
    {"West City", 19510},
    {"Hercule Beach", 2141},
    {"Satan City", 13020},
};

static const land_spots land_frigid = {
    {"Ice Crown City", 4264},
    {"Ice Highway", 4300},
    {"Topica Snowfield", 4351},
    {"Glug's Volcano", 4400},
    {"Platonic Sea", 4600},
    {"Slave City", 4800},
    {"Acturian Woods", 5100},
    {"Desolate Demesne", 5150},
    {"Chateau Ishran", 5165},
    {"Wyrm Spine Mountain", 5200},
    {"Cloud Ruler Temple", 5500},
    {"Koltoan Mine", 4944},
};

static const land_spots land_konack = {
    {"Tiranoc City", 8006},
    {"Great Oroist Temple", 8300},
    {"Elzthuan Forest", 8400},
    {"Mazori Farm", 8447},
    {"Dres", 8500},
    {"Colvian Farm", 8600},
    {"St Alucia", 8700},
    {"Meridius Memorial", 8800},
    {"Desert of Illusion", 8900},
    {"Plains of Confusion", 8954},
    {"Turlon Fair", 9200},
    {"Wetlands", 9700},
    {"Kerberos", 9855},
    {"Shaeras Mansion", 9864},
    {"Slavinus Ravine", 9900},
    {"Furian Citadel", 9949},
};

static const land_spots land_vegeta = {
    {"Vegetos City", 2226},
    {"Blood Dunes", 2600},
    {"Ancestral Mountains", 2616},
    {"Destopa Swamp", 2709},
    {"Pride Forest", 2800},
    {"Pride Tower", 2899},
    {"Ruby Cave", 2615},
};

static const land_spots land_namek = {
    {"Senzu Village", 11600},
    {"Guru's House", 10182},
    {"Crystalline Cave", 10474},
    {"Elder Village", 13300},
    {"Frieza's Ship", 10203},
    {"Kakureta Village", 10922},
};

static const land_spots land_aether = {
    {"Haven City", 12010},
    {"Serenity Lake", 12103},
    {"Kaiju Forest", 12300},
    {"Ortusian Temple", 12400},
    {"Silent Glade", 12480},
};

static const land_spots land_yardrat = {
    {"Yardra City", 14008},
    {"Jade Forest", 14100},
    {"Jade Cliffs", 14200},
    {"Mount Valaria", 14300},
};

static const land_spots land_kanassa = {
    {"Aquis City", 14904},
    {"Yunkai Pirate Base", 15655},
};

static const land_spots land_cerria = {
    {"Cerria Colony", 17531},
    {"Crystalline Forest", 7950},
    {"Fistarl Volcano", 17420},
};

static const land_spots land_arlia = {
    {"Janacre", 16009},
    {"Arlian Wasteland", 16544},
    {"Arlia Mine", 16600},
};

static const land_spots land_zenith = {
    {"Utatlan City", 3412},
    {"Zenith Jungle", 3520},
    {"Ancient Castle", 19600},
};

land_spots getPlanetLandspots(const WhereFlag orbit) {
    switch(orbit) {
        case WhereFlag::planet_earth: return land_earth;
        case WhereFlag::planet_vegeta: return land_vegeta;
        case WhereFlag::planet_frigid: return land_frigid;
        case WhereFlag::planet_namek: return land_namek;
        case WhereFlag::planet_konack: return land_konack;
        case WhereFlag::planet_aether: return land_aether;
        case WhereFlag::planet_yardrat: return land_yardrat;
        case WhereFlag::planet_kanassa: return land_kanassa;
        case WhereFlag::planet_cerria: return land_cerria;
        case WhereFlag::planet_arlia: return land_arlia;
        case WhereFlag::moon_zenith: return land_zenith;
        default: return {};
    }
}

void displayLandSpots(struct char_data *ch, const std::string& planet_name, const land_spots& locations) {
    const int line_length = 60;  // Adjust the max line length as needed
    int current_length = 0;

        ch->send_to("@D------------------[ %s@D ]------------------@c\n", planet_name.c_str());

    for (size_t i = 0; i < locations.size(); ++i) {
        const std::string& location = locations[i].first;

        // Check if adding this location would exceed the line length
        if (current_length + location.length() + 2 > line_length) { // +2 for the ", "
                        ch->sendText("\n");  // Start a new line
            current_length = 0;
        }

        // Add the location to the line
                ch->send_to("%s", location.c_str());
        current_length += location.length();

        // Add a comma and space unless it's the last item
        if (i < locations.size() - 1) {
                        ch->sendText(", ");
            current_length += 2;
        }
    }

        ch->sendText(".\n");  // End the list with a period and new line
        ch->sendText("@D---------------------------------------------@n\n");
}

static const land_spots dock_earth = {
    {"1", 409},
    {"2", 411},
    {"3", 412},
    {"4", 410},
    {"Nexus City", 300},
    {"South Ocean", 800},
    {"Nexus Field", 1150},
    {"Cherry Blossom Mountain", 1180},
    {"Sandy Desert", 1287},
    {"Northern Plains", 1428},
    {"Korin's Tower", 1456},
    {"Kami's Lookout", 1506},
    {"Shadow Forest", 1600},
    {"Decrepit Area", 1710},
    {"West City", 19510},
    {"Hercule Beach", 2141},
    {"Satan City", 13020},
};

static const land_spots dock_frigid = {
    {"1", 4264},
    {"2", 4263},
    {"3", 4261},
    {"4", 4262},
    {"Ice Crown City", 4264},
    {"Ice Highway", 4300},
    {"Topica Snowfield", 4351},
    {"Glug's Volcano", 4400},
    {"Platonic Sea", 4600},
    {"Slave City", 4800},
    {"Acturian Woods", 5100},
    {"Desolate Demesne", 5150},
    {"Chateau Ishran", 5165},
    {"Wyrm Spine Mountain", 5200},
    {"Cloud Ruler Temple", 5500},
    {"Koltoan Mine", 4944},
};

static const land_spots dock_konack = {
    {"1", 8195},
    {"2", 8196},
    {"3", 8197},
    {"4", 8198},
    {"Tiranoc City", 8006},
    {"Great Oroist Temple", 8300},
    {"Elzthuan Forest", 8400},
    {"Mazori Farm", 8447},
    {"Dres", 8500},
    {"Colvian Farm", 8600},
    {"St Alucia", 8700},
    {"Meridius Memorial", 8800},
    {"Desert of Illusion", 8900},
    {"Plains of Confusion", 8954},
    {"Turlon Fair", 9200},
    {"Wetlands", 9700},
    {"Kerberos", 9855},
    {"Shaeras Mansion", 9864},
    {"Slavinus Ravine", 9900},
    {"Furian Citadel", 9949},
};

static const land_spots dock_vegeta = {
    {"1", 2319},
    {"2", 2318},
    {"3", 2320},
    {"4", 2322},
    {"Vegetos City", 2226},
    {"Blood Dunes", 2600},
    {"Ancestral Mountains", 2616},
    {"Destopa Swamp", 2709},
    {"Pride Forest", 2800},
    {"Pride Tower", 2899},
    {"Ruby Cave", 2615},
};

static const land_spots dock_namek = {
    {"1", 11628},
    {"2", 11629},
    {"3", 11630},
    {"4", 11627},
    {"Senzu Village", 11600},
    {"Guru's House", 10182},
    {"Crystalline Cave", 10474},
    {"Elder Village", 13300},
    {"Frieza's Ship", 10203},
    {"Kakureta Village", 10922},
};

static const land_spots dock_aether = {
    {"1", 12003},
    {"2", 12004},
    {"3", 12006},
    {"4", 12005},
    {"Haven City", 12010},
    {"Serenity Lake", 12103},
    {"Kaiju Forest", 12300},
    {"Ortusian Temple", 12400},
    {"Silent Glade", 12480},
};

static const land_spots dock_yardrat = {
    {"1", 14003},
    {"2", 14004},
    {"3", 14005},
    {"4", 14006},
    {"Yardra City", 14008},
    {"Jade Forest", 14100},
    {"Jade Cliffs", 14200},
    {"Mount Valaria", 14300},
};

static const land_spots dock_kanassa = {
    {"Aquis City", 14904},
    {"Yunkai Pirate Base", 15655},
};

static const land_spots dock_cerria = {
    {"Cerria Colony", 17531},
    {"Crystalline Forest", 7950},
    {"Fistarl Volcano", 17420},
};

static const land_spots dock_arlia = {
    {"1", 16065},
    {"2", 16066},
    {"3", 16067},
    {"4", 16068},
    {"Janacre", 16009},
    {"Arlian Wasteland", 16544},
    {"Arlia Mine", 16600},
    {"Kemabra Wastes", 16816},
};

static const land_spots dock_zenith = {
    {"Utatlan City", 3412},
    {"Zenith Jungle", 3520},
    {"Ancient Castle", 19600},
};

land_spots getPlanetSpacepads(const WhereFlag orbit) {
    switch(orbit) {
        case WhereFlag::planet_earth: return dock_earth;
        case WhereFlag::planet_vegeta: return dock_vegeta;
        case WhereFlag::planet_frigid: return dock_frigid;
        case WhereFlag::planet_namek: return dock_namek;
        case WhereFlag::planet_konack: return dock_konack;
        case WhereFlag::planet_aether: return dock_aether;
        case WhereFlag::planet_yardrat: return dock_yardrat;
        case WhereFlag::planet_kanassa: return dock_kanassa;
        case WhereFlag::planet_cerria: return dock_cerria;
        case WhereFlag::planet_arlia: return dock_arlia;
        case WhereFlag::moon_zenith: return dock_zenith;
        default: return {};
    }
}