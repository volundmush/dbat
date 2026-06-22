local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou focus divine energy through your eyes and strike @C$N@W with a @Yblessed hammer@W beam in $S body!@n",
        target = "@c$n@W focuses divine energy through $s eyes and strikes you with a @Yblessed hammer@W beam in your body!@n",
        room   = "@c$n@W focuses divine energy through $s eyes and strikes @C$N@W with a @Yblessed hammer@W beam in $S body!@n",
    },
    head = {
        actor  = "@WYou focus divine energy through your eyes and strike @C$N@W with a @Yblessed hammer@W beam in $S head!@n",
        target = "@c$n@W focuses divine energy through $s eyes and strikes you with a @Yblessed hammer@W beam in your head!@n",
        room   = "@c$n@W focuses divine energy through $s eyes and strikes @C$N@W with a @Yblessed hammer@W beam in $S head!@n",
    },
    arm = {
        actor  = "@WYou focus divine energy through your eyes and strike @C$N@W with a @Yblessed hammer@W beam in $S arm!@n",
        target = "@c$n@W focuses divine energy through $s eyes and strikes you with a @Yblessed hammer@W beam in your arm!@n",
        room   = "@c$n@W focuses divine energy through $s eyes and strikes @C$N@W with a @Yblessed hammer@W beam in $S arm!@n",
    },
    leg = {
        actor  = "@WYou focus divine energy through your eyes and strike @C$N@W with a @Yblessed hammer@W beam in $S leg!@n",
        target = "@c$n@W focuses divine energy through $s eyes and strikes you with a @Yblessed hammer@W beam in your leg!@n",
        room   = "@c$n@W focuses divine energy through $s eyes and strikes @C$N@W with a @Yblessed hammer@W beam in $S leg!@n",
    },
}

return {
    id     = "blessedhammer",
    family = "ki",
    name   = "Blessed Hammer",
    skill  = "blessed hammer",
    tier   = 1,
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
    base_power    = 0.8,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_modify_accuracy = function(inst)
        inst.accuracy_modifier = 15
    end,

    on_hit = function(inst)
        -- Sanctuary bonus: divine beams amplified against sanctuary field
        if inst.target:condition_has("sanctuary") then
            inst.damage = math.floor(inst.damage * 3)
        end
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour blessed hammer misses @C$N@W!@n",
            target = "@c$n@W fires blessed hammer beams at you but misses!@n",
            room   = "@c$n@W fires blessed hammer beams at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your blessed hammer!@n",
            target = "@WYou dodge @c$n@W's blessed hammer!@n",
            room   = "@C$N@W dodges @c$n@W's blessed hammer!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your blessed hammer!@n",
            target = "@WYou block @c$n@W's blessed hammer!@n",
            room   = "@C$N@W blocks @c$n@W's blessed hammer!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
