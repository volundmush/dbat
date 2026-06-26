local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local IT   = dbat.consts.item_types
local EF   = dbat.consts.item_extra_flags
local OCMD = dbat.consts.obj_cmd_types

local function an(word)
    return (word or ""):match("^[aeiouAEIOU]") and "an" or "a"
end

local function senzu_restore(ch)
    ch:meter_set("powerlevel", 1000000)
    ch:meter_set("ki", 1000000)
    ch:meter_set("stamina", 1000000)
    local limb_names = {"right arm", "left arm", "right leg", "left leg"}
    for i = 1, 4 do
        local lc = ch:limbcond_get(i)
        if lc <= 0 then
            ch:send_line("Your %s grows back!", limb_names[i])
        elseif lc < 50 then
            ch:send_line("Your %s is no longer broken!", limb_names[i])
        end
        ch:limbcond_set(i, 100)
    end
    if ch:condition_has("knocked_out") then
        local carrier = ch:carried_by_char_get()
        if carrier then
            carrier:carry_drop(carrier:stat_get("alignment") > 50 and 0 or 1)
        end
        ch:condition_remove("knocked_out", "recovered")
    end
    if ch:condition_has("burned") then
        ch:send_line("Your burns are healed now.")
        act.around(ch, "$n@w's burns are now healed.@n", {})
        ch:condition_remove("burned", "healing_burned")
    end
    if ch:condition_has("poison") then
        act.around(ch, "@C$n@W suddenly looks a lot better!@b", {})
        ch:condition_remove("poison", "healing_poisoned")
    end
    ch:meter_set("lifeforce", 1000000)
    if ch:stat_get("hunger") > -1 then
        ch:stat_set("hunger", 48)
    end
end

return {
    id = "swallow",
    aliases = { {"swallow", 5} },
    execute = function(ctx)
        local ch  = ctx.ch
        local arg = ctx.argparams.tokens[1] or ""
        if arg == "" then
            ch:send_line("Swallow what?")
            return
        end
        local obj = Search(ch):add_character_inventory(ch):find_one(arg)
        if not obj then
            ch:send_line("You don't seem to have %s %s.", an(arg), arg)
            return
        end
        if obj:type_get() ~= IT.POTION then
            ch:send_line("You can only swallow beans.")
            return
        end
        if ch:race_get() == "android" then
            ch:send_line("You can't swallow beans, you are an android.")
            return
        end
        if obj:extra_flagged(EF.FORGED) then
            ch:send_line("You can't swallow that, it is fake!")
            return
        end
        if obj:extra_flagged(EF.BROKEN) then
            ch:send_line("You can't swallow that, it is broken!")
            return
        end
        if not dbat.dgscripts.consume_otrigger(obj, ch, OCMD.QUAFF) then return end
        act.to_char(ch, "You swallow $p.", { object = obj })
        local ad = obj:action_description_get()
        if ad then
            act.around(ch, ad, { object = obj })
        else
            act.around(ch, "$n swallows $p.", { object = obj })
        end
        senzu_restore(ch)
        obj:extract()
    end,
}
