import typing
from dataclasses import dataclass


if typing.TYPE_CHECKING:
    from .characters import Character


class HasInteractive:
    """
    This is for entities that can be interacted with.
    """

    def __init__(self):
        # Keywords are used for searching/targeting. They are simple string tokens like plant, saiyan, bear, etc.
        self.keywords = list()
    
    def get_keywords(self, viewer: Character) -> list[str]:
        """
        Get the keywords for this entity. This is used for searching/targeting.
        Viewer is the character trying to interact with the entity.
        """
        return self.keywords.copy()


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