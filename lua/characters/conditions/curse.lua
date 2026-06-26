local function on_tick(ch, cond)
    local pl_max  = ch:meter_max("powerlevel")
    local floor   = math.floor(pl_max * 0.4)
    local pl_cur  = ch:meter_current("powerlevel")
    if pl_cur <= floor then return end

    local drain = math.floor(pl_max * 0.01)
    ch:meter_mod("powerlevel", -drain)

    -- If a demon in the room isn't at full PL, absorb the drained energy
    -- NOTE: original C++ had a bug here (incCurLF(ch) instead of incCurLF(tch));
    -- porting faithfully — the cursed person gets refilled, not the demon
    for person in ch:room_get():people() do
        if person:race_get() == "demon"
            and person:meter_current("powerlevel") < person:meter_max("powerlevel")
        then
            ch:meter_mod("powerlevel", drain)
            person:act(
                "@CYou feel the life energy from @c$N@C's cursed body flow out and you draw it into yourself!@n",
                true, nil, ch, "char")
        end
    end

    if ch:meter_current("powerlevel") < floor then
        ch:meter_set("powerlevel", floor)
    end
end

return {
    id = "curse",
    name = "Curse",
    tags = { "curse", "affliction", "healthy_clear" },
    persistent = false,
    modifiers = function()
        return {
            { target = { "regen", "vitals" }, kind = "multiplier", value = -8000, label = "Curse" },
        }
    end,
    on_apply = function(ch, cond)
        cond:schedule_event("tick", 2000, 2000)
    end,
    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", 2000, 2000)
        end
    end,
    on_remove = function(ch, cond)
        cond:cancel_event("tick")
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,
}
