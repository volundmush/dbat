local dbat = require("dbat")

local M = {}

-- Convenience builder for non-attack damage sources.
-- element: "physical" | "ki" | "fire" | nil (generic, no element-specific hooks)
function M.build_ctx(source, target, amount, element)
    local by_el = {}
    if element then by_el[element] = amount end
    return {
        source            = source,
        target            = target,
        damage            = amount,
        damage_by_element = by_el,
        spar              = false,
        hooks_applied     = false,
    }
end

local function dispatch_on_kill(source, ctx)
    for _, cond_id in ipairs(source:conditions()) do
        local def = dbat.get("conditions", cond_id)
        if def and def.on_kill then
            def.on_kill(source, source:condition(cond_id), ctx)
        end
    end
    local race_id = source:race_get()
    if race_id then
        local race_def = dbat.get("races", race_id)
        if race_def and race_def.on_kill then
            race_def.on_kill(source, ctx)
        end
    end
    local sensei_id = source:sensei_get()
    if sensei_id then
        local sensei_def = dbat.get("senseis", sensei_id)
        if sensei_def and sensei_def.on_kill then
            sensei_def.on_kill(source, ctx)
        end
    end
end

-- Inflict damage onto ctx.target.
--
-- ctx fields (attack pipeline inst is a valid superset of this schema):
--   source (Character|nil)      — dealer; nil for environmental damage
--   target (Character)          — recipient
--   damage (int)                — total damage; hooks may mutate this
--   damage_by_element (table)   — { physical=N, ki=N, fire=N, … } breakdown
--   spar (bool)                 — if true, clamp powerlevel at 1 instead of killing
--   hooks_applied (bool)        — if true, skip offense/defense hook dispatch
--                                 (set by attack pipeline since hooks ran earlier)
--
-- When hooks_applied is false (non-attack sources), the pipeline:
--   1. Applies the 60% ki-baseline reduction to the ki element
--   2. Dispatches on_check_attack_offense to source's conditions/race/sensei
--   3. Dispatches on_check_attack_defense to target's conditions/race/sensei
--   4. Aborts if ctx.dodged or ctx.absorbed are set
--
-- After hooks: applies ctx.damage to the powerlevel meter. On death:
--   dispatches on_kill to source's conditions then calls target:die(source).
function M.inflict(ctx)
    local target = ctx.target
    -- attack pipeline uses ctx.attacker; non-attack code uses ctx.source
    local source = ctx.source or ctx.attacker

    if not ctx.hooks_applied then
        -- Ki-element damage is 60% as effective as physical by default.
        local by_el = ctx.damage_by_element
        local ki_dmg = by_el and by_el.ki
        if ki_dmg and ki_dmg > 0 then
            local reduced = math.floor(ki_dmg * 0.6)
            by_el.ki = reduced
            ctx.damage = math.max(0, ctx.damage - (ki_dmg - reduced))
        end

        if source then source:check_attack_offense(ctx) end
        target:check_attack_defense(ctx)

        if ctx.dodged or ctx.absorbed then return end
    end

    if ctx.damage <= 0 then return end

    local spar = ctx.spar
    target:meter_mod_int("powerlevel", -ctx.damage)

    local remaining = target:meter_current("powerlevel")
    if remaining <= 0 then
        if spar then
            target:send_line("You are too exhausted to continue sparring!")
            target:meter_set_int("powerlevel", 1)
        else
            ctx.killed = true
            if source then dispatch_on_kill(source, ctx) end
            target:die(source)
        end
    end
end

return M
