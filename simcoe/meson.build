src = [
    'src/core/system.cpp',
    'src/core/logging.cpp',
    'src/core/window.cpp',
    'src/core/panic.cpp',
    'src/core/util.cpp',

    'src/core/simcoe.cpp'
]

libcore = library('core', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'dbghelp.lib' ]
)

core = declare_dependency(
    include_directories : 'include',
    link_with : libcore,
    link_args : [ 'dbghelp.lib' ]
)

src = [ 
    'src/input/input.cpp',
    'src/input/desktop.cpp',
    'src/input/gamepad.cpp'
]

libinput = library('input', src,
    include_directories : 'include',
    cpp_args : args,
    dependencies : [ core ]
)

input = declare_dependency(
    include_directories : 'include',
    link_with : libinput,
    dependencies : [ core ]
)

src = [
    'src/memory/bitmap.cpp'
]

libmemory = library('memory', src,
    include_directories : 'include',
    cpp_args : args,
    dependencies : [ core ]
)

memory = declare_dependency(
    include_directories : 'include',
    link_with : libmemory,
    dependencies : [ core ]
)

src = [
    'include/imgui/imgui_demo.cpp',
    'include/imgui/imgui_draw.cpp',
    'include/imgui/imgui_tables.cpp',
    'include/imgui/imgui_widgets.cpp',
    'include/imgui/imgui.cpp',
    'include/imgui/backends/imgui_impl_dx12.cpp',
    'include/imgui/backends/imgui_impl_win32.cpp'
]

inc = [ 'include/imgui', 'include/imgui/backends' ]

libimgui = library('imgui', src,
    dependencies : [ core ],
    include_directories : inc
)

imgui = declare_dependency(
    link_with : libimgui,
    dependencies : [ core ],
    include_directories : inc
)

src = [
    'src/render/context.cpp',
    'src/render/heap.cpp',
    'src/render/vendor/agility.cpp',
    'src/render/vendor/vendor.cpp'
]

librender = library('render', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, memory, d3d12 ]
)

render = declare_dependency(
    include_directories : 'include',
    link_with : librender,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, d3d12 ]
)

src = [
    'src/locale/locale.cpp'
]

liblocale = library('locale', src,
    include_directories : 'include',
    cpp_args : args,
    dependencies : [ core, input ]
)

locale = declare_dependency(
    include_directories : 'include',
    link_with : liblocale,
    dependencies : [ core, input ]
)
