local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou grip your sword with both hands and unleash a devastatingly quick @Ytwin slash@W across @C$N's@W body!@n",
        target = "@c$n@W grips $s sword with both hands and unleashes a devastatingly quick @Ytwin slash@W across your body!@n",
        room   = "@c$n@W grips $s sword with both hands and unleashes a devastatingly quick @Ytwin slash@W across @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou sweep your sword across @C$N's@W neck in a @Ytwin slash@W!@n",
        target = "@c$n@W sweeps $s sword across your neck in a @Ytwin slash@W!@n",
        room   = "@c$n@W sweeps $s sword across @C$N's@W neck in a @Ytwin slash@W!@n",
    },
    arm = {
        actor  = "@WYou bring your sword down in a @Ytwin slash@W against @C$N's@W arm!@n",
        target = "@c$n@W brings $s sword down in a @Ytwin slash@W against your arm!@n",
        room   = "@c$n@W brings $s sword down in a @Ytwin slash@W against @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou sweep your sword low in a @Ytwin slash@W against @C$N's@W leg!@n",
        target = "@c$n@W sweeps $s sword low in a @Ytwin slash@W against your leg!@n",
        room   = "@c$n@W sweeps $s sword low in a @Ytwin slash@W against @C$N's@W leg!@n",
    },
}

local function weapon_level(weapon)
    local EF = require("dbat").consts.item_extra_flags
    if weapon:extra_flagged(EF.WEAPLVL5) then return 5
    elseif weapon:extra_flagged(EF.WEAPLVL4) then return 4
    elseif weapon:extra_flagged(EF.WEAPLVL3) then return 3
    elseif weapon:extra_flagged(EF.WEAPLVL2) then return 2
    elseif weapon:extra_flagged(EF.WEAPLVL1) then return 1
    else return 0
    end
end

return {
    id     = "twinslash",
    family = "ki",
    name   = "Twin Slash",
    skill  = "twin slash",
    tier   = 3,
    elements = { ki = 1.0 },
    consumes_charge = true,
    limbs_required = { "arm" },
    damages_limbs = true,
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
        local ch = inst.attacker
        if ch:condition_has("mystic_melody") then
            return false, "Your mystic melody prevents you from using twin slash."
        end
        local grav_ok, grav_msg = ki.can_grav(ch)
        if not grav_ok then return false, grav_msg end
        if ch:wielded_weapon_type() ~= "slash" then
            ch:send_line("You need to be wielding a slashing weapon to use twin slash.")
            return false
        end
        return true
    end,

    on_calculate_cost = function(inst)
        if inst.attacker:perf_get() == 1 then
            local max_ki = inst.attacker:meter_max("ki")
            inst.cost.ki = (inst.cost.ki or 0) + math.floor(max_ki * 0.05)
        end
    end,

    on_modify_accuracy = function(inst)
        if inst.attacker:perf_get() == 2 then
            inst.accuracy_modifier = inst.accuracy_modifier + 5
        end
    end,

    on_calculate_damage = function(inst)
        local ch  = inst.attacker
        local sk  = inst.skill_level
        local atk = require("lua.libs.attack")
        local base = atk._base_damage(ch, inst.def, inst)
        -- weapon level bonus
        local weapon = ch:equipment_get(require("dbat").consts.wear_positions.WIELD1)
        local wlvl   = weapon and weapon_level(weapon) or 0
        local wbonus = sk >= 100 and 0.05 or sk >= 60 and 0.02 or sk >= 40 and 0.01 or 0
        return math.floor(base * (1 + wbonus * wlvl))
    end,

    on_hit = function(inst)
        local ch     = inst.attacker
        local target = inst.target
        local ctx    = { actor = ch, target = target }
        local loc    = inst.hit_location

        act().message(HIT_MSGS[loc] or HIT_MSGS.body, ctx)

        if loc == "head" then
            -- instakill if damage exceeds PL/5 and target is not majin/bio
            local race = target:race_get()
            if race ~= "majin" and race ~= "bio" then
                if inst.damage > math.floor(target:meter_max("powerlevel") / 5) then
                    -- force death via meter drain
                    target:meter_mod_int("powerlevel", -target:meter_max("powerlevel"))
                    act().message({
                        actor  = "@YYour twin slash severs @C$N's@Y head clean off!@n",
                        target = "@Y@c$n@Y's twin slash severs your head clean off!@n",
                        room   = "@Y@c$n@Y's twin slash severs @C$N's@Y head clean off!@n",
                    }, ctx)
                    return
                end
            end
        elseif loc == "arm" then
            -- 30%+ limb severing, not NPC sanctuary, not spar
            if not inst.spar and not target:is_npc()
               and not target:condition_has("sanctuary")
               and math.random(100) > 70 then
                local limb = (target:limbcond_get(1) > 0) and 1 or (target:limbcond_get(2) > 0 and 2 or nil)
                if limb then
                    target:remove_limb(limb)
                    target:limbcond_set(limb, 0)
                    act().message({
                        actor  = "@YYour twin slash severs @C$N's@Y arm!@n",
                        target = "@Y@c$n@Y's twin slash severs your arm!@n",
                        room   = "@Y@c$n@Y's twin slash severs @C$N's@Y arm!@n",
                    }, ctx)
                end
            end
        elseif loc == "leg" then
            if not inst.spar and not target:is_npc()
               and not target:condition_has("sanctuary")
               and math.random(100) > 70 then
                local limb = (target:limbcond_get(3) > 0) and 3 or (target:limbcond_get(4) > 0 and 4 or nil)
                if limb then
                    target:remove_limb(limb)
                    target:limbcond_set(limb, 0)
                    act().message({
                        actor  = "@YYour twin slash severs @C$N's@Y leg!@n",
                        target = "@Y@c$n@Y's twin slash severs your leg!@n",
                        room   = "@Y@c$n@Y's twin slash severs @C$N's@Y leg!@n",
                    }, ctx)
                end
            end
        elseif loc == "body" then
            -- 50% tail cut
            if not inst.spar and target:has_tail() and math.random(2) == 1 then
                target:lose_tail()
                act().message({
                    actor  = "@YYour twin slash cuts off @C$N's@Y tail!@n",
                    target = "@Y@c$n@Y's twin slash cuts off your tail!@n",
                    room   = "@Y@c$n@Y's twin slash cuts off @C$N's@Y tail!@n",
                }, ctx)
            end
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou swing your sword in a twin slash but miss @C$N@W entirely!@n",
            target = "@c$n@W swings $s sword in a twin slash but misses you entirely!@n",
            room   = "@c$n@W swings $s sword in a twin slash but misses @C$N@W entirely!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your twin slash!@n",
            target = "@WYou dodge @c$n@W's twin slash!@n",
            room   = "@C$N@W dodges @c$n@W's twin slash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your twin slash!@n",
            target = "@WYou block @c$n@W's twin slash!@n",
            room   = "@C$N@W blocks @c$n@W's twin slash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_after_cost = function(inst)
        if inst.attacker:perf_get() == 3 and inst.hit then
            local max_ki = inst.attacker:meter_max("ki")
            inst.attacker:meter_mod_int("ki", math.floor(max_ki * 0.05))
        end
    end,
}
