import uuid
import dbat
from datetime import datetime
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from .misc import HasColorName, HasColorDescription, HasFlags
from rich.text import Text


class HeDiffHandler:
    """
    Base class for the behaviors of a HeDiff.
    While Characters store the 'state', this has the logic. A set of HeDiffHandlers are stored on the Indexer at startup using its key.
    """
    # Whether this hediff should be automatically removed when its severity reaches 0.0
    clears_on_zero = True

    # Whether this hediff should be automatically removed when its source
    # is no longer valid.
    source_independent = True

    # If this HeDiff is health-related. This affects where it's displayed.
    health_related = False

    # If false, each hediff is a unique instance. If true, then attempts to create new instances
    # will instead add to the existing severity. This is used for things like Bloodloss.
    stacks = False

    def key(self) -> str:
        """
        Return the key that identifies this hediff handler. This is used for looking up the handler for a given hediff.
        """
        raise NotImplementedError("HeDiffHandler subclasses must implement the key() method to return their unique key.")
    
    def on_apply(self, hediff: "HeDiff", character: "Character"):
        """
        Apply the behavior of the given hediff to the given character. This is called automatically by the game engine, and should not be called manually.
        """
        pass

    def on_remove(self, hediff: "HeDiff", character: "Character"):
        """
        Remove the behavior of the given hediff from the given character. This is called automatically by the game engine when a hediff is removed, and should not be called manually.
        """
        pass

    def on_update(self, hediff: "HeDiff", character: "Character", delta_time: float):
        """
        Update the behavior of the given hediff on the given character. This is called automatically by the game engine every tick, and should not be called manually.
        """
        pass

    def check_valid(self, hediff: "HeDiff", character: "Character") -> bool:
        """
        Check if this HeDiff is still valid on the given character.
        """
        if self.clears_on_zero and hediff.severity <= 0.0:
            return False
        if not self.source_independent and (hediff.source is not None and hediff.source.is_entity()):
            if not (ent := hediff.source.get()):
                return False
        return True
    
    def report_name(self, hediff: "HeDiff", character: "Character") -> Text:
        """
        Return the name of this hediff to be shown in the character's displays.
        """
        return hediff.name or Text(hediff.key)

    def report_description(self, hediff: "HeDiff", character: "Character") -> Text:
        """
        Return the description of this hediff to be shown in the character's displays.
        """
        return Text(hediff.key)


class EquippedItem(HeDiffHandler):
    """
    A hediff representing an item that a character has equipped. 
    This is used for things like armor, weapons, accessories, etc. 
    It can be used to apply the effects of the item to the character, 
    and to display the item in the character's displays.
    The source of this hediff should be the item that is equipped.

    This is mainly used for stat bonuses.
    """
    health_related = False

    def key(self) -> str:
        return "equipped_item"


class TreatableBase(HeDiffHandler):
    """
    If something can be medically treated, it inherits from this.
    This is the base of all injuries and diseases.
    """
    health_related = True

    def get_treat_quality(self, hediff: "HeDiff", character: "Character") -> float:
        """
        Return the quality of treatment that this hediff would receive if treated right now. This is used for things like determining how effective a treatment would be, and for displaying the quality of treatment in the character's displays.
        The quality is a number from 0.0 to 100.0, where 0.0 means no treatment and 100.0 means perfect treatment.
        By default, this returns 100.0 if the hediff is treatable, and 0.0 if it's not. Subclasses can override this to provide more nuanced behavior.
        """
        return hediff.data.get("treat_quality", 0.0)

    def is_treated(self, hediff: "HeDiff", character: "Character") -> bool:
        """
        Return whether this hediff is currently being treated. This is used for things like determining how effective a treatment would be, and for displaying the quality of treatment in the character's displays.
        By default, this checks if the "is_treated" flag is set in the hediff's data. Subclasses can override this to provide more nuanced behavior.
        """
        return self.get_treat_quality(hediff, character) > 0.0


class Bloodloss(TreatableBase):
    """
    Blood loss. This is a special hediff that represents the 
    amount of blood a character has lost. It can be caused by injuries, 
    and can cause death if it reaches 100%. 
    It can be treated with things like bandages, tourniquets, 
    and blood transfusions.
    """
    stacks = True

    def key(self) -> str:
        return "blood_loss"


# Now for the different kinds of HeDiffHandlers.
class InjuryBase(TreatableBase):
    """
    Injuries heal over time, usually, and can be treated. They usually bleed.
    Their display names vary with severity.
    """

    def update_bloodloss(self, hediff: "HeDiff", character: "Character", delta_time: float):
        """
        Process the blood loss caused by this injury. This is called automatically by the game engine every tick, and should not be called manually.
        Subclasses can override this to provide more nuanced behavior.
        """
        # TODO: make sure that certain races just cannot bleed. Example: Android, Majin.
        if self.is_treated(hediff, character):
            return
    
    def update_healing(self, hediff: "HeDiff", character: "Character", delta_time: float):
        """
        Handle the healing over time of this injury. This should reduce its severity.
        """

    def on_update(self, hediff: "HeDiff", character: "Character", delta_time: float):
        self.update_bloodloss(hediff, character, delta_time)
        self.update_healing(hediff, character, delta_time)


class Impact(InjuryBase):
    """
    Blunt trauma. usually caused by punches, kicks, falls, etc.
    At low severity, a bruise. At high, a shattering/broken bone.
    Generally doesn't bleed.

    Dragon Ball characters can handle a lot of this, in general.
    For some, they're insanely resistant to it.
    """
    def key(self) -> str:
        return "impact"


