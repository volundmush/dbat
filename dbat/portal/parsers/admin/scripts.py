import dbat
from ..base import BaseParser, StringEditorParser
from dbat.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from dbat.utils import partial_match

from dbat.models.scripts import MobTriggerType, ObjectTriggerType, RoomTriggerType, TrigProtoData
from dbat.models.scripts import DgMobScript, DgRoomScript, DgObjectScript

class ScriptEditorParser(BaseParser):
    """
    Parser for editing individual DgScript triggers.
    
    Provides commands for editing script properties and launching
    the string editor for script code.
    """
    
    def __init__(self, script: DgMobScript | DgRoomScript | DgObjectScript):
        super().__init__()
        self.model = script

    async def on_start(self):
        await self.handle_look()

    async def handle_look(self):
        """Display current script information."""
        script = self.model
        script_type_name = script.attach_type.name.capitalize() if hasattr(script, 'attach_type') else "Script"
        await self.send_line(f"=== {script_type_name} Script Editor ===")
        await self.send_line(f"VNum:        #{script.vn}")
        await self.send_line(f"Name:        {script.name or '(unnamed)'}")
        await self.send_line(f"Triggers:    {', '.join(script.get_trigger_names())}")
        await self.send_line(f"Argument:    {script.arglist or '(none)'}")
        await self.send_line(f"Narg:          {script.narg}")

        if script.cmdlist:
            await self.send_line("Script Text:")
            for line in script.cmdlist:
                await self.send_line(f"{line}")
        
        table = self.make_table("Command", "Description", title="Script Commands")
        table.add_row("vnum <number>", "Change the vnum of the script to save as. Don't accidentally save over a different script!")
        table.add_row("name <name>", "Set the script name")
        table.add_row("arg <text>", "Set the trigger argument")
        table.add_row("narg <number>", "Set the numeric arguments for this script.")
        table.add_row("triggers <list>", "Set trigger types (space-separated)")
        table.add_row("code", "Edit the script code (launches line editor)")
        table.add_row("save", "Save the script.")
        table.add_row("exit", "Exit the script editor.")
        await self.send_rich(table)

    async def handle_command(self, event: str):
        """Handle script editing commands."""
        if not event.strip():
            return

        matched = CMD_MATCH.match(event)
        if not matched:
            await self.send_line("Invalid command. Type a command or 'exit' to quit.")
            return

        match_dict = {k: v for k, v in matched.groupdict().items() if v is not None}
        cmd = match_dict.get("cmd", "").lower()
        args = match_dict.get("args", "").strip()

        if cmd in ("look", "l", ""):
            await self.handle_look()
        elif cmd == "vnum":
            await self.cmd_vnum(args)
        elif cmd == "name":
            await self.cmd_name(args)
        elif cmd in ("arg", "argument"):
            await self.cmd_arg(args)
        elif cmd == "narg":
            await self.cmd_narg(args)
        elif cmd in ("triggers", "trigger"):
            await self.cmd_triggers(args)
        elif cmd == "code":
            await self.cmd_code()
        elif cmd == "save":
            await self.cmd_save()
        elif cmd in ("exit", "quit"):
            await self.handle_exit()
        else:
            await self.send_line(f"Unknown command: {cmd}")
            await self.send_line("Type 'look' for help.")

    async def cmd_vnum(self, args: str):
        """Change the script's vnum."""
        if not args:
            await self.send_line("Usage: vnum <number>")
            return

        try:
            new_vnum = int(args)
        except ValueError:
            await self.send_line("Invalid vnum. Must be a number.")
            return

        if new_vnum < 0:
            await self.send_line("VNum must be non-negative.")
            return

        old_vnum = self.model.vn
        self.model.vn = new_vnum
        await self.send_line(f"VNum changed from {old_vnum} to {new_vnum}.")

    async def cmd_name(self, args: str):
        """Set the script name."""
        if not args:
            await self.send_line("Usage: name <name>")
            return

        old_name = self.model.name
        self.model.name = args
        await self.send_line(f"Name changed from '{old_name}' to '{args}'.")

    async def cmd_arg(self, args: str):
        """Set the trigger argument."""
        old_arg = self.model.arglist
        self.model.arglist = args
        if args:
            await self.send_line(f"Argument changed from '{old_arg}' to '{args}'.")
        else:
            await self.send_line(f"Argument cleared (was '{old_arg}').")

    async def cmd_narg(self, args: str):
        """Set the numeric argument."""
        if not args:
            await self.send_line("Usage: narg <number>")
            return

        try:
            new_narg = int(args)
        except ValueError:
            await self.send_line("Invalid narg. Must be a number.")
            return

        old_narg = self.model.narg
        self.model.narg = new_narg
        await self.send_line(f"Narg changed from {old_narg} to {new_narg}.")

    async def cmd_triggers(self, args: str):
        """Set trigger types."""
        if not args:
            # Show available triggers
            available = self.model.available_trigger_types()
            await self.send_line("Available trigger types:")
            for trigger in available:
                enabled = "✓" if (self.model.trigger_type & getattr(type(self.model.trigger_type), trigger)) else " "
                await self.send_line(f"  {enabled} {trigger}")
            await self.send_line("\nUsage: triggers <list of trigger names>")
            await self.send_line("Example: triggers GREET SPEECH COMMAND")
            return

        # Parse trigger names
        trigger_names = args.upper().split()
        available = self.model.available_trigger_types()
        trigger_type_class = type(self.model.trigger_type)
        
        new_trigger_value = 0
        invalid_triggers = []

        for name in trigger_names:
            if name in available:
                flag = getattr(trigger_type_class, name)
                new_trigger_value |= flag
            else:
                invalid_triggers.append(name)

        if invalid_triggers:
            await self.send_line(f"Invalid trigger types: {', '.join(invalid_triggers)}")
            await self.send_line(f"Available types: {', '.join(available)}")
            return

        old_triggers = ', '.join(self.model.get_trigger_names())
        self.model.trigger_type = trigger_type_class(new_trigger_value)
        new_triggers = ', '.join(self.model.get_trigger_names())
        
        await self.send_line(f"Triggers changed from '{old_triggers}' to '{new_triggers}'.")

    async def cmd_code(self):
        """Launch the string editor for script code."""
        current_code = '\n'.join(self.model.cmdlist) if self.model.cmdlist else ""
        
        async def on_save(new_code: str):
            # Split code into lines and store
            self.model.cmdlist = new_code.splitlines() if new_code.strip() else []
            await self.send_line("Script code updated.")
            await self.handle_look()

        async def on_abort():
            await self.send_line("Script code editing aborted.")
            await self.handle_look()

        editor = StringEditorParser(
            name=f"Script #{self.model.vn} Code",
            data=current_code,
            on_save=on_save,
            on_abort=on_abort
        )
        
        await self.send_line("Launching string editor...")
        await self.connection.push_parser(editor)

    async def cmd_save(self):
        """Save the script."""
        try:
            # Validate the script
            self.model.check_values()
            
            # TODO: Actually save to database/API
            # For now, just confirm the save operation
            await self.send_line(f"Script #{self.model.vn} '{self.model.name}' saved successfully.")
            await self.send_line("(Note: Actual database saving not yet implemented)")
            
        except ValueError as e:
            await self.send_line(f"Cannot save script: {e}")
        except Exception as e:
            await self.send_line(f"Error saving script: {e}")


