psl = executable('psl', 'psl.c',
  link_with : libpsl,
  include_directories : [configinc, includedir],
  c_args : ['-DHAVE_CONFIG_H'],
  install : true,
)

install_man('psl.1')
