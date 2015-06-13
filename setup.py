#!/usr/bin/env python3

from distutils.core import setup
from Cython.Build import cythonize

setup(
    name = "Cotton - a Python sandbox",
    ext_modules = cythonize("cotton.pyx"),
)
