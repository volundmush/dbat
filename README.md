# What is this?
This is the codebase for Dragon Ball Advent Truth, a MUD written atop of CircleMUD (with Goodies) 3.5 "Rasputin". Originally written in C, refactored with some C++, and now running in a horrific mishmash of Python and C++ in an attempt to modernize it.

# Can I run it?
Sort of. I have not yet included all of the game assets that it needs to run. This is just the code.

# Using the Sucker

This project is designed to be run on an Ubuntu 22.04 (or later) server, using GCC for the C++ compiler and CPython3.11 for running the Python. If you can get it to work in some other way, good on you, this is just what I know works.

The recommended IDE is VS Code due to the multi-language nature of the project. More on that later.

## Ubuntu Setup
The following apt packages are needed:

`sudo apt-get install cmake build-essential gdb python3.11 python3.11-dev python3-git python3-virtualenv git ninja-build`

Clone the directory.

`git clone https://github.com/volundmush/dbat`

(NOTE: if using VS Code, you can skip creating the venv here as VS Code will be able to create one instead.)

Create a virtualenv for the Python packages.

`python3.11 -m virtualenv venv`

Activate the venv.

`source venv/bin/activate`

Switch to folder.

`cd dbat`

Install requirements.

`pip install -r requirements.txt`

## Windows

### Getting WSL working (Windows Subsystem for Linux)
Though I've struggled to get this sucker working on Windows, it fights me at every turn.

Installing WSL2 varies slightly depending on your Windows version. There are numerous tutorials. Generally, you must ensure that your BIOS's virtualization technology is enabled (VT-d or AMD-v or whatever it's called for your CPU) which you can check in Task Manager's Performance tab. (Virtualization: enabled). The Windows Features "Virtual Machine Platform" and "Windows Subsystem for Linux" need to be enabled using "Turn Windows Features on or Off" you can access from control panel or windows search.

Open up a Command Prompt as admin (right click the shortcut/icon to access "run as admin"):

`wsl --install`

and

`wsl --update`

`wsl --set-default-version 2`

just to be sure everything is in order.

To install your Ubuntu distro,

`wsl --install -d Ubuntu`

Once it's installed, you can make it your default for any `wsl` commands.

`wsl --set-default Ubuntu`

Inside your WSL2 instance, you can now follow the Ubuntu instructions. Skip the venv stuff; it's better to let VS Code handle that.

### VS Code
Install the following Extensions:

`Remote Development`

`C/C++ Extension Pack`

`Python Extensionn Pack`

Restart VS Code if needed.

After Remote Development is installed, the toolbar on the left side has a "Remote Explorer" icon. Under that is a Dropdown which should have "WSL Targets" option. Select that from the dropdown and Ubuntu should be in thhe WSL Targets. You can use this to "connect" to the WSL2 instance. VS Code will install remote management tools to the Ubuntu instance so it can remote connect.

From there, open the `dbat` folder (or whatever else you cloned it to) as your project folder.

You will need to then go to Extensions again and tell VS Code to install your extensions in WSL.

Finally, after that, use the Command Palette (F5) and tell it to Python: Create Environment. it should detect Python3.11 of some variation as an option, and after that it should see requirements.txt. Tell it to use those. After this, VS Code will manage the Virtual Environment for you.

# Project Breakdown
The project relies on several major components to work properly.

The first is `libcirclemud.a` which is compiled from the .h and .cpp files in `include/` and `src/`.

The second is the `circlemud` Python Extension module compiled by Cython, which links to `libcirclemud.a`.

The third is `server.py`, which runs a specially configured Sanic server instance that operates the game and a SocketIO server.

Fourth is `portal.py`, which runs a Telnet listener and maps these to SocketIO sessions that then talk to the SocketIO server.

Although the provided `CMakeLists.txt` can compile `libcirclemud.a`, there is a `compile.py` which will automate building it and the Cython extension. So, use `./compile.py debug` (or `./compile.py release` if appropriate) or `python compile.py <arg>` to handle compiling.

A handy helper `launcher.py` is provided for running the game. you can use either `./launcher.py --help` or `python launcher.py --help` for more info.

In order for the game to work, the `lib` folder must be present in the project root. That is not (yet) included.

# Things to Watch Out For
In my experiences, WSL2 can get screwy sometimes, though that was working with JetBrains IDEs. I'm not sure about VS Code. Still, if the WSL instance freezes or starts acting bizarre, you can use `wsl --shutdown` and then re-open the WSL instance.

Beware corruption of your WSL2 volume. It has happened to me three times over a year (likely due to all the shutdowns I was doing...). Do not forget to take frequent backups/git pushes.