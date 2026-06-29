local dbat = require("dbat")

local text = dbat.lib.text

local function in_arena(ch)
    local room = ch:room_get()
    local vnum = room and room:vnum_get() or 0
    return vnum >= 17800 and vnum <= 17874
end

return {
    id         = "saiyan_zenkai",
    name       = "Saiyan Zenkai",
    tags       = { "goop", "saiyan_zenkai" },
    persistent = false,

    on_remove = function(ch, cond, reason)
        if reason ~= "expired" then return end

        ch:meter_set("powerlevel", 500000)
        if ch:meter_get("ki") < 250000 then ch:meter_set("ki", 250000) end
        if ch:meter_get("stamina") < 250000 then ch:meter_set("stamina", 250000) end

        if not in_arena(ch) then
            local zenkai_pl = math.floor(ch:stat_get("powerlevel") * 0.03)
            local zenkai_ki = math.floor(ch:stat_get("ki") * 0.015)
            local zenkai_st = math.floor(ch:stat_get("stamina") * 0.015)
            ch:stat_mod("powerlevel", zenkai_pl)
            ch:stat_mod("ki", zenkai_ki)
            ch:stat_mod("stamina", zenkai_st)
            ch:send_line("@D[@YZ@ye@wn@Wk@Ya@yi @YB@yo@wo@Ws@Yt@D] @WYou feel much stronger!")
            ch:send_line("@D[@RPL@Y:@n+%s@D] @D[@CKI@Y:@n+%s@D] @D[@GSTA@Y:@n+%s@D]@n",
                text.add_commas(zenkai_pl), text.add_commas(zenkai_ki), text.add_commas(zenkai_st))
        end

        ch:act("@RYou collapse to the ground, body pushed beyond the typical limits of exhaustion. The passage of time distorts and an indescribable amount of time passes as raw emotions pass through your very being. Your eyes open and focus with a newfound clarity as your unadulterated emotions and feelings revive you for a second wind!@n",
            true, nil, nil, "char")
        ch:act("@r$n@R collapses to the ground, seemingly dead. After a brief moment, their eyes flash open with a determined look on their face!",
            true, nil, nil, "room")
    end,
}
