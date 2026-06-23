local act = dbat.lib.act

local function on_tick(ch, cond)
    if ch:is_fighting() then return end
    if math.random(1, 101) > ch:skill_get("fireshield") then
        ch:condition_remove("fireshield", "expired")
    end
end

local function on_check_attack_defense(ch, cond, ctx)
    if ctx.absorbed or ctx.damage <= 0 then return end
    if math.random(1, 200) >= ch:skill_get("fireshield") then return end

    local attacker = ctx.attacker or ctx.source
    if attacker then
        act.message({
            actor  = "@c$N's@C fireshield repels the damage!@n",
            target = "@CYour fireshield repels the damage!@n",
            room   = "@c$N's@C fireshield repels the damage!@n",
        }, { actor = attacker, target = ch })
    else
        ch:send_line("@CYour fireshield repels the damage!@n")
    end

    if math.random(1, 3) == 3 then
        if attacker then
            act.message({
                actor  = "@c$N's@C fireshield disappears...@n",
                target = "@CYour fireshield disappears...@n",
                room   = "@c$N's@C fireshield disappears...@n",
            }, { actor = attacker, target = ch })
        else
            ch:send_line("@CYour fireshield disappears...@n")
        end
        ch:condition_remove("fireshield", "blocked")
    end

    ctx.absorbed = true
end

return {
    id         = "fireshield",
    name       = "Fireshield",
    tags       = { "fireshield" },
    persistent = false,
    legacy_affects = { dbat.consts.aff_flags.FIRESHIELD },
    on_check_attack_defense = on_check_attack_defense,
    on_apply = function(ch, cond)
        cond:schedule_event("tick", 100000, 100000)
    end,
    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", 100000, 100000)
        end
    end,
    on_remove = function(ch, cond)
        cond:cancel_event("tick")
        ch:send_line("Your fireshield disappears.")
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,
}
