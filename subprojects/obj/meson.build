project('tinyobj', 'cpp')

src = [ 'tinyobj/tinyobj.cpp' ]

libobj = library('libobj', src, include_directories : 'tinyobj')

obj = declare_dependency(
    link_with : libobj,
    include_directories : '.'
)
