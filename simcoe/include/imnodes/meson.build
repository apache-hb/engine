src = [ 'src/imnodes.cpp' ]

libimnodes = library('imnodes', src,
    dependencies : [ imgui ],
    include_directories : 'include',
    cpp_args : args
)

imnodes = declare_dependency(
    link_with : libimnodes,
    dependencies : [ imgui ],
    include_directories : 'include'
)
