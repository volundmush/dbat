local dbat   = require("dbat")
local act    = dbat.lib.act

local APPLY = dbat.consts.applies
local EF    = dbat.consts.item_extra_flags

local function boost_tier(lvl)
    for _, t in ipairs({100, 90, 80, 70, 60, 50, 40, 30}) do
        if lvl >= t then return t end
    end
    return 0
end

local function boost_equipment(obj, ch, count)
    count = count or 1
    local lvl   = ch:stat_get("level")
    local boost = boost_tier(lvl)
    if boost == 0 then return end
    obj:level_set(boost)
    obj:affect_set(0, APPLY.KI, 0, count * boost * lvl)
    local loc2 = (obj:vnum_get() == 91) and APPLY.STR or APPLY.INT
    obj:affect_set(1, loc2, 0, math.floor(boost / 20))
end

local function conc_discount(ch)
    local conc = ch:skill_get("concentration")
    for _, e in ipairs({
        {100, 0.50}, {90, 0.60}, {80, 0.65}, {70, 0.70}, {60, 0.75},
        {50,  0.80}, {40, 0.85}, {30, 0.90}, {20, 0.95},
    }) do
        if conc >= e[1] then return e[2] end
    end
    return 1.0
end

local function spawn(vnum)
    return dbat.obj_protos.by_id(vnum):spawn()
end

local function finish_create(ch, obj, cost, to_room)
    if to_room then
        obj:to_room(ch:room_get())
    else
        obj:to_char(ch)
    end
    ch:reveal_hiding(0)
    ch:cooldown_set(10)
    act.to_actor("You hold out your hand and create $p out of your ki!", { actor = ch, tool = obj })
    act.around(ch, "$n holds out $s hand and creates $p out of thin air!", { actor = ch, tool = obj })
    ch:meter_mod_int("ki", -cost)
end

local function parse_quality4(ch, qa, skill)
    if qa == "" then
        ch:send_line("Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)")
        ch:send_line("If you are capable you will make it. If not you will make a low quality version.")
        return 0
    end
    if qa == "highest" and skill >= 100 then return 3 end
    if qa == "high"    and skill >= 75  then return 2 end
    if qa == "mid"     and skill >= 50  then return 1 end
    return 0
end

