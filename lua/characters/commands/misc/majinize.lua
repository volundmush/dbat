local dbat = require("dbat")
local PLR  = dbat.consts.player_flags

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or "")

    if ch:race_get() ~= "majin" then
        ch:send_line("You are not a majin and can not majinize anyone.")
        return
    end
    if arg == "" then
        ch:send_line("Who do you want to majinize?")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("That target isn't here.")
        return
    end
    if vict:is_npc() then
        ch:send_line("Why would you waste your time majinizing them?")
        return
    end
    if vict == ch then
        ch:send_line("You can't majinize yourself.")
        return
    end
    if vict:player_flagged(PLR.PR) then
        ch:send_line("You can't majinize them their potential has been released!")
        return
    end

    local already_lord = vict:condition_has("majinized")
                         and vict:condition_number_get("majinized", "lord") == ch:player_id_get()

    if vict:condition_has("majinized") and not already_lord then
        ch:send_line("They are already majinized before by someone else.")
        return
    end
    if vict:following_get() ~= ch then
        ch:send_line("They must be following you in order for you to majinize them.")
        return
    end

    local align_diff = math.abs(ch:stat_get("alignment") - vict:stat_get("alignment"))
    if align_diff > 1500 then
        ch:send_line("Their alignment is so opposed to your's that they resist your attempts to enslave them!")
        return
    end
    if vict:meter_max("powerlevel") > ch:meter_max("powerlevel") * 4 then
        ch:send_line("Their powerlevel is so much higher than yours they resist your attempts to enslave them!")
        return
    end

    if already_lord then
        ch:reveal_hiding(0)
        ch:act("You remove $N's majinization, freeing them from your influence, but also weakening them.", true, nil, vict, "char")
        ch:act("$n removes your majinization, freeing you from their influence, and weakening you!", true, nil, vict, "vict")
        ch:act("$n waves a hand at $N, and instantly the glowing M on $S forehead disappears!", true, nil, vict, "notvict")
        vict:condition_remove("majinized", "majinize_free")
        ch:stat_mod("boosts", 1)
        return
    end

    if ch:stat_get("boosts") == 0 then
        local level = ch:stat_get("level")
        ch:send_line("You are incapable of majinizing%s.", level < 100 and " right now" or " anymore")
        if level < 25 then
            ch:send_line("Your next available majinize will be at level 25")
        elseif level < 50 then
            ch:send_line("Your next available majinize will be at level 50")
        elseif level < 75 then
            ch:send_line("Your next available majinize will be at level 75")
        elseif level < 100 then
            ch:send_line("Your next available majinize will be at level 100")
        end
        return
    end

    ch:reveal_hiding(0)
    ch:act("You focus your power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", true, nil, vict, "char")
    ch:act("$n focuses power into you, influencing your mind and increasing your strength! After the struggle in your mind ends a glowing purple M forms on your forehead.", true, nil, vict, "vict")
    ch:act("$n focuses power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", true, nil, vict, "notvict")
    vict:condition_add("majinized", "skill", "majinize")
    vict:condition_set_number("majinized", "lord", ch:player_id_get())
    vict:condition_set_number("majinized", "bonus", math.floor(vict:stat_get("powerlevel") * 0.4))
    ch:stat_mod("boosts", -1)
end

return {
    id      = "majinize",
    aliases = { {"majinize", 6} },
    execute = execute,
}
