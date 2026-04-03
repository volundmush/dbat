import typing
from dataclasses import dataclass
from rich.text import Text
from rich.errors import MarkupError

if typing.TYPE_CHECKING:
    from .characters import Character

class HasColorName:
    """
    This is for entities that have a color name.
    """

    def __init__(self):
        self.color_name: Text = Text()
    
    @property
    def name(self) -> str:
        return self.color_name.plain
    
    def set_color_name(self, color_name: str | Text):
        if isinstance(color_name, Text):
            self.color_name = color_name
            return
        try:
            self.color_name = Text.from_markup(color_name)
        except MarkupError:
            self.color_name = Text(color_name)

class HasColorDescription:
    """
    This is for entities that have a color description.
    """

    def __init__(self):
        self.color_description: Text = Text()
    
    @property
    def description(self) -> str:
        return self.color_description.plain
    
    def set_color_description(self, color_description: str | Text):
        if isinstance(color_description, Text):
            self.color_description = color_description
            return
        try:
            self.color_description = Text.from_markup(color_description)
        except MarkupError:
            self.color_description = Text(color_description)

class HasInteractive:
    """
    This is for entities that can be interacted with.
    """

    def __init__(self):
        # Keywords are used for searching/targeting. They are simple string tokens like plant, saiyan, bear, etc.
        self.keywords = set()
    
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


class HasFlags:
    """
    This is for entities that have flags.
    """

    def __init__(self):
        self.flags: set[str] = set()
    
    def has_flag(self, flag: str) -> bool:
        return flag in self.flags


@dataclass(slots=True)
class ExtraDescription:
    keywords: str = ""
    description: str = ""

class HasExtraDescriptions:
    """
    This is for entities that have extra descriptions.
    """

    def __init__(self):
        self.extra_descriptions: list[ExtraDescription] = list()