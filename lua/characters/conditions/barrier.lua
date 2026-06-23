local text = dbat.lib.text

local function shed_rate(skill)
    if     skill >= 100 then return 0.01
    elseif skill >= 95  then return 0.02
    elseif skill >= 90  then return 0.04
    elseif skill >= 80  then return 0.08
    elseif skill >= 70  then return 0.10
    elseif skill >= 60  then return 0.15
    elseif skill >= 50  then return 0.20
    elseif skill >= 40  then return 0.25
    elseif skill >= 30  then return 0.27
    elseif skill >= 20  then return 0.29
    else                     return 0.30
    end
end

local function on_tick(ch, cond)
    if ch:skill_get("aqua_barrier") > 0 then return end

    local rate    = shed_rate(ch:skill_get("barrier"))
    local current = cond:number_get("amount")
    local loss    = math.floor(current * rate)
    local recharge = 0

    if ch:skill_get("concentration") >= dbat.axion_dice(0) then
        recharge = math.floor(loss * 0.5)
    end

    current = current - loss
    cond:number_set("amount", math.max(current, 0))

    if current <= 0 then
        ch:condition_remove("barrier", "shed")
    else
        ch:send_line("@cYour barrier loses some energy.")
        ch:send_line(string.format("@D[@C%s@D]", text.add_commas(loss)))
        ch:act_around("@c$n@c's barrier sends some sparks into the air as it seems to get a bit weaker.")
        ch:improve_skill("barrier", 0)
    end

    if recharge > 0 and ch:meter_current("ki") < ch:meter_max("ki") then
        ch:meter_mod_int("ki", recharge)
        ch:send_line("@CYou reabsorb some of the energy lost into your body!")
    end
end

return {
    id         = "barrier",
    name       = "Barrier",
    tags       = { "barrier" },
    persistent = false,
    legacy_affects = { dbat.consts.aff_flags.SANCTUARY },
    on_apply = function(ch, cond)
        cond:schedule_event("tick", 100000, 100000)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
        if reason == "shed" then
            ch:send_line("@cYour barrier disappears.")
            ch:act_around("@c$n@c's barrier disappears.")
        elseif reason ~= "burst" and reason ~= "released" and reason ~= "tank" then
            cond:number_set("amount", 0)
        end
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,
}
