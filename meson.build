project('PrimeCuts', 'cpp',
  default_options : ['cpp_std=c++17', 'warning_level=3'])

glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')

executable('primecuts',
  ['src/main.cpp',
   'src/config_loader.cpp',
   'src/command_manager.cpp',
   'src/dbus_provider.cpp'],
  dependencies: [glib_dep, gio_dep],
  install: true,
  install_dir: get_option('bindir'))
