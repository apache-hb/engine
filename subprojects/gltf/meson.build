project('gltf', 'cpp')

stb = subproject('stb').get_variable('stb')

args = [ '-DTINYGLTF_USE_CPP14=1' ]

libgltf = library('gltf', 'gltf/tinygltf.cpp',
    include_directories : '.',
    cpp_args : args,
    dependencies : stb
)

gltf = declare_dependency(
    link_with : libgltf,
    include_directories : '.',
    compile_args : args,
    dependencies : stb
)
