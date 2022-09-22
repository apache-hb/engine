project('imgui', 'cpp')

src = [
    'imgui/imgui_demo.cpp',
    'imgui/imgui_draw.cpp',
    'imgui/imgui_tables.cpp',
    'imgui/imgui_widgets.cpp',
    'imgui/imgui.cpp',
    'imgui/backends/imgui_impl_dx12.cpp',
    'imgui/backends/imgui_impl_glfw.cpp'
]

inc = [ '.', 'imgui', 'imgui/backends' ]

glfw = dependency('glfw3', fallback : [ 'glfw', 'glfw' ])

libimgui = library('imgui', src,
    include_directories : inc,
    dependencies : [ glfw ]
)

imgui = declare_dependency(
    link_with : libimgui,
    include_directories : inc,
    dependencies : [ glfw ]
)
