local dbat = require("dbat")

local M = {}

-- Tier-based defaults
local base_divisor = { 500, 400, 300, 200, 100 }
local lag_for_tier = { 4, 5, 6, 7, 8 }

local _PLR, _MF, _AFF
local function flags()
    if not _PLR then
        _PLR = dbat.consts.player_flags
        _MF  = dbat.consts.mob_flags
        _AFF = dbat.consts.aff_flags
    end
    return _PLR, _MF, _AFF
end

local function is_sparring(ch)
    local PLR, MF = flags()
    if ch:is_npc() then return ch:mob_flagged(MF.SPAR) end
    return ch:player_flagged(PLR.SPAR)
end

local function roll_accuracy(ch, skill)
    local roll = math.floor(dbat.axion_dice(0) / 2)
    return math.max(0, skill + roll)
end

local function speed_modifier(ch, vict)
    local asp = ch:der_total("speed_index")
    local vsp = vict:der_total("speed_index")
    if vsp <= 0 then return 15 end
    local ratio = asp / vsp
    if     ratio >= 4    then return  15
    elseif ratio >= 2    then return  10
    elseif ratio >  1    then return   5
    elseif ratio <  0.25 then return -15
    elseif ratio <  0.5  then return -10
    elseif ratio <  1    then return  -5
    else return 0 end
end

local function defense_total(vict)
    if vict:condition_has("knocked") then return 0 end
    local p = vict:skill_get("parry")
    local b = vict:skill_get("block")
    local d = vict:skill_get("dodge")
    return math.floor((p + b + d) / 3)
end

local function roll_hitloc(ch, vict, skill)
    local r = math.random(1, 100)
    if r <= 50 then
        return "body"
    elseif r <= 65 then
        return "head"
    elseif r <= 85 then
        if vict:limbcond_get(1) <= 0 and vict:limbcond_get(2) <= 0 then return "body" end
        return "arm"
    else
        if vict:limbcond_get(3) <= 0 and vict:limbcond_get(4) <= 0 then return "body" end
        return "leg"
    end
end

local function calc_crit(ch, loc)
    if loc == "head" then
        return true, 2.0
    elseif loc == "arm" or loc == "leg" then
        return false, 0.5
    else
        return false, 1.0
    end
end

local function base_damage(ch, def, inst)
    local pl    = ch:der_total("powerlevel")
    local tier  = def.tier or 1
    local tier_scale = { 0.0001, 0.00015, 0.0002, 0.0003, 0.0005 }
    local base  = math.floor(pl * (tier_scale[tier] or 0.0001))
    local skill_mult = 1.0 + (inst.skill_level / 100) * 0.5
    base = math.floor(base * skill_mult * (def.base_power or 1.0))
    if def.family == "melee" then
        base = math.floor(base * 0.92)
        if base > ch:meter_max("powerlevel") * 0.10 then
            base = math.floor(base * 0.60)
        end
    end
    return base
end

-- Returns true if inst matches all provided filter criteria.
-- filter keys: family, attack_ids (table set), tier_min, tier_max, elements (table set)
function M.matches_filter(inst, filter)
    if filter.family     and inst.def.family ~= filter.family then return false end
    if filter.attack_ids and not filter.attack_ids[inst.def.id] then return false end
    if filter.tier_min   and inst.def.tier < filter.tier_min then return false end
    if filter.tier_max   and inst.def.tier > filter.tier_max then return false end
    if filter.elements then
        local found = false
        for el in pairs(inst.def.elements or {}) do
            if filter.elements[el] then found = true; break end
        end
        if not found then return false end
    end
    return true
end

local COMBO_SEQUENCE = {
    "punch", "kick", "elbow", "knee",
    "roundhouse", "uppercut", "slam", "heeldrop",
}

-- Returns the next attack_id for a combo chain, or nil if chain is exhausted/unavailable.
-- Checks on_check_combo on the candidate def to validate limb/skill eligibility.
function M.combo_next(ch, inst)
    if not ch:condition_has("combo") then return nil end
    local cond_def = dbat.get("conditions", "combo")
    if not cond_def or not cond_def.next_attack then return nil end
    local cond = ch:condition("combo")
    for _ = 1, #COMBO_SEQUENCE do
        local id = cond_def.next_attack(ch, cond)
        if not id then return nil end
        local next_def = dbat.get("attacks", id)
        if next_def and next_def.in_multihit ~= false then
            local ok = not next_def.on_check_combo or next_def.on_check_combo(ch)
            if ok then return id end
        end
    end
    return nil
end

