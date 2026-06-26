local dbat = require("dbat")
local PLR  = dbat.consts.player_flags
local MOB  = dbat.consts.mob_flags
local AFF  = dbat.consts.aff_flags
local POS  = dbat.consts.positions
local SECT = dbat.consts.sector_types

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if ch:is_npc() then return end

    local dragging = ch:dragging_get()
    if dragging then
        ch:dragging_set(nil)
        dragging:being_dragged_set(nil)
        ch:act("@wYou stop dragging @C$N@W.@n", true, nil, dragging, "char")
        ch:act("@C$n@W stops dragging @c$N@W.@n", true, nil, dragging, "room")
        return
    end

    if ch:player_flagged(PLR.PILOTING) then
        ch:send_line("You are busy piloting a ship!")
        return
    end
    if ch:carrying_char_get() then
        ch:send_line("You are busy carrying someone at the moment.")
        return
    end
    if arg == "" then
        ch:send_line("Who do you want to drag?")
        return
    end
    if ch:is_fighting() then
        ch:send_line("You are a bit busy fighting right now!")
        return
    end

    local room = ch:room_get()
    local sect = room:sector_type_get()
    if sect == SECT.WATER_NOSWIM or sect == SECT.WATER_SWIM then
        ch:send_line("You decide to not be a tugboat instead.")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("Drag who?")
        return
    end
    if vict == ch then
        ch:send_line("You can't drag yourself.")
        return
    end
    if vict:being_dragged_get() then
        ch:send_line("They are already being dragged!")
        return
    end
    if vict:is_npc() and vict:mob_flagged(MOB.NOKILL) then
        ch:send_line("They are not to be touched!")
        return
    end

    if vict:position_get() ~= POS.SLEEPING then
        ch:reveal_hiding(0)
        ch:act("@wYou try to grab and pull @C$N@W with you, but $E resists!@n", true, nil, vict, "char")
        ch:act("@C$n@W tries to grab and pull you! However you resist!@n", true, nil, vict, "vict")
        ch:act("@C$n@W tries to grab and pull @c$N@W but $E resists!@n", true, nil, vict, "notvict")
        if vict:is_npc() and not vict:is_fighting() then
            vict:start_fighting(ch)
        end
        return
    end

    local vict_weight = vict:der_total("weight") + vict:der_total("weight_carried")
    if vict_weight > ch:der_total("weight_carry_capacity") then
        ch:reveal_hiding(0)
        ch:act("@wYou try to grab and pull @C$N@W with you, but $E is too heavy!@n", true, nil, vict, "char")
        ch:act("@C$n@W tries to grab and pull @c$N@W but $E is too heavy!@n", true, nil, vict, "room")
        return
    end

    ch:reveal_hiding(0)
    ch:act("@wYou grab and start dragging @C$N@W.@n", true, nil, vict, "char")
    ch:act("@C$n@W grabs and starts dragging @c$N@W.@n", true, nil, vict, "notvict")
    ch:dragging_set(vict)
    vict:being_dragged_set(ch)
    if not vict:aff_flagged(AFF.KNOCKED) and not vict:aff_flagged(AFF.SLEEP)
       and math.random(1, 3) ~= 0 then
        vict:send_line("You feel your sleeping body being moved.")
        if vict:is_npc() and not vict:is_fighting() then
            vict:start_fighting(ch)
        end
    end
end

return {
    id      = "drag",
    aliases = { {"drag", 4} },
    execute = execute,
}
