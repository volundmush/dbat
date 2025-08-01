import typing
import pydantic
from enum import IntFlag, Enum
from pydantic import BaseModel, Field, ConfigDict, field_serializer, field_validator
from . import names
from . import defs

# Mob trigger types - IntFlag for bitwise operations
class MobTriggerType(IntFlag):
    """Mob trigger types using bitshift values from C++"""
    GLOBAL = 1 << 0      # check even if zone empty
    RANDOM = 1 << 1      # checked randomly
    COMMAND = 1 << 2     # character types a command
    SPEECH = 1 << 3      # a char says a word/phrase
    ACT = 1 << 4         # word or phrase sent to act
    DEATH = 1 << 5       # character dies
    GREET = 1 << 6       # something enters room seen
    GREET_ALL = 1 << 7   # anything enters room
    ENTRY = 1 << 8       # the mob enters a room
    RECEIVE = 1 << 9     # character is given obj
    FIGHT = 1 << 10      # each pulse while fighting
    HITPRCNT = 1 << 11   # fighting and below some hp
    BRIBE = 1 << 12      # coins are given to mob
    LOAD = 1 << 13       # the mob is loaded
    MEMORY = 1 << 14     # mob sees someone remembered
    CAST = 1 << 15       # mob targeted by spell
    LEAVE = 1 << 16      # someone leaves room seen
    DOOR = 1 << 17       # door manipulated in room
    TIME = 1 << 19       # trigger based on specific game hour
    HOURLY = 1 << 20     # triggered every game hour
    QUARTER = 1 << 21    # triggered every 15 game minutes

# Object trigger types
class ObjectTriggerType(IntFlag):
    """Object trigger types using bitshift values from C++"""
    GLOBAL = 1 << 0      # unused
    RANDOM = 1 << 1      # checked randomly
    COMMAND = 1 << 2     # character types a command
    TIMER = 1 << 5       # item's timer expires
    GET = 1 << 6         # item is picked up
    DROP = 1 << 7        # character tries to drop obj
    GIVE = 1 << 8        # character tries to give obj
    WEAR = 1 << 9        # character tries to wear obj
    REMOVE = 1 << 11     # character tries to remove obj
    LOAD = 1 << 13       # the object is loaded
    CAST = 1 << 15       # object targeted by spell
    LEAVE = 1 << 16      # someone leaves room seen
    CONSUME = 1 << 18    # char tries to eat/drink obj
    TIME = 1 << 19       # trigger based on specific game hour
    HOURLY = 1 << 20     # triggered every game hour
    QUARTER = 1 << 21    # triggered every 15 game minutes

# Room/World trigger types
class RoomTriggerType(IntFlag):
    """Room trigger types using bitshift values from C++"""
    GLOBAL = 1 << 0      # check even if zone empty
    RANDOM = 1 << 1      # checked randomly
    COMMAND = 1 << 2     # character types a command
    SPEECH = 1 << 3      # a char says word/phrase
    RESET = 1 << 5       # zone has been reset
    ENTER = 1 << 6       # character enters room
    DROP = 1 << 7        # something dropped in room
    CAST = 1 << 15       # spell cast in room
    LEAVE = 1 << 16      # character leaves the room
    DOOR = 1 << 17       # door manipulated in room
    TIME = 1 << 19       # trigger based on specific game hour
    HOURLY = 1 << 20     # triggered every game hour
    QUARTER = 1 << 21    # triggered every 15 game minutes

# Object command trigger types (for OTRIG_COMMAND)
class ObjectCommandType(IntFlag):
    """Object command trigger subtypes"""
    EQUIP = 1 << 0       # obj must be in char's equip
    INVEN = 1 << 1       # obj must be in char's inven
    ROOM = 1 << 2        # obj must be in char's room

# Object consume trigger commands (for OTRIG_CONSUME)
class ObjectConsumeType(Enum):
    """Object consume trigger subtypes"""
    EAT = 1
    DRINK = 2
    QUAFF = 3


class _DgScriptBaseModel(BaseModel):
    vn: int = -1
    name: str = ""
    narg: int = 0
    arglist: str = ""
    cmdlist: typing.List[str] = Field(default_factory=list)

    def check_values(self) -> bool:
        """Check if the script has valid values."""
        if not self.name:
            raise ValueError("Script name cannot be empty")
        if self.vn < 0:
            raise ValueError("VNum must be a non-negative integer")
        return True


class DgMobScript(_DgScriptBaseModel):
    trigger_type: MobTriggerType = MobTriggerType(0)

    @field_serializer('attach_type', return_type=int)
    def always_attach_type(self, _):
        return defs.UnitType.character.value

    def available_trigger_types(self) -> list[str]:
        """Return a list of available trigger types for this script."""
        return [flag.name for flag in MobTriggerType]

    class Config:
        # Needed for `model.dict()` to include computed fields
        serialize_default_extras = True


class DgObjectScript(_DgScriptBaseModel):
    trigger_type: ObjectTriggerType = ObjectTriggerType(0)

    @field_serializer('attach_type', return_type=int)
    def always_attach_type(self, _):
        return defs.UnitType.object.value

    def available_trigger_types(self) -> list[str]:
        """Return a list of available trigger types for this script."""
        return [flag.name for flag in ObjectTriggerType]

    class Config:
        # Needed for `model.dict()` to include computed fields
        serialize_default_extras = True

class DgRoomScript(_DgScriptBaseModel):
    trigger_type: RoomTriggerType = RoomTriggerType(0)

    @field_serializer('attach_type', return_type=int)
    def always_attach_type(self, _):
        return defs.UnitType.room.value

    def available_trigger_types(self) -> list[str]:
        """Return a list of available trigger types for this script."""
        return [flag.name for flag in RoomTriggerType]

    class Config:
        # Needed for `model.dict()` to include computed fields
        serialize_default_extras = True

class TrigProtoData(_DgScriptBaseModel):
    attach_type: defs.UnitType = defs.UnitType.unknown
    trigger_type: int = 0

    def to_specific_script(self) -> typing.Union[DgMobScript, DgObjectScript, DgRoomScript]:
        """Convert to specific script type based on attach_type."""
        match self.attach_type:
            case defs.UnitType.character:
                return DgMobScript(**self.model_dump())
            case defs.UnitType.object:
                return DgObjectScript(**self.model_dump())
            case defs.UnitType.room:
                return DgRoomScript(**self.model_dump())
            case _:
                raise ValueError(f"Unknown attach type: {self.attach_type}")
