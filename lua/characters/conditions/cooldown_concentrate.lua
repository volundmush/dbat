return {
    id         = "cooldown_concentrate",
    name       = "Concentrate Cooldown",
    tags       = { "cooldown", "cooldown_concentrate" },
    persistent = false,

    on_remove = function(ch, cond, reason)
        if reason == "expired" then
            ch:send_line("You can concentrate again.")
        end
    end,
}
