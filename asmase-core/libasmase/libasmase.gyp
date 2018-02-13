{
  'targets': [
    {
      'target_name': 'libasmase',
      'type': 'static_library',
      'sources': [
        'libasmase.c',
        'tracee.c',
        'x86_64.c',
        'x86_64_code.S',
      ],
      'defines': [
        '_GNU_SOURCE'
      ],
      'include_dirs': [
        'include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include'
        ]
      }
    }
  ]
}
