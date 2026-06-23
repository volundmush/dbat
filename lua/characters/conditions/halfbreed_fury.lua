local INTERVAL_MS = 5000

return {
    id          = "halfbreed_fury",
    name        = "Halfbreed Fury",
    persistent  = false,
    on_apply = function(ch, cond)
        ch:stat_set("rage_meter", 0)
        cond:schedule_event("tick", INTERVAL_MS, INTERVAL_MS)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
        ch:stat_set("rage_meter", 0)
    end,
    on_event = function(ch, cond, event)
        if event ~= "tick" then return end
        ch:stat_mod("rage_meter", 1)
        if ch:stat_get("rage_meter") >= 1000 then
            ch:meter_mod_int("powerlevel", math.floor(ch:meter_max("powerlevel") * 0.15))
            ch:meter_mod_int("ki",         math.floor(ch:meter_max("ki")         * 0.15))
            ch:meter_mod_int("stamina",    math.floor(ch:meter_max("stamina")    * 0.15))
            ch:send_line("Your fury has called forth more of your hidden power and you feel better!")
        end
    end,
}
