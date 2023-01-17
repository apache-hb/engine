
dxc = subproject('dxc').get_variable('dxc')

src = [ 'src/shaders/shaders.cpp' ]

libshaders = library('shaders', src,
    dependencies : [ base, dxc ],
    include_directories : 'include',
    cpp_args : args
)

shaders = declare_dependency(
    include_directories : 'include',
    link_with : libshaders
)

# executable('editor', 'main.cpp',
#     include_directories : 'include',
#     dependencies : [ input, render, ui, imgui, shaders ],
#     cpp_args : args,
#     win_subsystem : 'console'
# )
