local dbat = require("dbat")

local function on_tick(ch, cond)
    local gain = ch:der_total("ki_regen")
    local conc = ch:skill_get("concentration")
    if conc > 0 then gain = gain + math.floor(gain * 0.005 * conc) end
    ch:meter_mod_int("ki", gain)
end

return {
    id         = "ki_regen",
    name       = "Ki Regen",
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
