local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local MISS_MSGS = {
    actor  = "@WYou quickly bring your hands in front of your body and cup them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as your ki is condensed between your hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Your concentration waivers however and the energy slips away from your control harmlessly!@n",
    target = "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Suddenly $s concentration seems to waiver and the energy slips away from $s control harmlessly!@n",
    room   = "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Suddenly $s concentration seems to waiver and the energy slips away from $s control harmlessly!@n",
}

local HIT_LAUNCH_MSGS = {
    actor  = "@WYou quickly bring your hands in front of your body and cup them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as your ki is condensed between your hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. You shout @r'@YLIGHT GRENADE@r'@W as the orb launches from your hands at @C$N@W!@n",
    target = "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. @C$n shouts @r'@YLIGHT GRENADE@r'@W as the orb launches from $s hands at YOU!@n",
    room   = "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. @C$n shouts @r'@YLIGHT GRENADE@r'@W as the orb launches from $s hands at @c$N@W!@n",
}

local function try_knockdown(person)
    if not person:condition_has("flying")
            and person:position_get() == require("dbat").consts.positions.STANDING
            and math.random(1, 4) == 4 then
        person:condition_apply("knocked", "combat", "stunned")
    end
end

return {
    id     = "lightgrenade",
    family = "ki",
    name   = "Light Grenade",
    skill  = "light grenade",
    tier   = 3,
    elements = { ki = 1.0 },
    consumes_charge = true,
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 3.5,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ch      = inst.attacker
        local target  = inst.target
        local dbat_m  = require("dbat")
        local AFF     = dbat_m.consts.aff_flags
        local MF      = dbat_m.consts.mob_flags
        local dodge_threshold = dbat_m.axion_dice(math.floor(inst.skill_level * 0.5))

        -- Primary target hit messages
        act().message(HIT_LAUNCH_MSGS, { actor = ch, target = target })
        act().message({
            actor  = "@R$N@r is hit by the light grenade which explodes all around $m!@n",
            target = "@RYou are hit by the light grenade which explodes all around you!@n",
            room   = "@R$N@r is hit by the light grenade which explodes all around $m!@n",
        }, { actor = ch, target = target })
        try_knockdown(target)

        -- AoE splash to bystanders
        local room = ch:room_get()
        if not room then return end

        for person in room:people() do
            if person:id_get() == ch:id_get() then goto continue end
            if person:id_get() == target:id_get() then goto continue end
            if person:mob_flagged(MF.NOKILL) then goto continue end
            if not person:is_npc() and person:aff_flagged(AFF.SPIRIT) then goto continue end
            if not person:is_npc() and (person:stat_get("level") or 0) <= 8 then goto continue end

            -- Full group ally filter for splash
            if person:condition_has("group") and not person:is_npc() then
                local their_master = person:following_get()
                local my_master    = ch:following_get()
                if their_master and their_master:id_get() == ch:id_get() then goto continue end
                if my_master and my_master:id_get() == person:id_get() then goto continue end
                if their_master and my_master and their_master:id_get() == my_master:id_get() then goto continue end
            end

            if not ch:fighting_get() then ch:start_fighting(person) end
            if not person:fighting_get() then person:start_fighting(ch) end

            -- Zanzoken / icer avoidance
            local icer_dodge = not person:is_npc() and person:race_get() == "icer" and math.random(1, 30) >= 28
            local has_zan = person:condition_has("zanzoken")
            local st_ok = person:meter_get("stamina") >= 1
            local not_sleeping = person:position_get() ~= dbat_m.consts.positions.SLEEPING
            if (icer_dodge or has_zan) and st_ok and not_sleeping then
                if has_zan then person:condition_remove("zanzoken") end
                person:meter_mod_int("stamina", -math.floor(person:meter_max("powerlevel") / 200))
                act().message({
                    actor  = "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n",
                    target = "@cYou disappear, avoiding the explosion before reappearing elsewhere!@n",
                    room   = "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n",
                }, { actor = ch, target = person })
                goto continue
            end

            -- Dodge check uses axion_dice threshold
            if (person:skill_get("dodge") or 0) > dodge_threshold then
                act().message({
                    actor  = "@c$N@W manages to escape the explosion!@n",
                    target = "@WYou manage to escape the explosion!@n",
                    room   = "@c$N@W manages to escape the explosion!@n",
                }, { actor = ch, target = person })
                goto continue
            end

            -- Bystander hit (half damage)
            act().message({
                actor  = "@R$N@r is caught by the light grenade's explosion!@n",
                target = "@RYou are caught by the light grenade's explosion!@n",
                room   = "@R$N@r is caught by the light grenade's explosion!@n",
            }, { actor = ch, target = person })
            try_knockdown(person)
            person:meter_mod_int("powerlevel", -math.max(0, math.floor(inst.damage * 0.5)))

            ::continue::
        end
    end,

    on_miss = function(inst)
        act().message(MISS_MSGS, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@c$N@W manages to dodge the light grenade!@n",
            target = "@WYou manages to dodge the light grenade!@n",
            room   = "@c$N@W manages to dodge the light grenade!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
