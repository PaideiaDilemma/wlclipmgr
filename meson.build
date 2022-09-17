project('wlclipmgr', 'cpp', 'c',
    default_options: [
      'cpp_std=c++20',
      'c_std=c99',
      'warning_level=1',
      'debug=true'
      ]
  )

cc = meson.get_compiler('c')
lprocps = cc.find_library('procps')

subdir('thirdParty')

source_files = [
  'main.cpp',
  'clipboard.cpp',
  'procblock.cpp'
  ]

wlclipmgr = executable(
  meson.project_name(),
  source_files,
  link_with : [xdgmime],
  dependencies: [lprocps],
  native: true
  )
