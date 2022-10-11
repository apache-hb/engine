project('tinygltf', 'cpp')

src = [ 'tinygltf/tinygltf.cpp' ]

stb = subproject('stb')

image = stb.get_variable('image')
image_write = stb.get_variable('image_write')
json = subproject('rapidjson').get_variable('json')

args = [
    '-DTINYGLTF_USE_RAPIDJSON=1',
    '/Wv:18',
    '-D_HAS_EXCEPTIONS=0'
]

libgltf = library('tinygltf', src,
    include_directories : 'tinygltf',
    cpp_args : args,
    dependencies : [ image, image_write, json ]
)

gltf = declare_dependency(
    link_with : libgltf,
    compile_args : args,
    include_directories : '.',
    dependencies : [ image, image_write, json ]
)
