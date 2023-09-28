src = [
    'include/imgui/imgui_demo.cpp',
    'include/imgui/imgui_draw.cpp',
    'include/imgui/imgui_tables.cpp',
    'include/imgui/imgui_widgets.cpp',
    'include/imgui/imgui.cpp',
    'include/imgui/backends/imgui_impl_dx12.cpp',
    'include/imgui/backends/imgui_impl_win32.cpp'
]

inc = [ 'include', 'include/imgui', 'include/imgui/backends' ]

libimgui = library('imgui', src,
    dependencies : [ engine ],
    include_directories : inc
)

imgui = declare_dependency(
    link_with : libimgui,
    dependencies : [ engine ],
    include_directories : inc
)

src = [ 'src/imnodes/imnodes.cpp' ]

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

shaders = [
    hlsl.process('../data/shaders/blit.hlsl'),
    hlsl.process('../data/shaders/scene.hlsl'),
    hlsl.process('../data/shaders/cubemap.hlsl'),
]

src = [
    'src/game/game.cpp',
    'src/game/camera.cpp',
    'src/game/window.cpp',
    'src/game/render.cpp',
    'src/game/registry.cpp',

    'src/game/render/global.cpp',
    'src/game/render/imgui.cpp',
    'src/game/render/scene.cpp',
    'src/game/render/cubemap.cpp',
    'src/game/render/blit.cpp'
]

libgame = library('game', src + shaders,
    dependencies : [ engine, imnodes ],
    include_directories : 'include',
    cpp_args : args
)

game = declare_dependency(
    link_with : libgame,
    dependencies : [ engine, imnodes ],
    include_directories : 'include'
)

executable('game', 'main.cpp',
    dependencies : [ game ],
    cpp_args : args,
    include_directories : 'include',
    win_subsystem : 'console',
    export_dynamic : true
)
