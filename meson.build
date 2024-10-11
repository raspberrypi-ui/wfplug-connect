project(
    'wfplug_connect',
    'c',
    'cpp',
    version: '0.8.0',
    license: 'MIT',
    meson_version: '>=0.51.0',
    default_options: [
        'cpp_std=c++17',
        'c_std=c11',
        'warning_level=2',
        'werror=false',
    ],
)

incdir = include_directories('/usr/include/wf-panel-pi')

add_project_arguments(['-Wno-pedantic', '-Wno-unused-parameter', '-Wno-parentheses'], language: 'cpp')

share_dir = join_paths(get_option('prefix'), 'share')
resource_dir = join_paths(share_dir, 'wf-panel-pi')
metadata_dir = join_paths(resource_dir, 'metadata')

gettext_package = meson.project_name()

add_project_arguments('-DMETADATA_DIR="' + metadata_dir + '"', language : 'cpp')
add_project_arguments('-DPACKAGE_DATA_DIR="' + resource_dir + '"', language : [ 'c', 'cpp' ])
add_project_arguments('-DGETTEXT_PACKAGE="' + gettext_package + '"', language : [ 'c', 'cpp' ])
add_project_arguments('-DPACKAGE_LOCALE_DIR="' + share_dir + '/locale"', language : [ 'c', 'cpp' ])

icons = ['rpc-disabled.png', 'rpc-enabled.png', 'rpc-active.png']
foreach icon : icons
  icon_path = join_paths('data', icon)
  install_data(icon_path, install_dir: get_option('datadir') / 'icons/hicolor/128x128/status')
endforeach

subdir('src')
subdir('po')
