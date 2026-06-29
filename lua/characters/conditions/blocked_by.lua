local dbat = require("dbat")

local function blocker(cond)
    local id = cond:number_get("target_id") or 0
    return id > 0 and dbat.characters.by_id(id) or nil
end

local function stale_check(ch, cond)
    local vict = blocker(cond)
    if not vict or ch:room_get() ~= vict:room_get() then
        if vict then vict:blocking_set(nil) end
        ch:blocked_by_set(nil)
    end
end

return {
    id = "blocked_by",
    name = "Blocked By",
    tags = { "blocked_by" },
    persistent = false,

    on_apply = function(ch, cond)
        cond:schedule_event("stale_check", 2000, 2000)
    end,

    on_game_activate = function(ch, cond)
        if not cond:event_pending("stale_check") then
            cond:schedule_event("stale_check", 2000, 2000)
        end
    end,

    on_remove = function(ch, cond)
        cond:cancel_event("stale_check")
    end,

    on_event = function(ch, cond, event)
        if event == "stale_check" then stale_check(ch, cond) end
    end,
}
