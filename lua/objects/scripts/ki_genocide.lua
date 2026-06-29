-- Genocide ball countdown + single-target detonation.
-- Script vars: damage (i64), target_id (i64), user_id (i64), kidist (i64)
return {
    id = "ki_genocide",

    on_event = function(obj, script, event_name)
        if event_name ~= "tick" then return end
        local db   = require("dbat")
        local room = obj:room_get()
        if not room then obj:extract(); return end

        local kidist = script:number_get("kidist")
        local user   = db.characters.by_id(script:number_get("user_id"))
        local target = db.characters.by_id(script:number_get("target_id"))
        require("lua.objects.scripts.huge_attack").provoke_room_mobs(obj, script)

        if kidist > 1 then
            script:number_set("kidist", kidist - 1)
            room:send_text(("@MA swirling @mGenocide@M ball descends! @W(%d seconds remain)@n\r\n"):format(kidist - 1))
            if user then user:send_line("@MYour Genocide attack closes in!@n") end
            return
        end

        -- Detonation: single target
        room:send_text("@MThe @mGenocide@M ball DETONATES in a blast of deadly energy!@n\r\n")
        if target and target:room_get() and target:room_get():vnum_get() == room:vnum_get() then
            local dmg = script:number_get("damage")
            target:damage({ powerlevel = dmg }, user)
            target:send_line("@MThe Genocide attack detonates directly on you!@n")
        end
        obj:extract()
    end,
}
