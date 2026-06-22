return {
    id       = "cancel",
    priority = 1000,
    aliases  = {{"--", 2}},
    execute  = function(ctx)
        ctx.ch:command_queue_clear()
        ctx.ch:send_line("Command queue cleared.")
    end,
}
