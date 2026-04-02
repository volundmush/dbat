import uuid
import dbat
import typing

if typing.TYPE_CHECKING:
    from .grids import POINT

class DgScript:
    
    def __init__(self):
        self.id: str = ""

class DgReference:
    """
    Variables in DgScripts can be either a string or a reference to another game object.
    This handles the latter case.
    """

    def __init__(self, entity_type: str, entity_id: uuid.UUID, point: POINT = (0,0,0)):
        self.entity_type = entity_type
        self.entity_id = entity_id
        self.point = point
    
    def __bool__(self):
        return bool(self.get_target())
    
    def get_target(self) -> HasDgScripts | None:
        match self.entity_type:
            case "zone":
                if z := dbat.ZONES.get(self.entity_id, None):
                    return z.tiles.get(self.point, None)
            case "structure":
                if s := dbat.STRUCTURES.get(self.entity_id, None):
                    return s.tiles.get(self.point, None)
            case "character":
                return dbat.CHARACTERS.get(self.entity_id, None)
            case "object":
                return dbat.OBJECTS.get(self.entity_id, None)
            case _:
                return None
    
    def dump(self) -> dict:
        return {"entity_type": self.entity_type,
                "entity_id": self.entity_id,
                "point": list(self.point)
                }
    
    @classmethod
    def load(cls, data: dict) -> DgReference:
        return DgReference(**data)

class HasDgScripts:

    def __init__(self):
        self.proto_script: list[str] = list()
        self.dgscripts: dict[str, "DgScriptInstance"] = dict()
        self.script_order: list[str] | None = None
        self.dg_variables: dict[str, str | DgReference] = dict()
    
    def evaluate_dgscript_member(self, script: "DgScriptInstance", field: str, arg: str) -> None | str:
        return None
    
    def as_dg_ref(self) -> DgReference:
        raise NotImplementedError("Every HasDgScripts needs to implement as_dg_ref()!")


class DgScriptInstance:
    
    def __init__(self, script: DgScript, owner: HasDgScripts):
        self.script = script
        self.owner = owner
        self.variables: dict[str, str | DgReference] = dict()
    
    def __bool__(self):
        return bool(self.owner)