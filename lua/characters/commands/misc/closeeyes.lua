local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then return end

    if ch:player_flagged(PLR.EYEC) then
        ch:player_flag_set(PLR.EYEC, false)
        ch:send_line("@wYou open your eyes.@n")
        ch:act_around("@C$n@w opens $s eyes.@n")
    else
        ch:player_flag_set(PLR.EYEC, true)
        ch:send_line("@wYou close your eyes.@n")
        ch:act_around("@C$n@w closes $s eyes.@n")
    end

    ch:wait_set(10)
end

return { id = "closeeyes", aliases = { { "closeeyes", 7 } }, execute = execute }
