local dbat = require("dbat")
local act  = dbat.lib.act
local BON  = dbat.consts.bonuses

local PULSE_2SEC = 20

-- Ingredient category → valid object vnums
local INGREDIENT_VNUMS = {
    tomato    = { 17212 },
    potato    = { 17213 },
    onion     = { 17215 },
    cucumber  = { 17217 },
    redpep    = { 17219 },
    carrot    = { 17223 },
    greenbean = { 17225 },
    normmeat  = { 1612 },
    normfish  = { 1000, 1004, 1008, 1012 },
    goodfish  = { 1001, 1005, 1009, 1013 },
    greatfish = { 1002, 1006, 1010, 1014 },
    bestfish  = { 1003, 1007, 1011, 1015 },
    brownmush = { 1608 },
    garlic    = { 1131 },
    rice      = { 1590 },
    flour     = { 1591 },
    lettuce   = { 17227 },
    appleplum = { 8001 },
    fberry    = { 4901 },
    carambola = { 3416 },
}

-- Reverse map: vnum → ingredient category
local VNUM_TO_INGREDIENT = {}
for cat, vnums in pairs(INGREDIENT_VNUMS) do
    for _, v in ipairs(vnums) do
        VNUM_TO_INGREDIENT[v] = cat
    end
end

-- Missing ingredient → display message
local function missing_msg(cat, n)
    local s = n > 1
    if cat == "tomato"    then return string.format("@WYou need @m%d@W tomato%s for this recipe.@n", n, s and "es" or "") end
    if cat == "potato"    then return string.format("@WYou need @m%d@W potato%s for this recipe.@n", n, s and "es" or "") end
    if cat == "onion"     then return string.format("@WYou need @m%d@W onion%s for this recipe.@n", n, s and "s" or "") end
    if cat == "appleplum" then return string.format("@WYou need @m%d@W appleplum%s for this recipe.@n", n, s and "s" or "") end
    if cat == "fberry"    then return string.format("@WYou need @m%d@W frozen berry%s for this recipe.@n", n, s and "s" or "") end
    if cat == "carambola" then return string.format("@WYou need @m%d@W carambola%s for this recipe.@n", n, s and "s" or "") end
    if cat == "lettuce"   then return string.format("@WYou need @m%d@W head%s of lettuce for this recipe.@n", n, s and "s" or "") end
    if cat == "flour"     then return string.format("@WYou need @m%d@W cup%s of white flour for this recipe.@n", n, s and "s" or "") end
    if cat == "rice"      then return string.format("@WYou need @m%d@W cup%s of white rice for this recipe.@n", n, s and "s" or "") end
    if cat == "garlic"    then return string.format("@WYou need @m%d@W garlic clove%s for this recipe.@n", n, s and "s" or "") end
    if cat == "carrot"    then return string.format("@WYou need @m%d@W carrot%s for this recipe.@n", n, s and "s" or "") end
    if cat == "cucumber"  then return string.format("@WYou need @m%d@W cucumber%s for this recipe.@n", n, s and "s" or "") end
    if cat == "greenbean" then return string.format("@WYou need @m%d@W green bean%s for this recipe.@n", n, s and "s" or "") end
    if cat == "normmeat"  then return string.format("@WYou need @m%d@W normal raw steak%s for this recipe.@n", n, s and "s" or "") end
    if cat == "redpep"    then return string.format("@WYou need @m%d@W chili pepper%s for this recipe.@n", n, s and "s" or "") end
    if cat == "normfish"  then return string.format("@WYou need @m%d@W black bass, flounder, narri, or gravel reboi for this recipe.@n", n) end
    if cat == "goodfish"  then return string.format("@WYou need @m%d@W silver trout, silver eel, valbish, or voos pike for this recipe.@n", n) end
    if cat == "greatfish" then return string.format("@WYou need @m%d@W striped bass, cobia, gusblat, or shadowfish for this recipe.@n", n) end
    if cat == "bestfish"  then return string.format("@WYou need @m%d@W blue catfish, tambor, repeeil, or shadeeel for this recipe.@n", n) end
    if cat == "brownmush" then return string.format("@WYou need @m%d@W brown mushroom%s for this recipe.@n", n, s and "s" or "") end
    return nil
