project(
    'wfplug_connect',
    'c',
    'cpp',
    version: '0.8.0',
    license: 'MIT',
    meson_version: '>=0.60.0',
    default_options: [
        'cpp_std=c++17',
        'c_std=c11',
        'warning_level=2',
        'werror=false',
    ],
)

share_dir = join_paths(get_option('prefix'), 'share')
resource_dir = join_paths(share_dir, 'wf-panel-pi')
metadata_dir = join_paths(resource_dir, 'metadata')

add_project_arguments('-DPACKAGE_LOCALE_DIR="' + share_dir + '/locale"', language : [ 'c', 'cpp' ])
add_project_arguments('-D_GNU_SOURCE', language : [ 'c', 'cpp' ])

icons = ['rpc-disabled.png', 'rpc-enabled.png', 'rpc-active.png']
foreach icon : icons
  icon_path = join_paths('data', icon)
  install_data(icon_path, install_dir: get_option('datadir') / 'icons/hicolor/128x128/status')
endforeach

subdir('src')
subdir('po')
