local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local function execute(ctx)
    local ch   = ctx.ch
    local tok  = ctx.argparams.tokens
    local arg1 = tok[1] or ""
    local arg2 = tok[2] or ""

    if arg1 == "" or arg2 == "" then
        ch:send_line("Syntax: table (red | blue | green | yellow) (card name)")
        return
    end

    local room  = ch:room_get()
    local table_obj = Search(ch):add_room_objects(room):add_filter(function(s, e)
        return s:can_see(e)
    end):find_one(arg1)
    if not table_obj then
        ch:send_line("You don't see that table here.")
        return
    end

    local card = Search(ch):add_object_inventory(table_obj):add_filter(function(s, e)
        return s:can_see(e)
    end):find_one(arg2)
    if not card then
        ch:send_line("That card doesn't seem to be on that table.")
        return
    end

    act.around(ch, string.format("$n looks at %s on %s.\r\n",
        card:short_description_get(), table_obj:short_description_get()))
    ch:send(card:action_description_get() or "")
end

return { id = "table", aliases = { { "table", 4 } }, execute = execute }
