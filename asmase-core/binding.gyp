{
  'targets': [
    {
      'target_name': 'binding',
      'sources': ['src/main.cc'],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
      ],
      'dependencies': [
        'libasmase/libasmase.gyp:libasmase'
      ]
    }
  ]
}
