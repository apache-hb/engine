project('glfw', 'c',
    default_options : [
        'werror=false',
        'warning_level=1',
        'c_std=c99'
    ]
)

src = [
    'glfw/src/context.c',
    'glfw/src/init.c',
    'glfw/src/input.c',
    'glfw/src/monitor.c',
    'glfw/src/vulkan.c',
    'glfw/src/window.c',
    'glfw/src/platform.c',

    'glfw/src/null_init.c',
    'glfw/src/null_joystick.c',
    'glfw/src/null_monitor.c',
    'glfw/src/null_window.c',

    # windows specific
    'glfw/src/win32_init.c',
    'glfw/src/win32_joystick.c',
    'glfw/src/win32_monitor.c',
    'glfw/src/win32_time.c',
    'glfw/src/win32_thread.c',
    'glfw/src/win32_window.c',
    'glfw/src/win32_module.c',
    'glfw/src/wgl_context.c',
    'glfw/src/egl_context.c',
    'glfw/src/osmesa_context.c'
]

args = [ 
    '-D_GLFW_WIN32=1',
    '-D_CRT_SECURE_NO_WARNINGS=1'
]

libglfw = library('glfw', src,
    c_args : args,
    include_directories : [ 'glfw/include' ]
)

glfw = declare_dependency(
    link_with : libglfw,
    include_directories : 'glfw/include'
)
