local dbat = require("dbat")

local function furniture(cond)
    local id = cond:number_get("obj_id") or 0
    return id > 0 and dbat.objects.by_id(id) or nil
end

local function stale_check(ch, cond)
    local obj = furniture(cond)
    if not obj or ch:room_get() ~= obj:room_get() then
        if obj then obj:sitting_set(nil) end
        ch:sits_set(nil)
    end
end

return {
    id = "using_furniture",
    name = "Using Furniture",
    tags = { "using_furniture" },
    persistent = false,

    modifiers = function(ch, cond)
        local obj = furniture(cond)
        return obj and obj:modifiers() or {}
    end,

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
        local obj = furniture(cond)
        if obj and obj:sitting_get() == ch:id_get() then obj:sitting_set(nil) end
    end,

    on_event = function(ch, cond, event)
        if event == "stale_check" then stale_check(ch, cond) end
    end,
}
