import typing

if typing.TYPE_CHECKING:
    from .objects import Object

class HasInventory:

    def __init__(self):
        self.__inventory: list[Object] = list()

    def iter_inventory(self):
        for i in self.__inventory.copy():
            if i:
                yield i
    
    def add_to_inventory(self, obj: Object):
        self.__inventory.append(obj)
        self.on_add_to_inventory(obj)
    
    def on_add_to_inventory(self, obj: Object):
        pass

    def remove_from_inventory(self, obj: Object):
        self.__inventory.remove(obj)
        self.on_remove_from_inventory(obj)
    
    def on_remove_From_inventory(self, obj: Object):
        pass