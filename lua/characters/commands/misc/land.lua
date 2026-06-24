local dbat = require("dbat")
local act  = require("lua.libs.act")

local LOCATIONS = {
    [50] = {
        default = 300,
        display = "@D------------------[ @GEarth@D ]------------------@c\n" ..
                  "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n" ..
                  "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n" ..
                  "Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Nexus City",              300},
            {"South Ocean",             800},
            {"Nexus Field",             1150},
            {"Cherry Blossom Mountain", 1180},
            {"Sandy Desert",            1287},
            {"Northern Plains",         1428},
            {"Korin's Tower",           1456},
            {"Kami's Lookout",          1506},
            {"Shadow Forest",           1636},
            {"Decrepit Area",           1710},
            {"West City",               19510},
            {"Hercule Beach",           2141},
            {"Satan City",              13020},
        },
    },
    [51] = {
        default = 4264,
        display = "@D------------------[ @CFrigid@D ]------------------@c\n" ..
                  "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n" ..
                  "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n" ..
                  "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Ice Crown City",      4264},
            {"Ice Highway",         4300},
            {"Topica Snowfield",    4351},
            {"Glug's Volcano",      4400},
            {"Platonic Sea",        4600},
            {"Slave City",          4800},
            {"Acturian Woods",      5100},
            {"Desolate Demesne",    5150},
            {"Chateau Ishran",      5165},
            {"Wyrm Spine Mountain", 5200},
            {"Cloud Ruler Temple",  5500},
            {"Koltoan Mine",        4944},
        },
    },
    [52] = {
        default = 8006,
        display = "@D------------------[ @MKonack@D ]------------------@c\n" ..
                  "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n" ..
                  "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n" ..
                  "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n" ..
                  "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Tiranoc City",        8006},
            {"Great Oroist Temple", 8300},
            {"Elzthuan Forest",     8400},
            {"Mazori Farm",         8447},
            {"Dres",                8500},
            {"Colvian Farm",        8600},
            {"St Alucia",           8700},
            {"Meridius Memorial",   8800},
            {"Desert of Illusion",  8900},
            {"Plains of Confusion", 8954},
            {"Turlon Fair",         9200},
            {"Wetlands",            9700},
            {"Kerberos",            9855},
            {"Shaeras Mansion",     9864},
            {"Slavinus Ravine",     9900},
            {"Furian Citadel",      9949},
        },
    },
    [53] = {
        default = 2226,
        display = "@D------------------[ @YVegeta@D ]------------------@c\n" ..
                  "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n" ..
                  "Pride Forest, Pride tower, Ruby Cave.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Vegetos City",        2226},
            {"Blood Dunes",         2600},
            {"Ancestral Mountains", 2616},
            {"Destopa Swamp",       2709},
            {"Pride forest",        2800},
            {"Pride Tower",         2899},
            {"Ruby Cave",           2615},
        },
    },
    [54] = {
        default = 11600,
        display = "@D------------------[ @gNamek@D ]------------------@c\n" ..
                  "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n" ..
                  "Frieza's Ship, Kakureta Village.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Senzu Village",    11600},
            {"Guru's House",     10182},
            {"Crystalline Cave", 10474},
            {"Elder Village",    13300},
            {"Frieza's Ship",    10203},
            {"Kakureta Village", 10922},
        },
    },
    [55] = {
        default = 12010,
        display = "@D------------------[ @BAether@D ]-----------------@c\n" ..
                  "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n" ..
                  "Silent Glade.\n" ..
                  "@D--------------------------------------------@n",
        locs = {
            {"Haven City",      12010},
            {"Serenity Lake",   12103},
            {"Kaiju Forest",    12300},
            {"Ortusian Temple", 12400},
            {"Silent Glade",    12480},
        },
    },
    [56] = {
        default = 14008,
        display = "@D-----------------[ @mYardrat@D ]-----------------@c\n" ..
                  "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n" ..
                  "@D-------------------------------------------@n",
        locs = {
            {"Yardra City",   14008},
            {"Jade Forest",   14100},
            {"Jade Cliffs",   14200},
            {"Mount Valaria", 14300},
        },
    },
    [57] = {
        default = 3412,
        display = "@D-----------------[ @CZennith@D ]-----------------@c\n" ..
                  "Utatlan City, Zenith Jungle, Ancient Castle.\n" ..
                  "@D-------------------------------------------@n",
        locs = {
            {"Utatlan City",   3412},
            {"Zenith Jungle",  3520},
            {"Ancient Castle", 19600},
        },
    },
    [58] = {
        default = 14904,
        display = "@D-----------------[ @CKanassa@D ]-----------------@c\n" ..
                  "Aquis City, Yunkai Pirate Base.\n" ..
                  "@D-------------------------------------------@n",
        locs = {
            {"Aquis City",         14904},
            {"Yunkai Pirate Base", 15655},
        },
    },
    [59] = {
        default = 16009,
        display = "@D------------------[ @MArlia@D ]------------------@c\n" ..
                  "Janacre, Arlian Wasteland, Arlia Mine.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Janacre",          16009},
            {"Arlian Wasteland", 16544},
            {"Arlia Mine",       16600},
        },
    },
    [198] = {
        default = 17531,
        display = "@D------------------[ @MCerria@D ]------------------@c\n" ..
                  "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n" ..
                  "@D---------------------------------------------@n",
        locs = {
            {"Cerria Colony",      17531},
            {"Crystalline Forest", 7950},
            {"Fistarl Volcano",    17420},
        },
    },
}

