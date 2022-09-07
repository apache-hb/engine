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

fmt = subproject('fmt').get_variable('fmt')

subdir('engine')

executable('game', 'game/main.cpp',
    dependencies : [ platform, logging ],
    cpp_args : args,
    win_subsystem : 'console'
)
