local dbat = require("dbat")

local function in_arena(ch)
    local rv = ch:room_vnum_get()
    return rv >= 17800 and rv <= 17874
end

local function comm_name(ch)
    if ch:admin_level_get() > 0 then return ch:name_get() end
    return ch:user_get() or ch:name_get()
end

local function execute(ctx)
    local ch = ctx.ch
    local message = (ctx.argparams.raw or ""):match("^%s*(.-)%s*$")

    if ch:is_npc() then return end

    if in_arena(ch) then
        ch:send_line("Lol, no.")
        return
    end

    if ch:skill_get("telepathy") > 0 then
        ch:send_line("You can just use telepathy.")
        return
    end

    local target = ch:mindlinked_get()
    if not target then
        ch:send_line("No one has linked with your mind.")
        return
    end

    if message == "" then
        ch:send_line("Syntax: think (message)")
        return
    end

    ch:send_line("@c%s@w reads your thoughts, '@C%s@w'@n", target:name_get(), message)
    target:send_line("@c%s@w thinks, '@C%s@w'@n", ch:name_get(), message)
    dbat.send_to_imm(string.format("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n",
        comm_name(ch), comm_name(target), message))
end

return {
    id = "think",
    aliases = { {"think", 4} },
    execute = execute,
}
