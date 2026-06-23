local dbat = require("dbat")
local act  = dbat.lib.act
local P    = dbat.consts.pulses
local POS  = dbat.consts.positions

local function execute(ctx)
    local ch = ctx.ch

    local absorber = ch:absorbed_by_get()
    local grappler = ch:grappled_get()

    if not absorber and not grappler then
        ch:send_line("You are not in anyone's grasp!")
        return
    end

    local num    = ch:stat_get("strength") or 0
    local offender = absorber or grappler
    local opl  = offender:meter_current("powerlevel")
    local dpl  = ch:meter_current("powerlevel")
    local skill = absorber and (absorber:skill_get("absorb") or 0)
                           or  (grappler:skill_get("grapple") or 0)

    if     dpl > opl * 10 then num   = num   + math.random(10, 15)
    elseif dpl > opl * 5  then num   = num   + math.random(6, 10)
    elseif dpl > opl * 2  then num   = num   + math.random(4, 8)
    elseif dpl > opl      then num   = num   + math.random(2, 5)
    elseif dpl * 10 <= opl then skill = skill - math.random(10, 15)
    elseif dpl * 5  <= opl then skill = skill - math.random(6, 10)
    elseif dpl * 2  <= opl then skill = skill - math.random(4, 8)
    elseif dpl < opl       then skill = skill - math.random(2, 5)
    end

    local escaped = (skill - num) < dbat.axion_dice(0)

    if absorber then
        if escaped then
            act.message({
                actor  = "@c$N@W manages to break loose of your hold!@n",
                target = "@WYou manage to break loose of @C$n's@W hold!@n",
                room   = "@c$N@W manages to break loose of @C$n's@W hold!@n",
            }, { actor = absorber, target = ch })
            if not ch:fighting_get()       then ch:start_fighting(absorber) end
            if not absorber:fighting_get() then absorber:start_fighting(ch) end
            ch:absorbed_by_set(nil)
            absorber:absorbing_set(nil)
        else
            act.message({
                actor  = "@c$N@W struggles to break loose of your hold!@n",
                target = "@WYou struggle to break loose of @C$n's@W hold!@n",
                room   = "@c$N@W struggles to break loose of @C$n's@W hold!@n",
            }, { actor = absorber, target = ch })
            if math.random(1, 3) == 3 then
                local dmg = math.floor(ch:meter_max("powerlevel") * 0.025)
                absorber:damage({ powerlevel = dmg }, ch)
                -- auto-escape if absorber was knocked out
                if absorber:room_get() and absorber:position_get() == POS.SLEEPING then
                    act.message({
                        actor  = "@c$N@W manages to break loose of your hold!@n",
                        target = "@WYou manage to break loose of @C$n's@W hold!@n",
                        room   = "@c$N@W manages to break loose of @C$n's@W hold!@n",
                    }, { actor = absorber, target = ch })
                    ch:absorbed_by_set(nil)
                    absorber:absorbing_set(nil)
                end
            end
            ch:wait_set(P.two_sec)
        end
    end

    if grappler then
        if escaped then
            if grappler:graptype_get() == 4 then
                act.message({
                    actor  = "@c$N@M flexes with all $S might and causes your body to explode outward into gooey chunks!@n",
                    target = "@MYou flex with all your might and cause @C$n's@M body to explode outward into gooey chunks!@n",
                    room   = "@c$N@M flexes with all $S might and causes @C$n's@M body to explode outward into gooey chunks!@n",
                }, { actor = grappler, target = ch })
                grappler:send_line("@MYou reform your body mere moments later.@n")
                grappler:act_around("@C$n@M reforms $s body mere moments later.@n")
            else
                act.message({
                    actor  = "@c$N@W manages to break loose of your hold!@n",
                    target = "@WYou manage to break loose of @C$n's@W hold!@n",
                    room   = "@c$N@W manages to break loose of @C$n's@W hold!@n",
                }, { actor = grappler, target = ch })
            end
            if not ch:fighting_get()       then ch:start_fighting(grappler) end
            if not grappler:fighting_get() then grappler:start_fighting(ch) end
            grappler:grappling_set(nil, 0)
            ch:grappled_set(nil, 0)
        else
            act.message({
                actor  = "@c$N@W struggles to break loose of your hold!@n",
                target = "@WYou struggle to break loose of @C$n's@W hold!@n",
                room   = "@c$N@W struggles to break loose of @C$n's@W hold!@n",
            }, { actor = grappler, target = ch })
            if math.random(1, 3) == 3 then
                local dmg = math.floor(ch:meter_max("powerlevel") * 0.025)
                grappler:damage({ powerlevel = dmg }, ch)
                -- auto-escape if grappler was knocked out
                if grappler:room_get() and grappler:position_get() == POS.SLEEPING then
                    act.message({
                        actor  = "@c$N@W manages to break loose of your hold!@n",
                        target = "@WYou manage to break loose of @C$n's@W hold!@n",
                        room   = "@c$N@W manages to break loose of @C$n's@W hold!@n",
                    }, { actor = grappler, target = ch })
                    grappler:grappling_set(nil, 0)
                    ch:grappled_set(nil, 0)
                end
            end
            ch:wait_set(P.two_sec)
        end
    end
end

return {
    id      = "escape",
    aliases = { { "escape", 4 } },
    can_execute = function(ch)
        if ch:position_get() < POS.RESTING then
            return false, "You can't do that in your current state!"
        end
        return true
    end,
    execute = execute,
}
