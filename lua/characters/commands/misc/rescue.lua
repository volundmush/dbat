local dbat   = require("dbat")

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if arg == "" then
        ch:send_line("Whom do you wish to rescue?")
        return
    end

    local helpee = ch:acquire_room_target(arg)
    if not helpee then
        ch:send_line("%s", dbat.consts.noperson)
        return
    end
    if helpee == ch then
        ch:send_line("You can't help yourself any more than this!")
        return
    end

    local opponent = helpee:fighting_get()
    if not opponent then
        ch:send_line("They are not fighting anyone!")
        return
    end
    if ch:fighting_get() and not ch:is_npc() then
        ch:send_line("You are a little too busy fighting for yourself!")
        return
    end

    local my_speed = ch:der_total("speed_index")
    local mob_bonus = ch:is_npc() and math.floor(my_speed * 0.2) or 0
    local actlib = dbat.lib.act
    if (my_speed + mob_bonus) < opponent:der_total("speed_index") and math.random(1, 3) ~= 3 then
        actlib.message({
            actor  = "@GYou leap towards @g$N@G and try to rescue $M but are too slow!@n",
            target = "@g$n@G leaps towards you! $n is too slow and fails to rescue you!@n",
            room   = "@g$n@G leaps towards @g$N@G and tries to rescue $M but is too slow!@n",
        }, { actor = ch, target = helpee })
        return
    end

    actlib.message({
        actor  = "@GYou leap in front of @g$N@G and rescue $M!@n",
        target = "@g$n@G leaps in front of you! You are rescued!@n",
        room   = "@g$n@G leaps in front of @g$N@G and rescues $M!@n",
    }, { actor = ch, target = helpee })

    opponent:stop_fighting()
    local lvl = ch:stat_get("level")
    helpee:damage({ powerlevel = math.random(1, lvl) }, ch)
    ch:damage({ powerlevel = math.random(1, opponent:stat_get("level")) }, opponent)
end

return { id = "rescue", aliases = { { "rescue", 3 } }, execute = execute }
