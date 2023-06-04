project(
  'kranewl',
  'cpp',
  version: '1.0.0',
  license: 'BSD-3',
  meson_version: '>=1.0.0',
  default_options: [
    'cpp_std=c++20',
    'buildtype=release',
    'debug=false',
    'default_library=static',
    'optimization=2',
    'warning_level=2',
    'werror=true',
    'b_ndebug=if-release',
  ],
)

GIT_BRANCH = run_command('git', 'rev-parse', '--abbrev-ref', 'HEAD', check: false).stdout().strip()
GIT_COMMIT_HASH = run_command('git', 'rev-parse', 'HEAD', check: false).stdout().strip()
GIT_COMMIT_MESSAGE = run_command('sh', '-c', 'git show | head -n 5 | tail -n 1', check: false).stdout().strip()
GIT_DIRTY = run_command('sh', '-c', 'git diff-index --quiet HEAD -- || echo "dirty"', check: false).stdout().strip()

add_project_arguments(
  [
    '-w',
    '-Wundef',
    '-Wno-unused-parameter',
    '-Wno-unused-value',
    '-Wno-missing-field-initializers',
    '-Wno-narrowing',

    '-DWLR_USE_UNSTABLE',
    '-DXWAYLAND',
    '-DTRACING_DISABLED',

    f'-DGIT_BRANCH="@GIT_BRANCH@"',
    f'-DGIT_COMMIT_HASH="@GIT_COMMIT_HASH@"',
    f'-DGIT_COMMIT_MESSAGE="@GIT_COMMIT_MESSAGE@"',
    f'-DGIT_DIRTY="@GIT_DIRTY@"',
  ],
  language: 'cpp',
)

wayland = subproject(
  'wayland',
  version: '>=1.21',
  default_options: [
    'tests=false',
    'documentation=false',
    'default_library=shared',
    'werror=false',
  ],
)

wlroots = subproject(
  'wlroots',
  version: ['>=0.16.0', '<0.17.0'],
  default_options: [
    'examples=false',
    'default_library=static',
    'werror=false',
  ],
)

have_xwlr = wlroots.get_variable('features').get('xwayland')
xcb = dependency('xcb')
have_xwayland = xcb.found() and have_xwlr
if have_xwayland
  add_project_arguments('-DXWAYLAND', language: 'cpp')
endif

subdir('protocols')

kranewl_inc = include_directories('include')
src_files = run_command('globber.sh', check: true)
kranewl_src = src_files.stdout().strip().split('\n')

kranewl_deps = [
  wlroots.get_variable('wlroots').as_link_whole(),
  xcb,
  dependency('egl'),
  dependency('gl', 'opengl'),
  dependency('libdrm'),
  dependency('libinput'),
  dependency('pixman-1'),
  dependency('spdlog'),
  dependency('threads'),
  dependency('xcb-icccm'),
  dependency('xkbcommon'),
]

executable(
  'kranewl',
  kranewl_src + protocol_src,
  include_directories: [kranewl_inc, wlroots.get_variable('wlr_inc')],
  dependencies: kranewl_deps,
  install: true
)
