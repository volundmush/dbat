import uuid
import typing
from collections import defaultdict
from pathlib import Path
import orjson

if typing.TYPE_CHECKING:
    from ..types.characters import PlayerCharacter, Mobile, MobilePrototype, Character
    from ..types.zones import Zone
    from ..types.objects import Object, ObjectPrototype
    from ..types.dgscripts import DgScript
    from ..types.shops import Shop
    from ..types.guilds import Guild
    from ..types.structures import Structure, StructurePrototype
    from ..types.accounts import Account

class Indexer:
    """
    This class is responsible for indexing/storing all game entities.
    """

    def __init__(self):
        self.slugs: dict[str, dict[str, typing.Any]] = defaultdict(dict)

        self.mobiles: dict[uuid.UUID, Mobile] = {}
        self.players: dict[uuid.UUID, PlayerCharacter] = {}
        self.characters: dict[uuid.UUID, Character] = {}

        self.mobile_prototypes: dict[str, MobilePrototype] = {}
        self.object_prototypes: dict[str, ObjectPrototype] = {}
        self.dgscript_prototypes: dict[str, DgScript] = {}
        self.structure_prototypes: dict[str, StructurePrototype] = {}

        self.entities: dict[uuid.UUID, Object | Character] = dict()

        self.objects: dict[uuid.UUID, Object] = {}
        self.zones: dict[uuid.UUID, Zone] = {}
        self.structures: dict[uuid.UUID, Structure] = {}
        self.shops: dict[str, Shop] = {}
        self.guilds: dict[str, Guild] = {}

        self.accounts: dict[uuid.UUID, Account] = {}

        self.dirty_mobile_prototypes: set[str] = set()
        self.dirty_object_prototypes: set[str] = set()
        self.dirty_dgscript_prototypes: set[str] = set()
        self.dirty_shops: set[str] = set()
        self.dirty_guilds: set[str] = set()
        self.dirty_zones: set[uuid.UUID] = set()
        self.dirty_structures: set[uuid.UUID] = set()
        self.dirty_accounts: set[uuid.UUID] = set()
        self.dirty_players: set[uuid.UUID] = set()
    
    def get_slug(self, category: str, slug: str) -> typing.Any | None:
        return self.slugs.get(category, dict()).get(slug, None)
    
    def get_mobile(self, mobile_id: uuid.UUID) -> Mobile | None:
        return self.mobiles.get(mobile_id, None)
    
    def iter_mobiles(self):
        # copy to prevent modification during iteration issues.
        mobiles = self.mobiles.copy()
        for v in mobiles.values():
            yield v
    
    def get_player(self, player_id: uuid.UUID) -> PlayerCharacter | None:
        return self.players.get(player_id, None)
    
    def iter_players(self):
        # copy to prevent modification during iteration issues.
        players = self.players.copy()
        for v in players.values():
            yield v
    
    def get_character(self, character_id: uuid.UUID) -> Character | None:
        return self.characters.get(character_id, None)
    
    def iter_characters(self):
        # copy to prevent modification during iteration issues.
        characters = self.characters.copy()
        for v in characters.values():
            yield v

    def get_mobile_prototype(self, prototype_id: str) -> MobilePrototype | None:
        return self.mobile_prototypes.get(prototype_id, None)
    
    def iter_mobile_prototypes(self):
        # copy to prevent modification during iteration issues.
        prototypes = self.mobile_prototypes.copy()
        for v in prototypes.values():
            yield v

    def get_object_prototype(self, prototype_id: str) -> ObjectPrototype | None:
        return self.object_prototypes.get(prototype_id, None)
    
    def iter_object_prototypes(self):
        # copy to prevent modification during iteration issues.
        prototypes = self.object_prototypes.copy()
        for v in prototypes.values():
            yield v

    def get_dgscript_prototype(self, prototype_id: str) -> DgScript | None:
        return self.dgscript_prototypes.get(prototype_id, None)
    
    def iter_dgscript_prototypes(self):
        # copy to prevent modification during iteration issues.
        prototypes = self.dgscript_prototypes.copy()
        for v in prototypes.values():
            yield v
        
    def get_structure_prototype(self, prototype_id: str) -> StructurePrototype | None:
        return self.structure_prototypes.get(prototype_id, None)
    
    def iter_structure_prototypes(self):
        # copy to prevent modification during iteration issues.
        prototypes = self.structure_prototypes.copy()
        for v in prototypes.values():
            yield v

    def get_object(self, object_id: uuid.UUID) -> Object | None:
        return self.objects.get(object_id, None)
    
    def iter_objects(self):
        # copy to prevent modification during iteration issues.
        objects = self.objects.copy()
        for v in objects.values():
            yield v

    def get_zone(self, zone_id: uuid.UUID) -> Zone | None:
        return self.zones.get(zone_id, None)
    
    def iter_zones(self):
        # copy to prevent modification during iteration issues.
        zones = self.zones.copy()
        for v in zones.values():
            yield v

    def get_structure(self, structure_id: uuid.UUID) -> Structure | None:
        return self.structures.get(structure_id, None)
    
    def iter_structures(self):
        # copy to prevent modification during iteration issues.
        structures = self.structures.copy()
        for v in structures.values():
            yield v
    
    def get_shop(self, shop_id: str) -> Shop | None:
        return self.shops.get(shop_id, None)
    
    def iter_shops(self):
        # copy to prevent modification during iteration issues.
        shops = self.shops.copy()
        for v in shops.values():
            yield v

    def get_guild(self, guild_id: str) -> Guild | None:
        return self.guilds.get(guild_id, None)

    def iter_guilds(self):
        # copy to prevent modification during iteration issues.
        guilds = self.guilds.copy()
        for v in guilds.values():
            yield v

    def get_account(self, account_id: uuid.UUID) -> Account | None:
        return self.accounts.get(account_id, None)

    def iter_accounts(self):
        # copy to prevent modification during iteration issues.
        accounts = self.accounts.copy()
        for v in accounts.values():
            yield v
    

    # Dump
    def dump_assets(self):
        # starting from current working directory...
        p = Path() / "data"

        assets = p / "assets"

        if self.dirty_zones:
            zones = assets / "zones"
            zones.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_zones:
                file = zones / f"{k}.json"
                if z := self.zones.get(k):
                    with open(file, "wb") as f:
                        f.write(z.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_zones.clear()

        if self.dirty_object_prototypes:
            oprotos = assets / "objects"
            oprotos.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_object_prototypes:
                file = oprotos / f"{k}.json"
                if o := self.object_prototypes.get(k):
                    with open(file, "wb") as f:
                        f.write(o.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_object_prototypes.clear()
        
        if self.dirty_mobile_prototypes:
            mprotos = assets / "mobiles"
            mprotos.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_mobile_prototypes:
                file = mprotos / f"{k}.json"
                if m := self.mobile_prototypes.get(k):
                    with open(file, "wb") as f:
                        f.write(m.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_mobile_prototypes.clear()
        
        if self.dirty_dgscript_prototypes:
            dgprotos = assets / "dgscripts"
            dgprotos.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_dgscript_prototypes:
                file = dgprotos / f"{k}.json"
                if m := self.dgscript_prototypes.get(k):
                    with open(file, "wb") as f:
                        f.write(m.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_dgscript_prototypes.clear()
        
        if self.dirty_shops:
            shops = assets / "shops"
            shops.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_shops:
                file = shops / f"{k}.json"
                if s := self.shops.get(k):
                    with open(file, "wb") as f:
                        f.write(s.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_shops.clear()
        
        if self.dirty_guilds:
            guilds = assets / "guilds"
            guilds.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_guilds:
                file = guilds / f"{k}.json"
                if g := self.guilds.get(k):
                    with open(file, "wb") as f:
                        f.write(g.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_guilds.clear()

        if self.dirty_structures:
            structures = assets / "structures"
            structures.mkdir(exist_ok=True, parents=True)

            for k in self.dirty_structures:
                file = structures / f"{k}.json"
                if s := self.structures.get(k):
                    with open(file, "wb") as f:
                        f.write(s.model_dump_json(indent=4).encode())
                else:
                    if file.exists():
                        file.unlink()
            self.dirty_structures.clear()