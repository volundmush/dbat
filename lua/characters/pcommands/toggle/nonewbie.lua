local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nonewbie", aliases = { { "nonewbie", 8 } }, flag = T.PRF.NOAUCT, off = "You can now hear newbie.\r\n", on = "You are now deaf to newbie.\r\n" }
