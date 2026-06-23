return {
    id = "rune_purisaz",
    name = "Purisaz Rune",
    tags = { "rune_purisaz" },
    persistent = true,

    -- Amplifies ki-element outgoing damage by 30%.
    on_check_attack_offense = function(ch, cond, ctx)
        local ki_dmg = ctx.damage_by_element and ctx.damage_by_element.ki
        if not ki_dmg or ki_dmg <= 0 then return end
        ctx.damage = math.floor(ctx.damage * 1.3)
        ctx.damage_by_element.ki = math.floor(ki_dmg * 1.3)
        local room = ch:room_get()
        if room then
            for person in room:people() do
                person:send_line("@wThere is a bright flash of @Yyellow@w light in the wake of the attack!@n")
            end
        end
    end,
}
