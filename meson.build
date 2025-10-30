project('PrimeCuts', 'cpp',
  default_options : ['cpp_std=c++17', 'warning_level=3'])

glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
gtkmm_dep = dependency('gtkmm-3.0')

executable('primecuts',
  ['src/main.cpp',
   'src/config_loader.cpp',
   'src/command_manager.cpp',
   'src/dbus_provider.cpp',
   'src/json11.cpp'],
  dependencies: [glib_dep, gio_dep],
  install: true,
  install_dir: get_option('bindir'))

executable('primecuts-editor',
  ['src/editor_main.cpp',
   'src/editor_window.cpp',
   'src/config_loader.cpp',
   'src/command_manager.cpp',
   'src/json11.cpp',
   'src/edit_dialog.cpp'],
  dependencies: [glib_dep, gio_dep, gtkmm_dep],
  install: true,
  install_dir: get_option('bindir'))
