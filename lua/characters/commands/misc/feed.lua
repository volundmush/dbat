local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local IT   = dbat.consts.item_types
local EF   = dbat.consts.item_extra_flags
local OCMD = dbat.consts.obj_cmd_types

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
    id = "feed",
    aliases = { {"feed", 3} },
    execute = function(ctx)
        local ch = ctx.ch
        if ch:is_npc() then return end

        local arg, arg2
        if ctx.argparams and ctx.argparams.tokens then
            arg  = ctx.argparams.tokens[1] or ""
            arg2 = ctx.argparams.tokens[2] or ""
        else
            arg, arg2 = "", ""
        end

        if arg == "" then
            ch:send_line("Feed a senzu to whom?")
            return
        end

        local room = ch:room_get()
        local vict = Search(ch):add_room_people(room):find_one(arg)
        if not vict then
            ch:send_line("That target isn't here.")
            return
        end

        if vict:race_get() == "android" then
            ch:send_line("They are unaffected by senzu beans.")
            return
        end

        if arg2 == "" then
            ch:send_line("You need to give them a senzu.")
            return
        end

        local obj = Search(ch):add_character_inventory(ch):find_one(arg2)
        if not obj then
            ch:send_line("You need to give them a senzu.")
            return
        end

        if obj:type_get() ~= IT.POTION then
            ch:send_line("You can only feed senzu beans.")
            return
        end

        if obj:extra_flagged(EF.FORGED) then
            ch:send_line("They can't swallow that, it is fake!")
            return
        end

        if obj:extra_flagged(EF.BROKEN) then
            ch:send_line("They can't swallow that, it is broken!")
            return
        end

        if vict:is_fighting() then
            ch:send_line("They are a bit busy at the moment!")
            return
        end

        local vict_master = vict:following_get()
        local ch_master   = ch:following_get()
        if vict_master ~= ch and ch_master ~= vict and ch_master ~= vict_master then
            ch:send_line("You need to be grouped with them first.")
            return
        end
        if not ch:condition_has("group") or not vict:condition_has("group") then
            ch:send_line("You need to be grouped with them first.")
            return
        end

        if not dbat.dgscripts.consume_otrigger(obj, vict, OCMD.QUAFF) then return end

        act.message({
            actor  = "@WYou take $p@W and pop it into @C$N@W's mouth!@n",
            target = "@C$n@W takes $p@W and pops it into YOUR mouth!@n",
            room   = "@C$n@W takes $p@W and pops it into @c$N@W's mouth!@n",
        }, { actor = ch, target = vict, object = obj })

        senzu_restore(vict)
        obj:extract()
    end,
}
