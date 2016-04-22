#!/usr/bin/env python3
from setuptools import setup
from Cython.Build import cythonize

setup(
    name="cotton",
    version="0.1",
    author="algorithm-ninja",
    author_email="algorithm@ninja",
    ext_modules = cythonize("cotton.pyx"),
    url="https://github.com/algorithm-ninja/cotton"
)
