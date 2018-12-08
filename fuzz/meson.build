fuzzer_cargs = [
  '-DSRCDIR="@0@"'.format(meson.source_root()),
  '-DTEST_RUN',
]

foreach test_name : ['fuzzer', 'load_fuzzer', 'load_dafsa_fuzzer']
  source_file = 'libpsl_@0@.c'.format(test_name)
  exe = executable(test_name, source_file, 'main.c',
    link_with : libpsl,
    c_args : fuzzer_cargs,
    include_directories : [configinc, includedir],
    dependencies : libicu_dep,
  )
  test(test_name, exe)
endforeach
