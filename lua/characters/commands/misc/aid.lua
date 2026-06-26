local dbat = require("dbat")
local PLR  = dbat.consts.player_flags
local AFF  = dbat.consts.aff_flags
local BON  = dbat.consts.bonuses
local PULSE_3SEC = 30
local SEARCH_WORKING_GENUINE = 3  -- not broken, not forged

local function execute(ctx)
    local ch   = ctx.ch
    local arg  = string.lower(ctx.argparams.tokens[1] or "")
    local arg2 = ctx.argparams.tokens[2] or ""

    if ch:is_npc() then return end

    if arg == "" then
        ch:send_line("Syntax: aid heal (target)")
        ch:send_line("        aid adrenex")
        ch:send_line("        aid antitoxin")
        ch:send_line("        aid salve")
        ch:send_line("        aid formula-82")
        return
    end

    -- Determine which item to look for
    local item_vnum = 47   -- bandages
    local prod_vnum = 0    -- no crafted product (heal mode)
    if     arg == "adrenex"    then item_vnum = 380; prod_vnum = 381
    elseif arg == "antitoxin"  then item_vnum = 380; prod_vnum = 383
    elseif arg == "salve"      then item_vnum = 380; prod_vnum = 382
    elseif arg == "formula-82" then item_vnum = 380; prod_vnum = 385
    end

    local aid_obj = ch:inventory_find_vnum(item_vnum, SEARCH_WORKING_GENUINE)
    if not aid_obj then
        if item_vnum == 47 then
            ch:send_line("You need bandages to be able to use first aid.")
        else
            ch:send_line("You need a TCX-Medical Equipment Construction Kit.")
        end
        return
    end

    local skill = ch:skill_get("first aid")

    if item_vnum == 47 then
        -- Heal mode
        local vict = dbat.search.find_char_in_room(ch, arg2)
        if not vict then
            ch:send_line("Apply first aid to who?")
            return
        end
        if vict:is_npc() then
            ch:send_line("What ever for?")
            return
        end
        if vict:race_get() == "android" then
            ch:send_line("They are an android!")
            return
        end

        if not vict:aff_flagged(AFF.SPIRIT) and not vict:player_flagged(PLR.BANDAGED) then
            if vict ~= ch then
                ch:send_line("You attempt to lend first aid to %s.", vict:name_get())
            end
            ch:act("$n attempts to bandage $N's wounds.", true, nil, vict, "room")

            if (skill + 1) > dbat.axion_dice(0) then
                ch:send_line("You bandage %s's wounds.", vict:name_get())
                local pl_max = vict:meter_max("powerlevel")
                local roll   = math.floor(pl_max / 100) * math.floor(ch:stat_get("wisdom") / 4)
                             + math.floor(pl_max * 0.25)
                if ch:bonus_flagged(BON.HEALER) then
                    roll = math.floor(roll + roll * 0.1)
                end
                vict:meter_mod_int("powerlevel", math.floor(roll))
                vict:send_line("Your wounds are bandaged by %s!", ch:name_get())
                vict:act("$n's wounds are stablized by $N!", true, nil, ch, "notvict")
                vict:player_flag_set(PLR.BANDAGED, true)
                aid_obj:extract()
            else
                if vict ~= ch then
                    ch:send_line("You fail to bandage their wounds properly, wasting the set of bandages...")
                    vict:act("$N fails to bandage $n's wounds properly, wasting an entire set of bandages...", true, nil, ch, "notvict")
                    vict:act("$N fails to bandage your wounds properly, wasting an entire set of bandages...", true, nil, ch, "char")
                else
                    vict:act("$N fails to bandage $s wounds properly, wasting an entire set of bandages...", true, nil, ch, "notvict")
                    vict:act("You fail to bandage your wounds properly, wasting an entire set of bandages...", true, nil, ch, "vict")
                end
                aid_obj:extract()
            end
            ch:improve_skill("first aid", 1)
        elseif vict:player_flagged(PLR.BANDAGED) then
            ch:send_line("They are already bandaged!")
        elseif vict:aff_flagged(AFF.SPIRIT) then
            ch:send_line("The dead don't need first aid.")
        else
            ch:send_line("They apparently do not need bandaging.")
        end

    elseif prod_vnum == 381 then
        -- Adrenex
        if skill < 65 then
            ch:send_line("You need at least a skill level of 65 in First Aid.")
            return
        end
        if skill < dbat.axion_dice(15) then
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you begin to construct an Andrenex Adreneline Injector you screw up and end up breaking the water tight seal. The adreneline leaks out and is wasted.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, nil, nil, "room")
            aid_obj:extract()
        else
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully construct an Adrenex Adreneline Injector@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Adrenex Adreneline Injector!@n", true, nil, nil, "room")
            local prod = dbat.read_object(prod_vnum)
            prod:to_char(ch)
            aid_obj:extract()
            ch:improve_skill("first aid", 1)
        end

    elseif prod_vnum == 382 then
        -- Salve
        if skill < 50 then
            ch:send_line("You need at least a skill level of 50 in First Aid.")
            return
        end
        if skill < dbat.axion_dice(10) then
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you go to put the salve ingredients into the kit's salve compartment and set the temperature you accidentally set it too high. The salve is burned and ruined. Yes you managed to burn a burn salve.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, nil, nil, "room")
            aid_obj:extract()
        else
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully boil a burn salve to perfection and it is automatically placed in a jar.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a jar of burn salve!@n", true, nil, nil, "room")
            local prod = dbat.read_object(prod_vnum)
            prod:to_char(ch)
            aid_obj:extract()
            ch:improve_skill("first aid", 1)
        end

    elseif prod_vnum == 383 then
        -- Antitoxin
        if skill < 40 then
            ch:send_line("You need at least a skill level of 40 in First Aid.")
            return
        end
        if skill < dbat.axion_dice(5) then
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete the Antitoxin Injector you notice that you didn't seal the syringe properly and it all leaks out.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, nil, nil, "room")
            aid_obj:extract()
        else
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble the Antitoxin Injector.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Antitoxin Injector!@n", true, nil, nil, "room")
            local prod = dbat.read_object(prod_vnum)
            prod:to_char(ch)
            aid_obj:extract()
            ch:improve_skill("first aid", 1)
        end

    elseif prod_vnum == 385 then
        -- Formula-82
        if skill < 40 then
            ch:send_line("You need at least a skill level of 40 in First Aid.")
            return
        end
        if skill < dbat.axion_dice(15) then
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete a vial of Formula 82 you notice that you read the mixture measurements wrong. You dispose of the vile vial immediately.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", true, nil, nil, "room")
            aid_obj:extract()
        else
            ch:act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble a vial of Formula 82.@n", true, nil, nil, "char")
            ch:act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Vial of Formula 82!@n", true, nil, nil, "room")
            local prod = dbat.read_object(prod_vnum)
            prod:to_char(ch)
            aid_obj:extract()
            ch:improve_skill("first aid", 1)
        end
    end

    ch:wait_set(PULSE_3SEC)
end

return {
    id      = "aid",
    aliases = { {"aid", 3} },
    execute = execute,
}
