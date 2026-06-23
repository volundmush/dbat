local dbat   = require("dbat")
local AFF    = dbat.consts.aff_flags
local PLR    = dbat.consts.player_flags
local POS    = dbat.consts.positions
local ITYPE  = dbat.consts.item_types
local SECT   = dbat.consts.sector_types
local search = dbat.lib.search
local act    = dbat.lib.act

local function stop_dragging(ch)
    local dragged = ch:dragging_get()
    if not dragged then return end
    act.to_char(ch, "@WYou stop dragging @C$N@W!@n", { actor = ch, target = dragged })
    act.around(ch, "@C$n@W stops dragging @c$N@W!@n", { actor = ch, target = dragged })
    dragged:being_dragged_set(nil)
    ch:dragging_set(nil)
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:player_flagged(PLR.PILOTING) then
        ch:send_line("You are busy piloting a ship!")
        return
    end
    if ch:player_flagged(PLR.HEALT) then
        ch:send_line("You are inside a healing tank!")
        return
    end

    stop_dragging(ch)

    if ch:carrying_char_get() then
        ch:send_line("You are busy carrying someone!")
        return
    end

    if ch:condition_has("flying") then ch:land() end

    local pos = ch:position_get()

    if arg == "" then
        if pos == POS.STANDING then
            ch:reveal_hiding(0)
            ch:send_line("You sit down.")
            ch:act_around("$n sits down.")
            ch:position_set(POS.SITTING)
        elseif pos == POS.SITTING then
            ch:send_line("You're sitting already.")
        elseif pos == POS.RESTING then
            ch:send_line("You stop resting, and sit up.")
            ch:act_around("$n stops resting.")
            ch:position_set(POS.SITTING)
        elseif pos == POS.SLEEPING then
            ch:send_line("You have to wake up first.")
        elseif pos == POS.FIGHTING then
            ch:send_line("Sit down while fighting? Are you MAD?")
        else
            ch:send_line("You stop floating around, and sit down.")
            ch:act_around("$n stops floating around, and sits down.")
            ch:position_set(POS.SITTING)
        end
    else
        if ch:sits_get() then
            ch:send_line("You are already on something!")
            return
        end
        local room  = ch:room_get()
        local chair = search.new(ch):add_room_objects(room):add_filter(function(s, e)
            return s:can_see(e)
        end):find_one(arg)
        if not chair then
            ch:send_line("That isn't here.")
            return
        end
        if chair:vnum_get() == 65 then
            ch:send_line("You can't get on that!")
            return
        end
        if chair:type_get() ~= ITYPE.CHAIR and chair:type_get() ~= ITYPE.BED then
            ch:send_line("You can't sit on that!")
            return
        end
        if chair:size_get() + 1 < ch:size_get() then
            ch:send_line("You are too large for it!")
            return
        end
        if pos == POS.STANDING then
            ch:reveal_hiding(0)
            act.to_char(ch, "You sit down on $p.", { actor = ch, tool = chair })
            act.around(ch, "$n sits down on $p.", { actor = ch, tool = chair })
            ch:position_set(POS.SITTING)
            ch:sits_set(chair)
            chair:sitting_set(ch)
        elseif pos == POS.SITTING or pos == POS.RESTING then
            ch:send_line("You should stand up first.")
        elseif pos == POS.SLEEPING then
            ch:send_line("You have to wake up first.")
        elseif pos == POS.FIGHTING then
            ch:send_line("Sit down while fighting? Are you MAD?")
        else
            ch:send_line("You stop floating around, and sit down.")
            ch:act_around("$n stops floating around, and sits down.")
            ch:position_set(POS.SITTING)
        end
    end
end

return { id = "sit", aliases = { { "sit", 2 } }, execute = execute }
