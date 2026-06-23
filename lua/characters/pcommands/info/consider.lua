local dbat   = require("dbat")
local Search = dbat.lib.search.new

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if arg == "" then
        ch:send_line("Consider killing who?")
        return
    end

    local room   = ch:room_get()
    local victim = Search(ch):add_room_people(room):add_filter(function(s, e)
        return s:can_see(e)
    end):find_one(arg)

    if not victim then
        ch:send_line("Consider killing who?")
        return
    end

    if victim:id_get() == ch:id_get() then
        ch:send_line("Easy!  Very easy indeed!")
        return
    end

    local diff = victim:stat_get("level") - ch:stat_get("level")

    if     diff <= -10 then ch:send_line("Now where did that chicken go?")
    elseif diff <=  -5 then ch:send_line("You could do it with a needle!")
    elseif diff <=  -2 then ch:send_line("Easy.")
    elseif diff <=  -1 then ch:send_line("Fairly easy.")
    elseif diff ==   0 then ch:send_line("The perfect match!")
    elseif diff <=   1 then ch:send_line("You could probably manage it.")
    elseif diff <=   2 then ch:send_line("You might take a beating.")
    elseif diff <=   3 then ch:send_line("You MIGHT win, maybe.")
    elseif diff <=   5 then ch:send_line("Do you feel lucky? You better.")
    elseif diff <=  10 then ch:send_line("Better bring some tough backup!")
    elseif diff <=  25 then ch:send_line("Maybe if they are allergic to you, otherwise your last words will be 'Oh shit.'")
    else                    ch:send_line("No chance.")
    end
end

return { id = "consider", aliases = { { "consider", 3 } }, execute = execute }
