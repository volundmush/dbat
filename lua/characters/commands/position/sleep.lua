local dbat   = require("dbat")
local PLR    = dbat.consts.player_flags
local PRF    = dbat.consts.pref_flags
local BONUS  = dbat.consts.bonus_flags
local POS    = dbat.consts.positions
local ITYPE  = dbat.consts.item_types
local SECT   = dbat.consts.sector_types
local Search = dbat.lib.search
local act    = dbat.lib.act

local function stop_dragging(ch)
    local dragged = ch:dragging_get()
    if not dragged then return end
    act.to_char(ch, "@WYou stop dragging @C$N@W!@n", { actor = ch, target = dragged })
    act.around(ch, "@C$n@W stops dragging @c$N@W!@n", { actor = ch, target = dragged })
    dragged:being_dragged_set(nil)
    ch:dragging_set(nil)
end

local function lose_fury(ch)
    if not ch:is_npc() and ch:player_flagged(PLR.FURY) then
        ch:send_line("Your fury subsides for now. Next time try to take advantage of it before you calm down.")
        ch:player_flag_set(PLR.FURY, false)
    end
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if not ch:is_npc() then
        if ch:pref_flagged(PRF.ARENAWATCH) then
            ch:pref_flag_set(PRF.ARENAWATCH, false)
            ch:arena_idnum_set(-1)
            ch:send_line("You stop watching the arena action.")
        end
    end

    if ch:bonus_flagged(BONUS.INSOMNIAC) then
        ch:send_line("You don't feel the least bit tired.")
        return
    end

    local room = ch:room_get()
    if room:sector_type_get() == SECT.WATER_NOSWIM then
        ch:send_line("You can't rest here!")
        return
    end
    if ch:player_flagged(PLR.PILOTING) then
        ch:send_line("You are busy piloting a ship!")
        return
    end
    if ch:condition_has("fighting") then
        ch:send_line("You are a bit busy at the moment!")
        return
    end
    if ch:player_flagged(PLR.HEALT) then
        ch:send_line("You are inside a healing tank!")
        return
    end
    if ch:player_flagged(PLR.POWERUP) then
        ch:send_line("You are busy powering up!")
        return
    end
    if ch:condition_has("barrier") then
        if ch:know_skill("barrier") then
            ch:send_line("You have a barrier around you and can't sleep.")
            return
        else
            ch:condition_remove("barrier", "sleep")
        end
    end
    if ch:stat_get("kaioken") > 0 then
        ch:send_line("You are utilizing kaioken and can't sleep!")
        return
    end
    if ch:sleeptime_get() > 0 then
        ch:send_line("You aren't sleepy enough.")
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
            act.to_char(ch, "You can't sleep on $p.", { actor = ch, tool = chair })
            return
        end
        if pos == POS.STANDING or pos == POS.SITTING or pos == POS.RESTING then
            ch:reveal_hiding(0)
            ch:send_line("You go to sleep.")
            ch:act_around("$n lies down and falls asleep.")
            lose_fury(ch)
            ch:position_set(POS.SLEEPING)
        elseif pos == POS.SLEEPING then
            ch:send_line("You are already sound asleep.")
        elseif pos == POS.FIGHTING then
            ch:send_line("Sleep while fighting?  Are you MAD?")
        else
            ch:send_line("You stop floating around, and lie down to sleep.")
            ch:act_around("$n stops floating around, and lie down to sleep.")
            ch:position_set(POS.SLEEPING)
        end
    else
        if ch:sits_get() then
            ch:send_line("You are already on something!")
            return
        end
        local chair = Search(ch):add_room_objects(room):add_filter(function(s, e)
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
        if chair:sitting_get() then
            ch:send_line("Someone is already on that one!")
            return
        end
        if chair:type_get() ~= ITYPE.BED then
            ch:send_line("You can't sleep on that!")
            return
        end
        if chair:size_get() + 1 < ch:size_get() then
            ch:send_line("You are too large for it!")
            return
        end
        if pos == POS.RESTING or pos == POS.SITTING then
            ch:send_line("You need to get up first!")
        elseif pos == POS.STANDING then
            ch:reveal_hiding(0)
            act.to_char(ch, "You lay down on $p and sleep.", { actor = ch, tool = chair })
            act.around(ch, "$n lays down on $p and sleeps.", { actor = ch, tool = chair })
            lose_fury(ch)
            ch:sits_set(chair)
            chair:sitting_set(ch)
            ch:position_set(POS.SLEEPING)
        elseif pos == POS.SLEEPING then
            ch:send_line("You are already sound asleep.")
        elseif pos == POS.FIGHTING then
            ch:send_line("Sleep while fighting?  Are you MAD?")
        else
            ch:send_line("You stop floating around, and lie down to sleep.")
            ch:act_around("$n stops floating around, and lie down to sleep.")
            ch:position_set(POS.SLEEPING)
        end
    end
end

return { id = "sleep", aliases = { { "sleep", 2 } }, execute = execute }
