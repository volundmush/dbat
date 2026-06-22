local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou raise your hand and form a spinning @Ydisk of energy@W that slices into @C$N's@W body!@n",
        target = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into your body!@n",
        room   = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou raise your hand and form a spinning @Ydisk of energy@W that slices into @C$N's@W head!@n",
        target = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into your head!@n",
        room   = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou raise your hand and form a spinning @Ydisk of energy@W that slices into @C$N's@W arm!@n",
        target = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into your arm!@n",
        room   = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou raise your hand and form a spinning @Ydisk of energy@W that slices into @C$N's@W leg!@n",
        target = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into your leg!@n",
        room   = "@c$n@W raises $s hand and forms a spinning @Ydisk of energy@W that slices into @C$N's@W leg!@n",
    },
}

return {
    id     = "kienzan",
    family = "ki",
    name   = "Kienzan",
    skill  = "kienzan",
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

    on_calculate_damage = function(inst)
        -- Skill-based damage bonus: +25/15/5% at skill 100/60/40
        local sk = inst.skill_level
        local bonus = sk >= 100 and 0.25 or sk >= 60 and 0.15 or sk >= 40 and 0.05 or 0
        if bonus > 0 then
            inst.damage = math.floor(inst.damage * (1 + bonus))
        end
    end,

    on_hit = function(inst)
        local target = inst.target
        local ctx    = { actor = inst.attacker, target = target }

        -- Instant-kill: if damage exceeds 1/5 of target max HP, the disk cuts them in half.
        -- Majins and bios are immune (regeneration).
        local race = target:race_get()
        if inst.damage > target:meter_max("powerlevel") / 5
            and race ~= "majin" and race ~= "bio" then
            act().message({
                actor  = "@RYour kienzan disk slices @C$N@W clean in half!@n",
                target = "@R@c$n@R's kienzan disk slices you clean in half!@n",
                room   = "@R@c$n@R's kienzan disk slices @C$N@R clean in half!@n",
            }, ctx)
            -- Overwhelm remaining HP to guarantee death
            inst.damage = target:meter_current("powerlevel") + 99999
            return
        end

        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour kienzan disk misses @C$N@W and dissipates!@n",
            target = "@c$n@W hurls a kienzan disk at you but misses!@n",
            room   = "@c$n@W hurls a kienzan disk at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your kienzan disk!@n",
            target = "@WYou dodge @c$n@W's kienzan disk!@n",
            room   = "@C$N@W dodges @c$n@W's kienzan disk!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your kienzan!@n",
            target = "@WYou block @c$n@W's kienzan!@n",
            room   = "@C$N@W blocks @c$n@W's kienzan!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
