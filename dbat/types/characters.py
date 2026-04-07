import uuid
import typing
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from .location import HasLocation, Point
from .equipment import HasEquipment
from .inventory import HasInventory
from .dgscripts import HasDgScripts, DgReference
from .misc import HasInteractive, HasFlags, HasColorName, HasColorDescription
import dbat
import copy
from loguru import logger
from rich.text import Text
from rich.errors import MarkupError
from .location import Location

if typing.TYPE_CHECKING:
    from dbat.sessions import Session
    from .objects import Object
    from .accounts import Account
    from .structures import Structure
    from .grids import Direction


class PhysiologyComponent(BaseModel):
    race: str = Field(default="human", description="The race of this character.")
    sex: str = Field(default="male", description="The sex of this character.")

    def get_traits(self, character: Character) -> set[str]:
        pass


class FormData(HasColorDescription, HasColorName, HasFlags):
    duration_in_form: float = Field(default=0.0, description="The duration that the character has been in this form. This is used for things like determining when to end temporary forms, etc.")
    grade: int = Field(default=0, description="The grade of this form. This is used for things like determining if a character can see another character in this form, etc.")
    visible: bool = Field(default=True, description="Whether this form is visible to the user.")
    limit_broken: bool = Field(default=False, description="Whether this form is limit broken.")
    unlocked: bool = Field(default=False, description="Whether this form is unlocked.")
    stats: dict[str, float] = Field(default_factory=dict, description="A dictionary for storing any additional data for this form.")


class FormComponent(BaseModel):
    current: str = Field(default="base", description="The current form of this character.")
    technique: str = Field(default="base", description="The current technique of this character.")
    permanents: set[str] = Field(default_factory=set, description="A set of permanent forms that this character has.")
    data: dict[str, FormData] = Field(default_factory=dict, description="A mapping of form names to form data. This is used for storing the data for each form, such as stat changes or special abilities.")


class SenseiComponent(BaseModel):
    current: str = Field(default="nobody", description="The current sensei of this character.")
    data: dict[str, dict] = Field(default_factory=dict, description="A mapping of sensei names to sensei data. This is used for storing the data for each sensei, such as stat changes or special abilities.")


class CharacterBase(HasColorName, HasColorDescription, HasFlags, HasDgScripts, HasInteractive):
    physiology: PhysiologyComponent = Field(default_factory=PhysiologyComponent, description="The physiology of this character. This is used for things like determining what they can see, what they look like to others, etc.")
    form: FormComponent = Field(default_factory=FormComponent, description="The form of this character. This is used for things like determining what they can do, what they look like to others, etc.")
    sensei: SenseiComponent = Field(default_factory=SenseiComponent, description="The sensei of this character. This is used for things like determining what they can do, what they look like to others, etc.")


class MobilePrototype(CharacterBase):
    _instances: set[uuid.UUID] = PrivateAttr(default_factory=set)
    id: str = Field(..., description="The unique ID of this mobile prototype.")
    
    def save(self):
        dbat.INDEX.dirty_mobile_prototypes.add(self.id)
    
    def spawn(self) -> Mobile:
        data = self.model_dump()
        id = uuid.uuid4()
        data["id"] = id
        mob = Mobile(**data)
        mob.proto_id = self.id
        mob.game_activate()
        return mob


class Character(CharacterBase, HasLocation, HasEquipment, HasInventory):
    """
    Base class for characters. Should not be used directly.
    """
    id: uuid.UUID = Field(..., description="The unique ID of this character.")
    _session: Session | None = PrivateAttr(default=None)
    _command_queue: list[str] = PrivateAttr(default_factory=list)
    __deleted: bool = PrivateAttr(default=False)

    def __bool__(self):
        return not self.__deleted
    
    def save(self):
        pass

    @property
    def admin_level(self) -> int:
        if self._session:
            return self._session.user.admin_level
        return 0

    def send_rich(self, text: str | Text):
        if not self._session:
            return
        try:
            rich_text = Text.from_markup(text) if isinstance(text, str) else text
        except MarkupError:
            logger.warning(f"Failed to parse rich text: {text}")
            rich_text = Text(str(text))
        self._session.append_rich(rich_text)

    def send_text(self, text: str):
        if self._session:
            self._session.append_text(text)
    
    def send_line(self, text: str):
        if self._session:
            self._session.append_line(text)
    
    def send_gmcp(self, package: str, data: dict):
        if self._session:
            self._session.append_gmcp(package, data)
    
    def send_event(self, event):
        if self._session:
            self._session.append_event(event)

    def is_npc(self) -> bool:
        return True
    
    def is_pc(self) -> bool:
        return not self.is_npc()
    
    def valid_location_coordinates(self, point):
        return True

    def enqueue_command(self, command: str):
        self._command_queue.append(command)
        dbat.SUBSCRIPTIONS["pending_commands"].add(self.id)

    def clear_command_queue(self):
        self._command_queue.clear()
        dbat.SUBSCRIPTIONS["pending_commands"].discard(self.id)
    
    def available_commands(self):
        priorities = sorted(list(dbat.CHARACTER_COMMANDS_PRIORITY.keys()), reverse=True)
        for priority in priorities:
            for command_cls in dbat.CHARACTER_COMMANDS_PRIORITY[priority]:
                if command_cls.check_access(self):
                    yield command_cls
    
    def execute_command(self, command_str: str):
        for command_cls in self.available_commands():
            match = command_cls.check_match(self, command_str)
            if match is not None:
                command = command_cls(self, command_str, match)
                return command.execute()
        return {"ok": False, "error": "No matching command found."}

    def process_command_queue(self, delta_time: float):
        if not self._command_queue:
            dbat.SUBSCRIPTIONS["pending_commands"].discard(self.id)
            return
        
        command_str = self._command_queue.pop(0)
        self.execute_command(command_str)

    def as_dg_ref(self) -> DgReference:
        return DgReference(entity_type="character", entity_id=self.id)
    
    def get_apparent_race(self, viewer: Character) -> str:
        return self.physiology.race
    
    def get_apparent_sex(self, viewer: Character) -> str:
        return self.physiology.sex
    
    def get_keywords(self, viewer: Character) -> set[str]:
        out = super().get_keywords(viewer)

        app_race = self.get_apparent_race(viewer)
        app_sex = self.get_apparent_sex(viewer)

        # we'll need to add some checks for if race and sex should be used as keywords but for now...

        if app_race:
            out.add(app_race)
        if app_sex:
            out.add(app_sex)

        return out
    
    def look_at(self, target: Character | Direction | Location | Object | Structure):
        if hasattr(target, "render_look") and callable(target.render_look):
            target.render_look(self)
        else:
            self.send_text("You see nothing special.")

