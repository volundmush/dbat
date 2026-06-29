local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "health", aliases = { { "health", 3 } }, flag = T.PRF.GHEALTH, off = "You will no longer view group health.\r\n", on = "You will now view group health.\r\n" }
