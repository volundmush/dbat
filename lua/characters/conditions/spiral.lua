local dbat   = require("dbat")
local attack = require("lua.libs.attack")
local function act() return dbat.lib.act end

local FIRST_TICK_MS    = 100
local TICK_INTERVAL_MS = 1000

local HIT_LOCS = {
    { name = "chest", dam_mult = 1.0 },
    { name = "head",  dam_mult = 2.0 },
    { name = "body",  dam_mult = 1.0 },
    { name = "arm",   dam_mult = 0.5 },
    { name = "leg",   dam_mult = 0.5 },
}

local function do_tick(ch, cond)
    local target_id = cond:number_get("target_id") or 0
    local skill     = cond:number_get("skill") or 0
    local is_first  = (cond:number_get("first") or 1) == 1

    local target = (target_id > 0 and dbat.characters.by_id(target_id)) or ch:fighting_get()

    if not target then
        ch:send_line("@WHaving lost your target you slow down until your vortex disappears, and end your attack.@n")
        ch:act_around("@C$n@W slows down until $s vortex disappears.@n")
        ch:condition_remove("spiral")
        return
    end

    if ch:charge_get() <= 0 then
        ch:send_line("@WHaving no more charged ki you slow down until your vortex disappears, and end your attack.@n")
        ch:act_around("@C$n@W slows down until $s vortex disappears.@n")
        ch:condition_remove("spiral")
        return
    end

    if is_first then
        cond:number_set("first",     0)
        cond:number_set("target_id", target:id_get())
    end

    local max_ki  = ch:meter_max("ki")
    local level   = ch:stat_get("level") or 1
    local ki_cost = math.floor(max_ki * (is_first and 0.5 or 0.05))
    local base    = math.floor((max_ki * (is_first and 0.5 or 0.01) + level * 1000) * 1.25)

    -- Accuracy roll: mirrors handle_spiral's prob/perc/avo pattern via attack.lua helpers
    local prob  = attack._roll_accuracy(ch, skill)
    local speed = attack._speed_modifier(ch, target)
    local def   = attack._defense_total(target)
    local avo   = math.floor(def / 4)
    local eff   = prob - avo + speed
    local thr   = dbat.axion_dice(0) - 20

    local blk = target:skill_get("block") or 0
    local dge = target:skill_get("dodge") or 0

    if eff < thr then
        if (target:meter_current("stamina") or 0) > 0 then
            if blk > math.random(1, 130) then
                act().message({
                    actor  = "@C$N@W moves quickly and blocks your Spiral Comet blast!@n",
                    target = "@WYou move quickly and block @C$n's@W Spiral Comet blast!@n",
                    room   = "@C$N@W moves quickly and blocks @c$n's@W Spiral Comet blast!@n",
                }, { actor = ch, target = target })
                ch:meter_mod_int("ki", -ki_cost)
                target:damage({ powerlevel = math.floor(base / 4) }, ch)
                if not ch:fighting_get() then ch:start_fighting(target) end
                if not target:fighting_get() then target:start_fighting(ch) end
                return
            elseif dge > math.random(1, 130) then
                act().message({
                    actor  = "@C$N@W manages to dodge your Spiral Comet blast, letting it slam into the surroundings!@n",
                    target = "@WYou dodge @C$n's@W Spiral Comet blast, letting it slam into the surroundings!@n",
                    room   = "@C$N@W manages to dodge @c$n's@W Spiral Comet blast, letting it slam into the surroundings!@n",
                }, { actor = ch, target = target })
                local room = target:room_get()
                if room then room:send_line("@wA bright explosion erupts from the impact!\r\n") end
                ch:meter_mod_int("ki", -ki_cost)
                if not ch:fighting_get() then ch:start_fighting(target) end
                if not target:fighting_get() then target:start_fighting(ch) end
                return
            end
        end
        act().message({
            actor  = "@WYou can't believe it but your Spiral Comet blast misses, flying through the air harmlessly!@n",
            target = "@C$n@W fires a Spiral Comet blast at you, but misses!@n",
            room   = "@c$n@W fires a Spiral Comet blast at @C$N@W, but somehow misses!@n",
        }, { actor = ch, target = target })
        ch:meter_mod_int("ki", -ki_cost)
        if not ch:fighting_get() then ch:start_fighting(target) end
        if not target:fighting_get() then target:start_fighting(ch) end
    else
        local loc  = HIT_LOCS[math.random(1, 5)]
        local dmg  = math.floor(base * loc.dam_mult)
        local part = loc.name
        act().message({
            actor  = ("@WYou launch a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S %s and explodes!@n"):format(part),
            target = ("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at YOU! It slams into YOUR %s and explodes!@n"):format(part),
            room   = ("@C$n@W launches a bright @mp@Mu@mr@Mp@ml@Me@W ball of energy down at @c$N@W! It slams into $S %s and explodes!@n"):format(part),
        }, { actor = ch, target = target })
        ch:meter_mod_int("ki", -ki_cost)
        target:damage({ powerlevel = dmg }, ch)
        if not ch:fighting_get() then ch:start_fighting(target) end
        if not target:fighting_get() then target:start_fighting(ch) end
    end
end

return {
    id         = "spiral",
    name       = "Spiral Comet",
    tags       = { "spiral" },
    persistent = false,

    on_apply = function(ch, cond)
        cond:schedule_event("tick", FIRST_TICK_MS, TICK_INTERVAL_MS)
    end,

    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", TICK_INTERVAL_MS, TICK_INTERVAL_MS)
        end
    end,

    on_remove = function(ch, cond)
        cond:cancel_event("tick")
    end,

    on_event = function(ch, cond, event)
        if event == "tick" then do_tick(ch, cond) end
    end,
}
