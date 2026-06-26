local dbat = require("dbat")
local D    = dbat.consts.directions
local RF   = dbat.consts.room_flags
local EF   = dbat.consts.exit_flags
local PLR  = dbat.consts.plr_flags
local POS  = dbat.consts.positions
local ITEM = dbat.consts.item_types
local P    = dbat.consts.pulses

-- direction index -> name; rev_dir maps each direction to its reverse
local DIR_NAMES = {
    [0]="north", [1]="east", [2]="south", [3]="west",
    [4]="up", [5]="down",
    [6]="northwest", [7]="northeast", [8]="southeast", [9]="southwest",
    [10]="inside", [11]="outside"
}
local REV_DIR = {
    [0]=2, [1]=3, [2]=0, [3]=1, [4]=5, [5]=4,
    [6]=8, [7]=9, [8]=6, [9]=7, [10]=11, [11]=10
}

-- speed tier -> wait state pulses (30/20/15/10/5)
local WAIT_BY_SPEED = { [1]=30, [2]=20, [3]=15, [4]=10, [5]=5 }

-- Named landing locations for special ships (vnum 46000-46099)
-- orbit_vnum is the room vnum the ship must be in; default is the fallback room
local NAMED_LOCATIONS = {
    [50] = {
        default = 300,
        locs = {
            ["nexus city"]              = function() return 300 + math.random(0, 63) end,
            ["south ocean"]             = function() return 800 + math.random(0, 99) end,
            ["nexus field"]             = function() return 1150 + math.random(-16, 28) end,
            ["cherry blossom mountain"] = function() return 1180 + math.random(0, 19) end,
            ["sandy desert"]            = function() return 1287 + math.random(0, 64) end,
            ["northern plains"]         = function() return 1428 + math.random(0, 55) end,
            ["korin's tower"]           = function() return 1456 end,
            ["kami's lookout"]          = function() return 1506 + math.random(0, 30) end,
            ["shadow forest"]           = function() return 1600 + math.random(0, 66) end,
            ["decrepit area"]           = function() return 1710 end,
            ["west city"]               = function() return 19510 end,
            ["hercule beach"]           = function() return 2141 + math.random(0, 53) end,
            ["satan city"]              = function() return 1150 + math.random(-16, 28) end,
        },
        display = "@D------------------[ @GEarth@D ]------------------@c\n"
            .. "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n"
            .. "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n"
            .. "Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n"
            .. "@D---------------------------------------------@n\n"
    },
    [51] = {
        default = 4264,
        locs = {
            ["ice crown city"]    = function() return 4264 end,
            ["ice highway"]       = function() return 4300 end,
            ["topica snowfield"]  = function() return 4351 end,
            ["glug's volcano"]    = function() return 4400 end,
            ["platonic sea"]      = function() return 4600 end,
            ["slave city"]        = function() return 4800 end,
            ["acturian woods"]    = function() return 5100 end,
            ["desolate demesne"]  = function() return 5150 end,
            ["chateau ishran"]    = function() return 5165 end,
            ["wyrm spine mountain"] = function() return 5200 end,
            ["cloud ruler temple"] = function() return 5500 end,
            ["koltoan mine"]      = function() return 4944 end,
        },
        display = "@D------------------[ @CFrigid@D ]------------------@c\n"
            .. "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n"
            .. "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n"
            .. "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n"
            .. "@D---------------------------------------------@n\n"
    },
    [52] = {
        default = 8006,
        locs = {
            ["tiranoc city"]       = function() return 8006 end,
            ["great oroist temple"] = function() return 8300 end,
            ["elzthuan forest"]    = function() return 8400 end,
            ["mazori farm"]        = function() return 8447 end,
            ["dres"]               = function() return 8500 end,
            ["colvian farm"]       = function() return 8600 end,
            ["st alucia"]          = function() return 8700 end,
            ["meridius memorial"]  = function() return 8800 end,
            ["desert of illusion"] = function() return 8900 end,
            ["plains of confusion"] = function() return 8954 end,
            ["turlon fair"]        = function() return 9200 end,
            ["wetlands"]           = function() return 9700 end,
            ["kerberos"]           = function() return 9855 end,
            ["shaeras mansion"]    = function() return 9864 end,
            ["slavinus ravine"]    = function() return 9900 end,
            ["furian citadel"]     = function() return 9949 end,
        },
        display = "@D------------------[ @MKonack@D ]------------------@c\n"
            .. "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n"
            .. "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n"
            .. "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n"
            .. "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n"
            .. "@D---------------------------------------------@n\n"
    },
    [53] = {
        default = 2226,
        locs = {
            ["vegetos city"]      = function() return 2226 end,
            ["blood dunes"]       = function() return 2600 end,
            ["ancestral mountains"] = function() return 2616 end,
            ["destopa swamp"]     = function() return 2709 end,
            ["pride forest"]      = function() return 2800 end,
            ["pride tower"]       = function() return 2899 end,
            ["ruby cave"]         = function() return 2615 end,
        },
        display = "@D------------------[ @YVegeta@D ]------------------@c\n"
            .. "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n"
            .. "Pride Forest, Pride tower, Ruby Cave.\n"
            .. "@D---------------------------------------------@n\n"
    },
    [54] = {
        default = 11600,
        locs = {
            ["senzu village"]     = function() return 11600 end,
            ["guru's house"]      = function() return 10182 end,
            ["crystalline cave"]  = function() return 10474 end,
            ["elder village"]     = function() return 13300 end,
            ["frieza's ship"]     = function() return 10203 end,
            ["kakureta village"]  = function() return 10922 end,
        },
        display = "@D------------------[ @gNamek@D ]------------------@c\n"
            .. "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n"
            .. "Frieza's Ship, Kakureta Village.\n"
            .. "@D---------------------------------------------@n\n"
    },
    [55] = {
        default = 12010,
        locs = {
            ["haven city"]        = function() return 12010 end,
            ["serenity lake"]     = function() return 12103 end,
            ["kaiju forest"]      = function() return 12300 end,
            ["ortusian temple"]   = function() return 12400 end,
            ["silent glade"]      = function() return 12480 end,
        },
        display = "@D------------------[ @BAether@D ]-----------------@c\n"
            .. "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n"
            .. "Silent Glade.\n"
            .. "@D--------------------------------------------@n\n"
    },
    [56] = {
        default = 14008,
        locs = {
            ["yardra city"]   = function() return 14008 end,
            ["jade forest"]   = function() return 14100 end,
            ["jade cliffs"]   = function() return 14200 end,
            ["mount valaria"] = function() return 14300 end,
        },
        display = "@D-----------------[ @mYardrat@D ]-----------------@c\n"
            .. "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n"
            .. "@D-------------------------------------------@n\n"
    },
    [57] = {
        default = 3412,
        locs = {
            ["utatlan city"]  = function() return 3412 end,
            ["zenith jungle"] = function() return 3520 end,
            ["ancient castle"] = function() return 19600 end,
        },
        display = "@D-----------------[ @CZennith@D ]-----------------@c\n"
            .. "Utatlan City, Zenith Jungle, Ancient Castle.\n"
            .. "@D-------------------------------------------@n\n"
    },
    [58] = {
        default = 14904,
        locs = {
            ["aquis city"]        = function() return 14904 end,
            ["yunkai pirate base"] = function() return 15655 end,
        },
        display = "@D-----------------[ @CKanassa@D ]-----------------@c\n"
            .. "Aquis City, Yunkai Pirate Base.\n"
            .. "@D-------------------------------------------@n\n"
    },
    [59] = {
        default = 16009,
        locs = {
            ["janacre"]          = function() return 16009 end,
            ["arlian wasteland"] = function() return 16544 end,
            ["arlia mine"]       = function() return 16600 end,
            ["kemabra wastes"]   = function() return 16816 end,
        },
        display = "@D------------------[ @MArlia@D ]------------------@c\n"
            .. "Janacre, Arlian Wasteland, Arlia Mine, Kemabra Wastes.\n"
            .. "@D---------------------------------------------@n\n"
    },
    [198] = {
        default = 17531,
        locs = {
            ["cerria colony"]      = function() return 17531 end,
            ["crystalline forest"] = function() return 7950 end,
            ["fistarl volcano"]    = function() return 17420 end,
        },
        display = "@D------------------[ @MCerria@D ]------------------@c\n"
            .. "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n"
            .. "@D---------------------------------------------@n\n"
    },
}

