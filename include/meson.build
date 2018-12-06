version_arr = meson.project_version().split('.')
cdata = configuration_data()
cdata.set('LIBPSL_VERSION', meson.project_version())
cdata.set('LIBPSL_VERSION_MAJOR', version_arr[0])
cdata.set('LIBPSL_VERSION_MINOR', version_arr[1])
cdata.set('LIBPSL_VERSION_PATCH', version_arr[2])

# FIXME: We should not run a command-line for this, but I don't think Meson
# has API for that.
printf = find_program('printf')
r = run_command(printf, '0x%02x%02x%02x', version_arr, check : true)
cdata.set('LIBPSL_VERSION_NUMBER', r.stdout().strip())

configure_file(
  input : 'libpsl.h.in',
  output : 'libpsl.h',
  configuration : cdata)
