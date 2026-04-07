import uuid
from pydantic import BaseModel, Field, ConfigDict, model_validator, PrivateAttr
from venv import logger
import dbat
import typing
from enum import Enum
from .location import Point

if typing.TYPE_CHECKING:
    from .grids import POINT


class TriggerType(Enum):
    """
    A general note: "in this location" means the current Tile/coordinates, but only if the entity is on the map.
    For instance, an object in a character's inventory would not trigger GREET when someone enters the same room as the character,
    nor will it consider other things being added to the same inventory as a GREET. It would only trigger if it's 'on the ground'.

    For tiles, "in this location" means the tile itself.

    """
    # General Triggers
    GLOBAL = "global"   # happens even if zone empty???
    RANDOM = "random"   # checked at random intervals
    COMMAND = "command" # player types a command in entity's location. must match command field.
    SPEECH = "speech"  # player says something in entity's location.
    ACT = "act"  # emotes, etc, seen in entity's location
    DEATH = "death"  # when this entity dies / is destroyed.
    GREET = "greet"  # when a player enters the location this entity is in, and they're visible. Tiles always see everything.
    GREET_ALL = "greet_all"  # when a player enters the location this entity is in, regardless of visibility.
    ENTRY = "entry"  # when this entity enters a new location
    LEAVE = "leave"  # when someone leaves the location this entity is in, and they're visible. It can prevent leaving.
    RECEIVE = "receive"  # when this entity is given an object.
    LOAD = "load"  # when the entity spawns into the world via Map Resets.
    DOOR = "door"  # when someone manipulates a door in the current location
    TIME = "time"  # checked at game hour narg
    HOURLY = "hourly"  # checked every game hour
    QUARTERLY = "quarterly"  # checked every game quarter hour
    CAST = "cast"  # something about spellcasting

    # Mobile Specific
    FIGHT = "fight"  # checked every combat round when this entity is fighting.
    HEALTH_PERCENT = "health_percent"  # checked same as fight, when health% drops below narg
    BRIBE = "bribe"  # when this entity is given money.
    MEMORY = "memory"  # when the entity sees someone it remembers

    # Object Specific
    GET = "get"  # when this entity is picked up
    DROP = "drop"  # when this entity is attempted to be dropped
    GIVE = "give"  # when this entity is attempted to be given to someone
    WEAR = "wear"  # when this entity is attempted to be equipped
    REMOVE = "remove"  # when this entity is attempted to be unequipped
    CONSUME = "consume"  # when this entity is attempted to be consumed (eaten, quaffed, etc)
    TIMER = "timer"  # the object has some kind of timer.

    # Tile Specific
    RESET = "reset"  # when the tile resets.
    #DROP = "drop"  # when something is attempted to be dropped on this tile.

class ScriptLineType(Enum):
    COMMAND = "command"
    IF = "if"
    ELSEIF = "elseif"
    ELSE = "else"
    END = "end"
    SWITCH = "switch"
    CASE = "case"
    BREAK = "break"
    DEFAULT = "default"
    WHILE = "while"
    DONE = "done"
    COMMENT = "comment"

def parse_script(code: str) -> list[tuple[ScriptLineType, str]]:
    out = list()

    for line in code.splitlines():
        trimmed = line.strip()
        if not trimmed:
            continue
        
        # Handle numbered lines like "1: if (...)" or "108: elseif"
        # Strip the leading number and colon if present
        if trimmed[0:1].isdigit():
            colon_pos = trimmed.find(":")
            if colon_pos != -1:
                trimmed = trimmed[colon_pos + 1:].strip()
        
        if not trimmed:
            continue
        if trimmed.startswith("*"):
            out.append((ScriptLineType.COMMENT, trimmed[1:]))
            continue

        # we need to 'get the first token', which is terminated by a space or open paren.
        first_token = ""
        for c in trimmed:
            if c in (" ", "("):
                break
            first_token += c
        
        match first_token.lower():
            case "if":
                out.append((ScriptLineType.IF, trimmed[len(first_token):]))
            case "elseif":
                out.append((ScriptLineType.ELSEIF, trimmed[len(first_token):]))
            case "else":
                out.append((ScriptLineType.ELSE, trimmed[len(first_token):]))
            case "end":
                out.append((ScriptLineType.END, ""))
            case "switch":
                out.append((ScriptLineType.SWITCH, trimmed[len(first_token):]))
            case "case":
                out.append((ScriptLineType.CASE, trimmed[len(first_token):]))
            case "break":
                out.append((ScriptLineType.BREAK, ""))
            case "default":
                out.append((ScriptLineType.DEFAULT, ""))
            case "while":
                out.append((ScriptLineType.WHILE, trimmed[len(first_token):]))
            case "done":
                out.append((ScriptLineType.DONE, ""))
            case _:
                out.append((ScriptLineType.COMMAND, trimmed))
    return out

