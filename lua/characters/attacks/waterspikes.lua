local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@CYou create a ball of water, form it into frozen spikes, and launch them at @R$N@C slamming into $S chest!@n",
        target = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @RYOU@C slamming into YOUR chest!@n",
        room   = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @R$N@C slamming into $s chest!@n",
    },
    head = {
        actor  = "@CYou create a ball of water, form it into frozen spikes, and launch them at @R$N@C slamming into $S head!@n",
        target = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @RYOU@C slamming into YOUR head!@n",
        room   = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @R$N@C slamming into $s head!@n",
    },
    arm = {
        actor  = "@CYou create a ball of water, form it into frozen spikes, and launch them at @R$N@C slamming into $S arm!@n",
        target = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @RYOU@C slamming into YOUR arm!@n",
        room   = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @R$N@C slamming into $s arm!@n",
    },
    leg = {
        actor  = "@CYou create a ball of water, form it into frozen spikes, and launch them at @R$N@C slamming into $S leg!@n",
        target = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @RYOU@C slamming into YOUR leg!@n",
        room   = "@c$n@C creates a ball of water, forms it into frozen spikes, and launches them at @R$N@C slamming into $s leg!@n",
    },
}

return {
    id     = "waterspikes",
    family = "ki",
    name   = "Water Spikes",
    skill  = "water spikes",
    tier   = 2,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.8,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_modify_accuracy = function(inst)
        local perf = inst.attacker:skill_perf_get("water spikes")
        if perf >= 2 then
            inst.accuracy_modifier = inst.accuracy_modifier + 5
        end
    end,

    on_calculate_cost = function(inst)
        local perf = inst.attacker:skill_perf_get("water spikes")
        if perf == 1 then
            inst.ki_cost = math.floor(inst.ki_cost * 1.05 / 0.14 * 0.14)
        elseif perf == 3 then
            inst.ki_cost = math.max(
                math.floor(inst.attacker:meter_max("ki") * 0.01),
                inst.ki_cost - math.floor(inst.attacker:meter_max("ki") * 0.05)
            )
        end
    end,

    on_hit = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        ki.aqua_barrier_boost(ch, inst.damage)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = ch, target = vict })
        -- Head hit: chance to knock out (if target has > 1/5 ch HP and no sanctuary)
        if inst.hit_location == "head"
            and math.random(3) >= 2
            and not vict:condition_has("knocked")
            and not vict:condition_has("sanctuary") then
            vict:condition_apply("knocked", "combat", "stunned")
            act().message({
                actor  = "@C$N@W is knocked out!@n",
                target = "@WYou are knocked out!@n",
                room   = "@C$N@W is knocked out!@n",
            }, { actor = ch, target = vict })
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your water spikes miss, flying through the air harmlessly!@n",
            target = "@c$n@W fires water spikes at you, but misses!@n",
            room   = "@c$n@W fires water spikes at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your water spikes, letting them slam into the surroundings!@n",
            target = "@WYou dodge @C$n's@W water spikes, letting them slam into the surroundings!@n",
            room   = "@C$N@W manages to dodge @c$n's@W water spikes, letting them slam into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        inst.attacker:room_get():send_line("@wA bright explosion erupts from the impact!@n")
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W moves quickly and blocks your water spikes!@n",
            target = "@WYou move quickly and block @C$n's@W water spikes!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W water spikes!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_after_cost = function(inst)
        -- Ki refund mastery
        local ch = inst.attacker
        local sk = inst.skill_level
        local frac = sk >= 100 and 0.3 or sk >= 60 and 0.1 or sk >= 40 and 0.05 or 0
        if frac > 0 then
            local refund = math.floor(ch:meter_max("ki") * inst.charge_fraction * frac)
            ch:meter_mod_int("ki", refund)
        end
        -- Perf 3: extra lag
        if inst.attacker:skill_perf_get("water spikes") == 3 then
            inst.attacker:wait_set(3)
        end
    end,
}
