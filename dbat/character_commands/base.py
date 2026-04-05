import re
import typing

if typing.TYPE_CHECKING:
    from ..types.characters import Character, Mobile, PlayerCharacter

CMD_MATCH = re.compile(
    r"(?s)^(?P<cmd>\S+?)(?:/(?P<switches>\S+)?)?(?P<fullargs> +(?P<args>(?P<lsargs>.+?)(?:=(?P<rsargs>.*))?)?)?$"
)


class CharacterCommand:
    """
    Help not implemented for this command. Contact staff!
    """

    # The unique key for this command. This is used for identifying it,
    # but also for overriding it with plugins.
    key = "dbat/notset"

    # If help_* is None, the command will not be listed in the help system.
    help_name = None
    help_category = None

    # higher priority commands are checked first, by grouping.
    priority = 0

    # match_defs is a dict of command name to minimum length for matching. 
    # The command will be matched if the input matches the command name, 
    # or if it is at least the minimum length and matches the start of 
    # the command name.
    match_defs: dict[str, int] = dict()

    # The minimum admin_level of the character (user) to use the command.
    min_level = 0

    # Set this to true if you want the command to exist but never reach the parser.
    # this could be helpful for creating help files or meta-topics.
    unusable = False

    class Error(ValueError):
        pass

    @classmethod
    def check_match(cls, character: Character, command: str) -> typing.Optional[dict[str, str]]:
        """
        Check if the command matches the user's input.

        Command will already be trimmed and lowercase. Equal to the <cmd> in the regex.

        We are a match if it is a direct match with an alias, or if it is a complete match
        with the command name, or if it is a partial match with the command name starting
        with min_length and not contradicting the name.

        IE: "north" should respond to "nort" but not "norb"
        """
        match_data = CMD_MATCH.match(command)
        if not match_data:
            return None
        match_dict = {k: v for k, v in match_data.groupdict().items() if v}

        cmd = match_dict.get("cmd", "").lower()
        
        for k, v in cls.match_defs.items():
            if cmd == k:
                match_dict["full_command"] = k
                return match_dict
            if len(cmd) >= v and k.startswith(cmd):
                match_dict["full_command"] = k
                return match_dict
        return None

    @classmethod
    def check_access(cls, character: Character) -> bool:
        """
        Check if the user should have access to the command.
        If they don't, they don't see it at all.

        Args:
            character: The character to check access for.

        Returns:
            bool: True if the user has access, False otherwise.
        """
        return character.admin_level >= cls.min_level

    def __init__(
        self,
        character: Character,
        raw: str,
        parsed: dict[str, str],
    ):
        self.character = character
        self.raw = raw
        self.parsed = parsed

    def can_execute(self) -> bool:
        """
        Check if the command can be executed.
        """
        return True

    def execute(self) -> dict:
        """
        Execute the command.

        Returns:
            dict: The result of the command execution.

        Raises:
            HTTPException: If the command cannot be executed.
        """
        if not self.can_execute():
            return {"ok": False, "error": "Cannot execute command"}
        try:
            result = self.func()
            return result or {"ok": True}
        except self.Error as err:
            self.send_line(f"{err}")
            return {"ok": False, "error": str(err)}

    def func(self) -> dict | None:
        """
        Execute the command. Overload this.
        """
        pass

    def send_text(self, text: str):
        self.character.send_text(text)

    def send_line(self, text: str):
        self.character.send_line(text)

    def send_data(self, package: str, data):
        self.character.send_gmcp(package, data)
    
    def send_event(self, event):
        self.character.send_event(event)
