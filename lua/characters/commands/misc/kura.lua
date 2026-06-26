local dbat = require("dbat")
local P    = dbat.consts.pulses

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if not ch:know_skill("kuraiiro seiki") then return end

    if ch:meter_current("ki") >= ch:meter_max("ki") then
        ch:send_line("Your ki is already maxed out!")
        return
    end

    if arg == "" then
        ch:send_line("Syntax: kuraiiro (1-100).")
        return
    end

    local num   = math.tointeger(tonumber(arg) or 0) or 0
    local skill = ch:skill_get("kuraiiro seiki")

    if num > skill then
        ch:send_line("The number can not be greater than your skill.")
        return
    end
    if num <= 0 then
        ch:send_line("The number can not be less than 1.")
        return
    end

    local cost = math.floor(ch:meter_max("ki") / 100) * num

    if ch:meter_current("stamina") < cost then
        ch:send_line("You do not have enough stamina for that high a number.")
        return
    end

    ch:reveal_hiding(0)

    if skill <= dbat.axion_dice(0) then
        ch:meter_mod_int("stamina", -cost)
        ch:act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies but you fail!", true, nil, nil, "char")
        ch:act("@c$n@w crouches down and screams as $s eyes turn red and $e attempts to tap into dark energies but fails!", true, nil, nil, "room")
        ch:improve_skill("kuraiiro seiki", 0)
        ch:wait_set(P.two_sec)
        return
    end

    ch:meter_mod_int("stamina", -cost)
    ch:meter_mod_int("ki", cost)
    ch:act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies and succeed as a rush of energy explodes around you!", true, nil, nil, "char")
    ch:act("@c$n@w crouches down and screams as $s eyes turn red. Suddenly $e manages to tap into dark energies and a rush of energy explodes around $m!", true, nil, nil, "room")
    ch:improve_skill("kuraiiro seiki", 0)
    ch:wait_set(P.two_sec)
end

return {
    id      = "kura",
    aliases = { {"kuraiiro", 7} },
    execute = execute,
}
