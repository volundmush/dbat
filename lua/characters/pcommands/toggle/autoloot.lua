local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "autoloot", aliases = { { "autoloot", 6 } }, flag = T.PRF.AUTOLOOT, off = "Autoloot disabled.\r\n", on = "Autoloot enabled.\r\n" }
