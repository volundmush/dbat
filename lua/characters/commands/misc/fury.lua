local dbat = require("dbat")

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if ch:race_get() ~= "halfbreed" or ch:is_npc() then
        ch:send_line("You are furious, but you'll get over it.")
        return
    end

    if ch:condition_has("halfbreed_fury") then
        ch:send_line("You are already furious, your next attack will devestate, hurry use it!")
        return
    end

    if ch:stat_get("fury") < 100 then
        ch:send_line("You do not have enough anger to release your fury upon your foes!")
        return
    end

    if arg == "" then
        if ch:meter_current("powerlevel") < ch:meter_max("powerlevel") then
            local cur_lf = ch:meter_current("lifeforce")
            if cur_lf >= ch:meter_max("lifeforce") * 0.2 then
                ch:meter_set("powerlevel", 1000000)
                ch:meter_mod("lifeforce", -200000)
            else
                ch:meter_mod_int("powerlevel", cur_lf)
                ch:meter_mod("lifeforce", -2000000)
            end
        end
        ch:stat_set("fury", 0)
    elseif arg == "attack" then
        ch:stat_set("fury", 50)
    else
        ch:send_line("Syntax: fury (attack) <--- this will not use up your LF to restore PL.")
        ch:send_line("        fury <--- fury by itself will do both LF to PL restore and attack boost.")
        return
    end

    ch:reveal_hiding(0)
    ch:act("You release your fury! Your very next attack is guaranteed to rip your foes a new one!", true, nil, nil, "char")
    ch:act("$n screams furiously as a look of anger appears on $s face!", true, nil, nil, "room")
    ch:condition_add("halfbreed_fury", "fury", "released")
end

return {
    id      = "fury",
    aliases = { {"fury", 4} },
    execute = execute,
}
