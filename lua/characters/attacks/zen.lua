local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local BASE_MSG = {
    actor  = "@CRaising your blade above your head, and closing your eyes, you focus ki into its edge. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as you open your eyes and instantly move past @g$N's@C body while slashing with the pure energy of your resolve! A large explosion of energy erupts across $S %s!@n",
    target = "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past YOUR body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across YOUR %s!@n",
    room   = "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past @g$N's@C body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across @g$N's@C %s!@n",
}

local function zen_msg(part)
    return {
        actor  = BASE_MSG.actor:format(part),
        target = BASE_MSG.target:format(part),
        room   = BASE_MSG.room:format(part),
    }
end

return {
    id     = "zen",
    family = "ki",
    name   = "Zen Blade Strike",
    skill  = "zen blade strike",
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
    base_power    = 2.0,

    on_check = function(inst)
        local ch = inst.attacker
        if ch:condition_has("mystic_melody") then
            return false, "You are currently playing a song! Enter the song command in order to stop!"
        end
        local ok, msg = ki.can_grav(ch)
        if not ok then return false, msg end
        if ch:wielded_weapon_type() ~= "slash" then
            return false, "You are not wielding a sword, you need one to use this technique."
        end
        return true
    end,

    on_calculate_cost = function(inst)
        local perf = inst.attacker:skill_perf_get(inst.def.skill)
        if perf == 1 then
            inst.cost.ki = math.floor(inst.cost.ki * 1.05)
        elseif perf == 3 then
            inst.cost.ki = math.max(1, math.floor(inst.cost.ki * 0.95))
        end
    end,

    on_modify_accuracy = function(inst)
        local perf = inst.attacker:skill_perf_get(inst.def.skill)
        if perf == 2 then
            inst.accuracy_modifier = inst.accuracy_modifier + 5
        end
    end,

    on_hit = function(inst)
        local ch     = inst.attacker
        local target = inst.target
        local loc    = inst.hit_location

        if loc == "body" then
            act().message(zen_msg("body"), { actor = ch, target = target })
            if target:has_tail() and not inst.spar then
                act().message({
                    actor  = "@rYou cut off $S tail!@n",
                    target = "@rYour tail is cut off!@n",
                    room   = "@R$N@r's tail is cut off!@n",
                }, { actor = ch, target = target })
                target:lose_tail()
            end

        elseif loc == "head" then
            act().message(zen_msg("head"), { actor = ch, target = target })
            if inst.damage > math.floor(target:meter_max("powerlevel") / 5) then
                local race = target:race_get()
                if race == "majin" or race == "bio" then
                    local regen = target:skill_get("regenerate") or 0
                    local ki_cost = math.floor(target:meter_max("ki") / 40)
                    if regen > math.random(1, 101) and target:meter_get("ki") >= ki_cost then
                        act().message({
                            actor  = "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n",
                            target = "@rYou have your head cut off by the attack but regenerate a moment later!@n",
                            room   = "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n",
                        }, { actor = ch, target = target })
                        target:meter_mod_int("ki", -ki_cost)
                    else
                        act().message({
                            actor  = "@R$N@r has $S head cut off by the attack!@n",
                            target = "@rYou have your head cut off by the attack!@n",
                            room   = "@R$N@r has $S head cut off by the attack!@n",
                        }, { actor = ch, target = target })
                        inst.damage = target:meter_max("powerlevel") * 10
                    end
                else
                    act().message({
                        actor  = "@R$N@r has $S head cut off by the attack!@n",
                        target = "@rYou have your head cut off by the attack!@n",
                        room   = "@R$N@r has $S head cut off by the attack!@n",
                    }, { actor = ch, target = target })
                    inst.damage = target:meter_max("powerlevel") * 10
                end
            end

        elseif loc == "arm" then
            act().message(zen_msg("arm"), { actor = ch, target = target })
            if math.random(1, 100) >= 80 and not target:is_npc()
                    and not target:condition_has("barrier")
                    and not inst.spar then
                local limb
                if target:limbcond_get(2) > 0 and math.random(1, 2) == 2 then
                    limb = 2
                elseif target:limbcond_get(1) > 0 then
                    limb = 1
                end
                if limb then
                    local side = (limb == 2) and "left" or "right"
                    act().message({
                        actor  = "@RYour attack severs $N's " .. side .. " arm!@n",
                        target = "@R$n's attack severs your " .. side .. " arm!@n",
                        room   = "@R$N's " .. side .. " arm is severed in the attack!@n",
                    }, { actor = ch, target = target })
                    target:limbcond_set(limb, 0)
                    target:remove_limb(limb)
                end
            end

        elseif loc == "leg" then
            act().message(zen_msg("leg"), { actor = ch, target = target })
            if math.random(1, 100) >= 80 and not target:is_npc()
                    and not target:condition_has("barrier")
                    and not inst.spar then
                local limb
                if target:limbcond_get(4) > 0 and math.random(1, 2) == 2 then
                    limb = 4
                elseif target:limbcond_get(3) > 0 then
                    limb = 3
                end
                if limb then
                    local side = (limb == 4) and "left" or "right"
                    act().message({
                        actor  = "@RYour attack severs $N's " .. side .. " leg!@n",
                        target = "@R$n's attack severs your " .. side .. " leg!@n",
                        room   = "@R$N's " .. side .. " leg is severed in the attack!@n",
                    }, { actor = ch, target = target })
                    target:limbcond_set(limb, 0)
                    target:remove_limb(limb)
                end
            end
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Zen Blade Strike misses, flying through the air harmlessly!@n",
            target = "@C$n@W fires a Zen Blade Strike at you, but misses!@n",
            room   = "@c$n@W fires a Zen Blade Strike at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Zen Blade Strike, letting it slam into the surroundings!@n",
            target = "@WYou dodge @C$n's@W Zen Blade Strike, letting it slam into the surroundings!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Zen Blade Strike, letting it slam into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        local room = inst.attacker:room_get()
        if room then
            room:send_all("@wA bright explosion erupts from the impact!\r\n")
            if room:damage_get() <= 95 then room:damage_mod(5) end
        end
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W moves quickly and blocks your Zen Blade Strike!@n",
            target = "@WYou move quickly and block @C$n's@W Zen Blade Strike!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W Zen Blade Strike!@n",
        }, { actor = inst.attacker, target = inst.target })
        inst.damage = math.floor(inst.damage / 4)
    end,
}
