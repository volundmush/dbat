local dbat = require("dbat")
local POS  = dbat.consts.positions
local P    = dbat.consts.pulses
local SMH  = dbat.consts.secs_per_mud_hour

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if not ch:know_skill("spit") then return end
    if ch:is_fighting() then
        ch:send_line("You can't manage to spit in this fight!")
        return
    end
    if arg == "" then
        ch:send_line("Yes but who do you want to petrify?")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("That target isn't here.")
        return
    end
    if not ch:can_kill(vict) then return end
    if vict:condition_has("stone_spit") then
        ch:act("$N has already been turned to stone.", true, nil, vict, "char")
        return
    end
    if vict:is_fighting() then
        ch:send_line("You can't manage to spit on them, they are moving around too much!")
        return
    end

    local ki_max = ch:meter_max("ki")
    local skill  = ch:skill_get("spit")
    local cost   = math.floor(ki_max / math.floor(skill / 4)) + math.floor(ki_max / 100)

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to petrifiy with your spit!")
        return
    end

    if skill < dbat.axion_dice(0) then
        ch:meter_mod_int("ki", -cost)
        ch:reveal_hiding(0)
        ch:act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, but it falls short of hitting @c$N@W!@n", true, nil, vict, "char")
        ch:act("@C$n@W seems to focus ki before hawking a loogie at you! Fortunatly the loogie falls short.@n", true, nil, vict, "vict")
        ch:act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! Fortunatly for @c$N@W the loogie falls short.@n", true, nil, vict, "notvict")
        ch:improve_skill("spit", 0)
        ch:wait_set(P.two_sec)
    elseif vict:condition_has("zanzoken") and vict:meter_current("stamina") >= 1
           and vict:position_get() ~= POS.SLEEPING then
        ch:meter_mod_int("ki", -cost)
        ch:reveal_hiding(0)
        ch:act("@C$N@c disappears, avoiding your spit before reappearing!@n", false, nil, vict, "char")
        ch:act("@cYou disappear, avoiding @C$n's@c @rstone spit@c before reappearing!@n", false, nil, vict, "vict")
        ch:act("@C$N@c disappears, avoiding @C$n's@c @rstone spit@c before reappearing!@n", false, nil, vict, "notvict")
        vict:condition_remove("zanzoken", "zanzoken_over")
        ch:wait_set(P.two_sec)
        ch:improve_skill("spit", 0)
    else
        vict:condition_apply_with_duration("stone_spit", "skill", "spit", math.random(1, 2) * SMH)
        ch:meter_mod_int("ki", -cost)
        ch:reveal_hiding(0)
        ch:act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, and it smacks into @c$N@W turning $M into stone!@n", true, nil, vict, "char")
        ch:act("@C$n@W seems to focus ki before hawking a loogie at you! It manages to hit and you instantly turn to stone!@n", true, nil, vict, "vict")
        ch:act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! It manages to hit and $E instantly turns to stone!@n", true, nil, vict, "notvict")
        ch:improve_skill("spit", 0)
        ch:wait_set(P.two_sec)
    end
end

return {
    id      = "spit",
    aliases = { {"spit", 3}, {"stone", 4} },
    execute = execute,
}
