import asyncio
import uuid
import dbat
from pathlib import Path
import orjson
from loguru import logger
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
        from .types.grids import Tile, Shape
        from .types.zones import Zone
        from .types.structures import Structure

        p = Path() / "assets"

        # Loading the game maps - Zones, and Structures
        # load zones
        zones = p / "zones"
        zone_parents: dict[uuid.UUID, uuid.UUID] = dict()
        if zones.exists():
            for file in zones.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    zone = Zone.load(data)
                    dbat.ZONES[zone.id] = zone
                    if par := data.get("parent", None):
                        zone_parents[zone.id] = uuid.UUID(par)
        
        # assign parents and children
        for zone_id, parent_id in zone_parents.items():
            if not (parent := dbat.ZONES.get(parent_id, None)):
                logger.warning(f"Zone {zone_id} has parent {parent_id} which does not exist.")
                continue
            if not (child := dbat.ZONES.get(zone_id, None)):
                logger.warning(f"Zone {zone_id} has parent {parent_id} but the child does not exist.")
                continue
            child.parent = parent
            parent.children.add(child)

        # load structures
        structures = p / "structures"
        if structures.exists():
            for file in structures.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    structure = Structure.load(data)
                    dbat.STRUCTURES[structure.id] = structure
        
        for k, v in dbat.STRUCTURES.items():
            v.register_location()

        objects = p / "objects"
        if objects.exists():
            for file in objects.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    obj = ObjectPrototype.load(data)
                    dbat.OBJECT_PROTOTYPES[obj.id] = obj
        
        mobiles = p / "mobiles"
        if mobiles.exists():
            for file in mobiles.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    mob = MobilePrototype.load(data)
                    dbat.MOBILE_PROTOTYPES[mob.id] = mob
        
        shops = p / "shops"
        if shops.exists():
            for file in shops.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    shop = Shop.load(data)
                    dbat.SHOPS[shop.id] = shop

        guilds = p / "guilds"
        if guilds.exists():
            for file in guilds.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    guild = Guild.load(data)
                    dbat.GUILDS[guild.id] = guild
        
        dgscripts = p / "dgscripts"