-- Pad landing rooms per orbit vnum (pads 1-4, plus secret codes)
local PAD_ROOMS = {
    [50] = {  -- Earth
        ["1"] = 409, ["2"] = 411, ["3"] = 412, ["4"] = 410,
        ["4365"] = 18904, ["6329"] = 18925, ["1983"] = 18995,
    },
    [51] = {  -- Frigid
        ["1"] = 4264, ["2"] = 4263, ["3"] = 4261, ["4"] = 4262,
        ["1337"] = 18116,
    },
    [52] = {  -- Konack
        ["1"] = 8195, ["2"] = 8196, ["3"] = 8197, ["4"] = 8198,
    },
    [53] = {  -- Vegeta
        ["1"] = 2319, ["2"] = 2318, ["3"] = 2320, ["4"] = 2322,
        ["4126"] = 18212,
    },
    [54] = {  -- Namek
        ["1"] = 11628, ["2"] = 11629, ["3"] = 11630, ["4"] = 11627,
    },
    [55] = {  -- Aether
        ["1"] = 12003, ["2"] = 12004, ["3"] = 12006, ["4"] = 12005,
    },
    [56] = {  -- Yardrat
        ["1"] = 14003, ["2"] = 14004, ["3"] = 14005, ["4"] = 14006,
    },
    [57] = {  -- Zenith: defaults to single room
    },
    [58] = {  -- Kanassa: defaults to single room
    },
    [59] = {  -- Arlia
        ["1"] = 16065, ["2"] = 16066, ["3"] = 16067, ["4"] = 16068,
    },
    [198] = {  -- Cerria: defaults to single room
    },
}