end

-- Recipe definitions: ingredients, output vnum, difficulty bonus, and bonuses
-- campfire_ok: can be cooked on a campfire (ITEM_CAMPFIRE). Stove allows all.
-- NOTE: Due to cook_element() returning bool, the Lua binding always returns 0/1
--       so campfire restriction applies to all heat sources (faithful to C++ behavior).
local RECIPES = {
    [1]  = { ingr = { normmeat = 1 },                                                   meal = 1221, prob_bonus = 8,  psbonus = 1, expbonus = 5,   campfire_ok = true  },
    [2]  = { ingr = { tomato = 2 },                                                     meal = 1220, prob_bonus = 0,  psbonus = 2, expbonus = 15,  campfire_ok = false },
    [3]  = { ingr = { potato = 2 },                                                     meal = 1222, prob_bonus = 0,  psbonus = 1, expbonus = 20,  campfire_ok = false },
    [4]  = { ingr = { potato = 1, tomato = 1, carrot = 1, greenbean = 1, onion = 1 },  meal = 1223, prob_bonus = 0,  psbonus = 3, expbonus = 45,  campfire_ok = false },
    [5]  = { ingr = { normmeat = 1, potato = 1, tomato = 1, garlic = 1 },              meal = 1224, prob_bonus = 0,  psbonus = 2, expbonus = 50,  campfire_ok = false },
    [6]  = { ingr = { normmeat = 1, redpep = 4, tomato = 2 },                          meal = 1226, prob_bonus = 0,  psbonus = 0, expbonus = 100, campfire_ok = false },
    [7]  = { ingr = { normfish = 1 },                                                   meal = 1227, prob_bonus = 6,  psbonus = 2, expbonus = 12,  campfire_ok = true  },
    [8]  = { ingr = { goodfish = 1 },                                                   meal = 1228, prob_bonus = 10, psbonus = 3, expbonus = 40,  campfire_ok = true  },
    [9]  = { ingr = { greatfish = 1 },                                                  meal = 1229, prob_bonus = 12, psbonus = 5, expbonus = 80,  campfire_ok = true  },
    [10] = { ingr = { bestfish = 1 },                                                   meal = 1230, prob_bonus = 16, psbonus = 7, expbonus = 125, campfire_ok = true  },
    [11] = { ingr = { rice = 1 },                                                       meal = 1231, prob_bonus = 0,  psbonus = 1, expbonus = 8,   campfire_ok = false },
    [12] = { ingr = { rice = 1, normfish = 1 },                                         meal = 1232, prob_bonus = 0,  psbonus = 2, expbonus = 20,  campfire_ok = false },
    [13] = { ingr = { flour = 1 },                                                      meal = 1233, prob_bonus = 0,  psbonus = 1, expbonus = 8,   campfire_ok = false },
    [14] = { ingr = { tomato = 1, cucumber = 1, carrot = 1, lettuce = 1 },             meal = 1234, prob_bonus = 0,  psbonus = 5, expbonus = 8,   campfire_ok = false },
    [15] = { ingr = { flour = 1, appleplum = 1 },                                       meal = 1235, prob_bonus = 0,  psbonus = 1, expbonus = 9,   campfire_ok = false },
    [16] = { ingr = { flour = 1, fberry = 1 },                                          meal = 1236, prob_bonus = 0,  psbonus = 3, expbonus = 12,  campfire_ok = false },
    [17] = { ingr = { flour = 1, carambola = 1 },                                       meal = 1237, prob_bonus = 0,  psbonus = 1, expbonus = 9,   campfire_ok = false },
}

-- Check (and optionally consume) ingredients. Returns true if all present.
-- When consume=true, extracts the items and always returns true.
local function check_ingredients(ch, needed, consume)
    local remaining = {}
    for cat, count in pairs(needed) do
        remaining[cat] = count
    end

    for obj in ch:inventory() do
        local cat = VNUM_TO_INGREDIENT[obj:vnum_get()]
        if cat and remaining[cat] and remaining[cat] > 0 then
            remaining[cat] = remaining[cat] - 1
            if consume then obj:extract() end
        end
    end

    if consume then return true end

    local ok = true
    for cat, left in pairs(remaining) do
        if left > 0 then
            ok = false
            local msg = missing_msg(cat, left)
            if msg then ch:send_line(msg) end
        end
    end
    return ok
