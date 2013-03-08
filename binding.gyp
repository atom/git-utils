{
  'targets': [
    {
      'target_name': 'git2',
      'variables': {
        'git2-arch-flag': '',
      },
      'conditions': [
        ['target_arch=="ia32"', {
          'variables': {
            'git2-arch-flag': '-m32'
          }
        }]
      ],
      'actions': [
        {
          'action_name': 'Build libgit2',
          'inputs': ['deps/libgit2'],
          'outputs': ['deps/libgit2/builds/libgit2.a'],
          'action': [
            'sh',
            'build-libgit2.sh',
            '<(git2-arch-flag)'
          ]
        }
      ]
    },
    {
      'target_name': 'git',
      'dependencies': [
        'git2'
      ],
      'sources': [
        'src/repository.cc'
      ],
      'libraries': [
        '../deps/libgit2/build/libgit2.a'
      ],
      'include_dirs': [
        'deps/libgit2/include'
      ]
    }
  ]
}
