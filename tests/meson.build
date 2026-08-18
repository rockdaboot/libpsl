fs = import('fs')

psl_dafsa_src = join_paths(meson.current_source_dir(), 'psl.dafsa')
psl_ascii_dafsa_src = join_paths(meson.current_source_dir(), 'psl_ascii.dafsa')
has_dafsa_files = fs.exists(psl_dafsa_src) and fs.exists(psl_ascii_dafsa_src)

if has_dafsa_files
  psl_dafsa = psl_dafsa_src
  psl_ascii_dafsa = psl_ascii_dafsa_src
else
  if not python.found()
    error('Python is required to build DAFSA files. Install python or provide psl.dafsa and psl_ascii.dafsa in tests/')
  endif

  psl_dafsa = custom_target('psl.dafsa',
    input : psl_file,
    output : 'psl.dafsa',
    build_by_default: false,
    command : [python, psl_make_dafsa, '--output-format=binary', '@INPUT@', '@OUTPUT@'])

  psl_ascii_dafsa = custom_target('psl_ascii.dafsa',
    input : psl_file,
    output : 'psl_ascii.dafsa',
    build_by_default: false,
    command : [python, psl_make_dafsa, '--output-format=binary', '--encoding=ascii', '@INPUT@', '@OUTPUT@'])
endif

if has_dafsa_files
  psl_dafsa_path = fs.as_posix(psl_dafsa_src)
  psl_ascii_dafsa_path = fs.as_posix(psl_ascii_dafsa_src)
else
  psl_dafsa_path = fs.as_posix(psl_dafsa.full_path())
  psl_ascii_dafsa_path = fs.as_posix(psl_ascii_dafsa.full_path())
endif

tests_cargs = [
  '-DHAVE_CONFIG_H',
  '-DSRCDIR="@0@"'.format(meson.current_source_dir()),
  '-DPSL_FILE="@0@"'.format(psl_file),
  '-DPSL_TESTFILE="@0@"'.format(psl_test_file),
  '-DPSL_DAFSA="@0@"'.format(psl_dafsa_path),
  '-DPSL_ASCII_DAFSA="@0@"'.format(psl_ascii_dafsa_path),
]

tests = [
  'test-is-public',
  'test-is-public-all',
  'test-is-cookie-domain-acceptable',
]

if enable_builtin
  tests += ['test-is-public-builtin', 'test-registrable-domain']
endif

libtestcommon = static_library('testcommon', 'common.c',
  build_by_default: false)

foreach test_name : tests
  sources = [test_name + '.c', 'common.c', 'common.h']
  exe = executable(test_name, sources,
    build_by_default: false,
    c_args : tests_cargs,
    link_with : [libpsl, libtestcommon],
    include_directories : configinc,
    link_language : link_language,
    dependencies : [libpsl_dep, networking_deps])
  if not has_dafsa_files
    test(test_name, exe, depends : [psl_dafsa, psl_ascii_dafsa])
  else
    test(test_name, exe)
  endif
endforeach
