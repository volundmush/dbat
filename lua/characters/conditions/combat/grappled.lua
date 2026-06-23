local dbat     = require("dbat")
local commands = require("lua.characters.commands")

local INTERVAL_MS = 5000

return {
    id         = "grappled",
    name       = "Grappled",
    tags       = { "grappled" },
    persistent = false,
    on_apply = function(ch, cond)
        if ch:is_npc() then
            cond:schedule_event("tick", INTERVAL_MS, INTERVAL_MS)
        end
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
    end,
    on_event = function(ch, cond, event)
        if event ~= "tick" then return end
        if not ch:mob_flagged(dbat.consts.mob_flags.DUMMY) and math.random(1, 5) >= 4 then
            ch:execute_command("escape", commands)
        end
    end,
}
