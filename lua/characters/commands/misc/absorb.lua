local dbat = require("dbat")

local text = dbat.lib.text

local AFF = dbat.consts.aff_flags
local MOB = dbat.consts.mob_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local PULSE_3SEC = 30
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

local function is_android(ch)
    return ch:race_get() == "android"
end

local function is_bio(ch)
    return ch:race_get() == "bio"
end

local function base_pl(ch)
    return ch:stat_get("powerlevel")
end

local function base_ki(ch)
    return ch:stat_get("ki")
end

local function base_st(ch)
    return ch:stat_get("stamina")
end

local function target_name(ch)
    return ch and ch:name_get() or "Someone"
end

local function start_fighting_pair(ch, vict)
    if not ch:fighting_get() then ch:start_fighting(vict) end
    if not vict:fighting_get() then vict:start_fighting(ch) end
end

local function stop_absorbing(ch)
    local vict = ch:absorbing_get()
    if not vict then return false end

    ch:act("@WYou stop absorbing from @c$N@W!@n", true, nil, vict, "char")
    ch:act("$n stops absorbing from you!", true, nil, vict, "vict")
    ch:act("$n stops absorbing from $N!", true, nil, vict, "notvict")
    if vict:is_npc() and not vict:fighting_get() then vict:start_fighting(ch) end
    ch:absorbing_set(nil)
    vict:absorbed_by_set(nil)
    return true
end

local function avoid_messages(ch, vict)
    ch:act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, nil, vict, "char")
    ch:act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, nil, vict, "vict")
    ch:act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, nil, vict, "notvict")
end

local function absorb_gain_line(ch, pl, ki, st)
    ch:send_line("@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n",
        text.add_commas(pl), text.add_commas(ki), text.add_commas(st))
end

local function android_absorb(ch, vict)
    if not ch:can_kill(vict, 0) then return end
    local absorber = vict:absorbed_by_get()
    if absorber then ch:send("%s is already absorbing from them!", target_name(absorber)); return end
    if vict:meter_max("powerlevel") > ch:meter_max("powerlevel") * 2 then ch:send_line("They are too strong for you to absorb from."); return end
    if vict:meter_max("powerlevel") * 20 < ch:meter_max("powerlevel") then ch:send_line("They are too weak for you to bother absorbing from."); return end
    if vict:meter_current("stamina") < math.floor(vict:meter_max("stamina") / 20)
        and vict:meter_current("ki") < math.floor(vict:meter_max("ki") / 20) then
        ch:send_line("They have nothing to absorb right now, they are drained...")
        return
    end

    ch:reveal_hiding(0)
    if ch:init_skill("absorb") < dbat.axion_dice(0) then
        avoid_messages(ch, vict)
        ch:improve_skill("absorb", 1)
        if vict:is_npc() and vict:is_humanoid() and math.random(1, 3) == 3 then
            start_fighting_pair(ch, vict)
        end
        ch:wait_set(PULSE_3SEC)
        return
    end

    ch:act("@WYou rush at @c$N@W and try to absorb from them, and manage to grab on!@n", true, nil, vict, "char")
    ch:act("@C$n@W rushes at you and tries to grab you, and manages to grab on!@n", true, nil, vict, "vict")
    ch:act("@C$n@w rushes at @c$N@W and tries to grab $M, and manages to grab on!@n", true, nil, vict, "notvict")
    ch:improve_skill("absorb", 1)
    ch:absorbing_set(vict)
    vict:absorbed_by_set(ch)
    ch:wait_set(PULSE_3SEC)
end

local function bio_swallow(ch, vict)
    local absorber = vict:absorbed_by_get()
    if absorber then ch:send("%s is already absorbing from them!", target_name(absorber)); return end
    if not ch:can_kill(vict, 0) then return end
    if ch:absorbs_get() < 1 then ch:send_line("You already have already absorbed 3 people."); return end
    if vict:meter_max("powerlevel") >= base_pl(ch) * 3 then
        ch:send_line("You are too weak to absorb them into your cellular structure!")
        return
    end

    ch:reveal_hiding(0)
    if ch:skill_get("absorb") < dbat.axion_dice(0) then
        avoid_messages(ch, vict)
        ch:improve_skill("absorb", 1)
        start_fighting_pair(ch, vict)
        ch:wait_set(PULSE_3SEC)
        return
    end

    ch:act("@WYou rush at @c$N@W and your tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing $m!@n", true, nil, vict, "char")
    ch:act("@C$n@W rushes at you and $s tail engulfs you! $e quickly sucks your squirming body into $s tail, absorbing you!@n", true, nil, vict, "vict")
    ch:act("@C$n@w rushes at @c$N@W and $s tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing @c$N@W!@n", true, nil, vict, "notvict")

    ch:absorbs_mod(-1)
    local st = math.floor(base_st(vict) / 5)
    local ki = math.floor(base_ki(vict) / 5)
    local pl = math.floor(base_pl(vict) / 5)
    ch:stat_mod("powerlevel", pl)
    ch:stat_mod("stamina", st)
    ch:stat_mod("ki", ki)
    if not vict:is_npc() and not ch:is_npc() then
        vict:player_flag_set(PLR.ABSORBED, true)
    end
    absorb_gain_line(ch, pl, ki, st)
    ch:improve_skill("absorb", 1)
    vict:die(nil)
