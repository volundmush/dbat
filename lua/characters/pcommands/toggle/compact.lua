local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "compact", aliases = { { "compact", 7 } }, flag = T.PRF.COMPACT, off = "Compact mode off.\r\n", on = "Compact mode on.\r\n" }