local function execute(ctx)
    local ch      = ctx.ch
    local tokens  = ctx.argparams.tokens
    local arg     = string.lower(tokens[1] or "")
    local arg2    = string.lower(tokens[2] or "")
    local arg3    = string.lower(tokens[3] or "")

    if not ch:know_skill("create") then
        return
    end

    local skill    = ch:skill_get("create")
    local discount = conc_discount(ch)

    if arg == "" then
        ch:send_line("What do you want to create?")
        ch:send_line("@GCreation @WMenu@n")
        ch:send_line("@D---------------@n")
        ch:send_line(string.format(
            "@wcreate food\ncreate water\n%s%s%s%s%s%s%s%s%s%s%s%s%s@n",
            skill >= 20  and "create light\n"                                    or "",
            skill >= 30  and "create bag\n"                                      or "",
            skill >= 40  and "create mattress\n"                                 or "",
            skill >= 50  and "create weapon (sword | club | dagger | spear | gun )\n" or "",
            skill >= 50  and "create pants\n"                                    or "",
            skill >= 50  and "create gi\n"                                       or "",
            skill >= 50  and "create wristband\n"                                or "",
            skill >= 50  and "create boots\n"                                    or "",
            skill >= 60  and "create clothesbeam (target)\n"                    or "",
            skill >= 70  and "create shuriken\n"                                 or "",
            skill >= 80  and "create senzu\n"                                    or "",
            skill >= 90  and "create kachin\n"                                   or "",
            skill >= 100 and "create elixir\n"                                   or ""))
        return
    end

    ch:reveal_hiding(0)

    local maxki = ch:meter_max("ki")

    local function ki_fail(cost)
        if ch:meter_current("ki") >= cost then return false end
        ch:send_line(string.format("You do not have enough ki to create %s", arg))
        return true
    end

    local function locked(min_skill)
        if skill >= min_skill then return false end
        ch:send_line("What do you want to create?")
        return true
    end

    if arg == "food" then
        local vnums = {70, 1510, 1511, 1512}
        local cost  = math.floor(maxki / (skill / 2.0) * discount)
        if ki_fail(cost) then return end
        local obj = spawn(vnums[parse_quality4(ch, arg2, skill) + 1])
        finish_create(ch, obj, cost)
        return
    end

    if arg == "water" then
        local vnums = {71, 1513, 1514, 1515}
        local cost  = math.floor(maxki / (skill * 2.0) * discount)
        if ki_fail(cost) then return end
        local obj = spawn(vnums[parse_quality4(ch, arg2, skill) + 1])
        finish_create(ch, obj, cost)
        return
    end

    if arg == "bag" then
        local cost = math.floor(maxki / (skill * 2.0) * discount)
        if locked(30) or ki_fail(cost) then return end
        local obj = spawn(319)
        finish_create(ch, obj, cost)
        return
    end

    if arg == "mattress" then
        local cost = math.floor(maxki / skill * discount)
        if locked(40) or ki_fail(cost) then return end
        local obj = spawn(16)
        obj:to_char(ch)
        ch:reveal_hiding(0)
        act.to_actor("You hold out your hand and create $p out of your ki!", { actor = ch, tool = obj })
        act.around(ch, "$n holds out $s hand and creates $p out of thin air!", { actor = ch, tool = obj })
        ch:meter_mod_int("ki", -cost)
        return
    end

    if arg == "weapon" then
        local weapon_types = {
            sword  = {90,   1516, 1517, 1518, 1519},
            dagger = {1536, 1537, 1538, 1539, 1540},
            club   = {1541, 1542, 1543, 1544, 1545},
            spear  = {1546, 1547, 1548, 1549, 1550},
            gun    = {1551, 1552, 1553, 1554, 1555},
        }
        local cost = math.floor(maxki / 5.0 * discount)
        if locked(50) or ki_fail(cost) then return end
        if arg2 == "" then
            ch:send_line("What type of weapon?\nSyntax: create weapon (sword | club | spear | dagger | gun)")
            return
        end
        if arg3 == "" then
            ch:send_line("Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | higher | highest)")
            ch:send_line("If you are capable you will make it. If not you will make a low quality version.")
        end
        local qlv = 0
        if arg3 == "highest" and skill >= 100 then qlv = 4
        elseif arg3 == "higher" and skill >= 75 then qlv = 3
        elseif arg3 == "high"   and skill >= 50 then qlv = 2
        elseif arg3 == "mid"    and skill >= 30 then qlv = 1
        end
        local wt = weapon_types[arg2]
        if not wt then
            ch:send_line("What type of weapon?\nSyntax: create weapon (sword | club | spear | dagger | gun)")
            return
        end
        local obj = spawn(wt[qlv + 1])
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "clothesbeam" then
        local cost = math.floor(maxki / 2.0 * discount)
        if locked(60) or ki_fail(cost) then return end
        if arg2 == "" then
            ch:send_line("Who do you want to hit with clothesbeam?\nSyntax: create clothesbeam (target)")
            return
        end
        local vict = ch:acquire_room_target(arg2)
        if not vict then
            ch:send_line("Clothesbeam who?\nSyntax: create clothesbeam (target)")
            return
        end
        -- Check victim is following ch
        if not (vict:condition_has("following") and
                vict:condition_number_get("following", "target_id") == ch:id_get()) then
            ch:send_line("They must be following you first.")
            return
        end
        local beam_items = {
            {vnum = 92,   boost_count = 1},
            {vnum = 91,   boost_count = 1},
            {vnum = 1528, boost_count = 1},
            {vnum = 1528, boost_count = 1},
            {vnum = 1532, boost_count = 2},
        }
        local last_obj
        for _, bi in ipairs(beam_items) do
            last_obj = spawn(bi.vnum)
            boost_equipment(last_obj, ch, bi.boost_count)
            last_obj:size_set(vict:size_get())
            last_obj:to_char(vict)
        end
        vict:command_enqueue("wear all")
        ch:reveal_hiding(0)
        ch:cooldown_set(10)
        act.to_actor("You hold out your hand and create $p out of your ki!", { actor = ch, tool = last_obj })
        act.around(ch, "$n holds out $s hand and creates $p out of thin air!", { actor = ch, tool = last_obj })
        ch:meter_mod_int("ki", -cost)
        return
    end

    if arg == "gi" then
        local cost = math.floor(maxki / 5.0 * discount)
        if locked(50) or ki_fail(cost) then return end
        local obj = spawn(92)
        boost_equipment(obj, ch)
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "pants" then
        local cost = math.floor(maxki / 5.0 * discount)
        if locked(50) or ki_fail(cost) then return end
        local obj = spawn(91)
        boost_equipment(obj, ch)
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "wristband" then
        local cost = math.floor(maxki / 5.0 * discount)
        if locked(50) or ki_fail(cost) then return end
        local obj = spawn(1528)
        boost_equipment(obj, ch)
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "boots" then
        local cost = math.floor(maxki / 5.0 * discount)
        if locked(50) or ki_fail(cost) then return end
        local obj = spawn(1532)
        boost_equipment(obj, ch)
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "shuriken" then
        local cost = math.floor(maxki / 4.0 * discount)
        if locked(70) or ki_fail(cost) then return end
        local obj = spawn(19053)
        obj:extra_flag_set(EF.NORENT, true)
        obj:extra_flag_set(EF.NOSELL, true)
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "light" then
        local cost = math.floor(maxki / (skill * 2.0) * discount)
        if locked(20) or ki_fail(cost) then return end
        local obj = spawn(72)
        finish_create(ch, obj, cost)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "kachin" then
        local cost = math.floor((maxki - 1) * discount)
        if locked(90) or ki_fail(cost) then return end
        local obj = spawn(87)
        finish_create(ch, obj, cost, true)
        obj:size_set(ch:size_get())
        return
    end

    if arg == "elixir" then
        local cost = math.floor((maxki - 1) * discount)
        if locked(100) or ki_fail(cost) then return end
        if ch:meter_current("powerlevel") < ch:meter_max("powerlevel") then
            ch:send_line(string.format("You need to be at full powerlevel to create %s", arg))
            return
        end
        if ch:stat_get("practices") < 10 then
            ch:send_line(string.format("You do not have enough PS to create %s, you need at least 10.", arg))
            return
        end
        local obj = spawn(86)
        finish_create(ch, obj, cost, true)
        obj:size_set(ch:size_get())
        local pl_drain = math.max(1, math.floor(ch:meter_max("powerlevel") * 0.01))
        ch:meter_mod_int("powerlevel", -pl_drain)
        ch:stat_mod("practices", -10)
        return
    end

    if arg == "senzu" then
        local cost  = maxki
        local cost2 = ch:meter_max("powerlevel") - 1
        if locked(80) then return end
        if ch:meter_current("ki") < cost then
            ch:send_line(string.format("You do not have enough ki to create %s, you need full ki.", arg))
            return
        end
        if ch:meter_current("powerlevel") <= cost2 then
            ch:send_line(string.format("You do not have enough powerlevel to create %s, you need to be at full.", arg))
            return
        end
        if ch:meter_current("stamina") < ch:meter_max("stamina") then
            ch:send_line(string.format("You do not have enough stamina to create %s, you need to be at full.", arg))
            return
        end
        if ch:stat_get("practices") < 50 then
            ch:send_line(string.format("You do not have enough PS to create %s, you need at least 50.", arg))
            return
        end
        local obj = spawn(1)
        finish_create(ch, obj, cost)
        ch:meter_mod_int("powerlevel", -cost2)
        local st_drain = math.max(1, math.floor(ch:meter_max("stamina") * 0.01))
        ch:meter_mod_int("stamina", -st_drain)
        ch:stat_mod("practices", -50)
        return
    end

    ch:send_line("Create what?")
end

return {
    id      = "create",
    aliases = { { "create", 4 } },
    execute = execute,
}