-- Planet flag -> orbit vnum for launch command
local PLANET_ORBIT = {
    { flag = RF.EARTH,   orbit = 50  },
    { flag = RF.FRIGID,  orbit = 51  },
    { flag = RF.KONACK,  orbit = 52  },
    { flag = RF.VEGETA,  orbit = 53  },
    { flag = RF.NAMEK,   orbit = 54  },
    { flag = RF.AETHER,  orbit = 55  },
    { flag = RF.YARDRAT, orbit = 56  },
    { flag = RF.KANASSA, orbit = 58  },
    { flag = RF.ARLIA,   orbit = 59  },
    { flag = RF.CERRIA,  orbit = 198 },
}

local function is_zenith_zone(vnum)
    return (vnum >= 3400 and vnum <= 3599) or (vnum >= 62900 and vnum <= 62999) or vnum == 19600
end

local function find_control(ch)
    for obj in ch:room_get():contents() do
        if obj:type_get() == ITEM.CONTROL then return obj end
    end
    for obj in ch:inventory() do
        if obj:type_get() == ITEM.CONTROL then return obj end
    end
    for obj in ch:equipment() do
        if obj:type_get() == ITEM.CONTROL then return obj end
    end
    return nil
end

local function broadcast_engine_sound(room)
    for d = 0, 13 do
        local exit = room:exit_get(d)
        if exit then
            local neighbor = exit:destination()
            if neighbor then
                neighbor:send_line("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n")
            end
        end
    end
end

local function tick_fuel(controls)
    local counter = controls:value_get(3) + 1
    if counter >= 5 then
        local fuel = controls:value_get(2) - 1
        if fuel < 0 then fuel = 0 end
        controls:value_set(2, fuel)
        controls:value_set(3, 0)
    else
        controls:value_set(3, counter)
    end
end

local function fuel_color(fuel)
    if fuel >= 200 then return "@G"
    elseif fuel >= 100 then return "@Y"
    else return "@r" end
end

