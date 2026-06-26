local dbat = require("dbat")
local RF   = dbat.consts.room_flags

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then return end

    if not ch:know_skill("taisha reiki") then return end

    local room = ch:room_get()

    if room:flagged(RF.AURA) then
        ch:send_line("This area already has an aura of regeneration around it.")
        return
    end

    if room:gravity_get() > 0 then
        ch:send_line("This area's gravity is too hostile to an aura.")
        return
    end

    local ki_cost = math.floor(ch:meter_max("ki") / 3)
    if ch:meter_current("ki") < ki_cost then
        ch:send_line("You don't have enough ki.")
        return
    end

    local prob = ch:skill_get("taisha reiki")
    local perc = dbat.axion_dice(0)

    ch:meter_mod_int("ki", -ki_cost)

    if prob < perc then
        ch:reveal_hiding(0)
        ch:act("@WYou hold up your hands while channeling ki. Your technique fails to produce an aura though....@n", true, nil, nil, "char")
        ch:act("@g$n@W holds up $s hands while channeling ki. $s technique fails to produce an aura though....", true, nil, nil, "room")
        ch:improve_skill("taisha reiki", 1)
    else
        ch:reveal_hiding(0)
        ch:act("@WYou hold up your hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", true, nil, nil, "char")
        ch:act("@g$n holds up $s hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", true, nil, nil, "room")
        ch:improve_skill("taisha reiki", 1)
        ch:room_get():flag_set(RF.AURA, true)
    end
end

return {
    id      = "taisha",
    aliases = { {"taisha", 5} },
    execute = execute,
}
