project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest',
        'default_library=static'
    ]
)

src = [ 
    'src/util/error.ixx',
    'src/util/timer.ixx',
    'src/util/units.ixx',
    'src/util/strings.ixx',
    'src/main.cpp'
]

args = [ '-DUNICODE=1', '/stdIfcDir', 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.30.30705\\ifc\\x64' ]

if get_option('agility-sdk').enabled()
    src += [ 'src/render/agility.cpp' ]
    args += [ '-DAGILITY=1' ]
else
    args += [ '-DAGILITY=0' ]
endif

if get_option('debug-d3d12').enabled()
    args += [ '-DD3D12_DEBUG=1' ]
else
    args += [ '-DD3D12_DEBUG=0' ]
endif

deps = []

executable('engine', src,
    include_directories : 'src',
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib', 'd3dcompiler.lib', 'dxva2.lib' ],
    cpp_args : args,
    dependencies : deps,
    win_subsystem : 'console'
)
