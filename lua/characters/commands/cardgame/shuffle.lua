local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act
local CARD   = dbat.consts.item_extra_flags.ANTI_HIEROPHANT

local STAGING_ROOM_ID = 48
local DUEL_TABLE_MIN  = 604
local DUEL_TABLE_MAX  = 607

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
    local ch = ctx.ch

    local case_obj = Search(ch):add_character_inventory(ch):find_one("case")
    if not case_obj then
        ch:send_line("You don't have a case.")
        return
    end

    -- Count cards
    local count = 0
    for obj in case_obj:inventory() do
        if obj:extra_flagged(CARD) then count = count + 1 end
    end
    if count == 0 then
        ch:send_line("You don't have any cards in the case!")
        return
    end
    local total = count

    -- Move all cards to staging room
    local staging = dbat.rooms.by_id(STAGING_ROOM_ID)
    local cards = {}
    for obj in case_obj:inventory() do
        if obj:extra_flagged(CARD) then
            cards[#cards+1] = obj
        end
    end
    for _, obj in ipairs(cards) do
        obj:from_container()
        obj:to_room(staging)
    end

    -- Shuffle back randomly
    while count > 0 do
        for obj in staging:contents() do
            if obj:extra_flagged(CARD) then
                if count > 1 and math.random(4) == 3 then
                    count = count - 1
                    obj:from_room()
                    obj:to_container(case_obj)
                elseif count == 1 then
                    count = count - 1
                    obj:from_room()
                    obj:to_container(case_obj)
                end
            end
        end
    end

    ch:send_line("You shuffle the cards carefully.")
    act.around(ch, "$n shuffles their deck.")
    local room = ch:room_get()
    if room then
        room:send_line("There were %d cards in the deck.", total)
    end
end

return { id = "shuffle", aliases = { { "shuffle", 5 } }, execute = execute, can_execute = can_execute }