local function drive_in_direction(ch, controls, vehicle, dir)
    local outer = vehicle:room_get()
    local exit  = outer:exit_get(dir)
    if not exit then
        ch:send_line("@wApparently %s doesn't exist there.", DIR_NAMES[dir])
        return
    end
    local dest = exit:destination()
    if not dest then
        ch:send_line("@wApparently %s doesn't exist there.", DIR_NAMES[dir])
        return
    end
    if exit:flagged(EF.CLOSED) then
        local kw = exit:keyword()
        if kw and kw ~= "" then
            local first = kw:match("^(%S+)")
            ch:send_line("@wThe %s seems to be closed.", first or kw)
        else
            ch:send_line("@wIt seems to be closed.")
        end
        return
    end
    if not (dest:flagged(RF.VEHICLE) or dest:flagged(RF.SPACE)) then
        ch:send_line("@wThe ship can't fit there!")
        return
    end

    outer:send_line("%s @wflies %s.\r\n", vehicle:short_description_get(), DIR_NAMES[dir])
    vehicle:from_room()
    vehicle:to_room(dest)
    tick_fuel(controls)

    ch:act_around("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.")
    ch:send_line("@wThe ship flies onward:")
    ch:look_at_specific_room(dest)
    local fuel = controls:value_get(2)
    ch:send_line("@RFUEL@D: %s%d@n", fuel_color(fuel), fuel)
    broadcast_engine_sound(ch:room_get())
    dest:send_line("%s @wflies in from the %s.\r\n", vehicle:short_description_get(), DIR_NAMES[REV_DIR[dir]])
end

local function drive_into_vehicle(ch, vehicle, arg2)
    if arg2 == "" then
        ch:send_line("@wDrive into what?")
        return
    end
    local target_name = arg2:lower()
    local outer = vehicle:room_get()
    local target_vehicle = nil
    for obj in outer:contents() do
        if obj:type_get() == ITEM.VEHICLE and obj:short_description_get():lower():find(target_name, 1, true) then
            target_vehicle = obj
            break
        end
    end
    if not target_vehicle then
        ch:send_line("@wNothing here by that name!")
        return
    end
    if target_vehicle == vehicle then
        ch:send_line("@wMy, we are in a clever mood today, aren't we.")
        return
    end
    local interior = dbat.rooms.by_id(target_vehicle:value_get(0))
    if not interior or not interior:flagged(RF.VEHICLE) then
        ch:send_line("@wThat ship can't carry other ships.")
        return
    end
    outer:send_line("%s @wenters %s.\r\n", vehicle:short_description_get(), target_vehicle:short_description_get())
    vehicle:from_room()
    vehicle:to_room(interior)
    ch:send_line("@wThe ship flies onward:")
    ch:look_at_specific_room(interior)
    interior:send_line("%s @wenters.\r\n", vehicle:short_description_get())
end

local function drive_outof_vehicle(ch, vehicle)
    local interior = vehicle:room_get()
    local hatch = nil
    for obj in interior:contents() do
        if obj:type_get() == ITEM.HATCH then hatch = obj break end
    end
    if not hatch then
        ch:send_line("@wNowhere to pilot out of.")
        return
    end
    local outer_vehicle = hatch:hatch_vehicle_get()
    if not outer_vehicle then
        ch:send_line("@wYou can't pilot out anywhere!")
        return
    end
    interior:send_line("%s @wexits %s.\r\n", vehicle:short_description_get(), outer_vehicle:short_description_get())
    vehicle:from_room()
    vehicle:to_room(outer_vehicle:room_get())
    ch:act_around("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.")
    ch:send_line("@wThe ship flies onward:")
    ch:look_at_specific_room(vehicle:room_get())
    broadcast_engine_sound(ch:room_get())
    vehicle:room_get():send_line("%s @wflies out of %s.\r\n", vehicle:short_description_get(), outer_vehicle:short_description_get())
end

local function ship_land_location(ch, orbit_vnum, arg2_lower)
    local planet = NAMED_LOCATIONS[orbit_vnum]
    if not planet then return -1 end
    local fn = planet.locs[arg2_lower]
    if fn then
        return fn()
    end
    ch:send_line("You don't know where that made up place is, but decided to land anyway.")
    return planet.default
end

local function disp_ship_locations(ch, orbit_vnum)
    local planet = NAMED_LOCATIONS[orbit_vnum]
    if planet then
        ch:send_line(planet.display)
    else
        ch:send_line("You are not above a planet!")
    end
end

