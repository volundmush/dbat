return {
    id         = "cooldown_selfdestruct",
    name       = "Self Destruct Cooldown",
    tags       = { "cooldown", "cooldown_selfdestruct" },
    persistent = false,

    on_remove = function(ch, cond, reason)
        if reason == "expired" then
            ch:send_line("Your body has recovered from your last selfdestruct.")
        end
    end,
}
