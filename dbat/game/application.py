import mudforge
from mudforge.game.application import Application as _OldApp

import dbat_ext

class Application(_OldApp):
    
    async def setup_asyncpg(self):
        """
        We don't actually need asyncpg.
        So we'll just pass. This prevents
        the connection error.
        """
        pass
    
    async def postgre_listener(self):
        pass
    
    async def setup_dbat(self):
        dbat_ext.load_db()
    
    async def setup(self):
        await super().setup()
        await self.setup_dbat()
    
    async def start(self):
        await super().start()
        # Start the game loop. We'll run at 50ms per tick, 5m save timer.
        self.task_group.create_task(dbat_ext.run_game(0.05, 60.0 * 5.0))
        