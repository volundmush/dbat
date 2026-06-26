local dbat   = require("dbat")
local act    = dbat.lib.act
local Search = dbat.lib.search.new
local IT     = dbat.consts.item_types
local IWEAR  = dbat.consts.item_wear_flags
local EF     = dbat.consts.item_extra_flags
local WP     = dbat.consts.wear_positions

-- VAL_WEAPON_DAMTYPE values for slashing/piercing/stabbing (TYPE_X - TYPE_HIT)
local CUT_DAMTYPES = { [3] = true, [11] = true, [14] = true }

local function is_cutting_weapon(w)
    return w and CUT_DAMTYPES[w:value_get(3)] ~= nil
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:is_npc() then return end

    if arg == "" then
        ch:send_line("What corpse do you want to decapitate?")
        return
    end

    local obj = Search(ch):add_room_objects(ch:room_get()):add_filter(function(s, e)
        return s:can_see(e)
    end):find_one(arg)

    if not obj then
        ch:send_line("No corpse around here by that name.")
        return
    end

    if obj:value_get(8) == 0 then  -- VAL_CORPSE_HEAD = 8
        ch:send_line("That corpse is already missing its head.")
        return
    end

    local has_cutter = is_cutting_weapon(ch:equipment_get(WP.WIELD1))
                    or is_cutting_weapon(ch:equipment_get(WP.WIELD2))

    if has_cutter then
        act.to_char(ch, "@WYou reach down and @rcut@W the head off of @R$p@W!@n", { object = obj })
        act.around(ch, "@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", { object = obj })
    else
        act.to_char(ch, "@WYou reach down and @rtear@W the head off of @R$p@W!@n", { object = obj })
        act.around(ch, "@C$n@W reaches down and @rtears@W the head off of @R$p@W!@n", { object = obj })
    end

    obj:value_set(8, 0)

    -- Build head name by stripping corpse-related keywords from the corpse's name
    local part = obj:name_get() or ""
    for _, word in ipairs({ "headless", "corpse", "half", "burnt", "chunks", "beaten", "bloody" }) do
        part = part:gsub(word, "")
    end
    part = part:match("^%s*(.-)%s*$")  -- trim

    local head = dbat.obj_create()
    if not head then return end
    head:name_set("bloody head " .. part)
    head:description_set(string.format("@wThe bloody head of %s@w is lying here@n", part))
    head:short_description_set(string.format("@wThe bloody head of %s@w@n", part))
    head:type_set(IT.OTHER)
    head:wear_flag_set(IWEAR.TAKE, true)
    head:extra_flag_set(EF.UNIQUE_SAVE, true)
    head:value_set(4, 1)
    head:value_set(5, 1)
    head:weight_set(math.random(4, 10))
    head:to_char(ch)
end

return {
    id      = "spoil",
    aliases = { {"spoil", 4} },
    execute = execute,
}
