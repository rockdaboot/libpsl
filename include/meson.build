version = meson.project_version()
version_arr = version.split('.')
major = version_arr[0].to_int()
minor = version_arr[1].to_int()
patch = version_arr[2].to_int()

# Extend shared _cdata from parent scope with version defines
_cdata.set('LIBPSL_VERSION', version)
_cdata.set('LIBPSL_VERSION_MAJOR', major)
_cdata.set('LIBPSL_VERSION_MINOR', minor)
_cdata.set('LIBPSL_VERSION_PATCH', patch)

hex = '0123456789abcdef'
ver = '0x@0@@1@@2@@3@@4@@5@'
_cdata.set('LIBPSL_VERSION_NUMBER',
  ver.format(
    hex[major / 16], hex[major % 16],
    hex[minor / 16], hex[minor % 16],
    hex[patch / 16], hex[patch % 16]))

libpsl_h = configure_file(
  input : 'libpsl.h.in',
  output : 'libpsl.h',
  configuration : _cdata)

install_headers(libpsl_h)
