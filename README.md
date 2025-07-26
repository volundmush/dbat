# What is this?
This is the codebase for Dragon Ball Advent Truth, a MUD written atop of CircleMUD (with Goodies) 3.5 "Rasputin". Originally written in C, refactored with some C++ in an attempt to modernize it.

# Can I run it?
Sort of. I have not yet included all of the game assets that it needs to run. This is just the code.

# Using the Sucker

This project is designed to be run on an Ubuntu 24.04 (or later) server, using GCC for the C++ compiler and CPython3.12 or later for running the Python. If you can get it to work in some other way, good on you, this is just what I know works.

The recommended IDE is VS Code due to the multi-language nature of the project. More on that later.

## Ubuntu Setup
The following apt packages are needed:

`sudo apt-get install -y cmake build-essential gdb git ninja-build ccache mold libboost-all-dev python3 python3-virtualenv libpython3-dev`

Clone the directory.

`git clone https://github.com/volundmush/dbat`
The main project.


`cd dbat` to enter the folder....


Create a virtualenv:
`python -m virtualenv .venv`


Activate the virtualenv...
`source .venv/bin/activate`


Install Python dependencies:
`pip install -r requirements.txt`

(you can also allow Visual Studio Code to handle the Python stuff)

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

And `Python` related extensions.

Restart VS Code if needed.

After Remote Development is installed, the toolbar on the left side has a "Remote Explorer" icon. Under that is a Dropdown which should have "WSL Targets" option. Select that from the dropdown and Ubuntu should be in thhe WSL Targets. You can use this to "connect" to the WSL2 instance. VS Code will install remote management tools to the Ubuntu instance so it can remote connect.

From there, open the `dbat` folder (or whatever else you cloned it to) as your project folder.

You will need to then go to Extensions again and tell VS Code to install your extensions in WSL2.

# Project Breakdown
The project relies on several major components to work properly.

## C++ Components
The project is compiled using CMake via the provided CMakeLists.txt. It will download all of its dependencies using the CMake Package Manager.

 `libcirclemud.a` is compiled from the .h and .cpp files in `include/` and `src/`.

Utility executables such as `migrate`, which links to the above and is found in `apps/`

## Python Components
The Python module `dbat` depends on the `mudforge` package found at `https://github.com/volundmush/mudforge`.

MudForge runs two processes: the telnet portal, and the server backend which is accessed through a FastAPI HTTP/2 + TLS REST connection.

## Cython Bridge
The `dbat_ext` folder contains Cython code as .pyx and .pxd files which creates the `dbat_ext` module used by the Python code above.

## Data Directory
In order for the game to work, the `lib` folder must be present in the project root. That is not (yet) included in the repository. This folder contains the MUD database.

## Config Files
The root directory contains `config.plugin-001.toml` and `config.user.toml`

These use MudForge's default settings as a base and progressively override them.

`config.plugin-001.toml` is meant to hold information specific to DBAT in general, while `config.user.toml` is for development overrides and customization for this specific instance.

# How do you compile it?
`./compile.sh <debug|release>`

This will first compile the C++ code and then run setup.py to ensure the Cython extension is built.

# How do you run it?
A proper launcher is pending.

a `launch.json` has been included for VS Code.

However, make sure the cwd is the root `dbat` folder and the virtualenv is active, and then:
`python -m mudforge.portal`
`python -m mudforge.game`

# Things to Watch Out For
In my experiences, WSL2 can get screwy sometimes, though that was working with JetBrains IDEs. I'm not sure about VS Code. Still, if the WSL instance freezes or starts acting bizarre, you can use `wsl --shutdown` from the Windows Command Prompt and then re-open the WSL instance.

Beware corruption of your WSL2 volume. It has happened to me three times over a year (likely due to all the shutdowns I was doing...). Do not forget to take frequent backups/git pushes.