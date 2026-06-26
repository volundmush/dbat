local dbat = require("dbat")
local BON  = dbat.consts.bonuses

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if ch:is_npc() then return end

    if ch:race_get() == "android" then
        ch:send_line("You are unable to suppress your powerlevel.")
        return
    end
    if ch:bonus_flagged(BON.ARROGANT) then
        ch:send_line("You are far too arrogant to hide your strength.")
        return
    end
    if ch:condition_has("powering_up") then
        ch:send_line("You are currently powering up, can't suppress.")
        return
    end
    if ch:stat_get("kaioken") ~= 0 then
        ch:send_line("You are currently concentrating on kaioken!")
        return
    end

    if arg == "" then
        ch:send_line("Suppress to what percent?\r\nSyntax: suppress (1 - 99 | release)")
        return
    end

    if arg:lower() == "release" then
        if ch:stat_get("suppression") ~= 0 then
            ch:reveal_hiding(0)
            ch:act("@GYou stop suppressing your current powerlevel!@n", true, nil, nil, "char")
            ch:act("@G$n smiles as a rush of power erupts around $s body briefly.@n", true, nil, nil, "room")
            ch:stat_set("suppression", 0)
        else
            ch:send_line("You are not suppressing!")
        end
        return
    end

    local num = math.tointeger(tonumber(arg) or 0) or 0
    if num > 99 or num <= 0 then
        ch:send_line("Out of suppression range.\r\nSyntax: suppress (1 - 99 | release)")
        return
    end

    ch:reveal_hiding(0)
    if ch:stat_get("suppression") ~= 0 then
        ch:act("@GYou alter your suppression level!@n", true, nil, nil, "char")
        ch:act("@G$n seems to concentrate for a moment.@n", true, nil, nil, "room")
    else
        ch:act("@GYou suppress your current powerlevel!@n", true, nil, nil, "char")
        ch:act("@G$n seems to concentrate for a moment.@n", true, nil, nil, "room")
    end
    ch:stat_set("suppression", num)
end

return {
    id      = "suppress",
    aliases = { {"suppress", 7} },
    execute = execute,
}
