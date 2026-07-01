local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "refuel",
  aliases = { {"refuel", 5} },
  min_position = POS.SITTING,
}
