project('engine', [ 'cpp', 'c' ],
    license : 'GPLv3',
    version : '0.1.0',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
        'warning_level=3',
        'werror=true',
        'b_vscrt=static_from_buildtype',
        'default_library=static',
        'b_lundef=true'
    ]
)

args = [ '-D_HAS_EXCEPTIONS=0', '/wd4324' ]

d3d12 = subproject('d3d12').get_variable('d3d12')
imgui = subproject('imgui').get_variable('imgui')
gltf = subproject('tinygltf').get_variable('gltf')

subdir('engine')

executable('editor', 'editor/main.cpp',
    dependencies : [ input, render, ui ],
    cpp_args : args,
    win_subsystem : 'console'
)

executable('game', 'game/main.cpp',
    dependencies : [ input, render, ui ],
    cpp_args : args,
    win_subsystem : 'console'
)
