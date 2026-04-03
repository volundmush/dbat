from typing import TYPE_CHECKING
from .inventory import HasInventory
from .location import HasLocation
from .dgscripts import HasDgScripts, DgReference
from .misc import HasExtraDescriptions, HasFlags, HasInteractive, HasColorName, HasColorDescription
import dbat
import uuid
import copy
from rich.text import Text

if TYPE_CHECKING:
    from .equipment import HasEquipment
    from .location import Location


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

class ObjectPrototype(HasColorName, HasColorDescription, HasInteractive, HasFlags, HasExtraDescriptions):

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        HasExtraDescriptions.__init__(self)
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
        self.instances: set[Object] = set()

    def spawn(self) -> Object:
        obj = Object()
        obj.id = uuid.uuid4()
        obj.proto = self
        obj.color_name = self.color_name.copy()
        obj.color_description = self.color_description.copy()
        if self.extra_descriptions:
            obj.extra_descriptions = copy.deepcopy(self.extra_descriptions)
        obj.flags = set(self.flags)
        obj.keywords = set(self.keywords)
        obj.proto_script = list(self.proto_script)

        # Components are deepcopied to ensure that each spawned object has its own instance of the component, allowing for individual state management.
        # Since deepcopy on none returns none, we don't gotta care about the details here.

        obj.wearable = copy.deepcopy(self.wearable)
        obj.weapon = copy.deepcopy(self.weapon)
        obj.consumable = copy.deepcopy(self.consumable)
        obj.tool = copy.deepcopy(self.tool)
        obj.breakable = copy.deepcopy(self.breakable)
        obj.container = copy.deepcopy(self.container)
        obj.fuel_consumer = copy.deepcopy(self.fuel_consumer)
        obj.fuel_provider = copy.deepcopy(self.fuel_provider)
        obj.value = copy.deepcopy(self.value)
        obj.game_activate()
        return obj

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



class Object(HasColorName, HasColorDescription, HasLocation, HasInventory, HasDgScripts, HasInteractive, HasFlags, HasExtraDescriptions):

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasLocation.__init__(self)
        HasInventory.__init__(self)
        HasDgScripts.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        HasExtraDescriptions.__init__(self)
        self.id: uuid.UUID = uuid.NIL
        # if deleted, this will be cleaned up after the current tick.
        self.deleted = False
        self.proto: ObjectPrototype | None = None

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
        # Set by reset commands, should be cleared if it is ever picked up or relocated.
        self.spawn_location: Location | None = None

        # Stores arbitrary data for use by components.
        self.data: dict = dict()
    
    def __repr__(self):
        return f"<Object: {self.color_name.plain} ({self.id})>"
    
    def __bool__(self):
        return not self.deleted
    
    def game_activate(self):
        dbat.OBJECTS[self.id] = self
        if self.proto:
            self.proto.instances.add(self)
    
    def game_deactivate(self):
        dbat.OBJECTS.pop(self.id, None)
        if self.proto:
            self.proto.instances.discard(self)
    
    def on_equipped(self, equipper: HasEquipment, slot: int):
        pass

    def on_unequipped(self, equipper: HasEquipment, slot: int):
        pass

    def valid_location_coordinates(self, point):
        return True
    
    def as_dg_ref(self) -> DgReference:
        return DgReference("object", self.id)