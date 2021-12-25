project('engine', [ 'c', 'cpp' ],
    default_options : [
        'cpp_std=c++latest',
        'warning_level=3',
        'werror=true',
        'b_vscrt=static_from_buildtype', # needs to be `md` for dlss to work
        'default_library=static'
    ]
)

src = [ 
    # render utility objects
    'src/render/util.cpp',
    'src/render/factory.cpp',

    'src/render/debug/debug.cpp',

    # core render objects
    'src/render/render.cpp',
    'src/render/upload.cpp',
    'src/render/scene.cpp',

    # scene rendering
    'src/render/scene/scene.cpp',

    # d3d wrapper objects
    'src/render/objects/library.cpp',
    'src/render/objects/compute.cpp',
    'src/render/objects/heap.cpp',
    'src/render/objects/bundle.cpp',

    'src/util/strings.cpp',
    'src/util/units.cpp',
    'src/util/timer.cpp',
    'src/util/win32.cpp',

    'src/assets/gltf.cpp',

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

imgui = subproject('imgui').get_variable('imgui')
tinyobj = subproject('tinyobj').get_variable('tinyobj')
queue = subproject('atomic-queue').get_variable('queue')
stb = subproject('stb').get_variable('stb')
d3d12 = subproject('d3d12').get_variable('d3d12')
gltf = subproject('gltf').get_variable('gltf')

deps = [ imgui, queue, stb, d3d12, tinyobj, gltf ]

links = [ 
    'd3d12.lib', 
    'dxgi.lib', 
    'dxguid.lib', 
    'd3dcompiler.lib',
    'xinput.lib'
]

agility = get_option('agility-sdk').enabled()
dlss = get_option('dlss-sdk').enabled()
debugd3d12 = get_option('debug-d3d12').enabled()

message('agility-sdk: @0@'.format(agility))
message('dlss-sdk: @0@'.format(dlss))
message('debug-d3d12: @0@'.format(debugd3d12))

if agility
    src += [ 'src/render/agility/agility.cpp' ]
    args += [ '-DENABLE_AGILITY=1' ]
else
    args += [ '-DENABLE_AGILITY=0' ]
endif

if dlss
    ndk = subproject('ndk').get_variable('ndk')
    deps += [ ndk ]
    src += [ 'src/render/dlss/dlss.cpp' ]
    args += [ '-DENABLE_DLSS=1' ]
else
    args += [ '-DENABLE_DLSS=0' ]
endif

if debugd3d12
    args += [ '-DD3D12_DEBUG=1' ]
else
    args += [ '-DD3D12_DEBUG=0' ]
endif

executable('engine', src,
    include_directories : 'src',
    dependencies : deps,
    link_args : links,
    cpp_args : args,
    win_subsystem : 'console'
)
