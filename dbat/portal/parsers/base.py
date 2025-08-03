import re
import shlex
from typing import List, Optional, Callable, Awaitable, Union
from dbat.models.game import AccountData


class BaseParser:

    def __init__(self):
        self.connection: "BaseConnection" = None
        self.index: int = 0
    
    async def get_user(self) -> AccountData:
        user_name = self.connection.payload.get("sub")
        user_data = await self.api_call("GET", f"/users/name/{user_name}")
        return AccountData(**user_data)

    async def handle_exit(self):
        await self.connection.pop_parser()

    async def on_start(self):
        pass

    async def on_end(self):
        pass

    async def on_resume(self):
        await self.on_start()

    async def handle_command(self, event: str):
        pass

    async def send_text(self, text: str):
        await self.connection.send_text(text)

    async def send_line(self, text: str):
        await self.connection.send_line(text)
    
    async def send_circle(self, text: str):
        await self.connection.send_circle(text)

    async def send_rich(self, *args, **kwargs):
        await self.connection.send_rich(*args, **kwargs)

    async def send_gmcp(self, command: str, data: dict):
        await self.connection.send_gmcp(command, data)

    async def api_call(self, *args, **kwargs):
        return await self.connection.api_call(*args, **kwargs)

    async def api_stream(self, *args, **kwargs):
        return await self.connection.api_stream(*args, **kwargs)

    def make_table(self, *args, **kwargs):
        return self.connection.make_table(*args, **kwargs)


