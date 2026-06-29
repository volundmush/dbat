local text = dbat.lib.text

local INTERVAL_SECONDS = 60 * 60
local INTERVAL_MS = INTERVAL_SECONDS * 1000
local DAY_SECONDS = 24 * INTERVAL_SECONDS
local CATCHUP_SECONDS = 3 * DAY_SECONDS
local CATCHUP_CAP = 75000

local function now()
    return os.time()
end

local function schedule_tick(cond)
    if not cond:event_pending("tick") then
        cond:schedule_event("tick", INTERVAL_MS, INTERVAL_MS)
    end
end

local function process_interest(ch, cond, catchup)
    local current = now()
    local last = cond:number_get("last_ts")
    if last <= 0 or last > current then
        cond:number_set("last_ts", current)
        cond:number_set("carry", 0)
        return
    end

    local elapsed = current - last
    if elapsed < INTERVAL_SECONDS then return end

    local whole_intervals = math.floor(elapsed / INTERVAL_SECONDS)
    local effective_seconds = whole_intervals * INTERVAL_SECONDS
    if catchup and effective_seconds > CATCHUP_SECONDS then
        effective_seconds = CATCHUP_SECONDS
    end

    local daily_interest = ch:der_total("bank_interest")
    if daily_interest <= 0 then
        cond:number_set("last_ts", current)
        cond:number_set("carry", 0)
        return
    end

    local raw = daily_interest * effective_seconds + cond:number_get("carry")
    local payout = math.floor(raw / DAY_SECONDS)
    local carry = raw % DAY_SECONDS

    if catchup and payout > CATCHUP_CAP then
        payout = CATCHUP_CAP
        carry = 0
    end

    if payout > 0 then
        ch:stat_mod("money_bank", payout)
        ch:send_line("@cBank Interest@D: @Y%s@n", text.add_commas(payout))
    end

    cond:number_set("carry", carry)
    if catchup then
        cond:number_set("last_ts", current)
    else
        cond:number_set("last_ts", last + effective_seconds)
    end
end

return {
    id = "bank_interest",
    name = "Bank Interest",
    tags = { "bank_interest", "system" },
    persistent = true,
    on_apply = function(ch, cond)
        if cond:number_get("last_ts") <= 0 then
            cond:number_set("last_ts", now())
        end
        schedule_tick(cond)
    end,
    on_game_activate = function(ch, cond)
        schedule_tick(cond)
    end,
    on_remove = function(ch, cond)
        cond:cancel_event("tick")
    end,
    on_event = function(ch, cond, event)
        if event == "tick" then
            process_interest(ch, cond, false)
        elseif event == "catchup" then
            process_interest(ch, cond, true)
        end
    end,
}
