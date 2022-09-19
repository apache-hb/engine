libbase = library('base', 'src/base/io.cpp',
    cpp_args : args,
    include_directories : 'include'
)

base = declare_dependency(
    link_with : libbase,
    include_directories : 'include'
)

liblogging = library('logging', 'src/logging/logging.cpp',
    cpp_args : args,
    dependencies : [ fmt, base ],
    include_directories : 'include'
)

logging = declare_dependency(
    link_with : liblogging,
    dependencies : [ fmt, base ],
    include_directories : 'include'
)

src = [ 'src/render/agility.cpp', 'src/render/render.cpp']

librender = library('render', src,
    cpp_args : args,
    dependencies : [ d3d12 ],
    include_directories : 'include'
)

render = declare_dependency(
    link_with : librender,
    dependencies : [ d3d12 ],
    include_directories : 'include'
)
