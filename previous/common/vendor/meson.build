src = [
    'imgui/imgui_demo.cpp',
    'imgui/imgui_draw.cpp',
    'imgui/imgui_tables.cpp',
    'imgui/imgui_widgets.cpp',
    'imgui/imgui.cpp',
    'imgui/backends/imgui_impl_dx12.cpp',
    'imgui/backends/imgui_impl_win32.cpp'
]

inc = [ '.', 'imgui', 'imgui/backends' ]

libimgui = library('imgui', src,
    dependencies : [ base ],
    include_directories : inc
)

imgui = declare_dependency(
    link_with : libimgui,
    dependencies : [ base ],
    include_directories : inc
)

src = [ 'nodes/src/imnodes.cpp' ]

libwidgets = library('widgets', src,
    dependencies : [ imgui ],
    include_directories : 'nodes/include',
    cpp_args : args
)

widgets = declare_dependency(
    link_with : libwidgets,
    dependencies : [ imgui ],
    include_directories : 'nodes/include'
)
