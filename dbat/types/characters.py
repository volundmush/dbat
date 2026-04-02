import uuid
import typing
from .location import HasLocation
from .equipment import HasEquipment
from .inventory import HasInventory
from .dgscripts import HasDgScripts, DgReference
from .misc import HasInteractive, HasFlags, HasColorName, HasColorDescription
import dbat
from loguru import logger
from rich.text import Text
from rich.errors import MarkupError

if typing.TYPE_CHECKING:
    from dbat.sessions import Session
    from .objects import Object

class MobilePrototype(HasColorName, HasColorDescription, HasFlags, HasInteractive):
    
    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasFlags.__init__(self)
        HasInteractive.__init__(self)
        self.id = ""

        self.proto_script: list[str] = list()
    
    def save(self):
        dbat.DIRTY_MOBILE_PROTOTYPES.add(self.id)
    
    def dump(self) -> dict:
        return {
            "id": self.id,
            "color_name": self.color_name.markup,
            "color_description": self.color_description.markup,
            "flags": list(self.flags),
            "keywords": self.keywords,
            "proto_script": self.proto_script,
        }


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

class Mobile(Character):
    """
    Class for Mobiles/NPCs.
    """

    def __init__(self, proto: MobilePrototype):
        Character.__init__(self)
        self.proto = proto


class PlayerCharacter(Character):
    """
    Class for Player Characters.
    """

    def __init__(self):
        Character.__init__(self)
    
    def is_npc(self) -> bool:
        return False
    
    def save(self):
        """
        This will save the player character to the database.
        Or rather, it will enqueue them to be saved after this tick.
        """
        dbat.DIRTY_PLAYERS.add(self.id)