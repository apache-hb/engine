project('simdjson', 'cpp')

args = [
    '-DSIMDJSON_DISABLE_DEPRECATED_API=1'
]

libjson = library('json', 'simdjson/src/simdjson.cpp',
    cpp_args : args,
    include_directories : 'simdjson/include'
)

json = declare_dependency(
    link_with : libjson,
    compile_args : args,
    include_directories : 'simdjson/include'
)
