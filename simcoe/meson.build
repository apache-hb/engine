src = [
    'src/core/system.cpp',
    'src/core/logging.cpp',
    'src/core/window.cpp',
    'src/core/panic.cpp'
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
    'src/render/context.cpp'
]

librender = library('render', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, d3d12 ]
)

render = declare_dependency(
    include_directories : 'include',
    link_with : librender,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, d3d12 ]
)