local function handle_land(ch, controls, vehicle, arg2)
    local orbit_vnum = vehicle:room_get():vnum_get()
    local vnum = vehicle:vnum_get()
    local is_special = (vnum >= 46000 and vnum <= 46099)

    if arg2 == "" then
        ch:send_line("@wLand on which pad? 1, 2, 3 or 4?")
        if is_special then
            ch:send_line("@CSpecial Ship Ability@D: @wpilot land (area name)\n@GExample@D: @wpilot land Nexus City")
            disp_ship_locations(ch, orbit_vnum)
        end
        return
    end

    local arg2_lower = arg2:lower()
    local land_vnum = 50  -- sentinel: not a named location
    local is_pad = (arg2_lower == "1" or arg2_lower == "2" or arg2_lower == "3" or arg2_lower == "4")

    if vnum > 46099 then
        -- Regular ship: only pads
        if not is_pad and not PAD_ROOMS[orbit_vnum] then
            -- also check secret codes via fallthrough
        end
        if not is_pad then
            local pad_map = PAD_ROOMS[orbit_vnum]
            local secret = pad_map and pad_map[arg2_lower]
            if secret then
                is_pad = true
                land_vnum = 50  -- use pad-style landing
            else
                ch:send_line("@wLand on which pad? 1, 2, 3 or 4?")
                return
            end
        end
    elseif not is_pad then
        -- Special ship: try named location
        land_vnum = ship_land_location(ch, orbit_vnum, arg2_lower)
        if land_vnum < 0 then
            ch:send_line("@wYou are not above a planet!")
            return
        end
    end

    ch:send_line("@wYou set the controls to descend.@n")
    ch:act_around("@C$n @wmanipulates the ship controls.@n")
    ch:send_line("@RThe ship rocks and shakes as it descends through the atmosphere!")
    ch:act_around("@RThe ship rocks and shakes as it descends through the atmosphere!")

    if land_vnum <= 50 then
        -- Pad landing
        local pad_map = PAD_ROOMS[orbit_vnum]
        local dest_vnum = pad_map and (pad_map[arg2_lower])
        if not dest_vnum then
            ch:send_line("@wLanding sequence aborted, improper coordinates.@n")
            ch:act_around("@wLanding sequence aborted, improper coordinates.@n")
            return
        end
        ch:send_line("@wThe ship has landed.@n")
        ch:act_around("@wThe ship has landed.@n")
        vehicle:from_room()
        vehicle:to_room(dbat.rooms.by_id(dest_vnum))
        ch:look_at_specific_room(vehicle:room_get())
        vehicle:room_get():send_line("%s @wcomes in from above and slowly settles on the launch-pad.@n\r\n", vehicle:short_description_get())
    else
        -- Named location crash landing
        ch:send_line("@wThe ship slams into the ground and forms a small crater!@n")
        ch:act_around("@wThe ship slams into the ground and forms a small crater!@n")
        vehicle:from_room()
        vehicle:to_room(dbat.rooms.by_id(land_vnum))
        ch:look_at_specific_room(vehicle:room_get())
        vehicle:room_get():send_line("%s @wcomes in from above and slams into the ground!@n\r\n", vehicle:short_description_get())
    end
end

local function handle_launch(ch, controls, vehicle)
    local room = vehicle:room_get()
    local orbit_vnum = nil

    for _, p in ipairs(PLANET_ORBIT) do
        if room:flagged(p.flag) then
            orbit_vnum = p.orbit
            break
        end
    end
    if not orbit_vnum then
        -- Check Zenith zone
        if is_zenith_zone(room:vnum_get()) then
            orbit_vnum = 57
        end
    end
    if not orbit_vnum then
        ch:send_line("@wYou are not on a planet.@n")
        return
    end

    ch:send_line("@wYou set the controls to launch.@n")
    ch:act_around("@C$n @wmanipulates the ship controls.@n")
    ch:send_line("@RThe ship shudders as it launches up into the sky!")
    ch:act_around("@RThe ship shudders as it launches up into the sky!")
    ch:send_line("@wThe ship has reached low orbit.@n")
    ch:act_around("@wThe ship has reached low orbit.@n")
    room:send_line("@R%s @Rshudders before blasting off into the sky!@n\r\n", vehicle:short_description_get())

    tick_fuel(controls)
    vehicle:from_room()
    vehicle:to_room(dbat.rooms.by_id(orbit_vnum))
    ch:look_at_specific_room(vehicle:room_get())
    local fuel = controls:value_get(2)
    ch:send_line("@RFUEL@D: %s%d@n", fuel_color(fuel), fuel)
end

