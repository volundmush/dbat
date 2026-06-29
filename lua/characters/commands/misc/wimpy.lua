local function leading_integer(arg)
    local digits = arg:match("^(%d+)")
    return digits and tonumber(digits) or nil
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if arg == "" then
        local current = ch:wimp_level_get()
        if current ~= 0 then
            ch:send_line("Your current wimp level is %d powerlevel.", current)
        else
            ch:send_line("At the moment, you're not a wimp.  (sure, sure...)")
        end
        return
    end

    local value = leading_integer(arg)
    if not value then
        ch:send_line("Specify a value.  (0 to disable)")
        return
    end

    if ch:is_npc() then return end

    if value ~= 0 then
        local max_powerlevel = ch:meter_max("powerlevel")
        if value > max_powerlevel then
            ch:send_line("That doesn't make much sense, now does it?")
        elseif value > math.floor(max_powerlevel * 0.5) then
            ch:send_line("You can't set your wimp level above half your powerlevel.")
        else
            ch:send_line("Okay, you'll wimp out if you drop below %d powerlevel.", value)
            ch:wimp_level_set(value)
        end
    else
        ch:send_line("Okay, you'll now tough out fights to the bitter end.")
        ch:wimp_level_set(0)
    end
end

return {
    id = "wimpy",
    aliases = { {"wimpy", 5} },
    execute = execute,
}
