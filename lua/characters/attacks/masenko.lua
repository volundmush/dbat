local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou thrust your hands forward and unleash a blazing @Ymasenko@W into @C$N's@W body!@n",
        target = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into your body!@n",
        room   = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou thrust your hands forward and unleash a blazing @Ymasenko@W into @C$N's@W head!@n",
        target = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into your head!@n",
        room   = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou thrust your hands forward and unleash a blazing @Ymasenko@W into @C$N's@W arm!@n",
        target = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into your arm!@n",
        room   = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou thrust your hands forward and unleash a blazing @Ymasenko@W into @C$N's@W leg!@n",
        target = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into your leg!@n",
        room   = "@c$n@W thrusts $s hands forward and unleashes a blazing @Ymasenko@W into @C$N's@W leg!@n",
    },
}

return {
    id    = "masenko",
    family = "ki",
    name  = "Masenko",
    skill = "masenko",
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
    base_accuracy = 1.0,
    base_power    = 2.5,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Piccolo sensei bonus: extra ki-based damage
        if inst.attacker:sensei_get() == "piccolo" then
            local ki_max = inst.attacker:meter_max("ki")
            local skill  = inst.skill_level
            local bonus  = skill >= 100 and 0.08 or skill >= 75 and 0.05 or 0.03
            inst.damage  = inst.damage + math.floor(ki_max * bonus)
        end

        -- Stamina drain: 50% chance, sanctuary blocks it
        if math.random(2) == 1 and not inst.target:condition_has("sanctuary") then
            inst.target:meter_mod_int("stamina", -math.floor(inst.damage / 4))
            act().message({
                actor  = "@RThe masenko takes a heavy toll on @C$N@R's stamina!@n",
                target = "@RThe masenko takes a heavy toll on your stamina!@n",
                room   = "@RThe masenko takes a heavy toll on @C$N@R's stamina!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour masenko misses @C$N@W completely!@n",
            target = "@c$n@W fires a masenko at you but misses!@n",
            room   = "@c$n@W fires a masenko at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W barely evades your masenko!@n",
            target = "@WYou barely evade @c$n@W's masenko!@n",
            room   = "@C$N@W barely evades @c$n@W's masenko!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your masenko with a powerful guard!@n",
            target = "@WYou block @c$n@W's masenko with a powerful guard!@n",
            room   = "@C$N@W blocks @c$n@W's masenko with a powerful guard!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_absorbed = function(inst)
        act().message({
            actor  = "@C$N@W absorbs your masenko into $S android systems!@n",
            target = "@WYou absorb @c$n@W's masenko into your android systems!@n",
            room   = "@C$N@W absorbs @c$n@W's masenko into $S android systems!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
