local dbat = require("dbat")
local PRF  = dbat.consts.prf_flags

local SYNTAX = "Usage: prompt { P | K | T | S | F | H | G | L | C | M | all/on | none/off }"

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.raw:match("^%s*(.-)%s*$") or ""

    if arg == "" then
        ch:send_line(SYNTAX)
        return
    end

    local larg = arg:lower()

    if larg == "on" or larg == "all" then
        ch:pref_flag_set(PRF.DISPHP,   true)
        ch:pref_flag_set(PRF.DISPMOVE, true)
        ch:pref_flag_set(PRF.DISPKI,   true)
        ch:pref_flag_set(PRF.DISPTNL,  true)
        ch:pref_flag_set(PRF.FURY,     true)
        ch:pref_flag_set(PRF.DISTIME,  true)
        ch:pref_flag_set(PRF.DISGOLD,  true)
        ch:pref_flag_set(PRF.DISPRAC,  true)
        ch:pref_flag_set(PRF.DISHUTH,  true)
        ch:pref_flag_set(PRF.DISPERC,  true)
    elseif larg == "off" or larg == "none" then
        ch:pref_flag_set(PRF.DISPHP,   false)
        ch:pref_flag_set(PRF.DISPMOVE, false)
        ch:pref_flag_set(PRF.DISPKI,   false)
        ch:pref_flag_set(PRF.DISPTNL,  false)
        ch:pref_flag_set(PRF.FURY,     false)
        ch:pref_flag_set(PRF.DISTIME,  false)
        ch:pref_flag_set(PRF.DISGOLD,  false)
        ch:pref_flag_set(PRF.DISPRAC,  false)
        ch:pref_flag_set(PRF.DISHUTH,  false)
        ch:pref_flag_set(PRF.DISPERC,  false)
    else
        for i = 1, #arg do
            local c = arg:sub(i, i):lower()
            if     c == "p" then ch:pref_flag_toggle(PRF.DISPHP)
            elseif c == "s" then ch:pref_flag_toggle(PRF.DISPMOVE)
            elseif c == "k" then ch:pref_flag_toggle(PRF.DISPKI)
            elseif c == "t" then ch:pref_flag_toggle(PRF.DISPTNL)
            elseif c == "h" then ch:pref_flag_toggle(PRF.DISTIME)
            elseif c == "g" then ch:pref_flag_toggle(PRF.DISGOLD)
            elseif c == "l" then ch:pref_flag_toggle(PRF.DISPRAC)
            elseif c == "c" then ch:pref_flag_toggle(PRF.DISPERC)
            elseif c == "m" then ch:pref_flag_toggle(PRF.DISHUTH)
            elseif c == "f" then
                if ch:race_get() ~= "halfbreed" then
                    ch:send_line("Only halfbreeds use fury.")
                end
                ch:pref_flag_toggle(PRF.FURY)
            else
                ch:send_line(SYNTAX)
                return
            end
        end
    end

    ch:send_line("Ok.")
end

local function can_execute(ch)
    if ch:is_npc() then return false, "Monsters don't need displays. Go away." end
    return true
end

return {
    id         = "display",
    aliases    = { {"display", 4}, {"prompt", 3} },
    execute    = execute,
    can_execute = can_execute,
}