class Laceration(InjuryBase):
    """
    Cuts. Slashes from swords or knives, claws from animals, etc.
    At low severity, a scratch, at middle a gut, high is a deep gash, 
    100% may mean amputation.

    Bleeding for low severity is meh, but it quickly gets worse.
    """
    def key(self) -> str:
        return "laceration"


class Puncture(InjuryBase):
    """
    Stabs and piercings. Thrusts from swords, knives, spears. Bites from animals with nasty teeth.
    Bleeding is usually bad from the start and quickly gets worse.
    """
    def key(self) -> str:
        return "puncture"


class Burn(InjuryBase):
    """
    Burns. Usually caused by fire, heat, or chemicals.
    """

    def key(self) -> str:
        return "burn"


class Frostbite(InjuryBase):
    """
    Frostbite. Usually caused by exposure to extreme cold.
    """
    def key(self) -> str:
        return "frostbite"


class Shock(InjuryBase):
    """
    Shock. Bzzzt. Electric damage.
    """
    def key(self) -> str:
        return "shock"


class Abrasion(InjuryBase):
    """
    Abrasion. Scrapes and grinding.
    For Dragon Ball, this is the damage inflicted by many ki blasts.
    Low severity is scrapes and grazes, high severity means lots of skin and underlying flesh is missing, serious bleeding.
    """
    def key(self) -> str:
        return "abrasion"






## Below is what goes on Characters.

class HeDiffBase(HasColorName, HasFlags):
    """
    Base data storage of HeDiff, which is used for Prototypes.
    """
    parts: set[str] = Field(default_factory=set, description="The parts affected by this hediff. This might be a body part, a sense, or something abstract like the soul.")
    severity: float = Field(default=0.0, min=0.0, max=100.0, description="The severity of this hediff. This is a general-purpose number that can be used for anything. It ranges from 0.0 to 100.0")
    key: str = Field(..., description="The type/key used to lookup the object that implements this hediff's behavior.")
    data: dict = Field(default_factory=dict, description="The data for this hediff. This is used for storing any additional data that the hediff's behavior might need.")


class HeDiffPrototype(HeDiffBase):
    """
    A prototype for a hediff. This is used for defining the base properties of a hediff, which can then be applied to characters as HeDiff instances.
    """
    pass

class HeDiffSource(BaseModel):
    """
    A source of a hediff. This is used for things like determining where a disease came from, or who caused a wound.
    """
    type: str = Field(..., description="The type of the source. This is used for things like determining where a disease came from, or who caused a wound.")
    entity_type: str | None = Field(default=None, description="The entity type of the source. This is used for things like determining where a disease came from, or who caused a wound.")
    id: uuid.UUID | None = Field(default=None, description="The ID of the source. This is used for things like determining where a disease came from, or who caused a wound.")
    name: str | None = Field(default=None, description="The name of the source. This is used for things like determining where a disease came from, or who caused a wound.")

    def get(self):
        """
        Get the actual source object that this HeDiffSource refers to, if it exists. This is used for things like determining where a disease came from, or who caused a wound.
        """
        if self.id is None:
            return None
        
        match self.type:
            case "character":
                return dbat.INDEX.get_character(self.id)
            case "object":
                return dbat.INDEX.get_object(self.id)
            case "structure":
                return dbat.INDEX.get_structure(self.id)
            case _:
                return None
    
    def is_entity(self) -> bool:
        """
        Whether the source is an "entity" of some kind,
        meaning there is a corresponding object we could possibly retrieve.
        """
        match self.type:
            case "character" | "object" | "structure" | "equipment":
                return True
        return False

class HeDiff(HeDiffBase):
    """
    We're basically copying RimWorld here.
    A HeDiff ("Health Difference") is a condition that affects a character,
    whether good or bad. It might be a disease, a wound, a magical buff,
    a curse, a cybernetic implant, etc. Some wear off over time, others last indefinitely.
    """
    id: uuid.UUID = Field(default_factory=uuid.uuid4, description="The unique ID of this hediff.")
    created_at: datetime = Field(default_factory=datetime.now, description="The time this hediff was created.")
    source: HeDiffSource | None = Field(default=None, description="The source of this hediff. This is used for things like determining where a disease came from, or who caused a wound.")
    _handler = PrivateAttr(default=None)  # this will be set to the object that implements this hediff's behavior, after it's looked up using the type_key. We store it here so we don't have to look it up every time we want to apply the hediff's behavior.

    @property
    def handler(self) -> HeDiffHandler:
        if self._handler is None:
            self._handler = dbat.INDEX.get_hediff(self.key)
            if self._handler is None:
                raise ValueError(f"No handler found for hediff key: {self.key}")
        return self._handler
    
    def on_apply(self, character: "Character"):
        """
        Apply the behavior of this hediff to the given character. This is called automatically by the game engine, and should not be called manually.
        """
        self.handler.on_apply(self, character)
    
    def on_remove(self, character: "Character"):
        """
        Remove the behavior of this hediff from the given character. This is called automatically by the game engine when a hediff is removed, and should not be called manually.
        """
        self.handler.on_remove(self, character)

    def on_update(self, character: "Character", delta_time: float):
        """
        Update the behavior of this hediff on the given character. This is called automatically by the game engine every tick, and should not be called manually.
        """
        self.handler.on_update(self, character, delta_time)