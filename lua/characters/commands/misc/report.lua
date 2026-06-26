local function execute(ctx)
    local ch = ctx.ch

    if not ch:condition_has("group") then
        ch:send_line("But you are not a member of any group!")
        return
    end

    local msg = string.format("$n reports: %d/%dH, %d/%dM, %d/%dV",
        ch:meter_current("powerlevel"), ch:meter_max("powerlevel"),
        ch:meter_current("ki"),         ch:meter_max("ki"),
        ch:meter_current("stamina"),    ch:meter_max("stamina"))

    local leader = ch:following_get() or ch

    if not leader:is_same(ch) and leader:condition_has("group") then
        ch:act(msg, true, nil, leader, "vict")
    end

    leader:followers_each(function(fol)
        if fol:condition_has("group") and not fol:is_same(ch) then
            ch:act(msg, true, nil, fol, "vict")
        end
    end)

    ch:send_line("You report to the group.")
end

return {
    id      = "report",
    aliases = { {"report", 3} },
    execute = execute,
}
