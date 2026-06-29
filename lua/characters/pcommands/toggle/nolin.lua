local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nolin", aliases = { { "nolin", 5 } }, flag = T.PRF.NODEC, off = "Screen Reader Friendly Mode Deactivated.\r\n", on = "Screen Reader Friendly Mode Activated..\r\n" }
