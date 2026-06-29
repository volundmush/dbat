local dbat = require("dbat")

local Search = dbat.lib.search.new
local text   = dbat.lib.text

local BON = dbat.consts.bonuses
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions
local RF  = dbat.consts.room_flags

local SYNTAX = "Syntax: meditate (object)\nSyntax: meditate expand\nSyntax: meditate break"

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
    [0]     = { 1,     1 },
    [10]    = { 2,     4 },
    [20]    = { 5,     10 },
    [30]    = { 10,    15 },
    [40]    = { 15,    20 },
    [50]    = { 40,    60 },
    [100]   = { 180,   250 },
    [200]   = { 400,   600 },
    [300]   = { 800,   1200 },
    [400]   = { 2000,  3000 },
    [500]   = { 4000,  6000 },
    [1000]  = { 9000,  10000 },
    [5000]  = { 15000, 20000 },
    [10000] = { 25000, 30000 },
}

local function command_position_blocked(ch)
    local pos = ch:position_get()
    if pos >= POS.SITTING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    end

    return true
end

local function adjust_int(value, amount)
    return math.floor(value + amount)
end

local function compute_cost(ch, room, weight)
    local gravity = room:gravity_get()
    local cost = 1
    local is_bardock = ch:sensei_get() == "bardock"

    if (gravity ~= 10 and is_bardock) or not is_bardock then
        cost = cost + gravity * ((gravity * 2) + ch:stat_get("level"))
    else
        cost = cost + gravity * ch:stat_get("level")
    end

    cost = cost + weight * math.floor((gravity + 1) / 5)
    cost = cost + (GRAVITY_EXTRA_COST[gravity] or 0)
    if cost < weight then cost = weight end
    if cost == 1 or cost == 0 then cost = 25 end

    if ch:bonus_flagged(BON.HARDWORKER) then
        cost = adjust_int(cost, -(cost * 0.25))
    elseif ch:bonus_flagged(BON.SLACKER) then
        cost = adjust_int(cost, cost * 0.25)
    end

    return cost
end

local function apply_challenge_message(ch, cost, max_ki, bonus)
    if cost >= math.floor(max_ki / 2) then
        ch:send_line("The object weight and gravity are a great challenge for you!")
        return bonus * 10
    elseif cost >= math.floor(max_ki / 4) then
        ch:send_line("The object weight and gravity are an awesome challenge for you!")
        return bonus * 8
    elseif cost >= math.floor(max_ki / 10) then
        ch:send_line("The object weight and gravity are a good challenge for you!")
        return bonus * 6
    elseif cost < math.floor(max_ki / 1000) then
        ch:send_line("The object weight and gravity are so easy to you, you could do it in your sleep....")
        return math.floor(bonus / 8)
    elseif cost < math.floor(max_ki / 100) then
        ch:send_line("The object weight and gravity are the opposite of a challenge for you....")
        return math.floor(bonus / 5)
    elseif cost < math.floor(max_ki / 50) then
        ch:send_line("The object weight and gravity are definitely not a challenge for you....")
        return math.floor(bonus / 4)
    elseif cost < math.floor(max_ki / 30) then
        ch:send_line("The object weight and gravity are barely a challenge for you....")
        return math.floor(bonus / 3)
    elseif cost < math.floor(max_ki / 20) then
        ch:send_line("The object weight and gravity are hardly a challenge for you....")
        return math.floor(bonus / 2)
    end

    ch:send_line("This gravity is just perfect for you....")
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

local function expand_mind(ch)
    local cost = ch:race_get() == "saiyan" and 7000 or 3500

    if ch:stat_get("practices") < cost then
        ch:send_line("You do not have enough practice sessions to expand your mind and ability to remember skills.")
        ch:send_line("%s needed.", text.add_commas(cost))
        return
    end

    local cap = ch:bonus_flagged(BON.GMEMORY) and 65 or 60
    if ch:stat_get("skill_slots") >= cap then
        ch:send_line("You can not have any more slots through this process.")
        return
    end

    ch:send_line("During your meditation you manage to expand your mind and get the feeling you could learn some new skills.")
    ch:stat_mod("skill_slots", 1)
    ch:stat_mod("practices", -cost)
end

