local dbat   = require("dbat")
local act    = dbat.lib.act
local search = dbat.lib.search

local PULSE_3SEC    = 30  -- 3 * PASSES_PER_SEC (10)
local SMH           = dbat.consts.secs_per_mud_hour

local function show_help(ch, skill)
    ch:send_line("Syntax: runic (target) (skill)")
    ch:send_line("@D----@GRunic Skills@D----@n")
    ch:send_line(string.format(
        "@Rkenaz%s%s%s%s%s%s@n",
        skill >= 40  and "\n@Galgiz"    or "",
        skill >= 40  and "\n@moagaz"    or "",
        skill >= 50  and "\n@CLaguz"    or "",
        skill >= 60  and "\n@Ywunjo"    or "",
        skill >= 80  and "\n@rpurisaz"  or "",
        skill >= 100 and "\n@mgebo"     or ""))
end

local function execute(ctx)
    local ch    = ctx.ch
    local tokens = ctx.argparams.tokens
    local arg    = string.lower(tokens[1] or "")
    local arg2   = string.lower(tokens[2] or "")
    local skill  = ch:skill_get("runic")

    if not ch:know_skill("runic") then
        ch:send_line("You do not know how to write down runes.")
        return
    end

    if ch:condition_has("fighting") then
        ch:send_line("You are too busy fighting to write runes!")
        return
    end

    if arg == "" or arg2 == "" then
        show_help(ch, skill)
        return
    end

    local bottle = search.new(ch)
        :add_character_inventory(ch)
        :add_filter(function(_, e) return e:vnum_get() == 3424 and e:value_get(6) > 0 end)
        :find_one("*")
    if not bottle then
        ch:send_line("You do not have a bottle with enough ink in it.")
        return
    end
    local amount = bottle:value_get(6)

    if not ch:inventory_find_vnum(3427) then
        ch:send_line("You do not have a brush!")
        return
    end

    local cost    = math.floor(ch:meter_max("ki") * 0.05)
    local inkcost = 0
    local bonus   = 0

    if ch:race_get() == "hoshijin" then
        bonus = 10
    else
        inkcost = inkcost + 2
    end

    local vict = ch:acquire_room_target(arg)
    if not vict then
        ch:send_line("You can't seem to find that person.")
        return
    end
    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to write runes.")
        return
    end

    -- Catastrophic failure: roll 1-in-5 chance when skill fails
    if skill + bonus < dbat.axion_dice(0) and math.random(1, 5) == 5 then
        act.to_actor(
            "@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up destroying the ink bottle!@n",
            { actor = ch })
        act.around(ch,
            "@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before a look of panic dons $s face. Just at that moment the bottle of ink explodes! Strange...@n",
            { actor = ch })
        bottle:extract()
        ch:improve_skill("runic", 1)
        ch:meter_mod_int("ki", -cost)
        ch:wait_set(PULSE_3SEC)
        return
    end

    -- Soft failure: evaporate some ink
    if skill + bonus < dbat.axion_dice(0) then
        ch:meter_mod_int("ki", -cost)
        act.to_actor(
            "@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up evaporating some ink!@n",
            { actor = ch })
        act.around(ch,
            "@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before some ink evaporates. Strange...@n",
            { actor = ch })
        ch:improve_skill("runic", 1)
        local new_ink = math.max(0, bottle:value_get(6) - math.random(1, 3))
        bottle:value_set(6, new_ink)
        ch:wait_set(PULSE_3SEC)
        return
    end

    local function ink_ok(needed)
        if amount < needed then
            ch:send_line(string.format(
                "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n",
                needed))
            return false
        end
        return true
    end

    local function spend_ink(needed)
        local remaining = bottle:value_get(6) - needed
        if remaining <= 0 then
            bottle:extract()
            local empty = dbat.obj_protos.by_id(3423):spawn()
            empty:to_char(ch)
        else
            bottle:value_set(6, remaining)
        end
    end

    local function apply_rune(rune_name, condition_id, duration, vict_msg)
        ch:meter_mod_int("ki", -cost)
        local ac = { actor = ch, target = vict }
        if vict == ch then
            act.to_actor(
                string.format(
                    "@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@C%s@D'@B rune on your skin!@n",
                    rune_name),
                ac)
            act.around(ch,
                string.format(
                    "@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@C%s@D'@B rune on $s skin.@n",
                    rune_name),
                ac)
        else
            act.to_actor(
                string.format(
                    "@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@C%s@D'@B rune on @b$N's@B skin!@n",
                    rune_name),
                ac)
            act.to_target(
                string.format(
                    "@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@C%s@D'@B rune on @RYOUR@B skin.@n",
                    rune_name),
                ac)
            act.to_room(
                string.format(
                    "@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@C%s@D'@B rune on @b$N's@B skin.@n",
                    rune_name),
                { actor = ch, target = vict, exclude = { ch, vict } })
        end
        ch:send_line(string.format("@D[@B%d@b ink used.@D]@n", inkcost))
        vict:send_line(vict_msg)
        if condition_id and duration > 0 then
            vict:condition_apply_with_duration(condition_id, "skill", "runic", duration * SMH)
        end
        spend_ink(inkcost)
    end

    local function calc_dur(mult)
        return math.max(1, math.floor(skill * mult))
    end

    if arg2 == "kenaz" then
        inkcost = inkcost + 1
        local dur = calc_dur(0.16)
        apply_rune("Kenaz", "rune_kenaz", dur,
            string.format("@GYou can now see in the dark! @D(@WLasts@D: @w%d@D)@n", dur))

    elseif arg2 == "algiz" then
        inkcost = inkcost + 2
        if not ink_ok(inkcost) then return end
        local dur = calc_dur(0.05)
        apply_rune("Algiz", "ethereal_armor", dur,
            string.format("@GYou now have Ethereal Armor! @D(@WLasts@D: @w%d@D)@n", dur))

    elseif arg2 == "oagaz" then
        inkcost = inkcost + 3
        if not ink_ok(inkcost) then return end
        local dur = calc_dur(0.04)
        apply_rune("Oagaz", "rune_oagaz", dur,
            string.format("@GYou now are protected by Ethereal Chains! @D(@WLasts@D: @w%d@D)@n", dur))

    elseif arg2 == "laguz" then
        inkcost = inkcost + 4
        if not ink_ok(inkcost) then return end
        local dur = calc_dur(0.04)
        apply_rune("Laguz", "rune_laguz", dur,
            string.format("@GYou now have water breathing! @D(@WLasts@D: @w%d@D)@n", dur))

    elseif arg2 == "wunjo" then
        inkcost = inkcost + 4
        if not ink_ok(inkcost) then return end
        local dur = calc_dur(0.08)
        apply_rune("Wunjo", "rune_wunjo", dur,
            string.format("@GYou are now blessed with a deeper understanding of things you experience! @D(@WLasts@D: @w%d@D)@n", dur))

    elseif arg2 == "purisaz" then
        inkcost = inkcost + 4
        if not ink_ok(inkcost) then return end
        local dur = calc_dur(0.06)
        apply_rune("Purisaz", "rune_purisaz", dur,
            string.format("@GYou feel as if your inner energy is more potent! @D(@WLasts@D: @w%d@D)@n", dur))

    elseif arg2 == "gebo" then
        inkcost = inkcost + 10
        if not ink_ok(inkcost) then return end
        ch:meter_mod_int("ki", -cost)
        local ac = { actor = ch, target = vict }
        if vict == ch then
            act.to_actor(
                "@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CGebo@D'@B rune on your skin! The rune flashes out of existence immediately!@n",
                ac)
            act.around(ch,
                "@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on $s skin. The rune flashes out of existence immediately!@n",
                ac)
        else
            act.to_actor(
                "@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CGebo@D'@B rune on @b$N's@B skin! The rune flashes out of existence immediately!@n",
                ac)
            act.to_target(
                "@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on @RYOUR@B skin. The rune flashes out of existence immediately!@n",
                ac)
            act.to_room(
                "@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on @b$N's@B skin. The rune flashes out of existence immediately!@n",
                { actor = ch, target = vict, exclude = { ch, vict } })
        end
        ch:send_line(string.format("@D[@B%d@b ink used.@D]@n", inkcost))
        vict:stat_mod("practices", 125)
        vict:send_line("@GYou feel like you've just gained a lot of knowledge. Now if only you could apply it. @D[@m+125 PS@D]@n")
        spend_ink(inkcost)

    else
        show_help(ch, skill)
        return
    end

    ch:improve_skill("runic", 1)
    ch:wait_set(PULSE_3SEC)
end

return {
    id      = "runic",
    aliases = { { "runic", 4 } },
    execute = execute,
}
