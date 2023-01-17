src = [
    'src/core/logging.cpp',
    'src/core/window.cpp'
]

libcore = library('core', src,
    include_directories : 'include',
    cpp_args : args
)

core = declare_dependency(
    include_directories : 'include',
    link_with : libcore
)
