local dbat = require("dbat")
local MF   = dbat.consts.mob_flags
local PLR  = dbat.consts.player_flags  -- PLR.AURALIGHT still used

local function make_msg(max_pl)
    if max_pl < 50000 then
        return
            "@RYou begin to powerup, and air billows outward around you!@n",
            "@R$n begins to powerup, and air billows outward around $m!@n"
    elseif max_pl < 500000 then
        return
            "@RYou begin to powerup, and loose objects are lifted into the air!@n",
            "@R$n begins to powerup, and loose objects are lifted into the air!@n"
    elseif max_pl < 5000000 then
        return
            "@RYou begin to powerup, and torrents of energy crackle around you!@n",
            "@R$n begins to powerup, and torrents of energy crackle around $m!@n"
    elseif max_pl < 50000000 then
        return
            "@RYou begin to powerup, and the entire area begins to shudder!@n",
            "@R$n begins to powerup, and the entire area begins to shudder!@n"
    elseif max_pl < 100000000 then
        return
            "@RYou begin to powerup, and massive cracks begin to form beneath you!@n",
            "@R$n begins to powerup, and massive cracks begin to form beneath $m!@n"
    elseif max_pl < 300000000 then
        return
            "@RYou begin to powerup, and everything around you shudders from the power!@n",
            "@R$n begins to powerup, and everything around $m shudders from the power!@n"
    else
        return
            "@RYou begin to powerup, and the very air around you begins to burn!@n",
            "@R$n begins to powerup, and the very air around $m begins to burn!@n"
    end
end

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then
        ch:mob_flag_set(MF.POWERUP, true)
        local self_msg, room_msg = make_msg(ch:meter_max("powerlevel"))
        ch:send_line(self_msg)
        ch:act_around(room_msg)
        return
    end

    if ch:player_flagged(PLR.AURALIGHT) then
        ch:send_line("@WYou are concentrating too much on your aura to be able to power up.")
        return
    end
    if ch:race_get() == "android" then
        ch:send_line("@WYou are an android, you do not powerup.@n")
        return
    end
    if (ch:stat_get("suppression") or 0) > 0 then
        ch:send_line("@WYou currently have your powerlevel suppressed to %d percent.@n", ch:stat_get("suppression"))
        return
    end
    if ch:condition_has("powering_up") then
        ch:send_line("@WYou stop powering up.@n")
        ch:condition_remove("powering_up", "silent")
        return
    end
    if ch:meter_current("powerlevel") >= ch:meter_max("powerlevel") then
        ch:send_line("@WYou are already at max!@n")
        return
    end
    if ch:meter_current("ki") < math.floor(ch:meter_max("ki") / 20) then
        ch:send_line("@WYou do not have enough ki to powerup!@n")
        return
    end

    ch:reveal_hiding(0)
    local self_msg, room_msg = make_msg(ch:meter_max("powerlevel"))
    ch:send_line(self_msg)
    ch:act_around(room_msg)
    ch:condition_apply("powering_up", "command", "started")
end

return { id = "powerup", aliases = { { "powerup", 3 } }, execute = execute }
