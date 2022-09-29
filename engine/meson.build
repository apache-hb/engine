src = [ 
    'src/base/panic.cpp', 
    'src/base/io.cpp', 
    'src/base/logging.cpp', 
    'src/base/util.cpp',
    'src/base/window.cpp'
]

libbase = library('base', src,
    cpp_args : args,
    dependencies : [ fmt, imgui ],
    include_directories : 'include'
)

base = declare_dependency(
    link_with : libbase,
    dependencies : [ fmt ],
    include_directories : 'include'
)

src = [ 'src/input/input.cpp' ]

libinput = library('input', src,
    cpp_args : args,
    link_args : [ 'xinput.lib' ],
    dependencies : base,
    include_directories : 'include'
)

input = declare_dependency(
    link_with : libinput,
    link_args : [ 'xinput.lib' ],
    dependencies : base,
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
    dependencies : [ base, d3d12, imgui ],
    include_directories : [ 'include', 'src/rhi' ]
)

rhi = declare_dependency(
    link_with : librhi,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ base, d3d12, imgui ],
    include_directories : 'include'
)

src = [ 'src/render/render.cpp', 'src/render/camera.cpp' ]

librender = library('render', src,
    cpp_args : args,
    dependencies : [ base, rhi ],
    include_directories : 'include'
)

render = declare_dependency(
    link_with : librender,
    dependencies : [ base, rhi ],
    include_directories : 'include'
)
