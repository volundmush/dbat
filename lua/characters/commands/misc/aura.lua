local dbat = require("dbat")
local PLR  = dbat.consts.player_flags

local AURA_NAMES = { "white", "blue", "red", "green", "pink", "purple", "yellow", "black", "orange" }

local function aura_name(ch)
    return AURA_NAMES[ch:aura_get() + 1] or "white"
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = string.lower(ctx.argparams.tokens[1] or "")

    if arg == "" then
        ch:send_line("Syntax: aura light")
        return
    end

    if ch:charge_get() > 0 then
        ch:send_line("You can't focus enough on this while charging.")
        return
    end

    if ch:condition_has("powering_up") then
        ch:send_line("You are busy powering up!")
        return
    end

    if arg == "light" then
        if ch:skill_get("focus") < 75 or ch:skill_get("concentration") < 75 then
            ch:send_line("You need at least a skill level of 75 in Focus and Concentration to use this.")
            return
        end

        if ch:player_flagged(PLR.AURALIGHT) then
            ch:send_line("Your aura fades as you stop shining light.")
            ch:act("$n's aura fades as they stop shining light on the area.", true, nil, nil, "room")
            ch:condition_remove("auralight", "player_deactivated")
        elseif ch:meter_current("ki") > math.floor(ch:meter_max("ki") * 0.12) then
            ch:meter_mod("ki", -120000)  -- -12%
            ch:send_line("A bright %s aura begins to burn around you as you provide light to the surrounding area!", aura_name(ch))
            ch:act(string.format("@wA %s aura flashes up brightly around $n@w as they provide light to the area.@n", aura_name(ch)), true, nil, nil, "room")
            ch:condition_apply("auralight", "aura", "player_activated")
        else
            ch:send_line("You don't have enough KI to do that.")
        end
    end
end

return {
    id      = "aura",
    aliases = { {"aura", 4} },
    execute = execute,
}
