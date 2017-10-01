{
  'targets': [
    {
      'target_name': 'libasmase',
      'type': 'static_library',
      'sources': [
        'assembler.cpp',
        'libasmase.c',
        'registers.c',
        'tracee.c',
        'x86.c'
      ],
      'defines': [
        '_GNU_SOURCE'
      ],
      'include_dirs': [
        'include'
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
