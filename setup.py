#!/usr/bin/env python3

from setuptools import setup, Extension
import numpy

module = Extension(
    name='asmase',
    sources=[
        'python/asmase_module.c',
        'python/asmase_assembler.c',
        'python/asmase_instance.c'
    ],
    include_dirs=['libasmase/include', numpy.get_include()],
    library_dirs=['libasmase'],
    libraries=['asmase'],
)

setup(name='asmase',
      ext_modules=[module],
      test_suite='tests')