def render_code(code: list[tuple[ScriptLineType, str]]) -> str:
    out = ""

    depth = 0
    i = 0
    for line_type, arg in code:
        line = ""
        predepthmod = 0
        postdepthmod = 0

        def strip_unnecessary_parens(s: str) -> str:
            if s.startswith("(") and s.endswith(")"):
                out = s[1:-1].strip()
                return strip_unnecessary_parens(out)
            return s.strip()

        match line_type:
            case ScriptLineType.COMMAND:
                line = arg
            case ScriptLineType.IF | ScriptLineType.WHILE:
                line = f"{line_type.value} {strip_unnecessary_parens(arg.strip())}"
                postdepthmod = 1
            case ScriptLineType.ELSEIF | ScriptLineType.ELSE | ScriptLineType.CASE | ScriptLineType.DEFAULT:
                predepthmod = -1
                line = f"{line_type.value} {strip_unnecessary_parens(arg.strip())}".strip()
                postdepthmod = 1
            case ScriptLineType.END | ScriptLineType.DONE:
                predepthmod = -1
                line = line_type.value
            case ScriptLineType.SWITCH:
                line = f"switch {strip_unnecessary_parens(arg.strip())}"
                postdepthmod = 1
            case ScriptLineType.BREAK:
                line = line_type.value
            case ScriptLineType.COMMENT:
                line = f"* {arg.strip()}"
        
        if predepthmod != 0:
            depth = max(0, depth + predepthmod)
        out += ("  " * depth) + line + "\n"
        if postdepthmod != 0:
            depth = max(0, depth + postdepthmod)
        i+=1

    return out

class DgTreeNode:
    """
    This is a 'compiled' version of a DgScript, beyond the simple parse_script function.
    It bundles the script into distinct sections to make navigation easier.
    
    Each node preserves the original line number for pause/resume functionality.
    """

    def __init__(self, line: int, line_type: ScriptLineType, arg: str, parent: "DgTreeNode | None" = None):
        self.line: int = line
        self.line_type: ScriptLineType = line_type
        self.arg: str = arg
        self.parent: DgTreeNode | None = parent
        self.children: list[DgTreeNode] = []
    
    def find_parent_of_type(self, line_type: ScriptLineType) -> "DgTreeNode | None":
        """Find the nearest ancestor node with the given type."""
        current = self.parent
        while current:
            if current.line_type == line_type:
                return current
            current = current.parent
        return None
    
    def get_section_nodes(self) -> list["DgTreeNode"]:
        """Get the immediate children of this block node."""
        return self.children
    
    def get_all_nodes(self) -> list["DgTreeNode"]:
        """Get all descendant nodes in execution order."""
        result = [self]
        for child in self.children:
            result.extend(child.get_all_nodes())
        return result
    
    def __repr__(self):
        return f"DgTreeNode(line={self.line}, type={self.line_type.value}, arg='{self.arg[:20]}...')"


