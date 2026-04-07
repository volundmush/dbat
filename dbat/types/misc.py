import typing
from pydantic import BaseModel, Field, ConfigDict
from muplugins.core.db.fields import RichText
from rich.text import Text
from rich.errors import MarkupError

if typing.TYPE_CHECKING:
    from .characters import Character

class HasColorName(BaseModel):
    """
    This is for entities that have a color name.
    """
    model_config = ConfigDict(validate_assignment=True)
    color_name: RichText = Field(default_factory=RichText, description="The color name of this entity")
    
    @property
    def name(self) -> str:
        return self.color_name.plain


class HasColorDescription(BaseModel):
    """
    This is for entities that have a color description.
    """
    model_config = ConfigDict(validate_assignment=True)
    color_description: RichText = Field(default_factory=RichText, description="The color description of this entity")
    
    @property
    def description(self) -> str:
        return self.color_description.plain

class HasInteractive(BaseModel):
    """
    This is for entities that can be interacted with.
    """
    keywords: set[str] = Field(default_factory=set, description="A set of keywords for this entity, used for searching/targeting")
    
    def get_keywords(self, viewer: Character) -> set[str]:
        """
        Get the keywords for this entity. This is used for searching/targeting.
        Viewer is the character trying to interact with the entity.
        """
        return self.keywords.copy()

    def has_light(self) -> bool:
        return True

    def can_see(self, target: HasInteractive) -> bool:
        if not target:
            return False
        if not self.has_light():
            return False
        gmsee = self.get_seegmhide_grade()
        if gmsee <= 0.0:
            if target.get_hide_grade() > self.get_see_grade():
                return False
            if target.get_invis_grade() > self.get_seeinvis_grade():
                return False
        if target.get_gmhide_grade() > gmsee:
            return False
        return True

    def get_hide_grade(self) -> float:
        """
        The current hide grade is used for normal stealth checks.
        """
        return 0.0
    
    def get_invis_grade(self) -> float:
        """
        The current invisibility grade is used for mystical or SCIENCE! invisibility.
        Cloaking fields, magical spells, etc.
        """
        return 0.0
    
    def get_gmhide_grade(self) -> float:
        """
        The current admin hide grade is used for admin-level stealth checks.
        """
        return 0.0
    
    def get_see_grade(self) -> float:
        """
        The current see grade is used for seeing through stealth.
        """
        return 0.0
    
    def get_seeinvis_grade(self) -> float:
        """
        The current see invis grade is used for seeing through invisibility.
        """
        return 0.0

    def get_seegmhide_grade(self) -> float:
        """
        The current see gmhide grade is used for seeing through admin hide.
        """
        return 0.0
    
    def get_display_name(self, viewer: Character, capitalize: bool = False) -> Text:
        """
        Get the display name for this entity. This is used for displaying the entity to the viewer.
        It may be different from the name for other viewers if the entity is hidden or invisible.
        """
        return self.name


class HasFlags(BaseModel):
    """
    This is for entities that have flags.
    """
    model_config = ConfigDict(validate_assignment=True)
    flags: set[str] = Field(default_factory=set, description="A set of flags for this entity")
    
    def has_flag(self, flag: str) -> bool:
        return flag in self.flags


class ExtraDescription(BaseModel):
    keywords: str = Field("", description="The keywords for this extra description, used for searching/targeting")
    description: RichText = Field(default_factory=RichText, description="The extra description text")

class HasExtraDescriptions:
    """
    This is for entities that have extra descriptions.
    """
    model_config = ConfigDict(validate_assignment=True)
    extra_descriptions: list[ExtraDescription] = Field(default_factory=list, description="A list of extra descriptions for this entity")