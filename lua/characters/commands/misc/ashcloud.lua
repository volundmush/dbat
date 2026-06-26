local dbat   = require("dbat")
local act    = dbat.lib.act
local ST     = dbat.consts.sector_types

local ASH_VNUM      = 1305
local ASHCLOUD_VNUM = 1306

local LEVEL_PARAMS = {
    [1] = { mult = 20, initial = 0.25, ticks = 1 },
    [2] = { mult = 10, initial = 0.10, ticks = 2 },
    [3] = { mult =  5, initial = 0.05, ticks = 4 },
}

local function room_is_sunken(room)
    return room:geffect_get() < 0 or room:sector_type_get() == ST.UNDERWATER
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:race_get() ~= "demon" then
        ch:send_line("You are not trained in the use of ash and fire!")
        return
    end
    if arg == "" then
        ch:send_line("Syntax: ashcloud (1 | 2 | 3)")
        return
    end

    -- Find ash in inventory
    local ash = nil
    for obj in ch:inventory() do
        if obj:vnum_get() == ASH_VNUM then
            ash = obj
            break
        end
    end

    -- Check for existing ashcloud in room
    local room = ch:room_get()
    for obj in room:contents() do
        if obj:vnum_get() == ASHCLOUD_VNUM then
            ch:send_line("You can not pile more ash into the air without causing it to clump together and settle.")
            return
        end
    end

    if not ash then
        ch:send_line("You do not have any ash!")
        return
    end

    local level = tonumber(arg)
    local params = level and LEVEL_PARAMS[math.floor(level)]
    if not params then
        ch:send_line("Syntax: ashcloud (1 | 2 | 3)")
        return
    end

    local cost = math.floor(ch:meter_max_get("ki") * params.initial) + ch:stat_get("intelligence") * params.mult

    if ch:meter_get("ki") < cost then
        ch:send_line("You do not have enough ki!")
        return
    end
    if room_is_sunken(room) then
        ch:send_line("You can not create an ashcloud here, because it is too wet.")
        return
    end
    if room:sector_type_get() == ST.SPACE then
        ch:send_line("You can not create an ashcloud in space.")
        return
    end

    ch:reveal_hiding(0)

    if ch:stat_get("intelligence") < dbat.axion_dice(-10) then
        act.to_char(ch, "@RYou take a handful of ashes, and when you go to blow flames at it you lose focus. The ashes are blown from your hands by your huge gust of breath.@n")
        act.around(ch, "@r$n@R takes a handful of ashes from $s belongings and blows it out of $s hands with a strong gust of air. @YStrange.@n", {})
        ash:extract()
        ch:meter_mod_int("ki", -cost)
        return
    end

    local level_msgs = {
        [3] = "@WThe ashes ripple with an intense aftershock of power.@n",
        [2] = "@WThe ashes ripple with a strong aftershock of power.@n",
        [1] = nil,
    }

    act.to_char(ch, "@RYou take a handful of ashes and you create a fierce heat within your lungs. With the heat ready you breathe ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n")
    act.around(ch, "@r$n@R takes a handful of ashes and $e breathes ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", {})

    local extra_msg = level_msgs[math.floor(level)]
    if extra_msg then
        room:send_text(extra_msg .. "\r\n")
    end

    local proto = dbat.obj_protos.by_id(ASHCLOUD_VNUM)
    if not proto then return end
    local ashcloud = proto:spawn()
    ashcloud:to_room(room)
    ash:extract()
    ch:meter_mod_int("ki", -cost)

    ashcloud:script_add("ashcloud")
    local s = ashcloud:script("ashcloud")
    s:number_set("ticks_remaining", params.ticks)
    s:number_set("power",           math.floor(level))
    ashcloud:event_schedule("script:ashcloud:burn",
        math.floor(dbat.consts.secs_per_mud_hour / 3 * 1000),
        math.floor(dbat.consts.secs_per_mud_hour / 3 * 1000))

    -- Burn caster immediately (first tick is instant in C++ original)
    local AFF = dbat.consts.aff_flags
    local PLR = dbat.consts.player_flags
    local race = ch:race_get()
    if race ~= "android" and race ~= "demon" and race ~= "icer" then
        if math.random(1, 15) >= 14 then
            ch:reveal_hiding(0)
            local dmg = math.floor(((ch:meter_max_get("stamina") * 0.005) + 20) * math.floor(level))
            ch:meter_mod_int("stamina", -dmg)
            act.to_char(ch, "@RYou choke on the the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n")
            act.around(ch, "@r$n@R chokes on the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n", {})
            if not ch:plr_flagged(PLR.EYEC) and not ch:aff_flagged(AFF.BLIND) then
                ch:reveal_hiding(0)
                act.to_char(ch, "@DYour eyes sting from the hot ash! You can't see!@n")
                act.around(ch, "@r$n@D eyes appear to have been hurt by the ash!@n", {})
                ch:condition_apply_with_duration("ash_blinded", "skill", "ash_burn",
                    dbat.consts.secs_per_mud_hour)
            end
        end
    end
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return ch:race_get() == "demon"
end

return {
    id          = "ashcloud",
    aliases     = { {"ashcloud", 6} },
    execute     = execute,
    can_execute = can_execute,
}
