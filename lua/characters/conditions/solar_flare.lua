local INTERVAL_MS = 5000

return {
    id             = "solar_flare",
    name           = "Solar Flare",
    tags           = { "solar", "healthy_clear", "blind_aff", "remove_on_death" },
    persistent     = true,
    legacy_affects = { dbat.consts.aff_flags.BLIND },
    on_apply = function(ch, cond)
        if ch:is_npc() then
            cond:schedule_event("recovery_check", INTERVAL_MS, INTERVAL_MS)
        end
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("recovery_check")
    end,
    on_event = function(ch, cond, event)
        if event == "recovery_check" and math.random(1, 200) >= 190 then
            ch:act_around("@W$n@W is no longer blind.@n")
            ch:condition_remove("solar_flare", "recovered")
        end
    end,
}
