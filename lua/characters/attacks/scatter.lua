local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou raise both arms and unleash a scattering volley of @Gki blasts@W that tear into @C$N's@W body!@n",
        target = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into your body!@n",
        room   = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou raise both arms and unleash a scattering volley of @Gki blasts@W that tear into @C$N's@W head!@n",
        target = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into your head!@n",
        room   = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou raise both arms and unleash a scattering volley of @Gki blasts@W that tear into @C$N's@W arm!@n",
        target = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into your arm!@n",
        room   = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou raise both arms and unleash a scattering volley of @Gki blasts@W that tear into @C$N's@W leg!@n",
        target = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into your leg!@n",
        room   = "@c$n@W raises both arms and unleashes a scattering volley of @Gki blasts@W that tear into @C$N's@W leg!@n",
    },
}

return {
    id     = "scatter",
    family = "ki",
    name   = "Scatter Shot",
    skill  = "scatter shot",
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

    on_modify_accuracy = function(inst)
        -- Random accuracy bonus (+10 to +20)
        inst.accuracy_modifier = math.random(10, 20)
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_after_cost = function(inst)
        -- Piccolo sensei cooldown reduction based on skill level
        if inst.attacker:sensei_get() == "piccolo" then
            local sk = inst.skill_level
            inst.attacker:wait_set(sk >= 100 and 5 or sk >= 75 and 6 or sk >= 50 and 7 or 8)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour scatter shot misses @C$N@W!@n",
            target = "@c$n@W fires a scatter shot at you but misses!@n",
            room   = "@c$n@W fires a scatter shot at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your scatter shot!@n",
            target = "@WYou dodge @c$n@W's scatter shot!@n",
            room   = "@C$N@W dodges @c$n@W's scatter shot!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your scatter shot!@n",
            target = "@WYou block @c$n@W's scatter shot!@n",
            room   = "@C$N@W blocks @c$n@W's scatter shot!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
