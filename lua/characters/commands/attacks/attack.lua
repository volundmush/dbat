local dbat = require("dbat")
local WP   = dbat.consts.wear_positions

local DAMTYPE_TO_ATTACK = {
    slash  = "slash",
    pierce = "pierce",
    crush  = "crush",
    stab   = "stab",
    blast  = "shoot",
    brawl  = "brawl",
}

local function attack_id_for(weap)
    if not weap then return nil end
    return DAMTYPE_TO_ATTACK[weap:weapon_damtype_get()] or "brawl"
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1]

    local w1 = ch:equipment_get(WP.WIELD1)
    local w2 = ch:equipment_get(WP.WIELD2)

    if not w1 and not w2 then
        ch:send_line("You need to wield a weapon to use this, without one try punch, kick, or other no weapon attacks.")
        return
    end

    local target = ch:acquire_room_target(arg)
    if not target then
        ch:send_line("Direct it at who?")
        return
    end

    ch:launch_attack(attack_id_for(w1 or w2), target)

    -- Dual-wield off-hand follow-up (mirrors C++ do_attack2 trigger condition)
    if w1 and w2 and target:meter_current("powerlevel") > 1 then
        if ch:skill_get("dual wield") > dbat.axion_dice(0) then
            ch:launch_attack(attack_id_for(w2), target)
        end
    end
end

local function can_execute(ch)
    return ch:equipment_get(WP.WIELD1) ~= nil or ch:equipment_get(WP.WIELD2) ~= nil
end

return { id = "attack", aliases = { { "attack", 6 } }, execute = execute, can_execute = can_execute }
