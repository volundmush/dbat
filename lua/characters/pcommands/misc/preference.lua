local dbat = require("dbat")
local FP    = dbat.consts.fight_prefs

local SYNTAX = "Syntax: preference (throw | weapon | hand | ki)"

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if arg == "" then
        ch:send_line(SYNTAX)
        return
    end

    if ch:preference_get() > 0 then
        ch:send_line("You've already chosen a specialization. No going back.")
        return
    end

    if arg == "throw" then
        ch:send_line("You will now favor throwing weapons as fighting specialization. You're sure to nail it.")
        ch:preference_set(FP.THROWING)
        local base = ch:skill_base_get("throw")
        if base <= 90 then
            ch:skill_base_set("throw", base + 10)
        elseif base < 100 then
            ch:skill_base_set("throw", 100)
        end
    elseif arg == "hand" then
        ch:send_line("You will now favor your body as your fighting specialization. Your body is ready.")
        ch:preference_set(FP.H2H)
    elseif arg == "ki" then
        ch:send_line("You will now favor your ki energy as your fighting specialization. I expect more than a few smoldering craters.")
        ch:preference_set(FP.KI)
    elseif arg == "weapon" then
        ch:send_line("You will now favor your weapons as your fighting specialization. Let the blood fly!")
        ch:preference_set(FP.WEAPON)
    else
        ch:send_line(SYNTAX)
    end
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return true
end

return { id = "preference", aliases = { { "preference", 4 } }, execute = execute, can_execute = can_execute }
