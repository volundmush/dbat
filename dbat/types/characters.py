import uuid
import typing
from .location import HasLocation
from .equipment import HasEquipment
from .inventory import HasInventory
from .dgscripts import HasDgScripts, DgReference
from .misc import HasInteractive, HasFlags, HasColorName, HasColorDescription
import dbat
import copy
from loguru import logger
from rich.text import Text
from rich.errors import MarkupError

if typing.TYPE_CHECKING:
    from dbat.sessions import Session
    from .objects import Object
    from .location import Location


class MobilePrototype(HasColorName, HasColorDescription, HasFlags, HasInteractive):
    
    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasFlags.__init__(self)
        HasInteractive.__init__(self)
        self.id = ""

        self.proto_script: list[str] = list()
        self.instances: set[Mobile] = set()
    
    def save(self):
        dbat.DIRTY_MOBILE_PROTOTYPES.add(self.id)
    
    def dump(self) -> dict:
        return {
            "id": self.id,
            "color_name": self.color_name.markup,
            "color_description": self.color_description.markup,
            "flags": list(self.flags),
            "keywords": list(self.keywords),
            "proto_script": self.proto_script,
        }
    
    @classmethod
    def load(cls, data: dict) -> MobilePrototype:
        mob = cls()
        mob.id = data["id"]
        mob.color_name = Text.from_markup(data["color_name"])
        mob.color_description = Text.from_markup(data["color_description"])
        mob.flags = set(data["flags"])
        mob.keywords = set(data["keywords"])
        mob.proto_script = data.get("proto_script", list())
        return mob
    
    def spawn(self) -> Mobile:
        mob = Mobile(self)
        mob.id = uuid.uuid4()
        mob.color_name = self.color_name.copy()
        mob.color_description = self.color_description.copy()
        mob.flags = self.flags.copy()
        mob.proto = self
        mob.game_activate()
        return mob


class Character(HasColorName, HasColorDescription, HasLocation, HasEquipment, HasInventory, HasDgScripts, HasInteractive, HasFlags):
    """
    Base class for characters. Shoul not be used directly.
    """
    location_type: str = "character"
    slug_type: str = "character"

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasLocation.__init__(self)
        HasEquipment.__init__(self)
        HasInventory.__init__(self)
        HasDgScripts.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        
        self.id: uuid = uuid.NIL
        self.deleted = False
        # The session of an attached user, if any.
        self.session: Session | None = None
        self.command_queue: list[str] = list()

    def __bool__(self):
        return not self.deleted
    
    def save(self):
        pass

    def send_rich(self, text: str | Text):
        if not self.session:
            return
        try:
            rich_text = Text.from_markup(text) if isinstance(text, str) else text
        except MarkupError:
            logger.warning(f"Failed to parse rich text: {text}")
            rich_text = Text(str(text))
        self.session.append_rich(rich_text)

    def send_text(self, text: str):
        if self.session:
            self.session.append_text(text)
    
    def send_line(self, text: str):
        if self.session:
            self.session.append_line(text)
    
    def send_gmcp(self, package: str, data: dict):
        if self.session:
            self.session.append_gmcp(package, data)
    
    def send_event(self, event):
        if self.session:
            self.session.append_event(event)

    def is_npc(self) -> bool:
        return True
    
    def is_pc(self) -> bool:
        return not self.is_npc()
    
    def valid_location_coordinates(self, point):
        return True

    def enqueue_command(self, command: str):
        self.command_queue.append(command)
        dbat.SUBSCRIPTIONS["pending_command"].add(self)

    def clear_command_queue(self):
        self.command_queue.clear()
        dbat.SUBSCRIPTIONS["pending_command"].discard(self)
    
    def as_dg_ref(self) -> DgReference:
        return DgReference("object", self.id)
    
    def get_apparent_race(self, viewer: Character) -> str:
        return "unknown"
    
    def get_apparent_sex(self, viewer: Character) -> str:
        return "unknown"
    
    def get_keywords(self, viewer: Character) -> set[str]:
        out = super().get_keywords(viewer)

        app_race = self.get_apparent_race(viewer)
        app_sex = self.get_apparent_sex(viewer)

        # we'll need to add some checks for if race and sex should be used as keywords but for now...

        if app_race:
            out.add(app_race)
        if app_sex:
            out.add(app_sex)

        return out

class Mobile(Character):
    """
    Class for Mobiles/NPCs.
    """

    def __init__(self, proto: MobilePrototype):
        Character.__init__(self)
        self.proto = proto
        # Set by reset commands, should be cleared if it is ever picked up or relocated.
        self.spawn_location: Location | None = None
    
    def __repr__(self):
        return f"<Mobile: {self.color_name.plain} ({self.id})>"
    
    def game_activate(self):
        dbat.MOBILES[self.id] = self
        dbat.CHARACTERS[self.id] = self
        if self.proto:
            self.proto.instances.add(self)
    
    def game_deactivate(self):
        dbat.MOBILES.pop(self.id, None)
        dbat.CHARACTERS.pop(self.id, None)
        if self.proto:
            self.proto.instances.discard(self)

    def get_display_name(self, viewer: Character, capitalize: bool = False) -> Text:
        out = self.color_name.copy()
        if capitalize:
            out.plain = out.plain.capitalize()
        
        if not viewer.can_see(self):
            return Text("Someone" if capitalize else "someone")

        return out


class PlayerCharacter(Character):
    """
    Class for Player Characters.
    """

    def __init__(self):
        Character.__init__(self)
        self.dub_names: dict[uuid.UUID, str] = dict()

    def __repr__(self):
        return f"<PlayerCharacter: {self.color_name.plain} ({self.id})>"
    
    def game_activate(self):
        dbat.PLAYERS[self.id] = self
        dbat.CHARACTERS[self.id] = self
    
    def game_deactivate(self):
        dbat.PLAYERS.pop(self.id, None)
        dbat.CHARACTERS.pop(self.id, None)

    def is_npc(self) -> bool:
        return False
    
    def is_admin(self) -> bool:
        return False
    
    def save(self):
        """
        This will save the player character to the database.
        Or rather, it will enqueue them to be saved after this tick.
        """
        dbat.DIRTY_PLAYERS.add(self.id)
    
    def get_keywords(self, viewer: Character) -> set[str]:
        out = super().get_keywords(viewer)

        if dub := viewer.dub_names.get(self.id, None):
            out.update(dub.lower().split())
            for x in ("a", "the", "an"):
                out.discard(x)
        
        return out
    
    def get_display_name(self, viewer: Character, capitalize: bool = False) -> Text:
        """
        Get the display name for this character. This is used for displaying the character to the viewer.
        It may be different from the name for other viewers if the character is hidden or invisible.
        For PCs, we want to show their name even if they are hidden, so we override this method.
        """
        out = self.color_name.copy()
        if capitalize:
            out.plain = out.plain.capitalize()
        
        if not viewer.can_see(self):
            return Text("Someone" if capitalize else "someone")

        if viewer.is_npc() or viewer.is_admin() or self.is_admin():
            return out

        if dub := viewer.dub_names.get(self.id, None):
            return Text(dub)

        # Okay it's a normal player viewing another normal player.
        # This is just placeholder logic for now. Will do a richer breakdown later.
        app_race = self.get_apparent_race(viewer)
        app_sex = self.get_apparent_sex(viewer)

        prefix = "A" if capitalize else "a"
        
        return Text(f"{prefix} {app_sex} {app_race}")