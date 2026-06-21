local dbat = require("dbat")
local PLR  = dbat.consts.player_flags
local POS  = dbat.consts.positions

local LIMB_LABELS = {"right arm", "left arm", "right leg", "left leg"}

local function on_tick(ch, cond)
    local healrate = 0
    if ch:player_flagged(PLR.BANDAGED) then healrate = healrate + 10 end
    local pos = ch:position_get()
    if pos == POS.SITTING then
        healrate = healrate + 1
    elseif pos == POS.RESTING then
        healrate = healrate + 3
    elseif pos == POS.SLEEPING then
        healrate = healrate + 5
    end
    if healrate == 0 then return end

    local recovered = false
    for i = 1, 4 do
        local v = ch:limbcond_get(i)
        if v > 0 and v < 100 then
            local nv = math.min(100, v + healrate)
            ch:limbcond_set(i, nv)
            if v < 50 and nv >= 50 then
                ch:send_line("You realize your " .. LIMB_LABELS[i] .. " is no longer broken.")
                ch:act_around("$n starts moving $s " .. LIMB_LABELS[i] .. " gingerly for a moment.")
                recovered = true
            elseif nv >= 100 then
                ch:send_line("Your " .. LIMB_LABELS[i] .. " has fully recovered.")
            else
                ch:send_line(string.format(
                    "Your %s feels a little better @D[@G%d%%@D/@g100%%@D]@n.",
                    LIMB_LABELS[i], nv))
            end
        end
    end

    if recovered then
        if ch:player_flagged(PLR.BANDAGED) then
            ch:player_flag_set(PLR.BANDAGED, false)
            ch:send_line("You remove your bandages.")
        else
            local con = ch:stat_get("constitution")
            if dbat.axion_dice(-10) > con then
                for _, s in ipairs({"strength", "agility", "speed"}) do
                    ch:stat_mod(s, -1)
                    if ch:stat_get(s) < 4 then ch:stat_set(s, 4) end
                end
                ch:send_line("@RYou lose 1 Strength, Agility, and Speed!@n")
            end
        end
    end
end

return {
    id = "limb_healing",
    name = "Limb Healing",
    persistent = false,
    on_apply = function(ch, cond)
        cond:schedule_event("tick", 100000, 100000)
    end,
    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", 100000, 100000)
        end
    end,
    on_remove = function(ch, cond)
        cond:cancel_event("tick")
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,
}
