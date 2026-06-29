local dbat = require("dbat")

local Search = dbat.lib.search.new
local AFF = dbat.consts.aff_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local PULSE_4SEC = 40

local GRAPPLE_TYPES = {
    hold = 1,
    choke = 2,
    grab = 3,
    wrap = 4,
}

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
    return Search(ch):add_room_people(ch:room_get()):add_filter(function(s, e)
        return s:can_see_char(e)
    end):find_one(name)
end

local function stop_current_grapple(ch, vict)
    ch:act("@RYou stop grappling with @r$N@R!@n", true, nil, vict, "char")
    ch:act("@r$n@R stops grappling with @rYOU!!@n", true, nil, vict, "vict")
    ch:act("@r$n@R stops grappling with @r$N@R!@n", true, nil, vict, "notvict")
    ch:grappling_set(nil, 0)
    vict:grappled_set(nil, 0)
end

local function spend_fail(ch, cost)
    ch:meter_mod_int("stamina", -cost)
    ch:improve_skill("grapple", 1)
    ch:wait_set(PULSE_4SEC)
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
        ch:act("@C$N@c disappears, avoiding your grapple attempt before reappearing!@n",
            false, nil, vict, "char")
        ch:act("@cYou disappear, avoiding @C$n's@c grapple attempt before reappearing!@n",
            false, nil, vict, "vict")
        ch:act("@C$N@c disappears, avoiding @C$n's@c grapple attempt before reappearing!@n",
            false, nil, vict, "notvict")
        if ch_zan then ch:condition_remove("zanzoken", "zanzoken_over") end
        vict:condition_remove("zanzoken", "zanzoken_over")
        ch:meter_mod_int("stamina", -cost)
        ch:wait_set(PULSE_4SEC)
        return true
    end

    ch:reveal_hiding(0)
    ch:act("@C$N@c disappears, trying to avoid your grapple but your zanzoken is faster!@n",
        false, nil, vict, "char")
    ch:act("@cYou zanzoken to avoid the grapple attempt but @C$n's@c zanzoken is faster!@n",
        false, nil, vict, "vict")
    ch:act("@C$N@c disappears, trying to avoid @C$n's@c grapple attempt but @C$n's@c zanzoken is faster!@n",
        false, nil, vict, "notvict")
    vict:condition_remove("zanzoken", "zanzoken_over")
    ch:condition_remove("zanzoken", "zanzoken_over")
    return false
end

local function overpower_failure(ch, vict, cost)
    ch:reveal_hiding(0)
    ch:act("@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n",
        true, nil, vict, "char")
    ch:act("@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n",
        true, nil, vict, "vict")
    ch:act("@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n",
        true, nil, vict, "notvict")
    spend_fail(ch, cost)
end

local function avoid_failure(ch, vict, cost)
    ch:reveal_hiding(0)
    ch:act("@RYou try to grapple with @r$N@R, but $E manages to avoid it!@n",
        true, nil, vict, "char")
    ch:act("@r$n@R tries to grapple with YOU, but you manage to avoid it!@n",
        true, nil, vict, "vict")
    ch:act("@r$n@R tries to grapple with @r$N@R, but $E manages to avoid it!@n",
        true, nil, vict, "notvict")
    spend_fail(ch, cost)
end

local function apply_success(ch, vict, arg, cost)
    local grap_type = GRAPPLE_TYPES[arg]

    if arg == "hold" then
        ch:reveal_hiding(0)
        ch:act("@RYou rush at @r$N@R and manage to get $M in a hold from behind!@n",
            true, nil, vict, "char")
        ch:act("@r$n@R rushes at YOU and manages to get you in a hold from behind!@n",
            true, nil, vict, "vict")
        ch:act("@r$n@R rushes at @r$N@R and manages to get $M in a hold from behind!@n",
            true, nil, vict, "notvict")
    elseif arg == "choke" then
        ch:reveal_hiding(0)
        ch:act("@RYou rush at @r$N@R and manage to grab $S throat with both hands!@n",
            true, nil, vict, "char")
        ch:act("@r$n@R rushes at YOU and manages to grab your throat with both hands!@n",
            true, nil, vict, "vict")
        ch:act("@r$n@R rushes at @r$N@R and manages to grab $S throat with both hands!@n",
            true, nil, vict, "notvict")
    elseif arg == "wrap" then
        if ch:race_get() ~= "majin" then
            ch:send_line("Your body is not flexible enough to wrap around a target!")
            return
        end
        ch:act("@MMoving quickly you stretch your body out and wrap it around the length of @c$N's@M body! You tighten your body until you begin crushing @c$N@M!",
            true, nil, vict, "char")
        ch:act("@C$n@M quickly stretches out $s body and wraps it around @RYOU@M! You feel $s body begin to crush your own!@n",
            true, nil, vict, "vict")
        ch:act("@C$n@M quickly stretches out $s body and wraps it around @c$N@M! It appears that @c$N's@M body is being crushed slowly!@n",
            true, nil, vict, "notvict")
    elseif arg == "grab" then
        ch:reveal_hiding(0)
        ch:act("@RYou rush at @r$N@R and manage to lock your arm onto $S!@n",
            true, nil, vict, "char")
        ch:act("@r$n@R rushes at YOU and manages to lock $s arm onto your's!@n",
            true, nil, vict, "vict")
        ch:act("@r$n@R rushes at @r$N@R and manages to lock $s arm onto @r$N's@R!@n",
            true, nil, vict, "notvict")
        if not vict:player_flagged(PLR.THANDW) then
            vict:player_flag_set(PLR.THANDW, false)
        end
    end

    ch:grappling_set(vict, grap_type)
    vict:grappled_set(ch, grap_type)
    ch:meter_mod_int("stamina", -cost)
    ch:improve_skill("grapple", 1)
    ch:wait_set(PULSE_4SEC)