end

local MENU = table.concat({
    "@D---------------------@RCooking@D---------------------@n",
    "@Y 1@B) @CCooked Steak\t\t@Y17@B) @CCarambola Bread@n",
    "@Y 2@B) @CTomato Soup\t\t@n",
    "@Y 3@B) @CPotato Soup\t\t@n",
    "@Y 4@B) @CVegetable Soup\t\t@n",
    "@Y 5@B) @CMeat Stew\t\t\t@n",
    "@Y 6@B) @CChili Soup\t\t@n",
    "@Y 7@B) @CGrilled Fish\t\t@n",
    "@Y 8@B) @CGood Grilled Fish\t\t@n",
    "@Y 9@B) @CGreat Grilled Fish\t@n",
    "@Y10@B) @CMagnificent Grilled Fish\t@n",
    "@Y11@B) @CCooked White Rice\t\t@n",
    "@Y12@B) @CSushi\t\t\t@n",
    "@Y13@B) @CWhite Bread\t\t@n",
    "@Y14@B) @CBasic Salad\t\t@n",
    "@Y15@B) @CAppleplum Chasan\t\t@n",
    "@Y16@B) @CFrozen Berry Muffin\t@n",
    "@wSyntax: cook (recipe number)@n",
}, "\r\n")

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:is_npc() then return end

    local room     = ch:room_get()
    local cook_val = room:cook_element()

    if cook_val == 0 then
        ch:send_line("You need a campfire or Flambus Stove nearby to cook.")
        return
    end

    local skill = ch:skill_get("cooking")
    if skill == 0 then
        ch:send_line("You don't even know the basics!")
        return
    end

    if arg == "" then
        ch:send_line(MENU)
        return
    end

    local num = tonumber(arg)
    local recipe = num and RECIPES[math.floor(num)]
    if not recipe then
        ch:send_line("That is not a valid dish!")
        return
    end

    if not check_ingredients(ch, recipe.ingr, false) then
        return
    end

    -- cook_element always returns 0 or 1 (bool), so campfire restriction applies
    -- to all heat sources (faithful to C++ behavior)
    if not recipe.campfire_ok then
        ch:send_line("You can not cook that dish over a campfire.")
        return
    end

    check_ingredients(ch, recipe.ingr, true)

    local prob = dbat.axion_dice(0) + recipe.prob_bonus
    if skill < prob then
        act.to_char(ch, "@wYou screw up the preparation of the recipe and end up wasting the ingredients!@n")
        act.around(ch, "@C$n@w starts to prepare some food, but ends up ruining the ingredients instead!@n", {})
        ch:improve_skill("cooking", 0)
        ch:wait_set(PULSE_2SEC)
        return
    end

    local proto = dbat.obj_protos.by_id(recipe.meal)
    if not proto then return end
    local meal = proto:spawn()
    meal:to_char(ch)

    act.to_char(ch, "@wYou carefully prepare the ingredients and then start cooking them. After a while of patience  and skillful care you successfully make @D'@C$p@D'@w!@n", { object = meal })
    act.around(ch, "@C$n@w carefully prepares some ingredients and starts cooking them. After a while of patience and skillful care $e succeeds in making @D'@C$p@D'@w!@n", { object = meal })
    ch:improve_skill("cooking", 0)

    local psbonus  = recipe.psbonus
    local expbonus = recipe.expbonus
    if ch:bonus_flagged(BON.RECIPE) then
        psbonus  = psbonus + 1
        expbonus = expbonus + 3
    end
    if psbonus > 0 and skill * 0.10 > 0 then
        psbonus = math.floor(skill * 0.10 * psbonus)
    end
    if expbonus > 0 then
        expbonus = skill * expbonus
    end

    meal:value_set(2, psbonus)   -- VAL_FOOD_PSBONUS
    meal:value_set(6, expbonus)  -- VAL_FOOD_EXPBONUS

    ch:wait_set(PULSE_2SEC)
end

return {
    id      = "cook",
    aliases = { {"cook", 4} },
    execute = execute,
}
