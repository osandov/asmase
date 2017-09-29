{
  'targets': [
    {
      'target_name': 'binding',
      'sources': ['src/main.cc'],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
        '../libasmase'
      ],
      'link_settings': {
        'libraries': ['-lasmase'],
        'ldflags': ['-L../../libasmase']
      }
    }
  ]
}
