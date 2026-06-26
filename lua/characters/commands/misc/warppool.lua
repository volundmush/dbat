local dbat = require("dbat")
local act  = dbat.lib.act
local RF   = dbat.consts.room_flags
local ST   = dbat.consts.sector_types

local DESTINATIONS = {
    earth   = 850,
    frigid  = 4609,
    namek   = 10904,
    kanassa = 15100,
    aether  = 12252,
}

local PLANET_FLAGS = {
    earth   = RF.EARTH,
    frigid  = RF.FRIGID,
    kanassa = RF.KANASSA,
    namek   = RF.NAMEK,
    aether  = RF.AETHER,
}

local MSG_FAIL_CHAR = "@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n"
local MSG_FAIL_ROOM = "@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n"
local MSG_WIN_CHAR  = "@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n"
local MSG_WIN_ROOM  = "@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n"
local MSG_ARRIVE    = "@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n"

local function in_valid_sea(ch)
    local vnum = ch:room_get():vnum_get()
    local room = ch:room_get()
    return (vnum >= 4600 and vnum < 4700)
        or (vnum >= 795 and vnum < 1099)
        or (vnum >= 15100 and vnum < 15299)
        or (vnum >= 13155 and vnum < 13199)
        or (room:flagged(RF.NAMEK) and room:sector_type_get() == ST.WATER_NOSWIM)
        or (vnum >= 12103 and vnum < 12289)
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are grappling with someone!")
        return
    end
    if ch:absorbing_get() or ch:absorbed_by_get() then
        ch:send_line("You are struggling with someone!")
        return
    end
    if ch:sits_get() then
        ch:send_line("You should get up first.")
        return
    end

    local cost = math.floor(ch:meter_max_get("ki") / 20)

    if arg == "" then
        ch:send_line("What planet are you wanting to warp to?\n[ earth | frigid | kanassa | namek | aether ]")
        return
    end
    if ch:meter_get("ki") < cost then
        ch:send_line("You do not have enough ki to perform the technique.")
        return
    end
    if not in_valid_sea(ch) then
        ch:send_line("You must be on or in a sea or ocean for warp pool to work.")
        return
    end

    local dest_vnum = DESTINATIONS[arg]
    if not dest_vnum then
        ch:send_line("That is not an acceptable choice. It must be a planet with a large body of water.\n[ earth | frigid | kanassa | namek | aether ]")
        return
    end

    local pf = PLANET_FLAGS[arg]
    if pf and ch:room_get():flagged(pf) then
        ch:send_line("You are already on %s!", arg:sub(1,1):upper() .. arg:sub(2))
        return
    end

    local perc = ch:skill_get("warp")
    local prob = dbat.axion_dice(0)

    if prob > perc then
        act.to_char(ch, MSG_FAIL_CHAR)
        act.around(ch, MSG_FAIL_ROOM, {})
        ch:meter_mod_int("ki", -cost)
        ch:improve_skill("warp", 1)
    else
        act.to_char(ch, MSG_WIN_CHAR)
        act.around(ch, MSG_WIN_ROOM, {})
        ch:improve_skill("warp", 1)
        ch:from_room()
        ch:to_room(dbat.rooms.by_id(dest_vnum))
        act.around(ch, MSG_ARRIVE, {})
        ch:meter_mod_int("ki", -cost)
    end
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return ch:know_skill("warp")
end

return {
    id          = "warppool",
    aliases     = { {"warppool", 5} },
    execute     = execute,
    can_execute = can_execute,
}
