local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act
local IWEAR  = dbat.consts.item_wear_flags

local SIZE_SMALL  = 3
local SIZE_MEDIUM = 4

local BODY_WEAR_FLAGS = {
    IWEAR.FINGER, IWEAR.NECK,   IWEAR.BODY,   IWEAR.HEAD,  IWEAR.LEGS,
    IWEAR.FEET,   IWEAR.HANDS,  IWEAR.ARMS,   IWEAR.SHIELD, IWEAR.ABOUT,
    IWEAR.WAIST,  IWEAR.WRIST,  IWEAR.WIELD,  IWEAR.EYE,   IWEAR.EAR,
}

local function is_wearable(obj)
    for _, flag in ipairs(BODY_WEAR_FLAGS) do
        if obj:wear_flagged(flag) then return true end
    end
    return false
end

local function execute(ctx)
    local ch   = ctx.ch
    local arg1 = ctx.argparams.tokens[1] or ""
    local arg2 = (ctx.argparams.tokens[2] or ""):lower()

    if ch:skill_get("build") <= 0 then
        ch:send_line("You do not have the skill to resize equipment!")
        return
    end
    if ch:skill_get("build") < 80 then
        ch:send_line("Your build skill must be at least level 80 before you can resize equipment.")
        return
    end
    if arg1 == "" or arg2 == "" then
        ch:send_line("Syntax: resize (obj) (small | medium)")
        return
    end

    local obj = Search(ch):add_character_inventory(ch):find_one(arg1)
    if not obj then
        ch:send_line("You don't have that object!")
        return
    end
    if not is_wearable(obj) then
        ch:send_line("That is not equipment! You can only resize equipment.")
        return
    end

    local stamina_cost = obj:weight_get() + math.floor(ch:meter_max_get("stamina") / 40)
    if ch:meter_get("stamina") < stamina_cost then
        ch:send_line("You do not have enough stamina to resize this object at this time.")
        return
    end

    if arg2 == "small" then
        if obj:size_get() == SIZE_SMALL then
            ch:send_line("The equipment is already small sized.")
            return
        end
        act.to_char(ch, "@WYou carefully adjust the size of @c$p@W.@n", { object = obj })
        act.around(ch, "@C$n@W carefully adjusts the size of @c$p@W.@n", { object = obj })
        obj:size_set(SIZE_SMALL)
        ch:meter_mod_int("stamina", -stamina_cost)
    elseif arg2 == "medium" then
        if obj:size_get() == SIZE_MEDIUM then
            ch:send_line("The equipment is already medium sized.")
            return
        end
        act.to_char(ch, "@WYou carefully adjust the size of @c$p@W.@n", { object = obj })
        act.around(ch, "@C$n@W carefully adjusts the size of @c$p@W.@n", { object = obj })
        obj:size_set(SIZE_MEDIUM)
        ch:meter_mod_int("stamina", -stamina_cost)
    else
        ch:send_line("Syntax: resize (obj) (small | medium)")
    end
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return ch:know_skill("build")
end

return {
    id          = "resize",
    aliases     = { {"resize", 3} },
    execute     = execute,
    can_execute = can_execute,
}
