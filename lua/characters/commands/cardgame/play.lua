local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local DUEL_TABLE_MIN = 604
local DUEL_TABLE_MAX = 607

local function can_execute(ch)
    local sits = ch:sits_get()
    if not sits then
        return false, "You need to be sitting at an official table to play."
    end
    local vnum = sits:vnum_get()
    if vnum < DUEL_TABLE_MIN or vnum > DUEL_TABLE_MAX then
        return false, "You need to be sitting at an official table to play."
    end
    return true
end

local function execute(ctx)
    local ch   = ctx.ch
    local arg  = ctx.argparams.tokens[1] or ""

    if arg == "" then
        ch:send_line("Syntax: play (card name)")
        return
    end

    local card = Search(ch):add_character_inventory(ch):add_filter(function(s, e)
        return s:can_see(e)
    end):find_one(arg)
    if not card then
        ch:send_line("You don't have that card to play.")
        return
    end

    -- Find the play surface: sits vnum - 4 (the table object)
    local sits       = ch:sits_get()
    local table_vnum = sits:vnum_get() - 4
    local table_obj  = nil
    local room       = ch:room_get()
    for obj in room:contents() do
        if obj:vnum_get() == table_vnum then
            table_obj = obj
            break
        end
    end

    if not table_obj then
        ch:send_line("Your table is missing. Inform an immortal of this problem.")
        return
    end

    act.to_char(ch, "You play $p on your table.", { object = card })
    act.around(ch,  "$n plays $p on $s table.",   { object = card })
    card:from_char()
    card:to_container(table_obj)
end

return { id = "play", aliases = { { "play", 3 } }, execute = execute, can_execute = can_execute }
