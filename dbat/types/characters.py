import uuid
import typing
from .location import HasLocation, IsLocation
from .equipment import HasEquipment
from .inventory import HasInventory
from .dgscripts import HasDgScripts
from .misc import HasInteractive, HasFlags
import dbat

if typing.TYPE_CHECKING:
    from dbat.sessions import Session
    from .objects import Object

class MobilePrototype:
    pass


class Character(HasLocation, IsLocation, HasEquipment, HasInventory, HasDgScripts, HasInteractive, HasFlags):
    """
    Base class for characters. Shoul not be used directly.
    """
    location_type: str = "character"
    slug_type: str = "character"

    def __init__(self):
        HasLocation.__init__(self)
        IsLocation.__init__(self)
        HasEquipment.__init__(self)
        HasInventory.__init__(self)
        HasDgScripts.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        self.id: str = ""
        self.deleted = False
        # The session of an attached user, if any.
        self.session: Session | None = None

    def __bool__(self):
        return not self.deleted
    
    def save(self):
        pass

    def send_text(self, text: str):
        if self.session:
            self.session.append_text(text)
    
    def send_line(self, text: str):
        if self.session:
            self.session.append_line(text)
    
    def send_gmcp(self, package: str, data: dict):
        if self.session:
            self.session.append_gmcp(package, data)
    
    def can_see(self, other: "Character | Object") -> bool:
        return True
    
    def is_npc(self) -> bool:
        return True
    
    def is_pc(self) -> bool:
        return not self.is_npc()


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

    def __init__(self, pc_id: uuid.UUID):
        Character.__init__(self)
        self.pc_id = str(pc_id)
    
    def is_npc(self) -> bool:
        return False
    
    def save(self):
        """
        This will save the player character to the database.
        Or rather, it will enqueue them to be saved after this tick.
        """
        dbat.DIRTY_PLAYERS.add(self)