local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act
local ST     = dbat.consts.sector_types
local EF     = dbat.consts.item_extra_flags

local DIG_SECTORS = {
    [ST.FIELD]    = true,
    [ST.HILLS]    = true,
    [ST.FOREST]   = true,
    [ST.DESERT]   = true,
    [ST.MOUNTAIN] = true,
}

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if not (ch:limbcond_get(1) > 0 or ch:limbcond_get(2) > 0) then
        ch:send_line("You have no arms!")
        return
    end
    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are busy grappling with someone!")
        return
    end
    if ch:absorbing_get() or ch:absorbed_by_get() then
        ch:send_line("You are busy struggling with someone!")
        return
    end

    if arg == "" then
        ch:send_line("Syntax: dig [bury (item) | uncover]")
        return
    end

    local room = ch:room_get()
    local sect = room:sector_type_get()

    if not DIG_SECTORS[sect] then
        ch:send_line("You are not in a room with enough available dirt or sand to dig.")
        return
    end

    local is_desert = sect == ST.DESERT

    if arg == "bury" then
        local obj_name = ctx.argparams.tokens[2] or ""
        if obj_name == "" then
            ch:send_line("Bury what?")
            return
        end
        local obj = Search(ch):add_character_inventory(ch):find_one(obj_name)
        if not obj then
            ch:send_line("You don't have that object to bury.")
            return
        end
        -- Check if something already buried here
        for buried in room:contents() do
            if buried:extra_flagged(EF.BURIED) then
                ch:send_line("There is already something buried near here.")
                return
            end
        end
        if not is_desert then
            act.to_char(ch,   "@yYou start digging in a spot of soft dirt. Once you have an appropriately sized hole you drop @G$p@y in and then cover it.@n", { object = obj })
            act.around(ch, "@C$n@y starts digging in a spot of soft dirt. Once $e has an appropriately sized hole $e drops @G$p@y in and then covers it.@n", { object = obj })
        else
            act.to_char(ch,   "@YYou start digging in a spot of soft sand. Once you have an appropriately sized hole you drop @G$p@Y in and then cover it.@n", { object = obj })
            act.around(ch, "@C$n@Y starts digging in a spot of soft sand. Once $e has an appropriately sized hole $e drops @G$p@Y in and then covers it.@n", { object = obj })
        end
        obj:from_char()
        obj:to_room(room)
        obj:extra_flag_set(EF.BURIED, true)

    elseif arg == "uncover" then
        local fobj = nil
        for buried in room:contents() do
            if buried:extra_flagged(EF.BURIED) then
                fobj = buried
                break
            end
        end
        if not fobj then
            ch:send_line("There is nothing buried here.")
            return
        end
        if not is_desert then
            act.to_char(ch,   "@yYou slowly dig and reveal @G$p@y buried in the dirt! You pull it out and set it on the ground before covering the hole back up.@n", { object = fobj })
            act.around(ch, "@C$n@y starts digging and shortly reveals @G$p@y buried in the dirt! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", { object = fobj })
        else
            act.to_char(ch,   "@YYou slowly dig and reveal @G$p@Y buried in the sand! You pull it out and set it on the ground before covering the hole back up.@n", { object = fobj })
            act.around(ch, "@C$n@Y starts digging and shortly reveals @G$p@Y buried in the sand! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", { object = fobj })
        end
        fobj:extra_flag_set(EF.BURIED, false)
    else
        ch:send_line("Syntax: dig [bury (item) | uncover]")
    end
end

return {
    id      = "bury",
    aliases = { {"bury", 3}, {"dig", 3} },
    execute = execute,
}
