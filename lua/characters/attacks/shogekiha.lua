local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou thrust out your palm and release a @Yshogekiha@W into @C$N's@W body!@n",
        target = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into your body!@n",
        room   = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou thrust out your palm and release a @Yshogekiha@W into @C$N's@W head!@n",
        target = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into your head!@n",
        room   = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou thrust out your palm and release a @Yshogekiha@W into @C$N's@W arm!@n",
        target = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into your arm!@n",
        room   = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou thrust out your palm and release a @Yshogekiha@W into @C$N's@W leg!@n",
        target = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into your leg!@n",
        room   = "@c$n@W thrusts out $s palm and releases a @Yshogekiha@W into @C$N's@W leg!@n",
    },
}

return {
    id    = "shogekiha",
    family = "ki",
    name  = "Shogekiha",
    skill = "shogekiha",
    tier  = 2,
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
    base_power    = 1.7,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ch    = inst.attacker
        local skill = inst.skill_level
        local ctx   = { actor = ch, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Charge drain mastery: chance to drain 25% of victim's ki
        local chance = skill >= 100 and 20 or skill >= 75 and 10 or 5
        if math.random(100) <= chance then
            local drain = math.floor(inst.target:meter_max("ki") * 0.25)
            inst.target:meter_mod_int("ki", -drain)
            act().message({
                actor  = "@BYour shogekiha drains @C$N's@B charged ki!@n",
                target = "@B@c$n's@B shogekiha drains your charged ki!@n",
                room   = "@B@c$n's@B shogekiha drains @C$N's@B charged ki!@n",
            }, ctx)
        end

        -- Kibito ki regeneration on hit
        if ch:sensei_get() == "kibito" then
            local regen_pct = skill >= 100 and 0.15 or skill >= 60 and 0.10 or 0.05
            local regen = math.floor(ch:meter_max("ki") * regen_pct)
            ch:meter_mod_int("ki", regen)
        end
    end,

    on_after_cost = function(inst)
        -- Kibito at skill 100: no cooldown (reset wait immediately)
        if inst.attacker:sensei_get() == "kibito" and inst.skill_level >= 100 then
            inst.attacker:wait_set(0)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou thrust out your palm but your shogekiha misses @C$N@W!@n",
            target = "@c$n@W thrusts out $s palm at you but the shogekiha misses!@n",
            room   = "@c$n@W thrusts out $s palm at @C$N@W but the shogekiha misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your shogekiha!@n",
            target = "@WYou dodge @c$n@W's shogekiha!@n",
            room   = "@C$N@W dodges @c$n@W's shogekiha!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your shogekiha!@n",
            target = "@WYou block @c$n@W's shogekiha!@n",
            room   = "@C$N@W blocks @c$n@W's shogekiha!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
