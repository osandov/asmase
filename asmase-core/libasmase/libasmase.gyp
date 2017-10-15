{
  'targets': [
    {
      'target_name': 'libasmase',
      'type': 'static_library',
      'sources': [
        'assembler.cpp',
        'libasmase.c',
        'tracee.c',
        'x86_64/arch.S',
        'x86_64/regs.c',
      ],
      'defines': [
        '_GNU_SOURCE'
      ],
      'include_dirs': [
        'include',
        'x86_64'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include'
        ]
      },
      'link_settings': {
        'ldflags': ['<!(llvm-config --ldflags)'],
        'libraries': [
          '<!(llvm-config --libs)',
        ]
      }
    }
  ]
}
