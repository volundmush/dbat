local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

-- Module-level guard: prevents on_hit from recursing into sub-launches
local _in_subhit = false

local HIT_MSGS = {
    body = {
        actor  = "@WYou raise both hands and fire a @Cdual beam@W that slams into @C$N's@W body!@n",
        target = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into your body!@n",
        room   = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou raise both hands and fire a @Cdual beam@W that slams into @C$N's@W head!@n",
        target = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into your head!@n",
        room   = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou raise both hands and fire a @Cdual beam@W that slams into @C$N's@W arm!@n",
        target = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into your arm!@n",
        room   = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou raise both hands and fire a @Cdual beam@W that slams into @C$N's@W leg!@n",
        target = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into your leg!@n",
        room   = "@c$n@W raises both hands and fires a @Cdual beam@W that slams into @C$N's@W leg!@n",
    },
}

return {
    id    = "dualbeam",
    family = "ki",
    name  = "Dual Beam",
    skill = "dual beam",
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
    base_power    = 1.5,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Fire 2 additional beams (3 total). Guard prevents infinite recursion.
        if not _in_subhit then
            _in_subhit = true
            local atk = require("lua.libs.attack")
            for _ = 1, 2 do
                atk.launch(inst.attacker, "dualbeam", inst.target, { cost = {} })
            end
            _in_subhit = false
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a dual beam at @C$N@W but miss!@n",
            target = "@c$n@W fires a dual beam at you but misses!@n",
            room   = "@c$n@W fires a dual beam at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your dual beam!@n",
            target = "@WYou dodge @c$n@W's dual beam!@n",
            room   = "@C$N@W dodges @c$n@W's dual beam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your dual beam!@n",
            target = "@WYou block @c$n@W's dual beam!@n",
            room   = "@C$N@W blocks @c$n@W's dual beam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_absorbed = function(inst)
        act().message({
            actor  = "@C$N@W absorbs your dual beam into $S android systems!@n",
            target = "@WYou absorb @c$n@W's dual beam into your android systems!@n",
            room   = "@C$N@W absorbs @c$n@W's dual beam into $S android systems!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
