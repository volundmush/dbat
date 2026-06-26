local dbat   = require("dbat")
local ITEM   = dbat.consts.item_types
local Search = dbat.lib.search.new

local function execute(ctx)
    local ch   = ctx.ch
    local arg1 = ctx.argparams.tokens[1] or ""
    local arg2 = ctx.argparams.tokens[2] or ""

    if arg1 == "" or arg2 == "" then
        ch:send_line("Compare what to what?")
        return
    end

    local search = Search(ch):add_character_inventory(ch):add_character_equipment(ch)
    local obj1 = search:find_one(arg1)
    local obj2 = search:find_one(arg2)

    if not obj1 or not obj2 then
        ch:send_line("You do not have that item.")
        return
    end

    local d1 = obj1:short_description_get()
    local d2 = obj2:short_description_get()

    if obj1:is_same(obj2) then
        ch:send_line("You compare %s to itself.  It looks about the same.", d1)
        return
    end

    local type1 = obj1:type_get()
    local type2 = obj2:type_get()

    if type1 ~= type2 then
        ch:send_line("You can't compare %s and %s.", d1, d2)
        return
    end

    local value1, value2

    if type1 == ITEM.ARMOR then
        value1 = obj1:value_get(0)  -- VAL_ARMOR_APPLYAC = 0
        value2 = obj2:value_get(0)
    elseif type1 == ITEM.WEAPON then
        value1 = (1 + obj1:value_get(2)) * obj1:value_get(1)  -- (1+DAMSIZE)*DAMDICE
        value2 = (1 + obj2:value_get(2)) * obj2:value_get(1)
    else
        ch:send_line("You can't compare %s and %s.", d1, d2)
        return
    end

    if value1 == value2 then
        ch:send_line("%s and %s look about the same.", d1, d2)
    elseif value1 > value2 then
        ch:send_line("%s looks better than %s.", d1, d2)
    else
        ch:send_line("%s looks worse than %s.", d1, d2)
    end
end

return {
    id      = "compare",
    aliases = { {"compare", 4} },
    execute = execute,
}
