local dbat   = require("dbat")
local BONUS  = dbat.consts.bonus_flags
local POS    = dbat.consts.positions
local Search = dbat.lib.search
local act    = dbat.lib.act

local function stop_drag_for(dragger, vict)
    act.to_char(dragger, "@WYou stop dragging @C$N@W!@n", { actor = dragger, target = vict })
    act.around(dragger, "@C$n@W stops dragging @c$N@W!@n", { actor = dragger, target = vict })
    vict:being_dragged_set(nil)
    dragger:dragging_set(nil)
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:condition_has("knocked_out") then
        ch:send_line("You are knocked out cold for right now!")
        return
    end
    if ch:bonus_flagged(BONUS.LATE) and ch:position_get() == POS.SLEEPING then
        ch:send_line("Nah you're enjoying sleeping too much.")
        return
    end

    local handled_self = false

    if arg ~= "" then
        if ch:position_get() == POS.SLEEPING then
            ch:send_line("Maybe you should wake yourself up first.")
            return
        end
        local room = ch:room_get()
        local vict = Search(ch):add_room_people(room):add_filter(function(s, e)
            return s:can_see(e)
        end):find_one(arg)
        if not vict then
            ch:send_line("There is no one by that name here.")
            return
        end
        if vict:id_get() == ch:id_get() then
            handled_self = true
        elseif vict:position_get() > POS.SLEEPING then
            act.to_char(ch, "$E is already awake.", { actor = ch, target = vict })
            return
        elseif vict:condition_has("yoikominminken") then
            act.to_char(ch, "You can't wake $M up!", { actor = ch, target = vict })
            return
        elseif vict:position_get() < POS.SLEEPING then
            act.to_char(ch, "$E's in pretty bad shape!", { actor = ch, target = vict })
            return
        elseif vict:condition_has("knocked_out") then
            ch:send_line("They are knocked out cold for right now!")
            return
        elseif ch:bonus_flagged(BONUS.LATE) then
            ch:send_line("They say 'Yeah yeah...' and then roll back over.")
            return
        else
            act.to_char(ch, "You wake $M up.", { actor = ch, target = vict })
            act.to_char(vict, "You are awakened by $n.", { actor = ch, target = vict })
            vict:position_set(POS.SITTING)
            local dragger = vict:being_dragged_get()
            if dragger then stop_drag_for(dragger, vict) end
            local carrier = vict:carried_by_char_get()
            if carrier then
                carrier:carry_drop(carrier:stat_get("alignment") > 50 and 0 or 1)
            end
            return
        end
    end

    if not handled_self then
        if ch:condition_has("yoikominminken") then
            ch:send_line("You can't wake up!")
            return
        elseif ch:position_get() > POS.SLEEPING then
            ch:send_line("You are already awake...")
            return
        end
    end

    ch:send_line("You awaken, and sit up.")
    ch:act_around("$n awakens.")
    local dragger = ch:being_dragged_get()
    if dragger then stop_drag_for(dragger, ch) end
    local carrier = ch:carried_by_char_get()
    if carrier then
        carrier:carry_drop(carrier:stat_get("alignment") > 50 and 0 or 1)
    end
    ch:position_set(POS.SITTING)
end

return { id = "wake", aliases = { { "wake", 2 } }, execute = execute }
