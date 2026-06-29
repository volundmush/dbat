local TICK_INTERVAL_MS = 1000
local MAX_TICKS        = 60

local function on_tick(ch, cond)
    local ticks = (cond:number_get("ticks") or 0) + 1
    cond:number_set("ticks", ticks)

    if ticks >= MAX_TICKS then
        ch:send_line("@wYour body slowly stops flashing. The energy you built up dissipates harmlessly.@n")
        ch:act_around("@w$n@w's body slowly stops flashing as the built-up energy dissipates harmlessly.@n")
        ch:condition_remove("self_destructing", "timeout")
        return
    end

    if (cond:number_get("phase") or 1) == 1 then
        local sk = ch:skill_get("self destruct") or 0
        if math.random(4, 100) < sk then
            cond:number_set("phase", 2)
            ch:send_line("@RYou feel you are ready to self destruct!@n")
        end
    end
end

return {
    id         = "self_destructing",
    name       = "Self Destructing",
    tags       = { "selfdestruct", "self_destructing" },
    persistent = false,

    on_apply = function(ch, cond)
        cond:number_set("phase", 1)
        cond:number_set("ticks", 0)
        cond:schedule_event("tick", TICK_INTERVAL_MS, TICK_INTERVAL_MS)
    end,

    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", TICK_INTERVAL_MS, TICK_INTERVAL_MS)
        end
    end,

    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
    end,

    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,
}