end

local function execute(ctx)
    local ch = ctx.ch
    local target_name = ctx.argparams.tokens[1] or ""
    local mode = (ctx.argparams.tokens[2] or ""):lower()

    if blocked_by_position(ch) then return end
    if not ch:know_skill("grapple") then return end

    if ch:condition_has("mystic_melody") then
        ch:send_line("You are currently playing a song! Enter the song command in order to stop!")
        return
    end
    if ch:player_flagged(PLR.THANDW) then
        ch:send_line("Your are too busy wielding your weapon with two hands!")
        return
    end
    if ch:absorbing_get() then ch:send_line("You are currently absorbing from someone!"); return end
    if ch:absorbed_by_get() then ch:send_line("You are currently being absorbed by someone! Try 'escape'!"); return end

    local current = ch:grappling_get()
    if current then stop_current_grapple(ch, current); return end

    if ch:grappled_get() then
        ch:send_line("You are currently a victim of grappling! Try 'escape' to break free!")
        return
    end
    if not ch:has_arms() then ch:send_line("You have no available arms!"); return end

    if target_name == "" or mode == "" then
        ch:send_line("Syntax: grapple (target) (hold | choke | grab)")
        return
    end

    local vict = room_target(ch, target_name)
    if not vict then
        vict = ch:fighting_get()
        if not vict then ch:send_line("That target isn't here."); return end
    end

    if not ch:can_kill(vict, 0) then return end
    if vict:aff_flagged(AFF.KNOCKED) then ch:send_line("They are unconcious. What would be the point?"); return end
    if vict:grappled_get() then ch:send_line("They are currently in someone else's grasp!"); return end
    if vict:absorbed_by_get() then ch:send_line("They are currently in someone else's grasp!"); return end
    if vict:absorbing_get() then ch:send_line("They are currently absorbing from someone!"); return end

    if not GRAPPLE_TYPES[mode] then
        ch:send_line("Syntax: grapple (target) (hold | choke | grab | wrap)")
        return
    end

    local cost = math.floor(ch:meter_max("stamina") / 100)
    if ch:meter_current("stamina") < cost then
        ch:send_line("You do not have enough stamina to grapple!")
        return
    end

    if zanzoken_evades(ch, vict, cost) then return end

    local perc = speed_adjust(ch:skill_get("grapple"), ch, vict)
    local prob = dbat.axion_dice(0)
    local ch_power = ch:meter_current("powerlevel")
    local vict_power = vict:meter_current("powerlevel")
    local ch_str = ch:der_total("strength")
    local vict_str = vict:der_total("strength")

    if (ch_power * 0.02) * ch_str < (vict_power * 0.01) * vict_str then
        overpower_failure(ch, vict, cost)
        return
    elseif (ch_power * 0.01) * ch_str < (vict_power * 0.01) * vict_str and math.random(1, 4) == 1 then
        overpower_failure(ch, vict, cost)
        return
    elseif perc < prob then
        avoid_failure(ch, vict, cost)
        return
    elseif not vict:has_arms() and mode == "grab" then
        ch:send_line("They don't even have an arm to grab onto!")
        return
    end

    apply_success(ch, vict, mode, cost)
end

return {
    id = "grapple",
    aliases = { {"grapple", 5} },
    execute = execute,
}
