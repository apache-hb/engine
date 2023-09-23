src = [
    'src/core/system.cpp',
    'src/core/logging.cpp',
    'src/core/window.cpp',
    'src/core/panic.cpp',
    'src/core/util.cpp',
    'src/core/io.cpp',
    'src/core/units.cpp',

    'src/core/simcoe.cpp'
]

libcore = library('core', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'dbghelp.lib' ]
)

core = declare_dependency(
    include_directories : 'include',
    link_with : libcore,
    link_args : [ 'dbghelp.lib' ]
)

src = [
    'src/input/input.cpp',
    'src/input/desktop.cpp',
    'src/input/gamepad.cpp'
]

libinput = library('input', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'xinput.lib' ],
    dependencies : [ core ]
)

input = declare_dependency(
    include_directories : 'include',
    link_with : libinput,
    link_args : [ 'xinput.lib' ],
    dependencies : [ core ]
)

src = [
    'src/memory/bitmap.cpp'
]

libmemory = library('memory', src,
    include_directories : 'include',
    cpp_args : args,
    dependencies : [ core ]
)

memory = declare_dependency(
    include_directories : 'include',
    link_with : libmemory,
    dependencies : [ core ]
)

src = [
    'src/render/context.cpp',
    'src/render/heap.cpp',
    'src/render/graph.cpp',
    'src/render/queue.cpp',

    'src/render/vendor/vendor.cpp'
]

if agility
    message('enabling agility sdk')
    src += [ 'src/render/vendor/agility.cpp' ]
else
    message('agility sdk disabled')
endif

librender = library('render', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, memory, d3d12, queue ]
)

render = declare_dependency(
    include_directories : 'include',
    link_with : librender,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, memory, d3d12, queue ]
)

src = [
    'src/assets/gltf.cpp'
]

libassets = library('assets', src,
    include_directories : 'include',
    cpp_args : args,
    dependencies : [ core, memory, gltf, stb_image ]
)

assets = declare_dependency(
    include_directories : 'include',
    link_with : libassets,
    dependencies : [ core, memory ]
)
