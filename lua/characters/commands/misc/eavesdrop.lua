local dbat = require("dbat")
local EX   = dbat.consts.exit_flags
local POS  = dbat.consts.positions

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.RESTING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    end

    return true
end

local function direction_index(arg)
    if arg == "" or arg:sub(1, 1) == "!" then return nil end

    local word = arg:lower()
    for i, name in ipairs(dbat.consts.direction_names) do
        if name:sub(1, 1) == "\n" then break end
        if name:sub(1, #word) == word then return i - 1 end
    end
    return nil
end

local function first_word(text)
    return (text or ""):match("^(%S+)") or text
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end

    if ch:eavesdrop_get() > 0 then
        ch:send_line("You stop eavesdropping.")
        ch:eavesdrop_set(0)
        ch:eavesdrop_dir_set(-1)
        return
    end

    if arg == "" then
        ch:send_line("In which direction would you like to eavesdrop?")
        return
    end

    local dir = direction_index(arg)
    if not dir then
        ch:send_line("Which directions is that?")
        return
    end

    if not ch:know_skill("eavesdrop") then return end

    local room = ch:room_get()
    local exit = room and room:exit_get(dir)
    if exit then
        local keyword = exit:keyword()
        if exit:flagged(EX.CLOSED) and keyword and keyword ~= "" then
            ch:send_line("The %s is closed.", first_word(keyword))
        else
            local dest = exit:destination()
            if dest then
                ch:eavesdrop_set(dest:vnum_get())
                ch:eavesdrop_dir_set(dir)
                ch:send_line("Okay.")
            else
                ch:send_line("There is not a room there...")
            end
        end
    else
        ch:send_line("There is not a room there...")
    end
end

return {
    id      = "eavesdrop",
    aliases = { {"eavesdrop", 5} },
    execute = execute,
}
