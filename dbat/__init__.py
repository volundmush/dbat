from muforge.plugin import BasePlugin
from collections import defaultdict
import uuid
import typing

if typing.TYPE_CHECKING:
    from .types.objects import Object, ObjectPrototype
    from .types.zones import Zone
    from .types.characters import PlayerCharacter, Character, Mobile
    from .types.dgscripts import DgScript

# Slugs are used for legacy indexing and special tags. for instance: room:5
SLUGS: dict[str, dict[str, typing.Any]] = defaultdict(dict)

ZONES: dict[str, Zone] = dict()
STRUCTURES: dict[str, typing.Any] = dict()
CHARACTERS: dict[str, Character] = dict()
MOBILES: dict[str, Mobile] = dict()
OBJECTS: dict[str, Object] = dict()

PLAYERS: dict[uuid.UUID, PlayerCharacter] = dict()

OBJECT_PROTOTYPES: dict[str, ObjectPrototype] = dict()
MOBILE_PROTOTYPES: dict[str, ObjectPrototype] = dict()
DGSCRIPT_PROTOTYPES: dict[str, DgScript] = dict()

SHOPS: dict[str, typing.Any] = dict()
GUILDS: dict[str, typing.Any] = dict()

DIRTY_PLAYERS: set[PlayerCharacter] = set()
DIRTY_OBJECT_PROTOTYPES: set[str] = set()
DIRTY_MOBILE_PROTOTYPES: set[str] = set()
DIRTY_DGSCRIPT_PROTOTYPES: set[str] = set()
DIRTY_SHOPS: set[str] = set()
DIRTY_GUILDS: set[str] = set()
DIRTY_ZONES: set[str] = set()
DIRTY_STRUCTURES: set[str] = set()

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