local function ke() return require("lua.libs.ki_effects") end
local function act() return require("dbat").lib.act end

local HIT_MSGS = {
    body = {
        actor  = "@WYou fire a @Yblinding ki ball@W at @C$N's@W chest!@n",
        target = "@c$n@W fires a @Yblinding ki ball@W at your chest!@n",
        room   = "@c$n@W fires a @Yblinding ki ball@W at @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou fire a @Yblinding ki ball@W at @C$N's@W head!@n",
        target = "@c$n@W fires a @Yblinding ki ball@W at your head!@n",
        room   = "@c$n@W fires a @Yblinding ki ball@W at @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou fire a @Yblinding ki ball@W at @C$N's@W arm!@n",
        target = "@c$n@W fires a @Yblinding ki ball@W at your arm!@n",
        room   = "@c$n@W fires a @Yblinding ki ball@W at @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou fire a @Yblinding ki ball@W at @C$N's@W leg!@n",
        target = "@c$n@W fires a @Yblinding ki ball@W at your leg!@n",
        room   = "@c$n@W fires a @Yblinding ki ball@W at @C$N's@W leg!@n",
    },
}

return {
    id    = "kiball",
    name  = "Ki Ball",
    skill = "kiball",
    tier  = 2,
    elements = { ki = 1.0 },
    limbs_required = {},
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = true,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.0,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W narrowly dodges your ki ball — it crashes into the surroundings!@n",
            target = "@WYou narrowly dodge @c$n@W's ki ball — it crashes into the surroundings!@n",
            room   = "@C$N@W narrowly dodges @c$n@W's ki ball — it crashes into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        ke().ki_terrain_hit(inst.attacker)
    end,

    on_parried = function(inst)
        act().message({
            actor  = "@C$N@W deflects your ki ball!@n",
            target = "@WYou deflect @c$n@W's ki ball!@n",
            room   = "@C$N@W deflects @c$n@W's ki ball!@n",
        }, { actor = inst.attacker, target = inst.target })
        ke().ki_parry_redirect(inst, inst.base_damage)
    end,

    on_hit = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Multi-shot at high skill tiers
        local skill  = inst.skill_level
        local chance = skill >= 100 and 30 or skill >= 75 and 15 or skill >= 50 and 10 or 0
        if chance > 0 and math.random(1, 100) <= chance then
            local extra = skill >= 100 and math.random(1, 2) or 1
            local atk   = require("lua.libs.attack")
            for _ = 1, extra do
                atk.launch(inst.attacker, "kiball", inst.target, { cost = {} })
            end
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a ki ball at @C$N@W but miss!@n",
            target = "@c$n@W fires a ki ball at you but misses!@n",
            room   = "@c$n@W fires a ki ball at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your ki ball!@n",
            target = "@WYou block @c$n@W's ki ball!@n",
            room   = "@C$N@W blocks @c$n@W's ki ball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
