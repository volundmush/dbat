import uuid
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from muplugins.core.db.fields import RichText

from ..types.misc import HasFlags, HasColorName, HasColorDescription, HasExtraDescriptions

class InteractiveLine(HasColorName, HasColorDescription, HasFlags):
    id: uuid.UUID = Field(..., description="The unique ID of this interactive line.")
    keywords: list[str] = Field(default_factory=list, description="The keywords that can be used to refer to this interactive line.")
    mods: list[RichText] = Field(default_factory=list, description="A list of special sections shown after the name. Like [GLOWING] or [HUMMING].")

class InventoryLine(InteractiveLine, HasExtraDescriptions):
    pass
    
class LocationCharacterLine(InteractiveLine):
    pass

class LocationObjectLine(InteractiveLine, HasExtraDescriptions):
    pass

class LocationStructureLine(InteractiveLine, HasExtraDescriptions):
    pass