local dbat = require("dbat")

local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local PULSE_1SEC = 10

local KAIOKEN_LEVELS = {
    [0] = 0,
    [1] = 0,
    [2] = 0,
    [3] = 5000,
    [4] = 10000,
    [5] = 15000,
    [6] = 25000,
    [7] = 35000,
    [8] = 50000,
    [9] = 75000,
    [10] = 100000,
    [11] = 150000,
    [12] = 200000,
    [13] = 250000,
    [14] = 300000,
    [15] = 400000,
    [16] = 500000,
    [17] = 600000,
    [18] = 700000,
    [19] = 800000,
    [20] = 1000000,
}

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

local function has_kaioken_skill(ch)
    return ch:is_npc() or ch:know_skill("kaioken")
end

local function remove_kaioken(ch)
    if ch:stat_get("kaioken") <= 0 then return end
    ch:stat_set("kaioken", 0)
    ch:condition_remove("kaioken", "kaioken_removed")
    ch:send_line("You drop out of kaioken.")
    ch:act("$n@w drops out of kaioken.@n", true, nil, nil, "room")
end

local function apply_kaioken(ch, level)
    ch:stat_set("kaioken", level)
    ch:condition_apply_number("kaioken", "level", level)
    ch:condition_remove("powering_up", "kaioken")
    ch:send_line("@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n", level)
    ch:act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, nil, nil, "room")
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end
    if not has_kaioken_skill(ch) then return end

    if ch:stat_get("alignment") <= -50 then
        ch:send_line("Your heart is too corrupt to use that technique!")
        return
    end

    if not ch:is_npc() and ch:player_flagged(PLR.HEALT) then
        ch:send_line("You are inside a healing tank!")
        return
    end

    if arg == "" then
        ch:send("What level of kaioken do you want to try and achieve?\r\nSyntax: kaioken 1-20\r\n")
        return
    end

    local target = tonumber(arg) or 0
    target = math.floor(target)

    if target < 0 or target > 20 then
        ch:send("That level of kaioken dosn't exist...\r\nSyntax: kaioken 0-20\r\n")
        return
    end

    local current = ch:stat_get("kaioken")
    if target == 0 then
        if current > 0 then
            remove_kaioken(ch)
        else
            ch:send_line("You are not in kaioken!")
        end
        return
    end

    if target == current then
        ch:send_line("You are already at that kaioken level! To release, try kaioken 0")
        return
    end

    if not ch:is_npc() and (ch:is_transformed() or (ch:race_get() == "hoshijin" and ch:starphase_get() > 0)) and target > 5 then
        ch:send_line("You can not manage a kaioken level higher than 5 when transformed.")
        return
    end

    local cost_unit = math.floor(ch:meter_max("ki") / 50)
    local cost = (cost_unit * target) - (cost_unit * current)
    if target < current then cost = 0 end

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to focus into your body for that level.")
        return
    end

    local roll = math.random(1, target * 5 + 1)
    ch:reveal_hiding(0)
    ch:meter_mod_int("ki", -cost)
    ch:improve_skill("kaioken", 1)

    if ch:init_skill("kaioken") < roll then
        ch:send_line("You try to focus your ki into your body but mess up somehow.")
        ch:act("$n tries to use kaioken but messes up somehow.", true, nil, nil, "room")
        ch:wait_set(PULSE_1SEC)
        return
    end

    if ch:meter_max("powerlevel") < KAIOKEN_LEVELS[target] then
        ch:act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", true, nil, nil, "char")
        ch:act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", true, nil, nil, "room")
        return
    end

    apply_kaioken(ch, target)
    ch:wait_set(PULSE_1SEC)
end

return {
    id      = "kaioken",
    aliases = { {"kaioken", 7} },
    execute = execute,
}
