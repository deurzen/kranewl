wayland_protocols = dependency(
  'wayland-protocols',
  version: '>=1.25',
  fallback: 'wayland-protocols',
  default_options: ['tests=false'],
)

protocols_directory = wayland_protocols.get_variable('pkgdatadir')
wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = find_program(
  wayland_scanner_dep.get_variable('wayland_scanner'),
  native: true,
)

protocols = [
  [protocols_directory, 'stable/xdg-shell/xdg-shell.xml'],
  [protocols_directory, 'unstable/pointer-constraints/pointer-constraints-unstable-v1.xml'],
  ['wlr-layer-shell-unstable-v1.xml'],
]

protocol_src = []

foreach p : protocols
  xml_file = join_paths(p)

  protocol_src += custom_target(
    xml_file.underscorify() + '_server_h',
    input: xml_file,
    output: '@BASENAME@-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach
