from setuptools import setup, Extension
from Cython.Build import cythonize
import os

# Adjust these paths to point to your actual library and header locations
lib_dir = os.path.join(os.getcwd(), "bin")
include_dir = os.path.join(os.getcwd(), "include")

extensions = [
    Extension(
        "dbat_ext",                # The name of your Python extension module
        sources=["dbat_ext/core.pyx"],  # Your Cython source file(s)
        language="c++",            # Ensure C++ linkage
        extra_objects=[os.path.join(lib_dir, "libcirclemud.a")],
        include_dirs=[include_dir],
        library_dirs=[lib_dir],
        extra_compile_args=["-std=c++23"],  # Or the C++ standard you are using
    )
]

setup(
    name="dbat_ext",
    ext_modules=cythonize(extensions),
)