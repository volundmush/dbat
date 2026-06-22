local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou draw upon the @Rphoenix flame@W within your sword and unleash a @Rburning@W slash across @C$N's@W body!@n",
        target = "@c$n@W draws upon the @Rphoenix flame@W within $s sword and unleashes a @Rburning@W slash across your body!@n",
        room   = "@c$n@W draws upon the @Rphoenix flame@W within $s sword and unleashes a @Rburning@W slash across @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYour @Rphoenix slash@W scorches @C$N's@W head!@n",
        target = "@c$n@W's @Rphoenix slash@W scorches your head!@n",
        room   = "@c$n@W's @Rphoenix slash@W scorches @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYour @Rphoenix slash@W scorches @C$N's@W arm!@n",
        target = "@c$n@W's @Rphoenix slash@W scorches your arm!@n",
        room   = "@c$n@W's @Rphoenix slash@W scorches @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYour @Rphoenix slash@W scorches @C$N's@W leg!@n",
        target = "@c$n@W's @Rphoenix slash@W scorches your leg!@n",
        room   = "@c$n@W's @Rphoenix slash@W scorches @C$N's@W leg!@n",
    },
}

return {
    id     = "pslash",
    family = "ki",
    name   = "Phoenix Slash",
    skill  = "phoenix slash",
    tier   = 3,
    elements = { ki = 0.7, fire = 0.3 },
    consumes_charge = true,
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
        local ch = inst.attacker
        if ch:condition_has("mystic_melody") then
            return false, "Your mystic melody prevents you from using phoenix slash."
        end
        local grav_ok, grav_msg = ki.can_grav(ch)
        if not grav_ok then return false, grav_msg end
        if ch:wielded_weapon_type() ~= "slash" then
            ch:send_line("You need to be wielding a slashing weapon to use phoenix slash.")
            return false
        end
        return true
    end,

    on_hit = function(inst)
        local target = inst.target
        local ctx    = { actor = inst.attacker, target = target }
        local loc    = inst.hit_location

        -- Fire damage modifiers
        if target:condition_has("fireprone") then inst.damage = math.floor(inst.damage * 1.4) end
        if target:condition_has("fireproof") then inst.damage = math.floor(inst.damage * 0.6) end

        act().message(HIT_MSGS[loc] or HIT_MSGS.body, ctx)

        -- Apply burned condition; demons and fireproof are immune
        local immune = target:race_get() == "demon" or target:condition_has("fireproof")
        if not immune then
            target:condition_apply("burned")
            act().message({
                actor  = "@R@C$N@R is set ablaze by your phoenix slash!@n",
                target = "@RYou are set ablaze by @c$n@R's phoenix slash!@n",
                room   = "@R@C$N@R is set ablaze by @c$n@R's phoenix slash!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour @Rphoenix slash@W misses @C$N@W!@n",
            target = "@c$n@W's @Rphoenix slash@W misses you!@n",
            room   = "@c$n@W's @Rphoenix slash@W misses @C$N@W!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your phoenix slash!@n",
            target = "@WYou dodge @c$n@W's phoenix slash!@n",
            room   = "@C$N@W dodges @c$n@W's phoenix slash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your phoenix slash!@n",
            target = "@WYou block @c$n@W's phoenix slash!@n",
            room   = "@C$N@W blocks @c$n@W's phoenix slash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
