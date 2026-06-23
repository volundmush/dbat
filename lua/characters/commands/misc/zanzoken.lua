local dbat = require("dbat")

local function execute(ctx)
    local ch = ctx.ch

    if not ch:is_npc() and ch:know_skill("zanzoken") == false then
        return
    end

    if ch:condition_has("zanzoken") then
        ch:condition_remove("zanzoken", "zanzoken_over")
        ch:send_line("You release the ki you had prepared for a zanzoken.")
        return
    end

    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are busy in a grapple!")
        return
    end

    local prob = ch:is_npc() and math.random(80, 90) or ch:skill_get("zanzoken")
    local cost = math.floor(ch:meter_max("ki") / 50)
    if prob > 75 then
        cost = cost * 2
    elseif prob > 50 then
        cost = cost * 4
    elseif prob >= 25 then
        cost = cost * 8
    else
        cost = cost * 10
    end

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki.")
        return
    end

    if prob < dbat.axion_dice(0) then
        ch:send_line("You focus your ki in preparation of a zanzoken but mess up and waste your ki!")
        ch:improve_skill("zanzoken", 1)
        ch:meter_mod_int("ki", -cost)
        ch:wait_set(20)
        return
    end

    ch:send_line("@wYou focus your ki, preparing to move at super speeds if necessary.@n")
    ch:meter_mod_int("ki", -cost)
    ch:condition_apply("zanzoken", "skill", "zanzoken")
    ch:improve_skill("zanzoken", 1)
    ch:wait_set(20)
end

local function can_execute(ch)
    return ch:is_npc() or ch:know_skill("zanzoken")
end

return { id = "zanzoken", aliases = { { "zanzoken", 5 } }, execute = execute, can_execute = can_execute }