local function handle_mark(ch, vehicle, arg2)
    if arg2 == "" or (arg2 ~= "1" and arg2 ~= "2" and arg2 ~= "3") then
        ch:send_line("@wWhich marker are you wanting to launch? 1, 2, or 3?")
        return
    end
    if not vehicle:room_get():flagged(RF.SPACE) then
        ch:send_line("@wYou need to be in space to launch a marker buoy.")
        return
    end
    local spot_vnum = vehicle:room_get():vnum_get()
    if arg2 == "1" then
        if ch:radar1_get() > 0 then ch:send_line("@wYou need to 'deactivate' that marker.") return end
        ch:send_line("@wYou enter a unique code and launch a marker buoy.@n")
        ch:act_around("@C$n@w manipulates the ship controls.@n")
        ch:radar1_set(spot_vnum)
    elseif arg2 == "2" then
        if ch:radar2_get() > 0 then ch:send_line("@wYou need to 'deactivate' that marker.") return end
        ch:send_line("@wYou enter a unique code and launch a marker buoy.@n")
        ch:act_around("@C$n@w manipulates the ship controls.@n")
        ch:radar2_set(spot_vnum)
    elseif arg2 == "3" then
        if ch:radar3_get() > 0 then ch:send_line("@wYou need to 'deactivate' that marker.") return end
        ch:send_line("@wYou enter a unique code and launch a marker buoy.@n")
        ch:act_around("@C$n@w manipulates the ship controls.@n")
        ch:radar3_set(spot_vnum)
    end
end

local function handle_deactivate(ch, arg2)
    if arg2 == "" or (arg2 ~= "1" and arg2 ~= "2" and arg2 ~= "3") then
        ch:send_line("@wWhich marker are you wanting to deactivate? 1, 2, or 3?")
        return
    end
    if arg2 == "1" then
        if ch:radar1_get() <= 0 then ch:send_line("@wYou haven't launched that buoy yet.") return end
        ch:send_line("@wYou enter buoy one's code and command it to deactivate.@n")
        ch:act_around("@C$n@w manipulates the ship controls.@n")
        ch:radar1_set(0)
    elseif arg2 == "2" then
        if ch:radar2_get() <= 0 then ch:send_line("@wYou haven't launched that buoy yet.") return end
        ch:send_line("@wYou enter buoy two's code and command it to deactivate.@n")
        ch:act_around("@C$n@w manipulates the ship controls.@n")
        ch:radar2_set(0)
    elseif arg2 == "3" then
        if ch:radar3_get() <= 0 then ch:send_line("@wYou haven't launched that buoy yet.") return end
        ch:send_line("@wYou enter buoy three's code and command it to deactivate.@n")
        ch:act_around("@C$n@w manipulates the ship controls.@n")
        ch:radar3_set(0)
    end
end

-- Returns direction index for argument string, or nil if not a direction
local DIR_MAP = {
    north=0, n=0,
    east=1, e=1,
    south=2, s=2,
    west=3, w=3,
    up=4, u=4,
    down=5, d=5,
    northwest=6, nw=6, northw=6,
    northeast=7, ne=7, northe=7,
    southeast=8, se=8, southe=8,
    southwest=9, sw=9, southw=9,
    inside=10,
    outside=11,
}