local function break_mindlink(ch)
    local linked = ch:mindlinked_get()
    if not linked then
        ch:send_line("You are not mind linked with anyone.")
        return
    end
    if ch:linker_get() == 1 then
        ch:send_line("This is not how you break YOUR mind link.")
        return
    end

    local cost = math.floor(linked:meter_max("ki") * 0.05)
    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to manage a break.")
        return
    end

    local ch_roll = ch:stat_get("intelligence") + math.random(-5, 10)
    local linked_roll = linked:stat_get("intelligence") + math.floor(linked:skill_get("telepathy") * 0.1)
    if ch_roll >= linked_roll then
        ch:act("@rYou manage to break the mind link between you and @R$N@r!@n", false, nil, linked, "char")
        ch:act("$n closes their eyes for a few seconds.", false, nil, linked, "room")
        linked:send_line("@rYour mind linked target manages to push you out!@n")

        if linked:stat_get("intelligence") < dbat.axion_dice(-10) and not linked:condition_has("shocked") then
            linked:send_line("Your mind is shocked by the flood of mental energy that pushed it out!@n")
            linked:condition_apply("shocked", "combat", "telepathy_feedback")
        end

        linked:linker_set(0)
        ch:mindlinked_set(nil)
        linked:mindlinked_set(nil)
        return
    end

    ch:act("@rYou struggle to free your mind of @R$N's@r link, but fail!@n", false, nil, linked, "char")
    ch:act("$n closes their eyes for a few seconds, and appears to struggle quite a bit.", false, nil, linked, "room")
    linked:send_line("@rYour mind linked target struggles to free their mind, but fails!@n")
    ch:meter_mod_int("ki", -cost)
end

local function room_object(ch, arg)
    return Search(ch):add_room_objects(ch:room_get()):find_one(arg)
end

local function object_weight_with_sitter(obj)
    local weight = obj:weight_get()
    local sitter_id = obj:sitting_get()
    local sitter = sitter_id ~= 0 and dbat.characters.by_id(sitter_id) or nil
    if sitter then
        weight = weight + sitter:stat_get("weight")
    end
    return weight
end

local function meditate_object(ch, obj)
    if obj:vnum_get() == 79 then
        ch:send_line("It's frozen to the surface.")
        return
    end

    local room = ch:room_get()
    local gravity = room:gravity_get()
    local weight = object_weight_with_sitter(obj)
    local cost = compute_cost(ch, room, weight)
    if ch:meter_current("ki") < cost then
        ch:send_line("You don't have enough ki!")
        return
    end

    ch:act("@cYou close your eyes and concentrate, lifting $p@c with your ki.@n", true, obj, nil, "char")
    ch:act("@c$n closes $s eyes and lifts $p@c with $s ki.@n", true, obj, nil, "room")

    local gravity_bonus = GRAVITY_BONUS[gravity]
    local bonus = gravity_bonus and math.random(gravity_bonus[1], gravity_bonus[2]) or 0
    bonus = bonus + math.floor((weight + 1) / 500) + 1
    bonus = apply_challenge_message(ch, cost, ch:meter_max("ki"), bonus)

    if ch:is_soft_cap(1) then
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
    if bonus <= 1 and room:flagged(RF.WORKOUT) then
        if rv >= 19100 and rv <= 19199 then
            bonus = 12
        else
            bonus = 6
        end
    end

    if bonus ~= 1 and ch:race_get() == "demon" and math.random(1, 100) >= 80 then
        local pl_bonus = math.floor(bonus / 2)
        ch:send_line("Your spirit magnifies the strength of your body! @D[@G+%s@D]@n", text.add_commas(pl_bonus))
        ch:stat_mod("powerlevel", pl_bonus)
    end

    bonus = bonus + math.floor(ch:stat_get("level") / 20)
    if ch:race_get() == "namekian" then
        bonus = math.floor(bonus + bonus / 2)
    end
    if ch:bonus_flagged(BON.LONER) then
        bonus = math.floor(bonus + bonus * 0.1)
    end

    ch:send_line("You feel your spirit grow stronger @D[@G+%s@D]@n.", text.add_commas(bonus))
    ch:stat_mod("ki", bonus)
    wait_for_gravity(ch, gravity)
    ch:meter_mod_int("ki", -cost)
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:is_npc() then
        ch:send_line("You are a mob fool!")
        return
    end
    if command_position_blocked(ch) then return end

    local room = ch:room_get()
    if room:flagged(RF.HELL) then
        ch:send_line("The fire makes it too hot!")
        return
    end
    if ch:condition_has("fishing") then
        ch:send_line("Stop fishing first.")
        return
    end
    if RACE_NO_EXERCISE[ch:race_get() or ""] then
        ch:send_line("You will gain nothing from exercising!")
        return
    end
    if ch:carrying_char_get() then
        ch:send_line("You are carrying someone!")
        return
    end
    if ch:dragging_get() then
        ch:send_line("You are dragging someone!")
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
    if ch:position_get() ~= POS.SITTING then
        ch:send_line("You need to be sitting to meditate.")
        return
    end
    if arg == "" then
        ch:send_line(SYNTAX)
        return
    end

    if arg:lower() == "expand" then
        expand_mind(ch)
        return
    elseif arg:lower() == "break" then
        break_mindlink(ch)
        return
    end

    local obj = room_object(ch, arg)
    if not obj then
        ch:send_line(SYNTAX)
        return
    end

    meditate_object(ch, obj)
end

return {
    id      = "meditate",
    aliases = { {"meditate", 6} },
    execute = execute,
}
