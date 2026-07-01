local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "twohand",
  aliases = { {"twohand", 7} },
  min_position = POS.DEAD,
}
