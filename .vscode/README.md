# VS Code Workspace Configuration

This workspace is configured with:

## Python Environment
- Virtual environment: `.venv/`
- Python interpreter: `.venv/bin/python`
- Automatic activation in terminals

## Build System
- Debug build: `Ctrl+Shift+P` → "Tasks: Run Task" → "Compile Debug"
- Release build: `Ctrl+Shift+P` → "Tasks: Run Task" → "Compile Release"
- Default build task: `Ctrl+Shift+B` (runs debug build)
- Clean build: `Ctrl+Shift+P` → "Tasks: Run Task" → "Clean Build"

## C++ Configuration
- Standard: C++23
- Include paths: `include/` and build dependencies
- Compile commands: `build/compile_commands.json`

## Usage for GitHub Copilot
When GitHub Copilot encounters this workspace in future chats, it will automatically:
1. Use the Python virtual environment (`.venv/`)
2. Use the compile script (`./compile.sh debug` or `./compile.sh release`)
3. Understand the C++ project structure and build system
4. Default to the correct working directory (project root)

## Opening the Workspace
To ensure all settings are applied correctly, open VS Code with:
```bash
code dbat.code-workspace
```
or use "File" → "Open Workspace from File..." in VS Code.
