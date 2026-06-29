local dbat = require("dbat")

local Search = dbat.lib.search.new
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local PULSE_4SEC = 40

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.FIGHTING then return false end

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

local function room_target(ch, name)
    if name == "" then return nil end
    return Search(ch):add_room_people(ch:room_get()):add_filter(function(s, e)
        return s:can_see_char(e)
    end):find_one(name)
end

local function speed_adjust(perc, ch, vict)
    local ch_speed = ch:der_total("speed_index")
    local vict_speed = vict:der_total("speed_index")

    if ch_speed > vict_speed * 2 then
        return perc + 5
    elseif ch_speed > vict_speed then
        return perc + 2
    elseif ch_speed * 2 < vict_speed then
        return perc - 5
    elseif ch_speed < vict_speed then
        return perc - 2
    end
    return perc
end

local function force_fighting_pair(ch, vict)
    local ch_fighting = ch:fighting_get()
    if not ch_fighting or not ch_fighting:is_same(vict) then ch:start_fighting(vict) end

    local vict_fighting = vict:fighting_get()
    if not vict_fighting or not vict_fighting:is_same(ch) then vict:start_fighting(ch) end
end

local function start_fighting_if_idle(ch, vict)
    if not ch:fighting_get() then ch:start_fighting(vict) end
    if not vict:fighting_get() then vict:start_fighting(ch) end
end

local function zanzoken_evades(ch, vict, cost)
    local vict_can_zanzoken = ((not vict:is_npc() and vict:race_get() == "icer" and math.random(1, 30) >= 28)
        or vict:condition_has("zanzoken"))
        and vict:meter_current("stamina") >= 1
        and vict:position_get() ~= POS.SLEEPING
    if not vict_can_zanzoken then return false end

    local ch_zan = ch:condition_has("zanzoken")
    local ch_speed = ch:der_total("speed_index")
    local vict_speed = vict:der_total("speed_index")

    if not ch_zan or (ch_speed + math.random(1, 5) < vict_speed + math.random(1, 5)) then
        ch:reveal_hiding(0)
        ch:act("@C$N@c disappears, avoiding your trip before reappearing!@n",
            false, nil, vict, "char")
        ch:act("@cYou disappear, avoiding @C$n's@c trip before reappearing!@n",
            false, nil, vict, "vict")
        ch:act("@C$N@c disappears, avoiding @C$n's@c trip before reappearing!@n",
            false, nil, vict, "notvict")
        if ch_zan then ch:condition_remove("zanzoken", "zanzoken_over") end
        vict:condition_remove("zanzoken", "zanzoken_over")
        ch:meter_mod_int("stamina", -cost)
        ch:wait_set(PULSE_4SEC)
        return true
    end

    ch:reveal_hiding(0)
    ch:act("@C$N@c disappears, trying to avoid your trip but your zanzoken is faster!@n",
        false, nil, vict, "char")
    ch:act("@cYou zanzoken to avoid the trip but @C$n's@c zanzoken is faster!@n",
        false, nil, vict, "vict")
    ch:act("@C$N@c disappears, trying to avoid @C$n's@c trip but @C$n's@c zanzoken is faster!@n",
        false, nil, vict, "notvict")
    vict:condition_remove("zanzoken", "zanzoken_over")
    ch:condition_remove("zanzoken", "zanzoken_over")
    return false
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end
    if not ch:is_npc() and not ch:know_skill("trip") then return end

    local cost = math.floor(ch:meter_max("powerlevel") / 200)
    if cost > ch:meter_current("stamina") then
        ch:send_line("You don't have enough stamina.")
        return
    end

    local perc = ch:init_skill("trip")
    local prob = math.random(1, 114)
    if perc == 0 then perc = ch:stat_get("level") + math.random(1, 10) end

    local vict = room_target(ch, arg)
    if not vict then
        local fighting = ch:fighting_get()
        if fighting and fighting:room_get() and fighting:room_get():is_same(ch:room_get()) then
            vict = fighting
        else
            ch:send_line("That target isn't here.")
            return
        end
    end

    if not ch:can_kill(vict, 0) then return end

    if vict:condition_has("flying") then ch:send_line("They are flying and are not on their feet!"); return end
    if vict:position_get() == POS.SITTING then ch:send_line("They are not on their feet!"); return end
    if vict:player_flagged(PLR.HEALT) then ch:send_line("They are inside a healing tank!"); return end

    perc = speed_adjust(perc, ch, vict)
    if zanzoken_evades(ch, vict, cost) then return end

    if perc < prob then
        ch:reveal_hiding(0)
        ch:act("@mYou move to trip $N@m, but you screw up and $E keeps $S footing!@n",
            true, nil, vict, "char")
        ch:act("@m$n@m moves to trip YOU, but $e screws up and you manage to keep your footing!@n",
            true, nil, vict, "vict")
        ch:act("@m$n@m moves to trip $N@m, but $e screws up and $N@m manages to keep $S footing!@n",
            true, nil, vict, "notvict")
        ch:improve_skill("trip", 0)
        ch:meter_mod_int("stamina", -cost)
        ch:wait_set(PULSE_4SEC)
        force_fighting_pair(ch, vict)
        return
    end

    ch:reveal_hiding(0)
    ch:act("@mYou move to trip $N@m, and manage to knock $M off $S feet!@n",
        true, nil, vict, "char")
    ch:act("@m$n@m moves to trip YOU, and manages to knock you off your feet!@n",
        true, nil, vict, "vict")
    ch:act("@m$n@m moves to trip $N@m, and manages to knock $N@m off $S feet!@n",
        true, nil, vict, "notvict")
    ch:improve_skill("trip", 0)
    ch:meter_mod_int("stamina", -cost)
    vict:position_set(POS.SITTING)
    ch:wait_set(PULSE_4SEC)
    start_fighting_if_idle(ch, vict)
end

return {
    id = "trip",
    aliases = { {"trip", 4} },
    execute = execute,
}
