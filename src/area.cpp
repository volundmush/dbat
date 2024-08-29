#include "dbat/area.h"
#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/utils.h"

static const std::unordered_map<int, int> planetFlags = {
    {ROOM_EARTH, PLANET_EARTH},
    {ROOM_VEGETA, PLANET_VEGETA},
    {ROOM_FRIGID, PLANET_FRIGID},
    {ROOM_NAMEK, PLANET_NAMEK},
    {ROOM_KONACK, PLANET_KONACK},
    {ROOM_AETHER, PLANET_AETHER},
    {ROOM_YARDRAT, PLANET_YARDRAT},
    {ROOM_KANASSA, PLANET_KANASSA},
    {ROOM_CERRIA, PLANET_CERRIA},
    {ROOM_ARLIA, PLANET_ARLIA},
};

int getPlanet(const room_vnum room) {
    auto find = world.find(room);
    if(find == world.end()) return 0;

    auto &r = find->second;

    // this approach covers most, but not all, planets.
    for(const auto& [flag, planet] : planetFlags) {
        if(r.room_flags.test(flag)) return planet;
    }

    if((room >= 3400 && room <= 3599) || (room >= 62900 && room <= 62999) || room == 19600)
        return PLANET_ZENITH;

    return 0;

};

std::string getPlanetName(const int planet) {
    switch(planet) {
        case PLANET_EARTH: return "Earth";
        case PLANET_VEGETA: return "Vegeta";
        case PLANET_FRIGID: return "Frigid";
        case PLANET_NAMEK: return "Namek";
        case PLANET_KONACK: return "Konack";
        case PLANET_AETHER: return "Aether";
        case PLANET_YARDRAT: return "Yardrat";
        case PLANET_KANASSA: return "Kanassa";
        case PLANET_CERRIA: return "Cerria";
        case PLANET_ARLIA: return "Arlia";
        case PLANET_ZENITH: return "Zenith";
        default: return "Unknown";
    }
}

std::string getPlanetColorName(const int planet) {
    switch(planet) {
        case PLANET_EARTH: return "@GEarth@n";
        case PLANET_VEGETA: return "@YVegeta@n";
        case PLANET_FRIGID: return "@CFrigid@n";
        case PLANET_NAMEK: return "@gNamek@n";
        case PLANET_KONACK: return "@MKonack@n";
        case PLANET_AETHER: return "@MAether@n";
        case PLANET_YARDRAT: return "@mYardrat@n";
        case PLANET_KANASSA: return "@BKanassa@n";
        case PLANET_CERRIA: return "@RCerria@n";
        case PLANET_ARLIA: return "@GArlia@n";
        case PLANET_ZENITH: return "@BZenith@n";
        default: return "Unknown";
    }
}

room_vnum getPlanetOrbit(const int planet) {
    switch(planet) {
        case PLANET_EARTH: return ORBIT_EARTH;
        case PLANET_VEGETA: return ORBIT_VEGETA;
        case PLANET_FRIGID: return ORBIT_FRIGID;
        case PLANET_NAMEK: return ORBIT_NAMEK;
        case PLANET_KONACK: return ORBIT_KONACK;
        case PLANET_AETHER: return ORBIT_AETHER;
        case PLANET_YARDRAT: return ORBIT_YARDRAT;
        case PLANET_KANASSA: return ORBIT_KANASSA;
        case PLANET_CERRIA: return ORBIT_CERRIA;
        case PLANET_ARLIA: return ORBIT_ARLIA;
        case PLANET_ZENITH: return ORBIT_ZENITH;
        default: return NOWHERE;
    }
}

std::optional<double> getPlanetEnvironment(const int planet, const int environment) {
    switch(environment) {
        case ENV_GRAVITY:
            switch(planet) {
                case PLANET_VEGETA:
                    return 10.0;
            }
            break;
        case ENV_MOONLIGHT:
            switch(planet) {
                case PLANET_EARTH:
                case PLANET_AETHER:
                case PLANET_VEGETA:
                case PLANET_FRIGID:
                    return MOON_TIMECHECK() ? 100.0 : 0.0;
                default:
                    return -1;
            }
        case ENV_ETHER_STREAM:
            switch(planet) {
                case PLANET_EARTH:
                case PLANET_AETHER:
                case PLANET_NAMEK:
                case PLANET_ZENITH:
                    return 100.0;
                default: return 0.0;
            }
            break;
    };

    return {};
};

