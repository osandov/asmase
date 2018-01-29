{
  'targets': [
    {
      'target_name': 'addon',
      'sources': ['src/main.cc'],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
      ],
      'link_settings': {
        'ldflags': ['<!(llvm-config --ldflags)'],
        'libraries': [
          '<!(llvm-config --libs)',
        ]
      }
    }
  ]
}
