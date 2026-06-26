local dbat = require("dbat")
local act  = dbat.lib.act
local text = dbat.lib.text

local function execute(ctx)
    local ch = ctx.ch

    if ch:race_get() ~= "arlian" or ch:sex_get() ~= "female" then
        ch:send_line("You are not an arlian female.")
        return
    end
    if not ch:condition_has("group") then
        ch:send_line("You need to be in a group to use this skill!")
        return
    end

    local cost = math.floor(ch:meter_max_get("stamina") * 0.02)
                 + math.floor(ch:meter_max_get("lifeforce") * 0.02)

    if ch:meter_get("powerlevel") >= ch:meter_max_get("powerlevel") * 0.8 then
        cost = math.floor(cost * 0.5)
    end

    local heal = cost * 3

    if ch:meter_get("stamina") < cost then
        ch:send_line("You do not have enough stamina to perform this technique.")
        return
    end

    local chance = dbat.axion_dice(0)

    if chance > ch:stat_get("wisdom") + math.random(1, 10) then
        act.to_char(ch, "@GYou spread your wings and begin to concentrate. Your wings begin to glow a soft sea green color. As you prepare to release a cloud of your charged wing dust you lose focus and the power you had begun to charge into your wings dissipates.@n")
        act.around(ch, "@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly $s wings begin to glow a soft sea green color. This soft glow grows brighter for a second before fading completely.@n", {})
        ch:meter_mod_int("stamina", -cost)
        ch:wait_set(10)
        return
    end

    ch:meter_set("powerlevel", math.min(ch:meter_get("powerlevel") + heal, ch:meter_max_get("powerlevel")))
    ch:meter_mod_int("stamina", -cost)
    ch:wait_set(10)

    act.to_char(ch, "@GYou spread your wings and begin to concentrate. Your wings begin to glow a soft sea green color. As your wings grow brighter you focus your charged bio energy in a shockwave the unleashes a cloud of glowing green dust. You breath in the dust and feel it rejuvinate your body's cells!@n")
    act.around(ch, "@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly $s wings begin to glow a soft sea green color. This soft glow grows brighter and as $e flexes $s wings to their full extent a shockwave of energy explodes outward. Carried on this shockwave is a cloud of glowing dust! You notice some of the dust being breathed in by $s!@n", {})
    ch:send_line("@RHeal@Y: @C%s@n", text.add_commas(heal))

    local ch_master = ch:following_get()
    for vict in ch:room_get():people() do
        if not vict:is_same(ch) and vict:condition_has("group") then
            local vm = vict:following_get()
            if (ch_master ~= nil and ch_master == vm)
                or vm == ch
                or ch_master == vict
            then
                vict:meter_set("powerlevel", math.min(vict:meter_get("powerlevel") + heal, vict:meter_max_get("powerlevel")))
                act.to_char(vict, "@CYou breathe in the dust and are healed by it somewhat!@n")
                act.around(vict, "@c$n@C breathes in the dust and is healed somewhat!@n", {})
                vict:send_line("@RHeal@Y: @C%s@n", text.add_commas(heal))
            end
        end
    end
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return ch:race_get() == "arlian"
end

return {
    id          = "moondust",
    aliases     = { {"moondust", 4} },
    execute     = execute,
    can_execute = can_execute,
}