local function land_location(ch, arg)
    local planet = LOCATIONS[ch:room_get():vnum_get()]
    if not planet then
        ch:send_line("You are not above a planet!")
        return -1
    end
    local lower = arg:lower()
    for _, loc in ipairs(planet.locs) do
        if loc[1]:lower() == lower then return loc[2] end
    end
    ch:send_line("You don't know where that made up place is, but decided to land anyway.")
    return planet.default
end

local function disp_locations(ch)
    local planet = LOCATIONS[ch:room_get():vnum_get()]
    if not planet then
        ch:send_line("You are not above a planet!")
        return
    end
    ch:send_line(planet.display)
end

local SKY_ROOMS = {}
for vnum in pairs(LOCATIONS) do SKY_ROOMS[vnum] = true end

return {
    id = "land",
    aliases = {
        {"land", 4},
    },
    execute = function(ctx)
        local ch    = ctx.ch
        local arg   = ctx.argparams and ctx.argparams.tokens and ctx.argparams.tokens[1] or ""
        if ctx.argparams and ctx.argparams.tokens then
            arg = table.concat(ctx.argparams.tokens, " ")
        end
        arg = arg:match("^%s*(.-)%s*$")

        local vnum = ch:room_get() and ch:room_get():vnum_get() or 0
        local above_planet = SKY_ROOMS[vnum] == true

        if arg == "" then
            if above_planet then
                ch:send_line("Land where?")
                disp_locations(ch)
            else
                ch:send_line("You are not even in the lower atmosphere of a planet!")
            end
            return
        end

        local landing = land_location(ch, arg)
        if landing ~= -1 then
            local was_in = vnum
            ch:send_line("You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.")
            ch:from_room()
            ch:to_room(landing)
            local location_name = ch:sense_location() or "the surface"
            ch:from_room()
            ch:to_room(was_in)
            local msg = string.format("@C$n@Y flies down through the atmosphere toward @G%s@Y!@n", location_name)
            act.around(ch, msg, {actor=ch})
            ch:from_room()
            ch:to_room(landing)
            ch:fly_zone("can be seen landing from space nearby!@n\r\n")
            ch:send_to_sense(1, "landing on the planet")
            ch:send_to_scouter("A powerlevel signal has been detected landing on the planet", 0, 1)
            act.around(ch, "$n comes down from high above in the sky and quickly lands on the ground.", {actor=ch})
        end
    end,
}
