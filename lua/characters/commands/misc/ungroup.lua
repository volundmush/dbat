local dbat = require("dbat")
local AFF  = dbat.consts.aff_flags

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if arg == "" then
        if ch:following_get() ~= nil or not ch:condition_has("group") then
            ch:send_line("But you lead no group!")
            return
        end

        ch:followers_each(function(fol)
            if fol:condition_has("group") then
                fol:condition_remove("group", "leave_group")
                fol:act("$N has disbanded the group.", true, nil, ch, "char")
                if not fol:aff_flagged(AFF.CHARM) then
                    fol:stop_follower()
                end
            end
        end)

        ch:condition_remove("group", "leave_group")
        ch:send_line("You disband the group.")
        return
    end

    local tch = dbat.search.find_char_in_room(ch, arg)
    if not tch then
        ch:send_line("There is no such person!")
        return
    end

    local tch_master = tch:following_get()
    if tch_master == nil or not tch_master:is_same(ch) then
        ch:send_line("That person is not following you!")
        return
    end

    if not tch:condition_has("group") then
        ch:send_line("That person isn't in your group.")
        return
    end

    tch:condition_remove("group", "leave_group")
    ch:act("$N is no longer a member of your group.", false, nil, tch, "char")
    ch:act("You have been kicked out of $n's group!", false, nil, tch, "vict")
    ch:act("$N has been kicked out of $n's group!", false, nil, tch, "notvict")

    if not tch:aff_flagged(AFF.CHARM) then
        tch:stop_follower()
    end
end

return {
    id      = "ungroup",
    aliases = { {"ungroup", 7} },
    execute = execute,
}
