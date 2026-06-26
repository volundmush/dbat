local dbat      = require("dbat")
local MOB       = dbat.consts.mob_flags
local PULSE_3SEC = 30  -- 3 * PASSES_PER_SEC (10)

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if ch:race_get() ~= "majin" then
        ch:send_line("You are not a majin, how can you do that?")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("Turn who into candy?")
        return
    end
    if not ch:can_kill(vict) then return end

    if not vict:is_npc() then
        ch:send_line("You can't turn them into candy.")
        return
    end

    local ch_max   = ch:meter_max("powerlevel")
    local vict_max = vict:meter_max("powerlevel")
    local level    = ch:stat_get("level")

    if vict_max > ch_max * 2 then
        ch:send_line("They are too powerful.")
        return
    end
    if vict_max < ch_max * 0.25 and level < 100 then
        ch:send_line("They are too weak.")
        return
    end
    if vict_max < ch_max * 0.09 and level == 100 then
        ch:send_line("They are too weak.")
        return
    end

    local ki_cost = math.floor(ch_max / 15)
    if ch:meter_current("ki") < ki_cost then
        ch:send_line("You do not have enough ki.")
        return
    end

    local actual_ki_cost = math.floor(ch:meter_max("ki") / 15)

    if math.random(1, 6) == 6 then
        ch:meter_mod_int("ki", -actual_ki_cost)
        ch:reveal_hiding(0)
        ch:act("@cYou aim your forelock at @R$N@c and fire a beam of energy but it is dodged!@n", true, nil, vict, "char")
        ch:act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy but the beam is dodged!@n", true, nil, vict, "notvict")
        if not ch:is_fighting()   then ch:start_fighting(vict) end
        if not vict:is_fighting() then vict:start_fighting(ch) end
        ch:wait_set(PULSE_3SEC)
        return
    end

    ch:meter_mod_int("ki", -actual_ki_cost)
    ch:reveal_hiding(0)
    ch:act("@cYou aim your forelock at @R$N@c and fire a beam of energy that envelopes $S entire body and changes $M into candy!@n", true, nil, vict, "char")
    ch:act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy that envelopes $S entire body and changes $M into candy!@n ", true, nil, vict, "notvict")

    local candy_vnum
    if vict_max >= ch_max * 1.5 then
        candy_vnum = 95
    elseif vict_max >= ch_max then
        candy_vnum = 94
    elseif vict_max >= ch_max * (level < 100 and 0.5 or 0.1) then
        candy_vnum = 93
    else
        candy_vnum = 53
    end
    ch:send_line("You grab the candy as it falls.")
    local obj = dbat.read_object(candy_vnum)
    obj:to_char(ch)

    vict:mob_flag_set(MOB.HUSK, true)
    vict:die(ch)
end

return {
    id      = "candy",
    aliases = { {"candy", 4} },
    execute = execute,
}
