local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "ihealth", aliases = { { "ihealth", 4 } }, flag = T.PRF.IHEALTH, off = "You will no longer view item health.\r\n", on = "You will now view item health.\r\n" }