local function process_defense(inst)
    if inst.dodged or inst.absorbed then
        inst.damage = 0
        inst.damage_by_element = {}
        return
    end
    if inst.partial_negation > 0 then
        local factor = 1.0 - math.min(1.0, inst.partial_negation)
        inst.damage = math.floor(inst.damage * factor)
        for elem, amt in pairs(inst.damage_by_element) do
            inst.damage_by_element[elem] = math.floor(amt * factor)
        end
    end
end

local function apply_backlash(ch, entry, inst)
    if entry.apply then
        entry.apply(ch, inst)
        return
    end
    local dmg = entry.damage or 0
    if dmg > 0 then
        if inst.spar then
            local cur = ch:meter_current("powerlevel")
            if cur > 1 then dmg = math.min(dmg, cur - 1) end
        end
        ch:meter_mod_int("powerlevel", -dmg)
    end
    for _, cond_id in ipairs(entry.conditions or {}) do
        ch:condition_apply(cond_id)
    end
end

local function multihit_check(ch, vict)
    return ch:der_total("speed_index") >= vict:der_total("speed_index") + math.random(1, 15)
end

local function build_instance(ch, def, target, opts)
    opts = opts or {}
    return {
        def             = def,
        attacker        = ch,
        target          = target,
        target_is_object = (target:reftype() == "object"),
        spar            = is_sparring(ch),

        weapon      = nil,

        cost        = {},
        charge_used = ch:charge_get(),
        skill_level = (def.skill and ch:skill_get(def.skill)) or 0,

        accuracy_roll  = 0,
        hit_threshold  = 0,
        speed_modifier = 0,
        defense_total  = 0,

        blocked          = false,
        parried          = false,
        dodged           = false,
        absorbed         = false,
        partial_negation = 0.0,
        counterattack    = nil,
        backlash         = nil,

        hit_location      = nil,
        base_damage       = 0,
        damage            = 0,
        damage_to         = {},
        damage_by_element = {},
        crit              = false,
        crit_multiplier   = 1.0,

        is_multihit    = opts.is_multihit    or false,
        multihit_index = opts.multihit_index or 0,

        hit          = false,
        xp_credit    = 0,
        skill_credit = 0,
    }
end

local function aoe_valid_target(ch, person)
    local _, MF, AFF = flags()
    if person:id_get() == ch:id_get() then return false end
    if not person:is_npc() and person:aff_flagged(AFF.SPIRIT) then return false end
    if not person:is_npc() and (person:stat_get("level") or 0) <= 8 then return false end
    if person:mob_flagged(MF.NOKILL) then return false end
    return true
end

local function launch_aoe_instance(ch, def, inst)
    local tier = def.tier or 1

    -- 1. Default cost
    if next(inst.cost) == nil then
        inst.cost.stamina = math.floor(ch:meter_max("stamina") / (base_divisor[tier] or 500))
        if def.consumes_charge and inst.charge_used > 0 then
            inst.cost.ki = inst.charge_used
        end
    end

    -- 2. on_calculate_cost
    if def.on_calculate_cost then def.on_calculate_cost(inst) end

    -- 3. Cost precondition checks
    for slug, amount in pairs(inst.cost) do
        if ch:meter_current(slug) < amount then
            ch:send_line("You do not have enough %s.", slug)
            return false
        end
    end

    -- 4. Limbs check
    if def.limbs_required then
        for _, limb in ipairs(def.limbs_required) do
            if limb == "arm" then
                if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then
                    ch:send_line("You have no available arms!")
                    return false
                end
            elseif limb == "leg" then
                if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then
                    ch:send_line("You have no available legs!")
                    return false
                end
            end
        end
    end

    -- 5. on_check
    if def.on_check then
        local ok, reason = def.on_check(inst)
        if ok == false then
            if reason then ch:send_line(reason) end
            return false
        end
    end

    -- 6. Peaceful room check
    local RF = dbat.consts.room_flags
    local room = ch:room_get()
    if room and room:flagged(RF.PEACEFUL) then
        ch:send_line("This room just has such a peaceful, easy feeling...")
        return false
    end

    -- 7. Count valid targets
    local count = 0
    if room then
        for person in room:people() do
            if aoe_valid_target(ch, person) then count = count + 1 end
        end
    end
    if count == 0 then
        ch:send_line("There is no one worth targeting around.")
        return false
    end

    -- 8. Accuracy roll
    inst.accuracy_roll    = roll_accuracy(ch, inst.skill_level)
    inst.hit_threshold    = dbat.axion_dice(0)
    inst.accuracy_modifier = 0
    if def.on_modify_accuracy then def.on_modify_accuracy(inst) end
    inst.hit = (inst.accuracy_roll + inst.accuracy_modifier >= inst.hit_threshold - 20)

    -- 9. Miss or hit
    if inst.hit then
        if def.on_calculate_damage then
            inst.base_damage = def.on_calculate_damage(inst)
        else
            inst.base_damage = base_damage(ch, def, inst)
        end
        inst.damage = inst.base_damage
        if def.on_aoe then def.on_aoe(inst, count) end
    else
        if def.on_aoe_miss then def.on_aoe_miss(inst) end
    end

    -- 10. Cost deduction
    for slug, amount in pairs(inst.cost) do
        if amount > 0 then ch:meter_mod_int(slug, -amount) end
    end

    -- 11. on_after_cost
    if def.on_after_cost then def.on_after_cost(inst) end

    -- 12. Lag
    ch:wait_set(lag_for_tier[tier] or 4)

    -- 13. Skill gain
    if inst.hit and def.skill then
        ch:improve_skill(def.skill, 1)
    end

    return true
