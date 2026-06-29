local dbat = require("dbat")

return {
    id         = "cooldown_radar_ping",
    name       = "Radar Ping Cooldown",
    tags       = { "cooldown", "cooldown_radar_ping" },
    persistent = false,

    on_remove = function(ch, cond, reason)
        if reason == "expired" and ch:player_flagged(dbat.consts.player_flags.PILOTING) then
            ch:send_line("Your radar is ready to calculate the direction of another destination.")
        end
    end,
}
