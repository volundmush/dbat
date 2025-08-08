from enum import Enum

class UnitType(Enum):
    """Unit types for trigger attachment"""
    character = 0  # MOB_TRIGGER
    object = 1     # OBJ_TRIGGER  
    room = 2       # WLD_TRIGGER
    unknown = 3