local dbat = require("dbat")
local act  = require("lua.libs.act")
local text = dbat.lib.text

local INTERVAL_MS = 5000
local PREF_KI     = dbat.consts.fight_prefs.KI

local TIERS = {
    { threshold =       50000, self_msg = "@RYou continue to powerup, as wind billows out from around you!@n",                   room_msg = "@R$n continues to powerup, as wind billows out from around $m!@n"                  },
    { threshold =      500000, self_msg = "@RYou continue to powerup, as the ground splits beneath you!@n",                      room_msg = "@R$n continues to powerup, as the ground splits beneath $m!@n"                     },
    { threshold =     5000000, self_msg = "@RYou continue to powerup, as the ground shudders and splits beneath you!@n",         room_msg = "@R$n continues to powerup, as the ground shudders and splits beneath $m!@n"         },
    { threshold =    50000000, self_msg = "@RYou continue to powerup, as a huge depression forms beneath you!@n",                room_msg = "@R$n continues to powerup, as a huge depression forms beneath $m!@n"               },
    { threshold =   100000000, self_msg = "@RYou continue to powerup, as the entire area quakes around you!@n",                  room_msg = "@R$n continues to powerup, as the entire area quakes around $m!@n"                 },
    { threshold =   300000000, self_msg = "@RYou continue to powerup, as huge chunks of ground are ripped apart beneath you!@n", room_msg = "@R$n continues to powerup, as huge chunks of ground are ripped apart beanth $m!@n" },
}

local function stop_powerup(ch, self_msg)
    act.to_char(ch, self_msg, {actor=ch})
    act.around(ch, "@R$n stops powering up in a flash of light!@n", {actor=ch})
    ch:send_to_sense(0, "You sense someone stop powering up")
    local pl_str = text.add_commas(ch:meter_current("powerlevel"))
    ch:send_to_scouter(string.format("@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", pl_str), 1, 0)
    ch:condition_remove("powering_up", "silent")
end

local function on_tick(ch)
    if ch:position_get() <= dbat.consts.positions.RESTING then
        ch:condition_remove("powering_up", "interrupted")
        return
    end
    if math.random(1, 3) ~= 3 then return end

    local ki_pref     = ch:preference_get() == PREF_KI
    local gmaxki      = ch:meter_max("ki")
    local ki_threshold = ki_pref and math.floor(gmaxki * 0.0375) + 1 or math.floor(gmaxki / 20)
    local ki_cost      = ki_pref and math.floor(gmaxki * 0.0375)     or math.floor(gmaxki / 20)

    local ghit    = ch:meter_current("powerlevel")
    local gmaxhit = ch:meter_max("powerlevel")
    local gki     = ch:meter_current("ki")

    local function st_boost()
        ch:meter_mod_int("stamina", math.floor(ch:meter_max("stamina") * 0.02))
    end

    if ghit >= gmaxhit and gki >= ki_threshold then
        if ki_pref or gki >= gmaxki * 0.5 then st_boost() end
        ch:restore_announced(false)
        ch:meter_mod_int("ki", -(ki_pref and ki_threshold or math.floor(gmaxki / 20)))
        ch:dispel_ash()
        stop_powerup(ch, "@RYou have reached your maximum!@n")
        return
    end

    if gki < ki_threshold then
        ch:meter_mod_int("ki", -ki_threshold)
        stop_powerup(ch, "@RYou have run out of ki.@n")
        return
    end

    -- Active tick: healing and ki drain
    ch:meter_mod("powerlevel", 0.1)
    ch:meter_mod_int("ki", -ki_cost)
    if ch:meter_current("ki") >= gmaxki * 0.5 then st_boost() end

    local self_msg = "@RYou continue to powerup, as the very air around you crackles and burns!@n"
    local room_msg = "@R$n continues to powerup, as the very air around $m crackles and burns!@n"
    gmaxhit = ch:meter_max("powerlevel")
    for _, tier in ipairs(TIERS) do
        if gmaxhit < tier.threshold then
            self_msg = tier.self_msg
            room_msg = tier.room_msg
            break
        end
    end
    act.to_char(ch, self_msg, {actor=ch})
    act.around(ch, room_msg, {actor=ch})
    ch:send_to_sense(0, "You sense someone powering up")
    ch:send_to_worlds()
    local pl_str = text.add_commas(ch:meter_current("powerlevel"))
    ch:send_to_scouter(string.format("@D[@GBlip@D]@r Rising Powerlevel Detected@D: [@Y%s@D]", pl_str), 1, 0)
    ch:dispel_ash()
end

return {
    id         = "powering_up",
    name       = "Powering Up",
    persistent = false,
    on_apply = function(ch, cond)
        cond:schedule_event("tick", INTERVAL_MS, INTERVAL_MS)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch) end
    end,
}
