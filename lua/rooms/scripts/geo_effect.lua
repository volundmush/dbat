local TICK_INTERVAL_MS = 4000

local STAGE_MESSAGES = {
    "@RLava spews up violently from the cracks in the ground!@n",
    "@RThe lava bubbles and gives off tremendous heat!@n",
    "@RNoxious fumes rise from the bubbling lava!@n",
    "@RSome of the lava cools as it spreads further from the source!@n",
}

local function ensure_tick(script)
    if not script:event_pending("tick") then
        script:schedule_event("tick", TICK_INTERVAL_MS, TICK_INTERVAL_MS)
    end
end

local function tick(room, script)
    local geffect = room:geffect_get()
    if geffect < 1 then
        room:script_remove("geo_effect", "inactive")
        return
    end

    if geffect >= 6 then
        room:script_remove("geo_effect", "complete")
        return
    end

    if math.random(1, 100) < 96 then return end

    if geffect <= 4 then
        room:send_line(STAGE_MESSAGES[math.random(1, #STAGE_MESSAGES)])
    elseif geffect == 5 then
        room:send_line("@RLava covers the entire area now!@n")
    end

    room:geffect_set(geffect + 1)
end

return {
    id = "geo_effect",
    name = "Geo Effect",

    on_apply = function(room, script)
        ensure_tick(script)
    end,

    on_game_activate = function(room, script)
        ensure_tick(script)
    end,

    on_remove = function(room, script, reason)
        script:cancel_event("tick")
    end,

    on_event = function(room, script, event)
        if event == "tick" then tick(room, script) end
    end,
}
