project('fastgltf', 'cpp',
    default_options : [
        'warning_level=0',
        'werror=false'
    ]
)

json = subproject('simdjson').get_variable('json')

src = [ 'src/base64_decode.cpp', 'src/fastgltf.cpp' ]

libgltf = library('fastgltf', src,
    dependencies : json,
    include_directories : [ 'src', 'include/fastgltf' ]
)

gltf = declare_dependency(
    link_with : libgltf,
    dependencies : json,
    include_directories : 'include'
)
