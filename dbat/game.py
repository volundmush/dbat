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
    
    async def load_assets(self):
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
        from .types.dgscripts import DgScript

        p = Path() / "data"/ "assets"
        
        idx = dbat.INDEX

        # Loading the game maps - Zones, and Structures
        # load zones
        zones = p / "zones"
        zone_parents: dict[uuid.UUID, uuid.UUID] = dict()
        if zones.exists():
            logger.info("Loading zones...")
            for file in zones.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    zone = Zone(**data)
                    idx.zones[zone.id] = zone
                    if zone.slug:
                        idx.slugs["zone"][zone.slug] = zone
                    if par := data.get("parent", None):
                        zone_parents[zone.id] = uuid.UUID(par)
            logger.info(f"Loaded {len(idx.zones)} zones.")

        # assign parents and children
        for zone_id, parent_id in zone_parents.items():
            if not (parent := idx.zones.get(parent_id, None)):
                logger.warning(f"Zone {zone_id} has parent {parent_id} which does not exist.")
                continue
            if not (child := idx.zones.get(zone_id, None)):
                logger.warning(f"Zone {parent.id}: {parent.name} has child {zone_id} but the child does not exist.")
                continue
            parent.add_child(child)
        

        # load structures
        structures = p / "structures"
        if structures.exists():
            logger.info("Loading structures...")
            for file in structures.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    structure = Structure(**data)
                    if structure.slug:
                        idx.slugs["structure"][structure.slug] = structure
                    idx.structures[structure.id] = structure
        
            for k, v in idx.structures.items():
                v.register_location()
            logger.info(f"Loaded {len(idx.structures)} structures.")

        objects = p / "objects"
        if objects.exists():
            logger.info("Loading object prototypes...")
            for file in objects.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    obj = ObjectPrototype(**data)
                    idx.object_prototypes[obj.id] = obj
            logger.info(f"Loaded {len(idx.object_prototypes)} object prototypes.")

        mobiles = p / "mobiles"
        if mobiles.exists():
            logger.info("Loading mobile prototypes...")
            for file in mobiles.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    mob = MobilePrototype(**data)
                    idx.mobile_prototypes[mob.id] = mob
            logger.info(f"Loaded {len(idx.mobile_prototypes)} mobile prototypes.")

        shops = p / "shops"
        if shops.exists():
            logger.info("Loading shops...")
            for file in shops.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    shop = Shop(**data)
                    idx.shops[shop.id] = shop
            logger.info(f"Loaded {len(idx.shops)} shops.")

        guilds = p / "guilds"
        if guilds.exists():
            logger.info("Loading guilds...")

            for file in guilds.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    guild = Guild(**data)
                    idx.guilds[guild.id] = guild
            logger.info(f"Loaded {len(idx.guilds)} guilds.")

        dgscripts = p / "dgscripts"
        if dgscripts.exists():
            logger.info("Loading dg scripts...")
            for file in dgscripts.glob("*.json"):
                with file.open("r") as f:
                    data = orjson.loads(f.read())
                    dgscript = DgScript(**data)
                    idx.dgscript_prototypes[dgscript.id] = dgscript
            logger.info(f"Loaded {len(idx.dgscript_prototypes)} dg scripts.")
        
    async def reset_world(self):
        logger.info("Running initial reset on zones...")
        for k, v in dbat.INDEX.zones.items():
            v.reset_grid()

        logger.info("Running initial reset on structures...")
        for k, v in dbat.INDEX.structures.items():
            v.reset_grid()

    async def setup(self):
        await self.load_assets()
    
    async def process_pending_commands(self, delta_time: float):
        pending = dbat.SUBSCRIPTIONS.get("pending_commands", set()).copy()
        for id in pending:
            if character := dbat.INDEX.get_character(id):
                character.process_command_queue(delta_time)


    async def update(self, delta_time: float):
        """
        Run an instance of the main loop and advance game state by delta_time.
        """

        # We will want to have a more elaborate game services loop in the future but for now let's just process pending commands.
        await self.process_pending_commands(delta_time)

    async def run(self):
        """
        The game simulation runs at a tick speed of 0.1 seconds.
        Every 0.1 seconds we call the update loops and pass in delta time.
        We sleep for the remainder of the tick if the update loops finish early.
        """
        await self.reset_world()
        
        tick_speed = 0.1
        last_time = asyncio.get_event_loop().time()
        while True:
            now = asyncio.get_event_loop().time()
            delta_time = now - last_time
            await self.update(delta_time)
            elapsed = asyncio.get_event_loop().time() - now
            if elapsed < tick_speed:
                await asyncio.sleep(tick_speed - elapsed)
            else:
                # even if we're running behind, we still want to yield control to the event loop so we don't block other tasks.
                await asyncio.sleep(0)
            last_time = now