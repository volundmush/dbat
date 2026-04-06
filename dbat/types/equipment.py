from weakref import ref
import typing
import uuid
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr

import dbat

if typing.TYPE_CHECKING:
    from .objects import Object

class HasEquipment(BaseModel):
    model_config = ConfigDict(validate_assignment=True)
    __equipment: dict[str, uuid.UUID] = PrivateAttr(default_factory=dict)

    
    def get_equipment(self, slot: str) -> Object | None:
        if not (obj_id := self.__equipment.get(slot, None)):
            return None
        return dbat.INDEX.get_object(obj_id, None)
    
    def get_all_equipment(self) -> dict[str, Object]:
        out = dict()
        for k, v in self.__equipment.copy().items():
            if not v:
                continue
            if (obj := dbat.INDEX.get_object(v, None)) is not None:
                out[k] = obj
        return out

    def iter_equipment(self):
        for slot, obj in self.__equipment.copy().items():
            if not (e := dbat.INDEX.get_object(obj, None)):
                continue
            yield slot, e
    
    def add_equipment(self, slot: str, obj: Object):
        if slot in self.__equipment:
            raise ValueError(f"Slot {slot} is already occupied")
        self.__equipment[slot] = obj.id
        obj.equipped_by = self
        obj.equipped_at = slot
        self.on_add_equipment(slot, obj)
        obj.on_equipped(self, slot)
    
    def on_add_equipment(self, slot: str, obj: Object):
        pass
    
    def remove_equipment(self, slot: str) -> Object | None:
        if (obj_id := self.__equipment.pop(slot, None)) is not None:
            if (obj := dbat.INDEX.get_object(obj_id, None)) is not None:
                obj.equipped_by = None
                obj.equipped_at = ""
                self.on_remove_equipment(slot, obj)
                obj.on_unequipped(self, slot)
                return obj
        return None
    
    def on_remove_equipment(self, slot: str, obj: Object):
        pass