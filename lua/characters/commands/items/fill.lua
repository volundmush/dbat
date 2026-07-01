local POS = require("dbat").consts.positions
local I = require("lua.libs.item_legacy_commands")

return I.legacy {
  id = "fill",
  aliases = { {"fill", 3} },
  min_position = POS.STANDING,
}
