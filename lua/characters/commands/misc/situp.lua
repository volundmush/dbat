local dbat = require("dbat")
local BON  = dbat.consts.bonuses
local PLR  = dbat.consts.player_flags
local POS  = dbat.consts.positions
local RF   = dbat.consts.room_flags
local text = dbat.lib.text

local RACE_NO_EXERCISE = {
    android = true,
    bio     = true,
    majin   = true,
    arlian  = true,
}

local GRAVITY_EXTRA_COST = {
    [300]   = 1000000,
    [400]   = 2000000,
    [500]   = 7500000,
    [1000]  = 15000000,
    [5000]  = 25000000,
    [10000] = 50000000,
}

local GRAVITY_BONUS = {
    [0]     = { 1,     1,     "@gYou do a situp.@n", "@g$n does a situp.@n" },
    [10]    = { 3,     7,     "@gYou do a situp, feeling the strain of gravity.@n", "@g$n does a situp, while sweating.@n" },
    [20]    = { 8,     14,    "@gYou do a situp, and feel gravity's pull.@n", "@g$n does a situp, while sweating.@n" },
    [30]    = { 14,    20,    "@gYou do a situp, and feel the burn.@n", "@g$n does a situp, while sweating.@n" },
    [40]    = { 20,    35,    "@gYou do a situp, and feel the burn.@n", "@g$n does a situp, while sweating.@n" },
    [50]    = { 40,    60,    "@gYou do a situp, and feel the burn.@n", "@g$n does a situp, while sweating.@n" },
    [100]   = { 180,   250,   "@gYou do a situp, and really strain against the gravity.@n", "@g$n does a situp, while sweating.@n" },
    [200]   = { 400,   600,   "@gYou do a situp, and really strain against the gravity.@n", "@g$n does a situp, while sweating.@n" },
    [300]   = { 800,   1200,  "@gYou do a situp, and really strain against the gravity.@n", "@g$n does a situp, while sweating.@n" },
    [400]   = { 2000,  3000,  "@gYou do a situp, and really strain against the gravity.@n", "@g$n does a situp, while sweating.@n" },
    [500]   = { 4000,  6000,  "@gYou do a situp, and really strain against the gravity.@n", "@g$n does a situp, while sweating.@n" },
    [1000]  = { 9000,  10000, "@gYou do a situp, and it was a really hard one to finish.@n", "@g$n does a situp, while sweating profusely.@n" },
    [5000]  = { 15000, 20000, "@gYou do a situp, and it was a really hard one to finish.@n", "@g$n does a situp, while sweating profusely.@n" },
    [10000] = { 25000, 30000, "@gYou do a situp, and it was a really hard one to finish.@n", "@g$n does a situp, while sweating profusely.@n" },
}

local function adjust_int(value, amount)
    return math.floor(value + amount)
end

local function compute_cost(ch, gravity)
    local cost = 1
    local is_bardock = ch:sensei_get() == "bardock"

    if (gravity ~= 10 and is_bardock) or not is_bardock then
        cost = cost + gravity * ((gravity * 2) + ch:stat_get("level"))
    else
        cost = cost + gravity * ch:stat_get("level")
    end

    cost = cost + (GRAVITY_EXTRA_COST[gravity] or 0)
    if cost == 1 or cost == 0 then cost = 25 end

    if ch:bonus_flagged(BON.HARDWORKER) then
        cost = adjust_int(cost, -(cost * 0.25))
    elseif ch:bonus_flagged(BON.SLACKER) then
        cost = adjust_int(cost, cost * 0.25)
    end

    return cost
end

local function apply_challenge_message(ch, cost, max_stamina, bonus)
    if cost >= math.floor(max_stamina / 2) then
        ch:send_line("This gravity is a great challenge for you!")
        return bonus * 10
    elseif cost >= math.floor(max_stamina / 4) then
        ch:send_line("This gravity is an awesome challenge for you!")
        return bonus * 8
    elseif cost >= math.floor(max_stamina / 10) then
        ch:send_line("This gravity is a good challenge for you!")
        return bonus * 6
    elseif cost < math.floor(max_stamina / 1000) then
        ch:send_line("This gravity is so easy to you, you could do it in your sleep...")
        return math.floor(bonus / 8)
    elseif cost < math.floor(max_stamina / 100) then
        ch:send_line("This gravity is the opposite of a challenge for you...")
        return math.floor(bonus / 5)
    elseif cost < math.floor(max_stamina / 50) then
        ch:send_line("This gravity is definitely not a challenge for you...")
        return math.floor(bonus / 4)
    elseif cost < math.floor(max_stamina / 30) then
        ch:send_line("This gravity is barely a challenge for you...")
        return math.floor(bonus / 3)
    elseif cost < math.floor(max_stamina / 20) then
        ch:send_line("This gravity is hardly a challenge for you...")
        return math.floor(bonus / 2)
    end

    ch:send_line("This gravity is just perfect for you...")
    return bonus * 4
