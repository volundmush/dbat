local dbat = require("dbat")
local PLR  = dbat.consts.player_flags
local POS  = dbat.consts.positions

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if ch:is_npc() then return end

    if arg == "" then
        ch:send_line("Rip the tail off who?")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("That target isn't here.")
        return
    end

    if not vict:player_flagged(PLR.TAIL) and not vict:player_flagged(PLR.STAIL) then
        ch:send_line("They do not have a tail to rip off!")
        return
    end

    local st_cost = math.floor(ch:meter_max("stamina") / 20)

    if ch ~= vict and ch:position_get() > POS.SLEEPING then
        if ch:meter_current("stamina") < st_cost then
            ch:send_line("You are too tired to manage to grab their tail!")
            return
        end
        ch:meter_mod_int("stamina", -st_cost)
        if ch:der_total("speed_index") > vict:der_total("speed_index") then
            if ch:meter_current("powerlevel") > vict:meter_current("powerlevel") * 2 then
                ch:reveal_hiding(0)
                ch:act("@rYou rush at @R$N@r and grab $S tail! With a powerful tug you pull it off!@n", true, nil, vict, "char")
                ch:act("@R$n@r rushes at YOU and grabs your tail! With a powerful tug $e pulls it off!@n", true, nil, vict, "vict")
                ch:act("@R$n@R rushes at @R$N@r and grab $S tail! With a powerful tug $e pulls it off!@n", true, nil, vict, "notvict")
                vict:lose_tail()
            else
                ch:reveal_hiding(0)
                ch:act("@rYou rush at @R$N@r and grab $S tail! You are too weak to pull it off though!@n", true, nil, vict, "char")
                ch:act("@R$n@r rushes at YOU and grabs your tail! $e is too weak to pull it off though!@n", true, nil, vict, "vict")
                ch:act("@R$n@R rushes at @R$N@r and grab $S tail! $e is too weak to pull it off though!@n", true, nil, vict, "notvict")
            end
        else
            ch:reveal_hiding(0)
            ch:act("@rYou rush at @R$N@r and try to grab $S tail, but fail!@n", true, nil, vict, "char")
            ch:act("@R$n@r rushes at YOU and tries to grab your tail, but fails!@n", true, nil, vict, "vict")
            ch:act("@R$n@R rushes at @R$N@r and tries to grab $S tail, but fails!@n", true, nil, vict, "notvict")
        end
    elseif ch == vict then
        ch:reveal_hiding(0)
        ch:act("@rYou grab your own tail and yank it off!@n", true, nil, nil, "char")
        ch:act("@R$n@r grabs $s own tail and yanks it off!@n", true, nil, nil, "room")
        vict:lose_tail()
    else
        if ch:meter_current("stamina") < st_cost then
            ch:send_line("You are too tired to manage to grab their tail!")
            return
        end
        ch:meter_mod_int("stamina", -math.floor(ch:meter_max("ki") / 20))
        ch:reveal_hiding(0)
        ch:act("@rYou reach and grab @R$N's@r tail! With a powerful tug you pull it off!@n", true, nil, vict, "char")
        ch:act("@RYou feel your tail pulled off!@n", true, nil, vict, "vict")
        ch:act("@R$n@R reaches and grabs @R$N's@r tail! With a powerful tug $e pulls it off!@n", true, nil, vict, "notvict")
        vict:lose_tail()
    end
end

return {
    id      = "rip",
    aliases = { {"rip", 2} },
    execute = execute,
}
