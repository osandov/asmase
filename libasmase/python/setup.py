#!/usr/bin/env python3

from distutils.core import setup, Extension

module = Extension(
    name='asmase',
    sources=['asmase_module.c', 'asmase_assembler.c', 'asmase_instance.c'],
    include_dirs=['../include'],
    library_dirs=['..'],
    libraries=['asmase'],
)

setup(name='libasmase',
      ext_modules=[module])
