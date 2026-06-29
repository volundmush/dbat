local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "autogold", aliases = { { "autogold", 6 } }, flag = T.PRF.AUTOGOLD, off = "Autogold disabled.\r\n", on = "Autogold enabled.\r\n" }
