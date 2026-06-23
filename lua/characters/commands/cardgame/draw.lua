local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local DUEL_TABLE_MIN = 604
local DUEL_TABLE_MAX = 607

local function can_execute(ch)
    local sits = ch:sits_get()
    if not sits then
        return false, "You are not sitting at a duel table."
    end
    local vnum = sits:vnum_get()
    if vnum < DUEL_TABLE_MIN or vnum > DUEL_TABLE_MAX then
        return false, "You need to be sitting at an official table to play."
    end
    return true
end

local function execute(ctx)
    local ch   = ctx.ch
    local sits = ch:sits_get()

    local case_obj = Search(ch):add_character_inventory(ch):find_one("case")
    if not case_obj then
        ch:send_line("You don't have a case.")
        return
    end

    local card = nil
    for obj in case_obj:inventory() do
        card = obj
        break
    end

    if not card then
        ch:send_line("You don't have any cards in the case!")
        return
    end

    card:from_container()
    card:to_char(ch)
    act.around(ch, "$n draws a card from $s $p.\r\n", { object = case_obj })
    ch:send_line("You draw a card.\r\n%s", card:action_description_get() or "")
end

return { id = "draw", aliases = { { "draw", 3 } }, execute = execute, can_execute = can_execute }
