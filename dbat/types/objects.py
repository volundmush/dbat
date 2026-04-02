from typing import TYPE_CHECKING
from .inventory import HasInventory
from .location import IsLocation, HasLocation
from .dgscripts import HasDgScripts
from .misc import HasFlags, HasInteractive

if TYPE_CHECKING:
    from .equipment import HasEquipment


class WearableComponent:
    """
    This object can be worn. It is equipment.
    """

class WeaponComponent:
    """
    This object can be used as a weapon. It is equipment.
    If the item doesn't also have WearableComponent, this is not very useful.
    """

class ConsumableComponent:
    """
    This object can be consumed. It is food or a potion or something.
    """

class ToolComponent:
    """
    This object can be used as a tool for various purposes like gathering, crafting, etc.
    Pens, Torches, Pickaxes, Fishing poles, etc.
    """

class BreakableComponent:
    """
    This object can be broken. It has a certain amount of hit points, and can be damaged by weapons.
    When it reaches 0 hit points, it is broken and becomes unusable until repaired.
    """

class ContainerComponent:
    """
    This object can contain other objects. It is a container.
    It has a certain capacity, and can be opened and closed.
    It might also only contain certain types of objects. Like a quiver for arrows.
    """

class FuelConsumerComponent:
    """
    This object can consume fuel. It is a fuel consumer. this might be used for lanterns or tools
    that require fuel to operate.
    """

class FuelProviderComponent:
    """
    This object provides fuel.
    """

class ValueComponent:
    """
    This object is worth something to someone. 
    It has intrinsic value and can be sold to shops.
    """

    def __init__(self):
        self.value = 0

class ObjectPrototype:
    pass



class Object(IsLocation, HasLocation, HasInventory, HasDgScripts, HasInteractive, HasFlags):

    def __init__(self):
        IsLocation.__init__(self)
        HasLocation.__init__(self)
        HasInventory.__init__(self)
        HasDgScripts.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        # if deleted, this will be cleaned up after the current tick.
        self.deleted = False

        self.equipped_by: HasEquipment | None = None
        self.stored_by: HasInventory | None = None
        self.wearable: WearableComponent | None = None
        self.weapon: WeaponComponent | None = None
        self.consumable: ConsumableComponent | None = None
        self.tool: ToolComponent | None = None
        self.breakable: BreakableComponent | None = None
        self.container: ContainerComponent | None = None
        self.fuel_consumer: FuelConsumerComponent | None = None
        self.fuel_provider: FuelProviderComponent | None = None
        self.value: ValueComponent | None = None

        # Stores arbitrary data for use by components.
        self.data: dict = dict()
    
    def __bool__(self):
        return not self.deleted
    
    async def on_equipped(self, equipper: HasEquipment, slot: int):
        pass

    async def on_unequipped(self, equipper: HasEquipment, slot: int):
        pass