def compile_dgscript(code: list[tuple[ScriptLineType, str]]) -> tuple[dict[int, DgTreeNode], DgTreeNode]:
    """
    Compiles parsed script lines into a tree structure.
    Returns (node_map, root_node) where node_map maps line numbers to nodes.
    
    The tree structure:
    - IF/ELSEIF/ELSE/END form a single block
    - WHILE/DONE form a loop block
    - SWITCH/CASE/DEFAULT/BREAK form a switch block
    - Commands are leaf nodes
    """
    # Root node to hold the entire script
    root = DgTreeNode(0, ScriptLineType.COMMAND, "root")
    node_map: dict[int, DgTreeNode] = {}
    
    # Stack tracks parent nodes for block nesting
    block_stack: list[DgTreeNode] = [root]
    
    # Track if/else chain for ELSEIF/ELSE to find the correct parent IF
    if_stack: list[DgTreeNode] = []
    
    for line_num, (line_type, arg) in enumerate(code, start=1):
        node = DgTreeNode(line_num, line_type, arg, block_stack[-1])
        node_map[line_num] = node
        block_stack[-1].children.append(node)
        
        match line_type:
            case ScriptLineType.IF:
                # New if block
                if_stack.append(node)
                block_stack.append(node)
                
            case ScriptLineType.ELSEIF:
                # Elseif attaches to the most recent IF (not elseif)
                # Pop the previous elseif/else from stacks, but keep the IF
                while block_stack and block_stack[-1].line_type in (ScriptLineType.ELSEIF, ScriptLineType.ELSE):
                    block_stack.pop()
                # Now attach to the IF node
                if if_stack:
                    if_node = if_stack[-1]
                    node.parent = if_node
                    # Defensive: only remove if actually present
                    if node.parent != block_stack[-1]:
                        try:
                            block_stack[-1].children.remove(node)
                        except ValueError:
                            pass
                        if node not in if_node.children:
                            if_node.children.append(node)
                    block_stack.append(node)
                else:
                    # No matching IF - treat as command (not an error)
                    pass
                
            case ScriptLineType.ELSE:
                # Same as elseif - attach to the IF node
                while block_stack and block_stack[-1].line_type in (ScriptLineType.ELSEIF, ScriptLineType.ELSE):
                    block_stack.pop()
                if if_stack:
                    if_node = if_stack[-1]
                    node.parent = if_node
                    if node.parent != block_stack[-1]:
                        try:
                            block_stack[-1].children.remove(node)
                        except ValueError:
                            pass
                        if node not in if_node.children:
                            if_node.children.append(node)
                    block_stack.append(node)
                else:
                    # No matching IF - treat as command
                    pass
                
            case ScriptLineType.END:
                # End current if/while block - defensive checks
                if block_stack and len(block_stack) > 1:  # len > 1 because root is always there
                    top = block_stack[-1]
                    if top.line_type in (ScriptLineType.IF, ScriptLineType.ELSEIF, ScriptLineType.ELSE):
                        block_stack.pop()
                
                # Pop from if_stack if we hit the matching IF
                if not block_stack:
                    # Nothing to pop, script malformed
                    pass
                elif if_stack:
                    try:
                        if block_stack[-1].line_type == ScriptLineType.ELSEIF:
                            block_stack.pop()
                        if block_stack and block_stack[-1].line_type == ScriptLineType.ELSE:
                            block_stack.pop()
                        if block_stack and block_stack[-1].line_type == ScriptLineType.IF:
                            if_stack.pop()
                            block_stack.pop()
                    except (IndexError, AttributeError):
                        pass  # Malformed script, skip
                    
            case ScriptLineType.WHILE:
                block_stack.append(node)
                
            case ScriptLineType.DONE:
                # End while OR switch - pop whichever is on top
                if block_stack and len(block_stack) > 1:
                    top_type = block_stack[-1].line_type
                    if top_type in (ScriptLineType.WHILE, ScriptLineType.SWITCH):
                        block_stack.pop()
                    
            case ScriptLineType.SWITCH:
                block_stack.append(node)
                
            case ScriptLineType.CASE | ScriptLineType.DEFAULT:
                # Find enclosing switch - search in block_stack
                switch_parent = None
                for s in reversed(block_stack):
                    if s.line_type == ScriptLineType.SWITCH:
                        switch_parent = s
                        break
                if switch_parent:
                    node.parent = switch_parent
                    if node not in switch_parent.children:
                        switch_parent.children.append(node)
                block_stack.append(node)
                
            case ScriptLineType.BREAK:
                # Break exits the current case/switch/while - pop until we find one, but don't pop it
                while block_stack and block_stack[-1].line_type not in (ScriptLineType.WHILE, ScriptLineType.SWITCH, ScriptLineType.CASE, ScriptLineType.DEFAULT):
                    block_stack.pop()
                # Don't pop the switch/while, just exit the case
                    
            case _:
                # COMMAND, COMMENT - leaf nodes
                pass
    
    return node_map, root


class DgScript(BaseModel):
    id: str = Field(default=..., description="Unique identifier for this script.")
    name: str = Field(default="", description="A human-readable name for this script.")
    attach_type: str = Field(default="", description="The type of entity this script can be attached to. This is just a label and has no real meaning in code.")
    narg: int = Field(default=0, description="The narg field from the original DgScript. This is just a label and has no real meaning in code.")
    triggers: set[TriggerType] = Field(default_factory=set, description="The set of triggers that activate this script.")
    command: str = Field(default="", description="The command field from the original DgScript. This is just a label and has no real meaning in code.")
    body: str = Field(default="", description="The original code body of this script. This is just a label and has no real meaning in code, as the actual code that is executed is stored in __lines after parsing.")
    __lines: list[tuple[ScriptLineType, str]] = PrivateAttr(default_factory=list)
    __root_node: DgTreeNode | None = PrivateAttr(default=None)
    __node_map: dict[int, DgTreeNode] = PrivateAttr(default_factory=dict)
    
    def script(self):
        return render_code(self.__lines)

    @model_validator(mode="after")
    def _compile_from_body(self):
        if self.body and not self.__lines:
            self.__lines = parse_script(self.body)
            dg_tree, root = compile_dgscript(self.__lines)
            self.__root_node = root
            self.__node_map = dg_tree
        return self


