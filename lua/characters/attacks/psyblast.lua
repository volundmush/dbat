local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou focus your mind and unleash a @Mpsychic blast@W that tears into @C$N's@W body!@n",
        target = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into your body!@n",
        room   = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou focus your mind and unleash a @Mpsychic blast@W that tears into @C$N's@W head!@n",
        target = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into your head!@n",
        room   = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou focus your mind and unleash a @Mpsychic blast@W that tears into @C$N's@W arm!@n",
        target = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into your arm!@n",
        room   = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou focus your mind and unleash a @Mpsychic blast@W that tears into @C$N's@W leg!@n",
        target = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into your leg!@n",
        room   = "@c$n@W focuses $s mind and unleashes a @Mpsychic blast@W that tears into @C$N's@W leg!@n",
    },
}

return {
    id    = "psyblast",
    family = "ki",
    name  = "Psychic Blast",
    skill = "psychic blast",
    tier  = 4,
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
    base_power    = 2.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Ki drain: 33% chance
        if math.random(3) == 2 then
            local drain = math.floor(inst.damage * 0.20)
            inst.target:meter_mod_int("ki", -drain)
            act().message({
                actor  = "@BThe psychic blast tears some of @C$N's@B ki away!@n",
                target = "@BThe psychic blast tears some of your ki away!@n",
                room   = "@BThe psychic blast tears some of @C$N's@B ki away!@n",
            }, ctx)
        end

        -- Shocked condition: 25% chance, sanctuary blocks
        if math.random(4) == 4 and not inst.target:condition_has("sanctuary") then
            inst.target:condition_apply("shocked")
            act().message({
                actor  = "@C$N@W reels from the shock of your psychic blast!@n",
                target = "@WYou reel from the shock of @c$n@W's psychic blast!@n",
                room   = "@C$N@W reels from the shock of @c$n@W's psychic blast!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a psychic blast at @C$N@W but miss!@n",
            target = "@c$n@W fires a psychic blast at you but misses!@n",
            room   = "@c$n@W fires a psychic blast at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your psychic blast!@n",
            target = "@WYou dodge @c$n@W's psychic blast!@n",
            room   = "@C$N@W dodges @c$n@W's psychic blast!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your psychic blast!@n",
            target = "@WYou block @c$n@W's psychic blast!@n",
            room   = "@C$N@W blocks @c$n@W's psychic blast!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
