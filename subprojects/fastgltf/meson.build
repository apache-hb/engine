project('fastgltf', 'cpp',
    default_options : [
        'cpp_eh=none'
    ]
)

json = subproject('simdjson').get_variable('json')

src = [ 
    'fastgltf/base64_decode.cpp',
    'fastgltf/fastgltf.cpp'
]

args = [
    '-D_HAS_EXCEPTIONS=0'
]

libgltf = library('fastgltf', src,
    cpp_args : args,
    include_directories : 'fastgltf',
    dependencies : [ json ]
)

gltf = declare_dependency(
    link_with : libgltf,
    include_directories : '.',
    dependencies : [ json ]
)
