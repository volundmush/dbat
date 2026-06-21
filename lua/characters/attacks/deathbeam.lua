local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou extend your finger and unleash a razor-thin @Rdeathbeam@W through @C$N's@W body!@n",
        target = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through your body!@n",
        room   = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou extend your finger and unleash a razor-thin @Rdeathbeam@W through @C$N's@W head!@n",
        target = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through your head!@n",
        room   = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou extend your finger and unleash a razor-thin @Rdeathbeam@W through @C$N's@W arm!@n",
        target = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through your arm!@n",
        room   = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou extend your finger and unleash a razor-thin @Rdeathbeam@W through @C$N's@W leg!@n",
        target = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through your leg!@n",
        room   = "@c$n@W extends $s finger and unleashes a razor-thin @Rdeathbeam@W through @C$N's@W leg!@n",
    },
}

return {
    id    = "deathbeam",
    family = "ki",
    name  = "Deathbeam",
    skill = "deathbeam",
    tier  = 3,
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
    base_accuracy = 1.3,  -- built-in +15 prob accuracy bonus from C++
    base_power    = 1.9,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Lifeforce drain: skill-based fraction of damage
        local skill = inst.skill_level
        local pct   = skill >= 100 and 0.40 or skill >= 60 and 0.20 or 0.05
        local drain = math.floor(inst.damage * pct)
        if drain > 0 and inst.target:meter_current("lifeforce") >= 2 then
            inst.target:meter_mod_int("lifeforce", -drain)
            act().message({
                actor  = "@RYour deathbeam saps the life force from @C$N@R!@n",
                target = "@RYou feel your life force being sapped by @c$n@R's deathbeam!@n",
                room   = "@c$n@R's deathbeam saps the life force from @C$N@R!@n",
            }, ctx)
        end
    end,

    on_after_cost = function(inst)
        -- Perf type 3: stunning stasis
        if inst.attacker:skill_perf_get("deathbeam") >= 3 then
            inst.attacker:wait_set(12)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour deathbeam narrowly misses @C$N@W!@n",
            target = "@c$n@W's deathbeam narrowly misses you!@n",
            room   = "@c$n@W's deathbeam narrowly misses @C$N@W!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W narrowly evades your deathbeam!@n",
            target = "@WYou narrowly evade @c$n@W's deathbeam!@n",
            room   = "@C$N@W narrowly evades @c$n@W's deathbeam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your deathbeam!@n",
            target = "@WYou block @c$n@W's deathbeam!@n",
            room   = "@C$N@W blocks @c$n@W's deathbeam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_absorbed = function(inst)
        act().message({
            actor  = "@C$N@W absorbs your deathbeam into $S android systems!@n",
            target = "@WYou absorb @c$n@W's deathbeam into your android systems!@n",
            room   = "@C$N@W absorbs @c$n@W's deathbeam into $S android systems!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
