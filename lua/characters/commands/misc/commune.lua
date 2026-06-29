local dbat = require("dbat")

local PULSE_2SEC = 20

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then return end
    if not ch:know_skill("commune") then return end

    if ch:meter_current("stamina") >= ch:meter_max("stamina") then
        ch:send_line("Your stamina is already at full.")
        return
    end

    local skill = ch:skill_get("commune")
    local cost = math.floor(ch:meter_max("stamina") * 0.05)

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to commune with the Eldritch Star.")
        return
    end

    ch:meter_mod_int("ki", -cost)
    ch:reveal_hiding(0)

    if skill < dbat.axion_dice(0) then
        ch:act("@cYou close your eyes and try to commune with the Eldritch Star. You are unable to concentrate though.@n",
            true, nil, nil, "char")
        ch:act("@W$n closes $s eyes for a moment. Then $e reopens them and frowns.@n",
            true, nil, nil, "room")
        ch:wait_set(PULSE_2SEC)
        return
    end

    ch:meter_mod_int("stamina", -cost)
    ch:act("@cYou close your eyes and commune with the Eldritch Star spiritually. You feel your stamina replenish some.@n",
        true, nil, nil, "char")
    ch:act("@W$n closes $s eyes for a moment. Then $e reopens them and smiles.@n",
        true, nil, nil, "room")
    ch:wait_set(PULSE_2SEC)
end

return {
    id = "commune",
    aliases = { {"commune", 4} },
    execute = execute,
}