class ScriptListParser(BaseParser):
    """
    Parser for managing a list of DgScript triggers.
    
    Provides commands for creating, editing, and deleting scripts,
    plus listing existing scripts with filtering options.
    """
    
    def __init__(self):
        super().__init__()
        self.scripts = {}  # vnum -> script mapping
        self.script_type = None  # 'mob', 'obj', or 'room'

    async def on_start(self):
        await self.select_script_type()

    async def select_script_type(self):
        """Prompt for script type selection."""
        await self.send_line("=== DgScript Manager ===")
        await self.send_line("Select script type:")
        await self.send_line("1) Mobile (Mob) scripts")
        await self.send_line("2) Object scripts") 
        await self.send_line("3) Room scripts")
        await self.send_line("")
        await self.send_line("Enter choice (1-3) or 'quit' to exit:")

    async def handle_command(self, event: str):
        """Handle script management commands."""
        if not event.strip():
            return
        
        command = event.strip().lower()
        
        # Handle script type selection
        if self.script_type is None:
            if command == 'quit':
                await self.handle_exit()
                return
            
            if command in ('1', 'mob', 'mobile'):
                self.script_type = 'mob'
                await self.send_line("Selected: Mobile scripts")
                await self.load_scripts()
                await self.show_menu()
            elif command in ('2', 'obj', 'object'):
                self.script_type = 'obj'
                await self.send_line("Selected: Object scripts")
                await self.load_scripts()
                await self.show_menu()
            elif command in ('3', 'room'):
                self.script_type = 'room'
                await self.send_line("Selected: Room scripts")
                await self.load_scripts()
                await self.show_menu()
            else:
                await self.send_line("Invalid choice. Enter 1-3 or 'quit'.")
            return
        
        # Handle main commands
        parts = command.split(maxsplit=1)
        cmd = parts[0]
        args = parts[1] if len(parts) > 1 else ""
        
        if cmd in ('list', 'show', 'ls'):
            await self.cmd_list(args)
        elif cmd in ('create', 'new', 'add'):
            await self.cmd_create(args)
        elif cmd in ('edit', 'modify'):
            await self.cmd_edit(args)
        elif cmd in ('delete', 'remove', 'del'):
            await self.cmd_delete(args)
        elif cmd in ('copy', 'clone'):
            await self.cmd_copy(args)
        elif cmd in ('search', 'find'):
            await self.cmd_search(args)
        elif cmd in ('menu', 'help', '?'):
            await self.show_menu()
        elif cmd in ('type', 'switch'):
            await self.cmd_switch_type()
        elif cmd in ('quit', 'exit'):
            await self.handle_exit()
        else:
            await self.send_line(f"Unknown command: {cmd}")
            await self.show_menu()

    async def show_menu(self):
        """Display the main command menu."""
        script_type_name = self.script_type.capitalize()
        await self.send_line(f"\n=== {script_type_name} Script Manager ===")
        await self.send_line("Commands:")
        await self.send_line("  list [pattern]     - List scripts (optionally filtered)")
        await self.send_line("  create <vnum>      - Create new script")
        await self.send_line("  edit <vnum>        - Edit existing script")
        await self.send_line("  delete <vnum>      - Delete script")
        await self.send_line("  copy <from> <to>   - Copy script to new vnum")
        await self.send_line("  search <text>      - Search script content")
        await self.send_line("  type               - Switch script type")
        await self.send_line("  quit               - Exit manager")
        await self.send_line("")

    async def load_scripts(self):
        """Load scripts from the database/API."""
        # TODO: Implement actual loading from database
        # For now, create some sample scripts
        if self.script_type == 'mob':
            self.scripts = {
                1001: DgMobScript(vn=1001, name="Guard Greeting", arglist="hello hi", 
                                cmdlist=["say Hello there, traveler!", "smile %actor.name%"]),
                1002: DgMobScript(vn=1002, name="Merchant Banter", arglist="", 
                                cmdlist=["say Welcome to my shop!", "emote polishes his wares"])
            }
        elif self.script_type == 'obj':
            self.scripts = {
                2001: DgObjectScript(vn=2001, name="Magic Sword", arglist="", 
                                   cmdlist=["say The sword glows with power!", "emote hums with energy"]),
            }
        elif self.script_type == 'room':
            self.scripts = {
                3001: DgRoomScript(vn=3001, name="Scary Room", arglist="", 
                                 cmdlist=["echo A chill runs down your spine.", "echo The shadows seem to move..."])
            }
        
        await self.send_line(f"Loaded {len(self.scripts)} {self.script_type} scripts.")

    async def cmd_list(self, pattern: str = ""):
        """List scripts, optionally filtered by pattern."""
        if not self.scripts:
            await self.send_line(f"No {self.script_type} scripts found.")
            return
        
        await self.send_line(f"\n=== {self.script_type.capitalize()} Scripts ===")
        await self.send_line("VNum  Name                 Triggers")
        await self.send_line("----  -------------------  --------")
        
        for vnum, script in sorted(self.scripts.items()):
            name = script.name or "(unnamed)"
            if len(name) > 19:
                name = name[:16] + "..."
            
            triggers = ', '.join(script.get_trigger_names())
            if len(triggers) > 20:
                triggers = triggers[:17] + "..."
            
            # Apply pattern filter if provided
            if pattern and pattern.lower() not in name.lower() and pattern.lower() not in triggers.lower():
                continue
            
            await self.send_line(f"{vnum:4d}  {name:19s}  {triggers}")

    async def cmd_create(self, vnum_str: str):
        """Create a new script."""
        if not vnum_str:
            await self.send_line("Usage: create <vnum>")
            return
        
        try:
            vnum = int(vnum_str)
        except ValueError:
            await self.send_line("Invalid vnum. Must be a number.")
            return
        
        if vnum in self.scripts:
            await self.send_line(f"Script #{vnum} already exists. Use 'edit {vnum}' to modify it.")
            return
        
        # Create new script based on type
        if self.script_type == 'mob':
            script = DgMobScript(vn=vnum, name=f"Mobile Script {vnum}", 
                               arglist="", cmdlist=["* New mobile script"])
        elif self.script_type == 'obj':
            script = DgObjectScript(vn=vnum, name=f"Object Script {vnum}", 
                                  arglist="", cmdlist=["* New object script"])
        elif self.script_type == 'room':
            script = DgRoomScript(vn=vnum, name=f"Room Script {vnum}", 
                                arglist="", cmdlist=["* New room script"])
        
        self.scripts[vnum] = script
        await self.send_line(f"Created new {self.script_type} script #{vnum}.")
        
        # Automatically open the editor
        await self.cmd_edit(vnum_str)

    async def cmd_edit(self, vnum_str: str):
        """Edit an existing script."""
        if not vnum_str:
            await self.send_line("Usage: edit <vnum>")
            return
        
        try:
            vnum = int(vnum_str)
        except ValueError:
            await self.send_line("Invalid vnum. Must be a number.")
            return
        
        if vnum not in self.scripts:
            await self.send_line(f"Script #{vnum} not found.")
            await self.send_line(f"Use 'create {vnum}' to create it first.")
            return
        
        script = self.scripts[vnum]
        editor = ScriptEditorParser(script)
        
        await self.send_line(f"Opening script #{vnum} for editing...")
        await self.connection.push_parser(editor)

    async def cmd_delete(self, vnum_str: str):
        """Delete a script."""
        if not vnum_str:
            await self.send_line("Usage: delete <vnum>")
            return
        
        try:
            vnum = int(vnum_str)
        except ValueError:
            await self.send_line("Invalid vnum. Must be a number.")
            return
        
        if vnum not in self.scripts:
            await self.send_line(f"Script #{vnum} not found.")
            return
        
        script = self.scripts[vnum]
        await self.send_line(f"Delete script #{vnum} '{script.name}'? (yes/no)")
        
        # TODO: Implement confirmation handling
        # For now, just delete immediately
        del self.scripts[vnum]
        await self.send_line(f"Script #{vnum} deleted.")

    async def cmd_copy(self, args: str):
        """Copy a script to a new vnum."""
        parts = args.split()
        if len(parts) != 2:
            await self.send_line("Usage: copy <from_vnum> <to_vnum>")
            return
        
        try:
            from_vnum = int(parts[0])
            to_vnum = int(parts[1])
        except ValueError:
            await self.send_line("Invalid vnums. Must be numbers.")
            return
        
        if from_vnum not in self.scripts:
            await self.send_line(f"Source script #{from_vnum} not found.")
            return
        
        if to_vnum in self.scripts:
            await self.send_line(f"Destination vnum #{to_vnum} already exists.")
            return
        
        # Create a copy of the script
        original = self.scripts[from_vnum]
        if self.script_type == 'mob':
            copy = DgMobScript(vn=to_vnum, name=f"Copy of {original.name}", 
                             arglist=original.arglist, cmdlist=original.cmdlist.copy())
        elif self.script_type == 'obj':
            copy = DgObjectScript(vn=to_vnum, name=f"Copy of {original.name}", 
                                arglist=original.arglist, cmdlist=original.cmdlist.copy())
        elif self.script_type == 'room':
            copy = DgRoomScript(vn=to_vnum, name=f"Copy of {original.name}", 
                              arglist=original.arglist, cmdlist=original.cmdlist.copy())
        
        # Copy trigger flags
        copy.trigger_type = original.trigger_type
        
        self.scripts[to_vnum] = copy
        await self.send_line(f"Copied script #{from_vnum} to #{to_vnum}.")

    async def cmd_search(self, pattern: str):
        """Search script content for a pattern."""
        if not pattern:
            await self.send_line("Usage: search <text>")
            return
        
        pattern = pattern.lower()
        matches = []
        
        for vnum, script in self.scripts.items():
            # Search in name
            if script.name and pattern in script.name.lower():
                matches.append((vnum, f"Name: {script.name}"))
            
            # Search in argument
            if script.arglist and pattern in script.arglist.lower():
                matches.append((vnum, f"Argument: {script.arglist}"))
            
            # Search in script code
            for i, line in enumerate(script.cmdlist):
                if pattern in line.lower():
                    matches.append((vnum, f"Line {i+1}: {line}"))
        
        if not matches:
            await self.send_line(f"No scripts found containing '{pattern}'.")
            return
        
        await self.send_line(f"\nSearch results for '{pattern}':")
        current_vnum = None
        for vnum, match in matches:
            if vnum != current_vnum:
                await self.send_line(f"\nScript #{vnum}:")
                current_vnum = vnum
            await self.send_line(f"  {match}")

    async def cmd_switch_type(self):
        """Switch to a different script type."""
        self.script_type = None
        self.scripts = {}
        await self.select_script_type()

