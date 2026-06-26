local dbat = require("dbat")
local act  = dbat.lib.act

-- Copy conditions tagged clones_inherit from original to clone.
-- No existing conditions use this tag yet; infrastructure for future use.
local function inherit_conditions(original, clone)
    for cond_id in pairs(original:conditions_active()) do
        local def = dbat.registry.conditions[cond_id]
        if def and def.clones_inherit then
            clone:condition_apply(cond_id, "multiform", "clone")
            for k, v in pairs(original:condition_number_vars(cond_id)) do
                clone:condition_number_set(cond_id, k, v)
            end
            for k, v in pairs(original:condition_string_vars(cond_id)) do
                clone:condition_string_set(cond_id, k, v)
            end
        end
    end
end

local function generate_multiform(ch, count)
    local proto = dbat.mob_protos.by_id(25)
    if not proto then
        dbat.send_to_imm("Multiform Clone prototype (vnum 25) doesn't exist!")
        return
    end

    local room        = ch:room_get()
    local clone_name  = string.format("%s's Clone", ch:name_get())
    local clone_sdesc = string.format("%s's @CClone@n", ch:name_get())
    local clone_ldesc = string.format("%s's @CClone@w is standing here.@n\n", ch:name_get())

    for _ = 1, count do
        local clone = proto:spawn()
        clone:name_set(clone_name)
        clone:short_description_set(clone_sdesc)
        clone:long_description_set(clone_ldesc)
        local desc = ch:description_get()
        if desc and desc ~= "" then
            clone:description_set(desc)
        end
        clone:race_set(ch:race_get())
        clone:sex_set(ch:sex_get())

        -- Copy appearance
        clone:aura_set(ch:aura_get())
        clone:hairl_set(ch:hairl_get())
        clone:hairs_set(ch:hairs_get())
        clone:hairc_set(ch:hairc_get())
        clone:skin_set(ch:skin_get())
        clone:eye_set(ch:eye_get())
        clone:distfea_set(ch:distfea_get())

        -- Copy stats
        for _, s in ipairs({ "weight", "height", "level", "powerlevel", "ki", "stamina", "alignment" }) do
            clone:stat_set(s, ch:stat_get(s))
        end

        -- Register in Zig clone map and apply clone conditions
        ch:clone_add(clone)
        clone:condition_apply_with_number("multiform_clone", "multiform", "clone", "target_id", ch:id_get())
        clone:condition_apply_with_number("multiform",       "skill",     "multiform", "original_id", ch:id_get())

        inherit_conditions(ch, clone)

        clone:to_room(room)
        clone:add_follower(ch)
    end

    if not ch:condition_has("multiform_original") then
        ch:condition_apply("multiform_original", "skill", "multiform")
    end
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if not ch:is_npc() and not ch:know_skill("multiform") then
        ch:send_line("You do not know how to perform that technique.")
        return
    end

    if arg == "merge" then
        local room  = ch:room_get()
        local found = false
        for clone in ch:clones() do
            if clone:room_get() == room then
                clone:extract()
                found = true
            end
        end
        if not found then
            ch:send_line("You have no multiforms present to merge with!")
        else
            ch:condition_remove("multiform_original", "command")
        end
        return
    end

    if arg == "split" then
        local skill   = ch:skill_get("multiform")
        local max_ki  = ch:meter_max_get("ki")
        local max_st  = ch:meter_max_get("stamina")
        local cost    = math.floor(((max_ki * 0.005) + (max_st * 0.005) + 2) * (skill * 0.2))
        local penalty = ch:is_fighting() and math.random(8, 15) or 0
        local roll    = dbat.axion_dice(penalty)

        if ch:meter_get("ki") < cost then
            ch:send_line("You do not have enough ki to split!")
            return
        end
        if ch:meter_get("stamina") < cost then
            ch:send_line("You do not have enough stamina to split!")
            return
        end

        ch:improve_skill("multiform", 1)

        if skill < roll then
            act.to_char(ch, "@YYou focus your ki into your body while concentrating on the image of your body splitting into two. @yYou lose your concentration and fail to split though...@n")
            act.around(ch, "@y$n@Y seems to concentrate really hard for a moment, before relaxing.@n", {})
            ch:meter_mod_int("stamina", -cost)
            ch:meter_mod_int("ki", -cost)
            return
        end

        act.to_char(ch, "@YYou focus your ki into your body while concentrating on the image of your body splitting into two. Another you splits out of your body!@n")
        act.around(ch, "@YSuddenly @y$n@Y seems to concentrates really and after a brief moment splits into two copies of $mself!@n", {})
        generate_multiform(ch, 1)
        return
    end

    ch:send_line("Huh? Try help multiform")
end

return {
    id      = "multiform",
    aliases = { {"multiform", 8} },
    execute = execute,
}
