src = [ 
    # base
    'src/base/panic.cpp', 
    'src/base/io/io.cpp', 
    'src/base/logging.cpp', 
    'src/base/util.cpp',
    'src/base/win32.cpp',
    'src/base/mutex.cpp',

    # memory
    'src/base/memory/bitmap.cpp',

    # input
    'src/base/input/input.cpp', 
    'src/base/input/gamepad.cpp', 
    'src/base/input/desktop.cpp',

    # locale
    'src/base/locale/locale.cpp'
]

libbase = library('base', src,
    cpp_args : args,
    link_args : [ 'dbghelp.lib', 'xinput.lib' ],
    include_directories : 'include'
)

base = declare_dependency(
    link_with : libbase,
    link_args : [ 'dbghelp.lib', 'xinput.lib' ],
    include_directories : 'include'
)

subdir('vendor')

src = [ 'src/window/window.cpp' ]

libwindow = library('window', src,
    cpp_args : args,
    dependencies : [ base, imgui ],
    include_directories : 'include'
)

window = declare_dependency(
    link_with : libwindow,
    dependencies : [ base ],
    include_directories : 'include'
)

src = [ 'src/ui/ui.cpp' ]

libui = library('ui', src,
    cpp_args : args,
    dependencies : [ window, imgui ],
    include_directories : 'include'
)

ui = declare_dependency(
    link_with : libui,
    dependencies : [ window, imgui ],
    include_directories : 'include'
)

# d3d12 rhi
src = [
    'src/rhi/vendor/agility.cpp', 
    'src/rhi/vendor/vendor.cpp',

    'src/rhi/objects/common.cpp',
    'src/rhi/objects/allocator.cpp',
    'src/rhi/objects/fence.cpp',
    'src/rhi/objects/buffer.cpp',
    'src/rhi/objects/descriptors.cpp',
    'src/rhi/objects/swapchain.cpp',
    'src/rhi/objects/commands.cpp',
    'src/rhi/objects/pipeline.cpp',
    'src/rhi/objects/queue.cpp',
    'src/rhi/objects/device.cpp',

    'src/rhi/rhi.cpp'
]

librhi = library('d3d12-rhi', src,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ base, d3d12, imgui, window ],
    include_directories : [ 'include', 'src/rhi' ]
)

rhi = declare_dependency(
    link_with : librhi,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ base, d3d12, imgui, window ],
    include_directories : 'include'
)

src = [
    'src/assets/gltf.cpp'
]

libassets = library('assets', src,
    cpp_args : args,
    dependencies : [ base, rhi, gltf ],
    include_directories : 'include'
)

assets = declare_dependency(
    link_with : libassets,
    dependencies : [ base, gltf ],
    include_directories : 'include'
)

src = [
    'src/render/queue.cpp',
    'src/render/render.cpp',
    'src/render/graph.cpp',
    'src/render/scene.cpp',
    'src/render/async.cpp',

    'src/render/resources/resources.cpp',

    'src/render/passes/present.cpp',
    'src/render/passes/imgui.cpp',
    'src/render/passes/global.cpp'
]

librender = library('render', src,
    cpp_args : args,
    dependencies : [ base, rhi, window, gltf, widgets, assets, queue ],
    include_directories : 'include'
)

render = declare_dependency(
    link_with : librender,
    dependencies : [ base, rhi, window, queue ],
    include_directories : 'include'
)

src = [
    'src/audio/audio.cpp'
]

libaudio = library('audio', src,
    cpp_args : args,
    dependencies : [ base ],
    include_directories : 'include'
)

audio = declare_dependency(
    link_with : libaudio,
    dependencies : [ base ],
    include_directories : 'include'
)
