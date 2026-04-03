from typing import TYPE_CHECKING
from .inventory import HasInventory
from .location import HasLocation
from .dgscripts import HasDgScripts, DgReference
from .misc import HasFlags, HasInteractive, HasColorName, HasColorDescription
import dbat
from rich.text import Text

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

class ObjectPrototype(HasColorName, HasColorDescription, HasInteractive, HasFlags):

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        self.id: str = ""
        self.wearable: WearableComponent | None = None
        self.weapon: WeaponComponent | None = None
        self.consumable: ConsumableComponent | None = None
        self.tool: ToolComponent | None = None
        self.breakable: BreakableComponent | None = None
        self.container: ContainerComponent | None = None
        self.fuel_consumer: FuelConsumerComponent | None = None
        self.fuel_provider: FuelProviderComponent | None = None
        self.value: ValueComponent | None = None

        self.proto_script: list[str] = list()

    def spawn(self) -> Object:
        pass

    def dump(self) -> dict:
        return {
            "id": self.id,
            "color_name": self.color_name.markup,
            "color_description": self.color_description.markup,
            "flags": list(self.flags),
            "keywords": list(self.keywords),
            "proto_script": self.proto_script,
        }
    
    def save(self):
        dbat.DIRTY_OBJECT_PROTOTYPES.add(self.id)
    
    @classmethod
    def load(cls, data: dict) -> ObjectPrototype:
        obj = cls()
        obj.id = data["id"]
        obj.color_name = Text.from_markup(data["color_name"])
        obj.color_description = Text.from_markup(data["color_description"])
        obj.flags = set(data["flags"])
        obj.keywords = set(data["keywords"])
        obj.proto_script = data.get("proto_script", list())
        return obj



class Object(HasColorName, HasColorDescription, HasLocation, HasInventory, HasDgScripts, HasInteractive, HasFlags):

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasLocation.__init__(self)
        HasInventory.__init__(self)
        HasDgScripts.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        # if deleted, this will be cleaned up after the current tick.
        self.deleted = False

        self.equipped_by: HasEquipment | None = None
        self.equipped_at: str = ""
        self.stored_by: HasInventory | None = None

        # components
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
    
    def on_equipped(self, equipper: HasEquipment, slot: int):
        pass

    def on_unequipped(self, equipper: HasEquipment, slot: int):
        pass

    def valid_location_coordinates(self, point):
        return True
    
    def as_dg_ref(self) -> DgReference:
        return DgReference("object", self.id)