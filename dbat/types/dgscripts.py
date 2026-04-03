import uuid
from venv import logger
import dbat
import typing
from enum import Enum

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
    LEAVE = "leave"  # when someone leaves the location this entity is in, and they're visible.
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


class DgScript:
    
    def __init__(self):
        self.id: str = ""
        self.name: str = ""
        # attach_type has no real meaning in code, it's just a label.
        self.attach_type: str = ""
        self.narg: int = 0
        self.triggers: set[TriggerType] = set()
        self.command: str = ""
        self.code: list[tuple[ScriptLineType, str | None]] = []
        self.root_node: DgTreeNode | None = None
        self.node_map: dict[int, DgTreeNode] = dict()
    
    def script(self):
        return render_code(self.code)
    
    def dump(self) -> dict:
        return {"id": self.id,
                "name": self.name,
                "attach_type": self.attach_type,
                "narg": self.narg,
                "triggers": [t.value for t in self.triggers],
                "command": self.command,
                "code": self.script()
                }

    @classmethod
    def load(cls, data: dict) -> "DgScript":
        s = cls()
        s.id = data["id"]
        s.name = data["name"]
        s.attach_type = data["attach_type"]
        s.narg = data["narg"]
        s.triggers = set(TriggerType(t) for t in data["triggers"])
        s.command = data["command"]
        s.code = parse_script(data["code"])
        dg_tree, root = compile_dgscript(s.code)
        s.root_node = root
        s.node_map = dg_tree
        return s

class DgReference:
    """
    Variables in DgScripts can be either a string or a reference to another game object.
    This handles the latter case.
    """

    def __init__(self, entity_type: str, entity_id: uuid.UUID, point: POINT = (0,0,0)):
        self.entity_type = entity_type
        self.entity_id = entity_id
        self.point = point
    
    def __bool__(self):
        return bool(self.get_target())
    
    def get_target(self) -> HasDgScripts | None:
        match self.entity_type:
            case "zone":
                if z := dbat.ZONES.get(self.entity_id, None):
                    return z.tiles.get(self.point, None)
            case "structure":
                if s := dbat.STRUCTURES.get(self.entity_id, None):
                    return s.tiles.get(self.point, None)
            case "character":
                return dbat.CHARACTERS.get(self.entity_id, None)
            case "object":
                return dbat.OBJECTS.get(self.entity_id, None)
            case _:
                return None
    
    def dump(self) -> dict:
        return {"entity_type": self.entity_type,
                "entity_id": self.entity_id,
                "point": list(self.point)
                }
    
    @classmethod
    def load(cls, data: dict) -> DgReference:
        return DgReference(**data)

class HasDgScripts:

    def __init__(self):
        self.proto_script: list[str] = list()
        self.dgscripts: dict[str, "DgScriptInstance"] = dict()
        self.script_order: list[str] | None = None
        self.dg_variables: dict[str, str | DgReference] = dict()
    
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
        if self.script_order is not None:
            return self.script_order.copy()
        return self.proto_script.copy()
    
    def iter_dgscripts(self):
        for script_id in self.get_dgscript_order():
            if script_id in self.dgscripts:
                yield self.dgscripts[script_id]

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
        if script.id in self.dgscripts:
            raise ValueError(f"Script {script.id} is already attached to this entity.")
        instance = DgScriptInstance(script, self)
        self.dgscripts[script.id] = instance
        if not self.script_order:
            self.script_order = self.proto_script.copy()
        self.script_order.append(script.id)
    
    def remove_dgscript(self, script_id: str):
        """
        Removes a DgScript from this entity, but doesn't remove it from the prototype.
        """
        if script_id in self.dgscripts:
            del self.dgscripts[script_id]
            if self.script_order and script_id in self.script_order:
                self.script_order.remove(script_id)

class DgScriptInstance:
    
    def __init__(self, script: DgScript, owner: HasDgScripts):
        self.script = script
        self.owner = owner
        self.variables: dict[str, str | DgReference] = dict()
    
    def __bool__(self):
        return bool(self.owner)