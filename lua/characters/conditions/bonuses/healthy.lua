local dbat = require("dbat")
local POS  = dbat.consts.positions

local function on_tick(ch, cond)
    if ch:position_get() ~= POS.SLEEPING then return end
    if math.random(1, 100) < 70 then return end

    local change = false

    if ch:condition_has("shocked") then
        ch:condition_remove("shocked", "healthy")
        change = true
    end

    if ch:condition_has("mental_break") then
        ch:condition_remove("mental_break", "healthy")
        change = true
    end

    if ch:condition_remove_tag("healthy_clear", "healthy") > 0 then
        change = true
    end

    if ch:condition_has("knocked_out") then
        ch:condition_remove("knocked_out", "healthy")
        ch:position_set(POS.SITTING)
        change = true
    end

    if change then
        ch:send_line("@CYou feel your body recover from all its ailments!@n")
    end
end

return {
    id         = "bonus_healthy",
    name       = "Healthy",
    tags       = { "bonus" },
    persistent = false,
    description = "Healthy - 30% chance per regen tick to recover from ill effects when sleeping",

    on_apply = function(ch, cond)
        cond:schedule_event("tick", 2000, 2000)
    end,

    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", 2000, 2000)
        end
    end,

    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
    end,

    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,
}
