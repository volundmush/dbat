local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou breathe in deeply and release a torrent of @Rfire@W engulfing @C$N's@W body!@n",
        target = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing your body!@n",
        room   = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou breathe in deeply and release a torrent of @Rfire@W engulfing @C$N's@W head!@n",
        target = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing your head!@n",
        room   = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou breathe in deeply and release a torrent of @Rfire@W engulfing @C$N's@W arm!@n",
        target = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing your arm!@n",
        room   = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou breathe in deeply and release a torrent of @Rfire@W engulfing @C$N's@W leg!@n",
        target = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing your leg!@n",
        room   = "@c$n@W breathes in deeply and releases a torrent of @Rfire@W engulfing @C$N's@W leg!@n",
    },
}

return {
    id    = "honoo",
    family = "ki",
    name  = "Honoo",
    skill = "honoo",
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
    base_power    = 2.3,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local target = inst.target
        local ctx    = { actor = inst.attacker, target = target }

        -- Fire damage modifiers based on victim's fire resistance
        local has_fireprone = target:condition_has("fireprone")
        local has_fireproof = target:condition_has("fireproof")
        if has_fireprone  then inst.damage = math.floor(inst.damage * 1.4) end
        if has_fireproof then inst.damage = math.floor(inst.damage * 0.6) end

        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Burned condition: 25% chance, demons and fireproof are immune, fireprone guaranteed
        local immune = target:race_get() == "demon" or has_fireproof
        if not immune and (has_fireprone or math.random(4) == 3) then
            target:condition_apply("burned")
            act().message({
                actor  = "@R@C$N@R is set ablaze by your honoo!@n",
                target = "@RYou are set ablaze by @c$n@R's honoo!@n",
                room   = "@R@C$N@R is set ablaze by @c$n@R's honoo!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou breathe fire at @C$N@W but miss!@n",
            target = "@c$n@W breathes fire at you but misses!@n",
            room   = "@c$n@W breathes fire at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your honoo!@n",
            target = "@WYou dodge @c$n@W's honoo!@n",
            room   = "@C$N@W dodges @c$n@W's honoo!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your honoo!@n",
            target = "@WYou block @c$n@W's honoo!@n",
            room   = "@C$N@W blocks @c$n@W's honoo!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
