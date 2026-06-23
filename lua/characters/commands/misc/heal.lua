local dbat = require("dbat")

local function limb_ok_arm(ch)
    if ch:condition_has("mystic_melody") then
        ch:send_line("You are currently playing a song! Enter the song command in order to stop!")
        return false
    end
    if not ch:is_npc() then
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then
            ch:send_line("You have no available arms!")
            return false
        end
    end
    return true
end

local function same_group(a, b)
    local ma = a:following_get()
    local mb = b:following_get()
    return (ma and ma == b) or (mb and mb == a) or (ma and mb and ma == mb)
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if not ch:know_skill("heal") then return end
    if not limb_ok_arm(ch) then return end

    if arg == "" then
        ch:send_line("You want to heal WHO?")
        return
    end

    local vict = ch:acquire_room_target(arg)
    if not vict then
        ch:send_line("You do not see the target here.")
        return
    end

    local prob = ch:skill_get("heal")
    local max_ki = ch:meter_max("ki")
    local max_pl = vict:meter_max("powerlevel")
    local cost, heal

    if prob >= 100 then
        cost  = math.floor(max_ki / 20)
        heal  = math.floor(max_pl / 5)
    elseif prob >= 90 then
        cost  = math.floor(max_ki / 16)
        heal  = math.floor(max_pl / 10)
    elseif prob >= 75 then
        cost  = math.floor(max_ki / 14)
        heal  = math.floor(max_pl / 12)
    elseif prob >= 50 then
        cost  = math.floor(max_ki / 12)
        heal  = math.floor(max_pl / 15)
    elseif prob >= 25 then
        cost  = math.floor(max_ki / 10)
        heal  = math.floor(max_pl / 20)
    else
        cost  = math.floor(max_ki / 6)
        heal  = math.floor(max_pl / 20)
    end

    if ch:bonus_flagged(BON.HEALER) then
        heal = heal + math.floor(heal * 0.1)
    end

    if heal < max_pl then
        heal = heal + math.floor((heal / 100) * math.floor(ch:stat_get("wisdom") / 4))
    end

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki.")
        return
    end

    local cur_pl = vict:meter_current("powerlevel")
    local suppress = vict:stat_get("suppression")
    if cur_pl >= max_pl then
        if vict == ch then
            ch:send_line("You are already at full health.")
        else
            ch:send_line("They are already at full health.")
        end
        return
    end
    if suppress > 0 and cur_pl >= math.floor((max_pl / 100) * suppress) then
        ch:send_line("They are already at full health.")
        return
    end

    local bonus = math.floor(ch:stat_get("intelligence") / 2) + math.floor(ch:stat_get("wisdom") / 3)
    prob = prob + bonus
    local perc = math.random(0, 110)

    if prob < perc then
        if vict ~= ch then
            local actlib = dbat.lib.act
            actlib.message({
                actor  = "@WYou place your hands near @c$N@W, but fail to concentrate enough to heal them!@n",
                target = "@C$n@W places $s hands near you, but nothing happens!@n",
                room   = "@C$n@W places $s hands near @c$N@W, but nothing happens.",
            }, { actor = ch, target = vict })
        else
            ch:send_line("@WYou place your hands on your body, but fail to concentrate to heal yourself!@n")
            ch:act_around("@C$n@W places $s hands on $s body, but nothing happens.")
        end
        ch:meter_mod_int("ki", -cost)
        ch:improve_skill("heal", 0)
        ch:wait_set(20)
        return
    end

    local actlib = dbat.lib.act

    if vict ~= ch then
        if ch:bonus_flagged(BON.HEALER) then
            heal = heal + math.floor(heal * 0.25)
        end
        actlib.message({
            actor  = "@WYou place your hands near @c$N@W and an orange glow surrounds $M!@n",
            target = "@C$n@W places $s hands near you and an orange glow surrounds you!@n",
            room   = "@C$n@W places $s hands near @c$N@W and an orange glow surrounds $M.",
        }, { actor = ch, target = vict })
        ch:meter_mod_int("ki", -cost)
        vict:meter_mod("powerlevel", heal)

        local sensei = ch:sensei_get()
        if sensei == "nail" then
            local heal_skill = ch:skill_get("heal")
            local st_bonus
            if heal_skill >= 100 then
                st_bonus = math.floor(heal * 0.4)
            elseif heal_skill >= 60 then
                st_bonus = math.floor(heal * 0.2)
            elseif heal_skill >= 40 then
                st_bonus = math.floor(heal * 0.1)
            end
            if st_bonus and st_bonus > 0 then
                vict:meter_mod("stamina", st_bonus)
                vict:send_line("@GYou feel some of your stamina return as well!@n")
            end
        end

        vict:condition_remove("poison", "skill_heal")
        vict:condition_remove_tag("blind", "skill_heal")
        if vict:condition_has("burned") then
            vict:condition_remove("burned", "skill_heal")
        end

        vict:limbcond_set(1, 100)
        vict:limbcond_set(2, 100)
        vict:limbcond_set(3, 100)
        vict:limbcond_set(4, 100)

        if not vict:is_npc() then
            local race = vict:race_get()
            if not vict:player_flagged(PLR.TAIL) and (race == "bio-android" or race == "icer") then
                vict:gain_tail()
            end
            if not vict:player_flagged(PLR.STAIL) and (race == "saiyan" or race == "halfbreed") then
                vict:gain_tail()
            end
        end

        local max_lf = vict:meter_max("lifeforce")
        if vict:meter_current("lifeforce") <= math.floor(max_lf * 0.5) and vict:race_get() ~= "android" then
            vict:meter_mod("lifeforce", math.floor(max_lf * 0.35))
            vict:send_line("You feel that your lifeforce has recovered some!")
        end

        ch:improve_skill("heal", 0)

        if same_group(ch, vict) and sensei == "nail" and vict:race_get() == "namek" then
            local exp_needed = ch:level_exp(ch:stat_get("level") + 1) - ch:stat_get("experience")
            if exp_needed > 0 and vict:meter_current("powerlevel") <= math.floor(max_pl * 0.85) and math.random(1, 3) == 3 then
                ch:stat_mod("experience", math.floor(ch:level_exp(ch:stat_get("level") + 1) * 0.005))
            end
        end

        ch:wait_set(20)
    else
        if ch:bonus_flagged(BON.HEALER) then
            heal = heal + math.floor(heal * 0.25)
        end
        ch:send_line("@WYou place your hands on your body and an orange glow surrounds you!@n")
        ch:act_around("@C$n@W places $s hands on $s body and an orange glow surrounds $m.")
        ch:meter_mod_int("ki", -cost)
        vict:meter_mod("powerlevel", heal)

        local sensei = ch:sensei_get()
        if sensei == "nail" then
            local heal_skill = ch:skill_get("heal")
            local st_bonus
            if heal_skill >= 100 then
                st_bonus = math.floor(heal * 0.4)
            elseif heal_skill >= 60 then
                st_bonus = math.floor(heal * 0.2)
            elseif heal_skill >= 40 then
                st_bonus = math.floor(heal * 0.1)
            end
            if st_bonus and st_bonus > 0 then
                vict:meter_mod("stamina", st_bonus)
                vict:send_line("@GYou feel some of your stamina return as well!@n")
            end
        end

        vict:condition_remove("poison", "skill_heal")
        vict:condition_remove_tag("blind", "skill_heal")

        vict:limbcond_set(1, 100)
        vict:limbcond_set(2, 100)
        vict:limbcond_set(3, 100)
        vict:limbcond_set(4, 100)

        if not vict:is_npc() then
            local race = vict:race_get()
            if not vict:player_flagged(PLR.TAIL) and (race == "bio-android" or race == "icer") then
                vict:gain_tail()
            end
            if not vict:player_flagged(PLR.STAIL) and (race == "saiyan" or race == "halfbreed") then
                vict:gain_tail()
            end
        end

        ch:improve_skill("heal", 0)
        ch:wait_set(20)
    end
end

local function can_execute(ch)
    return ch:know_skill("heal")
end

return { id = "heal", aliases = { { "heal", 3 } }, execute = execute, can_execute = can_execute }
