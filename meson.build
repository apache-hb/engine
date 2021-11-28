project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest'
    ]
)

src = [ 
    'src/render/context.cpp',
    'src/utils.cpp',
    'src/main.cpp',
    'src/window.cpp'
]

executable('engine', src,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib' ],
    cpp_args : [ '-DUNICODE=1' ],
    win_subsystem : 'console'
)
