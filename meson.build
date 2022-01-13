project('engine', [ 'c', 'cpp' ],
    license : 'GPLv3',
    version : '0.0.1',
    default_options : [
        'cpp_std=c++latest',
        'cpp_rtti=false',
        'warning_level=3',
        'werror=true',
        'b_vscrt=static_from_buildtype', # needs to be `md` for the dlss sdk to link
        'default_library=static',
        'b_lto=true',
        'b_lundef=true'
    ]
)

src = [ 
    # render utility objects
    'src/render/util.cpp',

    # core render objects
    'src/render/context/context.cpp',
    'src/render/context/lifetime.cpp',
    'src/render/context/access.cpp',
    #'src/render/context/upload.cpp',
    #'src/render/context/sync.cpp',

    #'src/render/camera/camera.cpp',

    #'src/render/viewport/viewport.cpp',

    # scene rendering
    #'src/render/scene/3dscene.cpp',

    # d3d wrapper objects
    'src/render/d3d12/debug.cpp',
    'src/render/d3d12/factory.cpp',
    'src/render/d3d12/device.cpp',
    'src/render/d3d12/library.cpp',
    #'src/render/objects/signature.cpp',
    #'src/render/objects/heap.cpp',

    'src/util/strings.cpp',
    'src/util/units.cpp',
    'src/util/timer.cpp',
    'src/util/win32.cpp',

    'src/assets/texture.cpp',
    'src/assets/loader/gltf.cpp',
    'src/assets/loader/debug.cpp',

    'src/system/window.cpp',
    'src/system/system.cpp',
    'src/system/gamesdk.cpp',

    'src/logging/log.cpp',
    'src/logging/debug.cpp',

    'src/input/inputx.cpp',
    'src/input/mnk.cpp',

    'src/main.cpp'
]


# python = find_program('python')
# dxc = generator(python,
#     arguments : [ '@INPUT@', '@OUTPUT@', 'ps,vs', '6_0' ],
#     output : [ '@BASENAME@.ps.pso', '@BASENAME@.vs.pso' ]
# )

args = [ '-DUNICODE=1', '/wd4324' ]

imgui = subproject('imgui').get_variable('imgui')
widgets = subproject('imgui-widgets').get_variable('widgets')
queue = subproject('atomic-queue').get_variable('queue')
stb = subproject('stb').get_variable('stb')
d3d12 = subproject('d3d12').get_variable('d3d12')
gltf = subproject('gltf').get_variable('gltf')
obj = subproject('obj').get_variable('obj')
gamesdk = subproject('discord').get_variable('gamesdk')

deps = [ imgui, widgets, queue, stb, d3d12, gltf, obj, gamesdk ]

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
