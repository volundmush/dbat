import typing
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from .inventory import HasInventory
from .location import HasLocation
from .dgscripts import HasDgScripts, DgReference
from .misc import HasExtraDescriptions, HasFlags, HasInteractive, HasColorName, HasColorDescription
import dbat
import uuid
import copy
from rich.text import Text

if typing.TYPE_CHECKING:
    from .equipment import HasEquipment
    from .location import Location


class WearableComponent(BaseModel):
    """
    This object can be worn. It is equipment.
    """

class WeaponComponent(BaseModel):
    """
    This object can be used as a weapon. It is equipment.
    If the item doesn't also have WearableComponent, this is not very useful.
    """

class ConsumableComponent(BaseModel):
    """
    This object can be consumed. It is food or a potion or something.
    """

class ToolComponent(BaseModel):
    """
    This object can be used as a tool for various purposes like gathering, crafting, etc.
    Pens, Torches, Pickaxes, Fishing poles, etc.
    """

class BreakableComponent(BaseModel):
    """
    This object can be broken. It has a certain amount of hit points, and can be damaged by weapons.
    When it reaches 0 hit points, it is broken and becomes unusable until repaired.
    """

class ContainerComponent(BaseModel):
    """
    This object can contain other objects. It is a container.
    It has a certain capacity, and can be opened and closed.
    It might also only contain certain types of objects. Like a quiver for arrows.
    """

class FuelConsumerComponent(BaseModel):
    """
    This object can consume fuel. It is a fuel consumer. this might be used for lanterns or tools
    that require fuel to operate.
    """

class FuelProviderComponent(BaseModel):
    """
    This object provides fuel.
    """

class PriceComponent(BaseModel):
    """
    This object is worth something to someone. 
    It has intrinsic value and can be sold to shops.
    """
    value: int = Field(0, description="The value of this object in coins. This is the base value and can be modified by shopkeepers, appraisers, etc.")


class ObjectBase(HasColorName, HasColorDescription, HasInteractive, HasFlags, HasDgScripts, HasExtraDescriptions):
    wearable: WearableComponent | None = Field(default=None, description="The wearable component of this object, if any.")
    weapon: WeaponComponent | None = Field(default=None, description="The weapon component of this object, if any.")
    consumable: ConsumableComponent | None = Field(default=None, description="The consumable component of this object, if any.")
    tool: ToolComponent | None = Field(default=None, description="The tool component of this object, if any.")
    breakable: BreakableComponent | None = Field(default=None, description="The breakable component of this object, if any.")
    container: ContainerComponent | None = Field(default=None, description="The container component of this object, if any.")
    fuel_consumer: FuelConsumerComponent | None = Field(default=None, description="The fuel consumer component of this object, if any.")
    fuel_provider: FuelProviderComponent | None = Field(default=None, description="The fuel provider component of this object, if any.")
    price: PriceComponent | None = Field(default=None, description="The price component of this object, if any.")


class ObjectPrototype(ObjectBase):
    id: str = Field(..., description="The unique ID of this object prototype.")
    __instances: set[uuid.UUID] = PrivateAttr(default_factory=set)

    def spawn(self) -> Object:
        data = self.model_dump()
        id = uuid.uuid4()
        data["id"] = id
        obj = Object(**data)
        obj.__proto = self
        obj.game_activate()
        return obj

    def save(self):
        dbat.INDEX.dirty_object_prototypes.add(self.id)

class Object(ObjectBase, HasLocation, HasInventory):
    id: uuid.UUID = Field(..., description="The unique ID of this object.")
    equipped_at: str = Field("", description="The slot this object is equipped at, if it is equipped. Otherwise, an empty string.", exclude=True)
    equipped_by: uuid.UUID | None = Field(default=None, description="The entity that has this object equipped, if any.", exclude=True)
    stored_by: uuid.UUID | None = Field(default=None, description="The entity that has this object stored in their inventory, if any.", exclude=True)
    spawn_location: Location | None = Field(default=None, description="The location this object was spawned at. This is set by reset commands, and should be cleared if it is ever picked up or relocated.", exclude=True)
    __deleted: bool = PrivateAttr(default=False)
    data: dict = Field(default_factory=dict, description="Arbitrary data for use by components.")
    proto: str = Field(default="", description="The prototype this object was spawned from, if any.")
    
    def __repr__(self):
        return f"<Object: {self.color_name.plain} ({self.id})>"
    
    def __bool__(self):
        return not self.__deleted
    
    def game_activate(self):
        dbat.INDEX.objects[self.id] = self
        dbat.INDEX.entities[self.id] = self
        if self.__proto:
            self.__proto.instances.add(self)
    
    def game_deactivate(self):
        dbat.INDEX.objects.pop(self.id, None)
        dbat.INDEX.entities.pop(self.id, None)
        if self.__proto:
            self.__proto.instances.discard(self)
    
    def on_equipped(self, equipper: HasEquipment, slot: int):
        pass

    def on_unequipped(self, equipper: HasEquipment, slot: int):
        pass

    def valid_location_coordinates(self, point):
        return True
    
    def as_dg_ref(self) -> DgReference:
        return DgReference("object", self.id)