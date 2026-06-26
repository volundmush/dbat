local dbat   = require("dbat")
local EF     = dbat.consts.item_extra_flags
local MAT    = dbat.consts.materials
local PLR    = dbat.consts.player_flags
local text   = dbat.lib.text
local Search = dbat.lib.search.new

local PULSE_2SEC = 20
local PULSE_5SEC = 50
local SEARCH_WORKING_GENUINE = 3  -- not broken, not forged

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if not ch:know_skill("repair") then return end

    if arg == "" then
        ch:send_line("Usually you fix SOMETHING.")
        return
    end

    local is_self = arg:lower() == "self"

    if is_self and ch:race_get() ~= "android" then
        ch:send_line("Only androids can fix their bodies with repair kits.")
        return
    end

    local obj
    if not is_self then
        obj = Search(ch)
            :add_character_inventory(ch)
            :add_character_equipment(ch)
            :add_room_objects(ch:room_get())
            :find_one(arg)
        if not obj then
            ch:send_line("Can't seem to find what you want to fix!")
            return
        end

        if obj:value_get(4) >= 100 then  -- VAL_ALL_HEALTH = 4
            ch:send_line("But it isn't even damaged!")
            return
        end

        if obj:extra_flagged(EF.FORGED) then
            ch:send_line("That is fake, why bother fixing it?")
            return
        end

        local mat = obj:value_get(7)  -- VAL_ALL_MATERIAL = 7
        if mat == MAT.ORGANIC or mat == MAT.FOOD or mat == MAT.PAPER or mat == MAT.LIQUID then
            ch:send_line("You can't repair that.")
            return
        end
    end

    local is_custom = (not is_self) and (obj:vnum_get() == 20099 or obj:vnum_get() == 20098)

    local kit_vnum = is_custom and 13593 or 48
    local kit = ch:inventory_find_vnum(kit_vnum, SEARCH_WORKING_GENUINE)
    if not kit then
        if is_custom then
            ch:send_line("You do not even have a Nano-tech Repair Orb.")
        else
            ch:send_line("You do not even have a repair kit.")
        end
        return
    end

    local skill = ch:skill_get("repair")

    if not is_self then
        if skill < dbat.axion_dice(0) then
            ch:act("You try to repair $p but screw up..", true, obj, nil, "char")
            ch:act("$n tries to repair $p but screws up..", true, obj, nil, "room")
            kit:extract()
            ch:improve_skill("repair", 1)
            ch:wait_set(PULSE_2SEC)
            return
        end

        local current_health = obj:value_get(4)
        if current_health + skill < 100 then
            ch:send_line("You repair %s a bit.", obj:short_description_get())
            ch:act("$n repairs $p a bit.", false, obj, nil, "room")
            obj:value_set(4, current_health + skill)
        else
            ch:send_line("You repair %s completely.", obj:short_description_get())
            ch:act("$n repairs $p completely.", false, obj, nil, "room")
            obj:value_set(4, 100)
        end
        obj:extra_flag_set(EF.BROKEN, false)

        local level = ch:stat_get("level")
        if obj:carried_by_get() == nil and not ch:player_flagged(PLR.REPLEARN) and
           (ch:level_exp(level + 1) - ch:stat_get("experience") > 0 or level >= 100) then
            local gain = math.floor(ch:level_exp(level + 1) * 0.0003 * skill)
            ch:send_line("@mYou've learned a bit from repairing it. @D[@gEXP@W: @G+%s@D]@n",
                text.add_commas(gain))
            ch:player_flag_set(PLR.REPLEARN, true)
            ch:gain_exp(gain)
        elseif math.random(2, 12) >= 10 and ch:player_flagged(PLR.REPLEARN) then
            ch:player_flag_set(PLR.REPLEARN, false)
            ch:send_line("@mYou think you might be on to something...@n")
        end

        ch:improve_skill("repair", 1)
        kit:extract()
        ch:wait_set(PULSE_2SEC)
    else
        -- Android self-repair
        if ch:meter_current("powerlevel") >= ch:meter_max("powerlevel") then
            ch:send_line("Your body is already in peak condition.")
            return
        end

        if skill < dbat.axion_dice(0) then
            ch:act("You try to repair your body but screw up..", true, nil, nil, "char")
            ch:act("$n tries to repair $s body but screws up..", true, nil, nil, "room")
            kit:extract()
            ch:improve_skill("repair", 1)
            ch:wait_set(PULSE_5SEC)
            return
        end

        ch:act("You use the repair kit to fix part of your body...", true, nil, nil, "char")
        ch:act("$n works on their body with a repair kit.", true, nil, nil, "room")
        local add = math.floor((ch:meter_max("powerlevel") * 0.005 + 10) * skill)
        kit:extract()
        ch:meter_mod_int("powerlevel", add)
        if ch:meter_current("powerlevel") >= ch:meter_max("powerlevel") then
            ch:send_line("Your body has been totally repaired.")
        else
            ch:send_line("Your body still needs some work done to it.")
        end
        ch:wait_set(PULSE_5SEC)
    end
end

return {
    id      = "fix",
    aliases = { {"fix", 3} },
    execute = execute,
}
