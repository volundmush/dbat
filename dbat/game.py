import asyncio
import uuid
from pathlib import Path
import orjson
from muforge.application import Service

class GameService(Service):
    def __init__(self, app, plugin):
        super().__init__(app, plugin)
    
    @property
    def core(self):
        return self.app.plugins["core"]
    
    async def setup(self):
        """
        We have to load up the game. This is gonna be interesting!
        """
        from .types.characters import Mobile, MobilePrototype, PlayerCharacter
        from .types.objects import Object, ObjectPrototype
        from .types.shops import Shop
        from .types.guilds import Guild
        from .types.grids import Grid, Tile, Shape
        from .types.zones import Zone
        from .types.structures import Structure