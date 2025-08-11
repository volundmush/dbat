import glob
import os
import sys
import re
from pathlib import Path
from setuptools import setup, find_packages, Extension
from Cython.Build import cythonize

os.chdir(os.path.dirname(os.path.realpath(__file__)))

OS_WINDOWS = os.name == "nt"

pattern = re.compile(r"(\w+):(\w+)=(.*)")

CMAKE_CACHE = dict()

build_dir = Path() / "build"
deps = build_dir / "_deps"

with open(build_dir / "CMakeCache.txt", mode="r") as f:
    for line in f:
        text = line.strip()
        if not text or (text.startswith("#") or text.startswith("//")):
            continue
        if (match := pattern.match(text)):
            key, var, value = match.groups()
            CMAKE_CACHE[key] = (var, value)

INCLUDES = CMAKE_CACHE["CIRCLE_INCLUDE_DIRS"][1].split(";")
LINKS = ["compiled"] + [str(p.absolute()) for p in deps.glob("*-build")]

LIBRARY_DIRS = LINKS + ["/usr/lib/x86_64-linux-gnu"]

print(LIBRARY_DIRS)

LIBRARIES = [
    "circlemud",
    "fmtd",
    "boost_system",
    "boost_iostreams",
    "boost_program_options",
    "boost_regex",
    "z"
]


extensions = [
    Extension(
        "dbat_ext",                # The name of your Python extension module
        sources=["dbat_ext/core.pyx"],  # Your Cython source file(s)
        language="c++",            # Ensure C++ linkage
        libraries=LIBRARIES,       # The libraries to link against
        include_dirs=INCLUDES,
        library_dirs=LIBRARY_DIRS,
        extra_compile_args=["-std=c++23", "-g", "-Wno-write-strings", "-Wno-format-truncation", "-Wno-multichar", "-Wno-unused-result", "-fpermissive"],  # Or the C++ standard you are using
    )
]

def get_requirements():
    """
    To update the requirements for Evennia, edit the requirements.txt file.
    """
    with open("requirements.txt", "r") as f:
        req_lines = f.readlines()
    reqs = []
    for line in req_lines:
        # Avoid adding comments.
        line = line.split("#")[0].strip()
        if line:
            reqs.append(line)
    return reqs

def get_scripts():
    """
    Determine which executable scripts should be added. For Windows,
    this means creating a .bat file.
    """
    if OS_WINDOWS:
        batpath = os.path.join("bin", "windows", "dbat.bat")
        scriptpath = os.path.join(sys.prefix, "Scripts", "dbat.py")
        with open(batpath, "w") as batfile:
            batfile.write('@"%s" "%s" %%*' % (sys.executable, scriptpath))
        return [batpath, os.path.join("bin", "windows", "dbat.py")]
    else:
        return [os.path.join("bin", "unix", "dbat")]

def package_data():
    """
    By default, the distribution tools ignore all non-python files.

    Make sure we get everything.
    """
    file_set = []
    for root, dirs, files in os.walk("dbat"):
        for f in files:
            if ".git" in f.split(os.path.normpath(os.path.join(root, f))):
                # Prevent the repo from being added.
                continue
            file_name = os.path.relpath(os.path.join(root, f), "dbat")
            file_set.append(file_name)
    return file_set

setup(
    name="dbat_ext",
    ext_modules=cythonize(extensions),
    author="VolundMush",
    maintainer="VolundMush",
    scripts=get_scripts(),
    install_requires=get_requirements(),
    package_data={"": package_data()},
    zip_safe=False,
)