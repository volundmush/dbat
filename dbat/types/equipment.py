from weakref import ref
from typing import TYPE_CHECKING
from .location import Location, POINT

if TYPE_CHECKING:
    from .objects import Object

class HasEquipment:
    
    def __init__(self):
        self.__equipment: dict[str, Object] = {}
    
    def get_equipment(self, slot: str) -> Object | None:
        return self.__equipment.get(slot, None)
    
    def get_all_equipment(self) -> dict[str, Object]:
        return {k: v for k, v in self.__equipment if v}

    def iter_equipment(self):
        for slot, obj in self.__equipment.copy().items():
            if not obj:
                continue
            yield slot, obj
    
    def add_equipment(self, slot: str, obj: Object):
        if slot in self.__equipment:
            raise ValueError(f"Slot {slot} is already occupied")
        self.__equipment[slot] = obj
        obj.equipped_by = self
        obj.equipped_at = slot
        self.on_add_equipment(slot, obj)
        obj.on_equipped(self, slot)
    
    def on_add_equipment(self, slot: str, obj: Object):
        pass
    
    def remove_equipment(self, slot: str) -> Object | None:
        if (obj := self.__equipment.pop(slot, None)) is not None:
            obj.equipped_by = None
            obj.equipped_at = ""
            self.on_remove_equipment(slot, obj)
            obj.on_unequipped(self, slot)
            return obj
        return None
    
    def on_remove_equipment(self, slot: str, obj: Object):
        pass