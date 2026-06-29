local dbat = require("dbat")
local T = require("lua.libs.pcommand_toggles")

local PLR = dbat.consts.player_flags

return {
    id = "hide",
    aliases = { { "hide", 4 } },
    can_execute = T.admin_can(T.ADMLVL.NONE),
    execute = function(ctx)
        local ch = ctx.ch
        local charge = ch:charge_get()
        local ki_pref = ch:preference_get() == dbat.consts.fight_prefs.KI
        if (charge > 0 and not ki_pref)
            or (charge > ch:meter_max("ki") * 0.1 and ki_pref)
            or ch:condition_has("powering_up")
            or ch:condition_has("flying")
        then
            ch:send_line("You stand out too much to hide right now!")
            return
        elseif ch:player_flagged(PLR.HEALT) then
            ch:send_line("You are inside a healing tank!")
            return
        end

        if ch:skill_get("hide") <= 0 then
            if ch:slot_count() + 1 <= ch:der_total("skill_slots") then
                ch:send_line("@GYou learn the very minimal basics to hiding.@n")
                ch:skill_base_set("hide", math.random(1, 5))
            else
                ch:send_line("@RYou need more skill slots in order to learn this skill.@n")
                return
            end
        end

        local on = not ch:aff_flagged(T.AFF.HIDE)
        ch:aff_flag_set(T.AFF.HIDE, on)
        ch:send(on and "You will try to stay hidden.\r\n" or "You will no longer attempt to stay hidden.\r\n")
    end,
}
