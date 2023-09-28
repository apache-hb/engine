project('engine', [ 'cpp', 'c' ],
    license : 'GPLv3',
    version : '0.1.0',
    default_options : [
        'cpp_std=c++20',
        'cpp_rtti=false',
        'cpp_eh=none',
        'b_ndebug=if-release',
        'warning_level=3',
        'werror=true',
        'b_vscrt=static_from_buildtype',
        'default_library=static',
        'b_lundef=true',
        'auto_features=enabled'
    ]
)

d3d12 = subproject('d3d12').get_variable('d3d12')
gdk = subproject('gdk').get_variable('gdk')
queue = subproject('atomic-queue').get_variable('queue')

render_debug = get_option('render-debug').enabled()

args = [
    '-D_HAS_EXCEPTIONS=0', # msvc stl requires this
    '/wd4324', # warnings about padding
    '/wd4127' # warnings about if constexpr
]

src = [
    # core
    'engine/src/core/system.cpp',
    'engine/src/core/logging.cpp',
    'engine/src/core/panic.cpp',
    'engine/src/core/util.cpp',
    'engine/src/core/io.cpp',
    'engine/src/core/units.cpp',
    'engine/src/core/simcoe.cpp',

    # input
    'engine/src/input/input.cpp',
    'engine/src/input/desktop.cpp',
    'engine/src/input/gamepad.cpp',

    # memory
    'engine/src/memory/bitmap.cpp',

    # render
    'engine/src/render/context.cpp',
    'engine/src/render/heap.cpp',
    'engine/src/render/graph.cpp',
    'engine/src/render/queue.cpp',

    # rhi
    'engine/src/rhi/rhi.cpp',

    ###
    ### vendor code
    ###

    # gdk
    'vendor/src/microsoft/gdk.cpp',

    # agility
    'vendor/src/microsoft/agility.cpp',

    # gpu vendor code
    'vendor/src/gpu/gpu.cpp'
]

links = [
    # core
    'dbghelp.lib',

    # input
    'xinput.lib',

    'd3d12.lib',
    'dxgi.lib',
    'dxguid.lib'
]

deps = [
    # render
    d3d12, queue,

    ###
    ### vendor deps
    ###

    # gdk
    gdk
]

libengine = library('engine', src,
    include_directories : [ 'engine/include', 'vendor/include' ],
    dependencies : deps,
    cpp_args : args,
    link_args : links
)

engine = declare_dependency(
    include_directories : [ 'engine/include', 'vendor/include' ],
    link_with : [ libengine ],
    dependencies :deps,
    link_args : links
)

###
### d3d rhi
###

src = [ 'engine/src/rhi/dx/rhi.cpp' ]

links = [
    'd3d12.lib',
    'dxgi.lib',
    'dxguid.lib'
]

librhid3d = shared_library('rhi-d3d12', src,
    include_directories : [ 'engine/include', 'vendor/include' ],
    dependencies : [ engine ],
    cpp_args : args,
    link_args : links
)

###
### vk rhi
###

src = [ 'engine/src/rhi/vk/rhi.cpp' ]

links = [

]

librhivk = shared_library('rhi-vk', src,
    include_directories : [ 'engine/include', 'vendor/include' ],
    dependencies : [ engine ],
    cpp_args : args,
    link_args : links
)

subdir('data')

subdir('game')
