local dbat = require("dbat")
local BON  = dbat.consts.bonuses
local POS  = dbat.consts.positions

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if ch:is_npc() or ch:race_get() ~= "kanassan" then
        ch:send_line("You are incapable of this ability.")
        return
    end

    if arg == "" then
        ch:send_line("Bestow advance future sight on who?")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("Bestow advance future sight on who?")
        return
    end

    if vict:condition_has("future_sight") then
        ch:send_line("They already can see the future.")
        return
    end
    if vict:is_npc() then
        ch:send_line("You can't target them, there would be no point.")
        return
    end

    local ki_cost = math.floor(ch:meter_max("ki") / 40)
    if ch:meter_current("ki") < ki_cost then
        ch:send_line("You do not have enough ki.")
        return
    end
    if ch:stat_get("practices") < 100 then
        ch:send_line("You do not have enough PS to activate or pass on this ability.")
        return
    end

    if vict ~= ch then
        if vict:stat_get("speed") + 5 > 70 and vict:bonus_flagged(BON.SLOW) then
            ch:send_line("They can't handle having their speed increased beyond 70.")
            return
        end
        if vict:stat_get("intelligence") + 2 > 70 and vict:bonus_flagged(BON.DULL) then
            ch:send_line("They can't handle having their intelligence increased beyond 70.")
            return
        end
        ch:meter_mod_int("ki", -ki_cost)
        ch:stat_mod("practices", -100)
        ch:reveal_hiding(0)
        ch:act("@CYou focus your energy into your fingers before stabbing your claws into $N and bestowing the power of Future Sight upon $M. Shortly after $E passes out.@n", true, nil, vict, "char")
        ch:act("@C$n focuses $s energy into $s fingers before stabbing $s claws into YOUR neck and bestowing the power of Future Sight upon you! Soon after you pass out!@n", true, nil, vict, "vict")
        ch:act("@C$n focuses $s energy into $s fingers before stabbing $s claws into $N's neck and bestowing the power of Future Sight upon $M! Soon after $E passes out!@n", true, nil, vict, "notvict")
        vict:condition_add("future_sight", "skill", "future_sight")
        vict:position_set(POS.SLEEPING)
    else
        if ch:stat_get("speed") + 5 > 70 and ch:bonus_flagged(BON.SLOW) then
            ch:send_line("You can't handle having your speed increased beyond 70.")
            return
        end
        if ch:stat_get("intelligence") + 2 > 70 and ch:bonus_flagged(BON.DULL) then
            ch:send_line("You can't handle having your intelligence increased beyond 70.")
            return
        end
        ch:meter_mod_int("ki", -ki_cost)
        ch:stat_mod("practices", -100)
        ch:reveal_hiding(0)
        ch:act("@CYou focus your energy into your mind and awaken your latent Future Sight powers!@n", true, nil, nil, "char")
        ch:act("@C$n focuses $s energy while closing $s eyes for a moment.@n", true, nil, nil, "room")
        ch:condition_add("future_sight", "skill", "future_sight")
        ch:position_set(POS.SLEEPING)
    end
end

return {
    id      = "future",
    aliases = { {"future", 4} },
    execute = execute,
}
