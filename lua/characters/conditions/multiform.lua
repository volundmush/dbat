local dbat = require("dbat")

local ASSIST_INTERVAL_MS = 10000
local POS = dbat.consts.positions

local function schedule_assist(ch, cond)
    if not ch:is_npc() then return end
    if not cond:event_pending("assist") then
        cond:schedule_event("assist", ASSIST_INTERVAL_MS, ASSIST_INTERVAL_MS)
    end
end

local function original_for(ch, cond)
    local id = cond:number_get("original_id")
    if id <= 0 then
        id = ch:condition_number_get("multiform_clone", "target_id")
    end
    return id > 0 and dbat.characters.by_id(id) or nil
end

local function assist_original(ch, cond)
    if not ch:is_npc() then return end
    if ch:position_get() <= POS.SLEEPING then return end

    local original = original_for(ch, cond)
    if not original or math.random(1, 5) < 4 then return end

    local target = original:fighting_get()
    if not target or ch:fighting_get() then return end

    local command
    if math.random(1, 5) >= 4 then
        command = "kick"
    elseif math.random(1, 5) >= 4 then
        command = "elbow"
    else
        command = "punch"
    end

    ch:launch_attack(command, target)
end

return {
    id = "multiform",
    name = "Multiform Clone",
    tags = { "multiform", "multiform_clone" },
    persistent = false,

    on_apply = function(ch, cond)
        schedule_assist(ch, cond)
    end,

    on_game_activate = function(ch, cond)
        schedule_assist(ch, cond)
    end,

    on_remove = function(_, cond)
        cond:cancel_event("assist")
    end,

    on_event = function(ch, cond, event)
        if event == "assist" then assist_original(ch, cond) end
    end,
}
