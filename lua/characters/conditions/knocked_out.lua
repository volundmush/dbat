local INTERVAL_MS = 5000

return {
    id             = "knocked_out",
    name           = "Knocked Out",
    tags           = { "knocked" },
    persistent     = false,
    legacy_affects = { dbat.consts.aff_flags.KNOCKED },
    on_apply = function(ch, cond)
        cond:schedule_event("recovery_check", INTERVAL_MS, INTERVAL_MS)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("recovery_check")
        if reason ~= "recovered_silent" then
            ch:act_around("$n is no longer senseless, and wakes up.")
            ch:send_line("You are no longer knocked out, and wake up!")
        end
        ch:position_set(dbat.consts.positions.SITTING)
    end,
    on_event = function(ch, cond, event)
        if event ~= "recovery_check" then return end
        if math.random(1, 200) >= 195 then
            ch:cure_knocked_out()
            if ch:is_npc() and math.random(1, 20) >= 12 then
                ch:act_around("@W$n@W stands up.@n")
                ch:position_set(dbat.consts.positions.STANDING)
            end
        end
    end,
}
