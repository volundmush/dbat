local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local function execute(ctx)
    local ch   = ctx.ch
    local tok  = ctx.argparams.tokens
    local arg1 = tok[1] or ""
    local arg2 = tok[2] or ""

    if arg1 == "" or arg2 == "" then
        ch:send_line("You want to show what item to what character?")
        return
    end

    local obj = Search(ch):add_character_inventory(ch):add_filter(function(s, e)
        return s:can_see(e)
    end):find_one(arg1)
    if not obj then
        ch:send_line("You don't seem to have that.")
        return
    end

    local room   = ch:room_get()
    local victim = Search(ch):add_room_people(room):add_filter(function(s, e)
        return s:can_see(e) and e:id_get() ~= ch:id_get() and not e:is_npc()
    end):find_one(arg2)
    if not victim then
        ch:send_line("There is no such person around.")
        return
    end

    act.to_char(ch,   "@WYou hold up $p@W for @C$N@W to see:@n", { object = obj, target = victim })
    act.to_vict(ch,   "@C$n@W holds up $p@W for you to see:@n",  { object = obj, target = victim })
    act.around(ch,    "@C$n@W holds up $p@W for @c$N@W to see.@n", { object = obj, target = victim, exclude = victim })
    obj:show_to(victim)
end

return { id = "show", aliases = { { "show", 4 } }, execute = execute }
