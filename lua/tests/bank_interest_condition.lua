local test = require("lua.test").new()
local dbat = require("dbat")

local function mob()
  return assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
end

local function dispatch(ch, event)
  local def = assert(dbat.get("conditions", "bank_interest"))
  local cond = assert(ch:condition("bank_interest"))
  def:dispatch_event(ch, cond, event)
end

test:case("bank interest pays an hourly slice", function(t)
  local ch = mob()
  ch:stat_set("money_bank", 1000000)
  t:eq(ch:condition_apply("bank_interest"), true)

  local cond = assert(ch:condition("bank_interest"))
  cond:number_set("last_ts", os.time() - 3600)
  dispatch(ch, "tick")

  t:eq(ch:stat_get("money_bank"), 1001041)
  t:assert(cond:number_get("last_ts") > os.time() - 60, "last_ts should advance")
  ch:extract()
end)

test:case("bank interest catchup is capped at three max days", function(t)
  local ch = mob()
  ch:stat_set("money_bank", 1000000)
  t:eq(ch:condition_apply("bank_interest"), true)

  local cond = assert(ch:condition("bank_interest"))
  cond:number_set("last_ts", os.time() - 10 * 86400)
  dispatch(ch, "catchup")

  t:eq(ch:stat_get("money_bank"), 1075000)
  t:assert(os.time() - cond:number_get("last_ts") <= 1, "catchup should discard time beyond the cap")
  ch:extract()
end)

test:case("bank interest condition is persistent", function(t)
  local def = assert(dbat.get("conditions", "bank_interest"))
  t:eq(def.persistent, true)
end)

return test:run()
