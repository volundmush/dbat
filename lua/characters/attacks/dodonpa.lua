local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou extend your finger and fire a bright @Ydodonpa@W straight into @C$N's@W body!@n",
        target = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into your body!@n",
        room   = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou extend your finger and fire a bright @Ydodonpa@W straight into @C$N's@W head!@n",
        target = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into your head!@n",
        room   = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou extend your finger and fire a bright @Ydodonpa@W straight into @C$N's@W arm!@n",
        target = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into your arm!@n",
        room   = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou extend your finger and fire a bright @Ydodonpa@W straight into @C$N's@W leg!@n",
        target = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into your leg!@n",
        room   = "@c$n@W extends $s finger and fires a bright @Ydodonpa@W straight into @C$N's@W leg!@n",
    },
}

return {
    id    = "dodonpa",
    family = "ki",
    name  = "Dodonpa",
    skill = "dodonpa",
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
    base_power    = 1.6,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Kibito sensei: extra dodonpa damage
        if inst.attacker:sensei_get() == "kibito" then
            local skill = inst.skill_level
            local bonus = skill >= 100 and 0.10 or skill >= 75 and 0.05 or 0.03
            inst.damage = inst.damage + math.floor(inst.attacker:meter_max("ki") * bonus)
        end

        -- Ki drain: 33% chance
        if math.random(3) == 1 then
            inst.target:meter_mod_int("ki", -math.floor(inst.damage / 4))
            act().message({
                actor  = "@RYour dodonpa drains some of @C$N@R's ki!@n",
                target = "@RYou feel some of your ki drained away by @c$n@R's dodonpa!@n",
                room   = "@c$n@R's dodonpa drains some of @C$N@R's ki!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a dodonpa at @C$N@W but miss!@n",
            target = "@c$n@W fires a dodonpa at you but misses!@n",
            room   = "@c$n@W fires a dodonpa at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your dodonpa!@n",
            target = "@WYou dodge @c$n@W's dodonpa!@n",
            room   = "@C$N@W dodges @c$n@W's dodonpa!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your dodonpa!@n",
            target = "@WYou block @c$n@W's dodonpa!@n",
            room   = "@C$N@W blocks @c$n@W's dodonpa!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_absorbed = function(inst)
        act().message({
            actor  = "@C$N@W absorbs your dodonpa into $S android systems!@n",
            target = "@WYou absorb @c$n@W's dodonpa into your android systems!@n",
            room   = "@C$N@W absorbs @c$n@W's dodonpa into $S android systems!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
