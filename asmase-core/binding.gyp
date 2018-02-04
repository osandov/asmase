{
  'targets': [
    {
      'target_name': 'addon',
      'sources': ['addon.cc'],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
      ],
      'dependencies': [
        'libasmase/libasmase.gyp:libasmase'
      ]
    }
  ]
}
