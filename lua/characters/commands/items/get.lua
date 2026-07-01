local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "get",
  aliases = { {"get", 3}, {"take", 3} },
  min_position = POS.RESTING,
  legacy = "get",
}
