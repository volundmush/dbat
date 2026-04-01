from typing import TYPE_CHECKING
from .inventory import HasInventory
from .location import IsLocation, HasLocation
from .dgscripts import HasDgScripts

if TYPE_CHECKING:
    from .equipment import HasEquipment

class ObjectPrototype:
    pass

class Object(IsLocation, HasLocation, HasInventory, HasDgScripts):

    __slots__ = ("deleted", "equipped_by")

    def __init__(self):
        super().__init__()
        # if deleted, this will be cleaned up after the current tick.
        self.deleted = False

        self.equipped_by: HasEquipment | None = None
        self.stored_by: HasInventory | None = None
    
    def __bool__(self):
        return not self.deleted
    
    async def on_equipped(self, equipper: HasEquipment, slot: int):
        pass

    async def on_unequipped(self, equipper: HasEquipment, slot: int):
        pass