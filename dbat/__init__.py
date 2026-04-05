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
    from .types.accounts import Account

# Slugs are used for legacy indexing and special tags. for instance: room:5
SLUGS: dict[str, dict[str, typing.Any]] = defaultdict(dict)

SUBSCRIPTIONS: dict[str, set[typing.Any]] = defaultdict(set)

ZONES: dict[uuid.UUID, Zone] = dict()
STRUCTURES: dict[uuid.UUID, typing.Any] = dict()
CHARACTERS: dict[uuid.UUID, Character] = dict()
MOBILES: dict[uuid.UUID, Mobile] = dict()
OBJECTS: dict[uuid.UUID, Object] = dict()


PLAYERS: dict[uuid.UUID, PlayerCharacter] = dict()
ACCOUNTS: dict[uuid.UUID, Account] = dict()

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
DIRTY_ACCOUNTS: set[uuid.UUID] = set()

CHARACTER_COMMANDS: dict[str, type["CharacterCommand"]] = dict()
CHARACTER_COMMANDS_PRIORITY: dict[int, list[type["CharacterCommand"]]] = defaultdict(list)

def dump_assets():
    """
    Dump all assets to disk.
    """
    
    # starting from current working directory...
    p = Path() / "data"

    assets = p / "assets"

    global DIRTY_ZONES, DIRTY_OBJECT_PROTOTYPES, DIRTY_MOBILE_PROTOTYPES, DIRTY_DGSCRIPT_PROTOTYPES, DIRTY_SHOPS, DIRTY_GUILDS, DIRTY_STRUCTURES

    if DIRTY_ZONES:
        zones = assets / "zones"
        zones.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_ZONES:
            file = zones / f"{k}.json"
            if z := ZONES.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(z.dump(), option=orjson.OPT_INDENT_2))
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
                    f.write(orjson.dumps(o.dump(), option=orjson.OPT_INDENT_2))
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
                    f.write(orjson.dumps(m.dump(), option=orjson.OPT_INDENT_2))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_MOBILE_PROTOTYPES.clear()
    
    if DIRTY_DGSCRIPT_PROTOTYPES:
        dgprotos = assets / "dgscripts"
        dgprotos.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_DGSCRIPT_PROTOTYPES:
            file = dgprotos / f"{k}.json"
            if m := DGSCRIPT_PROTOTYPES.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(m.dump(), option=orjson.OPT_INDENT_2))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_DGSCRIPT_PROTOTYPES.clear()
    
    if DIRTY_SHOPS:
        shops = assets / "shops"
        shops.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_SHOPS:
            file = shops / f"{k}.json"
            if s := SHOPS.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(s.dump(), option=orjson.OPT_INDENT_2))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_SHOPS.clear()
    
    if DIRTY_GUILDS:
        guilds = assets / "guilds"
        guilds.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_GUILDS:
            file = guilds / f"{k}.json"
            if g := GUILDS.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(g.dump(), option=orjson.OPT_INDENT_2))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_GUILDS.clear()

    if DIRTY_STRUCTURES:
        structures = assets / "structures"
        structures.mkdir(exist_ok=True, parents=True)

        for k in DIRTY_STRUCTURES:
            file = structures / f"{k}.json"
            if s := STRUCTURES.get(k):
                with open(file, "wb") as f:
                    f.write(orjson.dumps(s.dump(), option=orjson.OPT_INDENT_2))
            else:
                if file.exists():
                    file.unlink()
        DIRTY_STRUCTURES.clear()


class DBAT(BasePlugin):

    def __init__(self, app):
        super().__init__(app)
        self.registered_character_commands = dict()
        self.character_commands_priority = defaultdict(list)

    def name(self):
        return "Dragon Ball: Advent Truth"

    def slug(self):
        return "dbat"

    def version(self):
        return "0.0.1"
    
    def game_services(self):
        from .game import GameService

        return {"game": GameService}

    def core_events(self):
        from .events.informative import LookLocation

        all_events = [LookLocation]

        return {ev.event_type(): ev for ev in all_events}
    
    def character_commands(self) -> list[type["CharacterCommand"]]:
        out = list()
        from .character_commands.informative import Look

        out.append(Look)

        from .character_commands.movement import Move
        out.append(Move)

        return out

    async def setup_character_commands(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_commands"):
                continue
            for command in p.character_commands():
                self.registered_character_commands[command.key] = command
        
        # sort by priority
        for command in self.registered_character_commands.values():
            self.character_commands_priority[command.priority].append(command)
        for v in self.character_commands_priority.values():
            v.sort(key=lambda c: c.key)
        
        global CHARACTER_COMMANDS, CHARACTER_COMMANDS_PRIORITY
        CHARACTER_COMMANDS = self.registered_character_commands
        CHARACTER_COMMANDS_PRIORITY = self.character_commands_priority

    async def setup_final(self, app_name: str):
        match app_name:
            case "game":
                dbat = self.app.services["game"]
                self.app.fastapi_instance.state.dbat_game = dbat
                await self.setup_character_commands()

    def depends(self):
        return [("core", ">=0.0.1")]

    def game_migrations(self):
        return dict()
    
    def game_classes(self):
        # Replaces the Core session.
        from .sessions import Session

        return {"session": Session}