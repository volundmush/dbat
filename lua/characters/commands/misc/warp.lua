local dbat = require("dbat")
local RF   = dbat.consts.room_flags
local PLR  = dbat.consts.plr_flags
local ITEM = dbat.consts.item_types

local VALID_ARGS = "Syntax: iwarp [ earth | vegeta | namek | konack | aether | frigid | buoy1 | buoy2 | buoy3 ]\r\n"

local WARP_TARGETS = {
    earth  = { dest = 40979, check_vnums = {40979, 50} },
    namek  = { dest = 42880, check_vnums = {42880, 54} },
    frigid = { dest = 30889, check_vnums = {30889, 51} },
    konack = { dest = 27065, check_vnums = {27065, 52} },
    vegeta = { dest = 32365, check_vnums = {32365, 53} },
    aether = { dest = 41959, check_vnums = {41959, 55} },
}

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

local function do_warp(ch, vehicle, dest_vnum)
    local vname = vehicle:short_description_get()
    local outer = vehicle:room_get()
    outer:send_line("%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", vname)
    ch:send_line("@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n")
    ch:act_around("@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n")
    vehicle:from_room()
    local dest_room = dbat.rooms.by_id(dest_vnum)
    vehicle:to_room(dest_room)
    dest_room:send_line("@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", vname)
    ch:look_at_specific_room(dest_room)
end

local function execute(ctx)
    local ch    = ctx.ch
    local arg   = (ctx.argparams.tokens[1] or ""):lower()

    if ch:is_npc() then return end

    if not ch:has_arms() then
        ch:send_line("You have no arms!")
        return
    end
    if not ch:player_flagged(PLR.PILOTING) then
        ch:send_line("@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]")
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
        ch:send_line(VALID_ARGS)
        return
    end
    if not vehicle:room_get():flagged(RF.SPACE) then
        ch:send_line("Your ship needs to be in space to utilize its Instant Travel Warp Accelerator.")
        return
    end
    if vehicle:vnum_get() ~= 18400 then
        ch:send_line("Your ship is not outfitted with an Instant Travel Warp Accelerator.")
        return
    end

    local cur_vnum = vehicle:room_get():vnum_get()

    local target = WARP_TARGETS[arg]
    if target then
        for _, v in ipairs(target.check_vnums) do
            if cur_vnum == v then
                ch:send_line("Your ship is already there!")
                return
            end
        end
        do_warp(ch, vehicle, target.dest)
        return
    end

    if arg == "buoy1" then
        local r = ch:radar1_get()
        if r <= 0 then ch:send_line("You have not launched that buoy!") return end
        if cur_vnum == r then ch:send_line("Your ship is already there!") return end
        do_warp(ch, vehicle, r)
        return
    end
    if arg == "buoy2" then
        local r = ch:radar2_get()
        if r <= 0 then ch:send_line("You have not launched that buoy!") return end
        if cur_vnum == r then ch:send_line("Your ship is already there!") return end
        do_warp(ch, vehicle, r)
        return
    end
    if arg == "buoy3" then
        local r = ch:radar3_get()
        if r <= 0 then ch:send_line("You have not launched that buoy!") return end
        if cur_vnum == r then ch:send_line("Your ship is already there!") return end
        do_warp(ch, vehicle, r)
        return
    end

    ch:send_line(VALID_ARGS)
end

return { id = "warp", aliases = { {"iwarp", 3} }, execute = execute }
