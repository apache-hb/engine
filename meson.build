project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest'
    ]
)

src = [ 
    'src/main.cpp',
    'src/utils.cpp',
    'src/window.cpp'
]

executable('engine', src,
    cpp_args : [ '-DUNICODE=1' ],
    win_subsystem : 'windows'
)
