local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:is_npc() then return end

    local amount = tonumber(arg)
    if not amount or amount ~= math.floor(amount) then
        ch:send_line("How much zenni do you wish to split with your group?")
        return
    end

    if amount <= 0 then
        ch:send_line("Sorry, you can't do that.")
        return
    end

    if amount > ch:stat_get("money") then
        ch:send_line("You don't seem to have that much gold to split.")
        return
    end

    ch:stat_mod("money", -amount)

    local leader  = ch:following_get() or ch
    local ch_room = ch:room_get()

    local function same_room(other)
        local r = other:room_get()
        return r ~= nil and ch_room ~= nil and r:vnum_get() == ch_room:vnum_get()
    end

    local num = 0
    if leader:condition_has("group") and same_room(leader) then num = 1 end
    leader:followers_each(function(fol)
        if fol:condition_has("group") and not fol:is_npc()
           and not fol:is_same(ch) and same_room(fol) then
            num = num + 1
        end
    end)

    if num == 0 or not ch:condition_has("group") then
        ch:send_line("With whom do you wish to share your gold?")
        ch:stat_mod("money", amount)
        return
    end

    local share = math.floor(amount / num)
    local rest  = amount % num

    local ch_name = ch:name_get()
    local member_msg = string.format("%s splits %d zenni; you receive %d.\r\n",
        ch_name, amount, share)
    if rest > 0 then
        member_msg = member_msg .. string.format(
            "%d zenni %s not splitable, so %s keeps the money.\r\n",
            rest, rest == 1 and "was" or "were", ch_name)
    end

    ch:stat_mod("money", share)

    if leader:condition_has("group") and same_room(leader) and not leader:is_npc() and not leader:is_same(ch) then
        leader:stat_mod("money", share)
        leader:send_line("%s", member_msg)
    end

    leader:followers_each(function(fol)
        if fol:condition_has("group") and not fol:is_npc()
           and same_room(fol) and not fol:is_same(ch) then
            fol:stat_mod("money", share)
            fol:send_line("%s", member_msg)
        end
    end)

    ch:send_line("You split %d zenni among %d members -- %d zenni each.", amount, num, share)

    if rest > 0 then
        ch:send_line("%d zenni %s not splitable, so you keep the money.",
            rest, rest == 1 and "was" or "were")
        ch:stat_mod("money", rest)
    end
end

return {
    id      = "split",
    aliases = { {"split", 5} },
    execute = execute,
}
