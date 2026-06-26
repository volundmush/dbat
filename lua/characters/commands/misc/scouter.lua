local dbat   = require("dbat")
local RF     = dbat.consts.room_flags
local EF     = dbat.consts.item_extra_flags
local WEAR   = dbat.consts.wear_positions
local PLR    = dbat.consts.plr_flags
local BFS    = dbat.consts.bfs
local text   = dbat.lib.text

local PLANET_FLAGS = {
    RF.FRIGID, RF.EARTH, RF.NAMEK, RF.VEGETA, RF.AETHER,
    RF.KONACK, RF.KANASSA, RF.YARDRAT, RF.AL, RF.HELL,
    RF.ARLIA, RF.NEO, RF.CERRIA,
}

local function planet_check(ch, vict)
    if vict:admin_level_get() > 0 then return false end
    local r1 = ch:room_get()
    local r2 = vict:room_get()
    for _, flag in ipairs(PLANET_FLAGS) do
        if r1:flagged(flag) and r2:flagged(flag) then return true end
    end
    return ch:is_planet_zenith() and vict:is_planet_zenith()
end

local function stam_label(cur, max)
    if max <= 0 then max = 1 end
    local pct = cur / max * 100
    if pct < 10  then return "Exhausted"
    elseif pct < 25  then return "Extremely Tired"
    elseif pct < 50  then return "Very Tired"
    elseif pct < 75  then return "Tired"
    elseif pct < 90  then return "Winded"
    elseif pct < 100 then return "Untired"
    else return "Energetic"
    end
end

local function scouter_overloads(obj, pl)
    if obj:extra_flagged(EF.BSCOUTER) and pl >= 150000   then return true end
    if obj:extra_flagged(EF.MSCOUTER) and pl >= 5000000  then return true end
    if obj:extra_flagged(EF.ASCOUTER) and pl >= 15000000 then return true end
    return false
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if not ch:has_arms() then
        ch:send_line("You have no available arms!")
        return
    end

    local obj = ch:equipment_get(WEAR.EYE)
    if not obj then
        ch:send_line("You do not even have a scouter!")
        return
    end

    if arg == "" then
        ch:send_line("[Syntax] scouter < target | scan>")
        return
    end

    ch:reveal_hiding(3)

    if arg:lower() == "scan" then
        local count = 0
        for vict in dbat.characters.all() do
            if vict:is_same(ch)   then goto continue end
            if vict:is_npc()      then goto continue end
            if vict:race_get() == "android" then goto continue end
            if not planet_check(ch, vict)   then goto continue end

            local vict_room = vict:room_get()
            local my_room   = ch:room_get()
            local dir       = my_room:find_first_step(vict_room)
            local same_zone = (function()
                local z1 = ch:zone_get()
                local z2 = vict:zone_get()
                return z1 and z2 and z1:is_same(z2)
            end)()

            local pathway
            if dir == BFS.ERROR then
                pathway = "@rERROR"
            elseif dir == BFS.ALREADY_THERE then
                pathway = "@RHERE"
            elseif dir == BFS.NO_PATH then
                pathway = "@MUNKNOWN"
            else
                pathway = "@G" .. (dbat.consts.direction_names[dir + 1] or "?")
            end

            local location = same_zone and pathway or (vict:sense_location() or pathway)
            local pl = vict:meter_current("powerlevel")

            if scouter_overloads(obj, pl) then
                ch:send_line("@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n", location)
            else
                ch:send_line("@D<@GPowerlevel Detected@D: [@Y%s@D]@w ---> @C%s@n",
                    text.add_commas(pl), location)
            end
            count = count + 1
            ::continue::
        end
        if count == 0 then
            ch:send_line("You didn't detect anyone of notice.")
        else
            ch:send_line("%d powerlevels detected.", count)
        end
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("They don't seem to be here.")
        return
    end

    if vict:race_get() == "android" then
        ch:act("$n points $s scouter at you.", false, nil, vict, "vict")
        ch:act("$n points $s scouter at $N.", false, nil, vict, "notvict")
        ch:send_line("@D,==================================|@n")
        ch:send_line("@D|@1                                  @n@D|@n")
        ch:send_line("@D|@1@RReading target...                 @n@D|@n")
        ch:send_line("@D|@1                                  @n@D|@n")
        ch:send_line("@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D:               @RERROR@n@D|@n")
        ch:send_line("@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D:                 @RERROR@n@D|@n")
        ch:send_line("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D:                 @RERROR@n@D|@n")
        ch:send_line("@D|@1                                  @n@D|@n")
        ch:send_line("@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D:                 @RERROR@n@D|@n")
        ch:send_line("@D|@1                                  @n@D|@n")
        ch:send_line("@D`==================================|@n")
        return
    end

    local pl = vict:meter_current("powerlevel")

    if scouter_overloads(obj, pl) then
        ch:act("$n points $s scouter at you.", false, nil, vict, "vict")
        ch:act("$n points $s scouter at $N.", false, nil, vict, "notvict")
        ch:unequip(WEAR.EYE)
        ch:send_line("Your scouter overloads and explodes!")
        ch:act("$n's scouter explodes!", false, nil, nil, "room")
        obj:extract()
        ch:rp_save()
        return
    end

    local stam     = math.max(1, vict:meter_current("stamina"))
    local stam_max = math.max(1, vict:meter_max("stamina"))

    local charged_ki
    if not vict:is_npc() then
        charged_ki = text.add_commas(vict:charge_get())
    else
        local mobcharge = vict:condition_number_get("fighting", "mobcharge")
        local lvl = ch:stat_get("level")
        charged_ki = text.add_commas(mobcharge * math.random(lvl * 50, lvl * 200))
    end

    ch:act("$n points $s scouter at you.", false, nil, vict, "vict")
    ch:act("$n points $s scouter at $N.", false, nil, vict, "notvict")

    ch:send_line("@D,==================================|@n")
    ch:send_line("@D|@1                                  @n@D|@n")
    ch:send_line("@D|@1@RReading target...                 @n@D|@n")
    ch:send_line("@D|@1                                  @n@D|@n")
    ch:send_line("@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D: @Y%21s@n@D|@n",
        text.add_commas(pl))
    ch:send_line("@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n", charged_ki)
    ch:send_line("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n",
        stam_label(stam, stam_max))
    ch:send_line("@D|@1                                  @n@D|@n")

    -- Extra Info section
    local infos = {}
    if vict:condition_has("zanzoken")     then infos[#infos+1] = "Zanzoken Prepared" end
    if vict:condition_has("hasshuken")    then infos[#infos+1] = "Accelerated Arms" end
    if vict:condition_has("healing_glow") then infos[#infos+1] = "Healing Glow Prepared" end
    if vict:condition_has("poison")       then infos[#infos+1] = "Poisoned" end
    if vict:player_flagged(PLR.SELFD)     then infos[#infos+1] = "Explosive Energy" end

    ch:send_line("@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D: @Y%21s@n@D|@n",
        infos[1] or "None Detected.")
    for i = 2, #infos do
        ch:send_line("@D|@1             @Y%21s@n@D|@n", infos[i])
    end

    ch:send_line("@D|@1                                  @n@D|@n")
    ch:send_line("@D`==================================|@n")
end

return {
    id      = "scouter",
    aliases = { {"scouter", 3} },
    execute = execute,
}
