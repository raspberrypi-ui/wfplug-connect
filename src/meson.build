gtkmm = dependency('gtkmm-3.0', version: '>=3.24')

sources = files(
  'connect.cpp',
  'connect.c'
)

deps = [ gtkmm ]

incdir = include_directories('/usr/include/wf-panel-pi')

args = [ '-DPLUGIN_NAME="' + meson.project_name() + '"', '-DPACKAGE_DATA_DIR="' + resource_dir + '"', '-DGETTEXT_PACKAGE="wfplug_' + meson.project_name() +'"' ]

shared_module('lib' + meson.project_name(), sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        c_args : args,
        cpp_args : args,
        include_directories : incdir,
        name_prefix: ''
)

metadata = files()
install_data(metadata, install_dir: metadata_dir)