class DgReference(BaseModel):
    """
    Variables in DgScripts can be either a string or a reference to another game object.
    This handles the latter case.
    """
    entity_type: str = Field(..., description="The type of entity this reference points to. Can be 'character', 'object', 'zone', or 'structure'.")
    entity_id: uuid.UUID = Field(..., description="The unique ID of the entity this reference points to.")
    point: Point = Field(default_factory=Point, description="For tile reference. For characters/objects, this is unused.")    
    
    def __bool__(self):
        return bool(self.get_target())
    
    def get_target(self) -> HasDgScripts | None:
        match self.entity_type:
            case "zone":
                if z := dbat.index.get_zone(self.entity_id):
                    return z.tiles.get(self.point, None)
            case "structure":
                if s := dbat.index.get_structure(self.entity_id):
                    return s.tiles.get(self.point, None)
            case "character":
                return dbat.index.get_character(self.entity_id)
            case "object":
                return dbat.index.get_object(self.entity_id)
            case _:
                return None

class HasDgScripts(BaseModel):
    proto_script: list[str] = Field(default_factory=list, description="The list of script IDs that are attached to this entity by its prototype. This is used to determine the order of script execution, and should not be modified directly.")
    __dgscripts: dict[str, DgScriptInstance] = PrivateAttr(default_factory=dict)
    __script_order: list[str] | None = PrivateAttr(default=None)
    dg_variables: dict[str, str | DgReference] = Field(default_factory=dict, description="The variables that are currently set for this entity's DgScripts.")
    
    def evaluate_dgscript_member(self, script: "DgScriptInstance", field: str, arg: str) -> None | str:
        return None
    
    def as_dg_ref(self) -> DgReference:
        raise NotImplementedError("Every HasDgScripts needs to implement as_dg_ref()!")

    def get_dgscript_order(self) -> list[str]:
        """
        Most entities use the proto_script as their script order,
        but a few change their script order around during runtime,
        so we have script_order as an override.
        """
        if self.__script_order is not None:
            return self.__script_order.copy()
        return self.proto_script.copy()
    
    def iter_dgscripts(self):
        for script_id in self.get_dgscript_order():
            if script_id in self.__dgscripts:
                yield self.__dgscripts[script_id]

    def trigger_dgscripts(self, trigger: TriggerType, **kwargs):
        """
        Trigger all DgScripts matching the given trigger type, in order.
        """
        # we need to santy check the variables passed in, as they'll be given to the DgScripts.
        # Valid variables: strings, DgReferences or anything that can become one
        vars = dict()
        for k, v in kwargs.items():
            if isinstance(v, (str, DgReference)):
                vars[k] = v
            elif isinstance(v, HasDgScripts):
                vars[k] = v.as_dg_ref()
            else:
                logger.warning(f"trigger_dgscripts: Invalid variable type for {k}: {type(v)}. Skipping this variable.")

        for script in self.iter_dgscripts():
            # We will fire off the first script that can accept the trigger.
            if trigger not in script.script.triggers:
                continue
            if not script.ready():
                # The script isn't in a ready state. It is either running or waiting.
                continue
            script.variables.update(vars)
            res = script.execute(trigger)

            return res

    def add_dgscript(self, script: DgScript):
        """
        Adds a DgScript to this entity, but doesn't add it to the prototype.
        """
        if script.id in self.__dgscripts:
            raise ValueError(f"Script {script.id} is already attached to this entity.")
        instance = DgScriptInstance(script, self)
        self.__dgscripts[script.id] = instance
        if not self.__script_order:
            self.__script_order = self.proto_script.copy()
        self.__script_order.append(script.id)
    
    def remove_dgscript(self, script_id: str):
        """
        Removes a DgScript from this entity, but doesn't remove it from the prototype.
        """
        if script_id in self.__dgscripts:
            del self.__dgscripts[script_id]
            if self.__script_order and script_id in self.__script_order:
                self.__script_order.remove(script_id)

class DgScriptInstance(BaseModel):
    script: DgScript = Field(..., description="The DgScript this instance is based on.", exclude=True)
    owner: HasDgScripts = Field(..., description="The entity this script is attached to.", exclude=True)
    variables: dict[str, str | DgReference] = Field(default_factory=dict, description="The variables currently set for this script instance. These are used when executing the script.", exclude=True)
    
    def __bool__(self):
        return bool(self.owner)