local dbat = require("dbat")

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    local has_barrier      = ch:skill_get("barrier") > 0
    local has_aqua_barrier = ch:skill_get("aqua_barrier") > 0

    if arg == "" then
        ch:send_line("[Syntax] barrier < 1-75 | release >")
        return
    end

    if arg == "release" then
        if ch:condition_has("barrier") then
            ch:send_line("@BYou dispel your barrier, releasing its energy.@n")
            ch:act_around("@B$n@B dispels $s barrier, releasing its energy.@n")
            ch:barrier_set(0)
            ch:condition_remove("barrier", "released")
        else
            ch:send_line("You don't have a barrier.")
        end
        return
    end

    if ch:condition_has("barrier") then
        ch:send_line("You already have a barrier, try releasing it.")
        return
    end

    if ch:cooldown_get() > 0 then
        ch:send_line("You must wait a short period before concentrating again.")
        return
    end

    local size = tonumber(arg) or 0
    local prob = has_barrier and ch:skill_get("barrier") or ch:skill_get("aqua_barrier")
    local cost = math.floor(ch:meter_max("ki") * 0.005 * size)

    if size > prob then
        ch:send_line("You can not create a barrier that is stronger than your skill in barrier.")
        return
    elseif size < 1 then
        ch:send_line("You have to put at least some ki into the barrier!")
        return
    elseif size > 75 then
        ch:send_line("You can't control a barrier with more than 75 percent!")
        return
    elseif ch:charge_get() < cost then
        ch:send_line("You do not have enough ki charged up!")
        return
    end

    local skill_id = has_barrier and "barrier" or "aqua_barrier"

    if prob < dbat.axion_dice(0) then
        ch:send_line("@BYou shout as you form a barrier of ki around your body, but you imbalance it and it explodes outward!@n")
        ch:act_around("@B$n@B shouts as $e forms a barrier of ki around $s body, but it becomes imbalanced and explodes outward!@n")
        ch:charge_set(ch:charge_get() - cost)
        ch:improve_skill(skill_id, 1)
        ch:cooldown_set(30)
        return
    end

    if has_barrier then
        ch:send_line("@BYou shout as you form a barrier of ki around your body!@n")
        ch:act_around("@B$n@B shouts as $e forms a barrier of ki around $s body!@n")
    else
        ch:send_line("@BYou shout as you form a barrier of ki and raging waters around your body!@n")
        ch:act_around("@B$n@B shouts as $e forms a barrier of ki and raging waters around $s body!@n")
    end

    local amount = math.floor(ch:meter_max("ki") / 100 * size)
    ch:condition_apply_number("barrier", "amount", amount, "skill", "barrier")
    ch:charge_set(ch:charge_get() - cost)
    ch:improve_skill(skill_id, 1)
    ch:cooldown_set(20)
end

local function can_execute(ch)
    if ch:skill_get("barrier") == 0 and ch:skill_get("aqua_barrier") == 0 then
        return false
    end
    return true
end

return { id = "barrier", aliases = { { "barrier", 5 } }, execute = execute, can_execute = can_execute }