class Mobile(Character):
    """
    Class for Mobiles/NPCs.
    """
    proto_id: str = Field(default="", description="The prototype this mobile was spawned from, if any.")
    spawn_location: Location | None = Field(default=None, description="The location this mobile was spawned at. This is set by reset commands, and should be cleared if it is ever picked up or relocated.", exclude=True)
    
    def __repr__(self):
        return f"<Mobile: {self.color_name.plain} ({self.id})>"
    
    def proto(self) -> MobilePrototype | None:
        return dbat.INDEX.mobile_prototypes.get(self.proto_id, None)

    def game_activate(self):
        dbat.INDEX.mobiles[self.id] = self
        dbat.INDEX.characters[self.id] = self
        dbat.INDEX.entities[self.id] = self
        if p := self.proto():
            p._instances.add(self.id)
    
    def game_deactivate(self):
        dbat.INDEX.mobiles.pop(self.id, None)
        dbat.INDEX.characters.pop(self.id, None)
        dbat.INDEX.entities.pop(self.id, None)
        if p := self.proto():
            p._instances.discard(self.id)
    
    def get_display_name(self, viewer: Character, capitalize: bool = False) -> Text:
        out = self.color_name.copy()
        if capitalize:
            out.plain = out.plain.capitalize()
        
        if not viewer.can_see(self):
            return Text("Someone" if capitalize else "someone")

        return out


class PlayerCharacter(Character):
    """
    Class for Player Characters.
    """
    dub_names: dict[uuid.UUID, str] = Field(default_factory=dict, description="A mapping of character IDs to the dub name for that character. This is used for displaying other characters with a different name than their actual name, such as for NPCs or for players who have chosen to dub another player with a different name.")
    account_id: uuid.UUID | None = Field(default=None, description="The account this player character belongs to.")

    def __repr__(self):
        return f"<PlayerCharacter: {self.color_name.plain} ({self.id})>"
    
    def game_activate(self):
        dbat.INDEX.players[self.id] = self
        dbat.INDEX.characters[self.id] = self
        dbat.INDEX.entities[self.id] = self

        # Adding in a location hack for dev time now.
        if not self.location:
            z = dbat.INDEX.slugs["zone"]["3"]
            l = Location(location_type="zone", location_id=z.id, point=Point(x=300,y=0,z=0))
            self.add_to_location(l)
    
    def game_deactivate(self):
        dbat.INDEX.players.pop(self.id, None)
        dbat.INDEX.characters.pop(self.id, None)
        dbat.INDEX.entities.pop(self.id, None)

    def is_npc(self) -> bool:
        return False
    
    def is_admin(self) -> bool:
        return False
    
    def save(self):
        """
        This will save the player character to the database.
        Or rather, it will enqueue them to be saved after this tick.
        """
        dbat.INDEX.dirty_players.add(self.id)
    
    def get_keywords(self, viewer: Character) -> set[str]:
        out = super().get_keywords(viewer)

        if dub := viewer.dub_names.get(self.id, None):
            out.update(dub.lower().split())
            for x in ("a", "the", "an"):
                out.discard(x)
        
        return out
    
    def get_display_name(self, viewer: Character, capitalize: bool = False) -> Text:
        """
        Get the display name for this character. This is used for displaying the character to the viewer.
        It may be different from the name for other viewers if the character is hidden or invisible.
        For PCs, we want to show their name even if they are hidden, so we override this method.
        """
        out = self.color_name.copy()
        if capitalize:
            out.plain = out.plain.capitalize()
        
        if not viewer.can_see(self):
            return Text("Someone" if capitalize else "someone")

        if viewer.is_npc() or viewer.is_admin() or self.is_admin():
            return out

        if dub := viewer.dub_names.get(self.id, None):
            return Text(dub)

        # Okay it's a normal player viewing another normal player.
        # This is just placeholder logic for now. Will do a richer breakdown later.
        app_race = self.get_apparent_race(viewer)
        app_sex = self.get_apparent_sex(viewer)

        prefix = "A" if capitalize else "a"
        
        return Text(f"{prefix} {app_sex} {app_race}")