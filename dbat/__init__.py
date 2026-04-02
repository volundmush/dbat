from muforge.plugin import BasePlugin
from collections import defaultdict
import uuid
import typing
import orjson
from pathlib import Path

if typing.TYPE_CHECKING:
    from .types.objects import Object, ObjectPrototype
    from .types.zones import Zone
    from .types.characters import PlayerCharacter, Character, Mobile
    from .types.dgscripts import DgScript

# Slugs are used for legacy indexing and special tags. for instance: room:5
SLUGS: dict[str, dict[str, typing.Any]] = defaultdict(dict)

SUBSCRIPTIONS: dict[str, set[typing.Any]] = defaultdict(set)

ZONES: dict[uuid.UUID, Zone] = dict()
STRUCTURES: dict[uuid.UUID, typing.Any] = dict()
CHARACTERS: dict[uuid.UUID, Character] = dict()
MOBILES: dict[uuid.UUID, Mobile] = dict()
OBJECTS: dict[uuid.UUID, Object] = dict()

PLAYERS: dict[uuid.UUID, PlayerCharacter] = dict()

OBJECT_PROTOTYPES: dict[str, ObjectPrototype] = dict()
MOBILE_PROTOTYPES: dict[str, ObjectPrototype] = dict()
DGSCRIPT_PROTOTYPES: dict[str, DgScript] = dict()
SHOPS: dict[str, typing.Any] = dict()
GUILDS: dict[str, typing.Any] = dict()

DIRTY_PLAYERS: set[uuid.UUID] = set()

DIRTY_OBJECT_PROTOTYPES: set[str] = set()
DIRTY_MOBILE_PROTOTYPES: set[str] = set()
DIRTY_DGSCRIPT_PROTOTYPES: set[str] = set()
DIRTY_SHOPS: set[str] = set()
DIRTY_GUILDS: set[str] = set()
DIRTY_ZONES: set[uuid.UUID] = set()
DIRTY_STRUCTURES: set[uuid.UUID] = set()

def dump_assets():
    """
    Dump all assets to disk.
    """
    
    # starting from current working directory...
    p = Path()

    assets = p / "assets"

    global DIRTY_ZONES, DIRTY_OBJECT_PROTOTYPES, DIRTY_MOBILE_PROTOTYPES, DIRTY_DGSCRIPT_PROTOTYPES, DIRTY_SHOPS, DIRTY_GUILDS

    if DIRTY_ZONES:
        zones = assets / "zones"
        zones.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_ZONES:
            file = zones / f"{k}.json"
            if z := ZONES.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(z.dump()))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_ZONES.clear()

    if DIRTY_OBJECT_PROTOTYPES:
        oprotos = assets / "objects"
        oprotos.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_OBJECT_PROTOTYPES:
            file = oprotos / f"{k}.json"
            if o := OBJECT_PROTOTYPES.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(o.dump()))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_OBJECT_PROTOTYPES.clear()
    
    if DIRTY_MOBILE_PROTOTYPES:
        mprotos = assets / "mobiles"
        mprotos.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_MOBILE_PROTOTYPES:
            file = mprotos / f"{k}.json"
            if m := MOBILE_PROTOTYPES.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(m.dump()))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_MOBILE_PROTOTYPES.clear()


class DBAT(BasePlugin):

    def name(self):
        return "Dragon Ball: Advent Truth"

    def slug(self):
        return "dbat"

    def version(self):
        return "0.0.1"
    
    def game_services(self):
        from .requests import RequestHandler

        return {"request_handler": RequestHandler}

    async def setup_final(self):
        request_handler = self.app.services["request_handler"]
        self.app.fastapi_instance.state.dbat_requests = request_handler

    def depends(self):
        return [("core", ">=0.0.1")]

    def game_migrations(self):
        from .migrations import version001

        return [("version001", version001)]