class StringEditorParser(BaseParser):
    """
    A MUD-style line editor for editing multi-line text content.
    
    Provides classic MUD text editing commands like insert, delete, replace, format, etc.
    This is a Python replacement for the traditional C++ improved-edit functionality.
    """
    
    def __init__(
        self, 
        name: str, 
        data: str, 
        on_save: Callable[[str], Awaitable[None]], 
        on_abort: Callable[[], Awaitable[None]], 
        formatter: Optional[Callable[[str], Awaitable[List[str]]]] = None
    ):
        """
        Initialize the string editor.
        
        Args:
            name: Name/title of what is being edited
            data: Initial text content
            on_save: Async callback when text is saved, receives final text
            on_abort: Async callback when edit is aborted
            formatter: Optional async formatter that takes text and returns formatted lines
        """
        super().__init__()
        self.name: str = name
        self.data: str = data
        self.lines: List[str] = data.splitlines() if data else []
        self.on_save: Callable[[str], Awaitable[None]] = on_save
        self.on_abort: Callable[[], Awaitable[None]] = on_abort
        self.formatter: Optional[Callable[[str], Awaitable[List[str]]]] = formatter
    
    async def on_start(self) -> None:
        """Called when the editor starts."""
        await self.send_line(f"=== String Editor: {self.name} ===")
        await self.send_line("Type '/h' for help, '/s' to save, '/a' to abort.")
        await self.send_line("Enter text or editor commands (commands start with '/'):")
        if self.lines:
            await self.cmd_list_numbered()
        await self.send_line("> ")
    
    async def handle_command(self, event: str) -> None:
        """
        Handle user input - either editor commands or new text lines.
        
        Args:
            event: User input string
        """           
        # Check if it's an editor command (starts with /)
        if event.startswith('/'):
            await self._handle_editor_command(event[1:])
        else:
            # Add new line to the end
            self.lines.append(event)
            await self.send_line("> ")
    
    async def _handle_editor_command(self, command: str) -> None:
        """
        Process editor commands.
        
        Args:
            command: The command string without the leading '/'
        """
        if not command:
            await self.send_line("Invalid command. Type '/h' for help.")
            await self.send_line("> ")
            return
        
        cmd_char = command[0].lower()
        args = command[1:].strip()
        
        try:
            if cmd_char == 'a':
                await self.cmd_abort()
            elif cmd_char == 'c':
                await self.cmd_clear()
            elif cmd_char == 'd':
                await self.cmd_delete(args)
            elif cmd_char == 'e':
                await self.cmd_edit(args)
            elif cmd_char == 'f':
                await self.cmd_format(args)
            elif cmd_char == 'h':
                await self.cmd_help()
            elif cmd_char == 'i':
                await self.cmd_insert(args)
            elif cmd_char == 'l':
                await self.cmd_list()
            elif cmd_char == 'n':
                await self.cmd_list_numbered()
            elif cmd_char == 'r':
                await self.cmd_replace(args)
            elif cmd_char == 's':
                await self.cmd_save()
            else:
                await self.send_line(f"Unknown command: /{cmd_char}")
                await self.send_line("Type '/h' for help.")
        except Exception as e:
            await self.send_line(f"Error executing command: {e}")
    
    async def cmd_abort(self) -> None:
        """Abort editing without saving."""
        await self.send_line("Aborted.")
        await self.on_abort()
        await self.handle_exit()
    
    async def cmd_clear(self) -> None:
        """Clear the entire buffer."""
        self.lines.clear()
        await self.send_line("Buffer cleared.")
    
    async def cmd_delete(self, args: str) -> None:
        """
        Delete a line or range of lines.
        
        Args:
            args: Line number or range (e.g., "5", "2-4")
        """
        if not args:
            await self.send_line("Usage: /d# (delete line #)")
            return
        
        try:
            if '-' in args:
                # Range deletion
                start_str, end_str = args.split('-', 1)
                start = int(start_str.strip()) - 1
                end = int(end_str.strip()) - 1
                
                if start < 0 or end >= len(self.lines) or start > end:
                    await self.send_line("Invalid line range.")
                    return
                
                # Delete from end to start to maintain indices
                for i in range(end, start - 1, -1):
                    del self.lines[i]
                
                await self.send_line(f"Deleted lines {start + 1}-{end + 1}.")
            else:
                # Single line deletion
                line_num = int(args) - 1
                
                if line_num < 0 or line_num >= len(self.lines):
                    await self.send_line("Invalid line number.")
                    return
                
                del self.lines[line_num]
                await self.send_line(f"Deleted line {line_num + 1}.")
                
        except ValueError:
            await self.send_line("Invalid line number format.")
    
    async def cmd_edit(self, args: str) -> None:
        """
        Replace the content of a specific line.
        
        Args:
            args: Line number followed by new text (e.g., "3 new content")
        """
        if not args:
            await self.send_line("Usage: /e# <text> (edit line # with <text>)")
            return
        
        try:
            parts = args.split(' ', 1)
            line_num = int(parts[0]) - 1
            
            if line_num < 0 or line_num >= len(self.lines):
                await self.send_line("Invalid line number.")
                return
            
            new_text = parts[1] if len(parts) > 1 else ""
            old_text = self.lines[line_num]
            self.lines[line_num] = new_text
            
            await self.send_line(f"Line {line_num + 1} changed from:")
            await self.send_line(f"  '{old_text}'")
            await self.send_line(f"to:")
            await self.send_line(f"  '{new_text}'")
            
        except (ValueError, IndexError):
            await self.send_line("Invalid format. Usage: /e# <text>")
    
    async def cmd_format(self, args: str) -> None:
        """
        Format text using the provided formatter.
        
        Args:
            args: Optional line number or range for targeted formatting
        """
        if not self.formatter:
            await self.send_line("No formatter available.")
            return
        
        try:
            if args.startswith('i'):
                # Indented formatting
                await self._format_indented(args[1:].strip())
            elif args:
                # Specific line(s)
                await self._format_lines(args)
            else:
                # Format entire buffer
                text = '\n'.join(self.lines)
                formatted_lines = await self.formatter(text)
                self.lines = formatted_lines
                await self.send_line("Buffer formatted.")
        except Exception as e:
            await self.send_line(f"Formatting error: {e}")
    
    async def _format_indented(self, args: str) -> None:
        """Handle indented formatting."""
        if not args:
            # Format entire buffer with indentation
            text = '\n'.join(self.lines)
            formatted_lines = await self.formatter(text)
            # Add basic indentation (this would be customized based on content type)
            self.lines = [f"    {line}" if line.strip() else line for line in formatted_lines]
            await self.send_line("Buffer formatted with indentation.")
        else:
            await self._format_lines(args, indent=True)
    
    async def _format_lines(self, args: str, indent: bool = False) -> None:
        """Format specific line(s)."""
        try:
            if '-' in args:
                # Range formatting
                start_str, end_str = args.split('-', 1)
                start = int(start_str.strip()) - 1
                end = int(end_str.strip()) - 1
            else:
                # Single line
                start = end = int(args) - 1
            
            if start < 0 or end >= len(self.lines) or start > end:
                await self.send_line("Invalid line range.")
                return
            
            # Extract lines to format
            lines_to_format = self.lines[start:end + 1]
            text = '\n'.join(lines_to_format)
            formatted_lines = await self.formatter(text)
            
            if indent:
                formatted_lines = [f"    {line}" if line.strip() else line for line in formatted_lines]
            
            # Replace the lines
            self.lines[start:end + 1] = formatted_lines
            
            if start == end:
                await self.send_line(f"Line {start + 1} formatted.")
            else:
                await self.send_line(f"Lines {start + 1}-{end + 1} formatted.")
                
        except ValueError:
            await self.send_line("Invalid line number format.")
    
    async def cmd_help(self) -> None:
        """Display help for editor commands."""
        help_text = [
            "=== String Editor Commands ===",
            "/a         - aborts editor",
            "/c         - clears buffer", 
            "/d#        - deletes a line #",
            "/e# <text> - changes the line at # with <text>",
            "/f         - formats text",
            "/fi        - indented formatting of text", 
            "/fi#       - indented formatting on a specific line",
            "/fi #-#    - indented formatting on specific lines",
            "/h         - list text editor commands",
            "/i# <text> - inserts <text> before line #",
            "/l         - lists buffer",
            "/n         - lists buffer with line numbers",
            "/r 'a' 'b' - replace 1st occurrence of text <a> in buffer with text <b>",
            "/ra 'a' 'b'- replace all occurrences of text <a> within buffer with text <b>",
            "           usage: /r[a] 'pattern' 'replacement'",
            "/s         - saves text",
            "",
            "To add lines, simply type them (without a leading /)."
        ]
        
        for line in help_text:
            await self.send_line(line)
    
    async def cmd_insert(self, args: str) -> None:
        """
        Insert text before a specific line.
        
        Args:
            args: Line number followed by text to insert
        """
        if not args:
            await self.send_line("Usage: /i# <text> (insert <text> before line #)")
            return
        
        try:
            parts = args.split(' ', 1)
            line_num = int(parts[0]) - 1
            
            if line_num < 0 or line_num > len(self.lines):
                await self.send_line("Invalid line number.")
                return
            
            new_text = parts[1] if len(parts) > 1 else ""
            self.lines.insert(line_num, new_text)
            
            await self.send_line(f"Inserted at line {line_num + 1}: '{new_text}'")
            
        except (ValueError, IndexError):
            await self.send_line("Invalid format. Usage: /i# <text>")
    
    async def cmd_list(self) -> None:
        """List the buffer contents without line numbers."""
        if not self.lines:
            await self.send_line("(empty buffer)")
            return
        
        for line in self.lines:
            await self.send_line(line)
    
    async def cmd_list_numbered(self) -> None:
        """List the buffer contents with line numbers."""
        if not self.lines:
            await self.send_line("(empty buffer)")
            return
        
        for i, line in enumerate(self.lines, 1):
            await self.send_line(f"{i:3}: {line}")
    
    async def cmd_replace(self, args: str) -> None:
        """
        Replace text in the buffer.
        
        Args:
            args: Replace command arguments (e.g., "'old' 'new'" or "a 'old' 'new'")
        """
        if not args:
            await self.send_line("Usage: /r[a] 'pattern' 'replacement'")
            return
        
        # Check if it's replace all
        replace_all = args.startswith('a ')
        if replace_all:
            args = args[2:].strip()
        
        # Parse quoted arguments
        try:
            # Use shlex to properly handle quoted strings
            parts = shlex.split(args)
            
            if len(parts) != 2:
                await self.send_line("Usage: /r[a] 'pattern' 'replacement'")
                return
            
            pattern, replacement = parts
            
            if not pattern:
                await self.send_line("Empty search pattern.")
                return
            
            count = 0
            new_lines = []
            
            for line in self.lines:
                if replace_all:
                    # Replace all occurrences in this line
                    new_line = line.replace(pattern, replacement)
                    count += line.count(pattern)
                else:
                    # Replace only first occurrence in entire buffer
                    if pattern in line and count == 0:
                        new_line = line.replace(pattern, replacement, 1)
                        count = 1
                    else:
                        new_line = line
                new_lines.append(new_line)
            
            self.lines = new_lines
            
            if count == 0:
                await self.send_line(f"Pattern '{pattern}' not found.")
            elif replace_all:
                await self.send_line(f"Replaced {count} occurrence(s) of '{pattern}' with '{replacement}'.")
            else:
                await self.send_line(f"Replaced first occurrence of '{pattern}' with '{replacement}'.")
                
        except ValueError as e:
            await self.send_line(f"Error parsing arguments: {e}")
            await self.send_line("Usage: /r[a] 'pattern' 'replacement'")
    
    async def cmd_save(self) -> None:
        """Save the current buffer content."""
        content = '\r\n'.join(self.lines)
        await self.on_save(content)
        await self.handle_exit()
