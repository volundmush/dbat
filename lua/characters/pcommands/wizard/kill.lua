local dbat   = require("dbat")
local search = dbat.lib.search

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:is_npc() then return end

    if arg == "" then
        ch:send_line("Kill who?")
        return
    end

    local vict = search.new(ch):add_room_people(ch:room_get()):find_one(arg)
    if not vict then
        ch:send_line("They aren't here.")
        return
    end
    if vict == ch then
        ch:send_line("Your mother would be so sad.. :(")
        return
    end

    local actlib = dbat.lib.act
    actlib.message({
        actor  = "You chop $N to pieces!  Ah!  The blood!",
        target = "$N chops you to pieces!",
        room   = "$n brutally slays $N!",
    }, { actor = ch, target = vict })
    vict:die(ch)
end

return { id = "kill", aliases = { { "kill", 4 } }, execute = execute }
