local dbat  = require("dbat")
local FP    = dbat.consts.fight_prefs

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then return end

    if ch:preference_get() ~= FP.THROWING then
        ch:send_line("You aren't dedicated to throwing!")
        return
    end

    local sk = ch:skill_get("energize") or 0
    if sk == 0 then
        if (ch:skill_get("focus") or 0) >= 30 then
            ch:skill_set("energize", math.random(10, 14))
            ch:send_line("You learn the basics for energizing thrown weapons! Now use the energize command again.")
        else
            ch:send_line("You need a Focus skill level of 30 to figure out the basics of this technique.")
        end
        return
    end

    if ch:condition_has("energize") then
        ch:send_line("You stop focusing ki into your fingertips.")
        ch:condition_remove("energize", "toggle_off")
    else
        ch:send_line("You start focusing your latent ki into your fingertips.")
        ch:condition_apply("energize", "command", "energize")
    end
end

return {
    id      = "energize",
    aliases = { { "energize", 7 } },
    execute = execute,
}
