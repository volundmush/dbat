import uuid
import typing
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from muplugins.core.db.fields import RichText

from ..types.misc import HasFlags, HasColorName, HasColorDescription, HasExtraDescriptions

if typing.TYPE_CHECKING:
    from ..types.location import Location
    from ..types.grids import Exit, Direction
    from ..types.characters import Character, Mobile, PlayerCharacter
    from ..types.objects import Object
    from ..types.structures import Structure
    from ..types.zones import Zone

class InteractiveLine(HasColorName, HasColorDescription, HasFlags):
    id: uuid.UUID = Field(..., description="The unique ID of this interactive line.")
    keywords: list[str] = Field(default_factory=list, description="The keywords that can be used to refer to this interactive line.")
    mods: list[RichText] = Field(default_factory=list, description="A list of special sections shown after the name. Like [GLOWING] or [HUMMING].")

class InventoryLine(InteractiveLine, HasExtraDescriptions):
    pass
    
class LocationCharacterLine(InteractiveLine):
    
    @classmethod
    def from_character(cls, char: Character, viewer: Character) -> "LocationCharacterLine":
        return cls(
            id=char.id,
            color_name=char.get_display_name(viewer),
            color_description=char.color_description,
            keywords=char.keywords,
            #mods=char.mods
        )

class LocationObjectLine(InteractiveLine, HasExtraDescriptions):
    
    @classmethod
    def from_object(cls, obj: Object, viewer: Character) -> "LocationObjectLine":
        return cls(
            id=obj.id,
            color_name=obj.get_display_name(viewer),
            color_description=obj.color_description,
            keywords=obj.keywords,
            #mods=obj.mods,
            extra_descriptions=obj.extra_descriptions
        )

class LocationStructureLine(InteractiveLine, HasExtraDescriptions):
    
    @classmethod
    def from_structure(cls, struct: Structure, viewer: Character) -> "LocationStructureLine":
        return cls(
            id=struct.id,
            color_name=struct.get_display_name(viewer),
            color_description=struct.color_description,
            keywords=struct.keywords,
            #mods=struct.mods,
            extra_descriptions=struct.extra_descriptions
        )