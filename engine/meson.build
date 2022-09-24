src = [ 
    'src/base/panic.cpp', 
    'src/base/io.cpp', 
    'src/base/logging.cpp', 
    'src/base/util.cpp',
    'src/base/window.cpp'
]

libbase = library('base', src,
    cpp_args : args,
    dependencies : [ fmt, glfw ],
    include_directories : 'include'
)

base = declare_dependency(
    link_with : libbase,
    dependencies : [ fmt, glfw ],
    include_directories : 'include'
)

# d3d12 rhi
src = [ 'src/rhi/d3d12/agility.cpp', 'src/rhi/d3d12/rhi.cpp', 'src/rhi/vendor.cpp' ]

librhi = library('d3d12-rhi', src,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ base, d3d12 ],
    include_directories : 'include'
)

rhi = declare_dependency(
    link_with : librhi,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ base, d3d12 ],
    include_directories : 'include'
)

src = [ 'src/render/render.cpp' ]

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
