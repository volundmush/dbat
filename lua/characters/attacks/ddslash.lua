local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@DYou unleash a @BDarkness Dragon Slash@D across @C$N's@D body, a wave of dark energy trailing your blade!@n",
        target = "@c$n@D unleashes a @BDarkness Dragon Slash@D across your body, a wave of dark energy trailing $s blade!@n",
        room   = "@c$n@D unleashes a @BDarkness Dragon Slash@D across @C$N's@D body!@n",
    },
    head = {
        actor  = "@DYou drive a @BDarkness Dragon Slash@D across @C$N's@D head!@n",
        target = "@c$n@D drives a @BDarkness Dragon Slash@D across your head!@n",
        room   = "@c$n@D drives a @BDarkness Dragon Slash@D across @C$N's@D head!@n",
    },
    arm = {
        actor  = "@DYou slash through @C$N's@D arm with a @BDarkness Dragon Slash@D!@n",
        target = "@c$n@D slashes through your arm with a @BDarkness Dragon Slash@D!@n",
        room   = "@c$n@D slashes through @C$N's@D arm with a @BDarkness Dragon Slash@D!@n",
    },
    leg = {
        actor  = "@DYou slash through @C$N's@D leg with a @BDarkness Dragon Slash@D!@n",
        target = "@c$n@D slashes through your leg with a @BDarkness Dragon Slash@D!@n",
        room   = "@c$n@D slashes through @C$N's@D leg with a @BDarkness Dragon Slash@D!@n",
    },
}

return {
    id     = "ddslash",
    family = "ki",
    name   = "Darkness Dragon Slash",
    skill  = "darkness dragon slash",
    tier   = 3,
    elements = { ki = 1.0 },
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
    base_power    = 1.5,

    on_check = function(inst)
        local ch = inst.attacker
        if ch:condition_has("mystic_melody") then
            return false, "Your mystic melody prevents you from using darkness dragon slash."
        end
        local grav_ok, grav_msg = ki.can_grav(ch)
        if not grav_ok then return false, grav_msg end
        if ch:wielded_weapon_type() ~= "slash" then
            ch:send_line("You need to be wielding a slashing weapon to use darkness dragon slash.")
            return false
        end
        return true
    end,

    on_calculate_damage = function(inst)
        local sk  = inst.skill_level
        local atk = require("lua.libs.attack")
        local base = atk._base_damage(inst.attacker, inst.def, inst)
        local bonus = sk >= 100 and 0.15 or sk >= 60 and 0.10 or sk >= 40 and 0.05 or 0
        return math.floor(base * (1 + bonus))
    end,

    on_hit = function(inst)
        local ch     = inst.attacker
        local target = inst.target
        local ctx    = { actor = ch, target = target }
        local dbat   = require("dbat")
        local loc    = inst.hit_location

        act().message(HIT_MSGS[loc] or HIT_MSGS.body, ctx)

        -- Head: instakill if dmg > PL/5 and not majin/bio
        if loc == "head" then
            local race = target:race_get()
            if race ~= "majin" and race ~= "bio" then
                if inst.damage > math.floor(target:meter_max("powerlevel") / 5) then
                    target:meter_mod_int("powerlevel", -target:meter_max("powerlevel"))
                    act().message({
                        actor  = "@DYour Darkness Dragon Slash severs @C$N's@D head!@n",
                        target = "@D@c$n@D's Darkness Dragon Slash severs your head!@n",
                        room   = "@D@c$n@D's Darkness Dragon Slash severs @C$N's@D head!@n",
                    }, ctx)
                    return
                end
            end
        end

        -- 33% chance to blind via darkness_dragon_slash condition (1 mud-hour)
        if math.random(3) == 1 then
            target:condition_apply_with_duration(
                "darkness_dragon_slash", "skill", "darkness_dragon_slash",
                dbat.consts.secs_per_mud_hour
            )
            act().message({
                actor  = "@DDark energy blinds @C$N@D!@n",
                target = "@DDark energy from @c$n@D's slash blinds you!@n",
                room   = "@DDark energy blinds @C$N@D from @c$n@D's slash!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@DYou swing your @BDarkness Dragon Slash@D but miss @C$N@D!@n",
            target = "@c$n@D swings a @BDarkness Dragon Slash@D but misses you!@n",
            room   = "@c$n@D swings a @BDarkness Dragon Slash@D but misses @C$N@D!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@D dodges your Darkness Dragon Slash!@n",
            target = "@WYou dodge @c$n@D's Darkness Dragon Slash!@n",
            room   = "@C$N@D dodges @c$n@D's Darkness Dragon Slash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@D blocks your Darkness Dragon Slash!@n",
            target = "@WYou block @c$n@D's Darkness Dragon Slash!@n",
            room   = "@C$N@D blocks @c$n@D's Darkness Dragon Slash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
