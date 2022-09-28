project('fastgltf', 'cpp',
    default_options : [
        'cpp_std=c++17'
    ]
)

json = subproject('simdjson').get_variable('json')

src = [ 
    'fastgltf/src/fastgltf.cpp',
    'fastgltf/src/base64_decode.cpp'
]

libgltf = library('fastgltf', src,
    include_directories : 'fastgltf/src',
    dependencies : json
)

gltf = declare_dependency(
    link_with : libgltf,
    dependencies : json,
    include_directories : 'fastgltf/src'
)
