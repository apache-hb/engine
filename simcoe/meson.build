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