class ScriptListParser(BaseParser):
    
    async def on_start(self):
        await self.handle_look()
    
    async def handle_help(self, args: str, user = None):
        if user is None:
            user = await self.get_user()
        help_table = self.make_table("Command", "Description", title="User Commands")
        help_table.add_row("search <text>", "Searches for scripts with name containing <text>.")
        help_table.add_row("list <min>[=<max>]", "Lists scripts with vnums in the range. If no max, it's min+100.")
        help_table.add_row("view <vnum>", "Displays the script with the given vnum.")
        help_table.add_row("create <type>", "Create a <mob|room|object> new script.")
        help_table.add_row("edit <vnum>", "Edit the script with the given vnum.")
        if user.admin_level > 0:
            help_table.add_row("delete <vnum>=YES", "Deletes a script.")
        help_table.add_row("exit", "Exits back to the admin menu.")
        await self.send_rich(help_table)
    
    async def handle_look(self, user = None):
        if user is None:
            user = await self.get_user()

        await self.handle_help("", user)
    
    async def handle_search(self, args: str, user = None):
        if user is None:
            user = await self.get_user()
        if not args:
            await self.send_line("You must supply a search term.")
            return
        # todo: finish
    
    async def handle_list(self, lsargs: str = None, rsargs: str = None, user = None):
        if user is None:
            user = await self.get_user()
        if not lsargs:
            await self.send_line("You must supply a minimum vnum.")
            return
        
        try:
            min_vnum = int(lsargs)
            max_vnum = int(rsargs) if rsargs else None
        except ValueError:
            await self.send_line("Invalid vnum range format. Use <min>[=<max>].")
            return
        
        if max_vnum is None:
            max_vnum = min_vnum + 100
        
        if min_vnum > max_vnum:
            await self.send_line("Minimum vnum cannot be greater than maximum vnum.")
            return
        
        # Prevent excessive loads; no more than 1000 scripts at once.
        if max_vnum - min_vnum > 1000:
            await self.send_line("Too many scripts in this range. Please narrow it down.")
            return
        # todo: finish

    async def handle_view(self, args: str, user = None):
        if user is None:
            user = await self.get_user()
        if not args:
            await self.send_line("You must supply a vnum to view.")
            return
        try:
            vnum = int(args)
        except ValueError:
            await self.send_line("Invalid vnum format. Use an integer.")
            return
        
        script_data = await self.api_call("GET", f"/scripts/{vnum}")
        if not script_data:
            await self.send_line(f"No script found with vnum {vnum}.")
            return
        
        data = TrigProtoData(**script_data)
        script = data.to_specific_script()
        # todo: finish

    async def handle_create(self, args: str, user = None):
        if user is None:
            user = await self.get_user()
        if not args:
            await self.send_line("You must specify the type of script to create (mob, room, object).")
            return

        choices = {
            "mob": MobTriggerType,
            "room": RoomTriggerType,
            "object": ObjectTriggerType
        }
        if not (choice := partial_match(args, choices.keys())):
            await self.send_line("Invalid script type. Use 'mob', 'room', or 'object'.")
            return
        script_type = choices[choice]

        new_script = script_type(name=f"New {choice} Script", vn=-1)
        # todo: finish
    
    async def handle_edit(self, args: str, user = None):
        if user is None:
            user = await self.get_user()
        if not args:
            await self.send_line("You must supply a vnum to edit.")
            return
        try:
            vnum = int(args)
        except ValueError:
            await self.send_line("Invalid vnum format. Use an integer.")
            return
        
        script_data = await self.api_call("GET", f"/scripts/{vnum}")
        if not script_data:
            await self.send_line(f"No script found with vnum {vnum}.")
            return
        
        data = TrigProtoData(**script_data)
        script = data.to_specific_script()

    async def handle_command(self, event: str, user = None):
        if user is None:
            user = await self.get_user()
        matched = CMD_MATCH.match(event)
        if not matched:
            await self.send_line("Invalid command. Type 'help' for help.")
            return
        match_dict = {k: v for k, v in matched.groupdict().items() if v is not None}
        cmd = match_dict.get("cmd", "")
        args = match_dict.get("args", "")
        lsargs = match_dict.get("lsargs", "")
        rsargs = match_dict.get("rsargs", "")
        match cmd.lower():
            case "help":
                await self.handle_help(args, user)
            case "look":
                await self.handle_look()
            case "search":
                await self.handle_search(args, user)
            case "list":
                await self.handle_list(lsargs, rsargs, user)
            case "view":
                await self.handle_view(args, user)
            case "create":
                await self.handle_create(args, user)
            case "edit":
                await self.handle_edit(args, user)
            case "delete":
                await self.handle_delete(args, user)
            case "exit":
                await self.handle_exit()
            case _:
                await self.send_line("Invalid command. Type 'help' for help.")
