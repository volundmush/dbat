local T = require("lua.libs.pcommand_toggles")
return T.aff{ id = "sneak", aliases = { { "sneak", 5 } }, flag = T.AFF.SNEAK, off = "You will no longer attempt to be sneaky.\r\n", on = "You will try to move as silently as you can.\r\n" }