end

local function launch_instance(ch, def, target, inst)
    local tier = def.tier or 1

    -- 5. Default cost (skip on free multihit follow-ups; cost={} stays empty)
    if next(inst.cost) == nil then
        inst.cost.stamina = math.floor(ch:meter_max("stamina") / (base_divisor[tier] or 500))
        if def.consumes_charge and inst.charge_used > 0 then
            inst.cost.ki = inst.charge_used
        end
    end

    -- 6. on_calculate_cost
    if def.on_calculate_cost then def.on_calculate_cost(inst) end

    -- 7. Precondition checks
    for slug, amount in pairs(inst.cost) do
        if ch:meter_current(slug) < amount then
            ch:send_line("You do not have enough %s.", slug)
            return false
        end
    end

    if def.limbs_required then
        for _, limb in ipairs(def.limbs_required) do
            if limb == "arms" then
                if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then
                    ch:send_line("You cannot use that attack with both arms broken!")
                    return false
                end
            elseif limb == "legs" then
                if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then
                    ch:send_line("You cannot use that attack with both legs broken!")
                    return false
                end
            end
        end
    end

    if def.consumes_charge and (def.min_charge or 0) > 0 then
        if inst.charge_used < def.min_charge then
            ch:send_line("You need at least %d charged ki for this attack!", def.min_charge)
            return false
        end
    end

    if def.on_check then
        local ok, reason = def.on_check(inst)
        if ok == false then
            if reason then ch:send_line(reason) end
            return false
        end
    end

    -- 8. Accuracy phase
    inst.accuracy_roll     = roll_accuracy(ch, inst.skill_level)
    inst.hit_threshold     = dbat.axion_dice(0)
    inst.accuracy_modifier = 0
    if def.on_modify_accuracy then def.on_modify_accuracy(inst) end
    if inst.target_is_object then
        inst.speed_modifier = 0
        inst.defense_total  = 0
        inst.hit            = true
    else
        inst.speed_modifier = speed_modifier(ch, target)
        inst.defense_total  = defense_total(target)
        local avo              = math.floor(inst.defense_total / 4)
        local effective        = inst.accuracy_roll - avo + inst.speed_modifier + inst.accuracy_modifier
        inst.effective_accuracy = effective
        inst.hit               = (effective >= inst.hit_threshold - 20)
    end

    -- 9. Damage phase
    if inst.hit then
        inst.hit_location               = inst.target_is_object and "body"
                                          or roll_hitloc(ch, target, inst.skill_level)
        inst.crit, inst.crit_multiplier = calc_crit(ch, inst.hit_location)
        if def.on_calculate_damage then
            inst.base_damage = def.on_calculate_damage(inst)
        else
            inst.base_damage = base_damage(ch, def, inst)
        end
        -- on_hit_location: attack def sets base crit_multiplier per location before condition hooks run
        if def.on_hit_location then def.on_hit_location(inst) end
        inst.damage                     = math.floor(inst.base_damage * inst.crit_multiplier)
        for elem, portion in pairs(def.elements or {}) do
            inst.damage_by_element[elem] = math.floor(inst.damage * portion)
        end
    end

    -- 9b. Attacker offense hooks (damage multipliers: hasshuken, infuse, kaioken, etc.)
    if inst.hit then
        ch:check_attack_offense(inst)
        if def.on_check_attack_offense then def.on_check_attack_offense(inst) end
    end

    -- 10. Victim defense hooks (evasion: zanzoken, absorb, fireshield backlash, etc.)
    target:check_attack_defense(inst)

    -- 11. process_defense
    process_defense(inst)

    -- 12. Hit vs miss routing
    if inst.hit and inst.damage > 0 then
        if next(inst.damage_to) == nil then
            inst.damage_to = { powerlevel = inst.damage }
        end
        if inst.target_is_object then
            if def.on_hit_object then def.on_hit_object(inst) end
        else
            if def.on_hit then def.on_hit(inst) end
        end
        target:on_attacked(inst)
    else
        if inst.target_is_object then
            if def.on_miss_object then def.on_miss_object(inst) end
        else
            if inst.dodged   and def.on_dodged   then def.on_dodged(inst)   end
            if inst.blocked  and def.on_blocked  then def.on_blocked(inst)  end
            if inst.parried  and def.on_parried  then def.on_parried(inst)  end
            if inst.absorbed and def.on_absorbed then def.on_absorbed(inst) end
            if not (inst.dodged or inst.blocked or inst.parried or inst.absorbed) then
                if def.on_miss then def.on_miss(inst) end
            end
        end
    end

    -- 12.5. Engage combat (both directions, regardless of hit/miss)
    if not inst.target_is_object then
        if not ch:fighting_get() then ch:start_fighting(target) end
        if not target:fighting_get() then target:start_fighting(ch) end
    end

    -- 12.6. Advance combo streak on successful non-multihit melee hits
    if inst.hit and inst.damage > 0 and not inst.target_is_object
       and not inst.is_multihit and def.family == "melee"
    then
        local had_combo = ch:condition_has("combo")
        if had_combo then
            local cond     = ch:condition("combo")
            local state    = cond:number_get("state")
            local expected = COMBO_SEQUENCE[(state % #COMBO_SEQUENCE) + 1]
            if def.id ~= expected then
                ch:condition_remove("combo")
                ch:send_line("@RCombo broken!@W You needed to use @C%s@n.", expected)
                had_combo = false
            end
        end
        ch:condition_apply("combo")
        local next_id = M.combo_next(ch, inst)
        if next_id then
            if had_combo then
                ch:send_line("@GCOMBO@n! Try a @C%s@n next!", next_id)
            else
                ch:send_line("You have a chance for a @GCOMBO@n! Try a @C%s@n next!", next_id)
            end
        end
    end

    -- 13. Backlash (only on successful hits)
    if inst.hit and inst.damage > 0 and inst.backlash then
        for _, entry in ipairs(inst.backlash) do
            apply_backlash(ch, entry, inst)
        end
    end

    -- 14. Cost deduction
    for slug, amount in pairs(inst.cost) do
        if amount > 0 then
            ch:meter_mod_int(slug, -amount)
        end
    end

    -- 14.5. Post-cost hook (e.g. partial ki recovery)
    if def.on_after_cost then def.on_after_cost(inst) end

    -- 15. Lag
    ch:wait_set(lag_for_tier[tier] or 4)

    -- 16. Skill gain
    if inst.hit and def.skill then
        ch:improve_skill(def.skill, 1)
    end

    -- 17. Multi-hit / combo follow-up (never for object targets)
    if inst.hit
       and not inst.target_is_object
       and def.can_trigger_multihit ~= false
       and inst.multihit_index < 3
    then
        local atk_dex = ch:stat_get("dexterity") or 0
        local def_dex = (target:stat_get("dexterity") or 0) + math.random(1, 15)
        local style   = ch:skill_get("style") or 0
        local reduction = style >= 100 and 0.10 or style >= 80 and 0.08
                       or style >= 60  and 0.06 or style >= 40 and 0.04
                       or style >= 20  and 0.02 or 0
        def_dex = math.floor(def_dex * (1 - reduction))

        if atk_dex >= def_dex and def.in_multihit ~= false then
            local follow = build_instance(ch, def, target, {
                is_multihit    = true,
                multihit_index = inst.multihit_index + 1,
            })
            follow.cost = {}
            launch_instance(ch, def, target, follow)
        end
    end

    return true
end

function M.launch(ch, attack_id, target, opts)
    local def = dbat.get("attacks", attack_id)
    if not def then
        ch:send_line("Unknown attack: %s", tostring(attack_id))
        return false
    end
    if def.is_aoe then
        local inst = {
            def         = def,
            attacker    = ch,
            target      = nil,
            spar        = is_sparring(ch),
            cost        = {},
            charge_used = ch:charge_get(),
            skill_level = (def.skill and ch:skill_get(def.skill)) or 0,
            accuracy_roll     = 0,
            accuracy_modifier = 0,
            hit_threshold     = 0,
            hit           = false,
            base_damage   = 0,
            damage        = 0,
        }
        return launch_aoe_instance(ch, def, inst)
    end
    local inst = build_instance(ch, def, target, opts)
    return launch_instance(ch, def, target, inst)
end

M._roll_accuracy   = roll_accuracy
M._speed_modifier  = speed_modifier
M._defense_total   = defense_total
M._roll_hitloc     = roll_hitloc
M._calc_crit       = calc_crit
M._base_damage     = base_damage
M._process_defense = process_defense
M._build_instance  = build_instance
M._launch_instance = launch_instance
M._aoe_valid_target = aoe_valid_target

return M
