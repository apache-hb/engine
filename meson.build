project('engine', [ 'cpp', 'c' ],
    license : 'GPLv3',
    version : '0.1.0',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
        'b_ndebug=if-release',
        'warning_level=3',
        'werror=true',
        'b_vscrt=static_from_buildtype',
        'default_library=static',
        'b_lundef=true'
    ]
)

args = [ 
    '-D_HAS_EXCEPTIONS=0', # msvc stl requires this
    '/wd4324', # warnings about padding
    '/wd4127' # warnings about if constexpr
]

d3d12 = subproject('d3d12').get_variable('d3d12')
gltf = subproject('tinygltf').get_variable('gltf')
toml = subproject('toml').get_variable('toml')

subdir('engine')

executable('game', 'game/main.cpp',
    dependencies : [ input, render, ui ],
    cpp_args : args,
    win_subsystem : 'console'
)

subdir('editor')
