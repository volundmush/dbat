local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "vieworder", aliases = { { "vieworder", 4 } }, flag = T.PRF.VIEWORDER, off = "Viewing newest board messages at top of list.\r\n", on = "Viewing newest board messages at bottom of list.\r\n" }
