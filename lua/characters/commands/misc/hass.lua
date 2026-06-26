local dbat = require("dbat")
local SMH  = dbat.consts.secs_per_mud_hour

local function execute(ctx)
    local ch = ctx.ch

    if not ch:know_skill("hasshuken") then return end

    local stamina_cost = math.floor(ch:meter_max("stamina") / 30)
    if ch:meter_current("stamina") < stamina_cost then
        ch:send_line("You do not have enough stamina.")
        return
    end

    local perc = ch:init_skill("hasshuken")
    local prob = dbat.axion_dice(0)

    ch:reveal_hiding(0)

    if perc < prob then
        ch:act("@WYou try to move your arms at incredible speeds but screw up and waste some of your stamina.@n", true, nil, nil, "char")
        ch:act("@C$n@W tries to move $s arms at incredible speeds but screws up and wastes some of $s stamina.@n", true, nil, nil, "room")
        ch:meter_mod_int("stamina", -stamina_cost)
        ch:improve_skill("hasshuken", 0)
        return
    end

    ch:act("@WYou concentrate and start to move your arms at incredible speeds.@n", true, nil, nil, "char")
    ch:act("@C$n@W concentrates and starts to move $s arms at incredible speeds.@n", true, nil, nil, "room")
    local duration = math.floor(perc / 15) * SMH
    ch:condition_apply_with_duration("hasshuken", "skill", "hasshuken", duration)
    ch:meter_mod_int("stamina", -stamina_cost)
    ch:improve_skill("hasshuken", 0)
end

return {
    id      = "hass",
    aliases = { {"hasshuken", 8} },
    execute = execute,
}
