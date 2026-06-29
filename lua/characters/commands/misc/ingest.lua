local dbat = require("dbat")

local Search = dbat.lib.search.new
local AFF = dbat.consts.aff_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions
local text = dbat.lib.text

local PULSE_3SEC = 30

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.STANDING then return false end

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

local function target_name(ch)
    return ch and ch:name_get() or "Someone"
end

local function level_absorb_cap_reached(level, absorbs)
    return (level < 100 and level >= 75 and absorbs == 3)
        or (level < 75 and level >= 50 and absorbs == 2)
        or (level < 50 and level >= 25 and absorbs == 1)
end

local function zanzoken_evades(ch, vict)
    if not vict:condition_has("zanzoken")
        or vict:meter_current("stamina") < 1
        or vict:position_get() == POS.SLEEPING then
        return false
    end

    ch:act("@C$N@c disappears, avoiding your attempted ingestion!@n",
        false, nil, vict, "char")
    ch:act("@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n",
        false, nil, vict, "vict")
    ch:act("@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n",
        false, nil, vict, "notvict")
    vict:condition_remove("zanzoken", "zanzoken_over")
    ch:wait_set(PULSE_3SEC)
    return true
end

local function ingestion_misses(ch, vict)
    if ch:der_total("speed_index") + math.random(1, 5) >= ch:der_total("speed_index") + math.random(1, 5) then
        return false
    end

    ch:act("@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E manages to avoid your blob of goo though!@n",
        true, nil, vict, "char")
    ch:act("@C$n@W flings a piece of goo at you, you manage to avoid it though!@n",
        true, nil, vict, "vict")
    ch:act("@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n",
        true, nil, vict, "notvict")
    ch:wait_set(PULSE_3SEC)
    return true
end

local function adjust_toward(ch, vict, stat)
    local current = ch:stat_get(stat)
    local target = vict:stat_get(stat)
    if current > target then
        ch:stat_mod(stat, -math.floor((current - target) / 2))
    elseif current < target then
        ch:stat_mod(stat, math.floor((target - current) / 2))
    else
        ch:stat_set(stat, target)
    end
end

local function mutate_appearance(ch, vict)
    if math.random(1, 3) == 3 then
        ch:send_line("You get %s's eye color.", target_name(vict))
        ch:eye_set(vict:eye_get())
    elseif math.random(1, 3) == 3 then
        ch:send_line("%s changes your height.", target_name(vict))
        adjust_toward(ch, vict, "height")
    elseif math.random(1, 3) == 3 then
        ch:send_line("%s changes your weight.", target_name(vict))
        adjust_toward(ch, vict, "weight")
    else
        ch:send_line("Your forelock length changes because of %s.", target_name(vict))
        ch:hairl_set(vict:hairl_get())
    end
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end

    if ch:race_get() ~= "majin" then
        ch:send_line("You are not a majin, you can not ingest.")
        return
    end

    if arg == "" then
        ch:send_line("Who do you want to ingest?")
        return
    end

    local vict = room_target(ch, arg)
    if not vict then
        ch:send_line("Ingest who?")
        return
    end

    if not ch:can_kill(vict, 0) then return end

    local absorber = vict:absorbed_by_get()
    if absorber then ch:send_text("%s is already absorbing from them!", target_name(absorber)); return end

    local absorbs = ch:absorbs_get()
    if absorbs > 3 then ch:send_line("You already have already ingested 4 people."); return end

    local level = ch:stat_get("level")
    if level < 25 then
        ch:send_line("You can't ingest yet.")
        return
    end
    if level_absorb_cap_reached(level, absorbs) then
        ch:send_line("You already have ingested as much as you can. You'll have to get more experienced.")
        return
    end

    if vict:meter_max("powerlevel") >= ch:stat_get("powerlevel") * 3 then
        ch:send_line("You are too weak to ingest them into your body!")
        return
    end
    if vict:aff_flagged(AFF.SANCTUARY) then
        ch:send_line("You can't ingest them, they have a barrier!")
        return
    end

    ch:reveal_hiding(0)
    if zanzoken_evades(ch, vict) then return end
    if ingestion_misses(ch, vict) then return end

    ch:act("@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then returns to your body!@n",
        true, nil, vict, "char")
    ch:act("@C$n@W flings a piece of goo at you! The goo engulfs your body and then returns to @C$n@W!@n",
        true, nil, vict, "vict")
    ch:act("@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then return to @C$n@W!@n",
        true, nil, vict, "notvict")

    ch:absorbs_mod(1)
    local pl = math.floor(vict:stat_get("powerlevel") / 6)
    local stam = math.floor(vict:stat_get("stamina") / 6)
    local ki = math.floor(vict:stat_get("ki") / 6)
    ch:stat_mod("powerlevel", pl)
    ch:stat_mod("stamina", stam)
    ch:stat_mod("ki", ki)

    if not vict:is_npc() and not ch:is_npc() then
        dbat.send_to_imm(string.format("[PK] %s killed %s at room [%d]\r\n",
            target_name(ch), target_name(vict), vict:room_vnum_get()))
        vict:player_flag_set(PLR.ABSORBED, true)
    end

    ch:send_line("@D[@mINGEST@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n",
        text.add_commas(pl), text.add_commas(ki), text.add_commas(stam))
    mutate_appearance(ch, vict)
    ch:handle_ingest_learn(vict)
    vict:die(nil)
end

return {
    id = "ingest",
    aliases = { {"ingest", 5} },
    execute = execute,
}
