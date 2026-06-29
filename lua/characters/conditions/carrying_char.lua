local function target(ch, cond)
    local id = cond:number_get("target_id") or 0
    return id > 0 and require("dbat").characters.by_id(id) or nil
end

local function stale_check(ch, cond)
    local vict = target(ch, cond)
    if not vict or ch:room_get() ~= vict:room_get() then
        ch:carry_drop(3)
    end
end

return {
    id = "carrying_char",
    name = "Carrying",
    tags = { "carrying" },
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
