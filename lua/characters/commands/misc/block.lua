local dbat = require("dbat")
local search = dbat.lib.search

local function get_blocking(ch)
    if not ch:condition_has("blocking") then return nil end
    local id = ch:condition_number_get("blocking", "target_id")
    if not id or id <= 0 then return nil end
    return dbat.characters.by_id(id)
end

local function get_blocked_by(ch)
    if not ch:condition_has("blocked_by") then return nil end
    local id = ch:condition_number_get("blocked_by", "target_id")
    if not id or id <= 0 then return nil end
    return dbat.characters.by_id(id)
end

local function clear_blocking(ch, vict)
    ch:condition_remove("blocking", "update")
    vict:condition_remove("blocked_by", "update")
end

local function set_blocking(ch, vict)
    ch:condition_apply_number("blocking", "target_id", vict:id_get(), "combat", "block")
    vict:condition_apply_number("blocked_by", "target_id", ch:id_get(), "combat", "block")
end

local function send_block_msgs(ch, vict, starting)
    local actlib = dbat.lib.act
    local ctx = { actor = ch, target = vict }
    if starting then
        actlib.message({
            actor  = "@wYou start blocking @c$N's@w escape.@n",
            target = "@C$n@w starts blocking your escape.@n",
            room   = "@C$n@w starts blocking @c$N's@w escape.@n",
        }, ctx)
    else
        actlib.message({
            actor  = "@wYou stop blocking @c$N@w.@n",
            target = "@C$n@w stops blocking you.@n",
            room   = "@C$n@w stops blocking @c$N@w.@n",
        }, ctx)
    end
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:is_npc() then return end

    local blocking = get_blocking(ch)

    if arg == "" then
        if not blocking then
            ch:send_line("You want to block who?")
            return
        end
        send_block_msgs(ch, blocking, false)
        clear_blocking(ch, blocking)
        return
    end

    local vict = search.new(ch):add_room_people(ch:room_get()):find_one(arg)
    if not vict then
        ch:send_line("You do not see the target here.")
        return
    end

    if blocking == vict then
        ch:send_line("They are already blocked by you!")
        return
    end

    if ch == vict then
        ch:send_line("You can't block yourself, are you mental?")
        return
    end

    if get_blocked_by(vict) then
        ch:send_line("They are already blocked by someone else!")
        return
    end

    if blocking then
        send_block_msgs(ch, blocking, false)
        clear_blocking(ch, blocking)
    end

    set_blocking(ch, vict)
    ch:reveal_hiding(0)
    send_block_msgs(ch, vict, true)
end

return { id = "block", aliases = { { "block", 5 } }, execute = execute }
