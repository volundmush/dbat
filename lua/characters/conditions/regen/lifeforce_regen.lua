local dbat = require("dbat")
local BON  = dbat.consts.bonuses

local REGEN_INTERVAL_MS = 100000
local HEAL_INTERVAL_MS  = 5000

local function on_regen_tick(ch, cond)
    local gain = ch:der_total("lifeforce_regen")
    ch:meter_mod_int("lifeforce", gain)
end

local function on_heal_tick(ch)
    local life_pct = ch:stat_get("life_percent") or 0
    if life_pct <= 0 or ch:race_get() == "android" then return end

    local pl_max = ch:meter_max("powerlevel")
    if pl_max > 0 and ch:meter_current("powerlevel") / 1000000.0 >= life_pct / 100.0 then
        return
    end

    local cur_lf = ch:meter_current("lifeforce")
    if cur_lf <= 0 then return end

    -- ~2/15 trigger chance per second at 5s interval → 2/3 chance per tick
    if math.random(1, 3) < 3 then return end

    local max_lf       = ch:meter_max("lifeforce")
    local healing_glow = ch:condition_has("healing_glow")
    local diehard      = ch:bonus_flagged(BON.DIEHARD)
    local race         = ch:race_get()
    local mutant_regen = race == "mutant" and
                         (ch:genome_get(0) == 2 or ch:genome_get(1) == 2)
    local is_kanassan  = race == "kanassan"

    local threshold = is_kanassan and max_lf * 0.03 or max_lf * 0.05
    if cur_lf >= threshold or healing_glow then
        local lfcost
        local refill
        if     diehard and not mutant_regen then refill = math.floor(max_lf * 0.1)
        elseif diehard and     mutant_regen then refill = math.floor(max_lf * 0.17)
        elseif mutant_regen                 then refill = math.floor(max_lf * 0.12)
        elseif is_kanassan then
            refill = math.floor(max_lf * 0.03)
            lfcost = refill
        else
            refill = math.floor(max_lf * 0.05)
        end
        lfcost = lfcost or math.floor(max_lf * 0.05)
        ch:meter_mod_int("powerlevel", refill)
        if not healing_glow then
            ch:meter_mod_int("lifeforce", -lfcost)
        end
    else
        ch:meter_mod_int("powerlevel", cur_lf)
        ch:meter_mod("lifeforce", -2000000)
    end
    ch:send_line("@YYour life force has kept you strong@n!")
end

return {
    id         = "lifeforce_regen",
    name       = "Lifeforce Regen",
    persistent = false,

    derived = {
        id               = "lifeforce_regen",
        name             = "Lifeforce Regen",
        modifier_targets = { { "regen", "vitals" } },
        calculate_base   = function(ch)
            return math.ceil((ch:der_total("ki_regen") + ch:der_total("stamina_regen")) / 4)
        end,
    },

    on_apply = function(ch, cond)
        cond:schedule_event("tick", REGEN_INTERVAL_MS, REGEN_INTERVAL_MS)
        cond:schedule_event("heal", HEAL_INTERVAL_MS,  HEAL_INTERVAL_MS)
    end,

    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", REGEN_INTERVAL_MS, REGEN_INTERVAL_MS)
        end
        if not cond:event_pending("heal") then
            cond:schedule_event("heal", HEAL_INTERVAL_MS, HEAL_INTERVAL_MS)
        end
    end,

    on_remove = function(ch, cond)
        cond:cancel_event("tick")
        cond:cancel_event("heal")
    end,

    on_event = function(ch, cond, event)
        if event == "tick" then on_regen_tick(ch, cond)
        elseif event == "heal" then on_heal_tick(ch)
        end
    end,
}
