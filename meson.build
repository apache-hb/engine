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
        'b_lto=true',
        'b_lundef=true'
    ]
)

# python = find_program('python')
# dxc = generator(python,
#     arguments : [ '@INPUT@', '@OUTPUT@', 'ps,vs', '6_0' ],
#     output : [ '@BASENAME@.ps.pso', '@BASENAME@.vs.pso' ]
# )

args = [ '-D_HAS_EXCEPTIONS=0', '/wd4324' ]

glfw = dependency('glfw3', fallback : [ 'glfw', 'glfw' ])
d3d12 = subproject('d3d12').get_variable('d3d12')
fmt = subproject('fmt').get_variable('fmt')

subdir('engine')

executable('game', 'game/main.cpp',
    dependencies : [ glfw, base, render ],
    cpp_args : args,
    win_subsystem : 'console'
)
