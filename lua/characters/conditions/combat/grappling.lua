local dbat = require("dbat")
local act  = require("lua.libs.act")

local INTERVAL_MS = 5000

local function on_tick(ch)
    local target = ch:grappling_get()
    if not target then return end
    local gtype = ch:graptype_get()

    if gtype == 2 and math.random(1, 11) >= 8 then
        local st_cur = target:meter_current("stamina")
        local st_max = target:meter_max("stamina")
        if st_cur >= st_max / 8 then
            act.to_char(ch,     "@WYou choke @C$N@W!@n",        {actor=ch, target=target})
            act.to_char(target, "@C$n@W chokes YOU@W!@n",       {actor=ch, target=target})
            act.around(ch,      "@C$n@W chokes @c$N@W!@n",      {actor=ch, target=target})
            target:meter_mod_int("stamina", -math.floor(st_max / 8))
        else
            act.to_char(ch,     "@WYou choke @C$N@W, and $E passes out!@n",  {actor=ch, target=target})
            act.to_char(target, "@C$n@W chokes YOU@W, and you pass out!@n",  {actor=ch, target=target})
            act.around(ch,      "@C$n@W chokes @c$N@W, and $E passes out!@n",{actor=ch, target=target})
            target:condition_apply("knocked_out", "combat", "choke")
            target:position_set(dbat.consts.positions.SLEEPING)
            ch:grappling_set(nil, 0)
            target:grappled_set(nil, 0)
        end
    elseif gtype == 4 and math.random(1, 12) >= 8 then
        act.to_char(ch,     "@WYou crush @C$N@W some more!@n",    {actor=ch, target=target})
        act.to_char(target, "@C$n@W crushes YOU@W some more!@n",  {actor=ch, target=target})
        act.around(ch,      "@C$n@W crushes @c$N@W some more!@n", {actor=ch, target=target})
        local dmg = math.floor(ch:stat_get("strength") * (10 + ch:meter_max("powerlevel") * 0.005))
        ch:hurt_target(target, dmg)
    end
end

return {
    id         = "grappling",
    name       = "Grappling",
    tags       = { "grappling" },
    persistent = false,
    on_apply = function(ch, cond)
        cond:schedule_event("tick", INTERVAL_MS, INTERVAL_MS)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch) end
    end,
}