static const std::vector<std::pair<std::string, room_vnum>> land_earth = {
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

static const std::vector<std::pair<std::string, room_vnum>> land_frigid = {
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

static const std::vector<std::pair<std::string, room_vnum>> land_konack = {
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

static const std::vector<std::pair<std::string, room_vnum>> land_vegeta = {
    {"Vegetos City", 2226},
    {"Blood Dunes", 2600},
    {"Ancestral Mountains", 2616},
    {"Destopa Swamp", 2709},
    {"Pride Forest", 2800},
    {"Pride Tower", 2899},
    {"Ruby Cave", 2615},
};

static const std::vector<std::pair<std::string, room_vnum>> land_namek = {
    {"Senzu Village", 11600},
    {"Guru's House", 10182},
    {"Crystalline Cave", 10474},
    {"Elder Village", 13300},
    {"Frieza's Ship", 10203},
    {"Kakureta Village", 10922},
};

static const std::vector<std::pair<std::string, room_vnum>> land_aether = {
    {"Haven City", 12010},
    {"Serenity Lake", 12103},
    {"Kaiju Forest", 12300},
    {"Ortusian Temple", 12400},
    {"Silent Glade", 12480},
};

static const std::vector<std::pair<std::string, room_vnum>> land_yardrat = {
    {"Yardra City", 14008},
    {"Jade Forest", 14100},
    {"Jade Cliffs", 14200},
    {"Mount Valaria", 14300},
};

static const std::vector<std::pair<std::string, room_vnum>> land_kanassa = {
    {"Aquis City", 14904},
    {"Yunkai Pirate Base", 15655},
};

static const std::vector<std::pair<std::string, room_vnum>> land_cerria = {
    {"Cerria Colony", 17531},
    {"Crystalline Forest", 7950},
    {"Fistarl Volcano", 17420},
};

static const std::vector<std::pair<std::string, room_vnum>> land_arlia = {
    {"Janacre", 16009},
    {"Arlian Wasteland", 16544},
    {"Arlia Mine", 16600},
};

static const std::vector<std::pair<std::string, room_vnum>> land_zenith = {
    {"Utatlan City", 3412},
    {"Zenith Jungle", 3520},
    {"Ancient Castle", 19600},
};

std::vector<std::pair<std::string, room_vnum>> getPlanetLandspots(const room_vnum orbit) {
    switch(orbit) {
        case ORBIT_EARTH: return land_earth;
        case ORBIT_VEGETA: return land_vegeta;
        case ORBIT_FRIGID: return land_frigid;
        case ORBIT_NAMEK: return land_namek;
        case ORBIT_KONACK: return land_konack;
        case ORBIT_AETHER: return land_aether;
        case ORBIT_YARDRAT: return land_yardrat;
        case ORBIT_KANASSA: return land_kanassa;
        case ORBIT_CERRIA: return land_cerria;
        case ORBIT_ARLIA: return land_arlia;
        case ORBIT_ZENITH: return land_zenith;
        default: return {};
    }
}

void displayLandSpots(struct char_data *ch, const std::string& planet_name, const std::vector<std::pair<std::string, room_vnum>>& locations) {
    const int line_length = 60;  // Adjust the max line length as needed
    int current_length = 0;

    send_to_char(ch, "@D------------------[ %s@D ]------------------@c\n", planet_name.c_str());

    for (size_t i = 0; i < locations.size(); ++i) {
        const std::string& location = locations[i].first;

        // Check if adding this location would exceed the line length
        if (current_length + location.length() + 2 > line_length) { // +2 for the ", "
            send_to_char(ch, "\n");  // Start a new line
            current_length = 0;
        }

        // Add the location to the line
        send_to_char(ch, "%s", location.c_str());
        current_length += location.length();

        // Add a comma and space unless it's the last item
        if (i < locations.size() - 1) {
            send_to_char(ch, ", ");
            current_length += 2;
        }
    }

    send_to_char(ch, ".\n");  // End the list with a period and new line
    send_to_char(ch, "@D---------------------------------------------@n\n");
}

static const std::vector<std::pair<std::string, room_vnum>> dock_earth = {
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

static const std::vector<std::pair<std::string, room_vnum>> dock_frigid = {
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

static const std::vector<std::pair<std::string, room_vnum>> dock_konack = {
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

static const std::vector<std::pair<std::string, room_vnum>> dock_vegeta = {
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

static const std::vector<std::pair<std::string, room_vnum>> dock_namek = {
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

static const std::vector<std::pair<std::string, room_vnum>> dock_aether = {
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

static const std::vector<std::pair<std::string, room_vnum>> dock_yardrat = {
    {"1", 14003},
    {"2", 14004},
    {"3", 14005},
    {"4", 14006},
    {"Yardra City", 14008},
    {"Jade Forest", 14100},
    {"Jade Cliffs", 14200},
    {"Mount Valaria", 14300},
};

static const std::vector<std::pair<std::string, room_vnum>> dock_kanassa = {
    {"Aquis City", 14904},
    {"Yunkai Pirate Base", 15655},
};

static const std::vector<std::pair<std::string, room_vnum>> dock_cerria = {
    {"Cerria Colony", 17531},
    {"Crystalline Forest", 7950},
    {"Fistarl Volcano", 17420},
};

static const std::vector<std::pair<std::string, room_vnum>> dock_arlia = {
    {"1", 16065},
    {"2", 16066},
    {"3", 16067},
    {"4", 16068},
    {"Janacre", 16009},
    {"Arlian Wasteland", 16544},
    {"Arlia Mine", 16600},
    {"Kemabra Wastes", 16816},
};

static const std::vector<std::pair<std::string, room_vnum>> dock_zenith = {
    {"Utatlan City", 3412},
    {"Zenith Jungle", 3520},
    {"Ancient Castle", 19600},
};

std::pair<int, std::vector<std::pair<std::string, room_vnum>>> getPlanetSpacepads(const room_vnum orbit) {
    switch(orbit) {
        case ORBIT_EARTH: return {PLANET_EARTH, dock_earth};
        case ORBIT_VEGETA: return {PLANET_VEGETA, dock_vegeta};
        case ORBIT_FRIGID: return {PLANET_FRIGID, dock_frigid};
        case ORBIT_NAMEK: return {PLANET_NAMEK, dock_namek};
        case ORBIT_KONACK: return {PLANET_KONACK, dock_konack};
        case ORBIT_AETHER: return {PLANET_AETHER, dock_aether};
        case ORBIT_YARDRAT: return {PLANET_YARDRAT, dock_yardrat};
        case ORBIT_KANASSA: return {PLANET_KANASSA, dock_kanassa};
        case ORBIT_CERRIA: return {PLANET_CERRIA, dock_cerria};
        case ORBIT_ARLIA: return {PLANET_ARLIA, dock_arlia};
        case ORBIT_ZENITH: return {PLANET_ZENITH, dock_zenith};
        default: return {};
    }
}