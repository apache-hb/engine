src = [ 'src/base/panic.cpp', 'src/base/io.cpp', 'src/base/logging.cpp' ]

libbase = library('base', src,
    cpp_args : args,
    dependencies : [ fmt ],
    include_directories : 'include'
)

base = declare_dependency(
    link_with : libbase,
    dependencies : [ fmt ],
    include_directories : 'include'
)

src = [ 'src/render/agility.cpp', 'src/render/render.cpp', 'src/render/window.cpp' ]

librender = library('render', src,
    cpp_args : args,
    link_args : [ 'd3d12.lib', 'dxgi.lib' ],
    dependencies : [ base, glfw, d3d12 ],
    include_directories : 'include'
)

render = declare_dependency(
    link_with : librender,
    link_args : [ 'd3d12.lib', 'dxgi.lib' ],
    dependencies : [ base, glfw, d3d12 ],
    include_directories : 'include'
)
