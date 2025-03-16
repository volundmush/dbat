import mudforge
from mudforge.game.application import Application as _OldApp

import dbat_ext

class Application(_OldApp):
    
    async def setup_dbat(self):
        dbat_ext.load_db()
    
    async def setup(self):
        await super().setup()
        await self.setup_dbat()