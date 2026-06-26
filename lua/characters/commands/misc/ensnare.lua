local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act
local EF     = dbat.consts.item_extra_flags

local PULSE_3SEC = 30  -- 3 * PASSES_PER_SEC (10)

-- vnums from valid_silk() in act.misc.cpp:1443-1455
local SILK_VNUMS = {
    [16700] = true, [16701] = true, [16702] = true,
    [16703] = true, [16704] = true, [16708] = true,
}

local function find_silk(ch)
    for obj in ch:inventory() do
        if SILK_VNUMS[obj:vnum_get()] and not obj:extra_flagged(EF.FORGED) then
            return obj
        end
    end
    return nil
end

local function has_arms(vict)
    return vict:limbcond_get(1) > 0 or vict:limbcond_get(2) > 0
end

local MSG_MISS = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately you miss and lose the bundle...@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately $e misses and loses the bundle...@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately $e misses and loses the bundle...@n",
}

local MSG_VICT_ZAN_EVADE = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately @c$N@W zanzokens away avoiding it and you lose the bundle...@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately you zanzoken away avoiding it and @C$n@W loses the bundle...@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately @c$N@W zanzokens away avoiding it and @C$n@W loses the bundle...@n",
}

local MSG_BOTH_ZAN_MISS = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! You both zanzoken! Unfortunately @c$N@W manages to avoid it and you lose the bundle...@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! You both zanzoken! Fortunately you manage to avoid it and @C$n@W loses the bundle...@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! They both zanzoken! Fortunately @c$N@W manages to avoid it and @C$n@W loses the bundle...@n",
}

local MSG_BOTH_ZAN_HIT = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! You both zanzoken! Quickly you spin around $M and ensnare $S arms with the silk!@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! You both zanzoken! Quickly $e spins around you and ensnares your arms with the silk!@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! They both zanzoken! Quickly $e spins around @c$N@W and ensnares $S arms with the silk!@n",
}

local MSG_CH_ZAN_HIT = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! Quickly you zanzoken and spin around $M and ensnare $S arms with the silk!@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! Quickly $e zanzokens and spins around you and ensnares your arms with the silk!@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! Quickly $e zanzokens and spins around @c$N@W and ensnares $S arms with the silk!@n",
}

local MSG_SPEED_MISS = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately @c$N@W manages to avoid it and you lose the bundle...@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately you manage to avoid it and @C$n@W loses the bundle...@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately @c$N@W manages to avoid it and @C$n@W loses the bundle...@n",
}

local MSG_HIT = {
    actor  = "@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! Quickly you spin around $M and ensnare $S arms with the silk!@n",
    target = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! Quickly $e spins around you and ensnares your arms with the silk!@n",
    room   = "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! Quickly $e spins around @c$N@W and ensnares $S arms with the silk!@n",
}

local function execute(ctx)
    local ch = ctx.ch

    local silk = find_silk(ch)
    if not silk then
        ch:send_line("You do not have a bundle of silk to ensnare an opponent with!")
        return
    end

    local arg = ctx.argparams.tokens[1] or ""
    if arg == "" then
        ch:send_line("Syntax: ensnare (target)")
        return
    end

    local room = ch:room_get()
    local vict = Search(ch):add_room_people(room):find_one(arg)
    if not vict then
        ch:send_line("Who are you trying to target with ensnare?")
        return
    end
    if vict:condition_has("ensnared") then
        ch:send_line("They are already ensnared!")
        return
    end
    if not has_arms(vict) then
        ch:send_line("They don't have arms to ensnare!")
        return
    end

    local prob = ch:skill_get("ensnare")
    local perc = dbat.axion_dice(0)
    local ctx2 = { actor = ch, target = vict }

    local ch_zan   = ch:condition_has("zanzoken")
    local vict_zan = vict:condition_has("zanzoken")

    if prob <= perc then
        act.message(MSG_MISS, ctx2)
    elseif vict_zan and not ch_zan then
        act.message(MSG_VICT_ZAN_EVADE, ctx2)
        vict:condition_remove("zanzoken", "zanzoken_over")
    elseif vict_zan and ch_zan then
        local ch_roll   = ch:der_total("speed_index") + math.random(1, 100)
        local vict_roll = vict:der_total("speed_index") + math.random(1, 100)
        if ch_roll < vict_roll then
            act.message(MSG_BOTH_ZAN_MISS, ctx2)
        else
            act.message(MSG_BOTH_ZAN_HIT, ctx2)
            vict:condition_apply("ensnared", "skill", "ensnare")
        end
        vict:condition_remove("zanzoken", "zanzoken_over")
        ch:condition_remove("zanzoken", "zanzoken_over")
    elseif ch_zan and not vict_zan then
        act.message(MSG_CH_ZAN_HIT, ctx2)
        vict:condition_apply("ensnared", "skill", "ensnare")
        ch:condition_remove("zanzoken", "zanzoken_over")
    elseif ch:der_total("speed_index") + math.random(1, 100) <
           vict:der_total("speed_index") + math.random(1, 100) then
        act.message(MSG_SPEED_MISS, ctx2)
    else
        act.message(MSG_HIT, ctx2)
        vict:condition_apply("ensnared", "skill", "ensnare")
    end

    silk:extract()
    ch:wait_set(PULSE_3SEC)
    ch:improve_skill("ensnare", 0)
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    if not ch:know_skill("ensnare") then return false end
    return true
end

return {
    id          = "ensnare",
    aliases     = { {"ensnare", 5} },
    execute     = execute,
    can_execute = can_execute,
}
