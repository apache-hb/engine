project('engine', 'cpp',
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

args = [ '-D_HAS_EXCEPTIONS=0' ]

d3d12 = subproject('d3d12').get_variable('d3d12')
ubn = subproject('atomic-queue').get_variable('queue')
fmt = subproject('fmt').get_variable('fmt')

subdir('engine')

executable('game', 'game/main.cpp',
    dependencies : [ platform, logging, render ],
    cpp_args : args,
    win_subsystem : 'console'
)
