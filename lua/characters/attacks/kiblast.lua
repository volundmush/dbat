local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou fire a small @Yblast of ki@W at @C$N's@W chest!@n",
        target = "@c$n@W fires a small @Yblast of ki@W at your chest!@n",
        room   = "@c$n@W fires a small @Yblast of ki@W at @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou fire a small @Yblast of ki@W at @C$N's@W face!@n",
        target = "@c$n@W fires a small @Yblast of ki@W at your face!@n",
        room   = "@c$n@W fires a small @Yblast of ki@W at @C$N's@W face!@n",
    },
    arm = {
        actor  = "@WYou fire a small @Yblast of ki@W at @C$N's@W arm!@n",
        target = "@c$n@W fires a small @Yblast of ki@W at your arm!@n",
        room   = "@c$n@W fires a small @Yblast of ki@W at @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou fire a small @Yblast of ki@W at @C$N's@W leg!@n",
        target = "@c$n@W fires a small @Yblast of ki@W at your leg!@n",
        room   = "@c$n@W fires a small @Yblast of ki@W at @C$N's@W leg!@n",
    },
}

return {
    id    = "kiblast",
    family = "ki",
    name  = "Ki Blast",
    skill = "kiblast",
    tier  = 2,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
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
    base_power    = 1.2,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Android bonus damage
        if inst.attacker:race_get() == "android" then
            local skill = inst.skill_level
            local mult  = skill >= 100 and 0.15 or skill >= 60 and 0.10 or skill >= 40 and 0.05 or 0
            if mult > 0 then
                inst.damage = math.floor(inst.damage * (1 + mult))
            end
        end

        -- Knockout mastery
        local skill   = inst.skill_level
        local chance  = skill >= 100 and 30 or skill >= 75 and 20 or 15
        if math.random(100) <= chance and not inst.target:condition_has("knocked_out") then
            inst.target:condition_apply("knocked_out")
            act().message({
                actor  = "@C$N@W is knocked out by your kiblast!@n",
                target = "@WYou are knocked out by @c$n@W's kiblast!@n",
                room   = "@C$N@W is knocked out by @c$n@W's kiblast!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a kiblast at @C$N@W but miss!@n",
            target = "@c$n@W fires a kiblast at you but misses!@n",
            room   = "@c$n@W fires a kiblast at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your kiblast!@n",
            target = "@WYou dodge @c$n@W's kiblast!@n",
            room   = "@C$N@W dodges @c$n@W's kiblast!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_parried = function(inst)
        act().message({
            actor  = "@C$N@W deflects your kiblast!@n",
            target = "@WYou deflect @c$n@W's kiblast!@n",
            room   = "@C$N@W deflects @c$n@W's kiblast!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your kiblast!@n",
            target = "@WYou block @c$n@W's kiblast!@n",
            room   = "@C$N@W blocks @c$n@W's kiblast!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
