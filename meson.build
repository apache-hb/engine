project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest'
    ]
)

src = [ 
    'src/render/context.cpp',
    'src/render/agility.cpp',
    'src/render/util.cpp',
    'src/util/strings.cpp',
    'src/util/units.cpp',
    'src/util/win32.cpp',
    'src/system/system.cpp',
    'src/logging/log.cpp',
    'src/main.cpp',
    'src/window.cpp'
]

executable('engine', src,
    include_directories : 'src',
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib', 'd3dcompiler.lib' ],
    cpp_args : [ '-DUNICODE=1' ],
    win_subsystem : 'console'
)
