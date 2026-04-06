import typing
import uuid
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr

import dbat

if typing.TYPE_CHECKING:
    from .objects import Object

class HasInventory(BaseModel):
    model_config = ConfigDict(validate_assignment=True)
    __inventory: list[uuid.UUID] = PrivateAttr(default_factory=list)

    def iter_inventory(self):
        for i in self.__inventory.copy():
            if (e := dbat.INDEX.get_object(i)):
                yield e
    
    def add_to_inventory(self, obj: Object):
        obj.can_relocate()
        obj.spawn_location = None
        self.__inventory.append(obj.id)
        self.on_add_to_inventory(obj)
    
    def on_add_to_inventory(self, obj: Object):
        pass

    def remove_from_inventory(self, obj: Object):
        self.__inventory.remove(obj.id)
        self.on_remove_from_inventory(obj)
    
    def on_remove_From_inventory(self, obj: Object):
        pass

    def can_store(self, obj: Object) -> bool:
        return True