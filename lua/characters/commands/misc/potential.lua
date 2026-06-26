local dbat = require("dbat")
local PLR  = dbat.consts.player_flags

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if not ch:know_skill("potential") then return end

    if arg == "" then
        ch:send_line("Who's potential do you want to release?")
        ch:send_line("Potential Releases: %d", ch:stat_get("boosts"))
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("That target isn't here.")
        return
    end
    if vict:is_npc() then
        ch:send_line("Why would you waste your time releasing their potential?")
        return
    end
    if vict == ch then
        ch:send_line("You can't release your own potential.")
        return
    end
    if ch:stat_get("boosts") == 0 then
        ch:send_line("You have no potential releases to perform.")
        return
    end
    if vict:player_flagged(PLR.PR) then
        ch:send_line("Their potential has already been released")
        return
    end
    if vict:race_get() == "android" then
        ch:send_line("They are a machine and have no potential to release.")
        return
    end
    if vict:condition_has("majinized") then
        ch:send_line("They are already majinized and have no potential to release.")
        return
    end
    if vict:race_get() == "majin" then
        ch:send_line("They have no potential to release...")
        return
    end

    local boost = math.floor(ch:skill_get("potential") / 2)
    vict:player_flag_set(PLR.PR, true)
    vict:stat_mod("powerlevel", math.floor(vict:stat_get("powerlevel") / 100 * boost))
    if vict:race_get() == "halfbreed" then
        vict:stat_mod("ki",      math.floor(vict:stat_get("ki")      / 100 * boost))
        vict:stat_mod("stamina", math.floor(vict:stat_get("stamina") / 100 * boost))
    end
    ch:reveal_hiding(0)
    ch:act("You place your hand on top of $N's head. After a moment of concentrating you release their hidden potential.", true, nil, vict, "char")
    ch:act("$n places $s hand on top of your head. After a moment you feel a rush of power as your hidden potential is released!", true, nil, vict, "vict")
    ch:act("$n places $s hand on $N's head. After a moment a rush of power explodes off of $N's body!", true, nil, vict, "notvict")
    ch:improve_skill("potential", 0)
    ch:improve_skill("potential", 0)
    ch:improve_skill("potential", 0)
    ch:improve_skill("potential", 0)
    ch:stat_mod("boosts", -1)
end

return {
    id      = "potential",
    aliases = { {"potential", 5} },
    execute = execute,
}
