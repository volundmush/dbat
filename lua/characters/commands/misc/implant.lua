local dbat = require("dbat")

local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions
local Search = dbat.lib.search.new

local CYBERNETIC_LIMB_VNUM = 66
local SEARCH_WORKING_GENUINE = 3

local PARTS = {
    rarm = { limb = 1, flag = PLR.CRARM, name = "right arm" },
    larm = { limb = 2, flag = PLR.CLARM, name = "left arm" },
    rleg = { limb = 3, flag = PLR.CRLEG, name = "right leg" },
    lleg = { limb = 4, flag = PLR.CLLEG, name = "left leg" },
}

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.RESTING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    end

    return true
end

local function room_target(ch, name)
    return Search(ch):add_room_people(ch:room_get()):add_filter(function(s, e)
        return s:can_see_char(e)
    end):find_one(name)
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()
    local target_name = ctx.argparams.tokens[2] or ""

    if blocked_by_position(ch) then return end
    if not ch:limb_ok(0) then return end

    if arg == "" then
        ch:send_line("Syntax: implant (rarm | larm | rleg | lleg) (target)")
        return
    end

    local vict = ch
    if target_name ~= "" then
        vict = room_target(ch, target_name)
        if not vict then
            ch:send_line("That person isn't here.")
            return
        end
    end

    local limb = ch:inventory_find_vnum(CYBERNETIC_LIMB_VNUM, SEARCH_WORKING_GENUINE)
    if not limb then
        ch:send_line("You do not have a cybernetic limb to implant.")
        return
    end

    local part = PARTS[arg]
    if not part then
        ch:send_line("Syntax: implant (rarm | larm | rleg | rleg)")
        return
    end

    if vict:limbcond_get(part.limb) >= 1 then
        if vict:is_same(ch) then
            ch:send_line("You already have a %s!", part.name)
        else
            ch:send_line("They already have a %s!", part.name)
        end
        return
    end

    ch:reveal_hiding(0)
    if vict:is_same(ch) then
        ch:act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new " .. part.name .. "!@n",
            true, limb, nil, "char")
        ch:act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new " .. part.name .. "!@n",
            true, limb, nil, "room")
    else
        ch:act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new " .. part.name .. "!@n",
            true, limb, vict, "char")
        ch:act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new " .. part.name .. "!@n",
            true, limb, vict, "vict")
        ch:act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new " .. part.name .. "!@n",
            true, limb, vict, "notvict")
    end

    vict:player_flag_set(part.flag, true)
    limb:from_char()
    limb:extract()
end

return {
    id = "implant",
    aliases = { {"implant", 6} },
    execute = execute,
}
