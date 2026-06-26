local dbat   = require("dbat")
local EF     = dbat.consts.item_extra_flags
local ADMLVL = dbat.consts.adm_levels
local Search = dbat.lib.search.new

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:admin_level_get() < ADMLVL.IMMORT then
        ch:send_line("Huh?!?")
        return
    end

    if arg == "" then
        ch:send_line("Usually you break SOMETHING.")
        return
    end

    local obj = Search(ch):add_character_inventory(ch):add_character_equipment(ch):find_one(arg)
    if not obj then
        ch:send_line("Can't seem to find what you want to break!")
        return
    end

    if obj:extra_flagged(EF.BROKEN) then
        ch:send_line("Seems like it's already broken!")
        return
    end

    ch:send_line("You ruin %s.", obj:short_description_get())
    ch:act("$n ruins $p.", false, obj, nil, "room")
    obj:value_set(4, 0)  -- VAL_ALL_HEALTH = 4
    obj:extra_flag_toggle(EF.BROKEN)
end

return {
    id      = "break",
    aliases = { {"break", 5} },
    execute = execute,
}
