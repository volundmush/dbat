local dbat = require("dbat")
local AFF  = dbat.consts.aff_flags
local BON  = dbat.consts.bonuses
local SMH  = dbat.consts.secs_per_mud_hour

local function limb_ok_arm(ch)
    if ch:condition_has("mystic_melody") then
        ch:send_line("You are currently playing a song! Enter the song command in order to stop!")
        return false
    end
    if not ch:is_npc() then
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then
            ch:send_line("You have no available arms!")
            return false
        end
    end
    return true
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if not ch:know_skill("paralyze") then return end

    if arg == "" then
        ch:send_line("Who are you wanting to paralyze?")
        return
    end

    if not limb_ok_arm(ch) then return end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("That target isn't here.")
        return
    end
    if not ch:can_kill(vict) then return end

    if vict:aff_flagged(AFF.PARA) then
        ch:send_line("They are already partially paralyzed!")
        return
    end

    local ki_threshold = math.floor(vict:meter_current("powerlevel") / 10)
                       + math.floor(ch:meter_max("ki") / 20)
    if ch:meter_current("ki") < ki_threshold then
        ch:send_line("You realize you can't paralyze them. You don't have enough ki to restrain them!")
        return
    end

    local prob = ch:skill_get("paralyze")
    local perc = dbat.axion_dice(0)

    local ch_speed   = ch:der_total("speed_index")
    local vict_speed = vict:der_total("speed_index")
    if ch_speed * 2 < vict_speed then
        prob = prob - 10
    end
    if ch_speed + math.floor(ch_speed / 2) < vict_speed then
        prob = prob - 5
    end

    local ki_cost = math.floor(vict:meter_current("powerlevel") / 6)
                  + math.floor(ch:meter_max("ki") / 20)

    if vict:bonus_flagged(BON.INSOMNIAC) then
        ch:meter_mod_int("ki", -ki_cost)
        ch:act("@RYou focus ki and point both your arms at @r$N@R. However $N seems to shake off your paralysis attack!@n", true, nil, vict, "char")
        ch:act("@r$n @Rfocuses ki and points both $s arms at YOU! Your insomnia makes you immune to $s feeble paralysis attempt.@n", true, nil, vict, "vict")
        ch:act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $N seems to shake off $s paralysis attack!@n", true, nil, vict, "notvict")
        return
    elseif prob < perc then
        ch:reveal_hiding(0)
        ch:act("@RYou focus ki and point both your arms at @r$N@R. However $E manages to avoid your attempt to paralyze $M!@n", true, nil, vict, "char")
        ch:act("@r$n @Rfocuses ki and points both $s arms at YOU! You manage to avoid $s technique though...@n", true, nil, vict, "vict")
        ch:act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $E manages to avoid @r$n's@R attempted technique...@n", true, nil, vict, "notvict")
        ch:meter_mod_int("ki", -ki_cost)
        ch:improve_skill("paralyze", 0)
    else
        ch:reveal_hiding(0)
        ch:act("@RYou focus ki and point both your arms at @r$N@R. Your ki flows into $S body and partially paralyzes $M!@n", true, nil, vict, "char")
        ch:act("@r$n @Rfocuses ki and points both $s arms at YOU! You are caught in $s paralysis technique and now can barely move!@n", true, nil, vict, "vict")
        ch:act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. @r$n's@R ki flows into @r$N@R body and partially paralyzes $M!@n", true, nil, vict, "notvict")
        local duration = math.floor(ch:stat_get("intelligence") / 15)
        vict:condition_apply_with_duration("paralyze", "skill", "paralyze", duration * SMH)
        ch:meter_mod_int("ki", -ki_cost)
        ch:improve_skill("paralyze", 0)
    end
end

return {
    id      = "paralyze",
    aliases = { {"paralyze", 7} },
    execute = execute,
}
