project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest'
    ]
)

src = [ 
    'src/main.cpp',
    'src/utils.cpp',
    'src/dx12.cpp',
    'src/window.cpp'
]

executable('engine', src,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'd3dcompiler.lib' ],
    cpp_args : [ '-DUNICODE=1' ],
    win_subsystem : 'console'
)
