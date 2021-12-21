project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest',
        'warning_level=3',
        'werror=true',
        'default_library=static'
    ]
)

src = [ 
    'src/render/util.cpp',
    'src/render/render.cpp',
    'src/render/upload.cpp',
    'src/render/debug.cpp',
    'src/render/library.cpp',
    'src/render/heap.cpp',
    'src/render/factory.cpp',

    'src/util/strings.cpp',
    'src/util/units.cpp',
    'src/util/timer.cpp',
    'src/util/win32.cpp',

    'src/assets/obj.cpp',
    'src/assets/image.cpp',

    'src/system/window.cpp',
    'src/system/system.cpp',

    'src/logging/log.cpp',

    'src/input/inputx.cpp',
    'src/input/mnk.cpp',
    'src/input/camera.cpp',

    'src/main.cpp'
]

args = [ 
    '-DUNICODE=1', 
    '/wd4324' # C4324: structure was padded due to alignment specifier
]

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

links = [ 
    'd3d12.lib', 
    'dxgi.lib', 
    'dxguid.lib', 
    'd3dcompiler.lib',
    'xinput.lib'
]

imgui = subproject('imgui').get_variable('imgui_dep')
tinyobj = subproject('tinyobj').get_variable('tinyobj_dep')
queue = subproject('atomic-queue').get_variable('queue')
stb = subproject('stb').get_variable('stb')
d3d12 = subproject('d3d12').get_variable('d3d12')

deps = [ imgui, tinyobj, queue, stb, d3d12 ]

executable('engine', src,
    include_directories : 'src',
    dependencies : deps,
    link_args : links,
    cpp_args : args,
    win_subsystem : 'console'
)