end

local function bio_extract(ch, vict)
    local failthresh = math.random(1, 125)
    if vict:stat_get("level") > 99 then failthresh = failthresh + (vict:stat_get("level") - 95) * 2 end

    local absorber = vict:absorbed_by_get()
    if absorber then ch:send("%s is already absorbing from them!", target_name(absorber)); return end
    if not ch:can_kill(vict, 0) then return end
    if vict:meter_max("powerlevel") >= ch:meter_max("powerlevel") then
        ch:send_line("You are too weak to absorb them into your cellular structure!")
        return
    end
    if vict:meter_max("powerlevel") < math.floor(ch:meter_max("powerlevel") / 5) then
        ch:send_line("They would be worthless to you at your strength!")
        return
    end
    if not vict:is_npc() then
        ch:send_line("You can't absorb their bio extract, you need to swallow them with your tail!")
        return
    end
    if ch:is_soft_cap(0) then
        ch:send_line("You can not handle any more bio extract at your current level.")
        return
    end

    if ch:skill_get("absorb") < failthresh then
        avoid_messages(ch, vict)
        ch:improve_skill("absorb", 0)
        start_fighting_pair(ch, vict)
        ch:wait_set(PULSE_4SEC)
        return
    end

    ch:act("@WYou rush at @c$N@W and stab them with your tail! You quickly suck out all the bio extract you need and leave the empty husk behind!", true, nil, vict, "char")
    ch:act("@C$n@w rushes at @c$N@W and stabs $M with $s tail! $e quickly sucks out all the bio extract and leaves the empty husk of @c$N@W behind!@n", true, nil, vict, "notvict")

    local level = ch:stat_get("level")
    local st = math.floor(base_st(vict) / 2000) + math.random(level, level * 2)
    local ki = math.floor(base_ki(vict) / 2000) + math.random(level, level * 2)
    local pl = math.floor(base_pl(vict) / 2000) + math.random(level, level * 2)
    st = math.min(st, 1500000)
    ki = math.min(ki, 1500000)
    pl = math.min(pl, 1500000)

    ch:stat_mod("powerlevel", pl)
    ch:stat_mod("stamina", st)
    ch:stat_mod("ki", ki)
    ch:meter_mod_int("lifeforce", math.floor(ch:meter_max("lifeforce") * 0.05))
    absorb_gain_line(ch, pl, ki, st)
    ch:improve_skill("absorb", 0)
    ch:wait_set(PULSE_4SEC)
    vict:mob_flag_set(MOB.HUSK, true)
    vict:die(ch)
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""
    local arg2 = ctx.argparams.tokens[2] or ""

    if blocked_by_position(ch) then return end
    if not ch:is_npc() and not ch:know_skill("absorb") then return end

    local android = is_android(ch)
    local bio = is_bio(ch)

    if android and not ch:limb_ok(0) then return end
    if not ch:is_npc() and bio and not ch:player_flagged(PLR.TAIL) then
        ch:send_line("You have no tail!")
        return
    end
    if not android and not bio then
        ch:send_line("You shouldn't have this skill, you are incapable of absorbing.")
        return
    end
    if ch:fighting_get() and not android then ch:send_line("You are too busy fighting!"); return end
    if ch:grappled_get() then ch:send_line("You are currently being grappled with! Try 'escape'!"); return end
    if ch:grappling_get() then ch:send_line("You are currently grappling with someone!"); return end

    stop_absorbing(ch)

    local vict
    if android then
        if arg == "" then ch:send_line("Who do you want to absorb?"); return end
        vict = ch:acquire_room_target(arg)
        if not vict then ch:send_line("Absorb from who?"); return end
    elseif bio then
        if arg == "" then ch:send_line("Syntax: absorb (swallow | extract) (target)"); return end
        vict = ch:acquire_room_target(arg2)
        if not vict then ch:send_line("Syntax: absorb (swallow | extract) (target)"); return end
    end

    if vict:aff_flagged(AFF.SANCTUARY) then
        ch:send_line("You can't absorb them, they have a barrier!")
        return
    end

    if android then
        if not ch:is_npc() and not ch:player_flagged(PLR.ABSORB) then
            ch:send_line("You are not an absorbtion model.")
            return
        end
        android_absorb(ch, vict)
    elseif arg == "swallow" then
        bio_swallow(ch, vict)
    elseif arg == "extract" then
        bio_extract(ch, vict)
    else
        ch:send_line("Syntax: absorb (extract | swallow) (target)")
    end
end

return {
    id = "absorb",
    aliases = { {"absorb", 5} },
    execute = execute,
}
