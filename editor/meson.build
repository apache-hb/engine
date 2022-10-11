src = [ 'src/widgets/nodes.cpp', 'src/widgets/nodes-ez.cpp' ]

libwidgets = library('widgets', src,
    dependencies : [ imgui ],
    include_directories : 'include',
    cpp_args : args
)

widgets = declare_dependency(
    link_with : libwidgets,
    dependencies : [ imgui ],
    include_directories : 'include'
)

executable('editor', 'main.cpp',
    include_directories : 'include',
    dependencies : [ input, render, ui, imgui ],
    cpp_args : args,
    win_subsystem : 'console'
)
