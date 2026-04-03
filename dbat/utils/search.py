import typing
import dbat
import re
from enum import IntEnum

if typing.TYPE_CHECKING:
    from dbat.types.characters import Character
    from dbat.types.objects import Object
    from dbat.types.location import Location
    from dbat.types.structures import Structure
    from dbat.types.inventory import HasInventory
    from dbat.types.equipment import HasEquipment
    from dbat.types.misc import HasInteractive


class SearchType(IntEnum):
    INVENTORY = 0
    EQUIPMENT = 1
    LOCATION_CHARACTERS = 2
    LOCATION_OBJECTS = 3
    LOCATION_STRUCTURES = 4
    WORLD_OBJECTS = 5
    WORLD_CHARACTERS = 6
    WORLD_STRUCTURES = 7

SEARCHFILTER = typing.Callable[[Character, HasInteractive], bool]

RE_SEARCH_ONE = re.compile(r"^((?P<index>\d+)\.)?(?P<name>(\w|\?|\*)+)$", re.IGNORECASE)
RE_SEARCH_MANY = re.compile(r"^((?P<index>(\d+)|all)\.)?(?P<name>(\w|\?|\*)+)$", re.IGNORECASE)

def VisibleFilter(viewer: Character, target: HasInteractive) -> bool:
    return viewer.can_see(target)

def NotSelfFilter(viewer: Character, target: HasInteractive) -> bool:
    return viewer != target

class Searcher:
    """
    This is a helper class for searching the database.
    """

    def __init__(self, viewer: Character, visible_only: bool = True, exclude_self: bool = True):
        self.viewer = viewer
        self.targets: list[tuple[SearchType, typing.Any | None]] = list()
        self.filters: list[SEARCHFILTER] = list()
        if visible_only:
            self.filters.append(VisibleFilter)
        if exclude_self:
            self.filters.append(NotSelfFilter)
    
    def add_inventory(self, target: HasInventory) -> Searcher:
        self.targets.append((SearchType.INVENTORY, target))
        return self
    
    def add_equipment(self, target: HasEquipment) -> Searcher:
        self.targets.append((SearchType.EQUIPMENT, target))
        return self
    
    def add_location_characters(self, target: Location) -> Searcher:
        self.targets.append((SearchType.LOCATION_CHARACTERS, target))
        return self
    
    def add_location_objects(self, target: Location) -> Searcher:
        self.targets.append((SearchType.LOCATION_OBJECTS, target))
        return self
    
    def add_location_structures(self, target: Location) -> Searcher:
        self.targets.append((SearchType.LOCATION_STRUCTURES, target))
        return self
    
    def add_world_objects(self) -> Searcher:
        self.targets.append((SearchType.WORLD_OBJECTS, None))
        return self
    
    def add_world_characters(self) -> Searcher:
        self.targets.append((SearchType.WORLD_CHARACTERS, None))
        return self
    
    def add_world_structures(self) -> Searcher:
        self.targets.append((SearchType.WORLD_STRUCTURES, None))
        return self

    def add_filter(self, filter: SEARCHFILTER) -> Searcher:
        self.filters.append(filter)
        return self
    
    def _stream_items(self):
        for target_type, target in self.targets:
            match target_type:
                case SearchType.INVENTORY:
                    for item in target.iter_inventory():
                        yield item
                case SearchType.EQUIPMENT:
                    for slot, item in target.iter_equipment():
                        yield item
                case SearchType.LOCATION_CHARACTERS:
                    for item in target.entity.get_contents_at_location(target.point):
                        yield item
                case SearchType.LOCATION_OBJECTS:
                    for item in target.entity.get_contents_at_location(target.point):
                        yield item
                case SearchType.LOCATION_STRUCTURES:
                    for item in target.entity.get_contents_at_location(target.point):
                        yield item
                case SearchType.WORLD_CHARACTERS:
                    for character in dbat.CHARACTERS.values():
                        yield character
                case SearchType.WORLD_OBJECTS:
                    for object in dbat.OBJECTS.values():
                        yield object
                case SearchType.WORLD_STRUCTURES:
                    for structure in dbat.STRUCTURES.values():
                        yield structure
    
    def _apply_filters(self, item: HasInteractive) -> bool:
        for filter in self.filters:
            if not filter(self.viewer, item):
                return False
        return True
    
    def _search_stream(self):
        for item in self._stream_items():
            if self._apply_filters(item):
                yield item
    
    def is_match(self, item: HasInteractive, query: str) -> bool:
        """
        Query uses glob matching, starting from the beginning of a keyword. 
        So "swo" would match an item with the keyword "sword", but not "longsword". 
        "swo*" would match sword, and also swordsmith.
        "swo??" would match sword, but also swole.
        """
        keywords = item.get_keywords(self.viewer)
        for keyword in keywords:
            if re.match(f"^{query.replace('*', '.*?').replace('?', '.')}$", keyword, re.IGNORECASE):
                return True
        return False
    
    def search_one(self, query: str) -> Object | Character | Structure | None:
        """
        Given a query such as "sword" or "2.sword", search through the targets and return the first matching object, or None if no match is found.
        """
        match = RE_SEARCH_ONE.match(query)
        if not match:
            raise ValueError(f"Invalid search query: {query}")
        
        index = max(1, int(match.group("index") or 1))
        name = match.group("name").lower()

        for item in self._search_stream():
            if self.is_match(item, name):
                index -= 1
                if index == 0:
                    return item
        
        return None
    
    def search_many(self, query: str) -> list[Object | Character | Structure]:
        """
        Given a query such as "sword" or "2.sword" or "all.sword", search through the targets and return a list of matching objects.
        If the query starts with "all.", return all matches. Otherwise, return only the first match.

        So for example, all.* would get a LOT of stuff.
        """
        match = RE_SEARCH_MANY.match(query)
        if not match:
            raise ValueError(f"Invalid search query: {query}")
        
        index_str = match.group("index") or "1"
        name = match.group("name").lower()

        if index_str == "all":
            return [item for item in self._search_stream() if self.is_match(item, name)]
        
        # can fearlessly convert this due to regex.
        index = max(1, int(index_str))
        for item in self._search_stream():
            if self.is_match(item, name):
                index -= 1
                if index == 0:
                    return [item]
        
        return []