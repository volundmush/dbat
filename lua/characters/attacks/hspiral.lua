local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou hold out your hand, palm upward, as @Rred@W energy pools in your palm. You punch the three-foot orb of energy, sending it flying into @C$N's@W chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        target = "@c$n@W holds out $s hand, palm upward, as @Rred@W energy pools in $s palm. $e punches the orb and sends it flying into YOUR chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        room   = "@c$n@W holds out $s hand, palm upward, as @Rred@W energy pools in $s palm. $e punches the orb and sends it flying into @c$N's@W chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
    },
    head = {
        actor  = "@WYou punch your @Rred@W energy orb into @C$N's@W head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        target = "@c$n@W punches a @Rred@W energy orb into YOUR head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        room   = "@c$n@W punches a @Rred@W energy orb into @c$N's@W head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
    },
    arm = {
        actor  = "@WYou punch your @Rred@W energy orb into @C$N's@W arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        target = "@c$n@W punches a @Rred@W energy orb into YOUR arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        room   = "@c$n@W punches a @Rred@W energy orb into @c$N's@W arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
    },
    leg = {
        actor  = "@WYou punch your @Rred@W energy orb into @C$N's@W leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        target = "@c$n@W punches a @Rred@W energy orb into YOUR leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
        room   = "@c$n@W punches a @Rred@W energy orb into @c$N's@W leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n",
    },
}

return {
    id     = "hspiral",
    family = "ki",
    name   = "Hell Spiral",
    skill  = "hell spiral",
    tier   = 3,
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
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Hell Spiral misses, flying through the air harmlessly!@n",
            target = "@c$n@W fires a Hell Spiral at you, but misses!@n",
            room   = "@c$n@W fires a Hell Spiral at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Hell Spiral, letting it fly harmlessly by!@n",
            target = "@WYou dodge @C$n's@W Hell Spiral, letting it fly harmlessly by!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Hell Spiral, letting it fly harmlessly by!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your Hell Spiral!@n",
            target = "@WYou block @C$n's@W Hell Spiral!@n",
            room   = "@C$N@W blocks @c$n's@W Hell Spiral!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
