local dbat = require("dbat")

local BON = dbat.consts.bonuses
local EF  = dbat.consts.item_extra_flags
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

    if ch:race_get() ~= "android" then
        ch:send_line("Only androids can use repair, maybe you want 'fix' instead?")
        return
    end

    if ch:cooldown_get() > 0 then
        ch:send_line("You must wait a short period before your nanites can repair you.")
        return
    end

    if not ch:is_npc() and not ch:player_flagged(PLR.REPAIR) then
        ch:send_line("You are not a repair model android.")
        return
    end

    local cost = math.floor(ch:meter_max("powerlevel") / 40)
    if ch:meter_current("stamina") < cost then
        ch:send_line("You do not have enough stamina to repair yourself.")
        return
    end

    if ch:meter_current("powerlevel") >= ch:meter_max("powerlevel") then
        ch:send_line("You are already at full functionality and do not require repairs.")
        return
    end

    ch:reveal_hiding(0)
    ch:cooldown_set(10)
    ch:act("You repair some of your outer casings and internal systems, with the small nano-robots contained in your body.", true, nil, nil, "char")
    ch:act("$n stops a moment as small glowing particles move across $s body.", true, nil, nil, "room")

    local repaired = false
    if not ch:is_npc() then
        for _, obj in ch:equipment() do
            if obj:value_get(4) < 100 then
                obj:value_set(4, math.min(100, obj:value_get(4) + 20))
                if obj:extra_flagged(EF.BROKEN) then obj:extra_flag_set(EF.BROKEN, false) end
                repaired = true
            end
        end
    end

    if repaired then
        ch:send_line("@GYour nano-robots also repair all of your equipment a little bit.@n")
    end

    ch:meter_mod_int("stamina", -cost)
    local heal = cost * 2
    if ch:bonus_flagged(BON.HEALER) then heal = heal + math.floor(heal * 0.25) end

    ch:meter_mod_int("powerlevel", heal)
    if ch:meter_current("powerlevel") >= ch:meter_max("powerlevel") then
        ch:send_line("You are fully repaired now.")
    end

    if not ch:is_npc() and math.random(1, 3) == 2 and ch:meter_current("ki") < ch:meter_max("ki") then
        ch:send_line("@GThe repairs have managed to relink power reserves and boost your current energy level.@n")
        ch:meter_mod_int("ki", cost)
    end

    ch:wait_set(PULSE_2SEC)
end

return {
    id = "repair",
    aliases = { {"repair", 5} },
    execute = execute,
}
