local function modifiers(ch, cond)
    local mods = {}

    mods[#mods + 1] = { target = { "derived", "strength" }, kind = "flat", value = -3, label = "Wither" }
    mods[#mods + 1] = { target = { "derived", "speed" }, kind = "flat", value = -3, label = "Wither" }

    return mods
end

return {
    id         = "wither",
    name       = "Wither",
    tags       = { "wither", "healthy_clear" },
    persistent = true,
    modifiers  = modifiers,

    on_check_attack_defense = function(ch, cond, ctx)
        if ctx.absorbed or ctx.damage <= 0 then return end
        ctx.damage = math.floor(ctx.damage * 1.2)
    end,
    status_line = function(ch, cond) return "You've been withered! You feel so weak..." end,
    on_apply = function(ch, cond)
        cond:schedule_expire(800)
    end,
    on_game_activate = function(ch, cond)
        if not cond:event_pending("expire") then
            cond:schedule_expire(800)
        end
    end,
    on_remove = function(ch, cond, reason)
        ch:send_line("@wYour body returns to normal and you beat the withering that plagued you.")
        ch:act_around("@W$n@W's looks more fit now.")
    end,
}
