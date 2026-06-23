local dbat   = require("dbat")
local act    = dbat.lib.act

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    local current = ch:defending_for_get()

    if arg == "" then
        if not current then
            ch:send_line("Defend who?")
            return
        end
        -- stop defending
        local actx = { actor = ch, target = current }
        act.to_char(ch, "@YYou stop defending @y$N@Y.@n", actx)
        act.to_target("@y$n@Y stops defending you.@n", actx)
        act.to_room("@y$n@Y stops defending @y$N@Y.@n", { actor = ch, target = current, exclude = { ch, current } })
        current:defended_by_set(nil)
        ch:defending_for_set(nil)
        return
    end

    local vict = ch:acquire_room_target(arg)
    if not vict then
        ch:send_line("You can't seem to find that person.")
        return
    end

    if vict:id_get() == ch:id_get() then
        ch:send_line("Well hopefully you are smart enough to defend yourself.")
        return
    end

    local actx = { actor = ch, target = vict }
    act.to_char(ch, "@YYou start defending @y$N@Y.@n", actx)
    act.to_target("@y$n@Y starts defending you.@n", actx)
    act.to_room("@y$n@Y starts defending @y$N@Y.@n", { actor = ch, target = vict, exclude = { ch, vict } })
    vict:defended_by_set(ch)
    ch:defending_for_set(vict)
end

return { id = "defend", aliases = { { "defend", 3 } }, execute = execute }
