import dbat
from ..base import BaseParser, StringEditorParser
from dbat.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from dbat.utils import partial_match

from dbat.bridge.models.scripts import MobTriggerType, ObjectTriggerType, RoomTriggerType, TrigProtoData
from dbat.bridge.models.scripts import DgMobScript, DgRoomScript, DgObjectScript

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
        table.add_row("save", "Save the script. Be careful not to accidentally overwrite existing scripts!")
        table.add_row("exit", "Exit the script editor. Remember to save first!")
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

        available_commands = ["look", "vnum", "name", "arg", "narg", "triggers", "code", "save", "exit"]

        cmd = partial_match(cmd, available_commands)

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
            if (found := partial_match(name, available)):
                flag = getattr(trigger_type_class, found)
                # if set, unset. if unset, set.
                if self.model.trigger_type & flag:
                    new_trigger_value &= ~flag
                else:
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

    async def on_start(self):
        await self.handle_look()

    async def handle_look(self, user=None):
        if user is None:
            user = await self.get_user()
        table = self.make_table("Command", "Description", title="Script Management Commands")
        table.add_row("search <txt>", "Search for scripts where name contains txt.")
        table.add_row("list <min>[=<max>]", "List scripts with vnums in the range. If no max, it's min+100.")
        table.add_row("view <vnum>", "Display the script with the given vnum.")
        table.add_row("create <type>", "Create a <mob|room|object> new script. You'll select vnum in the editor.")
        table.add_row("edit <vnum>", "Edit an existing script by vnum.")
        table.add_row("delete <vnum>", "Delete a script by vnum.")
        table.add_row("exit", "Exit the script manager.")
        await self.send_rich(table)

    async def handle_command(self, event: str):
        """Handle script management commands."""
        if not event.strip():
            return

        matched = CMD_MATCH.match(event)
        if not matched:
            await self.send_line("Invalid command. Type a command or 'exit' to quit.")
            return

        match_dict = {k: v for k, v in matched.groupdict().items() if v is not None}
        raw_cmd = match_dict.get("cmd", "").lower()
        args = match_dict.get("args", "").strip()
        lsargs = match_dict.get("lsargs", "").strip()
        rsargs = match_dict.get("rsargs", "").strip()

        user = await self.get_user()

        if not (cmd := partial_match(raw_cmd, ["search", "list", "view", "create", "edit", "delete", "exit"])):
            await self.send_line("Unknown command.")
            await self.handle_look(user)
            return

        match cmd:
            case "search":
                await self.cmd_search(args, user)
            case "list":
                await self.cmd_list(lsargs, rsargs, user)
            case "view":
                await self.cmd_view(args, user)
            case "create":
                await self.cmd_create(args, user)
            case "edit":
                await self.cmd_edit(args, user)
            case "delete":
                await self.cmd_delete(args, user)
            case "exit":
                await self.handle_exit()
            case _:
                # shouldn't be possible given partial_match but...
                await self.send_line(f"Unknown command: {cmd}")
                await self.handle_look(user)


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
        
        editor = ScriptEditorParser(new_script)
        await self.connection.push_parser(editor)
    
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

        editor = ScriptEditorParser(script)
        await self.connection.push_parser(editor)

    async def handle_command(self, event: str, user = None):
        """Handle script management commands."""
        if not event.strip():
            return

        matched = CMD_MATCH.match(event)
        if not matched:
            await self.send_line("Invalid command. Type a command or 'exit' to quit.")
            return

        match_dict = {k: v for k, v in matched.groupdict().items() if v is not None}
        raw_cmd = match_dict.get("cmd", "").lower()
        args = match_dict.get("args", "").strip()
        lsargs = match_dict.get("lsargs", "").strip()
        rsargs = match_dict.get("rsargs", "").strip()

        user = await self.get_user()

        if not (cmd := partial_match(raw_cmd, ["search", "list", "view", "create", "edit", "delete", "exit"])):
            await self.send_line("Unknown command.")
            await self.handle_look(user)
            return

        match cmd:
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
                # shouldn't be possible given partial_match but...
                await self.send_line(f"Unknown command: {cmd}")
                await self.handle_look(user)
