{
  'targets': [
    {
      'target_name': 'proc-title-watch',
      'include_dirs': [ '<!(node -e "require(\'nan\')")' ],
      'sources': [
        'src/main.cc',
        'src/procwatch.h',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources': [
            'src/procwatch_mac.cc',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
            ],
          },
        }],
        ['OS=="win"', {
          'sources': [
            'src/procwatch_win.cc',
          ],
          'msvs_disabled_warnings': [
          ],
        }],
        ['OS not in ["mac", "win"]', {
          'sources': [
            'src/procwatch_posix.cc',
          ],
          'cflags': [
          ],
          'link_settings': {
            'ldflags': [
            ],
            'libraries': [
            ],
          },
        }],
      ],
    }
  ]
}
