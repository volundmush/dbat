local dbat = require("dbat")
local BON  = dbat.consts.bonuses

local POSES = {
    {
        char = "@WYou turn around with your back to everyone. You bend forward dramatically and put your head between your legs!@n",
        room = "@C$n@W turns around with $s back to you. $e bends forward dramatically and puts $s head between $s legs. Strange...@n",
    },
    {
        char = "@WYou turn to the side while flexing your muscles and extend your arms up at an angle dramatically!@n",
        room = "@C$n@W turns to the side while flexing $s muscles and extending $s arms up at an angle dramatically!@n",
    },
    {
        char = "@WYou extend one leg outward while you bend forward, balancing on a single leg!@n",
        room = "@C$n@W extends one leg outward while $e bends forward, balancing on a single leg!@n",
    },
    {
        char = "@WYou drop down to one knee while angling your arms up to either side and slanting your hands down like wings!@n",
        room = "@C$n@W drops down to one knee while angling $s arms up to either side and slanting $s hands down like wings!@n",
    },
}

local function execute(ctx)
    local ch = ctx.ch

    if not ch:know_skill("special pose") then return end

    local stamina_cost = math.floor(ch:meter_max("stamina") / 40)
    if ch:meter_current("stamina") < stamina_cost then
        ch:send_line("You do not have enough stamina to pull off such an exciting pose!")
        return
    end

    if ch:condition_has("special_pose") then
        ch:send_line("You are already feeling good and confident from a previous pose.")
        return
    end

    if ch:is_fighting() then
        ch:send_line("You are too busy to pose right now!")
        return
    end

    if ch:stat_get("strength") + 8 > 70 and ch:bonus_flagged(BON.WIMP) then
        ch:send_line("You can't handle having your strength increased beyond 70.")
        return
    end
    if ch:stat_get("wisdom") + 8 > 70 and ch:bonus_flagged(BON.FOOLISH) then
        ch:send_line("You can't handle having your wisdom increased beyond 70.")
        return
    end

    local prob = ch:skill_get("special pose")
    local perc = math.random(1, 70)

    ch:reveal_hiding(0)

    if prob < perc then
        ch:act("@WYou attempt to strike an awe inspiring pose, but end up falling on your face!@n", true, nil, nil, "char")
        ch:act("@C$n@W attempts to strike an awe inspiring pose, but ends up falling on $s face!@n", true, nil, nil, "room")
        ch:meter_mod_int("stamina", -stamina_cost)
        ch:improve_skill("special pose", 0)
        return
    end

    local pose = POSES[math.random(1, #POSES)]
    ch:act(pose.char, true, nil, nil, "char")
    ch:act(pose.room, true, nil, nil, "room")

    ch:send_line("@WYou feel your confidence increase! @G+8 Str @Wand@G +8 Wis!@n")
    local before_max = ch:meter_max("powerlevel")
    local duration   = 5 * ch:skill_get("special pose")
    ch:condition_apply_with_duration("special_pose", "affect", "special_pose", duration)
    local delta = ch:meter_max("powerlevel") - before_max
    if delta > 0 then
        ch:meter_mod_int("powerlevel", delta)
    end
    ch:meter_mod_int("stamina", -stamina_cost)
    ch:improve_skill("special pose", 0)
end

return {
    id      = "pose",
    aliases = { {"pose", 3} },
    execute = execute,
}
