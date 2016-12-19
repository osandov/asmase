#!/usr/bin/env python3

from setuptools import setup, Extension
import numpy

module = Extension(
    name='asmase',
    sources=['asmase_module.c', 'asmase_assembler.c', 'asmase_instance.c'],
    include_dirs=['../include', numpy.get_include()],
    library_dirs=['..'],
    libraries=['asmase'],
)

setup(name='libasmase',
      ext_modules=[module],
      test_suite='tests')
