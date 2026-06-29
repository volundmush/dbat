local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "noeq", aliases = { { "noeq", 4 } }, flag = T.PRF.NOEQSEE, off = "You will now see equipment when looking at someone.\r\n", on = "You will no longer see equipment when looking at someone.\r\n" }
