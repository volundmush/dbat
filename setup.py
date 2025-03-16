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
LINKS = ["bin"] + [str(p.absolute()) for p in deps.glob("*-build")]

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
        extra_compile_args=["-std=c++23", "-g"],  # Or the C++ standard you are using
    )
]

setup(
    name="dbat_ext",
    ext_modules=cythonize(extensions),
)