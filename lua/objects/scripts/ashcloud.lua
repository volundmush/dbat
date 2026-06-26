local dbat = require("dbat")
local act  = dbat.lib.act
local AFF  = dbat.consts.aff_flags
local PLR  = dbat.consts.player_flags

-- Burns characters standing in the same room as the ashcloud.
-- Immune races: android, demon, icer.
-- Stamina damage = ((max_stamina * 0.005) + 20) * power
-- Blinds players whose eyes are open (1 MUD hour duration).
local function burn_character(ch, power)
    local race = ch:race_get()
    if race == "android" or race == "demon" or race == "icer" then return end

    if math.random(1, 15) < 14 then return end  -- ~1-in-7 chance (same as C++)

    ch:reveal_hiding(0)
    local dmg = math.floor(((ch:meter_max_get("stamina") * 0.005) + 20) * power)
    ch:meter_mod_int("stamina", -dmg)
    act.to_char(ch, "@RYou choke on the the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n")
    act.around(ch, "@r$n@R chokes on the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n", {})

    if not ch:is_npc() then
        if not ch:plr_flagged(PLR.EYEC) and not ch:aff_flagged(AFF.BLIND) then
            ch:reveal_hiding(0)
            act.to_char(ch, "@DYour eyes sting from the hot ash! You can't see!@n")
            act.around(ch, "@r$n@D eyes appear to have been hurt by the ash!@n", {})
            ch:condition_apply_with_duration("ash_blinded", "skill", "ash_burn",
                dbat.consts.secs_per_mud_hour)
        end
    end
end

local BURN_INTERVAL_MS = math.floor(dbat.consts.secs_per_mud_hour / 3 * 1000)

return {
    id = "ashcloud",

    on_apply = function(obj, script)
        obj:event_schedule("script:ashcloud:burn", BURN_INTERVAL_MS, BURN_INTERVAL_MS)
    end,

    on_remove = function(obj, script, reason)
        obj:event_cancel("script:ashcloud:burn")
    end,

    on_event = function(obj, script, event)
        if event ~= "script:ashcloud:burn" then return end

        local room = obj:room_get()
        if not room then obj:extract(); return end

        for person in room:people() do
            burn_character(person, script:number_get("power"))
        end

        local ticks = script:number_get("ticks_remaining") - 1
        if ticks <= 0 then
            room:send_text("@WThe ashes settle to the ground and go out.@n\r\n")
            obj:extract()
        else
            script:number_set("ticks_remaining", ticks)
        end
    end,
}
