local dbat   = require("dbat")
local MOB    = dbat.consts.mob_flags
local PLR    = dbat.consts.plr_flags
local EF     = dbat.consts.item_extra_flags
local ITEM   = dbat.consts.item_types
local POS    = dbat.consts.positions
local Search = dbat.lib.search.new
local text   = dbat.lib.text

local PULSE_3SEC = 30

local function gold_carry(ch)
    local level = ch:stat_get("level")
    if level < 50  then return level * 10000 end
    if level < 100 then return 500000 end
    return 50000000
end

local function find_equipped(viewer, owner, needle)
    local needle_lower = needle:lower()
    for pos, eq_obj in owner:equipment() do
        if viewer:can_see_obj(eq_obj) then
            for _, kw in ipairs(eq_obj:keywords_for(viewer)) do
                local kw_lower = kw:lower()
                if kw_lower == needle_lower or kw_lower:sub(1, #needle_lower) == needle_lower then
                    return eq_obj, pos
                end
            end
        end
    end
    return nil, nil
end

local function execute(ctx)
    local ch   = ctx.ch
    local arg1 = ctx.argparams.tokens[1] or ""
    local arg2 = ctx.argparams.tokens[2] or ""

    if not ch:know_skill("sleight_of_hand") then
        if ch:slot_count() + 1 <= ch:der_total("skill_slots") then
            ch:send_line("You learn the very veeeery basics of theft. Which is don't get caught.")
            ch:skill_base_set("sleight_of_hand", 1)
        else
            ch:send_line("You can't learn any more skills and thus can not steal right now!")
            return
        end
    end

    if ch:condition_has("mystic_melody") then
        ch:send_line("You are currently playing a song! Enter the song command in order to stop!")
        return
    end

    if arg1 == "" then
        ch:send_line("An important basic of theft is actually having a victim!")
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg2)
    if not vict then
        ch:send_line("Steal what from who?")
        return
    end

    if vict:is_same(ch) then
        ch:send_line("Come on now, that's rather stupid!")
        return
    end

    if not ch:can_kill(vict) then return end

    if ch:stat_get("level") <= 8 then
        ch:send_line("You are trapped inside the newbie shield until level 9 and can't piss off those bigger and better than you. Awww...")
        return
    end

    if vict:is_npc() and vict:mob_flagged(MOB.NOKILL) and ch:admin_level_get() == 0 then
        ch:send_line("That isn't such a good idea...")
        return
    end

    local st     = ch:meter_current("stamina")
    local st_max = ch:meter_max("stamina")
    if st < math.floor(st_max / 40) + ch:carry_weight_get() then
        ch:send_line("You do not have enough stamina.")
        return
    end

    local prob, perc
    if not vict:is_npc() and vict:skill_get("spot") > 0 then
        perc = vict:skill_get("spot") + vict:stat_get("intelligence")
    else
        perc = math.random(vict:stat_get("intelligence"), vict:stat_get("intelligence") + 10)
        if vict:is_npc() then
            perc = perc + math.floor(vict:stat_get("level") * 0.25)
        end
    end

    local vict_pos = vict:position_get()
    if     vict_pos == POS.SITTING  then perc = perc - 5
    elseif vict_pos == POS.RESTING  then perc = perc - 10
    elseif vict_pos <= POS.SLEEPING then perc = perc - 25
    end

    prob = ch:skill_get("sleight_of_hand") + ch:stat_get("dexterity")

    perc = perc + math.random(-5, 5)
    prob = prob + math.random(-5, 5)

    if dbat.axion_dice(0) > 100 and vict_pos ~= POS.SLEEPING then
        ch:reveal_hiding(0)
        ch:act("@r$N@R just happens to glance in your direction! What terrible luck!@n", true, nil, vict, "char")
        ch:act("@RYou just happen to glance behind you and spot @r$n@R trying to STEAL from you!@n", true, nil, vict, "vict")
        ch:act("@r$N@R just happens to glance in @r$n's@R direction and catches $m trying to STEAL!@n", true, nil, vict, "notvict")
        prob = -1000
    end

    if prob + 20 < perc and vict_pos ~= POS.SLEEPING then
        ch:reveal_hiding(0)
        ch:act("@rYou are caught trying to stick your hand in @R$N's@r possessions!@n", true, nil, vict, "char")
        ch:act("@rYou catch @R$n@r trying to rummage through your possessions!@n", true, nil, vict, "vict")
        ch:act("@R$n@R is caught by @R$N@r as $e sticks $s hand in @R$N's@r possessions!@n", true, nil, vict, "notvict")
        ch:wait_set(PULSE_3SEC)
        if vict:is_npc() then vict:start_fighting(ch) end
        vict:improve_skill("spot", 1)
        return
    end

    if arg1:lower() == "zenni" then
        if prob > perc then
            local gold = vict:stat_get("money")
            if gold <= 0 then
                ch:send_line("It appears like they are broke...")
                return
            end
            if gold > 100 then
                gold = math.floor(gold / 100) * math.random(1, 10)
            end
            if gold + ch:stat_get("money") > gold_carry(ch) then
                ch:send_line("You can't hold that much more zenni on your person!")
                return
            end
            vict:stat_mod("money", -gold)
            ch:stat_mod("money", gold)
            if not vict:is_npc() then
                vict:player_flag_set(PLR.STOLEN, true)
                dbat.send_to_imm(string.format("THEFT: %s has stolen %s zenni from %s",
                    ch:name_get(), text.add_commas(gold), vict:name_get()))
            end
            if gold > 1 then
                ch:send_line("Bingo!  You got %d zenni.", gold)
            else
                ch:send_line("You manage to swipe a solitary zenni.")
            end
            if dbat.axion_dice(0) > prob then
                ch:send_line("You think that your movements might have been a bit obvious.")
                ch:reveal_hiding(0)
                ch:act("@R$n@r just stole zenni from @R$N@r!@n", true, nil, vict, "room")
                vict:send_line("You feel like something may be missing...")
                if vict:is_npc() and math.random(1, 3) == 3 then
                    vict:start_fighting(ch)
                end
                vict:improve_skill("spot", 1)
            end
            ch:improve_skill("sleight_of_hand", 1)
        else
            ch:reveal_hiding(0)
            ch:act("@rYou are caught trying to steal zenni from @R$N@r!@n", true, nil, vict, "char")
            ch:act("@rYou catch @R$n's@r hand trying to snatch your zenni!@n", true, nil, vict, "vict")
            ch:act("@R$N@r catches @R$n's@r hand trying to snatch $S zenni!@n", true, nil, vict, "notvict")
            ch:wait_set(PULSE_3SEC)
            if vict:is_npc() then vict:start_fighting(ch) end
            ch:improve_skill("sleight_of_hand", 2)
            vict:improve_skill("spot", 1)
        end
        return
    end

    -- Object steal: search inventory first
    local obj = Search(ch):add_character_inventory(vict):find_one(arg1)
    local eq_pos = nil

    if not obj then
        obj, eq_pos = find_equipped(ch, vict, arg1)
        if not obj then
            ch:act("$E isn't wearing that item.", false, nil, vict, "char")
            return
        end
        -- Stealing equipped item
        if vict_pos > POS.SLEEPING then
            ch:send_line("Steal worn equipment from them while they are awake? That's a stupid idea...")
            return
        end
        local vnum = obj:vnum_get()
        if vnum >= 20000 or (vnum >= 18800 and vnum <= 18999) or (vnum >= 19100 and vnum <= 19199) then
            ch:send_line("You can't steal that!")
            return
        end
        if obj:type_get() == ITEM.KEY then
            ch:send_line("No stealing keys!")
            return
        end
        if obj:extra_flagged(EF.NOSTEAL) then
            ch:send_line("You can't steal that!")
            return
        end
        if obj:weight_get() + ch:carry_weight_get() > ch:carry_weight_max() then
            ch:send_line("You can't carry that much weight.")
            return
        end
        if ch:inventory_count() + 1 > 50 then
            ch:send_line("You don't have the room for it right now!")
            return
        end
        if prob > perc then
            ch:act("You unequip $p and steal it.", false, obj, vict, "char")
            if dbat.axion_dice(0) > prob then
                ch:send_line("You think that your movements might have been a bit obvious.")
                ch:reveal_hiding(0)
                ch:act("@R$n@r just stole $p@r from @R$N@r!@n", true, obj, vict, "room")
                vict:send_line("You feel your body being disturbed.")
                vict:improve_skill("spot", 1)
            end
            local stolen = vict:unequip(eq_pos)
            if stolen then stolen:to_char(ch) end
            ch:improve_skill("sleight_of_hand", 1)
        else
            ch:reveal_hiding(0)
            vict:position_set(POS.SITTING)
            ch:act("@rYou are caught trying to steal $p@r from @R$N@r!@n", true, obj, vict, "char")
            ch:act("@rYou feel your body being shifted while you sleep and wake up to find @R$n@r trying to steal $p@r from you!@n", true, obj, vict, "vict")
            ch:act("@R$N@r catches @R$n's@r trying to $p@r from $M during $S sleep!@n", true, obj, vict, "notvict")
            ch:wait_set(PULSE_3SEC)
            if vict:is_npc() then
                vict:position_set(POS.STANDING)
                vict:start_fighting(ch)
            end
            ch:improve_skill("sleight_of_hand", 2)
            vict:improve_skill("spot", 1)
        end
        return
    end

    -- Stealing from inventory
    local vnum = obj:vnum_get()
    if vnum >= 20000 then
        ch:send_line("You can't steal that!")
        return
    end
    if obj:extra_flagged(EF.NOSTEAL) then
        ch:send_line("You can't steal that!")
        return
    end
    if obj:type_get() == ITEM.KEY then
        ch:send_line("No stealing keys!")
        return
    end
    if obj:weight_get() + ch:carry_weight_get() > ch:carry_weight_max() then
        ch:send_line("You can't carry that much weight.")
        return
    end
    if ch:inventory_count() + 1 > 50 then
        ch:send_line("You don't have the room for it right now!")
        return
    end

    if prob > perc then
        ch:act("You steal $p from $N.", false, obj, vict, "char")
        obj:from_char()
        obj:to_char(ch)
        if not vict:is_npc() then
            vict:player_flag_set(PLR.STOLEN, true)
            dbat.send_to_imm(string.format("THEFT: %s has stolen %s from %s",
                ch:name_get(), obj:short_description_get(), vict:name_get()))
        end
        if dbat.axion_dice(0) > prob then
            ch:reveal_hiding(0)
            ch:send_line("You think that your movements might have been a bit obvious.")
            ch:act("@R$n@r just stole $p@r from @R$N@r!@n", true, obj, vict, "room")
            vict:send_line("You feel like something may be missing...")
            if vict:is_npc() and math.random(1, 3) == 3 then
                vict:start_fighting(ch)
            end
            vict:improve_skill("spot", 1)
        end
        ch:improve_skill("sleight_of_hand", 1)
    else
        ch:reveal_hiding(0)
        ch:act("@rYou are caught trying to steal $p@r from @R$N@r!@n", true, obj, vict, "char")
        ch:act("@rYou catch @R$n@r trying to steal $p@r from you!@n", true, obj, vict, "vict")
        ch:act("@R$N@r catches @R$n's@r trying to $p@r!@n", true, obj, vict, "notvict")
        ch:wait_set(PULSE_3SEC)
        if vict:is_npc() then
            vict:position_set(POS.STANDING)
            vict:start_fighting(ch)
        end
        ch:improve_skill("sleight_of_hand", 2)
        vict:improve_skill("spot", 1)
    end
end

return {
    id      = "steal",
    aliases = { {"steal", 3} },
    execute = execute,
}