local function execute(ctx)
    local ch     = ctx.ch
    local tokens = ctx.argparams.tokens
    local raw    = ctx.argparams.raw or ""
    local arg    = (tokens[1] or ""):lower()
    local arg2   = raw:match("^%S+%s+(.+)$") or ""

    if not ch:has_arms() then
        ch:send_line("You have no arms!")
        return
    end

    -- unready
    if arg == "unready" and not ch:is_npc() then
        if not ch:player_flagged(PLR.PILOTING) then
            ch:send_line("You are already not flying the ship!")
        else
            ch:act_around("@w$n stands up and stops piloting the ship.")
            ch:send_line("@wYou stand up from the pilot's seat.")
            ch:position_set(POS.STANDING)
            ch:player_flag_set(PLR.PILOTING, false)
        end
        return
    end

    -- ready
    if arg == "ready" and not ch:is_npc() then
        if not find_control(ch) then
            ch:send_line("@wYou have nothing to control here!")
            return
        end
        if ch:player_flagged(PLR.PILOTING) then
            ch:send_line("@wYou are already piloting the ship, try [pilot unready].")
            return
        end
        if ch:carrying_char_get() then
            ch:send_line("@wYou are busy carrying someone.")
            return
        end
        if ch:dragging_get() then
            ch:send_line("@wYou are busy dragging someone.")
            return
        end
        -- Check if another player in same room is already piloting
        for other in ch:room_get():people() do
            if other ~= ch and not other:is_npc() and other:player_flagged(PLR.PILOTING) then
                ch:send_line("@w%s is already piloting the ship!", other:name_get())
                return
            end
        end
        ch:player_flag_set(PLR.PILOTING, true)
        ch:act_around("@w$n sits down and begins piloting the ship.")
        ch:position_set(POS.SITTING)
        ch:send_line("@wYou take a seat in the pilot's chair.")
        return
    end

    -- Need to be piloting for everything else
    if not ch:player_flagged(PLR.PILOTING) then
        ch:send_line("@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]")
        return
    end
    if ch:position_get() < POS.SLEEPING then
        ch:send_line("@wYou can't see anything but stars!")
        return
    end
    if ch:aff_flagged(dbat.consts.aff_flags.BLIND) then
        ch:send_line("@wYou can't see a damned thing, you're blind!")
        return
    end
    if ch:room_get():is_dark() and not ch:can_see_in_dark() then
        ch:send_line("@wIt is pitch black...")
        return
    end

    local controls = find_control(ch)
    if not controls then
        ch:send_line("@wYou have nothing to control here!")
        return
    end
    local vehicle = dbat.objects.find_vehicle(controls:value_get(0))
    if not vehicle then
        ch:send_line("@wYou can't find anything to pilot.")
        return
    end

    if arg == "" then
        ch:send_line("@wPilot, yes, but where?")
        return
    end

    -- into / onto
    if arg == "into" or arg == "onto" or
       (arg:len() >= 2 and ("into"):sub(1, arg:len()) == arg) then
        drive_into_vehicle(ch, vehicle, arg2)
        return
    end

    -- out (without OUTDIR exit in outer room)
    if arg == "out" then
        local outer_exit = vehicle:room_get():exit_get(D.OUTDIR)
        if not outer_exit then
            drive_outof_vehicle(ch, vehicle)
            return
        end
        -- fall through to treat "out" as OUTDIR direction
    end

    -- Direction movement
    local dir = DIR_MAP[arg]
    if dir ~= nil then
        if controls:value_get(2) <= 0 then
            ch:send_line("Your ship doesn't have enough fuel to move.")
            return
        end
        -- Check hatch is closed (CONT_CLOSED bit in value slot 1)
        if (vehicle:value_get(1) & 4) == 0 then
            ch:send_line("@wThe hatch is open, are you insane!?")
            return
        end
        drive_in_direction(ch, controls, vehicle, dir)
        local speed = controls:value_get(1)
        ch:wait_set(WAIT_BY_SPEED[speed] or P.two_sec)
        return
    end

    -- land
    if arg == "land" then
        if controls:value_get(2) <= 0 then
            ch:send_line("Your ship doesn't have enough fuel to move.")
            return
        end
        if (vehicle:value_get(1) & 4) == 0 then
            ch:send_line("@wThe hatch is open, are you insane!?")
            return
        end
        handle_land(ch, controls, vehicle, arg2)
        return
    end

    -- launch
    if arg == "launch" then
        if controls:value_get(2) <= 0 then
            ch:send_line("Your ship doesn't have enough fuel to move.")
            return
        end
        if (vehicle:value_get(1) & 4) == 0 then
            ch:send_line("@wThe hatch is open, are you insane!?")
            return
        end
        handle_launch(ch, controls, vehicle)
        return
    end

    -- mark
    if arg == "mark" then
        handle_mark(ch, vehicle, arg2)
        return
    end

    -- deactivate
    if arg == "deactivate" then
        handle_deactivate(ch, arg2)
        return
    end

    ch:send_line("@wThats not a valid direction.")
    ch:send_line("Try one of these.")
    ch:send_line("[ north/n  | south/s  | east/e  |  west/w  ]")
    ch:send_line("[ up/u | down/d | northeast/ne/northe | northwest/nw/northw]")
    ch:send_line("[  southeast/se/southe  |  southwest/sw/southw]")
    ch:send_line("[  into  |  onto  |  inside  |  outside  ]@n")
    ch:send_line("[ land | launch ]@n")
end

return { id = "drive", aliases = { {"pilot", 5} }, execute = execute }
