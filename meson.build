project('engine', [ 'c', 'cpp' ],
    license : 'GPLv3',
    version : '0.0.1',
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

    'src/render/debug/debug.cpp',

    # core render objects
    'src/render/context/context.cpp',
    'src/render/context/lifetime.cpp',
    'src/render/context/access.cpp',
    #'src/render/context/upload.cpp',

    # scene rendering
    #'src/render/scene/scene.cpp',

    # d3d wrapper objects
    'src/render/objects/factory.cpp',
    #'src/render/objects/device.cpp',
    #'src/render/objects/library.cpp',
    #'src/render/objects/compute.cpp',
    'src/render/objects/heap.cpp',
    #'src/render/objects/commands.cpp',
    #'src/render/objects/queue.cpp',
    #'src/render/objects/root.cpp',
    #'src/render/objects/pipeline.cpp',

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

args = [ '-DUNICODE=1', '/wd4324' ]

imgui = subproject('imgui').get_variable('imgui')
queue = subproject('atomic-queue').get_variable('queue')
stb = subproject('stb').get_variable('stb')
d3d12 = subproject('d3d12').get_variable('d3d12')
gltf = subproject('gltf').get_variable('gltf')

deps = [ imgui, queue, stb, d3d12, gltf ]

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
