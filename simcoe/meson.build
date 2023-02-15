src = [
    'src/core/system.cpp',
    'src/core/logging.cpp',
    'src/core/window.cpp',
    'src/core/panic.cpp',
    'src/core/util.cpp',
    'src/core/io.cpp',

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
    'src/audio/audio.cpp'
]

libaudio = library('audio', src,
    include_directories : 'include',
    cpp_args : args,
    dependencies : [ core ]
)

audio = declare_dependency(
    include_directories : 'include',
    link_with : libaudio,
    dependencies : [ core ]
)

src = [ 
    'src/input/input.cpp',
    'src/input/desktop.cpp',
    'src/input/gamepad.cpp'
]

libinput = library('input', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'xinput.lib' ],
    dependencies : [ core ]
)

input = declare_dependency(
    include_directories : 'include',
    link_with : libinput,
    link_args : [ 'xinput.lib' ],
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

src = [ 'include/imnodes/src/imnodes.cpp' ]

libimnodes = library('imnodes', src,
    dependencies : [ imgui ],
    include_directories : 'include/imnodes/include',
    cpp_args : args
)

imnodes = declare_dependency(
    link_with : libimnodes,
    dependencies : [ imgui ],
    include_directories : 'include/imnodes/include'
)

src = [
    'src/render/context.cpp',
    'src/render/heap.cpp',
    'src/render/graph.cpp',
    'src/render/queue.cpp',
    
    'src/render/vendor/vendor.cpp'
]

if agility
    message('enabling agility sdk')
    src += [ 'src/render/vendor/agility.cpp' ]
else
    message('agility sdk disabled')
endif

librender = library('render', src,
    include_directories : 'include',
    cpp_args : args,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, memory, d3d12, queue ]
)

render = declare_dependency(
    include_directories : 'include',
    link_with : librender,
    link_args : [ 'd3d12.lib', 'dxgi.lib', 'dxguid.lib' ],
    dependencies : [ core, memory, d3d12, queue ]
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
