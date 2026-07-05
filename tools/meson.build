psl = executable('psl', 'psl.c',
  include_directories : configinc,
  c_args : ['-DHAVE_CONFIG_H'],
  link_language : link_language,
  dependencies : [libpsl_dep, networking_deps],
  install : true,
)

psl_man = configure_file(
  input : 'psl.1.in',
  output : 'psl.1',
  configuration : _cdata)

install_man(psl_man)