end

local function wait_for_gravity(ch, gravity)
    if gravity <= 50 then
        ch:wait_set(20)
    elseif gravity <= 200 then
        ch:wait_set(30)
    elseif gravity <= 500 then
        ch:wait_set(40)
    elseif gravity <= 1000 then
        ch:wait_set(50)
    elseif gravity <= 5000 then
        ch:wait_set(60)
    elseif gravity <= 10000 then
        ch:wait_set(70)
    end
end

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

    if ch:is_npc() then
        ch:send_line("You are a mob fool!")
        return
    end
    if blocked_by_position(ch) then return end

    local room = ch:room_get()

    if room:flagged(RF.HELL) then
        ch:send_line("The fire makes it too hot!")
        return
    end
    if ch:dragging_get() then
        ch:send_line("You are dragging someone!")
        return
    end
    if ch:condition_has("fishing") then
        ch:send_line("Stop fishing first.")
        return
    end
    if ch:carrying_char_get() then
        ch:send_line("You are carrying someone!")
        return
    end

    local gravity = room:gravity_get()
    local cost = compute_cost(ch, gravity)

    if RACE_NO_EXERCISE[ch:race_get() or ""] then
        ch:send_line("You will gain nothing from exercising!")
        return
    end

    if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then
        return
    end

    if ch:meter_current("stamina") < cost then
        ch:send_line("You are too tired!")
        return
    end
    if ch:player_flagged(PLR.SPAR) then
        ch:send_line("You shouldn't be sparring if you want to work out, it could be dangerous.")
        return
    end
    if ch:is_fighting() then
        ch:send_line("You are fighting you moron!")
        return
    end
    if ch:condition_has("flying") then
        ch:send_line("You can't do situps in midair!")
        return
    end

    local gravity_bonus = GRAVITY_BONUS[gravity]
    if not gravity_bonus then return end

    local bonus = math.random(gravity_bonus[1], gravity_bonus[2])
    ch:act(gravity_bonus[3], true, nil, nil, "char")
    ch:act(gravity_bonus[4], true, nil, nil, "room")

    bonus = apply_challenge_message(ch, cost, ch:meter_max("stamina"), bonus)

    if ch:is_soft_cap(2) then
        bonus = 0
    end

    local rv = room:vnum_get()
    if room:flagged(RF.HBTC) then
        ch:send_line("@rThis place feels like it operates on a different time frame, it feels great...@n")
        bonus = bonus * 10
    elseif room:flagged(RF.WORKOUT) then
        if rv >= 19100 and rv <= 19199 then
            bonus = bonus * 10
        else
            bonus = bonus * 5
        end
    elseif rv >= 19800 and rv <= 19899 then
        ch:send_line("@rThis place feels like... Magic.@n")
        bonus = bonus * 20
    end

    if bonus <= 0 and not room:flagged(RF.HBTC) then
        bonus = 1
    end
    if bonus <= 0 and room:flagged(RF.HBTC) then
        bonus = 15
    end
    if bonus <= 0 and room:flagged(RF.WORKOUT) then
        if rv >= 19100 and rv <= 19199 then
            bonus = 12
        else
            bonus = 6
        end
    end

    bonus = bonus + math.floor(ch:stat_get("level") / 20)
    if ch:race_get() == "namekian" then
        bonus = bonus - math.floor(bonus / 4)
    end
    if ch:bonus_flagged(BON.HARDWORKER) then
        bonus = math.floor(bonus + bonus * 0.5)
    end
    if ch:bonus_flagged(BON.LONER) then
        bonus = math.floor(bonus + bonus * 0.1)
    end

    ch:send_line("You feel slightly more vigorous @D[@G+%s@D]@n.", text.add_commas(bonus))
    ch:stat_mod("stamina", bonus)
    wait_for_gravity(ch, gravity)
    ch:meter_mod_int("stamina", -cost)
end

return {
    id      = "situp",
    aliases = { {"situp", 5} },
    execute = execute,
}
