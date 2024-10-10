name = 'connect'

sources = files(
  'connect.cpp',
  'connect.c'
)

gtkmm = dependency('gtkmm-3.0', version: '>=3.24')
deps = [ gtkmm ]

metadata = files()

shared_module(name, sources,
        dependencies: deps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : '-DPLUGIN_NAME="' + name + '"',
        include_directories : incdir
)

install_data(metadata, install_dir: metadata_dir)
