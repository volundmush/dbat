local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou gather your energy into a crackling @Wspirit ball@W and hurl it at @C$N's@W body!@n",
        target = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at your body!@n",
        room   = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou gather your energy into a crackling @Wspirit ball@W and hurl it at @C$N's@W head!@n",
        target = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at your head!@n",
        room   = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou gather your energy into a crackling @Wspirit ball@W and hurl it at @C$N's@W arm!@n",
        target = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at your arm!@n",
        room   = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou gather your energy into a crackling @Wspirit ball@W and hurl it at @C$N's@W leg!@n",
        target = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at your leg!@n",
        room   = "@c$n@W gathers $s energy into a crackling @Wspirit ball@W and hurls it at @C$N's@W leg!@n",
    },
}

return {
    id     = "spiritball",
    family = "ki",
    name   = "Spirit Ball",
    skill  = "spirit ball",
    tier   = 5,
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
    base_power    = 3.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour spirit ball misses @C$N@W!@n",
            target = "@c$n@W hurls a spirit ball at you but misses!@n",
            room   = "@c$n@W hurls a spirit ball at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        -- Dodging a spiritball is exhausting — costs the defender some stamina
        local drain = math.floor(inst.target:meter_max("stamina") / 200)
        if drain > 0 then
            inst.target:meter_mod_int("stamina", -drain)
        end
        act().message({
            actor  = "@C$N@W dodges your spirit ball!@n",
            target = "@WYou dodge @c$n@W's spirit ball!@n",
            room   = "@C$N@W dodges @c$n@W's spirit ball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your spirit ball!@n",
            target = "@WYou block @c$n@W's spirit ball!@n",
            room   = "@C$N@W blocks @c$n@W's spirit ball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
