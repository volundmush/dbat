local dbat = require("dbat")

local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local PULSE_2SEC = 20

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.STANDING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    elseif pos == POS.RESTING then
        ch:send_line("Nah... You feel too relaxed to do that..")
    elseif pos == POS.SITTING then
        ch:send_line("Maybe you should get on your feet first?")
    end

    return true
end

local function execute(ctx)
    local ch = ctx.ch

    if blocked_by_position(ch) then return end

    if ch:is_npc() or ch:race_get() ~= "android" then
        ch:send_line("Only androids can use recharge")
        return
    end

    if ch:cooldown_get() > 0 then
        ch:send_line("You must wait a short period before your nanites can convert your ki.")
        return
    end

    if not ch:player_flagged(PLR.REPAIR) then
        ch:send_line("You are not a repair model android.")
        return
    end

    local cost = math.floor(ch:meter_max("stamina") / 20)
    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to recharge your stamina.")
        return
    end

    if ch:meter_current("stamina") >= ch:meter_max("stamina") then
        ch:send_line("Your energy reserves are already full.")
        return
    end

    ch:reveal_hiding(0)
    ch:cooldown_set(10)
    ch:act("You focus your ki into your energy reserves, recharging them some.", true, nil, nil, "char")
    ch:act("$n stops and glows green briefly.", true, nil, nil, "room")
    ch:meter_mod_int("ki", -cost)

    if ch:meter_current("stamina") + cost * 2 < ch:meter_max("stamina") then
        ch:meter_mod_int("stamina", cost * 2)
    else
        ch:meter_set_int("stamina", ch:meter_max("stamina"))
        ch:send_line("You are fully recharged now.")
    end

    ch:wait_set(PULSE_2SEC)
end

return {
    id = "recharge",
    aliases = { {"recharge", 6} },
    execute = execute,
}
