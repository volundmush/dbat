# What is this?
This is the codebase for Dragon Ball Advent Truth, a MUD written atop of CircleMUD (with Goodies) 3.5 "Rasputin". Originally written in C, refactored with some C++ in an attempt to modernize it.

# Can I run it?
Sort of. I have not yet included all of the game assets that it needs to run. This is just the code.

# Using the Sucker

This project is designed to be run on an Ubuntu 22.04 (or later) server, using GCC for the C++ compiler and CPython3.11 for running the Python. If you can get it to work in some other way, good on you, this is just what I know works.

The recommended IDE is VS Code due to the multi-language nature of the project. More on that later.

## Ubuntu Setup
The following apt packages are needed:

`sudo apt-get install cmake build-essential gdb git ninja-build`

Clone the directory.

`git clone https://github.com/volundmush/dbat`
The main project.


`git clone https://github.com/volundmush/thermite`
The networking library and webclient.

## Installing Rust
The Thermite project will require Rust.
[The Official Rust website](https://www.rust-lang.org/tools/install) has the install instructions.

## Installing Boost
As of this writing, the project was written with Boost 1.84.0 in mind. Owing to its size and unique compilation requirements, it cannot be handled via CPM in the CMakeLists.txt, and must be installed as a system library.

The latest .tar.gz can be found at [The Boost Downloads website](https://www.boost.org/users/download/). The quickest way to install, using the 1.84 variant as an example, is...

`wget https://boostorg.jfrog.io/artifactory/main/release/1.84.0/source/boost_1_84_0.tar.gz`
to download boost into your home directory.

`tar zxvf boost_1_84_0.tar.gz`
to decompress it into a folder...

`cd boost_1_84_0`
to enter the folder... then...

`./bootstrap.sh --with-libraries=all --with-toolset=gcc`
to configure,

`./b2 toolset=gcc`
to compile, and

`sudo ./b2 install --prefix=/usr`
to install.

It does take a minute or two to configure and compile 'cuz boost is huge.

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

Restart VS Code if needed.

After Remote Development is installed, the toolbar on the left side has a "Remote Explorer" icon. Under that is a Dropdown which should have "WSL Targets" option. Select that from the dropdown and Ubuntu should be in thhe WSL Targets. You can use this to "connect" to the WSL2 instance. VS Code will install remote management tools to the Ubuntu instance so it can remote connect.

From there, open the `dbat` folder (or whatever else you cloned it to) as your project folder.

You will need to then go to Extensions again and tell VS Code to install your extensions in WSL.

# Project Breakdown
The project relies on several major components to work properly.

The first is `libcirclemud.a` which is compiled from the .h and .cpp files in `include/` and `src/`.

The second is the `circle` and `migrate` executables, which links to the above and is found in `apps/`

In order for the game to work, the `lib` folder must be present in the project root. That is not (yet) included.

The Thermite project opens up an internal listening port which DBAT connects to so the two can exchange client activity data. Thermite is the program which handles telnet, webclient, and the website static files.

For compiling and running Thermite, simply use `cargo build` then `cargo run` in Thermite project directory.

# Things to Watch Out For
In my experiences, WSL2 can get screwy sometimes, though that was working with JetBrains IDEs. I'm not sure about VS Code. Still, if the WSL instance freezes or starts acting bizarre, you can use `wsl --shutdown` from the Windows Command Prompt and then re-open the WSL instance.

Beware corruption of your WSL2 volume. It has happened to me three times over a year (likely due to all the shutdowns I was doing...). Do not forget to take frequent backups/git pushes.