local dbat = require("dbat")
local BON  = dbat.consts.bonuses
local AFF  = dbat.consts.aff_flags
local text = dbat.lib.text

local function get_member_line(viewer, member, prefix)
    local pl     = member:meter_current("powerlevel")
    local pl_max = member:meter_max("powerlevel")
    local ki     = member:meter_current("ki")
    local st     = member:meter_current("stamina")
    local level  = member:stat_get("level")
    local pl_col = pl > math.floor(pl_max / 10) and "@c" or "@r"
    local sensei_def = dbat.registry.senseis[member:sensei_get()]
    local race_def   = dbat.registry.races[member:race_get()]
    local s_abbr = sensei_def and sensei_def.abbreviation or "??"
    local r_abbr = race_def   and race_def.abbreviation   or "??"
    local name   = member:display_name_for(viewer)
    return string.format(
        "@g%s@D: @w%s @W- @D[@RPL@Y: %s%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
        prefix, name, pl_col, text.add_commas(pl),
        text.add_commas(ki), text.add_commas(st),
        level, s_abbr, r_abbr)
end

local function print_group(ch)
    if not ch:condition_has("group") then
        ch:send_line("But you are not the member of a group!")
        return
    end

    ch:send_line("Your group consists of:")

    local leader = ch:following_get() or ch
    if leader:condition_has("group") then
        ch:send_line("@D----------------@n")
        ch:send_line(get_member_line(ch, leader, "L"))
    end

    leader:followers_each(function(fol)
        if not fol:condition_has("group") then return end
        ch:send_line("@D----------------@n")
        ch:send_line(get_member_line(ch, fol, "F"))
    end)

    ch:send_line("@D----------------@n")
end

local function perform_group(leader, vict)
    if vict:condition_has("group") then return false end
    if not leader:can_see_char(vict) then return false end
    if vict:bonus_flagged(BON.LONER) then
        vict:act("$n is the loner type and refuses to be in your group.", true, nil, leader, "vict")
        return false
    end
    vict:condition_add("group", "party", "group")
    if not leader:is_same(vict) then
        leader:act("$N is now a member of your group.", false, nil, vict, "char")
    end
    leader:act("You are now a member of $n's group.", false, nil, vict, "vict")
    leader:act("$N is now a member of $n's group.", false, nil, vict, "notvict")
    return true
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:bonus_flagged(BON.LONER) then
        ch:send_line("You can not group as you prefer to be alone.")
        return
    end

    if arg == "" then
        print_group(ch)
        return
    end

    if ch:following_get() ~= nil then
        ch:send_line("You cannot enroll group members without being head of a group.")
        return
    end

    local highlvl = ch:stat_get("level")
    local lowlvl  = highlvl

    ch:followers_each(function(fol)
        if fol:condition_has("group") then
            local lvl = fol:stat_get("level")
            if lvl > highlvl then highlvl = lvl end
            if lvl < lowlvl  then lowlvl  = lvl end
        end
    end)

    if arg:lower() == "all" then
        perform_group(ch, ch)
        local found = 0
        ch:followers_each(function(fol)
            if perform_group(ch, fol) then
                found = found + 1
                local lvl = fol:stat_get("level")
                if     lvl > highlvl then highlvl = lvl
                elseif lvl < lowlvl  then lowlvl  = lvl
                end
            end
        end)
        if found == 0 then
            ch:send_line("Everyone following you is already in your group.")
        end
        return
    end

    local vict = dbat.search.find_char_in_room(ch, arg)
    if not vict then
        ch:send_line("%s", dbat.consts.noperson)
        return
    end

    local vict_master = vict:following_get()
    local follows_ch  = vict_master ~= nil and vict_master:is_same(ch)
    if not follows_ch and not vict:is_same(ch) then
        ch:act("$N must follow you to enter your group.", false, nil, vict, "char")
        return
    end

    if not vict:condition_has("group") then
        if not ch:condition_has("group") then
            ch:send_line("You form a group, with you as leader.")
            ch:condition_add("group", "party", "group")
        end
        perform_group(ch, vict)
    else
        if not ch:is_same(vict) then
            ch:act("$N is no longer a member of your group.", false, nil, vict, "char")
        end
        ch:act("You have been kicked out of $n's group!", false, nil, vict, "vict")
        ch:act("$N has been kicked out of $n's group!", false, nil, vict, "notvict")
        vict:condition_remove("group", "leave_group")
    end
end

return {
    id      = "group",
    aliases = { {"group", 3} },
    execute = execute,
}
