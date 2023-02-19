project('simdjson', 'cpp',
    default_options : [
        'cpp_eh=none'
    ]
)

args = [
    '-DSIMDJSON_DISABLE_DEPRECATED_API=1',
    '-D_HAS_EXCEPTIONS=0',
    '-DSIMDJSON_EXCEPTIONS=0',
    '/wd4127'
]

libjson = library('json', 'simdjson/simdjson.cpp',
    cpp_args : args,
    include_directories : 'simdjson'
)

json = declare_dependency(
    link_with : libjson, 
    include_directories : 'simdjson'
)
