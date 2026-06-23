local dbat   = require("dbat")
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

    local room = ch:room_get()
    if room:sector_type_get() == SECT.WATER_NOSWIM then
        ch:send_line("You can't rest here!")
        return
    end
    if ch:player_flagged(PLR.HEALT) then
        ch:send_line("You are inside a healing tank!")
        return
    end

    if ch:condition_has("barrier") then
        if ch:know_skill("barrier") then
            ch:send_line("You have a barrier around you and can't rest.")
            return
        else
            ch:condition_remove("barrier", "rest")
        end
    end

    if ch:condition_has("fighting") then
        ch:send_line("You are a bit busy at the moment!")
        return
    end
    if ch:stat_get("kaioken") > 0 then
        ch:send_line("You are utilizing kaioken and can't rest!")
        return
    end

    stop_dragging(ch)

    if ch:carrying_char_get() then
        ch:send_line("You are carrying someone!")
        return
    end

    if ch:condition_has("flying") then ch:land() end

    local pos = ch:position_get()

    if arg == "" then
        local chair = ch:sits_get()
        if chair and chair:type_get() ~= ITYPE.BED then
            ch:send_line("You can't lay on that!")
            return
        end
        if pos == POS.STANDING then
            ch:reveal_hiding(0)
            ch:send_line("You lay down and rest your tired bones.")
            ch:act_around("$n lays down and rests.")
            ch:position_set(POS.RESTING)
        elseif pos == POS.SITTING then
            ch:send_line("You rest your tired bones.")
            ch:act_around("$n rests.")
            ch:position_set(POS.RESTING)
        elseif pos == POS.RESTING then
            ch:send_line("You are already resting.")
        elseif pos == POS.SLEEPING then
            ch:send_line("You have to wake up first.")
        elseif pos == POS.FIGHTING then
            ch:send_line("Rest while fighting?  Are you MAD?")
        else
            ch:send_line("You stop floating around, and stop to rest your tired bones.")
            ch:act_around("$n stops floating around, and rests.")
            ch:position_set(POS.RESTING)
        end
    else
        if ch:sits_get() then
            ch:send_line("You are already on something!")
            return
        end
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
        if chair:size_get() + 1 < ch:size_get() then
            ch:send_line("You are too large for it!")
            return
        end
        if pos == POS.STANDING then
            ch:reveal_hiding(0)
            act.to_char(ch, "You lay down and rest on $p.", { actor = ch, tool = chair })
            act.around(ch, "$n lays down and rests on $p.", { actor = ch, tool = chair })
            ch:sits_set(chair)
            chair:sitting_set(ch)
            ch:position_set(POS.RESTING)
        elseif pos == POS.SITTING then
            ch:send_line("You should get up first.")
        elseif pos == POS.RESTING then
            ch:send_line("You are already resting.")
        elseif pos == POS.SLEEPING then
            ch:send_line("You have to wake up first.")
        elseif pos == POS.FIGHTING then
            ch:send_line("Rest while fighting?  Are you MAD?")
        else
            ch:send_line("You stop floating around, and stop to rest your tired bones.")
            ch:act_around("$n stops floating around, and rests.")
            ch:position_set(POS.RESTING)
        end
    end
end

return { id = "rest", aliases = { { "rest", 3 } }, execute = execute }
